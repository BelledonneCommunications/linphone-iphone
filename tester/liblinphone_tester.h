/*
	liblinphone_tester - liblinphone test suite
	Copyright (C) 2013  Belledonne Communications SARL

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef LIBLINPHONE_TESTER_H_
#define LIBLINPHONE_TESTER_H_



#include <bctoolbox/tester.h>
#include "linphone/core.h"
#include <mediastreamer2/msutils.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern test_suite_t setup_test_suite;
extern test_suite_t register_test_suite;
extern test_suite_t call_test_suite;
extern test_suite_t call_video_test_suite;
extern test_suite_t message_test_suite;
extern test_suite_t presence_test_suite;
extern test_suite_t presence_server_test_suite;
extern test_suite_t upnp_test_suite;
extern test_suite_t event_test_suite;
extern test_suite_t flexisip_test_suite;
extern test_suite_t stun_test_suite;
extern test_suite_t remote_provisioning_test_suite;
extern test_suite_t quality_reporting_test_suite;
extern test_suite_t log_collection_test_suite;
extern test_suite_t tunnel_test_suite;
extern test_suite_t player_test_suite;
extern test_suite_t dtmf_test_suite;
extern test_suite_t offeranswer_test_suite;
extern test_suite_t video_test_suite;
extern test_suite_t multicast_call_test_suite;
extern test_suite_t multi_call_test_suite;
extern test_suite_t proxy_config_test_suite;
#ifdef VCARD_ENABLED
extern test_suite_t vcard_test_suite;
#endif
extern test_suite_t audio_bypass_suite;
#if HAVE_SIPP
extern test_suite_t complex_sip_call_test_suite;
#endif
extern int manager_count;

extern int liblinphone_tester_ipv6_available(void);
extern int liblinphone_tester_ipv4_available(void);

/**
 * @brief Tells the tester whether or not to clean the accounts it has created between runs.
 * @details Setting this to 1 will not clear the list of created accounts between successive
 * calls to liblinphone_run_tests(). Some testing APIs call this function for *each* test,
 * in which case we should keep the accounts that were created for further testing.
 *
 * You are supposed to manually call liblinphone_tester_clear_account when all the tests are
 * finished.
 *
 * @param keep 1 to keep the accounts in-between runs, 0 to clear them after each run.
 */
extern void liblinphone_tester_keep_accounts( int keep );

/**
 * @brief Tells the test whether to not remove recorded audio/video files after the tests.
 * @details By default recorded files are erased after the test, unless the test is failed.
**/
void liblinphone_tester_keep_recorded_files(int keep);

/**
 * @brief Disable the automatic object leak detection. This is useful because the object leak detector prevents valgrind from seeing the leaks.
 * @details By default object leak detector is enabled.
**/
void liblinphone_tester_disable_leak_detector(int disabled);

/**
 * @brief Clears the created accounts during the testing session.
 */
extern void liblinphone_tester_clear_accounts(void);


extern const char* test_domain;
extern const char* auth_domain;
extern const char* test_username;
extern const char* test_password;
extern const char* test_route;
extern const char* userhostsfile;
extern bool_t liblinphone_tester_tls_support_disabled;
extern const MSAudioDiffParams audio_cmp_params;
extern const char *liblinphone_tester_mire_id;
extern bool_t liblinphonetester_ipv6;
extern bool_t liblinphonetester_show_account_manager_logs;

