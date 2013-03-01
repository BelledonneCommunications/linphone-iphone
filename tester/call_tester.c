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


void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg){
	char* to=linphone_address_as_string(linphone_call_get_call_log(call)->to);
	char* from=linphone_address_as_string(linphone_call_get_call_log(call)->from);

	ms_message("call from [%s] to [%s], new state is [%s]",from,to,linphone_call_state_to_string(cstate));
	ms_free(to);
	ms_free(from);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	switch (cstate) {
	case LinphoneCallIncomingReceived:counters->number_of_LinphoneCallIncomingReceived++;break;
	case LinphoneCallOutgoingInit :counters->number_of_LinphoneCallOutgoingInit++;break;
	case LinphoneCallOutgoingProgress :counters->number_of_LinphoneCallOutgoingProgress++;break;
	case LinphoneCallOutgoingRinging :counters->number_of_LinphoneCallOutgoingRinging++;break;
	case LinphoneCallOutgoingEarlyMedia :counters->number_of_LinphoneCallOutgoingEarlyMedia++;break;
	case LinphoneCallConnected :counters->number_of_LinphoneCallConnected++;break;
	case LinphoneCallStreamsRunning :counters->number_of_LinphoneCallStreamsRunning++;break;
	case LinphoneCallPausing :counters->number_of_LinphoneCallPausing++;break;
	case LinphoneCallPaused :counters->number_of_LinphoneCallPaused++;break;
	case LinphoneCallResuming :counters->number_of_LinphoneCallResuming++;break;
	case LinphoneCallRefered :counters->number_of_LinphoneCallRefered++;break;
	case LinphoneCallError :counters->number_of_LinphoneCallError++;break;
	case LinphoneCallEnd :counters->number_of_LinphoneCallEnd++;break;
	case LinphoneCallPausedByRemote :counters->number_of_LinphoneCallPausedByRemote++;break;
	case LinphoneCallUpdatedByRemote :counters->number_of_LinphoneCallUpdatedByRemote++;break;
	case LinphoneCallIncomingEarlyMedia :counters->number_of_LinphoneCallIncomingEarlyMedia++;break;
	case LinphoneCallUpdating :counters->number_of_LinphoneCallUpdating++;break;
	case LinphoneCallReleased :counters->number_of_LinphoneCallReleased++;break;
	default:
		CU_FAIL("unexpected event");break;
	}
}

void linphone_transfer_state_changed(LinphoneCore *lc, LinphoneCall *transfered, LinphoneCallState new_call_state) {
	char* to=linphone_address_as_string(linphone_call_get_call_log(transfered)->to);
	char* from=linphone_address_as_string(linphone_call_get_call_log(transfered)->from);

	ms_message("Transferred call from [%s] to [%s], new state is [%s]",from,to,linphone_call_state_to_string(new_call_state));
	ms_free(to);
	ms_free(from);

	stats* counters = (stats*)linphone_core_get_user_data(lc);
	switch (new_call_state) {
	case LinphoneCallOutgoingInit :counters->number_of_LinphoneTransferCallOutgoingInit++;break;
	case LinphoneCallOutgoingProgress :counters->number_of_LinphoneTransfertCallOutgoingProgress++;break;
	case LinphoneCallOutgoingRinging :counters->number_of_LinphoneTransfertCallOutgoingRinging++;break;
	case LinphoneCallOutgoingEarlyMedia :counters->number_of_LinphoneTransfertCallOutgoingEarlyMedia++;break;
	case LinphoneCallConnected :counters->number_of_LinphoneTransfertCallConnected++;break;
	case LinphoneCallStreamsRunning :counters->number_of_LinphoneTransfertCallStreamsRunning++;break;
	default:
		CU_FAIL("unexpected event");break;
	}
}

static void linphone_call_cb(LinphoneCall *call,void * user_data) {
	char* to=linphone_address_as_string(linphone_call_get_call_log(call)->to);
	char* from=linphone_address_as_string(linphone_call_get_call_log(call)->from);
	LinphoneCore* lc=(LinphoneCore*)user_data;
	ms_message("call from [%s] to [%s] receive iFrame",from,to);
	ms_free(to);
	ms_free(from);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_IframeDecoded++;
}

