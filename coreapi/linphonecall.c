
/*
linphone
Copyright (C) 2010  Belledonne Communications SARL
 (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifdef WIN32
#include <time.h>
#endif
#include "linphonecore.h"
#include "sipsetup.h"
#include "lpconfig.h"
#include "private.h"
#include <ortp/event.h>
#include <ortp/b64.h>


#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/msvolume.h"
#include "mediastreamer2/msequalizer.h"
#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/msjpegwriter.h"
#include "mediastreamer2/mseventqueue.h"
#include "mediastreamer2/mssndcard.h"

#ifdef VIDEO_ENABLED
static MSWebCam *get_nowebcam_device(){
	return ms_web_cam_manager_get_cam(ms_web_cam_manager_get(),"StaticImage: Static picture");
}
#endif

static bool_t generate_b64_crypto_key(int key_length, char* key_out) {
	int b64_size;
	uint8_t* tmp = (uint8_t*) malloc(key_length);			
	if (ortp_crypto_get_random(tmp, key_length)!=0) {
		ms_error("Failed to generate random key");
		free(tmp);
		return FALSE;
	}
	
	b64_size = b64_encode((const char*)tmp, key_length, NULL, 0);
	if (b64_size == 0) {
		ms_error("Failed to b64 encode key");
		free(tmp);
		return FALSE;
	}
	key_out[b64_size] = '\0';
	b64_encode((const char*)tmp, key_length, key_out, 40);
	free(tmp);
	return TRUE;
}

LinphoneCore *linphone_call_get_core(const LinphoneCall *call){
	return call->core;
}

const char* linphone_call_get_authentication_token(LinphoneCall *call){
	return call->auth_token;
}

bool_t linphone_call_get_authentication_token_verified(LinphoneCall *call){
	return call->auth_token_verified;
}

static bool_t linphone_call_are_all_streams_encrypted(LinphoneCall *call) {
	// Check ZRTP encryption in audiostream
	if (!call->audiostream_encrypted) {
		return FALSE;
	}

#ifdef VIDEO_ENABLED
	// If video enabled, check ZRTP encryption in videostream
	const LinphoneCallParams *params=linphone_call_get_current_params(call);
	if (params->has_video && !call->videostream_encrypted) {
		return FALSE;
	}
#endif

	return TRUE;
}

void propagate_encryption_changed(LinphoneCall *call){
	LinphoneCore *lc=call->core;
	if (!linphone_call_are_all_streams_encrypted(call)) {
		ms_message("Some streams are not encrypted");
		call->current_params.media_encryption=LinphoneMediaEncryptionNone;
		if (lc->vtable.call_encryption_changed)
			lc->vtable.call_encryption_changed(call->core, call, FALSE, call->auth_token);
	} else {
		ms_message("All streams are encrypted");
		call->current_params.media_encryption=LinphoneMediaEncryptionZRTP;
		if (lc->vtable.call_encryption_changed)
			lc->vtable.call_encryption_changed(call->core, call, TRUE, call->auth_token);
	}
}

#ifdef VIDEO_ENABLED
static void linphone_call_videostream_encryption_changed(void *data, bool_t encrypted){
	ms_message("Video stream is %s", encrypted ? "encrypted" : "not encrypted");

	LinphoneCall *call = (LinphoneCall *)data;
	call->videostream_encrypted=encrypted;
	propagate_encryption_changed(call);
}
#endif

static void linphone_call_audiostream_encryption_changed(void *data, bool_t encrypted) {
	char status[255]={0};
	ms_message("Audio stream is %s ", encrypted ? "encrypted" : "not encrypted");

	LinphoneCall *call = (LinphoneCall *)data;
	call->audiostream_encrypted=encrypted;
	
	if (encrypted && call->core->vtable.display_status != NULL) {
		snprintf(status,sizeof(status)-1,_("Authentication token is %s"),call->auth_token);
		 call->core->vtable.display_status(call->core, status);
	}

	propagate_encryption_changed(call);


#ifdef VIDEO_ENABLED
	// Enable video encryption
	const LinphoneCallParams *params=linphone_call_get_current_params(call);
	if (params->has_video) {
		ms_message("Trying to enable encryption on video stream");
		OrtpZrtpParams params;
		params.zid_file=NULL; //unused
		video_stream_enable_zrtp(call->videostream,call->audiostream,&params);
	}
#endif
}


static void linphone_call_audiostream_auth_token_ready(void *data, const char* auth_token, bool_t verified) {
	LinphoneCall *call=(LinphoneCall *)data;
	if (call->auth_token != NULL)
		ms_free(call->auth_token);

	call->auth_token=ms_strdup(auth_token);
	call->auth_token_verified=verified;

	ms_message("Authentication token is %s (%s)", auth_token, verified?"verified":"unverified");
}

void linphone_call_set_authentication_token_verified(LinphoneCall *call, bool_t verified){
	if (call->audiostream==NULL){
		ms_error("linphone_call_set_authentication_token_verified(): No audio stream");
	}
	if (call->audiostream->ortpZrtpContext==NULL){
		ms_error("linphone_call_set_authentication_token_verified(): No zrtp context.");
	}
	if (!call->auth_token_verified && verified){
		ortp_zrtp_sas_verified(call->audiostream->ortpZrtpContext);
	}else if (call->auth_token_verified && !verified){
		ortp_zrtp_sas_reset_verified(call->audiostream->ortpZrtpContext);
	}
	call->auth_token_verified=verified;
	propagate_encryption_changed(call);
}

static MSList *make_codec_list(LinphoneCore *lc, const MSList *codecs, int bandwidth_limit,int* max_sample_rate){
	MSList *l=NULL;
	const MSList *it;
	if (max_sample_rate) *max_sample_rate=0;
	for(it=codecs;it!=NULL;it=it->next){
		PayloadType *pt=(PayloadType*)it->data;
		if (pt->flags & PAYLOAD_TYPE_ENABLED){
			if (bandwidth_limit>0 && !linphone_core_is_payload_type_usable_for_bandwidth(lc,pt,bandwidth_limit)){
				ms_message("Codec %s/%i eliminated because of audio bandwidth constraint.",pt->mime_type,pt->clock_rate);
				continue;
			}
			if (linphone_core_check_payload_type_usability(lc,pt)){
				l=ms_list_append(l,payload_type_clone(pt));
				if (max_sample_rate && payload_type_get_rate(pt)>*max_sample_rate) *max_sample_rate=payload_type_get_rate(pt);
			}
		}
	}
	return l;
}

static SalMediaDescription *_create_local_media_description(LinphoneCore *lc, LinphoneCall *call, unsigned int session_id, unsigned int session_ver){
	MSList *l;
	PayloadType *pt;
	int i;
	const char *me=linphone_core_get_identity(lc);
	LinphoneAddress *addr=linphone_address_new(me);
	const char *username=linphone_address_get_username (addr);
	SalMediaDescription *md=sal_media_description_new();


	md->session_id=session_id;
	md->session_ver=session_ver;
	md->nstreams=1;
	strncpy(md->addr,call->localip,sizeof(md->addr));
	strncpy(md->username,username,sizeof(md->username));
	md->bandwidth=linphone_core_get_download_bandwidth(lc);

	/*set audio capabilities */
	strncpy(md->streams[0].addr,call->localip,sizeof(md->streams[0].addr));
	md->streams[0].port=call->audio_port;
	md->streams[0].proto=(call->params.media_encryption == LinphoneMediaEncryptionSRTP) ? 
		SalProtoRtpSavp : SalProtoRtpAvp;
	md->streams[0].type=SalAudio;
	md->streams[0].ptime=lc->net_conf.down_ptime;
	l=make_codec_list(lc,lc->codecs_conf.audio_codecs,call->params.audio_bw,&md->streams[0].max_rate);
	pt=payload_type_clone(rtp_profile_get_payload_from_mime(&av_profile,"telephone-event"));
	l=ms_list_append(l,pt);
	md->streams[0].payloads=l;
	


	if (call->params.has_video){
		md->nstreams++;
		md->streams[1].port=call->video_port;
		md->streams[1].proto=md->streams[0].proto;
		md->streams[1].type=SalVideo;
		l=make_codec_list(lc,lc->codecs_conf.video_codecs,0,NULL);
		md->streams[1].payloads=l;
	}
	
	for(i=0; i<md->nstreams; i++) {
		if (md->streams[i].proto == SalProtoRtpSavp) {
			md->streams[i].crypto[0].tag = 1;
			md->streams[i].crypto[0].algo = AES_128_SHA1_80;
			if (!generate_b64_crypto_key(30, md->streams[i].crypto[0].master_key))
				md->streams[i].crypto[0].algo = 0;
			md->streams[i].crypto[1].tag = 2;
			md->streams[i].crypto[1].algo = AES_128_SHA1_32;
			if (!generate_b64_crypto_key(30, md->streams[i].crypto[1].master_key))
				md->streams[i].crypto[1].algo = 0;
			md->streams[i].crypto[2].algo = 0;
		}
	}
	
	linphone_address_destroy(addr);
	return md;
}

