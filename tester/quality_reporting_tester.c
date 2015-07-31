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
#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"

/*avoid crash if x is NULL on libc versions <4.5.26 */
#define __strstr(x, y) ((x==NULL)?NULL:strstr(x,y))

void on_report_send_mandatory(const LinphoneCall *call, SalStreamType stream_type, const LinphoneContent *content){
	char * body = (char *)linphone_content_get_buffer(content);
	char * remote_metrics_start = __strstr(body, "RemoteMetrics:");
	reporting_session_report_t * report = call->log->reporting.reports[stream_type];
	MediaStream * ms;
	if (stream_type == LINPHONE_CALL_STATS_AUDIO){
		ms = (MediaStream*)call->audiostream;
	}else{
		ms = (MediaStream*)call->videostream;
	}
	BC_ASSERT_TRUE(
			__strstr(body, "VQIntervalReport\r\n")			== body ||
			__strstr(body, "VQSessionReport\r\n")			== body ||
			__strstr(body, "VQSessionReport: CallTerm\r\n") == body
	);

	BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "CallID:"));
	BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "LocalID:"));
	BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "RemoteID:"));
	BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "OrigID:"));
	BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "LocalGroup:"));
	BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "RemoteGroup:"));
	BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "LocalAddr:"));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "IP="));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "PORT="));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "SSRC="));
	BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "RemoteAddr:"));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "IP="));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "PORT="));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "SSRC="));
	BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "LocalMetrics:"));
	BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "Timestamps:"));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "START="));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "STOP="));

	BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "SessionDesc:"));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "PT="));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "PD="));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "SR="));

	/* We should have not reached RemoteMetrics section yet */
	BC_ASSERT_TRUE(!remote_metrics_start || body < remote_metrics_start);

	BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "DialogID:"));

	if (report->remote_metrics.rtcp_sr_count>0&&ms!=NULL&&ms->rc!=NULL){
		/* Hack: reset rtcp_sr_count to 0 because in case of interval reports, we need one RTCP SR by interval. */
		report->remote_metrics.rtcp_sr_count=0;
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "AdaptiveAlg:"));
	}
}

char * on_report_send_verify_metrics(const reporting_content_metrics_t *metrics, char * body){
	if (metrics->rtcp_xr_count){
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "SessionDesc:"));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "JitterBuffer:"));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "PacketLoss:"));
	}
	if (metrics->rtcp_sr_count+metrics->rtcp_xr_count>0){
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "Delay:"));
	}
	if (metrics->rtcp_xr_count){
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "QualityEst:"));
	}

	return body;
}

void on_report_send_with_rtcp_xr_local(const LinphoneCall *call, SalStreamType stream_type, const LinphoneContent *content){
	char * body = (char*)linphone_content_get_buffer(content);
	char * remote_metrics_start = __strstr(body, "RemoteMetrics:");
	reporting_session_report_t * report = call->log->reporting.reports[stream_type];
	on_report_send_mandatory(call,stream_type,content);
	BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "LocalMetrics:"));
	BC_ASSERT_TRUE(!remote_metrics_start || on_report_send_verify_metrics(&report->local_metrics,body) < remote_metrics_start);
}
void on_report_send_with_rtcp_xr_remote(const LinphoneCall *call, SalStreamType stream_type, const LinphoneContent *content){
	char * body = (char*)linphone_content_get_buffer(content);
	reporting_session_report_t * report = call->log->reporting.reports[stream_type];

	on_report_send_mandatory(call,stream_type,content);
	if (report->remote_metrics.rtcp_sr_count+report->remote_metrics.rtcp_xr_count>0){
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "RemoteMetrics:"));
		BC_ASSERT_PTR_NOT_NULL(body=__strstr(body, "Timestamps:"));
		on_report_send_verify_metrics(&report->remote_metrics,body);
	}
}
void on_report_send_with_rtcp_xr_both(const LinphoneCall *call, SalStreamType stream_type, const LinphoneContent *content){
	on_report_send_with_rtcp_xr_local(call,stream_type,content);
	on_report_send_with_rtcp_xr_remote(call,stream_type,content);
}

bool_t create_call_for_quality_reporting_tests(
		LinphoneCoreManager* marie,
		LinphoneCoreManager* pauline,
		LinphoneCall** call_marie,
		LinphoneCall** call_pauline,
		LinphoneCallParams * params_marie,
		LinphoneCallParams * params_pauline
		) {


	bool_t call_succeeded = call_with_params(marie,pauline,params_marie,params_pauline);
	BC_ASSERT_TRUE(call_succeeded);
	if (call_succeeded) {
		if (call_marie) {
			*call_marie = linphone_core_get_current_call(marie->lc);
			BC_ASSERT_PTR_NOT_NULL(*call_marie);
		}
		if (call_pauline) {
			*call_pauline = linphone_core_get_current_call(pauline->lc);
			BC_ASSERT_PTR_NOT_NULL(*call_pauline);
		}
	}
	return call_succeeded;
}

