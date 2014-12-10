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
#include "quality_reporting.h"

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
	belle_sip_object_t base;
	void *user_data;
	LinphoneCall *referer; /*in case this call creation is consecutive to an incoming transfer, this points to the original call */
	int audio_bw; /* bandwidth limit for audio stream */
	LinphoneMediaEncryption media_encryption;
	PayloadType *audio_codec; /*audio codec currently in use */
	PayloadType *video_codec; /*video codec currently in use */
	MSVideoSize sent_vsize; /* Size of the video currently being sent */
	MSVideoSize recv_vsize; /* Size of the video currently being received */
	float received_fps,sent_fps;
	int down_bw;
	int up_bw;
	int down_ptime;
	int up_ptime;
	char *record_file;
	char *session_name;
	SalCustomHeader *custom_headers;
	bool_t has_video;
	bool_t avpf_enabled; /* RTCP feedback messages are enabled */
	bool_t real_early_media; /*send real media even during early media (for outgoing calls)*/
	bool_t in_conference; /*in conference mode */
	bool_t low_bandwidth;
	bool_t no_user_consent;/*when set to TRUE an UPDATE request will be used instead of reINVITE*/
	uint16_t avpf_rr_interval; /*in milliseconds*/
	LinphonePrivacyMask privacy;
};

BELLE_SIP_DECLARE_VPTR(LinphoneCallParams);


struct _LinphoneQualityReporting{
	reporting_session_report_t * reports[2]; /**Store information on audio and video media streams (RFC 6035) */
	bool_t was_video_running; /*Keep video state since last check in order to detect its (de)activation*/
	LinphoneQualityReportingReportSendCb on_report_sent;
};

struct _LinphoneCallLog{
	belle_sip_object_t base;
	void *user_data;
	struct _LinphoneCore *lc;
	LinphoneCallDir dir; /**< The direction of the call*/
	LinphoneCallStatus status; /**< The status of the call*/
	LinphoneAddress *from; /**<Originator of the call as a LinphoneAddress object*/
	LinphoneAddress *to; /**<Destination of the call as a LinphoneAddress object*/
	char start_date[128]; /**<Human readable string containing the start date*/
	int duration; /**<Duration of the call starting in connected state in seconds*/
	char *refkey;
	rtp_stats_t local_stats;
	rtp_stats_t remote_stats;
	float quality;
	time_t start_date_time; /**Start date of the call in seconds as expressed in a time_t */
	time_t connected_date_time; /**Connecting date of the call in seconds as expressed in a time_t */
	char* call_id; /**unique id of a call*/
	struct _LinphoneQualityReporting reporting;
	bool_t video_enabled;
};

BELLE_SIP_DECLARE_VPTR(LinphoneCallLog);


typedef struct _CallCallbackObj
{
	LinphoneCallCbFunc _func;
	void * _user_data;
}CallCallbackObj;

struct _LinphoneChatMessageCbs {
	belle_sip_object_t base;
	void *user_data;
	LinphoneChatMessageCbsMsgStateChangedCb msg_state_changed;
	LinphoneChatMessageCbsFileTransferRecvCb file_transfer_recv; /**< Callback to store file received attached to a #LinphoneChatMessage */
	LinphoneChatMessageCbsFileTransferSendCb file_transfer_send; /**< Callback to collect file chunk to be sent for a #LinphoneChatMessage */
	LinphoneChatMessageCbsFileTransferProgressIndicationCb file_transfer_progress_indication; /**< Callback to indicate file transfer progress */
};

BELLE_SIP_DECLARE_VPTR(LinphoneChatMessageCbs);

typedef enum _LinphoneChatMessageDir{
	LinphoneChatMessageIncoming,
	LinphoneChatMessageOutgoing
} LinphoneChatMessageDir;

struct _LinphoneChatMessage {
	belle_sip_object_t base;
	LinphoneChatRoom* chat_room;
	LinphoneChatMessageCbs *callbacks;
	LinphoneChatMessageDir dir;
	char* message;
	LinphoneChatMessageStateChangedCb cb;
	void* cb_ud;
	void* message_userdata;
	char* appdata;
	char* external_body_url;
	LinphoneAddress *from;
	LinphoneAddress *to;
	time_t time;
	SalCustomHeader *custom_headers;
	LinphoneChatMessageState state;
	bool_t is_read;
	unsigned int storage_id;
	SalOp *op;
	LinphoneContent *file_transfer_information; /**< used to store file transfer information when the message is of file transfer type */
	char *content_type; /**< is used to specified the type of message to be sent, used only for file transfer message */
	belle_http_request_t *http_request; /**< keep a reference to the http_request in case of file transfer in order to be able to cancel the transfer */
	char *file_transfer_filepath;
};

