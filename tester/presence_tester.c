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

#include <stdio.h>
#include "CUnit/Basic.h"
#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"

static LinphoneCoreManager* presence_linphone_core_manager_new(char* username) {
	LinphoneCoreManager* mgr= linphone_core_manager_new2(liblinphone_tester_file_prefix, "empty_rc", FALSE);
	char* identity_char;
	mgr->identity= linphone_core_get_primary_contact_parsed(mgr->lc);
	linphone_address_set_username(mgr->identity,username);
	identity_char=linphone_address_as_string(mgr->identity);
	linphone_core_set_primary_contact(mgr->lc,identity_char);
	return mgr;
}
void new_subscribtion_request(LinphoneCore *lc, LinphoneFriend *lf, const char *url){
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
	char* from=linphone_address_as_string(linphone_friend_get_address(lf));
	ms_message("New Notify request  from [%s] ",from);
	ms_free(from);
	counters = get_stats(lc);
	counters->number_of_NotifyReceived++;

	switch(linphone_friend_get_status(lf)) {
		case LinphoneStatusOffline: counters->number_of_LinphoneStatusOffline++; break;
		case LinphoneStatusOnline: counters->number_of_LinphoneStatusOnline++; break;
		case LinphoneStatusBusy: counters->number_of_LinphoneStatusBusy++; break;
		case LinphoneStatusBeRightBack: counters->number_of_LinphoneStatusBeRightBack++; break;
		case LinphoneStatusAway: counters->number_of_LinphoneStatusAway++; break;
		case LinphoneStatusOnThePhone: counters->number_of_LinphoneStatusOnThePhone++; break;
		case LinphoneStatusOutToLunch: counters->number_of_LinphoneStatusOutToLunch++; break;
		case LinphoneStatusDoNotDisturb: counters->number_of_LinphoneStatusDoNotDisturb++; break;
		case LinphoneStatusMoved: counters->number_of_LinphoneStatusMoved++; break;
		case LinphoneStatusAltService: counters->number_of_LinphoneStatusMoved++; break;
		case LinphoneStatusPending: counters->number_of_LinphoneStatusPending++; break;
		case LinphoneStatusEnd: counters->number_of_LinphoneStatusEnd++; break;

	}
}

static void simple_publish(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new(liblinphone_tester_file_prefix, "marie_rc");
	LinphoneProxyConfig* proxy;
	int i=0;
	linphone_core_get_default_proxy(marie->lc,&proxy);
	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_enable_publish(proxy,TRUE);
	linphone_proxy_config_done(proxy);
	for (i=0;i<10;i++) {
		linphone_core_iterate(marie->lc);
		ms_usleep(100000);
	}
	linphone_core_set_presence_info(marie->lc,0,NULL,LinphoneStatusOffline);
	for (i=0;i<10;i++) {
		linphone_core_iterate(marie->lc);
		ms_usleep(100000);
	}
	linphone_core_manager_destroy(marie);
}

static bool_t subscribe_to_callee_presence(LinphoneCoreManager* caller_mgr,LinphoneCoreManager* callee_mgr) {
	stats initial_caller=caller_mgr->stat;
	stats initial_callee=callee_mgr->stat;
	bool_t result=FALSE;
	char* identity=linphone_address_as_string_uri_only(callee_mgr->identity);


	LinphoneFriend* friend=linphone_friend_new_with_addr(identity);
	linphone_friend_edit(friend);
	linphone_friend_enable_subscribes(friend,TRUE);
	linphone_friend_done(friend);

	linphone_core_add_friend(caller_mgr->lc,friend);

	result=wait_for(caller_mgr->lc,callee_mgr->lc,&callee_mgr->stat.number_of_LinphoneStatusOnline,initial_callee.number_of_LinphoneStatusOnline+1);
	result&=wait_for(caller_mgr->lc,callee_mgr->lc,&caller_mgr->stat.number_of_LinphoneStatusOnline,initial_caller.number_of_LinphoneStatusOnline+1);

	CU_ASSERT_EQUAL(callee_mgr->stat.number_of_NewSubscriptionRequest,initial_callee.number_of_NewSubscriptionRequest+1);
	CU_ASSERT_EQUAL(callee_mgr->stat.number_of_NotifyReceived,initial_callee.number_of_NotifyReceived+1);
	CU_ASSERT_EQUAL(caller_mgr->stat.number_of_NotifyReceived,initial_caller.number_of_NotifyReceived+1);
	ms_free(identity);
	return result;

}
static void simple_subscribe(void) {
	LinphoneCoreManager* marie = presence_linphone_core_manager_new("marie");
	LinphoneCoreManager* pauline = presence_linphone_core_manager_new("pauline");

	CU_ASSERT_TRUE(subscribe_to_callee_presence(marie,pauline));

	linphone_core_manager_destroy(marie);
	CU_ASSERT_FALSE(wait_for(NULL,pauline->lc,&pauline->stat.number_of_NewSubscriptionRequest,2)); /*just to wait for unsubscription even if not notified*/

	linphone_core_manager_destroy(pauline);
}

static void unsubscribe_while_subscribing(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new(liblinphone_tester_file_prefix, "marie_rc");
	LinphoneFriend* friend = linphone_friend_new_with_addr("sip:toto@git.linphone.org"); /*any unexisting address*/
	linphone_friend_edit(friend);
	linphone_friend_enable_subscribes(friend,TRUE);
	linphone_friend_done(friend);
	linphone_core_add_friend(marie->lc,friend);
	linphone_core_iterate(marie->lc);
	linphone_core_manager_destroy(marie);
}

static void call_with_presence(void) {
	LinphoneCoreManager* marie = presence_linphone_core_manager_new("marie");
	LinphoneCoreManager* pauline = presence_linphone_core_manager_new("pauline");
	LinphoneVideoPolicy pol={0};
	linphone_core_set_video_policy(marie->lc,&pol);
	CU_ASSERT_TRUE(subscribe_to_callee_presence(marie,pauline));

	CU_ASSERT_TRUE(call(marie,pauline));
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneStatusOnThePhone,1);
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneStatusOnThePhone,1);

	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);
	linphone_core_terminate_all_calls(marie->lc);
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneStatusOnline,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneStatusOnline,1));
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

test_t presence_tests[] = {
	{ "Simple Subscribe", simple_subscribe },
	{ "Simple Publish", simple_publish },
	{ "Call with Presence", call_with_presence },
	{ "Unsubscribe while subscribing", unsubscribe_while_subscribing },
};

test_suite_t presence_test_suite = {
	"Presence",
	NULL,
	NULL,
	sizeof(presence_tests) / sizeof(presence_tests[0]),
	presence_tests
};

