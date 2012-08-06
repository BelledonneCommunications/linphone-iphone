/***************************************************************************
 *            private.h
 *
 *  Mon Jun 13 14:23:23 2005
 *  Copyright  2005  Simon Morlat
 *  Email simon dot morlat at linphone dot org
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _PRIVATE_H
#define _PRIVATE_H
#ifdef __cplusplus
extern "C" {
#endif
#include "linphonecore.h"
#include "linphone_tunnel.h"
#include "linphonecore_utils.h"
#include "sal.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/msconference.h"

#ifndef LIBLINPHONE_VERSION
#define LIBLINPHONE_VERSION LINPHONE_VERSION
#endif

#ifndef PACKAGE_SOUND_DIR
#define PACKAGE_SOUND_DIR "."
#endif

#ifndef PACKAGE_DATA_DIR
#define PACKAGE_DATA_DIR "."
#endif

#ifdef HAVE_GETTEXT
#include <libintl.h>
#ifndef _
#define _(String) gettext(String)
#endif
#else
#ifndef _
#define _(something)	(something)
#endif
#ifndef ngettext
#define ngettext(singular, plural, number)	(((number)==1)?(singular):(plural))
#endif
#endif

struct _LinphoneCallParams{
	LinphoneCall *referer; /*in case this call creation is consecutive to an incoming transfer, this points to the original call */
	int audio_bw; /* bandwidth limit for audio stream */
	LinphoneMediaEncryption media_encryption;
	PayloadType *audio_codec;
	PayloadType *video_codec;
	bool_t has_video;
	bool_t real_early_media; /*send real media even during early media (for outgoing calls)*/
	bool_t in_conference; /*in conference mode */
	bool_t pad;
	
};
    
typedef struct _CallCallbackObj
{
    LinphoneCallCbFunc _func;
    void * _user_data;
}CallCallbackObj;

static const int linphone_call_magic=0x3343;

struct _LinphoneCall
{
	int magic; /*used to distinguish from proxy config*/
	struct _LinphoneCore *core;
	SalMediaDescription *localdesc;
	SalMediaDescription *resultdesc;
	LinphoneCallDir dir;
	LinphoneCall *referer; /*when this call is the result of a transfer, referer is set to the original call that caused the transfer*/
	struct _RtpProfile *audio_profile;
	struct _RtpProfile *video_profile;
	struct _LinphoneCallLog *log;
	SalOp *op;
	SalOp *ping_op;
	char localip[LINPHONE_IPADDR_SIZE]; /* our best guess for local ipaddress for this call */
	time_t start_time; /*time at which the call was initiated*/
	time_t media_start_time; /*time at which it was accepted, media streams established*/
	LinphoneCallState	state;
	LinphoneCallState transfer_state; /*idle if no transfer*/
	LinphoneReason reason;
	int refcnt;
	void * user_pointer;
	int audio_port;
	int video_port;
	struct _AudioStream *audiostream;  /**/
	struct _VideoStream *videostream;
	MSAudioEndpoint *endpoint; /*used for conferencing*/
	char *refer_to;
	LinphoneCallParams params;
	LinphoneCallParams current_params;
	LinphoneCallParams remote_params;
	int up_bw; /*upload bandwidth setting at the time the call is started. Used to detect if it changes during a call */
	int audio_bw;	/*upload bandwidth used by audio */
	bool_t refer_pending;
	bool_t media_pending;
	bool_t audio_muted;
	bool_t camera_active;
	bool_t all_muted; /*this flag is set during early medias*/
	bool_t playing_ringbacktone;
	bool_t owns_call_log;
	bool_t ringing_beep; /* whether this call is ringing through an already existent current call*/
	OrtpEvQueue *audiostream_app_evq;
	char *auth_token;
	OrtpEvQueue *videostream_app_evq;
	bool_t videostream_encrypted;
	bool_t audiostream_encrypted;
	bool_t auth_token_verified;
	bool_t defer_update;
	bool_t was_automatically_paused;
	CallCallbackObj nextVideoFrameDecoded;
	LinphoneCallStats stats[2];
};


LinphoneCall * linphone_call_new_outgoing(struct _LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to, const LinphoneCallParams *params);
LinphoneCall * linphone_call_new_incoming(struct _LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to, SalOp *op);
void linphone_call_set_state(LinphoneCall *call, LinphoneCallState cstate, const char *message);

