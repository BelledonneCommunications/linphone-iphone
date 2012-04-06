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

#include <ortp/telephonyevents.h>
#include <ortp/zrtp.h>
#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/mseventqueue.h"
#include "mediastreamer2/msvolume.h"
#include "mediastreamer2/msequalizer.h"
#include "mediastreamer2/dtmfgen.h"

#ifdef INET6
#ifndef WIN32
#include <netdb.h>
#endif
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*#define UNSTANDART_GSM_11K 1*/

#define ROOT_CA_FILE PACKAGE_DATA_DIR "/linphone/rootca.pem"

static const char *liblinphone_version=LIBLINPHONE_VERSION;
static void set_network_reachable(LinphoneCore* lc,bool_t isReachable, time_t curtime);
static void linphone_core_run_hooks(LinphoneCore *lc);
static void linphone_core_free_hooks(LinphoneCore *lc);

#include "enum.h"
const char *linphone_core_get_nat_address_resolved(LinphoneCore *lc);
void linphone_core_get_local_ip(LinphoneCore *lc, const char *dest, char *result);
static void toggle_video_preview(LinphoneCore *lc, bool_t val);

/* relative path where is stored local ring*/
#define LOCAL_RING "rings/oldphone.wav"
/* same for remote ring (ringback)*/
#define REMOTE_RING "ringback.wav"
#define HOLD_MUSIC "rings/toy-mono.wav"


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
    cl->status=LinphoneCallAborted; /*default status*/
	return cl;
}

void call_logs_write_to_config_file(LinphoneCore *lc){
	MSList *elem;
	char logsection[32];
	int i;
	char *tmp;
	LpConfig *cfg=lc->config;

	if (linphone_core_get_global_state (lc)==LinphoneGlobalStartup) return;

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
		lp_config_set_float(cfg,logsection,"quality",cl->quality);
        lp_config_set_int(cfg,logsection,"video_enabled", cl->video_enabled);
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
			cl->quality=lp_config_get_float(cfg,logsection,"quality",-1);
            cl->video_enabled=lp_config_get_int(cfg,logsection,"video_enabled",0);
			lc->call_logs=ms_list_append(lc->call_logs,cl);
		}else break;
	}
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

/**
 * Returns RTP statistics computed locally regarding the call.
 *
**/
const rtp_stats_t *linphone_call_log_get_local_stats(const LinphoneCallLog *cl){
	return &cl->local_stats;
}

/**
 * Returns RTP statistics computed by remote end and sent back via RTCP.
 *
 * @note Not implemented yet.
**/
const rtp_stats_t *linphone_call_log_get_remote_stats(const LinphoneCallLog *cl){
	return &cl->remote_stats;
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
	LinphoneCall *call=linphone_core_get_current_call((LinphoneCore *)lc);
	if (call)  return linphone_call_get_duration(call);
	return -1;
}

const LinphoneAddress *linphone_core_get_current_call_remote_address(struct _LinphoneCore *lc){
	LinphoneCall *call=linphone_core_get_current_call(lc);
	if (call==NULL) return NULL;
	return linphone_call_get_remote_address(call);
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


static void net_config_read (LinphoneCore *lc)
{
	int tmp;
	const char *tmpstr;
	LpConfig *config=lc->config;

	lc->net_conf.nat_address_ip = NULL;
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
	int tmp;
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
	tmp=lp_config_get_int(lc->config,"sound","alsa_forced_rate",-1);
	if (tmp>0) ms_alsa_card_set_forced_sample_rate(tmp);
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
		ms_warning("%s does not exist",tmpbuf);
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

	linphone_core_set_play_file(lc,lp_config_get_string(lc->config,"sound","hold_music",PACKAGE_SOUND_DIR "/" HOLD_MUSIC));
	check_sound_device(lc);
	lc->sound_conf.latency=0;
#ifndef __ios 
    tmp=TRUE;
#else
    tmp=FALSE; /* on iOS we have builtin echo cancellation.*/
#endif
    tmp=lp_config_get_int(lc->config,"sound","echocancellation",tmp);
	linphone_core_enable_echo_cancellation(lc,tmp);
	linphone_core_enable_echo_limiter(lc,
		lp_config_get_int(lc->config,"sound","echolimiter",0));
	linphone_core_enable_agc(lc,
		lp_config_get_int(lc->config,"sound","agc",0));

	gain=lp_config_get_float(lc->config,"sound","playback_gain_db",0);
	linphone_core_set_playback_gain_db (lc,gain);

	linphone_core_set_remote_ringback_tone (lc,lp_config_get_string(lc->config,"sound","ringback_tone",NULL));
}

static void sip_config_read(LinphoneCore *lc)
{
	char *contact;
	const char *tmpstr;
	LCSipTransports tr;
	int i,tmp;
	int ipv6;

	tmp=lp_config_get_int(lc->config,"sip","use_info",0);
	linphone_core_set_use_info_for_dtmf(lc,tmp);

	if (lp_config_get_int(lc->config,"sip","use_session_timers",0)==1){
		sal_use_session_timers(lc->sal,200);
	}

	sal_use_rport(lc->sal,lp_config_get_int(lc->config,"sip","use_rport",1));
	sal_use_101(lc->sal,lp_config_get_int(lc->config,"sip","use_101",1));
	sal_reuse_authorization(lc->sal, lp_config_get_int(lc->config,"sip","reuse_authorization",0));

	tmp=lp_config_get_int(lc->config,"sip","use_rfc2833",0);
	linphone_core_set_use_rfc2833_for_dtmf(lc,tmp);

	ipv6=lp_config_get_int(lc->config,"sip","use_ipv6",-1);
	if (ipv6==-1){
		ipv6=0;
	}
	linphone_core_enable_ipv6(lc,ipv6);
	memset(&tr,0,sizeof(tr));
	if (lp_config_get_int(lc->config,"sip","sip_random_port",0)) {
		tr.udp_port=(0xDFF&+random())+1024;
	} else {
		tr.udp_port=lp_config_get_int(lc->config,"sip","sip_port",5060);
	}
	if (lp_config_get_int(lc->config,"sip","sip_tcp_random_port",0)) {
		tr.tcp_port=(0xDFF&+random())+1024;
	} else {
		tr.tcp_port=lp_config_get_int(lc->config,"sip","sip_tcp_port",0);
	}
	if (lp_config_get_int(lc->config,"sip","sip_tls_random_port",0)) {
		tr.tls_port=(0xDFF&+random())+1024;
	} else {
		tr.tls_port=lp_config_get_int(lc->config,"sip","sip_tls_port",0);
	}

#ifdef __linux
	sal_set_root_ca(lc->sal, lp_config_get_string(lc->config,"sip","root_ca", "/etc/ssl/certs"));
#else
	sal_set_root_ca(lc->sal, lp_config_get_string(lc->config,"sip","root_ca", ROOT_CA_FILE));
#endif
	linphone_core_verify_server_certificates(lc,lp_config_get_int(lc->config,"sip","verify_server_certs",TRUE));
	/*start listening on ports*/
 	linphone_core_set_sip_transports(lc,&tr);

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
	/*this is to filter out unsupported encryption schemes*/
	linphone_core_set_media_encryption(lc,linphone_core_get_media_encryption(lc));

	/*for tuning or test*/
	lc->sip_conf.sdp_200_ack=lp_config_get_int(lc->config,"sip","sdp_200_ack",0);
	lc->sip_conf.register_only_when_network_is_up=
		lp_config_get_int(lc->config,"sip","register_only_when_network_is_up",1);
	lc->sip_conf.ping_with_options=lp_config_get_int(lc->config,"sip","ping_with_options",1);
	lc->sip_conf.auto_net_state_mon=lp_config_get_int(lc->config,"sip","auto_net_state_mon",1);
	lc->sip_conf.keepalive_period=lp_config_get_int(lc->config,"sip","keepalive_period",10000);
	sal_set_keepalive_period(lc->sal,lc->sip_conf.keepalive_period);
	sal_use_one_matching_codec_policy(lc->sal,lp_config_get_int(lc->config,"sip","only_one_codec",0));
	sal_use_double_registrations(lc->sal,lp_config_get_int(lc->config,"sip","use_double_registrations",1));
}

static void rtp_config_read(LinphoneCore *lc)
{
	int port;
	int jitt_comp;
	int nortp_timeout;
	bool_t rtp_no_xmit_on_audio_mute;

	port=lp_config_get_int(lc->config,"rtp","audio_rtp_port",7078);
	linphone_core_set_audio_port(lc,port);

	port=lp_config_get_int(lc->config,"rtp","video_rtp_port",9078);
	if (port==0) port=9078;
	linphone_core_set_video_port(lc,port);

	jitt_comp=lp_config_get_int(lc->config,"rtp","audio_jitt_comp",60);
	linphone_core_set_audio_jittcomp(lc,jitt_comp);
	jitt_comp=lp_config_get_int(lc->config,"rtp","video_jitt_comp",60);
	if (jitt_comp==0) jitt_comp=60;
	lc->rtp_conf.video_jitt_comp=jitt_comp;
	nortp_timeout=lp_config_get_int(lc->config,"rtp","nortp_timeout",30);
	linphone_core_set_nortp_timeout(lc,nortp_timeout);
	rtp_no_xmit_on_audio_mute=lp_config_get_int(lc->config,"rtp","rtp_no_xmit_on_audio_mute",FALSE);
	linphone_core_set_rtp_no_xmit_on_audio_mute(lc,rtp_no_xmit_on_audio_mute);
}

static PayloadType * find_payload(RtpProfile *prof, const char *mime_type, int clock_rate, const char *recv_fmtp){
	PayloadType *candidate=NULL;
	int i;
	PayloadType *it;
	for(i=0;i<127;++i){
		it=rtp_profile_get_payload(prof,i);
		if (it!=NULL && strcasecmp(mime_type,it->mime_type)==0
			&& (clock_rate==it->clock_rate || clock_rate<=0) ){
			if ( (recv_fmtp && it->recv_fmtp && strstr(recv_fmtp,it->recv_fmtp)!=NULL) ||
				(recv_fmtp==NULL && it->recv_fmtp==NULL) ){
				/*exact match*/
				if (recv_fmtp) payload_type_set_recv_fmtp(it,recv_fmtp);
				return it;
			}else {
				if (candidate){
					if (it->recv_fmtp==NULL) candidate=it;
				}else candidate=it;
			}
		}
	}
	if (candidate && recv_fmtp){
		payload_type_set_recv_fmtp(candidate,recv_fmtp);
	}
	return candidate;
}

static bool_t get_codec(LpConfig *config, const char* type, int index, PayloadType **ret){
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

#define RANK_END 10000
static const char *codec_pref_order[]={
	"SILK",
	"speex",
	"iLBC",
	"amr",
	"gsm",
	"pcmu",
	"pcma",
	"VP8",
	"H264",
	"MP4V-ES",
	"H263-1998",
	NULL,
};

static int find_codec_rank(const char *mime){
	int i;
	for(i=0;codec_pref_order[i]!=NULL;++i){
		if (strcasecmp(codec_pref_order[i],mime)==0)
			return i;
	}
	return RANK_END;
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
					/*unranked codecs are disabled by default*/
					if (find_codec_rank(pt->mime_type)!=RANK_END){
						payload_type_set_flag(pt,PAYLOAD_TYPE_ENABLED);
					}
					ms_message("Adding new codec %s/%i with fmtp %s",
					    pt->mime_type,pt->clock_rate,pt->recv_fmtp ? pt->recv_fmtp : "");
					l=ms_list_insert_sorted(l,pt,(int (*)(const void *, const void *))codec_compare);
				}
			}
		}
	}
	return l;
}

static MSList *codec_append_if_new(MSList *l, PayloadType *pt){
	MSList *elem;
	for (elem=l;elem!=NULL;elem=elem->next){
		PayloadType *ept=(PayloadType*)elem->data;
		if (pt==ept)
			return l;
	}
	l=ms_list_append(l,pt);
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
			}else audio_codecs=codec_append_if_new(audio_codecs,pt);
		}
	}
	audio_codecs=add_missing_codecs(SalAudio,audio_codecs);
	for (i=0;get_codec(lc->config,"video_codec",i,&pt);i++){
		if (pt){
			if (!ms_filter_codec_supported(pt->mime_type)){
				ms_warning("Codec %s is not supported by mediastreamer2, removed.",pt->mime_type);
			}else video_codecs=codec_append_if_new(video_codecs,(void *)pt);
		}
	}
	video_codecs=add_missing_codecs(SalVideo,video_codecs);
	linphone_core_set_audio_codecs(lc,audio_codecs);
	linphone_core_set_video_codecs(lc,video_codecs);
	linphone_core_update_allocated_audio_bandwidth(lc);
}

static void build_video_devices_table(LinphoneCore *lc){
	const MSList *elem;
	int i;
	int ndev;
	const char **devices;
	if (lc->video_conf.cams)
		ms_free(lc->video_conf.cams);
	/* retrieve all video devices */
	elem=ms_web_cam_manager_get_list(ms_web_cam_manager_get());
	ndev=ms_list_size(elem);
	devices=ms_malloc((ndev+1)*sizeof(const char *));
	for (i=0;elem!=NULL;elem=elem->next,i++){
		devices[i]=ms_web_cam_get_string_id((MSWebCam *)elem->data);
	}
	devices[ndev]=NULL;
	lc->video_conf.cams=devices;
}

