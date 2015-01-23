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

#include <stdio.h>
#include "CUnit/Basic.h"
#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"

static LinphoneCoreManager* presence_linphone_core_manager_new(char* username) {
	LinphoneCoreManager* mgr= linphone_core_manager_new2( "empty_rc", FALSE);
	char* identity_char;
	mgr->identity= linphone_core_get_primary_contact_parsed(mgr->lc);
	linphone_address_set_username(mgr->identity,username);
	identity_char=linphone_address_as_string(mgr->identity);
	linphone_core_set_primary_contact(mgr->lc,identity_char);
	return mgr;
}
void new_subscription_requested(LinphoneCore *lc, LinphoneFriend *lf, const char *url){
	char* from=linphone_address_as_string(linphone_friend_get_address(lf));
	stats* counters;
	ms_message("New subscription request  from [%s]  url [%s]",from,url);
	ms_free(from);
	counters = get_stats(lc);
	counters->number_of_NewSubscriptionRequest++;
	linphone_core_add_friend(lc,lf); /*accept subscription*/
}

void notify_presence_received(LinphoneCore *lc, LinphoneFriend * lf) {
	stats* counters;
	LinphonePresenceActivity *activity = NULL;
	char* from=linphone_address_as_string(linphone_friend_get_address(lf));
	ms_message("New Notify request  from [%s] ",from);
	ms_free(from);
	counters = get_stats(lc);
	counters->number_of_NotifyReceived++;

	counters->last_received_presence = linphone_friend_get_presence_model(lf);
	activity = linphone_presence_model_get_activity(counters->last_received_presence);
	switch (linphone_presence_activity_get_type(activity)) {
		case LinphonePresenceActivityOffline:
			counters->number_of_LinphonePresenceActivityOffline++; break;
		case LinphonePresenceActivityOnline:
			counters->number_of_LinphonePresenceActivityOnline++; break;
		case LinphonePresenceActivityAppointment:
			counters->number_of_LinphonePresenceActivityAppointment++; break;
		case LinphonePresenceActivityAway:
			counters->number_of_LinphonePresenceActivityAway++; break;
		case LinphonePresenceActivityBreakfast:
			counters->number_of_LinphonePresenceActivityBreakfast++; break;
		case LinphonePresenceActivityBusy:
			counters->number_of_LinphonePresenceActivityBusy++; break;
		case LinphonePresenceActivityDinner:
			counters->number_of_LinphonePresenceActivityDinner++; break;
		case LinphonePresenceActivityHoliday:
			counters->number_of_LinphonePresenceActivityHoliday++; break;
		case LinphonePresenceActivityInTransit:
			counters->number_of_LinphonePresenceActivityInTransit++; break;
		case LinphonePresenceActivityLookingForWork:
			counters->number_of_LinphonePresenceActivityLookingForWork++; break;
		case LinphonePresenceActivityLunch:
			counters->number_of_LinphonePresenceActivityLunch++; break;
		case LinphonePresenceActivityMeal:
			counters->number_of_LinphonePresenceActivityMeal++; break;
		case LinphonePresenceActivityMeeting:
			counters->number_of_LinphonePresenceActivityMeeting++; break;
		case LinphonePresenceActivityOnThePhone:
			counters->number_of_LinphonePresenceActivityOnThePhone++; break;
		case LinphonePresenceActivityOther:
			counters->number_of_LinphonePresenceActivityOther++; break;
		case LinphonePresenceActivityPerformance:
			counters->number_of_LinphonePresenceActivityPerformance++; break;
		case LinphonePresenceActivityPermanentAbsence:
			counters->number_of_LinphonePresenceActivityPermanentAbsence++; break;
		case LinphonePresenceActivityPlaying:
			counters->number_of_LinphonePresenceActivityPlaying++; break;
		case LinphonePresenceActivityPresentation:
			counters->number_of_LinphonePresenceActivityPresentation++; break;
		case LinphonePresenceActivityShopping:
			counters->number_of_LinphonePresenceActivityShopping++; break;
		case LinphonePresenceActivitySleeping:
			counters->number_of_LinphonePresenceActivitySleeping++; break;
		case LinphonePresenceActivitySpectator:
			counters->number_of_LinphonePresenceActivitySpectator++; break;
		case LinphonePresenceActivitySteering:
			counters->number_of_LinphonePresenceActivitySteering++; break;
		case LinphonePresenceActivityTravel:
			counters->number_of_LinphonePresenceActivityTravel++; break;
		case LinphonePresenceActivityTV:
			counters->number_of_LinphonePresenceActivityTV++; break;
		case LinphonePresenceActivityUnknown:
			counters->number_of_LinphonePresenceActivityUnknown++; break;
		case LinphonePresenceActivityVacation:
			counters->number_of_LinphonePresenceActivityVacation++; break;
		case LinphonePresenceActivityWorking:
			counters->number_of_LinphonePresenceActivityWorking++; break;
		case LinphonePresenceActivityWorship:
			counters->number_of_LinphonePresenceActivityWorship++; break;
	}
}