/* private: */
LinphoneCallLog * linphone_call_log_new(LinphoneCall *call, LinphoneAddress *local, LinphoneAddress * remote);
void linphone_call_log_completed(LinphoneCall *call);
void linphone_call_log_destroy(LinphoneCallLog *cl);
void linphone_call_set_transfer_state(LinphoneCall* call, LinphoneCallState state);

void linphone_auth_info_write_config(struct _LpConfig *config, LinphoneAuthInfo *obj, int pos);

void linphone_core_update_proxy_register(LinphoneCore *lc);
void linphone_core_refresh_subscribes(LinphoneCore *lc);
int linphone_core_abort_call(LinphoneCore *lc, LinphoneCall *call, const char *error);

int linphone_proxy_config_send_publish(LinphoneProxyConfig *cfg, LinphoneOnlineStatus os);
void linphone_proxy_config_set_state(LinphoneProxyConfig *cfg, LinphoneRegistrationState rstate, const char *message);

int linphone_online_status_to_eXosip(LinphoneOnlineStatus os);
void linphone_friend_close_subscriptions(LinphoneFriend *lf);
void linphone_friend_notify(LinphoneFriend *lf, LinphoneOnlineStatus os);
LinphoneFriend *linphone_find_friend_by_inc_subscribe(MSList *l, SalOp *op);
LinphoneFriend *linphone_find_friend_by_out_subscribe(MSList *l, SalOp *op);

int parse_hostname_to_addr(const char *server, struct sockaddr_storage *ss, socklen_t *socklen);
int set_lock_file();
int get_lock_file();
int remove_lock_file();
int do_registration(LinphoneCore *lc, bool_t doit);
void check_for_registration(LinphoneCore *lc);
void check_sound_device(LinphoneCore *lc);
void linphone_core_verify_codecs(LinphoneCore *lc);
void linphone_core_get_local_ip(LinphoneCore *lc, const char *to, char *result);
bool_t host_has_ipv6_network();
bool_t lp_spawn_command_line_sync(const char *command, char **result,int *command_ret);

static inline int get_min_bandwidth(int dbw, int ubw){
	if (dbw<=0) return ubw;
	if (ubw<=0) return dbw;
	return MIN(dbw,ubw);
}

static inline bool_t bandwidth_is_greater(int bw1, int bw2){
	if (bw1<0) return TRUE;
	else if (bw2<0) return FALSE;
	else return bw1>=bw2;
}

static inline int get_video_bandwidth(int total, int audio){
	if (total<=0) return 0;
	return total-audio-10;
}

static inline void set_string(char **dest, const char *src){
	if (*dest){
		ms_free(*dest);
		*dest=NULL;
	}
	if (src)
		*dest=ms_strdup(src);
}

#define PAYLOAD_TYPE_ENABLED	PAYLOAD_TYPE_USER_FLAG_0

SalPresenceStatus linphone_online_status_to_sal(LinphoneOnlineStatus os);
void linphone_process_authentication(LinphoneCore* lc, SalOp *op);
void linphone_authentication_ok(LinphoneCore *lc, SalOp *op);
void linphone_subscription_new(LinphoneCore *lc, SalOp *op, const char *from);
void linphone_notify_recv(LinphoneCore *lc, SalOp *op, SalSubscribeStatus ss, SalPresenceStatus status);
void linphone_proxy_config_process_authentication_failure(LinphoneCore *lc, SalOp *op);

void linphone_subscription_answered(LinphoneCore *lc, SalOp *op);
void linphone_subscription_closed(LinphoneCore *lc, SalOp *op);

MSList *linphone_find_friend(MSList *fl, const LinphoneAddress *fri, LinphoneFriend **lf);

void linphone_core_update_allocated_audio_bandwidth(LinphoneCore *lc);
void linphone_core_update_allocated_audio_bandwidth_in_call(LinphoneCall *call, const PayloadType *pt);
void linphone_core_run_stun_tests(LinphoneCore *lc, LinphoneCall *call);

void linphone_core_send_initial_subscribes(LinphoneCore *lc);
void linphone_core_write_friends_config(LinphoneCore* lc);
void linphone_friend_write_to_config_file(struct _LpConfig *config, LinphoneFriend *lf, int index);
LinphoneFriend * linphone_friend_new_from_config_file(struct _LinphoneCore *lc, int index);