static void video_config_read(LinphoneCore *lc){
#ifdef VIDEO_ENABLED
	int capture, display, self_view;
#endif
	const char *str;	
	LinphoneVideoPolicy vpol;

	build_video_devices_table(lc);

	str=lp_config_get_string(lc->config,"video","device",NULL);
	if (str && str[0]==0) str=NULL;
	linphone_core_set_video_device(lc,str);

	linphone_core_set_preferred_video_size_by_name(lc,
		lp_config_get_string(lc->config,"video","size","cif"));

#ifdef VIDEO_ENABLED
	capture=lp_config_get_int(lc->config,"video","capture",1);
	display=lp_config_get_int(lc->config,"video","display",1);
	self_view=lp_config_get_int(lc->config,"video","self_view",1);
	vpol.automatically_initiate=lp_config_get_int(lc->config,"video","automatically_initiate",1);
	vpol.automatically_accept=lp_config_get_int(lc->config,"video","automatically_accept",1);
	lc->video_conf.displaytype=lp_config_get_string(lc->config,"video","displaytype",NULL);
	if(lc->video_conf.displaytype)
		ms_message("we are using a specific display:%s\n",lc->video_conf.displaytype);

	linphone_core_enable_video(lc,capture,display);
	linphone_core_enable_self_view(lc,self_view);
	linphone_core_set_video_policy(lc,&vpol);
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

bool_t linphone_core_tunnel_available(void){
#ifdef TUNNEL_ENABLED
	return TRUE;
#else
	return FALSE;
#endif
}

/**
 * Enable adaptive rate control (experimental feature, audio-only).
 *
 * Adaptive rate control consists in using RTCP feedback provided information to dynamically
 * control the output bitrate of the encoders, so that we can adapt to the network conditions and
 * available bandwidth.
**/
void linphone_core_enable_adaptive_rate_control(LinphoneCore *lc, bool_t enabled){
	lp_config_set_int(lc->config,"net","adaptive_rate_control",(int)enabled);
}

/**
 * Returns whether adaptive rate control is enabled.
 *
 * See linphone_core_enable_adaptive_rate_control().
**/
bool_t linphone_core_adaptive_rate_control_enabled(const LinphoneCore *lc){
	return lp_config_get_int(lc->config,"net","adaptive_rate_control",TRUE);
}

bool_t linphone_core_rtcp_enabled(const LinphoneCore *lc){
	return lp_config_get_int(lc->config,"rtp","rtcp_enabled",TRUE);
}

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
	if (linphone_core_ready(lc)) lp_config_set_int(lc->config,"net","download_bw",bw);
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
	if (linphone_core_ready(lc)) lp_config_set_int(lc->config,"net","upload_bw",bw);
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
 * Set audio packetization time linphone expects to receive from peer
 */
void linphone_core_set_download_ptime(LinphoneCore *lc, int ptime) {
	lc->net_conf.down_ptime=ptime;
}

/**
 * Get audio packetization time linphone expects to receive from peer
 */
int linphone_core_get_download_ptime(LinphoneCore *lc) {
	return lc->net_conf.down_ptime;
}

/**
 * Set audio packetization time linphone will send (in absence of requirement from peer)
 * A value of 0 stands for the current codec default packetization time.
 *
**/
void linphone_core_set_upload_ptime(LinphoneCore *lc, int ptime){
	lp_config_set_int(lc->config,"rtp","upload_ptime",ptime);
}

/**
 * Set audio packetization time linphone will send (in absence of requirement from peer)
 * A value of 0 stands for the current codec default packetization time.
 *
**/
int linphone_core_get_upload_ptime(LinphoneCore *lc){
	return lp_config_get_int(lc->config,"rtp","upload_ptime",0);
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

static void linphone_core_assign_payload_type(LinphoneCore *lc, PayloadType *const_pt, int number, const char *recv_fmtp){
	PayloadType *pt;
	pt=payload_type_clone(const_pt);
	if (number==-1){
		/*look for a free number */
		MSList *elem;
		int i;
		for(i=lc->dyn_pt;i<=127;++i){
			bool_t already_assigned=FALSE;
			for(elem=lc->payload_types;elem!=NULL;elem=elem->next){
				PayloadType *it=(PayloadType*)elem->data;
				if (payload_type_get_number(it)==i){
					already_assigned=TRUE;
					break;
				}
			}
			if (!already_assigned){
				number=i;
				lc->dyn_pt=i+1;
				break;
			}
		}
		if (number==-1){
			ms_fatal("FIXME: too many codecs, no more free numbers.");
		}
	}
	ms_message("assigning %s/%i payload type number %i",pt->mime_type,pt->clock_rate,number);
	payload_type_set_number(pt,number);
	if (recv_fmtp!=NULL) payload_type_set_recv_fmtp(pt,recv_fmtp);
	rtp_profile_set_payload(&av_profile,number,pt);
	lc->payload_types=ms_list_append(lc->payload_types,pt);
}

static void linphone_core_handle_static_payloads(LinphoneCore *lc){
	RtpProfile *prof=&av_profile;
	int i;
	for(i=0;i<128;++i){
		PayloadType *pt=rtp_profile_get_payload(prof,i);
		if (pt){
			if (payload_type_get_number(pt)!=i){
				linphone_core_assign_payload_type(lc,pt,i,NULL);
			}
		}
	}
}

static void linphone_core_free_payload_types(LinphoneCore *lc){
	ms_list_for_each(lc->payload_types,(void (*)(void*))payload_type_destroy);
	ms_list_free(lc->payload_types);
	lc->payload_types=NULL;
}

void linphone_core_set_state(LinphoneCore *lc, LinphoneGlobalState gstate, const char *message){
	lc->state=gstate;
	if (lc->vtable.global_state_changed){
		lc->vtable.global_state_changed(lc,gstate,message);
	}
}
static void misc_config_read (LinphoneCore *lc) {
	LpConfig *config=lc->config;
    lc->max_call_logs=lp_config_get_int(config,"misc","history_max_size",15);
    lc->max_calls=lp_config_get_int(config,"misc","max_calls",NB_MAX_CALLS);
}




static void linphone_core_init (LinphoneCore * lc, const LinphoneCoreVTable *vtable, const char *config_path,
    const char *factory_config_path, void * userdata)
{
	memset (lc, 0, sizeof (LinphoneCore));
	lc->data=userdata;
	lc->ringstream_autorelease=TRUE;

	memcpy(&lc->vtable,vtable,sizeof(LinphoneCoreVTable));

	linphone_core_set_state(lc,LinphoneGlobalStartup,"Starting up");
	ortp_init();
	lc->dyn_pt=96;
	linphone_core_assign_payload_type(lc,&payload_type_pcmu8000,0,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_gsm,3,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_pcma8000,8,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_speex_nb,110,"vbr=on");
	linphone_core_assign_payload_type(lc,&payload_type_speex_wb,111,"vbr=on");
	linphone_core_assign_payload_type(lc,&payload_type_speex_uwb,112,"vbr=on");
	linphone_core_assign_payload_type(lc,&payload_type_telephone_event,101,"0-11");
	linphone_core_assign_payload_type(lc,&payload_type_g722,9,NULL);

#if defined(ANDROID) || defined (__IPHONE_OS_VERSION_MIN_REQUIRED)
	/*shorten the DNS lookup time and send more retransmissions on mobiles:
	 - to workaround potential packet losses
	 - to avoid hanging for 30 seconds when the network doesn't work despite the phone thinks it does.
	 */
	_linphone_core_configure_resolver();
#endif

#ifdef ENABLE_NONSTANDARD_GSM
	{
		PayloadType *pt;
		pt=payload_type_clone(&payload_type_gsm);
		pt->clock_rate=11025;
		linphone_core_assign_payload_type(lc,pt,-1,NULL);
		pt->clock_rate=22050;
		linphone_core_assign_payload_type(lc,pt,-1,NULL);
		payload_type_destroy(pt);
	}
#endif

#ifdef VIDEO_ENABLED
	linphone_core_assign_payload_type(lc,&payload_type_h263,34,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_theora,97,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_h263_1998,98,"CIF=1;QCIF=1");
	linphone_core_assign_payload_type(lc,&payload_type_mp4v,99,"profile-level-id=3");
	linphone_core_assign_payload_type(lc,&payload_type_h264,102,"profile-level-id=428014");
	linphone_core_assign_payload_type(lc,&payload_type_vp8,103,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_x_snow,-1,NULL);
	/* due to limited space in SDP, we have to disable this h264 line which is normally no more necessary */
	/* linphone_core_assign_payload_type(&payload_type_h264,-1,"packetization-mode=1;profile-level-id=428014");*/
#endif

	/*add all payload type for which we don't care about the number */
	linphone_core_assign_payload_type(lc,&payload_type_ilbc,-1,"mode=30");
	linphone_core_assign_payload_type(lc,&payload_type_amr,-1,"octet-align=1");
	linphone_core_assign_payload_type(lc,&payload_type_amrwb,-1,"octet-align=1");
	linphone_core_assign_payload_type(lc,&payload_type_lpc1015,-1,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_g726_16,-1,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_g726_24,-1,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_g726_32,-1,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_g726_40,-1,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_aal2_g726_16,-1,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_aal2_g726_24,-1,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_aal2_g726_32,-1,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_aal2_g726_40,-1,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_silk_nb,-1,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_silk_mb,-1,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_silk_wb,-1,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_silk_swb,-1,NULL);
	linphone_core_assign_payload_type(lc,&payload_type_g729,18,"annexb=no");
	linphone_core_handle_static_payloads(lc);
	
	ms_init();
	/* create a mediastreamer2 event queue and set it as global */
	/* This allows to run event's callback in linphone_core_iterate() */
	lc->msevq=ms_event_queue_new();
	ms_set_global_event_queue(lc->msevq);

	lc->config=lp_config_new(config_path);
	if (factory_config_path)
		lp_config_read_file(lc->config,factory_config_path);

	lc->sal=sal_init();
	sal_set_user_pointer(lc->sal,lc);
	sal_set_callbacks(lc->sal,&linphone_sal_callbacks);

	sip_setup_register_all();
	sound_config_read(lc);
	net_config_read(lc);
	rtp_config_read(lc);
	codecs_config_read(lc);
	sip_config_read(lc); /* this will start eXosip*/
	video_config_read(lc);
	//autoreplier_config_init(&lc->autoreplier_conf);
	lc->presence_mode=LinphoneStatusOnline;
	misc_config_read(lc);
	ui_config_read(lc);
#ifdef TUNNEL_ENABLED
	lc->tunnel=linphone_core_tunnel_new(lc);
	if (lc->tunnel) linphone_tunnel_configure(lc->tunnel);
#endif
	if (lc->vtable.display_status)
		lc->vtable.display_status(lc,_("Ready"));
	lc->auto_net_state_mon=lc->sip_conf.auto_net_state_mon;
	linphone_core_set_state(lc,LinphoneGlobalOn,"Ready");
}

/**
 * Instanciates a LinphoneCore object.
 * @ingroup initializing
 *
 * The LinphoneCore object is the primary handle for doing all phone actions.
 * It should be unique within your application.
 * @param vtable a LinphoneCoreVTable structure holding your application callbacks
 * @param config_path a path to a config file. If it does not exists it will be created.
 *        The config file is used to store all settings, call logs, friends, proxies... so that all these settings
 *	       become persistent over the life of the LinphoneCore object.
 *	       It is allowed to set a NULL config file. In that case LinphoneCore will not store any settings.
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
	const char *ip;
	if (linphone_core_get_firewall_policy(lc)==LinphonePolicyUseNatAddress
	    && (ip=linphone_core_get_nat_address_resolved(lc))!=NULL){
		strncpy(result,ip,LINPHONE_IPADDR_SIZE);
		return;
	}
	if (linphone_core_get_local_ip_for(lc->sip_conf.ipv6_enabled ? AF_INET6 : AF_INET,dest,result)==0)
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
	linphone_address_set_port_int(url,linphone_core_get_sip_port (lc));
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

bool_t linphone_core_get_rtp_no_xmit_on_audio_mute(const LinphoneCore *lc){
	return lc->rtp_conf.rtp_no_xmit_on_audio_mute;
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

void linphone_core_set_rtp_no_xmit_on_audio_mute(LinphoneCore *lc,bool_t rtp_no_xmit_on_audio_mute){
	lc->rtp_conf.rtp_no_xmit_on_audio_mute=rtp_no_xmit_on_audio_mute;
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
 * Deprecated: use linphone_core_get_sip_transports() instead.
 * @ingroup network_parameters
**/
int linphone_core_get_sip_port(LinphoneCore *lc)
{
	LCSipTransports *tr=&lc->sip_conf.transports;
	return tr->udp_port>0 ? tr->udp_port : (tr->tcp_port > 0 ? tr->tcp_port : tr->tls_port);
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

static void transport_error(LinphoneCore *lc, const char* transport, int port){
	char *msg=ortp_strdup_printf("Could not start %s transport on port %i, maybe this port is already used.",transport,port);
	ms_warning("%s",msg);
	if (lc->vtable.display_warning)
		lc->vtable.display_warning(lc,msg);
	ms_free(msg);
}

static bool_t transports_unchanged(const LCSipTransports * tr1, const LCSipTransports * tr2){
	return
		tr2->udp_port==tr1->udp_port &&
		tr2->tcp_port==tr1->tcp_port &&
		tr2->dtls_port==tr1->dtls_port &&
		tr2->tls_port==tr1->tls_port;
}

static int apply_transports(LinphoneCore *lc){
	Sal *sal=lc->sal;
	const char *anyaddr;
	LCSipTransports *tr=&lc->sip_conf.transports;

	/*first of all invalidate all current registrations so that we can register again with new transports*/
	__linphone_core_invalidate_registers(lc);
	
	if (lc->sip_conf.ipv6_enabled)
		anyaddr="::0";
	else
		anyaddr="0.0.0.0";

	sal_unlisten_ports(sal);
	if (tr->udp_port>0){
		if (sal_listen_port (sal,anyaddr,tr->udp_port,SalTransportUDP,FALSE)!=0){
			transport_error(lc,"udp",tr->udp_port);
			return -1;
		}
	}
	if (tr->tcp_port>0){
		if (sal_listen_port (sal,anyaddr,tr->tcp_port,SalTransportTCP,FALSE)!=0){
			transport_error(lc,"tcp",tr->tcp_port);
		}
	}
	if (tr->tls_port>0){
		if (sal_listen_port (sal,anyaddr,tr->tls_port,SalTransportTLS,TRUE)!=0){
			transport_error(lc,"tls",tr->tls_port);
		}
	}
	apply_user_agent(lc);
	return 0;
}

/**
 * Sets the ports to be used for each of transport (UDP or TCP)
 *
 * A zero value port for a given transport means the transport
 * is not used.
 *
 * @ingroup network_parameters
**/
int linphone_core_set_sip_transports(LinphoneCore *lc, const LCSipTransports * tr){

	if (transports_unchanged(tr,&lc->sip_conf.transports))
		return 0;
	memcpy(&lc->sip_conf.transports,tr,sizeof(*tr));

	if (linphone_core_ready(lc)){
		lp_config_set_int(lc->config,"sip","sip_port",tr->udp_port);
		lp_config_set_int(lc->config,"sip","sip_tcp_port",tr->tcp_port);
		lp_config_set_int(lc->config,"sip","sip_tls_port",tr->tls_port);
	}

	if (lc->sal==NULL) return 0;
	return apply_transports(lc);
}

/**
 * Retrieves the ports used for each transport (udp, tcp).
 * A zero value port for a given transport means the transport
 * is not used.
 * @ingroup network_parameters
**/
int linphone_core_get_sip_transports(LinphoneCore *lc, LCSipTransports *tr){
	memcpy(tr,&lc->sip_conf.transports,sizeof(*tr));
	return 0;
}

/**
 * Sets the UDP port to be used by SIP.
 *
 * Deprecated: use linphone_core_set_sip_transports() instead.
 * @ingroup network_parameters
**/
void linphone_core_set_sip_port(LinphoneCore *lc,int port)
{
	LCSipTransports tr;
	memset(&tr,0,sizeof(tr));
	tr.udp_port=port;
	linphone_core_set_sip_transports (lc,&tr);
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
			apply_transports(lc);
		}
	}
}


static void monitor_network_state(LinphoneCore *lc, time_t curtime){
	static time_t last_check=0;
	static bool_t last_status=FALSE;
	char result[LINPHONE_IPADDR_SIZE];
	bool_t new_status=last_status;

	/* only do the network up checking every five seconds */
	if (last_check==0 || (curtime-last_check)>=5){
		linphone_core_get_local_ip_for(lc->sip_conf.ipv6_enabled ? AF_INET6 : AF_INET,NULL,result);
		if (strcmp(result,"::1")!=0 && strcmp(result,"127.0.0.1")!=0){
			new_status=TRUE;
		}else new_status=FALSE;
		last_check=curtime;
		if (new_status!=last_status) {
			if (new_status){
				ms_message("New local ip address is %s",result);
			}
			set_network_reachable(lc,new_status, curtime);
			last_status=new_status;
		}
	}
}

static void proxy_update(LinphoneCore *lc){
	MSList *elem,*next;
	ms_list_for_each(lc->sip_conf.proxies,(void (*)(void*))&linphone_proxy_config_update);
	for(elem=lc->sip_conf.deleted_proxies;elem!=NULL;elem=next){
		LinphoneProxyConfig* cfg = (LinphoneProxyConfig*)elem->data;
		next=elem->next;
		if (ms_time(NULL) - cfg->deletion_date > 5) {
			lc->sip_conf.deleted_proxies =ms_list_remove_link(lc->sip_conf.deleted_proxies,elem);
			ms_message("clearing proxy config for [%s]",linphone_proxy_config_get_addr(cfg));
			linphone_proxy_config_destroy(cfg);
		}
	}
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
 * The application MUST call this function periodically, in its main loop.
 * Be careful that this function must be called from the same thread as
 * other liblinphone methods. If it is not the case make sure all liblinphone calls are
 * serialized with a mutex.
**/
void linphone_core_iterate(LinphoneCore *lc){
	MSList *calls;
	LinphoneCall *call;
	time_t curtime=time(NULL);
	int elapsed;
	bool_t one_second_elapsed=FALSE;

	if (curtime-lc->prevtime>=1){
		lc->prevtime=curtime;
		one_second_elapsed=TRUE;
	}

	if (lc->ecc!=NULL){
		LinphoneEcCalibratorStatus ecs=ec_calibrator_get_status(lc->ecc);
		if (ecs!=LinphoneEcCalibratorInProgress){
			if (lc->ecc->cb)
				lc->ecc->cb(lc,ecs,lc->ecc->delay,lc->ecc->cb_data);
			if (ecs==LinphoneEcCalibratorDone){
				int len=lp_config_get_int(lc->config,"sound","ec_tail_len",0);
				lp_config_set_int(lc->config, "sound", "ec_delay",MAX(lc->ecc->delay-(len/2),0));
			}
			ec_calibrator_destroy(lc->ecc);
			lc->ecc=NULL;
		}
	}

	if (lc->preview_finished){
		lc->preview_finished=0;
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
		lc_callback_obj_invoke(&lc->preview_finished_cb,lc);
	}

	if (lc->ringstream && lc->ringstream_autorelease && lc->dmfs_playing_start_time!=0
	    && (curtime-lc->dmfs_playing_start_time)>5){
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
		lc->dmfs_playing_start_time=0;
	}

	sal_iterate(lc->sal);
	if (lc->msevq) ms_event_queue_pump(lc->msevq);
	if (lc->auto_net_state_mon) monitor_network_state(lc,curtime);

	proxy_update(lc);

	//we have to iterate for each call
	calls= lc->calls;
	while(calls!= NULL){
		call = (LinphoneCall *)calls->data;
		 /* get immediately a reference to next one in case the one
		 we are going to examine is destroy and removed during
		 linphone_core_start_invite() */
		calls=calls->next;
		linphone_call_background_tasks(call,one_second_elapsed);
		if (call->state==LinphoneCallOutgoingInit && (curtime-call->start_time>=2)){
			/*start the call even if the OPTIONS reply did not arrive*/
			linphone_core_start_invite(lc,call,NULL);
		}
		if (call->state==LinphoneCallIncomingReceived){
			elapsed=curtime-call->start_time;
			ms_message("incoming call ringing for %i seconds",elapsed);
			if (elapsed>lc->sip_conf.inc_timeout){
				call->log->status=LinphoneCallMissed;
				linphone_core_terminate_call(lc,call);
			}
		}
	}
		
	if (linphone_core_video_preview_enabled(lc)){
		if (lc->previewstream==NULL && lc->calls==NULL)
			toggle_video_preview(lc,TRUE);
#ifdef VIDEO_ENABLED
		if (lc->previewstream) video_stream_iterate(lc->previewstream);
#endif
	}else{
		if (lc->previewstream!=NULL)
			toggle_video_preview(lc,FALSE);
	}

	linphone_core_run_hooks(lc);
	linphone_core_do_plugin_tasks(lc);

	if (lc->initial_subscribes_sent==FALSE && lc->netup_time!=0 &&
	    (curtime-lc->netup_time)>3){
		linphone_core_send_initial_subscribes(lc);
		lc->initial_subscribes_sent=TRUE;
	}

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
		if (lc->vtable.display_status!=NULL)
			lc->vtable.display_status(lc,_("Looking for telephone number destination..."));
		if (enum_lookup(enum_domain,&enumres)<0){
			if (lc->vtable.display_status!=NULL)
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
 * @ingroup proxies
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

void linphone_core_start_refered_call(LinphoneCore *lc, LinphoneCall *call){
	if (call->refer_pending){
		LinphoneCallParams *cp=linphone_core_create_default_call_parameters(lc);
		LinphoneCall *newcall;
		cp->has_video &= !!lc->video_policy.automatically_initiate;
		cp->referer=call;
		ms_message("Starting new call to refered address %s",call->refer_to);
		call->refer_pending=FALSE;
		newcall=linphone_core_invite_with_params(lc,call->refer_to,cp);
		linphone_call_params_destroy(cp);
		if (newcall) linphone_core_notify_refer_state(lc,call,newcall);
	}
}

void linphone_core_notify_refer_state(LinphoneCore *lc, LinphoneCall *referer, LinphoneCall *newcall){
	if (referer->op!=NULL){
		sal_call_notify_refer_state(referer->op,newcall ? newcall->op : NULL);
	}
}

LinphoneProxyConfig * linphone_core_lookup_known_proxy(LinphoneCore *lc, const LinphoneAddress *uri){
	const MSList *elem;
	LinphoneProxyConfig *found_cfg=NULL;
	LinphoneProxyConfig *default_cfg=lc->default_proxy;

	/*always prefer the default proxy if it is matching the destination uri*/
	if (default_cfg){
		const char *domain=linphone_proxy_config_get_domain(default_cfg);
		if (strcmp(domain,linphone_address_get_domain(uri))==0)
			return default_cfg;
	}

	/*otherwise iterate through the other proxy config and return the first matching*/
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

const char *linphone_core_find_best_identity(LinphoneCore *lc, const LinphoneAddress *to, const char **route){
	LinphoneProxyConfig *cfg=linphone_core_lookup_known_proxy(lc,to);
	if (cfg==NULL)
		linphone_core_get_default_proxy (lc,&cfg);
	if (cfg!=NULL){
		if (route) *route=linphone_proxy_config_get_route(cfg);
		return linphone_proxy_config_get_identity (cfg);
	}
	return linphone_core_get_primary_contact (lc);
}

static char *get_fixed_contact(LinphoneCore *lc, LinphoneCall *call , LinphoneProxyConfig *dest_proxy){
	LinphoneAddress *ctt;
	const char *localip=call->localip;

	/* first use user's supplied ip address if asked*/
	if (linphone_core_get_firewall_policy(lc)==LinphonePolicyUseNatAddress){
		ctt=linphone_core_get_primary_contact_parsed(lc);
		return ms_strdup_printf("sip:%s@%s",linphone_address_get_username(ctt),
		    	linphone_core_get_nat_address_resolved(lc));
	}

	/* if already choosed, don't change it */
	if (call->op && sal_op_get_contact(call->op)!=NULL){
		return NULL;
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

	/*if using a proxy, use the contact address as guessed with the REGISTERs*/
	if (dest_proxy && dest_proxy->op){
		const char *fixed_contact=sal_op_get_contact(dest_proxy->op);
		if (fixed_contact) {
			ms_message("Contact has been fixed using proxy to %s",fixed_contact);
			return ms_strdup(fixed_contact);
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

	//TODO : should probably not be done here
	linphone_call_init_media_streams(call);
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
	if (lc->vtable.display_status!=NULL)
		lc->vtable.display_status(lc,barmsg);
	ms_free(barmsg);

	if (err<0){
		if (lc->vtable.display_status!=NULL)
			lc->vtable.display_status(lc,_("Could not call"));
		linphone_call_stop_media_streams(call);
		linphone_call_set_state(call,LinphoneCallError,"Call failed");
	}else {
		linphone_call_set_state(call,LinphoneCallOutgoingProgress,"Outgoing call in progress");
	}
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
 *
 * The application doesn't own a reference to the returned LinphoneCall object.
 * Use linphone_call_ref() to safely keep the LinphoneCall pointer valid within your application.
 *
 * @return a LinphoneCall object or NULL in case of failure
**/
LinphoneCall * linphone_core_invite(LinphoneCore *lc, const char *url){
	LinphoneCall *call;
	LinphoneCallParams *p=linphone_core_create_default_call_parameters (lc);
	p->has_video &= !!lc->video_policy.automatically_initiate;
	call=linphone_core_invite_with_params(lc,url,p);
	linphone_call_params_destroy(p);
	return call;
}


/**
 * Initiates an outgoing call according to supplied call parameters
 *
 * @ingroup call_control
 * @param lc the LinphoneCore object
 * @param url the destination of the call (sip address, or phone number).
 * @param p call parameters
 *
 * The application doesn't own a reference to the returned LinphoneCall object.
 * Use linphone_call_ref() to safely keep the LinphoneCall pointer valid within your application.
 *
 * @return a LinphoneCall object or NULL in case of failure
**/
LinphoneCall * linphone_core_invite_with_params(LinphoneCore *lc, const char *url, const LinphoneCallParams *p){
	LinphoneAddress *addr=linphone_core_interpret_url(lc,url);
	if (addr){
		LinphoneCall *call;
		call=linphone_core_invite_address_with_params(lc,addr,p);
		linphone_address_destroy(addr);
		return call;
	}
	return NULL;
}

/**
 * Initiates an outgoing call given a destination LinphoneAddress
 *
 * @ingroup call_control
 * @param lc the LinphoneCore object
 * @param addr the destination of the call (sip address).
 *
 * The LinphoneAddress can be constructed directly using linphone_address_new(), or
 * created by linphone_core_interpret_url().
 * The application doesn't own a reference to the returned LinphoneCall object.
 * Use linphone_call_ref() to safely keep the LinphoneCall pointer valid within your application.
 *
 * @return a LinphoneCall object or NULL in case of failure
**/
LinphoneCall * linphone_core_invite_address(LinphoneCore *lc, const LinphoneAddress *addr){
	LinphoneCall *call;
	LinphoneCallParams *p=linphone_core_create_default_call_parameters(lc);
	p->has_video &= !!lc->video_policy.automatically_initiate;
	call=linphone_core_invite_address_with_params (lc,addr,p);
	linphone_call_params_destroy(p);
	return call;
}


/**
 * Initiates an outgoing call given a destination LinphoneAddress
 *
 * @ingroup call_control
 * @param lc the LinphoneCore object
 * @param addr the destination of the call (sip address).
	@param params call parameters
 *
 * The LinphoneAddress can be constructed directly using linphone_address_new(), or
 * created by linphone_core_interpret_url().
 * The application doesn't own a reference to the returned LinphoneCall object.
 * Use linphone_call_ref() to safely keep the LinphoneCall pointer valid within your application.
 *
 * @return a LinphoneCall object or NULL in case of failure
**/
LinphoneCall * linphone_core_invite_address_with_params(LinphoneCore *lc, const LinphoneAddress *addr, const LinphoneCallParams *params)
{
	const char *route=NULL;
	const char *from=NULL;
	LinphoneProxyConfig *proxy=NULL;
	LinphoneAddress *parsed_url2=NULL;
	char *real_url=NULL;
	LinphoneProxyConfig *dest_proxy=NULL;
	LinphoneCall *call;

	linphone_core_preempt_sound_resources(lc);
	
	if(!linphone_core_can_we_add_call(lc)){
		if (lc->vtable.display_warning)
			lc->vtable.display_warning(lc,_("Sorry, we have reached the maximum number of simultaneous calls"));
		return NULL;
	}
	linphone_core_get_default_proxy(lc,&proxy);
	route=linphone_core_get_route(lc);

	real_url=linphone_address_as_string(addr);
	dest_proxy=linphone_core_lookup_known_proxy(lc,addr);

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

	call=linphone_call_new_outgoing(lc,parsed_url2,linphone_address_clone(addr),params);
	sal_op_set_route(call->op,route);

	if(linphone_core_add_call(lc,call)!= 0)
	{
		ms_warning("we had a problem in adding the call into the invite ... weird");
		linphone_call_unref(call);
		return NULL;
	}
	/* this call becomes now the current one*/
	lc->current_call=call;
	linphone_call_set_state (call,LinphoneCallOutgoingInit,"Starting outgoing call");
	if (dest_proxy!=NULL || lc->sip_conf.ping_with_options==FALSE){
		linphone_core_start_invite(lc,call,dest_proxy);
	}else{
		/*defer the start of the call after the OPTIONS ping*/
		call->ping_op=sal_op_new(lc->sal);
		sal_ping(call->ping_op,from,real_url);
		sal_op_set_user_pointer(call->ping_op,call);
		call->start_time=time(NULL);
	}

	if (real_url!=NULL) ms_free(real_url);
	return call;
}

/**
 * Performs a simple call transfer to the specified destination.
 *
 * The remote endpoint is expected to issue a new call to the specified destination.
 * The current call remains active and thus can be later paused or terminated.
**/
int linphone_core_transfer_call(LinphoneCore *lc, LinphoneCall *call, const char *url)
{
	char *real_url=NULL;
	LinphoneAddress *real_parsed_url=linphone_core_interpret_url(lc,url);

	if (!real_parsed_url){
		/* bad url */
		return -1;
	}
	if (call==NULL){
		ms_warning("No established call to refer.");
		return -1;
	}
	//lc->call=NULL; //Do not do that you will lose the call afterward . . .
	real_url=linphone_address_as_string (real_parsed_url);
	sal_call_refer(call->op,real_url);
	ms_free(real_url);
	linphone_address_destroy(real_parsed_url);
	linphone_call_set_transfer_state(call, LinphoneCallOutgoingInit);
	return 0;
}

/**
 * Transfer a call to destination of another running call. This is used for "attended transfer" scenarios.
 * @param lc linphone core object
 * @param call a running call you want to transfer
 * @param dest a running call whose remote person will receive the transfer
 *
 * The transfered call is supposed to be in paused state, so that it is able to accept the transfer immediately.
 * The destination call is a call previously established to introduce the transfered person.
 * This method will send a transfer request to the transfered person. The phone of the transfered is then
 * expected to automatically call to the destination of the transfer. The receiver of the transfer will then automatically
 * close the call with us (the 'dest' call).
**/
int linphone_core_transfer_call_to_another(LinphoneCore *lc, LinphoneCall *call, LinphoneCall *dest){
	int result = sal_call_refer_with_replaces (call->op,dest->op);
	linphone_call_set_transfer_state(call, LinphoneCallOutgoingInit);
	return result;
}

bool_t linphone_core_inc_invite_pending(LinphoneCore*lc){
	LinphoneCall *call = linphone_core_get_current_call(lc);
	if(call != NULL)
	{
		if(call->dir==LinphoneCallIncoming
			&& (call->state == LinphoneCallIncomingReceived || call->state ==  LinphoneCallIncomingEarlyMedia))
			return TRUE;
	}
	return FALSE;
}

/**
 * @ingroup call_control
 * Updates a running call according to supplied call parameters or parameters changed in the LinphoneCore.
 *
 * In this version this is limited to the following use cases:
 * - setting up/down the video stream according to the video parameter of the LinphoneCallParams (see linphone_call_params_enable_video() ).
 * - changing the size of the transmitted video after calling linphone_core_set_preferred_video_size()
 *
 * In case no changes are requested through the LinphoneCallParams argument, then this argument can be omitted and set to NULL.
 * @param lc the core
 * @param call the call to be updated
 * @param params the new call parameters to use. (may be NULL)
 * @return 0 if successful, -1 otherwise.
**/
int linphone_core_update_call(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params){
	int err=0;
	if (params!=NULL){
		const char *subject;
		call->params=*params;
		call->camera_active=call->params.has_video;
		update_local_media_description(lc,call);
		
		if (params->in_conference){
			subject="Conference";
		}else{
			subject="Media change";
		}
		if (lc->vtable.display_status)
			lc->vtable.display_status(lc,_("Modifying call parameters..."));
		sal_call_set_local_media_description (call->op,call->localdesc);
		err=sal_call_update(call->op,subject);
	}else{
#ifdef VIDEO_ENABLED
		if (call->videostream!=NULL){
			video_stream_set_sent_video_size(call->videostream,linphone_core_get_preferred_video_size(lc));
			if (call->camera_active && call->videostream->cam!=lc->video_conf.device){
				video_stream_change_camera(call->videostream,lc->video_conf.device);
			}else video_stream_update_video_params(call->videostream);
		}
#endif
	}

	return err;
}

/**
 * @ingroup call_control
 * When receiving a #LinphoneCallUpdatedByRemote state notification, prevent LinphoneCore from performing an automatic answer.
 * 
 * When receiving a #LinphoneCallUpdatedByRemote state notification (ie an incoming reINVITE), the default behaviour of
 * LinphoneCore is to automatically answer the reINIVTE with call parameters unchanged.
 * However when for example when the remote party updated the call to propose a video stream, it can be useful
 * to prompt the user before answering. This can be achieved by calling linphone_core_defer_call_update() during 
 * the call state notifiacation, to deactivate the automatic answer that would just confirm the audio but reject the video.
 * Then, when the user responds to dialog prompt, it becomes possible to call linphone_core_accept_call_update() to answer
 * the reINVITE, with eventually video enabled in the LinphoneCallParams argument.
 * 
 * @Returns 0 if successful, -1 if the linphone_core_defer_call_update() was done outside a #LinphoneCallUpdatedByRemote notification, which is illegal.
**/
int linphone_core_defer_call_update(LinphoneCore *lc, LinphoneCall *call){
	if (call->state==LinphoneCallUpdatedByRemote){
		call->defer_update=TRUE;
		return 0;
	}
	ms_error("linphone_core_defer_call_update() not done in state LinphoneCallUpdatedByRemote");
	return -1;
}

/**
 * @ingroup call_control
 * Accept call modifications initiated by other end.
 * 
 * This call may be performed in response to a #LinphoneCallUpdatedByRemote state notification.
 * When such notification arrives, the application can decide to call linphone_core_defer_update_call() so that it can
 * have the time to prompt the user. linphone_call_get_remote_params() can be used to get information about the call parameters
 * requested by the other party, such as whether a video stream is requested.
 * 
 * When the user accepts or refuse the change, linphone_core_accept_call_update() can be done to answer to the other party.
 * If params is NULL, then the same call parameters established before the update request will continue to be used (no change).
 * If params is not NULL, then the update will be accepted according to the parameters passed.
 * Typical example is when a user accepts to start video, then params should indicate that video stream should be used 
 * (see linphone_call_params_enable_video()).
 * @param lc the linphone core object.
 * @param call the LinphoneCall object
 * @param params a LinphoneCallParams object describing the call parameters to accept.
 * @Returns 0 if sucessful, -1 otherwise (actually when this function call is performed outside ot #LinphoneCallUpdatedByRemote state).
**/
int linphone_core_accept_call_update(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params){
	SalMediaDescription *md;
	if (call->state!=LinphoneCallUpdatedByRemote){
		ms_error("linphone_core_accept_update(): invalid state %s to call this function.",
		         linphone_call_state_to_string(call->state));
		return -1;
	}
	if (params==NULL){
		call->params.has_video=lc->video_policy.automatically_accept;
	}else
		call->params=*params;

	if (call->current_params.in_conference) {
		ms_warning("Video isn't supported in conference");
		call->params.has_video = FALSE;
	}
	call->camera_active=call->params.has_video;
	update_local_media_description(lc,call);
	sal_call_set_local_media_description(call->op,call->localdesc);
	sal_call_accept(call->op);
	md=sal_call_get_final_media_description(call->op);
	if (md && !sal_media_description_empty(md))
		linphone_core_update_streams (lc,call,md);
	linphone_call_set_state(call,LinphoneCallStreamsRunning,"Connected (streams running)");
	return 0;
}

/**
 * Accept an incoming call.
 *
 * @ingroup call_control
 * Basically the application is notified of incoming calls within the
 * call_state_changed callback of the #LinphoneCoreVTable structure, where it will receive
 * a LinphoneCallIncoming event with the associated LinphoneCall object.
 * The application can later accept the call using this method.
 * @param lc the LinphoneCore object
 * @param call the LinphoneCall object representing the call to be answered.
 *
**/
int linphone_core_accept_call(LinphoneCore *lc, LinphoneCall *call){
	return linphone_core_accept_call_with_params(lc,call,NULL);
}

/**
 * Accept an incoming call, with parameters.
 *
 * @ingroup call_control
 * Basically the application is notified of incoming calls within the
 * call_state_changed callback of the #LinphoneCoreVTable structure, where it will receive
 * a LinphoneCallIncoming event with the associated LinphoneCall object.
 * The application can later accept the call using
 * this method.
 * @param lc the LinphoneCore object
 * @param call the LinphoneCall object representing the call to be answered.
 * @param params the specific parameters for this call, for example whether video is accepted or not. Use NULL to use default parameters.
 *
**/
int linphone_core_accept_call_with_params(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params)
{
	LinphoneProxyConfig *cfg=NULL,*dest_proxy=NULL;
	const char *contact=NULL;
	SalOp *replaced;
	SalMediaDescription *new_md;

	if (call==NULL){
		//if just one call is present answer the only one ...
		if(linphone_core_get_calls_nb (lc) != 1)
			return -1;
		else
			call = (LinphoneCall*)linphone_core_get_calls(lc)->data;
	}

	if (call->state==LinphoneCallConnected){
		/*call already accepted*/
		return -1;
	}

	/* check if this call is supposed to replace an already running one*/
	replaced=sal_call_get_replaces(call->op);
	if (replaced){
		LinphoneCall *rc=(LinphoneCall*)sal_op_get_user_pointer (replaced);
		if (rc){
			ms_message("Call %p replaces call %p. This last one is going to be terminated automatically.",
			           call,rc);
			linphone_core_terminate_call(lc,rc);
		}
	}

	if (lc->current_call!=call){
		linphone_core_preempt_sound_resources(lc);
	}

	/*stop ringing */
	if (lc->ringstream!=NULL) {
		ms_message("stop ringing");
		ring_stop(lc->ringstream);
		ms_message("ring stopped");
		lc->ringstream=NULL;
	}
	if (call->ringing_beep){
		linphone_core_stop_dtmf(lc);
		call->ringing_beep=FALSE;
	}

	linphone_core_get_default_proxy(lc,&cfg);
	dest_proxy=cfg;
	dest_proxy=linphone_core_lookup_known_proxy(lc,call->log->to);

	if (cfg!=dest_proxy && dest_proxy!=NULL) {
		ms_message("Overriding default proxy setting for this call:");
		ms_message("The used identity will be %s",linphone_proxy_config_get_identity(dest_proxy));
	}
	/*try to be best-effort in giving real local or routable contact address*/
	contact=get_fixed_contact(lc,call,dest_proxy);
	if (contact)
		sal_op_set_contact(call->op,contact);

	if (call->audiostream==NULL)
		linphone_call_init_media_streams(call);

	if (params){
		call->params=*params;
		call->camera_active=call->params.has_video;
		update_local_media_description(lc,call);
		sal_call_set_local_media_description(call->op,call->localdesc);
	}
	
	sal_call_accept(call->op);
	if (lc->vtable.display_status!=NULL)
		lc->vtable.display_status(lc,_("Connected."));
	lc->current_call=call;
	linphone_call_set_state(call,LinphoneCallConnected,"Connected");
	new_md=sal_call_get_final_media_description(call->op);
	linphone_core_update_streams(lc, call, new_md);
	if (new_md){
		linphone_call_set_state(call,LinphoneCallStreamsRunning,"Connected (streams running)");
	}else call->media_pending=TRUE;

	ms_message("call answered.");
	return 0;
}

int linphone_core_abort_call(LinphoneCore *lc, LinphoneCall *call, const char *error){
	sal_call_terminate(call->op);

	/*stop ringing*/
	if (lc->ringstream!=NULL) {
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
	}
	linphone_call_stop_media_streams(call);
	if (lc->vtable.display_status!=NULL)
		lc->vtable.display_status(lc,_("Call aborted") );
	linphone_call_set_state(call,LinphoneCallError,error);
	return 0;
}

static void terminate_call(LinphoneCore *lc, LinphoneCall *call){
	if (call->state==LinphoneCallIncomingReceived){
		call->reason=LinphoneReasonDeclined;
	}
	/*stop ringing*/
	if (lc->ringstream!=NULL) {
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
	}

	linphone_call_stop_media_streams(call);
	if (lc->vtable.display_status!=NULL)
		lc->vtable.display_status(lc,_("Call ended") );
}

int linphone_core_redirect_call(LinphoneCore *lc, LinphoneCall *call, const char *redirect_uri){
	if (call->state==LinphoneCallIncomingReceived){
		sal_call_decline(call->op,SalReasonRedirect,redirect_uri);
		call->reason=LinphoneReasonDeclined;
		terminate_call(lc,call);
		linphone_call_set_state(call,LinphoneCallEnd,"Call terminated");
	}else{
		ms_error("Bad state for call redirection.");
		return -1;
    }
	return 0;
}


/**
 * Terminates a call.
 *
 * @ingroup call_control
 * @param lc the LinphoneCore
 * @param the_call the LinphoneCall object representing the call to be terminated.
**/
int linphone_core_terminate_call(LinphoneCore *lc, LinphoneCall *the_call)
{
	LinphoneCall *call;
	if (the_call == NULL){
		call = linphone_core_get_current_call(lc);
		if (ms_list_size(lc->calls)==1){
			call=(LinphoneCall*)lc->calls->data;
		}else{
			ms_warning("No unique call to terminate !");
			return -1;
		}
	}
	else
	{
		call = the_call;
	}
	sal_call_terminate(call->op);
	terminate_call(lc,call);

	linphone_call_set_state(call,LinphoneCallEnd,"Call terminated");
	return 0;
}

/**
 * Terminates all the calls.
 *
 * @ingroup call_control
 * @param lc The LinphoneCore
**/
int linphone_core_terminate_all_calls(LinphoneCore *lc){
	MSList *calls=lc->calls;
	while(calls) {
		LinphoneCall *c=(LinphoneCall*)calls->data;
		calls=calls->next;
		linphone_core_terminate_call(lc,c);
	}
	return 0;
}

/**
 * Returns the current list of calls.
 *
 * Note that this list is read-only and might be changed by the core after a function call to linphone_core_iterate().
 * Similarly the LinphoneCall objects inside it might be destroyed without prior notice.
 * To hold references to LinphoneCall object into your program, you must use linphone_call_ref().
 *
 * @ingroup call_control
**/
const MSList *linphone_core_get_calls(LinphoneCore *lc)
{
	return lc->calls;
}

/**
 * Returns TRUE if there is a call running.
 *
 * @ingroup call_control
**/
bool_t linphone_core_in_call(const LinphoneCore *lc){
	return linphone_core_get_current_call((LinphoneCore *)lc)!=NULL || linphone_core_is_in_conference(lc);
}

/**
 * Returns The _LinphoneCall struct of the current call if one is in call
 *
 * @ingroup call_control
**/
LinphoneCall *linphone_core_get_current_call(const LinphoneCore *lc)
{
	return lc->current_call;
}

/**
 * Pauses the call. If a music file has been setup using linphone_core_set_play_file(),
 * this file will be played to the remote user.
 *
 * @ingroup call_control
**/
int linphone_core_pause_call(LinphoneCore *lc, LinphoneCall *call)
{
	const char *subject=NULL;

	if (call->state!=LinphoneCallStreamsRunning && call->state!=LinphoneCallPausedByRemote){
		ms_warning("Cannot pause this call, it is not active.");
		return -1;
	}
	update_local_media_description(lc,call);
	if (sal_media_description_has_dir(call->resultdesc,SalStreamSendRecv)){
		sal_media_description_set_dir(call->localdesc,SalStreamSendOnly);
		subject="Call on hold";
	}else if (sal_media_description_has_dir(call->resultdesc,SalStreamRecvOnly)){
		sal_media_description_set_dir(call->localdesc,SalStreamSendOnly);
		subject="Call on hold for me too";
	}else{
		ms_error("No reason to pause this call, it is already paused or inactive.");
		return -1;
	}
	sal_call_set_local_media_description(call->op,call->localdesc);
	if (sal_call_update(call->op,subject) != 0){
		if (lc->vtable.display_warning)
			lc->vtable.display_warning(lc,_("Could not pause the call"));
	}
	lc->current_call=NULL;
	linphone_call_set_state(call,LinphoneCallPausing,"Pausing call");
	if (lc->vtable.display_status)
		lc->vtable.display_status(lc,_("Pausing the current call..."));
	if (call->audiostream || call->videostream)
		linphone_call_stop_media_streams (call);
	return 0;
}

/**
 * Pause all currently running calls.
**/
int linphone_core_pause_all_calls(LinphoneCore *lc){
	const MSList *elem;
	for(elem=lc->calls;elem!=NULL;elem=elem->next){
		LinphoneCall *call=(LinphoneCall *)elem->data;
		LinphoneCallState cs=linphone_call_get_state(call);
		if (cs==LinphoneCallStreamsRunning || cs==LinphoneCallPausedByRemote){
			linphone_core_pause_call(lc,call);
		}
	}
	return 0;
}

void linphone_core_preempt_sound_resources(LinphoneCore *lc){
	LinphoneCall *current_call;
	if (linphone_core_is_in_conference(lc)){
		linphone_core_leave_conference(lc);
		return;
	}
	current_call=linphone_core_get_current_call(lc);
	if(current_call != NULL){
		ms_message("Pausing automatically the current call.");
		linphone_core_pause_call(lc,current_call);
	}
}

/**
 * Resumes the call.
 *
 * @ingroup call_control
**/
int linphone_core_resume_call(LinphoneCore *lc, LinphoneCall *the_call)
{
	char temp[255]={0};
	LinphoneCall *call = the_call;
	const char *subject="Call resuming";
	
	if(call->state!=LinphoneCallPaused ){
		ms_warning("we cannot resume a call that has not been established and paused before");
		return -1;
	}
	if (call->params.in_conference==FALSE){
		linphone_core_preempt_sound_resources(lc);
		ms_message("Resuming call %p",call);
	}

	/* Stop playing music immediately. If remote side is a conference it
	 prevents the participants to hear it while the 200OK comes back.*/
	if (call->audiostream) audio_stream_play(call->audiostream, NULL);

	update_local_media_description(lc,the_call);
	sal_call_set_local_media_description(call->op,call->localdesc);
	sal_media_description_set_dir(call->localdesc,SalStreamSendRecv);
	if (call->params.in_conference && !call->current_params.in_conference) subject="Conference";
	if(sal_call_update(call->op,subject) != 0){
		return -1;
	}
	linphone_call_set_state (call,LinphoneCallResuming,"Resuming");
	snprintf(temp,sizeof(temp)-1,"Resuming the call with %s",linphone_call_get_remote_address_as_string(call));
	if (lc->vtable.display_status)
		lc->vtable.display_status(lc,temp);
	return 0;
}

static int remote_address_compare(LinphoneCall *call, const LinphoneAddress *raddr){
	const LinphoneAddress *addr=linphone_call_get_remote_address (call);
	return !linphone_address_weak_equal (addr,raddr);
}

/**
 * Get the call with the remote_address specified
 * @param lc
 * @param remote_address
 * @return the LinphoneCall of the call if found
 */
LinphoneCall *linphone_core_get_call_by_remote_address(LinphoneCore *lc, const char *remote_address){
	LinphoneAddress *raddr=linphone_address_new(remote_address);
	MSList *elem=ms_list_find_custom(lc->calls,(int (*)(const void*,const void *))remote_address_compare,raddr);
	if (elem) return (LinphoneCall*) elem->data;
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

/**
 * Allow to control play level before entering sound card:  gain in db
 *
 * @ingroup media_parameters
**/
void linphone_core_set_playback_gain_db (LinphoneCore *lc, float gaindb){
	float gain=gaindb;
	LinphoneCall *call=linphone_core_get_current_call (lc);
	AudioStream *st;

	lc->sound_conf.soft_play_lev=gaindb;

	if (call==NULL || (st=call->audiostream)==NULL){
		ms_message("linphone_core_set_playback_gain_db(): no active call.");
		return;
	}
	if (st->volrecv){
		ms_filter_call_method(st->volrecv,MS_VOLUME_SET_DB_GAIN,&gain);
	}else ms_warning("Could not apply gain: gain control wasn't activated.");
}

/**
 * Get playback gain in db before entering  sound card.
 *
 * @ingroup media_parameters
**/
float linphone_core_get_playback_gain_db(LinphoneCore *lc) {
	return lc->sound_conf.soft_play_lev;
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
 * @param lc The LinphoneCore object
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
 * @param lc The LinphoneCore object
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
 * @param lc The LinphoneCore object
 * @param devid the device name as returned by linphone_core_get_sound_devices()
**/
int linphone_core_set_ringer_device(LinphoneCore *lc, const char * devid){
	MSSndCard *card=get_card_from_string_id(devid,MS_SND_CARD_CAP_PLAYBACK);
	lc->sound_conf.ring_sndcard=card;
	if (card && linphone_core_ready(lc))
		lp_config_set_string(lc->config,"sound","ringer_dev_id",ms_snd_card_get_string_id(card));
	return 0;
}

/**
 * Sets the sound device used for playback.
 *
 * @ingroup media_parameters
 * @param lc The LinphoneCore object
 * @param devid the device name as returned by linphone_core_get_sound_devices()
**/
int linphone_core_set_playback_device(LinphoneCore *lc, const char * devid){
	MSSndCard *card=get_card_from_string_id(devid,MS_SND_CARD_CAP_PLAYBACK);
	lc->sound_conf.play_sndcard=card;
	if (card &&  linphone_core_ready(lc))
		lp_config_set_string(lc->config,"sound","playback_dev_id",ms_snd_card_get_string_id(card));
	return 0;
}

/**
 * Sets the sound device used for capture.
 *
 * @ingroup media_parameters
 * @param lc The LinphoneCore object
 * @param devid the device name as returned by linphone_core_get_sound_devices()
**/
int linphone_core_set_capture_device(LinphoneCore *lc, const char * devid){
	MSSndCard *card=get_card_from_string_id(devid,MS_SND_CARD_CAP_CAPTURE);
	lc->sound_conf.capt_sndcard=card;
	if (card &&  linphone_core_ready(lc))
		lp_config_set_string(lc->config,"sound","capture_dev_id",ms_snd_card_get_string_id(card));
	return 0;
}

/**
 * Returns the name of the currently assigned sound device for ringing.
 *
 * @ingroup media_parameters
 * @param lc The LinphoneCore object
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
 * @param lc The LinphoneCore object
**/
const char * linphone_core_get_playback_device(LinphoneCore *lc)
{
	return lc->sound_conf.play_sndcard ? ms_snd_card_get_string_id(lc->sound_conf.play_sndcard) : NULL;
}

/**
 * Returns the name of the currently assigned sound device for capture.
 *
 * @ingroup media_parameters
 * @param lc The LinphoneCore object
**/
const char * linphone_core_get_capture_device(LinphoneCore *lc)
{
	return lc->sound_conf.capt_sndcard ? ms_snd_card_get_string_id(lc->sound_conf.capt_sndcard) : NULL;
}

/**
 * Returns an unmodifiable array of available sound devices.
 *
 * The array is NULL terminated.
 *
 * @ingroup media_parameters
 * @param lc The LinphoneCore object
**/
const char**  linphone_core_get_sound_devices(LinphoneCore *lc){
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

/**
 * Update detection of sound devices.
 * 
 * Use this function when the application is notified of USB plug events, so that
 * list of available hardwares for sound playback and capture is updated.
 **/
void linphone_core_reload_sound_devices(LinphoneCore *lc){
	const char *ringer,*playback,*capture;
	ringer=linphone_core_get_ringer_device(lc);
	playback=linphone_core_get_playback_device(lc);
	capture=linphone_core_get_capture_device(lc);
	ms_snd_card_manager_reload(ms_snd_card_manager_get());
	build_sound_devices_table(lc);
	linphone_core_set_ringer_device(lc,ringer);
	linphone_core_set_playback_device(lc,playback);
	linphone_core_set_capture_device(lc,capture);
}

/**
 * Update detection of camera devices.
 * 
 * Use this function when the application is notified of USB plug events, so that
 * list of available hardwares for video capture is updated.
 **/
void linphone_core_reload_video_devices(LinphoneCore *lc){
	const char *devid;
	devid=linphone_core_get_video_device(lc);
	ms_web_cam_manager_reload(ms_web_cam_manager_get());
	build_video_devices_table(lc);
	linphone_core_set_video_device(lc,devid);
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
 * @param path The file must be a wav 16bit linear. Local ring is disabled if null
 * @param lc The LinphoneCore object
 *
 * @ingroup media_parameters
**/
void linphone_core_set_ring(LinphoneCore *lc,const char *path){
	if (lc->sound_conf.local_ring!=0){
		ms_free(lc->sound_conf.local_ring);
		lc->sound_conf.local_ring=NULL;
	}
	if (path)
		lc->sound_conf.local_ring=ms_strdup(path);
	if ( linphone_core_ready(lc) && lc->sound_conf.local_ring)
		lp_config_set_string(lc->config,"sound","local_ring",lc->sound_conf.local_ring);
}

/**
 * Returns the path to the wav file used for ringing.
 *
 * @param lc The LinphoneCore object
 * @ingroup media_parameters
**/
const char *linphone_core_get_ring(const LinphoneCore *lc){
	return lc->sound_conf.local_ring;
}

/**
 * Sets the path to a file or folder containing trusted root CAs (PEM format)
 *
 * @param path
 * @param lc The LinphoneCore object
 *
 * @ingroup media_parameters
**/
void linphone_core_set_root_ca(LinphoneCore *lc,const char *path){
	sal_set_root_ca(lc->sal, path);
}

/**
 * Specify whether the tls server certificate must be verified when connecting to a SIP/TLS server.
**/
void linphone_core_verify_server_certificates(LinphoneCore *lc, bool_t yesno){
	sal_verify_server_certificates(lc->sal,yesno);
}

static void notify_end_of_ring(void *ud, MSFilter *f, unsigned int event, void *arg){
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
		MSSndCard *ringcard=lc->sound_conf.lsd_card ? lc->sound_conf.lsd_card : lc->sound_conf.ring_sndcard;
		lc->ringstream=ring_start_with_cb(ring,2000,ringcard,notify_end_of_ring,(void *)lc);
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
 * Enables or disable echo cancellation. Value is saved an used for subsequent calls
 *
 * @ingroup media_parameters
**/
void linphone_core_enable_echo_cancellation(LinphoneCore *lc, bool_t val){
	lc->sound_conf.ec=val;
	if ( linphone_core_ready(lc))
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
	LinphoneCall *call=linphone_core_get_current_call(lc);
	AudioStream *st=NULL;
	if (linphone_core_is_in_conference(lc)){
		lc->conf_ctx.local_muted=val;
		st=lc->conf_ctx.local_participant;
	}else if (call==NULL){
		ms_warning("linphone_core_mute_mic(): No current call !");
		return;
	}else{
		st=call->audiostream;
		call->audio_muted=val;
	}
	if (st!=NULL){
		audio_stream_set_mic_gain(st,
			(val==TRUE) ? 0 : lp_config_get_float(lc->config,"sound","mic_gain",1));
		if ( linphone_core_get_rtp_no_xmit_on_audio_mute(lc) ){
			audio_stream_mute_rtp(st,val);
		}
		
	}
}
/**
 * Returns whether microphone is muted.
**/
bool_t linphone_core_is_mic_muted(LinphoneCore *lc) {
	LinphoneCall *call=linphone_core_get_current_call(lc);
	if (linphone_core_is_in_conference(lc)){
		return lc->conf_ctx.local_muted;
	}else if (call==NULL){
		ms_warning("linphone_core_is_mic_muted(): No current call !");
		return FALSE;
	}
	return call->audio_muted;
}

// returns rtp transmission status for an active stream
// if audio is muted and config parameter rtp_no_xmit_on_audio_mute
// was set on then rtp transmission is also muted
bool_t linphone_core_is_rtp_muted(LinphoneCore *lc){
	LinphoneCall *call=linphone_core_get_current_call(lc);
	if (call==NULL){
		ms_warning("linphone_core_is_mic_muted(): No current call !");
		return FALSE;
	}
	if( linphone_core_get_rtp_no_xmit_on_audio_mute(lc)){
		return call->audio_muted;
	}
	return FALSE;
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
	LinphoneCall *call=linphone_core_get_current_call(lc);
	if (call==NULL){
		ms_warning("linphone_core_send_dtmf(): no active call");
		return;
	}
	/*By default we send DTMF RFC2833 if we do not have enabled SIP_INFO but we can also send RFC2833 and SIP_INFO*/
	if (linphone_core_get_use_rfc2833_for_dtmf(lc)!=0 || linphone_core_get_use_info_for_dtmf(lc)==0)
	{
		/* In Band DTMF */
		if (call->audiostream!=NULL){
			audio_stream_send_dtmf(call->audiostream,dtmf);
		}
		else
		{
			ms_error("we cannot send RFC2833 dtmf when we are not in communication");
		}
	}
	if (linphone_core_get_use_info_for_dtmf(lc)!=0){
		/* Out of Band DTMF (use INFO method) */
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

const char *linphone_core_get_nat_address(const LinphoneCore *lc) {
	return lc->net_conf.nat_address;
}

const char *linphone_core_get_nat_address_resolved(LinphoneCore *lc)
{
	struct sockaddr_storage ss;
	socklen_t ss_len;
	int error;
	char ipstring [INET6_ADDRSTRLEN];

	if (lc->net_conf.nat_address==NULL) return NULL;
	
	if (parse_hostname_to_addr (lc->net_conf.nat_address, &ss, &ss_len)<0) {
		return lc->net_conf.nat_address;
	}

	error = getnameinfo((struct sockaddr *)&ss, ss_len,
		ipstring, sizeof(ipstring), NULL, 0, NI_NUMERICHOST);
	if (error) {
		return lc->net_conf.nat_address;
	}

	if (lc->net_conf.nat_address_ip!=NULL){
		ms_free(lc->net_conf.nat_address_ip);
	}
	lc->net_conf.nat_address_ip = ms_strdup (ipstring);
	return lc->net_conf.nat_address_ip;
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
	if (val){
		if (lc->previewstream==NULL){
			lc->previewstream=video_preview_new();
			video_preview_set_size(lc->previewstream,lc->video_conf.vsize);
			if (lc->video_conf.displaytype)
				video_preview_set_display_filter_name(lc->previewstream,lc->video_conf.displaytype);
			if (lc->preview_window_id!=0)
				video_preview_set_native_window_id(lc->previewstream,lc->preview_window_id);
			video_preview_start(lc->previewstream,lc->video_conf.device);
		}
	}else{
		if (lc->previewstream!=NULL){
			video_preview_stop(lc->previewstream);
			lc->previewstream=NULL;
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
 * @param lc The LinphoneCore object
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

bool_t linphone_core_video_supported(LinphoneCore *lc){
#ifdef VIDEO_ENABLED
	return TRUE;
#else
	return FALSE;
#endif
}

/**
 * Returns TRUE if video is enabled, FALSE otherwise.
 * @ingroup media_parameters
**/
bool_t linphone_core_video_enabled(LinphoneCore *lc){
	return (lc->video_conf.display || lc->video_conf.capture);
}

/**
 * Sets the default policy for video.
 * This policy defines whether:
 * - video shall be initiated by default for outgoing calls
 * - video shall be accepter by default for incoming calls
**/
void linphone_core_set_video_policy(LinphoneCore *lc, const LinphoneVideoPolicy *policy){
	lc->video_policy=*policy;
	if (linphone_core_ready(lc)){
		lp_config_set_int(lc->config,"video","automatically_initiate",policy->automatically_initiate);
		lp_config_set_int(lc->config,"video","automatically_accept",policy->automatically_accept);
	}
}

/**
 * Get the default policy for video.
 * See linphone_core_set_video_policy() for more details.
**/
const LinphoneVideoPolicy *linphone_core_get_video_policy(LinphoneCore *lc){
	return &lc->video_policy;
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
#ifdef VIDEO_ENABLED
	LinphoneCall *call=linphone_core_get_current_call (lc);
	lc->video_conf.selfview=val;
	if (call && call->videostream){
		video_stream_enable_self_view(call->videostream,val);
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
 * @param lc The LinphoneCore object
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
	if ( linphone_core_ready(lc) && lc->video_conf.device){
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
 * @param lc The LinphoneCore object
 * @ingroup media_parameters
**/
const char *linphone_core_get_video_device(const LinphoneCore *lc){
	if (lc->video_conf.device) return ms_web_cam_get_string_id(lc->video_conf.device);
	return NULL;
}

#ifdef VIDEO_ENABLED
static VideoStream * get_active_video_stream(LinphoneCore *lc){
	VideoStream *vs = NULL;
	LinphoneCall *call=linphone_core_get_current_call (lc);
	/* Select the video stream from the call in the first place */
	if (call && call->videostream) {
		vs = call->videostream;
	}
	/* If not in call, select the video stream from the preview */
	if (vs == NULL && lc->previewstream) {
		vs = lc->previewstream;
	}
	return vs;
}
#endif

int linphone_core_set_static_picture(LinphoneCore *lc, const char *path) {
#ifdef VIDEO_ENABLED
	VideoStream *vs=get_active_video_stream(lc);
	/* If we have a video stream (either preview, either from call), we
		 have a source and it is using the static picture filter, then
		 force the filter to use that picture. */
	if (vs && vs->source) {
		if (ms_filter_get_id(vs->source) == MS_STATIC_IMAGE_ID) {
			ms_filter_call_method(vs->source, MS_STATIC_IMAGE_SET_IMAGE,
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

int linphone_core_set_static_picture_fps(LinphoneCore *lc, float fps) {
#ifdef VIDEO_ENABLED
	VideoStream *vs = NULL;

	vs=get_active_video_stream(lc);

	/* If we have a video stream (either preview, either from call), we
		 have a source and it is using the static picture filter, then
		 force the filter to use that picture. */
	if (vs && vs->source) {
		if (ms_filter_get_id(vs->source) == MS_STATIC_IMAGE_ID) {
			ms_filter_call_method(vs->source, MS_FILTER_SET_FPS,(void *)&fps);
		}
	}
#else
	ms_warning("Video support not compiled.");
#endif
	return 0;
}

float linphone_core_get_static_picture_fps(LinphoneCore *lc) {
#ifdef VIDEO_ENABLED
	VideoStream *vs = NULL;
	vs=get_active_video_stream(lc);
	/* If we have a video stream (either preview, either from call), we
		 have a source and it is using the static picture filter, then
		 force the filter to use that picture. */
	if (vs && vs->source) {
		if (ms_filter_get_id(vs->source) == MS_STATIC_IMAGE_ID) {

		        float fps;

			ms_filter_call_method(vs->source, MS_FILTER_GET_FPS,(void *)&fps);
			return fps;
		}
	}
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
	LinphoneCall *call=linphone_core_get_current_call (lc);
	if (call && call->videostream)
		return video_stream_get_native_window_id(call->videostream);
	if (lc->previewstream)
		return video_stream_get_native_window_id(lc->previewstream);
#endif
	return lc->video_window_id;
}

/**@ingroup media_parameters
 * Set the native video window id where the video is to be displayed.
 * If not set the core will create its own window.
**/
void linphone_core_set_native_video_window_id(LinphoneCore *lc, unsigned long id){
#ifdef VIDEO_ENABLED
	LinphoneCall *call=linphone_core_get_current_call(lc);
	lc->video_window_id=id;
	if (call!=NULL && call->videostream){
		video_stream_set_native_window_id(call->videostream,id);
	}
#endif
}

/**
 * Returns the native window handle of the video preview window, casted as an unsigned long.
 *
 * @ingroup media_parameters
**/
unsigned long linphone_core_get_native_preview_window_id(const LinphoneCore *lc){
#ifdef VIDEO_ENABLED
	LinphoneCall *call=linphone_core_get_current_call (lc);
	if (call && call->videostream)
		return video_stream_get_native_preview_window_id(call->videostream);
	if (lc->previewstream)
		return video_preview_get_native_window_id(lc->previewstream);
#endif
	return lc->preview_window_id;
}

/**
 * @ingroup media_parameters
 * Set the native window id where the preview video (local camera) is to be displayed.
 * This has to be used in conjonction with linphone_core_use_preview_window().
 * If not set the core will create its own window.
**/
void linphone_core_set_native_preview_window_id(LinphoneCore *lc, unsigned long id){
	lc->preview_window_id=id;
#ifdef VIDEO_ENABLED
	LinphoneCall *call=linphone_core_get_current_call(lc);
	if (call!=NULL && call->videostream){
		video_stream_set_native_preview_window_id(call->videostream,id);
	}
#endif
}

/**
 * Can be used to disable video showing to free XV port
**/
void linphone_core_show_video(LinphoneCore *lc, bool_t show){
#ifdef VIDEO_ENABLED
	ms_error("linphone_core_show_video %d", show);
	LinphoneCall *call=linphone_core_get_current_call(lc);
	if (call!=NULL && call->videostream){
		video_stream_show_video(call->videostream,show);
	}
#endif
}

/**
 * Tells the core to use a separate window for local camera preview video, instead of
 * inserting local view within the remote video window.
 *
**/
void linphone_core_use_preview_window(LinphoneCore *lc, bool_t yesno){
	lc->use_preview_window=yesno;
}
/**
 * @ingroup media_parameters
 *returns current device orientation
 */
int linphone_core_get_device_rotation(LinphoneCore *lc ) {
	return lc->device_rotation;
}
/**
 * @ingroup media_parameters
 * Tells the core the device current orientation. This can be used by capture filters
 * on mobile devices to select between portrait/landscape mode and to produce properly
 * oriented images. The exact meaning of the value in rotation if left to each device
 * specific implementations.
 *@param lc  object.
 *@param rotation . IOS supported values are 0 for UIInterfaceOrientationPortrait and 270 for UIInterfaceOrientationLandscapeRight.
 *
**/
void linphone_core_set_device_rotation(LinphoneCore *lc, int rotation) {
ms_message("%s : rotation=%d\n", __FUNCTION__, rotation);
	lc->device_rotation = rotation;
#ifdef VIDEO_ENABLED
	LinphoneCall *call=linphone_core_get_current_call(lc);
	if (call!=NULL && call->videostream){
		video_stream_set_device_rotation(call->videostream,rotation);
	}
#endif
}

static MSVideoSizeDef supported_resolutions[]={
#ifdef ENABLE_HD
	{	{MS_VIDEO_SIZE_1080P_W,MS_VIDEO_SIZE_1080P_H}	,	"1080p"	},
	{	{MS_VIDEO_SIZE_720P_W,MS_VIDEO_SIZE_720P_H}	,	"1080p"	},
#endif
	{	{MS_VIDEO_SIZE_SVGA_W,MS_VIDEO_SIZE_SVGA_H}	,	"svga"	},
	{	{MS_VIDEO_SIZE_4CIF_W,MS_VIDEO_SIZE_4CIF_H}	,	"4cif"	},
	{	{MS_VIDEO_SIZE_VGA_W,MS_VIDEO_SIZE_VGA_H}	,	"vga"	},
	{	{MS_VIDEO_SIZE_IOS_MEDIUM_H,MS_VIDEO_SIZE_IOS_MEDIUM_W}	,	"ios-medium"	},
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
		if ( linphone_core_ready(lc))
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

/**
 * Ask the core to stream audio from and to files, instead of using the soundcard.
**/
void linphone_core_use_files(LinphoneCore *lc, bool_t yesno){
	lc->use_files=yesno;
}

/**
 * Sets a wav file to be played when putting somebody on hold,
 * or when files are used instead of soundcards (see linphone_core_use_files()).
 *
 * The file must be a 16 bit linear wav file.
**/
void linphone_core_set_play_file(LinphoneCore *lc, const char *file){
	LinphoneCall *call=linphone_core_get_current_call(lc);
	if (lc->play_file!=NULL){
		ms_free(lc->play_file);
		lc->play_file=NULL;
	}
	if (file!=NULL) {
		lc->play_file=ms_strdup(file);
		if (call && call->audiostream && call->audiostream->ticker)
			audio_stream_play(call->audiostream,file);
	}
}


/**
 * Sets a wav file where incoming stream is to be recorded,
 * when files are used instead of soundcards (see linphone_core_use_files()).
 *
 * The file must be a 16 bit linear wav file.
**/
void linphone_core_set_record_file(LinphoneCore *lc, const char *file){
	LinphoneCall *call=linphone_core_get_current_call(lc);
	if (lc->rec_file!=NULL){
		ms_free(lc->rec_file);
		lc->rec_file=NULL;
	}
	if (file!=NULL) {
		lc->rec_file=ms_strdup(file);
		if (call && call->audiostream)
			audio_stream_record(call->audiostream,file);
	}
}


static MSFilter *get_dtmf_gen(LinphoneCore *lc){
	LinphoneCall *call=linphone_core_get_current_call (lc);
	AudioStream *stream=NULL;
	if (call){
		stream=call->audiostream;
	}else if (linphone_core_is_in_conference(lc)){
		stream=lc->conf_ctx.local_participant;
	}
	if (stream){
		return stream->dtmfgen;
	}
	if (lc->ringstream==NULL){
		float amp=0.1;
		MSSndCard *ringcard=lc->sound_conf.lsd_card ?lc->sound_conf.lsd_card : lc->sound_conf.ring_sndcard;
		if (ringcard == NULL)
			return NULL;

		lc->ringstream=ring_start(NULL,0,ringcard);
		ms_filter_call_method(lc->ringstream->gendtmf,MS_DTMF_GEN_SET_DEFAULT_AMPLITUDE,&amp);
		lc->dmfs_playing_start_time=time(NULL);
	}else{
		if (lc->dmfs_playing_start_time!=0)
			lc->dmfs_playing_start_time=time(NULL);
	}
	return lc->ringstream->gendtmf;
}

/**
 * @ingroup media_parameters
 * Plays a dtmf sound to the local user.
 * @param lc #LinphoneCore
 * @param dtmf DTMF to play ['0'..'16'] | '#' | '#'
 * @param duration_ms duration in ms, -1 means play until next further call to #linphone_core_stop_dtmf()
**/
void linphone_core_play_dtmf(LinphoneCore *lc, char dtmf, int duration_ms){
	MSFilter *f=get_dtmf_gen(lc);
	if (f==NULL){
		ms_error("No dtmf generator at this time !");
		return;
	}

	if (duration_ms>0)
		ms_filter_call_method(f, MS_DTMF_GEN_PLAY, &dtmf);
	else ms_filter_call_method(f, MS_DTMF_GEN_START, &dtmf);
}

/**
 * @ingroup media_parameters
 * Plays a repeated tone to the local user until next further call to #linphone_core_stop_dtmf()
 * @param lc #LinphoneCore
**/
void linphone_core_play_tone(LinphoneCore *lc){
	MSFilter *f=get_dtmf_gen(lc);
	MSDtmfGenCustomTone def;
	if (f==NULL){
		ms_error("No dtmf generator at this time !");
		return;
	}
	def.duration=300;
	def.frequency=500;
	def.amplitude=1;
	def.interval=2000;
	ms_filter_call_method(f, MS_DTMF_GEN_PLAY_CUSTOM,&def);
}

/**
 * @ingroup media_parameters
 *
 * Stops playing a dtmf started by linphone_core_play_dtmf().
**/
void linphone_core_stop_dtmf(LinphoneCore *lc){
	MSFilter *f=get_dtmf_gen(lc);
	if (f!=NULL)
		ms_filter_call_method_noarg (f, MS_DTMF_GEN_STOP);
}



/**
 * Retrieves the user pointer that was given to linphone_core_new()
 *
 * @ingroup initializing
**/
void *linphone_core_get_user_data(LinphoneCore *lc){
	return lc->data;
}

void linphone_core_set_user_data(LinphoneCore *lc, void *userdata){
	lc->data=userdata;
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

void linphone_core_set_rtp_transport_factories(LinphoneCore* lc, LinphoneRtpTransportFactories *factories){
	lc->rtptf=factories;
}

/**
 * Retrieve RTP statistics regarding current call.
 * @param local RTP statistics computed locally.
 * @param remote RTP statistics computed by far end (obtained via RTCP feedback).
 *
 * @note Remote RTP statistics is not implemented yet.
 *
 * @returns 0 or -1 if no call is running.
**/

int linphone_core_get_current_call_stats(LinphoneCore *lc, rtp_stats_t *local, rtp_stats_t *remote){
	LinphoneCall *call=linphone_core_get_current_call (lc);
	if (call!=NULL){
		if (call->audiostream!=NULL){
			memset(remote,0,sizeof(*remote));
			audio_stream_get_local_rtp_stats (call->audiostream,local);
			return 0;
		}
	}
	return -1;
}

void net_config_uninit(LinphoneCore *lc)
{
	net_config_t *config=&lc->net_conf;

	if (config->stun_server!=NULL){
		lp_config_set_string(lc->config,"net","stun_server",config->stun_server);
		ms_free(lc->net_conf.stun_server);
	}
	if (config->nat_address!=NULL){
		lp_config_set_string(lc->config,"net","nat_address",config->nat_address);
		ms_free(lc->net_conf.nat_address);
	}
	if (lc->net_conf.nat_address_ip !=NULL){
		ms_free(lc->net_conf.nat_address_ip);
	}
	lp_config_set_int(lc->config,"net","firewall_policy",config->firewall_policy);
	lp_config_set_int(lc->config,"net","mtu",config->mtu);
}


void sip_config_uninit(LinphoneCore *lc)
{
	MSList *elem;
	int i;
	sip_config_t *config=&lc->sip_conf;
	
	lp_config_set_int(lc->config,"sip","guess_hostname",config->guess_hostname);
	lp_config_set_string(lc->config,"sip","contact",config->contact);
	lp_config_set_int(lc->config,"sip","inc_timeout",config->inc_timeout);
	lp_config_set_int(lc->config,"sip","use_info",config->use_info);
	lp_config_set_int(lc->config,"sip","use_rfc2833",config->use_rfc2833);
	lp_config_set_int(lc->config,"sip","use_ipv6",config->ipv6_enabled);
	lp_config_set_int(lc->config,"sip","register_only_when_network_is_up",config->register_only_when_network_is_up);


	

	for(elem=config->proxies,i=0;elem!=NULL;elem=ms_list_next(elem),i++){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)(elem->data);
		linphone_proxy_config_edit(cfg);	/* to unregister */
	}

	for (i=0;i<20;i++){
		sal_iterate(lc->sal);
#ifndef WIN32
		usleep(100000);
#else
		Sleep(100);
#endif
	}

	ms_list_for_each(config->proxies,(void (*)(void*)) linphone_proxy_config_destroy);
	ms_list_free(config->proxies);
	config->proxies=NULL;

	linphone_proxy_config_write_to_config_file(lc->config,NULL,i);	/*mark the end */

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
	sprintf(key,"audio_codec_%i",index);
	lp_config_clean_section (lc->config,key);

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
	sprintf(key,"video_codec_%i",index);
	lp_config_clean_section (lc->config,key);

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
	linphone_core_free_hooks(lc);
	while(lc->calls)
	{
		LinphoneCall *the_call = lc->calls->data;
		linphone_core_terminate_call(lc,the_call);
		linphone_core_iterate(lc);
#ifdef WIN32
		Sleep(50000);
#else
		usleep(50000);
#endif
	}
	if (lc->friends)
		ms_list_for_each(lc->friends,(void (*)(void *))linphone_friend_close_subscriptions);
	linphone_core_set_state(lc,LinphoneGlobalShutdown,"Shutting down");
#ifdef VIDEO_ENABLED
	if (lc->previewstream!=NULL){
		video_preview_stop(lc->previewstream);
		lc->previewstream=NULL;
	}
#endif
	ms_event_queue_destroy(lc->msevq);
	lc->msevq=NULL;
	/* save all config */
	net_config_uninit(lc);
	rtp_config_uninit(lc);
	if (lc->ringstream) ring_stop(lc->ringstream);
	sound_config_uninit(lc);
	video_config_uninit(lc);
	codecs_config_uninit(lc);
	ui_config_uninit(lc);
	sip_config_uninit(lc);
	if (lp_config_needs_commit(lc->config)) lp_config_sync(lc->config);
	lp_config_destroy(lc->config);
	lc->config = NULL; /* Mark the config as NULL to block further calls */
	sip_setup_unregister_all();

	ms_list_for_each(lc->call_logs,(void (*)(void*))linphone_call_log_destroy);
	lc->call_logs=ms_list_free(lc->call_logs);

	linphone_core_free_payload_types(lc);
	ortp_exit();
	linphone_core_set_state(lc,LinphoneGlobalOff,"Off");
#ifdef TUNNEL_ENABLED
	if (lc->tunnel) linphone_tunnel_destroy(lc->tunnel);
#endif
}

static void set_network_reachable(LinphoneCore* lc,bool_t isReachable, time_t curtime){
	ms_message("Network state is now [%s]",isReachable?"UP":"DOWN");
	// second get the list of available proxies
	const MSList *elem=linphone_core_get_proxy_config_list(lc);
	for(;elem!=NULL;elem=elem->next){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		if (linphone_proxy_config_register_enabled(cfg) ) {
			if (!isReachable) {
				linphone_proxy_config_set_state(cfg, LinphoneRegistrationNone,"Registration impossible (network down)");
			}else{
				cfg->commit=TRUE;
			}
		}
	}
	lc->netup_time=curtime;
	lc->network_reachable=isReachable;
	if(!isReachable) {
		sal_unlisten_ports (lc->sal);
	} else {
		apply_transports(lc);
	}

}

void linphone_core_refresh_registers(LinphoneCore* lc) {
	const MSList *elem;
	if (!lc->network_reachable) {
		ms_warning("Refresh register operation not available (network unreachable)");
		return;
	}
	elem=linphone_core_get_proxy_config_list(lc);
	for(;elem!=NULL;elem=elem->next){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		if (linphone_proxy_config_register_enabled(cfg) ) {
			linphone_proxy_config_refresh_register(cfg);
		}
	}
}

void __linphone_core_invalidate_registers(LinphoneCore* lc){
	const MSList *elem=linphone_core_get_proxy_config_list(lc);
	for(;elem!=NULL;elem=elem->next){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		if (linphone_proxy_config_register_enabled(cfg) ) {
			linphone_proxy_config_edit(cfg);
			linphone_proxy_config_done(cfg);
		}
	}
}

void linphone_core_set_network_reachable(LinphoneCore* lc,bool_t isReachable) {
	//first disable automatic mode
	if (lc->auto_net_state_mon) {
		ms_message("Disabling automatic network state monitoring");
		lc->auto_net_state_mon=FALSE;
	}
	set_network_reachable(lc,isReachable, ms_time(NULL));
}

bool_t linphone_core_is_network_reachabled(LinphoneCore* lc) {
	return lc->network_reachable;
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
/**
 * Get the number of Call
 *
 * @ingroup call_control
**/
int linphone_core_get_calls_nb(const LinphoneCore *lc){
	return  ms_list_size(lc->calls);;
}

/**
 * Check if we do not have exceed the number of simultaneous call
 *
 * @ingroup call_control
**/
bool_t linphone_core_can_we_add_call(LinphoneCore *lc)
{
	if(linphone_core_get_calls_nb(lc) < lc->max_calls)
		return TRUE;
	ms_message("Maximum amount of simultaneous calls reached !");
	return FALSE;
}


int linphone_core_add_call( LinphoneCore *lc, LinphoneCall *call)
{
	if(linphone_core_can_we_add_call(lc))
	{
		lc->calls = ms_list_append(lc->calls,call);
		return 0;
	}
	return -1;
}

int linphone_core_del_call( LinphoneCore *lc, LinphoneCall *call)
{
	MSList *it;
	MSList *the_calls = lc->calls;

	it=ms_list_find(the_calls,call);
	if (it)
	{
		the_calls = ms_list_remove_link(the_calls,it);
	}
	else
	{
		ms_warning("could not find the call into the list\n");
		return -1;
	}
	lc->calls = the_calls;
	return 0;
}

/**
 * Specifiies a ring back tone to be played to far end during incoming calls.
**/
void linphone_core_set_remote_ringback_tone(LinphoneCore *lc, const char *file){
	if (lc->sound_conf.ringback_tone){
		ms_free(lc->sound_conf.ringback_tone);
		lc->sound_conf.ringback_tone=NULL;
	}
	if (file)
		lc->sound_conf.ringback_tone=ms_strdup(file);
}

/**
 * Returns the ring back tone played to far end during incoming calls.
**/
const char *linphone_core_get_remote_ringback_tone(const LinphoneCore *lc){
	return lc->sound_conf.ringback_tone;
}

static PayloadType* find_payload_type_from_list(const char* type, int rate,const MSList* from) {
	const MSList *elem;
	for(elem=from;elem!=NULL;elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
		if ((strcmp((char*)type, payload_type_get_mime(pt)) == 0) && (rate == -1 || rate==pt->clock_rate)) {
			return pt;
		}
	}
	return NULL;
}

/**
 * Get payload type  from mime type and clock rate
 * @ingroup media_parameters
 * This function searches in audio and video codecs for the given payload type name and clockrate.
 * Returns NULL if not found.
 */
PayloadType* linphone_core_find_payload_type(LinphoneCore* lc, const char* type, int rate) {
	PayloadType* result = find_payload_type_from_list(type, rate, linphone_core_get_audio_codecs(lc));
	if (result)  {
		return result;
	} else {
		result = find_payload_type_from_list(type, rate, linphone_core_get_video_codecs(lc));
		if (result) {
			return result;
		}
	}
	/*not found*/
	return NULL;
}

const char *linphone_global_state_to_string(LinphoneGlobalState gs){
	switch(gs){
		case LinphoneGlobalOff:
			return "LinphoneGlobalOff";
		break;
		case LinphoneGlobalOn:
			return "LinphoneGlobalOn";
		break;
		case LinphoneGlobalStartup:
			return "LinphoneGlobalStartup";
		break;
		case LinphoneGlobalShutdown:
			return "LinphoneGlobalShutdown";
		break;
	}
	return NULL;
}

LinphoneGlobalState linphone_core_get_global_state(const LinphoneCore *lc){
	return lc->state;
}

LinphoneCallParams *linphone_core_create_default_call_parameters(LinphoneCore *lc){
	LinphoneCallParams *p=ms_new0(LinphoneCallParams,1);
	linphone_core_init_default_params(lc, p);
	return p;
}

const char *linphone_reason_to_string(LinphoneReason err){
	switch(err){
		case LinphoneReasonNone:
			return "No error";
		case LinphoneReasonNoResponse:
			return "No response";
		case LinphoneReasonBadCredentials:
			return "Bad credentials";
		case LinphoneReasonDeclined:
			return "Call declined";
		case LinphoneReasonNotFound:
			return "User not found";
	}
	return "unknown error";
}

const char *linphone_error_to_string(LinphoneReason err){
	return linphone_reason_to_string(err);
}
/**
 * Enables signaling keep alive
 */
void linphone_core_enable_keep_alive(LinphoneCore* lc,bool_t enable) {
	if (enable > 0) {
		sal_set_keepalive_period(lc->sal,lc->sip_conf.keepalive_period);
	} else {
		sal_set_keepalive_period(lc->sal,0);
	}
}
/**
 * Is signaling keep alive enabled
 */
bool_t linphone_core_keep_alive_enabled(LinphoneCore* lc) {
	return sal_get_keepalive_period(lc->sal) > 0;
}

void linphone_core_start_dtmf_stream(LinphoneCore* lc) {
	get_dtmf_gen(lc); /*make sure ring stream is started*/
	lc->ringstream_autorelease=FALSE; /*disable autorelease mode*/
}

void linphone_core_stop_dtmf_stream(LinphoneCore* lc) {
	if (lc->ringstream && lc->dmfs_playing_start_time!=0) {
		ring_stop(lc->ringstream);
		lc->ringstream=NULL;
	}
}

int linphone_core_get_max_calls(LinphoneCore *lc) {
	return lc->max_calls;
}
void linphone_core_set_max_calls(LinphoneCore *lc, int max) {
	lc->max_calls=max;
}

typedef struct Hook{
	LinphoneCoreIterateHook fun;
	void *data;
}Hook;

static Hook *hook_new(LinphoneCoreIterateHook hook, void *hook_data){
	Hook *h=ms_new(Hook,1);
	h->fun=hook;
	h->data=hook_data;
	return h;
}

static void hook_invoke(Hook *h){
	h->fun(h->data);
}

void linphone_core_add_iterate_hook(LinphoneCore *lc, LinphoneCoreIterateHook hook, void *hook_data){
	lc->hooks=ms_list_append(lc->hooks,hook_new(hook,hook_data));
}

static void linphone_core_run_hooks(LinphoneCore *lc){
	ms_list_for_each(lc->hooks,(void (*)(void*))hook_invoke);
}

static void linphone_core_free_hooks(LinphoneCore *lc){
	ms_list_for_each(lc->hooks,(void (*)(void*))ms_free);
	ms_list_free(lc->hooks);
	lc->hooks=NULL;
}

void linphone_core_remove_iterate_hook(LinphoneCore *lc, LinphoneCoreIterateHook hook, void *hook_data){
	MSList *elem;
	for(elem=lc->hooks;elem!=NULL;elem=elem->next){
		Hook *h=(Hook*)elem->data;
		if (h->fun==hook && h->data==hook_data){
			ms_list_remove_link(lc->hooks,elem);
			ms_free(h);
			return;
		}
	}
	ms_error("linphone_core_remove_iterate_hook(): No such hook found.");
}

void linphone_core_set_zrtp_secrets_file(LinphoneCore *lc, const char* file){
	if (lc->zrtp_secrets_cache != NULL) {
		ms_free(lc->zrtp_secrets_cache);
	}
	lc->zrtp_secrets_cache=file ? ms_strdup(file) : NULL;
}

const LinphoneCall* linphone_core_find_call_from_uri(LinphoneCore *lc, const char *uri) {
	if (uri == NULL) return NULL;
	MSList *calls=lc->calls;
	while(calls) {
		const LinphoneCall *c=(LinphoneCall*)calls->data;
		calls=calls->next;
		const LinphoneAddress *address = linphone_call_get_remote_address(c);
		char *current_uri=linphone_address_as_string_uri_only(address);
		if (strcmp(uri,current_uri)==0) {
			ms_free(current_uri);
			return c;
		} else {
			ms_free(current_uri);
		}
	}
	return NULL;
}


/**
 * Check if a call will need the sound resources.
 *
 * @ingroup call_control
 * @param lc The LinphoneCore
**/
bool_t linphone_core_sound_resources_locked(LinphoneCore *lc){
	MSList *calls=lc->calls;
	while(calls) {
		LinphoneCall *c=(LinphoneCall*)calls->data;
		calls=calls->next;
		switch (c->state) {
			case LinphoneCallOutgoingInit:
			case LinphoneCallOutgoingProgress:
			case LinphoneCallOutgoingRinging:
			case LinphoneCallOutgoingEarlyMedia:
			case LinphoneCallConnected:
			case LinphoneCallRefered:
			case LinphoneCallIncomingEarlyMedia:
			case LinphoneCallUpdated:
				return TRUE;
			default:
				break;
		}
	}
	return FALSE;
}

void linphone_core_set_srtp_enabled(LinphoneCore *lc, bool_t enabled) {
	lp_config_set_int(lc->config,"sip","srtp",(int)enabled);
}

/**
 * Returns whether a media encryption scheme is supported by the LinphoneCore engine
**/
bool_t linphone_core_media_encryption_supported(const LinphoneCore *lc, LinphoneMediaEncryption menc){
	switch(menc){
		case LinphoneMediaEncryptionSRTP:
			return ortp_srtp_supported();
		case LinphoneMediaEncryptionZRTP:
			return ortp_zrtp_available();
		case LinphoneMediaEncryptionNone:
			return TRUE;
	}
	return FALSE;
}

int linphone_core_set_media_encryption(LinphoneCore *lc, enum LinphoneMediaEncryption menc) {
	const char *type="none";
	int ret=0;
	if (menc == LinphoneMediaEncryptionSRTP){
		if (!ortp_srtp_supported()){
			ms_warning("SRTP not supported by library.");
			type="none";
			ret=-1;
		}else type="srtp";
	}else if (menc == LinphoneMediaEncryptionZRTP){
		if (!ortp_zrtp_available()){
			ms_warning("ZRTP not supported by library.");
			type="none";
			ret=-1;
		}else type="zrtp";
	}
	lp_config_set_string(lc->config,"sip","media_encryption",type);
	return ret;
}

LinphoneMediaEncryption linphone_core_get_media_encryption(LinphoneCore *lc) {
	const char* menc = lp_config_get_string(lc->config, "sip", "media_encryption", NULL);
	
	if (menc == NULL)
		return LinphoneMediaEncryptionNone;
	else if (strcmp(menc, "srtp")==0)
		return LinphoneMediaEncryptionSRTP;
	else if (strcmp(menc, "zrtp")==0)
		return LinphoneMediaEncryptionZRTP;
	else
		return LinphoneMediaEncryptionNone;
}

bool_t linphone_core_is_media_encryption_mandatory(LinphoneCore *lc) {
	return (bool_t)lp_config_get_int(lc->config, "sip", "media_encryption_mandatory", 0);
}

void linphone_core_set_media_encryption_mandatory(LinphoneCore *lc, bool_t m) {
	lp_config_set_int(lc->config, "sip", "media_encryption_mandatory", (int)m);
}

void linphone_core_init_default_params(LinphoneCore*lc, LinphoneCallParams *params) {
	params->has_video=linphone_core_video_enabled(lc) && lc->video_policy.automatically_initiate;
	params->media_encryption=linphone_core_get_media_encryption(lc);	
	params->in_conference=FALSE;
}