static void wait_core(LinphoneCore *core) {
	int i;

	for (i = 0; i < 10; i++) {
		linphone_core_iterate(core);
		ms_usleep(100000);
	}
}

static void simple_publish_with_expire(int expires) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneProxyConfig* proxy;
	LinphonePresenceModel* presence;

	linphone_core_get_default_proxy(marie->lc,&proxy);
	linphone_proxy_config_edit(proxy);
	if (expires >0) {
		linphone_proxy_config_set_publish_expires(proxy,expires);
	}
	linphone_proxy_config_enable_publish(proxy,TRUE);
	linphone_proxy_config_done(proxy);
	wait_core(marie->lc);
	presence =linphone_presence_model_new_with_activity(LinphonePresenceActivityOffline,NULL);
	linphone_core_set_presence_model(marie->lc,presence);
	wait_core(marie->lc);
	linphone_core_manager_destroy(marie);
}

static void simple_publish() {
	simple_publish_with_expire(-1);
}

static void publish_with_expires() {
	simple_publish_with_expire(1);
}

static bool_t subscribe_to_callee_presence(LinphoneCoreManager* caller_mgr,LinphoneCoreManager* callee_mgr) {
	stats initial_caller=caller_mgr->stat;
	stats initial_callee=callee_mgr->stat;
	bool_t result=FALSE;
	char* identity=linphone_address_as_string_uri_only(callee_mgr->identity);


	LinphoneFriend* friend=linphone_friend_new_with_address(identity);
	linphone_friend_edit(friend);
	linphone_friend_enable_subscribes(friend,TRUE);
	linphone_friend_done(friend);

	linphone_core_add_friend(caller_mgr->lc,friend);

	result=wait_for(caller_mgr->lc,callee_mgr->lc,&caller_mgr->stat.number_of_LinphonePresenceActivityOnline,initial_caller.number_of_LinphonePresenceActivityOnline+1);
	/*without proxy, callee cannot subscribe to caller
	result&=wait_for(caller_mgr->lc,callee_mgr->lc,&callee_mgr->stat.number_of_LinphonePresenceActivityOnline,initial_callee.number_of_LinphonePresenceActivityOnline+1);
	*/

	CU_ASSERT_EQUAL(callee_mgr->stat.number_of_NewSubscriptionRequest,initial_callee.number_of_NewSubscriptionRequest+1);
	/*without proxy, callee cannot subscribe to caller
	CU_ASSERT_EQUAL(callee_mgr->stat.number_of_NotifyReceived,initial_callee.number_of_NotifyReceived+1);
	*/
	CU_ASSERT_EQUAL(caller_mgr->stat.number_of_NotifyReceived,initial_caller.number_of_NotifyReceived+1);

	ms_free(identity);
	return result;

}
static void subscribe_failure_handle_by_app(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("pauline_rc");
	LinphoneProxyConfig* config;
	LinphoneFriend* lf;
	char* lf_identity=linphone_address_as_string_uri_only(pauline->identity);
	 linphone_core_get_default_proxy(marie->lc,&config);

	CU_ASSERT_TRUE(subscribe_to_callee_presence(marie,pauline));
	wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_NewSubscriptionRequest,1); /*just to wait for unsubscription even if not notified*/

	sal_set_recv_error(marie->lc->sal, 0); /*simulate an error*/

	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneRegistrationProgress,2));
	CU_ASSERT_EQUAL(linphone_proxy_config_get_error(config),LinphoneReasonIOError);
	sal_set_recv_error(marie->lc->sal, 1);

	lf = linphone_core_get_friend_by_address(marie->lc,lf_identity);
	linphone_friend_edit(lf);
	linphone_friend_enable_subscribes(lf,FALSE); /*disable subscription*/
	linphone_friend_done(lf);
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneRegistrationOk,2)); /*wait for register ok*/
	linphone_friend_edit(lf);
	linphone_friend_enable_subscribes(lf,TRUE);
	linphone_friend_done(lf);
	CU_ASSERT_FALSE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_NewSubscriptionRequest,2)); /*just to wait for unsubscription even if not notified*/

	linphone_core_manager_destroy(marie);
	CU_ASSERT_FALSE(wait_for(NULL,pauline->lc,&pauline->stat.number_of_NewSubscriptionRequest,3)); /*just to wait for unsubscription even if not notified*/

	linphone_core_manager_destroy(pauline);
}

