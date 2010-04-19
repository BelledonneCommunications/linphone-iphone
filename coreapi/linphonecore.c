/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@linphone.org)

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

#include "linphonecore.h"
#include "sipsetup.h"
#include "lpconfig.h"
#include "private.h"
#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/msvolume.h"
#include "mediastreamer2/msequalizer.h"

#include <ortp/telephonyevents.h>


#ifdef INET6
#ifndef WIN32
#include <netdb.h>
#endif
#endif

/*#define UNSTANDART_GSM_11K 1*/

static const char *liblinphone_version=LIBLINPHONE_VERSION;
static void set_network_reachable(LinphoneCore* lc,bool_t isReachable);

#include "enum.h"

void linphone_core_get_local_ip(LinphoneCore *lc, const char *dest, char *result);
static void toggle_video_preview(LinphoneCore *lc, bool_t val);

/* relative path where is stored local ring*/
#define LOCAL_RING "rings/oldphone.wav"
/* same for remote ring (ringback)*/
#define REMOTE_RING "ringback.wav"

extern SalCallbacks linphone_sal_callbacks;

void lc_callback_obj_init(LCCallbackObj *obj,LinphoneCoreCbFunc func,void* ud)
{
  obj->_func=func;
  obj->_user_data=ud;
}

int lc_callback_obj_invoke(LCCallbackObj *obj, LinphoneCore *lc){
	if (obj->_func!=NULL) obj->_func(lc,obj->_user_data);
	return 0;
}


static MSList *make_codec_list(LinphoneCore *lc, const MSList *codecs, bool_t only_one_codec){
	MSList *l=NULL;
	const MSList *it;
	for(it=codecs;it!=NULL;it=it->next){
		PayloadType *pt=(PayloadType*)it->data;
		if ((pt->flags & PAYLOAD_TYPE_ENABLED) && linphone_core_check_payload_type_usability(lc,pt)){
			l=ms_list_append(l,payload_type_clone(pt));
			if (only_one_codec) break;
		}
	}
	return l;
}

static SalMediaDescription *create_local_media_description(LinphoneCore *lc, 
    		const char *localip, const char *username, bool_t only_one_codec){
	MSList *l;
	PayloadType *pt;
	SalMediaDescription *md=sal_media_description_new();
	md->nstreams=1;
	strncpy(md->addr,localip,sizeof(md->addr));
	strncpy(md->username,username,sizeof(md->username));
	md->bandwidth=linphone_core_get_download_bandwidth(lc);
	/*set audio capabilities */
	strncpy(md->streams[0].addr,localip,sizeof(md->streams[0].addr));
	md->streams[0].port=linphone_core_get_audio_port(lc);
	md->streams[0].proto=SalProtoRtpAvp;
	md->streams[0].type=SalAudio;
	md->streams[0].ptime=lc->net_conf.down_ptime;
	l=make_codec_list(lc,lc->codecs_conf.audio_codecs,only_one_codec);
	pt=payload_type_clone(rtp_profile_get_payload_from_mime(&av_profile,"telephone-event"));
	l=ms_list_append(l,pt);
	md->streams[0].payloads=l;
	
	if (lc->dw_audio_bw>0)
		md->streams[0].bandwidth=lc->dw_audio_bw;

	if (linphone_core_video_enabled (lc)){
		md->nstreams++;
		md->streams[1].port=linphone_core_get_video_port(lc);
		md->streams[1].proto=SalProtoRtpAvp;
		md->streams[1].type=SalVideo;
		l=make_codec_list(lc,lc->codecs_conf.video_codecs,only_one_codec);
		md->streams[1].payloads=l;
		if (lc->dw_video_bw)
			md->streams[1].bandwidth=lc->dw_video_bw;
	}
	return md;
}

static void linphone_call_init_common(LinphoneCall *call, LinphoneAddress *from, LinphoneAddress *to){
	call->state=LCStateInit;
	call->start_time=time(NULL);
	call->media_start_time=0;
	call->log=linphone_call_log_new(call, from, to);
	linphone_core_notify_all_friends(call->core,LINPHONE_STATUS_ONTHEPHONE);
	if (linphone_core_get_firewall_policy(call->core)==LINPHONE_POLICY_USE_STUN)
		linphone_core_run_stun_tests(call->core,call);
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

LinphoneCall * linphone_call_new_outgoing(struct _LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to)
{
	LinphoneCall *call=ms_new0(LinphoneCall,1);
	call->dir=LinphoneCallOutgoing;
	call->op=sal_op_new(lc->sal);
	sal_op_set_user_pointer(call->op,call);
	call->core=lc;
	linphone_core_get_local_ip(lc,linphone_address_get_domain(to),call->localip);
	call->localdesc=create_local_media_description (lc,call->localip,
		linphone_address_get_username(from),FALSE);
	linphone_call_init_common(call,from,to);
	discover_mtu(lc,linphone_address_get_domain (to));
	return call;
}

LinphoneCall * linphone_call_new_incoming(LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to, SalOp *op){
	LinphoneCall *call=ms_new0(LinphoneCall,1);
	LinphoneAddress *me=linphone_core_get_primary_contact_parsed(lc);
	char *to_str;
	char *from_str;

	call->dir=LinphoneCallIncoming;
	sal_op_set_user_pointer(op,call);
	call->op=op;
	call->core=lc;

	if (lc->sip_conf.ping_with_options){
		/*the following sends an option request back to the caller so that
		 we get a chance to discover our nat'd address before answering.*/
		call->ping_op=sal_op_new(lc->sal);
		to_str=linphone_address_as_string(to);
		from_str=linphone_address_as_string(from);
		sal_op_set_route(call->ping_op,sal_op_get_network_origin(call->op));
		sal_ping(call->ping_op,to_str,from_str);
		ms_free(to_str);
		ms_free(from_str);
	}
	
	linphone_address_clean(from);
	linphone_core_get_local_ip(lc,linphone_address_get_domain(from),call->localip);
	call->localdesc=create_local_media_description (lc,call->localip,
	    linphone_address_get_username(me),lc->sip_conf.only_one_codec);
	linphone_call_init_common(call, from, to);
	discover_mtu(lc,linphone_address_get_domain(from));
	linphone_address_destroy(me);
	return call;
}

void linphone_call_destroy(LinphoneCall *obj)
{
	linphone_core_notify_all_friends(obj->core,obj->core->prev_mode);
	linphone_call_log_completed(obj->log,obj);
	linphone_core_update_allocated_audio_bandwidth(obj->core);
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
	ms_free(obj);
}

/*prevent a gcc bug with %c*/
static size_t my_strftime(char *s, size_t max, const char  *fmt,  const struct tm *tm){
#if !defined(_WIN32_WCE)
	return strftime(s, max, fmt, tm);
#else
	return 0;
	/*FIXME*/
#endif /*_WIN32_WCE*/
}

static void set_call_log_date(LinphoneCallLog *cl, const struct tm *loctime){
	my_strftime(cl->start_date,sizeof(cl->start_date),"%c",loctime);
}

LinphoneCallLog * linphone_call_log_new(LinphoneCall *call, LinphoneAddress *from, LinphoneAddress *to){
	LinphoneCallLog *cl=ms_new0(LinphoneCallLog,1);
	struct tm loctime;
	cl->dir=call->dir;
#ifdef WIN32
#if !defined(_WIN32_WCE)
	loctime=*localtime(&call->start_time);
	/*FIXME*/
#endif /*_WIN32_WCE*/
#else
	localtime_r(&call->start_time,&loctime);
#endif
	set_call_log_date(cl,&loctime);
	cl->from=from;
	cl->to=to;
	return cl;
}

static void call_logs_write_to_config_file(LinphoneCore *lc){
	MSList *elem;
	char logsection[32];
	int i;
	char *tmp;
	LpConfig *cfg=lc->config;

	if (!lc->ready) return;
	
	for(i=0,elem=lc->call_logs;elem!=NULL;elem=elem->next,++i){
		LinphoneCallLog *cl=(LinphoneCallLog*)elem->data;
		snprintf(logsection,sizeof(logsection),"call_log_%i",i);
		lp_config_set_int(cfg,logsection,"dir",cl->dir);
		lp_config_set_int(cfg,logsection,"status",cl->status);
		tmp=linphone_address_as_string(cl->from);
		lp_config_set_string(cfg,logsection,"from",tmp);
		ms_free(tmp);
		tmp=linphone_address_as_string(cl->to);
		lp_config_set_string(cfg,logsection,"to",tmp);
		ms_free(tmp);
		lp_config_set_string(cfg,logsection,"start_date",cl->start_date);
		lp_config_set_int(cfg,logsection,"duration",cl->duration);
		if (cl->refkey) lp_config_set_string(cfg,logsection,"refkey",cl->refkey);
	}
	for(;i<lc->max_call_logs;++i){
		snprintf(logsection,sizeof(logsection),"call_log_%i",i);
		lp_config_clean_section(cfg,logsection);
	}
}

static void call_logs_read_from_config_file(LinphoneCore *lc){
	char logsection[32];
	int i;
	const char *tmp;
	LpConfig *cfg=lc->config;
	for(i=0;;++i){
		snprintf(logsection,sizeof(logsection),"call_log_%i",i);
		if (lp_config_has_section(cfg,logsection)){
			LinphoneCallLog *cl=ms_new0(LinphoneCallLog,1);
			cl->dir=lp_config_get_int(cfg,logsection,"dir",0);
			cl->status=lp_config_get_int(cfg,logsection,"status",0);
			tmp=lp_config_get_string(cfg,logsection,"from",NULL);
			if (tmp) cl->from=linphone_address_new(tmp);
			tmp=lp_config_get_string(cfg,logsection,"to",NULL);
			if (tmp) cl->to=linphone_address_new(tmp);
			tmp=lp_config_get_string(cfg,logsection,"start_date",NULL);
			if (tmp) strncpy(cl->start_date,tmp,sizeof(cl->start_date));
			cl->duration=lp_config_get_int(cfg,logsection,"duration",0);
			tmp=lp_config_get_string(cfg,logsection,"refkey",NULL);
			if (tmp) cl->refkey=ms_strdup(tmp);
			lc->call_logs=ms_list_append(lc->call_logs,cl);
		}else break;	
	}
}


void linphone_call_log_completed(LinphoneCallLog *calllog, LinphoneCall *call){
	LinphoneCore *lc=call->core;
	
	calllog->duration=time(NULL)-call->start_time;
	switch(call->state){
		case LCStateInit:
		case LCStatePreEstablishing:
			calllog->status=LinphoneCallAborted;
			break;
		case LCStateRinging:
			if (calllog->dir==LinphoneCallIncoming){
				char *info;
				calllog->status=LinphoneCallMissed;
				lc->missed_calls++;
				info=ortp_strdup_printf(ngettext("You have missed %i call.",
                            "You have missed %i calls.", lc->missed_calls),
                        lc->missed_calls);
				lc->vtable.display_status(lc,info);
				ms_free(info);
			}
			else calllog->status=LinphoneCallAborted;
			break;
		case LCStateAVRunning:
			calllog->status=LinphoneCallSuccess;
			break;
	}
	lc->call_logs=ms_list_prepend(lc->call_logs,(void *)calllog);
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
		lc->vtable.call_log_updated(lc,calllog);
	}
	call_logs_write_to_config_file(lc);
}

/**
 * @addtogroup call_logs
 * @{
**/

/**
 * Returns a human readable string describing the call.
 * 
 * @note: the returned char* must be freed by the application (use ms_free()).
**/
char * linphone_call_log_to_str(LinphoneCallLog *cl){
	char *status;
	char *tmp;
	char *from=linphone_address_as_string (cl->from);
	char *to=linphone_address_as_string (cl->to);
	switch(cl->status){
		case LinphoneCallAborted:
			status=_("aborted");
			break;
		case LinphoneCallSuccess:
			status=_("completed");
			break;
		case LinphoneCallMissed:
			status=_("missed");
			break;
		default:
			status="unknown";
	}
	tmp=ortp_strdup_printf(_("%s at %s\nFrom: %s\nTo: %s\nStatus: %s\nDuration: %i mn %i sec\n"),
			(cl->dir==LinphoneCallIncoming) ? _("Incoming call") : _("Outgoing call"),
			cl->start_date,
			from,
			to,
			status,
			cl->duration/60,
			cl->duration%60);
	ms_free(from);
	ms_free(to);
	return tmp;
}

void linphone_call_log_set_user_pointer(LinphoneCallLog *cl, void *up){
	cl->user_pointer=up;
}

void *linphone_call_log_get_user_pointer(const LinphoneCallLog *cl){
	return cl->user_pointer;
}



/**
 * Associate a persistent reference key to the call log.
 *
 * The reference key can be for example an id to an external database.
 * It is stored in the config file, thus can survive to process exits/restarts.
 *
**/
void linphone_call_log_set_ref_key(LinphoneCallLog *cl, const char *refkey){
	if (cl->refkey!=NULL){
		ms_free(cl->refkey);
		cl->refkey=NULL;
	}
	if (refkey) cl->refkey=ms_strdup(refkey);
	call_logs_write_to_config_file(cl->lc);
}

/**
 * Get the persistent reference key associated to the call log.
 *
 * The reference key can be for example an id to an external database.
 * It is stored in the config file, thus can survive to process exits/restarts.
 *
**/
const char *linphone_call_log_get_ref_key(const LinphoneCallLog *cl){
	return cl->refkey;
}

/** @} */

void linphone_call_log_destroy(LinphoneCallLog *cl){
	if (cl->from!=NULL) linphone_address_destroy(cl->from);
	if (cl->to!=NULL) linphone_address_destroy(cl->to);
	if (cl->refkey!=NULL) ms_free(cl->refkey);
	ms_free(cl);
}

/**
 * Returns TRUE if the LinphoneCall asked to autoanswer
 *
**/
bool_t linphone_call_asked_to_autoanswer(LinphoneCall *call){
	//return TRUE if the unique(for the moment) incoming call asked to be autoanswered
	if(call)
		return sal_call_autoanswer_asked(call->op);
	else
		return FALSE;
}

int linphone_core_get_current_call_duration(const LinphoneCore *lc){
	LinphoneCall *call=lc->call;
	if (call==NULL) return 0;
	if (call->media_start_time==0) return 0;
	return time(NULL)-call->media_start_time;
}

const LinphoneAddress *linphone_core_get_remote_uri(LinphoneCore *lc){
	LinphoneCall *call=lc->call;
	if (call==NULL) return 0;
	return call->dir==LinphoneCallIncoming ? call->log->from : call->log->to;
}

/**
 * Enable logs in supplied FILE*.
 *
 * @ingroup misc
 *
 * @param file a C FILE* where to fprintf logs. If null stdout is used.
 * 
**/
void linphone_core_enable_logs(FILE *file){
	if (file==NULL) file=stdout;
	ortp_set_log_file(file);
	ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
}

/**
 * Enable logs through the user's supplied log callback.
 *
 * @ingroup misc
 *
 * @param logfunc The address of a OrtpLogFunc callback whose protoype is
 *            	  typedef void (*OrtpLogFunc)(OrtpLogLevel lev, const char *fmt, va_list args);
 * 
**/
void linphone_core_enable_logs_with_cb(OrtpLogFunc logfunc){
	ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
	ortp_set_log_handler(logfunc);
}

/**
 * Entirely disable logging.
 *
 * @ingroup misc
**/
void linphone_core_disable_logs(){
	ortp_set_log_level_mask(ORTP_ERROR|ORTP_FATAL);
}


