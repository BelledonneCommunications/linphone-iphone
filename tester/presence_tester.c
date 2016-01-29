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


#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"

static void enable_publish(LinphoneCoreManager *mgr, bool_t enable);

static LinphoneCoreManager* presence_linphone_core_manager_new(char* username) {
	LinphoneCoreManager* mgr= linphone_core_manager_new2( "empty_rc", FALSE);
	char* identity_char;
	mgr->identity= linphone_core_get_primary_contact_parsed(mgr->lc);
	linphone_address_set_username(mgr->identity,username);
	identity_char=linphone_address_as_string(mgr->identity);
	linphone_core_set_primary_contact(mgr->lc,identity_char);
	ms_free(identity_char);
	return mgr;
}

void new_subscription_requested(LinphoneCore *lc, LinphoneFriend *lf, const char *url){
	char* from=linphone_address_as_string(linphone_friend_get_address(lf));
	stats* counters;
	ms_message("New subscription request from [%s] url [%s]",from,url);
	ms_free(from);
	counters = get_stats(lc);
	counters->number_of_NewSubscriptionRequest++;
	linphone_core_add_friend(lc,lf); /*accept subscription*/
}

void notify_presence_received(LinphoneCore *lc, LinphoneFriend * lf) {
	stats* counters;
	LinphonePresenceActivity *activity = NULL;
	char* from=linphone_address_as_string(linphone_friend_get_address(lf));
	ms_message("New Notify request from [%s] ",from);
	ms_free(from);
	counters = get_stats(lc);
	counters->number_of_NotifyPresenceReceived++;

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

void wait_core(LinphoneCore *core) {
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

	proxy = linphone_core_get_default_proxy_config(marie->lc);
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

static void simple_publish(void) {
	simple_publish_with_expire(-1);
}

static void publish_with_expires(void) {
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
	linphone_friend_unref(friend);

	result=wait_for(caller_mgr->lc,callee_mgr->lc,&caller_mgr->stat.number_of_LinphonePresenceActivityOnline,initial_caller.number_of_LinphonePresenceActivityOnline+1);
	/*without proxy, callee cannot subscribe to caller
	result&=wait_for(caller_mgr->lc,callee_mgr->lc,&callee_mgr->stat.number_of_LinphonePresenceActivityOnline,initial_callee.number_of_LinphonePresenceActivityOnline+1);
	*/

	BC_ASSERT_EQUAL(callee_mgr->stat.number_of_NewSubscriptionRequest,initial_callee.number_of_NewSubscriptionRequest+1, int, "%d");
	/*without proxy, callee cannot subscribe to caller
	BC_ASSERT_EQUAL(callee_mgr->stat.number_of_NotifyPresenceReceived,initial_callee.number_of_NotifyPresenceReceived+1, int, "%d");
	*/
	BC_ASSERT_EQUAL(caller_mgr->stat.number_of_NotifyPresenceReceived,initial_caller.number_of_NotifyPresenceReceived+1, int, "%d");

	ms_free(identity);
	return result;

}
static void subscribe_failure_handle_by_app(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("pauline_tcp_rc");
	LinphoneProxyConfig* config;
	LinphoneFriend* lf;
	char* lf_identity=linphone_address_as_string_uri_only(pauline->identity);
	
	config = linphone_core_get_default_proxy_config(marie->lc);

	BC_ASSERT_TRUE(subscribe_to_callee_presence(marie,pauline));
	wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_NewSubscriptionRequest,1); /*just to wait for unsubscription even if not notified*/

	sal_set_recv_error(marie->lc->sal, 0); /*simulate an error*/

	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneRegistrationProgress,2));
	BC_ASSERT_EQUAL(linphone_proxy_config_get_error(config),LinphoneReasonIOError, int, "%d");
	sal_set_recv_error(marie->lc->sal, 1);

	lf = linphone_core_get_friend_by_address(marie->lc,lf_identity);
	linphone_friend_edit(lf);
	linphone_friend_enable_subscribes(lf,FALSE); /*disable subscription*/
	linphone_friend_done(lf);
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneRegistrationOk,2)); /*wait for register ok*/
	linphone_friend_edit(lf);
	linphone_friend_enable_subscribes(lf,TRUE);
	linphone_friend_done(lf);
	BC_ASSERT_FALSE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_NewSubscriptionRequest,2)); /*just to wait for unsubscription even if not notified*/

	linphone_core_manager_destroy(marie);
	BC_ASSERT_FALSE(wait_for(NULL,pauline->lc,&pauline->stat.number_of_NewSubscriptionRequest,3)); /*just to wait for unsubscription even if not notified*/

	linphone_core_manager_destroy(pauline);
}

