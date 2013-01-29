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


static int init(void) {
	return 0;
}
static int uninit(void) {
	return 0;
}


static bool_t call(LinphoneCoreManager* caller_mgr,LinphoneCoreManager* callee_mgr) {
	LinphoneProxyConfig* proxy;
	linphone_core_get_default_proxy(callee_mgr->lc,&proxy);
	int retry=0;
	CU_ASSERT_PTR_NOT_NULL_FATAL(proxy);


	CU_ASSERT_PTR_NOT_NULL_FATAL(linphone_core_invite_address(caller_mgr->lc,callee_mgr->identity));

	/*linphone_core_invite(caller_mgr->lc,"pauline");*/

	CU_ASSERT_TRUE_FATAL(wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallIncomingReceived,1));
	CU_ASSERT_TRUE(linphone_core_inc_invite_pending(callee_mgr->lc));
	CU_ASSERT_EQUAL(caller_mgr->stat.number_of_LinphoneCallOutgoingProgress,1);

	while ((caller_mgr->stat.number_of_LinphoneCallOutgoingRinging<1
			|| caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia<1)  && retry++ <20) {
			linphone_core_iterate(caller_mgr->lc);
			linphone_core_iterate(callee_mgr->lc);
			ms_usleep(100000);
	}


	CU_ASSERT_TRUE_FATAL(caller_mgr->stat.number_of_LinphoneCallOutgoingRinging|caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia);

	linphone_core_get_default_proxy(caller_mgr->lc,&proxy);
	CU_ASSERT_PTR_NOT_NULL_FATAL(proxy);
	LinphoneAddress* identity = linphone_address_new(linphone_proxy_config_get_identity(proxy));
	CU_ASSERT_TRUE(linphone_address_weak_equal(identity,linphone_core_get_current_call_remote_address(callee_mgr->lc)));
	linphone_address_destroy(identity);

	linphone_core_accept_call(callee_mgr->lc,linphone_core_get_current_call(callee_mgr->lc));

	CU_ASSERT_TRUE_FATAL(wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallConnected,1));
	CU_ASSERT_TRUE_FATAL(wait_for(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallConnected,1));
	/*just to sleep*/
	return wait_for(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallStreamsRunning,1)
			&&
			wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallStreamsRunning,1);

}
static void simple_call() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");

	LinphoneCore* lc_marie=marie->lc;
	LinphoneCore* lc_pauline=pauline->lc;
	stats* stat_marie=&marie->stat;
	stats* stat_pauline=&pauline->stat;



	linphone_core_invite(lc_marie,"pauline");

	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallIncomingReceived,1));
	CU_ASSERT_TRUE(linphone_core_inc_invite_pending(lc_pauline));
	CU_ASSERT_EQUAL(stat_marie->number_of_LinphoneCallOutgoingProgress,1);
	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallOutgoingRinging,1));

	LinphoneProxyConfig* proxy;
	linphone_core_get_default_proxy(lc_marie,&proxy);
	CU_ASSERT_PTR_NOT_NULL_FATAL(proxy);
	LinphoneAddress* identity = linphone_address_new(linphone_proxy_config_get_identity(proxy));
	CU_ASSERT_TRUE(linphone_address_weak_equal(identity,linphone_core_get_current_call_remote_address(lc_pauline)));
	linphone_address_destroy(identity);

	linphone_core_accept_call(lc_pauline,linphone_core_get_current_call(lc_pauline));

	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallConnected,1));
	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallConnected,1));
	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallStreamsRunning,1));
	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallStreamsRunning,1));
	/*just to sleep*/
	wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallStreamsRunning,3);
	linphone_core_terminate_all_calls(lc_pauline);
	CU_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void call_canceled() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");

	LinphoneCall* out_call = linphone_core_invite(pauline->lc,"marie");
	linphone_call_ref(out_call);
	CU_ASSERT_TRUE_FATAL(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallOutgoingInit,1));

	linphone_core_terminate_call(pauline->lc,out_call);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	//CU_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonCanceled);
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallEnd,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallIncomingReceived,0);
	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void call_ringing_canceled() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");

	LinphoneCall* out_call = linphone_core_invite(pauline->lc,"marie");
	linphone_call_ref(out_call);
	CU_ASSERT_TRUE_FATAL(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallIncomingReceived,1));

	linphone_core_terminate_call(pauline->lc,out_call);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	//CU_ASSERT_EQUAL(linphone_call_get_reason(in_call),LinphoneReasonDeclined);
	//CU_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonDeclined);
	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_early_declined() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");

	LinphoneCall* out_call = linphone_core_invite(pauline->lc,"marie");
	linphone_call_ref(out_call);
	CU_ASSERT_TRUE_FATAL(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallIncomingReceived,1));
	LinphoneCall* in_call=linphone_core_get_current_call(marie->lc);
	linphone_call_ref(in_call);

	linphone_core_terminate_call(marie->lc,in_call);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_EQUAL(linphone_call_get_reason(in_call),LinphoneReasonDeclined);
	CU_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonDeclined);
	linphone_call_unref(in_call);
	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_terminated_by_caller() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");

	CU_ASSERT_TRUE(call(pauline,marie));
	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_paused_resumed() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	LinphoneCall* call_obj;

	CU_ASSERT_TRUE(call(pauline,marie));
	call_obj = linphone_core_get_current_call(pauline->lc);

	linphone_core_pause_call(pauline->lc,call_obj);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPaused,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,1));

	linphone_core_resume_call(pauline->lc,call_obj);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));

	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_srtp() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");

	linphone_core_set_media_encryption(marie->lc,LinphoneMediaEncryptionSRTP);
	linphone_core_set_media_encryption(pauline->lc,LinphoneMediaEncryptionSRTP);

	CU_ASSERT_TRUE(call(pauline,marie));

	CU_ASSERT_EQUAL(linphone_core_get_media_encryption(marie->lc),LinphoneMediaEncryptionSRTP);
	CU_ASSERT_EQUAL(linphone_core_get_media_encryption(pauline->lc),LinphoneMediaEncryptionSRTP);

	/*just to sleep*/
	linphone_core_terminate_all_calls(marie->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_early_media() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_early_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");


	CU_ASSERT_TRUE(call(pauline,marie));

	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallIncomingEarlyMedia,1);
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallOutgoingEarlyMedia,1);
	/*just to sleep*/
	linphone_core_terminate_all_calls(marie->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

int call_test_suite () {
	CU_pSuite pSuite = CU_add_suite("Call", init, uninit);
	if (NULL == CU_add_test(pSuite, "call_early_declined", call_early_declined)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "call_canceled", call_canceled)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "call_ringing_canceled", call_ringing_canceled)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "simple_call", simple_call)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "call_early_media", call_early_media)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "call_terminated_by_caller", call_terminated_by_caller)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "call_paused_resumed", call_paused_resumed)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "call_srtp", call_srtp)) {
			return CU_get_error();
	}


	return 0;
}
