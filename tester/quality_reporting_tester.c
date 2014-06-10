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

void create_call_for_quality_reporting_tests(
		LinphoneCoreManager* marie,
		LinphoneCoreManager* pauline,
		LinphoneCall** call_marie,
		LinphoneCall** call_pauline) {
	CU_ASSERT_TRUE(call(pauline,marie));
	*call_marie = linphone_core_get_current_call(marie->lc);
	*call_pauline = linphone_core_get_current_call(pauline->lc);
	CU_ASSERT_PTR_NOT_NULL(*call_marie);
	CU_ASSERT_PTR_NOT_NULL(*call_pauline);
}

static void quality_reporting_not_used_without_config() {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCall* call_marie = NULL;
	LinphoneCall* call_pauline = NULL;

	create_call_for_quality_reporting_tests(marie, pauline, &call_marie, &call_pauline);

	// marie has stats collection enabled since pauline has not
	CU_ASSERT_TRUE(linphone_proxy_config_quality_reporting_enabled(call_marie->dest_proxy));
	CU_ASSERT_FALSE(linphone_proxy_config_quality_reporting_enabled(call_pauline->dest_proxy));

	CU_ASSERT_EQUAL(strcmp("sip:collector@sip.example.org",
		linphone_proxy_config_get_quality_reporting_collector(call_marie->dest_proxy)), 0);

	// this field should be already filled
	CU_ASSERT_PTR_NOT_NULL(call_marie->log->reporting.reports[0]->info.local_addr.ip);

	// but not this one since it is updated at the end of call
	CU_ASSERT_PTR_NULL(call_marie->log->reporting.reports[0]->dialog_id);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void quality_reporting_not_sent_if_call_not_started() {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCallLog* out_call_log;
	LinphoneCall* out_call;

	linphone_core_set_max_calls(pauline->lc,0);
	out_call = linphone_core_invite(marie->lc,"pauline");
	linphone_call_ref(out_call);

	CU_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallError,1, 10000));
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallError,1);

	if (ms_list_size(linphone_core_get_call_logs(marie->lc))>0) {
		out_call_log=(LinphoneCallLog*)(linphone_core_get_call_logs(marie->lc)->data);
		CU_ASSERT_PTR_NOT_NULL(out_call_log);
		CU_ASSERT_EQUAL(linphone_call_log_get_status(out_call_log),LinphoneCallAborted);
	}
	linphone_call_unref(out_call);

	// wait a few time...
	wait_for_until(marie->lc,NULL,NULL,0,1000);

	// since the callee was busy, there should be no publish to do
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishProgress,0);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishOk,0);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void quality_reporting_at_call_termination() {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCall* call_marie = NULL;
	LinphoneCall* call_pauline = NULL;

	create_call_for_quality_reporting_tests(marie, pauline, &call_marie, &call_pauline);

	linphone_core_terminate_all_calls(marie->lc);

	// now dialog id should be filled
	CU_ASSERT_PTR_NOT_NULL(call_marie->log->reporting.reports[0]->dialog_id);

	CU_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallReleased,1, 10000));
	CU_ASSERT_TRUE(wait_for_until(pauline->lc,NULL,&pauline->stat.number_of_LinphoneCallReleased,1, 10000));

	CU_ASSERT_PTR_NULL(linphone_core_get_current_call(marie->lc));
	CU_ASSERT_PTR_NULL(linphone_core_get_current_call(pauline->lc));


	// PUBLISH submission to the collector should be ok
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphonePublishProgress,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphonePublishOk,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void quality_reporting_interval_report() {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCall* call_marie = NULL;
	LinphoneCall* call_pauline = NULL;

	create_call_for_quality_reporting_tests(marie, pauline, &call_marie, &call_pauline);
	linphone_proxy_config_set_quality_reporting_interval(call_marie->dest_proxy, 3);

	CU_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(marie->lc));
	CU_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(pauline->lc));

	// PUBLISH submission to the collector should be ok
	CU_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishProgress,3,25000));
	CU_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishOk,3,25000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void quality_reporting_session_report_if_video_stopped() {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	/*LinphoneCall* call_marie = NULL;*/
	LinphoneCall* call_pauline = NULL;
	LinphoneCallParams* pauline_params;
	LinphoneCallParams* marie_params;

	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, FALSE);
	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, FALSE);
	marie_params=linphone_core_create_default_call_parameters(marie->lc);
	linphone_call_params_enable_video(marie_params,TRUE);
	pauline_params=linphone_core_create_default_call_parameters(pauline->lc);
	linphone_call_params_enable_video(pauline_params,TRUE);
	CU_ASSERT_TRUE(call_with_params(pauline,marie,pauline_params,marie_params));
	/*call_marie=linphone_core_get_current_call(marie->lc);*/
	call_pauline=linphone_core_get_current_call(pauline->lc);

	CU_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishProgress,0);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishOk,0);

	CU_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,NULL,0,3000));
	CU_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(call_pauline)));

	/*remove video*/
	linphone_call_params_enable_video(pauline_params,FALSE);
	linphone_core_update_call(pauline->lc,call_pauline,pauline_params);

	CU_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishProgress,1,5000));
	CU_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishOk,1,5000));

	CU_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(call_pauline)));

	linphone_core_terminate_all_calls(marie->lc);

	CU_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishProgress,2,5000));
	CU_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishOk,2,5000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


test_t quality_reporting_tests[] = {
	{ "Not used if no config", quality_reporting_not_used_without_config},
	{ "Call term session report not sent if call did not start", quality_reporting_not_sent_if_call_not_started},
	{ "Call term session report sent if call ended normally", quality_reporting_at_call_termination},
	{ "Interval report if interval is configured", quality_reporting_interval_report},
	{ "Session report sent if video stopped during call", quality_reporting_session_report_if_video_stopped},
};

test_suite_t quality_reporting_test_suite = {
	"QualityReporting",
	NULL,
	NULL,
	sizeof(quality_reporting_tests) / sizeof(quality_reporting_tests[0]),
	quality_reporting_tests
};