static void quality_reporting_not_used_without_config() {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_quality_reporting_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCall* call_marie = NULL;
	LinphoneCall* call_pauline = NULL;

	if (create_call_for_quality_reporting_tests(marie, pauline, &call_marie, &call_pauline, NULL, NULL))  {
		// marie has stats collection enabled but pauline has not
		BC_ASSERT_TRUE(linphone_proxy_config_quality_reporting_enabled(call_marie->dest_proxy));
		BC_ASSERT_FALSE(linphone_proxy_config_quality_reporting_enabled(call_pauline->dest_proxy));

		// this field should be already filled
		BC_ASSERT_PTR_NOT_NULL(call_marie->log->reporting.reports[0]->info.local_addr.ip);

		// but not this one since it is updated at the end of call
		BC_ASSERT_PTR_NULL(call_marie->log->reporting.reports[0]->dialog_id);
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void quality_reporting_not_sent_if_call_not_started() {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_quality_reporting_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCallLog* out_call_log;
	LinphoneCall* out_call;

	linphone_core_set_max_calls(pauline->lc,0);
	out_call = linphone_core_invite(marie->lc,"pauline");
	BC_ASSERT_PTR_NOT_NULL(out_call);
	if(out_call == NULL) goto end;
	linphone_call_ref(out_call);

	BC_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallError,1, 10000));
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallError,1, int, "%d");

	if (ms_list_size(linphone_core_get_call_logs(marie->lc))>0) {
		out_call_log=(LinphoneCallLog*)(linphone_core_get_call_logs(marie->lc)->data);
		BC_ASSERT_PTR_NOT_NULL(out_call_log);
		BC_ASSERT_EQUAL(linphone_call_log_get_status(out_call_log),LinphoneCallAborted, int, "%d");
	}
	linphone_call_unref(out_call);

	// wait a few time...
	wait_for_until(marie->lc,NULL,NULL,0,1000);

	// since the callee was busy, there should be no publish to do
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishProgress,0, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishOk,0, int, "%d");

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void quality_reporting_not_sent_if_low_bandwidth() {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_quality_reporting_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCallParams* marie_params;

	marie_params=linphone_core_create_default_call_parameters(marie->lc);
	linphone_call_params_enable_low_bandwidth(marie_params,TRUE);

	if (create_call_for_quality_reporting_tests(marie, pauline, NULL, NULL, marie_params, NULL)) {
		linphone_core_terminate_all_calls(marie->lc);

		BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishProgress,0, int, "%d");
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishOk,0, int, "%d");
	}
	linphone_call_params_destroy(marie_params);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void on_report_send_remove_fields(const LinphoneCall *call, SalStreamType stream_type, const LinphoneContent *content){
	char *body = (char*)linphone_content_get_buffer(content);
	/*corrupt start of the report*/
	strncpy(body, "corrupted report is corrupted", strlen("corrupted report is corrupted"));
}