void linphone_proxy_config_update(LinphoneProxyConfig *cfg);
void linphone_proxy_config_get_contact(LinphoneProxyConfig *cfg, const char **ip, int *port);
LinphoneProxyConfig * linphone_core_lookup_known_proxy(LinphoneCore *lc, const LinphoneAddress *uri);
const char *linphone_core_find_best_identity(LinphoneCore *lc, const LinphoneAddress *to, const char **route);
int linphone_core_get_local_ip_for(int type, const char *dest, char *result);

LinphoneProxyConfig *linphone_proxy_config_new_from_config_file(struct _LpConfig *config, int index);
void linphone_proxy_config_write_to_config_file(struct _LpConfig* config,LinphoneProxyConfig *obj, int index);

int linphone_proxy_config_normalize_number(LinphoneProxyConfig *cfg, const char *username, char *result, size_t result_len);

void linphone_core_text_received(LinphoneCore *lc, const char *from, const char *msg);

void linphone_core_play_tone(LinphoneCore *lc);

void linphone_call_init_stats(LinphoneCallStats *stats, int type);

void linphone_call_init_media_streams(LinphoneCall *call);
void linphone_call_start_media_streams(LinphoneCall *call, bool_t all_inputs_muted, bool_t send_ringbacktone);
void linphone_call_stop_media_streams(LinphoneCall *call);

const char * linphone_core_get_identity(LinphoneCore *lc);
const char * linphone_core_get_route(LinphoneCore *lc);
void linphone_core_start_waiting(LinphoneCore *lc, const char *purpose);
void linphone_core_update_progress(LinphoneCore *lc, const char *purpose, float progresses);
void linphone_core_stop_waiting(LinphoneCore *lc);

int linphone_core_start_invite(LinphoneCore *lc, LinphoneCall *call, LinphoneProxyConfig *dest_proxy);
void linphone_core_start_refered_call(LinphoneCore *lc, LinphoneCall *call);
extern SalCallbacks linphone_sal_callbacks;
void linphone_proxy_config_set_error(LinphoneProxyConfig *cfg, LinphoneReason error);
bool_t linphone_core_rtcp_enabled(const LinphoneCore *lc);

LinphoneCall * is_a_linphone_call(void *user_pointer);
LinphoneProxyConfig * is_a_linphone_proxy_config(void *user_pointer);

static const int linphone_proxy_config_magic=0x7979;

struct _LinphoneProxyConfig
{
	int magic;
	struct _LinphoneCore *lc;
	char *reg_proxy;
	char *reg_identity;
	char *reg_route;
	char *realm;
	char *contact_params;
	int expires;
	int reg_time;
	SalOp *op;
	char *type;
	struct _SipSetupContext *ssctx;
	int auth_failures;
	char *dial_prefix;
	LinphoneRegistrationState state;
	SalOp *publish_op;
	bool_t commit;
	bool_t reg_sendregister;
	bool_t publish;
	bool_t dial_escape_plus;
	void* user_data;
	time_t deletion_date;
	LinphoneReason error;
};

struct _LinphoneAuthInfo
{
	char *username;
	char *realm;
	char *userid;
	char *passwd;
	char *ha1;
	int usecount;
	time_t last_use_time;
	bool_t works;
};

struct _LinphoneChatRoom{
	struct _LinphoneCore *lc;
	char  *peer;
	LinphoneAddress *peer_url;
	SalOp *op;
	void * user_data;
};

struct _LinphoneFriend{
	LinphoneAddress *uri;
	SalOp *insub;
	SalOp *outsub;
	LinphoneSubscribePolicy pol;
	LinphoneOnlineStatus status;
	struct _LinphoneCore *lc;
	BuddyInfo *info;
	char *refkey;
	bool_t subscribe;
	bool_t subscribe_active;
	bool_t inc_subscribe_pending;
	bool_t commit;
};


typedef struct sip_config
{
	char *contact;
	char *guessed_contact;
	MSList *proxies;
	MSList *deleted_proxies;
	int inc_timeout;	/*timeout after an un-answered incoming call is rejected*/
	unsigned int keepalive_period; /* interval in ms between keep alive messages sent to the proxy server*/
	LCSipTransports transports;
	bool_t use_info;
	bool_t use_rfc2833;	/*force RFC2833 to be sent*/
	bool_t guess_hostname;
	bool_t loopback_only;
	bool_t ipv6_enabled;
	bool_t sdp_200_ack;
	bool_t register_only_when_network_is_up;
	bool_t ping_with_options;
	bool_t auto_net_state_mon;
} sip_config_t;