static bool_t call(LinphoneCoreManager* caller_mgr,LinphoneCoreManager* callee_mgr) {
	LinphoneProxyConfig* proxy;
	linphone_core_get_default_proxy(callee_mgr->lc,&proxy);
	int retry=0;
	stats initial_caller=caller_mgr->stat;
	stats initial_callee=callee_mgr->stat;

	CU_ASSERT_PTR_NOT_NULL_FATAL(proxy);


	CU_ASSERT_PTR_NOT_NULL_FATAL(linphone_core_invite_address(caller_mgr->lc,callee_mgr->identity));

	/*linphone_core_invite(caller_mgr->lc,"pauline");*/

	CU_ASSERT_TRUE_FATAL(wait_for(callee_mgr->lc
									,caller_mgr->lc
									,&callee_mgr->stat.number_of_LinphoneCallIncomingReceived
									,initial_callee.number_of_LinphoneCallIncomingReceived+1));
	CU_ASSERT_TRUE(linphone_core_inc_invite_pending(callee_mgr->lc));
	CU_ASSERT_EQUAL(caller_mgr->stat.number_of_LinphoneCallOutgoingProgress,initial_caller.number_of_LinphoneCallOutgoingProgress+1);


	while (caller_mgr->stat.number_of_LinphoneCallOutgoingRinging!=(initial_caller.number_of_LinphoneCallOutgoingRinging + 1)
			&& caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia!=(initial_caller.number_of_LinphoneCallOutgoingEarlyMedia +1)
			&& retry++ <20) {
			linphone_core_iterate(caller_mgr->lc);
			linphone_core_iterate(callee_mgr->lc);
			ms_usleep(100000);
	}


	CU_ASSERT_TRUE_FATAL((caller_mgr->stat.number_of_LinphoneCallOutgoingRinging==initial_caller.number_of_LinphoneCallOutgoingRinging+1)
							|(caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia==initial_caller.number_of_LinphoneCallOutgoingEarlyMedia+1));

	linphone_core_get_default_proxy(caller_mgr->lc,&proxy);
	CU_ASSERT_PTR_NOT_NULL_FATAL(proxy);
	LinphoneAddress* identity = linphone_address_new(linphone_proxy_config_get_identity(proxy));
	CU_ASSERT_TRUE(linphone_address_weak_equal(identity,linphone_core_get_current_call_remote_address(callee_mgr->lc)));
	linphone_address_destroy(identity);

	linphone_core_accept_call(callee_mgr->lc,linphone_core_get_current_call(callee_mgr->lc));

	CU_ASSERT_TRUE_FATAL(wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallConnected,initial_callee.number_of_LinphoneCallConnected+1));
	CU_ASSERT_TRUE_FATAL(wait_for(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallConnected,initial_callee.number_of_LinphoneCallConnected+1));
	/*just to sleep*/
	return wait_for(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallStreamsRunning,initial_caller.number_of_LinphoneCallStreamsRunning+1)
			&&
			wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallStreamsRunning,initial_callee.number_of_LinphoneCallStreamsRunning+1);

}

