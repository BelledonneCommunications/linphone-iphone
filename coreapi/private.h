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

#include <memory>

#include "linphone/core.h"
#include "linphone/friend.h"
#include "linphone/friendlist.h"
#include "linphone/tunnel.h"
#include "linphone/core_utils.h"
#include "linphone/conference.h"

#include "address/address.h"
#include "c-wrapper/internal/c-sal.h"
#include "sal/call-op.h"
#include "sal/event-op.h"
#include "sal/message-op.h"
#include "sal/presence-op.h"
#include "sal/register-op.h"

#ifdef __cplusplus
#include "core/platform-helpers/platform-helpers.h"
#endif

#include "linphone/sipsetup.h"
#include "quality_reporting.h"
#include "linphone/ringtoneplayer.h"
#include "vcard_private.h"
#include "carddav.h"
#include "linphone/player.h"
#include "account_creator_private.h"
#include "tester_utils.h"

#include "bctoolbox/port.h"
#include "bctoolbox/map.h"
#include "bctoolbox/vfs.h"
#include "belle-sip/belle-sip.h" /*we need this include for all http operations*/


#include <ctype.h>


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "mediastreamer2/ice.h"
#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/msconference.h"


#ifndef LIBLINPHONE_VERSION
#define LIBLINPHONE_VERSION LINPHONE_VERSION
#endif

#ifdef __ANDROID__
#include <jni.h>
#endif

#ifdef _WIN32
#if defined(__MINGW32__) || !defined(WINAPI_FAMILY_PARTITION) || !defined(WINAPI_PARTITION_DESKTOP)
#define LINPHONE_WINDOWS_DESKTOP 1
#elif defined(WINAPI_FAMILY_PARTITION)
#if defined(WINAPI_PARTITION_DESKTOP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define LINPHONE_WINDOWS_DESKTOP 1
#elif defined(WINAPI_PARTITION_PHONE_APP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
#define LINPHONE_WINDOWS_PHONE 1
#elif defined(WINAPI_PARTITION_APP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#define LINPHONE_WINDOWS_UNIVERSAL 1
#endif
#endif
#endif
#ifdef _MSC_VER
#if (_MSC_VER >= 1900)
#define LINPHONE_MSC_VER_GREATER_19
#endif
#endif

#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>


#ifdef SQLITE_STORAGE_ENABLED
#include <sqlite3.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define STRING_RESET(field)	if (field) bctbx_free(field); (field) = NULL
#define STRING_SET(field, value)	do{ if (field){bctbx_free(field);field=NULL;}; field=bctbx_strdup(value); }while(0)
#define STRING_TRANSFER(field, newvalue)	do{ if (field){bctbx_free(field);field=NULL;}; field=newvalue; }while(0)



struct _LinphoneQualityReporting{
	reporting_session_report_t * reports[3]; /**Store information on audio and video media streams (RFC 6035) */
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
	unsigned int storage_id;
	LinphoneErrorInfo *error_info;
	bool_t video_enabled;
	bool_t was_conference; /**<That call was a call with a conference server */
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneCallLog);


typedef struct _CallCallbackObj
{
	LinphoneCallCbFunc _func;
	void * _user_data;
}CallCallbackObj;

//BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneChatMessageCbs);

/*struct _LinphoneChatMessage {
	belle_sip_object_t base;
	LinphoneChatRoom* chat_room;
	LinphoneChatMessageCbs *callbacks;
	LinphoneErrorInfo *ei;
	LinphoneChatMessageDir dir;
	char* message;
	void* message_state_changed_user_data;
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
	char *message_id;
	MessageOp *op;
	LinphoneContent *file_transfer_information; //< used to store file transfer information when the message is of file transfer type
	char *content_type; //< is used to specified the type of message to be sent, used only for file transfer message
	bool_t to_be_stored;
	belle_http_request_t *http_request; //< keep a reference to the http_request in case of file transfer in order to be able to cancel the transfer
	belle_http_request_listener_t *http_listener; // our listener, only owned by us
	char *file_transfer_filepath;
	unsigned long bg_task_id;
	bool_t is_secured;

#if defined(__clang__) || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic push
#endif
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#ifdef _MSC_VER
#pragma deprecated(message_state_changed_cb)
#endif
	LinphoneChatMessageStateChangedCb message_state_changed_cb;
#if defined(__clang__) || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif
};*/

typedef struct StunCandidate{
	char addr[64];
	int port;
}StunCandidate;


typedef struct _PortConfig{
	char multicast_ip[LINPHONE_IPADDR_SIZE];
	char multicast_bind_ip[LINPHONE_IPADDR_SIZE];
	int rtp_port;
	int rtcp_port;
}PortConfig;

LinphoneCallCbs *_linphone_call_cbs_new(void);


void linphone_call_notify_state_changed(LinphoneCall *call, LinphoneCallState cstate, const char *message);
void linphone_call_notify_dtmf_received(LinphoneCall *call, int dtmf);
void linphone_call_notify_encryption_changed(LinphoneCall *call, bool_t on, const char *authentication_token);
void linphone_call_notify_transfer_state_changed(LinphoneCall *call, LinphoneCallState cstate);
void linphone_call_notify_stats_updated(LinphoneCall *call, const LinphoneCallStats *stats);
void linphone_call_notify_info_message_received(LinphoneCall *call, const LinphoneInfoMessage *msg);
void linphone_call_notify_ack_processing(LinphoneCall *call, LinphoneHeaders *msg, bool_t is_received);

LinphoneCall * linphone_call_new_outgoing(struct _LinphoneCore *lc, const LinphoneAddress *from, const LinphoneAddress *to, const LinphoneCallParams *params, LinphoneProxyConfig *cfg);
LinphoneCall * linphone_call_new_incoming(struct _LinphoneCore *lc, const LinphoneAddress *from, const LinphoneAddress *to, LinphonePrivate::SalCallOp *op);
void _linphone_call_set_new_params(LinphoneCall *call, const LinphoneCallParams *params);
void linphone_call_set_state(LinphoneCall *call, LinphoneCallState cstate, const char *message);
/* private: */
LinphoneCallLog * linphone_call_log_new(LinphoneCallDir dir, LinphoneAddress *from, LinphoneAddress * to);
void linphone_call_set_transfer_state(LinphoneCall* call, LinphoneCallState state);
LinphonePlayer *linphone_call_build_player(LinphoneCall*call);
void linphone_call_refresh_sockets(LinphoneCall *call);
void linphone_call_replace_op(LinphoneCall *call, LinphonePrivate::SalOp *op);
void linphone_call_reinvite_to_recover_from_connection_loss(LinphoneCall *call);

LinphonePrivate::SalCallOp *linphone_call_get_op(const LinphoneCall *call);
LinphoneProxyConfig * linphone_call_get_dest_proxy(const LinphoneCall *call);
LINPHONE_PUBLIC MediaStream * linphone_call_get_stream(LinphoneCall *call, LinphoneStreamType type);
LinphoneCallLog * linphone_call_get_log(const LinphoneCall *call);
IceSession * linphone_call_get_ice_session(const LinphoneCall *call);
bool_t linphone_call_get_audio_muted(const LinphoneCall *call);
void linphone_call_set_audio_muted(LinphoneCall *call, bool_t value);
bool_t linphone_call_get_all_muted(const LinphoneCall *call);