void update_local_media_description(LinphoneCore *lc, LinphoneCall *call){
	SalMediaDescription *md=call->localdesc;
	if (md== NULL) {
		call->localdesc = create_local_media_description(lc,call);
	} else {
		call->localdesc = _create_local_media_description(lc,call,md->session_id,md->session_ver+1);
		sal_media_description_unref(md);
	}
}

SalMediaDescription *create_local_media_description(LinphoneCore *lc, LinphoneCall *call){
	unsigned int id=rand() & 0xfff;
	return _create_local_media_description(lc,call,id,id);
}

static int find_port_offset(LinphoneCore *lc){
	int offset;
	MSList *elem;
	int audio_port;
	bool_t already_used=FALSE;
	for(offset=0;offset<100;offset+=2){
		audio_port=linphone_core_get_audio_port (lc)+offset;
		already_used=FALSE;
		for(elem=lc->calls;elem!=NULL;elem=elem->next){
			LinphoneCall *call=(LinphoneCall*)elem->data;
			if (call->audio_port==audio_port) {
				already_used=TRUE;
				break;
			}
		}
		if (!already_used) break;
	}
	if (offset==100){
		ms_error("Could not find any free port !");
		return -1;
	}
	return offset;
}

static void linphone_call_init_common(LinphoneCall *call, LinphoneAddress *from, LinphoneAddress *to){
	int port_offset;
	call->magic=linphone_call_magic;
	call->refcnt=1;
	call->state=LinphoneCallIdle;
	call->transfer_state = LinphoneCallIdle;
	call->start_time=time(NULL);
	call->media_start_time=0;
	call->log=linphone_call_log_new(call, from, to);
	call->owns_call_log=TRUE;
	linphone_core_notify_all_friends(call->core,LinphoneStatusOnThePhone);
	port_offset=find_port_offset (call->core);
	if (port_offset==-1) return;
	call->audio_port=linphone_core_get_audio_port(call->core)+port_offset;
	call->video_port=linphone_core_get_video_port(call->core)+port_offset;
	linphone_call_init_stats(&call->stats[LINPHONE_CALL_STATS_AUDIO], LINPHONE_CALL_STATS_AUDIO);
	linphone_call_init_stats(&call->stats[LINPHONE_CALL_STATS_VIDEO], LINPHONE_CALL_STATS_VIDEO);
}

void linphone_call_init_stats(LinphoneCallStats *stats, int type) {
	stats->type = type;
	stats->received_rtcp = NULL;
	stats->sent_rtcp = NULL;
}

static void discover_mtu(LinphoneCore *lc, const char *remote){
	int mtu;
	if (lc->net_conf.mtu==0	){
		/*attempt to discover mtu*/
		mtu=ms_discover_mtu(remote);
		if (mtu>0){
			ms_set_mtu(mtu);
			ms_message("Discovered mtu is %i, RTP payload max size is %i",
				mtu, ms_get_payload_max_size());
		}
	}
}

LinphoneCall * linphone_call_new_outgoing(struct _LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to, const LinphoneCallParams *params)
{
	LinphoneCall *call=ms_new0(LinphoneCall,1);
	call->dir=LinphoneCallOutgoing;
	call->op=sal_op_new(lc->sal);
	sal_op_set_user_pointer(call->op,call);
	call->core=lc;
	linphone_core_get_local_ip(lc,linphone_address_get_domain(to),call->localip);
	linphone_call_init_common(call,from,to);
	call->params=*params;
	call->localdesc=create_local_media_description (lc,call);
	call->camera_active=params->has_video;
	if (linphone_core_get_firewall_policy(call->core)==LinphonePolicyUseStun)
		linphone_core_run_stun_tests(call->core,call);
	discover_mtu(lc,linphone_address_get_domain (to));
	if (params->referer){
		sal_call_set_referer(call->op,params->referer->op);
		call->referer=linphone_call_ref(params->referer);
	}
	return call;
}

LinphoneCall * linphone_call_new_incoming(LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to, SalOp *op){
	LinphoneCall *call=ms_new0(LinphoneCall,1);
	char *from_str;

	call->dir=LinphoneCallIncoming;
	sal_op_set_user_pointer(op,call);
	call->op=op;
	call->core=lc;

	if (lc->sip_conf.ping_with_options){
		/*the following sends an option request back to the caller so that
		 we get a chance to discover our nat'd address before answering.*/
		call->ping_op=sal_op_new(lc->sal);
		from_str=linphone_address_as_string_uri_only(from);
		sal_op_set_route(call->ping_op,sal_op_get_network_origin(op));
		sal_op_set_user_pointer(call->ping_op,call);
		sal_ping(call->ping_op,linphone_core_find_best_identity(lc,from,NULL),from_str);
		ms_free(from_str);
	}

	linphone_address_clean(from);
	linphone_core_get_local_ip(lc,linphone_address_get_domain(from),call->localip);
	linphone_call_init_common(call, from, to);
	linphone_core_init_default_params(lc, &call->params);
	call->params.has_video &= !!lc->video_policy.automatically_accept;
	call->localdesc=create_local_media_description (lc,call);
	call->camera_active=call->params.has_video;
	if (linphone_core_get_firewall_policy(call->core)==LinphonePolicyUseStun)
		linphone_core_run_stun_tests(call->core,call);
	discover_mtu(lc,linphone_address_get_domain(from));
	return call;
}

/* this function is called internally to get rid of a call.
 It performs the following tasks:
 - remove the call from the internal list of calls
 - update the call logs accordingly
*/

static void linphone_call_set_terminated(LinphoneCall *call){
	LinphoneCore *lc=call->core;

	linphone_core_update_allocated_audio_bandwidth(lc);

	call->owns_call_log=FALSE;
	linphone_call_log_completed(call);


	if (call == lc->current_call){
		ms_message("Resetting the current call");
		lc->current_call=NULL;
	}

	if (linphone_core_del_call(lc,call) != 0){
		ms_error("Could not remove the call from the list !!!");
	}

	if (ms_list_size(lc->calls)==0)
		linphone_core_notify_all_friends(lc,lc->presence_mode);

	linphone_core_conference_check_uninit(lc);
	if (call->ringing_beep){
		linphone_core_stop_dtmf(lc);
		call->ringing_beep=FALSE;
	}
	if (call->referer){
		linphone_call_unref(call->referer);
		call->referer=NULL;
	}
}

void linphone_call_fix_call_parameters(LinphoneCall *call){
	call->params.has_video=call->current_params.has_video;
	call->params.media_encryption=call->current_params.media_encryption;
}

const char *linphone_call_state_to_string(LinphoneCallState cs){
	switch (cs){
		case LinphoneCallIdle:
			return "LinphoneCallIdle";
		case LinphoneCallIncomingReceived:
			return "LinphoneCallIncomingReceived";
		case LinphoneCallOutgoingInit:
			return "LinphoneCallOutgoingInit";
		case LinphoneCallOutgoingProgress:
			return "LinphoneCallOutgoingProgress";
		case LinphoneCallOutgoingRinging:
			return "LinphoneCallOutgoingRinging";
		case LinphoneCallOutgoingEarlyMedia:
			return "LinphoneCallOutgoingEarlyMedia";
		case LinphoneCallConnected:
			return "LinphoneCallConnected";
		case LinphoneCallStreamsRunning:
			return "LinphoneCallStreamsRunning";
		case LinphoneCallPausing:
			return "LinphoneCallPausing";
		case LinphoneCallPaused:
			return "LinphoneCallPaused";
		case LinphoneCallResuming:
			return "LinphoneCallResuming";
		case LinphoneCallRefered:
			return "LinphoneCallRefered";
		case LinphoneCallError:
			return "LinphoneCallError";
		case LinphoneCallEnd:
			return "LinphoneCallEnd";
		case LinphoneCallPausedByRemote:
			return "LinphoneCallPausedByRemote";
		case LinphoneCallUpdatedByRemote:
			return "LinphoneCallUpdatedByRemote";
		case LinphoneCallIncomingEarlyMedia:
			return "LinphoneCallIncomingEarlyMedia";
		case LinphoneCallUpdated:
			return "LinphoneCallUpdated";
		case LinphoneCallReleased:
			return "LinphoneCallReleased";
	}
	return "undefined state";
}

