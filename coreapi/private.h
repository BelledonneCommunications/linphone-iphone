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
#include "linphonefriend.h"
#include "linphone_tunnel.h"
#include "linphonecore_utils.h"
#include "sal/sal.h"
#include "sipsetup.h"

#include <belle-sip/object.h>
#include <belle-sip/dict.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "mediastreamer2/ice.h"
#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/msconference.h"
#ifdef BUILD_UPNP
#include "upnp.h"
#endif //BUILD_UPNP

#ifdef MSG_STORAGE_ENABLED
#include "sqlite3.h"
#endif

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
#define _(String) dgettext(GETTEXT_PACKAGE,String)
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
	PayloadType *audio_codec; /*audio codec currently in use */
	PayloadType *video_codec; /*video codec currently in use */
	MSVideoSize sent_vsize; /* Size of the video currently being sent */
	MSVideoSize recv_vsize; /* Size of the video currently being received */
	int down_bw;
	int up_bw;
	int down_ptime;
	int up_ptime;
	char *record_file;
	char *session_name;
	SalCustomHeader *custom_headers;
	bool_t has_video;
	bool_t real_early_media; /*send real media even during early media (for outgoing calls)*/
	bool_t in_conference; /*in conference mode */
	bool_t low_bandwidth;
	LinphonePrivacyMask privacy;
};

struct _LinphoneCallLog{
	struct _LinphoneCore *lc;
	LinphoneCallDir dir; /**< The direction of the call*/
	LinphoneCallStatus status; /**< The status of the call*/
	LinphoneAddress *from; /**<Originator of the call as a LinphoneAddress object*/
	LinphoneAddress *to; /**<Destination of the call as a LinphoneAddress object*/
	char start_date[128]; /**<Human readable string containing the start date*/
	int duration; /**<Duration of the call in seconds*/
	char *refkey;
	void *user_pointer;
	rtp_stats_t local_stats;
	rtp_stats_t remote_stats;
	float quality;
	time_t start_date_time; /**Start date of the call in seconds as expressed in a time_t */
	char* call_id; /**unique id of a call*/
	bool_t video_enabled;
};

typedef struct _CallCallbackObj
{
	LinphoneCallCbFunc _func;
	void * _user_data;
}CallCallbackObj;

static const int linphone_call_magic=0x3343;

typedef enum _LinphoneChatMessageDir{
	LinphoneChatMessageIncoming,
	LinphoneChatMessageOutgoing
} LinphoneChatMessageDir;

struct _LinphoneChatMessage {
	LinphoneChatRoom* chat_room;
	LinphoneChatMessageDir dir;
	char* message;
	LinphoneChatMessageStateChangedCb cb;
	void* cb_ud;
	void* message_userdata;
	char* external_body_url;
	LinphoneAddress *from;
	LinphoneAddress *to;
	time_t time;
	SalCustomHeader *custom_headers;
	LinphoneChatMessageState state;
	bool_t is_read;
	unsigned int storage_id;
	LinphoneReason reason;
};

typedef struct StunCandidate{
	char addr[64];
	int port;
}StunCandidate;