static void
net_config_read (LinphoneCore *lc)
{
	int tmp;
	const char *tmpstr;
	LpConfig *config=lc->config;

	tmp=lp_config_get_int(config,"net","download_bw",0);
	linphone_core_set_download_bandwidth(lc,tmp);
	tmp=lp_config_get_int(config,"net","upload_bw",0);
	linphone_core_set_upload_bandwidth(lc,tmp);
	linphone_core_set_stun_server(lc,lp_config_get_string(config,"net","stun_server",NULL));
	tmpstr=lp_config_get_string(lc->config,"net","nat_address",NULL);
	if (tmpstr!=NULL && (strlen(tmpstr)<1)) tmpstr=NULL;
	linphone_core_set_nat_address(lc,tmpstr);
	tmp=lp_config_get_int(lc->config,"net","firewall_policy",0);
	linphone_core_set_firewall_policy(lc,tmp);
	tmp=lp_config_get_int(lc->config,"net","nat_sdp_only",0);
	lc->net_conf.nat_sdp_only=tmp;
	tmp=lp_config_get_int(lc->config,"net","mtu",0);
	linphone_core_set_mtu(lc,tmp);
	tmp=lp_config_get_int(lc->config,"net","download_ptime",0);
	linphone_core_set_download_ptime(lc,tmp);

}

static void build_sound_devices_table(LinphoneCore *lc){
	const char **devices;
	const char **old;
	int ndev;
	int i;
	const MSList *elem=ms_snd_card_manager_get_list(ms_snd_card_manager_get());
	ndev=ms_list_size(elem);
	devices=ms_malloc((ndev+1)*sizeof(const char *));
	for (i=0;elem!=NULL;elem=elem->next,i++){
		devices[i]=ms_snd_card_get_string_id((MSSndCard *)elem->data);
	}
	devices[ndev]=NULL;
	old=lc->sound_conf.cards;
	lc->sound_conf.cards=devices;
	if (old!=NULL) ms_free(old);
}

static void sound_config_read(LinphoneCore *lc)
{
	/*int tmp;*/
	const char *tmpbuf;
	const char *devid;
	float gain=0;
#ifdef __linux
	/*alsadev let the user use custom alsa device within linphone*/
	devid=lp_config_get_string(lc->config,"sound","alsadev",NULL);
	if (devid){
		MSSndCard *card=ms_alsa_card_new_custom(devid,devid);
		ms_snd_card_manager_add_card(ms_snd_card_manager_get(),card);
	}
#endif
	/* retrieve all sound devices */
	build_sound_devices_table(lc);

	devid=lp_config_get_string(lc->config,"sound","playback_dev_id",NULL);
	linphone_core_set_playback_device(lc,devid);

	devid=lp_config_get_string(lc->config,"sound","ringer_dev_id",NULL);
	linphone_core_set_ringer_device(lc,devid);

	devid=lp_config_get_string(lc->config,"sound","capture_dev_id",NULL);
	linphone_core_set_capture_device(lc,devid);

/*
	tmp=lp_config_get_int(lc->config,"sound","play_lev",80);
	linphone_core_set_play_level(lc,tmp);
	tmp=lp_config_get_int(lc->config,"sound","ring_lev",80);
	linphone_core_set_ring_level(lc,tmp);
	tmp=lp_config_get_int(lc->config,"sound","rec_lev",80);
	linphone_core_set_rec_level(lc,tmp);
	tmpbuf=lp_config_get_string(lc->config,"sound","source","m");
	linphone_core_set_sound_source(lc,tmpbuf[0]);
*/

	tmpbuf=PACKAGE_SOUND_DIR "/" LOCAL_RING;
	tmpbuf=lp_config_get_string(lc->config,"sound","local_ring",tmpbuf);
	if (ortp_file_exist(tmpbuf)==-1) {
		tmpbuf=PACKAGE_SOUND_DIR "/" LOCAL_RING;
	}
	if (strstr(tmpbuf,".wav")==NULL){
		/* it currently uses old sound files, so replace them */
		tmpbuf=PACKAGE_SOUND_DIR "/" LOCAL_RING;
	}

	linphone_core_set_ring(lc,tmpbuf);

	tmpbuf=PACKAGE_SOUND_DIR "/" REMOTE_RING;
	tmpbuf=lp_config_get_string(lc->config,"sound","remote_ring",tmpbuf);
	if (ortp_file_exist(tmpbuf)==-1){
		tmpbuf=PACKAGE_SOUND_DIR "/" REMOTE_RING;
	}
	if (strstr(tmpbuf,".wav")==NULL){
		/* it currently uses old sound files, so replace them */
		tmpbuf=PACKAGE_SOUND_DIR "/" REMOTE_RING;
	}
	linphone_core_set_ringback(lc,tmpbuf);
	check_sound_device(lc);
	lc->sound_conf.latency=0;

	linphone_core_enable_echo_cancellation(lc,
	    lp_config_get_int(lc->config,"sound","echocancelation",0) |
	    lp_config_get_int(lc->config,"sound","echocancellation",0)
		);

	linphone_core_enable_echo_limiter(lc,
		lp_config_get_int(lc->config,"sound","echolimiter",0));
	linphone_core_enable_agc(lc,
		lp_config_get_int(lc->config,"sound","agc",0));

	gain=lp_config_get_float(lc->config,"sound","soft_play_lev",0);
		linphone_core_set_soft_play_level(lc,gain);
}

static void sip_config_read(LinphoneCore *lc)
{
	char *contact;
	const char *tmpstr;
	int port;
	int i,tmp;
	int ipv6;
	port=lp_config_get_int(lc->config,"sip","use_info",0);
	linphone_core_set_use_info_for_dtmf(lc,port);

	port=lp_config_get_int(lc->config,"sip","use_rfc2833",0);
	linphone_core_set_use_rfc2833_for_dtmf(lc,port);

	ipv6=lp_config_get_int(lc->config,"sip","use_ipv6",-1);
	if (ipv6==-1){
		ipv6=0;
		if (host_has_ipv6_network()){
			lc->vtable.display_message(lc,_("Your machine appears to be connected to an IPv6 network. By default linphone always uses IPv4. Please update your configuration if you want to use IPv6"));
		}
	}
	linphone_core_enable_ipv6(lc,ipv6);
	port=lp_config_get_int(lc->config,"sip","sip_port",5060);
	linphone_core_set_sip_port(lc,port);

	tmpstr=lp_config_get_string(lc->config,"sip","contact",NULL);
	if (tmpstr==NULL || linphone_core_set_primary_contact(lc,tmpstr)==-1) {
		const char *hostname=NULL;
		const char *username=NULL;
#ifdef HAVE_GETENV
		hostname=getenv("HOST");
		username=getenv("USER");
		if (hostname==NULL) hostname=getenv("HOSTNAME");
#endif /*HAVE_GETENV*/
		if (hostname==NULL)
			hostname="unknown-host";
		if (username==NULL){
			username="toto";
		}
		contact=ortp_strdup_printf("sip:%s@%s",username,hostname);
		linphone_core_set_primary_contact(lc,contact);
		ms_free(contact);
	}

	tmp=lp_config_get_int(lc->config,"sip","guess_hostname",1);
	linphone_core_set_guess_hostname(lc,tmp);


	tmp=lp_config_get_int(lc->config,"sip","inc_timeout",15);
	linphone_core_set_inc_timeout(lc,tmp);

	/* get proxies config */
	for(i=0;; i++){
		LinphoneProxyConfig *cfg=linphone_proxy_config_new_from_config_file(lc->config,i);
		if (cfg!=NULL){
			linphone_core_add_proxy_config(lc,cfg);
		}else{
			break;
		}
	}
	/* get the default proxy */
	tmp=lp_config_get_int(lc->config,"sip","default_proxy",-1);
	linphone_core_set_default_proxy_index(lc,tmp);

	/* read authentication information */
	for(i=0;; i++){
		LinphoneAuthInfo *ai=linphone_auth_info_new_from_config_file(lc->config,i);
		if (ai!=NULL){
			linphone_core_add_auth_info(lc,ai);
			linphone_auth_info_destroy(ai);
		}else{
			break;
		}
	}
	
	/*for tuning or test*/
	lc->sip_conf.sdp_200_ack=lp_config_get_int(lc->config,"sip","sdp_200_ack",0);
	lc->sip_conf.only_one_codec=lp_config_get_int(lc->config,"sip","only_one_codec",0);
	lc->sip_conf.register_only_when_network_is_up=
		lp_config_get_int(lc->config,"sip","register_only_when_network_is_up",1);
	lc->sip_conf.ping_with_options=lp_config_get_int(lc->config,"sip","ping_with_options",1);
	lc->sip_conf.auto_net_state_mon=lp_config_get_int(lc->config,"sip","auto_net_state_mon",1);
}

static void rtp_config_read(LinphoneCore *lc)
{
	int port;
	int jitt_comp;
	int nortp_timeout;
	port=lp_config_get_int(lc->config,"rtp","audio_rtp_port",7078);
	linphone_core_set_audio_port(lc,port);

	port=lp_config_get_int(lc->config,"rtp","video_rtp_port",9078);
	if (port==0) port=9078;
	linphone_core_set_video_port(lc,port);

	jitt_comp=lp_config_get_int(lc->config,"rtp","audio_jitt_comp",60);
	linphone_core_set_audio_jittcomp(lc,jitt_comp);
	jitt_comp=lp_config_get_int(lc->config,"rtp","video_jitt_comp",60);
	nortp_timeout=lp_config_get_int(lc->config,"rtp","nortp_timeout",30);
	linphone_core_set_nortp_timeout(lc,nortp_timeout);
}

static PayloadType * find_payload(RtpProfile *prof, const char *mime_type, int clock_rate, const char *recv_fmtp){
	PayloadType *candidate=NULL;
	int i;
	PayloadType *it;
	for(i=0;i<127;++i){
		it=rtp_profile_get_payload(prof,i);
		if (it!=NULL && strcasecmp(mime_type,it->mime_type)==0
			&& (clock_rate==it->clock_rate || clock_rate<=0) ){
			if ( (recv_fmtp && it->recv_fmtp && strcasecmp(recv_fmtp,it->recv_fmtp)==0) ||
				(recv_fmtp==NULL && it->recv_fmtp==NULL) ){
				/*exact match*/
				return it;
			}else candidate=it;
		}
	}
	return candidate;
}

static bool_t get_codec(LpConfig *config, char* type, int index, PayloadType **ret){
	char codeckey[50];
	const char *mime,*fmtp;
	int rate,enabled;
	PayloadType *pt;

	*ret=NULL;
	snprintf(codeckey,50,"%s_%i",type,index);
	mime=lp_config_get_string(config,codeckey,"mime",NULL);
	if (mime==NULL || strlen(mime)==0 ) return FALSE;

	rate=lp_config_get_int(config,codeckey,"rate",8000);
	fmtp=lp_config_get_string(config,codeckey,"recv_fmtp",NULL);
	enabled=lp_config_get_int(config,codeckey,"enabled",1);
	pt=find_payload(&av_profile,mime,rate,fmtp);
	if (pt && enabled ) pt->flags|=PAYLOAD_TYPE_ENABLED;
	//ms_message("Found codec %s/%i",pt->mime_type,pt->clock_rate);
	if (pt==NULL) ms_warning("Ignoring codec config %s/%i with fmtp=%s because unsupported",
	    		mime,rate,fmtp ? fmtp : "");
	*ret=pt;
	return TRUE;
}

static const char *codec_pref_order[]={
	"speex",
	"gsm",
	"pcmu",
	"pcma",
	"H264",
	"MP4V-ES",
	"theora",
	"H263-1998",
	"H263",
	NULL,
};

static int find_codec_rank(const char *mime){
	int i;
	for(i=0;codec_pref_order[i]!=NULL;++i){
		if (strcasecmp(codec_pref_order[i],mime)==0)
			break;
	}
	return i;
}

static int codec_compare(const PayloadType *a, const PayloadType *b){
	int ra,rb;
	ra=find_codec_rank(a->mime_type);
	rb=find_codec_rank(b->mime_type);
	if (ra>rb) return 1;
	if (ra<rb) return -1;
	return 0;
}

static MSList *add_missing_codecs(SalStreamType mtype, MSList *l){
	int i;
	for(i=0;i<127;++i){
		PayloadType *pt=rtp_profile_get_payload(&av_profile,i);
		if (pt){
			if (mtype==SalVideo && pt->type!=PAYLOAD_VIDEO)
				pt=NULL;
			else if (mtype==SalAudio && (pt->type!=PAYLOAD_AUDIO_PACKETIZED 
			    && pt->type!=PAYLOAD_AUDIO_CONTINUOUS)){
				pt=NULL;
			}
			if (pt && ms_filter_codec_supported(pt->mime_type)){
				if (ms_list_find(l,pt)==NULL){
					payload_type_set_flag(pt,PAYLOAD_TYPE_ENABLED);
					ms_message("Adding new codec %s/%i with fmtp %s",
					    pt->mime_type,pt->clock_rate,pt->recv_fmtp ? pt->recv_fmtp : "");
					l=ms_list_insert_sorted(l,pt,(int (*)(const void *, const void *))codec_compare);
				}
			}
		}
	}
	return l;
}

static void codecs_config_read(LinphoneCore *lc)
{
	int i;
	PayloadType *pt;
	MSList *audio_codecs=NULL;
	MSList *video_codecs=NULL;
	for (i=0;get_codec(lc->config,"audio_codec",i,&pt);i++){
		if (pt){
			if (!ms_filter_codec_supported(pt->mime_type)){
				ms_warning("Codec %s is not supported by mediastreamer2, removed.",pt->mime_type);
			}else audio_codecs=ms_list_append(audio_codecs,pt);
		}
	}
	audio_codecs=add_missing_codecs(SalAudio,audio_codecs);
	for (i=0;get_codec(lc->config,"video_codec",i,&pt);i++){
		if (pt){
			if (!ms_filter_codec_supported(pt->mime_type)){
				ms_warning("Codec %s is not supported by mediastreamer2, removed.",pt->mime_type);
			}else video_codecs=ms_list_append(video_codecs,(void *)pt);
		}
	}
	video_codecs=add_missing_codecs(SalVideo,video_codecs);
	linphone_core_set_audio_codecs(lc,audio_codecs);
	linphone_core_set_video_codecs(lc,video_codecs);
	linphone_core_update_allocated_audio_bandwidth(lc);
}

static void video_config_read(LinphoneCore *lc){
	int capture, display, self_view;
	int enabled;
	const char *str;
	int ndev;
	const char **devices;
	const MSList *elem;
	int i;

	/* retrieve all video devices */
	elem=ms_web_cam_manager_get_list(ms_web_cam_manager_get());
	ndev=ms_list_size(elem);
	devices=ms_malloc((ndev+1)*sizeof(const char *));
	for (i=0;elem!=NULL;elem=elem->next,i++){
		devices[i]=ms_web_cam_get_string_id((MSWebCam *)elem->data);
	}
	devices[ndev]=NULL;
	lc->video_conf.cams=devices;

	str=lp_config_get_string(lc->config,"video","device",NULL);
	if (str && str[0]==0) str=NULL;
	linphone_core_set_video_device(lc,str);

	linphone_core_set_preferred_video_size_by_name(lc,
		lp_config_get_string(lc->config,"video","size","cif"));

	enabled=lp_config_get_int(lc->config,"video","enabled",1);
	capture=lp_config_get_int(lc->config,"video","capture",enabled);
	display=lp_config_get_int(lc->config,"video","display",enabled);
	self_view=lp_config_get_int(lc->config,"video","self_view",enabled);
#ifdef VIDEO_ENABLED
	linphone_core_enable_video(lc,capture,display);
	linphone_core_enable_self_view(lc,self_view);
#endif
}

static void ui_config_read(LinphoneCore *lc)
{
	LinphoneFriend *lf;
	int i;
	for (i=0;(lf=linphone_friend_new_from_config_file(lc,i))!=NULL;i++){
		linphone_core_add_friend(lc,lf);
	}
	call_logs_read_from_config_file(lc);
}