typedef struct rtp_config
{
	int audio_rtp_port;
	int video_rtp_port;
	int audio_jitt_comp;  /*jitter compensation*/
	int video_jitt_comp;  /*jitter compensation*/
	int nortp_timeout;
	bool_t rtp_no_xmit_on_audio_mute;
                              /* stop rtp xmit when audio muted */
}rtp_config_t;



typedef struct net_config
{
	char *nat_address; /* may be IP or host name */
	char *nat_address_ip; /* ip translated from nat_address */
	char *stun_server;
	char *relay;
	int download_bw;
	int upload_bw;
	int firewall_policy;
	int mtu;
	int down_ptime;
	bool_t nat_sdp_only;
}net_config_t;


typedef struct sound_config
{
	struct _MSSndCard * ring_sndcard;	/* the playback sndcard currently used */
	struct _MSSndCard * play_sndcard;	/* the playback sndcard currently used */
	struct _MSSndCard * capt_sndcard; /* the capture sndcard currently used */
	struct _MSSndCard * lsd_card; /* dummy playback card for Linphone Sound Daemon extension */
	const char **cards;
	int latency;	/* latency in samples of the current used sound device */
	char rec_lev;
	char play_lev;
	char ring_lev;
	char soft_play_lev;
	char source;
	char *local_ring;
	char *remote_ring;
	char *ringback_tone;
	bool_t ec;
	bool_t ea;
	bool_t agc;
} sound_config_t;

typedef struct codecs_config
{
	MSList *audio_codecs;  /* list of audio codecs in order of preference*/
	MSList *video_codecs;	/* for later use*/
}codecs_config_t;

typedef struct video_config{
	struct _MSWebCam *device;
	const char **cams;
	MSVideoSize vsize;
	bool_t capture;
	bool_t show_local;
	bool_t display;
	bool_t selfview; /*during calls*/
	const char *displaytype;
}video_config_t;

typedef struct ui_config
{
	int is_daemon;
	int is_applet;
	unsigned int timer_id;  /* the timer id for registration */
}ui_config_t;



typedef struct autoreplier_config
{
	int enabled;
	int after_seconds;		/* accept the call after x seconds*/
	int max_users;			/* maximum number of user that can call simultaneously */
	int max_rec_time;  	/* the max time of incoming voice recorded */
	int max_rec_msg;		/* maximum number of recorded messages */
	const char *message;		/* the path of the file to be played */
}autoreplier_config_t;

struct _LinphoneConference{
	MSAudioConference *conf;
	AudioStream *local_participant;
	MSAudioEndpoint *local_endpoint;
	RtpProfile *local_dummy_profile;
	bool_t local_muted;
};

typedef struct _LinphoneConference LinphoneConference;

struct _LinphoneCore
{
	LinphoneCoreVTable vtable;
	Sal *sal;
	LinphoneGlobalState state;
	struct _LpConfig *config;
	net_config_t net_conf;
	sip_config_t sip_conf;
	rtp_config_t rtp_conf;
	sound_config_t sound_conf;
	video_config_t video_conf;
	codecs_config_t codecs_conf;
	ui_config_t ui_conf;
	autoreplier_config_t autoreplier_conf;
	MSList *payload_types;
	int dyn_pt;
	LinphoneProxyConfig *default_proxy;
	MSList *friends;
	MSList *auth_info;
	struct _RingStream *ringstream;
	time_t dmfs_playing_start_time;
	LCCallbackObj preview_finished_cb;
	LinphoneCall *current_call;   /* the current call */
	MSList *calls;				/* all the processed calls */
	MSList *queued_calls;	/* used by the autoreplier */
	MSList *call_logs;
	MSList *chatrooms;
	int max_call_logs;
	int missed_calls;
	VideoPreview *previewstream;
	struct _MSEventQueue *msevq;
	LinphoneRtpTransportFactories *rtptf;
	MSList *bl_reqs;
	MSList *subscribers;	/* unknown subscribers */
	int minutes_away;
	LinphoneOnlineStatus presence_mode;
	char *alt_contact;
	void *data;
	char *play_file;
	char *rec_file;
	time_t prevtime;
	int audio_bw;
	LinphoneWaitingCallback wait_cb;
	void *wait_ctx;
	unsigned long video_window_id;
	unsigned long preview_window_id;
	time_t netup_time; /*time when network went reachable */
	struct _EcCalibrator *ecc;
	MSList *hooks;
	LinphoneConference conf_ctx;
	char* zrtp_secrets_cache;
	LinphoneVideoPolicy video_policy;
	bool_t use_files;
	bool_t apply_nat_settings;
	bool_t initial_subscribes_sent;
	bool_t bl_refresh;
	