static void simple_subscribe(void) {
	LinphoneCoreManager* marie = presence_linphone_core_manager_new("marie");
	LinphoneCoreManager* pauline = presence_linphone_core_manager_new("pauline");

	BC_ASSERT_TRUE(subscribe_to_callee_presence(marie,pauline));


	linphone_core_manager_destroy(marie);
	/*unsubscribe is not reported ?*/
	BC_ASSERT_FALSE(wait_for(NULL,pauline->lc,&pauline->stat.number_of_NewSubscriptionRequest,2)); /*just to wait for unsubscription even if not notified*/

	linphone_core_manager_destroy(pauline);
}

static void unsubscribe_while_subscribing(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneFriend* friend = linphone_friend_new_with_address("sip:toto@git.linphone.org"); /*any unexisting address*/
	linphone_friend_edit(friend);
	linphone_friend_enable_subscribes(friend,TRUE);
	linphone_friend_done(friend);
	linphone_core_add_friend(marie->lc,friend);
	linphone_friend_unref(friend);
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
	BC_ASSERT_TRUE(subscribe_to_callee_presence(marie,pauline));
	BC_ASSERT_TRUE(subscribe_to_callee_presence(pauline,marie));

	BC_ASSERT_TRUE(call(marie,pauline));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityOnThePhone,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphonePresenceActivityOnThePhone,1));

	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);
	linphone_core_terminate_all_calls(marie->lc);
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphonePresenceActivityOnline,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityOnline,1));
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

	BC_ASSERT_TRUE(subscribe_to_callee_presence(marie, pauline));

	/* Presence activity without description. */
	presence = linphone_presence_model_new_with_activity(LinphonePresenceActivityDinner, NULL);
	linphone_core_set_presence_model(pauline->lc, presence);
	wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityDinner,1);
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePresenceActivityDinner, 1, int, "%d");
	activity = linphone_presence_model_get_activity(marie->stat.last_received_presence);
	BC_ASSERT_PTR_NOT_NULL(activity);
	BC_ASSERT_EQUAL(linphone_presence_activity_get_type(activity), LinphonePresenceActivityDinner, int, "%d");
	description = linphone_presence_activity_get_description(activity);
	BC_ASSERT_PTR_NULL(description);

	/* Presence activity with description. */
	presence = linphone_presence_model_new_with_activity(LinphonePresenceActivitySteering, bike_description);
	linphone_core_set_presence_model(pauline->lc, presence);
	wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivitySteering,1);
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePresenceActivitySteering, 1, int, "%d");
	activity = linphone_presence_model_get_activity(marie->stat.last_received_presence);
	BC_ASSERT_PTR_NOT_NULL(activity);
	BC_ASSERT_EQUAL(linphone_presence_activity_get_type(activity), LinphonePresenceActivitySteering, int, "%d");
	description = linphone_presence_activity_get_description(activity);
	BC_ASSERT_PTR_NOT_NULL(description);
	if (description != NULL) BC_ASSERT_STRING_EQUAL(description, bike_description);

	/* Presence activity with description and note. */
	presence = linphone_presence_model_new_with_activity_and_note(LinphonePresenceActivityVacation, NULL, vacation_note, vacation_lang);
	linphone_core_set_presence_model(pauline->lc, presence);
	wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityVacation,1);
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePresenceActivityVacation, 1, int, "%d");
	activity = linphone_presence_model_get_activity(marie->stat.last_received_presence);
	BC_ASSERT_PTR_NOT_NULL(activity);
	BC_ASSERT_EQUAL(linphone_presence_activity_get_type(activity), LinphonePresenceActivityVacation, int, "%d");
	description = linphone_presence_activity_get_description(activity);
	BC_ASSERT_PTR_NULL(description);
	note = linphone_presence_model_get_note(marie->stat.last_received_presence, NULL);
	BC_ASSERT_PTR_NOT_NULL(note);
	if (note != NULL) {
		note_content = linphone_presence_note_get_content(note);
		BC_ASSERT_PTR_NOT_NULL(note_content);
		if (note_content != NULL) {
			BC_ASSERT_STRING_EQUAL(note_content, vacation_note);
		}
	}

	/* Presence contact. */
	presence = linphone_presence_model_new_with_activity(LinphonePresenceActivityOnThePhone, NULL);
	linphone_presence_model_set_contact(presence, contact);
	linphone_core_set_presence_model(pauline->lc, presence);
	wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityOnThePhone,1);
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePresenceActivityOnThePhone, 1, int, "%d");
	contact2 = linphone_presence_model_get_contact(presence);
	BC_ASSERT_PTR_NOT_NULL(contact2);
	if (contact2 != NULL) {
		BC_ASSERT_STRING_EQUAL(contact, contact2);
		ms_free(contact2);
	}

	/* Presence timestamp. */
	current_timestamp = ms_time(NULL);
	presence = linphone_presence_model_new_with_activity(LinphonePresenceActivityShopping, NULL);
	linphone_core_set_presence_model(pauline->lc, presence);
	wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityShopping,1);
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePresenceActivityShopping, 1, int, "%d");
	presence_timestamp = linphone_presence_model_get_timestamp(presence);
	BC_ASSERT_GREATER((unsigned)presence_timestamp , (unsigned)current_timestamp, unsigned, "%u");

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