/*
static void autoreplier_config_init(LinphoneCore *lc)
{
	autoreplier_config_t *config=&lc->autoreplier_conf;
	config->enabled=lp_config_get_int(lc->config,"autoreplier","enabled",0);
	config->after_seconds=lp_config_get_int(lc->config,"autoreplier","after_seconds",6);
	config->max_users=lp_config_get_int(lc->config,"autoreplier","max_users",1);
	config->max_rec_time=lp_config_get_int(lc->config,"autoreplier","max_rec_time",60);
	config->max_rec_msg=lp_config_get_int(lc->config,"autoreplier","max_rec_msg",10);
	config->message=lp_config_get_string(lc->config,"autoreplier","message",NULL);
}
*/

/**
 * Sets maximum available download bandwidth
 *
 * @ingroup media_parameters
 *
 * This is IP bandwidth, in kbit/s.
 * This information is used signaled to other parties during
 * calls (within SDP messages) so that the remote end can have
 * sufficient knowledge to properly configure its audio & video
 * codec output bitrate to not overflow available bandwidth.
 *
 * @param lc the LinphoneCore object
 * @param bw the bandwidth in kbits/s, 0 for infinite
 */
void linphone_core_set_download_bandwidth(LinphoneCore *lc, int bw){
	lc->net_conf.download_bw=bw;
	if (bw==0){ /*infinite*/
		lc->dw_audio_bw=-1;
		lc->dw_video_bw=-1;
	}else {
		lc->dw_audio_bw=MIN(lc->audio_bw,bw);
		lc->dw_video_bw=MAX(bw-lc->dw_audio_bw-10,0);/*-10: security margin*/
	}
}

/**
 * Sets maximum available upload bandwidth
 *
 * @ingroup media_parameters
 *
 * This is IP bandwidth, in kbit/s.
 * This information is used by liblinphone together with remote
 * side available bandwidth signaled in SDP messages to properly
 * configure audio & video codec's output bitrate.
 *
 * @param lc the LinphoneCore object
 * @param bw the bandwidth in kbits/s, 0 for infinite
 */
void linphone_core_set_upload_bandwidth(LinphoneCore *lc, int bw){
	lc->net_conf.upload_bw=bw;
	if (bw==0){ /*infinite*/
		lc->up_audio_bw=-1;
		lc->up_video_bw=-1;
	}else{
		lc->up_audio_bw=MIN(lc->audio_bw,bw);
		lc->up_video_bw=MAX(bw-lc->up_audio_bw-10,0);/*-10: security margin*/
	}
}

/**
 * Retrieve the maximum available download bandwidth.
 *
 * @ingroup media_parameters
 *
 * This value was set by linphone_core_set_download_bandwidth().
 *
**/
int linphone_core_get_download_bandwidth(const LinphoneCore *lc){
	return lc->net_conf.download_bw;
}

/**
 * Retrieve the maximum available upload bandwidth.
 *
 * @ingroup media_parameters
 *
 * This value was set by linphone_core_set_upload_bandwidth().
 *
**/
int linphone_core_get_upload_bandwidth(const LinphoneCore *lc){
	return lc->net_conf.upload_bw;
}
/**
 * set audio packetization time linphone expect to received from peer
 */
void linphone_core_set_download_ptime(LinphoneCore *lc, int ptime) {
	lc->net_conf.down_ptime=ptime;
}

int  linphone_core_get_download_ptime(LinphoneCore *lc) {
	return lc->net_conf.down_ptime;
}


/**
 * Returns liblinphone's version as a string.
 *
 * @ingroup misc
 *
**/
const char * linphone_core_get_version(void){
	return liblinphone_version;
}


static MSList *linphone_payload_types=NULL;

static void linphone_core_assign_payload_type(PayloadType *const_pt, int number, const char *recv_fmtp){
	PayloadType *pt;
	pt=payload_type_clone(const_pt);
	payload_type_set_number(pt,number);
	if (recv_fmtp!=NULL) payload_type_set_recv_fmtp(pt,recv_fmtp);
	rtp_profile_set_payload(&av_profile,number,pt);
	linphone_payload_types=ms_list_append(linphone_payload_types,pt);
}

static void linphone_core_free_payload_types(void){
	ms_list_for_each(linphone_payload_types,(void (*)(void*))payload_type_destroy);
	ms_list_free(linphone_payload_types);
	linphone_payload_types=NULL;
}

static void linphone_core_init (LinphoneCore * lc, const LinphoneCoreVTable *vtable, const char *config_path, 
    const char *factory_config_path, void * userdata)
{
	memset (lc, 0, sizeof (LinphoneCore));
	lc->data=userdata;

	memcpy(&lc->vtable,vtable,sizeof(LinphoneCoreVTable));

	gstate_initialize(lc);
	gstate_new_state(lc, GSTATE_POWER_STARTUP, NULL);

	ortp_init();
	linphone_core_assign_payload_type(&payload_type_pcmu8000,0,NULL);
	linphone_core_assign_payload_type(&payload_type_gsm,3,NULL);
	linphone_core_assign_payload_type(&payload_type_pcma8000,8,NULL);
	linphone_core_assign_payload_type(&payload_type_lpc1015,115,NULL);
	linphone_core_assign_payload_type(&payload_type_speex_nb,110,"vbr=on");
	linphone_core_assign_payload_type(&payload_type_speex_wb,111,"vbr=on");
	linphone_core_assign_payload_type(&payload_type_speex_uwb,112,"vbr=on");
	linphone_core_assign_payload_type(&payload_type_telephone_event,101,"0-11");
	linphone_core_assign_payload_type(&payload_type_ilbc,113,"mode=30");

#ifdef ENABLE_NONSTANDARD_GSM
	{
		PayloadType *pt;
		pt=payload_type_clone(&payload_type_gsm);
		pt->clock_rate=11025;
		rtp_profile_set_payload(&av_profile,114,pt);
		linphone_payload_types=ms_list_append(linphone_payload_types,pt);
		pt=payload_type_clone(&payload_type_gsm);
		pt->clock_rate=22050;
		rtp_profile_set_payload(&av_profile,115,pt);
		linphone_payload_types=ms_list_append(linphone_payload_types,pt);
	}
#endif

#ifdef VIDEO_ENABLED
	linphone_core_assign_payload_type(&payload_type_h263,34,NULL);
	linphone_core_assign_payload_type(&payload_type_theora,97,NULL);
	linphone_core_assign_payload_type(&payload_type_h263_1998,98,"CIF=1;QCIF=1");
	linphone_core_assign_payload_type(&payload_type_mp4v,99,"profile-level-id=3");
	linphone_core_assign_payload_type(&payload_type_x_snow,100,NULL);
	linphone_core_assign_payload_type(&payload_type_h264,102,NULL);
	linphone_core_assign_payload_type(&payload_type_h264,103,"packetization-mode=1");
#endif

	ms_init();

	lc->config=lp_config_new(config_path);
	if (factory_config_path)
		lp_config_read_file(lc->config,factory_config_path);

	lc->sal=sal_init();
	sal_set_user_pointer(lc->sal,lc);
	sal_set_callbacks(lc->sal,&linphone_sal_callbacks);
	if (lp_config_get_int(lc->config,"sip","use_session_timers",0)==1){
		sal_use_session_timers(lc->sal,200);
	}
	sip_setup_register_all();
	sound_config_read(lc);
	net_config_read(lc);
	rtp_config_read(lc);
	codecs_config_read(lc);
	sip_config_read(lc); /* this will start eXosip*/
	video_config_read(lc);
	//autoreplier_config_init(&lc->autoreplier_conf);
	lc->prev_mode=LINPHONE_STATUS_ONLINE;
	lc->presence_mode=LINPHONE_STATUS_ONLINE;
	lc->max_call_logs=15;
	ui_config_read(lc);
	lc->vtable.display_status(lc,_("Ready"));
        gstate_new_state(lc, GSTATE_POWER_ON, NULL);
	lc->auto_net_state_mon=lc->sip_conf.auto_net_state_mon;

    lc->ready=TRUE;
}

/**
 * Instanciates a LinphoneCore object.
 * @ingroup initializing
 * 
 * The LinphoneCore object is the primary handle for doing all phone actions.
 * It should be unique within your application.
 * @param vtable a LinphoneCoreVTable structure holding your application callbacks
 * @param config_path a path to a config file. If it does not exists it will be created.
 *        The config file is used to store all user settings, call logs, friends, proxies...
 * @param factory_config_path a path to a read-only config file that can be used to 
 *        to store hard-coded preference such as proxy settings or internal preferences.
 *        The settings in this factory file always override the one in the normal config file.
 *        It is OPTIONAL, use NULL if unneeded.
 * @param userdata an opaque user pointer that can be retrieved at any time (for example in
 *        callbacks) using linphone_core_get_user_data().
 * 
**/
LinphoneCore *linphone_core_new(const LinphoneCoreVTable *vtable,
						const char *config_path, const char *factory_config_path, void * userdata)
{
	LinphoneCore *core=ms_new(LinphoneCore,1);
	linphone_core_init(core,vtable,config_path, factory_config_path, userdata);
	return core;
}

/**
 * Returns the list of available audio codecs.
 *
 * This list is unmodifiable. The ->data field of the MSList points a PayloadType
 * structure holding the codec information.
 * It is possible to make copy of the list with ms_list_copy() in order to modify it
 * (such as the order of codecs).
**/
const MSList *linphone_core_get_audio_codecs(const LinphoneCore *lc)
{
	return lc->codecs_conf.audio_codecs;
}

/**
 * Returns the list of available video codecs.
 *
 * This list is unmodifiable. The ->data field of the MSList points a PayloadType
 * structure holding the codec information.
 * It is possible to make copy of the list with ms_list_copy() in order to modify it
 * (such as the order of codecs).
**/
const MSList *linphone_core_get_video_codecs(const LinphoneCore *lc)
{
	return lc->codecs_conf.video_codecs;
}

/**
 * Sets the local "from" identity.
 *
 * @ingroup proxies
 * This data is used in absence of any proxy configuration or when no
 * default proxy configuration is set. See LinphoneProxyConfig
**/
int linphone_core_set_primary_contact(LinphoneCore *lc, const char *contact)
{
	LinphoneAddress *ctt;

	if ((ctt=linphone_address_new(contact))==0) {
		ms_error("Bad contact url: %s",contact);
		return -1;
	}
	if (lc->sip_conf.contact!=NULL) ms_free(lc->sip_conf.contact);
	lc->sip_conf.contact=ms_strdup(contact);
	if (lc->sip_conf.guessed_contact!=NULL){
		ms_free(lc->sip_conf.guessed_contact);
		lc->sip_conf.guessed_contact=NULL;
	}
	linphone_address_destroy(ctt);
	return 0;
}


/*result must be an array of chars at least LINPHONE_IPADDR_SIZE */
void linphone_core_get_local_ip(LinphoneCore *lc, const char *dest, char *result){
	if (linphone_core_get_firewall_policy(lc)==LINPHONE_POLICY_USE_NAT_ADDRESS){
		strncpy(result,linphone_core_get_nat_address(lc),LINPHONE_IPADDR_SIZE);
		return;
	}
	if (dest==NULL) dest="87.98.157.38"; /*a public IP address*/
	if (linphone_core_get_local_ip_for(dest,result)==0)
		return;
	/*else fallback to SAL routine that will attempt to find the most realistic interface */
	sal_get_default_local_ip(lc->sal,lc->sip_conf.ipv6_enabled ? AF_INET6 : AF_INET,result,LINPHONE_IPADDR_SIZE);
}

static void update_primary_contact(LinphoneCore *lc){
	char *guessed=NULL;
	char tmp[LINPHONE_IPADDR_SIZE];

	LinphoneAddress *url;
	if (lc->sip_conf.guessed_contact!=NULL){
		ms_free(lc->sip_conf.guessed_contact);
		lc->sip_conf.guessed_contact=NULL;
	}
	url=linphone_address_new(lc->sip_conf.contact);
	if (!url){
		ms_error("Could not parse identity contact !");
		url=linphone_address_new("sip:unknown@unkwownhost");
	}
	linphone_core_get_local_ip(lc, NULL, tmp);
	if (strcmp(tmp,"127.0.0.1")==0 || strcmp(tmp,"::1")==0 ){
		ms_warning("Local loopback network only !");
		lc->sip_conf.loopback_only=TRUE;
	}else lc->sip_conf.loopback_only=FALSE;
	linphone_address_set_domain(url,tmp);
	linphone_address_set_port_int(url,lc->sip_conf.sip_port);
	guessed=linphone_address_as_string(url);
	lc->sip_conf.guessed_contact=guessed;
	linphone_address_destroy(url);
}

/**
 * Returns the default identity when no proxy configuration is used.
 *
 * @ingroup proxies
**/
const char *linphone_core_get_primary_contact(LinphoneCore *lc){
	char *identity;
	
	if (lc->sip_conf.guess_hostname){
		if (lc->sip_conf.guessed_contact==NULL || lc->sip_conf.loopback_only){
			update_primary_contact(lc);
		}
		identity=lc->sip_conf.guessed_contact;
	}else{
		identity=lc->sip_conf.contact;
	}
	return identity;
}

/**
 * Tells LinphoneCore to guess local hostname automatically in primary contact.
 *
 * @ingroup proxies
**/
void linphone_core_set_guess_hostname(LinphoneCore *lc, bool_t val){
	lc->sip_conf.guess_hostname=val;
}

/**
 * Returns TRUE if hostname part of primary contact is guessed automatically.
 *
 * @ingroup proxies
**/
bool_t linphone_core_get_guess_hostname(LinphoneCore *lc){
	return lc->sip_conf.guess_hostname;
}

/**
 * Same as linphone_core_get_primary_contact() but the result is a LinphoneAddress object
 * instead of const char*
 *
 * @ingroup proxies
**/
LinphoneAddress *linphone_core_get_primary_contact_parsed(LinphoneCore *lc){
	return linphone_address_new(linphone_core_get_primary_contact(lc));
}

/**
 * Sets the list of audio codecs.
 *
 * @ingroup media_parameters
 * The list is taken by the LinphoneCore thus the application should not free it.
 * This list is made of struct PayloadType describing the codec parameters.
**/
int linphone_core_set_audio_codecs(LinphoneCore *lc, MSList *codecs)
{
	if (lc->codecs_conf.audio_codecs!=NULL) ms_list_free(lc->codecs_conf.audio_codecs);
	lc->codecs_conf.audio_codecs=codecs;
	return 0;
}

/**
 * Sets the list of video codecs.
 *
 * @ingroup media_parameters
 * The list is taken by the LinphoneCore thus the application should not free it.
 * This list is made of struct PayloadType describing the codec parameters.
**/
int linphone_core_set_video_codecs(LinphoneCore *lc, MSList *codecs)
{
	if (lc->codecs_conf.video_codecs!=NULL) ms_list_free(lc->codecs_conf.video_codecs);
	lc->codecs_conf.video_codecs=codecs;
	return 0;
}

const MSList * linphone_core_get_friend_list(const LinphoneCore *lc)
{
	return lc->friends;
}

/**
 * Returns the nominal jitter buffer size in milliseconds.
 *
 * @ingroup media_parameters
**/
int linphone_core_get_audio_jittcomp(LinphoneCore *lc)
{
	return lc->rtp_conf.audio_jitt_comp;
}

/**
 * Returns the UDP port used for audio streaming.
 *
 * @ingroup network_parameters
**/
int linphone_core_get_audio_port(const LinphoneCore *lc)
{
	return lc->rtp_conf.audio_rtp_port;
}

/**
 * Returns the UDP port used for video streaming.
 *
 * @ingroup network_parameters
**/
int linphone_core_get_video_port(const LinphoneCore *lc){
	return lc->rtp_conf.video_rtp_port;
}


/**
 * Returns the value in seconds of the no-rtp timeout.
 *
 * @ingroup media_parameters
 * When no RTP or RTCP packets have been received for a while
 * LinphoneCore will consider the call is broken (remote end crashed or
 * disconnected from the network), and thus will terminate the call.
 * The no-rtp timeout is the duration above which the call is considered broken.
**/
int linphone_core_get_nortp_timeout(const LinphoneCore *lc){
	return lc->rtp_conf.nortp_timeout;
}