static void simple_subscribe(void) {
	LinphoneCoreManager* marie = presence_linphone_core_manager_new("marie");
	LinphoneCoreManager* pauline = presence_linphone_core_manager_new("pauline");

	CU_ASSERT_TRUE(subscribe_to_callee_presence(marie,pauline));


	linphone_core_manager_destroy(marie);
	/*unsubscribe is not reported ?*/
	CU_ASSERT_FALSE(wait_for(NULL,pauline->lc,&pauline->stat.number_of_NewSubscriptionRequest,2)); /*just to wait for unsubscription even if not notified*/

	linphone_core_manager_destroy(pauline);
}

static void unsubscribe_while_subscribing(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneFriend* friend = linphone_friend_new_with_address("sip:toto@git.linphone.org"); /*any unexisting address*/
	linphone_friend_edit(friend);
	linphone_friend_enable_subscribes(friend,TRUE);
	linphone_friend_done(friend);
	linphone_core_add_friend(marie->lc,friend);
	linphone_core_iterate(marie->lc);
	linphone_core_manager_destroy(marie);
}

#if 0
/* the core no longer changes the presence status when a call is ongoing, this is left to the application*/
static void call_with_presence(void) {
	LinphoneCoreManager* marie = presence_linphone_core_manager_new("marie");
	LinphoneCoreManager* pauline = presence_linphone_core_manager_new("pauline");
	LinphoneVideoPolicy pol={0};
	linphone_core_set_video_policy(marie->lc,&pol);
	CU_ASSERT_TRUE(subscribe_to_callee_presence(marie,pauline));
	CU_ASSERT_TRUE(subscribe_to_callee_presence(pauline,marie));

	CU_ASSERT_TRUE(call(marie,pauline));
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityOnThePhone,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphonePresenceActivityOnThePhone,1));

	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);
	linphone_core_terminate_all_calls(marie->lc);
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphonePresenceActivityOnline,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityOnline,1));
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#endif

static void presence_information(void) {
	const char *bike_description = "Riding my bike";
	const char *vacation_note = "I'm on vacation until July 4th";
	const char *vacation_lang = "en";
	const char *contact = "sip:toto@example.com";
	LinphoneCoreManager *marie = presence_linphone_core_manager_new("marie");
	LinphoneCoreManager *pauline = presence_linphone_core_manager_new("pauline");
	LinphonePresenceModel *presence;
	LinphonePresenceActivity *activity = NULL;
	LinphonePresenceNote *note = NULL;
	const char *description = NULL;
	const char *note_content = NULL;
	char *contact2;
	time_t current_timestamp, presence_timestamp;

	CU_ASSERT_TRUE(subscribe_to_callee_presence(marie, pauline));

	/* Presence activity without description. */
	presence = linphone_presence_model_new_with_activity(LinphonePresenceActivityDinner, NULL);
	linphone_core_set_presence_model(pauline->lc, presence);
	wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityDinner,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphonePresenceActivityDinner, 1);
	activity = linphone_presence_model_get_activity(marie->stat.last_received_presence);
	CU_ASSERT_PTR_NOT_NULL(activity);
	CU_ASSERT_EQUAL(linphone_presence_activity_get_type(activity), LinphonePresenceActivityDinner);
	description = linphone_presence_activity_get_description(activity);
	CU_ASSERT_PTR_NULL(description);

	/* Presence activity with description. */
	presence = linphone_presence_model_new_with_activity(LinphonePresenceActivitySteering, bike_description);
	linphone_core_set_presence_model(pauline->lc, presence);
	wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivitySteering,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphonePresenceActivitySteering, 1);
	activity = linphone_presence_model_get_activity(marie->stat.last_received_presence);
	CU_ASSERT_PTR_NOT_NULL(activity);
	CU_ASSERT_EQUAL(linphone_presence_activity_get_type(activity), LinphonePresenceActivitySteering);
	description = linphone_presence_activity_get_description(activity);
	CU_ASSERT_PTR_NOT_NULL(description);
	if (description != NULL) CU_ASSERT_EQUAL(strcmp(description, bike_description), 0);

	/* Presence activity with description and note. */
	presence = linphone_presence_model_new_with_activity_and_note(LinphonePresenceActivityVacation, NULL, vacation_note, vacation_lang);
	linphone_core_set_presence_model(pauline->lc, presence);
	wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityVacation,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphonePresenceActivityVacation, 1);
	activity = linphone_presence_model_get_activity(marie->stat.last_received_presence);
	CU_ASSERT_PTR_NOT_NULL(activity);
	CU_ASSERT_EQUAL(linphone_presence_activity_get_type(activity), LinphonePresenceActivityVacation);
	description = linphone_presence_activity_get_description(activity);
	CU_ASSERT_PTR_NULL(description);
	note = linphone_presence_model_get_note(marie->stat.last_received_presence, NULL);
	CU_ASSERT_PTR_NOT_NULL(note);
	if (note != NULL) {
		note_content = linphone_presence_note_get_content(note);
		CU_ASSERT_PTR_NOT_NULL(note_content);
		if (note_content != NULL) {
			CU_ASSERT_EQUAL(strcmp(note_content, vacation_note), 0);
		}
	}

	/* Presence contact. */
	presence = linphone_presence_model_new_with_activity(LinphonePresenceActivityOnThePhone, NULL);
	linphone_presence_model_set_contact(presence, contact);
	linphone_core_set_presence_model(pauline->lc, presence);
	wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityOnThePhone,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphonePresenceActivityOnThePhone, 1);
	contact2 = linphone_presence_model_get_contact(presence);
	CU_ASSERT_PTR_NOT_NULL(contact2);
	if (contact2 != NULL) {
		CU_ASSERT_EQUAL(strcmp(contact, contact2), 0);
		ms_free(contact2);
	}

	/* Presence timestamp. */
	current_timestamp = ms_time(NULL);
	presence = linphone_presence_model_new_with_activity(LinphonePresenceActivityShopping, NULL);
	linphone_core_set_presence_model(pauline->lc, presence);
	wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityShopping,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphonePresenceActivityShopping, 1);
	presence_timestamp = linphone_presence_model_get_timestamp(presence);
	CU_ASSERT_TRUE(presence_timestamp >= current_timestamp);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
