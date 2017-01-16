
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifdef _WIN32
#include <time.h>
#endif
#include "linphone/core.h"
#include "linphone/sipsetup.h"
#include "linphone/lpconfig.h"
#include "private.h"
#include "conference_private.h"

#include <ortp/event.h>
#include <ortp/b64.h>
#include <math.h>

#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/msvolume.h"
#include "mediastreamer2/msequalizer.h"
#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/msjpegwriter.h"
#include "mediastreamer2/mseventqueue.h"
#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msrtt4103.h"

static const char *EC_STATE_STORE = ".linphone.ecstate";
#define EC_STATE_MAX_LEN 1048576 // 1Mo

static void linphone_call_stats_uninit(LinphoneCallStats *stats);
static void linphone_call_get_local_ip(LinphoneCall *call, const LinphoneAddress *remote_addr);
static void _linphone_call_set_next_video_frame_decoded_trigger(LinphoneCall *call);
void linphone_call_handle_stream_events(LinphoneCall *call, int stream_index);


bool_t linphone_call_state_is_early(LinphoneCallState state){
	switch (state){
		case LinphoneCallIdle:
		case LinphoneCallOutgoingInit:
		case LinphoneCallOutgoingEarlyMedia:
		case LinphoneCallOutgoingRinging:
		case LinphoneCallOutgoingProgress:
		case LinphoneCallIncomingReceived:
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallEarlyUpdatedByRemote:
		case LinphoneCallEarlyUpdating:
			return TRUE;
		case LinphoneCallResuming:
		case LinphoneCallEnd:
		case LinphoneCallUpdating:
		case LinphoneCallRefered:
		case LinphoneCallPausing:
		case LinphoneCallPausedByRemote:
		case LinphoneCallPaused:
		case LinphoneCallConnected:
		case LinphoneCallError:
		case LinphoneCallUpdatedByRemote:
		case LinphoneCallReleased:
		case LinphoneCallStreamsRunning:
		break;
	}
	return FALSE;
}

MSWebCam *get_nowebcam_device(MSFactory* f){
#ifdef VIDEO_ENABLED
	return ms_web_cam_manager_get_cam(ms_factory_get_web_cam_manager(f),"StaticImage: Static picture");
#else
	return NULL;
#endif
}

static bool_t generate_b64_crypto_key(size_t key_length, char* key_out, size_t key_out_size) {
	size_t b64_size;
	uint8_t* tmp = (uint8_t*) ms_malloc0(key_length);
	if (sal_get_random_bytes(tmp, key_length)==NULL) {
		ms_error("Failed to generate random key");
		ms_free(tmp);
		return FALSE;
	}

	b64_size = b64_encode((const char*)tmp, key_length, NULL, 0);
	if (b64_size == 0) {
		ms_error("Failed to get b64 result size");
		ms_free(tmp);
		return FALSE;
	}
	if (b64_size>=key_out_size){
		ms_error("Insufficient room for writing base64 SRTP key");
		ms_free(tmp);
		return FALSE;
	}
	b64_size=b64_encode((const char*)tmp, key_length, key_out, key_out_size);
	if (b64_size == 0) {
		ms_error("Failed to b64 encode key");
		ms_free(tmp);
		return FALSE;
	}
	key_out[b64_size] = '\0';
	ms_free(tmp);
	return TRUE;
}