static void simple_call(void) {
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

static void cancelled_call(void) {
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

static void call_with_dns_time_out(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new(NULL);
	LCSipTransports transport = {9773,0,0,0};
	linphone_core_set_sip_transports(marie->lc,&transport);
	linphone_core_iterate(marie->lc);
	sal_set_dns_timeout(marie->lc->sal,0);
	linphone_core_invite(marie->lc,"sip:toto@toto.com");
	linphone_core_iterate(marie->lc);
	linphone_core_iterate(marie->lc);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallOutgoingInit,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallOutgoingProgress,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallError,1);
	linphone_core_manager_destroy(marie);
}

static void cancelled_ringing_call(void) {
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

static void early_declined_call(void) {
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

static void call_terminated_by_caller(void) {
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

static void call_paused_resumed(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	LinphoneCall* call_obj;

	CU_ASSERT_TRUE(call(pauline,marie));
	call_obj = linphone_core_get_current_call(pauline->lc);

	linphone_core_pause_call(pauline->lc,call_obj);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausing,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPaused,1));

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

static bool_t pause_call_1(LinphoneCoreManager* mgr_1,LinphoneCall* call_1,LinphoneCoreManager* mgr_2,LinphoneCall* call_2) {
	stats initial_call_stat_1=mgr_1->stat;
	stats initial_call_stat_2=mgr_2->stat;
	linphone_core_pause_call(mgr_1->lc,call_1);
	CU_ASSERT_TRUE(wait_for(mgr_1->lc,mgr_2->lc,&mgr_1->stat.number_of_LinphoneCallPausing,initial_call_stat_1.number_of_LinphoneCallPausing+1));
	CU_ASSERT_TRUE(wait_for(mgr_1->lc,mgr_2->lc,&mgr_1->stat.number_of_LinphoneCallPaused,initial_call_stat_1.number_of_LinphoneCallPaused+1));
	CU_ASSERT_TRUE(wait_for(mgr_1->lc,mgr_2->lc,&mgr_2->stat.number_of_LinphoneCallPausedByRemote,initial_call_stat_2.number_of_LinphoneCallPausedByRemote+1));
	CU_ASSERT_EQUAL(linphone_call_get_state(call_1),LinphoneCallPaused);
	CU_ASSERT_EQUAL(linphone_call_get_state(call_2),LinphoneCallPausedByRemote);
	return linphone_call_get_state(call_1) == LinphoneCallPaused && linphone_call_get_state(call_2)==LinphoneCallPausedByRemote;
}

static void call_paused_resumed_from_callee(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	LinphoneCall* call_obj;

	CU_ASSERT_TRUE(call(pauline,marie));
	call_obj = linphone_core_get_current_call(marie->lc);

	linphone_core_pause_call(marie->lc,call_obj);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausing,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausedByRemote,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPaused,1));

	linphone_core_resume_call(marie->lc,call_obj);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));

	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_video_added(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	LinphoneCall* call_obj;
	LinphoneVideoPolicy  pauline_policy;
	pauline_policy.automatically_accept=TRUE;
	pauline_policy.automatically_initiate=TRUE;
	LinphoneCallParams* marie_params;

	CU_ASSERT_TRUE(call(pauline,marie));

	linphone_core_enable_video(marie->lc,TRUE,TRUE);
	linphone_core_enable_video(pauline->lc,TRUE,FALSE);
	linphone_core_set_video_policy(pauline->lc,&pauline_policy);

	call_obj = linphone_core_get_current_call(marie->lc);
	marie_params = linphone_call_params_copy(linphone_call_get_current_params(call_obj));
	/*add video*/
	linphone_call_params_enable_video(marie_params,TRUE);
	linphone_core_update_call(marie->lc,call_obj,marie_params);

	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallUpdatedByRemote,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallUpdating,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));

	CU_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(marie->lc))));
	CU_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc))));

	linphone_call_set_next_video_frame_decoded_callback(call_obj,linphone_call_cb,marie->lc);
	/*send vfu*/
	linphone_call_send_vfu_request(call_obj);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_IframeDecoded,1));

	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void simple_conference(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	stats initial_marie_stat;
	stats initial_pauline_stat;
	stats initial_laure_stat;
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new("./tester/laure_rc");

	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);

	LinphoneCall* marie_call_pauline;
	LinphoneCall* pauline_called_by_marie;
	LinphoneCall* marie_call_laure;

	CU_ASSERT_TRUE(call(marie,pauline));
	marie_call_pauline=linphone_core_get_current_call(marie->lc);
	pauline_called_by_marie=linphone_core_get_current_call(pauline->lc);
	CU_ASSERT_TRUE(pause_call_1(marie,marie_call_pauline,pauline,pauline_called_by_marie));

	CU_ASSERT_TRUE(call(marie,laure));
	initial_marie_stat=marie->stat;
	initial_pauline_stat=pauline->stat;
	initial_laure_stat=laure->stat;

	marie_call_laure=linphone_core_get_current_call(marie->lc);

	linphone_core_add_to_conference(marie->lc,marie_call_laure);
	CU_ASSERT_TRUE(wait_for(marie->lc,laure->lc,&marie->stat.number_of_LinphoneCallUpdating,initial_marie_stat.number_of_LinphoneCallUpdating+1));



	linphone_core_add_to_conference(marie->lc,marie_call_pauline);

	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallResuming,initial_marie_stat.number_of_LinphoneCallResuming+1,2000));

	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,initial_pauline_stat.number_of_LinphoneCallStreamsRunning+1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,initial_marie_stat.number_of_LinphoneCallStreamsRunning+2,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,initial_laure_stat.number_of_LinphoneCallStreamsRunning+1,2000));

	CU_ASSERT_TRUE(linphone_core_is_in_conference(marie->lc));
	CU_ASSERT_EQUAL(linphone_core_get_conference_size(marie->lc),3)

	linphone_core_terminate_conference(marie->lc);

	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,1,2000));


	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	ms_list_free(lcs);
}