static void quality_reporting_invalid_report() {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_quality_reporting_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCall* call_marie = NULL;
	LinphoneCall* call_pauline = NULL;

	if (create_call_for_quality_reporting_tests(marie, pauline, &call_marie, &call_pauline, NULL, NULL)) {
		linphone_reporting_set_on_report_send(call_marie, on_report_send_remove_fields);

		linphone_core_terminate_all_calls(marie->lc);

		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishProgress,1));
		BC_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishError,1,3000));
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishError,1, int, "%d");
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishOk,0, int, "%d");
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void quality_reporting_at_call_termination() {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_quality_reporting_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc_rtcp_xr");
	LinphoneCall* call_marie = NULL;
	LinphoneCall* call_pauline = NULL;

	if (create_call_for_quality_reporting_tests(marie, pauline, &call_marie, &call_pauline, NULL, NULL)) {
		linphone_reporting_set_on_report_send(call_marie, on_report_send_with_rtcp_xr_remote);

		linphone_core_terminate_all_calls(marie->lc);

		// now dialog id should be filled
		BC_ASSERT_PTR_NOT_NULL(call_marie->log->reporting.reports[0]->dialog_id);

		BC_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallReleased,1, 10000));
		BC_ASSERT_TRUE(wait_for_until(pauline->lc,NULL,&pauline->stat.number_of_LinphoneCallReleased,1, 10000));

		BC_ASSERT_PTR_NULL(linphone_core_get_current_call(marie->lc));
		BC_ASSERT_PTR_NULL(linphone_core_get_current_call(pauline->lc));

		// PUBLISH submission to the collector should be ok
		BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphonePublishProgress,1));
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishProgress,1, int, "%d");
		BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphonePublishOk,1));
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void quality_reporting_interval_report() {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc_rtcp_xr");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc_rtcp_xr");
	LinphoneCall* call_marie = NULL;
	LinphoneCall* call_pauline = NULL;

	if (create_call_for_quality_reporting_tests(marie, pauline, &call_marie, &call_pauline, NULL, NULL))  {
		linphone_reporting_set_on_report_send(call_marie, on_report_send_mandatory);
		linphone_proxy_config_set_quality_reporting_interval(call_marie->dest_proxy, 1);

		BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(marie->lc));
		BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(pauline->lc));

		// PUBLISH submission to the collector should be ok
		BC_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishProgress,1,60000));
		BC_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishOk,1,60000));
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void quality_reporting_session_report_if_video_stopped() {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc_rtcp_xr");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCall* call_pauline = NULL;
	LinphoneCall* call_marie = NULL;
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

	if (create_call_for_quality_reporting_tests(marie, pauline, &call_marie, &call_pauline, marie_params, pauline_params)) {
		linphone_reporting_set_on_report_send(call_marie, on_report_send_with_rtcp_xr_local);

		BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishProgress,0, int, "%d");
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishOk,0, int, "%d");

		BC_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,NULL,0,3000));
		BC_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(call_pauline)));

		/*remove video*/
		linphone_call_params_enable_video(pauline_params,FALSE);
		linphone_core_update_call(pauline->lc,call_pauline,pauline_params);

		BC_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishProgress,1,10000));
		BC_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishOk,1,10000));

		BC_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(call_pauline)));

		linphone_core_terminate_all_calls(marie->lc);

		BC_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishProgress,2,5000));
		BC_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePublishOk,2,5000));
	}
	linphone_call_params_destroy(marie_params);
	linphone_call_params_destroy(pauline_params);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void publish_report_with_route_state_changed(LinphoneCore *lc, LinphoneEvent *ev, LinphonePublishState state){
	stats* counters = get_stats(lc);
	const LinphoneAddress* from_addr = linphone_event_get_from(ev);
	char* from = linphone_address_as_string(from_addr);
	ms_message("Publish state [%s] from [%s]",linphone_publish_state_to_string(state),from);
	ms_free(from);
	switch(state){
		case LinphonePublishProgress:
			BC_ASSERT_STRING_EQUAL(linphone_address_as_string(linphone_event_get_resource(ev)), linphone_proxy_config_get_quality_reporting_collector(linphone_core_get_default_proxy_config(lc)));
			counters->number_of_LinphonePublishProgress++;
			break;
		case LinphonePublishOk:
			counters->number_of_LinphonePublishOk++;
			break;
		case LinphonePublishError: counters->number_of_LinphonePublishError++; break;
		case LinphonePublishExpiring: counters->number_of_LinphonePublishExpiring++; break;
		case LinphonePublishCleared: counters->number_of_LinphonePublishCleared++;break;
		default:
		break;
	}

}

static void quality_reporting_sent_using_custom_route() {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_quality_reporting_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCall* call_marie = NULL;
	LinphoneCall* call_pauline = NULL;

	marie->lc->current_vtable->publish_state_changed = publish_report_with_route_state_changed;
	//INVALID collector: sip.linphone.org do not collect reports, so it will throw a 404 Not Found error
	linphone_proxy_config_set_quality_reporting_collector(linphone_core_get_default_proxy_config(marie->lc), "sip:sip.linphone.org");

	if (create_call_for_quality_reporting_tests(marie, pauline, &call_marie, &call_pauline, NULL, NULL)) {
		linphone_core_terminate_all_calls(marie->lc);
		BC_ASSERT_TRUE(wait_for_until(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallReleased,1, 10000));
		BC_ASSERT_TRUE(wait_for_until(pauline->lc,NULL,&pauline->stat.number_of_LinphoneCallReleased,1, 10000));

		// PUBLISH submission to the collector should be ERROR since route is not valid
		BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphonePublishProgress,1));
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishProgress,1, int, "%d");
		BC_ASSERT_TRUE(wait_for_until(marie->lc,NULL,&marie->stat.number_of_LinphonePublishError,1,10000));
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphonePublishOk,0,int, "%d");
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

test_t quality_reporting_tests[] = {
	{ "Not used if no config", quality_reporting_not_used_without_config},
	{ "Call term session report not sent if call did not start", quality_reporting_not_sent_if_call_not_started},
	{ "Call term session report not sent if low bandwidth", quality_reporting_not_sent_if_low_bandwidth},
	{ "Call term session report invalid if missing mandatory fields", quality_reporting_invalid_report},
	{ "Call term session report sent if call ended normally", quality_reporting_at_call_termination},
	{ "Interval report if interval is configured", quality_reporting_interval_report},
	{ "Session report sent if video stopped during call", quality_reporting_session_report_if_video_stopped},
	{ "Sent using custom route", quality_reporting_sent_using_custom_route},
};

test_suite_t quality_reporting_test_suite = {
	"QualityReporting",
	liblinphone_tester_setup,
	NULL,
	sizeof(quality_reporting_tests) / sizeof(quality_reporting_tests[0]),
	quality_reporting_tests
};