typedef struct _stats {
	int number_of_LinphoneRegistrationNone;
	int number_of_LinphoneRegistrationProgress ;
	int number_of_LinphoneRegistrationOk ;
	int number_of_LinphoneRegistrationCleared ;
	int number_of_LinphoneRegistrationFailed ;
	int number_of_auth_info_requested ;


	int number_of_LinphoneCallIncomingReceived;
	int number_of_LinphoneCallOutgoingInit;
	int number_of_LinphoneCallOutgoingProgress;
	int number_of_LinphoneCallOutgoingRinging;
	int number_of_LinphoneCallOutgoingEarlyMedia;
	int number_of_LinphoneCallConnected;
	int number_of_LinphoneCallStreamsRunning;
	int number_of_LinphoneCallPausing;
	int number_of_LinphoneCallPaused;
	int number_of_LinphoneCallResuming;
	int number_of_LinphoneCallRefered;
	int number_of_LinphoneCallError;
	int number_of_LinphoneCallEnd;
	int number_of_LinphoneCallPausedByRemote;
	int number_of_LinphoneCallUpdatedByRemote;
	int number_of_LinphoneCallIncomingEarlyMedia;
	int number_of_LinphoneCallUpdating;
	int number_of_LinphoneCallReleased;
	int number_of_LinphoneCallEarlyUpdatedByRemote;
	int number_of_LinphoneCallEarlyUpdating;

	int number_of_LinphoneTransferCallOutgoingInit;
	int number_of_LinphoneTransferCallOutgoingProgress;
	int number_of_LinphoneTransferCallOutgoingRinging;
	int number_of_LinphoneTransferCallOutgoingEarlyMedia;
	int number_of_LinphoneTransferCallConnected;
	int number_of_LinphoneTransferCallStreamsRunning;
	int number_of_LinphoneTransferCallError;

	int number_of_LinphoneMessageReceived;
	int number_of_LinphoneMessageReceivedWithFile;
	int number_of_LinphoneMessageReceivedLegacy;
	int number_of_LinphoneMessageExtBodyReceived;
	int number_of_LinphoneMessageInProgress;
	int number_of_LinphoneMessageDelivered;
	int number_of_LinphoneMessageNotDelivered;
	int number_of_LinphoneMessageFileTransferDone;
	int number_of_LinphoneMessageDeliveredToUser;
	int number_of_LinphoneMessageDisplayed;
	int number_of_LinphoneIsComposingActiveReceived;
	int number_of_LinphoneIsComposingIdleReceived;
	int progress_of_LinphoneFileTransfer;

	int number_of_IframeDecoded;

	int number_of_NewSubscriptionRequest;
	int number_of_NotifyReceived;
	int number_of_NotifyPresenceReceived;
	int number_of_NotifyPresenceReceivedForUriOrTel;
	int number_of_LinphonePresenceActivityOffline;
	int number_of_LinphonePresenceActivityOnline;
	int number_of_LinphonePresenceActivityAppointment;
	int number_of_LinphonePresenceActivityAway;
	int number_of_LinphonePresenceActivityBreakfast;
	int number_of_LinphonePresenceActivityBusy;
	int number_of_LinphonePresenceActivityDinner;
	int number_of_LinphonePresenceActivityHoliday;
	int number_of_LinphonePresenceActivityInTransit;
	int number_of_LinphonePresenceActivityLookingForWork;
	int number_of_LinphonePresenceActivityLunch;
	int number_of_LinphonePresenceActivityMeal;
	int number_of_LinphonePresenceActivityMeeting;
	int number_of_LinphonePresenceActivityOnThePhone;
	int number_of_LinphonePresenceActivityOther;
	int number_of_LinphonePresenceActivityPerformance;
	int number_of_LinphonePresenceActivityPermanentAbsence;
	int number_of_LinphonePresenceActivityPlaying;
	int number_of_LinphonePresenceActivityPresentation;
	int number_of_LinphonePresenceActivityShopping;
	int number_of_LinphonePresenceActivitySleeping;
	int number_of_LinphonePresenceActivitySpectator;
	int number_of_LinphonePresenceActivitySteering;
	int number_of_LinphonePresenceActivityTravel;
	int number_of_LinphonePresenceActivityTV;
	int number_of_LinphonePresenceActivityUnknown;
	int number_of_LinphonePresenceActivityVacation;
	int number_of_LinphonePresenceActivityWorking;
	int number_of_LinphonePresenceActivityWorship;
	const LinphonePresenceModel *last_received_presence;

	int number_of_LinphonePresenceBasicStatusOpen;
	int number_of_LinphonePresenceBasicStatusClosed;

	int number_of_inforeceived;
	LinphoneInfoMessage* last_received_info_message;

	int number_of_LinphoneSubscriptionIncomingReceived;
	int number_of_LinphoneSubscriptionOutgoingProgress;
	int number_of_LinphoneSubscriptionPending;
	int number_of_LinphoneSubscriptionActive;
	int number_of_LinphoneSubscriptionTerminated;
	int number_of_LinphoneSubscriptionError;
	int number_of_LinphoneSubscriptionExpiring;

	int number_of_LinphonePublishProgress;
	int number_of_LinphonePublishOk;
	int number_of_LinphonePublishExpiring;
	int number_of_LinphonePublishError;
	int number_of_LinphonePublishCleared;

	int number_of_LinphoneConfiguringSkipped;
	int number_of_LinphoneConfiguringFailed;
	int number_of_LinphoneConfiguringSuccessful;

	int number_of_LinphoneCallEncryptedOn;
	int number_of_LinphoneCallEncryptedOff;
	int number_of_NetworkReachableTrue;
	int number_of_NetworkReachableFalse;
	int number_of_player_eof;
	LinphoneChatMessage* last_received_chat_message;

	char * dtmf_list_received;
	int dtmf_count;

	int number_of_LinphoneCallStatsUpdated;
	int number_of_rtcp_sent;
	int number_of_rtcp_received; /*total number of rtcp packet received */
	int number_of_rtcp_received_via_mux;/*number of rtcp packet received in rtcp-mux mode*/

	int number_of_video_windows_created;

	int number_of_LinphoneFileTransferDownloadSuccessful;
	int number_of_LinphoneCoreLogCollectionUploadStateDelivered;
	int number_of_LinphoneCoreLogCollectionUploadStateNotDelivered;
	int number_of_LinphoneCoreLogCollectionUploadStateInProgress;
	int audio_download_bandwidth[3];
	int audio_upload_bandwidth[3];

	int video_download_bandwidth[3];
	int video_upload_bandwidth[3];
	int current_bandwidth_index[2] /*audio and video only*/;

	int number_of_rtcp_generic_nack;
}stats;