LinphoneCallParams * linphone_call_params_new(LinphoneCore *core);
SalMediaProto get_proto_from_call_params(const LinphoneCallParams *params);
SalStreamDir get_audio_dir_from_call_params(const LinphoneCallParams *params);
SalStreamDir get_video_dir_from_call_params(const LinphoneCallParams *params);
void linphone_call_params_set_custom_headers(LinphoneCallParams *params, const SalCustomHeader *ch);
void linphone_call_params_set_custom_sdp_attributes(LinphoneCallParams *params, const SalCustomSdpAttribute *csa);
void linphone_call_params_set_custom_sdp_media_attributes(LinphoneCallParams *params, LinphoneStreamType type, const SalCustomSdpAttribute *csa);
bool_t linphone_call_params_get_in_conference(const LinphoneCallParams *params);
void linphone_call_params_set_in_conference(LinphoneCallParams *params, bool_t value);
bool_t linphone_call_params_get_internal_call_update(const LinphoneCallParams *params);
void linphone_call_params_set_internal_call_update(LinphoneCallParams *params, bool_t value);
bool_t linphone_call_params_implicit_rtcp_fb_enabled(const LinphoneCallParams *params);
void linphone_call_params_enable_implicit_rtcp_fb(LinphoneCallParams *params, bool_t value);
int linphone_call_params_get_down_bandwidth(const LinphoneCallParams *params);
void linphone_call_params_set_down_bandwidth(LinphoneCallParams *params, int value);
int linphone_call_params_get_up_bandwidth(const LinphoneCallParams *params);
void linphone_call_params_set_up_bandwidth(LinphoneCallParams *params, int value);
int linphone_call_params_get_down_ptime(const LinphoneCallParams *params);
void linphone_call_params_set_down_ptime(LinphoneCallParams *params, int value);
int linphone_call_params_get_up_ptime(const LinphoneCallParams *params);
void linphone_call_params_set_up_ptime(LinphoneCallParams *params, int value);
SalCustomHeader * linphone_call_params_get_custom_headers(const LinphoneCallParams *params);
SalCustomSdpAttribute * linphone_call_params_get_custom_sdp_attributes(const LinphoneCallParams *params);
SalCustomSdpAttribute * linphone_call_params_get_custom_sdp_media_attributes(const LinphoneCallParams *params, LinphoneStreamType type);
LinphoneCall * linphone_call_params_get_referer(const LinphoneCallParams *params);
void linphone_call_params_set_referer(LinphoneCallParams *params, LinphoneCall *referer);
bool_t linphone_call_params_get_update_call_when_ice_completed(const LinphoneCallParams *params);
void linphone_call_params_set_update_call_when_ice_completed(LinphoneCallParams *params, bool_t value);
void linphone_call_params_set_sent_vsize(LinphoneCallParams *params, MSVideoSize vsize);
void linphone_call_params_set_recv_vsize(LinphoneCallParams *params, MSVideoSize vsize);
void linphone_call_params_set_sent_video_definition(LinphoneCallParams *params, LinphoneVideoDefinition *vdef);
void linphone_call_params_set_received_video_definition(LinphoneCallParams *params, LinphoneVideoDefinition *vdef);
void linphone_call_params_set_sent_fps(LinphoneCallParams *params, float value);
void linphone_call_params_set_received_fps(LinphoneCallParams *params, float value);
void linphone_call_params_set_used_audio_codec(LinphoneCallParams *params, OrtpPayloadType *codec);
void linphone_call_params_set_used_video_codec(LinphoneCallParams *params, OrtpPayloadType *codec);
void linphone_call_params_set_used_text_codec(LinphoneCallParams *params, OrtpPayloadType *codec);
bool_t linphone_call_params_get_no_user_consent(const LinphoneCallParams *params);
void linphone_call_params_set_no_user_consent(LinphoneCallParams *params, bool_t value);

void linphone_auth_info_write_config(LinphoneConfig *config, LinphoneAuthInfo *obj, int pos);
LinphoneAuthInfo * linphone_auth_info_new_from_config_file(LpConfig *config, int pos);
void linphone_core_write_auth_info(LinphoneCore *lc, LinphoneAuthInfo *ai);
const LinphoneAuthInfo *_linphone_core_find_tls_auth_info(LinphoneCore *lc);
const LinphoneAuthInfo *_linphone_core_find_auth_info(LinphoneCore *lc, const char *realm, const char *username, const char *domain, bool_t ignore_realm);

void linphone_core_update_proxy_register(LinphoneCore *lc);
const char *linphone_core_get_nat_address_resolved(LinphoneCore *lc);

int linphone_proxy_config_send_publish(LinphoneProxyConfig *cfg, LinphonePresenceModel *presence);
void linphone_proxy_config_set_state(LinphoneProxyConfig *cfg, LinphoneRegistrationState rstate, const char *message);
void linphone_proxy_config_stop_refreshing(LinphoneProxyConfig *obj);
void linphone_proxy_config_write_all_to_config_file(LinphoneCore *lc);
void _linphone_proxy_config_release(LinphoneProxyConfig *cfg);
void _linphone_proxy_config_unpublish(LinphoneProxyConfig *obj);
void linphone_proxy_config_notify_publish_state_changed(LinphoneProxyConfig *cfg, LinphonePublishState state);
LinphoneEvent *linphone_proxy_config_create_publish(LinphoneProxyConfig *cfg, const char *event, int expires);
/*
 * returns service route as defined in as defined by rfc3608, might be a list instead of just one.
 * Can be NULL
 * */
const LinphoneAddress* linphone_proxy_config_get_service_route(const LinphoneProxyConfig* cfg);
const LinphoneAddress *_linphone_proxy_config_get_contact_without_params (const LinphoneProxyConfig *cfg);

void linphone_friend_list_invalidate_subscriptions(LinphoneFriendList *list);
void linphone_friend_list_notify_presence_received(LinphoneFriendList *list, LinphoneEvent *lev, const LinphoneContent *body);
void linphone_friend_list_subscription_state_changed(LinphoneCore *lc, LinphoneEvent *lev, LinphoneSubscriptionState state);
void _linphone_friend_list_release(LinphoneFriendList *list);
/*get rls either from list or core if any*/
const LinphoneAddress * _linphone_friend_list_get_rls_address(const LinphoneFriendList *list);

LINPHONE_PUBLIC void linphone_friend_invalidate_subscription(LinphoneFriend *lf);
void linphone_friend_close_subscriptions(LinphoneFriend *lf);
void _linphone_friend_release(LinphoneFriend *lf);
LINPHONE_PUBLIC void linphone_friend_update_subscribes(LinphoneFriend *fr, bool_t only_when_registered);
void linphone_friend_notify(LinphoneFriend *lf, LinphonePresenceModel *presence);
void linphone_friend_apply(LinphoneFriend *fr, LinphoneCore *lc);
void linphone_friend_add_incoming_subscription(LinphoneFriend *lf, LinphonePrivate::SalOp *op);
void linphone_friend_remove_incoming_subscription(LinphoneFriend *lf, LinphonePrivate::SalOp *op);
const char * linphone_friend_phone_number_to_sip_uri(LinphoneFriend *lf, const char *phone_number);
const char * linphone_friend_sip_uri_to_phone_number(LinphoneFriend *lf, const char *uri);
void linphone_friend_clear_presence_models(LinphoneFriend *lf);
LinphoneFriend *linphone_friend_list_find_friend_by_inc_subscribe(const LinphoneFriendList *list, LinphonePrivate::SalOp *op);
LinphoneFriend *linphone_friend_list_find_friend_by_out_subscribe(const LinphoneFriendList *list, LinphonePrivate::SalOp *op);
LinphoneFriend *linphone_core_find_friend_by_out_subscribe(const LinphoneCore *lc, LinphonePrivate::SalOp *op);
LinphoneFriend *linphone_core_find_friend_by_inc_subscribe(const LinphoneCore *lc, LinphonePrivate::SalOp *op);
MSList *linphone_find_friend_by_address(MSList *fl, const LinphoneAddress *addr, LinphoneFriend **lf);
bool_t linphone_core_should_subscribe_friends_only_when_registered(const LinphoneCore *lc);
void linphone_core_update_friends_subscriptions(LinphoneCore *lc);
void _linphone_friend_list_update_subscriptions(LinphoneFriendList *list, LinphoneProxyConfig *cfg, bool_t only_when_registered);
void linphone_core_friends_storage_init(LinphoneCore *lc);
void linphone_core_friends_storage_close(LinphoneCore *lc);
void linphone_core_store_friend_in_db(LinphoneCore *lc, LinphoneFriend *lf);
void linphone_core_remove_friend_from_db(LinphoneCore *lc, LinphoneFriend *lf);
void linphone_core_store_friends_list_in_db(LinphoneCore *lc, LinphoneFriendList *list);
void linphone_core_remove_friends_list_from_db(LinphoneCore *lc, LinphoneFriendList *list);
LINPHONE_PUBLIC MSList* linphone_core_fetch_friends_from_db(LinphoneCore *lc, LinphoneFriendList *list);
LINPHONE_PUBLIC MSList* linphone_core_fetch_friends_lists_from_db(LinphoneCore *lc);
LINPHONE_PUBLIC LinphoneFriendListStatus linphone_friend_list_import_friend(LinphoneFriendList *list, LinphoneFriend *lf, bool_t synchronize);