static void srtp_call(void) {
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

static void early_media_call(void) {
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

static void simple_call_transfer(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new("./tester/laure_rc");
	LinphoneCall* pauline_called_by_marie;

	char* laure_identity=linphone_address_as_string(laure->identity);
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);


	CU_ASSERT_TRUE(call(marie,pauline));
	pauline_called_by_marie=linphone_core_get_current_call(pauline->lc);

	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);
	reset_counters(&laure->stat);


	linphone_core_transfer_call(pauline->lc,pauline_called_by_marie,laure_identity);
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallRefered,1,2000));
	/*marie pausing pauline*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallPausing,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallPausedByRemote,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallPaused,1,2000));
	/*marie calling laure*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallOutgoingProgress,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneTransferCallOutgoingInit,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallIncomingReceived,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallOutgoingRinging,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneTransfertCallOutgoingProgress,1,2000));
	linphone_core_accept_call(laure->lc,linphone_core_get_current_call(laure->lc));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallConnected,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallConnected,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneTransfertCallConnected,1,2000));

	/*terminate marie to pauline call*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,2000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	ms_list_free(lcs);
}

static void call_transfer_existing_call_outgoing_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new("./tester/laure_rc");

	MSList* lcs=ms_list_append(NULL,marie->lc);
	const MSList* calls;
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);

	LinphoneCall* marie_call_pauline;
	LinphoneCall* pauline_called_by_marie;
	LinphoneCall* marie_call_laure;
	LinphoneCall* laure_called_by_marie;

	/*marie call pauline*/
	CU_ASSERT_TRUE(call(marie,pauline));
	marie_call_pauline=linphone_core_get_current_call(marie->lc);
	pauline_called_by_marie=linphone_core_get_current_call(pauline->lc);
	/*marie pause pauline*/
	CU_ASSERT_TRUE(pause_call_1(marie,marie_call_pauline,pauline,pauline_called_by_marie));

	/*marie call laure*/
	CU_ASSERT_TRUE(call(marie,laure));
	marie_call_laure=linphone_core_get_current_call(marie->lc);
	laure_called_by_marie=linphone_core_get_current_call(laure->lc);
	/*marie pause pauline*/
	CU_ASSERT_TRUE(pause_call_1(marie,marie_call_laure,laure,laure_called_by_marie));

	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);
	reset_counters(&laure->stat);


	linphone_core_transfer_call_to_another(marie->lc,marie_call_pauline,marie_call_laure);
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallRefered,1,2000));

	/*pauline pausing marie*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallPausing,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallPaused,1,2000));
	/*pauline calling laure*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingProgress,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransferCallOutgoingInit,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallIncomingReceived,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransfertCallOutgoingProgress,1,2000));

	/*laure accept call*/
	for(calls=linphone_core_get_calls(laure->lc);calls!=NULL;calls=calls->next) {
		LinphoneCall* call = (LinphoneCall*)calls->data;
		if (linphone_call_get_state(call) == LinphoneCallIncomingReceived) {
			CU_ASSERT_EQUAL(linphone_call_get_replaced_call(call),laure_called_by_marie);
			linphone_core_accept_call(laure->lc,call);
			break;
		}
	}
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallConnected,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransfertCallConnected,1,2000));

	/*terminate marie to pauline/laure call*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,2,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,1,2000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	ms_list_free(lcs);
}


test_t call_tests[] = {
	{ "Early declined call", early_declined_call },
	{ "Cancelled call", cancelled_call },
	{ "Call with DNS timeout", call_with_dns_time_out },
	{ "Cancelled ringing call", cancelled_ringing_call },
	{ "Simple call", simple_call },
	{ "Early-media call", early_media_call },
	{ "Call terminated by caller", call_terminated_by_caller },
	{ "Call paused resumed", call_paused_resumed },
	{ "Call paused resumed from callee", call_paused_resumed_from_callee },
	{ "SRTP call", srtp_call },
	{ "Call with video added", call_with_video_added },
	{ "Simple conference", simple_conference },
	{ "Simple call transfer", simple_call_transfer },
	{ "Call transfer existing call outgoing call", call_transfer_existing_call_outgoing_call }
};

test_suite_t call_test_suite = {
	"Call",
	NULL,
	NULL,
	sizeof(call_tests) / sizeof(call_tests[0]),
	call_tests
};

