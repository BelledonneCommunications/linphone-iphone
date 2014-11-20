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
#include "lpconfig.h"
#include <event.h>
#include "liblinphone_tester.h"


static const char *subscribe_content="<somexml>blabla</somexml>";
static const char *notify_content="<somexml2>blabla</somexml2>";

const char *liblinphone_tester_get_subscribe_content(void){
	return subscribe_content;
}

const char *liblinphone_tester_get_notify_content(void){
	return notify_content;
}

void linphone_notify_received(LinphoneCore *lc, LinphoneEvent *lev, const char *eventname, const LinphoneContent *content){
	LinphoneCoreManager *mgr;
	CU_ASSERT_PTR_NOT_NULL_FATAL(content);
	CU_ASSERT_TRUE(strcmp(notify_content,(const char*)linphone_content_get_buffer(content))==0);
	mgr=get_manager(lc);
	mgr->stat.number_of_NotifyReceived++;
}

void linphone_subscription_state_change(LinphoneCore *lc, LinphoneEvent *lev, LinphoneSubscriptionState state) {
	stats* counters = get_stats(lc);
	LinphoneCoreManager *mgr=get_manager(lc);
	LinphoneContent* content;
	const LinphoneAddress* from_addr = linphone_event_get_from(lev);
	char* from = linphone_address_as_string(from_addr);
	content = linphone_core_create_content(lc);
	linphone_content_set_type(content,"application");
	linphone_content_set_subtype(content,"somexml2");
	linphone_content_set_buffer(content,notify_content,strlen(notify_content));
	
	ms_message("Subscription state [%s] from [%s]",linphone_subscription_state_to_string(state),from);
	ms_free(from);

	switch(state){
		case LinphoneSubscriptionNone:
		break;
		case LinphoneSubscriptionIncomingReceived:
			counters->number_of_LinphoneSubscriptionIncomingReceived++;
			mgr->lev=lev;
			if (!mgr->decline_subscribe)
				linphone_event_accept_subscription(lev);
			else
				linphone_event_deny_subscription(lev, LinphoneReasonDeclined);
		break;
		case LinphoneSubscriptionOutgoingInit:
			counters->number_of_LinphoneSubscriptionOutgoingInit++;
		break;
		case LinphoneSubscriptionPending:
			counters->number_of_LinphoneSubscriptionPending++;
		break;
		case LinphoneSubscriptionActive:
			counters->number_of_LinphoneSubscriptionActive++;
			if (linphone_event_get_subscription_dir(lev)==LinphoneSubscriptionIncoming){
				mgr->lev=lev;
				linphone_event_notify(lev,content);
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
		case LinphoneSubscriptionExpiring:
			counters->number_of_LinphoneSubscriptionExpiring++;
			mgr->lev=NULL;
		break;
	}
	linphone_content_unref(content);
}

void linphone_publish_state_changed(LinphoneCore *lc, LinphoneEvent *ev, LinphonePublishState state){
	stats* counters = get_stats(lc);
	const LinphoneAddress* from_addr = linphone_event_get_from(ev);
	char* from = linphone_address_as_string(from_addr);
	ms_message("Publish state [%s] from [%s]",linphone_publish_state_to_string(state),from);
	ms_free(from);
	switch(state){
		case LinphonePublishProgress: counters->number_of_LinphonePublishProgress++; break;
		case LinphonePublishOk: 
			/*make sure custom header access API is working*/
			CU_ASSERT_PTR_NOT_NULL(linphone_event_get_custom_header(ev,"From"));
			counters->number_of_LinphonePublishOk++; 
			break;
		case LinphonePublishError: counters->number_of_LinphonePublishError++; break;
		case LinphonePublishExpiring: counters->number_of_LinphonePublishExpiring++; break;
		case LinphonePublishCleared: counters->number_of_LinphonePublishCleared++;break;
		default:
		break;
	}
	
}

static void subscribe_test_declined(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneContent* content;
	LinphoneEvent *lev;
	const LinphoneErrorInfo *ei;
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);

	content = linphone_core_create_content(marie->lc);
	linphone_content_set_type(content,"application");
	linphone_content_set_subtype(content,"somexml");
	linphone_content_set_buffer(content,subscribe_content,strlen(subscribe_content));

	pauline->decline_subscribe=TRUE;

	lev=linphone_core_subscribe(marie->lc,pauline->identity,"dodo",600,content);
	linphone_event_ref(lev);

	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionOutgoingInit,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionIncomingReceived,1,3000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionError,1,21000));/*yes flexisip may wait 20 secs in case of forking*/
	ei=linphone_event_get_error_info(lev);
	CU_ASSERT_PTR_NOT_NULL(ei);
	if (ei){
		CU_ASSERT_EQUAL(linphone_error_info_get_protocol_code(ei),603);
		CU_ASSERT_PTR_NOT_NULL(linphone_error_info_get_phrase(ei));
	}
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionTerminated,1,1000));

	linphone_content_unref(content);
	linphone_event_unref(lev);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