typedef struct _LinphoneCoreManager {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	stats stat;
	LinphoneAddress* identity;
	LinphoneEvent *lev;
	bool_t decline_subscribe;
	int number_of_bcunit_error_at_creation;
	char* phone_alias;
} LinphoneCoreManager;

typedef struct _LinphoneConferenceServer {
	LinphoneCoreManager base;
	LinphoneCall *first_call;
	LinphoneCoreVTable *vtable;
	LinphoneRegistrationState reg_state;
} LinphoneConferenceServer;

typedef struct _LinphoneCallTestParams {
	LinphoneCallParams *base;
	bool_t sdp_removal;
	bool_t sdp_simulate_error;
} LinphoneCallTestParams;


void liblinphone_tester_add_suites(void);

void linphone_core_manager_init(LinphoneCoreManager *mgr, const char* rc_file, const char* phone_alias);
void linphone_core_manager_start(LinphoneCoreManager *mgr, int check_for_proxies);
LinphoneCoreManager* linphone_core_manager_new3(const char* rc_file, int check_for_proxies, const char* phone_alias);
LinphoneCoreManager* linphone_core_manager_new2(const char* rc_file, int check_for_proxies);
LinphoneCoreManager* linphone_core_manager_new(const char* rc_file);
void linphone_core_manager_stop(LinphoneCoreManager *mgr);
void linphone_core_manager_uninit(LinphoneCoreManager *mgr);
void linphone_core_manager_wait_for_stun_resolution(LinphoneCoreManager *mgr);
void linphone_core_manager_destroy(LinphoneCoreManager* mgr);

void reset_counters( stats* counters);

