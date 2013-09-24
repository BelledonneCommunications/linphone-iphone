/*
	belle-sip - SIP (RFC3261) library.
    Copyright (C) 2010  Belledonne Communications SARL

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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



#include "CUnit/Basic.h"
#include "linphonecore.h"

typedef void (*test_function_t)(void);
typedef int (*test_suite_function_t)(const char *name);

typedef struct {
	const char *name;
	test_function_t func;
} test_t;

typedef struct {
	const char *name;
	CU_InitializeFunc init_func;
	CU_CleanupFunc cleanup_func;
	int nb_tests;
	test_t *tests;
} test_suite_t;

#ifdef __cplusplus
extern "C" {
#endif

extern const char *liblinphone_tester_file_prefix;
extern test_suite_t setup_test_suite;
extern test_suite_t register_test_suite;
extern test_suite_t call_test_suite;
extern test_suite_t message_test_suite;
extern test_suite_t presence_test_suite;
extern test_suite_t upnp_test_suite;
extern test_suite_t event_test_suite;


extern int liblinphone_tester_nb_test_suites(void);
extern int liblinphone_tester_nb_tests(const char *suite_name);
extern const char * liblinphone_tester_test_suite_name(int suite_index);
extern const char * liblinphone_tester_test_name(const char *suite_name, int test_index);
extern void liblinphone_tester_init(void);
extern void liblinphone_tester_uninit(void);
extern int liblinphone_tester_run_tests(const char *suite_name, const char *test_name);


#ifdef __cplusplus
};
#endif


const char* test_domain;
const char* auth_domain;
const char* test_username;
const char* test_password;
const char* test_route;


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

	int number_of_LinphoneTransferCallOutgoingInit;
	int number_of_LinphoneTransferCallOutgoingProgress;
	int number_of_LinphoneTransferCallOutgoingRinging;
	int number_of_LinphoneTransferCallOutgoingEarlyMedia;
	int number_of_LinphoneTransferCallConnected;
	int number_of_LinphoneTransferCallStreamsRunning;
	int number_of_LinphoneTransferCallError;

	int number_of_LinphoneMessageReceived;
	int number_of_LinphoneMessageReceivedLegacy;
	int number_of_LinphoneMessageExtBodyReceived;
	int number_of_LinphoneMessageInProgress;
	int number_of_LinphoneMessageDelivered;
	int number_of_LinphoneMessageNotDelivered;


	int number_of_IframeDecoded;

	int number_of_NewSubscriptionRequest;
	int number_of_NotifyReceived;
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
	
	int number_of_inforeceived;
	int number_of_inforeceived_with_body;

	int number_of_LinphoneSubscriptionIncomingReceived;
	int number_of_LinphoneSubscriptionOutgoingInit;
	int number_of_LinphoneSubscriptionPending;
	int number_of_LinphoneSubscriptionActive;
	int number_of_LinphoneSubscriptionTerminated;
	int number_of_LinphoneSubscriptionError;

	int number_of_LinphonePublishProgress;
	int number_of_LinphonePublishOk;
	int number_of_LinphonePublishExpiring;
	int number_of_LinphonePublishError;
	int number_of_LinphonePublishCleared;
}stats;

typedef struct _LinphoneCoreManager {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	stats stat;
	LinphoneAddress* identity;
	LinphoneEvent *lev;
	bool_t decline_subscribe;
} LinphoneCoreManager;

LinphoneCoreManager* linphone_core_manager_new2(const char* rc_file, int check_for_proxies);
LinphoneCoreManager* linphone_core_manager_new(const char* rc_file);
void linphone_core_manager_stop(LinphoneCoreManager *mgr);
void linphone_core_manager_destroy(LinphoneCoreManager* mgr);

void reset_counters( stats* counters);

void registration_state_changed(struct _LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message);
void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg);
void linphone_transfer_state_changed(LinphoneCore *lc, LinphoneCall *transfered, LinphoneCallState new_call_state);
void notify_presence_received(LinphoneCore *lc, LinphoneFriend * lf);
void text_message_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from_address, const char *message);
void message_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage* message);
void info_message_received(LinphoneCore *lc, LinphoneCall *call, const LinphoneInfoMessage *msg);
void new_subscribtion_request(LinphoneCore *lc, LinphoneFriend *lf, const char *url);
void auth_info_requested(LinphoneCore *lc, const char *realm, const char *username);
void linphone_subscription_state_change(LinphoneCore *lc, LinphoneEvent *ev, LinphoneSubscriptionState state);
void linphone_publish_state_changed(LinphoneCore *lc, LinphoneEvent *ev, LinphonePublishState state);
void linphone_notify_received(LinphoneCore *lc, LinphoneEvent *lev, const char *eventname, const LinphoneContent *content);

LinphoneAddress * create_linphone_address(const char * domain);
bool_t wait_for(LinphoneCore* lc_1, LinphoneCore* lc_2,int* counter,int value);
bool_t wait_for_list(MSList* lcs,int* counter,int value,int timeout_ms);
bool_t wait_for_until(LinphoneCore* lc_1, LinphoneCore* lc_2,int* counter,int value,int timout_ms);

bool_t call(LinphoneCoreManager* caller_mgr,LinphoneCoreManager* callee_mgr);
stats * get_stats(LinphoneCore *lc);
LinphoneCoreManager *get_manager(LinphoneCore *lc);

#endif /* LIBLINPHONE_TESTER_H_ */