struct _LinphoneCall
{
	int magic; /*used to distinguish from proxy config*/
	struct _LinphoneCore *core;
	int af; /*the address family to prefer for RTP path, guessed from signaling path*/
	LinphoneCallDir dir;
	SalMediaDescription *biggestdesc; /*media description with all already proposed streams, used to remember the mapping of streams*/
	SalMediaDescription *localdesc;
	SalMediaDescription *resultdesc;
	struct _RtpProfile *audio_profile;
	struct _RtpProfile *video_profile;
	struct _LinphoneCallLog *log;
	SalOp *op;
	SalOp *ping_op;
	char localip[LINPHONE_IPADDR_SIZE]; /* our best guess for local ipaddress for this call */
	time_t media_start_time; /*time at which it was accepted, media streams established*/
	LinphoneCallState state;
	LinphoneCallState prevstate;
	LinphoneCallState transfer_state; /*idle if no transfer*/
	LinphoneReason reason;
	LinphoneProxyConfig *dest_proxy;
	int refcnt;
	void * user_pointer;
	int audio_port;
	int video_port;
	StunCandidate ac,vc; /*audio video ip/port discovered by STUN*/
	struct _AudioStream *audiostream;  /**/
	struct _VideoStream *videostream;
	MSAudioEndpoint *endpoint; /*used for conferencing*/
	char *refer_to;
	LinphoneCallParams params;
	LinphoneCallParams current_params;
	LinphoneCallParams remote_params;
	int up_bw; /*upload bandwidth setting at the time the call is started. Used to detect if it changes during a call */
	int audio_bw;	/*upload bandwidth used by audio */
	OrtpEvQueue *audiostream_app_evq;
	char *auth_token;
	OrtpEvQueue *videostream_app_evq;
	CallCallbackObj nextVideoFrameDecoded;
	LinphoneCallStats stats[2];
#ifdef BUILD_UPNP
	UpnpSession *upnp_session;
#endif //BUILD_UPNP
	IceSession *ice_session;
	LinphoneChatMessage* pending_message;
	int ping_time;
	unsigned int remote_session_id;
	unsigned int remote_session_ver;
	LinphoneCall *referer; /*when this call is the result of a transfer, referer is set to the original call that caused the transfer*/
	LinphoneCall *transfer_target;/*if this call received a transfer request, then transfer_target points to the new call created to the refer target */
	int localdesc_changed;

	bool_t refer_pending;
	bool_t expect_media_in_ack;
	bool_t audio_muted;
	bool_t camera_enabled;

	bool_t all_muted; /*this flag is set during early medias*/
	bool_t playing_ringbacktone;
	bool_t owns_call_log;
	bool_t ringing_beep; /* whether this call is ringing through an already existent current call*/

	bool_t videostream_encrypted;
	bool_t audiostream_encrypted;
	bool_t auth_token_verified;
	bool_t defer_update;

	bool_t was_automatically_paused;
	bool_t ping_replied;
	bool_t record_active;
	bool_t paused_by_app;
};


LinphoneCall * linphone_call_new_outgoing(struct _LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to, const LinphoneCallParams *params, LinphoneProxyConfig *cfg);
LinphoneCall * linphone_call_new_incoming(struct _LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to, SalOp *op);
void linphone_call_set_state(LinphoneCall *call, LinphoneCallState cstate, const char *message);
void linphone_call_set_contact_op(LinphoneCall* call);
/* private: */
LinphoneCallLog * linphone_call_log_new(LinphoneCall *call, LinphoneAddress *local, LinphoneAddress * remote);
void linphone_call_log_completed(LinphoneCall *call);
void linphone_call_log_destroy(LinphoneCallLog *cl);
void linphone_call_set_transfer_state(LinphoneCall* call, LinphoneCallState state);

void linphone_auth_info_write_config(struct _LpConfig *config, LinphoneAuthInfo *obj, int pos);

void linphone_core_update_proxy_register(LinphoneCore *lc);
void linphone_core_refresh_subscribes(LinphoneCore *lc);
int linphone_core_abort_call(LinphoneCore *lc, LinphoneCall *call, const char *error);
const char *linphone_core_get_nat_address_resolved(LinphoneCore *lc);

int linphone_proxy_config_send_publish(LinphoneProxyConfig *cfg, LinphonePresenceModel *presence);
void linphone_proxy_config_set_state(LinphoneProxyConfig *cfg, LinphoneRegistrationState rstate, const char *message);
void linphone_proxy_config_stop_refreshing(LinphoneProxyConfig *obj);
void linphone_proxy_config_write_all_to_config_file(LinphoneCore *lc);
/*
 * returns service route as defined in as defined by rfc3608, might be a list instead of just one.
 * Can be NULL
 * */