void linphone_call_set_state(LinphoneCall *call, LinphoneCallState cstate, const char *message){
	LinphoneCore *lc=call->core;

	if (call->state!=cstate){
		if (call->state==LinphoneCallEnd || call->state==LinphoneCallError){
			if (cstate!=LinphoneCallReleased){
				ms_warning("Spurious call state change from %s to %s, ignored.",linphone_call_state_to_string(call->state),
				   linphone_call_state_to_string(cstate));
				return;
			}
		}
		ms_message("Call %p: moving from state %s to %s",call,linphone_call_state_to_string(call->state),
		           linphone_call_state_to_string(cstate));
		if (cstate!=LinphoneCallRefered){
			/*LinphoneCallRefered is rather an event, not a state.
			 Indeed it does not change the state of the call (still paused or running)*/
			call->state=cstate;
		}
		if (cstate==LinphoneCallEnd || cstate==LinphoneCallError){
			switch(call->reason){
				case LinphoneReasonDeclined:
					call->log->status=LinphoneCallDeclined;
				break;
				case LinphoneReasonNotAnswered:
					call->log->status=LinphoneCallMissed;
				break;
				default:
				break;
			}
			linphone_call_set_terminated (call);
		}
		if (cstate == LinphoneCallConnected) {
			call->log->status=LinphoneCallSuccess;
			call->media_start_time=time(NULL);
		}

		if (lc->vtable.call_state_changed)
			lc->vtable.call_state_changed(lc,call,cstate,message);
		if (cstate==LinphoneCallReleased){
			if (call->op!=NULL) {
				/* so that we cannot have anymore upcalls for SAL
				 concerning this call*/
				sal_op_release(call->op);
				call->op=NULL;
			}
			linphone_call_unref(call);
		}
	}
}

static void linphone_call_destroy(LinphoneCall *obj)
{
	if (obj->op!=NULL) {
		sal_op_release(obj->op);
		obj->op=NULL;
	}
	if (obj->resultdesc!=NULL) {
		sal_media_description_unref(obj->resultdesc);
		obj->resultdesc=NULL;
	}
	if (obj->localdesc!=NULL) {
		sal_media_description_unref(obj->localdesc);
		obj->localdesc=NULL;
	}
	if (obj->ping_op) {
		sal_op_release(obj->ping_op);
	}
	if (obj->refer_to){
		ms_free(obj->refer_to);
	}
	if (obj->owns_call_log)
		linphone_call_log_destroy(obj->log);
	if (obj->auth_token) {
		ms_free(obj->auth_token);
	}

	ms_free(obj);
}

/**
 * @addtogroup call_control
 * @{
**/

/**
 * Increments the call 's reference count.
 * An application that wishes to retain a pointer to call object
 * must use this function to unsure the pointer remains
 * valid. Once the application no more needs this pointer,
 * it must call linphone_call_unref().
**/
LinphoneCall * linphone_call_ref(LinphoneCall *obj){
	obj->refcnt++;
	return obj;
}

/**
 * Decrements the call object reference count.
 * See linphone_call_ref().
**/
void linphone_call_unref(LinphoneCall *obj){
	obj->refcnt--;
	if (obj->refcnt==0){
		linphone_call_destroy(obj);
	}
}

/**
 * Returns current parameters associated to the call.
**/
const LinphoneCallParams * linphone_call_get_current_params(const LinphoneCall *call){
	return &call->current_params;
}

static bool_t is_video_active(const SalStreamDescription *sd){
	return sd->port!=0 && sd->dir!=SalStreamInactive;
}

/**
 * Returns call parameters proposed by remote.
 * 
 * This is useful when receiving an incoming call, to know whether the remote party
 * supports video, encryption or whatever.
**/
const LinphoneCallParams * linphone_call_get_remote_params(LinphoneCall *call){
	LinphoneCallParams *cp=&call->remote_params;
	memset(cp,0,sizeof(*cp));
	if (call->op){
		SalMediaDescription *md=sal_call_get_remote_media_description(call->op);
		if (md){
			SalStreamDescription *asd,*vsd,*secure_asd,*secure_vsd;

			asd=sal_media_description_find_stream(md,SalProtoRtpAvp,SalAudio);
			vsd=sal_media_description_find_stream(md,SalProtoRtpAvp,SalVideo);
			secure_asd=sal_media_description_find_stream(md,SalProtoRtpSavp,SalAudio);
			secure_vsd=sal_media_description_find_stream(md,SalProtoRtpSavp,SalVideo);
			if (secure_vsd){
				cp->has_video=is_video_active(secure_vsd);
				if (secure_asd || asd==NULL)
					cp->media_encryption=LinphoneMediaEncryptionSRTP;
			}else if (vsd){
				cp->has_video=is_video_active(vsd);
			}
			return cp;
		}
	}
	return NULL;
}

/**
 * Returns the remote address associated to this call
 *
**/
const LinphoneAddress * linphone_call_get_remote_address(const LinphoneCall *call){
	return call->dir==LinphoneCallIncoming ? call->log->from : call->log->to;
}

/**
 * Returns the remote address associated to this call as a string.
 *
 * The result string must be freed by user using ms_free().
**/
char *linphone_call_get_remote_address_as_string(const LinphoneCall *call){
	return linphone_address_as_string(linphone_call_get_remote_address(call));
}

/**
 * Retrieves the call's current state.
**/
LinphoneCallState linphone_call_get_state(const LinphoneCall *call){
	return call->state;
}

/**
 * Returns the reason for a call termination (either error or normal termination)
**/
LinphoneReason linphone_call_get_reason(const LinphoneCall *call){
	return call->reason;
}

/**
 * Get the user_pointer in the LinphoneCall
 *
 * @ingroup call_control
 *
 * return user_pointer an opaque user pointer that can be retrieved at any time
**/
void *linphone_call_get_user_pointer(LinphoneCall *call)
{
	return call->user_pointer;
}

/**
 * Set the user_pointer in the LinphoneCall
 *
 * @ingroup call_control
 *
 * the user_pointer is an opaque user pointer that can be retrieved at any time in the LinphoneCall
**/
void linphone_call_set_user_pointer(LinphoneCall *call, void *user_pointer)
{
	call->user_pointer = user_pointer;
}

/**
 * Returns the call log associated to this call.
**/
LinphoneCallLog *linphone_call_get_call_log(const LinphoneCall *call){
	return call->log;
}

/**
 * Returns the refer-to uri (if the call was transfered).
**/
const char *linphone_call_get_refer_to(const LinphoneCall *call){
	return call->refer_to;
}

/**
 * Returns direction of the call (incoming or outgoing).
**/
LinphoneCallDir linphone_call_get_dir(const LinphoneCall *call){
	return call->log->dir;
}

/**
 * Returns the far end's user agent description string, if available.
**/
const char *linphone_call_get_remote_user_agent(LinphoneCall *call){
	if (call->op){
		return sal_op_get_remote_ua (call->op);
	}
	return NULL;
}

/**
 * Returns true if this calls has received a transfer that has not been
 * executed yet.
 * Pending transfers are executed when this call is being paused or closed,
 * locally or by remote endpoint.
 * If the call is already paused while receiving the transfer request, the
 * transfer immediately occurs.
**/
bool_t linphone_call_has_transfer_pending(const LinphoneCall *call){
	return call->refer_pending;
}

/**
 * Returns call's duration in seconds.
**/
int linphone_call_get_duration(const LinphoneCall *call){
	if (call->media_start_time==0) return 0;
	return time(NULL)-call->media_start_time;
}

/**
 * Returns the call object this call is replacing, if any.
 * Call replacement can occur during call transfers.
 * By default, the core automatically terminates the replaced call and accept the new one.
 * This function allows the application to know whether a new incoming call is a one that replaces another one.
**/
LinphoneCall *linphone_call_get_replaced_call(LinphoneCall *call){
	SalOp *op=sal_call_get_replaces(call->op);
	if (op){
		return (LinphoneCall*)sal_op_get_user_pointer(op);
	}
	return NULL;
}

/**
 * Indicate whether camera input should be sent to remote end.
**/
void linphone_call_enable_camera (LinphoneCall *call, bool_t enable){
#ifdef VIDEO_ENABLED
	if (call->videostream!=NULL && call->videostream->ticker!=NULL){
		LinphoneCore *lc=call->core;
		MSWebCam *nowebcam=get_nowebcam_device();
		if (call->camera_active!=enable && lc->video_conf.device!=nowebcam){
			video_stream_change_camera(call->videostream,
			             enable ? lc->video_conf.device : nowebcam);
		}
	}
	call->camera_active=enable;
#endif
}

/**
 * Take a photo of currently received video and write it into a jpeg file.
**/
int linphone_call_take_video_snapshot(LinphoneCall *call, const char *file){
#ifdef VIDEO_ENABLED
	if (call->videostream!=NULL && call->videostream->jpegwriter!=NULL){
		return ms_filter_call_method(call->videostream->jpegwriter,MS_JPEG_WRITER_TAKE_SNAPSHOT,(void*)file);
	}
	ms_warning("Cannot take snapshot: no currently running video stream on this call.");
	return -1;
#endif
	return -1;
}