BELLE_SIP_DECLARE_VPTR(LinphoneChatMessage);

typedef struct StunCandidate{
	char addr[64];
	int port;
}StunCandidate;


typedef struct _PortConfig{
	int rtp_port;
	int rtcp_port;
}PortConfig;

struct _LinphoneCall{
	belle_sip_object_t base;
	void *user_data;
	struct _LinphoneCore *core;
	SalErrorInfo non_op_error;
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
	LinphoneCallState state;
	LinphoneCallState prevstate;
	LinphoneCallState transfer_state; /*idle if no transfer*/
	LinphoneProxyConfig *dest_proxy;
	PortConfig media_ports[2];
	MSMediaStreamSessions sessions[2]; /*the rtp, srtp, zrtp contexts for each stream*/
	StunCandidate ac,vc; /*audio video ip/port discovered by STUN*/
	struct _AudioStream *audiostream;  /**/
	struct _VideoStream *videostream;

	MSAudioEndpoint *endpoint; /*used for conferencing*/
	char *refer_to;
	LinphoneCallParams *params;
	LinphoneCallParams *current_params;
	LinphoneCallParams *remote_params;
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
	int ping_time;
	unsigned int remote_session_id;
	unsigned int remote_session_ver;
	LinphoneCall *referer; /*when this call is the result of a transfer, referer is set to the original call that caused the transfer*/
	LinphoneCall *transfer_target;/*if this call received a transfer request, then transfer_target points to the new call created to the refer target */
	int localdesc_changed;/*not a boolean, contains a mask representing changes*/
	LinphonePlayer *player;

	char *dtmf_sequence; /*DTMF sequence needed to be sent using #dtmfs_timer*/
	belle_sip_source_t *dtmfs_timer; /*DTMF timer needed to send a DTMF sequence*/

	bool_t refer_pending;
	bool_t expect_media_in_ack;
	bool_t audio_muted;
	bool_t camera_enabled;

	bool_t all_muted; /*this flag is set during early medias*/
	bool_t playing_ringbacktone;
	bool_t ringing_beep; /* whether this call is ringing through an already existent current call*/
	bool_t auth_token_verified;

	bool_t defer_update;
	bool_t was_automatically_paused;
	bool_t ping_replied;
	bool_t record_active;

	bool_t paused_by_app;
};

BELLE_SIP_DECLARE_VPTR(LinphoneCall);


LinphoneCall * linphone_call_new_outgoing(struct _LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to, const LinphoneCallParams *params, LinphoneProxyConfig *cfg);
LinphoneCall * linphone_call_new_incoming(struct _LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to, SalOp *op);
void linphone_call_set_new_params(LinphoneCall *call, const LinphoneCallParams *params);
void linphone_call_set_state(LinphoneCall *call, LinphoneCallState cstate, const char *message);
void linphone_call_set_contact_op(LinphoneCall* call);
void linphone_call_set_compatible_incoming_call_parameters(LinphoneCall *call, const SalMediaDescription *md);
/* private: */
LinphoneCallLog * linphone_call_log_new(LinphoneCallDir dir, LinphoneAddress *local, LinphoneAddress * remote);
void linphone_call_log_completed(LinphoneCall *call);
void linphone_call_log_destroy(LinphoneCallLog *cl);
void linphone_call_set_transfer_state(LinphoneCall* call, LinphoneCallState state);
LinphonePlayer *linphone_call_build_player(LinphoneCall*call);

LinphoneCallParams * linphone_call_params_new(void);
SalMediaProto get_proto_from_call_params(const LinphoneCallParams *params);

void linphone_auth_info_write_config(struct _LpConfig *config, LinphoneAuthInfo *obj, int pos);

void linphone_core_update_proxy_register(LinphoneCore *lc);
void linphone_core_refresh_subscribes(LinphoneCore *lc);
int linphone_core_abort_call(LinphoneCore *lc, LinphoneCall *call, const char *error);
const char *linphone_core_get_nat_address_resolved(LinphoneCore *lc);
/**
 * @brief Equivalent to _linphone_core_get_firewall_policy_with_lie(lc, TRUE)
 * @param lc LinphoneCore instance
 * @return Fairewall policy
 */