int linphone_parse_host_port(const char *input, char *host, size_t hostlen, int *port);
int parse_hostname_to_addr(const char *server, struct sockaddr_storage *ss, socklen_t *socklen, int default_port);

bool_t host_has_ipv6_network(void);
bool_t lp_spawn_command_line_sync(const char *command, char **result,int *command_ret);

static MS2_INLINE void set_string(char **dest, const char *src, bool_t lowercase){
	if (*dest){
		ms_free(*dest);
		*dest=NULL;
	}
	if (src) {
		*dest=ms_strdup(src);
		if (lowercase) {
			char *cur = *dest;
			for (; *cur; cur++) *cur = (char)tolower(*cur);
		}
	}
}

void linphone_process_authentication(LinphoneCore* lc, LinphonePrivate::SalOp *op);
void linphone_authentication_ok(LinphoneCore *lc, LinphonePrivate::SalOp *op);
void linphone_subscription_new(LinphoneCore *lc, LinphonePrivate::SalSubscribeOp *op, const char *from);
void linphone_core_send_presence(LinphoneCore *lc, LinphonePresenceModel *presence);
void linphone_notify_parse_presence(const char *content_type, const char *content_subtype, const char *body, SalPresenceModel **result);
void linphone_notify_convert_presence_to_xml(LinphonePrivate::SalOp *op, SalPresenceModel *presence, const char *contact, char **content);
void linphone_notify_recv(LinphoneCore *lc, LinphonePrivate::SalOp *op, SalSubscribeStatus ss, SalPresenceModel *model);
void linphone_proxy_config_process_authentication_failure(LinphoneCore *lc, LinphonePrivate::SalOp *op);
void linphone_core_soundcard_hint_check(LinphoneCore* lc);


void linphone_subscription_answered(LinphoneCore *lc, LinphonePrivate::SalOp *op);
void linphone_subscription_closed(LinphoneCore *lc, LinphonePrivate::SalOp *op);

void linphone_core_update_allocated_audio_bandwidth(LinphoneCore *lc);

LINPHONE_PUBLIC int linphone_run_stun_tests(LinphoneCore *lc, int audioPort, int videoPort, int textPort,
	char *audioCandidateAddr, int *audioCandidatePort, char *videoCandidateAddr, int *videoCandidatePort, char *textCandidateAddr, int *textCandidatePort);