typedef enum RefreshTestType{
	NoRefresh,
	AutoRefresh,
	ManualRefresh
}RefreshTestType;

static void subscribe_test_with_args(bool_t terminated_by_subscriber, RefreshTestType refresh_type) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneContent* content;
	LinphoneEvent *lev;
	int expires= refresh_type!=NoRefresh ? 4 : 600;
	MSList* lcs=ms_list_append(NULL,marie->lc);

	lcs=ms_list_append(lcs,pauline->lc);

	if (refresh_type==ManualRefresh){
		lp_config_set_int(marie->lc->config,"sip","refresh_generic_subscribe",0);
	}

	content = linphone_core_create_content(marie->lc);
	linphone_content_set_type(content,"application");
	linphone_content_set_subtype(content,"somexml");
	linphone_content_set_buffer(content,subscribe_content,strlen(subscribe_content));

	lev=linphone_core_subscribe(marie->lc,pauline->identity,"dodo",expires,content);
	
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionOutgoingInit,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionIncomingReceived,1,3000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionActive,1,3000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionActive,1,1000));

	/*make sure marie receives first notification before terminating*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_NotifyReceived,1,1000));

	if (refresh_type==AutoRefresh){
		wait_for_list(lcs,NULL,0,6000);
		CU_ASSERT_TRUE(linphone_event_get_subscription_state(pauline->lev)==LinphoneSubscriptionActive);
	}else if (refresh_type==ManualRefresh){
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionExpiring,1,4000));
		linphone_event_update_subscribe(lev,NULL);
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionActive,2,2000));
	}

	if (terminated_by_subscriber){
		linphone_event_terminate(lev);
	}else{
		CU_ASSERT_PTR_NOT_NULL_FATAL(pauline->lev);
		linphone_event_terminate(pauline->lev);
	}

	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionTerminated,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionTerminated,1,1000));

	linphone_content_unref(content);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void subscribe_test_with_args2(bool_t terminated_by_subscriber, RefreshTestType refresh_type) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneContent* content;
	LinphoneEvent *lev;
	int expires= refresh_type!=NoRefresh ? 4 : 600;
	MSList* lcs=ms_list_append(NULL,marie->lc);
	
	lcs=ms_list_append(lcs,pauline->lc);

	if (refresh_type==ManualRefresh){
		lp_config_set_int(marie->lc->config,"sip","refresh_generic_subscribe",0);
	}

	content = linphone_core_create_content(marie->lc);
	linphone_content_set_type(content,"application");
	linphone_content_set_subtype(content,"somexml");
	linphone_content_set_buffer(content,subscribe_content,strlen(subscribe_content));

	lev=linphone_core_create_subscribe(marie->lc,pauline->identity,"dodo",expires);
	linphone_event_add_custom_header(lev,"My-Header","pouet");
	linphone_event_add_custom_header(lev,"My-Header2","pimpon");
	linphone_event_send_subscribe(lev,content);

	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionOutgoingInit,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionIncomingReceived,1,3000));

	/*check good receipt of custom headers*/
	CU_ASSERT_STRING_EQUAL(linphone_event_get_custom_header(pauline->lev,"My-Header"),"pouet");
	CU_ASSERT_STRING_EQUAL(linphone_event_get_custom_header(pauline->lev,"My-Header2"),"pimpon");
	
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionActive,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionActive,1,1000));

	/*make sure marie receives first notification before terminating*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_NotifyReceived,1,1000));
	
	if (refresh_type==AutoRefresh){
		wait_for_list(lcs,NULL,0,6000);
		CU_ASSERT_TRUE(linphone_event_get_subscription_state(pauline->lev)==LinphoneSubscriptionActive);
	}else if (refresh_type==ManualRefresh){
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionExpiring,1,4000));
		linphone_event_update_subscribe(lev,NULL);
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionActive,2,2000));
	}

	if (terminated_by_subscriber){
		linphone_event_terminate(lev);
	}else{
		CU_ASSERT_PTR_NOT_NULL_FATAL(pauline->lev);
		linphone_event_terminate(pauline->lev);
	}

	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionTerminated,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionTerminated,1,1000));

	linphone_content_unref(content);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void subscribe_test_terminated_by_subscriber(void){
	subscribe_test_with_args(TRUE,NoRefresh);
}

static void subscribe_test_terminated_by_notifier(void){
	subscribe_test_with_args(FALSE,NoRefresh);
}

/* Caution: this test does not really check that the subscribe are refreshed, because the core is not managing the expiration of 
 * unrefreshed subscribe dialogs. So it is just checking that it is not crashing.
 */