/**
 * Sets the nominal audio jitter buffer size in milliseconds.
 *
 * @ingroup media_parameters
**/
void linphone_core_set_audio_jittcomp(LinphoneCore *lc, int value)
{
	lc->rtp_conf.audio_jitt_comp=value;
}

/**
 * Sets the UDP port used for audio streaming.
 *
 * @ingroup network_parameters
**/
void linphone_core_set_audio_port(LinphoneCore *lc, int port)
{
	lc->rtp_conf.audio_rtp_port=port;
}

/**
 * Sets the UDP port used for video streaming.
 *
 * @ingroup network_parameters
**/
void linphone_core_set_video_port(LinphoneCore *lc, int port){
	lc->rtp_conf.video_rtp_port=port;
}

/**
 * Sets the no-rtp timeout value in seconds.
 * 
 * @ingroup media_parameters
 * See linphone_core_get_nortp_timeout() for details.
**/
void linphone_core_set_nortp_timeout(LinphoneCore *lc, int nortp_timeout){
	lc->rtp_conf.nortp_timeout=nortp_timeout;
}

/**
 * Indicates whether SIP INFO is used for sending digits.
 *
 * @ingroup media_parameters
**/
bool_t linphone_core_get_use_info_for_dtmf(LinphoneCore *lc)
{
	return lc->sip_conf.use_info;
}

/**
 * Sets whether SIP INFO is to be used for sending digits.
 *
 * @ingroup media_parameters
**/
void linphone_core_set_use_info_for_dtmf(LinphoneCore *lc,bool_t use_info)
{
	lc->sip_conf.use_info=use_info;
}

/**
 * Indicates whether RFC2833 is used for sending digits.
 *
 * @ingroup media_parameters
**/
bool_t linphone_core_get_use_rfc2833_for_dtmf(LinphoneCore *lc)
{
	return lc->sip_conf.use_rfc2833;
}

/**
 * Sets whether RFC2833 is to be used for sending digits.
 *
 * @ingroup media_parameters
**/
void linphone_core_set_use_rfc2833_for_dtmf(LinphoneCore *lc,bool_t use_rfc2833)
{
	lc->sip_conf.use_rfc2833=use_rfc2833;
}

/**
 * Returns the UDP port used by SIP.
 *
 * @ingroup network_parameters
**/
int linphone_core_get_sip_port(LinphoneCore *lc)
{
	return lc->sip_conf.sip_port;
}

static char _ua_name[64]="Linphone";
static char _ua_version[64]=LINPHONE_VERSION;

#ifdef HAVE_EXOSIP_GET_VERSION
extern const char *eXosip_get_version();
#endif

static void apply_user_agent(LinphoneCore *lc){
	char ua_string[256];
	snprintf(ua_string,sizeof(ua_string)-1,"%s/%s (eXosip2/%s)",_ua_name,_ua_version,
#ifdef HAVE_EXOSIP_GET_VERSION
		 eXosip_get_version()
#else
		 "unknown"
#endif
	);
	if (lc->sal) sal_set_user_agent(lc->sal,ua_string);
}

/**
 * Sets the user agent string used in SIP messages.
 *
 * @ingroup misc
**/
void linphone_core_set_user_agent(const char *name, const char *ver){
	strncpy(_ua_name,name,sizeof(_ua_name)-1);
	strncpy(_ua_version,ver,sizeof(_ua_version));
}

/**
 * Sets the UDP port to be used by SIP.
 *
 * @ingroup network_parameters
**/
void linphone_core_set_sip_port(LinphoneCore *lc,int port)
{
	const char *anyaddr;
	int err=0;
	if (port==lc->sip_conf.sip_port) return;
	lc->sip_conf.sip_port=port;

	if (lc->sal==NULL) return;
	
	if (lc->sip_conf.ipv6_enabled)
		anyaddr="::0";
	else
		anyaddr="0.0.0.0";
	err=sal_listen_port (lc->sal,anyaddr,port, SalTransportDatagram,FALSE);
	if (err<0){
		char *msg=ortp_strdup_printf("UDP port %i seems already in use ! Cannot initialize.",port);
		ms_warning(msg);
		lc->vtable.display_warning(lc,msg);
		ms_free(msg);
		return;
	}
	apply_user_agent(lc);
}

/**
 * Returns TRUE if IPv6 is enabled.
 *
 * @ingroup network_parameters
 * See linphone_core_enable_ipv6() for more details on how IPv6 is supported in liblinphone.
**/
bool_t linphone_core_ipv6_enabled(LinphoneCore *lc){
	return lc->sip_conf.ipv6_enabled;
}

/**
 * Turns IPv6 support on or off.
 *
 * @ingroup network_parameters
 *
 * @note IPv6 support is exclusive with IPv4 in liblinphone:
 * when IPv6 is turned on, IPv4 calls won't be possible anymore.
 * By default IPv6 support is off.
**/
void linphone_core_enable_ipv6(LinphoneCore *lc, bool_t val){
	if (lc->sip_conf.ipv6_enabled!=val){
		lc->sip_conf.ipv6_enabled=val;
		if (lc->sal){
			/* we need to restart eXosip */
			linphone_core_set_sip_port(lc, lc->sip_conf.sip_port);
		}
	}
}

static void display_bandwidth(RtpSession *as, RtpSession *vs){
	ms_message("bandwidth usage: audio=[d=%.1f,u=%.1f] video=[d=%.1f,u=%.1f] kbit/sec",
	(as!=NULL) ? (rtp_session_compute_recv_bandwidth(as)*1e-3) : 0,
	(as!=NULL) ? (rtp_session_compute_send_bandwidth(as)*1e-3) : 0,
	(vs!=NULL) ? (rtp_session_compute_recv_bandwidth(vs)*1e-3) : 0,
	(vs!=NULL) ? (rtp_session_compute_send_bandwidth(vs)*1e-3) : 0);
}

static void linphone_core_disconnected(LinphoneCore *lc){
	lc->vtable.display_warning(lc,_("Remote end seems to have disconnected, the call is going to be closed."));
	linphone_core_terminate_call(lc,NULL);
}

static void monitor_network_state(LinphoneCore *lc, time_t curtime){
	static time_t last_check=0;
	static bool_t last_status=FALSE;
	char result[LINPHONE_IPADDR_SIZE];
	bool_t new_status=last_status;

	/* only do the network up checking every five seconds */
	if (last_check==0 || (curtime-last_check)>=5){
		sal_get_default_local_ip(lc->sal,
		    lc->sip_conf.ipv6_enabled ? AF_INET6 : AF_INET,
		    result,LINPHONE_IPADDR_SIZE);
		if (strcmp(result,"::1")!=0 && strcmp(result,"127.0.0.1")!=0){
			new_status=TRUE;
		}else new_status=FALSE;
		last_check=curtime;
		if (new_status!=last_status) {
			set_network_reachable(lc,new_status);
			last_status=new_status;
		}
	}
}

static void proxy_update(LinphoneCore *lc){
	ms_list_for_each(lc->sip_conf.proxies,(void (*)(void*))&linphone_proxy_config_update);
}

static void assign_buddy_info(LinphoneCore *lc, BuddyInfo *info){
	LinphoneFriend *lf=linphone_core_get_friend_by_address(lc,info->sip_uri);
	if (lf!=NULL){
		lf->info=info;
		ms_message("%s has a BuddyInfo assigned with image %p",info->sip_uri, info->image_data);
		if (lc->vtable.buddy_info_updated)
			lc->vtable.buddy_info_updated(lc,lf);
	}else{
		ms_warning("Could not any friend with uri %s",info->sip_uri);
	}
}

static void analyze_buddy_lookup_results(LinphoneCore *lc, LinphoneProxyConfig *cfg){
	MSList *elem;
	SipSetupContext *ctx=linphone_proxy_config_get_sip_setup_context(cfg);
	for (elem=lc->bl_reqs;elem!=NULL;elem=ms_list_next(elem)){
		BuddyLookupRequest *req=(BuddyLookupRequest *)elem->data;
		if (req->status==BuddyLookupDone || req->status==BuddyLookupFailure){
			if (req->results!=NULL){
				BuddyInfo *i=(BuddyInfo*)req->results->data;
				ms_list_free(req->results);
				req->results=NULL;
				assign_buddy_info(lc,i);
			}
			sip_setup_context_buddy_lookup_free(ctx,req);
			elem->data=NULL;
		}
	}
	/*purge completed requests */
	while((elem=ms_list_find(lc->bl_reqs,NULL))!=NULL){
		lc->bl_reqs=ms_list_remove_link(lc->bl_reqs,elem);
	}
}

static void linphone_core_grab_buddy_infos(LinphoneCore *lc, LinphoneProxyConfig *cfg){
	const MSList *elem;
	SipSetupContext *ctx=linphone_proxy_config_get_sip_setup_context(cfg);
	for(elem=linphone_core_get_friend_list(lc);elem!=NULL;elem=elem->next){
		LinphoneFriend *lf=(LinphoneFriend*)elem->data;
		if (lf->info==NULL){
			if (linphone_core_lookup_known_proxy(lc,lf->uri)==cfg){
				if (linphone_address_get_username(lf->uri)!=NULL){
					BuddyLookupRequest *req;
					char *tmp=linphone_address_as_string_uri_only(lf->uri);
					req=sip_setup_context_create_buddy_lookup_request(ctx);
					buddy_lookup_request_set_key(req,tmp);
					buddy_lookup_request_set_max_results(req,1);
					sip_setup_context_buddy_lookup_submit(ctx,req);
					lc->bl_reqs=ms_list_append(lc->bl_reqs,req);
					ms_free(tmp);
				}
			}
		}
	}
}

static void linphone_core_do_plugin_tasks(LinphoneCore *lc){
	LinphoneProxyConfig *cfg=NULL;
	linphone_core_get_default_proxy(lc,&cfg);
	if (cfg){
		if (lc->bl_refresh){
			SipSetupContext *ctx=linphone_proxy_config_get_sip_setup_context(cfg);
			if (ctx && (sip_setup_context_get_capabilities(ctx) & SIP_SETUP_CAP_BUDDY_LOOKUP)){
				linphone_core_grab_buddy_infos(lc,cfg);
				lc->bl_refresh=FALSE;
			}
		}
		if (lc->bl_reqs) analyze_buddy_lookup_results(lc,cfg);
	}
}

/**
 * Main loop function. It is crucial that your application call it periodically.
 *
 * @ingroup initializing
 * linphone_core_iterate() performs various backgrounds tasks:
 * - receiving of SIP messages
 * - handles timers and timeout
 * - performs registration to proxies
 * - authentication retries
 * The application MUST call this function from periodically, in its main loop.
 * Be careful that this function must be call from the same thread as
 * other liblinphone methods. In not the case make sure all liblinphone calls are 
 * serialized with a mutex.
**/
void linphone_core_iterate(LinphoneCore *lc){
	int disconnect_timeout = linphone_core_get_nortp_timeout(lc);
	time_t curtime=time(NULL);
	int elapsed;
	bool_t one_second_elapsed=FALSE;
	bool_t disconnected=FALSE;

	if (curtime-lc->prevtime>=1){
		lc->prevtime=curtime;
		one_second_elapsed=TRUE;
	}

	if (lc->preview_finished){
		lc->preview_finished=0;
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
		lc_callback_obj_invoke(&lc->preview_finished_cb,lc);
	}

	sal_iterate(lc->sal);
	if (lc->auto_net_state_mon) monitor_network_state(lc,curtime);

	proxy_update(lc);

	if (lc->call!=NULL){
		LinphoneCall *call=lc->call;
		if (call->state==LCStatePreEstablishing && (curtime-call->start_time>=2)){
			/*start the call even if the OPTIONS reply did not arrive*/
			linphone_core_start_invite(lc,call,NULL);
		}
		if (call->dir==LinphoneCallIncoming && call->state==LCStateRinging){
			elapsed=curtime-call->start_time;
			ms_message("incoming call ringing for %i seconds",elapsed);
			if (elapsed>lc->sip_conf.inc_timeout){
				linphone_core_terminate_call(lc,NULL);
			}
		}else if (call->state==LCStateAVRunning){
			if (one_second_elapsed){
				RtpSession *as=NULL,*vs=NULL;
				lc->prevtime=curtime;
				if (lc->audiostream!=NULL)
					as=lc->audiostream->session;
				if (lc->videostream!=NULL)
					vs=lc->videostream->session;
				display_bandwidth(as,vs);
			}
#ifdef VIDEO_ENABLED
			if (lc->videostream!=NULL)
				video_stream_iterate(lc->videostream);
#endif
			if (lc->audiostream!=NULL && disconnect_timeout>0)
				disconnected=!audio_stream_alive(lc->audiostream,disconnect_timeout);
		}
	}
	if (linphone_core_video_preview_enabled(lc)){
		if (lc->previewstream==NULL)
			toggle_video_preview(lc,TRUE);
#ifdef VIDEO_ENABLED
		else video_stream_iterate(lc->previewstream);
#endif
	}else{
		if (lc->previewstream!=NULL)
			toggle_video_preview(lc,FALSE);
	}
	if (disconnected)
		linphone_core_disconnected(lc);

	linphone_core_do_plugin_tasks(lc);

	if (one_second_elapsed && lp_config_needs_commit(lc->config)){
		lp_config_sync(lc->config);
	}
}

/**
 * Interpret a call destination as supplied by the user, and returns a fully qualified
 * LinphoneAddress.
 *
 * A sip address should look like DisplayName <sip:username@domain:port> .
 * Basically this function performs the following tasks
 * - if a phone number is entered, prepend country prefix of the default proxy
 *   configuration, eventually escape the '+' by 00.
 * - if no domain part is supplied, append the domain name of the default proxy
 * - if no sip: is present, prepend it
 * 
 * The result is a syntaxically correct SIP address.
**/

LinphoneAddress * linphone_core_interpret_url(LinphoneCore *lc, const char *url){
	enum_lookup_res_t *enumres=NULL;
	char *enum_domain=NULL;
	LinphoneProxyConfig *proxy=lc->default_proxy;;
	char *tmpurl;
	LinphoneAddress *uri;
	
	if (is_enum(url,&enum_domain)){
		lc->vtable.display_status(lc,_("Looking for telephone number destination..."));
		if (enum_lookup(enum_domain,&enumres)<0){
			lc->vtable.display_status(lc,_("Could not resolve this number."));
			ms_free(enum_domain);
			return NULL;
		}
		ms_free(enum_domain);
		tmpurl=enumres->sip_address[0];
		uri=linphone_address_new(tmpurl);
		enum_lookup_res_free(enumres);
		return uri;
	}
	/* check if we have a "sip:" */
	if (strstr(url,"sip:")==NULL){
		/* this doesn't look like a true sip uri */
		if (strchr(url,'@')!=NULL){
			/* seems like sip: is missing !*/
			tmpurl=ms_strdup_printf("sip:%s",url);
			uri=linphone_address_new(tmpurl);
			ms_free(tmpurl);
			if (uri){
				return uri;
			}
		}
		
		if (proxy!=NULL){
			/* append the proxy domain suffix */
			const char *identity=linphone_proxy_config_get_identity(proxy);
			char normalized_username[128];
			uri=linphone_address_new(identity);
			if (uri==NULL){
				return NULL;
			}
			linphone_address_set_display_name(uri,NULL);
			linphone_proxy_config_normalize_number(proxy,url,normalized_username,
			    					sizeof(normalized_username));
			linphone_address_set_username(uri,normalized_username);
			return uri;
		}else return NULL;
	}
	uri=linphone_address_new(url);
	if (uri!=NULL){
		return uri;
	}
	/* else we could not do anything with url given by user, so display an error */
	if (lc->vtable.display_warning!=NULL){
		lc->vtable.display_warning(lc,_("Could not parse given sip address. A sip url usually looks like sip:user@domain"));
	}
	return NULL;
}