void linphone_core_resolve_stun_server(LinphoneCore *lc);
LINPHONE_PUBLIC const struct addrinfo *linphone_core_get_stun_server_addrinfo(LinphoneCore *lc);
LINPHONE_PUBLIC void linphone_core_enable_forced_ice_relay(LinphoneCore *lc, bool_t enable);
LINPHONE_PUBLIC void linphone_core_enable_short_turn_refresh(LinphoneCore *lc, bool_t enable);
void linphone_call_update_ice_state_in_call_stats(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_call_stats_fill(LinphoneCallStats *stats, MediaStream *ms, OrtpEvent *ev);
void linphone_call_stats_update(LinphoneCallStats *stats, MediaStream *stream);
LinphoneCallStats *_linphone_call_stats_new(void);
void _linphone_call_stats_uninit(LinphoneCallStats *stats);
void _linphone_call_stats_clone(LinphoneCallStats *dst, const LinphoneCallStats *src);
void _linphone_call_stats_set_ice_state (LinphoneCallStats *stats, LinphoneIceState state);
void _linphone_call_stats_set_type (LinphoneCallStats *stats, LinphoneStreamType type);
mblk_t *_linphone_call_stats_get_received_rtcp (const LinphoneCallStats *stats);
void _linphone_call_stats_set_received_rtcp (LinphoneCallStats *stats, mblk_t *m);
void _linphone_call_stats_set_sent_rtcp (LinphoneCallStats *stats, mblk_t *m);
int _linphone_call_stats_get_updated (const LinphoneCallStats *stats);
void _linphone_call_stats_set_updated (LinphoneCallStats *stats, int updated);
void _linphone_call_stats_set_rtp_stats (LinphoneCallStats *stats, const rtp_stats_t *rtpStats);
void _linphone_call_stats_set_download_bandwidth (LinphoneCallStats *stats, float bandwidth);
void _linphone_call_stats_set_upload_bandwidth (LinphoneCallStats *stats, float bandwidth);
void _linphone_call_stats_set_rtcp_download_bandwidth (LinphoneCallStats *stats, float bandwidth);
void _linphone_call_stats_set_rtcp_upload_bandwidth (LinphoneCallStats *stats, float bandwidth);
void _linphone_call_stats_set_ip_family_of_remote (LinphoneCallStats *stats, LinphoneAddressFamily family);
bool_t _linphone_call_stats_rtcp_received_via_mux (const LinphoneCallStats *stats);
void linphone_call_update_local_media_description_from_ice_or_upnp(LinphoneCall *call);
void linphone_call_update_ice_from_remote_media_description(LinphoneCall *call, const SalMediaDescription *md, bool_t is_offer);
void linphone_call_clear_unused_ice_candidates(LinphoneCall *call, const SalMediaDescription *md);
bool_t linphone_core_media_description_contains_video_stream(const SalMediaDescription *md);

void linphone_core_send_initial_subscribes(LinphoneCore *lc);
void linphone_core_write_friends_config(LinphoneCore* lc);
void linphone_friend_write_to_config_file(LinphoneConfig *config, LinphoneFriend *lf, int index);
LinphoneFriend * linphone_friend_new_from_config_file(struct _LinphoneCore *lc, int index);

void linphone_proxy_config_update(LinphoneProxyConfig *cfg);
LinphoneProxyConfig * linphone_core_lookup_known_proxy(LinphoneCore *lc, const LinphoneAddress *uri);
const char *linphone_core_find_best_identity(LinphoneCore *lc, const LinphoneAddress *to);
int linphone_core_get_local_ip_for(int type, const char *dest, char *result);
LINPHONE_PUBLIC void linphone_core_get_local_ip(LinphoneCore *lc, int af, const char *dest, char *result);

LinphoneProxyConfig *linphone_proxy_config_new_from_config_file(LinphoneCore *lc, int index);
void linphone_proxy_config_write_to_config_file(LinphoneConfig* config,LinphoneProxyConfig *obj, int index);

int linphone_core_message_received(LinphoneCore *lc, LinphonePrivate::SalOp *op, const SalMessage *msg);
void linphone_core_real_time_text_received(LinphoneCore *lc, LinphoneChatRoom *cr, uint32_t character, LinphoneCall *call);

void linphone_call_init_media_streams(LinphoneCall *call);
void linphone_call_start_media_streams_for_ice_gathering(LinphoneCall *call);
void linphone_call_stop_media_streams(LinphoneCall *call);
void linphone_call_delete_upnp_session(LinphoneCall *call);
int _linphone_core_apply_transports(LinphoneCore *lc);
const char * linphone_core_get_identity(LinphoneCore *lc);

void linphone_core_start_waiting(LinphoneCore *lc, const char *purpose);
void linphone_core_update_progress(LinphoneCore *lc, const char *purpose, float progresses);
void linphone_core_stop_waiting(LinphoneCore *lc);

int linphone_call_proceed_with_invite_if_ready(LinphoneCall *call, LinphoneProxyConfig *dest_proxy);
int linphone_call_start_invite(LinphoneCall *call, const LinphoneAddress *destination/* = NULL if to be taken from the call log */);
/*
 * param automatic_offering aims is to take into account previous answer for video in case of automatic re-invite.
 *  Purpose is to avoid to re-ask video previously declined */
int linphone_call_start_update(LinphoneCall *call);
int linphone_call_start_accept_update(LinphoneCall *call, LinphoneCallState next_state, const char *state_info);
void linphone_core_notify_incoming_call(LinphoneCore *lc, LinphoneCall *call);
bool_t linphone_core_incompatible_security(LinphoneCore *lc, SalMediaDescription *md);
extern LinphonePrivate::Sal::Callbacks linphone_sal_callbacks;
LINPHONE_PUBLIC bool_t linphone_core_rtcp_enabled(const LinphoneCore *lc);
LINPHONE_PUBLIC bool_t linphone_core_symmetric_rtp_enabled(LinphoneCore*lc);
bool_t _linphone_core_is_conference_creation (const LinphoneCore *lc, const LinphoneAddress *addr);
LinphoneChatRoom *_linphone_core_create_server_group_chat_room (LinphoneCore *lc, LinphonePrivate::SalCallOp *op);

void linphone_core_queue_task(LinphoneCore *lc, belle_sip_source_func_t task_fun, void *data, const char *task_description);


LINPHONE_PUBLIC LinphoneProxyConfigAddressComparisonResult linphone_proxy_config_address_equal(const LinphoneAddress *a, const LinphoneAddress *b);
LINPHONE_PUBLIC LinphoneProxyConfigAddressComparisonResult linphone_proxy_config_is_server_config_changed(const LinphoneProxyConfig* obj);
/**
 * unregister without moving the register_enable flag
 */
void _linphone_proxy_config_unregister(LinphoneProxyConfig *obj);
void _linphone_proxy_config_release_ops(LinphoneProxyConfig *obj);

/*chat*/
LinphoneChatRoom * linphone_chat_room_new(LinphoneCore *core, const LinphoneAddress *addr);
LinphoneChatRoom *_linphone_client_group_chat_room_new (LinphoneCore *core, const char *uri, const char *subject);
LinphoneChatRoom *_linphone_server_group_chat_room_new (LinphoneCore *core, LinphonePrivate::SalCallOp *op);
void linphone_chat_room_release(LinphoneChatRoom *cr);
void linphone_chat_room_set_call(LinphoneChatRoom *cr, LinphoneCall *call);
bctbx_list_t * linphone_chat_room_get_transient_messages(const LinphoneChatRoom *cr);
LinphoneChatRoomCbs * linphone_chat_room_cbs_new (void);
/**/

struct _LinphoneProxyConfig
{
	belle_sip_object_t base;
	void *user_data;
	struct _LinphoneCore *lc;
	LinphoneErrorInfo *ei;
	char *reg_proxy;
	char *reg_identity;
	LinphoneAddress* identity_address;
	LinphoneAddress *contact_address;
	LinphoneAddress *contact_address_without_params;
	char *reg_route;
	char *quality_reporting_collector;
	char *realm;
	char *contact_params;
	char *contact_uri_params;
	int expires;
	int publish_expires;
	LinphonePrivate::SalRegisterOp *op;
	SalCustomHeader *sent_headers;
	char *type;
	struct _SipSetupContext *ssctx;
	int auth_failures;
	char *dial_prefix;
	LinphoneRegistrationState state;
	LinphoneAVPFMode avpf_mode;
	LinphoneNatPolicy *nat_policy;

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
	bool_t register_changed;
	bool_t unused[3];
	/*---*/
	LinphoneAddress *pending_contact; /*use to store previous contact in case of network failure*/
	LinphoneEvent *presence_publish_event;
	unsigned long long previous_publish_config_hash[2];

	char *refkey;
	char *sip_etag; /*publish context*/
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneProxyConfig);

struct _LinphoneAuthInfo
{
	belle_sip_object_t base;
	char *username;
	char *realm;
	char *userid;
	char *passwd;
	char *ha1;
	char *domain;
	char *tls_cert;
	char *tls_key;
	char *tls_cert_path;
	char *tls_key_path;
};

typedef enum _LinphoneIsComposingState {
	LinphoneIsComposingIdle,
	LinphoneIsComposingActive
} LinphoneIsComposingState;

typedef struct _LinphoneChatMessageCharacter {
	uint32_t value;
	bool_t has_been_read;
} LinphoneChatMessageCharacter;


typedef struct _LinphoneFriendPresence {
	char *uri_or_tel;
	LinphonePresenceModel *presence;
} LinphoneFriendPresence;

typedef struct _LinphoneFriendPhoneNumberSipUri {
	char *number;
	char *uri;
} LinphoneFriendPhoneNumberSipUri;

struct _LinphoneFriend{
	belle_sip_object_t base;
	void *user_data;
	LinphoneAddress *uri;
	MSList *insubs; /*list of SalOp. There can be multiple instances of a same Friend that subscribe to our presence*/
	LinphonePrivate::SalPresenceOp *outsub;
	LinphoneSubscribePolicy pol;
	MSList *presence_models; /* list of LinphoneFriendPresence. It associates SIP URIs and phone numbers with their respective presence models. */
	MSList *phone_number_sip_uri_map; /* list of LinphoneFriendPhoneNumberSipUri. It associates phone numbers with their corresponding SIP URIs. */
	struct _LinphoneCore *lc;
	BuddyInfo *info;
	char *refkey;
	bool_t subscribe;
	bool_t subscribe_active;
	bool_t inc_subscribe_pending;
	bool_t commit;
	bool_t initial_subscribes_sent; /*used to know if initial subscribe message was sent or not*/
	bool_t presence_received;
	LinphoneVcard *vcard;
	unsigned int storage_id;
	LinphoneFriendList *friend_list;
	LinphoneSubscriptionState out_sub_state;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneFriend);

struct _LinphoneFriendListCbs {
	belle_sip_object_t base;
	void *user_data;
	LinphoneFriendListCbsContactCreatedCb contact_created_cb;
	LinphoneFriendListCbsContactDeletedCb contact_deleted_cb;
	LinphoneFriendListCbsContactUpdatedCb contact_updated_cb;
	LinphoneFriendListCbsSyncStateChangedCb sync_state_changed_cb;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneFriendListCbs);

struct _LinphoneFriendList {
	belle_sip_object_t base;
	void *user_data;
	LinphoneCore *lc;
	LinphoneEvent *event;
	char *display_name;
	char *rls_uri; /*this field is take in sync with rls_addr*/
	LinphoneAddress *rls_addr;
	MSList *friends;
	bctbx_map_t *friends_map;
	bctbx_map_t *friends_map_uri;
	unsigned char *content_digest;
	int expected_notification_version;
	unsigned int storage_id;
	char *uri;
	MSList *dirty_friends_to_update;
	int revision;
	LinphoneFriendListCbs *cbs;
	bool_t enable_subscriptions;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneFriendList);



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
	LinphoneSipTransports transports;
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
	bool_t save_auth_info; // if true, auth infos will be write in the config file when they are added to the list
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
	char* audio_multicast_addr;
	bool_t audio_multicast_enabled;
	int audio_multicast_ttl;
	char* video_multicast_addr;
	int video_multicast_ttl;
	bool_t video_multicast_enabled;
	int text_rtp_min_port;
	int text_rtp_max_port;
}rtp_config_t;



typedef struct net_config
{
	char *nat_address; /* may be IP or host name */
	char *nat_address_ip; /* ip translated from nat_address */
	struct addrinfo *stun_addrinfo;
	int download_bw;
	int upload_bw;
	int mtu;
	OrtpNetworkSimulatorParams netsim_params;
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
	MSList *video_codecs;
	MSList *text_codecs;
	int dyn_pt;
	int telephone_event_pt;
}codecs_config_t;

typedef struct video_config{
	struct _MSWebCam *device;
	const char **cams;
	MSVideoSize vsize;
	MSVideoSize preview_vsize; /*is 0,0 if no forced preview size is set, in which case vsize field above is used.*/
	LinphoneVideoDefinition *vdef;
	LinphoneVideoDefinition *preview_vdef;
	float fps;
	bool_t capture;
	bool_t show_local;
	bool_t display;
	bool_t selfview; /*during calls*/
	bool_t reuse_preview_source;
}video_config_t;

typedef struct text_config{
	bool_t enabled;
}text_config_t;

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


typedef struct _LinphoneToneDescription{
	LinphoneReason reason; /*the call error code*/
	LinphoneToneID toneid; /*A tone type to play when this error arrives. This is played using tone generator*/
	char *audiofile; /*An override audio file to play instead, when this error arrives*/
	/*Note that some tones are not affected to any error, in which case it is affected LinphoneReasonNone*/
}LinphoneToneDescription;