static bool_t linphone_call_encryption_mandatory(LinphoneCall *call){
	if (call->params->media_encryption==LinphoneMediaEncryptionDTLS) {
		ms_message("Forced encryption mandatory on call [%p] due to SRTP-DTLS",call);
		return TRUE;
	}
	return call->params->encryption_mandatory;
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

static bool_t at_least_one_stream_started(const LinphoneCall *call){
	return (call->audiostream && media_stream_get_state((MediaStream *)call->audiostream) == MSStreamStarted )
		|| (call->videostream && media_stream_get_state((MediaStream *)call->videostream) == MSStreamStarted)
		|| (call->textstream && media_stream_get_state((MediaStream *)call->textstream) == MSStreamStarted);
}

static bool_t linphone_call_all_streams_encrypted(const LinphoneCall *call) {
	int number_of_encrypted_stream = 0;
	int number_of_active_stream = 0;

	if (call->audiostream && media_stream_get_state((MediaStream *)call->audiostream) == MSStreamStarted) {
		number_of_active_stream++;
		if(media_stream_secured((MediaStream *)call->audiostream))
			number_of_encrypted_stream++;
	}
	if (call->videostream && media_stream_get_state((MediaStream *)call->videostream) == MSStreamStarted) {
		number_of_active_stream++;
		if (media_stream_secured((MediaStream *)call->videostream))
			number_of_encrypted_stream++;
	}
	if (call->textstream && media_stream_get_state((MediaStream *)call->textstream) == MSStreamStarted) {
		number_of_active_stream++;
		if (media_stream_secured((MediaStream *)call->textstream))
			number_of_encrypted_stream++;
	}
	return number_of_active_stream>0 && number_of_active_stream==number_of_encrypted_stream;
}

static bool_t linphone_call_all_streams_avpf_enabled(const LinphoneCall *call) {
	int nb_active_streams = 0;
	int nb_avpf_enabled_streams = 0;
	if (call) {
		if (call->audiostream && media_stream_get_state((MediaStream *)call->audiostream) == MSStreamStarted) {
			nb_active_streams++;
			if (media_stream_avpf_enabled((MediaStream *)call->audiostream))
				nb_avpf_enabled_streams++;
		}
		if (call->videostream && media_stream_get_state((MediaStream *)call->videostream) == MSStreamStarted) {
			nb_active_streams++;
			if (media_stream_avpf_enabled((MediaStream *)call->videostream))
				nb_avpf_enabled_streams++;
		}
	}
	return ((nb_active_streams > 0) && (nb_active_streams == nb_avpf_enabled_streams));
}

static uint16_t linphone_call_get_avpf_rr_interval(const LinphoneCall *call) {
	uint16_t rr_interval = 0;
	uint16_t stream_rr_interval;
	if (call) {
		if (call->audiostream && media_stream_get_state((MediaStream *)call->audiostream) == MSStreamStarted) {
			stream_rr_interval = media_stream_get_avpf_rr_interval((MediaStream *)call->audiostream);
			if (stream_rr_interval > rr_interval) rr_interval = stream_rr_interval;
		}
		if (call->videostream && media_stream_get_state((MediaStream *)call->videostream) == MSStreamStarted) {
			stream_rr_interval = media_stream_get_avpf_rr_interval((MediaStream *)call->videostream);
			if (stream_rr_interval > rr_interval) rr_interval = stream_rr_interval;
		}
	} else {
		rr_interval = 5000;
	}
	return rr_interval;
}

static void propagate_encryption_changed(LinphoneCall *call){
	if (!linphone_call_all_streams_encrypted(call)) {
		ms_message("Some streams are not encrypted");
		call->current_params->media_encryption=LinphoneMediaEncryptionNone;
		linphone_core_notify_call_encryption_changed(call->core, call, FALSE, call->auth_token);
	} else {
		if (call->auth_token) {/* ZRTP only is using auth_token */
			call->current_params->media_encryption=LinphoneMediaEncryptionZRTP;
		} else { /* otherwise it must be DTLS as SDES doesn't go through this function */
			call->current_params->media_encryption=LinphoneMediaEncryptionDTLS;
		}
		ms_message("All streams are encrypted key exchanged using %s", call->current_params->media_encryption==LinphoneMediaEncryptionZRTP?"ZRTP":call->current_params->media_encryption==LinphoneMediaEncryptionDTLS?"DTLS":"Unknown mechanism");
		linphone_core_notify_call_encryption_changed(call->core, call, TRUE, call->auth_token);
#ifdef VIDEO_ENABLED
		if (linphone_call_encryption_mandatory(call) && call->videostream && media_stream_started((MediaStream *)call->videostream)) {
			video_stream_send_vfu(call->videostream); /*nothing could have been sent yet so generating key frame*/
		}
#endif
	}
}

static void linphone_call_audiostream_encryption_changed(void *data, bool_t encrypted) {
	char status[255]={0};
	LinphoneCall *call;

	call = (LinphoneCall *)data;

	if (encrypted) {
		if (call->params->media_encryption==LinphoneMediaEncryptionZRTP) { /* if encryption is DTLS, no status to be displayed */
			snprintf(status,sizeof(status)-1,_("Authentication token is %s"),call->auth_token);
			linphone_core_notify_display_status(call->core, status);
		}
	}

	propagate_encryption_changed(call);

#ifdef VIDEO_ENABLED
	// Enable video encryption
	if (call->params->media_encryption==LinphoneMediaEncryptionZRTP) {
		const LinphoneCallParams *params=linphone_call_get_current_params(call);
		if (params->has_video) {
			ms_message("Trying to start ZRTP encryption on video stream");
			video_stream_start_zrtp(call->videostream);
		}
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
	if (call->audiostream==NULL || !media_stream_started(&call->audiostream->ms)){
		ms_error("linphone_call_set_authentication_token_verified(): No audio stream or not started");
		return;
	}
	if (call->audiostream->ms.sessions.zrtp_context==NULL){
		ms_error("linphone_call_set_authentication_token_verified(): No zrtp context.");
		return;
	}
	if (!call->auth_token_verified && verified){
		ms_zrtp_sas_verified(call->audiostream->ms.sessions.zrtp_context);
	}else if (call->auth_token_verified && !verified){
		ms_zrtp_sas_reset_verified(call->audiostream->ms.sessions.zrtp_context);
	}
	call->auth_token_verified=verified;
	propagate_encryption_changed(call);
}

static int get_max_codec_sample_rate(const bctbx_list_t *codecs){
	int max_sample_rate=0;
	const bctbx_list_t *it;
	for(it=codecs;it!=NULL;it=it->next){
		PayloadType *pt=(PayloadType*)it->data;
		int sample_rate;

		if( strcasecmp("G722",pt->mime_type) == 0 ){
			/* G722 spec says 8000 but the codec actually requires 16000 */
			sample_rate = 16000;
		}else sample_rate=pt->clock_rate;
		if (sample_rate>max_sample_rate) max_sample_rate=sample_rate;
	}
	return max_sample_rate;
}

static int find_payload_type_number(const bctbx_list_t *assigned, const PayloadType *pt){
	const bctbx_list_t *elem;
	const PayloadType *candidate=NULL;
	for(elem=assigned;elem!=NULL;elem=elem->next){
		const PayloadType *it=(const PayloadType*)elem->data;
		if ((strcasecmp(pt->mime_type, payload_type_get_mime(it)) == 0)
			&& (it->clock_rate==pt->clock_rate)
			&& (it->channels==pt->channels || pt->channels<=0)) {
			candidate=it;
			if ((it->recv_fmtp!=NULL && pt->recv_fmtp!=NULL && strcasecmp(it->recv_fmtp, pt->recv_fmtp)==0)
				|| (it->recv_fmtp==NULL && pt->recv_fmtp==NULL)){
				break;/*exact match*/
			}
		}
	}
	return candidate ? payload_type_get_number(candidate) : -1;
}

bool_t is_payload_type_number_available(const bctbx_list_t *l, int number, const PayloadType *ignore){
	const bctbx_list_t *elem;
	for (elem=l; elem!=NULL; elem=elem->next){
		const PayloadType *pt=(PayloadType*)elem->data;
		if (pt!=ignore && payload_type_get_number(pt)==number) return FALSE;
	}
	return TRUE;
}

static void linphone_core_assign_payload_type_numbers(LinphoneCore *lc, bctbx_list_t *codecs){
	bctbx_list_t *elem;
	int dyn_number=lc->codecs_conf.dyn_pt;
	PayloadType *red = NULL, *t140 = NULL;

	for (elem=codecs; elem!=NULL; elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
		int number=payload_type_get_number(pt);

		/*check if number is duplicated: it could be the case if the remote forced us to use a mapping with a previous offer*/
		if (number!=-1 && !(pt->flags & PAYLOAD_TYPE_FROZEN_NUMBER)){
			if (!is_payload_type_number_available(codecs, number, pt)){
				ms_message("Reassigning payload type %i %s/%i because already offered.", number, pt->mime_type, pt->clock_rate);
				number=-1; /*need to be re-assigned*/
			}
		}

		if (number==-1){
			while(dyn_number<127){
				if (is_payload_type_number_available(codecs, dyn_number, NULL)){
					payload_type_set_number(pt, dyn_number);
					dyn_number++;
					break;
				}
				dyn_number++;
			}
			if (dyn_number==127){
				ms_error("Too many payload types configured ! codec %s/%i is disabled.", pt->mime_type, pt->clock_rate);
				payload_type_set_enable(pt, FALSE);
			}
		}

		if (strcmp(pt->mime_type, payload_type_t140_red.mime_type) == 0) {
			red = pt;
		} else if (strcmp(pt->mime_type, payload_type_t140.mime_type) == 0) {
			t140 = pt;
		}
	}

	if (t140 && red) {
		int t140_payload_type_number = payload_type_get_number(t140);
		char *red_fmtp = ms_strdup_printf("%i/%i/%i", t140_payload_type_number, t140_payload_type_number, t140_payload_type_number);
		payload_type_set_recv_fmtp(red, red_fmtp);
		ms_free(red_fmtp);
	}
}

static bool_t has_telephone_event_at_rate(const bctbx_list_t *tev, int rate){
	const bctbx_list_t *it;
	for(it=tev;it!=NULL;it=it->next){
		const PayloadType *pt=(PayloadType*)it->data;
		if (pt->clock_rate==rate) return TRUE;
	}
	return FALSE;
}

static bctbx_list_t * create_telephone_events(LinphoneCore *lc, const bctbx_list_t *codecs){
	const bctbx_list_t *it;
	bctbx_list_t *ret=NULL;
	for(it=codecs;it!=NULL;it=it->next){
		const PayloadType *pt=(PayloadType*)it->data;
		if (!has_telephone_event_at_rate(ret,pt->clock_rate)){
			PayloadType *tev=payload_type_clone(&payload_type_telephone_event);
			tev->clock_rate=pt->clock_rate;
			/*let it choose the number dynamically as for normal codecs*/
			payload_type_set_number(tev, -1);
			if (ret==NULL){
				/*But for first telephone-event, prefer the number that was configured in the core*/
				if (is_payload_type_number_available(codecs, lc->codecs_conf.telephone_event_pt, NULL)){
					payload_type_set_number(tev, lc->codecs_conf.telephone_event_pt);
				}
			}
			ret=bctbx_list_append(ret,tev);
		}
	}
	return ret;
}

static bctbx_list_t *create_special_payload_types(LinphoneCore *lc, const bctbx_list_t *codecs){
	bctbx_list_t *ret=create_telephone_events(lc, codecs);
	if (linphone_core_generic_comfort_noise_enabled(lc)){
		PayloadType *cn=payload_type_clone(&payload_type_cn);
		payload_type_set_number(cn, 13);
		ret=bctbx_list_append(ret, cn);
	}
	return ret;
}

typedef struct _CodecConstraints{
	int bandwidth_limit;
	int max_codecs;
	bctbx_list_t *previously_used;
}CodecConstraints;

static bctbx_list_t *make_codec_list(LinphoneCore *lc, CodecConstraints * hints, SalStreamType stype, const bctbx_list_t *codecs){
	bctbx_list_t *l=NULL;
	const bctbx_list_t *it;
	int nb = 0;

	for(it=codecs;it!=NULL;it=it->next){
		PayloadType *pt=(PayloadType*)it->data;
		int num;

		if (!payload_type_enabled(pt)) {
			continue;
		}
		if (hints->bandwidth_limit>0 && !linphone_core_is_payload_type_usable_for_bandwidth(lc,pt,hints->bandwidth_limit)){
			ms_message("Codec %s/%i eliminated because of audio bandwidth constraint of %i kbit/s",
					pt->mime_type,pt->clock_rate,hints->bandwidth_limit);
			continue;
		}
		if (!linphone_core_check_payload_type_usability(lc,pt)){
			continue;
		}
		pt=payload_type_clone(pt);

		/*look for a previously assigned number for this codec*/
		num=find_payload_type_number(hints->previously_used, pt);
		if (num!=-1){
			payload_type_set_number(pt,num);
			payload_type_set_flag(pt, PAYLOAD_TYPE_FROZEN_NUMBER);
		}

		l=bctbx_list_append(l, pt);
		nb++;
		if ((hints->max_codecs > 0) && (nb >= hints->max_codecs)) break;
	}
	if (stype==SalAudio){
		bctbx_list_t *specials=create_special_payload_types(lc,l);
		l=bctbx_list_concat(l,specials);
	}
	linphone_core_assign_payload_type_numbers(lc, l);
	return l;
}

static void update_media_description_from_stun(SalMediaDescription *md, const StunCandidate *ac, const StunCandidate *vc, const StunCandidate *tc){
	int i;
	for (i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (!sal_stream_description_active(&md->streams[i])) continue;
		if ((md->streams[i].type == SalAudio) && (ac->port != 0)) {
			strcpy(md->streams[i].rtp_addr,ac->addr);
			md->streams[i].rtp_port=ac->port;
			if ((ac->addr[0]!='\0' && vc->addr[0]!='\0' && strcmp(ac->addr,vc->addr)==0) || sal_media_description_get_nb_active_streams(md)==1){
				strcpy(md->addr,ac->addr);
			}
		} else if ((md->streams[i].type == SalVideo) && (vc->port != 0)) {
			strcpy(md->streams[i].rtp_addr,vc->addr);
			md->streams[i].rtp_port=vc->port;
		} else if ((md->streams[i].type == SalText) && (tc->port != 0)) {
			strcpy(md->streams[i].rtp_addr,tc->addr);
			md->streams[i].rtp_port=tc->port;
		}
	}
}

static int setup_encryption_key(SalSrtpCryptoAlgo *crypto, MSCryptoSuite suite, unsigned int tag){
	size_t keylen=0;
	crypto->tag=tag;
	crypto->algo=suite;
	switch(suite){
		case MS_AES_128_SHA1_80:
		case MS_AES_128_SHA1_32:
		case MS_AES_128_NO_AUTH:
		case MS_NO_CIPHER_SHA1_80: /*not sure for this one*/
			keylen=30;
		break;
		case MS_AES_256_SHA1_80:
		case MS_AES_CM_256_SHA1_80:
		case MS_AES_256_SHA1_32:
			keylen=46;
		break;
		case MS_CRYPTO_SUITE_INVALID:
		break;
	}
	if (keylen==0 || !generate_b64_crypto_key(keylen, crypto->master_key, SAL_SRTP_KEY_SIZE)){
		ms_error("Could not generate SRTP key.");
		crypto->algo = 0;
		return -1;
	}
	return 0;
}
static void setup_dtls_keys(LinphoneCall *call, SalMediaDescription *md){
	int i;
	for(i=0; i<SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (!sal_stream_description_active(&md->streams[i])) continue;
		/* if media encryption is set to DTLS check presence of fingerprint in the call which shall have been set at stream init but it may have failed when retrieving certificate resulting in no fingerprint present and then DTLS not usable */
		if (sal_stream_description_has_dtls(&md->streams[i]) == TRUE) {
			strncpy(md->streams[i].dtls_fingerprint, call->dtls_certificate_fingerprint, sizeof(md->streams[i].dtls_fingerprint)); /* get the self fingerprint from call(it's computed at stream init) */
			md->streams[i].dtls_role = SalDtlsRoleUnset; /* if we are offering, SDP will have actpass setup attribute when role is unset, if we are responding the result mediadescription will be set to SalDtlsRoleIsClient */
		} else {
			md->streams[i].dtls_fingerprint[0] = '\0';
			md->streams[i].dtls_role = SalDtlsRoleInvalid;

		}
	}

}
static void setup_encryption_keys(LinphoneCall *call, SalMediaDescription *md){
	LinphoneCore *lc=call->core;
	int i,j;
	SalMediaDescription *old_md=call->localdesc;
	bool_t keep_srtp_keys=lp_config_get_int(lc->config,"sip","keep_srtp_keys",1);

	for(i=0; i<SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (!sal_stream_description_active(&md->streams[i])) continue;
		if (sal_stream_description_has_srtp(&md->streams[i]) == TRUE) {
			if (keep_srtp_keys && old_md && (sal_stream_description_active(&old_md->streams[i]) == TRUE) && (sal_stream_description_has_srtp(&old_md->streams[i]) == TRUE)) {
				int j;
				ms_message("Keeping same crypto keys.");
				for(j=0;j<SAL_CRYPTO_ALGO_MAX;++j){
					memcpy(&md->streams[i].crypto[j],&old_md->streams[i].crypto[j],sizeof(SalSrtpCryptoAlgo));
				}
			}else{
				const MSCryptoSuite *suites=linphone_core_get_srtp_crypto_suites(lc);
				for(j=0;suites!=NULL && suites[j]!=MS_CRYPTO_SUITE_INVALID && j<SAL_CRYPTO_ALGO_MAX;++j){
					setup_encryption_key(&md->streams[i].crypto[j],suites[j],j+1);
				}
			}
		}
	}
}


static void setup_zrtp_hash(LinphoneCall *call, SalMediaDescription *md) {
	int i;
	if (linphone_core_media_encryption_supported(call->core, LinphoneMediaEncryptionZRTP)) { /* set the hello hash for all streams */
		for(i=0; i<SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
			if (!sal_stream_description_active(&md->streams[i])) continue;
			if (call->sessions[i].zrtp_context!=NULL) {
				ms_zrtp_getHelloHash(call->sessions[i].zrtp_context, md->streams[i].zrtphash, 128);
				if (call->params->media_encryption==LinphoneMediaEncryptionZRTP) { /* turn on the flag to use it if ZRTP is set */
					md->streams[i].haveZrtpHash = 1;
				} else {
					md->streams[i].haveZrtpHash = 0;
				}
			} else {
				md->streams[i].haveZrtpHash = 0;
			}
		}
	}
}

static void setup_rtcp_fb(LinphoneCall *call, SalMediaDescription *md) {
	bctbx_list_t *pt_it;
	PayloadType *pt;
	PayloadTypeAvpfParams avpf_params;
	LinphoneCore *lc = call->core;
	int i;

	for (i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (!sal_stream_description_active(&md->streams[i])) continue;
		md->streams[i].rtcp_fb.generic_nack_enabled = lp_config_get_int(lc->config, "rtp", "rtcp_fb_generic_nack_enabled", 0);
		md->streams[i].rtcp_fb.tmmbr_enabled = lp_config_get_int(lc->config, "rtp", "rtcp_fb_tmmbr_enabled", 0);
		md->streams[i].implicit_rtcp_fb = call->params->implicit_rtcp_fb;

		for (pt_it = md->streams[i].payloads; pt_it != NULL; pt_it = pt_it->next) {
			pt = (PayloadType *)pt_it->data;

			if (call->params->avpf_enabled == FALSE && call->params->implicit_rtcp_fb == FALSE)  {
				payload_type_unset_flag(pt, PAYLOAD_TYPE_RTCP_FEEDBACK_ENABLED);
				memset(&avpf_params, 0, sizeof(avpf_params));
			}else {
				payload_type_set_flag(pt, PAYLOAD_TYPE_RTCP_FEEDBACK_ENABLED);
				avpf_params = payload_type_get_avpf_params(pt);
				avpf_params.trr_interval = call->params->avpf_rr_interval;
			}
			payload_type_set_avpf_params(pt, avpf_params);
		}
	}
}

static void setup_rtcp_xr(LinphoneCall *call, SalMediaDescription *md) {
	LinphoneCore *lc = call->core;
	int i;

	md->rtcp_xr.enabled = lp_config_get_int(lc->config, "rtp", "rtcp_xr_enabled", 1);
	if (md->rtcp_xr.enabled == TRUE) {
		const char *rcvr_rtt_mode = lp_config_get_string(lc->config, "rtp", "rtcp_xr_rcvr_rtt_mode", "all");
		if (strcasecmp(rcvr_rtt_mode, "all") == 0) md->rtcp_xr.rcvr_rtt_mode = OrtpRtcpXrRcvrRttAll;
		else if (strcasecmp(rcvr_rtt_mode, "sender") == 0) md->rtcp_xr.rcvr_rtt_mode = OrtpRtcpXrRcvrRttSender;
		else md->rtcp_xr.rcvr_rtt_mode = OrtpRtcpXrRcvrRttNone;
		if (md->rtcp_xr.rcvr_rtt_mode != OrtpRtcpXrRcvrRttNone) {
			md->rtcp_xr.rcvr_rtt_max_size = lp_config_get_int(lc->config, "rtp", "rtcp_xr_rcvr_rtt_max_size", 10000);
		}
		md->rtcp_xr.stat_summary_enabled = lp_config_get_int(lc->config, "rtp", "rtcp_xr_stat_summary_enabled", 1);
		if (md->rtcp_xr.stat_summary_enabled == TRUE) {
			md->rtcp_xr.stat_summary_flags = OrtpRtcpXrStatSummaryLoss | OrtpRtcpXrStatSummaryDup | OrtpRtcpXrStatSummaryJitt | OrtpRtcpXrStatSummaryTTL;
		}
		md->rtcp_xr.voip_metrics_enabled = lp_config_get_int(lc->config, "rtp", "rtcp_xr_voip_metrics_enabled", 1);
	}
	for (i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (!sal_stream_description_active(&md->streams[i])) continue;
		memcpy(&md->streams[i].rtcp_xr, &md->rtcp_xr, sizeof(md->streams[i].rtcp_xr));
	}
}

void linphone_call_increment_local_media_description(LinphoneCall *call){
	SalMediaDescription *md=call->localdesc;
	md->session_ver++;
}

void linphone_call_update_local_media_description_from_ice_or_upnp(LinphoneCall *call){
	LinphoneCore *lc = call->core;
	if (call->ice_session != NULL) {
		/*set this to FALSE once flexisip are updated everywhere, let's say in December 2016.*/
		bool_t use_nortpproxy = lp_config_get_int(lc->config, "sip", "ice_uses_nortpproxy", TRUE);
		_update_local_media_description_from_ice(call->localdesc, call->ice_session, use_nortpproxy);
		linphone_core_update_ice_state_in_call_stats(call);
	}
#ifdef BUILD_UPNP
	if(call->upnp_session != NULL) {
		linphone_core_update_local_media_description_from_upnp(call->localdesc, call->upnp_session);
		linphone_core_update_upnp_state_in_call_stats(call);
	}
#endif  //BUILD_UPNP
}

static void transfer_already_assigned_payload_types(SalMediaDescription *old, SalMediaDescription *md){
	int i;
	for(i=0;i<SAL_MEDIA_DESCRIPTION_MAX_STREAMS;++i){
		md->streams[i].already_assigned_payloads=old->streams[i].already_assigned_payloads;
		old->streams[i].already_assigned_payloads=NULL;
	}
}

static const char *linphone_call_get_bind_ip_for_stream(LinphoneCall *call, int stream_index){
	const char *bind_ip = lp_config_get_string(call->core->config,"rtp","bind_address",
				call->af == AF_INET6 ? "::0" : "0.0.0.0");
	PortConfig *pc = &call->media_ports[stream_index];
	if (pc->multicast_ip[0]!='\0'){
		if (call->dir==LinphoneCallOutgoing){
			/*as multicast sender, we must decide a local interface to use to send multicast, and bind to it*/
			linphone_core_get_local_ip_for(strchr(pc->multicast_ip,':') ? AF_INET6 : AF_INET,
				NULL, pc->multicast_bind_ip);
			bind_ip = pc->multicast_bind_ip;
		}else{
			/*otherwise we shall use an address family of the same family of the multicast address, because
			 * dual stack socket and multicast don't work well on Mac OS (linux is OK, as usual).*/
			bind_ip = strchr(pc->multicast_ip,':') ? "::0" : "0.0.0.0";
		}
	}
	return bind_ip;
}

static const char *linphone_call_get_public_ip_for_stream(LinphoneCall *call, int stream_index){
	const char *public_ip=call->media_localip;

	if (call->media_ports[stream_index].multicast_ip[0]!='\0')
		public_ip=call->media_ports[stream_index].multicast_ip;
	return public_ip;
}

void linphone_call_update_biggest_desc(LinphoneCall *call, SalMediaDescription *md){
	if (call->biggestdesc==NULL || md->nb_streams>call->biggestdesc->nb_streams){
		/*we have been offered and now are ready to proceed, or we added a new stream*/
		/*store the media description to remember the mapping of calls*/
		if (call->biggestdesc){
			sal_media_description_unref(call->biggestdesc);
			call->biggestdesc=NULL;
		}
		call->biggestdesc=sal_media_description_ref(md);
	}
}

static void force_streams_dir_according_to_state(LinphoneCall *call, SalMediaDescription *md){
	int i;

	for (i=0; i<SAL_MEDIA_DESCRIPTION_MAX_STREAMS; ++i){
		SalStreamDescription *sd = &md->streams[i];

		switch (call->state){
			case LinphoneCallPausing:
			case LinphoneCallPaused:
				if (sd->dir != SalStreamInactive) {
					sd->dir = SalStreamSendOnly;
					if (sd->type == SalVideo){
						if (lp_config_get_int(call->core->config, "sip", "inactive_video_on_pause", 0)) {
							sd->dir = SalStreamInactive;
						}
					}
				}
				break;
			default:
				break;
		}

		/* Reflect the stream directions in the call params */
		if (i == call->main_audio_stream_index) {
			linphone_call_params_set_audio_direction(call->current_params, media_direction_from_sal_stream_dir(sd->dir));
		} else if (i == call->main_video_stream_index) {
			linphone_call_params_set_video_direction(call->current_params, media_direction_from_sal_stream_dir(sd->dir));
		}
	}
}

void linphone_call_make_local_media_description(LinphoneCall *call) {
	bctbx_list_t *l;
	SalMediaDescription *old_md=call->localdesc;
	int i;
	int max_index = 0;
	SalMediaDescription *md=sal_media_description_new();
	LinphoneAddress *addr;
	const char *subject;
	CodecConstraints codec_hints={0};
	LinphoneCallParams *params = call->params;
	LinphoneCore *lc = call->core;
	bool_t rtcp_mux = lp_config_get_int(lc->config, "rtp", "rtcp_mux", 0);

	/*multicast is only set in case of outgoing call*/
	if (call->dir == LinphoneCallOutgoing && linphone_call_params_audio_multicast_enabled(params)) {
		md->streams[call->main_audio_stream_index].ttl=linphone_core_get_audio_multicast_ttl(lc);
		md->streams[call->main_audio_stream_index].multicast_role = SalMulticastSender;
	}

	if (call->dir == LinphoneCallOutgoing && linphone_call_params_video_multicast_enabled(params)) {
		md->streams[call->main_video_stream_index].ttl=linphone_core_get_video_multicast_ttl(lc);
		md->streams[call->main_video_stream_index].multicast_role = SalMulticastSender;
	}

	subject=linphone_call_params_get_session_name(params);

	linphone_core_adapt_to_network(lc,call->ping_time,params);

	if (call->dest_proxy) {
		addr=linphone_address_clone(linphone_proxy_config_get_identity_address(call->dest_proxy));
	} else {
		addr=linphone_address_new(linphone_core_get_identity(lc));
	}

	md->session_id=(old_md ? old_md->session_id : (rand() & 0xfff));
	md->session_ver=(old_md ? (old_md->session_ver+1) : (rand() & 0xfff));
	md->nb_streams=(call->biggestdesc ? call->biggestdesc->nb_streams : 1);

	/*re-check local ip address each time we make a new offer, because it may change in case of network reconnection*/
	linphone_call_get_local_ip(call, call->dir == LinphoneCallOutgoing ?  call->log->to : call->log->from);
	strncpy(md->addr,call->media_localip,sizeof(md->addr));
	if (linphone_address_get_username(addr)) /*might be null in case of identity without userinfo*/
		strncpy(md->username,linphone_address_get_username(addr),sizeof(md->username));
	if (subject) strncpy(md->name,subject,sizeof(md->name));

	if (params->down_bw)
		md->bandwidth=params->down_bw;
	else md->bandwidth=linphone_core_get_download_bandwidth(lc);

	if (params->custom_sdp_attributes)
		md->custom_sdp_attributes = sal_custom_sdp_attribute_clone(params->custom_sdp_attributes);

	/*set audio capabilities */

	codec_hints.bandwidth_limit=params->audio_bw;
	codec_hints.max_codecs=-1;
	codec_hints.previously_used=old_md ? old_md->streams[call->main_audio_stream_index].already_assigned_payloads : NULL;
	l=make_codec_list(lc, &codec_hints, SalAudio, lc->codecs_conf.audio_codecs);

	if (params->has_audio && l != NULL) {
		strncpy(md->streams[call->main_audio_stream_index].rtp_addr,linphone_call_get_public_ip_for_stream(call,call->main_audio_stream_index),sizeof(md->streams[call->main_audio_stream_index].rtp_addr));
		strncpy(md->streams[call->main_audio_stream_index].rtcp_addr,linphone_call_get_public_ip_for_stream(call,call->main_audio_stream_index),sizeof(md->streams[call->main_audio_stream_index].rtcp_addr));
		strncpy(md->streams[call->main_audio_stream_index].name,"Audio",sizeof(md->streams[call->main_audio_stream_index].name)-1);
		md->streams[call->main_audio_stream_index].rtp_port=call->media_ports[call->main_audio_stream_index].rtp_port;
		md->streams[call->main_audio_stream_index].rtcp_port=call->media_ports[call->main_audio_stream_index].rtcp_port;
		md->streams[call->main_audio_stream_index].proto=get_proto_from_call_params(params);
		md->streams[call->main_audio_stream_index].dir=get_audio_dir_from_call_params(params);
		md->streams[call->main_audio_stream_index].type=SalAudio;
		md->streams[call->main_audio_stream_index].rtcp_mux = rtcp_mux;
		if (params->down_ptime)
			md->streams[call->main_audio_stream_index].ptime=params->down_ptime;
		else
			md->streams[call->main_audio_stream_index].ptime=linphone_core_get_download_ptime(lc);
		md->streams[call->main_audio_stream_index].max_rate=get_max_codec_sample_rate(l);
		md->streams[call->main_audio_stream_index].payloads=l;
		if (call->audiostream && call->audiostream->ms.sessions.rtp_session) {
			char* me = linphone_address_as_string_uri_only(call->me);
			md->streams[call->main_audio_stream_index].rtp_ssrc=rtp_session_get_send_ssrc(call->audiostream->ms.sessions.rtp_session);
			strncpy(md->streams[call->main_audio_stream_index].rtcp_cname,me,sizeof(md->streams[call->main_audio_stream_index].rtcp_cname));
			ms_free(me);
		}
		else
			ms_warning("Cannot get audio local ssrc for call [%p]",call);
		if (call->main_audio_stream_index > max_index)
			max_index = call->main_audio_stream_index;
	} else {
		ms_message("Don't put audio stream on local offer for call [%p]",call);
		md->streams[call->main_audio_stream_index].dir = SalStreamInactive;
		if(l) l=bctbx_list_free_with_data(l, (void (*)(void *))payload_type_destroy);
	}
	if (params->custom_sdp_media_attributes[LinphoneStreamTypeAudio])
		md->streams[call->main_audio_stream_index].custom_sdp_attributes = sal_custom_sdp_attribute_clone(params->custom_sdp_media_attributes[LinphoneStreamTypeAudio]);

	md->streams[call->main_video_stream_index].proto=md->streams[call->main_audio_stream_index].proto;
	md->streams[call->main_video_stream_index].dir=get_video_dir_from_call_params(params);
	md->streams[call->main_video_stream_index].type=SalVideo;
	md->streams[call->main_video_stream_index].rtcp_mux = rtcp_mux;
	strncpy(md->streams[call->main_video_stream_index].name,"Video",sizeof(md->streams[call->main_video_stream_index].name)-1);

	codec_hints.bandwidth_limit=0;
	codec_hints.max_codecs=-1;
	codec_hints.previously_used=old_md ? old_md->streams[call->main_video_stream_index].already_assigned_payloads : NULL;
	l=make_codec_list(lc, &codec_hints, SalVideo, lc->codecs_conf.video_codecs);

	if (params->has_video && l != NULL){
		strncpy(md->streams[call->main_video_stream_index].rtp_addr,linphone_call_get_public_ip_for_stream(call,call->main_video_stream_index),sizeof(md->streams[call->main_video_stream_index].rtp_addr));
		strncpy(md->streams[call->main_video_stream_index].rtcp_addr,linphone_call_get_public_ip_for_stream(call,call->main_video_stream_index),sizeof(md->streams[call->main_video_stream_index].rtcp_addr));
		md->streams[call->main_video_stream_index].rtp_port=call->media_ports[call->main_video_stream_index].rtp_port;
		md->streams[call->main_video_stream_index].rtcp_port=call->media_ports[call->main_video_stream_index].rtcp_port;
		md->streams[call->main_video_stream_index].payloads=l;
		if (call->videostream && call->videostream->ms.sessions.rtp_session) {
			char* me = linphone_address_as_string_uri_only(call->me);
			md->streams[call->main_video_stream_index].rtp_ssrc=rtp_session_get_send_ssrc(call->videostream->ms.sessions.rtp_session);
			strncpy(md->streams[call->main_video_stream_index].rtcp_cname,me,sizeof(md->streams[call->main_video_stream_index].rtcp_cname));
			ms_free(me);
		}
		else
			ms_warning("Cannot get video local ssrc for call [%p]",call);
		if (call->main_video_stream_index > max_index)
			max_index = call->main_video_stream_index;
	} else {
		ms_message("Don't put video stream on local offer for call [%p]",call);
		md->streams[call->main_video_stream_index].dir = SalStreamInactive;
		if(l) l=bctbx_list_free_with_data(l, (void (*)(void *))payload_type_destroy);
	}
	if (params->custom_sdp_media_attributes[LinphoneStreamTypeVideo])
		md->streams[call->main_video_stream_index].custom_sdp_attributes = sal_custom_sdp_attribute_clone(params->custom_sdp_media_attributes[LinphoneStreamTypeVideo]);

	md->streams[call->main_text_stream_index].proto=md->streams[call->main_audio_stream_index].proto;
	md->streams[call->main_text_stream_index].dir=SalStreamSendRecv;
	md->streams[call->main_text_stream_index].type=SalText;
	md->streams[call->main_text_stream_index].rtcp_mux = rtcp_mux;
	strncpy(md->streams[call->main_text_stream_index].name,"Text",sizeof(md->streams[call->main_text_stream_index].name)-1);
	if (params->realtimetext_enabled) {
		strncpy(md->streams[call->main_text_stream_index].rtp_addr,linphone_call_get_public_ip_for_stream(call,call->main_text_stream_index),sizeof(md->streams[call->main_text_stream_index].rtp_addr));
		strncpy(md->streams[call->main_text_stream_index].rtcp_addr,linphone_call_get_public_ip_for_stream(call,call->main_text_stream_index),sizeof(md->streams[call->main_text_stream_index].rtcp_addr));

		md->streams[call->main_text_stream_index].rtp_port=call->media_ports[call->main_text_stream_index].rtp_port;
		md->streams[call->main_text_stream_index].rtcp_port=call->media_ports[call->main_text_stream_index].rtcp_port;

		codec_hints.bandwidth_limit=0;
		codec_hints.max_codecs=-1;
		codec_hints.previously_used=old_md ? old_md->streams[call->main_text_stream_index].already_assigned_payloads : NULL;
		l=make_codec_list(lc, &codec_hints, SalText, lc->codecs_conf.text_codecs);
		md->streams[call->main_text_stream_index].payloads=l;
		if (call->textstream && call->textstream->ms.sessions.rtp_session) {
			char* me = linphone_address_as_string_uri_only(call->me);
			md->streams[call->main_text_stream_index].rtp_ssrc=rtp_session_get_send_ssrc(call->textstream->ms.sessions.rtp_session);
			strncpy(md->streams[call->main_text_stream_index].rtcp_cname,me,sizeof(md->streams[call->main_text_stream_index].rtcp_cname));
			ms_free(me);
		}
		else
			ms_warning("Cannot get text local ssrc for call [%p]",call);
		if (call->main_text_stream_index > max_index)
			max_index = call->main_text_stream_index;
	} else {
		ms_message("Don't put text stream on local offer for call [%p]",call);
		md->streams[call->main_text_stream_index].dir = SalStreamInactive;
	}
	if (params->custom_sdp_media_attributes[LinphoneStreamTypeText])
		md->streams[call->main_text_stream_index].custom_sdp_attributes = sal_custom_sdp_attribute_clone(params->custom_sdp_media_attributes[LinphoneStreamTypeText]);

	md->nb_streams = MAX(md->nb_streams,max_index+1);

	/* Deactivate unused streams. */
	for (i = md->nb_streams; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (md->streams[i].rtp_port == 0) {
			md->streams[i].dir = SalStreamInactive;
			if (call->biggestdesc && i < call->biggestdesc->nb_streams) {
				md->streams[i].proto = call->biggestdesc->streams[i].proto;
				md->streams[i].type = call->biggestdesc->streams[i].type;
			}
		}
	}
	setup_encryption_keys(call,md);
	setup_dtls_keys(call,md);
	setup_zrtp_hash(call, md);

	setup_rtcp_fb(call, md);
	setup_rtcp_xr(call, md);

	update_media_description_from_stun(md, &call->ac, &call->vc, &call->tc);
	call->localdesc=md;
	linphone_call_update_local_media_description_from_ice_or_upnp(call);
	linphone_address_unref(addr);
	if (old_md){
		transfer_already_assigned_payload_types(old_md,md);
		call->localdesc_changed=sal_media_description_equals(md,old_md);
		sal_media_description_unref(old_md);
		if (call->params->internal_call_update){
			/*
			 * An internal call update (ICE reINVITE) is not expected to modify the actual media stream parameters.
			 * However, the localdesc may change between first INVITE and ICE reINVITE, for example if the remote party has declined a video stream.
			 * We use the internal_call_update flag to prevent trigger an unnecessary media restart.
			 */
			call->localdesc_changed = 0;
		}
	}
	force_streams_dir_according_to_state(call, md);
}

static int find_port_offset(LinphoneCore *lc, int stream_index, int base_port){
	int offset;
	bctbx_list_t *elem;
	int tried_port;
	int existing_port;
	bool_t already_used=FALSE;

	for(offset=0;offset<100;offset+=2){
		tried_port=base_port+offset;
		already_used=FALSE;
		for(elem=lc->calls;elem!=NULL;elem=elem->next){
			LinphoneCall *call=(LinphoneCall*)elem->data;
			existing_port=call->media_ports[stream_index].rtp_port;
			if (existing_port==tried_port) {
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

static int select_random_port(LinphoneCore *lc, int stream_index, int min_port, int max_port) {
	bctbx_list_t *elem;
	int nb_tries;
	int tried_port = 0;
	int existing_port = 0;
	bool_t already_used = FALSE;

	tried_port = (ortp_random() % (max_port - min_port) + min_port) & ~0x1;
	if (tried_port < min_port) tried_port = min_port + 2;
	for (nb_tries = 0; nb_tries < 100; nb_tries++) {
		for (elem = lc->calls; elem != NULL; elem = elem->next) {
			LinphoneCall *call = (LinphoneCall *)elem->data;
			existing_port=call->media_ports[stream_index].rtp_port;
			if (existing_port == tried_port) {
				already_used = TRUE;
				break;
			}
		}
		if (!already_used) break;
	}
	if (nb_tries == 100) {
		ms_error("Could not find any free port!");
		return -1;
	}
	return tried_port;
}

static void port_config_set_random(LinphoneCall *call, int stream_index){
	call->media_ports[stream_index].rtp_port=-1;
	call->media_ports[stream_index].rtcp_port=-1;
}

static void port_config_set(LinphoneCall *call, int stream_index, int min_port, int max_port){
	int port_offset;
	if (min_port>0 && max_port>0){
		if (min_port == max_port) {
			/* Used fixed RTP audio port. */
			port_offset=find_port_offset(call->core, stream_index, min_port);
			if (port_offset==-1) {
				port_config_set_random(call, stream_index);
				return;
			}
			call->media_ports[stream_index].rtp_port=min_port+port_offset;
		} else {
			/* Select random RTP audio port in the specified range. */
			call->media_ports[stream_index].rtp_port = select_random_port(call->core, stream_index, min_port, max_port);
		}
		call->media_ports[stream_index].rtcp_port=call->media_ports[stream_index].rtp_port+1;
	}else port_config_set_random(call,stream_index);
}

static void linphone_call_init_common(LinphoneCall *call, LinphoneAddress *from, LinphoneAddress *to){
	int min_port, max_port;
	ms_message("New LinphoneCall [%p] initialized (LinphoneCore version: %s)",call,linphone_core_get_version());
	call->core->send_call_stats_periodical_updates = lp_config_get_int(call->core->config, "misc", "send_call_stats_periodical_updates", 0);
	call->main_audio_stream_index = LINPHONE_CALL_STATS_AUDIO;
	call->main_video_stream_index = LINPHONE_CALL_STATS_VIDEO;
	call->main_text_stream_index = LINPHONE_CALL_STATS_TEXT;
	call->state=LinphoneCallIdle;
	call->transfer_state = LinphoneCallIdle;
	call->log=linphone_call_log_new(call->dir, from, to);
	call->camera_enabled=TRUE;
	call->current_params = linphone_call_params_new();
	call->current_params->media_encryption=LinphoneMediaEncryptionNone;
	call->dtls_certificate_fingerprint = NULL;
	if (call->dir == LinphoneCallIncoming)
		call->me=to;
	 else
		call->me=from;
	linphone_address_ref(call->me);

	linphone_core_get_audio_port_range(call->core, &min_port, &max_port);
	port_config_set(call,call->main_audio_stream_index,min_port,max_port);

	linphone_core_get_video_port_range(call->core, &min_port, &max_port);
	port_config_set(call,call->main_video_stream_index,min_port,max_port);

	linphone_core_get_text_port_range(call->core, &min_port, &max_port);
	port_config_set(call,call->main_text_stream_index,min_port,max_port);

	linphone_call_init_stats(&call->stats[LINPHONE_CALL_STATS_AUDIO], LINPHONE_CALL_STATS_AUDIO);
	linphone_call_init_stats(&call->stats[LINPHONE_CALL_STATS_VIDEO], LINPHONE_CALL_STATS_VIDEO);
	linphone_call_init_stats(&call->stats[LINPHONE_CALL_STATS_TEXT], LINPHONE_CALL_STATS_TEXT);
}

void linphone_call_init_stats(LinphoneCallStats *stats, int type) {
	stats->type = type;
	stats->received_rtcp = NULL;
	stats->sent_rtcp = NULL;
	stats->ice_state = LinphoneIceStateNotActivated;
#ifdef BUILD_UPNP
	stats->upnp_state = LinphoneUpnpStateIdle;
#else
	stats->upnp_state = LinphoneUpnpStateNotAvailable;
#endif //BUILD_UPNP
}

static void discover_mtu(LinphoneCore *lc, const char *remote){
	int mtu;
	if (lc->net_conf.mtu==0	){
		/*attempt to discover mtu*/
		mtu=ms_discover_mtu(remote);
		if (mtu>0){
			ms_factory_set_mtu(lc->factory, mtu);
			ms_message("Discovered mtu is %i, RTP payload max size is %i",
				mtu, ms_factory_get_payload_max_size(lc->factory));
		}
	}
}

void linphone_call_create_op(LinphoneCall *call){
	if (call->op) sal_op_release(call->op);
	call->op=sal_op_new(call->core->sal);
	sal_op_set_user_pointer(call->op,call);
	if (call->params->referer)
		sal_call_set_referer(call->op,call->params->referer->op);
	linphone_configure_op(call->core,call->op,call->log->to,call->params->custom_headers,FALSE);
	if (call->params->privacy != LinphonePrivacyDefault)
		sal_op_set_privacy(call->op,(SalPrivacyMask)call->params->privacy);
	/*else privacy might be set by proxy */
}

/*
 * Choose IP version we are going to use for RTP streams IP address advertised in SDP.
 * The algorithm is as follows:
 * - if ipv6 is disabled at the core level, it is always AF_INET
 * - Otherwise, if the destination address for the call is an IPv6 address, use IPv6.
 * - Otherwise, if the call is done through a known proxy config, then use the information obtained during REGISTER
 * to know if IPv6 is supported by the server.
**/
static void linphone_call_outgoing_select_ip_version(LinphoneCall *call, LinphoneAddress *to, LinphoneProxyConfig *cfg){
	if (linphone_core_ipv6_enabled(call->core)){
		if (sal_address_is_ipv6((SalAddress*)to)){
			call->af=AF_INET6;
		}else if (cfg && cfg->op){
			call->af=sal_op_get_address_family(cfg->op);
		}else{
			call->af=AF_UNSPEC;
		}
		if (call->af == AF_UNSPEC) {
			char ipv4[LINPHONE_IPADDR_SIZE];
			char ipv6[LINPHONE_IPADDR_SIZE];
			bool_t have_ipv6 = FALSE;
			bool_t have_ipv4 = FALSE;
			/*check connectivity for IPv4 and IPv6*/
			if (linphone_core_get_local_ip_for(AF_INET6, NULL, ipv6) == 0){
				have_ipv6 = TRUE;
			}
			if (linphone_core_get_local_ip_for(AF_INET, NULL, ipv4) == 0){
				have_ipv4 = TRUE;
			}
			if (have_ipv6){
				if (!have_ipv4) {
					call->af = AF_INET6;
				}else if (lp_config_get_int(call->core->config, "rtp", "prefer_ipv6", 1)){ /*this property tells whether ipv6 is prefered if two versions are available*/
					call->af = AF_INET6;
				}else{
					call->af = AF_INET;
				}
			}else call->af = AF_INET;
			/*fill the media_localip default value since we have it here*/
			strncpy(call->media_localip,call->af == AF_INET6 ? ipv6 : ipv4, LINPHONE_IPADDR_SIZE);
		}
	}else call->af=AF_INET;
}

/**
 * Fill the local ip that routes to the internet according to the destination, or guess it by other special means (upnp).
 */
static void linphone_call_get_local_ip(LinphoneCall *call, const LinphoneAddress *remote_addr){
	const char *ip = NULL;
	int af = call->af;
	const char *dest = NULL;

	if (linphone_core_get_firewall_policy(call->core)==LinphonePolicyUseNatAddress
		&& (ip=linphone_core_get_nat_address_resolved(call->core))!=NULL){
		strncpy(call->media_localip,ip,LINPHONE_IPADDR_SIZE);
		return;
	}
#ifdef BUILD_UPNP
	else if (call->core->upnp != NULL && linphone_core_get_firewall_policy(call->core)==LinphonePolicyUseUpnp &&
			linphone_upnp_context_get_state(call->core->upnp) == LinphoneUpnpStateOk) {
		ip = linphone_upnp_context_get_external_ipaddress(call->core->upnp);
		strncpy(call->media_localip,ip,LINPHONE_IPADDR_SIZE);
		goto found;
	}
#endif //BUILD_UPNP

	/*next, sometime, override from config*/
	if ((ip=lp_config_get_string(call->core->config,"rtp","bind_address",NULL)) != NULL)
		goto found;

	/*if a known proxy was identified for this call, then we may have a chance to take the local ip address
	* from the socket that connect to this proxy */
	if (call->dest_proxy && call->dest_proxy->op){
		if ((ip = sal_op_get_local_address(call->dest_proxy->op, NULL)) != NULL){
			ms_message("Found media local-ip from signaling.");
			goto found;
		}
	}

	/*in last resort, attempt to find the local ip that routes to destination if given as an IP address,
	 or the default route (dest=NULL)*/
	if (call->dest_proxy == NULL) {
		struct addrinfo hints;
		struct addrinfo *res = NULL;
		int err;
		/*FIXME the following doesn't work for IPv6 address because of brakets*/
		const char *domain = linphone_address_get_domain(remote_addr);
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = AI_NUMERICHOST;
		err = getaddrinfo(domain, NULL, &hints, &res);
		if (err == 0) {
			dest = domain;
		}
		if (res != NULL) freeaddrinfo(res);
	}

	if (dest != NULL || call->media_localip[0] == '\0' || call->need_localip_refresh){
		call->need_localip_refresh = FALSE;
		linphone_core_get_local_ip(call->core, af, dest, call->media_localip);
	}
	return;
found:
	strncpy(call->media_localip,ip,LINPHONE_IPADDR_SIZE);
}

static void linphone_call_destroy(LinphoneCall *obj);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneCall);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneCall, belle_sip_object_t,
	(belle_sip_object_destroy_t)linphone_call_destroy,
	NULL, // clone
	NULL, // marshal
	FALSE
);

void linphone_call_fill_media_multicast_addr(LinphoneCall *call) {
	if (linphone_call_params_audio_multicast_enabled(call->params)){
		strncpy(call->media_ports[call->main_audio_stream_index].multicast_ip,
				linphone_core_get_audio_multicast_addr(call->core), sizeof(call->media_ports[call->main_audio_stream_index].multicast_ip));
	} else
		call->media_ports[call->main_audio_stream_index].multicast_ip[0]='\0';

	if (linphone_call_params_video_multicast_enabled(call->params)){
		strncpy(call->media_ports[call->main_video_stream_index].multicast_ip,
				linphone_core_get_video_multicast_addr(call->core), sizeof(call->media_ports[call->main_video_stream_index].multicast_ip));
	} else
		call->media_ports[call->main_video_stream_index].multicast_ip[0]='\0';
}

void linphone_call_check_ice_session(LinphoneCall *call, IceRole role, bool_t is_reinvite){
	if (call->ice_session) return; /*already created*/

	if (!linphone_nat_policy_ice_enabled(linphone_core_get_nat_policy(call->core))){
		return;
	}

	if (is_reinvite && lp_config_get_int(call->core->config, "net", "allow_late_ice", 0) == 0) return;

	call->ice_session = ice_session_new();
	/*for backward compatibility purposes, shall be enabled by default in futur*/
	ice_session_enable_message_integrity_check(call->ice_session,lp_config_get_int(call->core->config,"net","ice_session_enable_message_integrity_check",1));
	if (lp_config_get_int(call->core->config, "net", "dont_default_to_stun_candidates", 0)){
		IceCandidateType types[ICT_CandidateTypeMax];
		types[0] = ICT_HostCandidate;
		types[1] = ICT_RelayedCandidate;
		types[2] = ICT_CandidateInvalid;
		ice_session_set_default_candidates_types(call->ice_session, types);
	}
	ice_session_set_role(call->ice_session, role);
}

LinphoneCall * linphone_call_new_outgoing(struct _LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to, const LinphoneCallParams *params, LinphoneProxyConfig *cfg){
	LinphoneCall *call = belle_sip_object_new(LinphoneCall);
	call->dir=LinphoneCallOutgoing;
	call->core=lc;
	call->dest_proxy=cfg;
	linphone_call_outgoing_select_ip_version(call,to,cfg);
	linphone_call_get_local_ip(call, to);
	call->params = linphone_call_params_copy(params);
	linphone_call_init_common(call, from, to);

	call->current_params->update_call_when_ice_completed = call->params->update_call_when_ice_completed; /*copy param*/

	linphone_call_fill_media_multicast_addr(call);

	linphone_call_check_ice_session(call, IR_Controlling, FALSE);

	if (linphone_core_get_firewall_policy(call->core) == LinphonePolicyUseStun) {
		call->ping_time=linphone_core_run_stun_tests(call->core,call);
	}
#ifdef BUILD_UPNP
	if (linphone_core_get_firewall_policy(call->core) == LinphonePolicyUseUpnp) {
		if(!lc->rtp_conf.disable_upnp) {
			call->upnp_session = linphone_upnp_session_new(call);
		}
	}
#endif //BUILD_UPNP

	discover_mtu(lc,linphone_address_get_domain (to));
	if (params->referer){
		call->referer=linphone_call_ref(params->referer);
	}

	linphone_call_create_op(call);
	return call;
}

/*Select IP version to use for advertising local addresses of RTP streams, for an incoming call.
 *If the call is received through a know proxy that is IPv6, use IPv6.
 *Otherwise check the remote contact address.
 *If later the resulting media description tells that we have to send IPv4, it won't be a problem because the RTP sockets
 * are dual stack.
 */
static void linphone_call_incoming_select_ip_version(LinphoneCall *call, LinphoneProxyConfig *cfg){
	if (linphone_core_ipv6_enabled(call->core)){
		if (cfg && cfg->op){
			call->af=sal_op_get_address_family(cfg->op);
		}else{
			call->af=sal_op_get_address_family(call->op);
		}
	}else call->af=AF_INET;
}

/**
 * Fix call parameters on incoming call to eg. enable AVPF if the incoming call propose it and it is not enabled locally.
 */
void linphone_call_set_compatible_incoming_call_parameters(LinphoneCall *call, SalMediaDescription *md) {
	/* Handle AVPF, SRTP and DTLS. */
	call->params->avpf_enabled = sal_media_description_has_avpf(md);
	if (call->dest_proxy != NULL) {
		call->params->avpf_rr_interval = linphone_proxy_config_get_avpf_rr_interval(call->dest_proxy) * 1000;
	} else {
		call->params->avpf_rr_interval = linphone_core_get_avpf_rr_interval(call->core)*1000;
	}

	if ((sal_media_description_has_zrtp(md) == TRUE) && (linphone_core_media_encryption_supported(call->core, LinphoneMediaEncryptionZRTP) == TRUE)) {
		call->params->media_encryption = LinphoneMediaEncryptionZRTP;
	}else if ((sal_media_description_has_dtls(md) == TRUE) && (media_stream_dtls_supported() == TRUE)) {
		call->params->media_encryption = LinphoneMediaEncryptionDTLS;
	}else if ((sal_media_description_has_srtp(md) == TRUE) && (ms_srtp_supported() == TRUE)) {
		call->params->media_encryption = LinphoneMediaEncryptionSRTP;
	}else if (call->params->media_encryption != LinphoneMediaEncryptionZRTP){
		call->params->media_encryption = LinphoneMediaEncryptionNone;
	}

	/*in case of nat64, even ipv4 addresses are reachable from v6. Should be enhanced to manage stream by stream connectivity (I.E v6 or v4)*/
	/*if (!sal_media_description_has_ipv6(md)){
		ms_message("The remote SDP doesn't seem to offer any IPv6 connectivity, so disabling IPv6 for this call.");
		call->af = AF_INET;
	}*/
	linphone_call_fix_call_parameters(call, md);
}

static void linphone_call_compute_streams_indexes(LinphoneCall *call, const SalMediaDescription *md) {
	int i, j;
	bool_t audio_found = FALSE, video_found = FALSE, text_found = FALSE;

	for (i = 0; i < md->nb_streams; i++) {
		if (md->streams[i].type == SalAudio) {
			if (!audio_found) {
				call->main_audio_stream_index = i;
				audio_found = TRUE;
				ms_message("audio stream index found: %i, updating main audio stream index", i);
			} else {
				ms_message("audio stream index found: %i, but main audio stream already set to %i", i, call->main_audio_stream_index);
			}

			// Check that the default value of a another stream doesn't match the new one
			if (i == call->main_video_stream_index) {
				for (j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; j++) {
					if (sal_stream_description_active(&md->streams[j])) continue;
					if (j != call->main_video_stream_index && j != call->main_text_stream_index) {
						ms_message("%i was used for video stream ; now using %i", i, j);
						call->main_video_stream_index = j;
						break;
					}
				}
			}
			if (i == call->main_text_stream_index) {
				for (j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; j++) {
					if (sal_stream_description_active(&md->streams[j])) continue;
					if (j != call->main_video_stream_index && j != call->main_text_stream_index) {
						ms_message("%i was used for text stream ; now using %i", i, j);
						call->main_text_stream_index = j;
						break;
					}
				}
			}
		} else if (md->streams[i].type == SalVideo) {
			if (!video_found) {
				call->main_video_stream_index = i;
				video_found = TRUE;
				ms_message("video stream index found: %i, updating main video stream index", i);
			} else {
				ms_message("video stream index found: %i, but main video stream already set to %i", i, call->main_video_stream_index);
			}

			// Check that the default value of a another stream doesn't match the new one
			if (i == call->main_audio_stream_index) {
				for (j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; j++) {
					if (sal_stream_description_active(&md->streams[j])) continue;
					if (j != call->main_audio_stream_index && j != call->main_text_stream_index) {
						ms_message("%i was used for audio stream ; now using %i", i, j);
						call->main_audio_stream_index = j;
						break;
					}
				}
			}
			if (i == call->main_text_stream_index) {
				for (j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; j++) {
					if (sal_stream_description_active(&md->streams[j])) continue;
					if (j != call->main_audio_stream_index && j != call->main_text_stream_index) {
						ms_message("%i was used for text stream ; now using %i", i, j);
						call->main_text_stream_index = j;
						break;
					}
				}
			}
		} else if (md->streams[i].type == SalText) {
			if (!text_found) {
				call->main_text_stream_index = i;
				text_found = TRUE;
				ms_message("text stream index found: %i, updating main text stream index", i);
			} else {
				ms_message("text stream index found: %i, but main text stream already set to %i", i, call->main_text_stream_index);
			}

			// Check that the default value of a another stream doesn't match the new one
			if (i == call->main_audio_stream_index) {
				for (j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; j++) {
					if (sal_stream_description_active(&md->streams[j])) continue;
					if (j != call->main_video_stream_index && j != call->main_audio_stream_index) {
						ms_message("%i was used for audio stream ; now using %i", i, j);
						call->main_audio_stream_index = j;
						break;
					}
				}
			}
			if (i == call->main_video_stream_index) {
				for (j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; j++) {
					if (sal_stream_description_active(&md->streams[j])) continue;
					if (j != call->main_video_stream_index && j != call->main_audio_stream_index) {
						ms_message("%i was used for video stream ; now using %i", i, j);
						call->main_video_stream_index = j;
						break;
					}
				}
			}
		}
	}
}

LinphoneCall * linphone_call_new_incoming(LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to, SalOp *op){
	LinphoneCall *call = belle_sip_object_new(LinphoneCall);
	SalMediaDescription *md;
	LinphoneNatPolicy *nat_policy = NULL;
	int i;
	call->dir=LinphoneCallIncoming;
	sal_op_set_user_pointer(op,call);
	call->op=op;
	call->core=lc;

	call->dest_proxy = linphone_core_lookup_known_proxy(call->core, to);
	linphone_call_incoming_select_ip_version(call, call->dest_proxy);
	/*note that the choice of IP version for streams is later refined by
	 * linphone_call_set_compatible_incoming_call_parameters() when examining the remote offer, if any.
	 * If the remote offer contains IPv4 addresses, we should propose IPv4 as well*/

	sal_op_cnx_ip_to_0000_if_sendonly_enable(op,lp_config_get_default_int(lc->config,"sip","cnx_ip_to_0000_if_sendonly_enabled",0));

	md = sal_call_get_remote_media_description(op);

	if (lc->sip_conf.ping_with_options){
#ifdef BUILD_UPNP
		if (lc->upnp != NULL && linphone_core_get_firewall_policy(lc)==LinphonePolicyUseUpnp &&
			linphone_upnp_context_get_state(lc->upnp) == LinphoneUpnpStateOk) {
#else //BUILD_UPNP
		{
#endif //BUILD_UPNP
			/*the following sends an option request back to the caller so that
			 we get a chance to discover our nat'd address before answering.*/
			call->ping_op=sal_op_new(lc->sal);

			linphone_configure_op(lc, call->ping_op, from, NULL, FALSE);

			sal_op_set_route(call->ping_op,sal_op_get_network_origin(op));
			sal_op_set_user_pointer(call->ping_op,call);

			sal_ping(call->ping_op,sal_op_get_from(call->ping_op), sal_op_get_to(call->ping_op));
		}
	}

	linphone_address_clean(from);
	linphone_call_get_local_ip(call, from);
	call->params = linphone_call_params_new();
	linphone_call_init_common(call, from, to);
	call->log->call_id=ms_strdup(sal_op_get_call_id(op)); /*must be known at that time*/
	linphone_core_init_default_params(lc, call->params);

	/*
	 * Initialize call parameters according to incoming call parameters. This is to avoid to ask later (during reINVITEs) for features that the remote
	 * end apparently does not support. This features are: privacy, video
	 */
	/*set privacy*/
	call->current_params->privacy=(LinphonePrivacyMask)sal_op_get_privacy(call->op);
	/*config params*/
	call->current_params->update_call_when_ice_completed = call->params->update_call_when_ice_completed; /*copy config params*/

	/*set video support */
	call->params->has_video = linphone_core_video_enabled(lc) && lc->video_policy.automatically_accept;
	if (md) {
		// It is licit to receive an INVITE without SDP
		// In this case WE chose the media parameters according to policy.
		linphone_call_set_compatible_incoming_call_parameters(call, md);
		/* set multicast role & address if any*/
		if (!sal_call_is_offerer(op)){
			for (i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
				if (md->streams[i].dir == SalStreamInactive) {
					continue;
				}

				if (md->streams[i].rtp_addr[0]!='\0' && ms_is_multicast(md->streams[i].rtp_addr)){
					md->streams[i].multicast_role = SalMulticastReceiver;
					strncpy(call->media_ports[i].multicast_ip,md->streams[i].rtp_addr,sizeof(call->media_ports[i].multicast_ip));
				}
			}
		}
	}

	if (call->dest_proxy != NULL) nat_policy = linphone_proxy_config_get_nat_policy(call->dest_proxy);
	if (nat_policy == NULL) nat_policy = linphone_core_get_nat_policy(call->core);
	if ((nat_policy != NULL) && linphone_nat_policy_ice_enabled(nat_policy)) {
		/* Create the ice session now if ICE is required */
		if (md){
			linphone_call_check_ice_session(call, IR_Controlled, FALSE);
		}else{
			nat_policy = NULL;
			ms_warning("ICE not supported for incoming INVITE without SDP.");
		}
	}

	/*reserve the sockets immediately*/
	linphone_call_init_media_streams(call);
	if (nat_policy != NULL) {
		if (linphone_nat_policy_ice_enabled(nat_policy)) {
			call->defer_notify_incoming = linphone_call_prepare_ice(call,TRUE) == 1;
		} else if (linphone_nat_policy_stun_enabled(nat_policy)) {
			call->ping_time=linphone_core_run_stun_tests(call->core,call);
		} else if (linphone_nat_policy_upnp_enabled(nat_policy)) {
#ifdef BUILD_UPNP
			if(!lc->rtp_conf.disable_upnp) {
				call->upnp_session = linphone_upnp_session_new(call);
				if (call->upnp_session != NULL) {
					if (linphone_core_update_upnp_from_remote_media_description(call, sal_call_get_remote_media_description(op))<0) {
						/* uPnP port mappings failed, proceed with the call anyway. */
						linphone_call_delete_upnp_session(call);
					}
				}
			}
#endif //BUILD_UPNP
		}
	}

	discover_mtu(lc,linphone_address_get_domain(from));
	return call;
}

/*
 * Frees the media resources of the call.
 * This has to be done at the earliest, unlike signaling resources that sometimes need to be kept a bit more longer.
 * It is called by linphone_call_set_terminated() (for termination of calls signaled to the application), or directly by the destructor of LinphoneCall
 * (_linphone_call_destroy) if the call was never notified to the application.
 */
static void linphone_call_free_media_resources(LinphoneCall *call){
	int i;

	linphone_call_stop_media_streams(call);
	linphone_call_delete_upnp_session(call);
	linphone_call_delete_ice_session(call);
	for (i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; ++i){
		ms_media_stream_sessions_uninit(&call->sessions[i]);
	}
	linphone_call_stats_uninit(&call->stats[LINPHONE_CALL_STATS_AUDIO]);
	linphone_call_stats_uninit(&call->stats[LINPHONE_CALL_STATS_VIDEO]);
	linphone_call_stats_uninit(&call->stats[LINPHONE_CALL_STATS_TEXT]);
}

/*
 * Called internally when reaching the Released state, to perform cleanups to break circular references.
**/
static void linphone_call_set_released(LinphoneCall *call){
	if (call->op!=NULL) {
		/*transfer the last error so that it can be obtained even in Released state*/
		if (call->non_op_error.reason==SalReasonNone){
			const SalErrorInfo *ei=sal_op_get_error_info(call->op);
			sal_error_info_set(&call->non_op_error,ei->reason,ei->protocol_code,ei->status_string,ei->warnings);
		}
		/* so that we cannot have anymore upcalls for SAL
			concerning this call*/
		sal_op_release(call->op);
		call->op=NULL;
	}
	/*it is necessary to reset pointers to other call to prevent circular references that would result in memory never freed.*/
	if (call->referer){
		linphone_call_unref(call->referer);
		call->referer=NULL;
	}
	if (call->transfer_target){
		linphone_call_unref(call->transfer_target);
		call->transfer_target=NULL;
	}
	if (call->chat_room){
		linphone_chat_room_unref(call->chat_room);
		call->chat_room = NULL;
	}
	linphone_call_unref(call);
}

/* this function is called internally to get rid of a call that was notified to the application, because it reached the end or error state.
 It performs the following tasks:
 - remove the call from the internal list of calls
 - update the call logs accordingly
*/
static void linphone_call_set_terminated(LinphoneCall *call){
	LinphoneCore *lc=call->core;

	linphone_call_free_media_resources(call);
	linphone_call_log_completed(call);

	if (call == lc->current_call){
		ms_message("Resetting the current call");
		lc->current_call=NULL;
	}

	if (linphone_core_del_call(lc,call) != 0){
		ms_error("Could not remove the call from the list !!!");
	}
	if(lc->conf_ctx) linphone_conference_on_call_terminating(lc->conf_ctx, call);
	if (call->ringing_beep){
		linphone_core_stop_dtmf(lc);
		call->ringing_beep=FALSE;
	}
	if (call->chat_room){
		call->chat_room->call = NULL;
	}
}

/*function to be called at each incoming reINVITE, in order to adjust various local parameters to what is being offered by remote:
 * - the video enablement parameter according to what is offered and our local policy.
 * Fixing the call->params to proper values avoid request video by accident during internal call updates, pauses and resumes
 * - the stream indexes.
 */
void linphone_call_fix_call_parameters(LinphoneCall *call, SalMediaDescription *rmd){
	const LinphoneCallParams* rcp;

	if (rmd) {
		linphone_call_compute_streams_indexes(call, rmd);
		linphone_call_update_biggest_desc(call, rmd);
		/* Why disabling implicit_rtcp_fb ? It is a local policy choice actually. It doesn't disturb to propose it again and again
		 * even if the other end apparently doesn't support it.
		 * The following line of code is causing trouble, while for example making an audio call, then adding video.
		 * Due to the 200Ok response of the audio-only offer where no rtcp-fb attribute is present, implicit_rtcp_fb is set to
		 * FALSE, which is then preventing it to be eventually used when video is later added to the call.
		 * I did the choice of commenting it out.
		 */
		/*call->params->implicit_rtcp_fb &= sal_media_description_has_implicit_avpf(rmd);*/
	}
	rcp = linphone_call_get_remote_params(call);
	if (rcp){
		if (call->params->has_audio && !rcp->has_audio){
			ms_message("Call [%p]: disabling audio in our call params because the remote doesn't want it.", call);
			call->params->has_audio = FALSE;
		}
		if (call->params->has_video && !rcp->has_video){
			ms_message("Call [%p]: disabling video in our call params because the remote doesn't want it.", call);
			call->params->has_video = FALSE;
		}

		if (rcp->has_video && call->core->video_policy.automatically_accept && linphone_core_video_enabled(call->core) && !call->params->has_video){
			ms_message("Call [%p]: re-enabling video in our call params because the remote wants it and the policy allows to automatically accept.", call);
			linphone_call_params_enable_video(call->params, TRUE);
		}

		if (rcp->realtimetext_enabled && !call->params->realtimetext_enabled) {
			call->params->realtimetext_enabled = TRUE;
		}
	}
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
		case LinphoneCallUpdating:
			return "LinphoneCallUpdating";
		case LinphoneCallReleased:
			return "LinphoneCallReleased";
		case LinphoneCallEarlyUpdatedByRemote:
			return "LinphoneCallEarlyUpdatedByRemote";
		case LinphoneCallEarlyUpdating:
			return "LinphoneCallEarlyUpdating";
	}
	return "undefined state";
}

void linphone_call_set_state(LinphoneCall *call, LinphoneCallState cstate, const char *message){
	LinphoneCore *lc=call->core;

	if (call->state!=cstate){
		call->prevstate=call->state;
		if (call->state==LinphoneCallEnd || call->state==LinphoneCallError){
			if (cstate!=LinphoneCallReleased){
				ms_fatal("Spurious call state change from %s to %s, ignored."	,linphone_call_state_to_string(call->state)
																				,linphone_call_state_to_string(cstate));
				return;
			}
		}
		ms_message("Call %p: moving from state %s to %s",call
														,linphone_call_state_to_string(call->state)
														,linphone_call_state_to_string(cstate));

		if (cstate!=LinphoneCallRefered){
			/*LinphoneCallRefered is rather an event, not a state.
			 Indeed it does not change the state of the call (still paused or running)*/
			call->state=cstate;
		}

		switch (cstate) {
		case LinphoneCallOutgoingInit:
		case LinphoneCallIncomingReceived:
#ifdef ANDROID
			ms_message("Call [%p] acquires both wifi and multicast lock",call);
			linphone_core_wifi_lock_acquire(call->core);
			linphone_core_multicast_lock_acquire(call->core); /*does no affect battery more than regular rtp traffic*/
#endif
			break;
		case LinphoneCallEnd:
		case LinphoneCallError:
			switch(linphone_error_info_get_reason(linphone_call_get_error_info(call))) {
			case LinphoneReasonDeclined:
				call->log->status=LinphoneCallDeclined;
				break;
			case LinphoneReasonNotAnswered:
				call->log->status=LinphoneCallMissed;
				break;
			default:
				break;
			}
			linphone_call_set_terminated(call);
			break;
		case LinphoneCallConnected:
			call->log->status=LinphoneCallSuccess;
			call->log->connected_date_time = ms_time(NULL);
			break;
		case LinphoneCallReleased:
#ifdef ANDROID
			ms_message("Call [%p] releases wifi/multicast lock",call);
			linphone_core_wifi_lock_release(call->core);
			linphone_core_multicast_lock_release(call->core);
#endif
			break;
		case LinphoneCallStreamsRunning:
			if (call->prevstate == LinphoneCallUpdating || call->prevstate == LinphoneCallUpdatedByRemote) {
				LinphoneReason reason = linphone_call_get_reason(call);
				char *msg;
				if (reason != LinphoneReasonNone) {
					msg = ms_strdup_printf(_("Call parameters could not be modified: %s."), linphone_reason_to_string(reason));
				} else {
					msg = ms_strdup(_("Call parameters were successfully modified."));
				}
				linphone_core_notify_display_status(lc, msg);
				ms_free(msg);
			}
			break;
		default:
			break;
		}

		if(cstate!=LinphoneCallStreamsRunning) {
			if (call->dtmfs_timer!=NULL){
				/*cancelling DTMF sequence, if any*/
				linphone_call_cancel_dtmfs(call);
			}
		}
		if (!message) {
			ms_error("%s(): You must fill a reason when changing call state (from %s o %s)."
					, __FUNCTION__
					, linphone_call_state_to_string(call->prevstate)
					, linphone_call_state_to_string(call->state));
		}
		linphone_core_notify_call_state_changed(lc,call,cstate,message?message:"");
		linphone_reporting_call_state_updated(call);
		if (cstate==LinphoneCallReleased) {/*shall be performed after  app notification*/
			linphone_call_set_released(call);
		}
	}
}

static void linphone_call_destroy(LinphoneCall *obj){
	ms_message("Call [%p] freed.",obj);
	if (obj->audiostream || obj->videostream){
		linphone_call_free_media_resources(obj);
	}
	if (obj->op!=NULL) {
		sal_op_release(obj->op);
		obj->op=NULL;
	}
	if (obj->biggestdesc!=NULL){
		sal_media_description_unref(obj->biggestdesc);
		obj->biggestdesc=NULL;
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
		obj->ping_op=NULL;
	}
	if (obj->refer_to){
		ms_free(obj->refer_to);
		obj->refer_to=NULL;
	}
	if (obj->referer){
		linphone_call_unref(obj->referer);
		obj->referer=NULL;
	}
	if (obj->transfer_target){
		linphone_call_unref(obj->transfer_target);
		obj->transfer_target=NULL;
	}
	if (obj->log) {
		linphone_call_log_unref(obj->log);
		obj->log=NULL;
	}
	if (obj->auth_token) {
		ms_free(obj->auth_token);
		obj->auth_token=NULL;
	}
	if (obj->dtls_certificate_fingerprint) {
		ms_free(obj->dtls_certificate_fingerprint);
		obj->dtls_certificate_fingerprint=NULL;
	}
	if (obj->dtmfs_timer) {
		linphone_call_cancel_dtmfs(obj);
	}
	if (obj->params){
		linphone_call_params_unref(obj->params);
		obj->params=NULL;
	}
	if (obj->current_params){
		linphone_call_params_unref(obj->current_params);
		obj->current_params=NULL;
	}
	if (obj->remote_params != NULL) {
		linphone_call_params_unref(obj->remote_params);
		obj->remote_params=NULL;
	}
	if (obj->me) {
		linphone_address_unref(obj->me);
		obj->me = NULL;
	}
	if (obj->onhold_file) ms_free(obj->onhold_file);

	sal_error_info_reset(&obj->non_op_error);
}

LinphoneCall * linphone_call_ref(LinphoneCall *obj){
	belle_sip_object_ref(obj);
	return obj;
}

void linphone_call_unref(LinphoneCall *obj){
	belle_sip_object_unref(obj);
}

static unsigned int linphone_call_get_n_active_streams(const LinphoneCall *call) {
	SalMediaDescription *md=NULL;
	if (call->op)
		md = sal_call_get_remote_media_description(call->op);
	if (!md)
		return 0;
	return sal_media_description_nb_active_streams_of_type(md, SalAudio) + sal_media_description_nb_active_streams_of_type(md, SalVideo) + sal_media_description_nb_active_streams_of_type(md, SalText);
}

const LinphoneCallParams * linphone_call_get_current_params(LinphoneCall *call){
	SalMediaDescription *md=call->resultdesc;
	int all_streams_encrypted = 0;
#ifdef VIDEO_ENABLED
	VideoStream *vstream;
#endif
	MS_VIDEO_SIZE_ASSIGN(call->current_params->sent_vsize, UNKNOWN);
	MS_VIDEO_SIZE_ASSIGN(call->current_params->recv_vsize, UNKNOWN);
#ifdef VIDEO_ENABLED
	vstream = call->videostream;
	if (vstream != NULL) {
		call->current_params->sent_vsize = video_stream_get_sent_video_size(vstream);
		call->current_params->recv_vsize = video_stream_get_received_video_size(vstream);
		call->current_params->sent_fps = video_stream_get_sent_framerate(vstream);
		call->current_params->received_fps = video_stream_get_received_framerate(vstream);
	}
#endif

	/* REVISITED
	 * Previous code was buggy.
	 * Relying on the mediastream's state (added by jehan: only) to know the current encryption is unreliable.
	 * For (added by jehan: both DTLS and) ZRTP it is though necessary.
	 * But for all others the current_params->media_encryption state should reflect (added by jehan: both) what is agreed by the offer/answer
	 * mechanism  (added by jehan: and encryption status from media which is much stronger than only result of offer/answer )
	 * Typically there can be inactive streams for which the media layer has no idea of whether they are encrypted or not.
	 */

	switch (call->params->media_encryption) {
	case LinphoneMediaEncryptionZRTP:
		if (at_least_one_stream_started(call)){
			if ((all_streams_encrypted = linphone_call_all_streams_encrypted(call)) && linphone_call_get_authentication_token(call)) {
				call->current_params->media_encryption=LinphoneMediaEncryptionZRTP;
			} else {
				/*to avoid to many traces*/
				ms_debug("Encryption was requested to be %s, but isn't effective (all_streams_encrypted=%i, auth_token=%s)",
					linphone_media_encryption_to_string(call->params->media_encryption), all_streams_encrypted, call->auth_token == NULL ? "" : call->auth_token);
				call->current_params->media_encryption=LinphoneMediaEncryptionNone;
			}
		}//else don't update the state if all streams are shutdown.
		break;
	case LinphoneMediaEncryptionDTLS:
	case LinphoneMediaEncryptionSRTP:
		if (at_least_one_stream_started(call)){
			if (linphone_call_get_n_active_streams(call)==0 || (all_streams_encrypted = linphone_call_all_streams_encrypted(call))) {
				call->current_params->media_encryption = call->params->media_encryption;
			} else {
				/*to avoid to many traces*/
				ms_debug("Encryption was requested to be %s, but isn't effective (all_streams_encrypted=%i)",
					linphone_media_encryption_to_string(call->params->media_encryption), all_streams_encrypted);
				call->current_params->media_encryption=LinphoneMediaEncryptionNone;
			}
		}//else don't update the state if all streams are shutdown.
		break;
	case LinphoneMediaEncryptionNone:
		/* check if we actually switched to ZRTP */
		if (at_least_one_stream_started(call) && (all_streams_encrypted = linphone_call_all_streams_encrypted(call)) && linphone_call_get_authentication_token(call)) {
				call->current_params->media_encryption=LinphoneMediaEncryptionZRTP;
		} else {
			call->current_params->media_encryption=LinphoneMediaEncryptionNone;
		}
		break;
	}
	call->current_params->avpf_enabled = linphone_call_all_streams_avpf_enabled(call) && sal_media_description_has_avpf(md);
	if (call->current_params->avpf_enabled == TRUE) {
		call->current_params->avpf_rr_interval = linphone_call_get_avpf_rr_interval(call);
	} else {
		call->current_params->avpf_rr_interval = 0;
	}
	if (md){
		const char *rtp_addr;

		SalStreamDescription *sd=sal_media_description_find_best_stream(md,SalAudio);

		call->current_params->audio_dir=sd ? media_direction_from_sal_stream_dir(sd->dir) : LinphoneMediaDirectionInactive;
		if (call->current_params->audio_dir != LinphoneMediaDirectionInactive) {
			rtp_addr = sd->rtp_addr[0]!='\0' ? sd->rtp_addr : call->resultdesc->addr;
			call->current_params->audio_multicast_enabled = ms_is_multicast(rtp_addr);
		} else
			call->current_params->audio_multicast_enabled = FALSE;

		sd=sal_media_description_find_best_stream(md,SalVideo);
		call->current_params->implicit_rtcp_fb = sd ? sal_stream_description_has_implicit_avpf(sd): FALSE;
		call->current_params->video_dir=sd ? media_direction_from_sal_stream_dir(sd->dir) : LinphoneMediaDirectionInactive;
		if (call->current_params->video_dir != LinphoneMediaDirectionInactive) {
			rtp_addr = sd->rtp_addr[0]!='\0' ? sd->rtp_addr : call->resultdesc->addr;
			call->current_params->video_multicast_enabled = ms_is_multicast(rtp_addr);
		} else
			call->current_params->video_multicast_enabled = FALSE;

	}

	return call->current_params;
}

const LinphoneCallParams * linphone_call_get_remote_params(LinphoneCall *call){
	if (call->op){
		LinphoneCallParams *cp;
		SalMediaDescription *md;
		const SalCustomHeader *ch;

		md=sal_call_get_remote_media_description(call->op);
		if (md) {
			SalStreamDescription *sd;
			unsigned int i;
			unsigned int nb_audio_streams = sal_media_description_nb_active_streams_of_type(md, SalAudio);
			unsigned int nb_video_streams = sal_media_description_nb_active_streams_of_type(md, SalVideo);
			unsigned int nb_text_streams = sal_media_description_nb_active_streams_of_type(md, SalText);
			if (call->remote_params != NULL) linphone_call_params_unref(call->remote_params);
			cp = call->remote_params = linphone_call_params_new();

			for (i = 0; i < nb_video_streams; i++) {
				sd = sal_media_description_get_active_stream_of_type(md, SalVideo, i);
				if (sal_stream_description_active(sd) == TRUE) cp->has_video = TRUE;
				if (sal_stream_description_has_srtp(sd) == TRUE) cp->media_encryption = LinphoneMediaEncryptionSRTP;
			}
			for (i = 0; i < nb_audio_streams; i++) {
				sd = sal_media_description_get_active_stream_of_type(md, SalAudio, i);
				if (sal_stream_description_has_srtp(sd) == TRUE) cp->media_encryption = LinphoneMediaEncryptionSRTP;
			}
			for (i = 0; i < nb_text_streams; i++) {
				sd = sal_media_description_get_active_stream_of_type(md, SalText, i);
				if (sal_stream_description_has_srtp(sd) == TRUE) cp->media_encryption = LinphoneMediaEncryptionSRTP;
				cp->realtimetext_enabled = TRUE;
			}
			if (!cp->has_video){
				if (md->bandwidth>0 && md->bandwidth<=linphone_core_get_edge_bw(call->core)){
					cp->low_bandwidth=TRUE;
				}
			}
			if (md->name[0]!='\0') linphone_call_params_set_session_name(cp,md->name);

			linphone_call_params_set_custom_sdp_attributes(call->remote_params, md->custom_sdp_attributes);
			linphone_call_params_set_custom_sdp_media_attributes(call->remote_params, LinphoneStreamTypeAudio, md->streams[call->main_audio_stream_index].custom_sdp_attributes);
			linphone_call_params_set_custom_sdp_media_attributes(call->remote_params, LinphoneStreamTypeVideo, md->streams[call->main_video_stream_index].custom_sdp_attributes);
			linphone_call_params_set_custom_sdp_media_attributes(call->remote_params, LinphoneStreamTypeText, md->streams[call->main_text_stream_index].custom_sdp_attributes);
		}
		ch = sal_op_get_recv_custom_header(call->op);
		if (ch){
			/*instanciate a remote_params only if a SIP message was received before (custom headers indicates this).*/
			if (call->remote_params == NULL) call->remote_params = linphone_call_params_new();
			linphone_call_params_set_custom_headers(call->remote_params, ch);
		}
		return call->remote_params;
	}
	return NULL;
}

const LinphoneAddress * linphone_call_get_remote_address(const LinphoneCall *call){
	return call->dir==LinphoneCallIncoming ? call->log->from : call->log->to;
}

char *linphone_call_get_remote_address_as_string(const LinphoneCall *call){
	return linphone_address_as_string(linphone_call_get_remote_address(call));
}

const LinphoneAddress * linphone_call_get_diversion_address(const LinphoneCall *call){
	return call->op?(const LinphoneAddress *)sal_op_get_diversion_address(call->op):NULL;
}

LinphoneCallState linphone_call_get_state(const LinphoneCall *call){
	return call->state;
}

LinphoneReason linphone_call_get_reason(const LinphoneCall *call){
	return linphone_error_info_get_reason(linphone_call_get_error_info(call));
}

const LinphoneErrorInfo *linphone_call_get_error_info(const LinphoneCall *call){
	if (call->non_op_error.reason!=SalReasonNone){
		return (const LinphoneErrorInfo*)&call->non_op_error;
	}else return linphone_error_info_from_sal_op(call->op);
}

void *linphone_call_get_user_data(const LinphoneCall *call)
{
	return call->user_data;
}

void linphone_call_set_user_data(LinphoneCall *call, void *user_pointer)
{
	call->user_data = user_pointer;
}

LinphoneCallLog *linphone_call_get_call_log(const LinphoneCall *call){
	return call->log;
}

const char *linphone_call_get_refer_to(const LinphoneCall *call){
	return call->refer_to;
}

LinphoneCall *linphone_call_get_transferer_call(const LinphoneCall *call){
	return call->referer;
}

LinphoneCall *linphone_call_get_transfer_target_call(const LinphoneCall *call){
	return call->transfer_target;
}

LinphoneCallDir linphone_call_get_dir(const LinphoneCall *call){
	return call->log->dir;
}

const char *linphone_call_get_remote_user_agent(LinphoneCall *call){
	if (call->op){
		return sal_op_get_remote_ua (call->op);
	}
	return NULL;
}

const char *linphone_call_get_remote_contact(LinphoneCall *call){
	if( call->op ){
		/*sal_op_get_remote_contact preserves header params*/
		return sal_op_get_remote_contact(call->op);
	}
	return NULL;
}

bool_t linphone_call_has_transfer_pending(const LinphoneCall *call){
	return call->refer_pending;
}

int linphone_call_get_duration(const LinphoneCall *call){
	if (call->log->connected_date_time==0) return 0;
	return (int)(ms_time(NULL) - call->log->connected_date_time);
}

LinphoneCall *linphone_call_get_replaced_call(LinphoneCall *call){
	SalOp *op=sal_call_get_replaces(call->op);
	if (op){
		return (LinphoneCall*)sal_op_get_user_pointer(op);
	}
	return NULL;
}

void linphone_call_enable_camera (LinphoneCall *call, bool_t enable){
#ifdef VIDEO_ENABLED
	call->camera_enabled=enable;
	switch(call->state) {
		case LinphoneCallStreamsRunning:
		case LinphoneCallOutgoingEarlyMedia:
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallConnected:
			if(call->videostream!=NULL
				&& video_stream_started(call->videostream)
				&& video_stream_get_camera(call->videostream) != linphone_call_get_video_device(call)) {
				const char *cur_cam, *new_cam;
				cur_cam = video_stream_get_camera(call->videostream) ? ms_web_cam_get_name(video_stream_get_camera(call->videostream)) : "NULL";
				new_cam = linphone_call_get_video_device(call) ? ms_web_cam_get_name(linphone_call_get_video_device(call)) : "NULL";
				ms_message("Switching video cam from [%s] to [%s] on call [%p]"	, cur_cam, new_cam, call);
				video_stream_change_camera(call->videostream, linphone_call_get_video_device(call));
			}
			break;

		default: break;
	}
#endif
}

void linphone_call_send_vfu_request(LinphoneCall *call) {
#ifdef VIDEO_ENABLED
	const LinphoneCallParams *current_params = linphone_call_get_current_params(call);
	if ((current_params->avpf_enabled || current_params->implicit_rtcp_fb  )&& call->videostream && media_stream_get_state((const MediaStream *)call->videostream) == MSStreamStarted) { // || sal_media_description_has_implicit_avpf((const SalMediaDescription *)call->resultdesc)
		ms_message("Request Full Intra Request on call [%p]", call);
		video_stream_send_fir(call->videostream);
	} else if (call->core->sip_conf.vfu_with_info) {
		ms_message("Request SIP INFO FIR on call [%p]", call);
		if (LinphoneCallStreamsRunning == linphone_call_get_state(call))
			sal_call_send_vfu_request(call->op);
	} else {
		ms_message("vfu request using sip disabled from config [sip,vfu_with_info]");
	}
#endif
}

int linphone_call_take_video_snapshot(LinphoneCall *call, const char *file){
#ifdef VIDEO_ENABLED
	if (call->videostream!=NULL && call->videostream->jpegwriter!=NULL){
		return ms_filter_call_method(call->videostream->jpegwriter,MS_JPEG_WRITER_TAKE_SNAPSHOT,(void*)file);
	}
	ms_warning("Cannot take snapshot: no currently running video stream on this call.");
#endif
	return -1;
}

int linphone_call_take_preview_snapshot(LinphoneCall *call, const char *file){
#ifdef VIDEO_ENABLED
	if (call->videostream!=NULL && call->videostream->local_jpegwriter!=NULL){
		return ms_filter_call_method(call->videostream->local_jpegwriter,MS_JPEG_WRITER_TAKE_SNAPSHOT,(void*)file);
	}
	ms_warning("Cannot take local snapshot: no currently running video stream on this call.");
	return -1;
#endif
	return -1;
}

bool_t linphone_call_camera_enabled (const LinphoneCall *call){
	return call->camera_enabled;
}

/**
 * @ingroup call_control
 * @return string value of LinphonePrivacy enum
 **/
const char* linphone_privacy_to_string(LinphonePrivacy privacy) {
	switch(privacy) {
	case LinphonePrivacyDefault: return "LinphonePrivacyDefault";
	case LinphonePrivacyUser: return "LinphonePrivacyUser";
	case LinphonePrivacyHeader: return "LinphonePrivacyHeader";
	case LinphonePrivacySession: return "LinphonePrivacySession";
	case LinphonePrivacyId: return "LinphonePrivacyId";
	case LinphonePrivacyNone: return "LinphonePrivacyNone";
	case LinphonePrivacyCritical: return "LinphonePrivacyCritical";
	default: return "Unknown privacy mode";
	}
}


#ifdef TEST_EXT_RENDERER
static void rendercb(void *data, const MSPicture *local, const MSPicture *remote){
	ms_message("rendercb, local buffer=%p, remote buffer=%p",
			   local ? local->planes[0] : NULL, remote? remote->planes[0] : NULL);
}
#endif

#ifdef VIDEO_ENABLED
static void video_stream_event_cb(void *user_pointer, const MSFilter *f, const unsigned int event_id, const void *args){
	LinphoneCall* call = (LinphoneCall*) user_pointer;
	switch (event_id) {
		case MS_VIDEO_DECODER_DECODING_ERRORS:
			ms_warning("MS_VIDEO_DECODER_DECODING_ERRORS");
			if (call->videostream && (video_stream_is_decoding_error_to_be_reported(call->videostream, 5000) == TRUE)) {
				video_stream_decoding_error_reported(call->videostream);
				linphone_call_send_vfu_request(call);
			}
			break;
		case MS_VIDEO_DECODER_RECOVERED_FROM_ERRORS:
			ms_message("MS_VIDEO_DECODER_RECOVERED_FROM_ERRORS");
			if (call->videostream) {
				video_stream_decoding_error_recovered(call->videostream);
			}
			break;
		case MS_VIDEO_DECODER_FIRST_IMAGE_DECODED:
			ms_message("First video frame decoded successfully");
			if (call->nextVideoFrameDecoded._func != NULL){
				call->nextVideoFrameDecoded._func(call, call->nextVideoFrameDecoded._user_data);
				call->nextVideoFrameDecoded._func = NULL;
				call->nextVideoFrameDecoded._user_data = NULL;
			}
			break;
		case MS_VIDEO_DECODER_SEND_PLI:
		case MS_VIDEO_DECODER_SEND_SLI:
		case MS_VIDEO_DECODER_SEND_RPSI:
			/* Handled internally by mediastreamer2. */
			break;
		default:
			ms_warning("Unhandled event %i", event_id);
			break;
	}
}
#endif

static void _linphone_call_set_next_video_frame_decoded_trigger(LinphoneCall *call){
#ifdef VIDEO_ENABLED
	if (call->nextVideoFrameDecoded._func && call->videostream && call->videostream->ms.decoder)
		ms_filter_call_method_noarg(call->videostream->ms.decoder, MS_VIDEO_DECODER_RESET_FIRST_IMAGE_NOTIFICATION);
#endif
}

void linphone_call_set_next_video_frame_decoded_callback(LinphoneCall *call, LinphoneCallCbFunc cb, void* user_data) {
	call->nextVideoFrameDecoded._func = cb;
	call->nextVideoFrameDecoded._user_data = user_data;
	_linphone_call_set_next_video_frame_decoded_trigger(call);
}

static void port_config_set_random_choosed(LinphoneCall *call, int stream_index, RtpSession *session){
	call->media_ports[stream_index].rtp_port=rtp_session_get_local_port(session);
	call->media_ports[stream_index].rtcp_port=rtp_session_get_local_rtcp_port(session);
}

static void _linphone_call_prepare_ice_for_stream(LinphoneCall *call, int stream_index, bool_t create_checklist){
	MediaStream *ms = stream_index == call->main_audio_stream_index ? (MediaStream*)call->audiostream : stream_index == call->main_video_stream_index ? (MediaStream*)call->videostream : (MediaStream*)call->textstream;
	if ((linphone_core_get_firewall_policy(call->core) == LinphonePolicyUseIce) && (call->ice_session != NULL)){
		IceCheckList *cl;
		rtp_session_set_pktinfo(ms->sessions.rtp_session, TRUE);
		cl=ice_session_check_list(call->ice_session, stream_index);
		if (cl == NULL && create_checklist) {
			cl=ice_check_list_new();
			ice_session_add_check_list(call->ice_session, cl, stream_index);
			ms_message("Created new ICE check list for stream [%i]",stream_index);
		}
		if (cl) {
			media_stream_set_ice_check_list(ms, cl);
		}
	}
}

int linphone_call_prepare_ice(LinphoneCall *call, bool_t incoming_offer){
	SalMediaDescription *remote = NULL;
	int err;
	bool_t has_video=FALSE;

	if ((linphone_core_get_firewall_policy(call->core) == LinphonePolicyUseIce) && (call->ice_session != NULL)){
		if (incoming_offer){
			remote=sal_call_get_remote_media_description(call->op);
			has_video=linphone_core_video_enabled(call->core) && linphone_core_media_description_contains_video_stream(remote);
		}else has_video=call->params->has_video;

		_linphone_call_prepare_ice_for_stream(call,call->main_audio_stream_index,TRUE);
		if (has_video) _linphone_call_prepare_ice_for_stream(call,call->main_video_stream_index,TRUE);
		if (call->params->realtimetext_enabled) _linphone_call_prepare_ice_for_stream(call,call->main_text_stream_index,TRUE);
		/*start ICE gathering*/
		if (incoming_offer)
			linphone_call_update_ice_from_remote_media_description(call, remote, TRUE); /*this may delete the ice session*/
		if (call->ice_session && !ice_session_candidates_gathered(call->ice_session)){
			if (call->audiostream->ms.state==MSStreamInitialized)
				audio_stream_prepare_sound(call->audiostream, NULL, NULL);
#ifdef VIDEO_ENABLED
			if (has_video && call->videostream && call->videostream->ms.state==MSStreamInitialized) {
				video_stream_prepare_video(call->videostream);
			}
#endif
			if (call->params->realtimetext_enabled && call->textstream->ms.state==MSStreamInitialized) {
				text_stream_prepare_text(call->textstream);
			}
			err = linphone_core_gather_ice_candidates(call->core,call);
			if (err == 0) {
				/* Ice candidates gathering wasn't started, but we can proceed with the call anyway. */
				linphone_call_stop_media_streams_for_ice_gathering(call);
			}else if (err == -1) {
				linphone_call_stop_media_streams_for_ice_gathering(call);
				linphone_call_delete_ice_session(call);
			}
			return err;/* 1= gathering in progress, wait; 0=proceed*/
		}
	}
	return 0;
}

/*eventually join to a multicast group if told to do so*/
static void linphone_call_join_multicast_group(LinphoneCall *call, int stream_index, MediaStream *ms){
	if (call->media_ports[stream_index].multicast_ip[stream_index]!='\0'){
		media_stream_join_multicast_group(ms, call->media_ports[stream_index].multicast_ip);
	} else
		ms_error("Cannot join multicast group if multicast ip is not set for call [%p]",call);
}

static SalMulticastRole linphone_call_get_multicast_role(const LinphoneCall *call,SalStreamType type) {
	SalMulticastRole multicast_role=SalMulticastInactive;
	SalMediaDescription *remotedesc, *localdesc;
	SalStreamDescription *stream_desc = NULL;
	if (!call->op) goto end;
	remotedesc = sal_call_get_remote_media_description(call->op);
	localdesc = call->localdesc;
	if (!localdesc && !remotedesc && call->dir == LinphoneCallOutgoing) {
		/*well using call dir*/
		if ((type == SalAudio && linphone_call_params_audio_multicast_enabled(call->params))
			|| (type == SalVideo && linphone_call_params_video_multicast_enabled(call->params)))
			multicast_role=SalMulticastSender;
	} else	if (localdesc && (!remotedesc || sal_call_is_offerer(call->op))) {
		stream_desc = sal_media_description_find_best_stream(localdesc, type);
	} else if (!sal_call_is_offerer(call->op) && remotedesc)
		stream_desc = sal_media_description_find_best_stream(remotedesc, type);

	if (stream_desc)
		multicast_role = stream_desc->multicast_role;

	end:
	ms_message("Call [%p], stream type [%s], multicast role is [%s]",call, sal_stream_type_to_string(type),
		sal_multicast_role_to_string(multicast_role));
	return multicast_role;
}

static void setup_dtls_params(LinphoneCall *call, MediaStream* stream) {
	LinphoneCore *lc=call->core;
	if (call->params->media_encryption==LinphoneMediaEncryptionDTLS) {
		MSDtlsSrtpParams params;
		char *certificate, *key;
		memset(&params,0,sizeof(MSDtlsSrtpParams));
		/* TODO : search for a certificate with CNAME=sip uri(retrieved from variable me) or default : linphone-dtls-default-identity */
		/* This will parse the directory to find a matching fingerprint or generate it if not found */
		/* returned string must be freed */
		sal_certificates_chain_parse_directory(&certificate, &key, &call->dtls_certificate_fingerprint, lc->user_certificates_path, "linphone-dtls-default-identity", SAL_CERTIFICATE_RAW_FORMAT_PEM, TRUE, TRUE);

		if (key!= NULL && certificate!=NULL) {
			params.pem_certificate = (char *)certificate;
			params.pem_pkey = (char *)key;
			params.role = MSDtlsSrtpRoleUnset; /* default is unset, then check if we have a result SalMediaDescription */
			media_stream_enable_dtls(stream,&params);
			ms_free(certificate);
			ms_free(key);
		} else {
			ms_error("Unable to retrieve or generate DTLS certificate and key - DTLS disabled");
			/* TODO : check if encryption forced, if yes, stop call */
		}
	}
}

static void setZrtpCryptoTypesParameters(MSZrtpParams *params, LinphoneCore *lc)
{
	int i;
	const MSCryptoSuite *srtp_suites;
	MsZrtpCryptoTypesCount ciphersCount, authTagsCount;

	if (params == NULL) return;
	if (lc == NULL) return;

	srtp_suites = linphone_core_get_srtp_crypto_suites(lc);
	if (srtp_suites!=NULL) {
		for(i=0; srtp_suites[i]!=MS_CRYPTO_SUITE_INVALID && i<SAL_CRYPTO_ALGO_MAX && i<MS_MAX_ZRTP_CRYPTO_TYPES; ++i){
			switch (srtp_suites[i]) {
				case MS_AES_128_SHA1_32:
					params->ciphers[params->ciphersCount++] = MS_ZRTP_CIPHER_AES1;
					params->authTags[params->authTagsCount++] = MS_ZRTP_AUTHTAG_HS32;
					break;
				case MS_AES_128_NO_AUTH:
					params->ciphers[params->ciphersCount++] = MS_ZRTP_CIPHER_AES1;
					break;
				case MS_NO_CIPHER_SHA1_80:
					params->authTags[params->authTagsCount++] = MS_ZRTP_AUTHTAG_HS80;
					break;
				case MS_AES_128_SHA1_80:
					params->ciphers[params->ciphersCount++] = MS_ZRTP_CIPHER_AES1;
					params->authTags[params->authTagsCount++] = MS_ZRTP_AUTHTAG_HS80;
					break;
				case MS_AES_CM_256_SHA1_80:
				    ms_warning("Deprecated crypto suite MS_AES_CM_256_SHA1_80, use MS_AES_256_SHA1_80 instead");
				case MS_AES_256_SHA1_80:
				    params->ciphers[params->ciphersCount++] = MS_ZRTP_CIPHER_AES3;
                    params->authTags[params->authTagsCount++] = MS_ZRTP_AUTHTAG_HS80;
                    break;
				case MS_AES_256_SHA1_32:
					params->ciphers[params->ciphersCount++] = MS_ZRTP_CIPHER_AES3;
					params->authTags[params->authTagsCount++] = MS_ZRTP_AUTHTAG_HS32;
					break;
				case MS_CRYPTO_SUITE_INVALID:
					break;
			}
		}
	}

	/* linphone_core_get_srtp_crypto_suites is used to determine sensible defaults; here each can be overridden */
	ciphersCount = linphone_core_get_zrtp_cipher_suites(lc, params->ciphers); /* if not present in config file, params->ciphers is not modified */
	if (ciphersCount!=0) { /* use zrtp_cipher_suites config only when present, keep config from srtp_crypto_suite otherwise */
		params->ciphersCount = ciphersCount;
	}
	params->hashesCount = linphone_core_get_zrtp_hash_suites(lc, params->hashes);
	authTagsCount = linphone_core_get_zrtp_auth_suites(lc, params->authTags); /* if not present in config file, params->authTags is not modified */
	if (authTagsCount!=0) {
		params->authTagsCount = authTagsCount; /* use zrtp_auth_suites config only when present, keep config from srtp_crypto_suite otherwise */
	}
	params->sasTypesCount = linphone_core_get_zrtp_sas_suites(lc, params->sasTypes);
	params->keyAgreementsCount = linphone_core_get_zrtp_key_agreement_suites(lc, params->keyAgreements);
}

static OrtpJitterBufferAlgorithm name_to_jb_algo(const char *value){
	if (value){
		if (strcasecmp(value, "basic") == 0) return OrtpJitterBufferBasic;
		else if (strcasecmp(value, "rls") == 0) return OrtpJitterBufferRecursiveLeastSquare;
	}
	ms_error("Invalid jitter buffer algorithm: %s", value);
	return OrtpJitterBufferRecursiveLeastSquare;
}

static void apply_jitter_buffer_params(LinphoneCore *lc, RtpSession *session, LinphoneStreamType type){
	JBParameters params;
	
	rtp_session_get_jitter_buffer_params(session, &params);
	params.min_size = lp_config_get_int(lc->config, "rtp", "jitter_buffer_min_size", 40);
	params.max_size = lp_config_get_int(lc->config, "rtp", "jitter_buffer_max_size", 500);
	params.max_packets = params.max_size * 200 / 1000; /*allow 200 packet per seconds, quite large*/
	params.buffer_algorithm = name_to_jb_algo(lp_config_get_string(lc->config, "rtp", "jitter_buffer_algorithm", "rls"));
	params.refresh_ms = lp_config_get_int(lc->config, "rtp", "jitter_buffer_refresh_period", 5000);
	params.ramp_refresh_ms = lp_config_get_int(lc->config, "rtp", "jitter_buffer_ramp_refresh_period", 5000);
	params.ramp_step_ms = lp_config_get_int(lc->config, "rtp", "jitter_buffer_ramp_step", 20);
	params.ramp_threshold = lp_config_get_int(lc->config, "rtp", "jitter_buffer_ramp_threshold", 70);
	
	switch (type){
		case LinphoneStreamTypeAudio:
		case LinphoneStreamTypeText: /*let's use the same params for text as for audio.*/
			params.nom_size = linphone_core_get_audio_jittcomp(lc);
			params.adaptive = linphone_core_audio_adaptive_jittcomp_enabled(lc);
		break;
		case LinphoneStreamTypeVideo:
			params.nom_size = linphone_core_get_video_jittcomp(lc);
			params.adaptive = linphone_core_video_adaptive_jittcomp_enabled(lc);
		break;
		case LinphoneStreamTypeUnknown:
			ms_fatal("apply_jitter_buffer_params: should not happen");
		break;
	}
	params.enabled = params.nom_size > 0;
	if (params.enabled){
		if (params.min_size > params.nom_size){
			params.min_size = params.nom_size;
		}
		if (params.max_size < params.nom_size){
			params.max_size = params.nom_size;
		}
	}
	rtp_session_set_jitter_buffer_params(session, &params);
}

void linphone_call_init_audio_stream(LinphoneCall *call){
	LinphoneCore *lc=call->core;
	AudioStream *audiostream;
	const char *location;
	int dscp;
	const char *rtcp_tool=linphone_core_get_user_agent(call->core);
	char* cname;

	if (call->audiostream != NULL) return;
	if (call->sessions[call->main_audio_stream_index].rtp_session==NULL){
		SalMulticastRole multicast_role = linphone_call_get_multicast_role(call,SalAudio);
		SalMediaDescription *remotedesc=NULL;
		SalStreamDescription *stream_desc = NULL;
		if (call->op) remotedesc = sal_call_get_remote_media_description(call->op);
		if (remotedesc)
				stream_desc = sal_media_description_find_best_stream(remotedesc, SalAudio);

		call->audiostream=audiostream=audio_stream_new2(lc->factory, linphone_call_get_bind_ip_for_stream(call,call->main_audio_stream_index),
				multicast_role ==  SalMulticastReceiver ? stream_desc->rtp_port : call->media_ports[call->main_audio_stream_index].rtp_port,
				multicast_role ==  SalMulticastReceiver ? 0 /*disabled for now*/ : call->media_ports[call->main_audio_stream_index].rtcp_port);
		if (multicast_role == SalMulticastReceiver)
			linphone_call_join_multicast_group(call, call->main_audio_stream_index, &audiostream->ms);
		rtp_session_enable_network_simulation(call->audiostream->ms.sessions.rtp_session, &lc->net_conf.netsim_params);
		apply_jitter_buffer_params(lc, call->audiostream->ms.sessions.rtp_session, LinphoneStreamTypeAudio);
		cname = linphone_address_as_string_uri_only(call->me);
		audio_stream_set_rtcp_information(call->audiostream, cname, rtcp_tool);
		ms_free(cname);
		rtp_session_set_symmetric_rtp(audiostream->ms.sessions.rtp_session,linphone_core_symmetric_rtp_enabled(lc));
		setup_dtls_params(call, &audiostream->ms);

		/* init zrtp even if we didn't explicitely set it, just in case peer offers it */
		if (linphone_core_media_encryption_supported(lc, LinphoneMediaEncryptionZRTP)) {
			char *uri = linphone_address_as_string_uri_only((call->dir==LinphoneCallIncoming) ? call->log->from : call->log->to);
			MSZrtpParams params;
			memset(&params,0,sizeof(MSZrtpParams));
			/*call->current_params.media_encryption will be set later when zrtp is activated*/
			params.zid_file=lc->zrtp_secrets_cache;
			params.uri=uri;
			setZrtpCryptoTypesParameters(&params,call->core);
			audio_stream_enable_zrtp(call->audiostream,&params);
			if (uri != NULL) ms_free(uri);
		}

		media_stream_reclaim_sessions(&audiostream->ms, &call->sessions[call->main_audio_stream_index]);
	}else{
		call->audiostream=audio_stream_new_with_sessions(lc->factory, &call->sessions[call->main_audio_stream_index]);

	}
	audiostream=call->audiostream;
	if (call->media_ports[call->main_audio_stream_index].rtp_port==-1){
		port_config_set_random_choosed(call,call->main_audio_stream_index,audiostream->ms.sessions.rtp_session);
	}
	dscp=linphone_core_get_audio_dscp(lc);
	if (dscp!=-1)
		audio_stream_set_dscp(audiostream,dscp);
	if (linphone_core_echo_limiter_enabled(lc)){
		const char *type=lp_config_get_string(lc->config,"sound","el_type","mic");
		if (strcasecmp(type,"mic")==0)
			audio_stream_enable_echo_limiter(audiostream,ELControlMic);
		else if (strcasecmp(type,"full")==0)
			audio_stream_enable_echo_limiter(audiostream,ELControlFull);
	}

	/* equalizer location in the graph: 'mic' = in input graph, otherwise in output graph.
		Any other value than mic will default to output graph for compatibility */
	location = lp_config_get_string(lc->config,"sound","eq_location","hp");
	audiostream->eq_loc = (strcasecmp(location,"mic") == 0) ? MSEqualizerMic : MSEqualizerHP;
	ms_message("Equalizer location: %s", location);

	audio_stream_enable_gain_control(audiostream,TRUE);
	if (linphone_core_echo_cancellation_enabled(lc)){
		int len,delay,framesize;
		len=lp_config_get_int(lc->config,"sound","ec_tail_len",0);
		delay=lp_config_get_int(lc->config,"sound","ec_delay",0);
		framesize=lp_config_get_int(lc->config,"sound","ec_framesize",0);
		audio_stream_set_echo_canceller_params(audiostream,len,delay,framesize);
		if (audiostream->ec) {
			char *statestr=ms_malloc0(EC_STATE_MAX_LEN);
			if (lp_config_relative_file_exists(lc->config, EC_STATE_STORE)
				 && lp_config_read_relative_file(lc->config, EC_STATE_STORE, statestr, EC_STATE_MAX_LEN) == 0) {
				ms_filter_call_method(audiostream->ec, MS_ECHO_CANCELLER_SET_STATE_STRING, statestr);
			}
			ms_free(statestr);
		}
	}
	audio_stream_enable_automatic_gain_control(audiostream,linphone_core_agc_enabled(lc));
	{
		int enabled=lp_config_get_int(lc->config,"sound","noisegate",0);
		audio_stream_enable_noise_gate(audiostream,enabled);
	}

	audio_stream_set_features(audiostream,linphone_core_get_audio_features(lc));

	if (lc->rtptf){
		RtpTransport *meta_rtp;
		RtpTransport *meta_rtcp;

		rtp_session_get_transports(audiostream->ms.sessions.rtp_session,&meta_rtp,&meta_rtcp);
		if (meta_rtp_transport_get_endpoint(meta_rtp) == NULL) {
			ms_message("LinphoneCall[%p]: using custom audio RTP transport endpoint.", call);
			meta_rtp_transport_set_endpoint(meta_rtp,lc->rtptf->audio_rtp_func(lc->rtptf->audio_rtp_func_data, call->media_ports[call->main_audio_stream_index].rtp_port));
		}
		if (meta_rtp_transport_get_endpoint(meta_rtcp) == NULL) {
			meta_rtp_transport_set_endpoint(meta_rtcp,lc->rtptf->audio_rtcp_func(lc->rtptf->audio_rtcp_func_data, call->media_ports[call->main_audio_stream_index].rtcp_port));
		}
	}

	call->audiostream_app_evq = ortp_ev_queue_new();
	rtp_session_register_event_queue(audiostream->ms.sessions.rtp_session,call->audiostream_app_evq);

	_linphone_call_prepare_ice_for_stream(call,call->main_audio_stream_index,FALSE);
}

void linphone_call_init_video_stream(LinphoneCall *call){
#ifdef VIDEO_ENABLED
	LinphoneCore *lc=call->core;
	char* cname;
	const char *rtcp_tool = linphone_core_get_user_agent(call->core);

	if (call->videostream == NULL){
		int video_recv_buf_size=lp_config_get_int(lc->config,"video","recv_buf_size",0);
		int dscp=linphone_core_get_video_dscp(lc);
		const char *display_filter=linphone_core_get_video_display_filter(lc);

		if (call->sessions[call->main_video_stream_index].rtp_session==NULL){
			SalMulticastRole multicast_role = linphone_call_get_multicast_role(call,SalVideo);
			SalMediaDescription *remotedesc=NULL;
			SalStreamDescription *stream_desc = NULL;
			if (call->op) remotedesc = sal_call_get_remote_media_description(call->op);
			if (remotedesc)
					stream_desc = sal_media_description_find_best_stream(remotedesc, SalVideo);

			call->videostream=video_stream_new2(lc->factory, linphone_call_get_bind_ip_for_stream(call,call->main_video_stream_index),
					multicast_role ==  SalMulticastReceiver ? stream_desc->rtp_port : call->media_ports[call->main_video_stream_index].rtp_port,
					multicast_role ==  SalMulticastReceiver ?  0 /*disabled for now*/ : call->media_ports[call->main_video_stream_index].rtcp_port);
			if (multicast_role == SalMulticastReceiver)
				linphone_call_join_multicast_group(call, call->main_video_stream_index, &call->videostream->ms);
			rtp_session_enable_network_simulation(call->videostream->ms.sessions.rtp_session, &lc->net_conf.netsim_params);
			apply_jitter_buffer_params(lc, call->videostream->ms.sessions.rtp_session, LinphoneStreamTypeVideo);
			cname = linphone_address_as_string_uri_only(call->me);
			video_stream_set_rtcp_information(call->videostream, cname, rtcp_tool);
			ms_free(cname);
			rtp_session_set_symmetric_rtp(call->videostream->ms.sessions.rtp_session,linphone_core_symmetric_rtp_enabled(lc));
			setup_dtls_params(call, &call->videostream->ms);
			/* init zrtp even if we didn't explicitely set it, just in case peer offers it */
			if (linphone_core_media_encryption_supported(lc, LinphoneMediaEncryptionZRTP)) {
				video_stream_enable_zrtp(call->videostream, call->audiostream);
			}

			media_stream_reclaim_sessions(&call->videostream->ms, &call->sessions[call->main_video_stream_index]);
		}else{
			call->videostream=video_stream_new_with_sessions(lc->factory, &call->sessions[call->main_video_stream_index]);
		}

		if (call->media_ports[call->main_video_stream_index].rtp_port==-1){
			port_config_set_random_choosed(call,call->main_video_stream_index,call->videostream->ms.sessions.rtp_session);
		}
		if (dscp!=-1)
			video_stream_set_dscp(call->videostream,dscp);
		video_stream_enable_display_filter_auto_rotate(call->videostream, lp_config_get_int(lc->config,"video","display_filter_auto_rotate",0));
		if (video_recv_buf_size>0) rtp_session_set_recv_buf_size(call->videostream->ms.sessions.rtp_session,video_recv_buf_size);

		if (display_filter != NULL)
			video_stream_set_display_filter_name(call->videostream,display_filter);
		video_stream_set_event_callback(call->videostream,video_stream_event_cb, call);

		if (lc->rtptf){
			RtpTransport *meta_rtp;
			RtpTransport *meta_rtcp;

			rtp_session_get_transports(call->videostream->ms.sessions.rtp_session,&meta_rtp,&meta_rtcp);
			if (meta_rtp_transport_get_endpoint(meta_rtp) == NULL) {
				ms_message("LinphoneCall[%p]: using custom video RTP transport endpoint.", call);
				meta_rtp_transport_set_endpoint(meta_rtp,lc->rtptf->video_rtp_func(lc->rtptf->video_rtp_func_data, call->media_ports[call->main_video_stream_index].rtp_port));
			}
			if (meta_rtp_transport_get_endpoint(meta_rtcp) == NULL) {
				meta_rtp_transport_set_endpoint(meta_rtcp,lc->rtptf->video_rtcp_func(lc->rtptf->video_rtcp_func_data, call->media_ports[call->main_video_stream_index].rtcp_port));
			}
		}
		call->videostream_app_evq = ortp_ev_queue_new();
		rtp_session_register_event_queue(call->videostream->ms.sessions.rtp_session,call->videostream_app_evq);
		_linphone_call_prepare_ice_for_stream(call,call->main_video_stream_index,FALSE);
#ifdef TEST_EXT_RENDERER
		video_stream_set_render_callback(call->videostream,rendercb,NULL);
#endif
	}
#else
	call->videostream=NULL;
#endif
}

void linphone_call_init_text_stream(LinphoneCall *call){
	TextStream *textstream;
	LinphoneCore *lc=call->core;
	char* cname;

	if (call->textstream != NULL) return;
	if (call->sessions[call->main_text_stream_index].rtp_session == NULL) {
		SalMulticastRole multicast_role = linphone_call_get_multicast_role(call, SalText);
		SalMediaDescription *remotedesc = NULL;
		SalStreamDescription *stream_desc = NULL;
		if (call->op) remotedesc = sal_call_get_remote_media_description(call->op);
		if (remotedesc) stream_desc = sal_media_description_find_best_stream(remotedesc, SalText);

		call->textstream = textstream = text_stream_new2(lc->factory, linphone_call_get_bind_ip_for_stream(call,call->main_text_stream_index),
				multicast_role ==  SalMulticastReceiver ? stream_desc->rtp_port : call->media_ports[call->main_text_stream_index].rtp_port,
				multicast_role ==  SalMulticastReceiver ? 0 /*disabled for now*/ : call->media_ports[call->main_text_stream_index].rtcp_port);
		if (multicast_role == SalMulticastReceiver)
			linphone_call_join_multicast_group(call, call->main_text_stream_index, &textstream->ms);
		rtp_session_enable_network_simulation(call->textstream->ms.sessions.rtp_session, &lc->net_conf.netsim_params);
		apply_jitter_buffer_params(lc, call->textstream->ms.sessions.rtp_session, LinphoneStreamTypeText);
		cname = linphone_address_as_string_uri_only(call->me);
		ms_free(cname);
		rtp_session_set_symmetric_rtp(textstream->ms.sessions.rtp_session,linphone_core_symmetric_rtp_enabled(lc));
		setup_dtls_params(call, &textstream->ms);
		media_stream_reclaim_sessions(&textstream->ms, &call->sessions[call->main_text_stream_index]);
	} else {
		call->textstream = text_stream_new_with_sessions(lc->factory, &call->sessions[call->main_text_stream_index]);
	}
	textstream = call->textstream;
	if (call->media_ports[call->main_text_stream_index].rtp_port == -1) {
		port_config_set_random_choosed(call, call->main_text_stream_index, textstream->ms.sessions.rtp_session);
	}

	if (lc->rtptf){
		RtpTransport *meta_rtp;
		RtpTransport *meta_rtcp;

		rtp_session_get_transports(textstream->ms.sessions.rtp_session, &meta_rtp, &meta_rtcp);
		if (meta_rtp_transport_get_endpoint(meta_rtp) == NULL) {
			meta_rtp_transport_set_endpoint(meta_rtp,lc->rtptf->audio_rtp_func(lc->rtptf->audio_rtp_func_data, call->media_ports[call->main_text_stream_index].rtp_port));
		}
		if (meta_rtp_transport_get_endpoint(meta_rtcp) == NULL) {
			meta_rtp_transport_set_endpoint(meta_rtcp,lc->rtptf->audio_rtcp_func(lc->rtptf->audio_rtcp_func_data, call->media_ports[call->main_text_stream_index].rtcp_port));
		}
	}

	call->textstream_app_evq = ortp_ev_queue_new();
	rtp_session_register_event_queue(textstream->ms.sessions.rtp_session, call->textstream_app_evq);

	_linphone_call_prepare_ice_for_stream(call, call->main_text_stream_index, FALSE);
}

void linphone_call_init_media_streams(LinphoneCall *call){
	linphone_call_init_audio_stream(call);
	linphone_call_init_video_stream(call);
	linphone_call_init_text_stream(call);
}


static int dtmf_tab[16]={'0','1','2','3','4','5','6','7','8','9','*','#','A','B','C','D'};

static void linphone_core_dtmf_received(LinphoneCall *call, int dtmf){
	if (dtmf<0 || dtmf>15){
		ms_warning("Bad dtmf value %i",dtmf);
		return;
	}
	linphone_core_notify_dtmf_received(call->core, call, dtmf_tab[dtmf]);
}

static void parametrize_equalizer(LinphoneCore *lc, AudioStream *st){
	const char *eq_active = lp_config_get_string(lc->config, "sound", "eq_active", NULL);
	const char *eq_gains = lp_config_get_string(lc->config, "sound", "eq_gains", NULL);

	if(eq_active) ms_warning("'eq_active' linphonerc parameter has not effect anymore. Please use 'mic_eq_active' or 'spk_eq_active' instead");
	if(eq_gains) ms_warning("'eq_gains' linphonerc parameter has not effect anymore. Please use 'mic_eq_gains' or 'spk_eq_gains' instead");
	if (st->mic_equalizer){
		MSFilter *f=st->mic_equalizer;
		int enabled=lp_config_get_int(lc->config,"sound","mic_eq_active",0);
		const char *gains=lp_config_get_string(lc->config,"sound","mic_eq_gains",NULL);
		ms_filter_call_method(f,MS_EQUALIZER_SET_ACTIVE,&enabled);
		if (enabled && gains){
			bctbx_list_t *gains_list = ms_parse_equalizer_string(gains);
			bctbx_list_t *it;
			for(it=gains_list; it; it=it->next) {
				MSEqualizerGain *g = (MSEqualizerGain *)it->data;
				ms_message("Read microphone equalizer gains: %f(~%f) --> %f",g->frequency,g->width,g->gain);
				ms_filter_call_method(f,MS_EQUALIZER_SET_GAIN, g);
			}
			if(gains_list) bctbx_list_free_with_data(gains_list, ms_free);
		}
	}
	if (st->spk_equalizer){
		MSFilter *f=st->spk_equalizer;
		int enabled=lp_config_get_int(lc->config,"sound","spk_eq_active",0);
		const char *gains=lp_config_get_string(lc->config,"sound","spk_eq_gains",NULL);
		ms_filter_call_method(f,MS_EQUALIZER_SET_ACTIVE,&enabled);
		if (enabled && gains){
			bctbx_list_t *gains_list = ms_parse_equalizer_string(gains);
			bctbx_list_t *it;
			for(it=gains_list; it; it=it->next) {
				MSEqualizerGain *g = (MSEqualizerGain *)it->data;
				ms_message("Read speaker equalizer gains: %f(~%f) --> %f",g->frequency,g->width,g->gain);
				ms_filter_call_method(f,MS_EQUALIZER_SET_GAIN, g);
			}
			if(gains_list) bctbx_list_free_with_data(gains_list, ms_free);
		}
	}
}

void set_mic_gain_db(AudioStream *st, float gain){
	audio_stream_set_mic_gain_db(st, gain);
}

void set_playback_gain_db(AudioStream *st, float gain){
	if (st->volrecv){
		ms_filter_call_method(st->volrecv,MS_VOLUME_SET_DB_GAIN,&gain);
	}else ms_warning("Could not apply playback gain: gain control wasn't activated.");
}

/*This function is not static because used internally in linphone-daemon project*/
void _post_configure_audio_stream(AudioStream *st, LinphoneCore *lc, bool_t muted){
	float mic_gain=lc->sound_conf.soft_mic_lev;
	float thres = 0;
	float recv_gain;
	float ng_thres=lp_config_get_float(lc->config,"sound","ng_thres",0.05f);
	float ng_floorgain=lp_config_get_float(lc->config,"sound","ng_floorgain",0);
	int dc_removal=lp_config_get_int(lc->config,"sound","dc_removal",0);
	float speed;
	float force;
	int sustain;
	float transmit_thres;
	MSFilter *f=NULL;
	float floorgain;
	int spk_agc;

	if (!muted)
		set_mic_gain_db(st,mic_gain);
	else
		audio_stream_set_mic_gain(st,0);

	recv_gain = lc->sound_conf.soft_play_lev;
	if (recv_gain != 0) {
		set_playback_gain_db(st,recv_gain);
	}

	if (st->volsend){
		ms_filter_call_method(st->volsend,MS_VOLUME_REMOVE_DC,&dc_removal);
		speed=lp_config_get_float(lc->config,"sound","el_speed",-1);
		thres=lp_config_get_float(lc->config,"sound","el_thres",-1);
		force=lp_config_get_float(lc->config,"sound","el_force",-1);
		sustain=lp_config_get_int(lc->config,"sound","el_sustain",-1);
		transmit_thres=lp_config_get_float(lc->config,"sound","el_transmit_thres",-1);
		f=st->volsend;
		if (speed==-1) speed=0.03f;
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
		floorgain = (float)(1/pow(10,mic_gain/10));
		spk_agc=lp_config_get_int(lc->config,"sound","speaker_agc_enabled",0);
		ms_filter_call_method(st->volrecv, MS_VOLUME_ENABLE_AGC, &spk_agc);
		ms_filter_call_method(st->volrecv,MS_VOLUME_SET_NOISE_GATE_THRESHOLD,&ng_thres);
		ms_filter_call_method(st->volrecv,MS_VOLUME_SET_NOISE_GATE_FLOORGAIN,&floorgain);
	}
	parametrize_equalizer(lc,st);
}

static void post_configure_audio_streams(LinphoneCall *call, bool_t muted){
	AudioStream *st=call->audiostream;
	LinphoneCore *lc=call->core;
	_post_configure_audio_stream(st,lc,muted);
	if (linphone_core_dtmf_received_has_listener(lc)){
		audio_stream_play_received_dtmfs(call->audiostream,FALSE);
	}
	if (call->record_active)
		linphone_call_start_recording(call);
}

static int get_ideal_audio_bw(LinphoneCall *call, const SalMediaDescription *md, const SalStreamDescription *desc){
	int remote_bw=0;
	int upload_bw;
	int total_upload_bw=linphone_core_get_upload_bandwidth(call->core);
	const LinphoneCallParams *params=call->params;
	bool_t will_use_video=linphone_core_media_description_contains_video_stream(md);
	bool_t forced=FALSE;

	if (desc->bandwidth>0) remote_bw=desc->bandwidth;
	else if (md->bandwidth>0) {
		/*case where b=AS is given globally, not per stream*/
		remote_bw=md->bandwidth;
	}
	if (params->up_bw>0){
		forced=TRUE;
		upload_bw=params->up_bw;
	}else upload_bw=total_upload_bw;
	upload_bw=get_min_bandwidth(upload_bw,remote_bw);
	if (!will_use_video || forced) return upload_bw;

	if (bandwidth_is_greater(upload_bw,512)){
		upload_bw=100;
	}else if (bandwidth_is_greater(upload_bw,256)){
		upload_bw=64;
	}else if (bandwidth_is_greater(upload_bw,128)){
		upload_bw=40;
	}else if (bandwidth_is_greater(upload_bw,0)){
		upload_bw=24;
	}
	return upload_bw;
}

static int get_video_bw(LinphoneCall *call, const SalMediaDescription *md, const SalStreamDescription *desc){
	int remote_bw=0;
	int bw;
	if (desc->bandwidth>0) remote_bw=desc->bandwidth;
	else if (md->bandwidth>0) {
		/*case where b=AS is given globally, not per stream*/
		remote_bw=get_remaining_bandwidth_for_video(md->bandwidth,call->audio_bw);
	} else {
		remote_bw = lp_config_get_int(call->core->config, "net", "default_max_bandwidth", 1500);
	}
	bw=get_min_bandwidth(get_remaining_bandwidth_for_video(linphone_core_get_upload_bandwidth(call->core),call->audio_bw),remote_bw);
	return bw;
}

static RtpProfile *make_profile(LinphoneCall *call, const SalMediaDescription *md, const SalStreamDescription *desc, int *used_pt){
	int bw=0;
	const bctbx_list_t *elem;
	RtpProfile *prof=rtp_profile_new("Call profile");
	bool_t first=TRUE;
	LinphoneCore *lc=call->core;
	int up_ptime=0;
	const LinphoneCallParams *params=call->params;

	*used_pt=-1;

	if (desc->type==SalAudio)
		bw=get_ideal_audio_bw(call,md,desc);
	else if (desc->type==SalVideo)
		bw=get_video_bw(call,md,desc);
	//else if (desc->type== SalText)


	for(elem=desc->payloads;elem!=NULL;elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
		int number;
		/* make a copy of the payload type, so that we left the ones from the SalStreamDescription unchanged.
		 If the SalStreamDescription is freed, this will have no impact on the running streams*/
		pt=payload_type_clone(pt);

		if ((pt->flags & PAYLOAD_TYPE_FLAG_CAN_SEND) && first) {
			/*first codec in list is the selected one*/
			if (desc->type==SalAudio){
				/*this will update call->audio_bw*/
				linphone_core_update_allocated_audio_bandwidth_in_call(call,pt,bw);
				bw=call->audio_bw;
				if (params->up_ptime)
					up_ptime=params->up_ptime;
				else up_ptime=linphone_core_get_upload_ptime(lc);
			}
			*used_pt=payload_type_get_number(pt);
			first=FALSE;
		}
		if (pt->flags & PAYLOAD_TYPE_BITRATE_OVERRIDE){
			ms_message("Payload type [%s/%i] has explicit bitrate [%i] kbit/s", pt->mime_type, pt->clock_rate, pt->normal_bitrate/1000);
			pt->normal_bitrate=get_min_bandwidth(pt->normal_bitrate,bw*1000);
		} else pt->normal_bitrate=bw*1000;
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

static void configure_rtp_session_for_rtcp_fb(LinphoneCall *call, const SalStreamDescription *stream) {
	RtpSession *session = NULL;

	if (stream->type == SalAudio) {
		session = call->audiostream->ms.sessions.rtp_session;
	} else if (stream->type == SalVideo) {
		session = call->videostream->ms.sessions.rtp_session;
	} else {
		// Do nothing for streams that are not audio or video
		return;
	}
	if (stream->rtcp_fb.generic_nack_enabled)
		rtp_session_enable_avpf_feature(session, ORTP_AVPF_FEATURE_GENERIC_NACK, TRUE);
	else
		rtp_session_enable_avpf_feature(session, ORTP_AVPF_FEATURE_GENERIC_NACK, FALSE);
	if (stream->rtcp_fb.tmmbr_enabled)
		rtp_session_enable_avpf_feature(session, ORTP_AVPF_FEATURE_TMMBR, TRUE);
	else
		rtp_session_enable_avpf_feature(session, ORTP_AVPF_FEATURE_TMMBR, FALSE);
}

static void configure_rtp_session_for_rtcp_xr(LinphoneCore *lc, LinphoneCall *call, SalStreamType type) {
	RtpSession *session = NULL;
	const OrtpRtcpXrConfiguration *localconfig;
	const OrtpRtcpXrConfiguration *remoteconfig;
	OrtpRtcpXrConfiguration currentconfig;
	const SalStreamDescription *localstream;
	const SalStreamDescription *remotestream;
	SalMediaDescription *remotedesc = sal_call_get_remote_media_description(call->op);

	if (!remotedesc) return;

	localstream = sal_media_description_find_best_stream(call->localdesc, type);
	if (!localstream) return;
	localconfig = &localstream->rtcp_xr;
	remotestream = sal_media_description_find_best_stream(remotedesc, type);
	if (!remotestream) return;
	remoteconfig = &remotestream->rtcp_xr;

	if (localstream->dir == SalStreamInactive) return;
	else if (localstream->dir == SalStreamRecvOnly) {
		/* Use local config for unilateral parameters and remote config for collaborative parameters. */
		memcpy(&currentconfig, localconfig, sizeof(currentconfig));
		currentconfig.rcvr_rtt_mode = remoteconfig->rcvr_rtt_mode;
		currentconfig.rcvr_rtt_max_size = remoteconfig->rcvr_rtt_max_size;
	} else {
		memcpy(&currentconfig, remoteconfig, sizeof(currentconfig));
	}
	if (type == SalAudio) {
		session = call->audiostream->ms.sessions.rtp_session;
	} else if (type == SalVideo) {
		session = call->videostream->ms.sessions.rtp_session;
	} else if (type == SalText) {
		session = call->textstream->ms.sessions.rtp_session;
	}
	rtp_session_configure_rtcp_xr(session, &currentconfig);
}

static void start_dtls( MSMediaStreamSessions *sessions,  const SalStreamDescription *sd,const SalStreamDescription *remote) {
	if (sal_stream_description_has_dtls(sd) == TRUE) {
		/*DTLS*/
		SalDtlsRole salRole = sd->dtls_role;
		if (salRole!=SalDtlsRoleInvalid) { /* if DTLS is available at both end points */
			/* give the peer certificate fingerprint to dtls context */
			ms_dtls_srtp_set_peer_fingerprint(sessions->dtls_context, remote->dtls_fingerprint);
			ms_dtls_srtp_set_role(sessions->dtls_context, (salRole == SalDtlsRoleIsClient)?MSDtlsSrtpRoleIsClient:MSDtlsSrtpRoleIsServer); /* set the role to client */
			ms_dtls_srtp_start(sessions->dtls_context);  /* then start the engine, it will send the DTLS client Hello */
		} else {
			ms_warning("unable to start DTLS engine on stream session [%p], Dtls role in resulting media description is invalid",sessions);
		}
	}
}

static void start_dtls_on_all_streams(LinphoneCall *call) {
	SalMediaDescription *remote_desc = sal_call_get_remote_media_description(call->op);
	SalMediaDescription *result_desc = sal_call_get_final_media_description(call->op);
	if( remote_desc == NULL || result_desc == NULL ){
		/* this can happen in some tricky cases (early-media without SDP in the 200). In that case, simply skip DTLS code */
		return;
	}

	if (call->audiostream && (media_stream_get_state((const MediaStream *)call->audiostream) == MSStreamStarted))/*dtls must start at the end of ice*/
			start_dtls(&call->audiostream->ms.sessions
							,sal_media_description_find_best_stream(result_desc,SalAudio)
							,sal_media_description_find_best_stream(remote_desc,SalAudio));
#if VIDEO_ENABLED
	if (call->videostream && (media_stream_get_state((const MediaStream *)call->videostream) == MSStreamStarted))/*dtls must start at the end of ice*/
			start_dtls(&call->videostream->ms.sessions
						,sal_media_description_find_best_stream(result_desc,SalVideo)
						,sal_media_description_find_best_stream(remote_desc,SalVideo));
#endif
	if (call->textstream && (media_stream_get_state((const MediaStream *)call->textstream) == MSStreamStarted))/*dtls must start at the end of ice*/
	start_dtls(&call->textstream->ms.sessions
					,sal_media_description_find_best_stream(result_desc,SalText)
					,sal_media_description_find_best_stream(remote_desc,SalText));
	return;
}

static void set_dtls_fingerprint( MSMediaStreamSessions *sessions,  const SalStreamDescription *sd,const SalStreamDescription *remote) {
	if (sal_stream_description_has_dtls(sd) == TRUE) {
		/*DTLS*/
		SalDtlsRole salRole = sd->dtls_role;
		if (salRole!=SalDtlsRoleInvalid) { /* if DTLS is available at both end points */
			/* give the peer certificate fingerprint to dtls context */
			ms_dtls_srtp_set_peer_fingerprint(sessions->dtls_context, remote->dtls_fingerprint);
		} else {
			ms_warning("unable to start DTLS engine on stream session [%p], Dtls role in resulting media description is invalid",sessions);
		}
	}
}

static void set_dtls_fingerprint_on_all_streams(LinphoneCall *call) {
	SalMediaDescription *remote_desc = sal_call_get_remote_media_description(call->op);
	SalMediaDescription *result_desc = sal_call_get_final_media_description(call->op);

	if( remote_desc == NULL || result_desc == NULL ){
		/* this can happen in some tricky cases (early-media without SDP in the 200). In that case, simply skip DTLS code */
		return;
	}

	if (call->audiostream && (media_stream_get_state((const MediaStream *)call->audiostream) == MSStreamStarted))/*dtls must start at the end of ice*/
			set_dtls_fingerprint(&call->audiostream->ms.sessions
							,sal_media_description_find_best_stream(result_desc,SalAudio)
							,sal_media_description_find_best_stream(remote_desc,SalAudio));
#if VIDEO_ENABLED
	if (call->videostream && (media_stream_get_state((const MediaStream *)call->videostream) == MSStreamStarted))/*dtls must start at the end of ice*/
			set_dtls_fingerprint(&call->videostream->ms.sessions
						,sal_media_description_find_best_stream(result_desc,SalVideo)
						,sal_media_description_find_best_stream(remote_desc,SalVideo));
#endif
	if (call->textstream && (media_stream_get_state((const MediaStream *)call->textstream) == MSStreamStarted))/*dtls must start at the end of ice*/
	set_dtls_fingerprint(&call->textstream->ms.sessions
					,sal_media_description_find_best_stream(result_desc,SalText)
					,sal_media_description_find_best_stream(remote_desc,SalText));
	return;
}

static RtpSession * create_audio_rtp_io_session(LinphoneCall *call) {
	PayloadType *pt;
	LinphoneCore *lc = call->core;
	const char *local_ip = lp_config_get_string(lc->config, "sound", "rtp_local_addr", "127.0.0.1");
	const char *remote_ip = lp_config_get_string(lc->config, "sound", "rtp_remote_addr", "127.0.0.1");
	int local_port = lp_config_get_int(lc->config, "sound", "rtp_local_port", 17076);
	int remote_port = lp_config_get_int(lc->config, "sound", "rtp_remote_port", 17078);
	int ptnum = lp_config_get_int(lc->config, "sound", "rtp_ptnum", 0);
	const char *rtpmap = lp_config_get_string(lc->config, "sound", "rtp_map", "pcmu/8000/1");
	int symmetric = lp_config_get_int(lc->config, "sound", "rtp_symmetric", 0);
	int jittcomp = lp_config_get_int(lc->config, "sound", "rtp_jittcomp", 0); /* 0 means no jitter buffer*/
	RtpSession *rtp_session = NULL;
	pt = rtp_profile_get_payload_from_rtpmap(call->audio_profile, rtpmap);
	if (pt != NULL) {
		call->rtp_io_audio_profile = rtp_profile_new("RTP IO audio profile");
		rtp_profile_set_payload(call->rtp_io_audio_profile, ptnum, payload_type_clone(pt));
		rtp_session = ms_create_duplex_rtp_session(local_ip, local_port, -1, ms_factory_get_mtu(lc->factory));
		rtp_session_set_profile(rtp_session, call->rtp_io_audio_profile);
		rtp_session_set_remote_addr_and_port(rtp_session, remote_ip, remote_port, -1);
		rtp_session_enable_rtcp(rtp_session, FALSE);
		rtp_session_set_payload_type(rtp_session, ptnum);
		rtp_session_set_jitter_compensation(rtp_session, jittcomp);
		rtp_session_enable_jitter_buffer(rtp_session, jittcomp>0);
		rtp_session_set_symmetric_rtp(rtp_session, (bool_t)symmetric);
	}
	return rtp_session;
}

static void linphone_call_set_on_hold_file(LinphoneCall *call, const char *file){
	if (call->onhold_file){
		ms_free(call->onhold_file);
		call->onhold_file = NULL;
	}
	if (file){
		call->onhold_file = ms_strdup(file);
	}
}

static void linphone_call_start_audio_stream(LinphoneCall *call, LinphoneCallState next_state, bool_t use_arc){
	LinphoneCore *lc=call->core;
	int used_pt=-1;
	const SalStreamDescription *stream;
	MSSndCard *playcard;
	MSSndCard *captcard;
	bool_t use_ec;
	bool_t mute;
	const char *playfile;
	const char *recfile;
	const char *file_to_play = NULL;
	const SalStreamDescription *local_st_desc;
	int crypto_idx;
	MSMediaStreamIO io = MS_MEDIA_STREAM_IO_INITIALIZER;
	bool_t use_rtp_io = lp_config_get_int(lc->config, "sound", "rtp_io", FALSE);
	bool_t use_rtp_io_enable_local_output = lp_config_get_int(lc->config, "sound", "rtp_io_enable_local_output", FALSE);

	stream = sal_media_description_find_best_stream(call->resultdesc, SalAudio);
	if (stream && stream->dir!=SalStreamInactive && stream->rtp_port!=0){
		/* get remote stream description to check for zrtp-hash presence */
		SalMediaDescription *remote_desc = sal_call_get_remote_media_description(call->op);
		const SalStreamDescription *remote_stream = sal_media_description_find_best_stream(remote_desc, SalAudio);

		const char *rtp_addr=stream->rtp_addr[0]!='\0' ? stream->rtp_addr : call->resultdesc->addr;
		bool_t is_multicast=ms_is_multicast(rtp_addr);
		playcard=lc->sound_conf.lsd_card ?
			lc->sound_conf.lsd_card : lc->sound_conf.play_sndcard;
		captcard=lc->sound_conf.capt_sndcard;
		playfile=lc->play_file;
		recfile=lc->rec_file;
		call->audio_profile=make_profile(call,call->resultdesc,stream,&used_pt);

		if (used_pt!=-1){
			bool_t ok = TRUE;
			call->current_params->audio_codec = rtp_profile_get_payload(call->audio_profile, used_pt);
			call->current_params->has_audio = TRUE;
			if (playcard==NULL) {
				ms_warning("No card defined for playback !");
			}
			if (captcard==NULL) {
				ms_warning("No card defined for capture !");
			}
			/*Don't use file or soundcard capture when placed in recv-only mode*/
			if (stream->rtp_port==0
					|| stream->dir==SalStreamRecvOnly
					|| (stream->multicast_role == SalMulticastReceiver && is_multicast)){
				captcard=NULL;
				playfile=NULL;
			}
			if (next_state == LinphoneCallPaused){
				/*in paused state, we never use soundcard*/
				playcard=NULL;
				captcard=NULL;
				recfile=NULL;
				/*And we will eventually play "playfile" if set by the user*/
			}
			if (call->playing_ringbacktone){
				captcard=NULL;
				playfile=NULL;/* it is setup later*/
				if (lp_config_get_int(lc->config,"sound","send_ringback_without_playback", 0) == 1){
					playcard = NULL;
					recfile = NULL;
				}
			}
			/*if playfile are supplied don't use soundcards*/
			if (lc->use_files || (use_rtp_io && !use_rtp_io_enable_local_output)) {	
				captcard=NULL;
				playcard=NULL;
			}
			if (call->params->in_conference){
				/* first create the graph without soundcard resources*/
				captcard=playcard=NULL;
			}
			if (!linphone_call_sound_resources_available(call)){
				ms_message("Sound resources are used by another call, not using soundcard.");
				captcard=playcard=NULL;
			}
			use_ec=captcard==NULL ? FALSE : linphone_core_echo_cancellation_enabled(lc);
			audio_stream_enable_echo_canceller(call->audiostream, use_ec);
			if (playcard &&  stream->max_rate>0) ms_snd_card_set_preferred_sample_rate(playcard, stream->max_rate);
			if (captcard &&  stream->max_rate>0) ms_snd_card_set_preferred_sample_rate(captcard, stream->max_rate);
			audio_stream_enable_adaptive_bitrate_control(call->audiostream,use_arc);
			media_stream_set_adaptive_bitrate_algorithm(&call->audiostream->ms,
								ms_qos_analyzer_algorithm_from_string(linphone_core_get_adaptive_rate_algorithm(lc)));
			
			rtp_session_enable_rtcp_mux(call->audiostream->ms.sessions.rtp_session, stream->rtcp_mux);
			if (!call->params->in_conference && call->params->record_file){
				audio_stream_mixed_record_open(call->audiostream,call->params->record_file);
				call->current_params->record_file=ms_strdup(call->params->record_file);
			}
			/* valid local tags are > 0 */
			if (sal_stream_description_has_srtp(stream) == TRUE) {
				local_st_desc=sal_media_description_find_stream(call->localdesc,stream->proto,SalAudio);
				crypto_idx = find_crypto_index_from_tag(local_st_desc->crypto, stream->crypto_local_tag);

				if (crypto_idx >= 0) {
					ms_media_stream_sessions_set_srtp_recv_key_b64(&call->audiostream->ms.sessions, stream->crypto[0].algo,stream->crypto[0].master_key);
					ms_media_stream_sessions_set_srtp_send_key_b64(&call->audiostream->ms.sessions, stream->crypto[0].algo,local_st_desc->crypto[crypto_idx].master_key);
				} else {
					ms_warning("Failed to find local crypto algo with tag: %d", stream->crypto_local_tag);
				}
			}
			configure_rtp_session_for_rtcp_fb(call, stream);
			configure_rtp_session_for_rtcp_xr(lc, call, SalAudio);
			if (is_multicast)
				rtp_session_set_multicast_ttl(call->audiostream->ms.sessions.rtp_session,stream->ttl);

			if (use_rtp_io) {
				if(use_rtp_io_enable_local_output){
					io.input.type = MSResourceRtp;
					io.input.session = create_audio_rtp_io_session(call);
					
					if (playcard){
						io.output.type = MSResourceSoundcard;
						io.output.soundcard = playcard;
					}else{
						io.output.type = MSResourceFile;
						io.output.file = recfile;
					}
				}
				else {
					io.input.type = io.output.type = MSResourceRtp;
					io.input.session = io.output.session = create_audio_rtp_io_session(call);
				}
			
				if (io.input.session == NULL) {
					ok = FALSE;
				}
			}else  {
				if (playcard){
					io.output.type = MSResourceSoundcard;
					io.output.soundcard = playcard;
				}else{
					io.output.type = MSResourceFile;
					io.output.file = recfile;
				}
				if (captcard){
					io.input.type = MSResourceSoundcard;
					io.input.soundcard = captcard;
				}else{
					io.input.type = MSResourceFile;
					file_to_play = playfile;
					io.input.file = NULL; /*we prefer to use the remote_play api, that allows to play multimedia files */
				}

			}
			if (ok == TRUE) {
				int err = audio_stream_start_from_io(call->audiostream,
					call->audio_profile,
					rtp_addr,
					stream->rtp_port,
					stream->rtcp_addr[0]!='\0' ? stream->rtcp_addr : call->resultdesc->addr,
					(linphone_core_rtcp_enabled(lc) && !is_multicast) ? (stream->rtcp_port ? stream->rtcp_port : stream->rtp_port+1) : 0,
					used_pt,
					&io);
				if (err == 0){
					post_configure_audio_streams(call, (call->all_muted || call->audio_muted) && !call->playing_ringbacktone);
				}
			}

			ms_media_stream_sessions_set_encryption_mandatory(&call->audiostream->ms.sessions, linphone_call_encryption_mandatory(call));

			if (next_state == LinphoneCallPaused && captcard == NULL && playfile != NULL){
				int pause_time=500;
				ms_filter_call_method(call->audiostream->soundread,MS_FILE_PLAYER_LOOP,&pause_time);
			}
			if (call->playing_ringbacktone){
				setup_ring_player(lc,call);
			}

			if (call->params->in_conference){
				/*transform the graph to connect it to the conference filter */
				mute = stream->dir==SalStreamRecvOnly;
				linphone_conference_on_call_stream_starting(lc->conf_ctx, call, mute);
			}
			call->current_params->in_conference=call->params->in_conference;
			call->current_params->low_bandwidth=call->params->low_bandwidth;

			/* start ZRTP engine if needed : set here or remote have a zrtp-hash attribute */
			if (linphone_core_media_encryption_supported(lc, LinphoneMediaEncryptionZRTP) &&
				(call->params->media_encryption == LinphoneMediaEncryptionZRTP || remote_stream->haveZrtpHash==1) ){
				audio_stream_start_zrtp(call->audiostream);
				if (remote_stream->haveZrtpHash == 1) {
					int retval;
					if ((retval = ms_zrtp_setPeerHelloHash(call->audiostream->ms.sessions.zrtp_context, (uint8_t *)remote_stream->zrtphash, strlen((const char *)(remote_stream->zrtphash)))) != 0) {
						ms_error("Zrtp hash mismatch 0x%x", retval);
					}
				}
			}
		}else ms_warning("No audio stream accepted ?");
	}
	linphone_call_set_on_hold_file(call, file_to_play);
}

#ifdef VIDEO_ENABLED
static RtpSession * create_video_rtp_io_session(LinphoneCall *call) {
	PayloadType *pt;
	LinphoneCore *lc = call->core;
	const char *local_ip = lp_config_get_string(lc->config, "video", "rtp_local_addr", "127.0.0.1");
	const char *remote_ip = lp_config_get_string(lc->config, "video", "rtp_remote_addr", "127.0.0.1");
	int local_port = lp_config_get_int(lc->config, "video", "rtp_local_port", 19076);
	int remote_port = lp_config_get_int(lc->config, "video", "rtp_remote_port", 19078);
	int ptnum = lp_config_get_int(lc->config, "video", "rtp_ptnum", 0);
	const char *rtpmap = lp_config_get_string(lc->config, "video", "rtp_map", "vp8/90000/1");
	int symmetric = lp_config_get_int(lc->config, "video", "rtp_symmetric", 0);
	int jittcomp = lp_config_get_int(lc->config, "video", "rtp_jittcomp", 0); /* 0 means no jitter buffer*/
	RtpSession *rtp_session = NULL;
	pt = rtp_profile_get_payload_from_rtpmap(call->video_profile, rtpmap);
	if (pt != NULL) {
		call->rtp_io_video_profile = rtp_profile_new("RTP IO video profile");
		rtp_profile_set_payload(call->rtp_io_video_profile, ptnum, payload_type_clone(pt));
		rtp_session = ms_create_duplex_rtp_session(local_ip, local_port, -1, ms_factory_get_mtu(lc->factory));
		rtp_session_set_profile(rtp_session, call->rtp_io_video_profile);
		rtp_session_set_remote_addr_and_port(rtp_session, remote_ip, remote_port, -1);
		rtp_session_enable_rtcp(rtp_session, FALSE);
		rtp_session_set_payload_type(rtp_session, ptnum);
		rtp_session_set_symmetric_rtp(rtp_session, (bool_t)symmetric);
		rtp_session_set_jitter_compensation(rtp_session, jittcomp);
		rtp_session_enable_jitter_buffer(rtp_session, jittcomp>0);
	}
	return rtp_session;
}
#endif

static void linphone_call_start_video_stream(LinphoneCall *call, LinphoneCallState next_state){
#ifdef VIDEO_ENABLED
	LinphoneCore *lc=call->core;
	int used_pt=-1;
	const SalStreamDescription *vstream;
	MSFilter* source = NULL;
	bool_t reused_preview = FALSE;
	bool_t use_rtp_io = lp_config_get_int(lc->config, "video", "rtp_io", FALSE);
	MSMediaStreamIO io = MS_MEDIA_STREAM_IO_INITIALIZER;

	/* shutdown preview */
	if (lc->previewstream!=NULL) {
		if( lc->video_conf.reuse_preview_source == FALSE) video_preview_stop(lc->previewstream);
		else source = video_preview_stop_reuse_source(lc->previewstream);
		lc->previewstream=NULL;
	}

	vstream = sal_media_description_find_best_stream(call->resultdesc, SalVideo);
	if (vstream!=NULL && vstream->dir!=SalStreamInactive && vstream->rtp_port!=0) {
		const char *rtp_addr=vstream->rtp_addr[0]!='\0' ? vstream->rtp_addr : call->resultdesc->addr;
		const char *rtcp_addr=vstream->rtcp_addr[0]!='\0' ? vstream->rtcp_addr : call->resultdesc->addr;
		const SalStreamDescription *local_st_desc=sal_media_description_find_stream(call->localdesc,vstream->proto,SalVideo);
		bool_t is_multicast=ms_is_multicast(rtp_addr);
		call->video_profile=make_profile(call,call->resultdesc,vstream,&used_pt);

		if (used_pt!=-1){
			MediaStreamDir dir= MediaStreamSendRecv;
			bool_t is_inactive=FALSE;
			MSWebCam *cam;

			call->current_params->video_codec = rtp_profile_get_payload(call->video_profile, used_pt);
			call->current_params->has_video=TRUE;

			video_stream_enable_adaptive_bitrate_control(call->videostream,
													  linphone_core_adaptive_rate_control_enabled(lc));
			media_stream_set_adaptive_bitrate_algorithm(&call->videostream->ms,
													  ms_qos_analyzer_algorithm_from_string(linphone_core_get_adaptive_rate_algorithm(lc)));
			rtp_session_enable_rtcp_mux(call->videostream->ms.sessions.rtp_session, vstream->rtcp_mux);
			if (lc->video_conf.preview_vsize.width!=0)
				video_stream_set_preview_size(call->videostream,lc->video_conf.preview_vsize);
			video_stream_set_fps(call->videostream,linphone_core_get_preferred_framerate(lc));
			if (lp_config_get_int(lc->config, "video", "nowebcam_uses_normal_fps", 0))
				call->videostream->staticimage_webcam_fps_optimization = FALSE;
			video_stream_set_sent_video_size(call->videostream,linphone_core_get_preferred_video_size(lc));
			video_stream_enable_self_view(call->videostream,lc->video_conf.selfview);
			if (call->video_window_id != NULL)
				video_stream_set_native_window_id(call->videostream, call->video_window_id);
			else if (lc->video_window_id != NULL)
				video_stream_set_native_window_id(call->videostream, lc->video_window_id);
			if (lc->preview_window_id != NULL)
				video_stream_set_native_preview_window_id(call->videostream, lc->preview_window_id);
			video_stream_use_preview_video_window (call->videostream,lc->use_preview_window);

			if (is_multicast){
				if (vstream->multicast_role == SalMulticastReceiver)
					dir=MediaStreamRecvOnly;
				else
					dir=MediaStreamSendOnly;
			} else if (vstream->dir==SalStreamSendOnly && lc->video_conf.capture ){
				dir=MediaStreamSendOnly;
			}else if (vstream->dir==SalStreamRecvOnly && lc->video_conf.display ){
				dir=MediaStreamRecvOnly;
			}else if (vstream->dir==SalStreamSendRecv){
				if (lc->video_conf.display && lc->video_conf.capture)
					dir=MediaStreamSendRecv;
				else if (lc->video_conf.display)
					dir=MediaStreamRecvOnly;
				else
					dir=MediaStreamSendOnly;
			}else{
				ms_warning("video stream is inactive.");
				/*either inactive or incompatible with local capabilities*/
				is_inactive=TRUE;
			}
			cam = linphone_call_get_video_device(call);
			if (!is_inactive){
				/* get remote stream description to check for zrtp-hash presence */
				SalMediaDescription *remote_desc = sal_call_get_remote_media_description(call->op);
				const SalStreamDescription *remote_stream = sal_media_description_find_best_stream(remote_desc, SalVideo);

				if (sal_stream_description_has_srtp(vstream) == TRUE) {
					int crypto_idx = find_crypto_index_from_tag(local_st_desc->crypto, vstream->crypto_local_tag);
					if (crypto_idx >= 0) {
						ms_media_stream_sessions_set_srtp_recv_key_b64(&call->videostream->ms.sessions, vstream->crypto[0].algo,vstream->crypto[0].master_key);
						ms_media_stream_sessions_set_srtp_send_key_b64(&call->videostream->ms.sessions, vstream->crypto[0].algo,local_st_desc->crypto[crypto_idx].master_key);
					}
				}
				configure_rtp_session_for_rtcp_fb(call, vstream);
				configure_rtp_session_for_rtcp_xr(lc, call, SalVideo);

				call->log->video_enabled = TRUE;
				video_stream_set_direction (call->videostream, dir);
				ms_message("%s lc rotation:%d\n", __FUNCTION__, lc->device_rotation);
				video_stream_set_device_rotation(call->videostream, lc->device_rotation);
				video_stream_set_freeze_on_error(call->videostream, lp_config_get_int(lc->config, "video", "freeze_on_error", 1));
				if (is_multicast)
					rtp_session_set_multicast_ttl(call->videostream->ms.sessions.rtp_session,vstream->ttl);

				video_stream_use_video_preset(call->videostream, lp_config_get_string(lc->config, "video", "preset", NULL));
				if (lc->video_conf.reuse_preview_source && source) {
					ms_message("video_stream_start_with_source kept: %p", source);
					video_stream_start_with_source(call->videostream,
												   call->video_profile, rtp_addr, vstream->rtp_port,
												   rtcp_addr,
												   linphone_core_rtcp_enabled(lc) ? (vstream->rtcp_port ? vstream->rtcp_port : vstream->rtp_port+1) : 0,
												   used_pt, -1, cam, source);
					reused_preview = TRUE;
				} else {
					bool_t ok = TRUE;
					if (use_rtp_io) {
						io.input.type = io.output.type = MSResourceRtp;
						io.input.session = io.output.session = create_video_rtp_io_session(call);
						if (io.input.session == NULL) {
							ok = FALSE;
							ms_warning("Cannot create video RTP IO session");
						}
					} else {
						io.input.type = MSResourceCamera;
						io.input.camera = cam;
						io.output.type = MSResourceDefault;
					}
					if (ok) {
						video_stream_start_from_io(call->videostream,
							call->video_profile, rtp_addr, vstream->rtp_port,
							rtcp_addr,
							(linphone_core_rtcp_enabled(lc) && !is_multicast)  ? (vstream->rtcp_port ? vstream->rtcp_port : vstream->rtp_port+1) : 0,
							used_pt, &io);
					}
				}
				ms_media_stream_sessions_set_encryption_mandatory(&call->videostream->ms.sessions,
					linphone_call_encryption_mandatory(call));
				_linphone_call_set_next_video_frame_decoded_trigger(call);

				/* start ZRTP engine if needed : set here or remote have a zrtp-hash attribute */
				if (call->params->media_encryption==LinphoneMediaEncryptionZRTP || remote_stream->haveZrtpHash==1) {
					/*audio stream is already encrypted and video stream is active*/
					if (media_stream_secured((MediaStream *)call->audiostream) && media_stream_get_state((MediaStream *)call->videostream) == MSStreamStarted) {
						video_stream_start_zrtp(call->videostream);
						if (remote_stream->haveZrtpHash == 1) {
							int retval;
							if ((retval = ms_zrtp_setPeerHelloHash(call->videostream->ms.sessions.zrtp_context, (uint8_t *)remote_stream->zrtphash, strlen((const char *)(remote_stream->zrtphash)))) != 0) {
								ms_error("video stream ZRTP hash mismatch 0x%x", retval);
							}
						}
					}
				}
			}
		}else ms_warning("No video stream accepted.");
	}else{
		ms_message("No valid video stream defined.");
	}
	if( reused_preview == FALSE && source != NULL ){
		/* destroy not-reused source filter */
		ms_warning("Video preview (%p) not reused: destroying it.", source);
		ms_filter_destroy(source);
	}

#endif
}

static void real_time_text_character_received(void *userdata, struct _MSFilter *f, unsigned int id, void *arg) {
	if (id == MS_RTT_4103_RECEIVED_CHAR) {
		LinphoneCall *call = (LinphoneCall *)userdata;
		RealtimeTextReceivedCharacter *data = (RealtimeTextReceivedCharacter *)arg;
		LinphoneChatRoom * chat_room = linphone_call_get_chat_room(call);
		linphone_core_real_time_text_received(call->core, chat_room, data->character, call);
	}
}

static void linphone_call_start_text_stream(LinphoneCall *call) {
	LinphoneCore *lc = call->core;
	int used_pt = -1;
	const SalStreamDescription *tstream;

	tstream = sal_media_description_find_best_stream(call->resultdesc, SalText);
	if (tstream != NULL && tstream->dir != SalStreamInactive && tstream->rtp_port != 0) {
		const char *rtp_addr = tstream->rtp_addr[0] != '\0' ? tstream->rtp_addr : call->resultdesc->addr;
		const char *rtcp_addr = tstream->rtcp_addr[0] != '\0' ? tstream->rtcp_addr : call->resultdesc->addr;
		const SalStreamDescription *local_st_desc = sal_media_description_find_stream(call->localdesc, tstream->proto, SalText);
		bool_t is_multicast = ms_is_multicast(rtp_addr);
		call->text_profile = make_profile(call, call->resultdesc, tstream, &used_pt);

		if (used_pt != -1) {
			call->current_params->text_codec = rtp_profile_get_payload(call->text_profile, used_pt);
			call->current_params->realtimetext_enabled = TRUE;

			if (sal_stream_description_has_srtp(tstream) == TRUE) {
				int crypto_idx = find_crypto_index_from_tag(local_st_desc->crypto, tstream->crypto_local_tag);
				if (crypto_idx >= 0) {
					ms_media_stream_sessions_set_srtp_recv_key_b64(&call->textstream->ms.sessions, tstream->crypto[0].algo, tstream->crypto[0].master_key);
					ms_media_stream_sessions_set_srtp_send_key_b64(&call->textstream->ms.sessions, tstream->crypto[0].algo, local_st_desc->crypto[crypto_idx].master_key);
				}
			}

			configure_rtp_session_for_rtcp_fb(call, tstream);
			configure_rtp_session_for_rtcp_xr(lc, call, SalText);
			rtp_session_enable_rtcp_mux(call->textstream->ms.sessions.rtp_session, tstream->rtcp_mux);

			if (is_multicast) rtp_session_set_multicast_ttl(call->textstream->ms.sessions.rtp_session,tstream->ttl);

			text_stream_start(call->textstream, call->text_profile, rtp_addr, tstream->rtp_port, rtcp_addr, (linphone_core_rtcp_enabled(lc) && !is_multicast)  ? (tstream->rtcp_port ? tstream->rtcp_port : tstream->rtp_port + 1) : 0, used_pt);
			ms_filter_add_notify_callback(call->textstream->rttsink, real_time_text_character_received, call, FALSE);

			ms_media_stream_sessions_set_encryption_mandatory(&call->textstream->ms.sessions,
				linphone_call_encryption_mandatory(call));
		} else ms_warning("No text stream accepted.");
	} else {
		ms_message("No valid text stream defined.");
	}
}

void linphone_call_set_symmetric_rtp(LinphoneCall *call, bool_t val){
	int i;
	for (i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; ++i){
		MSMediaStreamSessions *mss = &call->sessions[i];
		if (mss->rtp_session){
			rtp_session_set_symmetric_rtp(mss->rtp_session, val);
		}
	}
}

void linphone_call_start_media_streams(LinphoneCall *call, LinphoneCallState next_state){
	LinphoneCore *lc=call->core;
	bool_t use_arc = linphone_core_adaptive_rate_control_enabled(lc);
#ifdef VIDEO_ENABLED
	const SalStreamDescription *vstream=sal_media_description_find_best_stream(call->resultdesc,SalVideo);
#endif

	switch (next_state){
		case LinphoneCallIncomingEarlyMedia:
			if (linphone_core_get_remote_ringback_tone(lc)){
				call->playing_ringbacktone = TRUE;
			}
		case LinphoneCallOutgoingEarlyMedia:
			if (!call->params->real_early_media){
				call->all_muted = TRUE;
			}
		break;
		default:
			call->playing_ringbacktone = FALSE;
			call->all_muted = FALSE;
		break;
	}

	call->current_params->audio_codec = NULL;
	call->current_params->video_codec = NULL;
	call->current_params->text_codec = NULL;

	if ((call->audiostream == NULL) && (call->videostream == NULL)) {
		ms_fatal("start_media_stream() called without prior init !");
		return;
	}

	if (call->ice_session != NULL){
		/*if there is an ICE session when we are about to start streams, then ICE will conduct the media path checking and authentication properly.
		 * Symmetric RTP must be turned off*/
		linphone_call_set_symmetric_rtp(call, FALSE);
	}

	call->nb_media_starts++;
#if defined(VIDEO_ENABLED)
	if (vstream!=NULL && vstream->dir!=SalStreamInactive && vstream->payloads!=NULL){
		/*when video is used, do not make adaptive rate control on audio, it is stupid.*/
		use_arc=FALSE;
	}
#endif
	ms_message("linphone_call_start_media_streams() call=[%p] local upload_bandwidth=[%i] kbit/s; local download_bandwidth=[%i] kbit/s",
		   call, linphone_core_get_upload_bandwidth(lc),linphone_core_get_download_bandwidth(lc));

	call->current_params->has_audio = FALSE;
	if (call->audiostream!=NULL) {
		linphone_call_start_audio_stream(call, next_state, use_arc);
	} else {
		ms_warning("linphone_call_start_media_streams(): no audio stream!");
	}
	call->current_params->has_video=FALSE;
	if (call->videostream!=NULL) {
		if (call->audiostream) audio_stream_link_video(call->audiostream,call->videostream);
		linphone_call_start_video_stream(call, next_state);
	}
	/*the onhold file is to be played once both audio and video are ready.*/
	if (call->onhold_file && !call->params->in_conference && call->audiostream){
		MSFilter *player = audio_stream_open_remote_play(call->audiostream, call->onhold_file);
		if (player){
			int pause_time=500;
			ms_filter_call_method(player, MS_PLAYER_SET_LOOP, &pause_time);
			ms_filter_call_method_noarg(player, MS_PLAYER_START);
		}
	}

	call->up_bw=linphone_core_get_upload_bandwidth(lc);

	if (call->params->realtimetext_enabled) {
		linphone_call_start_text_stream(call);
	}

	set_dtls_fingerprint_on_all_streams(call);

	if ((call->ice_session != NULL) && (ice_session_state(call->ice_session) != IS_Completed)) {
		if (call->params->media_encryption==LinphoneMediaEncryptionDTLS) {
			call->current_params->update_call_when_ice_completed = FALSE;
			ms_message("Disabling update call when ice completed on call [%p]",call);
		}
		ice_session_start_connectivity_checks(call->ice_session);
	} else {
		/*should not start dtls until ice is completed*/
		start_dtls_on_all_streams(call);
	}

}

void linphone_call_stop_media_streams_for_ice_gathering(LinphoneCall *call){
	if(call->audiostream && call->audiostream->ms.state==MSStreamPreparing) audio_stream_unprepare_sound(call->audiostream);
#ifdef VIDEO_ENABLED
	if (call->videostream && call->videostream->ms.state==MSStreamPreparing) {
		video_stream_unprepare_video(call->videostream);
	}
#endif
	if (call->textstream && call->textstream->ms.state == MSStreamPreparing) {
		text_stream_unprepare_text(call->textstream);
	}
}

static bool_t update_stream_crypto_params(LinphoneCall *call, const SalStreamDescription *local_st_desc, SalStreamDescription *old_stream, SalStreamDescription *new_stream, MediaStream *ms){
	int crypto_idx = find_crypto_index_from_tag(local_st_desc->crypto, new_stream->crypto_local_tag);
	if (crypto_idx >= 0) {
		if (call->localdesc_changed & SAL_MEDIA_DESCRIPTION_CRYPTO_KEYS_CHANGED)
			ms_media_stream_sessions_set_srtp_send_key_b64(&ms->sessions, new_stream->crypto[0].algo,local_st_desc->crypto[crypto_idx].master_key);
		if (strcmp(old_stream->crypto[0].master_key,new_stream->crypto[0].master_key)!=0){
			ms_media_stream_sessions_set_srtp_recv_key_b64(&ms->sessions, new_stream->crypto[0].algo,new_stream->crypto[0].master_key);
		}
		return TRUE;
	} else {
		ms_warning("Failed to find local crypto algo with tag: %d", new_stream->crypto_local_tag);
	}
	return FALSE;
}

void linphone_call_update_crypto_parameters(LinphoneCall *call, SalMediaDescription *old_md, SalMediaDescription *new_md) {
	SalStreamDescription *old_stream;
	SalStreamDescription *new_stream;
	const SalStreamDescription *local_st_desc;

	local_st_desc = sal_media_description_find_secure_stream_of_type(call->localdesc, SalAudio);
	old_stream = sal_media_description_find_secure_stream_of_type(old_md, SalAudio);
	new_stream = sal_media_description_find_secure_stream_of_type(new_md, SalAudio);
	if (call->audiostream && local_st_desc && old_stream && new_stream &&
		update_stream_crypto_params(call,local_st_desc,old_stream,new_stream,&call->audiostream->ms)){
	}

	local_st_desc = sal_media_description_find_secure_stream_of_type(call->localdesc, SalText);
	old_stream = sal_media_description_find_secure_stream_of_type(old_md, SalText);
	new_stream = sal_media_description_find_secure_stream_of_type(new_md, SalText);
	if (call->textstream && local_st_desc && old_stream && new_stream &&
		update_stream_crypto_params(call,local_st_desc,old_stream,new_stream,&call->textstream->ms)){
	}

	start_dtls_on_all_streams(call);

#ifdef VIDEO_ENABLED
	local_st_desc = sal_media_description_find_secure_stream_of_type(call->localdesc, SalVideo);
	old_stream = sal_media_description_find_secure_stream_of_type(old_md, SalVideo);
	new_stream = sal_media_description_find_secure_stream_of_type(new_md, SalVideo);
	if (call->videostream && local_st_desc && old_stream && new_stream &&
		update_stream_crypto_params(call,local_st_desc,old_stream,new_stream,&call->videostream->ms)){
	}
#endif
}

void linphone_call_update_remote_session_id_and_ver(LinphoneCall *call) {
	SalMediaDescription *remote_desc = sal_call_get_remote_media_description(call->op);
	if (remote_desc) {
		call->remote_session_id = remote_desc->session_id;
		call->remote_session_ver = remote_desc->session_ver;
	}
}

void linphone_call_delete_ice_session(LinphoneCall *call){
	if (call->ice_session != NULL) {
		ice_session_destroy(call->ice_session);
		call->ice_session = NULL;
		if (call->audiostream != NULL) call->audiostream->ms.ice_check_list = NULL;
		if (call->videostream != NULL) call->videostream->ms.ice_check_list = NULL;
		if (call->textstream != NULL) call->textstream->ms.ice_check_list = NULL;
		call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = LinphoneIceStateNotActivated;
		call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = LinphoneIceStateNotActivated;
		call->stats[LINPHONE_CALL_STATS_TEXT].ice_state = LinphoneIceStateNotActivated;
	}
}

void linphone_call_delete_upnp_session(LinphoneCall *call){
#ifdef BUILD_UPNP
	if(call->upnp_session!=NULL) {
		linphone_upnp_session_destroy(call->upnp_session);
		call->upnp_session=NULL;
	}
#endif //BUILD_UPNP
}

static void linphone_call_log_fill_stats(LinphoneCallLog *log, MediaStream *st){
	float quality=media_stream_get_average_quality_rating(st);
	if (quality>=0){
		if (log->quality!=-1){
			log->quality*=quality/5.0f;
		}else log->quality=quality;
	}
}

static void update_rtp_stats(LinphoneCall *call, int stream_index) {
	if (stream_index >= linphone_call_get_stream_count(call)) {
		return;
	}

	if (call->sessions[stream_index].rtp_session) {
		const rtp_stats_t *stats = rtp_session_get_stats(call->sessions[stream_index].rtp_session);
		memcpy(&call->stats[stream_index].rtp_stats, stats, sizeof(*stats));
	}
}

static void linphone_call_stop_audio_stream(LinphoneCall *call) {
	LinphoneCore *lc = call->core;
	if (call->audiostream!=NULL) {
		linphone_reporting_update_media_info(call, LINPHONE_CALL_STATS_AUDIO);
		media_stream_reclaim_sessions(&call->audiostream->ms,&call->sessions[call->main_audio_stream_index]);

		if (call->audiostream->ec){
			char *state_str=NULL;
			ms_filter_call_method(call->audiostream->ec,MS_ECHO_CANCELLER_GET_STATE_STRING,&state_str);
			if (state_str){
				ms_message("Writing echo canceler state, %i bytes",(int)strlen(state_str));
				lp_config_write_relative_file(call->core->config, EC_STATE_STORE, state_str);
			}
		}
		audio_stream_get_local_rtp_stats(call->audiostream,&call->log->local_stats);
		linphone_call_log_fill_stats (call->log,(MediaStream*)call->audiostream);
		if (call->endpoint){
			linphone_conference_on_call_stream_stopping(lc->conf_ctx, call);
		}
		update_rtp_stats(call, call->main_audio_stream_index);
		audio_stream_stop(call->audiostream);
		call->audiostream=NULL;
		linphone_call_handle_stream_events(call, call->main_audio_stream_index);
		rtp_session_unregister_event_queue(call->sessions[call->main_audio_stream_index].rtp_session, call->audiostream_app_evq);
		ortp_ev_queue_flush(call->audiostream_app_evq);
		ortp_ev_queue_destroy(call->audiostream_app_evq);
		call->audiostream_app_evq=NULL;

		call->current_params->audio_codec = NULL;
	}
}

static void linphone_call_stop_video_stream(LinphoneCall *call) {
#ifdef VIDEO_ENABLED
	if (call->videostream!=NULL){
		linphone_reporting_update_media_info(call, LINPHONE_CALL_STATS_VIDEO);
		media_stream_reclaim_sessions(&call->videostream->ms,&call->sessions[call->main_video_stream_index]);
		linphone_call_log_fill_stats(call->log,(MediaStream*)call->videostream);
		update_rtp_stats(call, call->main_video_stream_index);
		video_stream_stop(call->videostream);
		call->videostream=NULL;
		linphone_call_handle_stream_events(call, call->main_video_stream_index);
		rtp_session_unregister_event_queue(call->sessions[call->main_video_stream_index].rtp_session, call->videostream_app_evq);
		ortp_ev_queue_flush(call->videostream_app_evq);
		ortp_ev_queue_destroy(call->videostream_app_evq);
		call->videostream_app_evq=NULL;
		call->current_params->video_codec = NULL;
	}
#endif
}

static void unset_rtp_profile(LinphoneCall *call, int i){
	if (call->sessions[i].rtp_session)
		rtp_session_set_profile(call->sessions[i].rtp_session,&av_profile);
}

static void linphone_call_stop_text_stream(LinphoneCall *call) {
	if (call->textstream != NULL) {
		linphone_reporting_update_media_info(call, LINPHONE_CALL_STATS_TEXT);
		media_stream_reclaim_sessions(&call->textstream->ms, &call->sessions[call->main_text_stream_index]);
		linphone_call_log_fill_stats(call->log, (MediaStream*)call->textstream);
		update_rtp_stats(call, call->main_text_stream_index);
		text_stream_stop(call->textstream);
		call->textstream = NULL;
		linphone_call_handle_stream_events(call, call->main_text_stream_index);
		rtp_session_unregister_event_queue(call->sessions[call->main_text_stream_index].rtp_session, call->textstream_app_evq);
		ortp_ev_queue_flush(call->textstream_app_evq);
		ortp_ev_queue_destroy(call->textstream_app_evq);
		call->textstream_app_evq = NULL;
		call->current_params->text_codec = NULL;
	}
}

void linphone_call_stop_media_streams(LinphoneCall *call){
	if (call->audiostream || call->videostream || call->textstream) {
		if (call->audiostream && call->videostream)
			audio_stream_unlink_video(call->audiostream, call->videostream);
		linphone_call_stop_audio_stream(call);
		linphone_call_stop_video_stream(call);
		linphone_call_stop_text_stream(call);

		if (call->core->msevq != NULL) {
			ms_event_queue_skip(call->core->msevq);
		}
	}

	if (call->audio_profile){
		rtp_profile_destroy(call->audio_profile);
		call->audio_profile=NULL;
		unset_rtp_profile(call,call->main_audio_stream_index);
	}
	if (call->video_profile){
		rtp_profile_destroy(call->video_profile);
		call->video_profile=NULL;
		unset_rtp_profile(call,call->main_video_stream_index);
	}
	if (call->text_profile){
		rtp_profile_destroy(call->text_profile);
		call->text_profile=NULL;
		unset_rtp_profile(call,call->main_text_stream_index);
	}
	if (call->rtp_io_audio_profile) {
		rtp_profile_destroy(call->rtp_io_audio_profile);
		call->rtp_io_audio_profile = NULL;
	}
	if (call->rtp_io_video_profile) {
		rtp_profile_destroy(call->rtp_io_video_profile);
		call->rtp_io_video_profile = NULL;
	}

	linphone_core_soundcard_hint_check(call->core);
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

float linphone_call_get_play_volume(LinphoneCall *call){
	AudioStream *st=call->audiostream;
	if (st && st->volrecv){
		float vol=0;
		ms_filter_call_method(st->volrecv,MS_VOLUME_GET,&vol);
		return vol;

	}
	return LINPHONE_VOLUME_DB_LOWEST;
}

float linphone_call_get_record_volume(LinphoneCall *call){
	AudioStream *st=call->audiostream;
	if (st && st->volsend && !call->audio_muted && call->state==LinphoneCallStreamsRunning){
		float vol=0;
		ms_filter_call_method(st->volsend,MS_VOLUME_GET,&vol);
		return vol;

	}
	return LINPHONE_VOLUME_DB_LOWEST;
}

float linphone_call_get_speaker_volume_gain(const LinphoneCall *call) {
	if(call->audiostream) return audio_stream_get_sound_card_output_gain(call->audiostream);
	else {
		ms_error("Could not get playback volume: no audio stream");
		return -1.0f;
	}
}

void linphone_call_set_speaker_volume_gain(LinphoneCall *call, float volume) {
	if(call->audiostream) audio_stream_set_sound_card_output_gain(call->audiostream, volume);
	else ms_error("Could not set playback volume: no audio stream");
}

float linphone_call_get_microphone_volume_gain(const LinphoneCall *call) {
	if(call->audiostream) return audio_stream_get_sound_card_input_gain(call->audiostream);
	else {
		ms_error("Could not get record volume: no audio stream");
		return -1.0f;
	}
}

void linphone_call_set_microphone_volume_gain(LinphoneCall *call, float volume) {
	if(call->audiostream) audio_stream_set_sound_card_input_gain(call->audiostream, volume);
	else ms_error("Could not set record volume: no audio stream");
}

static float agregate_ratings(float audio_rating, float video_rating){
	float result;
	if (audio_rating<0 && video_rating<0) result=-1;
	else if (audio_rating<0) result=video_rating*5.0f;
	else if (video_rating<0) result=audio_rating*5.0f;
	else result=audio_rating*video_rating*5.0f;
	return result;
}

float linphone_call_get_current_quality(LinphoneCall *call){
	float audio_rating=-1.f;
	float video_rating=-1.f;

	if (call->audiostream){
		audio_rating=media_stream_get_quality_rating((MediaStream*)call->audiostream)/5.0f;
	}
	if (call->videostream){
		video_rating=media_stream_get_quality_rating((MediaStream*)call->videostream)/5.0f;
	}
	return agregate_ratings(audio_rating, video_rating);
}

float linphone_call_get_average_quality(LinphoneCall *call){
	float audio_rating=-1.f;
	float video_rating=-1.f;

	if (call->audiostream){
		audio_rating = media_stream_get_average_quality_rating((MediaStream*)call->audiostream)/5.0f;
	}
	if (call->videostream){
		video_rating = media_stream_get_average_quality_rating((MediaStream*)call->videostream)/5.0f;
	}
	return agregate_ratings(audio_rating, video_rating);
}

static void update_local_stats(LinphoneCallStats *stats, MediaStream *stream) {
	const MSQualityIndicator *qi = media_stream_get_quality_indicator(stream);
	if (qi) {
		stats->local_late_rate=ms_quality_indicator_get_local_late_rate(qi);
		stats->local_loss_rate=ms_quality_indicator_get_local_loss_rate(qi);
	}
	media_stream_get_local_rtp_stats(stream, &stats->rtp_stats);
}

const LinphoneCallStats *linphone_call_get_audio_stats(LinphoneCall *call) {
	LinphoneCallStats *stats = &call->stats[LINPHONE_CALL_STATS_AUDIO];
	if (call->audiostream){
		update_local_stats(stats,(MediaStream*)call->audiostream);
	}
	return stats;
}

const LinphoneCallStats *linphone_call_get_video_stats(LinphoneCall *call) {
	LinphoneCallStats *stats = &call->stats[LINPHONE_CALL_STATS_VIDEO];
	if (call->videostream){
		update_local_stats(stats,(MediaStream*)call->videostream);
	}
	return stats;
}

const LinphoneCallStats *linphone_call_get_text_stats(LinphoneCall *call) {
	LinphoneCallStats *stats = &call->stats[LINPHONE_CALL_STATS_TEXT];
	if (call->textstream){
		update_local_stats(stats,(MediaStream*)call->textstream);
	}
	return stats;
}

static bool_t ice_in_progress(LinphoneCallStats *stats){
	return stats->ice_state==LinphoneIceStateInProgress;
}

bool_t linphone_call_media_in_progress(LinphoneCall *call){
	bool_t ret=FALSE;
	if (ice_in_progress(&call->stats[LINPHONE_CALL_STATS_AUDIO]) || ice_in_progress(&call->stats[LINPHONE_CALL_STATS_VIDEO]) || ice_in_progress(&call->stats[LINPHONE_CALL_STATS_TEXT]))
		ret=TRUE;
	/*TODO: could check zrtp state, upnp state*/
	return ret;
}

float linphone_call_stats_get_sender_loss_rate(const LinphoneCallStats *stats) {
	const report_block_t *srb = NULL;

	if (!stats || !stats->sent_rtcp)
		return 0.0;
	/* Perform msgpullup() to prevent crashes in rtcp_is_SR() or rtcp_is_RR() if the RTCP packet is composed of several mblk_t structure */
	if (stats->sent_rtcp->b_cont != NULL)
		msgpullup(stats->sent_rtcp, -1);
	if (rtcp_is_SR(stats->sent_rtcp))
		srb = rtcp_SR_get_report_block(stats->sent_rtcp, 0);
	else if (rtcp_is_RR(stats->sent_rtcp))
		srb = rtcp_RR_get_report_block(stats->sent_rtcp, 0);
	if (!srb)
		return 0.0;
	return 100.0f * report_block_get_fraction_lost(srb) / 256.0f;
}

float linphone_call_stats_get_receiver_loss_rate(const LinphoneCallStats *stats) {
	const report_block_t *rrb = NULL;

	if (!stats || !stats->received_rtcp)
		return 0.0;
	/* Perform msgpullup() to prevent crashes in rtcp_is_SR() or rtcp_is_RR() if the RTCP packet is composed of several mblk_t structure */
	if (stats->received_rtcp->b_cont != NULL)
		msgpullup(stats->received_rtcp, -1);
	if (rtcp_is_RR(stats->received_rtcp))
		rrb = rtcp_RR_get_report_block(stats->received_rtcp, 0);
	else if (rtcp_is_SR(stats->received_rtcp))
		rrb = rtcp_SR_get_report_block(stats->received_rtcp, 0);
	if (!rrb)
		return 0.0;
	return 100.0f * report_block_get_fraction_lost(rrb) / 256.0f;
}

float linphone_call_stats_get_sender_interarrival_jitter(const LinphoneCallStats *stats, LinphoneCall *call) {
	const LinphoneCallParams *params;
	const PayloadType *pt;
	const report_block_t *srb = NULL;

	if (!stats || !call || !stats->sent_rtcp)
		return 0.0;
	params = linphone_call_get_current_params(call);
	if (!params)
		return 0.0;
	/* Perform msgpullup() to prevent crashes in rtcp_is_SR() or rtcp_is_RR() if the RTCP packet is composed of several mblk_t structure */
	if (stats->sent_rtcp->b_cont != NULL)
		msgpullup(stats->sent_rtcp, -1);
	if (rtcp_is_SR(stats->sent_rtcp))
		srb = rtcp_SR_get_report_block(stats->sent_rtcp, 0);
	else if (rtcp_is_RR(stats->sent_rtcp))
		srb = rtcp_RR_get_report_block(stats->sent_rtcp, 0);
	if (!srb)
		return 0.0;
	if (stats->type == LINPHONE_CALL_STATS_AUDIO)
		pt = linphone_call_params_get_used_audio_codec(params);
	else
		pt = linphone_call_params_get_used_video_codec(params);
	if (!pt || (pt->clock_rate == 0))
		return 0.0;
	return (float)report_block_get_interarrival_jitter(srb) / (float)pt->clock_rate;
}

float linphone_call_stats_get_receiver_interarrival_jitter(const LinphoneCallStats *stats, LinphoneCall *call) {
	const LinphoneCallParams *params;
	const PayloadType *pt;
	const report_block_t *rrb = NULL;

	if (!stats || !call || !stats->received_rtcp)
		return 0.0;
	params = linphone_call_get_current_params(call);
	if (!params)
		return 0.0;
	/* Perform msgpullup() to prevent crashes in rtcp_is_SR() or rtcp_is_RR() if the RTCP packet is composed of several mblk_t structure */
	if (stats->received_rtcp->b_cont != NULL)
		msgpullup(stats->received_rtcp, -1);
	if (rtcp_is_SR(stats->received_rtcp))
		rrb = rtcp_SR_get_report_block(stats->received_rtcp, 0);
	else if (rtcp_is_RR(stats->received_rtcp))
		rrb = rtcp_RR_get_report_block(stats->received_rtcp, 0);
	if (!rrb)
		return 0.0;
	if (stats->type == LINPHONE_CALL_STATS_AUDIO)
		pt = linphone_call_params_get_used_audio_codec(params);
	else
		pt = linphone_call_params_get_used_video_codec(params);
	if (!pt || (pt->clock_rate == 0))
		return 0.0;
	return (float)report_block_get_interarrival_jitter(rrb) / (float)pt->clock_rate;
}

rtp_stats_t linphone_call_stats_get_rtp_stats(const LinphoneCallStats *stats) {
	rtp_stats_t rtp_stats;
	memset(&rtp_stats, 0, sizeof(rtp_stats));

	if (stats) {
		memcpy(&rtp_stats, &stats->rtp_stats, sizeof(stats->rtp_stats));
	}

	return rtp_stats;
}

uint64_t linphone_call_stats_get_late_packets_cumulative_number(const LinphoneCallStats *stats, LinphoneCall *call) {
	return linphone_call_stats_get_rtp_stats(stats).outoftime;
}

float linphone_call_stats_get_download_bandwidth(const LinphoneCallStats *stats) {
	return stats->download_bandwidth;
}

float linphone_call_stats_get_upload_bandwidth(const LinphoneCallStats *stats) {
	return stats->upload_bandwidth;
}

LinphoneIceState linphone_call_stats_get_ice_state(const LinphoneCallStats *stats) {
	return stats->ice_state;
}

LinphoneUpnpState linphone_call_stats_get_upnp_state(const LinphoneCallStats *stats) {
	return stats->upnp_state;
}

void linphone_call_start_recording(LinphoneCall *call){
	if (!call->params->record_file){
		ms_error("linphone_call_start_recording(): no output file specified. Use linphone_call_params_set_record_file().");
		return;
	}
	if (call->audiostream && !call->params->in_conference){
		audio_stream_mixed_record_start(call->audiostream);
	}
	call->record_active=TRUE;
}

void linphone_call_stop_recording(LinphoneCall *call){
	if (call->audiostream && !call->params->in_conference){
		audio_stream_mixed_record_stop(call->audiostream);
	}
	call->record_active=FALSE;
}

static void report_bandwidth(LinphoneCall *call, MediaStream *as, MediaStream *vs, MediaStream *ts){
	bool_t as_active =  as ? (media_stream_get_state(as) == MSStreamStarted) : FALSE;
	bool_t vs_active =  vs ? (media_stream_get_state(vs) == MSStreamStarted) : FALSE;
	bool_t ts_active =  ts ? (media_stream_get_state(ts) == MSStreamStarted) : FALSE;

	call->stats[LINPHONE_CALL_STATS_AUDIO].download_bandwidth=(as_active) ? (float)(media_stream_get_down_bw(as)*1e-3) : 0.f;
	call->stats[LINPHONE_CALL_STATS_AUDIO].upload_bandwidth=(as_active) ? (float)(media_stream_get_up_bw(as)*1e-3) : 0.f;
	call->stats[LINPHONE_CALL_STATS_VIDEO].download_bandwidth=(vs_active) ? (float)(media_stream_get_down_bw(vs)*1e-3) : 0.f;
	call->stats[LINPHONE_CALL_STATS_VIDEO].upload_bandwidth=(vs_active) ? (float)(media_stream_get_up_bw(vs)*1e-3) : 0.f;
	call->stats[LINPHONE_CALL_STATS_TEXT].download_bandwidth=(ts_active) ? (float)(media_stream_get_down_bw(ts)*1e-3) : 0.f;
	call->stats[LINPHONE_CALL_STATS_TEXT].upload_bandwidth=(ts_active) ? (float)(media_stream_get_up_bw(ts)*1e-3) : 0.f;
	call->stats[LINPHONE_CALL_STATS_AUDIO].rtcp_download_bandwidth=(as_active) ? (float)(media_stream_get_rtcp_down_bw(as)*1e-3) : 0.f;
	call->stats[LINPHONE_CALL_STATS_AUDIO].rtcp_upload_bandwidth=(as_active) ? (float)(media_stream_get_rtcp_up_bw(as)*1e-3) : 0.f;
	call->stats[LINPHONE_CALL_STATS_VIDEO].rtcp_download_bandwidth=(vs_active) ? (float)(media_stream_get_rtcp_down_bw(vs)*1e-3) : 0.f;
	call->stats[LINPHONE_CALL_STATS_VIDEO].rtcp_upload_bandwidth=(vs_active) ? (float)(media_stream_get_rtcp_up_bw(vs)*1e-3) : 0.f;
	call->stats[LINPHONE_CALL_STATS_TEXT].rtcp_download_bandwidth=(ts_active) ? (float)(media_stream_get_rtcp_down_bw(ts)*1e-3) : 0.f;
	call->stats[LINPHONE_CALL_STATS_TEXT].rtcp_upload_bandwidth=(ts_active) ? (float)(media_stream_get_rtcp_up_bw(ts)*1e-3) : 0.f;
	/* If not ipV6, it's not necessary IpV4, should be UNSPEC, TODO */
	call->stats[LINPHONE_CALL_STATS_AUDIO].rtp_remote_family=(as_active)
		? ((ortp_stream_is_ipv6((OrtpStream*)&(as->sessions.rtp_session->rtp.gs))) ? LinphoneAddressFamilyInet6 : LinphoneAddressFamilyInet) : LinphoneAddressFamilyUnspec;
	call->stats[LINPHONE_CALL_STATS_VIDEO].rtp_remote_family=(vs_active)
		? ((ortp_stream_is_ipv6((OrtpStream*)&(vs->sessions.rtp_session->rtp.gs))) ? LinphoneAddressFamilyInet6 : LinphoneAddressFamilyInet) : LinphoneAddressFamilyUnspec;
	call->stats[LINPHONE_CALL_STATS_TEXT].rtp_remote_family=(ts_active)
		? ((ortp_stream_is_ipv6((OrtpStream*)&(ts->sessions.rtp_session->rtp.gs))) ? LinphoneAddressFamilyInet6 : LinphoneAddressFamilyInet) : LinphoneAddressFamilyUnspec;

	if (call->core->send_call_stats_periodical_updates){
		call->stats[LINPHONE_CALL_STATS_AUDIO].updated|=LINPHONE_CALL_STATS_PERIODICAL_UPDATE;
		linphone_core_notify_call_stats_updated(call->core, call, &call->stats[LINPHONE_CALL_STATS_AUDIO]);
		call->stats[LINPHONE_CALL_STATS_AUDIO].updated=0;
		if (as_active) update_local_stats(&call->stats[LINPHONE_CALL_STATS_AUDIO], as);

		call->stats[LINPHONE_CALL_STATS_VIDEO].updated|=LINPHONE_CALL_STATS_PERIODICAL_UPDATE;
		linphone_core_notify_call_stats_updated(call->core, call, &call->stats[LINPHONE_CALL_STATS_VIDEO]);
		call->stats[LINPHONE_CALL_STATS_VIDEO].updated=0;
		if (vs_active) update_local_stats(&call->stats[LINPHONE_CALL_STATS_VIDEO], vs);

		call->stats[LINPHONE_CALL_STATS_TEXT].updated|=LINPHONE_CALL_STATS_PERIODICAL_UPDATE;
		linphone_core_notify_call_stats_updated(call->core, call, &call->stats[LINPHONE_CALL_STATS_TEXT]);
		call->stats[LINPHONE_CALL_STATS_TEXT].updated=0;
		if (ts_active) update_local_stats(&call->stats[LINPHONE_CALL_STATS_TEXT], ts);
	}

	ms_message(	"Bandwidth usage for call [%p]:\n"
				"\tRTP  audio=[d=%5.1f,u=%5.1f], video=[d=%5.1f,u=%5.1f], text=[d=%5.1f,u=%5.1f] kbits/sec\n"
				"\tRTCP audio=[d=%5.1f,u=%5.1f], video=[d=%5.1f,u=%5.1f], text=[d=%5.1f,u=%5.1f] kbits/sec",
				call,
				call->stats[LINPHONE_CALL_STATS_AUDIO].download_bandwidth,
				call->stats[LINPHONE_CALL_STATS_AUDIO].upload_bandwidth,
				call->stats[LINPHONE_CALL_STATS_VIDEO].download_bandwidth,
				call->stats[LINPHONE_CALL_STATS_VIDEO].upload_bandwidth,
				call->stats[LINPHONE_CALL_STATS_TEXT].download_bandwidth,
				call->stats[LINPHONE_CALL_STATS_TEXT].upload_bandwidth,
				call->stats[LINPHONE_CALL_STATS_AUDIO].rtcp_download_bandwidth,
				call->stats[LINPHONE_CALL_STATS_AUDIO].rtcp_upload_bandwidth,
				call->stats[LINPHONE_CALL_STATS_VIDEO].rtcp_download_bandwidth,
				call->stats[LINPHONE_CALL_STATS_VIDEO].rtcp_upload_bandwidth,
				call->stats[LINPHONE_CALL_STATS_TEXT].rtcp_download_bandwidth,
				call->stats[LINPHONE_CALL_STATS_TEXT].rtcp_upload_bandwidth
	);
}

static void linphone_call_lost(LinphoneCall *call, LinphoneReason reason){
	LinphoneCore *lc = call->core;
	char *temp = NULL;
	char *from=NULL;

	from = linphone_call_get_remote_address_as_string(call);
	switch(reason){
		case LinphoneReasonIOError:
			temp = ms_strdup_printf("Call with %s disconnected because of network, it is going to be closed.", from ? from : "?");
		break;
		default:
			temp = ms_strdup_printf("Media connectivity with %s is lost, call is going to be closed.", from ? from : "?");
		break;
	}
	if (from) ms_free(from);
	ms_message("LinphoneCall [%p]: %s",call, temp);
	linphone_core_notify_display_warning(lc, temp);
	linphone_core_terminate_call(lc,call);
	linphone_core_play_named_tone(lc,LinphoneToneCallLost);
	ms_free(temp);
}

static void linphone_call_on_ice_gathering_finished(LinphoneCall *call){
	int ping_time;
	const SalMediaDescription *rmd = sal_call_get_remote_media_description(call->op);
	if (rmd){
		linphone_call_clear_unused_ice_candidates(call, rmd);
	}
	ice_session_compute_candidates_foundations(call->ice_session);
	ice_session_eliminate_redundant_candidates(call->ice_session);
	ice_session_choose_default_candidates(call->ice_session);
	ping_time = ice_session_average_gathering_round_trip_time(call->ice_session);
	if (ping_time >=0) {
		call->ping_time=ping_time;
	}
}

static void handle_ice_events(LinphoneCall *call, OrtpEvent *ev){
	OrtpEventType evt=ortp_event_get_type(ev);
	OrtpEventData *evd=ortp_event_get_data(ev);

	if (evt == ORTP_EVENT_ICE_SESSION_PROCESSING_FINISHED) {
		switch (ice_session_state(call->ice_session)) {
			case IS_Completed:
			case IS_Failed:
				/* At least one ICE session has succeeded, so perform a call update. */
				if (ice_session_has_completed_check_list(call->ice_session) == TRUE) {
					const LinphoneCallParams *current_param =  linphone_call_get_current_params(call);
					if (ice_session_role(call->ice_session) == IR_Controlling && current_param->update_call_when_ice_completed ) {
						LinphoneCallParams *params = linphone_core_create_call_params(call->core, call);
						params->internal_call_update = TRUE;
						linphone_core_update_call(call->core, call, params);
						linphone_call_params_unref(params);
					}
					start_dtls_on_all_streams(call);
				}
				break;
			default:
				break;
		}
		linphone_core_update_ice_state_in_call_stats(call);
	} else if (evt == ORTP_EVENT_ICE_GATHERING_FINISHED) {
		if (evd->info.ice_processing_successful==FALSE) {
			ms_warning("No STUN answer from [%s], continuing without STUN",linphone_core_get_stun_server(call->core));
		}
		linphone_call_on_ice_gathering_finished(call);
		switch (call->state) {
			case LinphoneCallUpdating:
				linphone_core_start_update_call(call->core, call);
				break;
			case LinphoneCallUpdatedByRemote:
				linphone_core_start_accept_call_update(call->core, call,call->prevstate,linphone_call_state_to_string(call->prevstate));
				break;
			case LinphoneCallOutgoingInit:
				linphone_call_stop_media_streams_for_ice_gathering(call);
				linphone_core_proceed_with_invite_if_ready(call->core, call, NULL);
				break;
			case LinphoneCallIdle:
				linphone_call_stop_media_streams_for_ice_gathering(call);
				linphone_call_update_local_media_description_from_ice_or_upnp(call);
				sal_call_set_local_media_description(call->op,call->localdesc);
				linphone_core_notify_incoming_call(call->core, call);
				break;
			default:
				break;
		}
	} else if (evt == ORTP_EVENT_ICE_LOSING_PAIRS_COMPLETED) {
		if (call->state==LinphoneCallUpdatedByRemote){
			linphone_core_start_accept_call_update(call->core, call,call->prevstate,linphone_call_state_to_string(call->prevstate));
			linphone_core_update_ice_state_in_call_stats(call);
		}
	} else if (evt == ORTP_EVENT_ICE_RESTART_NEEDED) {
		ice_session_restart(call->ice_session, IR_Controlling);
		linphone_core_update_call(call->core, call, call->current_params);
	}
}

/*do not change the prototype of this function, it is also used internally in linphone-daemon.*/
void linphone_call_stats_fill(LinphoneCallStats *stats, MediaStream *ms, OrtpEvent *ev){
	OrtpEventType evt=ortp_event_get_type(ev);
	OrtpEventData *evd=ortp_event_get_data(ev);

	if (evt == ORTP_EVENT_RTCP_PACKET_RECEIVED) {
		stats->round_trip_delay = rtp_session_get_round_trip_propagation(ms->sessions.rtp_session);
		if(stats->received_rtcp != NULL)
			freemsg(stats->received_rtcp);
		stats->received_rtcp = evd->packet;
		stats->rtcp_received_via_mux = evd->info.socket_type == OrtpRTPSocket;
		evd->packet = NULL;
		stats->updated = LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE;
		update_local_stats(stats,ms);
	} else if (evt == ORTP_EVENT_RTCP_PACKET_EMITTED) {
		memcpy(&stats->jitter_stats, rtp_session_get_jitter_stats(ms->sessions.rtp_session), sizeof(jitter_stats_t));
		if (stats->sent_rtcp != NULL)
			freemsg(stats->sent_rtcp);
		stats->sent_rtcp = evd->packet;
		evd->packet = NULL;
		stats->updated = LINPHONE_CALL_STATS_SENT_RTCP_UPDATE;
		update_local_stats(stats,ms);
	}
}

void linphone_call_stats_uninit(LinphoneCallStats *stats){
	if (stats->received_rtcp) {
		freemsg(stats->received_rtcp);
		stats->received_rtcp=NULL;
	}
	if (stats->sent_rtcp){
		freemsg(stats->sent_rtcp);
		stats->sent_rtcp=NULL;
	}
}

void linphone_call_notify_stats_updated(LinphoneCall *call, int stream_index){
	LinphoneCallStats *stats = &call->stats[stream_index];
	LinphoneCore *lc = call->core;
	if (stats->updated){
		switch(stats->updated) {
			case LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE:
			case LINPHONE_CALL_STATS_SENT_RTCP_UPDATE:
				linphone_reporting_on_rtcp_update(call, stream_index == call->main_audio_stream_index ? SalAudio : stream_index == call->main_video_stream_index ? SalVideo : SalText);
				break;
			default:
				break;
		}
		linphone_core_notify_call_stats_updated(lc, call, stats);
		stats->updated = 0;
	}
}

static MediaStream * linphone_call_get_media_stream(LinphoneCall *call, int stream_index){
	if (stream_index == call->main_audio_stream_index)
		return (MediaStream*)call->audiostream;
	if (stream_index == call->main_video_stream_index)
		return (MediaStream*)call->videostream;
	if (stream_index == call->main_text_stream_index)
		return (MediaStream*)call->textstream;
	ms_error("linphone_call_get_media_stream(): no stream index %i", stream_index);
	return NULL;
}

static OrtpEvQueue *linphone_call_get_event_queue(LinphoneCall *call, int stream_index){
	if (stream_index == call->main_audio_stream_index)
		return call->audiostream_app_evq;
	if (stream_index == call->main_video_stream_index)
		return call->videostream_app_evq;
	if (stream_index == call->main_text_stream_index)
		return call->textstream_app_evq;
	ms_error("linphone_call_get_event_queue(): no stream index %i", stream_index);
	return NULL;
}

void linphone_call_handle_stream_events(LinphoneCall *call, int stream_index){
	MediaStream *ms = stream_index == call->main_audio_stream_index ? (MediaStream *)call->audiostream : (stream_index == call->main_video_stream_index ? (MediaStream *)call->videostream : (MediaStream *)call->textstream);
	OrtpEvQueue *evq;
	OrtpEvent *ev;

	if (ms){
		/* Ensure there is no dangling ICE check list. */
		if (call->ice_session == NULL) {
			media_stream_set_ice_check_list(ms, NULL);
		}

		switch(ms->type){
			case MSAudio:
				audio_stream_iterate((AudioStream*)ms);
			break;
			case MSVideo:
	#ifdef VIDEO_ENABLED
				video_stream_iterate((VideoStream*)ms);
	#endif
			break;
			case MSText:
				text_stream_iterate((TextStream*)ms);
			break;
			default:
				ms_error("linphone_call_handle_stream_events(): unsupported stream type.");
				return;
			break;
		}
	}
	/*yes the event queue has to be taken at each iteration, because ice events may perform operations re-creating the streams*/
	while((evq = linphone_call_get_event_queue(call, stream_index)) != NULL && NULL != (ev=ortp_ev_queue_get(evq))){
		OrtpEventType evt=ortp_event_get_type(ev);
		OrtpEventData *evd=ortp_event_get_data(ev);
		int stats_index = stream_index == call->main_audio_stream_index ? LINPHONE_CALL_STATS_AUDIO : (stream_index == call->main_video_stream_index ? LINPHONE_CALL_STATS_VIDEO : LINPHONE_CALL_STATS_TEXT);

		/*and yes the MediaStream must be taken at each iteration, because it may have changed due to the handling of events
		 * in this loop*/
		ms = linphone_call_get_media_stream(call, stream_index);

		if (ms) linphone_call_stats_fill(&call->stats[stats_index],ms,ev);
		linphone_call_notify_stats_updated(call,stats_index);

		if (evt == ORTP_EVENT_ZRTP_ENCRYPTION_CHANGED){
			if (stream_index == call->main_audio_stream_index)
				linphone_call_audiostream_encryption_changed(call, evd->info.zrtp_stream_encrypted);
			else if (stream_index == call->main_video_stream_index) {
				propagate_encryption_changed(call);
			}
		} else if (evt == ORTP_EVENT_ZRTP_SAS_READY) {
			if (stream_index == call->main_audio_stream_index)
				linphone_call_audiostream_auth_token_ready(call, evd->info.zrtp_sas.sas, evd->info.zrtp_sas.verified);
		} else if (evt == ORTP_EVENT_DTLS_ENCRYPTION_CHANGED) {
			if (stream_index == call->main_audio_stream_index)
				linphone_call_audiostream_encryption_changed(call, evd->info.dtls_stream_encrypted);
			else if (stream_index == call->main_video_stream_index)
				propagate_encryption_changed(call);
		}else if ((evt == ORTP_EVENT_ICE_SESSION_PROCESSING_FINISHED) || (evt == ORTP_EVENT_ICE_GATHERING_FINISHED)
			|| (evt == ORTP_EVENT_ICE_LOSING_PAIRS_COMPLETED) || (evt == ORTP_EVENT_ICE_RESTART_NEEDED)) {
			if (ms) handle_ice_events(call, ev);
		} else if (evt==ORTP_EVENT_TELEPHONE_EVENT){
			linphone_core_dtmf_received(call,evd->info.telephone_event);
		}
		ortp_event_destroy(ev);
	}
}

void linphone_call_background_tasks(LinphoneCall *call, bool_t one_second_elapsed){
	int disconnect_timeout = linphone_core_get_nortp_timeout(call->core);
	bool_t disconnected=FALSE;

	switch (call->state) {
	case LinphoneCallStreamsRunning:
	case LinphoneCallOutgoingEarlyMedia:
	case LinphoneCallIncomingEarlyMedia:
	case LinphoneCallPausedByRemote:
	case LinphoneCallPaused:
		if (one_second_elapsed){
			float audio_load=0, video_load=0, text_load=0;
			if (call->audiostream != NULL) {
				if (call->audiostream->ms.sessions.ticker)
					audio_load = ms_ticker_get_average_load(call->audiostream->ms.sessions.ticker);
			}
			if (call->videostream != NULL) {
				if (call->videostream->ms.sessions.ticker)
					video_load = ms_ticker_get_average_load(call->videostream->ms.sessions.ticker);
			}
			if (call->textstream != NULL) {
				if (call->textstream->ms.sessions.ticker)
					text_load = ms_ticker_get_average_load(call->textstream->ms.sessions.ticker);
			}
			report_bandwidth(call, (MediaStream*)call->audiostream, (MediaStream*)call->videostream,  (MediaStream*)call->textstream);
			ms_message("Thread processing load: audio=%f\tvideo=%f\ttext=%f", audio_load, video_load, text_load);
		}
		break;
	default:
		/*no stats for other states*/
		break;
	}

#ifdef BUILD_UPNP
	linphone_upnp_call_process(call);
#endif //BUILD_UPNP

	linphone_call_handle_stream_events(call, call->main_audio_stream_index);
	linphone_call_handle_stream_events(call, call->main_video_stream_index);
	linphone_call_handle_stream_events(call, call->main_text_stream_index);
	if ((call->state == LinphoneCallStreamsRunning ||
		call->state == LinphoneCallPausedByRemote) && one_second_elapsed && call->audiostream!=NULL
		&& call->audiostream->ms.state==MSStreamStarted && disconnect_timeout>0 )
		disconnected=!audio_stream_alive(call->audiostream,disconnect_timeout);
	if (disconnected)
		linphone_call_lost(call, LinphoneReasonUnknown);
}

void linphone_call_log_completed(LinphoneCall *call){
	LinphoneCore *lc=call->core;
	bool_t call_logs_sqlite_db_found = FALSE;

	call->log->duration=linphone_call_get_duration(call); /*store duration since connected*/

	if (call->log->status==LinphoneCallMissed){
		char *info;
		lc->missed_calls++;
		info=ortp_strdup_printf(ngettext("You have missed %i call.",
										 "You have missed %i calls.", lc->missed_calls),
								lc->missed_calls);
		linphone_core_notify_display_status(lc,info);
		ms_free(info);
	}

#ifdef SQLITE_STORAGE_ENABLED
	if (lc->logs_db) {
		call_logs_sqlite_db_found = TRUE;
		linphone_core_store_call_log(lc, call->log);
	}
#endif
	if (!call_logs_sqlite_db_found) {
		lc->call_logs=bctbx_list_prepend(lc->call_logs,linphone_call_log_ref(call->log));
		if (bctbx_list_size(lc->call_logs)>(size_t)lc->max_call_logs){
			bctbx_list_t *elem,*prevelem=NULL;
			/*find the last element*/
			for(elem=lc->call_logs;elem!=NULL;elem=elem->next){
				prevelem=elem;
			}
			elem=prevelem;
			linphone_call_log_unref((LinphoneCallLog*)elem->data);
			lc->call_logs=bctbx_list_erase_link(lc->call_logs,elem);
		}
		call_logs_write_to_config_file(lc);
	}
	linphone_core_notify_call_log_updated(lc,call->log);
}

LinphoneCallState linphone_call_get_transfer_state(LinphoneCall *call) {
	return call->transfer_state;
}

void linphone_call_set_transfer_state(LinphoneCall* call, LinphoneCallState state) {
	if (state != call->transfer_state) {
		LinphoneCore* lc = call->core;
		ms_message("Transfer state for call [%p] changed  from [%s] to [%s]",call
						,linphone_call_state_to_string(call->transfer_state)
						,linphone_call_state_to_string(state));
		call->transfer_state = state;
		linphone_core_notify_transfer_state_changed(lc, call, state);
	}
}

bool_t linphone_call_is_in_conference(const LinphoneCall *call) {
	return call->params->in_conference;
}

LinphoneConference *linphone_call_get_conference(const LinphoneCall *call) {
	return call->conf_ref;
}

void linphone_call_zoom_video(LinphoneCall* call, float zoom_factor, float* cx, float* cy) {
	VideoStream* vstream = call->videostream;
	if (vstream && vstream->output) {
		float zoom[3];
		float halfsize;

		if (zoom_factor < 1)
			zoom_factor = 1;
		halfsize = 0.5f * 1.0f / zoom_factor;

		if ((*cx - halfsize) < 0)
			*cx = 0 + halfsize;
		if ((*cx + halfsize) > 1)
			*cx = 1 - halfsize;
		if ((*cy - halfsize) < 0)
			*cy = 0 + halfsize;
		if ((*cy + halfsize) > 1)
			*cy = 1 - halfsize;

		zoom[0] = zoom_factor;
		zoom[1] = *cx;
		zoom[2] = *cy;
		ms_filter_call_method(vstream->output, MS_VIDEO_DISPLAY_ZOOM, &zoom);
	}else ms_warning("Could not apply zoom: video output wasn't activated.");
}

static LinphoneAddress *get_fixed_contact(LinphoneCore *lc, LinphoneCall *call , LinphoneProxyConfig *dest_proxy){
	LinphoneAddress *ctt=NULL;
	LinphoneAddress *ret=NULL;
	//const char *localip=call->localip;

	/* first use user's supplied ip address if asked*/
	if (linphone_core_get_firewall_policy(lc)==LinphonePolicyUseNatAddress){
		ctt=linphone_core_get_primary_contact_parsed(lc);
		linphone_address_set_domain(ctt,linphone_core_get_nat_address_resolved(lc));
		ret=ctt;
	} else if (call->op && sal_op_get_contact_address(call->op)!=NULL){
		/* if already choosed, don't change it */
		return NULL;
	} else if (call->ping_op && sal_op_get_contact_address(call->ping_op)) {
		/* if the ping OPTIONS request succeeded use the contact guessed from the
		 received, rport*/
		ms_message("Contact has been fixed using OPTIONS"/* to %s",guessed*/);
		ret=linphone_address_clone(sal_op_get_contact_address(call->ping_op));;
	} else 	if (dest_proxy && dest_proxy->op && sal_op_get_contact_address(dest_proxy->op)){
	/*if using a proxy, use the contact address as guessed with the REGISTERs*/
		ms_message("Contact has been fixed using proxy" /*to %s",fixed_contact*/);
		ret=linphone_address_clone(sal_op_get_contact_address(dest_proxy->op));
	} else {
		ctt=linphone_core_get_primary_contact_parsed(lc);
		if (ctt!=NULL){
			/*otherwise use supplied localip*/
			linphone_address_set_domain(ctt,NULL/*localip*/);
			linphone_address_set_port(ctt,-1/*linphone_core_get_sip_port(lc)*/);
			ms_message("Contact has not been fixed stack will do"/* to %s",ret*/);
			ret=ctt;
		}
	}
	return ret;
}

void linphone_call_set_contact_op(LinphoneCall* call) {
	LinphoneAddress *contact;

	if (call->dest_proxy == NULL) {
		/* Try to define the destination proxy if it has not already been done to have a correct contact field in the SIP messages */
		call->dest_proxy = linphone_core_lookup_known_proxy(call->core, call->log->to);
	}

	contact=get_fixed_contact(call->core,call,call->dest_proxy);
	if (contact){
		SalTransport tport=sal_address_get_transport((SalAddress*)contact);
		sal_address_clean((SalAddress*)contact); /* clean out contact_params that come from proxy config*/
		sal_address_set_transport((SalAddress*)contact,tport);
		sal_op_set_contact_address(call->op, contact);
		linphone_address_unref(contact);
	}
}

LinphonePlayer *linphone_call_get_player(LinphoneCall *call){
	if (call->player==NULL)
		call->player=linphone_call_build_player(call);
	return call->player;
}

void linphone_call_set_new_params(LinphoneCall *call, const LinphoneCallParams *params){
	LinphoneCallParams *cp=NULL;
	if (params) cp=linphone_call_params_copy(params);
	if (call->params) linphone_call_params_unref(call->params);
	call->params=cp;
}

static int send_dtmf_handler(void *data, unsigned int revents){
	LinphoneCall *call = (LinphoneCall*)data;
	/*By default we send DTMF RFC2833 if we do not have enabled SIP_INFO but we can also send RFC2833 and SIP_INFO*/
	if (linphone_core_get_use_rfc2833_for_dtmf(call->core)!=0 || linphone_core_get_use_info_for_dtmf(call->core)==0)
	{
		/* In Band DTMF */
		if (call->audiostream!=NULL){
			audio_stream_send_dtmf(call->audiostream,*call->dtmf_sequence);
		}
		else
		{
			ms_error("Cannot send RFC2833 DTMF when we are not in communication.");
			return FALSE;
		}
	}
	if (linphone_core_get_use_info_for_dtmf(call->core)!=0){
		/* Out of Band DTMF (use INFO method) */
		sal_call_send_dtmf(call->op,*call->dtmf_sequence);
	}

	/*this check is needed because linphone_call_send_dtmf does not set the timer since its a single character*/
	if (call->dtmfs_timer!=NULL) {
		memmove(call->dtmf_sequence, call->dtmf_sequence+1, strlen(call->dtmf_sequence));
	}
	/* continue only if the dtmf sequence is not empty*/
	if (call->dtmf_sequence!=NULL&&*call->dtmf_sequence!='\0') {
		return TRUE;
	} else {
		linphone_call_cancel_dtmfs(call);
		return FALSE;
	}
}

int linphone_call_send_dtmf(LinphoneCall *call,char dtmf){
	if (call==NULL){
		ms_warning("linphone_call_send_dtmf(): invalid call, canceling DTMF.");
		return -1;
	}
	call->dtmf_sequence = &dtmf;
	send_dtmf_handler(call,0);
	call->dtmf_sequence = NULL;
	return 0;
}

int linphone_call_send_dtmfs(LinphoneCall *call,const char *dtmfs) {
	if (call==NULL){
		ms_warning("linphone_call_send_dtmfs(): invalid call, canceling DTMF sequence.");
		return -1;
	}
	if (call->dtmfs_timer!=NULL){
		ms_warning("linphone_call_send_dtmfs(): a DTMF sequence is already in place, canceling DTMF sequence.");
		return -2;
	}
	if (dtmfs != NULL) {
		int delay_ms = lp_config_get_int(call->core->config,"net","dtmf_delay_ms",200);
		call->dtmf_sequence = ms_strdup(dtmfs);
		call->dtmfs_timer = sal_create_timer(call->core->sal, send_dtmf_handler, call, delay_ms, "DTMF sequence timer");
	}
	return 0;
}

void linphone_call_cancel_dtmfs(LinphoneCall *call) {
	/*nothing to do*/
	if (!call || !call->dtmfs_timer) return;

	sal_cancel_timer(call->core->sal, call->dtmfs_timer);
	belle_sip_object_unref(call->dtmfs_timer);
	call->dtmfs_timer = NULL;
	if (call->dtmf_sequence != NULL) {
		ms_free(call->dtmf_sequence);
		call->dtmf_sequence = NULL;
	}
}

void * linphone_call_get_native_video_window_id(const LinphoneCall *call) {
	if (call->video_window_id) {
		/* The video id was previously set by the app. */
		return call->video_window_id;
	}
#ifdef VIDEO_ENABLED
	else if (call->videostream) {
		/* It was not set but we want to get the one automatically created by mediastreamer2 (desktop versions only). */
		return video_stream_get_native_window_id(call->videostream);
	}
#endif
	return 0;
}

void linphone_call_set_native_video_window_id(LinphoneCall *call, void *id) {
	call->video_window_id = id;
#ifdef VIDEO_ENABLED
	if (call->videostream) {
		video_stream_set_native_window_id(call->videostream, id);
	}
#endif
}

MSWebCam *linphone_call_get_video_device(const LinphoneCall *call) {
	LinphoneCallState state = linphone_call_get_state(call);
	bool_t paused = (state == LinphoneCallPausing) || (state == LinphoneCallPaused);
	if (paused || call->all_muted || (call->camera_enabled == FALSE))
		return get_nowebcam_device(call->core->factory);
	else
		return call->core->video_conf.device;
}

void linphone_call_set_audio_route(LinphoneCall *call, LinphoneAudioRoute route) {
	if (call != NULL && call->audiostream != NULL){
		audio_stream_set_audio_route(call->audiostream, (MSAudioRoute) route);
	}
}

LinphoneChatRoom * linphone_call_get_chat_room(LinphoneCall *call) {
	if (!call->chat_room){
		if (call->state != LinphoneCallReleased && call->state != LinphoneCallEnd){
			call->chat_room = _linphone_core_create_chat_room_from_call(call);
		}
	}
	return call->chat_room;
}

int linphone_call_get_stream_count(LinphoneCall *call) {
	// Revisit when multiple media streams will be implemented
#ifdef VIDEO_ENABLED
	if (linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(call))) {
		return 3;
	}
	return 2;
#else
	if (linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(call))) {
		return 2;
	}
	return 1;
#endif
}

MSFormatType linphone_call_get_stream_type(LinphoneCall *call, int stream_index) {
	// Revisit when multiple media streams will be implemented
	if (stream_index == call->main_video_stream_index) {
		return MSVideo;
	} else if (stream_index == call->main_text_stream_index) {
		return MSText;
	} else if (stream_index == call->main_audio_stream_index){
		return MSAudio;
	}
	return MSUnknownMedia;
}

RtpTransport* linphone_call_get_meta_rtp_transport(LinphoneCall *call, int stream_index) {
	RtpTransport *meta_rtp;
	RtpTransport *meta_rtcp;

	if (!call || stream_index < 0 || stream_index >= linphone_call_get_stream_count(call)) {
		return NULL;
	}

	rtp_session_get_transports(call->sessions[stream_index].rtp_session, &meta_rtp, &meta_rtcp);
	return meta_rtp;
}

RtpTransport* linphone_call_get_meta_rtcp_transport(LinphoneCall *call, int stream_index) {
	RtpTransport *meta_rtp;
	RtpTransport *meta_rtcp;

	if (!call || stream_index < 0 || stream_index >= linphone_call_get_stream_count(call)) {
		return NULL;
	}

	rtp_session_get_transports(call->sessions[stream_index].rtp_session, &meta_rtp, &meta_rtcp);
	return meta_rtcp;
}

void linphone_call_set_broken(LinphoneCall *call){
	switch(call->state){
		/*for all the early states, we prefer to drop the call*/
		case LinphoneCallOutgoingInit:
		case LinphoneCallOutgoingProgress:
		case LinphoneCallOutgoingRinging:
		case LinphoneCallOutgoingEarlyMedia:
		case LinphoneCallIncomingReceived:
		case LinphoneCallIncomingEarlyMedia:
			/*during the early states, the SAL layer reports the failure from the dialog or transaction layer,
			 * hence, there is nothing special to do*/
		//break;
		case LinphoneCallStreamsRunning:
		case LinphoneCallUpdating:
		case LinphoneCallPausing:
		case LinphoneCallResuming:
		case LinphoneCallPaused:
		case LinphoneCallPausedByRemote:
		case LinphoneCallUpdatedByRemote:
			/*during these states, the dialog is established. A failure of a transaction is not expected to close it.
			 * Instead we have to repair the dialog by sending a reINVITE*/
			call->broken = TRUE;
			call->need_localip_refresh = TRUE;
		break;
		default:
			ms_error("linphone_call_set_broken() unimplemented case.");
		break;
	}
}

static void linphone_call_repair_by_invite_with_replaces(LinphoneCall *call) {
	const char *call_id = sal_op_get_call_id(call->op);
	const char *from_tag = sal_call_get_local_tag(call->op);
	const char *to_tag = sal_call_get_remote_tag(call->op);
	sal_op_kill_dialog(call->op);
	linphone_call_create_op(call);
	sal_call_set_replaces(call->op, call_id, from_tag, to_tag);
	linphone_core_start_invite(call->core, call, NULL);
}

void linphone_call_reinvite_to_recover_from_connection_loss(LinphoneCall *call) {
	LinphoneCallParams *params;
	ms_message("LinphoneCall[%p] is going to be updated (reINVITE) in order to recover from lost connectivity", call);
	if (call->ice_session){
		ice_session_restart(call->ice_session, IR_Controlling);
	}
	params = linphone_core_create_call_params(call->core, call);
	linphone_core_update_call(call->core, call, params);
	linphone_call_params_unref(params);
}

void linphone_call_repair_if_broken(LinphoneCall *call){
	if (!call->broken) return;
	if (!call->core->media_network_reachable) return;

	/*Make sure that the proxy from which we received this call, or to which we routed this call is registered first*/
	if (call->dest_proxy){
		/*in all other cases, ie no proxy config, or a proxy config for which no registration was requested, we can start the
		 * call repair immediately.*/
		if (linphone_proxy_config_register_enabled(call->dest_proxy)
			&& linphone_proxy_config_get_state(call->dest_proxy) != LinphoneRegistrationOk) return;
	}

	switch (call->state){
		case LinphoneCallUpdating:
		case LinphoneCallPausing:
			if (sal_call_dialog_request_pending(call->op)) {
				/* Need to cancel first re-INVITE as described in section 5.5 of RFC 6141 */
				sal_call_cancel_invite(call->op);
				call->reinvite_on_cancel_response_requested = TRUE;
			}
			break;
		case LinphoneCallStreamsRunning:
		case LinphoneCallPaused:
		case LinphoneCallPausedByRemote:
			if (!sal_call_dialog_request_pending(call->op)) {
				linphone_call_reinvite_to_recover_from_connection_loss(call);
			}
			break;
		case LinphoneCallUpdatedByRemote:
			if (sal_call_dialog_request_pending(call->op)) {
				sal_call_decline(call->op, SalReasonServiceUnavailable, NULL);
			}
			linphone_call_reinvite_to_recover_from_connection_loss(call);
			break;
		case LinphoneCallOutgoingInit:
		case LinphoneCallOutgoingProgress:
			sal_call_cancel_invite(call->op);
			call->reinvite_on_cancel_response_requested = TRUE;
			break;
		case LinphoneCallOutgoingEarlyMedia:
		case LinphoneCallOutgoingRinging:
			linphone_call_repair_by_invite_with_replaces(call);
			break;
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallIncomingReceived:
			/* Keep the call broken until a forked INVITE is received from the server. */
			break;
		default:
			ms_warning("linphone_call_repair_if_broken(): don't know what to do in state [%s]", linphone_call_state_to_string(call->state));
			call->broken = FALSE;
		break;
	}
}

void linphone_call_refresh_sockets(LinphoneCall *call){
	int i;
	for (i=0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; ++i){
		MSMediaStreamSessions *mss = &call->sessions[i];
		if (mss->rtp_session){
			rtp_session_refresh_sockets(mss->rtp_session);
		}
	}
}

void linphone_call_replace_op(LinphoneCall *call, SalOp *op) {
	SalOp *oldop = call->op;
	LinphoneCallState oldstate = linphone_call_get_state(call);
	call->op = op;
	sal_op_set_user_pointer(call->op, call);
	sal_call_set_local_media_description(call->op, call->localdesc);
	switch (linphone_call_get_state(call)) {
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallIncomingReceived:
			sal_call_notify_ringing(call->op, (linphone_call_get_state(call) == LinphoneCallIncomingEarlyMedia) ? TRUE : FALSE);
			break;
		case LinphoneCallConnected:
		case LinphoneCallStreamsRunning:
			sal_call_accept(call->op);
			break;
		default:
			ms_warning("linphone_call_replace_op(): don't know what to do in state [%s]", linphone_call_state_to_string(call->state));
			break;
	}
	switch (oldstate) {
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallIncomingReceived:
			sal_op_set_user_pointer(oldop, NULL); /* To make the call does not get terminated by terminating this op. */
			/* Do not terminate a forked INVITE */
			if (sal_call_get_replaces(op)) {
				sal_call_terminate(oldop);
			} else {
				sal_op_kill_dialog(oldop);
			}
			break;
		case LinphoneCallConnected:
		case LinphoneCallStreamsRunning:
			sal_call_terminate(oldop);
			sal_op_kill_dialog(oldop);
			break;
		default:
			break;
	}
	sal_op_release(oldop);
}
