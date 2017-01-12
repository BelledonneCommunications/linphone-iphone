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


#include "linphone/core.h"
#include "private.h"
#include "liblinphone_tester.h"

static LinphoneCoreManager* presence_linphone_core_manager_new_with_rc_name(char* username, char * rc_name) {
	LinphoneCoreManager* mgr= linphone_core_manager_new2( rc_name, FALSE);
	char* identity_char;
	mgr->identity= linphone_core_get_primary_contact_parsed(mgr->lc);
	linphone_address_set_username(mgr->identity,username);
	identity_char=linphone_address_as_string(mgr->identity);
	linphone_core_set_primary_contact(mgr->lc,identity_char);
	ms_free(identity_char);
	return mgr;
}
static LinphoneCoreManager* presence_linphone_core_manager_new(char* username) {
	return presence_linphone_core_manager_new_with_rc_name(username, "empty_rc");
}


void new_subscription_requested(LinphoneCore *lc, LinphoneFriend *lf, const char *url){
	stats* counters;
	const LinphoneAddress *addr = linphone_friend_get_address(lf);
	if (addr != NULL) {
		char* from=linphone_address_as_string(addr);
		ms_message("New subscription request from [%s] url [%s]",from,url);
		ms_free(from);
	}
	counters = get_stats(lc);
	counters->number_of_NewSubscriptionRequest++;
	linphone_core_add_friend(lc,lf); /*accept subscription*/
}

void notify_presence_received(LinphoneCore *lc, LinphoneFriend * lf) {
	stats* counters;
	unsigned int i;
	const LinphoneAddress *addr = linphone_friend_get_address(lf);
	if (addr != NULL) {
		char* from=linphone_address_as_string(addr);
		ms_message("New Notify request from [%s] ",from);
		ms_free(from);
	}
	counters = get_stats(lc);
	counters->number_of_NotifyPresenceReceived++;
	counters->last_received_presence = linphone_friend_get_presence_model(lf);
	if (linphone_presence_model_get_basic_status(counters->last_received_presence) == LinphonePresenceBasicStatusOpen) {
		counters->number_of_LinphonePresenceBasicStatusOpen++;
	} else if (linphone_presence_model_get_basic_status(counters->last_received_presence) == LinphonePresenceBasicStatusClosed) {
		counters->number_of_LinphonePresenceBasicStatusClosed++;
	} else {
		ms_error("Unexpected basic status [%i]",linphone_presence_model_get_basic_status(counters->last_received_presence));
	}
	if (linphone_presence_model_get_nb_activities(counters->last_received_presence) > 0) {
		for (i=0;counters->last_received_presence&&i<linphone_presence_model_get_nb_activities(counters->last_received_presence); i++) {
			LinphonePresenceActivity *activity = linphone_presence_model_get_nth_activity(counters->last_received_presence, i);
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
	} else {
		if (linphone_presence_model_get_basic_status(counters->last_received_presence) == LinphonePresenceBasicStatusOpen)
			counters->number_of_LinphonePresenceActivityOnline++;
		else
			counters->number_of_LinphonePresenceActivityOffline++;
	}
}

void notify_presence_received_for_uri_or_tel(LinphoneCore *lc, LinphoneFriend *lf, const char *uri_or_tel, const LinphonePresenceModel *presence) {
	stats *counters = get_stats(lc);
	ms_message("Presence notification for URI or phone number [%s]", uri_or_tel);
	counters->number_of_NotifyPresenceReceivedForUriOrTel++;
}

static void simple_publish_with_expire(int expires) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneProxyConfig* proxy;
	LinphonePresenceModel* presence;
	LinphoneCoreCbs *cbs = linphone_factory_create_core_cbs(linphone_factory_get());
	
	linphone_core_cbs_set_publish_state_changed(cbs, linphone_publish_state_changed);
	_linphone_core_add_callbacks(marie->lc, cbs, TRUE);
	linphone_core_cbs_unref(cbs);

	proxy = linphone_core_get_default_proxy_config(marie->lc);
	linphone_proxy_config_edit(proxy);
	if (expires > 0) {
		linphone_proxy_config_set_publish_expires(proxy,expires);
	}
	linphone_proxy_config_enable_publish(proxy,TRUE);
	linphone_proxy_config_done(proxy);

	BC_ASSERT_TRUE(wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphonePublishProgress,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphonePublishOk,1));

	presence =linphone_presence_model_new_with_activity(LinphonePresenceActivityOffline,NULL);
	linphone_core_set_presence_model(marie->lc,presence);

	BC_ASSERT_TRUE(wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphonePublishProgress,2));
	BC_ASSERT_TRUE(wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphonePublishOk,2));

	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_done(proxy);
	/*make sure no publish is sent*/
	BC_ASSERT_FALSE(wait_for_until(marie->lc,marie->lc,&marie->stat.number_of_LinphonePublishProgress,3,2000));

	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_enable_publish(proxy,FALSE);
	linphone_proxy_config_done(proxy);


	/*fixme PUBLISH state machine is too simple, clear state should only be propagated at API level  when 200ok is received*/
	/*BC_ASSERT_TRUE(wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphonePublishProgress,3));*/
	wait_for_until(marie->lc,marie->lc,NULL,0,2000);
	BC_ASSERT_TRUE(wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphonePublishCleared,1));

	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_enable_publish(proxy,TRUE);
	linphone_proxy_config_done(proxy);
	BC_ASSERT_TRUE(wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphonePublishProgress,3));
	BC_ASSERT_TRUE(wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphonePublishOk,3));

	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_set_publish_expires(proxy, linphone_proxy_config_get_publish_expires(proxy)+1);
	linphone_proxy_config_done(proxy);
	BC_ASSERT_TRUE(wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphonePublishProgress,4));
	BC_ASSERT_TRUE(wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphonePublishOk,4));

	linphone_core_manager_stop(marie);
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishCleared,3,int,"%i"); /*yes it is 3 because when we change the expires, a new LinphoneEvent is created*/
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishOk,4,int,"%i");
	linphone_core_manager_destroy(marie);
}