	bool_t preview_finished;
	bool_t auto_net_state_mon;
	bool_t network_reachable;
	bool_t use_preview_window;
	
	bool_t ringstream_autorelease;
	bool_t pad[3];
	int device_rotation;
	int max_calls;
	LinphoneTunnel *tunnel;
};

LinphoneTunnel *linphone_core_tunnel_new(LinphoneCore *lc);
void linphone_tunnel_destroy(LinphoneTunnel *tunnel);
void linphone_tunnel_configure(LinphoneTunnel *tunnel);
void linphone_tunnel_enable_logs_with_handler(LinphoneTunnel *tunnel, bool_t enabled, OrtpLogFunc logHandler);
	
bool_t linphone_core_can_we_add_call(LinphoneCore *lc);
int linphone_core_add_call( LinphoneCore *lc, LinphoneCall *call);
int linphone_core_del_call( LinphoneCore *lc, LinphoneCall *call);
int linphone_core_set_as_current_call(LinphoneCore *lc, LinphoneCall *call);
int linphone_core_get_calls_nb(const LinphoneCore *lc);

void linphone_core_set_state(LinphoneCore *lc, LinphoneGlobalState gstate, const char *message);

SalMediaDescription *create_local_media_description(LinphoneCore *lc, LinphoneCall *call);
void update_local_media_description(LinphoneCore *lc, LinphoneCall *call);

void linphone_core_update_streams(LinphoneCore *lc, LinphoneCall *call, SalMediaDescription *new_md);

bool_t linphone_core_is_payload_type_usable_for_bandwidth(LinphoneCore *lc, PayloadType *pt,  int bandwidth_limit);

#define linphone_core_ready(lc) ((lc)->state!=LinphoneGlobalStartup)
void _linphone_core_configure_resolver();

struct _EcCalibrator{
	ms_thread_t thread;
	MSSndCard *play_card,*capt_card;
	MSFilter *sndread,*det,*rec;
	MSFilter *play, *gen, *sndwrite,*resampler;
	MSTicker *ticker;
	LinphoneEcCalibrationCallback cb;
	void *cb_data;
	int recv_count;
	int sent_count;
	int64_t acc;
	int delay;
	unsigned int rate;
	LinphoneEcCalibratorStatus status;
};

typedef struct _EcCalibrator EcCalibrator;

LinphoneEcCalibratorStatus ec_calibrator_get_status(EcCalibrator *ecc);

void ec_calibrator_destroy(EcCalibrator *ecc);

void linphone_call_background_tasks(LinphoneCall *call, bool_t one_second_elapsed);
void linphone_core_preempt_sound_resources(LinphoneCore *lc);
/*conferencing subsystem*/
void _post_configure_audio_stream(AudioStream *st, LinphoneCore *lc, bool_t muted);
/* When a conference participant pause the conference he may send a music.
 * We don't want to hear that music or to send it to the other participants.
 * Use muted=yes to handle this case.
 */
void linphone_call_add_to_conf(LinphoneCall *call, bool_t muted);
void linphone_call_remove_from_conf(LinphoneCall *call);
void linphone_core_conference_check_uninit(LinphoneCore *lc);
bool_t linphone_core_sound_resources_available(LinphoneCore *lc);
void linphone_core_notify_refer_state(LinphoneCore *lc, LinphoneCall *referer, LinphoneCall *newcall);

void __linphone_core_invalidate_registers(LinphoneCore* lc);
void _linphone_core_codec_config_write(LinphoneCore *lc);

#define HOLD_OFF	(0)
#define HOLD_ON		(1)

#ifndef NB_MAX_CALLS
#define NB_MAX_CALLS	(10)
#endif
void call_logs_write_to_config_file(LinphoneCore *lc);



#ifdef __cplusplus
}
#endif

#endif /* _PRIVATE_H */