LinphoneFirewallPolicy _linphone_core_get_firewall_policy(const LinphoneCore *lc);
/**
 * @brief Get the firwall policy which has been set.
 * @param lc Instance of LinphoneCore
 * @param lie If true, the configured firewall policy will be returned only if no tunnel are enabled.
 * Otherwise, NoFirewallPolicy value will be returned.
 * @return The firewall policy
 */
LinphoneFirewallPolicy _linphone_core_get_firewall_policy_with_lie(const LinphoneCore *lc, bool_t lie);

int linphone_proxy_config_send_publish(LinphoneProxyConfig *cfg, LinphonePresenceModel *presence);
void linphone_proxy_config_set_state(LinphoneProxyConfig *cfg, LinphoneRegistrationState rstate, const char *message);
void linphone_proxy_config_stop_refreshing(LinphoneProxyConfig *obj);
void linphone_proxy_config_write_all_to_config_file(LinphoneCore *lc);
void _linphone_proxy_config_release(LinphoneProxyConfig *cfg);
/*
 * returns service route as defined in as defined by rfc3608, might be a list instead of just one.
 * Can be NULL
 * */
const LinphoneAddress* linphone_proxy_config_get_service_route(const LinphoneProxyConfig* cfg);
char* linphone_proxy_config_get_contact(const LinphoneProxyConfig *cfg);

void linphone_friend_close_subscriptions(LinphoneFriend *lf);
void linphone_friend_update_subscribes(LinphoneFriend *fr, LinphoneProxyConfig *cfg, bool_t only_when_registered);
void linphone_friend_notify(LinphoneFriend *lf, LinphonePresenceModel *presence);
LinphoneFriend *linphone_find_friend_by_inc_subscribe(MSList *l, SalOp *op);
LinphoneFriend *linphone_find_friend_by_out_subscribe(MSList *l, SalOp *op);
MSList *linphone_find_friend_by_address(MSList *fl, const LinphoneAddress *addr, LinphoneFriend **lf);
bool_t linphone_core_should_subscribe_friends_only_when_registered(const LinphoneCore *lc);
void linphone_core_update_friends_subscriptions(LinphoneCore *lc, LinphoneProxyConfig *cfg, bool_t only_when_registered);

int parse_hostname_to_addr(const char *server, struct sockaddr_storage *ss, socklen_t *socklen, int default_port);

bool_t host_has_ipv6_network();
bool_t lp_spawn_command_line_sync(const char *command, char **result,int *command_ret);

static MS2_INLINE int get_min_bandwidth(int dbw, int ubw){
	if (dbw<=0) return ubw;
	if (ubw<=0) return dbw;
	return MIN(dbw,ubw);
}

static MS2_INLINE bool_t bandwidth_is_greater(int bw1, int bw2){
	if (bw1<0) return TRUE;
	else if (bw2<0) return FALSE;
	else return bw1>=bw2;
}

static MS2_INLINE int get_remaining_bandwidth_for_video(int total, int audio){
	if (total<=0) return 0;
	return total-audio-10;
}

static MS2_INLINE void set_string(char **dest, const char *src){
	if (*dest){
		ms_free(*dest);
		*dest=NULL;
	}
	if (src)
		*dest=ms_strdup(src);
}

#define PAYLOAD_TYPE_ENABLED	PAYLOAD_TYPE_USER_FLAG_0
#define PAYLOAD_TYPE_BITRATE_OVERRIDE PAYLOAD_TYPE_USER_FLAG_3

void linphone_process_authentication(LinphoneCore* lc, SalOp *op);
void linphone_authentication_ok(LinphoneCore *lc, SalOp *op);
void linphone_subscription_new(LinphoneCore *lc, SalOp *op, const char *from);
void linphone_core_send_presence(LinphoneCore *lc, LinphonePresenceModel *presence);
void linphone_notify_parse_presence(SalOp *op, const char *content_type, const char *content_subtype, const char *body, SalPresenceModel **result);
void linphone_notify_convert_presence_to_xml(SalOp *op, SalPresenceModel *presence, const char *contact, char **content);
void linphone_notify_recv(LinphoneCore *lc, SalOp *op, SalSubscribeStatus ss, SalPresenceModel *model);
void linphone_proxy_config_process_authentication_failure(LinphoneCore *lc, SalOp *op);
void linphone_core_soundcard_hint_check(LinphoneCore* lc);