void registration_state_changed(struct _LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message);
void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg);
void linphone_transfer_state_changed(LinphoneCore *lc, LinphoneCall *transfered, LinphoneCallState new_call_state);
void notify_presence_received(LinphoneCore *lc, LinphoneFriend * lf);
void notify_presence_received_for_uri_or_tel(LinphoneCore *lc, LinphoneFriend *lf, const char *uri_or_tel, const LinphonePresenceModel *presence);
void text_message_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from_address, const char *message);
void message_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage* message);
void file_transfer_received(LinphoneChatMessage *message, const LinphoneContent* content, const LinphoneBuffer *buffer);
LinphoneBuffer * tester_file_transfer_send(LinphoneChatMessage *message, const LinphoneContent* content, size_t offset, size_t size);
LinphoneBuffer * tester_memory_file_transfer_send(LinphoneChatMessage *message, const LinphoneContent* content, size_t offset, size_t size);
void file_transfer_progress_indication(LinphoneChatMessage *message, const LinphoneContent* content, size_t offset, size_t total);
void is_composing_received(LinphoneCore *lc, LinphoneChatRoom *room);
void info_message_received(LinphoneCore *lc, LinphoneCall *call, const LinphoneInfoMessage *msg);
void new_subscription_requested(LinphoneCore *lc, LinphoneFriend *lf, const char *url);
void linphone_subscription_state_change(LinphoneCore *lc, LinphoneEvent *ev, LinphoneSubscriptionState state);
void linphone_publish_state_changed(LinphoneCore *lc, LinphoneEvent *ev, LinphonePublishState state);
void linphone_notify_received(LinphoneCore *lc, LinphoneEvent *lev, const char *eventname, const LinphoneContent *content);
void linphone_configuration_status(LinphoneCore *lc, LinphoneConfiguringState status, const char *message);
void linphone_call_encryption_changed(LinphoneCore *lc, LinphoneCall *call, bool_t on, const char *authentication_token);
void dtmf_received(LinphoneCore *lc, LinphoneCall *call, int dtmf);
void call_stats_updated(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallStats *stats);

LinphoneAddress * create_linphone_address(const char * domain);
bool_t wait_for(LinphoneCore* lc_1, LinphoneCore* lc_2,int* counter,int value);
bool_t wait_for_list(MSList* lcs,int* counter,int value,int timeout_ms);
bool_t wait_for_until(LinphoneCore* lc_1, LinphoneCore* lc_2,int* counter,int value,int timout_ms);

bool_t call_with_params(LinphoneCoreManager* caller_mgr
						,LinphoneCoreManager* callee_mgr
						, const LinphoneCallParams *caller_params
						, const LinphoneCallParams *callee_params);
bool_t call_with_test_params(LinphoneCoreManager* caller_mgr
				,LinphoneCoreManager* callee_mgr
				,const LinphoneCallTestParams *caller_test_params
				,const LinphoneCallTestParams *callee_test_params);
bool_t call_with_params2(LinphoneCoreManager* caller_mgr
						,LinphoneCoreManager* callee_mgr
						, const LinphoneCallTestParams *caller_test_params
						, const LinphoneCallTestParams *callee_test_params
						, bool_t build_callee_params);

bool_t call(LinphoneCoreManager* caller_mgr,LinphoneCoreManager* callee_mgr);
bool_t request_video(LinphoneCoreManager* caller,LinphoneCoreManager* callee, bool_t use_accept_call_update);
void end_call(LinphoneCoreManager *m1, LinphoneCoreManager *m2);
void disable_all_audio_codecs_except_one(LinphoneCore *lc, const char *mime, int rate);
void disable_all_video_codecs_except_one(LinphoneCore *lc, const char *mime);
void disable_all_codecs(const MSList* elem, LinphoneCoreManager* call);
stats * get_stats(LinphoneCore *lc);
bool_t transport_supported(LinphoneTransportType transport);
LinphoneCoreManager *get_manager(LinphoneCore *lc);
const char *liblinphone_tester_get_subscribe_content(void);
const char *liblinphone_tester_get_notify_content(void);
void liblinphone_tester_chat_message_state_change(LinphoneChatMessage* msg,LinphoneChatMessageState state,void* ud);
void liblinphone_tester_chat_message_msg_state_changed(LinphoneChatMessage *msg, LinphoneChatMessageState state);
void liblinphone_tester_check_rtcp(LinphoneCoreManager* caller, LinphoneCoreManager* callee);
void liblinphone_tester_clock_start(MSTimeSpec *start);
bool_t liblinphone_tester_clock_elapsed(const MSTimeSpec *start, int value_ms);
void linphone_core_manager_check_accounts(LinphoneCoreManager *m);
void account_manager_destroy(void);
LinphoneCore* configure_lc_from(LinphoneCoreVTable* v_table, const char* path, const char* file, void* user_data);