const LinphoneAddress* linphone_proxy_config_get_service_route(const LinphoneProxyConfig* cfg);


int linphone_online_status_to_eXosip(LinphoneOnlineStatus os);
void linphone_friend_close_subscriptions(LinphoneFriend *lf);
void linphone_friend_notify(LinphoneFriend *lf, LinphonePresenceModel *presence);
LinphoneFriend *linphone_find_friend_by_inc_subscribe(MSList *l, SalOp *op);
LinphoneFriend *linphone_find_friend_by_out_subscribe(MSList *l, SalOp *op);
MSList *linphone_find_friend_by_address(MSList *fl, const LinphoneAddress *addr, LinphoneFriend **lf);

int parse_hostname_to_addr(const char *server, struct sockaddr_storage *ss, socklen_t *socklen, int default_port);

void linphone_core_get_local_ip(LinphoneCore *lc, int af, char *result);
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

void linphone_process_authentication(LinphoneCore* lc, SalOp *op);
void linphone_authentication_ok(LinphoneCore *lc, SalOp *op);
void linphone_subscription_new(LinphoneCore *lc, SalOp *op, const char *from);
void linphone_core_send_presence(LinphoneCore *lc, LinphonePresenceModel *presence);
void linphone_notify_parse_presence(SalOp *op, const char *content_type, const char *content_subtype, const char *body, SalPresenceModel **result);
void linphone_notify_convert_presence_to_xml(SalOp *op, SalPresenceModel *presence, const char *contact, char **content);
void linphone_notify_recv(LinphoneCore *lc, SalOp *op, SalSubscribeStatus ss, SalPresenceModel *model);
void linphone_proxy_config_process_authentication_failure(LinphoneCore *lc, SalOp *op);

void linphone_subscription_answered(LinphoneCore *lc, SalOp *op);
void linphone_subscription_closed(LinphoneCore *lc, SalOp *op);

void linphone_core_update_allocated_audio_bandwidth(LinphoneCore *lc);
void linphone_core_update_allocated_audio_bandwidth_in_call(LinphoneCall *call, const PayloadType *pt);

int linphone_core_run_stun_tests(LinphoneCore *lc, LinphoneCall *call);
void linphone_core_resolve_stun_server(LinphoneCore *lc);
const struct addrinfo *linphone_core_get_stun_server_addrinfo(LinphoneCore *lc);
void linphone_core_adapt_to_network(LinphoneCore *lc, int ping_time_ms, LinphoneCallParams *params);
int linphone_core_gather_ice_candidates(LinphoneCore *lc, LinphoneCall *call);
void linphone_core_update_ice_state_in_call_stats(LinphoneCall *call);
void linphone_core_update_local_media_description_from_ice(SalMediaDescription *desc, IceSession *session);
void linphone_core_update_ice_from_remote_media_description(LinphoneCall *call, const SalMediaDescription *md);
bool_t linphone_core_media_description_contains_video_stream(const SalMediaDescription *md);
bool_t linphone_core_media_description_has_srtp(const SalMediaDescription *md);

void linphone_core_send_initial_subscribes(LinphoneCore *lc);
void linphone_core_write_friends_config(LinphoneCore* lc);
void linphone_friend_write_to_config_file(struct _LpConfig *config, LinphoneFriend *lf, int index);
LinphoneFriend * linphone_friend_new_from_config_file(struct _LinphoneCore *lc, int index);

void linphone_proxy_config_update(LinphoneProxyConfig *cfg);
void linphone_proxy_config_get_contact(LinphoneProxyConfig *cfg, const char **ip, int *port);
LinphoneProxyConfig * linphone_core_lookup_known_proxy(LinphoneCore *lc, const LinphoneAddress *uri);
const char *linphone_core_find_best_identity(LinphoneCore *lc, const LinphoneAddress *to);
int linphone_core_get_local_ip_for(int type, const char *dest, char *result);