/**
 * Returns TRUE if camera pictures are sent to the remote party.
**/
bool_t linphone_call_camera_enabled (const LinphoneCall *call){
	return call->camera_active;
}

/**
 * Enable video stream.
**/
void linphone_call_params_enable_video(LinphoneCallParams *cp, bool_t enabled){
	cp->has_video=enabled;
}

const PayloadType* linphone_call_params_get_used_audio_codec(const LinphoneCallParams *cp) {
	return cp->audio_codec;
}

const PayloadType* linphone_call_params_get_used_video_codec(const LinphoneCallParams *cp) {
	return cp->video_codec;
}

/**
 * Returns whether video is enabled.
**/
bool_t linphone_call_params_video_enabled(const LinphoneCallParams *cp){
	return cp->has_video;
}

enum LinphoneMediaEncryption linphone_call_params_get_media_encryption(const LinphoneCallParams *cp) {
	return cp->media_encryption;
}

void linphone_call_params_set_media_encryption(LinphoneCallParams *cp, enum LinphoneMediaEncryption e) {
	cp->media_encryption = e;
}


/**
 * Enable sending of real early media (during outgoing calls).
**/
void linphone_call_params_enable_early_media_sending(LinphoneCallParams *cp, bool_t enabled){
	cp->real_early_media=enabled;
}

bool_t linphone_call_params_early_media_sending_enabled(const LinphoneCallParams *cp){
	return cp->real_early_media;
}

/**
 * Returns true if the call is part of the locally managed conference.
**/
bool_t linphone_call_params_local_conference_mode(const LinphoneCallParams *cp){
	return cp->in_conference;
}

/**
 * Refine bandwidth settings for this call by setting a bandwidth limit for audio streams.
 * As a consequence, codecs whose bitrates are not compatible with this limit won't be used.
**/
void linphone_call_params_set_audio_bandwidth_limit(LinphoneCallParams *cp, int bandwidth){
	cp->audio_bw=bandwidth;
}

#ifdef VIDEO_ENABLED
/**
 * Request remote side to send us a Video Fast Update.
**/
void linphone_call_send_vfu_request(LinphoneCall *call)
{
	if (LinphoneCallStreamsRunning == linphone_call_get_state(call))
		sal_call_send_vfu_request(call->op);
}
#endif

/**
 *
**/
LinphoneCallParams * linphone_call_params_copy(const LinphoneCallParams *cp){
	LinphoneCallParams *ncp=ms_new0(LinphoneCallParams,1);
	memcpy(ncp,cp,sizeof(LinphoneCallParams));
	return ncp;
}

/**
 *
**/
void linphone_call_params_destroy(LinphoneCallParams *p){
	ms_free(p);
}

/**
 * @}
**/


#ifdef TEST_EXT_RENDERER
static void rendercb(void *data, const MSPicture *local, const MSPicture *remote){
	ms_message("rendercb, local buffer=%p, remote buffer=%p",
	           local ? local->planes[0] : NULL, remote? remote->planes[0] : NULL);
}
#endif

#ifdef VIDEO_ENABLED
static void video_stream_event_cb(void *user_pointer, const MSFilter *f, const unsigned int event_id, const void *args){
    LinphoneCall* call = (LinphoneCall*) user_pointer;
	ms_warning("In linphonecall.c: video_stream_event_cb");
	switch (event_id) {
		case MS_VIDEO_DECODER_DECODING_ERRORS:
			ms_warning("Case is MS_VIDEO_DECODER_DECODING_ERRORS");
			linphone_call_send_vfu_request(call);
			break;
        case MS_VIDEO_DECODER_FIRST_IMAGE_DECODED:
            ms_message("First video frame decoded successfully");
            if (call->nextVideoFrameDecoded._func != NULL)
                call->nextVideoFrameDecoded._func(call, call->nextVideoFrameDecoded._user_data);
            break;
		default:
			ms_warning("Unhandled event %i", event_id);
			break;
	}
}
#endif

void linphone_call_set_next_video_frame_decoded_callback(LinphoneCall *call, LinphoneCallCbFunc cb, void* user_data) {
    call->nextVideoFrameDecoded._func = cb;
    call->nextVideoFrameDecoded._user_data = user_data;
#ifdef VIDEO_ENABLED
    ms_filter_call_method_noarg(call->videostream->decoder, MS_VIDEO_DECODER_RESET_FIRST_IMAGE_NOTIFICATION);
#endif
}

void linphone_call_init_media_streams(LinphoneCall *call){
	LinphoneCore *lc=call->core;
	SalMediaDescription *md=call->localdesc;
	AudioStream *audiostream;

	call->audiostream=audiostream=audio_stream_new(md->streams[0].port,linphone_core_ipv6_enabled(lc));
	if (linphone_core_echo_limiter_enabled(lc)){
		const char *type=lp_config_get_string(lc->config,"sound","el_type","mic");
		if (strcasecmp(type,"mic")==0)
			audio_stream_enable_echo_limiter(audiostream,ELControlMic);
		else if (strcasecmp(type,"full")==0)
			audio_stream_enable_echo_limiter(audiostream,ELControlFull);
	}
	audio_stream_enable_gain_control(audiostream,TRUE);
	if (linphone_core_echo_cancellation_enabled(lc)){
		int len,delay,framesize;
		const char *statestr=lp_config_get_string(lc->config,"sound","ec_state",NULL);
		len=lp_config_get_int(lc->config,"sound","ec_tail_len",0);
		delay=lp_config_get_int(lc->config,"sound","ec_delay",0);
		framesize=lp_config_get_int(lc->config,"sound","ec_framesize",0);
		audio_stream_set_echo_canceller_params(audiostream,len,delay,framesize);
		if (statestr && audiostream->ec){
			ms_filter_call_method(audiostream->ec,MS_ECHO_CANCELLER_SET_STATE_STRING,(void*)statestr);
		}
	}
	audio_stream_enable_automatic_gain_control(audiostream,linphone_core_agc_enabled(lc));
	{
		int enabled=lp_config_get_int(lc->config,"sound","noisegate",0);
		audio_stream_enable_noise_gate(audiostream,enabled);
	}

	if (lc->rtptf){
		RtpTransport *artp=lc->rtptf->audio_rtp_func(lc->rtptf->audio_rtp_func_data, call->audio_port);
		RtpTransport *artcp=lc->rtptf->audio_rtcp_func(lc->rtptf->audio_rtcp_func_data, call->audio_port+1);
		rtp_session_set_transports(audiostream->session,artp,artcp);
	}

	call->audiostream_app_evq = ortp_ev_queue_new();
	rtp_session_register_event_queue(audiostream->session,call->audiostream_app_evq);

#ifdef VIDEO_ENABLED

	if ((lc->video_conf.display || lc->video_conf.capture) && md->streams[1].port>0){
		int video_recv_buf_size=lp_config_get_int(lc->config,"video","recv_buf_size",0);
		call->videostream=video_stream_new(md->streams[1].port,linphone_core_ipv6_enabled(lc));
		video_stream_enable_display_filter_auto_rotate(call->videostream, lp_config_get_int(lc->config,"video","display_filter_auto_rotate",0));
		if (video_recv_buf_size>0) rtp_session_set_recv_buf_size(call->videostream->session,video_recv_buf_size);
          
		if( lc->video_conf.displaytype != NULL)
			video_stream_set_display_filter_name(call->videostream,lc->video_conf.displaytype);
		video_stream_set_event_callback(call->videostream,video_stream_event_cb, call);
		if (lc->rtptf){
			RtpTransport *vrtp=lc->rtptf->video_rtp_func(lc->rtptf->video_rtp_func_data, call->video_port);
			RtpTransport *vrtcp=lc->rtptf->video_rtcp_func(lc->rtptf->video_rtcp_func_data, call->video_port+1);
			rtp_session_set_transports(call->videostream->session,vrtp,vrtcp);
		}
		call->videostream_app_evq = ortp_ev_queue_new();
		rtp_session_register_event_queue(call->videostream->session,call->videostream_app_evq);
#ifdef TEST_EXT_RENDERER
		video_stream_set_render_callback(call->videostream,rendercb,NULL);
#endif
	}
#else
	call->videostream=NULL;
#endif
}


static int dtmf_tab[16]={'0','1','2','3','4','5','6','7','8','9','*','#','A','B','C','D'};