LinphoneToneDescription * linphone_tone_description_new(LinphoneReason reason, LinphoneToneID id, const char *audiofile);
void linphone_tone_description_destroy(LinphoneToneDescription *obj);
LinphoneToneDescription *linphone_core_get_call_error_tone(const LinphoneCore *lc, LinphoneReason reason);
void linphone_core_play_call_error_tone(LinphoneCore *lc, LinphoneReason reason);
void _linphone_core_set_tone(LinphoneCore *lc, LinphoneReason reason, LinphoneToneID id, const char *audiofile);
LINPHONE_PUBLIC const char *linphone_core_get_tone_file(const LinphoneCore *lc, LinphoneToneID id);

typedef struct _LinphoneTaskList{
	MSList *hooks;
}LinphoneTaskList;

void linphone_task_list_init(LinphoneTaskList *t);
void linphone_task_list_add(LinphoneTaskList *t, LinphoneCoreIterateHook hook, void *hook_data);
void linphone_task_list_remove(LinphoneTaskList *t, LinphoneCoreIterateHook hook, void *hook_data);
void linphone_task_list_run(LinphoneTaskList *t);
void linphone_task_list_free(LinphoneTaskList *t);


struct _LinphoneCoreCbs {
	belle_sip_object_t base;
	LinphoneCoreVTable *vtable;
	bool_t autorelease;
};

LinphoneCoreCbs * _linphone_core_cbs_new(void);
void _linphone_core_cbs_set_v_table(LinphoneCoreCbs *cbs, LinphoneCoreVTable *vtable, bool_t autorelease);

typedef struct _LCCallbackObj {
	LinphoneCoreCbFunc _func;
	void *_user_data;
} LCCallbackObj;

namespace LinphonePrivate {
	class Core;
};

struct _LinphoneCore
{
	belle_sip_object_t base;
	MSFactory* factory;
	MSList* vtable_refs;
	int vtable_notify_recursion;
	LinphonePrivate::Sal *sal;

	// For migration purposes
	std::shared_ptr<LinphonePrivate::Core> cppCore;

	void *platform_helper; /*is a LinphonePrivate::PlatformHelpers but cannot be used as is because private.h is compiled as C in testers.*/

	LinphoneGlobalState state;
	struct _LpConfig *config;
	MSList *default_audio_codecs;
	MSList *default_video_codecs;
	MSList *default_text_codecs;
	net_config_t net_conf;
	sip_config_t sip_conf;
	rtp_config_t rtp_conf;
	sound_config_t sound_conf;
	video_config_t video_conf;
	text_config_t text_conf;
	codecs_config_t codecs_conf;
	ui_config_t ui_conf;
	autoreplier_config_t autoreplier_conf;
	LinphoneProxyConfig *default_proxy;
	MSList *friends_lists;
	MSList *auth_info;
	struct _RingStream *ringstream;
	time_t dmfs_playing_start_time;
	LCCallbackObj preview_finished_cb;
	LinphoneCall *current_call;   /* the current call */
	MSList *calls;				/* all the processed calls */
	MSList *queued_calls;	/* used by the autoreplier */
	MSList *call_logs;
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
	uint64_t prevtime_ms;
	int audio_bw; /*IP bw consumed by audio codec, set as soon as used codec is known, its purpose is to know the remaining bw for video*/
	LinphoneCoreWaitingCallback wait_cb;
	void *wait_ctx;
	void *video_window_id;
	void *preview_window_id;
	time_t netup_time; /*time when network went reachable */
	struct _EcCalibrator *ecc;
	struct _EchoTester *ect;
	LinphoneTaskList hooks; /*tasks periodically executed in linphone_core_iterate()*/
	LinphoneConference *conf_ctx;
	char* zrtp_secrets_cache; /**< zrtp cache filename */
	char* user_certificates_path;
	LinphoneVideoPolicy video_policy;
	time_t network_last_check;
	LinphoneNatPolicy *nat_policy;
	LinphoneImNotifPolicy *im_notif_policy;

	bool_t use_files;
	bool_t apply_nat_settings;
	bool_t initial_subscribes_sent;
	bool_t bl_refresh;

	bool_t preview_finished;
	bool_t auto_net_state_mon;
	bool_t sip_network_reachable;
	bool_t media_network_reachable;

	bool_t network_reachable_to_be_notified; /*set to true when state must be notified in next iterate*/
	bool_t use_preview_window;
	bool_t network_last_status;
	bool_t ringstream_autorelease;

	bool_t vtables_running;
	bool_t send_call_stats_periodical_updates;
	bool_t forced_ice_relay;
	bool_t short_turn_refresh;

	char localip[LINPHONE_IPADDR_SIZE];
	int device_rotation;
	int max_calls;
	LinphoneTunnel *tunnel;
	char* device_id;
	char *logs_db_file;
	char *friends_db_file;
#ifdef SQLITE_STORAGE_ENABLED
	sqlite3 *zrtp_cache_db; /**< zrtp sqlite cache, used by both zrtp and lime */
	sqlite3 *logs_db;
	sqlite3 *friends_db;
	bool_t debug_storage;
#endif
	belle_http_provider_t *http_provider;
	belle_tls_crypto_config_t *http_crypto_config;
	belle_http_request_listener_t *provisioning_http_listener;
#ifdef ENABLE_UPDATE_CHECK
	belle_http_request_listener_t *update_check_http_listener;
	char *update_check_current_version;
#endif
	MSList *tones;
	LinphoneReason chat_deny_code;
	char *file_transfer_server;
	const char **supported_formats;
	LinphoneContent *log_collection_upload_information;
	LinphoneCoreCbs *current_cbs; // the latest LinphoneCoreCbs object to call a callback, see linphone_core_get_current_cbs()
	LinphoneRingtonePlayer *ringtoneplayer;
	LinphoneVcardContext *vcard_context;

	/*for tests only*/
	bool_t zrtp_not_available_simulation;

	/* string for TLS auth instead of path to files */
	char *tls_cert;
	char *tls_key;

	LinphoneAddress *default_rls_addr; /*default resource list server*/
	LinphoneImEncryptionEngine *im_encryption_engine;
	struct _LinphoneAccountCreatorService *default_ac_service;
	MSBandwidthController *bw_controller;

	// For migration purpose.
	bctbx_list_t *chat_rooms;
};

#ifdef __cplusplus
#define getPlatformHelpers(lc) static_cast<LinphonePrivate::PlatformHelpers*>(lc->platform_helper)
#endif


struct _LinphoneEvent{
	belle_sip_object_t base;
	LinphoneErrorInfo *ei;
	LinphoneSubscriptionDir dir;
	LinphoneCore *lc;
	LinphonePrivate::SalEventOp *op;
	SalCustomHeader *send_custom_headers;
	LinphoneSubscriptionState subscription_state;
	LinphonePublishState publish_state;
	void *userdata;
	char *name;
	int expires;
	bool_t terminating;
	bool_t is_out_of_dialog_op; /*used for out of dialog notify*/
	bool_t internal;
	bool_t oneshot;

	// For migration purpose. (Do not use directly!)
	// Cache.
	LinphoneAddress *to_address;
	LinphoneAddress *from_address;
	LinphoneAddress *remote_contact_address;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneEvent);

LinphoneTunnel *linphone_core_tunnel_new(LinphoneCore *lc);
void linphone_tunnel_configure(LinphoneTunnel *tunnel);
void linphone_tunnel_enable_logs_with_handler(LinphoneTunnel *tunnel, bool_t enabled, OrtpLogFunc logHandler);

/**
 * Check if we do not have exceed the number of simultaneous call
 *
 * @ingroup call_control
**/
bool_t linphone_core_can_we_add_call(LinphoneCore *lc);

int linphone_core_add_call( LinphoneCore *lc, LinphoneCall *call);
int linphone_core_del_call( LinphoneCore *lc, LinphoneCall *call);
int linphone_core_get_calls_nb(const LinphoneCore *lc);

void linphone_core_set_state(LinphoneCore *lc, LinphoneGlobalState gstate, const char *message);
void linphone_call_update_biggest_desc(LinphoneCall *call, SalMediaDescription *md);
void linphone_call_make_local_media_description(LinphoneCall *call);
void linphone_call_make_local_media_description_with_params(LinphoneCore *lc, LinphoneCall *call, LinphoneCallParams *params);