static void simple_publish(void) {
	simple_publish_with_expire(-1);
}

static void publish_with_expires(void) {
	simple_publish_with_expire(2);
}

static bool_t subscribe_to_callee_presence(LinphoneCoreManager* caller_mgr,LinphoneCoreManager* callee_mgr) {
	stats initial_caller=caller_mgr->stat;
	stats initial_callee=callee_mgr->stat;
	bool_t result=FALSE;
	char* identity=linphone_address_as_string_uri_only(callee_mgr->identity);


	LinphoneFriend* friend=linphone_core_create_friend_with_address(caller_mgr->lc,identity);
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

/* BEWARE this test will fail if the machine it is run on is behind an active firewall not sending ICMP errors on incoming connections! */
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
	ms_free(lf_identity);
	BC_ASSERT_PTR_NOT_NULL(lf);
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
static void simple_subscribe_with_early_notify(void) {

	LinphoneCoreManager* marie = presence_linphone_core_manager_new("marie");
	LinphoneCoreManager* pauline = presence_linphone_core_manager_new("pauline");
	LinphoneAddress *marie_identity_addr = linphone_address_clone(marie->identity);
	LpConfig *pauline_lp;

	char* pauline_identity=linphone_address_as_string_uri_only(pauline->identity);
	char* marie_identity;

	LinphoneFriend* pauline_s_friend;
	LinphoneFriend* marie_s_friend=linphone_core_create_friend_with_address(marie->lc,pauline_identity);

	pauline_lp = linphone_core_get_config(pauline->lc);
	lp_config_set_int(pauline_lp,"sip","notify_pending_state",1);

	linphone_friend_edit(marie_s_friend);
	linphone_friend_enable_subscribes(marie_s_friend,TRUE);
	linphone_friend_done(marie_s_friend);
	linphone_core_add_friend(marie->lc,marie_s_friend);
	ms_free(pauline_identity);


	/*to simulate pending state.*/

	linphone_address_set_port(marie_identity_addr,0);
	marie_identity=linphone_address_as_string_uri_only(marie_identity_addr);
	pauline_s_friend=linphone_core_create_friend_with_address(pauline->lc,marie_identity);
	linphone_core_add_friend(pauline->lc,pauline_s_friend);

	ms_free(marie_identity);

	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_NotifyPresenceReceived,1));
	BC_ASSERT_EQUAL(linphone_friend_get_subscription_state(marie_s_friend), LinphoneSubscriptionPending,int, "%d");

	wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityOnline,marie->stat.number_of_LinphonePresenceActivityOnline+1);

	BC_ASSERT_EQUAL(marie->stat.number_of_NotifyPresenceReceived,2, int, "%d");

	linphone_friend_unref(marie_s_friend);
	linphone_friend_unref(pauline_s_friend);
	linphone_address_unref(marie_identity_addr);
	linphone_core_manager_destroy(marie);

	linphone_core_manager_destroy(pauline);
}