/**
 * Returns the default identity SIP address.
 *
 * @ingroup proxiesb
 * This is an helper function:
 *
 * If no default proxy is set, this will return the primary contact (
 * see linphone_core_get_primary_contact() ). If a default proxy is set
 * it returns the registered identity on the proxy.
**/
const char * linphone_core_get_identity(LinphoneCore *lc){
	LinphoneProxyConfig *proxy=NULL;
	const char *from;
	linphone_core_get_default_proxy(lc,&proxy);
	if (proxy!=NULL) {
		from=linphone_proxy_config_get_identity(proxy);
	}else from=linphone_core_get_primary_contact(lc);
	return from;
}

const char * linphone_core_get_route(LinphoneCore *lc){
	LinphoneProxyConfig *proxy=NULL;
	const char *route=NULL;
	linphone_core_get_default_proxy(lc,&proxy);
	if (proxy!=NULL) {
		route=linphone_proxy_config_get_route(proxy);
	}
	return route;
}

bool_t linphone_core_is_in_communication_with(LinphoneCore *lc, const char *to)
{
	char *tmp;
	bool_t returned;
	const LinphoneAddress *la=linphone_core_get_remote_uri(lc);
	if(la == NULL)
	{
		return FALSE;
	}
	tmp = linphone_address_as_string(la);
	if(!strcmp(tmp,to))
		returned = TRUE;
	else
		returned = FALSE;
	if(tmp)
		ms_free(tmp);
	return returned;
}

LinphoneProxyConfig * linphone_core_lookup_known_proxy(LinphoneCore *lc, const LinphoneAddress *uri){
	const MSList *elem;
	LinphoneProxyConfig *found_cfg=NULL;
	for (elem=linphone_core_get_proxy_config_list(lc);elem!=NULL;elem=elem->next){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		const char *domain=linphone_proxy_config_get_domain(cfg);
		if (domain!=NULL && strcmp(domain,linphone_address_get_domain(uri))==0){
			found_cfg=cfg;
			break;
		}
	}
	return found_cfg;
}

static char *get_fixed_contact(LinphoneCore *lc, LinphoneCall *call , LinphoneProxyConfig *dest_proxy){
	LinphoneAddress *ctt;
	const char *localip=call->localip;

	/* first use user's supplied ip address if asked*/
	if (linphone_core_get_firewall_policy(lc)==LINPHONE_POLICY_USE_NAT_ADDRESS){
		ctt=linphone_core_get_primary_contact_parsed(lc);
		return ms_strdup_printf("sip:%s@%s",linphone_address_get_username(ctt),
		    	linphone_core_get_nat_address(lc));
	}

	/* if already choosed, don't change it */
	if (call->op && sal_op_get_contact(call->op)!=NULL){
		return NULL;
	}
	/*if using a proxy, use the contact address as guessed with the REGISTERs*/
	if (dest_proxy && dest_proxy->op){
		const char *fixed_contact=sal_op_get_contact(dest_proxy->op);
		if (fixed_contact) {
			ms_message("Contact has been fixed using proxy to %s",fixed_contact);
			return ms_strdup(fixed_contact);
		}
	}
	/* if the ping OPTIONS request succeeded use the contact guessed from the
	 received, rport*/
	if (call->ping_op){
		const char *guessed=sal_op_get_contact(call->ping_op);
		if (guessed){
			ms_message("Contact has been fixed using OPTIONS to %s",guessed);
			return ms_strdup(guessed);
		}
	}
	
	ctt=linphone_core_get_primary_contact_parsed(lc);
	
	if (ctt!=NULL){
		char *ret;
		/*otherwise use supllied localip*/
		linphone_address_set_domain(ctt,localip);
		linphone_address_set_port_int(ctt,linphone_core_get_sip_port(lc));
		ret=linphone_address_as_string_uri_only(ctt);
		linphone_address_destroy(ctt);
		ms_message("Contact has been fixed using local ip to %s",ret);
		return ret;
	}
	return NULL;
}

int linphone_core_start_invite(LinphoneCore *lc, LinphoneCall *call, LinphoneProxyConfig *dest_proxy){
	int err;
	char *contact;
	char *real_url,*barmsg;
	char *from;
	/*try to be best-effort in giving real local or routable contact address */
	contact=get_fixed_contact(lc,call,dest_proxy);
	if (contact){
		sal_op_set_contact(call->op, contact);
		ms_free(contact);
	}
	call->state=LCStateInit;
	linphone_core_init_media_streams(lc,lc->call);
	if (!lc->sip_conf.sdp_200_ack){	
		call->media_pending=TRUE;
		sal_call_set_local_media_description(call->op,call->localdesc);
	}
	real_url=linphone_address_as_string(call->log->to);
	from=linphone_address_as_string(call->log->from);
	err=sal_call(call->op,from,real_url);

	if (lc->sip_conf.sdp_200_ack){
		call->media_pending=TRUE;
		sal_call_set_local_media_description(call->op,call->localdesc);
	}
	barmsg=ortp_strdup_printf("%s %s", _("Contacting"), real_url);
	lc->vtable.display_status(lc,barmsg);
	ms_free(barmsg);
	
	if (err<0){
		ms_warning("Could not initiate call.");
		lc->vtable.display_status(lc,_("could not call"));
		linphone_core_stop_media_streams(lc,call);
		linphone_call_destroy(call);
		lc->call=NULL;
	}else gstate_new_state(lc, GSTATE_CALL_OUT_INVITE, real_url);
	ms_free(real_url);
	ms_free(from);
	return err;
}

/**
 * Initiates an outgoing call
 *
 * @ingroup call_control
 * @param lc the LinphoneCore object
 * @param url the destination of the call (sip address, or phone number).
**/
int linphone_core_invite(LinphoneCore *lc, const char *url){
	LinphoneAddress *addr=linphone_core_interpret_url(lc,url);
	if (addr){
		int err;
		err=linphone_core_invite_address(lc,addr);
		linphone_address_destroy(addr);
		return err;
	}
	return -1;
}

/**
 * Initiates an outgoing call given a destination LinphoneAddress
 *
 * @ingroup call_control
 * @param lc the LinphoneCore object
 * @param url the destination of the call (sip address).
**/
int linphone_core_invite_address(LinphoneCore *lc, const LinphoneAddress *real_parsed_url)
{
	int err=0;
	const char *route=NULL;
	const char *from=NULL;
	LinphoneProxyConfig *proxy=NULL;
	LinphoneAddress *parsed_url2=NULL;
	char *real_url=NULL;
	LinphoneProxyConfig *dest_proxy=NULL;
	LinphoneCall *call;
	
	if (lc->call!=NULL){
		lc->vtable.display_warning(lc,_("Sorry, having multiple simultaneous calls is not supported yet !"));
		return -1;
	}

	linphone_core_get_default_proxy(lc,&proxy);
	route=linphone_core_get_route(lc);
	
	real_url=linphone_address_as_string(real_parsed_url);
	dest_proxy=linphone_core_lookup_known_proxy(lc,real_parsed_url);

	if (proxy!=dest_proxy && dest_proxy!=NULL) {
		ms_message("Overriding default proxy setting for this call:");
		ms_message("The used identity will be %s",linphone_proxy_config_get_identity(dest_proxy));
	}

	if (dest_proxy!=NULL)
		from=linphone_proxy_config_get_identity(dest_proxy);
	else if (proxy!=NULL)
		from=linphone_proxy_config_get_identity(proxy);

	/* if no proxy or no identity defined for this proxy, default to primary contact*/
	if (from==NULL) from=linphone_core_get_primary_contact(lc);

	parsed_url2=linphone_address_new(from);

	call=linphone_call_new_outgoing(lc,parsed_url2,linphone_address_clone(real_parsed_url));
	sal_op_set_route(call->op,route);
	
	lc->call=call;
	if (dest_proxy!=NULL || lc->sip_conf.ping_with_options==FALSE){
		err=linphone_core_start_invite(lc,call,dest_proxy);
	}else{
		/*defer the start of the call after the OPTIONS ping*/
		call->state=LCStatePreEstablishing;
		call->ping_op=sal_op_new(lc->sal);
		sal_ping(call->ping_op,from,real_url);
		call->start_time=time(NULL);
	}
	
	if (real_url!=NULL) ms_free(real_url);
	return err;
}

int linphone_core_refer(LinphoneCore *lc, const char *url)
{
	char *real_url=NULL;
	LinphoneAddress *real_parsed_url=linphone_core_interpret_url(lc,url);
	LinphoneCall *call;

	if (!real_parsed_url){
		/* bad url */
		return -1;
	}
	call=lc->call;
	if (call==NULL){
		ms_warning("No established call to refer.");
		return -1;
	}
	//lc->call=NULL; //Do not do that you will lose the call afterward . . .
	real_url=linphone_address_as_string (real_parsed_url);
	sal_refer(call->op,real_url);
	ms_free(real_url);
	return 0;
}

/**
 * Returns true if in incoming call is pending, ie waiting for being answered or declined.
 *
 * @ingroup call_control
**/
bool_t linphone_core_inc_invite_pending(LinphoneCore*lc){
	if (lc->call!=NULL && lc->call->dir==LinphoneCallIncoming){
		return TRUE;
	}
	return FALSE;
}