LinphoneProxyConfig *linphone_proxy_config_new_from_config_file(struct _LpConfig *config, int index);
void linphone_proxy_config_write_to_config_file(struct _LpConfig* config,LinphoneProxyConfig *obj, int index);

int linphone_proxy_config_normalize_number(LinphoneProxyConfig *cfg, const char *username, char *result, size_t result_len);

void linphone_core_message_received(LinphoneCore *lc, SalOp *op, const SalMessage *msg);
void linphone_core_is_composing_received(LinphoneCore *lc, SalOp *op, const SalIsComposing *is_composing);

void linphone_core_play_tone(LinphoneCore *lc);

void linphone_call_init_stats(LinphoneCallStats *stats, int type);
void linphone_call_fix_call_parameters(LinphoneCall *call);
void linphone_call_init_audio_stream(LinphoneCall *call);
void linphone_call_init_video_stream(LinphoneCall *call);
void linphone_call_init_media_streams(LinphoneCall *call);
void linphone_call_start_media_streams(LinphoneCall *call, bool_t all_inputs_muted, bool_t send_ringbacktone);
void linphone_call_start_media_streams_for_ice_gathering(LinphoneCall *call);
void linphone_call_stop_audio_stream(LinphoneCall *call);
void linphone_call_stop_video_stream(LinphoneCall *call);
void linphone_call_stop_media_streams(LinphoneCall *call);
void linphone_call_delete_ice_session(LinphoneCall *call);
void linphone_call_delete_upnp_session(LinphoneCall *call);
void linphone_call_stop_media_streams_for_ice_gathering(LinphoneCall *call);
void linphone_call_update_crypto_parameters(LinphoneCall *call, SalMediaDescription *old_md, SalMediaDescription *new_md);
void linphone_call_update_remote_session_id_and_ver(LinphoneCall *call);

const char * linphone_core_get_identity(LinphoneCore *lc);

void linphone_core_start_waiting(LinphoneCore *lc, const char *purpose);
void linphone_core_update_progress(LinphoneCore *lc, const char *purpose, float progresses);
void linphone_core_stop_waiting(LinphoneCore *lc);

int linphone_core_proceed_with_invite_if_ready(LinphoneCore *lc, LinphoneCall *call, LinphoneProxyConfig *dest_proxy);
int linphone_core_start_invite(LinphoneCore *lc, LinphoneCall *call, const LinphoneAddress* destination/* = NULL if to be taken from the call log */);
int linphone_core_restart_invite(LinphoneCore *lc, LinphoneCall *call);
int linphone_core_start_update_call(LinphoneCore *lc, LinphoneCall *call);
int linphone_core_start_accept_call_update(LinphoneCore *lc, LinphoneCall *call);
void linphone_core_notify_incoming_call(LinphoneCore *lc, LinphoneCall *call);
bool_t linphone_core_incompatible_security(LinphoneCore *lc, SalMediaDescription *md);
extern SalCallbacks linphone_sal_callbacks;
void linphone_proxy_config_set_error(LinphoneProxyConfig *cfg, LinphoneReason error);
bool_t linphone_core_rtcp_enabled(const LinphoneCore *lc);

LinphoneCall * is_a_linphone_call(void *user_pointer);
LinphoneProxyConfig * is_a_linphone_proxy_config(void *user_pointer);

void linphone_core_queue_task(LinphoneCore *lc, belle_sip_source_func_t task_fun, void *data, const char *task_description);

static const int linphone_proxy_config_magic=0x7979;

/*chat*/
void linphone_chat_message_destroy(LinphoneChatMessage* msg);
/**/

struct _LinphoneProxyConfig
{
	int magic;
	struct _LinphoneCore *lc;
	char *reg_proxy;
	char *reg_identity;
	char *reg_route;
	char *realm;
	char *contact_params;
	char *contact_uri_params;
	int expires;
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
	bool_t send_publish;
	bool_t pad[3];
	void* user_data;
	time_t deletion_date;
	LinphoneReason error;
	LinphonePrivacyMask privacy;
};