bool_t linphone_core_is_payload_type_usable_for_bandwidth(const LinphoneCore *lc, const PayloadType *pt, int bandwidth_limit);

#define linphone_core_ready(lc) ((lc)->state==LinphoneGlobalOn || (lc)->state==LinphoneGlobalShutdown)
void _linphone_core_configure_resolver(void);

void linphone_core_initialize_supported_content_types(LinphoneCore *lc);

struct _EcCalibrator{
	MSFactory *factory;
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
	bool_t play_cool_tones;
};

typedef struct _EcCalibrator EcCalibrator;

LinphoneEcCalibratorStatus ec_calibrator_get_status(EcCalibrator *ecc);

void ec_calibrator_destroy(EcCalibrator *ecc);

struct _EchoTester {
    MSFactory *factory;
    MSFilter *in,*out;
    MSSndCard *capture_card;
    MSSndCard *playback_card;
    MSTicker *ticker;
    unsigned int rate;
};

typedef struct _EchoTester EchoTester;

void linphone_call_set_broken(LinphoneCall *call);
void linphone_call_repair_if_broken(LinphoneCall *call);
void linphone_core_repair_calls(LinphoneCore *lc);
int linphone_core_preempt_sound_resources(LinphoneCore *lc);
int _linphone_call_pause(LinphoneCall *call);

/*conferencing subsystem*/
void _post_configure_audio_stream(AudioStream *st, LinphoneCore *lc, bool_t muted);
bool_t linphone_core_sound_resources_available(LinphoneCore *lc);
void linphone_core_notify_refer_state(LinphoneCore *lc, LinphoneCall *referer, LinphoneCall *newcall);
LINPHONE_PUBLIC unsigned int linphone_core_get_audio_features(LinphoneCore *lc);

void _linphone_core_codec_config_write(LinphoneCore *lc);

#define HOLD_OFF	(0)
#define HOLD_ON		(1)

#ifndef NB_MAX_CALLS
#define NB_MAX_CALLS	(10)
#endif

#define LINPHONE_MAX_CALL_HISTORY_UNLIMITED (-1)
#ifndef LINPHONE_MAX_CALL_HISTORY_SIZE
	#ifdef SQLITE_STORAGE_ENABLED
		#define LINPHONE_MAX_CALL_HISTORY_SIZE LINPHONE_MAX_CALL_HISTORY_UNLIMITED
	#else
		#define LINPHONE_MAX_CALL_HISTORY_SIZE 30
	#endif
#endif

LINPHONE_PUBLIC bctbx_list_t * linphone_core_read_call_logs_from_config_file(LinphoneCore *lc);
void call_logs_write_to_config_file(LinphoneCore *lc);
void linphone_core_call_log_storage_init(LinphoneCore *lc);
void linphone_core_call_log_storage_close(LinphoneCore *lc);
void linphone_core_store_call_log(LinphoneCore *lc, LinphoneCallLog *log);
LINPHONE_PUBLIC const MSList *linphone_core_get_call_history(LinphoneCore *lc);
LINPHONE_PUBLIC void linphone_core_delete_call_history(LinphoneCore *lc);
LINPHONE_PUBLIC void linphone_core_delete_call_log(LinphoneCore *lc, LinphoneCallLog *log);
LINPHONE_PUBLIC int linphone_core_get_call_history_size(LinphoneCore *lc);

int linphone_core_get_edge_bw(LinphoneCore *lc);
int linphone_core_get_edge_ptime(LinphoneCore *lc);

LinphoneCore *_linphone_core_new_with_config(LinphoneCoreCbs *cbs, struct _LpConfig *config, void *userdata, void *system_context);

int linphone_upnp_init(LinphoneCore *lc);
void linphone_upnp_destroy(LinphoneCore *lc);

#ifdef SQLITE_STORAGE_ENABLED
int _linphone_sqlite3_open(const char *db_file, sqlite3 **db);
#endif

void linphone_chat_message_set_time(LinphoneChatMessage* msg, time_t time);
void linphone_chat_message_set_incoming(LinphoneChatMessage *msg);
void linphone_chat_message_set_outgoing(LinphoneChatMessage *msg);
LinphoneChatMessageStateChangedCb linphone_chat_message_get_message_state_changed_cb(LinphoneChatMessage* msg);
void linphone_chat_message_set_message_state_changed_cb(LinphoneChatMessage* msg, LinphoneChatMessageStateChangedCb cb);
void linphone_chat_message_set_message_state_changed_cb_user_data(LinphoneChatMessage* msg, void *user_data);
void * linphone_chat_message_get_message_state_changed_cb_user_data(LinphoneChatMessage* msg);
void linphone_chat_message_set_state(LinphoneChatMessage *msg, LinphoneChatMessageState state);
void linphone_chat_message_set_message_id(LinphoneChatMessage *msg, char *id);
void linphone_chat_message_set_storage_id(LinphoneChatMessage *msg, unsigned int id);
SalCustomHeader * linphone_chat_message_get_sal_custom_headers(const LinphoneChatMessage *msg);
void linphone_chat_message_set_sal_custom_headers(LinphoneChatMessage *msg, SalCustomHeader *header);
belle_http_request_t * linphone_chat_message_get_http_request(const LinphoneChatMessage *msg);
void linphone_chat_message_set_http_request(LinphoneChatMessage *msg, belle_http_request_t *request);
void linphone_chat_message_set_file_transfer_information(LinphoneChatMessage *msg, LinphoneContent *content);
LinphoneChatMessageDir linphone_chat_message_get_direction(const LinphoneChatMessage *msg);
LinphonePrivate::SalOp * linphone_chat_message_get_sal_op(const LinphoneChatMessage *msg);
void linphone_chat_message_set_sal_op(LinphoneChatMessage *msg, LinphonePrivate::SalOp *op);
void linphone_chat_message_destroy(LinphoneChatMessage* msg);
void linphone_chat_message_update_state(LinphoneChatMessage *msg, LinphoneChatMessageState new_state);
void linphone_chat_message_set_is_secured(LinphoneChatMessage *msg, bool_t secured);
void linphone_chat_message_send_delivery_notification(LinphoneChatMessage *cm, LinphoneReason reason);
void linphone_chat_message_send_display_notification(LinphoneChatMessage *cm);
void _linphone_chat_message_cancel_file_transfer(LinphoneChatMessage *msg, bool_t unref);
int linphone_chat_room_upload_file(LinphoneChatMessage *msg);
LinphoneChatRoom *_linphone_core_create_chat_room_from_call(LinphoneCall *call);
void linphone_chat_room_remove_transient_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg);
void linphone_chat_message_deactivate(LinphoneChatMessage *msg);
void linphone_chat_message_release(LinphoneChatMessage *msg);
void linphone_chat_message_fetch_content_from_database(sqlite3 *db, LinphoneChatMessage *message, int content_id);

void linphone_core_play_named_tone(LinphoneCore *lc, LinphoneToneID id);
bool_t linphone_core_tone_indications_enabled(LinphoneCore*lc);
const char *linphone_core_create_uuid(LinphoneCore *lc);
void linphone_configure_op(LinphoneCore *lc, LinphonePrivate::SalOp *op, const LinphoneAddress *dest, SalCustomHeader *headers, bool_t with_contact);
void linphone_configure_op_with_proxy(LinphoneCore *lc, LinphonePrivate::SalOp *op, const LinphoneAddress *dest, SalCustomHeader *headers, bool_t with_contact, LinphoneProxyConfig *proxy);
void linphone_call_create_op(LinphoneCall *call);
void linphone_core_notify_info_message(LinphoneCore* lc,LinphonePrivate::SalOp *op, SalBodyHandler *body);
LinphoneContent * linphone_content_new(void);
LinphoneContent * linphone_content_copy(const LinphoneContent *ref);
SalBodyHandler *sal_body_handler_from_content(const LinphoneContent *content);
SalReason linphone_reason_to_sal(LinphoneReason reason);
LinphoneReason linphone_reason_from_sal(SalReason reason);
void linphone_error_info_to_sal(const LinphoneErrorInfo* ei, SalErrorInfo* sei);
LinphoneEvent *linphone_event_new(LinphoneCore *lc, LinphoneSubscriptionDir dir, const char *name, int expires);
LinphoneEvent *linphone_event_new_with_op(LinphoneCore *lc, LinphonePrivate::SalEventOp *op, LinphoneSubscriptionDir dir, const char *name);
void linphone_event_unpublish(LinphoneEvent *lev);
/**
 * Useful for out of dialog notify
 * */