static void linphone_core_dtmf_received(RtpSession* s, int dtmf, void* user_data){
	LinphoneCore* lc = (LinphoneCore*)user_data;
	if (dtmf<0 || dtmf>15){
		ms_warning("Bad dtmf value %i",dtmf);
		return;
	}
	if (lc->vtable.dtmf_received != NULL)
		lc->vtable.dtmf_received(lc, linphone_core_get_current_call(lc), dtmf_tab[dtmf]);
}

static void parametrize_equalizer(LinphoneCore *lc, AudioStream *st){
	if (st->equalizer){
		MSFilter *f=st->equalizer;
		int enabled=lp_config_get_int(lc->config,"sound","eq_active",0);
		const char *gains=lp_config_get_string(lc->config,"sound","eq_gains",NULL);
		ms_filter_call_method(f,MS_EQUALIZER_SET_ACTIVE,&enabled);
		if (enabled){
			if (gains){
				do{
					int bytes;
					MSEqualizerGain g;
					if (sscanf(gains,"%f:%f:%f %n",&g.frequency,&g.gain,&g.width,&bytes)==3){
						ms_message("Read equalizer gains: %f(~%f) --> %f",g.frequency,g.width,g.gain);
						ms_filter_call_method(f,MS_EQUALIZER_SET_GAIN,&g);
						gains+=bytes;
					}else break;
				}while(1);
			}
		}
	}
}

void _post_configure_audio_stream(AudioStream *st, LinphoneCore *lc, bool_t muted){
	float mic_gain=lp_config_get_float(lc->config,"sound","mic_gain",1);
	float thres = 0;
	float recv_gain;
	float ng_thres=lp_config_get_float(lc->config,"sound","ng_thres",0.05);
	float ng_floorgain=lp_config_get_float(lc->config,"sound","ng_floorgain",0);
	int dc_removal=lp_config_get_int(lc->config,"sound","dc_removal",0);

	if (!muted)
		audio_stream_set_mic_gain(st,mic_gain);
	else
		audio_stream_set_mic_gain(st,0);

	recv_gain = lc->sound_conf.soft_play_lev;
	if (recv_gain != 0) {
		linphone_core_set_playback_gain_db (lc,recv_gain);
	}
	
	if (st->volsend){
		ms_filter_call_method(st->volsend,MS_VOLUME_REMOVE_DC,&dc_removal);
		float speed=lp_config_get_float(lc->config,"sound","el_speed",-1);
		thres=lp_config_get_float(lc->config,"sound","el_thres",-1);
		float force=lp_config_get_float(lc->config,"sound","el_force",-1);
		int sustain=lp_config_get_int(lc->config,"sound","el_sustain",-1);
		float transmit_thres=lp_config_get_float(lc->config,"sound","el_transmit_thres",-1);
		MSFilter *f=NULL;
		f=st->volsend;
		if (speed==-1) speed=0.03;
		if (force==-1) force=25;
		ms_filter_call_method(f,MS_VOLUME_SET_EA_SPEED,&speed);
		ms_filter_call_method(f,MS_VOLUME_SET_EA_FORCE,&force);
		if (thres!=-1)
			ms_filter_call_method(f,MS_VOLUME_SET_EA_THRESHOLD,&thres);
		if (sustain!=-1)
			ms_filter_call_method(f,MS_VOLUME_SET_EA_SUSTAIN,&sustain);
		if (transmit_thres!=-1)
				ms_filter_call_method(f,MS_VOLUME_SET_EA_TRANSMIT_THRESHOLD,&transmit_thres);

		ms_filter_call_method(st->volsend,MS_VOLUME_SET_NOISE_GATE_THRESHOLD,&ng_thres);
		ms_filter_call_method(st->volsend,MS_VOLUME_SET_NOISE_GATE_FLOORGAIN,&ng_floorgain);
	}
	if (st->volrecv){
		/* parameters for a limited noise-gate effect, using echo limiter threshold */
		float floorgain = 1/mic_gain;
		int spk_agc=lp_config_get_int(lc->config,"sound","speaker_agc_enabled",0);
		ms_filter_call_method(st->volrecv, MS_VOLUME_ENABLE_AGC, &spk_agc);
		ms_filter_call_method(st->volrecv,MS_VOLUME_SET_NOISE_GATE_THRESHOLD,&ng_thres);
		ms_filter_call_method(st->volrecv,MS_VOLUME_SET_NOISE_GATE_FLOORGAIN,&floorgain);
	}
	parametrize_equalizer(lc,st);
}

static void post_configure_audio_streams(LinphoneCall*call){
	AudioStream *st=call->audiostream;
	LinphoneCore *lc=call->core;
	_post_configure_audio_stream(st,lc,call->audio_muted);
	if (lc->vtable.dtmf_received!=NULL){
		/* replace by our default action*/
		audio_stream_play_received_dtmfs(call->audiostream,FALSE);
		rtp_session_signal_connect(call->audiostream->session,"telephone-event",(RtpCallback)linphone_core_dtmf_received,(unsigned long)lc);
	}
}

static RtpProfile *make_profile(LinphoneCall *call, const SalMediaDescription *md, const SalStreamDescription *desc, int *used_pt){
	int bw;
	const MSList *elem;
	RtpProfile *prof=rtp_profile_new("Call profile");
	bool_t first=TRUE;
	int remote_bw=0;
	LinphoneCore *lc=call->core;
	int up_ptime=0;
	*used_pt=-1;

	for(elem=desc->payloads;elem!=NULL;elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
		int number;

		if ((pt->flags & PAYLOAD_TYPE_FLAG_CAN_SEND) && first) {
			if (desc->type==SalAudio){
				linphone_core_update_allocated_audio_bandwidth_in_call(call,pt);
				up_ptime=linphone_core_get_upload_ptime(lc);
			}
			*used_pt=payload_type_get_number(pt);
			first=FALSE;
		}
		if (desc->bandwidth>0) remote_bw=desc->bandwidth;
		else if (md->bandwidth>0) {
			/*case where b=AS is given globally, not per stream*/
			remote_bw=md->bandwidth;
			if (desc->type==SalVideo){
				remote_bw=get_video_bandwidth(remote_bw,call->audio_bw);
			}
		}

		if (desc->type==SalAudio){
				bw=get_min_bandwidth(call->audio_bw,remote_bw);
		}else bw=get_min_bandwidth(get_video_bandwidth(linphone_core_get_upload_bandwidth (lc),call->audio_bw),remote_bw);
		if (bw>0) pt->normal_bitrate=bw*1000;
		else if (desc->type==SalAudio){
			pt->normal_bitrate=-1;
		}
		if (desc->ptime>0){
			up_ptime=desc->ptime;
		}
		if (up_ptime>0){
			char tmp[40];
			snprintf(tmp,sizeof(tmp),"ptime=%i",up_ptime);
			payload_type_append_send_fmtp(pt,tmp);
		}
		number=payload_type_get_number(pt);
		if (rtp_profile_get_payload(prof,number)!=NULL){
			ms_warning("A payload type with number %i already exists in profile !",number);
		}else
			rtp_profile_set_payload(prof,number,pt);
	}
	return prof;
}


static void setup_ring_player(LinphoneCore *lc, LinphoneCall *call){
	int pause_time=3000;
	audio_stream_play(call->audiostream,lc->sound_conf.ringback_tone);
	ms_filter_call_method(call->audiostream->soundread,MS_FILE_PLAYER_LOOP,&pause_time);
}

#define LINPHONE_RTCP_SDES_TOOL "Linphone-" LINPHONE_VERSION