void linphone_subscription_answered(LinphoneCore *lc, SalOp *op);
void linphone_subscription_closed(LinphoneCore *lc, SalOp *op);

void linphone_core_update_allocated_audio_bandwidth(LinphoneCore *lc);
void linphone_core_update_allocated_audio_bandwidth_in_call(LinphoneCall *call, const PayloadType *pt, int maxbw);

LINPHONE_PUBLIC int linphone_core_run_stun_tests(LinphoneCore *lc, LinphoneCall *call);
void linphone_core_resolve_stun_server(LinphoneCore *lc);
const struct addrinfo *linphone_core_get_stun_server_addrinfo(LinphoneCore *lc);
void linphone_core_adapt_to_network(LinphoneCore *lc, int ping_time_ms, LinphoneCallParams *params);
int linphone_core_gather_ice_candidates(LinphoneCore *lc, LinphoneCall *call);
void linphone_core_update_ice_state_in_call_stats(LinphoneCall *call);
void linphone_call_stats_fill(LinphoneCallStats *stats, MediaStream *ms, OrtpEvent *ev);
void _update_local_media_description_from_ice(SalMediaDescription *desc, IceSession *session);
void linphone_call_update_local_media_description_from_ice_or_upnp(LinphoneCall *call);
void linphone_call_update_ice_from_remote_media_description(LinphoneCall *call, const SalMediaDescription *md);
bool_t linphone_core_media_description_contains_video_stream(const SalMediaDescription *md);

void linphone_core_send_initial_subscribes(LinphoneCore *lc);
void linphone_core_write_friends_config(LinphoneCore* lc);
void linphone_friend_write_to_config_file(struct _LpConfig *config, LinphoneFriend *lf, int index);
LinphoneFriend * linphone_friend_new_from_config_file(struct _LinphoneCore *lc, int index);

void linphone_proxy_config_update(LinphoneProxyConfig *cfg);
LinphoneProxyConfig * linphone_core_lookup_known_proxy(LinphoneCore *lc, const LinphoneAddress *uri);
const char *linphone_core_find_best_identity(LinphoneCore *lc, const LinphoneAddress *to);
int linphone_core_get_local_ip_for(int type, const char *dest, char *result);
void linphone_core_get_local_ip(LinphoneCore *lc, int af, const char *dest, char *result);

LinphoneProxyConfig *linphone_proxy_config_new_from_config_file(LinphoneCore *lc, int index);
void linphone_proxy_config_write_to_config_file(struct _LpConfig* config,LinphoneProxyConfig *obj, int index);

int linphone_proxy_config_normalize_number(LinphoneProxyConfig *cfg, const char *username, char *result, size_t result_len);

void linphone_core_message_received(LinphoneCore *lc, SalOp *op, const SalMessage *msg);
void linphone_core_is_composing_received(LinphoneCore *lc, SalOp *op, const SalIsComposing *is_composing);

void linphone_call_init_stats(LinphoneCallStats *stats, int type);
void linphone_call_fix_call_parameters(LinphoneCall *call);
void linphone_call_init_audio_stream(LinphoneCall *call);
void linphone_call_init_video_stream(LinphoneCall *call);
void linphone_call_init_media_streams(LinphoneCall *call);
void linphone_call_start_media_streams(LinphoneCall *call, bool_t all_inputs_muted, bool_t send_ringbacktone);
void linphone_call_start_media_streams_for_ice_gathering(LinphoneCall *call);
void linphone_call_stop_media_streams(LinphoneCall *call);
void linphone_call_delete_ice_session(LinphoneCall *call);
void linphone_call_delete_upnp_session(LinphoneCall *call);
void linphone_call_stop_media_streams_for_ice_gathering(LinphoneCall *call);
void linphone_call_update_crypto_parameters(LinphoneCall *call, SalMediaDescription *old_md, SalMediaDescription *new_md);
void linphone_call_update_remote_session_id_and_ver(LinphoneCall *call);
int _linphone_core_apply_transports(LinphoneCore *lc);
const char * linphone_core_get_identity(LinphoneCore *lc);

void linphone_core_start_waiting(LinphoneCore *lc, const char *purpose);
void linphone_core_update_progress(LinphoneCore *lc, const char *purpose, float progresses);
void linphone_core_stop_waiting(LinphoneCore *lc);

