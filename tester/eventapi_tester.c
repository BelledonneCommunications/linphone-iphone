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
#include <event.h>
#include "liblinphone_tester.h"


static const char *subscribe_content="<somexml>blabla</somexml>";
static const char *notify_content="<somexml2>blabla</somexml2>";

void linphone_notify_received(LinphoneCore *lc, LinphoneEvent *lev, const char *eventname, const LinphoneContent *content){
}

void linphone_subscription_state_change(LinphoneCore *lc, LinphoneEvent *lev, LinphoneSubscriptionState state) {
	stats* counters = get_stats(lc);
	LinphoneCoreManager *mgr=get_manager(lc);
	LinphoneContent content;
	
	content.type="application";
	content.subtype="somexml2";
	content.data=(void*)notify_content;
	content.size=strlen(notify_content);
	
	switch(state){
		case LinphoneSubscriptionNone:
		break;
		case LinphoneSubscriptionIncomingReceived:
			counters->number_of_LinphoneSubscriptionIncomingReceived++;
			if (!mgr->decline_subscribe)
				linphone_event_accept_subscription(lev);
			else
				linphone_event_deny_subscription(lev, LinphoneReasonDeclined);
		break;
		case LinphoneSubscriptionOutoingInit:
			counters->number_of_LinphoneSubscriptionOutgoingInit++;
		break;
		case LinphoneSubscriptionPending:
			counters->number_of_LinphoneSubscriptionPending++;
		break;
		case LinphoneSubscriptionActive:
			counters->number_of_LinphoneSubscriptionActive++;
			if (linphone_event_get_subscription_dir(lev)==LinphoneSubscriptionIncoming){
				mgr->lev=lev;
				linphone_event_notify(lev,&content);
			}
		break;
		case LinphoneSubscriptionTerminated:
			counters->number_of_LinphoneSubscriptionTerminated++;
			mgr->lev=NULL;
		break;
		case LinphoneSubscriptionError:
			counters->number_of_LinphoneSubscriptionError++;
			mgr->lev=NULL;
		break;
	}
}

static void subscribe_test_declined(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new(liblinphone_tester_file_prefix, "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(liblinphone_tester_file_prefix, "pauline_rc");
	LinphoneContent content;
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);


	content.type="application";
	content.subtype="somexml";
	content.data=(char*)subscribe_content;
	content.size=strlen(subscribe_content);
	
	pauline->decline_subscribe=TRUE;
	
	linphone_core_subscribe(marie->lc,pauline->identity,"dodo",600,&content);
	
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionOutgoingInit,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionIncomingReceived,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionError,1,21000));/*yes flexisip may wait 20 secs in case of forking*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionTerminated,1,1000));
	
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


static void subscribe_test_with_args(bool_t terminated_by_subscriber) {
	LinphoneCoreManager* marie = linphone_core_manager_new(liblinphone_tester_file_prefix, "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(liblinphone_tester_file_prefix, "pauline_rc");
	LinphoneContent content;
	LinphoneEvent *lev;
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);


	content.type="application";
	content.subtype="somexml";
	content.data=(char*)subscribe_content;
	content.size=strlen(subscribe_content);
	
	lev=linphone_core_subscribe(marie->lc,pauline->identity,"dodo",600,&content);
	
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionOutgoingInit,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionIncomingReceived,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionActive,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionActive,1,1000));

	if (terminated_by_subscriber){
		linphone_event_terminate(lev);
	}else{
		linphone_event_terminate(pauline->lev);
	}
	
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionTerminated,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionTerminated,1,1000));
	
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void subscribe_test_terminated_by_subscriber(void){
	subscribe_test_with_args(TRUE);
}

static void subscribe_test_terminated_by_notifier(void){
	subscribe_test_with_args(FALSE);
}

test_t subscribe_tests[] = {
	{ "Subscribe declined"	,	subscribe_test_declined 	},
	{ "Subscribe terminated by subscriber", subscribe_test_terminated_by_subscriber },
	{ "Subscribe terminated by notifier", subscribe_test_terminated_by_notifier }
};

test_suite_t subscribe_test_suite = {
	"Subscribe",
	NULL,
	NULL,
	sizeof(subscribe_tests) / sizeof(subscribe_tests[0]),
	subscribe_tests
};