static void subscribe_test_refreshed(void){
	subscribe_test_with_args(TRUE,AutoRefresh);
}

static void subscribe_test_with_custom_header(void){
	subscribe_test_with_args2(TRUE,NoRefresh);
}

static void subscribe_test_manually_refreshed(void){
	subscribe_test_with_args(TRUE,ManualRefresh);
}

static void publish_test_with_args(bool_t refresh, int expires){
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneContent* content;
	LinphoneEvent *lev;
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);

	content = linphone_core_create_content(marie->lc);
	linphone_content_set_type(content,"application");
	linphone_content_set_subtype(content,"somexml");
	linphone_content_set_buffer(content,subscribe_content,strlen(subscribe_content));

	lp_config_set_int(marie->lc->config,"sip","refresh_generic_publish",refresh);

	lev=linphone_core_create_publish(marie->lc,pauline->identity,"dodo",expires);
	linphone_event_add_custom_header(lev,"CustomHeader","someValue");
	linphone_event_send_publish(lev,content);
	linphone_event_ref(lev);

	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePublishProgress,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePublishOk,1,3000));

	if (!refresh){
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePublishExpiring,1,5000));
		linphone_event_update_publish(lev,content);
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePublishProgress,1,1000));
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePublishOk,1,3000));
	}else{
		
	}

	linphone_event_terminate(lev);

	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePublishCleared,1,3000));

	linphone_event_unref(lev);

	linphone_content_unref(content);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void publish_test(){
	publish_test_with_args(TRUE,5);
}

static void publish_no_auto_test(){
	publish_test_with_args(FALSE,5);
}

static void publish_without_expires(){
	publish_test_with_args(TRUE,-1);
}

test_t event_tests[] = {
	{ "Subscribe declined"	,	subscribe_test_declined 	},
	{ "Subscribe terminated by subscriber", subscribe_test_terminated_by_subscriber },
	{ "Subscribe with custom headers", subscribe_test_with_custom_header },
	{ "Subscribe refreshed", subscribe_test_refreshed },
	{ "Subscribe manually refreshed", subscribe_test_manually_refreshed },
	{ "Subscribe terminated by notifier", subscribe_test_terminated_by_notifier },
	{ "Publish", publish_test },
	{ "Publish without expires", publish_without_expires },
	{ "Publish without automatic refresh",publish_no_auto_test }
};

test_suite_t event_test_suite = {
	"Event",
	NULL,
	NULL,
	sizeof(event_tests) / sizeof(event_tests[0]),
	event_tests
};