static bool_t linphone_call_sound_resources_available(LinphoneCall *call){
	LinphoneCore *lc=call->core;
	LinphoneCall *current=linphone_core_get_current_call(lc);
	return !linphone_core_is_in_conference(lc) && 
		(current==NULL || current==call);
}
static int find_crypto_index_from_tag(const SalSrtpCryptoAlgo crypto[],unsigned char tag) {
    int i;
    for(i=0; i<SAL_CRYPTO_ALGO_MAX; i++) {
        if (crypto[i].tag == tag) {
            return i;
        }
    }
    return -1;
}
static void linphone_call_start_audio_stream(LinphoneCall *call, const char *cname, bool_t muted, bool_t send_ringbacktone, bool_t use_arc){
	LinphoneCore *lc=call->core;
	int jitt_comp=lc->rtp_conf.audio_jitt_comp;
	int used_pt=-1;
	/* look for savp stream first */
	const SalStreamDescription *stream=sal_media_description_find_stream(call->resultdesc,
	    					SalProtoRtpSavp,SalAudio);
	/* no savp audio stream, use avp */
	if (!stream)
		stream=sal_media_description_find_stream(call->resultdesc,
	    					SalProtoRtpAvp,SalAudio);

	if (stream && stream->dir!=SalStreamInactive && stream->port!=0){
		MSSndCard *playcard=lc->sound_conf.lsd_card ?
			lc->sound_conf.lsd_card : lc->sound_conf.play_sndcard;
		MSSndCard *captcard=lc->sound_conf.capt_sndcard;
		const char *playfile=lc->play_file;
		const char *recfile=lc->rec_file;
		call->audio_profile=make_profile(call,call->resultdesc,stream,&used_pt);
		bool_t use_ec;

		if (used_pt!=-1){
			call->current_params.audio_codec = rtp_profile_get_payload(call->audio_profile, used_pt);
			if (playcard==NULL) {
				ms_warning("No card defined for playback !");
			}
			if (captcard==NULL) {
				ms_warning("No card defined for capture !");
			}
			/*Replace soundcard filters by inactive file players or recorders
			 when placed in recvonly or sendonly mode*/
			if (stream->port==0 || stream->dir==SalStreamRecvOnly){
				captcard=NULL;
				playfile=NULL;
			}else if (stream->dir==SalStreamSendOnly){
				playcard=NULL;
				captcard=NULL;
				recfile=NULL;
				/*And we will eventually play "playfile" if set by the user*/
				/*playfile=NULL;*/
			}
			if (send_ringbacktone){
				captcard=NULL;
				playfile=NULL;/* it is setup later*/
			}
			/*if playfile are supplied don't use soundcards*/
			if (lc->use_files) {
				captcard=NULL;
				playcard=NULL;
			}
			if (call->params.in_conference){
				/* first create the graph without soundcard resources*/
				captcard=playcard=NULL;
			}
			if (!linphone_call_sound_resources_available(call)){
				ms_message("Sound resources are used by another call, not using soundcard.");
				captcard=playcard=NULL;
			}
			use_ec=captcard==NULL ? FALSE : linphone_core_echo_cancellation_enabled(lc);
			if (playcard &&  stream->max_rate>0) ms_snd_card_set_preferred_sample_rate(playcard, stream->max_rate);
			if (captcard &&  stream->max_rate>0) ms_snd_card_set_preferred_sample_rate(captcard, stream->max_rate);
			audio_stream_enable_adaptive_bitrate_control(call->audiostream,use_arc);
			audio_stream_start_full(
				call->audiostream,
				call->audio_profile,
				stream->addr[0]!='\0' ? stream->addr : call->resultdesc->addr,
				stream->port,
				linphone_core_rtcp_enabled(lc) ? (stream->port+1) : 0,
				used_pt,
				jitt_comp,
				playfile,
				recfile,
				playcard,
				captcard,
				use_ec
				);
			post_configure_audio_streams(call);
			if (muted && !send_ringbacktone){
				audio_stream_set_mic_gain(call->audiostream,0);
			}
			if (stream->dir==SalStreamSendOnly && playfile!=NULL){
				int pause_time=500;
				ms_filter_call_method(call->audiostream->soundread,MS_FILE_PLAYER_LOOP,&pause_time);
			}
			if (send_ringbacktone){
				setup_ring_player(lc,call);
			}
			audio_stream_set_rtcp_information(call->audiostream, cname, LINPHONE_RTCP_SDES_TOOL);
			
            /* valid local tags are > 0 */
			if (stream->proto == SalProtoRtpSavp) {
                const SalStreamDescription *local_st_desc=sal_media_description_find_stream(call->localdesc,
                                                                                            SalProtoRtpSavp,SalAudio);
                int crypto_idx = find_crypto_index_from_tag(local_st_desc->crypto, stream->crypto_local_tag);
                
                if (crypto_idx >= 0) {
                    audio_stream_enable_strp(
                                             call->audiostream, 
                                             stream->crypto[0].algo,
                                             local_st_desc->crypto[crypto_idx].master_key,
                                             stream->crypto[0].master_key);
                    call->audiostream_encrypted=TRUE;
                } else {
                    ms_warning("Failed to find local crypto algo with tag: %d", stream->crypto_local_tag);
                    call->audiostream_encrypted=FALSE;
                }
			}else call->audiostream_encrypted=FALSE;
			if (call->params.in_conference){
				/*transform the graph to connect it to the conference filter */
				bool_t mute=stream->dir==SalStreamRecvOnly;
				linphone_call_add_to_conf(call, mute);
			}
			call->current_params.in_conference=call->params.in_conference;
		}else ms_warning("No audio stream accepted ?");
	}
}

static void linphone_call_start_video_stream(LinphoneCall *call, const char *cname,bool_t all_inputs_muted){
#ifdef VIDEO_ENABLED
	LinphoneCore *lc=call->core;
	int used_pt=-1;
	/* look for savp stream first */
	const SalStreamDescription *vstream=sal_media_description_find_stream(call->resultdesc,
	    					SalProtoRtpSavp,SalVideo);
	/* no savp audio stream, use avp */
	if (!vstream)
		vstream=sal_media_description_find_stream(call->resultdesc,
	    					SalProtoRtpAvp,SalVideo);
	    					
	/* shutdown preview */
	if (lc->previewstream!=NULL) {
		video_preview_stop(lc->previewstream);
		lc->previewstream=NULL;
	}
	
	if (vstream!=NULL && vstream->dir!=SalStreamInactive && vstream->port!=0) {
		const char *addr=vstream->addr[0]!='\0' ? vstream->addr : call->resultdesc->addr;
		call->video_profile=make_profile(call,call->resultdesc,vstream,&used_pt);
		if (used_pt!=-1){
			call->current_params.video_codec = rtp_profile_get_payload(call->video_profile, used_pt);
			VideoStreamDir dir=VideoStreamSendRecv;
			MSWebCam *cam=lc->video_conf.device;
			bool_t is_inactive=FALSE;

			call->current_params.has_video=TRUE;

			video_stream_enable_adaptive_bitrate_control(call->videostream,
			                                          linphone_core_adaptive_rate_control_enabled(lc));
			video_stream_set_sent_video_size(call->videostream,linphone_core_get_preferred_video_size(lc));
			video_stream_enable_self_view(call->videostream,lc->video_conf.selfview);
			if (lc->video_window_id!=0)
				video_stream_set_native_window_id(call->videostream,lc->video_window_id);
			if (lc->preview_window_id!=0)
				video_stream_set_native_preview_window_id (call->videostream,lc->preview_window_id);
			video_stream_use_preview_video_window (call->videostream,lc->use_preview_window);
			
			if (vstream->dir==SalStreamSendOnly && lc->video_conf.capture ){
				cam=get_nowebcam_device();
				dir=VideoStreamSendOnly;
			}else if (vstream->dir==SalStreamRecvOnly && lc->video_conf.display ){
				dir=VideoStreamRecvOnly;
			}else if (vstream->dir==SalStreamSendRecv){
				if (lc->video_conf.display && lc->video_conf.capture)
					dir=VideoStreamSendRecv;
				else if (lc->video_conf.display)
					dir=VideoStreamRecvOnly;
				else
					dir=VideoStreamSendOnly;
			}else{
				ms_warning("video stream is inactive.");
				/*either inactive or incompatible with local capabilities*/
				is_inactive=TRUE;
			}
			if (call->camera_active==FALSE || all_inputs_muted){
				cam=get_nowebcam_device();
			}
			if (!is_inactive){
                call->log->video_enabled = TRUE;
				video_stream_set_direction (call->videostream, dir);
				ms_message("%s lc rotation:%d\n", __FUNCTION__, lc->device_rotation);
				video_stream_set_device_rotation(call->videostream, lc->device_rotation);
				video_stream_start(call->videostream,
					call->video_profile, addr, vstream->port,
					linphone_core_rtcp_enabled(lc) ? (vstream->port+1) : 0,
					used_pt, lc->rtp_conf.audio_jitt_comp, cam);
				video_stream_set_rtcp_information(call->videostream, cname,LINPHONE_RTCP_SDES_TOOL);
			}
			
			if (vstream->proto == SalProtoRtpSavp) {
				const SalStreamDescription *local_st_desc=sal_media_description_find_stream(call->localdesc,
	    					SalProtoRtpSavp,SalVideo);
	    					
				video_stream_enable_strp(
					call->videostream, 
					vstream->crypto[0].algo,
					local_st_desc->crypto[0].master_key, 
					vstream->crypto[0].master_key
					);
				call->videostream_encrypted=TRUE;
			}else{
				call->videostream_encrypted=FALSE;
			}
		}else ms_warning("No video stream accepted.");
	}else{
		ms_warning("No valid video stream defined.");
	}
#endif
}