LinphoneEvent *linphone_event_new_with_out_of_dialog_op(LinphoneCore *lc, LinphonePrivate::SalEventOp *op, LinphoneSubscriptionDir dir, const char *name);
void linphone_event_set_internal(LinphoneEvent *lev, bool_t internal);
bool_t linphone_event_is_internal(LinphoneEvent *lev);
void linphone_event_set_state(LinphoneEvent *lev, LinphoneSubscriptionState state);
void linphone_event_set_publish_state(LinphoneEvent *lev, LinphonePublishState state);
LinphoneSubscriptionState linphone_subscription_state_from_sal(SalSubscribeStatus ss);
LinphoneContent *linphone_content_from_sal_body_handler(SalBodyHandler *ref);
void linphone_core_invalidate_friend_subscriptions(LinphoneCore *lc);
void linphone_core_register_offer_answer_providers(LinphoneCore *lc);


struct _LinphoneContent {
	belle_sip_object_t base;
	void *user_data;
	SalBodyHandler *body_handler;
	char *name; /**< used by RCS File transfer messages to store the original filename of the file to be downloaded from server */
	char *key; /**< used by RCS File transfer messages to store the key to encrypt file if needed */
	size_t keyLength; /**< Length of key in bytes */
	void *cryptoContext; /**< crypto context used to encrypt file for RCS file transfer */
	bool_t owned_fields;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneContent);

struct _LinphoneBuffer {
	belle_sip_object_t base;
	void *user_data;
	uint8_t *content;	/**< A pointer to the buffer content */
	size_t size;	/**< The size of the buffer content */
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneBuffer);

struct _LinphoneNatPolicy {
	belle_sip_object_t base;
	void *user_data;
	LinphoneCore *lc;
	belle_sip_resolver_context_t *stun_resolver_context;
	struct addrinfo *stun_addrinfo;
	char *stun_server;
	char *stun_server_username;
	char *ref;
	bool_t stun_enabled;
	bool_t turn_enabled;
	bool_t ice_enabled;
	bool_t upnp_enabled;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneNatPolicy);

bool_t linphone_nat_policy_stun_server_activated(LinphoneNatPolicy *policy);
void linphone_nat_policy_save_to_config(const LinphoneNatPolicy *policy);

struct _LinphoneImNotifPolicy {
	belle_sip_object_t base;
	void *user_data;
	LinphoneCore *lc;
	bool_t send_is_composing;
	bool_t recv_is_composing;
	bool_t send_imdn_delivered;
	bool_t recv_imdn_delivered;
	bool_t send_imdn_displayed;
	bool_t recv_imdn_displayed;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneImNotifPolicy);

void linphone_core_create_im_notif_policy(LinphoneCore *lc);


/*****************************************************************************
 * XML-RPC interface                                                         *
 ****************************************************************************/

typedef struct _LinphoneXmlRpcArg {
	LinphoneXmlRpcArgType type;
	union {
		int i;
		char *s;
	} data;
} LinphoneXmlRpcArg;

struct _LinphoneXmlRpcRequestCbs {
	belle_sip_object_t base;
	void *user_data;
	LinphoneXmlRpcRequestCbsResponseCb response;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneXmlRpcRequestCbs);

struct _LinphoneXmlRpcRequest {
	belle_sip_object_t base;
	void *user_data;
	LinphoneXmlRpcRequestCbs *callbacks;
	belle_sip_list_t *arg_list;
	char *content;	/**< The string representation of the XML-RPC request */
	char *method;
	LinphoneXmlRpcStatus status;
	LinphoneXmlRpcArg response;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneXmlRpcRequest);

struct _LinphoneXmlRpcSession {
	belle_sip_object_t base;
	void *user_data;
	LinphoneCore *core;
	char *url;
	bool_t released;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneXmlRpcSession);



/*****************************************************************************
 * CardDAV interface                                                         *
 ****************************************************************************/

struct _LinphoneCardDavContext {
	LinphoneFriendList *friend_list;
	int ctag;
	void *user_data;
	LinphoneCardDavContactCreatedCb contact_created_cb;
	LinphoneCardDavContactUpdatedCb contact_updated_cb;
	LinphoneCardDavContactRemovedCb contact_removed_cb;
	LinphoneCardDavSynchronizationDoneCb sync_done_cb;
	LinphoneAuthInfo *auth_info;
};

struct _LinphoneCardDavQuery {
	LinphoneCardDavContext *context;
	char *url;
	const char *method;
	char *body;
	const char *depth;
	const char *ifmatch;
	belle_http_request_listener_t *http_request_listener;
	void *user_data;
	LinphoneCardDavQueryType type;
};

struct _LinphoneCardDavResponse {
	char *etag;
	char *url;
	char *vcard;
};

/*****************************************************************************
 * REMOTE PROVISIONING FUNCTIONS                                             *
 ****************************************************************************/

void linphone_configuring_terminated(LinphoneCore *lc, LinphoneConfiguringState state, const char *message);
int linphone_remote_provisioning_download_and_apply(LinphoneCore *lc, const char *remote_provisioning_uri);
LINPHONE_PUBLIC int linphone_remote_provisioning_load_file( LinphoneCore* lc, const char* file_path);

/*****************************************************************************
 * Player interface                                                          *
 ****************************************************************************/

struct _LinphonePlayerCbs {
	belle_sip_object_t base;
	void *user_data;
	LinphonePlayerCbsEofReachedCb eof;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphonePlayerCbs);

LinphonePlayerCbs *linphone_player_cbs_new(void);

struct _LinphonePlayer{
	belle_sip_object_t base;
	void *user_data;
	int (*open)(LinphonePlayer* player, const char *filename);
	int (*start)(LinphonePlayer* player);
	int (*pause)(LinphonePlayer* player);
	int (*seek)(LinphonePlayer* player, int time_ms);
	MSPlayerState (*get_state)(LinphonePlayer* player);
	int (*get_duration)(LinphonePlayer *player);
	int (*get_position)(LinphonePlayer *player);
	void (*close)(LinphonePlayer* player);
	void (*destroy)(LinphonePlayer *player);
	void *impl;
	LinphonePlayerCbs *callbacks;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphonePlayer);

LinphonePlayer * linphone_player_new(void);
void _linphone_player_destroy(LinphonePlayer *player);


/*****************************************************************************
 * XML UTILITY FUNCTIONS                                                     *
 ****************************************************************************/

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
void linphone_xml_xpath_context_set_node(xmlparsing_context_t *xml_ctx, xmlNodePtr node);
char * linphone_get_xml_text_content(xmlparsing_context_t *xml_ctx, const char *xpath_expression);
char * linphone_get_xml_attribute_text_content(xmlparsing_context_t *xml_ctx, const char *xpath_expression, const char *attribute_name);
void linphone_free_xml_text_content(char *text);
xmlXPathObjectPtr linphone_get_xml_xpath_object_for_node_list(xmlparsing_context_t *xml_ctx, const char *xpath_expression);
void linphone_xml_xpath_context_init_carddav_ns(xmlparsing_context_t *xml_ctx);

/*****************************************************************************
 * OTHER UTILITY FUNCTIONS                                                     *
 ****************************************************************************/
char * linphone_timestamp_to_rfc3339_string(time_t timestamp);


void linphone_error_info_from_sal_op(LinphoneErrorInfo *ei, const LinphonePrivate::SalOp *op);

void payload_type_set_enable(OrtpPayloadType *pt, bool_t value);
bool_t payload_type_enabled(const OrtpPayloadType *pt);

LinphonePayloadType *linphone_payload_type_new(LinphoneCore *lc, OrtpPayloadType *ortp_pt);
bool_t _linphone_core_check_payload_type_usability(const LinphoneCore *lc, const OrtpPayloadType *pt);
OrtpPayloadType *linphone_payload_type_get_ortp_pt(const LinphonePayloadType *pt);


