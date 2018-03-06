/*
 * private_functions.h
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _PRIVATE_FUNCTIONS_H_
#define _PRIVATE_FUNCTIONS_H_

#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <mediastreamer2/msconference.h>

#include "private_types.h"
#include "tester_utils.h"
#include "sal/op.h"
#include "sal/event-op.h"

#include "linphone/core_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

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
LinphoneCallLog * linphone_call_log_new(LinphoneCallDir dir, LinphoneAddress *from, LinphoneAddress * to);
LinphonePlayer *linphone_call_build_player(LinphoneCall*call);

LinphonePrivate::SalCallOp *linphone_call_get_op(const LinphoneCall *call);
LinphoneProxyConfig * linphone_call_get_dest_proxy(const LinphoneCall *call);
LINPHONE_PUBLIC MediaStream * linphone_call_get_stream(LinphoneCall *call, LinphoneStreamType type);
LinphoneCallLog * linphone_call_get_log(const LinphoneCall *call);
IceSession * linphone_call_get_ice_session(const LinphoneCall *call);
bool_t linphone_call_get_audio_muted(const LinphoneCall *call);
void linphone_call_set_audio_muted(LinphoneCall *call, bool_t value);
bool_t linphone_call_get_all_muted(const LinphoneCall *call);
void _linphone_call_set_conf_ref (LinphoneCall *call, LinphoneConference *ref);
MSAudioEndpoint *_linphone_call_get_endpoint (const LinphoneCall *call);
void _linphone_call_set_endpoint (LinphoneCall *call, MSAudioEndpoint *endpoint);

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
void _linphone_core_uninit(LinphoneCore *lc);
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
LINPHONE_PUBLIC void linphone_call_stats_fill(LinphoneCallStats *stats, MediaStream *ms, OrtpEvent *ev);
void linphone_call_stats_update(LinphoneCallStats *stats, MediaStream *stream);
LinphoneCallStats *_linphone_call_stats_new(void);
void _linphone_call_stats_uninit(LinphoneCallStats *stats);
void _linphone_call_stats_clone(LinphoneCallStats *dst, const LinphoneCallStats *src);
void _linphone_call_stats_set_ice_state (LinphoneCallStats *stats, LinphoneIceState state);
void _linphone_call_stats_set_type (LinphoneCallStats *stats, LinphoneStreamType type);
void _linphone_call_stats_set_received_rtcp (LinphoneCallStats *stats, mblk_t *m);
mblk_t *_linphone_call_stats_get_sent_rtcp (const LinphoneCallStats *stats);
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
int _linphone_core_apply_transports(LinphoneCore *lc);

void linphone_core_start_waiting(LinphoneCore *lc, const char *purpose);
void linphone_core_update_progress(LinphoneCore *lc, const char *purpose, float progresses);
void linphone_core_stop_waiting(LinphoneCore *lc);

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
LinphoneChatRoom *_linphone_server_group_chat_room_new (LinphoneCore *core, LinphonePrivate::SalCallOp *op);
void linphone_chat_room_set_call(LinphoneChatRoom *cr, LinphoneCall *call);
LinphoneChatRoomCbs * _linphone_chat_room_cbs_new (void);
void _linphone_chat_room_notify_is_composing_received(LinphoneChatRoom *cr, const LinphoneAddress *remoteAddr, bool_t isComposing);
void _linphone_chat_room_notify_message_received(LinphoneChatRoom *cr, LinphoneChatMessage *msg);
void _linphone_chat_room_notify_participant_added(LinphoneChatRoom *cr, const LinphoneEventLog *event_log);
void _linphone_chat_room_notify_participant_removed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log);
void _linphone_chat_room_notify_participant_device_added(LinphoneChatRoom *cr, const LinphoneEventLog *event_log);
void _linphone_chat_room_notify_participant_device_removed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log);
void _linphone_chat_room_notify_participant_admin_status_changed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log);
void _linphone_chat_room_notify_state_changed(LinphoneChatRoom *cr, LinphoneChatRoomState newState);
void _linphone_chat_room_notify_subject_changed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log);
void _linphone_chat_room_notify_undecryptable_message_received(LinphoneChatRoom *cr, LinphoneChatMessage *msg);
void _linphone_chat_room_notify_chat_message_received(LinphoneChatRoom *cr, const LinphoneEventLog *event_log);
void _linphone_chat_room_notify_chat_message_sent(LinphoneChatRoom *cr, const LinphoneEventLog *event_log);
void _linphone_chat_room_notify_conference_address_generation(LinphoneChatRoom *cr);
void _linphone_chat_room_notify_participant_device_fetched(LinphoneChatRoom *cr, const LinphoneAddress *participantAddr);
void _linphone_chat_room_notify_participants_capabilities_checked(LinphoneChatRoom *cr, const LinphoneAddress *deviceAddr, const bctbx_list_t *participantsAddr);
void _linphone_chat_room_notify_chat_message_should_be_stored(LinphoneChatRoom *cr, LinphoneChatMessage *msg);
void _linphone_chat_room_clear_callbacks (LinphoneChatRoom *cr);

LinphoneToneDescription * linphone_tone_description_new(LinphoneReason reason, LinphoneToneID id, const char *audiofile);
void linphone_tone_description_destroy(LinphoneToneDescription *obj);
LinphoneToneDescription *linphone_core_get_call_error_tone(const LinphoneCore *lc, LinphoneReason reason);
void linphone_core_play_call_error_tone(LinphoneCore *lc, LinphoneReason reason);
void _linphone_core_set_tone(LinphoneCore *lc, LinphoneReason reason, LinphoneToneID id, const char *audiofile);
LINPHONE_PUBLIC const char *linphone_core_get_tone_file(const LinphoneCore *lc, LinphoneToneID id);

void linphone_task_list_init(LinphoneTaskList *t);
void linphone_task_list_add(LinphoneTaskList *t, LinphoneCoreIterateHook hook, void *hook_data);
void linphone_task_list_remove(LinphoneTaskList *t, LinphoneCoreIterateHook hook, void *hook_data);
void linphone_task_list_run(LinphoneTaskList *t);
void linphone_task_list_free(LinphoneTaskList *t);

LinphoneCoreCbs * _linphone_core_cbs_new(void);
void _linphone_core_cbs_set_v_table(LinphoneCoreCbs *cbs, LinphoneCoreVTable *vtable, bool_t autorelease);


LinphoneTunnel *linphone_core_tunnel_new(LinphoneCore *lc);
void linphone_tunnel_configure(LinphoneTunnel *tunnel);
void linphone_tunnel_enable_logs_with_handler(LinphoneTunnel *tunnel, bool_t enabled, OrtpLogFunc logHandler);

int linphone_core_get_calls_nb(const LinphoneCore *lc);

void linphone_core_set_state(LinphoneCore *lc, LinphoneGlobalState gstate, const char *message);
void linphone_call_update_biggest_desc(LinphoneCall *call, SalMediaDescription *md);
void linphone_call_make_local_media_description_with_params(LinphoneCore *lc, LinphoneCall *call, LinphoneCallParams *params);

bool_t linphone_core_is_payload_type_usable_for_bandwidth(const LinphoneCore *lc, const PayloadType *pt, int bandwidth_limit);

#define linphone_core_ready(lc) ((lc)->state==LinphoneGlobalOn || (lc)->state==LinphoneGlobalShutdown)
void _linphone_core_configure_resolver(void);

void linphone_core_initialize_supported_content_types(LinphoneCore *lc);

LinphoneEcCalibratorStatus ec_calibrator_get_status(EcCalibrator *ecc);

void ec_calibrator_destroy(EcCalibrator *ecc);

int linphone_core_preempt_sound_resources(LinphoneCore *lc);
int _linphone_call_pause(LinphoneCall *call);

/*conferencing subsystem*/
void _post_configure_audio_stream(AudioStream *st, LinphoneCore *lc, bool_t muted);
bool_t linphone_core_sound_resources_available(LinphoneCore *lc);
LINPHONE_PUBLIC unsigned int linphone_core_get_audio_features(LinphoneCore *lc);