int linphone_core_proceed_with_invite_if_ready(LinphoneCore *lc, LinphoneCall *call, LinphoneProxyConfig *dest_proxy);
int linphone_core_start_invite(LinphoneCore *lc, LinphoneCall *call, const LinphoneAddress* destination/* = NULL if to be taken from the call log */);
int linphone_core_restart_invite(LinphoneCore *lc, LinphoneCall *call);
int linphone_core_start_update_call(LinphoneCore *lc, LinphoneCall *call);
int linphone_core_start_accept_call_update(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState next_state, const char *state_info);
void linphone_core_notify_incoming_call(LinphoneCore *lc, LinphoneCall *call);
bool_t linphone_core_incompatible_security(LinphoneCore *lc, SalMediaDescription *md);
extern SalCallbacks linphone_sal_callbacks;
bool_t linphone_core_rtcp_enabled(const LinphoneCore *lc);
bool_t linphone_core_symmetric_rtp_enabled(LinphoneCore*lc);

void linphone_core_queue_task(LinphoneCore *lc, belle_sip_source_func_t task_fun, void *data, const char *task_description);

typedef enum _LinphoneProxyConfigAddressComparisonResult{
	LinphoneProxyConfigAddressDifferent,
	LinphoneProxyConfigAddressEqual,
	LinphoneProxyConfigAddressWeakEqual
} LinphoneProxyConfigAddressComparisonResult;

LINPHONE_PUBLIC LinphoneProxyConfigAddressComparisonResult linphone_proxy_config_address_equal(const LinphoneAddress *a, const LinphoneAddress *b);
LINPHONE_PUBLIC LinphoneProxyConfigAddressComparisonResult linphone_proxy_config_is_server_config_changed(const LinphoneProxyConfig* obj);
void _linphone_proxy_config_unregister(LinphoneProxyConfig *obj);
void _linphone_proxy_config_release_ops(LinphoneProxyConfig *obj);

/*chat*/
void linphone_chat_room_release(LinphoneChatRoom *cr);
void linphone_chat_message_destroy(LinphoneChatMessage* msg);
void linphone_chat_message_update_state(LinphoneChatMessage* chat_msg );
/**/

struct _LinphoneProxyConfig
{
	belle_sip_object_t base;
	void *user_data;
	struct _LinphoneCore *lc;
	char *reg_proxy;
	char *reg_identity;
	char *reg_route;
	char *quality_reporting_collector;
	char *domain;
	char *realm;
	char *contact_params;
	char *contact_uri_params;
	int expires;
	int publish_expires;
	SalOp *op;
	char *type;
	struct _SipSetupContext *ssctx;
	int auth_failures;
	char *dial_prefix;
	LinphoneRegistrationState state;
	SalOp *publish_op;
	LinphoneAVPFMode avpf_mode;

	bool_t commit;
	bool_t reg_sendregister;
	bool_t publish;
	bool_t dial_escape_plus;

	bool_t send_publish;
	bool_t quality_reporting_enabled;
	uint8_t avpf_rr_interval;
	uint8_t quality_reporting_interval;

	time_t deletion_date;
	LinphonePrivacyMask privacy;
	/*use to check if server config has changed  between edit() and done()*/
	LinphoneAddress *saved_proxy;
	LinphoneAddress *saved_identity;
	/*---*/

};

BELLE_SIP_DECLARE_VPTR(LinphoneProxyConfig);

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
	belle_sip_object_t base;
	void *user_data;
	struct _LinphoneCore *lc;
	char  *peer;
	LinphoneAddress *peer_url;
	MSList *messages_hist;
	MSList *transient_messages;
	LinphoneIsComposingState remote_is_composing;
	LinphoneIsComposingState is_composing;
	belle_sip_source_t *remote_composing_refresh_timer;
	belle_sip_source_t *composing_idle_timer;
	belle_sip_source_t *composing_refresh_timer;
};

BELLE_SIP_DECLARE_VPTR(LinphoneChatRoom);


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
	bool_t vfu_with_info; /*use to enable vfu request using sip info*/
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
	MSCryptoSuite *srtp_suites;
	LinphoneAVPFMode avpf_mode;
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
	MSVideoSize preview_vsize; /*is 0,0 if no forced preview size is set, in which case vsize field above is used.*/
	float fps;
	bool_t capture;
	bool_t show_local;
	bool_t display;
	bool_t selfview; /*during calls*/
	bool_t reuse_preview_source;
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
	bool_t terminated;
};