struct _LinphoneAuthInfo
{
	char *username;
	char *realm;
	char *userid;
	char *passwd;
	char *ha1;
	char *domain;
};

typedef enum _LinphoneIsComposingState {
	LinphoneIsComposingIdle,
	LinphoneIsComposingActive
} LinphoneIsComposingState;

struct _LinphoneChatRoom{
	struct _LinphoneCore *lc;
	char  *peer;
	LinphoneAddress *peer_url;
	void * user_data;
	MSList *messages_hist;
	LinphoneIsComposingState remote_is_composing;
	LinphoneIsComposingState is_composing;
	belle_sip_source_t *remote_composing_refresh_timer;
	belle_sip_source_t *composing_idle_timer;
	belle_sip_source_t *composing_refresh_timer;
};



struct _LinphoneFriend{
	LinphoneAddress *uri;
	SalOp *insub;
	SalOp *outsub;
	LinphoneSubscribePolicy pol;
	LinphonePresenceModel *presence;
	struct _LinphoneCore *lc;
	BuddyInfo *info;
	char *refkey;
	void *up;
	bool_t subscribe;
	bool_t subscribe_active;
	bool_t inc_subscribe_pending;
	bool_t commit;
	bool_t initial_subscribes_sent; /*used to know if initial subscribe message was sent or not*/
};


typedef struct sip_config
{
	char *contact;
	char *guessed_contact;
	MSList *proxies;
	MSList *deleted_proxies;
	int inc_timeout;	/*timeout after an un-answered incoming call is rejected*/
	int in_call_timeout;	/*timeout after a call is hangup */
	int delayed_timeout; 	/*timeout after a delayed call is resumed */
	unsigned int keepalive_period; /* interval in ms between keep alive messages sent to the proxy server*/
	LCSipTransports transports;
	bool_t guess_hostname;
	bool_t loopback_only;
	bool_t ipv6_enabled;
	bool_t sdp_200_ack;
	bool_t register_only_when_network_is_up;
	bool_t register_only_when_upnp_is_ok;
	bool_t ping_with_options;
	bool_t auto_net_state_mon;
	bool_t tcp_tls_keepalive;
} sip_config_t;

typedef struct rtp_config
{
	int audio_rtp_min_port;
	int audio_rtp_max_port;
	int video_rtp_min_port;
	int video_rtp_max_port;
	int audio_jitt_comp;  /*jitter compensation*/
	int video_jitt_comp;  /*jitter compensation*/
	int nortp_timeout;
	int disable_upnp;
	bool_t rtp_no_xmit_on_audio_mute;
							  /* stop rtp xmit when audio muted */
	bool_t audio_adaptive_jitt_comp_enabled;
	bool_t video_adaptive_jitt_comp_enabled;
	bool_t pad;
}rtp_config_t;



typedef struct net_config
{
	char *nat_address; /* may be IP or host name */
	char *nat_address_ip; /* ip translated from nat_address */
	char *stun_server;
	struct addrinfo *stun_addrinfo;
	SalResolverContext * stun_res;
	int download_bw;
	int upload_bw;
	int mtu;
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
	float soft_play_lev; /*playback gain in db.*/
	float soft_mic_lev; /*mic gain in db.*/
	char rec_lev;
	char play_lev;
	char ring_lev;
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
	MSAudioEndpoint *record_endpoint;
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
	RtpProfile *default_profile;
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
	LinphonePresenceModel *presence_model;
	void *data;
	char *play_file;
	char *rec_file;
	time_t prevtime;
	int audio_bw;
	LinphoneCoreWaitingCallback wait_cb;
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

	time_t network_last_check;
	bool_t network_last_status;

	bool_t ringstream_autorelease;
	bool_t pad[3];
	int device_rotation;
	int max_calls;
	LinphoneTunnel *tunnel;
	char* device_id;
	MSList *last_recv_msg_ids;
	char *chat_db_file;
#ifdef MSG_STORAGE_ENABLED
	sqlite3 *db;
#endif
#ifdef BUILD_UPNP
	UpnpContext *upnp;
#endif //BUILD_UPNP
	belle_http_provider_t *http_provider;
};