static void unsubscribe_while_subscribing(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneFriend* friend = linphone_core_create_friend_with_address(marie->lc, "sip:toto@git.linphone.org"); /*any unexisting address*/
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
	LinphoneCoreManager* pauline1 = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_tcp_rc" : "pauline_tcp_rc");
	LinphoneCoreManager* pauline2 = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_tcp_rc" : "pauline_tcp_rc");
	LinphoneFriend *lf;
	bctbx_list_t *lcs = NULL;

	lcs = bctbx_list_append(lcs, marie->lc);
	lcs = bctbx_list_append(lcs, pauline1->lc);
	lcs = bctbx_list_append(lcs, pauline2->lc);

	lf = linphone_core_create_friend(marie->lc);
	linphone_friend_set_address(lf, pauline1->identity);
	linphone_friend_enable_subscribes(lf, TRUE);

	linphone_core_add_friend(marie->lc, lf);
	linphone_friend_unref(lf);

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline1->stat.number_of_NewSubscriptionRequest,1, 10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline2->stat.number_of_NewSubscriptionRequest,1, 2000));

	/*we should get only one notify*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePresenceActivityOnline,1, 10000));
	BC_ASSERT_FALSE(wait_for_list(lcs,&marie->stat.number_of_LinphonePresenceActivityOnline,2, 2000));

	/*marie also shall receive two SUBSCRIBEs from the two paulines, but won't be notified to the app since
	 Marie set Pauline as a friend.*/
	BC_ASSERT_EQUAL(marie->stat.number_of_NewSubscriptionRequest, 0, int, "%d");
	/*and the two paulines shall be notified of marie's presence*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline1->stat.number_of_LinphonePresenceActivityOnline,1, 3000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline2->stat.number_of_LinphonePresenceActivityOnline,1, 2000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline1);
	linphone_core_manager_destroy(pauline2);

	bctbx_list_free(lcs);
}

static void subscribe_presence_expired(void){
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline1 = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneFriend *lf;
	bctbx_list_t *lcs = NULL;

	lcs = bctbx_list_append(lcs, marie->lc);
	lcs = bctbx_list_append(lcs, pauline1->lc);

	lp_config_set_int(marie->lc->config, "sip", "subscribe_expires", 10);

	lf = linphone_core_create_friend(marie->lc);
	linphone_friend_set_address(lf, pauline1->identity);
	linphone_friend_enable_subscribes(lf, TRUE);

	linphone_core_add_friend(marie->lc, lf);
	linphone_friend_unref(lf);
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline1->stat.number_of_NewSubscriptionRequest,1, 5000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePresenceActivityOnline,1, 2000));

	lf = linphone_core_find_friend(pauline1->lc, marie->identity);
	BC_ASSERT_PTR_NOT_NULL(lf);
	if (lf) {
		BC_ASSERT_PTR_NOT_NULL(lf->insubs);

		/*marie comes offline suddenly*/
		linphone_core_set_network_reachable(marie->lc, FALSE);
		/*after a certain time, pauline shall see the incoming SUBSCRIBE expired*/
		wait_for_list(lcs,NULL, 0, 11000);
		BC_ASSERT_PTR_NULL(lf->insubs);

		/*just make network reachable so that marie can unregister properly*/
		linphone_core_set_network_reachable(marie->lc, TRUE);
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneRegistrationOk,2, 10000));
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline1);

	bctbx_list_free(lcs);
}

static void simple_subscribe_with_friend_from_rc(void) {
	LinphoneCoreManager* pauline = presence_linphone_core_manager_new("pauline");
	LinphoneCoreManager *marie = presence_linphone_core_manager_new_with_rc_name("marie", "pauline_as_friend_rc");
	LinphoneFriend *pauline_as_friend;

	BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(linphone_core_get_friend_list(marie->lc)), 1, unsigned int , "%u");

	if (bctbx_list_size(linphone_core_get_friend_list(marie->lc))>0) {
		pauline_as_friend = (LinphoneFriend*)linphone_core_get_friend_list(marie->lc)->data;
		linphone_friend_edit(pauline_as_friend);
		linphone_friend_set_address(pauline_as_friend, pauline->identity); /*hack to update addr with port number*/
		linphone_friend_done(pauline_as_friend);
	}

	BC_ASSERT_TRUE (wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityOnline,1));

	BC_ASSERT_EQUAL(pauline->stat.number_of_NewSubscriptionRequest,1, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_NotifyPresenceReceived,1, int, "%d");

	linphone_core_manager_destroy(marie);
	/*unsubscribe is not reported ?*/
	BC_ASSERT_FALSE(wait_for(NULL,pauline->lc,&pauline->stat.number_of_NewSubscriptionRequest,2)); /*just to wait for unsubscription even if not notified*/

	linphone_core_manager_destroy(pauline);
}

test_t presence_tests[] = {
	TEST_ONE_TAG("Simple Subscribe", simple_subscribe,"presence"),
	TEST_ONE_TAG("Simple Subscribe with early NOTIFY", simple_subscribe_with_early_notify,"presence"),
	TEST_NO_TAG("Simple Subscribe with friend from rc", simple_subscribe_with_friend_from_rc),
	TEST_NO_TAG("Simple Publish", simple_publish),
	TEST_NO_TAG("Simple Publish with expires", publish_with_expires),
	/*TEST_ONE_TAG("Call with presence", call_with_presence, "LeaksMemory"),*/
	TEST_NO_TAG("Unsubscribe while subscribing", unsubscribe_while_subscribing),
	TEST_NO_TAG("Presence information", presence_information),
	TEST_NO_TAG("App managed presence failure", subscribe_failure_handle_by_app),
	TEST_NO_TAG("Presence SUBSCRIBE forked", subscribe_presence_forked),
	TEST_NO_TAG("Presence SUBSCRIBE expired", subscribe_presence_expired),
};

test_suite_t presence_test_suite = {"Presence", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
									sizeof(presence_tests) / sizeof(presence_tests[0]), presence_tests};