void linphone_call_start_media_streams(LinphoneCall *call, bool_t all_inputs_muted, bool_t send_ringbacktone){
	LinphoneCore *lc=call->core;

	call->current_params.audio_codec = NULL;
	call->current_params.video_codec = NULL;

	LinphoneAddress *me=linphone_core_get_primary_contact_parsed(lc);
	char *cname;
	bool_t use_arc=linphone_core_adaptive_rate_control_enabled(lc);
#ifdef VIDEO_ENABLED
	const SalStreamDescription *vstream=sal_media_description_find_stream(call->resultdesc,
		    					SalProtoRtpAvp,SalVideo);
#endif

	if(call->audiostream == NULL)
	{
		ms_fatal("start_media_stream() called without prior init !");
		return;
	}
	cname=linphone_address_as_string_uri_only(me);

#if defined(VIDEO_ENABLED)
	if (vstream!=NULL && vstream->dir!=SalStreamInactive && vstream->payloads!=NULL){
		/*when video is used, do not make adaptive rate control on audio, it is stupid.*/
		use_arc=FALSE;
	}
#endif
	linphone_call_start_audio_stream(call,cname,all_inputs_muted,send_ringbacktone,use_arc);
	call->current_params.has_video=FALSE;
	if (call->videostream!=NULL) {
		linphone_call_start_video_stream(call,cname,all_inputs_muted);
	}

	call->all_muted=all_inputs_muted;
	call->playing_ringbacktone=send_ringbacktone;
	call->up_bw=linphone_core_get_upload_bandwidth(lc);

	if (call->params.media_encryption==LinphoneMediaEncryptionZRTP) {
		OrtpZrtpParams params;
		/*will be set later when zrtp is activated*/
		call->current_params.media_encryption=LinphoneMediaEncryptionNone;
		
		params.zid_file=lc->zrtp_secrets_cache;
		audio_stream_enable_zrtp(call->audiostream,&params);
	}else if (call->params.media_encryption==LinphoneMediaEncryptionSRTP){
		call->current_params.media_encryption=linphone_call_are_all_streams_encrypted(call) ?
			LinphoneMediaEncryptionSRTP : LinphoneMediaEncryptionNone;
	}

	/*also reflect the change if the "wished" params, in order to avoid to propose SAVP or video again
	 * further in the call, for example during pause,resume, conferencing reINVITEs*/
	linphone_call_fix_call_parameters(call);

	goto end;
	end:
		ms_free(cname);
		linphone_address_destroy(me);
}

static void linphone_call_log_fill_stats(LinphoneCallLog *log, AudioStream *st){
	audio_stream_get_local_rtp_stats (st,&log->local_stats);
	log->quality=audio_stream_get_average_quality_rating(st);
}

void linphone_call_stop_media_streams(LinphoneCall *call){
	if (call->audiostream!=NULL) {
		rtp_session_unregister_event_queue(call->audiostream->session,call->audiostream_app_evq);
		ortp_ev_queue_flush(call->audiostream_app_evq);
		ortp_ev_queue_destroy(call->audiostream_app_evq);

		if (call->audiostream->ec){
			const char *state_str=NULL;
			ms_filter_call_method(call->audiostream->ec,MS_ECHO_CANCELLER_GET_STATE_STRING,&state_str);
			if (state_str){
				ms_message("Writing echo canceler state, %i bytes",(int)strlen(state_str));
				lp_config_set_string(call->core->config,"sound","ec_state",state_str);
			}
		}
		linphone_call_log_fill_stats (call->log,call->audiostream);
		if (call->endpoint){
			linphone_call_remove_from_conf(call);
		}
		audio_stream_stop(call->audiostream);
		call->audiostream=NULL;
	}


#ifdef VIDEO_ENABLED
	if (call->videostream!=NULL){
		rtp_session_unregister_event_queue(call->videostream->session,call->videostream_app_evq);
		ortp_ev_queue_flush(call->videostream_app_evq);
		ortp_ev_queue_destroy(call->videostream_app_evq);
		video_stream_stop(call->videostream);
		call->videostream=NULL;
	}
#endif
	ms_event_queue_skip(call->core->msevq);
	
	if (call->audio_profile){
		rtp_profile_clear_all(call->audio_profile);
		rtp_profile_destroy(call->audio_profile);
		call->audio_profile=NULL;
	}
	if (call->video_profile){
		rtp_profile_clear_all(call->video_profile);
		rtp_profile_destroy(call->video_profile);
		call->video_profile=NULL;
	}
}



void linphone_call_enable_echo_cancellation(LinphoneCall *call, bool_t enable) {
	if (call!=NULL && call->audiostream!=NULL && call->audiostream->ec){
		bool_t bypass_mode = !enable;
		ms_filter_call_method(call->audiostream->ec,MS_ECHO_CANCELLER_SET_BYPASS_MODE,&bypass_mode);
	}
}
bool_t linphone_call_echo_cancellation_enabled(LinphoneCall *call) {
	if (call!=NULL && call->audiostream!=NULL && call->audiostream->ec){
		bool_t val;
		ms_filter_call_method(call->audiostream->ec,MS_ECHO_CANCELLER_GET_BYPASS_MODE,&val);
		return !val;
	} else {
		return linphone_core_echo_cancellation_enabled(call->core);
	}
}

void linphone_call_enable_echo_limiter(LinphoneCall *call, bool_t val){
	if (call!=NULL && call->audiostream!=NULL ) {
		if (val) {
		const char *type=lp_config_get_string(call->core->config,"sound","el_type","mic");
		if (strcasecmp(type,"mic")==0)
			audio_stream_enable_echo_limiter(call->audiostream,ELControlMic);
		else if (strcasecmp(type,"full")==0)
			audio_stream_enable_echo_limiter(call->audiostream,ELControlFull);
		} else {
			audio_stream_enable_echo_limiter(call->audiostream,ELInactive);
		}
	}
}

bool_t linphone_call_echo_limiter_enabled(const LinphoneCall *call){
	if (call!=NULL && call->audiostream!=NULL ){
		return call->audiostream->el_type !=ELInactive ;
	} else {
		return linphone_core_echo_limiter_enabled(call->core);
	}
}

/**
 * @addtogroup call_misc
 * @{
**/

/**
 * Returns the measured sound volume played locally (received from remote).
 * It is expressed in dbm0.
**/
float linphone_call_get_play_volume(LinphoneCall *call){
	AudioStream *st=call->audiostream;
	if (st && st->volrecv){
		float vol=0;
		ms_filter_call_method(st->volrecv,MS_VOLUME_GET,&vol);
		return vol;

	}
	return LINPHONE_VOLUME_DB_LOWEST;
}

/**
 * Returns the measured sound volume recorded locally (sent to remote).
 * It is expressed in dbm0.
**/
float linphone_call_get_record_volume(LinphoneCall *call){
	AudioStream *st=call->audiostream;
	if (st && st->volsend && !call->audio_muted && call->state==LinphoneCallStreamsRunning){
		float vol=0;
		ms_filter_call_method(st->volsend,MS_VOLUME_GET,&vol);
		return vol;

	}
	return LINPHONE_VOLUME_DB_LOWEST;
}

/**
 * Obtain real-time quality rating of the call
 *
 * Based on local RTP statistics and RTCP feedback, a quality rating is computed and updated
 * during all the duration of the call. This function returns its value at the time of the function call.
 * It is expected that the rating is updated at least every 5 seconds or so.
 * The rating is a floating point number comprised between 0 and 5.
 *
 * 4-5 = good quality <br>
 * 3-4 = average quality <br>
 * 2-3 = poor quality <br>
 * 1-2 = very poor quality <br>
 * 0-1 = can't be worse, mostly unusable <br>
 *
 * @returns The function returns -1 if no quality measurement is available, for example if no
 * active audio stream exist. Otherwise it returns the quality rating.
**/
float linphone_call_get_current_quality(LinphoneCall *call){
	if (call->audiostream){
		return audio_stream_get_quality_rating(call->audiostream);
	}
	return -1;
}

/**
 * Returns call quality averaged over all the duration of the call.
 *
 * See linphone_call_get_current_quality() for more details about quality measurement.
**/
float linphone_call_get_average_quality(LinphoneCall *call){
	if (call->audiostream){
		return audio_stream_get_average_quality_rating(call->audiostream);
	}
	return -1;
}

/**
 * Access last known statistics for audio stream, for a given call.
**/
const LinphoneCallStats *linphone_call_get_audio_stats(const LinphoneCall *call) {
	return &call->stats[LINPHONE_CALL_STATS_AUDIO];
}

/**
 * Access last known statistics for video stream, for a given call.
**/
const LinphoneCallStats *linphone_call_get_video_stats(const LinphoneCall *call) {
	return &call->stats[LINPHONE_CALL_STATS_VIDEO];
}


/**
 * @}
**/