static void subscribe_presence_forked(void){
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline1 = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCoreManager* pauline2 = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneFriend *lf;
	MSList *lcs = NULL;
	
	lcs = ms_list_append(lcs, marie->lc);
	lcs = ms_list_append(lcs, pauline1->lc);
	lcs = ms_list_append(lcs, pauline2->lc);
	
	lf = linphone_core_create_friend(marie->lc);
	linphone_friend_set_address(lf, pauline1->identity);
	linphone_friend_enable_subscribes(lf, TRUE);
	
	linphone_core_add_friend(marie->lc, lf);
	linphone_friend_unref(lf);
	
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline1->stat.number_of_NewSubscriptionRequest,1, 10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline2->stat.number_of_NewSubscriptionRequest,1, 2000));
	/*we should get two notifies*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePresenceActivityOnline,2, 10000));
	
	/*marie also shall receive two SUBSCRIBEs from the two paulines, but won't be notified to the app since 
	 Marie set Pauline as a friend.*/
	BC_ASSERT_EQUAL(marie->stat.number_of_NewSubscriptionRequest, 0, int, "%d");
	/*and the two paulines shall be notified of marie's presence*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline1->stat.number_of_LinphonePresenceActivityOnline,1, 3000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline2->stat.number_of_LinphonePresenceActivityOnline,1, 2000));
	
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline1);
	linphone_core_manager_destroy(pauline2);
	
	ms_list_free(lcs);
}

static void subscribe_presence_expired(void){
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline1 = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneFriend *lf;
	MSList *lcs = NULL;
	
	lcs = ms_list_append(lcs, marie->lc);
	lcs = ms_list_append(lcs, pauline1->lc);
	
	lp_config_set_int(marie->lc->config, "sip", "subscribe_expires", 10);

	lf = linphone_core_create_friend(marie->lc);
	linphone_friend_set_address(lf, pauline1->identity);
	linphone_friend_enable_subscribes(lf, TRUE);
	
	linphone_core_add_friend(marie->lc, lf);
	linphone_friend_unref(lf);
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline1->stat.number_of_NewSubscriptionRequest,1, 5000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePresenceActivityOnline,1, 2000));
	
	lf = linphone_core_find_friend(pauline1->lc, marie->identity);
	BC_ASSERT_PTR_NOT_NULL(lf->insubs);
	
	/*marie comes offline suddenly*/
	linphone_core_set_network_reachable(marie->lc, FALSE);
	/*after a certain time, pauline shall see the incoming SUBSCRIBE expired*/
	wait_for_list(lcs,NULL, 0, 11000);
	BC_ASSERT_PTR_NULL(lf->insubs);

	/*just make network reachable so that marie can unregister properly*/
	linphone_core_set_network_reachable(marie->lc, TRUE);
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneRegistrationOk,2, 10000));
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline1);
	
	ms_list_free(lcs);
}