typedef struct _LinphoneToneDescription{
	LinphoneReason reason;
	LinphoneToneID toneid;
	char *audiofile;
}LinphoneToneDescription;

LinphoneToneDescription * linphone_tone_description_new(LinphoneReason reason, LinphoneToneID id, const char *audiofile);
void linphone_tone_description_destroy(LinphoneToneDescription *obj);
LinphoneToneDescription *linphone_core_get_call_error_tone(const LinphoneCore *lc, LinphoneReason reason);
void linphone_core_play_call_error_tone(LinphoneCore *lc, LinphoneReason reason);
void _linphone_core_set_tone(LinphoneCore *lc, LinphoneReason reason, LinphoneToneID id, const char *audiofile);
const char *linphone_core_get_tone_file(const LinphoneCore *lc, LinphoneToneID id);
int _linphone_core_accept_call_update(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params, LinphoneCallState next_state, const char *state_info);
typedef struct _LinphoneConference LinphoneConference;

struct _LinphoneCore
{
	MSList* vtables;
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
	int audio_bw; /*IP bw consumed by audio codec, set as soon as used codec is known, its purpose is to know the remaining bw for video*/
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
	bool_t network_reachable_to_be_notified; /*set to true when state must be notified in next iterate*/
	bool_t use_preview_window;

	time_t network_last_check;

	bool_t network_last_status;
	bool_t ringstream_autorelease;
	bool_t pad[2];
	char localip[LINPHONE_IPADDR_SIZE];
	int device_rotation;
	int max_calls;
	LinphoneTunnel *tunnel;
	char* device_id;
	MSList *last_recv_msg_ids;
	char *chat_db_file;
#ifdef MSG_STORAGE_ENABLED
	sqlite3 *db;
	bool_t debug_storage;
#endif
#ifdef BUILD_UPNP
	UpnpContext *upnp;
#endif //BUILD_UPNP
	belle_http_provider_t *http_provider;
	belle_tls_verify_policy_t *http_verify_policy;
	MSList *tones;
	LinphoneReason chat_deny_code;
	const char **supported_formats;
	LinphoneContent *log_collection_upload_information;
	LinphoneCoreVTable *current_vtable; // the latest vtable to call a callback, see linphone_core_get_current_vtable
};