void _linphone_core_codec_config_write(LinphoneCore *lc);

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

LinphoneCore *_linphone_core_new_with_config(LinphoneCoreCbs *cbs, struct _LpConfig *config, void *userdata, void *system_context, bool_t automatically_start);

int linphone_upnp_init(LinphoneCore *lc);
void linphone_upnp_destroy(LinphoneCore *lc);

#ifdef SQLITE_STORAGE_ENABLED
int _linphone_sqlite3_open(const char *db_file, sqlite3 **db);
#endif

LinphoneChatMessageStateChangedCb linphone_chat_message_get_message_state_changed_cb(LinphoneChatMessage* msg);
void linphone_chat_message_set_message_state_changed_cb(LinphoneChatMessage* msg, LinphoneChatMessageStateChangedCb cb);
void linphone_chat_message_set_message_state_changed_cb_user_data(LinphoneChatMessage* msg, void *user_data);
void * linphone_chat_message_get_message_state_changed_cb_user_data(LinphoneChatMessage* msg);
LinphoneChatRoom *_linphone_core_create_chat_room_from_call(LinphoneCall *call);

void linphone_core_play_named_tone(LinphoneCore *lc, LinphoneToneID id);
bool_t linphone_core_tone_indications_enabled(LinphoneCore*lc);
const char *linphone_core_create_uuid(LinphoneCore *lc);
void linphone_configure_op(LinphoneCore *lc, LinphonePrivate::SalOp *op, const LinphoneAddress *dest, SalCustomHeader *headers, bool_t with_contact);
void linphone_configure_op_with_proxy(LinphoneCore *lc, LinphonePrivate::SalOp *op, const LinphoneAddress *dest, SalCustomHeader *headers, bool_t with_contact, LinphoneProxyConfig *proxy);
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
void _linphone_event_notify_notify_response(const LinphoneEvent *lev);
LinphoneSubscriptionState linphone_subscription_state_from_sal(SalSubscribeStatus ss);
LinphoneContent *linphone_content_from_sal_body_handler(SalBodyHandler *ref);
void linphone_core_invalidate_friend_subscriptions(LinphoneCore *lc);
void linphone_core_register_offer_answer_providers(LinphoneCore *lc);