void linphone_call_iframe_decoded_cb(LinphoneCall *call,void * user_data);
void call_paused_resumed_base(bool_t multicast,bool_t with_losses);
void simple_call_base(bool_t enable_multicast_recv_side);
void call_base_with_configfile(LinphoneMediaEncryption mode, bool_t enable_video,bool_t enable_relay,LinphoneFirewallPolicy policy,bool_t enable_tunnel, const char *marie_rc, const char *pauline_rc);
void call_base(LinphoneMediaEncryption mode, bool_t enable_video,bool_t enable_relay,LinphoneFirewallPolicy policy,bool_t enable_tunnel);
bool_t call_with_caller_params(LinphoneCoreManager* caller_mgr,LinphoneCoreManager* callee_mgr, const LinphoneCallParams *params);
bool_t pause_call_1(LinphoneCoreManager* mgr_1,LinphoneCall* call_1,LinphoneCoreManager* mgr_2,LinphoneCall* call_2);
void compare_files(const char *path1, const char *path2);
void check_media_direction(LinphoneCoreManager* mgr, LinphoneCall *call, MSList* lcs,LinphoneMediaDirection audio_dir, LinphoneMediaDirection video_dir);
void _call_with_ice_base(LinphoneCoreManager* pauline,LinphoneCoreManager* marie, bool_t caller_with_ice, bool_t callee_with_ice, bool_t random_ports, bool_t forced_relay);
int check_nb_media_starts(LinphoneCoreManager *caller, LinphoneCoreManager *callee, unsigned int caller_nb_media_starts, unsigned int callee_nb_media_starts);
void record_call(const char *filename, bool_t enableVideo, const char *video_codec);

/*
 * this function return max value in the last 3 seconds*/
int linphone_core_manager_get_max_audio_down_bw(const LinphoneCoreManager *mgr);
int linphone_core_manager_get_max_audio_up_bw(const LinphoneCoreManager *mgr);
int linphone_core_manager_get_mean_audio_down_bw(const LinphoneCoreManager *mgr);
int linphone_core_manager_get_mean_audio_up_bw(const LinphoneCoreManager *mgr);

void video_call_base_2(LinphoneCoreManager* pauline,LinphoneCoreManager* marie, bool_t using_policy,LinphoneMediaEncryption mode, bool_t callee_video_enabled, bool_t caller_video_enabled);

void liblinphone_tester_before_each(void);
void liblinphone_tester_after_each(void);
void liblinphone_tester_init(void(*ftester_printf)(int level, const char *fmt, va_list args));
void liblinphone_tester_uninit(void);
int liblinphone_tester_set_log_file(const char *filename);
bool_t check_ice(LinphoneCoreManager* caller, LinphoneCoreManager* callee, LinphoneIceState state);


LinphoneConferenceServer* linphone_conference_server_new(const char *rc_file, bool_t do_registration);
void linphone_conference_server_destroy(LinphoneConferenceServer *conf_srv);

LinphoneAddress * linphone_core_manager_resolve(LinphoneCoreManager *mgr, const LinphoneAddress *source);
FILE *sip_start(const char *senario, const char* dest_username, const char *passwd, LinphoneAddress* dest_addres);

void early_media_without_sdp_in_200_base( bool_t use_video, bool_t use_ice );


#ifdef __cplusplus
};
#endif

#endif /* LIBLINPHONE_TESTER_H_ */