struct _LinphoneEvent{
	LinphoneSubscriptionDir dir;
	LinphoneCore *lc;
	SalOp *op;
	SalCustomHeader *send_custom_headers;
	LinphoneSubscriptionState subscription_state;
	LinphonePublishState publish_state;
	void *userdata;
	int refcnt;
	char *name;
	int expires;
	bool_t terminating;
	bool_t is_out_of_dialog_op; /*used for out of dialog notify*/
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
void linphone_call_increment_local_media_description(LinphoneCall *call);
void linphone_core_update_streams(LinphoneCore *lc, LinphoneCall *call, SalMediaDescription *new_md);

bool_t linphone_core_is_payload_type_usable_for_bandwidth(LinphoneCore *lc, const PayloadType *pt,  int bandwidth_limit);

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
void call_logs_read_from_config_file(LinphoneCore *lc);
void call_logs_write_to_config_file(LinphoneCore *lc);

int linphone_core_get_edge_bw(LinphoneCore *lc);
int linphone_core_get_edge_ptime(LinphoneCore *lc);

int linphone_upnp_init(LinphoneCore *lc);
void linphone_upnp_destroy(LinphoneCore *lc);

#ifdef MSG_STORAGE_ENABLED
sqlite3 * linphone_message_storage_init();
void linphone_message_storage_init_chat_rooms(LinphoneCore *lc);
#endif
void linphone_chat_message_store_state(LinphoneChatMessage *msg);
void linphone_chat_message_store_appdata(LinphoneChatMessage* msg);
void linphone_core_message_storage_init(LinphoneCore *lc);
void linphone_core_message_storage_close(LinphoneCore *lc);
void linphone_core_message_storage_set_debug(LinphoneCore *lc, bool_t debug);

void linphone_core_play_named_tone(LinphoneCore *lc, LinphoneToneID id);
bool_t linphone_core_tone_indications_enabled(LinphoneCore*lc);
const char *linphone_core_create_uuid(LinphoneCore *lc);
void linphone_configure_op(LinphoneCore *lc, SalOp *op, const LinphoneAddress *dest, SalCustomHeader *headers, bool_t with_contact);
void linphone_call_create_op(LinphoneCall *call);
int linphone_call_prepare_ice(LinphoneCall *call, bool_t incoming_offer);
void linphone_core_notify_info_message(LinphoneCore* lc,SalOp *op, const SalBody *body);
LinphoneContent * linphone_content_new(void);
LinphoneContent * linphone_content_copy(const LinphoneContent *ref);
SalBody *sal_body_from_content(SalBody *body, const LinphoneContent *content);
SalReason linphone_reason_to_sal(LinphoneReason reason);
LinphoneReason linphone_reason_from_sal(SalReason reason);
LinphoneEvent *linphone_event_new(LinphoneCore *lc, LinphoneSubscriptionDir dir, const char *name, int expires);
LinphoneEvent *linphone_event_new_with_op(LinphoneCore *lc, SalOp *op, LinphoneSubscriptionDir dir, const char *name);
/**
 * Useful for out of dialog notify
 * */
LinphoneEvent *linphone_event_new_with_out_of_dialog_op(LinphoneCore *lc, SalOp *op, LinphoneSubscriptionDir dir, const char *name);
void linphone_event_set_state(LinphoneEvent *lev, LinphoneSubscriptionState state);
void linphone_event_set_publish_state(LinphoneEvent *lev, LinphonePublishState state);
LinphoneSubscriptionState linphone_subscription_state_from_sal(SalSubscribeStatus ss);
LinphoneContent *linphone_content_from_sal_body(const SalBody *ref);
void linphone_core_invalidate_friend_subscriptions(LinphoneCore *lc);


struct _LinphoneContent {
	belle_sip_object_t base;
	void *user_data;
	struct _LinphoneContentPrivate lcp;
	bool_t owned_fields;
};

BELLE_SIP_DECLARE_VPTR(LinphoneContent);

struct _LinphoneBuffer {
	belle_sip_object_t base;
	void *user_data;
	uint8_t *content;	/**< A pointer to the buffer content */
	size_t size;	/**< The size of the buffer content */
};

BELLE_SIP_DECLARE_VPTR(LinphoneBuffer);


/*****************************************************************************
 * REMOTE PROVISIONING FUNCTIONS                                                     *
 ****************************************************************************/

void linphone_configuring_terminated(LinphoneCore *lc, LinphoneConfiguringState state, const char *message);
int linphone_remote_provisioning_download_and_apply(LinphoneCore *lc, const char *remote_provisioning_uri);
int linphone_remote_provisioning_load_file( LinphoneCore* lc, const char* file_path);

/*****************************************************************************
 * Player interface
 ****************************************************************************/

struct _LinphonePlayer{
	int (*open)(struct _LinphonePlayer* player, const char *filename);
	int (*start)(struct _LinphonePlayer* player);
	int (*pause)(struct _LinphonePlayer* player);
	int (*seek)(struct _LinphonePlayer* player, int time_ms);
	MSPlayerState (*get_state)(struct _LinphonePlayer* player);
	int (*get_duration)(struct _LinphonePlayer *player);
	int (*get_position)(struct _LinphonePlayer *player);
	void (*close)(struct _LinphonePlayer* player);
	void (*destroy)(struct _LinphonePlayer *player);
	LinphonePlayerEofCallback cb;
	void *user_data;
	void *impl;
};

void _linphone_player_destroy(LinphonePlayer *player);


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

/*****************************************************************************
 * OTHER UTILITY FUNCTIONS                                                     *
 ****************************************************************************/
char * linphone_timestamp_to_rfc3339_string(time_t timestamp);


static MS2_INLINE const LinphoneErrorInfo *linphone_error_info_from_sal_op(const SalOp *op){
	if (op==NULL) return (LinphoneErrorInfo*)sal_error_info_none();
	return (const LinphoneErrorInfo*)sal_op_get_error_info(op);
}

const MSCryptoSuite * linphone_core_get_srtp_crypto_suites(LinphoneCore *lc);

/** Belle Sip-based objects need unique ids
  */

BELLE_SIP_DECLARE_TYPES_BEGIN(linphone,10000)
BELLE_SIP_TYPE_ID(LinphoneBuffer),
BELLE_SIP_TYPE_ID(LinphoneContactProvider),
BELLE_SIP_TYPE_ID(LinphoneContactSearch),
BELLE_SIP_TYPE_ID(LinphoneCall),
BELLE_SIP_TYPE_ID(LinphoneCallLog),
BELLE_SIP_TYPE_ID(LinphoneCallParams),
BELLE_SIP_TYPE_ID(LinphoneChatMessage),
BELLE_SIP_TYPE_ID(LinphoneChatMessageCbs),
BELLE_SIP_TYPE_ID(LinphoneChatRoom),
BELLE_SIP_TYPE_ID(LinphoneContent),
BELLE_SIP_TYPE_ID(LinphoneLDAPContactProvider),
BELLE_SIP_TYPE_ID(LinphoneLDAPContactSearch),
BELLE_SIP_TYPE_ID(LinphoneProxyConfig)
BELLE_SIP_DECLARE_TYPES_END



void linphone_core_notify_global_state_changed(LinphoneCore *lc, LinphoneGlobalState gstate, const char *message);
void linphone_core_notify_call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *message);
void linphone_core_notify_call_encryption_changed(LinphoneCore *lc, LinphoneCall *call, bool_t on, const char *authentication_token);
void linphone_core_notify_registration_state_changed(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message);
void linphone_core_notify_show_interface(LinphoneCore *lc);
void linphone_core_notify_display_status(LinphoneCore *lc, const char *message);
void linphone_core_notify_display_message(LinphoneCore *lc, const char *message);
void linphone_core_notify_display_warning(LinphoneCore *lc, const char *message);
void linphone_core_notify_display_url(LinphoneCore *lc, const char *message, const char *url);
void linphone_core_notify_notify_presence_received(LinphoneCore *lc, LinphoneFriend * lf);
void linphone_core_notify_new_subscription_requested(LinphoneCore *lc, LinphoneFriend *lf, const char *url);
void linphone_core_notify_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username, const char *domain);
void linphone_core_notify_call_log_updated(LinphoneCore *lc, LinphoneCallLog *newcl);
void linphone_core_notify_text_message_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from, const char *message);
void linphone_core_notify_message_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage *message);
void linphone_core_notify_file_transfer_recv(LinphoneCore *lc, LinphoneChatMessage *message, const LinphoneContent* content, const char* buff, size_t size);
void linphone_core_notify_file_transfer_send(LinphoneCore *lc, LinphoneChatMessage *message,  const LinphoneContent* content, char* buff, size_t* size);
void linphone_core_notify_file_transfer_progress_indication(LinphoneCore *lc, LinphoneChatMessage *message, const LinphoneContent* content, size_t offset, size_t total);
void linphone_core_notify_is_composing_received(LinphoneCore *lc, LinphoneChatRoom *room);
void linphone_core_notify_dtmf_received(LinphoneCore* lc, LinphoneCall *call, int dtmf);
/*
 * return true if at least a registered vtable has a cb for dtmf received*/