bool_t linphone_nat_policy_stun_server_activated(LinphoneNatPolicy *policy);
void linphone_nat_policy_save_to_config(const LinphoneNatPolicy *policy);

void linphone_core_create_im_notif_policy(LinphoneCore *lc);


/*****************************************************************************
 * REMOTE PROVISIONING FUNCTIONS                                             *
 ****************************************************************************/

void linphone_configuring_terminated(LinphoneCore *lc, LinphoneConfiguringState state, const char *message);
int linphone_remote_provisioning_download_and_apply(LinphoneCore *lc, const char *remote_provisioning_uri);
LINPHONE_PUBLIC int linphone_remote_provisioning_load_file( LinphoneCore* lc, const char* file_path);


/*****************************************************************************
 * Player interface                                                          *
 ****************************************************************************/

LinphonePlayerCbs *linphone_player_cbs_new(void);
LinphonePlayer * linphone_player_new(void);
void _linphone_player_destroy(LinphonePlayer *player);


/*****************************************************************************
 * XML UTILITY FUNCTIONS                                                     *
 ****************************************************************************/

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

LinphoneImEncryptionEngineCbs * linphone_im_encryption_engine_cbs_new(void);

LinphoneRange *linphone_range_new(void);

LINPHONE_PUBLIC LinphoneTransports *linphone_transports_new(void);

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
void linphone_core_notify_chat_room_state_changed (LinphoneCore *lc, LinphoneChatRoom *cr, LinphoneChatRoomState state);

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

void v_table_reference_destroy(VTableReference *ref);

LINPHONE_PUBLIC void _linphone_core_add_callbacks(LinphoneCore *lc, LinphoneCoreCbs *vtable, bool_t internal);

MSWebCam *get_nowebcam_device(MSFactory *f);

LinphoneLimeState linphone_core_lime_for_file_sharing_enabled(const LinphoneCore *lc);

int linphone_core_get_default_proxy_config_index(LinphoneCore *lc);

char *linphone_presence_model_to_xml(LinphonePresenceModel *model) ;

void linphone_core_report_call_log(LinphoneCore *lc, LinphoneCallLog *call_log);
void linphone_core_report_early_failed_call(LinphoneCore *lc, LinphoneCallDir dir, LinphoneAddress *from, LinphoneAddress *to, LinphoneErrorInfo *ei);

LinphoneVideoDefinition * linphone_video_definition_new(unsigned int width, unsigned int height, const char *name);

LinphoneVideoDefinition * linphone_factory_find_supported_video_definition(const LinphoneFactory *factory, unsigned int width, unsigned int height);
LinphoneVideoDefinition * linphone_factory_find_supported_video_definition_by_name(const LinphoneFactory *factory, const char *name);

const char* _linphone_config_load_from_xml_string(LpConfig *lpc, const char *buffer);
LinphoneNatPolicy * linphone_config_create_nat_policy_from_section(const LinphoneConfig *config, const char* section);

SalCustomHeader *linphone_info_message_get_headers (const LinphoneInfoMessage *im);
void linphone_info_message_set_headers (LinphoneInfoMessage *im, const SalCustomHeader *headers);

#ifdef __cplusplus
}
#endif

#endif /* _PRIVATE_FUNCTIONS_H_ */