#define USE_PRESENCE_SERVER 0

#if USE_PRESENCE_SERVER
static void test_subscribe_notify_publish(void) {

	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneProxyConfig* proxy;
	LinphonePresenceModel* presence;

	LpConfig *pauline_lp = linphone_core_get_config(pauline->lc);
	char* lf_identity=linphone_address_as_string_uri_only(marie->identity);
	LinphoneFriend *lf = linphone_core_create_friend_with_address(pauline->lc,lf_identity);

	lp_config_set_int(pauline_lp,"sip","subscribe_expires",5);

	linphone_core_add_friend(pauline->lc,lf);

	/*wait for subscribe acknowledgment*/
	wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_NotifyReceived,1,2000);
	CU_ASSERT_EQUAL(LinphoneStatusOffline,linphone_friend_get_status(lf));

	/*enable publish*/

	linphone_core_get_default_proxy(marie->lc,&proxy);
	linphone_proxy_config_edit(proxy);

	linphone_proxy_config_enable_publish(proxy,TRUE);
	linphone_proxy_config_set_publish_expires(proxy,3);
	linphone_proxy_config_done(proxy);

	/*wait for marie status*/
	wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_NotifyReceived,2,2000);
	CU_ASSERT_EQUAL(LinphoneStatusOnline,linphone_friend_get_status(lf));

	presence =linphone_presence_model_new_with_activity(LinphonePresenceActivityOffline,NULL);
	linphone_core_set_presence_model(marie->lc,presence);

	/*wait for new status*/
	wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_NotifyReceived,3,2000);
	CU_ASSERT_EQUAL(LinphonePresenceActivityOffline,linphone_friend_get_status(lf));

	/*wait for refresh*/
	wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_NotifyReceived,4,5000);
	CU_ASSERT_EQUAL(LinphonePresenceActivityOffline,linphone_friend_get_status(lf));

	//linphone_core_remove_friend(pauline->lc,lf);
	/*wait for final notify*/
	//wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_NotifyReceived,4,5000);
	//CU_ASSERT_EQUAL(LinphonePresenceActivityOffline,linphone_friend_get_status(lf));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#endif

test_t presence_tests[] = {
	{ "Simple Subscribe", simple_subscribe },
	{ "Simple Publish", simple_publish },
	{ "Simple Publish with expires", publish_with_expires },
	/*{ "Call with presence", call_with_presence },*/
	{ "Unsubscribe while subscribing", unsubscribe_while_subscribing },
	{ "Presence information", presence_information },
	{ "App managed presence failure", subscribe_failure_handle_by_app },
#if USE_PRESENCE_SERVER
	{ "Subscribe with late publish", test_subscribe_notify_publish },
#endif
};

test_suite_t presence_test_suite = {
	"Presence",
	NULL,
	NULL,
	sizeof(presence_tests) / sizeof(presence_tests[0]),
	presence_tests
};