static void subscriber_no_longuer_reachable(void){
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline1 = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneFriend *lf;
	MSList *lcs = NULL;
	LinphonePresenceModel * presence;
	
	lcs = ms_list_append(lcs, marie->lc);
	lcs = ms_list_append(lcs, pauline1->lc);
	
	lp_config_set_int(marie->lc->config, "sip", "subscribe_expires", 40);
	linphone_core_set_user_agent(marie->lc, "full-presence-support", NULL);
	linphone_core_set_user_agent(pauline1->lc, "full-presence-support", NULL);
	
	enable_publish(pauline1, TRUE);
	
	lf = linphone_core_create_friend(marie->lc);
	linphone_friend_set_address(lf, pauline1->identity);
	linphone_friend_enable_subscribes(lf, TRUE);
	
	linphone_core_add_friend(marie->lc, lf);
	linphone_friend_unref(lf);
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePresenceActivityOnline,1, 2000));
	
	
	presence =linphone_presence_model_new_with_activity(LinphonePresenceActivityBusy,NULL);
	linphone_core_set_presence_model(pauline1->lc,presence);

	/*don't schedule marie to simulate Notify timeout server side*/
	wait_for_until(pauline1->lc, NULL, 0, 0, 35000);
	
	//sal_set_send_error(marie->lc->sal,0);
	
	/*because of notify timeout detected by server, so subscription is reset*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePresenceActivityOffline,2, 4000));
	
	// now subscribetion is supposed to be dead because notify was not answered in time.
	presence =linphone_presence_model_new_with_activity(LinphonePresenceActivityOnline,NULL);
	linphone_core_set_presence_model(pauline1->lc,presence);
	
	/*becasue subscription is automatically restarted*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePresenceActivityOnline,2, 4000));
	
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline1);
	
	ms_list_free(lcs);

}

static void test_subscribe_notify_publish(void) {

	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneProxyConfig* proxy;
	LinphonePresenceModel* presence;
	LpConfig *pauline_lp;
	char* lf_identity;
 	LinphoneFriend *lf;

	linphone_core_set_user_agent(marie->lc, "full-presence-support", NULL);
	linphone_core_set_user_agent(pauline->lc, "full-presence-support", NULL);
	pauline_lp = linphone_core_get_config(pauline->lc);
	lf_identity=linphone_address_as_string_uri_only(marie->identity);
	lf = linphone_core_create_friend_with_address(pauline->lc,lf_identity);

	lp_config_set_int(pauline_lp,"sip","subscribe_expires",5);

	linphone_core_add_friend(pauline->lc,lf);

	/*wait for subscribe acknowledgment*/
	wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_NotifyPresenceReceived,1,2000);
	BC_ASSERT_EQUAL(LinphoneStatusOffline,linphone_friend_get_status(lf), int, "%d");

	/*enable publish*/

	proxy = linphone_core_get_default_proxy_config(marie->lc);
	linphone_proxy_config_edit(proxy);

	linphone_proxy_config_enable_publish(proxy,TRUE);
	linphone_proxy_config_set_publish_expires(proxy,3);
	linphone_proxy_config_done(proxy);

	/*wait for marie status*/
	wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_NotifyPresenceReceived,2,2000);
	BC_ASSERT_EQUAL(LinphoneStatusOnline,linphone_friend_get_status(lf), int, "%d");

	presence =linphone_presence_model_new_with_activity(LinphonePresenceActivityBusy,NULL);
	linphone_core_set_presence_model(marie->lc,presence);

	/*wait for new status*/
	wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_NotifyPresenceReceived,3,2000);
	BC_ASSERT_EQUAL(LinphoneStatusBusy,linphone_friend_get_status(lf), int, "%d");

	/*wait for refresh*/
	wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_NotifyPresenceReceived,4,5000);
	BC_ASSERT_EQUAL(LinphoneStatusBusy,linphone_friend_get_status(lf), int, "%d");

	/*linphone_core_remove_friend(pauline->lc,lf);*/
	/*wait for final notify*/
	/*wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_NotifyPresenceReceived,4,5000);
	BC_ASSERT_EQUAL(LinphonePresenceActivityOffline,linphone_friend_get_status(lf), int, "%d");
	 */
	
	/*Expect a notify at publication expiration*/
	wait_for_until(pauline->lc,pauline->lc,&pauline->stat.number_of_NotifyPresenceReceived,6,5000);
	BC_ASSERT_EQUAL(LinphoneStatusOffline,linphone_friend_get_status(lf), int, "%d");
	
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void test_forked_subscribe_notify_publish(void) {

	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneProxyConfig* proxy;
	LinphonePresenceModel* presence;
	LpConfig *pauline_lp;
	char* lf_identity;
	LinphoneFriend *lf;
	MSList* lcs=ms_list_append(NULL,pauline->lc);
	lcs=ms_list_append(lcs,marie->lc);
	lcs=ms_list_append(lcs,marie->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	linphone_core_set_user_agent(marie->lc, "full-presence-support", NULL);
	linphone_core_set_user_agent(marie2->lc, "full-presence-support", NULL);
	linphone_core_set_user_agent(pauline->lc, "full-presence-support", NULL);

	
	pauline_lp = linphone_core_get_config(pauline->lc);
	lf_identity=linphone_address_as_string_uri_only(marie->identity);
	lf = linphone_core_create_friend_with_address(pauline->lc,lf_identity);

	lp_config_set_int(pauline_lp,"sip","subscribe_expires",5);

	linphone_core_add_friend(pauline->lc,lf);

	/*wait for subscribe acknowledgment*/
	wait_for_list(lcs,&pauline->stat.number_of_NotifyPresenceReceived,1,2000);
	BC_ASSERT_EQUAL(LinphoneStatusOffline,linphone_friend_get_status(lf), int, "%d");

	/*enable publish*/

	proxy = linphone_core_get_default_proxy_config(marie->lc);
	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_enable_publish(proxy,TRUE);
	linphone_proxy_config_set_publish_expires(proxy,3);
	linphone_proxy_config_done(proxy);

	proxy = linphone_core_get_default_proxy_config(marie2->lc);
	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_enable_publish(proxy,TRUE);
	linphone_proxy_config_set_publish_expires(proxy,3);
	linphone_proxy_config_done(proxy);


	/*wait for marie status*/
	wait_for_list(lcs,&pauline->stat.number_of_NotifyPresenceReceived,3,2000);
	BC_ASSERT_EQUAL(LinphoneStatusOnline,linphone_friend_get_status(lf), int, "%d");

	presence =linphone_presence_model_new_with_activity(LinphonePresenceActivityBusy,NULL);
	linphone_core_set_presence_model(marie->lc,presence);

	/*wait for new status*/
	wait_for_list(lcs,&pauline->stat.number_of_NotifyPresenceReceived,4,2000);
	BC_ASSERT_EQUAL(LinphoneStatusBusy,linphone_friend_get_status(lf), int, "%d");


	presence =linphone_presence_model_new_with_activity(  LinphonePresenceActivityMeeting,NULL);
	linphone_core_set_presence_model(marie2->lc,presence);
	/*wait for new status*/
	wait_for_list(lcs,&pauline->stat.number_of_NotifyPresenceReceived,5,2000);
	BC_ASSERT_TRUE(LinphoneStatusBusy == linphone_friend_get_status(lf)
				   || LinphoneStatusDoNotDisturb == linphone_friend_get_status(lf)); /*because liblinphone compositor is very simple for now */

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(pauline);
}

const char * get_identity(LinphoneCoreManager *mgr) {
	LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(mgr->lc);
	return linphone_proxy_config_get_identity(cfg);
}

static void enable_publish(LinphoneCoreManager *mgr, bool_t enable) {
	LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(mgr->lc);
	linphone_proxy_config_edit(cfg);
	linphone_proxy_config_enable_publish(cfg, enable);
	linphone_proxy_config_set_publish_expires(cfg, 60);
	linphone_proxy_config_done(cfg);
}

static void test_presence_list(void) {
	LinphoneCoreManager *laure = linphone_core_manager_new("laure_tcp_rc");
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_rc");
	const char *rls_uri = "sip:rls@sip.example.org";
	LinphoneFriendList *lfl;
	LinphoneFriend *lf;
	const char *laure_identity;
	const char *marie_identity;
	const char *pauline_identity;
	MSList* lcs = NULL;
	int dummy = 0;

	laure_identity = get_identity(laure);
	marie_identity = get_identity(marie);
	pauline_identity = get_identity(pauline);
	enable_publish(marie, TRUE);
	enable_publish(pauline, TRUE);
	enable_publish(laure, TRUE);

	linphone_core_set_presence_model(marie->lc, linphone_core_create_presence_model_with_activity(marie->lc, LinphonePresenceActivityBusy, NULL));
	linphone_core_set_presence_model(pauline->lc, linphone_core_create_presence_model_with_activity(pauline->lc, LinphonePresenceActivityVacation, NULL));

	lfl = linphone_core_create_friend_list(laure->lc);
	linphone_friend_list_set_rls_uri(lfl, rls_uri);
	lf = linphone_core_create_friend_with_address(laure->lc, marie_identity);
	linphone_friend_list_add_friend(lfl, lf);
	lf = linphone_core_create_friend_with_address(laure->lc, pauline_identity);
	linphone_friend_list_add_friend(lfl, lf);
	lf = linphone_core_create_friend_with_address(laure->lc, "sip:michelle@sip.inexistentdomain.com");
	linphone_friend_list_add_friend(lfl, lf);
	linphone_core_set_friend_list(laure->lc, lfl);
	linphone_friend_list_unref(lfl);
	linphone_core_set_presence_model(laure->lc, linphone_core_create_presence_model_with_activity(laure->lc, LinphonePresenceActivityOnline, NULL));

	lcs = ms_list_append(lcs, laure->lc);
	lcs = ms_list_append(lcs, marie->lc);
	lcs = ms_list_append(lcs, pauline->lc);

	wait_for_list(lcs, &laure->stat.number_of_NotifyPresenceReceived, 2, 2000);
	BC_ASSERT_EQUAL(laure->stat.number_of_NotifyPresenceReceived, 2, int, "%d");
	BC_ASSERT_EQUAL(laure->lc->friendlist->expected_notification_version, 1, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(laure->lc->friendlist, marie_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusBusy, int, "%d");
	BC_ASSERT_EQUAL(lf->presence_received, TRUE, int, "%d");
	BC_ASSERT_EQUAL(lf->subscribe_active, TRUE, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(laure->lc->friendlist, pauline_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusVacation, int, "%d");
	BC_ASSERT_EQUAL(lf->presence_received, TRUE, int, "%d");
	BC_ASSERT_EQUAL(lf->subscribe_active, TRUE, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(laure->lc->friendlist, "sip:michelle@sip.inexistentdomain.com");
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOffline, int, "%d");
	BC_ASSERT_EQUAL(lf->presence_received, FALSE, int, "%d");
	BC_ASSERT_EQUAL(lf->subscribe_active, TRUE, int, "%d");

	lfl = linphone_core_create_friend_list(marie->lc);
	linphone_friend_list_set_rls_uri(lfl, rls_uri);
	lf = linphone_core_create_friend_with_address(marie->lc, laure_identity);
	linphone_friend_list_add_friend(lfl, lf);
	linphone_core_set_friend_list(marie->lc, lfl);
	linphone_friend_list_unref(lfl);
	linphone_friend_list_update_subscriptions(marie->lc->friendlist, NULL, FALSE);

	wait_for_list(lcs, &marie->stat.number_of_NotifyPresenceReceived, 1, 2000);
	BC_ASSERT_EQUAL(marie->stat.number_of_NotifyPresenceReceived, 1, int, "%d");
	BC_ASSERT_EQUAL(marie->lc->friendlist->expected_notification_version, 1, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(marie->lc->friendlist, laure_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOnline, int, "%d");
	BC_ASSERT_EQUAL(lf->presence_received, TRUE, int, "%d");
	BC_ASSERT_EQUAL(lf->subscribe_active, TRUE, int, "%d");

	lfl = linphone_core_create_friend_list(pauline->lc);
	linphone_friend_list_set_rls_uri(lfl, rls_uri);
	lf = linphone_core_create_friend_with_address(pauline->lc, marie_identity);
	linphone_friend_list_add_friend(lfl, lf);
	linphone_core_set_friend_list(pauline->lc, lfl);
	linphone_friend_list_unref(lfl);
	linphone_friend_list_update_subscriptions(pauline->lc->friendlist, NULL, FALSE);

	wait_for_list(lcs, &pauline->stat.number_of_NotifyPresenceReceived, 1, 2000);
	BC_ASSERT_EQUAL(pauline->stat.number_of_NotifyPresenceReceived, 1, int, "%d");
	BC_ASSERT_EQUAL(pauline->lc->friendlist->expected_notification_version, 1, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(pauline->lc->friendlist, marie_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusBusy, int, "%d");
	BC_ASSERT_EQUAL(lf->presence_received, TRUE, int, "%d");
	BC_ASSERT_EQUAL(lf->subscribe_active, TRUE, int, "%d");

	linphone_core_set_presence_model(marie->lc, linphone_core_create_presence_model_with_activity(marie->lc, LinphonePresenceActivityOnThePhone, NULL));

	wait_for_list(lcs, &laure->stat.number_of_NotifyPresenceReceived, 4, 2000);
	/* The number of PresenceReceived events can be 3 or 4 here. TODO: ideally it should always be 3. */
	BC_ASSERT_GREATER(laure->stat.number_of_NotifyPresenceReceived, 3, int, "%d");
	BC_ASSERT_LOWER(laure->stat.number_of_NotifyPresenceReceived, 4, int, "%d");
	BC_ASSERT_EQUAL(laure->lc->friendlist->expected_notification_version, 2, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(laure->lc->friendlist, marie_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOnThePhone, int, "%d");

	wait_for_list(lcs, &pauline->stat.number_of_NotifyPresenceReceived, 2, 2000);
	BC_ASSERT_EQUAL(pauline->stat.number_of_NotifyPresenceReceived, 2, int, "%d");
	BC_ASSERT_EQUAL(pauline->lc->friendlist->expected_notification_version, 2, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(pauline->lc->friendlist, marie_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOnThePhone, int, "%d");

	enable_publish(laure, FALSE);
	enable_publish(marie, FALSE);
	enable_publish(pauline, FALSE);

	wait_for_list(lcs, &dummy, 1, 2000); /* Wait a little bit for the presence notifications. TODO: Wait for the correct number of PresenceReceived events. */
	
	lf = linphone_friend_list_find_friend_by_uri(pauline->lc->friendlist, marie_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOffline, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(laure->lc->friendlist, pauline_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOffline, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(laure->lc->friendlist, marie_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOffline, int, "%d");
	
	lf = linphone_friend_list_find_friend_by_uri(marie->lc->friendlist, laure_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOffline, int, "%d");
	
	linphone_core_manager_destroy(laure);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
#if 0
static void test_presence_list_subscribe_before_publish(void) {
	LinphoneCoreManager *laure = linphone_core_manager_new("laure_tcp_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_rc");
	const char *rls_uri = "sip:rls@sip.example.org";
	LinphoneFriendList *lfl;
	LinphoneFriend *lf;
	const char *pauline_identity;
	MSList* lcs = NULL;
	int dummy = 0;

	pauline_identity = get_identity(pauline);

	linphone_core_set_presence_model(pauline->lc, linphone_core_create_presence_model_with_activity(pauline->lc, LinphonePresenceActivityVacation, NULL));

	lfl = linphone_core_create_friend_list(laure->lc);
	linphone_friend_list_set_rls_uri(lfl, rls_uri);
	lf = linphone_core_create_friend_with_address(laure->lc, pauline_identity);
	linphone_friend_list_add_friend(lfl, lf);
	lf = linphone_core_create_friend_with_address(laure->lc, "sip:michelle@sip.inexistentdomain.com");
	linphone_friend_list_add_friend(lfl, lf);
	linphone_core_set_friend_list(laure->lc, lfl);
	linphone_friend_list_unref(lfl);
	linphone_core_set_presence_model(laure->lc, linphone_core_create_presence_model_with_activity(laure->lc, LinphonePresenceActivityOnline, NULL));
	linphone_friend_list_update_subscriptions(laure->lc->friendlist, NULL, FALSE);

	lcs = ms_list_append(lcs, laure->lc);
	lcs = ms_list_append(lcs, pauline->lc);

	wait_for_list(lcs, &dummy, 1, 2000); /* Wait a little bit for the subscribe to happen */

	enable_publish(pauline, TRUE);
	wait_for_list(lcs, &pauline->stat.number_of_NotifyPresenceReceived, 1, 2000);
	BC_ASSERT_GREATER(laure->stat.number_of_NotifyPresenceReceived, 1, int, "%d");
	BC_ASSERT_GREATER(laure->lc->friendlist->expected_notification_version, 1, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(laure->lc->friendlist, pauline_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusVacation, int, "%d");
	BC_ASSERT_EQUAL(lf->presence_received, TRUE, int, "%d");
	BC_ASSERT_EQUAL(lf->subscribe_active, TRUE, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(laure->lc->friendlist, "sip:michelle@sip.inexistentdomain.com");
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOffline, int, "%d");
	BC_ASSERT_EQUAL(lf->presence_received, FALSE, int, "%d");
	BC_ASSERT_EQUAL(lf->subscribe_active, TRUE, int, "%d");

	enable_publish(laure, FALSE);
	enable_publish(pauline, FALSE);

	linphone_core_manager_destroy(laure);
	linphone_core_manager_destroy(pauline);
}
#endif
static void test_presence_list_subscription_expire_for_unknown(void) {
	LinphoneCoreManager *laure = linphone_core_manager_new("laure_tcp_rc");
	const char *rls_uri = "sip:rls@sip.example.org";
	LinphoneFriendList *lfl;
	LinphoneFriend *lf;
	lp_config_set_int(laure->lc->config, "sip", "rls_presence_expires", 3);
	
	lfl = linphone_core_create_friend_list(laure->lc);
	linphone_friend_list_set_rls_uri(lfl, rls_uri);
	lf = linphone_core_create_friend_with_address(laure->lc, "sip:michelle@sip.inexistentdomain.com");
	linphone_friend_list_add_friend(lfl, lf);
	linphone_core_set_friend_list(laure->lc, lfl);
	linphone_friend_list_update_subscriptions(lfl,NULL,FALSE);
	
	linphone_friend_list_unref(lfl);
	
	/* wait for refresh*/
	BC_ASSERT_FALSE(wait_for_until(laure->lc, NULL, &laure->stat.number_of_NotifyPresenceReceived, 1, 4000));
	
	linphone_core_manager_destroy(laure);
}

static void test_presence_list_subscribe_with_error(bool_t io_error) {
	LinphoneCoreManager *laure = linphone_core_manager_new("laure_tcp_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_rc");
	const char *rls_uri = "sip:rls@sip.example.org";
	LinphoneFriendList *lfl;
	LinphoneFriend *lf;
	const char *pauline_identity;
	MSList* lcs = NULL;
	int dummy = 0;
	lp_config_set_int(laure->lc->config, "sip", "rls_presence_expires", 3);
	

	pauline_identity = get_identity(pauline);
	
	linphone_core_set_presence_model(pauline->lc, linphone_core_create_presence_model_with_activity(pauline->lc, LinphonePresenceActivityVacation, NULL));
	
	lfl = linphone_core_create_friend_list(laure->lc);
	linphone_friend_list_set_rls_uri(lfl, rls_uri);
	lf = linphone_core_create_friend_with_address(laure->lc, pauline_identity);
	linphone_friend_list_add_friend(lfl, lf);
	lf = linphone_core_create_friend_with_address(laure->lc, "sip:michelle@sip.inexistentdomain.com");
	linphone_friend_list_add_friend(lfl, lf);
	linphone_core_set_friend_list(laure->lc, lfl);
	linphone_friend_list_unref(lfl);
	linphone_core_set_presence_model(laure->lc, linphone_core_create_presence_model_with_activity(laure->lc, LinphonePresenceActivityOnline, NULL));
	linphone_friend_list_update_subscriptions(laure->lc->friendlist, NULL, FALSE);
	
	lcs = ms_list_append(lcs, laure->lc);
	lcs = ms_list_append(lcs, pauline->lc);
	
	wait_for_list(lcs, &dummy, 1, 2000); /* Wait a little bit for the subscribe to happen */
	
	enable_publish(pauline, TRUE);
	wait_for_list(lcs, &pauline->stat.number_of_NotifyPresenceReceived, 1, 2000);
	BC_ASSERT_GREATER(laure->stat.number_of_NotifyPresenceReceived, 1, int, "%d");
	BC_ASSERT_GREATER(laure->lc->friendlist->expected_notification_version, 1, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(laure->lc->friendlist, pauline_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusVacation, int, "%d");
	BC_ASSERT_EQUAL(lf->presence_received, TRUE, int, "%d");
	BC_ASSERT_EQUAL(lf->subscribe_active, TRUE, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(laure->lc->friendlist, "sip:michelle@sip.inexistentdomain.com");
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOffline, int, "%d");
	BC_ASSERT_EQUAL(lf->presence_received, FALSE, int, "%d");
	BC_ASSERT_EQUAL(lf->subscribe_active, TRUE, int, "%d");
	
	BC_ASSERT_TRUE(wait_for_until(laure->lc, pauline->lc, &laure->stat.number_of_NotifyPresenceReceived, 2, 5000));
	if (io_error) {
		ms_message("Simulating socket error");
		sal_set_recv_error(laure->lc->sal, -1);
		wait_for_list(lcs, &dummy, 1, 1000); /* just time for socket to be closed */
		
	} else {
		ms_message("Simulating in/out packets losses");
		sal_set_send_error(laure->lc->sal,1500); /*make sure no refresh is sent, trash the message without generating error*/
		sal_set_recv_error(laure->lc->sal, 1500); /*make sure server notify to close the dialog is also ignored*/
		wait_for_list(lcs, &dummy, 1, 3000); /* Wait a little bit for the subscribe to happen */
	}
	/*restart normal behavior*/
	sal_set_send_error(laure->lc->sal,0);
	sal_set_recv_error(laure->lc->sal, 1);
	
	linphone_core_set_presence_model(pauline->lc, linphone_core_create_presence_model_with_activity(pauline->lc, LinphonePresenceActivityAway, NULL));

	BC_ASSERT_TRUE(wait_for_until(laure->lc, pauline->lc, &laure->stat.number_of_NotifyPresenceReceived, 3, 5000));
	lf = linphone_friend_list_find_friend_by_uri(laure->lc->friendlist, pauline_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusAway, int, "%d");
	
	linphone_core_manager_destroy(laure);
	linphone_core_manager_destroy(pauline);
}

static void test_presence_list_subscribe_dialog_expire(void) {
	test_presence_list_subscribe_with_error(FALSE);
}

static void test_presence_list_subscribe_io_error(void) {
	test_presence_list_subscribe_with_error(TRUE);
}

test_t presence_tests[] = {
	TEST_NO_TAG("Simple Subscribe", simple_subscribe),
	TEST_NO_TAG("Simple Publish", simple_publish),
	TEST_NO_TAG("Simple Publish with expires", publish_with_expires),
	/*TEST_NO_TAG("Call with presence", call_with_presence),*/
	TEST_NO_TAG("Unsubscribe while subscribing", unsubscribe_while_subscribing),
	TEST_NO_TAG("Presence information", presence_information),
	TEST_NO_TAG("App managed presence failure", subscribe_failure_handle_by_app),
	TEST_NO_TAG("Presence SUBSCRIBE forked", subscribe_presence_forked),
	TEST_NO_TAG("Presence SUBSCRIBE expired", subscribe_presence_expired),
	TEST_NO_TAG("Subscriber no loguer reachable using server",subscriber_no_longuer_reachable),
	TEST_NO_TAG("Subscribe with late publish", test_subscribe_notify_publish),
	TEST_NO_TAG("Forked subscribe with late publish", test_forked_subscribe_notify_publish),
	TEST_NO_TAG("Presence list", test_presence_list),
	TEST_NO_TAG("Presence list, subscription expiration for unknown contact",test_presence_list_subscription_expire_for_unknown),
	TEST_NO_TAG("Presence list, silent subscription expiration", test_presence_list_subscribe_dialog_expire),
	TEST_NO_TAG("Presence list, io error",test_presence_list_subscribe_io_error)
};

test_suite_t presence_test_suite = {"Presence", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
									sizeof(presence_tests) / sizeof(presence_tests[0]), presence_tests};