void linphone_core_init_media_streams(LinphoneCore *lc, LinphoneCall *call){
	SalMediaDescription *md=call->localdesc;
	lc->audiostream=audio_stream_new(md->streams[0].port,linphone_core_ipv6_enabled(lc));
	if (linphone_core_echo_limiter_enabled(lc)){
		const char *type=lp_config_get_string(lc->config,"sound","el_type","mic");
		if (strcasecmp(type,"mic")==0)
			audio_stream_enable_echo_limiter(lc->audiostream,ELControlMic);
		else if (strcasecmp(type,"speaker")==0)
			audio_stream_enable_echo_limiter(lc->audiostream,ELControlSpeaker);
	}
	audio_stream_enable_gain_control(lc->audiostream,TRUE);
	if (linphone_core_echo_cancellation_enabled(lc)){
		int len,delay,framesize;
		len=lp_config_get_int(lc->config,"sound","ec_tail_len",0);
		delay=lp_config_get_int(lc->config,"sound","ec_delay",0);
		framesize=lp_config_get_int(lc->config,"sound","ec_framesize",0);
		audio_stream_set_echo_canceller_params(lc->audiostream,len,delay,framesize);
	}
	audio_stream_enable_automatic_gain_control(lc->audiostream,linphone_core_agc_enabled(lc));
	{
		int enabled=lp_config_get_int(lc->config,"sound","noisegate",0);
		audio_stream_enable_noise_gate(lc->audiostream,enabled);
	}
	if (lc->a_rtp)
		rtp_session_set_transports(lc->audiostream->session,lc->a_rtp,lc->a_rtcp);

#ifdef VIDEO_ENABLED
	if ((lc->video_conf.display || lc->video_conf.capture) && md->streams[1].port>0)
		lc->videostream=video_stream_new(md->streams[1].port,linphone_core_ipv6_enabled(lc));
#else
	lc->videostream=NULL;
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
		lc->vtable.dtmf_received(lc, dtmf_tab[dtmf]);
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

static void post_configure_audio_streams(LinphoneCore *lc){
	AudioStream *st=lc->audiostream;
	float gain=lp_config_get_float(lc->config,"sound","mic_gain",-1);
	if (gain!=-1)
		audio_stream_set_mic_gain(st,gain);
	float recv_gain = lc->sound_conf.soft_play_lev;
	if (recv_gain != 0) {
		linphone_core_set_soft_play_level(lc,recv_gain);
	}
	if (linphone_core_echo_limiter_enabled(lc)){
		float speed=lp_config_get_float(lc->config,"sound","el_speed",-1);
		float thres=lp_config_get_float(lc->config,"sound","el_thres",-1);
		float force=lp_config_get_float(lc->config,"sound","el_force",-1);
		int sustain=lp_config_get_int(lc->config,"sound","el_sustain",-1);
		MSFilter *f=NULL;
		if (st->el_type==ELControlMic){
			f=st->volsend;
			if (speed==-1) speed=0.03;
			if (force==-1) force=10;
		}
		else if (st->el_type==ELControlSpeaker){
			f=st->volrecv;
			if (speed==-1) speed=0.02;
			if (force==-1) force=5;
		}
		if (speed!=-1)
			ms_filter_call_method(f,MS_VOLUME_SET_EA_SPEED,&speed);
		if (thres!=-1)
			ms_filter_call_method(f,MS_VOLUME_SET_EA_THRESHOLD,&thres);
		if (force!=-1)
			ms_filter_call_method(f,MS_VOLUME_SET_EA_FORCE,&force);
		if (sustain!=-1)
			ms_filter_call_method(f,MS_VOLUME_SET_EA_SUSTAIN,&sustain);

	}
	if (st->volsend){
		float ng_thres=lp_config_get_float(lc->config,"sound","ng_thres",0.05);
		float ng_floorgain=lp_config_get_float(lc->config,"sound","ng_floorgain",0);
		ms_filter_call_method(st->volsend,MS_VOLUME_SET_NOISE_GATE_THRESHOLD,&ng_thres);
		ms_filter_call_method(st->volsend,MS_VOLUME_SET_NOISE_GATE_FLOORGAIN,&ng_floorgain);
	}
	parametrize_equalizer(lc,st);
	if (lc->vtable.dtmf_received!=NULL){
		/* replace by our default action*/
		audio_stream_play_received_dtmfs(lc->audiostream,FALSE);
		rtp_session_signal_connect(lc->audiostream->session,"telephone-event",(RtpCallback)linphone_core_dtmf_received,(unsigned long)lc);
	}
}

static RtpProfile *make_profile(LinphoneCore *lc, const SalMediaDescription *md, const SalStreamDescription *desc, int *used_pt){
	int bw;
	const MSList *elem;
	RtpProfile *prof=rtp_profile_new("Call profile");
	bool_t first=TRUE;
	int remote_bw=0;
	
	for(elem=desc->payloads;elem!=NULL;elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
	
		if (first) {
			if (desc->type==SalAudio){
				linphone_core_update_allocated_audio_bandwidth_in_call(lc,pt);
			}
			*used_pt=payload_type_get_number(pt);
			first=FALSE;
		}
		if (desc->bandwidth>0) remote_bw=desc->bandwidth;
		else if (md->bandwidth>0) {
			/*case where b=AS is given globally, not per stream*/
			remote_bw=md->bandwidth;
			if (desc->type==SalVideo){
				remote_bw-=lc->audio_bw;
			}
		}
		
		if (desc->type==SalAudio){			
				bw=get_min_bandwidth(lc->up_audio_bw,remote_bw);
		}else bw=get_min_bandwidth(lc->up_video_bw,remote_bw);
		if (bw>0) pt->normal_bitrate=bw*1000;
		else if (desc->type==SalAudio){
			pt->normal_bitrate=-1;
		}
		if (desc->ptime>0){
			char tmp[40];
			snprintf(tmp,sizeof(tmp),"ptime=%i",desc->ptime);
			payload_type_append_send_fmtp(pt,tmp);
		}
		rtp_profile_set_payload(prof,payload_type_get_number(pt),pt);
	}
	return prof;
}

void linphone_core_start_media_streams(LinphoneCore *lc, LinphoneCall *call){
	LinphoneAddress *me=linphone_core_get_primary_contact_parsed(lc);
	const char *tool="linphone-" LINPHONE_VERSION;
	char *cname;
	int used_pt=-1;
	/* adjust rtp jitter compensation. It must be at least the latency of the sound card */
	int jitt_comp=MAX(lc->sound_conf.latency,lc->rtp_conf.audio_jitt_comp);

	if (call->media_start_time==0) call->media_start_time=time(NULL);

	cname=linphone_address_as_string_uri_only(me);
	{
		const SalStreamDescription *stream=sal_media_description_find_stream(call->resultdesc,
		    					SalProtoRtpAvp,SalAudio);
		if (stream){
			call->audio_profile=make_profile(lc,call->resultdesc,stream,&used_pt);
			if (!lc->use_files){
				MSSndCard *playcard=lc->sound_conf.play_sndcard;
				MSSndCard *captcard=lc->sound_conf.capt_sndcard;
				if (playcard==NULL) {
					ms_warning("No card defined for playback !");
					goto end;
				}
				if (captcard==NULL) {
					ms_warning("No card defined for capture !");
					goto end;
				}
				audio_stream_start_now(
					lc->audiostream,
					call->audio_profile,
					stream->addr[0]!='\0' ? stream->addr : call->resultdesc->addr,
					stream->port,
					stream->port+1,
					used_pt,
					jitt_comp,
					playcard,
					captcard,
					linphone_core_echo_cancellation_enabled(lc));
			}else{
				audio_stream_start_with_files(
					lc->audiostream,
					call->audio_profile,
					stream->addr[0]!='\0' ? stream->addr : call->resultdesc->addr,
					stream->port,
					stream->port+1,
					used_pt,
					100,
					lc->play_file,
					lc->rec_file);
			}
			post_configure_audio_streams(lc);
			audio_stream_set_rtcp_information(lc->audiostream, cname, tool);
		}else ms_warning("No audio stream defined ?");
	}
#ifdef VIDEO_ENABLED
	{
		const SalStreamDescription *stream=sal_media_description_find_stream(call->resultdesc,
		    					SalProtoRtpAvp,SalVideo);
		/* shutdown preview */
		if (lc->previewstream!=NULL) {
			video_preview_stop(lc->previewstream);
			lc->previewstream=NULL;
		}
		if (stream && (lc->video_conf.display || lc->video_conf.capture)) {
			const char *addr=stream->addr[0]!='\0' ? stream->addr : call->resultdesc->addr;
			call->video_profile=make_profile(lc,call->resultdesc,stream,&used_pt);
			video_stream_set_sent_video_size(lc->videostream,linphone_core_get_preferred_video_size(lc));
			video_stream_enable_self_view(lc->videostream,lc->video_conf.selfview);
			if (lc->video_conf.display && lc->video_conf.capture)
				video_stream_start(lc->videostream,
				call->video_profile, addr, stream->port,
				stream->port+1,
				used_pt, jitt_comp, lc->video_conf.device);
			else if (lc->video_conf.display)
				video_stream_recv_only_start(lc->videostream,
				call->video_profile, addr, stream->port,
				used_pt, jitt_comp);
			else if (lc->video_conf.capture)
				video_stream_send_only_start(lc->videostream,
				call->video_profile, addr, stream->port,
				stream->port+1,
				used_pt, jitt_comp, lc->video_conf.device);
			video_stream_set_rtcp_information(lc->videostream, cname,tool);
		}
	}
#endif
	goto end;
	end:
		ms_free(cname);
		linphone_address_destroy(me);
		lc->call->state=LCStateAVRunning;
}

void linphone_core_stop_media_streams(LinphoneCore *lc, LinphoneCall *call){
	if (lc->audiostream!=NULL) {
		audio_stream_stop(lc->audiostream);
		lc->audiostream=NULL;
	}
#ifdef VIDEO_ENABLED
	if (lc->videostream!=NULL){
		if (lc->video_conf.display && lc->video_conf.capture)
			video_stream_stop(lc->videostream);
		else if (lc->video_conf.display)
			video_stream_recv_only_stop(lc->videostream);
		else if (lc->video_conf.capture)
			video_stream_send_only_stop(lc->videostream);
		lc->videostream=NULL;
	}
	if (linphone_core_video_preview_enabled(lc)){
		if (lc->previewstream==NULL){
			lc->previewstream=video_preview_start(lc->video_conf.device, lc->video_conf.vsize);
		}
	}
#endif
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

/**
 * Accept an incoming call.
 *
 * @ingroup call_control
 * Basically the application is notified of incoming calls within the
 * invite_recv callback of the #LinphoneCoreVTable structure.
 * The application can later respond positively to the call using
 * this method.
 * @param lc the LinphoneCore object
 * @param url the SIP address of the originator of the call, or NULL.
 *            This argument is useful for managing multiple calls simulatenously,
 *            however this feature is not supported yet.
 *            Using NULL will accept the unique incoming call in progress.
**/
int linphone_core_accept_call(LinphoneCore *lc, const char *url)
{
	LinphoneCall *call=lc->call;
	const char *contact=NULL;
	
	if (call==NULL){
		return -1;
	}

	if (call->state==LCStateAVRunning){
		/*call already accepted*/
		return -1;
	}

	/*stop ringing */
	if (lc->ringstream!=NULL) {
		ms_message("stop ringing");
		ring_stop(lc->ringstream);
		ms_message("ring stopped");
		lc->ringstream=NULL;
	}
	
	/*try to be best-effort in giving real local or routable contact address*/
	contact=get_fixed_contact(lc,call,NULL);
	if (contact)
		sal_op_set_contact(call->op,contact);
	
	sal_call_accept(call->op);
	lc->vtable.display_status(lc,_("Connected."));
	gstate_new_state(lc, GSTATE_CALL_IN_CONNECTED, NULL);
	call->resultdesc=sal_call_get_final_media_description(call->op);
	if (call->resultdesc){
		sal_media_description_ref(call->resultdesc);
		linphone_core_start_media_streams(lc, call);
	}else call->media_pending=TRUE;
	ms_message("call answered.");
	return 0;
}

/**
 * Terminates a call.
 *
 * @ingroup call_control
 * @param lc The LinphoneCore
 * @param url the destination of the call to be terminated, use NULL if there is
 *            only one call (which is case in this version of liblinphone).
**/
int linphone_core_terminate_call(LinphoneCore *lc, const char *url)
{
	LinphoneCall *call=lc->call;
	if (call==NULL){
		return -1;
	}
	lc->call=NULL;
	sal_call_terminate(call->op);

	/*stop ringing*/
	if (lc->ringstream!=NULL) {
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
	}
	linphone_core_stop_media_streams(lc,call);
	lc->vtable.display_status(lc,_("Call ended") );
	gstate_new_state(lc, GSTATE_CALL_END, NULL);
	linphone_call_destroy(call);
	return 0;
}

/**
 * Returns TRUE if there is a call running or pending.
 *
 * @ingroup call_control
**/
bool_t linphone_core_in_call(const LinphoneCore *lc){
	return lc->call!=NULL;
}

/**
 * Returns The _LinphoneCall struct of the current call if one is in call
 *
 * @ingroup call_control
**/
struct _LinphoneCall *linphone_core_get_current_call(LinphoneCore *lc)
{
	if(linphone_core_in_call(lc))
		return lc->call;
	else
		return NULL;
}

int linphone_core_send_publish(LinphoneCore *lc,
			       LinphoneOnlineStatus presence_mode)
{
	const MSList *elem;
	for (elem=linphone_core_get_proxy_config_list(lc);elem!=NULL;elem=ms_list_next(elem)){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		if (cfg->publish) linphone_proxy_config_send_publish(cfg,presence_mode);
	}
	return 0;
}

/**
 * Set the incoming call timeout in seconds.
 *
 * @ingroup call_control
 * If an incoming call isn't answered for this timeout period, it is 
 * automatically declined.
**/
void linphone_core_set_inc_timeout(LinphoneCore *lc, int seconds){
	lc->sip_conf.inc_timeout=seconds;
}

/**
 * Returns the incoming call timeout
 *
 * @ingroup call_control
 * See linphone_core_set_inc_timeout() for details.
**/
int linphone_core_get_inc_timeout(LinphoneCore *lc){
	return lc->sip_conf.inc_timeout;
}

void linphone_core_set_presence_info(LinphoneCore *lc,int minutes_away,
													const char *contact,
													LinphoneOnlineStatus presence_mode)
{
	if (minutes_away>0) lc->minutes_away=minutes_away;
	
	if (lc->alt_contact!=NULL) {
		ms_free(lc->alt_contact);
		lc->alt_contact=NULL;
	}
	if (contact) lc->alt_contact=ms_strdup(contact);
	if (lc->presence_mode!=presence_mode){
		linphone_core_notify_all_friends(lc,presence_mode);
		/*
		   Improve the use of all LINPHONE_STATUS available.
		   !TODO Do not mix "presence status" with "answer status code"..
		   Use correct parameter to follow sip_if_match/sip_etag.
		 */
		linphone_core_send_publish(lc,presence_mode);
	}
	lc->prev_mode=lc->presence_mode;
	lc->presence_mode=presence_mode;
}

LinphoneOnlineStatus linphone_core_get_presence_info(const LinphoneCore *lc){
	return lc->presence_mode;
}

/**
 * Get playback sound level in 0-100 scale.
 *
 * @ingroup media_parameters
**/
int linphone_core_get_play_level(LinphoneCore *lc)
{
	return lc->sound_conf.play_lev;
}

/**
 * Get ring sound level in 0-100 scale
 *
 * @ingroup media_parameters
**/
int linphone_core_get_ring_level(LinphoneCore *lc)
{
	return lc->sound_conf.ring_lev;
}

/**
 * Get sound capture level in 0-100 scale
 *
 * @ingroup media_parameters
**/
int linphone_core_get_rec_level(LinphoneCore *lc){
	return lc->sound_conf.rec_lev;
}

/**
 * Set sound ring level in 0-100 scale
 *
 * @ingroup media_parameters
**/
void linphone_core_set_ring_level(LinphoneCore *lc, int level){
	MSSndCard *sndcard;
	lc->sound_conf.ring_lev=level;
	sndcard=lc->sound_conf.ring_sndcard;
	if (sndcard) ms_snd_card_set_level(sndcard,MS_SND_CARD_PLAYBACK,level);
}


void linphone_core_set_soft_play_level(LinphoneCore *lc, float level){
	float gain=level;
	lc->sound_conf.soft_play_lev=level;
	AudioStream *st=lc->audiostream;
	if (!st) return; /*just return*/

	if (st->volrecv){
		ms_filter_call_method(st->volrecv,MS_VOLUME_SET_DB_GAIN,&gain);
	}else ms_warning("Could not apply gain: gain control wasn't activated.");
}
float linphone_core_get_soft_play_level(LinphoneCore *lc) {
	float gain=0;
	AudioStream *st=lc->audiostream;
	if (st->volrecv){
		ms_filter_call_method(st->volrecv,MS_VOLUME_GET,&gain);
	}else ms_warning("Could not get gain: gain control wasn't activated.");

	return gain;
}

/**
 * Set sound playback level in 0-100 scale
 *
 * @ingroup media_parameters
**/
void linphone_core_set_play_level(LinphoneCore *lc, int level){
	MSSndCard *sndcard;
	lc->sound_conf.play_lev=level;
	sndcard=lc->sound_conf.play_sndcard;
	if (sndcard) ms_snd_card_set_level(sndcard,MS_SND_CARD_PLAYBACK,level);
}

/**
 * Set sound capture level in 0-100 scale
 *
 * @ingroup media_parameters
**/
void linphone_core_set_rec_level(LinphoneCore *lc, int level)
{
	MSSndCard *sndcard;
	lc->sound_conf.rec_lev=level;
	sndcard=lc->sound_conf.capt_sndcard;
	if (sndcard) ms_snd_card_set_level(sndcard,MS_SND_CARD_CAPTURE,level);
}

static MSSndCard *get_card_from_string_id(const char *devid, unsigned int cap){
	MSSndCard *sndcard=NULL;
	if (devid!=NULL){
		sndcard=ms_snd_card_manager_get_card(ms_snd_card_manager_get(),devid);
		if (sndcard!=NULL &&
			(ms_snd_card_get_capabilities(sndcard) & cap)==0 ){
			ms_warning("%s card does not have the %s capability, ignoring.",
				devid,
				cap==MS_SND_CARD_CAP_CAPTURE ? "capture" : "playback");
			sndcard=NULL;
		}
	}
	if (sndcard==NULL) {
		/* get a card that has read+write capabilities */
		sndcard=ms_snd_card_manager_get_default_card(ms_snd_card_manager_get());
		/* otherwise refine to the first card having the right capability*/
		if (sndcard==NULL){
			const MSList *elem=ms_snd_card_manager_get_list(ms_snd_card_manager_get());
			for(;elem!=NULL;elem=elem->next){
				sndcard=(MSSndCard*)elem->data;
				if (ms_snd_card_get_capabilities(sndcard) & cap) break;
			}
		}
		if (sndcard==NULL){/*looks like a bug! take the first one !*/
			const MSList *elem=ms_snd_card_manager_get_list(ms_snd_card_manager_get());
			if (elem) sndcard=(MSSndCard*)elem->data;
        }
	}
	if (sndcard==NULL) ms_error("Could not find a suitable soundcard !");
	return sndcard;
}

/**
 * Returns true if the specified sound device can capture sound.
 *
 * @ingroup media_parameters
 * @param devid the device name as returned by linphone_core_get_sound_devices()
**/
bool_t linphone_core_sound_device_can_capture(LinphoneCore *lc, const char *devid){
	MSSndCard *sndcard;
	sndcard=ms_snd_card_manager_get_card(ms_snd_card_manager_get(),devid);
	if (sndcard!=NULL && (ms_snd_card_get_capabilities(sndcard) & MS_SND_CARD_CAP_CAPTURE)) return TRUE;
	return FALSE;
}

/**
 * Returns true if the specified sound device can play sound.
 *
 * @ingroup media_parameters
 * @param devid the device name as returned by linphone_core_get_sound_devices()
**/
bool_t linphone_core_sound_device_can_playback(LinphoneCore *lc, const char *devid){
	MSSndCard *sndcard;
	sndcard=ms_snd_card_manager_get_card(ms_snd_card_manager_get(),devid);
	if (sndcard!=NULL && (ms_snd_card_get_capabilities(sndcard) & MS_SND_CARD_CAP_PLAYBACK)) return TRUE;
	return FALSE;
}

/**
 * Sets the sound device used for ringing.
 *
 * @ingroup media_parameters
 * @param devid the device name as returned by linphone_core_get_sound_devices()
**/
int linphone_core_set_ringer_device(LinphoneCore *lc, const char * devid){
	MSSndCard *card=get_card_from_string_id(devid,MS_SND_CARD_CAP_PLAYBACK);
	lc->sound_conf.ring_sndcard=card;
	if (card && lc->ready)
		lp_config_set_string(lc->config,"sound","ringer_dev_id",ms_snd_card_get_string_id(card));
	return 0;
}

/**
 * Sets the sound device used for playback.
 *
 * @ingroup media_parameters
 * @param devid the device name as returned by linphone_core_get_sound_devices()
**/
int linphone_core_set_playback_device(LinphoneCore *lc, const char * devid){
	MSSndCard *card=get_card_from_string_id(devid,MS_SND_CARD_CAP_PLAYBACK);
	lc->sound_conf.play_sndcard=card;
	if (card && lc->ready)
		lp_config_set_string(lc->config,"sound","playback_dev_id",ms_snd_card_get_string_id(card));
	return 0;
}

/**
 * Sets the sound device used for capture.
 *
 * @ingroup media_parameters
 * @param devid the device name as returned by linphone_core_get_sound_devices()
**/
int linphone_core_set_capture_device(LinphoneCore *lc, const char * devid){
	MSSndCard *card=get_card_from_string_id(devid,MS_SND_CARD_CAP_CAPTURE);
	lc->sound_conf.capt_sndcard=card;
	if (card && lc->ready)
		lp_config_set_string(lc->config,"sound","capture_dev_id",ms_snd_card_get_string_id(card));
	return 0;
}

/**
 * Returns the name of the currently assigned sound device for ringing.
 *
 * @ingroup media_parameters
**/
const char * linphone_core_get_ringer_device(LinphoneCore *lc)
{
	if (lc->sound_conf.ring_sndcard) return ms_snd_card_get_string_id(lc->sound_conf.ring_sndcard);
	return NULL;
}

/**
 * Returns the name of the currently assigned sound device for playback.
 *
 * @ingroup media_parameters
**/
const char * linphone_core_get_playback_device(LinphoneCore *lc)
{
	return lc->sound_conf.play_sndcard ? ms_snd_card_get_string_id(lc->sound_conf.play_sndcard) : NULL;
}

/**
 * Returns the name of the currently assigned sound device for capture.
 *
 * @ingroup media_parameters
**/
const char * linphone_core_get_capture_device(LinphoneCore *lc)
{
	return lc->sound_conf.capt_sndcard ? ms_snd_card_get_string_id(lc->sound_conf.capt_sndcard) : NULL;
}

/**
 * Returns an unmodifiable array of available sound devices.
 *
 * @ingroup media_parameters
 * The array is NULL terminated.
**/
const char**  linphone_core_get_sound_devices(LinphoneCore *lc){
	build_sound_devices_table(lc);
	return lc->sound_conf.cards;
}

/**
 * Returns an unmodifiable array of available video capture devices.
 *
 * @ingroup media_parameters
 * The array is NULL terminated.
**/
const char**  linphone_core_get_video_devices(const LinphoneCore *lc){
	return lc->video_conf.cams;
}

char linphone_core_get_sound_source(LinphoneCore *lc)
{
	return lc->sound_conf.source;
}

void linphone_core_set_sound_source(LinphoneCore *lc, char source)
{
	MSSndCard *sndcard=lc->sound_conf.capt_sndcard;
	lc->sound_conf.source=source;
	if (!sndcard) return;
	switch(source){
		case 'm':
			ms_snd_card_set_capture(sndcard,MS_SND_CARD_MIC);
			break;
		case 'l':
			ms_snd_card_set_capture(sndcard,MS_SND_CARD_LINE);
			break;
	}

}


/**
 * Sets the path to a wav file used for ringing.
 *
 * The file must be a wav 16bit linear.
 *
 * @ingroup media_parameters
**/
void linphone_core_set_ring(LinphoneCore *lc,const char *path){
	if (lc->sound_conf.local_ring!=0){
		ms_free(lc->sound_conf.local_ring);
	}
	lc->sound_conf.local_ring=ms_strdup(path);
	if (lc->ready && lc->sound_conf.local_ring)
		lp_config_set_string(lc->config,"sound","local_ring",lc->sound_conf.local_ring);
}

/**
 * Returns the path to the wav file used for ringing.
 *
 * @ingroup media_parameters
**/
const char *linphone_core_get_ring(const LinphoneCore *lc){
	return lc->sound_conf.local_ring;
}

static void notify_end_of_ring(void *ud ,unsigned int event, void * arg){
	LinphoneCore *lc=(LinphoneCore*)ud;
	lc->preview_finished=1;
}

int linphone_core_preview_ring(LinphoneCore *lc, const char *ring,LinphoneCoreCbFunc func,void * userdata)
{
	if (lc->ringstream!=0){
		ms_warning("Cannot start ring now,there's already a ring being played");
		return -1;
	}
	lc_callback_obj_init(&lc->preview_finished_cb,func,userdata);
	lc->preview_finished=0;
	if (lc->sound_conf.ring_sndcard!=NULL){
		lc->ringstream=ring_start_with_cb(ring,2000,lc->sound_conf.ring_sndcard,notify_end_of_ring,(void *)lc);
	}
	return 0;
}

/**
 * Sets the path to a wav file used for ringing back.
 *
 * Ringback means the ring that is heard when it's ringing at the remote party.
 * The file must be a wav 16bit linear.
 *
 * @ingroup media_parameters
**/
void linphone_core_set_ringback(LinphoneCore *lc, const char *path){
	if (lc->sound_conf.remote_ring!=0){
		ms_free(lc->sound_conf.remote_ring);
	}
	lc->sound_conf.remote_ring=ms_strdup(path);
}

/**
 * Returns the path to the wav file used for ringing back.
 *
 * @ingroup media_parameters
**/
const char * linphone_core_get_ringback(const LinphoneCore *lc){
	return lc->sound_conf.remote_ring;
}

/**
 * Enables or disable echo cancellation.
 *
 * @ingroup media_parameters
**/
void linphone_core_enable_echo_cancellation(LinphoneCore *lc, bool_t val){
	lc->sound_conf.ec=val;
	if (lc->ready)
		lp_config_set_int(lc->config,"sound","echocancellation",val);
}

/**
 * Returns TRUE if echo cancellation is enabled.
 *
 * @ingroup media_parameters
**/
bool_t linphone_core_echo_cancellation_enabled(LinphoneCore *lc){
	return lc->sound_conf.ec;
}

void linphone_core_enable_echo_limiter(LinphoneCore *lc, bool_t val){
	lc->sound_conf.ea=val;
}

bool_t linphone_core_echo_limiter_enabled(const LinphoneCore *lc){
	return lc->sound_conf.ea;
}

/**
 * Mutes or unmutes the local microphone.
 *
 * @ingroup media_parameters
**/
void linphone_core_mute_mic(LinphoneCore *lc, bool_t val){
	if (lc->audiostream!=NULL){
		 audio_stream_set_mic_gain(lc->audiostream,
			(val==TRUE) ? 0 : 1.0);
	}
}

bool_t linphone_core_is_mic_muted(LinphoneCore *lc) {
	float gain=1.0;
	if (lc->audiostream && lc->audiostream->volsend){
			ms_filter_call_method(lc->audiostream->volsend,MS_VOLUME_GET_GAIN,&gain);
	}else ms_warning("Could not get gain: gain control wasn't activated. ");

	return gain==0;
}

void linphone_core_enable_agc(LinphoneCore *lc, bool_t val){
	lc->sound_conf.agc=val;
}

bool_t linphone_core_agc_enabled(const LinphoneCore *lc){
	return lc->sound_conf.agc;
}

/**
 * Send the specified dtmf.
 *
 * @ingroup media_parameters
 * This function only works during calls. The dtmf is automatically played to the user.
 * @param lc The LinphoneCore object
 * @param dtmf The dtmf name specified as a char, such as '0', '#' etc...
 *
**/
void linphone_core_send_dtmf(LinphoneCore *lc, char dtmf)
{
	/*By default we send DTMF RFC2833 if we do not have enabled SIP_INFO but we can also send RFC2833 and SIP_INFO*/
	if (linphone_core_get_use_rfc2833_for_dtmf(lc)!=0 || linphone_core_get_use_info_for_dtmf(lc)==0)
	{
		/* In Band DTMF */
		if (lc->audiostream!=NULL){
			audio_stream_send_dtmf(lc->audiostream,dtmf);
		}
		else
		{
			ms_error("we cannot send RFC2833 dtmf when we are not in communication");
		}
	}
	if (linphone_core_get_use_info_for_dtmf(lc)!=0){
		/* Out of Band DTMF (use INFO method) */
		LinphoneCall *call=lc->call;
		if (call==NULL){
			return;
		}
		sal_call_send_dtmf(call->op,dtmf);
	}
}

void linphone_core_set_stun_server(LinphoneCore *lc, const char *server){
	if (lc->net_conf.stun_server!=NULL)
		ms_free(lc->net_conf.stun_server);
	if (server)
		lc->net_conf.stun_server=ms_strdup(server);
	else lc->net_conf.stun_server=NULL;
}

const char * linphone_core_get_stun_server(const LinphoneCore *lc){
	return lc->net_conf.stun_server;
}

const char * linphone_core_get_relay_addr(const LinphoneCore *lc){
	return lc->net_conf.relay;
}

int linphone_core_set_relay_addr(LinphoneCore *lc, const char *addr){
	if (lc->net_conf.relay!=NULL){
		ms_free(lc->net_conf.relay);
		lc->net_conf.relay=NULL;
	}
	if (addr){
		lc->net_conf.relay=ms_strdup(addr);
	}
	return 0;
}

void linphone_core_set_nat_address(LinphoneCore *lc, const char *addr)
{
	if (lc->net_conf.nat_address!=NULL){
		ms_free(lc->net_conf.nat_address);
	}
	if (addr!=NULL) lc->net_conf.nat_address=ms_strdup(addr);
	else lc->net_conf.nat_address=NULL;
	if (lc->sip_conf.contact) update_primary_contact(lc);
}

const char *linphone_core_get_nat_address(const LinphoneCore *lc)
{
	return lc->net_conf.nat_address;
}

void linphone_core_set_firewall_policy(LinphoneCore *lc, LinphoneFirewallPolicy pol){
	lc->net_conf.firewall_policy=pol;
	if (lc->sip_conf.contact) update_primary_contact(lc);
}

LinphoneFirewallPolicy linphone_core_get_firewall_policy(const LinphoneCore *lc){
	return lc->net_conf.firewall_policy;
}

/**
 * Get the list of call logs (past calls).
 *
 * @ingroup call_logs
**/
const MSList * linphone_core_get_call_logs(LinphoneCore *lc){
	lc->missed_calls=0;
	return lc->call_logs;
}

/**
 * Erase the call log.
 *
 * @ingroup call_logs
**/
void linphone_core_clear_call_logs(LinphoneCore *lc){
	lc->missed_calls=0;
	ms_list_for_each(lc->call_logs,(void (*)(void*))linphone_call_log_destroy);
	lc->call_logs=ms_list_free(lc->call_logs);
	call_logs_write_to_config_file(lc);
}

static void toggle_video_preview(LinphoneCore *lc, bool_t val){
#ifdef VIDEO_ENABLED
	if (lc->videostream==NULL){
		if (val){
			if (lc->previewstream==NULL){
				lc->previewstream=video_preview_start(lc->video_conf.device,
							lc->video_conf.vsize);
			}
		}else{
			if (lc->previewstream!=NULL){
				video_preview_stop(lc->previewstream);
				lc->previewstream=NULL;
			}
		}
	}
#endif
}

/**
 * Enables video globally.
 *
 * @ingroup media_parameters
 * This function does not have any effect during calls. It just indicates LinphoneCore to
 * initiate future calls with video or not. The two boolean parameters indicate in which
 * direction video is enabled. Setting both to false disables video entirely.
 *
 * @param vcap_enabled indicates whether video capture is enabled
 * @param display_enabled indicates whether video display should be shown
 *
**/
void linphone_core_enable_video(LinphoneCore *lc, bool_t vcap_enabled, bool_t display_enabled){
#ifndef VIDEO_ENABLED
	if (vcap_enabled || display_enabled)
		ms_warning("This version of linphone was built without video support.");
#endif
	lc->video_conf.capture=vcap_enabled;
	lc->video_conf.display=display_enabled;

	/* need to re-apply network bandwidth settings*/
	linphone_core_set_download_bandwidth(lc,
		linphone_core_get_download_bandwidth(lc));
	linphone_core_set_upload_bandwidth(lc,
		linphone_core_get_upload_bandwidth(lc));
}

/**
 * Returns TRUE if video is enabled, FALSE otherwise.
 * @ingroup media_parameters
**/
bool_t linphone_core_video_enabled(LinphoneCore *lc){
	return (lc->video_conf.display || lc->video_conf.capture);
}

/**
 * Controls video preview enablement.
 *
 * @ingroup media_parameters
 * Video preview refers to the action of displaying the local webcam image
 * to the user while not in call.
**/
void linphone_core_enable_video_preview(LinphoneCore *lc, bool_t val){
	lc->video_conf.show_local=val;
}

/**
 * Returns TRUE if video previewing is enabled.
 * @ingroup media_parameters
**/
bool_t linphone_core_video_preview_enabled(const LinphoneCore *lc){
	return lc->video_conf.show_local;
}

/**
 * Enables or disable self view during calls.
 *
 * @ingroup media_parameters
 * Self-view refers to having local webcam image inserted in corner
 * of the video window during calls.
 * This function works at any time, including during calls.
**/
void linphone_core_enable_self_view(LinphoneCore *lc, bool_t val){
	lc->video_conf.selfview=val;
#ifdef VIDEO_ENABLED
	if (lc->videostream){
		video_stream_enable_self_view(lc->videostream,val);
	}
#endif
}

/**
 * Returns TRUE if self-view is enabled, FALSE otherwise.
 *
 * @ingroup media_parameters
 *
 * Refer to linphone_core_enable_self_view() for details.
**/
bool_t linphone_core_self_view_enabled(const LinphoneCore *lc){
	return lc->video_conf.selfview;
}

/**
 * Sets the active video device.
 *
 * @ingroup media_parameters
 * @param id the name of the video device as returned by linphone_core_get_video_devices()
**/
int linphone_core_set_video_device(LinphoneCore *lc, const char *id){
	MSWebCam *olddev=lc->video_conf.device;
	const char *vd;
	if (id!=NULL){
		lc->video_conf.device=ms_web_cam_manager_get_cam(ms_web_cam_manager_get(),id);
		if (lc->video_conf.device==NULL){
			ms_warning("Could not found video device %s",id);
		}
	}
	if (lc->video_conf.device==NULL)
		lc->video_conf.device=ms_web_cam_manager_get_default_cam(ms_web_cam_manager_get());
	if (olddev!=NULL && olddev!=lc->video_conf.device){
		toggle_video_preview(lc,FALSE);/*restart the video local preview*/
	}
	if (lc->ready && lc->video_conf.device){
		vd=ms_web_cam_get_string_id(lc->video_conf.device);
		if (vd && strstr(vd,"Static picture")!=NULL){
			vd=NULL;
		}
		lp_config_set_string(lc->config,"video","device",vd);
	}
	return 0;
}

/**
 * Returns the name of the currently active video device.
 *
 * @ingroup media_parameters
**/
const char *linphone_core_get_video_device(const LinphoneCore *lc){
	if (lc->video_conf.device) return ms_web_cam_get_string_id(lc->video_conf.device);
	return NULL;
}

int linphone_core_set_static_picture(LinphoneCore *lc, const char *path) {
#ifdef VIDEO_ENABLED
	VideoStream *vs = NULL;
	/* Select the video stream from the call in the first place */
	if (lc && lc->videostream) {
		vs = lc->videostream;
	}
	/* If not in call, select the video stream from the preview */
	if (vs == NULL && lc && lc->previewstream) {
		vs = lc->previewstream;
	}
	
	/* If we have a video stream (either preview, either from call), we
		 have a source and it is using the static picture filter, then
		 force the filter to use that picture. */
	if (vs && vs->source) {
		if (ms_filter_get_id(vs->source) == MS_STATIC_IMAGE_ID) {
			ms_filter_call_method(vs->source, MS_FILTER_SET_IMAGE,
														(void *)path);
		}
	}

	/* Tell the static image filter to use that image from now on so
		 that the image will be used next time it has to be read */
	ms_static_image_set_default_image(path);
#else
	ms_warning("Video support not compiled.");
#endif
	return 0;
}

/**
 * Returns the native window handle of the video window, casted as an unsigned long.
 *
 * @ingroup media_parameters
**/
unsigned long linphone_core_get_native_video_window_id(const LinphoneCore *lc){
#ifdef VIDEO_ENABLED
	if (lc->videostream)
		return video_stream_get_native_window_id(lc->videostream);
	if (lc->previewstream)
		return video_stream_get_native_window_id(lc->previewstream);
#endif
	return 0;
}

static MSVideoSizeDef supported_resolutions[]={
	{	{MS_VIDEO_SIZE_SVGA_W,MS_VIDEO_SIZE_SVGA_H}	,	"svga"	},
	{	{MS_VIDEO_SIZE_4CIF_W,MS_VIDEO_SIZE_4CIF_H}	,	"4cif"	},
	{	{MS_VIDEO_SIZE_VGA_W,MS_VIDEO_SIZE_VGA_H}	,	"vga"	},
	{	{MS_VIDEO_SIZE_CIF_W,MS_VIDEO_SIZE_CIF_H}	,	"cif"	},
	{	{MS_VIDEO_SIZE_QVGA_W,MS_VIDEO_SIZE_QVGA_H}	,	"qvga"	},
	{	{MS_VIDEO_SIZE_QCIF_W,MS_VIDEO_SIZE_QCIF_H}	,	"qcif"	},
	{	{0,0}			,	NULL	}
};

/**
 * Returns the zero terminated table of supported video resolutions.
 *
 * @ingroup media_parameters
**/
const MSVideoSizeDef *linphone_core_get_supported_video_sizes(LinphoneCore *lc){
	return supported_resolutions;
}

static MSVideoSize video_size_get_by_name(const char *name){
	MSVideoSizeDef *pdef=supported_resolutions;
	MSVideoSize null_vsize={0,0};
	for(;pdef->name!=NULL;pdef++){
		if (strcasecmp(name,pdef->name)==0){
			return pdef->vsize;
		}
	}
	ms_warning("Video resolution %s is not supported in linphone.",name);
	return null_vsize;
}

static const char *video_size_get_name(MSVideoSize vsize){
	MSVideoSizeDef *pdef=supported_resolutions;
	for(;pdef->name!=NULL;pdef++){
		if (pdef->vsize.width==vsize.width && pdef->vsize.height==vsize.height){
			return pdef->name;
		}
	}
	return NULL;
}

static bool_t video_size_supported(MSVideoSize vsize){
	if (video_size_get_name(vsize)) return TRUE;
	ms_warning("Video resolution %ix%i is not supported in linphone.",vsize.width,vsize.height);
	return FALSE;
}

/**
 * Sets the preferred video size.
 *
 * @ingroup media_parameters
 * This applies only to the stream that is captured and sent to the remote party,
 * since we accept all standard video size on the receive path.
**/
void linphone_core_set_preferred_video_size(LinphoneCore *lc, MSVideoSize vsize){
	if (video_size_supported(vsize)){
		MSVideoSize oldvsize=lc->video_conf.vsize;
		lc->video_conf.vsize=vsize;
		if (!ms_video_size_equal(oldvsize,vsize) && lc->previewstream!=NULL){
			toggle_video_preview(lc,FALSE);
			toggle_video_preview(lc,TRUE);
		}
		if (lc->ready)
			lp_config_set_string(lc->config,"video","size",video_size_get_name(vsize));
	}
}

/**
 * Sets the preferred video size by its name.
 *
 * @ingroup media_parameters
 * This is identical to linphone_core_set_preferred_video_size() except
 * that it takes the name of the video resolution as input.
 * Video resolution names are: qcif, svga, cif, vga, 4cif, svga ...
**/
void linphone_core_set_preferred_video_size_by_name(LinphoneCore *lc, const char *name){
	MSVideoSize vsize=video_size_get_by_name(name);
	MSVideoSize default_vsize={MS_VIDEO_SIZE_CIF_W,MS_VIDEO_SIZE_CIF_H};
	if (vsize.width!=0)	linphone_core_set_preferred_video_size(lc,vsize);
	else linphone_core_set_preferred_video_size(lc,default_vsize);
}

/**
 * Returns the current preferred video size for sending.
 *
 * @ingroup media_parameters
**/
MSVideoSize linphone_core_get_preferred_video_size(LinphoneCore *lc){
	return lc->video_conf.vsize;
}

void linphone_core_use_files(LinphoneCore *lc, bool_t yesno){
	lc->use_files=yesno;
}

void linphone_core_set_play_file(LinphoneCore *lc, const char *file){
	if (lc->play_file!=NULL){
		ms_free(lc->play_file);
		lc->play_file=NULL;
	}
	if (file!=NULL) {
		lc->play_file=ms_strdup(file);
		if (lc->audiostream->ticker)
			audio_stream_play(lc->audiostream,file);
	}
}

void linphone_core_set_record_file(LinphoneCore *lc, const char *file){
	if (lc->rec_file!=NULL){
		ms_free(lc->rec_file);
		lc->rec_file=NULL;
	}
	if (file!=NULL) {
		lc->rec_file=ms_strdup(file);
		if (lc->audiostream)
			audio_stream_record(lc->audiostream,file);
	}
}

/**
 * Retrieves the user pointer that was given to linphone_core_new()
 *
 * @ingroup initializing
**/
void *linphone_core_get_user_data(LinphoneCore *lc){
	return lc->data;
}

int linphone_core_get_mtu(const LinphoneCore *lc){
	return lc->net_conf.mtu;
}

void linphone_core_set_mtu(LinphoneCore *lc, int mtu){
	lc->net_conf.mtu=mtu;
	if (mtu>0){
		if (mtu<500){
			ms_error("MTU too small !");
			mtu=500;
		}
		ms_set_mtu(mtu);
		ms_message("MTU is supposed to be %i, rtp payload max size will be %i",mtu, ms_get_payload_max_size());
	}else ms_set_mtu(0);//use mediastreamer2 default value
}

void linphone_core_set_waiting_callback(LinphoneCore *lc, LinphoneWaitingCallback cb, void *user_context){
	lc->wait_cb=cb;
	lc->wait_ctx=user_context;
}

void linphone_core_start_waiting(LinphoneCore *lc, const char *purpose){
	if (lc->wait_cb){
		lc->wait_ctx=lc->wait_cb(lc,lc->wait_ctx,LinphoneWaitingStart,purpose,0);
	}
}

void linphone_core_update_progress(LinphoneCore *lc, const char *purpose, float progress){
	if (lc->wait_cb){
		lc->wait_ctx=lc->wait_cb(lc,lc->wait_ctx,LinphoneWaitingProgress,purpose,progress);
	}else{
#ifdef WIN32
		Sleep(50000);
#else
		usleep(50000);
#endif
	}
}

void linphone_core_stop_waiting(LinphoneCore *lc){
	if (lc->wait_cb){
		lc->wait_ctx=lc->wait_cb(lc,lc->wait_ctx,LinphoneWaitingFinished,NULL,0);
	}
}

void linphone_core_set_audio_transports(LinphoneCore *lc, RtpTransport *rtp, RtpTransport *rtcp){
	lc->a_rtp=rtp;
	lc->a_rtcp=rtcp;
}

void net_config_uninit(LinphoneCore *lc)
{
	net_config_t *config=&lc->net_conf;
	lp_config_set_int(lc->config,"net","download_bw",config->download_bw);
	lp_config_set_int(lc->config,"net","upload_bw",config->upload_bw);

	if (config->stun_server!=NULL){
		lp_config_set_string(lc->config,"net","stun_server",config->stun_server);
		ms_free(lc->net_conf.stun_server);
	}
	if (config->nat_address!=NULL){
		lp_config_set_string(lc->config,"net","nat_address",config->nat_address);
		ms_free(lc->net_conf.nat_address);
	}
	lp_config_set_int(lc->config,"net","firewall_policy",config->firewall_policy);
	lp_config_set_int(lc->config,"net","mtu",config->mtu);	
}


void sip_config_uninit(LinphoneCore *lc)
{
	MSList *elem;
	int i;
	sip_config_t *config=&lc->sip_conf;
	lp_config_set_int(lc->config,"sip","sip_port",config->sip_port);
	lp_config_set_int(lc->config,"sip","guess_hostname",config->guess_hostname);
	lp_config_set_string(lc->config,"sip","contact",config->contact);
	lp_config_set_int(lc->config,"sip","inc_timeout",config->inc_timeout);
	lp_config_set_int(lc->config,"sip","use_info",config->use_info);
	lp_config_set_int(lc->config,"sip","use_rfc2833",config->use_rfc2833);
	lp_config_set_int(lc->config,"sip","use_ipv6",config->ipv6_enabled);
	lp_config_set_int(lc->config,"sip","register_only_when_network_is_up",config->register_only_when_network_is_up);

	lp_config_set_int(lc->config,"sip","default_proxy",linphone_core_get_default_proxy(lc,NULL));
	
	for(elem=config->proxies,i=0;elem!=NULL;elem=ms_list_next(elem),i++){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)(elem->data);
		linphone_proxy_config_write_to_config_file(lc->config,cfg,i);
		linphone_proxy_config_edit(cfg);	/* to unregister */
	}

	if (lc->sal){
	    int i;
		for (i=0;i<20;i++){
			sal_iterate(lc->sal);
#ifndef WIN32
			usleep(100000);
#else
			Sleep(100);
#endif
		}
	}

	ms_list_for_each(config->proxies,(void (*)(void*)) linphone_proxy_config_destroy);
	ms_list_free(config->proxies);
	config->proxies=NULL;
	
	linphone_proxy_config_write_to_config_file(lc->config,NULL,i);	/*mark the end */

	for(elem=lc->auth_info,i=0;elem!=NULL;elem=ms_list_next(elem),i++){
		LinphoneAuthInfo *ai=(LinphoneAuthInfo*)(elem->data);
		linphone_auth_info_write_config(lc->config,ai,i);
	}
	linphone_auth_info_write_config(lc->config,NULL,i); /* mark the end */
	ms_list_for_each(lc->auth_info,(void (*)(void*))linphone_auth_info_destroy);
	ms_list_free(lc->auth_info);
	lc->auth_info=NULL;
	
	sal_uninit(lc->sal);
	lc->sal=NULL;

	if (lc->sip_conf.guessed_contact)
		ms_free(lc->sip_conf.guessed_contact);
	if (config->contact)
		ms_free(config->contact);
	
}

void rtp_config_uninit(LinphoneCore *lc)
{
	rtp_config_t *config=&lc->rtp_conf;
	lp_config_set_int(lc->config,"rtp","audio_rtp_port",config->audio_rtp_port);
	lp_config_set_int(lc->config,"rtp","video_rtp_port",config->video_rtp_port);
	lp_config_set_int(lc->config,"rtp","audio_jitt_comp",config->audio_jitt_comp);
	lp_config_set_int(lc->config,"rtp","video_jitt_comp",config->video_jitt_comp);
	lp_config_set_int(lc->config,"rtp","nortp_timeout",config->nortp_timeout);
}

void sound_config_uninit(LinphoneCore *lc)
{
	sound_config_t *config=&lc->sound_conf;
	ms_free(config->cards);

	lp_config_set_string(lc->config,"sound","remote_ring",config->remote_ring);

	if (config->local_ring) ms_free(config->local_ring);
	if (config->remote_ring) ms_free(config->remote_ring);
	ms_snd_card_manager_destroy();
}

void video_config_uninit(LinphoneCore *lc)
{
	lp_config_set_int(lc->config,"video","enabled",linphone_core_video_enabled(lc));
	lp_config_set_string(lc->config,"video","size",video_size_get_name(linphone_core_get_preferred_video_size(lc)));
	lp_config_set_int(lc->config,"video","display",lc->video_conf.display);
	lp_config_set_int(lc->config,"video","capture",lc->video_conf.capture);
	lp_config_set_int(lc->config,"video","show_local",linphone_core_video_preview_enabled(lc));
	lp_config_set_int(lc->config,"video","self_view",linphone_core_self_view_enabled(lc));
	if (lc->video_conf.cams)
		ms_free(lc->video_conf.cams);
}

void codecs_config_uninit(LinphoneCore *lc)
{
	PayloadType *pt;
	codecs_config_t *config=&lc->codecs_conf;
	MSList *node;
	char key[50];
	int index;
	index=0;
	for(node=config->audio_codecs;node!=NULL;node=ms_list_next(node)){
		pt=(PayloadType*)(node->data);
		sprintf(key,"audio_codec_%i",index);
		lp_config_set_string(lc->config,key,"mime",pt->mime_type);
		lp_config_set_int(lc->config,key,"rate",pt->clock_rate);
		lp_config_set_int(lc->config,key,"enabled",linphone_core_payload_type_enabled(lc,pt));
		index++;
	}
	index=0;
	for(node=config->video_codecs;node!=NULL;node=ms_list_next(node)){
		pt=(PayloadType*)(node->data);
		sprintf(key,"video_codec_%i",index);
		lp_config_set_string(lc->config,key,"mime",pt->mime_type);
		lp_config_set_int(lc->config,key,"rate",pt->clock_rate);
		lp_config_set_int(lc->config,key,"enabled",linphone_core_payload_type_enabled(lc,pt));
		lp_config_set_string(lc->config,key,"recv_fmtp",pt->recv_fmtp);
		index++;
	}
	ms_list_free(lc->codecs_conf.audio_codecs);
	ms_list_free(lc->codecs_conf.video_codecs);
}

void ui_config_uninit(LinphoneCore* lc)
{
	if (lc->friends){
		ms_list_for_each(lc->friends,(void (*)(void *))linphone_friend_destroy);
		ms_list_free(lc->friends);
		lc->friends=NULL;
	}
}

/**
 * Returns the LpConfig object used to manage the storage (config) file.
 *
 * @ingroup misc
 * The application can use the LpConfig object to insert its own private 
 * sections and pairs of key=value in the configuration file.
 * 
**/
LpConfig *linphone_core_get_config(LinphoneCore *lc){
	return lc->config;
}

static void linphone_core_uninit(LinphoneCore *lc)
{
	if (lc->call){
		int i;
		linphone_core_terminate_call(lc,NULL);
		for(i=0;i<10;++i){
#ifndef WIN32
			usleep(50000);
#else
			Sleep(50);
#endif
			linphone_core_iterate(lc);
		}
	}
	if (lc->friends)
		ms_list_for_each(lc->friends,(void (*)(void *))linphone_friend_close_subscriptions);
	gstate_new_state(lc, GSTATE_POWER_SHUTDOWN, NULL);
#ifdef VIDEO_ENABLED
	if (lc->previewstream!=NULL){
		video_preview_stop(lc->previewstream);
		lc->previewstream=NULL;
	}
#endif
	/* save all config */
	net_config_uninit(lc);
	sip_config_uninit(lc);
	rtp_config_uninit(lc);
	sound_config_uninit(lc);
	video_config_uninit(lc);
	codecs_config_uninit(lc);
	ui_config_uninit(lc);
	if (lp_config_needs_commit(lc->config)) lp_config_sync(lc->config);
	lp_config_destroy(lc->config);
	lc->config = NULL; /* Mark the config as NULL to block further calls */
	sip_setup_unregister_all();

	ms_list_for_each(lc->call_logs,(void (*)(void*))linphone_call_log_destroy);
	lc->call_logs=ms_list_free(lc->call_logs);

	linphone_core_free_payload_types();

	ortp_exit();
	gstate_new_state(lc, GSTATE_POWER_OFF, NULL);
}

static void set_network_reachable(LinphoneCore* lc,bool_t isReachable){
	ms_message("Network state is now [%s]",isReachable?"UP":"DOWN");
	// second get the list of available proxies
	const MSList *elem=linphone_core_get_proxy_config_list(lc);
	for(;elem!=NULL;elem=elem->next){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		if (linphone_proxy_config_register_enabled(cfg) ) {
			if (!isReachable) {
				cfg->registered=0;
			}else{
				cfg->commit=TRUE;
			}
		}
	}
	lc->network_reachable=isReachable;
}

void linphone_core_set_network_reachable(LinphoneCore* lc,bool_t isReachable) {
	//first disable automatic mode
	if (lc->auto_net_state_mon) {
		ms_message("Disabling automatic network state monitoring");
		lc->auto_net_state_mon=FALSE;
	}
	set_network_reachable(lc,isReachable);
}

ortp_socket_t linphone_core_get_sip_socket(LinphoneCore *lc){
	return sal_get_socket(lc->sal);
}
/**
 * Destroys a LinphoneCore
 *
 * @ingroup initializing
**/
void linphone_core_destroy(LinphoneCore *lc){
	linphone_core_uninit(lc);
	ms_free(lc);
}