const MSCryptoSuite * linphone_core_get_srtp_crypto_suites(LinphoneCore *lc);
MsZrtpCryptoTypesCount linphone_core_get_zrtp_key_agreement_suites(LinphoneCore *lc, MSZrtpKeyAgreement keyAgreements[MS_MAX_ZRTP_CRYPTO_TYPES]);
MsZrtpCryptoTypesCount linphone_core_get_zrtp_cipher_suites(LinphoneCore *lc, MSZrtpCipher ciphers[MS_MAX_ZRTP_CRYPTO_TYPES]);
MsZrtpCryptoTypesCount linphone_core_get_zrtp_hash_suites(LinphoneCore *lc, MSZrtpHash hashes[MS_MAX_ZRTP_CRYPTO_TYPES]);
MsZrtpCryptoTypesCount linphone_core_get_zrtp_auth_suites(LinphoneCore *lc, MSZrtpAuthTag authTags[MS_MAX_ZRTP_CRYPTO_TYPES]);
MsZrtpCryptoTypesCount linphone_core_get_zrtp_sas_suites(LinphoneCore *lc, MSZrtpSasType sasTypes[MS_MAX_ZRTP_CRYPTO_TYPES]);

struct _LinphoneImEncryptionEngineCbs {
	belle_sip_object_t base;
	void *user_data;
	LinphoneImEncryptionEngineCbsIncomingMessageCb process_incoming_message;
	LinphoneImEncryptionEngineCbsOutgoingMessageCb process_outgoing_message;
	LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb is_encryption_enabled_for_file_transfer;
	LinphoneImEncryptionEngineCbsGenerateFileTransferKeyCb generate_file_transfer_key;
	LinphoneImEncryptionEngineCbsDownloadingFileCb process_downlading_file;
	LinphoneImEncryptionEngineCbsUploadingFileCb process_uploading_file;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneImEncryptionEngineCbs);

LinphoneImEncryptionEngineCbs * linphone_im_encryption_engine_cbs_new(void);

struct _LinphoneImEncryptionEngine {
	belle_sip_object_t base;
	void *user_data;
	LinphoneCore *lc;
	LinphoneImEncryptionEngineCbs *callbacks;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneImEncryptionEngine);

struct _LinphoneRange {
	belle_sip_object_t base;
	void *user_data;
	int min;
	int max;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneRange);

LinphoneRange *linphone_range_new(void);

struct _LinphoneTransports {
	belle_sip_object_t base;
	void *user_data;
	int udp_port; /**< SIP/UDP port */
	int tcp_port; /**< SIP/TCP port */
	int dtls_port; /**< SIP/DTLS port */
	int tls_port; /**< SIP/TLS port */
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneTransports);

LINPHONE_PUBLIC LinphoneTransports *linphone_transports_new(void);

struct _LinphoneVideoActivationPolicy {
	belle_sip_object_t base;
	void *user_data;
	bool_t automatically_initiate; /**<Whether video shall be automatically proposed for outgoing calls.*/
	bool_t automatically_accept; /**<Whether video shall be automatically accepted for incoming calls*/
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneVideoActivationPolicy);

LINPHONE_PUBLIC LinphoneVideoActivationPolicy *linphone_video_activation_policy_new(void);

void linphone_core_notify_global_state_changed(LinphoneCore *lc, LinphoneGlobalState gstate, const char *message);
void linphone_core_notify_call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *message);
void linphone_core_notify_call_encryption_changed(LinphoneCore *lc, LinphoneCall *call, bool_t on, const char *authentication_token);
void linphone_core_notify_registration_state_changed(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message);
void linphone_core_notify_new_subscription_requested(LinphoneCore *lc, LinphoneFriend *lf, const char *url);
void linphone_core_notify_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username, const char *domain);
void linphone_core_notify_authentication_requested(LinphoneCore *lc, LinphoneAuthInfo *auth_info, LinphoneAuthMethod method);
void linphone_core_notify_call_log_updated(LinphoneCore *lc, LinphoneCallLog *newcl);
void linphone_core_notify_text_message_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from, const char *message);
void linphone_core_notify_message_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage *message);
void linphone_core_notify_message_received_unable_decrypt(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage *message);
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
void linphone_core_notify_friend_list_created(LinphoneCore *lc, LinphoneFriendList *list);
void linphone_core_notify_friend_list_removed(LinphoneCore *lc, LinphoneFriendList *list);
void linphone_core_notify_call_created(LinphoneCore *lc, LinphoneCall *call);
void linphone_core_notify_version_update_check_result_received(LinphoneCore *lc, LinphoneVersionUpdateCheckResult result, const char *version, const char *url);
void linphone_core_notify_chat_room_instantiated (LinphoneCore *lc, LinphoneChatRoom *cr);

void linphone_core_notify_ec_calibration_result(LinphoneCore *lc, LinphoneEcCalibratorStatus status, int delay_ms);
void linphone_core_notify_ec_calibration_audio_init(LinphoneCore *lc);
void linphone_core_notify_ec_calibration_audio_uninit(LinphoneCore *lc);

void set_playback_gain_db(AudioStream *st, float gain);

LinphoneMediaDirection media_direction_from_sal_stream_dir(SalStreamDir dir);
SalStreamDir sal_dir_from_call_params_dir(LinphoneMediaDirection cpdir);

/*****************************************************************************
 * LINPHONE CONTENT PRIVATE ACCESSORS                                        *
 ****************************************************************************/

/**
 * Get the address of the crypto context associated with a RCS file transfer message if encrypted
 * @param[in] content LinphoneContent object.
 * @return The address of the pointer to the crypto context. Crypto context is managed(alloc/free)
 *         by the encryption/decryption functions, so we give the address to store/retrieve the pointer
 */
void ** linphone_content_get_cryptoContext_address(LinphoneContent *content);


struct _VTableReference{
	LinphoneCoreCbs *cbs;
	bool_t valid;
	bool_t autorelease;
	bool_t internal;
};

typedef struct _VTableReference  VTableReference;

void v_table_reference_destroy(VTableReference *ref);

LINPHONE_PUBLIC void _linphone_core_add_callbacks(LinphoneCore *lc, LinphoneCoreCbs *vtable, bool_t internal);

MSWebCam *get_nowebcam_device(MSFactory *f);

LinphoneLimeState linphone_core_lime_for_file_sharing_enabled(const LinphoneCore *lc);

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneTunnelConfig);

int linphone_core_get_default_proxy_config_index(LinphoneCore *lc);

char *linphone_presence_model_to_xml(LinphonePresenceModel *model) ;

#define LINPHONE_SQLITE3_VFS "sqlite3bctbx_vfs"

bool_t linphone_call_state_is_early(LinphoneCallState state);

struct _LinphoneErrorInfo{
	belle_sip_object_t base;
	LinphoneReason reason;
	char *protocol; /* */
	int protocol_code; /*from SIP response*/
	char *phrase; /*from SIP response*/
	char *warnings; /*from SIP response*/
	char *full_string; /*concatenation of status_string + warnings*/
	struct _LinphoneErrorInfo *sub_ei;
};
BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneErrorInfo);

void linphone_core_report_call_log(LinphoneCore *lc, LinphoneCallLog *call_log);
void linphone_core_report_early_failed_call(LinphoneCore *lc, LinphoneCallDir dir, LinphoneAddress *from, LinphoneAddress *to, LinphoneErrorInfo *ei);

struct _LinphoneVideoDefinition {
	belle_sip_object_t base;
	void *user_data;
	unsigned int width; /**< The width of the video */
	unsigned int height; /**< The height of the video */
	char *name; /** The name of the video definition */
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneVideoDefinition);

LinphoneVideoDefinition * linphone_video_definition_new(unsigned int width, unsigned int height, const char *name);

LinphoneVideoDefinition * linphone_factory_find_supported_video_definition(const LinphoneFactory *factory, unsigned int width, unsigned int height);
LinphoneVideoDefinition * linphone_factory_find_supported_video_definition_by_name(const LinphoneFactory *factory, const char *name);

const char* _linphone_config_load_from_xml_string(LpConfig *lpc, const char *buffer);
LinphoneNatPolicy * linphone_config_create_nat_policy_from_section(const LinphoneConfig *config, const char* section);


SalCustomHeader *linphone_info_message_get_headers (const LinphoneInfoMessage *im);


#ifdef __cplusplus
}
#endif

#endif /* _PRIVATE_H */