static void display_bandwidth(RtpSession *as, RtpSession *vs){
	ms_message("bandwidth usage: audio=[d=%.1f,u=%.1f] video=[d=%.1f,u=%.1f] kbit/sec",
	(as!=NULL) ? (rtp_session_compute_recv_bandwidth(as)*1e-3) : 0,
	(as!=NULL) ? (rtp_session_compute_send_bandwidth(as)*1e-3) : 0,
	(vs!=NULL) ? (rtp_session_compute_recv_bandwidth(vs)*1e-3) : 0,
	(vs!=NULL) ? (rtp_session_compute_send_bandwidth(vs)*1e-3) : 0);
}

static void linphone_core_disconnected(LinphoneCore *lc, LinphoneCall *call){
	char temp[256];
	char *from=NULL;
	if(call)
		from = linphone_call_get_remote_address_as_string(call);
	if (from)
	{
		snprintf(temp,sizeof(temp),"Remote end %s seems to have disconnected, the call is going to be closed.",from);
		free(from);
	}
	else
	{
		snprintf(temp,sizeof(temp),"Remote end seems to have disconnected, the call is going to be closed.");
	}
	if (lc->vtable.display_warning!=NULL)
		lc->vtable.display_warning(lc,temp);
	linphone_core_terminate_call(lc,call);
}

void linphone_call_background_tasks(LinphoneCall *call, bool_t one_second_elapsed){
	LinphoneCore* lc = call->core;
	int disconnect_timeout = linphone_core_get_nortp_timeout(call->core);
	bool_t disconnected=FALSE;

	if (call->state==LinphoneCallStreamsRunning && one_second_elapsed){
		RtpSession *as=NULL,*vs=NULL;
		float audio_load=0, video_load=0;
		if (call->audiostream!=NULL){
			as=call->audiostream->session;
			if (call->audiostream->ticker)
				audio_load=ms_ticker_get_average_load(call->audiostream->ticker);
		}
		if (call->videostream!=NULL){
			if (call->videostream->ticker)
				video_load=ms_ticker_get_average_load(call->videostream->ticker);
			vs=call->videostream->session;
		}
		display_bandwidth(as,vs);
		ms_message("Thread processing load: audio=%f\tvideo=%f",audio_load,video_load);
	}
#ifdef VIDEO_ENABLED
	if (call->videostream!=NULL) {
		// Beware that the application queue should not depend on treatments fron the
		// mediastreamer queue.
		video_stream_iterate(call->videostream);

		if (call->videostream_app_evq){
			OrtpEvent *ev;
			while (NULL != (ev=ortp_ev_queue_get(call->videostream_app_evq))){
				OrtpEventType evt=ortp_event_get_type(ev);
				OrtpEventData *evd=ortp_event_get_data(ev);
				if (evt == ORTP_EVENT_ZRTP_ENCRYPTION_CHANGED){
					linphone_call_videostream_encryption_changed(call, evd->info.zrtp_stream_encrypted);
				} else if (evt == ORTP_EVENT_RTCP_PACKET_RECEIVED) {
					call->stats[LINPHONE_CALL_STATS_VIDEO].round_trip_delay = rtp_session_get_round_trip_propagation(call->videostream->session);
					if(call->stats[LINPHONE_CALL_STATS_VIDEO].received_rtcp != NULL)
						freemsg(call->stats[LINPHONE_CALL_STATS_VIDEO].received_rtcp);
					call->stats[LINPHONE_CALL_STATS_VIDEO].received_rtcp = evd->packet;
					evd->packet = NULL;
					if (lc->vtable.call_stats_updated)
						lc->vtable.call_stats_updated(lc, call, &call->stats[LINPHONE_CALL_STATS_VIDEO]);
				} else if (evt == ORTP_EVENT_RTCP_PACKET_EMITTED) {
					memcpy(&call->stats[LINPHONE_CALL_STATS_VIDEO].jitter_stats, rtp_session_get_jitter_stats(call->videostream->session), sizeof(jitter_stats_t));
					if(call->stats[LINPHONE_CALL_STATS_VIDEO].sent_rtcp != NULL)
						freemsg(call->stats[LINPHONE_CALL_STATS_VIDEO].sent_rtcp);
					call->stats[LINPHONE_CALL_STATS_VIDEO].sent_rtcp = evd->packet;
					evd->packet = NULL;
					if (lc->vtable.call_stats_updated)
						lc->vtable.call_stats_updated(lc, call, &call->stats[LINPHONE_CALL_STATS_VIDEO]);
				}
				ortp_event_destroy(ev);
			}
		}
	}
#endif
	if (call->audiostream!=NULL) {
		// Beware that the application queue should not depend on treatments fron the
		// mediastreamer queue.
		audio_stream_iterate(call->audiostream);

		if (call->audiostream_app_evq){
			OrtpEvent *ev;
			while (NULL != (ev=ortp_ev_queue_get(call->audiostream_app_evq))){
				OrtpEventType evt=ortp_event_get_type(ev);
				OrtpEventData *evd=ortp_event_get_data(ev);
				if (evt == ORTP_EVENT_ZRTP_ENCRYPTION_CHANGED){
					linphone_call_audiostream_encryption_changed(call, evd->info.zrtp_stream_encrypted);
				} else if (evt == ORTP_EVENT_ZRTP_SAS_READY) {
					linphone_call_audiostream_auth_token_ready(call, evd->info.zrtp_sas.sas, evd->info.zrtp_sas.verified);
				} else if (evt == ORTP_EVENT_RTCP_PACKET_RECEIVED) {
					call->stats[LINPHONE_CALL_STATS_AUDIO].round_trip_delay = rtp_session_get_round_trip_propagation(call->audiostream->session);
					if(call->stats[LINPHONE_CALL_STATS_AUDIO].received_rtcp != NULL)
						freemsg(call->stats[LINPHONE_CALL_STATS_AUDIO].received_rtcp);
					call->stats[LINPHONE_CALL_STATS_AUDIO].received_rtcp = evd->packet;
					evd->packet = NULL;
					if (lc->vtable.call_stats_updated)
						lc->vtable.call_stats_updated(lc, call, &call->stats[LINPHONE_CALL_STATS_AUDIO]);
				} else if (evt == ORTP_EVENT_RTCP_PACKET_EMITTED) {
					memcpy(&call->stats[LINPHONE_CALL_STATS_AUDIO].jitter_stats, rtp_session_get_jitter_stats(call->audiostream->session), sizeof(jitter_stats_t));
					if(call->stats[LINPHONE_CALL_STATS_AUDIO].sent_rtcp != NULL)
						freemsg(call->stats[LINPHONE_CALL_STATS_AUDIO].sent_rtcp);
					call->stats[LINPHONE_CALL_STATS_AUDIO].sent_rtcp = evd->packet;
					evd->packet = NULL;
					if (lc->vtable.call_stats_updated)
						lc->vtable.call_stats_updated(lc, call, &call->stats[LINPHONE_CALL_STATS_AUDIO]);
				}
				ortp_event_destroy(ev);
			}
		}
	}
	if (call->state==LinphoneCallStreamsRunning && one_second_elapsed && call->audiostream!=NULL && disconnect_timeout>0 )
		disconnected=!audio_stream_alive(call->audiostream,disconnect_timeout);
	if (disconnected)
		linphone_core_disconnected(call->core,call);
}

void linphone_call_log_completed(LinphoneCall *call){
	LinphoneCore *lc=call->core;

	call->log->duration=time(NULL)-call->start_time;

	if (call->log->status==LinphoneCallMissed){
		char *info;
		lc->missed_calls++;
		info=ortp_strdup_printf(ngettext("You have missed %i call.",
                                         "You have missed %i calls.", lc->missed_calls),
                                lc->missed_calls);
        if (lc->vtable.display_status!=NULL)
            lc->vtable.display_status(lc,info);
		ms_free(info);
	}
	lc->call_logs=ms_list_prepend(lc->call_logs,(void *)call->log);
	if (ms_list_size(lc->call_logs)>lc->max_call_logs){
		MSList *elem,*prevelem=NULL;
		/*find the last element*/
		for(elem=lc->call_logs;elem!=NULL;elem=elem->next){
			prevelem=elem;
		}
		elem=prevelem;
		linphone_call_log_destroy((LinphoneCallLog*)elem->data);
		lc->call_logs=ms_list_remove_link(lc->call_logs,elem);
	}
	if (lc->vtable.call_log_updated!=NULL){
		lc->vtable.call_log_updated(lc,call->log);
	}
	call_logs_write_to_config_file(lc);
}

LinphoneCallState linphone_call_get_transfer_state(LinphoneCall *call) {
	return call->transfer_state;
}

void linphone_call_set_transfer_state(LinphoneCall* call, LinphoneCallState state) {
	if (state != call->transfer_state) {
		LinphoneCore* lc = call->core;
		call->transfer_state = state;
		if (lc->vtable.transfer_state_changed)
			lc->vtable.transfer_state_changed(lc, call, state);
	}
}