struct _LinphoneEvent{
	LinphoneSubscriptionDir dir;
	LinphoneCore *lc;
	SalOp *op;
	SalCustomHeader *send_custom_headers;
	LinphoneSubscriptionState subscription_state;
	LinphonePublishState publish_state;
	LinphoneReason reason;
	void *userdata;
	int refcnt;
	char *name;
	LinphoneAddress *from;
	LinphoneAddress *resource_addr;
	int expires;
	bool_t terminating;
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
void linphone_call_make_local_media_description(LinphoneCore *lc, LinphoneCall *call);
void linphone_core_update_streams(LinphoneCore *lc, LinphoneCall *call, SalMediaDescription *new_md);

bool_t linphone_core_is_payload_type_usable_for_bandwidth(LinphoneCore *lc, PayloadType *pt,  int bandwidth_limit);

#define linphone_core_ready(lc) ((lc)->state==LinphoneGlobalOn || (lc)->state==LinphoneGlobalShutdown)
void _linphone_core_configure_resolver();

struct _EcCalibrator{
	ms_thread_t thread;
	MSSndCard *play_card,*capt_card;
	MSFilter *sndread,*det,*rec;
	MSFilter *play, *gen, *sndwrite;
	MSFilter *read_resampler,*write_resampler;
	MSTicker *ticker;
	LinphoneEcCalibrationCallback cb;
	void *cb_data;
	LinphoneEcCalibrationAudioInit audio_init_cb;
	LinphoneEcCalibrationAudioUninit audio_uninit_cb;
	int64_t acc;
	int delay;
	unsigned int rate;
	LinphoneEcCalibratorStatus status;
	bool_t freq1,freq2,freq3;
};

typedef struct _EcCalibrator EcCalibrator;

LinphoneEcCalibratorStatus ec_calibrator_get_status(EcCalibrator *ecc);

void ec_calibrator_destroy(EcCalibrator *ecc);

void linphone_call_background_tasks(LinphoneCall *call, bool_t one_second_elapsed);
void linphone_core_preempt_sound_resources(LinphoneCore *lc);
int _linphone_core_pause_call(LinphoneCore *lc, LinphoneCall *call);

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
unsigned int linphone_core_get_audio_features(LinphoneCore *lc);

void __linphone_core_invalidate_registers(LinphoneCore* lc);
void _linphone_core_codec_config_write(LinphoneCore *lc);

#define HOLD_OFF	(0)
#define HOLD_ON		(1)

#ifndef NB_MAX_CALLS
#define NB_MAX_CALLS	(10)
#endif
void call_logs_write_to_config_file(LinphoneCore *lc);

int linphone_core_get_edge_bw(LinphoneCore *lc);
int linphone_core_get_edge_ptime(LinphoneCore *lc);
void _linphone_call_params_copy(LinphoneCallParams *params, const LinphoneCallParams *refparams);
void linphone_call_params_uninit(LinphoneCallParams *params);

int linphone_upnp_init(LinphoneCore *lc);
void linphone_upnp_destroy(LinphoneCore *lc);

#ifdef MSG_STORAGE_ENABLED
sqlite3 * linphone_message_storage_init();
void linphone_message_storage_init_chat_rooms(LinphoneCore *lc);
#endif
void linphone_chat_message_store_state(LinphoneChatMessage *msg);
void linphone_core_message_storage_init(LinphoneCore *lc);
void linphone_core_message_storage_close(LinphoneCore *lc);

typedef enum _LinphoneToneID{
	LinphoneToneBusy,
	LinphoneToneCallWaiting,
	LinphoneToneCallOnHold,
	LinphoneToneCallFailed
}LinphoneToneID;
void linphone_core_play_named_tone(LinphoneCore *lc, LinphoneToneID id);
bool_t linphone_core_tone_indications_enabled(LinphoneCore*lc);
const char *linphone_core_create_uuid(LinphoneCore *lc);
void linphone_configure_op(LinphoneCore *lc, SalOp *op, const LinphoneAddress *dest, SalCustomHeader *headers, bool_t with_contact);
void linphone_call_create_op(LinphoneCall *call);
void linphone_core_notify_info_message(LinphoneCore* lc,SalOp *op, const SalBody *body);
LinphoneContent *linphone_content_copy_from_sal_body(LinphoneContent *obj, const SalBody *ref);
SalBody *sal_body_from_content(SalBody *body, const LinphoneContent *lc);
SalReason linphone_reason_to_sal(LinphoneReason reason);
LinphoneReason linphone_reason_from_sal(SalReason reason);
LinphoneEvent *linphone_event_new(LinphoneCore *lc, LinphoneSubscriptionDir dir, const char *name, int expires);
LinphoneEvent *linphone_event_new_with_op(LinphoneCore *lc, SalOp *op, LinphoneSubscriptionDir dir, const char *name);
void linphone_event_set_state(LinphoneEvent *lev, LinphoneSubscriptionState state);
void linphone_event_set_publish_state(LinphoneEvent *lev, LinphonePublishState state);
void linphone_event_set_reason(LinphoneEvent *lev, LinphoneReason reason);
LinphoneSubscriptionState linphone_subscription_state_from_sal(SalSubscribeStatus ss);
const LinphoneContent *linphone_content_from_sal_body(LinphoneContent *obj, const SalBody *ref);
void linphone_core_invalidate_friend_subscriptions(LinphoneCore *lc);


/*****************************************************************************
 * REMOTE PROVISIONING FUNCTIONS                                                     *
 ****************************************************************************/

void linphone_configuring_terminated(LinphoneCore *lc, LinphoneConfiguringState state, const char *message);
int linphone_remote_provisioning_download_and_apply(LinphoneCore *lc, const char *remote_provisioning_uri);


/*****************************************************************************
 * XML UTILITY FUNCTIONS                                                     *
 ****************************************************************************/

#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#define XMLPARSING_BUFFER_LEN 2048
#define MAX_XPATH_LENGTH 256

typedef struct _xmlparsing_context {
	xmlDoc *doc;
	xmlXPathContextPtr xpath_ctx;
	char errorBuffer[XMLPARSING_BUFFER_LEN];
	char warningBuffer[XMLPARSING_BUFFER_LEN];
} xmlparsing_context_t;

xmlparsing_context_t * linphone_xmlparsing_context_new(void);
void linphone_xmlparsing_context_destroy(xmlparsing_context_t *ctx);
void linphone_xmlparsing_genericxml_error(void *ctx, const char *fmt, ...);
int linphone_create_xml_xpath_context(xmlparsing_context_t *xml_ctx);
char * linphone_get_xml_text_content(xmlparsing_context_t *xml_ctx, const char *xpath_expression);
void linphone_free_xml_text_content(const char *text);
xmlXPathObjectPtr linphone_get_xml_xpath_object_for_node_list(xmlparsing_context_t *xml_ctx, const char *xpath_expression);


/** Belle Sip-based objects need unique ids
  */

BELLE_SIP_DECLARE_TYPES_BEGIN(linphone,10000)
BELLE_SIP_TYPE_ID(LinphoneContactSearch),
BELLE_SIP_TYPE_ID(LinphoneContactProvider),
BELLE_SIP_TYPE_ID(LinphoneLDAPContactProvider),
BELLE_SIP_TYPE_ID(LinphoneLDAPContactSearch)
BELLE_SIP_DECLARE_TYPES_END


#ifdef __cplusplus
}
#endif

#endif /* _PRIVATE_H */
