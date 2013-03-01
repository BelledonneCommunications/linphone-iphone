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


void new_subscribtion_request(LinphoneCore *lc, LinphoneFriend *lf, const char *url){
	char* from=linphone_address_as_string(linphone_friend_get_address(lf));
	ms_message("New subscription request  from [%s]  url [%s]",from,url);
	ms_free(from);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_NewSubscriptionRequest++;
	linphone_core_add_friend(lc,lf); /*accept subscription*/
}

void notify_presence_received(LinphoneCore *lc, LinphoneFriend * lf) {
	char* from=linphone_address_as_string(linphone_friend_get_address(lf));
	ms_message("New Notify request  from [%s] ",from);
	ms_free(from);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_NotifyReceived++;
}

static void simple_publish(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneProxyConfig* proxy;
	linphone_core_get_default_proxy(marie->lc,&proxy);
	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_enable_publish(proxy,TRUE);
	linphone_proxy_config_done(proxy);
	linphone_core_iterate(marie->lc);
	linphone_core_manager_destroy(marie);
}

static void simple_subscribe(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	const MSList* marie_friends = linphone_core_get_friend_list(marie->lc);
	CU_ASSERT_PTR_NOT_NULL_FATAL(marie_friends);
	LinphoneFriend* friend = (LinphoneFriend*) marie_friends->data;
	linphone_friend_edit(friend);
	linphone_friend_enable_subscribes(friend,TRUE);
	linphone_friend_done(friend);

	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_NewSubscriptionRequest,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_NotifyReceived,1));

	linphone_core_manager_destroy(marie);
	CU_ASSERT_FALSE(wait_for(NULL,pauline->lc,&pauline->stat.number_of_NewSubscriptionRequest,2)); /*just to wait for unsubscription even if not notified*/

	linphone_core_manager_destroy(pauline);
}

static void unsubscribe_while_subscribing(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneFriend* friend = linphone_friend_new_with_addr("sip:toto@git.linphone.org"); /*any unexisting address*/
	linphone_friend_edit(friend);
	linphone_friend_enable_subscribes(friend,TRUE);
	linphone_friend_done(friend);
	linphone_core_add_friend(marie->lc,friend);
	linphone_core_iterate(marie->lc);
	linphone_core_manager_destroy(marie);
}


test_t presence_tests[] = {
	{ "Simple Subscribe", simple_subscribe },
	{ "Simple Publish", simple_publish },
	{ "Unsubscribe while subscribing", unsubscribe_while_subscribing },
};

test_suite_t presence_test_suite = {
	"Presence",
	NULL,
	NULL,
	sizeof(presence_tests) / sizeof(presence_tests[0]),
	presence_tests
};