bool_t linphone_core_dtmf_received_has_listener(const LinphoneCore* lc);
void linphone_core_notify_refer_received(LinphoneCore *lc, const char *refer_to);
void linphone_core_notify_buddy_info_updated(LinphoneCore *lc, LinphoneFriend *lf);
void linphone_core_notify_transfer_state_changed(LinphoneCore *lc, LinphoneCall *transfered, LinphoneCallState new_call_state);
void linphone_core_notify_call_stats_updated(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallStats *stats);
void linphone_core_notify_info_received(LinphoneCore *lc, LinphoneCall *call, const LinphoneInfoMessage *msg);
void linphone_core_notify_configuring_status(LinphoneCore *lc, LinphoneConfiguringState status, const char *message);
void linphone_core_notify_network_reachable(LinphoneCore *lc, bool_t reachable);

void linphone_core_notify_notify_received(LinphoneCore *lc, LinphoneEvent *lev, const char *notified_event, const LinphoneContent *body);
void linphone_core_notify_subscription_state_changed(LinphoneCore *lc, LinphoneEvent *lev, LinphoneSubscriptionState state);
void linphone_core_notify_publish_state_changed(LinphoneCore *lc, LinphoneEvent *lev, LinphonePublishState state);
void linphone_core_notify_log_collection_upload_state_changed(LinphoneCore *lc, LinphoneCoreLogCollectionUploadState state, const char *info);
void linphone_core_notify_log_collection_upload_progress_indication(LinphoneCore *lc, size_t offset, size_t total);

void set_mic_gain_db(AudioStream *st, float gain);
void set_playback_gain_db(AudioStream *st, float gain);

#ifdef __cplusplus
}
#endif

#endif /* _PRIVATE_H */
