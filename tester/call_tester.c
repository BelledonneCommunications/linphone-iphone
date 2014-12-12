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
#include <sys/types.h>
#include <sys/stat.h>
#include "CUnit/Basic.h"
#include "linphonecore.h"
#include "lpconfig.h"
#include "private.h"
#include "liblinphone_tester.h"
#include "mediastreamer2/dsptools.h"

#ifdef WIN32
#define unlink _unlink
#endif

static void srtp_call(void);
static void call_base(LinphoneMediaEncryption mode, bool_t enable_video,bool_t enable_relay,LinphoneFirewallPolicy policy);
static void disable_all_audio_codecs_except_one(LinphoneCore *lc, const char *mime, int rate);
static char *create_filepath(const char *dir, const char *filename, const char *ext);

// prototype definition for call_recording()
#ifdef ANDROID
#ifdef HAVE_OPENH264
extern void libmsopenh264_init(void);
#endif
#endif

void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg){
	char* to=linphone_address_as_string(linphone_call_get_call_log(call)->to);
	char* from=linphone_address_as_string(linphone_call_get_call_log(call)->from);
	stats* counters;
	ms_message(" %s call from [%s] to [%s], new state is [%s]"	,linphone_call_get_call_log(call)->dir==LinphoneCallIncoming?"Incoming":"Outgoing"
																,from
																,to
																,linphone_call_state_to_string(cstate));
	ms_free(to);
	ms_free(from);
	counters = get_stats(lc);
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
	case LinphoneCallEarlyUpdating: counters->number_of_LinphoneCallEarlyUpdating++;break;
	case LinphoneCallEarlyUpdatedByRemote: counters->number_of_LinphoneCallEarlyUpdatedByRemote++;break;
	default:
		CU_FAIL("unexpected event");break;
	}
}

void call_stats_updated(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallStats *lstats) {
	stats* counters = get_stats(lc);
	if (lstats->updated == LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE) {
		counters->number_of_rtcp_received++;
	} else if (lstats->updated == LINPHONE_CALL_STATS_SENT_RTCP_UPDATE) {
		counters->number_of_rtcp_sent++;
	}
}

void linphone_call_encryption_changed(LinphoneCore *lc, LinphoneCall *call, bool_t on, const char *authentication_token) {
	char* to=linphone_address_as_string(linphone_call_get_call_log(call)->to);
	char* from=linphone_address_as_string(linphone_call_get_call_log(call)->from);
	stats* counters;
	ms_message(" %s call from [%s] to [%s], is now [%s]",linphone_call_get_call_log(call)->dir==LinphoneCallIncoming?"Incoming":"Outgoing"
														,from
														,to
														,(on?"encrypted":"unencrypted"));
	ms_free(to);
	ms_free(from);
	counters = get_stats(lc);
	if (on)
		counters->number_of_LinphoneCallEncryptedOn++;
	else
		counters->number_of_LinphoneCallEncryptedOff++;
}
void linphone_transfer_state_changed(LinphoneCore *lc, LinphoneCall *transfered, LinphoneCallState new_call_state) {
	char* to=linphone_address_as_string(linphone_call_get_call_log(transfered)->to);
	char* from=linphone_address_as_string(linphone_call_get_call_log(transfered)->from);
	stats* counters;
	ms_message("Transferred call from [%s] to [%s], new state is [%s]",from,to,linphone_call_state_to_string(new_call_state));
	ms_free(to);
	ms_free(from);

	counters = get_stats(lc);
	switch (new_call_state) {
	case LinphoneCallOutgoingInit :counters->number_of_LinphoneTransferCallOutgoingInit++;break;
	case LinphoneCallOutgoingProgress :counters->number_of_LinphoneTransferCallOutgoingProgress++;break;
	case LinphoneCallOutgoingRinging :counters->number_of_LinphoneTransferCallOutgoingRinging++;break;
	case LinphoneCallOutgoingEarlyMedia :counters->number_of_LinphoneTransferCallOutgoingEarlyMedia++;break;
	case LinphoneCallConnected :counters->number_of_LinphoneTransferCallConnected++;break;
	case LinphoneCallStreamsRunning :counters->number_of_LinphoneTransferCallStreamsRunning++;break;
	case LinphoneCallError :counters->number_of_LinphoneTransferCallError++;break;
	default:
		CU_FAIL("unexpected event");break;
	}
}

#ifdef VIDEO_ENABLED
static void linphone_call_cb(LinphoneCall *call,void * user_data) {
	char* to=linphone_address_as_string(linphone_call_get_call_log(call)->to);
	char* from=linphone_address_as_string(linphone_call_get_call_log(call)->from);
	stats* counters;
	LinphoneCore* lc=(LinphoneCore*)user_data;
	ms_message("call from [%s] to [%s] receive iFrame",from,to);
	ms_free(to);
	ms_free(from);
	counters = (stats*)get_stats(lc);
	counters->number_of_IframeDecoded++;
}
#endif

void liblinphone_tester_check_rtcp(LinphoneCoreManager* caller, LinphoneCoreManager* callee) {
	LinphoneCall *c1,*c2;
	int dummy=0;
	MSTimeSpec ts;

	c1=linphone_core_get_current_call(caller->lc);
	c2=linphone_core_get_current_call(callee->lc);

	CU_ASSERT_PTR_NOT_NULL(c1);
	CU_ASSERT_PTR_NOT_NULL(c2);

	if (!c1 || !c2) return;
	linphone_call_ref(c1);
	linphone_call_ref(c2);

	liblinphone_tester_clock_start(&ts);
	do {
		if (linphone_call_get_audio_stats(c1)->round_trip_delay >0.0
				&& linphone_call_get_audio_stats(c2)->round_trip_delay >0.0
				&& (!linphone_call_log_video_enabled(linphone_call_get_call_log(c1)) || linphone_call_get_video_stats(c1)->round_trip_delay>0.0)
				&& (!linphone_call_log_video_enabled(linphone_call_get_call_log(c2))  || linphone_call_get_video_stats(c2)->round_trip_delay>0.0)) {
			break;

		}
		wait_for_until(caller->lc,callee->lc,&dummy,1,500); /*just to sleep while iterating*/
	}while (!liblinphone_tester_clock_elapsed(&ts,12000));
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(c1)->round_trip_delay>0.0);
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(c2)->round_trip_delay>0.0);
	if (linphone_call_log_video_enabled(linphone_call_get_call_log(c1))) {
		CU_ASSERT_TRUE(linphone_call_get_video_stats(c1)->round_trip_delay>0.0);
	}
	if (linphone_call_log_video_enabled(linphone_call_get_call_log(c2))) {
		CU_ASSERT_TRUE(linphone_call_get_video_stats(c2)->round_trip_delay>0.0);
	}
	linphone_call_unref(c1);
	linphone_call_unref(c2);
}

bool_t call_with_params2(LinphoneCoreManager* caller_mgr
						,LinphoneCoreManager* callee_mgr
						, const LinphoneCallTestParams *caller_test_params
						, const LinphoneCallTestParams *callee_test_params
						, bool_t build_callee_params) {
	int retry=0;
	stats initial_caller=caller_mgr->stat;
	stats initial_callee=callee_mgr->stat;
	bool_t result=FALSE;
	char hellopath[256];
	LinphoneCallParams *caller_params = caller_test_params->base;
	LinphoneCallParams *callee_params = callee_test_params->base;
	bool_t did_received_call;

	sal_default_enable_sdp_removal(caller_mgr->lc->sal, caller_test_params->sdp_removal);
	sal_default_enable_sdp_removal(callee_mgr->lc->sal, callee_test_params->sdp_removal);

	/*use playfile for callee to avoid locking on capture card*/
	linphone_core_use_files (callee_mgr->lc,TRUE);
	snprintf(hellopath,sizeof(hellopath), "%s/sounds/hello8000.wav", liblinphone_tester_file_prefix);
	linphone_core_set_play_file(callee_mgr->lc,hellopath);
	if (!caller_params){
		CU_ASSERT_PTR_NOT_NULL(linphone_core_invite_address(caller_mgr->lc,callee_mgr->identity));
	}else{
		CU_ASSERT_PTR_NOT_NULL(linphone_core_invite_address_with_params(caller_mgr->lc,callee_mgr->identity,caller_params));
	}

	did_received_call = wait_for(callee_mgr->lc
				,caller_mgr->lc
				,&callee_mgr->stat.number_of_LinphoneCallIncomingReceived
				,initial_callee.number_of_LinphoneCallIncomingReceived+1);
	CU_ASSERT_EQUAL(did_received_call, !callee_test_params->sdp_removal);

	sal_default_enable_sdp_removal(caller_mgr->lc->sal, FALSE);
	sal_default_enable_sdp_removal(callee_mgr->lc->sal, FALSE);

	if (!did_received_call) return 0;


	CU_ASSERT_TRUE(linphone_core_inc_invite_pending(callee_mgr->lc));
	CU_ASSERT_EQUAL(caller_mgr->stat.number_of_LinphoneCallOutgoingProgress,initial_caller.number_of_LinphoneCallOutgoingProgress+1);


	while (caller_mgr->stat.number_of_LinphoneCallOutgoingRinging!=(initial_caller.number_of_LinphoneCallOutgoingRinging + 1)
			&& caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia!=(initial_caller.number_of_LinphoneCallOutgoingEarlyMedia +1)
			&& retry++ <20) {
			linphone_core_iterate(caller_mgr->lc);
			linphone_core_iterate(callee_mgr->lc);
			ms_usleep(100000);
	}


	CU_ASSERT_TRUE((caller_mgr->stat.number_of_LinphoneCallOutgoingRinging==initial_caller.number_of_LinphoneCallOutgoingRinging+1)
							|(caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia==initial_caller.number_of_LinphoneCallOutgoingEarlyMedia+1));


	CU_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call_remote_address(callee_mgr->lc));
	if(!linphone_core_get_current_call(caller_mgr->lc) || !linphone_core_get_current_call(callee_mgr->lc) || !linphone_core_get_current_call_remote_address(callee_mgr->lc)) {
		return 0;
	} else {
		LinphoneAddress* callee_from=linphone_address_clone(caller_mgr->identity);
		linphone_address_set_port(callee_from,0); /*remove port because port is never present in from header*/

		if (linphone_call_params_get_privacy(linphone_call_get_current_params(linphone_core_get_current_call(caller_mgr->lc))) == LinphonePrivacyNone) {
			/*don't check in case of p asserted id*/
			if (!lp_config_get_int(callee_mgr->lc->config,"sip","call_logs_use_asserted_id_instead_of_from",0))
				CU_ASSERT_TRUE(linphone_address_weak_equal(callee_from,linphone_core_get_current_call_remote_address(callee_mgr->lc)));
		} else {
			CU_ASSERT_FALSE(linphone_address_weak_equal(callee_from,linphone_core_get_current_call_remote_address(callee_mgr->lc)));
		}
		linphone_address_destroy(callee_from);
	}
	if (callee_params){
		linphone_core_accept_call_with_params(callee_mgr->lc,linphone_core_get_current_call(callee_mgr->lc),callee_params);
	}else if (build_callee_params){
		LinphoneCallParams *default_params=linphone_core_create_call_params(callee_mgr->lc,linphone_core_get_current_call(callee_mgr->lc));
		ms_error("Created default call params with video=%i", linphone_call_params_video_enabled(default_params));
		linphone_core_accept_call_with_params(callee_mgr->lc,linphone_core_get_current_call(callee_mgr->lc),default_params);
		linphone_call_params_destroy(default_params);
	}else{
		linphone_core_accept_call(callee_mgr->lc,linphone_core_get_current_call(callee_mgr->lc));
	}

	CU_ASSERT_TRUE(wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallConnected,initial_callee.number_of_LinphoneCallConnected+1));
	CU_ASSERT_TRUE(wait_for(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallConnected,initial_callee.number_of_LinphoneCallConnected+1));
	/*just to sleep*/
	result = wait_for(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallStreamsRunning,initial_caller.number_of_LinphoneCallStreamsRunning+1)
			&&
			wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallStreamsRunning,initial_callee.number_of_LinphoneCallStreamsRunning+1);

	if (linphone_core_get_media_encryption(caller_mgr->lc) != LinphoneMediaEncryptionNone
		&& linphone_core_get_media_encryption(callee_mgr->lc) != LinphoneMediaEncryptionNone) {
		/*wait for encryption to be on, in case of zrtp, it can take a few seconds*/
		if (linphone_core_get_media_encryption(caller_mgr->lc) == LinphoneMediaEncryptionZRTP)
			wait_for(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallEncryptedOn,initial_caller.number_of_LinphoneCallEncryptedOn+1);
		if (linphone_core_get_media_encryption(callee_mgr->lc) == LinphoneMediaEncryptionZRTP)
			wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallEncryptedOn,initial_callee.number_of_LinphoneCallEncryptedOn+1);
		{
		const LinphoneCallParams* call_param = linphone_call_get_current_params(linphone_core_get_current_call(callee_mgr->lc));
		CU_ASSERT_EQUAL(linphone_call_params_get_media_encryption(call_param),linphone_core_get_media_encryption(caller_mgr->lc));
		call_param = linphone_call_get_current_params(linphone_core_get_current_call(caller_mgr->lc));
		CU_ASSERT_EQUAL(linphone_call_params_get_media_encryption(call_param),linphone_core_get_media_encryption(callee_mgr->lc));
		}
	}
	return result;
}

bool_t call_with_params(LinphoneCoreManager* caller_mgr
						,LinphoneCoreManager* callee_mgr
						,const LinphoneCallParams *caller_params
						,const LinphoneCallParams *callee_params){
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params =  {0};
	caller_test_params.base = (LinphoneCallParams*)caller_params;
	callee_test_params.base = (LinphoneCallParams*)caller_params;
	return call_with_params2(caller_mgr,callee_mgr,&caller_test_params,&callee_test_params,FALSE);
}

bool_t call_with_test_params(LinphoneCoreManager* caller_mgr
				,LinphoneCoreManager* callee_mgr
				,const LinphoneCallTestParams *caller_test_params
				,const LinphoneCallTestParams *callee_test_params){
	return call_with_params2(caller_mgr,callee_mgr,caller_test_params,callee_test_params,FALSE);
}

bool_t call_with_caller_params(LinphoneCoreManager* caller_mgr,LinphoneCoreManager* callee_mgr, const LinphoneCallParams *params) {
	return call_with_params(caller_mgr,callee_mgr,params,NULL);
}

bool_t call(LinphoneCoreManager* caller_mgr,LinphoneCoreManager* callee_mgr){
	return call_with_params(caller_mgr,callee_mgr,NULL,NULL);
}

void end_call(LinphoneCoreManager *m1, LinphoneCoreManager *m2){
	linphone_core_terminate_all_calls(m1->lc);
	CU_ASSERT_TRUE(wait_for(m1->lc,m2->lc,&m1->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(m1->lc,m2->lc,&m2->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(m1->lc,m2->lc,&m1->stat.number_of_LinphoneCallReleased,1));
	CU_ASSERT_TRUE(wait_for(m1->lc,m2->lc,&m2->stat.number_of_LinphoneCallReleased,1));
}

static void simple_call(void) {
	int begin;
	int leaked_objects;
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	const LinphoneAddress *from;
	LinphoneCall *pauline_call;
	LinphoneProxyConfig* marie_cfg;
	const char* marie_id = NULL;

	belle_sip_object_enable_leak_detector(TRUE);
	begin=belle_sip_object_get_object_count();

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new( "pauline_rc");
	
	/* with the account manager, we might lose the identity */
	marie_cfg = linphone_core_get_default_proxy_config(marie->lc);
	marie_id = linphone_proxy_config_get_identity(marie_cfg);
	{
		LinphoneAddress* marie_addr = linphone_address_new(marie_id);
		char* marie_tmp_id = NULL;
		linphone_address_set_display_name(marie_addr, "Super Marie");
		marie_tmp_id = linphone_address_as_string(marie_addr);

		linphone_proxy_config_edit(marie_cfg);
		linphone_proxy_config_set_identity(marie_cfg,marie_tmp_id);
		linphone_proxy_config_done(marie_cfg);

		ms_free(marie_tmp_id);
		linphone_address_unref(marie_addr);
	}

	CU_ASSERT_TRUE(call(marie,pauline));
	pauline_call=linphone_core_get_current_call(pauline->lc);
	CU_ASSERT_PTR_NOT_NULL(pauline_call);
	/*check that display name is correctly propagated in From */
	if (pauline_call){
		from=linphone_call_get_remote_address(linphone_core_get_current_call(pauline->lc));
		CU_ASSERT_PTR_NOT_NULL(from);
		if (from){
			const char *dname=linphone_address_get_display_name(from);
			CU_ASSERT_PTR_NOT_NULL(dname);
			if (dname){
				CU_ASSERT_STRING_EQUAL(dname, "Super Marie");
			}
		}
	}


	liblinphone_tester_check_rtcp(marie,pauline);
	end_call(marie,pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);

	leaked_objects=belle_sip_object_get_object_count()-begin;
	CU_ASSERT_TRUE(leaked_objects==0);
	if (leaked_objects>0){
		belle_sip_object_dump_active_objects();
	}
}

static void direct_call_over_ipv6(){
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;

	if (liblinphone_tester_ipv6_available()){
		LCSipTransports pauline_transports;
		LinphoneAddress* pauline_dest = linphone_address_new("sip:[::1];transport=tcp");
		char hellopath[256];
		marie = linphone_core_manager_new( "marie_rc");
		pauline = linphone_core_manager_new( "pauline_tcp_rc");

		/*use playfile for callee to avoid locking on capture card*/
		snprintf(hellopath,sizeof(hellopath), "%s/sounds/hello8000.wav", liblinphone_tester_file_prefix);
		linphone_core_set_play_file(pauline->lc,hellopath);
		linphone_core_use_files (pauline->lc,TRUE);

		linphone_core_enable_ipv6(marie->lc,TRUE);
		linphone_core_enable_ipv6(pauline->lc,TRUE);
		linphone_core_set_default_proxy_config(marie->lc,NULL);
		/*wait for register in v6 mode, however sip2.linphone.org has an ipv6 address but doesn't listen to it*/
#if 0
		CU_ASSERT_TRUE(wait_for_until(pauline->lc, NULL, &pauline->stat.number_of_LinphoneRegistrationOk, 2, 2000));
		CU_ASSERT_TRUE(wait_for_until(pauline->lc, NULL, &marie->stat.number_of_LinphoneRegistrationOk, 2, 2000));
#endif

		linphone_core_get_sip_transports_used(pauline->lc,&pauline_transports);
		linphone_address_set_port(pauline_dest,pauline_transports.tcp_port);
		linphone_core_invite_address(marie->lc,pauline_dest);

		CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallOutgoingRinging,1));
		CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallIncomingReceived,1));
		linphone_core_accept_call(pauline->lc,linphone_core_get_current_call(pauline->lc));
		CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,1));
		CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,1));

		liblinphone_tester_check_rtcp(marie,pauline);
		end_call(marie,pauline);
		linphone_core_manager_destroy(marie);
		linphone_core_manager_destroy(pauline);
		linphone_address_destroy(pauline_dest);
	}else ms_warning("Test skipped, no ipv6 available");
}

static void call_outbound_with_multiple_proxy() {
	LinphoneCoreManager* pauline = linphone_core_manager_new2( "pauline_rc", FALSE);
	LinphoneCoreManager* marie   = linphone_core_manager_new2( "marie_rc", FALSE);

	LinphoneProxyConfig* lpc = NULL;
	LinphoneProxyConfig* registered_lpc = linphone_proxy_config_new();

	linphone_core_get_default_proxy(marie->lc, &lpc);
	linphone_core_set_default_proxy(marie->lc,NULL);

	CU_ASSERT_FATAL(lpc != NULL);
	CU_ASSERT_FATAL(registered_lpc != NULL);

	// create new LPC that will successfully register
	linphone_proxy_config_set_identity(registered_lpc, linphone_proxy_config_get_identity(lpc));
	linphone_proxy_config_set_server_addr(registered_lpc, linphone_proxy_config_get_addr(lpc));
	linphone_proxy_config_set_route(registered_lpc, linphone_proxy_config_get_route(lpc));
	linphone_proxy_config_enable_register(registered_lpc, TRUE);

	linphone_core_add_proxy_config(marie->lc, registered_lpc);

	// set first LPC to unreacheable proxy addr
	linphone_proxy_config_edit(lpc);
	linphone_proxy_config_set_server_addr(lpc,"12.13.14.15:5223;transport=udp");
	linphone_proxy_config_set_route(lpc, "12.13.14.15:5223;transport=udp;lr");
	linphone_proxy_config_done(lpc);

	CU_ASSERT_TRUE(wait_for_until(pauline->lc, NULL, &pauline->stat.number_of_LinphoneRegistrationOk, 1, 2000));

	CU_ASSERT_TRUE(wait_for_until(marie->lc, NULL, &marie->stat.number_of_LinphoneRegistrationProgress, 2, 200));
	CU_ASSERT_TRUE(wait_for_until(marie->lc, NULL, &marie->stat.number_of_LinphoneRegistrationOk, 1, 2000));

	// calling marie should go through the second proxy config
	CU_ASSERT_TRUE(call(marie, pauline));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#if 0 /* TODO: activate test when the implementation is ready */
static void multiple_answers_call() {
	/* Scenario is this: pauline calls marie, which is registered 2 times.
	   Both linphones answer at the same time, and only one should get the
	   call running, the other should be terminated */
	char ringbackpath[256];
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc" );
	LinphoneCoreManager* marie1  = linphone_core_manager_new( "marie_rc" );
	LinphoneCoreManager* marie2  = linphone_core_manager_new( "marie_rc" );

	LinphoneCall* call1, *call2;

	MSList* lcs = ms_list_append(NULL,pauline->lc);
	lcs = ms_list_append(lcs,marie1->lc);
	lcs = ms_list_append(lcs,marie2->lc);

	linphone_core_use_files(pauline->lc, TRUE);
	linphone_core_use_files(marie1->lc, TRUE);
	linphone_core_use_files(marie2->lc, TRUE);

	snprintf(ringbackpath,sizeof(ringbackpath), "%s/sounds/hello8000.wav" /*use hello because rinback is too short*/, liblinphone_tester_file_prefix);

	CU_ASSERT_TRUE(wait_for_until(pauline->lc, NULL, &pauline->stat.number_of_LinphoneRegistrationOk, 1, 2000));

	CU_ASSERT_PTR_NOT_NULL( linphone_core_invite_address(pauline->lc, marie1->identity ) );

	CU_ASSERT_TRUE(wait_for_list(lcs,&marie1->stat.number_of_LinphoneCallIncomingReceived, 1, 2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived, 1, 2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingProgress, 1, 2000));

	// marie 1 and 2 answer at the same time
	call1 = linphone_core_get_current_call(marie1->lc);
	call2 = linphone_core_get_current_call(marie2->lc);

	CU_ASSERT_PTR_NOT_NULL_FATAL(call1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(call2);

	CU_ASSERT_EQUAL( linphone_core_accept_call(marie1->lc, call1), 0);
	CU_ASSERT_EQUAL( linphone_core_accept_call(marie2->lc, call2), 0);

	CU_ASSERT_TRUE( wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallStreamsRunning, 1, 2000) );
	CU_ASSERT_TRUE( wait_for_list(lcs, &marie1->stat.number_of_LinphoneCallStreamsRunning, 1, 2000) );
	CU_ASSERT_TRUE( wait_for_list(lcs, &marie2->stat.number_of_LinphoneCallEnd, 1, 2000) );


	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie1);
	linphone_core_manager_destroy(marie2);
}
#endif

static void multiple_answers_call_with_media_relay() {

	/* Scenario is this: pauline calls marie, which is registered 2 times.
	 *   Both linphones answer at the same time, and only one should get the
	 *   call running, the other should be terminated */
	char ringbackpath[256];
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc" );
	LinphoneCoreManager* marie1  = linphone_core_manager_new( "marie_rc" );
	LinphoneCoreManager* marie2  = linphone_core_manager_new( "marie_rc" );

	LinphoneCall* call1, *call2;

	MSList* lcs = ms_list_append(NULL,pauline->lc);
	lcs = ms_list_append(lcs,marie1->lc);
	lcs = ms_list_append(lcs,marie2->lc);

	linphone_core_use_files(pauline->lc, TRUE);
	linphone_core_use_files(marie1->lc, TRUE);
	linphone_core_use_files(marie2->lc, TRUE);

	linphone_core_set_user_agent(pauline->lc, "Natted Linphone", NULL);
	linphone_core_set_user_agent(marie1->lc, "Natted Linphone", NULL);
	linphone_core_set_user_agent(marie2->lc, "Natted Linphone", NULL);

	snprintf(ringbackpath,sizeof(ringbackpath), "%s/sounds/hello8000.wav" /*use hello because rinback is too short*/, liblinphone_tester_file_prefix);

	CU_ASSERT_TRUE(wait_for_until(pauline->lc, NULL, &pauline->stat.number_of_LinphoneRegistrationOk, 1, 2000));

	CU_ASSERT_PTR_NOT_NULL( linphone_core_invite_address(pauline->lc, marie1->identity ) );

	CU_ASSERT_TRUE(wait_for_list(lcs,&marie1->stat.number_of_LinphoneCallIncomingReceived, 1, 2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived, 1, 2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingProgress, 1, 2000));

	// marie 1 and 2 answer at the same time
	call1 = linphone_core_get_current_call(marie1->lc);
	call2 = linphone_core_get_current_call(marie2->lc);

	CU_ASSERT_PTR_NOT_NULL_FATAL(call1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(call2);

	CU_ASSERT_EQUAL( linphone_core_accept_call(marie1->lc, call1), 0);
	CU_ASSERT_EQUAL( linphone_core_accept_call(marie2->lc, call2), 0);

	CU_ASSERT_TRUE( wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallStreamsRunning, 1, 2000) );
	CU_ASSERT_TRUE( wait_for_list(lcs, &marie1->stat.number_of_LinphoneCallStreamsRunning, 1, 2000) );
	CU_ASSERT_TRUE( wait_for_list(lcs, &marie2->stat.number_of_LinphoneCallEnd, 1, 2000) );


	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie1);
	linphone_core_manager_destroy(marie2);
}

static void call_with_specified_codec_bitrate(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	const LinphoneCallStats *pauline_stats,*marie_stats;
	bool_t call_ok;
	char * codec = "opus";
	int rate = 48000;
	int min_bw=24;
	int max_bw=40;

#ifdef __arm__
	if (ms_get_cpu_count() <2) { /*2 opus codec channel + resampler is too much for a single core*/
#ifndef ANDROID
		codec = "speex";
		rate = 8000;
		min_bw=20;
		max_bw=35;
#else
		CU_PASS("Test requires at least a dual core");
		goto end;
#endif
	}
#endif

	if (linphone_core_find_payload_type(marie->lc,codec,rate,-1)==NULL){
		ms_warning("opus codec not supported, test skipped.");
		goto end;
	}

	disable_all_audio_codecs_except_one(marie->lc,codec,rate);
	disable_all_audio_codecs_except_one(pauline->lc,codec,rate);

	linphone_core_set_payload_type_bitrate(marie->lc,
		linphone_core_find_payload_type(marie->lc,codec,rate,-1),
		max_bw);
	linphone_core_set_payload_type_bitrate(pauline->lc,
		linphone_core_find_payload_type(pauline->lc,codec,rate,-1),
		min_bw);

	CU_ASSERT_TRUE((call_ok=call(pauline,marie)));
	if (!call_ok) goto end;
	liblinphone_tester_check_rtcp(marie,pauline);
	marie_stats=linphone_call_get_audio_stats(linphone_core_get_current_call(marie->lc));
	pauline_stats=linphone_call_get_audio_stats(linphone_core_get_current_call(pauline->lc));
	CU_ASSERT_TRUE(marie_stats->download_bandwidth<(min_bw+5+min_bw*.1));
	CU_ASSERT_TRUE(pauline_stats->download_bandwidth>(max_bw-5-max_bw*.1));

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void simple_call_compatibility_mode(void) {
	char route[256];
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	LinphoneCore* lc_marie=marie->lc;
	LinphoneCore* lc_pauline=pauline->lc;
	stats* stat_marie=&marie->stat;
	stats* stat_pauline=&pauline->stat;
	LinphoneProxyConfig* proxy;
	LinphoneAddress* identity;
	LinphoneAddress* proxy_address;
	char*tmp;
	LCSipTransports transport;

	linphone_core_get_default_proxy(lc_marie,&proxy);
	CU_ASSERT_PTR_NOT_NULL (proxy);
	identity = linphone_address_new(linphone_proxy_config_get_identity(proxy));


	proxy_address=linphone_address_new(linphone_proxy_config_get_addr(proxy));
	linphone_address_clean(proxy_address);
	tmp=linphone_address_as_string_uri_only(proxy_address);
	linphone_proxy_config_set_server_addr(proxy,tmp);
	sprintf(route,"sip:%s",test_route);
	linphone_proxy_config_set_route(proxy,route);
	ms_free(tmp);
	linphone_address_destroy(proxy_address);
	linphone_core_get_sip_transports(lc_marie,&transport);
	transport.udp_port=0;
	transport.tls_port=0;
	transport.dtls_port=0;
	/*only keep tcp*/
	linphone_core_set_sip_transports(lc_marie,&transport);
	stat_marie->number_of_LinphoneRegistrationOk=0;

	CU_ASSERT_TRUE (wait_for(lc_marie,lc_marie,&stat_marie->number_of_LinphoneRegistrationOk,1));

	linphone_core_invite_address(lc_marie,pauline->identity);

	CU_ASSERT_TRUE (wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallIncomingReceived,1));
	CU_ASSERT_TRUE(linphone_core_inc_invite_pending(lc_pauline));
	CU_ASSERT_EQUAL(stat_marie->number_of_LinphoneCallOutgoingProgress,1);
	CU_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallOutgoingRinging,1));

	CU_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call_remote_address(lc_pauline));
	if (linphone_core_get_current_call_remote_address(lc_pauline)) {
		CU_ASSERT_TRUE(linphone_address_weak_equal(identity,linphone_core_get_current_call_remote_address(lc_pauline)));
		linphone_address_destroy(identity);

		linphone_core_accept_call(lc_pauline,linphone_core_get_current_call(lc_pauline));

		CU_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallConnected,1));
		CU_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallConnected,1));
		CU_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallStreamsRunning,1));
		CU_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallStreamsRunning,1));
		/*just to sleep*/
		wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallStreamsRunning,3);
		linphone_core_terminate_all_calls(lc_pauline);
		CU_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallEnd,1));
		CU_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallEnd,1));
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


static void cancelled_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	LinphoneCall* out_call = linphone_core_invite_address(pauline->lc,marie->identity);
	linphone_call_ref(out_call);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallOutgoingInit,1));

	linphone_core_terminate_call(pauline->lc,out_call);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	//CU_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonCanceled);
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallEnd,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallIncomingReceived,0);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallReleased,1));

	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void disable_all_audio_codecs_except_one(LinphoneCore *lc, const char *mime, int rate){
	const MSList *elem=linphone_core_get_audio_codecs(lc);
	PayloadType *pt;

	for(;elem!=NULL;elem=elem->next){
		pt=(PayloadType*)elem->data;
		linphone_core_enable_payload_type(lc,pt,FALSE);
	}
	pt=linphone_core_find_payload_type(lc,mime,rate,-1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(pt);
	linphone_core_enable_payload_type(lc,pt,TRUE);
}

#ifdef VIDEO_ENABLED
static void disable_all_video_codecs_except_one(LinphoneCore *lc, const char *mime) {
	const MSList *codecs = linphone_core_get_video_codecs(lc);
	const MSList *it = NULL;
	PayloadType *pt = NULL;

	for(it = codecs; it != NULL; it = it->next) {
		linphone_core_enable_payload_type(lc, (PayloadType *)it->data, FALSE);
	}
	CU_ASSERT_PTR_NOT_NULL_FATAL(pt = linphone_core_find_payload_type(lc, mime, -1, -1));
	linphone_core_enable_payload_type(lc, pt, TRUE);
}
#endif

static void call_failed_because_of_codecs(void) {
	int begin,leaked_objects;

	belle_sip_object_enable_leak_detector(TRUE);
	begin=belle_sip_object_get_object_count();

	{
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
		LinphoneCall* out_call;

		disable_all_audio_codecs_except_one(marie->lc,"pcmu",-1);
		disable_all_audio_codecs_except_one(pauline->lc,"pcma",-1);
		out_call = linphone_core_invite_address(pauline->lc,marie->identity);
		linphone_call_ref(out_call);
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallOutgoingInit,1));

		/*flexisip will retain the 488 until the "urgent reply" timeout (I.E 5s) arrives.*/
		CU_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallError,1,7000));
		CU_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonNotAcceptable);
		CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallIncomingReceived,0);
		CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallReleased,0);

		linphone_call_unref(out_call);
		linphone_core_manager_destroy(marie);
		linphone_core_manager_destroy(pauline);
	}
	leaked_objects=belle_sip_object_get_object_count()-begin;
	CU_ASSERT_TRUE(leaked_objects==0);
	if (leaked_objects>0){
		belle_sip_object_dump_active_objects();
	}
}

static void call_with_dns_time_out(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new2( "empty_rc", FALSE);
	LCSipTransports transport = {9773,0,0,0};
	int i;

	linphone_core_set_sip_transports(marie->lc,&transport);
	linphone_core_iterate(marie->lc);
	sal_set_dns_timeout(marie->lc->sal,0);
	linphone_core_invite(marie->lc,"\"t\x8et\x8e\" <sip:toto@toto.com>"); /*just to use non ascii values*/
	for(i=0;i<10;i++){
		ms_usleep(200000);
		linphone_core_iterate(marie->lc);
	}
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallOutgoingInit,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallOutgoingProgress,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallError,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallReleased,1);
	linphone_core_manager_destroy(marie);
}

static void early_cancelled_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new2( "empty_rc",FALSE);

	LinphoneCall* out_call = linphone_core_invite_address(pauline->lc,marie->identity);

	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallOutgoingInit,1));
	linphone_core_terminate_call(pauline->lc,out_call);

	/*since everything is executed in a row, no response can be received from the server, thus the CANCEL cannot be sent.
	 It will ring at Marie's side.*/

	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));

	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallEnd,1);

	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallIncomingReceived,1));
	/* now the CANCEL should have been sent and the the call at marie's side should terminate*/
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallReleased,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void cancelled_ringing_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	LinphoneCall* out_call = linphone_core_invite_address(pauline->lc,marie->identity);
	linphone_call_ref(out_call);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallIncomingReceived,1));

	linphone_core_terminate_call(pauline->lc,out_call);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallReleased,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallReleased,1));
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallEnd,1);
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallEnd,1);

	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void early_declined_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCallLog* out_call_log;
	LinphoneCall* out_call;

	linphone_core_set_max_calls(marie->lc,0);
	out_call = linphone_core_invite_address(pauline->lc,marie->identity);
	linphone_call_ref(out_call);

	/*wait until flexisip transfers the busy...*/
	CU_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallError,1,33000));
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallError,1);
	/* FIXME http://git.linphone.org/mantis/view.php?id=757

	CU_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonBusy);
	 */
	if (ms_list_size(linphone_core_get_call_logs(pauline->lc))>0) {
		CU_ASSERT_PTR_NOT_NULL(out_call_log=(LinphoneCallLog*)(linphone_core_get_call_logs(pauline->lc)->data));
		CU_ASSERT_EQUAL(linphone_call_log_get_status(out_call_log),LinphoneCallAborted);
	}
	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_declined(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	LinphoneCall* in_call;
	LinphoneCall* out_call = linphone_core_invite_address(pauline->lc,marie->identity);
	linphone_call_ref(out_call);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallIncomingReceived,1));
	CU_ASSERT_PTR_NOT_NULL(in_call=linphone_core_get_current_call(marie->lc));
	if (in_call) {
		linphone_call_ref(in_call);
		linphone_core_terminate_call(marie->lc,in_call);
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallReleased,1));
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallReleased,1));
		CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallEnd,1);
		CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallEnd,1);
		CU_ASSERT_EQUAL(linphone_call_get_reason(in_call),LinphoneReasonDeclined);
		CU_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonDeclined);
		linphone_call_unref(in_call);
	}
	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_terminated_by_caller(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	CU_ASSERT_TRUE(call(pauline,marie));
	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_no_sdp(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	linphone_core_enable_sdp_200_ack(marie->lc,TRUE);

	CU_ASSERT_TRUE(call(marie,pauline));
	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_no_sdp_ack_without_sdp(void){
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCall *call;

	linphone_core_enable_sdp_200_ack(marie->lc,TRUE);
	
	linphone_core_invite_address(marie->lc,pauline->identity);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallIncomingReceived,1));
	call=linphone_core_get_current_call(pauline->lc);
	if (call){
		sal_call_enable_sdp_removal(call->op, TRUE); /*this will have the effect that the SDP received in the ACK will be ignored*/
		linphone_core_accept_call(pauline->lc, call);
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallError,1));
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


static bool_t check_ice(LinphoneCoreManager* caller, LinphoneCoreManager* callee, LinphoneIceState state) {
	LinphoneCall *c1,*c2;
	bool_t audio_success=FALSE;
	bool_t video_success=FALSE;
	bool_t video_enabled;
	MSTimeSpec ts;

	c1=linphone_core_get_current_call(caller->lc);
	c2=linphone_core_get_current_call(callee->lc);

	CU_ASSERT_PTR_NOT_NULL(c1);
	CU_ASSERT_PTR_NOT_NULL(c2);
	if (!c1 || !c2) return FALSE;
	linphone_call_ref(c1);
	linphone_call_ref(c2);

	CU_ASSERT_EQUAL(linphone_call_params_video_enabled(linphone_call_get_current_params(c1)),linphone_call_params_video_enabled(linphone_call_get_current_params(c2)));
	video_enabled=linphone_call_params_video_enabled(linphone_call_get_current_params(c1));
	liblinphone_tester_clock_start(&ts);
	do{
		if ((c1 != NULL) && (c2 != NULL)) {
			if (linphone_call_get_audio_stats(c1)->ice_state==state &&
				linphone_call_get_audio_stats(c2)->ice_state==state ){
				audio_success=TRUE;
				break;
			}
			linphone_core_iterate(caller->lc);
			linphone_core_iterate(callee->lc);
		}
		ms_usleep(20000);
	}while(!liblinphone_tester_clock_elapsed(&ts,10000));

	if (video_enabled){
		liblinphone_tester_clock_start(&ts);
		do{
			if ((c1 != NULL) && (c2 != NULL)) {
				if (linphone_call_get_video_stats(c1)->ice_state==state &&
					linphone_call_get_video_stats(c2)->ice_state==state ){
					video_success=TRUE;
					break;
				}
				linphone_core_iterate(caller->lc);
				linphone_core_iterate(callee->lc);
			}
			ms_usleep(20000);
		}while(!liblinphone_tester_clock_elapsed(&ts,10000));
	}

	 /*make sure encryption mode are preserved*/
	if (c1) {
		const LinphoneCallParams* call_param = linphone_call_get_current_params(c1);
		CU_ASSERT_EQUAL(linphone_call_params_get_media_encryption(call_param),linphone_core_get_media_encryption(caller->lc));
	}
	if (c2) {
		const LinphoneCallParams* call_param = linphone_call_get_current_params(c2);
		CU_ASSERT_EQUAL(linphone_call_params_get_media_encryption(call_param),linphone_core_get_media_encryption(callee->lc));
	}
	linphone_call_unref(c1);
	linphone_call_unref(c2);
	return video_enabled ? audio_success && video_success : audio_success;
}

static void _call_with_ice_base(LinphoneCoreManager* pauline,LinphoneCoreManager* marie, bool_t caller_with_ice, bool_t callee_with_ice, bool_t random_ports) {
	if (callee_with_ice){
		linphone_core_set_firewall_policy(marie->lc,LinphonePolicyUseIce);
		linphone_core_set_stun_server(marie->lc,"stun.linphone.org");
	}
	if (caller_with_ice){
		linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);
		linphone_core_set_stun_server(pauline->lc,"stun.linphone.org");
	}

	if (random_ports){
		linphone_core_set_audio_port(marie->lc,-1);
		linphone_core_set_video_port(marie->lc,-1);
		linphone_core_set_audio_port(pauline->lc,-1);
		linphone_core_set_video_port(pauline->lc,-1);
	}

	CU_ASSERT_TRUE(call(pauline,marie));

	if (callee_with_ice && caller_with_ice) {
		/*wait for the ICE reINVITE to complete*/
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));

		CU_ASSERT_TRUE(check_ice(pauline,marie,LinphoneIceStateHostConnection));
	}

	liblinphone_tester_check_rtcp(marie,pauline);
	/*then close the call*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

}
static void _call_with_ice(bool_t caller_with_ice, bool_t callee_with_ice, bool_t random_ports) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	_call_with_ice_base(pauline,marie,caller_with_ice,callee_with_ice,random_ports);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void call_with_ice(void){
	_call_with_ice(TRUE,TRUE,FALSE);
}

/*ICE is not expected to work in this case, however this should not crash*/
static void call_with_ice_no_sdp(void){
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	linphone_core_enable_sdp_200_ack(pauline->lc,TRUE);

	linphone_core_set_firewall_policy(marie->lc,LinphonePolicyUseIce);
	linphone_core_set_stun_server(marie->lc,"stun.linphone.org");

	linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);
	linphone_core_set_stun_server(pauline->lc,"stun.linphone.org");

	call(pauline,marie);

	liblinphone_tester_check_rtcp(marie,pauline);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_ice_random_ports(void){
	_call_with_ice(TRUE,TRUE,TRUE);
}

static void ice_to_not_ice(void){
	_call_with_ice(TRUE,FALSE,FALSE);
}

static void not_ice_to_ice(void){
	_call_with_ice(FALSE,TRUE,FALSE);
}

static void call_with_custom_headers(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCall *call_marie,*call_pauline;
	LinphoneCallParams *params;
	const LinphoneCallParams *marie_remote_params;
	const char *hvalue;
	char	*pauline_remote_contact_header,
				*pauline_remote_contact,
				*marie_remote_contact,
				*marie_remote_contact_header;
	LinphoneAddress* marie_identity;
	char* tmp=linphone_address_as_string_uri_only(marie->identity);
	char tmp2[256];
	snprintf(tmp2,sizeof(tmp2),"%s?uriHeader=myUriHeader",tmp);
	marie_identity=linphone_address_new(tmp2);
	ms_free(tmp);
	linphone_address_destroy(marie->identity);
	marie->identity=marie_identity;

	params=linphone_core_create_default_call_parameters(marie->lc);
	linphone_call_params_add_custom_header(params,"Weather","bad");
	linphone_call_params_add_custom_header(params,"Working","yes");

	CU_ASSERT_TRUE(call_with_caller_params(pauline,marie,params));
	linphone_call_params_destroy(params);

	call_marie=linphone_core_get_current_call(marie->lc);
	call_pauline=linphone_core_get_current_call(pauline->lc);

	CU_ASSERT_PTR_NOT_NULL(call_marie);
	CU_ASSERT_PTR_NOT_NULL(call_pauline);

	marie_remote_params=linphone_call_get_remote_params(call_marie);
	hvalue=linphone_call_params_get_custom_header(marie_remote_params,"Weather");
	CU_ASSERT_PTR_NOT_NULL(hvalue);
	CU_ASSERT_STRING_EQUAL(hvalue,"bad");
	hvalue=linphone_call_params_get_custom_header(marie_remote_params,"uriHeader");
	CU_ASSERT_PTR_NOT_NULL(hvalue);
	CU_ASSERT_STRING_EQUAL(hvalue,"myUriHeader");


	// FIXME: we have to strdup because successive calls to get_remote_params erase the returned const char*!!
	pauline_remote_contact        = ms_strdup(linphone_call_get_remote_contact(call_pauline));
	pauline_remote_contact_header = ms_strdup(linphone_call_params_get_custom_header(linphone_call_get_remote_params(call_pauline), "Contact"));

	marie_remote_contact        = ms_strdup(linphone_call_get_remote_contact(call_marie));
	marie_remote_contact_header = ms_strdup(linphone_call_params_get_custom_header(marie_remote_params, "Contact"));

	CU_ASSERT_PTR_NOT_NULL(pauline_remote_contact);
	CU_ASSERT_PTR_NOT_NULL(pauline_remote_contact_header);
	CU_ASSERT_PTR_NOT_NULL(marie_remote_contact);
	CU_ASSERT_PTR_NOT_NULL(marie_remote_contact_header);
	CU_ASSERT_STRING_EQUAL(pauline_remote_contact,pauline_remote_contact_header);
	CU_ASSERT_STRING_EQUAL(marie_remote_contact,marie_remote_contact_header);

	ms_free(pauline_remote_contact);
	ms_free(pauline_remote_contact_header);
	ms_free(marie_remote_contact);
	ms_free(marie_remote_contact_header);


	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_paused_resumed(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCall* call_pauline;
	const rtp_stats_t * stats;

	CU_ASSERT_TRUE(call(pauline,marie));
	call_pauline = linphone_core_get_current_call(pauline->lc);

	wait_for_until(pauline->lc, marie->lc, NULL, 5, 3000);

	linphone_core_pause_call(pauline->lc,call_pauline);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausing,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPaused,1));

	/*stay in pause a little while in order to generate traffic*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);

	linphone_core_resume_call(pauline->lc,call_pauline);

	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	/*same here: wait a while for a bit of a traffic, we need to receive a RTCP packet*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 5000);

	/*since RTCP streams are reset when call is paused/resumed, there should be no loss at all*/
	stats = rtp_session_get_stats(call_pauline->sessions->rtp_session);
	CU_ASSERT_EQUAL(stats->cum_packet_loss, 0);

	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));


	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#define CHECK_CURRENT_LOSS_RATE() \
	rtcp_count_current = pauline->stat.number_of_rtcp_sent; \
	/*wait for an RTCP packet to have an accurate cumulative lost value*/ \
	CU_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &pauline->stat.number_of_rtcp_sent, rtcp_count_current+1, 10000)); \
	stats = rtp_session_get_stats(call_pauline->audiostream->ms.sessions.rtp_session); \
	loss_percentage = stats->cum_packet_loss * 100.f / (stats->packet_recv + stats->cum_packet_loss); \
	CU_ASSERT_TRUE(.75 * params.loss_rate < loss_percentage); \
	CU_ASSERT_TRUE(loss_percentage < 1.25 * params.loss_rate)

static void call_paused_resumed_with_loss(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCall* call_pauline;
	const rtp_stats_t * stats;
	float loss_percentage;
	int rtcp_count_current;

	OrtpNetworkSimulatorParams params={0};
	params.enabled=TRUE;
	params.loss_rate=20;

	CU_ASSERT_TRUE(call(pauline,marie));
	call_pauline = linphone_core_get_current_call(pauline->lc);
	rtp_session_enable_network_simulation(call_pauline->audiostream->ms.sessions.rtp_session,&params);

	/*generate some traffic*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 6000);
	CHECK_CURRENT_LOSS_RATE();

	/*pause call*/
	linphone_core_pause_call(pauline->lc,call_pauline);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausing,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPaused,1));
	/*stay in pause a little while in order to generate traffic*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 5000);
	CHECK_CURRENT_LOSS_RATE();

	/*resume*/
	linphone_core_resume_call(pauline->lc,call_pauline);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 6000);

	/*since stats are NOT totally reset during pause, the stats->packet_recv is computed from
	the start of call. This test ensures that the loss rate is consistent during the entire call.*/
	CHECK_CURRENT_LOSS_RATE();
	linphone_core_terminate_all_calls(marie->lc);
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
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCall* call_marie;
	const rtp_stats_t * stats;

	CU_ASSERT_TRUE(call(pauline,marie));
	call_marie = linphone_core_get_current_call(marie->lc);

	linphone_core_pause_call(marie->lc,call_marie);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausing,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausedByRemote,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPaused,1));

	/*stay in pause a little while in order to generate traffic*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);

	linphone_core_resume_call(marie->lc,call_marie);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	/*same here: wait a while for a bit of a traffic, we need to receive a RTCP packet*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 5000);

	/*since RTCP streams are reset when call is paused/resumed, there should be no loss at all*/
	stats = rtp_session_get_stats(call_marie->sessions->rtp_session);
	CU_ASSERT_EQUAL(stats->cum_packet_loss, 0);

	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#ifdef VIDEO_ENABLED
static LinphoneCall* setup_video(LinphoneCoreManager* caller,LinphoneCoreManager* callee) {
	LinphoneVideoPolicy  caller_policy;
	LinphoneCallParams* callee_params;
	LinphoneCall* call_obj;

	if (!linphone_core_get_current_call(callee->lc) || linphone_call_get_state(linphone_core_get_current_call(callee->lc)) != LinphoneCallStreamsRunning
			|| !linphone_core_get_current_call(caller->lc) || linphone_call_get_state(linphone_core_get_current_call(caller->lc)) != LinphoneCallStreamsRunning ) {
		ms_warning("bad state for adding video");
		return FALSE;
	}

	caller_policy.automatically_accept=TRUE;
	caller_policy.automatically_initiate=TRUE;
	linphone_core_enable_video_capture(callee->lc, TRUE);
	linphone_core_enable_video_display(callee->lc, TRUE);
	linphone_core_enable_video_capture(caller->lc, TRUE);
	linphone_core_enable_video_display(caller->lc, FALSE);
	linphone_core_set_video_policy(caller->lc,&caller_policy);

	if ((call_obj = linphone_core_get_current_call(callee->lc))) {
		callee_params = linphone_call_params_copy(linphone_call_get_current_params(call_obj));
		/*add video*/
		linphone_call_params_enable_video(callee_params,TRUE);
		linphone_core_update_call(callee->lc,call_obj,callee_params);
	}
	return call_obj;
}

static bool_t add_video(LinphoneCoreManager* caller,LinphoneCoreManager* callee) {
	stats initial_caller_stat=caller->stat;
	stats initial_callee_stat=callee->stat;
	LinphoneCall *call_obj;
	if ((call_obj=setup_video(caller, callee))){
		CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&caller->stat.number_of_LinphoneCallUpdatedByRemote,initial_caller_stat.number_of_LinphoneCallUpdatedByRemote+1));
		CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallUpdating,initial_callee_stat.number_of_LinphoneCallUpdating+1));
		CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallStreamsRunning,initial_callee_stat.number_of_LinphoneCallStreamsRunning+1));
		CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&caller->stat.number_of_LinphoneCallStreamsRunning,initial_caller_stat.number_of_LinphoneCallStreamsRunning+1));

		CU_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(callee->lc))));
		CU_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(caller->lc))));
		if (linphone_core_get_media_encryption(caller->lc) != LinphoneMediaEncryptionNone
				&& linphone_core_get_media_encryption(callee->lc) != LinphoneMediaEncryptionNone) {
			/*wait for encryption to be on, in case of zrtp, it can take a few seconds*/
			if (linphone_core_get_media_encryption(caller->lc) == LinphoneMediaEncryptionZRTP)
					wait_for(callee->lc,caller->lc,&caller->stat.number_of_LinphoneCallEncryptedOn,initial_caller_stat.number_of_LinphoneCallEncryptedOn+1);
			if (linphone_core_get_media_encryption(callee->lc) == LinphoneMediaEncryptionZRTP)
				wait_for(callee->lc,caller->lc,&callee->stat.number_of_LinphoneCallEncryptedOn,initial_callee_stat.number_of_LinphoneCallEncryptedOn+1);

			{
			const LinphoneCallParams* call_param = linphone_call_get_current_params(linphone_core_get_current_call(callee->lc));
			CU_ASSERT_EQUAL(linphone_call_params_get_media_encryption(call_param),linphone_core_get_media_encryption(caller->lc));
			call_param = linphone_call_get_current_params(linphone_core_get_current_call(caller->lc));
			CU_ASSERT_EQUAL(linphone_call_params_get_media_encryption(call_param),linphone_core_get_media_encryption(caller->lc));
			}
		}

		linphone_call_set_next_video_frame_decoded_callback(call_obj,linphone_call_cb,callee->lc);
		/*send vfu*/
		linphone_call_send_vfu_request(call_obj);
		return wait_for(caller->lc,callee->lc,&callee->stat.number_of_IframeDecoded,initial_callee_stat.number_of_IframeDecoded+1);
	}
	return FALSE;
}

static bool_t remove_video(LinphoneCoreManager *caller, LinphoneCoreManager *callee) {
	LinphoneCallParams *callee_params;
	LinphoneCall *call_obj;
	stats initial_caller_stat = caller->stat;
	stats initial_callee_stat = callee->stat;

	if (!linphone_core_get_current_call(callee->lc)
		|| (linphone_call_get_state(linphone_core_get_current_call(callee->lc)) != LinphoneCallStreamsRunning)
		|| !linphone_core_get_current_call(caller->lc)
		|| (linphone_call_get_state(linphone_core_get_current_call(caller->lc)) != LinphoneCallStreamsRunning)) {
		ms_warning("bad state for removing video");
		return FALSE;
	}

	if ((call_obj = linphone_core_get_current_call(callee->lc))) {
		callee_params = linphone_call_params_copy(linphone_call_get_current_params(call_obj));

		/* Remove video. */
		linphone_call_params_enable_video(callee_params, FALSE);
		linphone_core_update_call(callee->lc, call_obj, callee_params);

		CU_ASSERT_TRUE(wait_for(caller->lc, callee->lc, &caller->stat.number_of_LinphoneCallUpdatedByRemote, initial_caller_stat.number_of_LinphoneCallUpdatedByRemote + 1));
		CU_ASSERT_TRUE(wait_for(caller->lc, callee->lc, &callee->stat.number_of_LinphoneCallUpdating, initial_callee_stat.number_of_LinphoneCallUpdating + 1));
		CU_ASSERT_TRUE(wait_for(caller->lc, callee->lc, &callee->stat.number_of_LinphoneCallStreamsRunning, initial_callee_stat.number_of_LinphoneCallStreamsRunning + 1));
		CU_ASSERT_TRUE(wait_for(caller->lc, callee->lc, &caller->stat.number_of_LinphoneCallStreamsRunning, initial_caller_stat.number_of_LinphoneCallStreamsRunning + 1));

		CU_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(callee->lc))));
		CU_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(caller->lc))));

		return TRUE;
	}
	return FALSE;
}

static void call_with_video_added(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	CU_ASSERT_TRUE(call(pauline,marie));

	CU_ASSERT_TRUE(add_video(pauline,marie));
	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_video_added_random_ports(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	linphone_core_set_audio_port(marie->lc,-1);
	linphone_core_set_video_port(marie->lc,-1);
	linphone_core_set_audio_port(pauline->lc,-1);
	linphone_core_set_video_port(pauline->lc,-1);

	CU_ASSERT_TRUE(call(pauline,marie));

	CU_ASSERT_TRUE(add_video(pauline,marie));
	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_several_video_switches(void) {
	int dummy = 0;
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	CU_ASSERT_TRUE(call(pauline,marie));

	CU_ASSERT_TRUE(add_video(pauline,marie));
	wait_for_until(pauline->lc,marie->lc,&dummy,1,1000); /* Wait for VFU request exchanges to be finished. */
	CU_ASSERT_TRUE(remove_video(pauline,marie));
	CU_ASSERT_TRUE(add_video(pauline,marie));
	wait_for_until(pauline->lc,marie->lc,&dummy,1,1000); /* Wait for VFU request exchanges to be finished. */
	CU_ASSERT_TRUE(remove_video(pauline,marie));
	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void srtp_call_with_several_video_switches(void) {
	int dummy = 0;
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	if (linphone_core_media_encryption_supported(marie->lc, LinphoneMediaEncryptionSRTP)) {
		linphone_core_set_media_encryption(marie->lc, LinphoneMediaEncryptionSRTP);
		linphone_core_set_media_encryption(pauline->lc, LinphoneMediaEncryptionSRTP);

		CU_ASSERT_TRUE(call(pauline,marie));

		CU_ASSERT_TRUE(add_video(pauline,marie));
		wait_for_until(pauline->lc,marie->lc,&dummy,1,1000); /* Wait for VFU request exchanges to be finished. */
		CU_ASSERT_TRUE(remove_video(pauline,marie));
		CU_ASSERT_TRUE(add_video(pauline,marie));
		wait_for_until(pauline->lc,marie->lc,&dummy,1,1000); /* Wait for VFU request exchanges to be finished. */
		CU_ASSERT_TRUE(remove_video(pauline,marie));
		/*just to sleep*/
		linphone_core_terminate_all_calls(pauline->lc);
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	} else {
		ms_warning("Not tested because SRTP is not available.");
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_declined_video_base(bool_t using_policy) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCall* marie_call;
	LinphoneCall* pauline_call;
	LinphoneVideoPolicy marie_policy, pauline_policy;
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params = {0};
	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, TRUE);
	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, FALSE);

	if (using_policy) {
		pauline_policy.automatically_initiate=TRUE;
		pauline_policy.automatically_accept=FALSE;
		marie_policy.automatically_initiate=FALSE;
		marie_policy.automatically_accept=FALSE;

		linphone_core_set_video_policy(marie->lc,&marie_policy);
		linphone_core_set_video_policy(pauline->lc,&pauline_policy);
	}

	caller_test_params.base=linphone_core_create_default_call_parameters(pauline->lc);
	if (!using_policy)
		linphone_call_params_enable_video(caller_test_params.base,TRUE);

	if (!using_policy){
		callee_test_params.base=linphone_core_create_default_call_parameters(marie->lc);
		linphone_call_params_enable_video(callee_test_params.base,FALSE);
	}

	CU_ASSERT_TRUE(call_with_params2(pauline,marie,&caller_test_params,&callee_test_params,using_policy));

	linphone_call_params_destroy(caller_test_params.base);
	if (callee_test_params.base) linphone_call_params_destroy(callee_test_params.base);
	marie_call=linphone_core_get_current_call(marie->lc);
	pauline_call=linphone_core_get_current_call(pauline->lc);

	CU_ASSERT_FALSE(linphone_call_log_video_enabled(linphone_call_get_call_log(marie_call)));
	CU_ASSERT_FALSE(linphone_call_log_video_enabled(linphone_call_get_call_log(pauline_call)));


	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void call_with_declined_video(void) {
	call_with_declined_video_base(FALSE);
}
static void call_with_declined_video_using_policy(void) {
	call_with_declined_video_base(TRUE);
}

static void video_call_base(LinphoneCoreManager* pauline,LinphoneCoreManager* marie, bool_t using_policy) {
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params = {0};
	LinphoneCall* marie_call;
	LinphoneCall* pauline_call;
	LinphoneVideoPolicy marie_policy, pauline_policy;
	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, TRUE);
	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, FALSE);

	if (using_policy) {
		marie_policy.automatically_initiate=FALSE;
		marie_policy.automatically_accept=TRUE;
		pauline_policy.automatically_initiate=TRUE;
		pauline_policy.automatically_accept=FALSE;

		linphone_core_set_video_policy(marie->lc,&marie_policy);
		linphone_core_set_video_policy(pauline->lc,&pauline_policy);
	}

	caller_test_params.base=linphone_core_create_default_call_parameters(pauline->lc);
	if (!using_policy)
		linphone_call_params_enable_video(caller_test_params.base,TRUE);

	if (!using_policy){
		callee_test_params.base=linphone_core_create_default_call_parameters(marie->lc);
		linphone_call_params_enable_video(callee_test_params.base,TRUE);
	}

	CU_ASSERT_TRUE(call_with_params2(pauline,marie,&caller_test_params,&callee_test_params,using_policy));
	marie_call=linphone_core_get_current_call(marie->lc);
	pauline_call=linphone_core_get_current_call(pauline->lc);

	linphone_call_params_destroy(caller_test_params.base);
	if (callee_test_params.base) linphone_call_params_destroy(callee_test_params.base);

	if (marie_call && pauline_call ) {
		CU_ASSERT_TRUE(linphone_call_log_video_enabled(linphone_call_get_call_log(marie_call)));
		CU_ASSERT_TRUE(linphone_call_log_video_enabled(linphone_call_get_call_log(pauline_call)));

		/*check video path*/
		linphone_call_set_next_video_frame_decoded_callback(marie_call,linphone_call_cb,marie->lc);
		linphone_call_send_vfu_request(marie_call);
		CU_ASSERT_TRUE( wait_for(marie->lc,pauline->lc,&marie->stat.number_of_IframeDecoded,1));

		liblinphone_tester_check_rtcp(marie,pauline);

		linphone_core_terminate_all_calls(pauline->lc);
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	}

}
static void video_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	video_call_base(marie,pauline,FALSE);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void video_call_using_policy(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	video_call_base(marie,pauline,TRUE);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void video_call_no_sdp(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	linphone_core_enable_sdp_200_ack(pauline->lc,TRUE);
	video_call_base(pauline,marie,FALSE);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_ice_video_to_novideo(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneVideoPolicy vpol={0};
	vpol.automatically_initiate=TRUE;
	linphone_core_set_video_policy(pauline->lc,&vpol);
	vpol.automatically_initiate=FALSE;
	linphone_core_set_video_policy(marie->lc,&vpol);
	_call_with_ice_base(pauline,marie,TRUE,TRUE,TRUE);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_ice_video_added(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneVideoPolicy vpol={0};
	linphone_core_set_video_policy(pauline->lc,&vpol);
	linphone_core_set_video_policy(marie->lc,&vpol);

	linphone_core_set_firewall_policy(marie->lc,LinphonePolicyUseIce);
	linphone_core_set_stun_server(marie->lc,"stun.linphone.org");


	linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);
	linphone_core_set_stun_server(pauline->lc,"stun.linphone.org");


	if (1){
		linphone_core_set_audio_port(marie->lc,-1);
		linphone_core_set_video_port(marie->lc,-1);
		linphone_core_set_audio_port(pauline->lc,-1);
		linphone_core_set_video_port(pauline->lc,-1);
	}

	CU_ASSERT_TRUE(call(pauline,marie));
	CU_ASSERT_TRUE(check_ice(pauline,marie,LinphoneIceStateHostConnection));
	/*wait for ICE reINVITEs to complete*/
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2)
			&&
			wait_for(pauline->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	CU_ASSERT_TRUE(add_video(pauline,marie));
	CU_ASSERT_TRUE(check_ice(pauline,marie,LinphoneIceStateHostConnection));
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void video_call_with_ice_no_matching_audio_codecs(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_rc");
	LinphoneCall *out_call;

	linphone_core_enable_payload_type(marie->lc, linphone_core_find_payload_type(marie->lc, "PCMU", 8000, 1), FALSE); /* Disable PCMU */
	linphone_core_enable_payload_type(marie->lc, linphone_core_find_payload_type(marie->lc, "PCMA", 8000, 1), TRUE); /* Enable PCMA */
	linphone_core_set_firewall_policy(marie->lc, LinphonePolicyUseIce);
	linphone_core_set_stun_server(marie->lc, "stun.linphone.org");
	linphone_core_set_firewall_policy(pauline->lc, LinphonePolicyUseIce);
	linphone_core_set_stun_server(pauline->lc, "stun.linphone.org");

	out_call = linphone_core_invite_address(marie->lc, pauline->identity);
	linphone_call_ref(out_call);
	CU_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallOutgoingInit, 1));

	/* flexisip will retain the 488 until the "urgent reply" timeout arrives. */
	CU_ASSERT_TRUE(wait_for_until(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallError, 1, 6000));
	CU_ASSERT_EQUAL(linphone_call_get_reason(out_call), LinphoneReasonNotAcceptable);
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallIncomingReceived, 0);

	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#endif /*VIDEO_ENABLED*/

static void _call_with_media_relay(bool_t random_ports) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);

	if (random_ports){
		linphone_core_set_audio_port(marie->lc,-1);
		linphone_core_set_video_port(marie->lc,-1);
		linphone_core_set_audio_port(pauline->lc,-1);
		linphone_core_set_video_port(pauline->lc,-1);
	}

	CU_ASSERT_TRUE(call(pauline,marie));
	liblinphone_tester_check_rtcp(pauline,marie);

#ifdef VIDEO_ENABLED
	CU_ASSERT_TRUE(add_video(pauline,marie));
	liblinphone_tester_check_rtcp(pauline,marie);
#endif
	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_media_relay(void) {
	_call_with_media_relay(FALSE);
}

static void call_with_media_relay_random_ports(void) {
	_call_with_media_relay(TRUE);
}

static void call_with_privacy(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCall *c1,*c2;
	LinphoneCallParams *params;
	LinphoneProxyConfig* pauline_proxy;
	params=linphone_core_create_default_call_parameters(pauline->lc);
	linphone_call_params_set_privacy(params,LinphonePrivacyId);

	CU_ASSERT_TRUE(call_with_caller_params(pauline,marie,params));
	linphone_call_params_destroy(params);

	c1=linphone_core_get_current_call(pauline->lc);
	c2=linphone_core_get_current_call(marie->lc);

	CU_ASSERT_PTR_NOT_NULL(c1);
	CU_ASSERT_PTR_NOT_NULL(c2);

	/*make sure local identity is unchanged*/
	CU_ASSERT_TRUE(linphone_address_weak_equal(linphone_call_log_get_from(linphone_call_get_call_log(c1)),pauline->identity));

	/*make sure remote identity is hidden*/
	CU_ASSERT_FALSE(linphone_address_weak_equal(linphone_call_get_remote_address(c2),pauline->identity));

	CU_ASSERT_EQUAL(linphone_call_params_get_privacy(linphone_call_get_current_params(c2)),LinphonePrivacyId);

	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	/*test proxy config privacy*/
	linphone_core_get_default_proxy(pauline->lc,&pauline_proxy);
	linphone_proxy_config_set_privacy(pauline_proxy,LinphonePrivacyId);

	CU_ASSERT_TRUE(call(pauline,marie));
	c1=linphone_core_get_current_call(pauline->lc);
	c2=linphone_core_get_current_call(marie->lc);

	CU_ASSERT_PTR_NOT_NULL(c1);
	CU_ASSERT_PTR_NOT_NULL(c2);

	/*make sure remote identity is hidden*/
	CU_ASSERT_FALSE(linphone_address_weak_equal(linphone_call_get_remote_address(c2),pauline->identity));

	CU_ASSERT_EQUAL(linphone_call_params_get_privacy(linphone_call_get_current_params(c2)),LinphonePrivacyId);
	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,2));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,2));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

/*this ones makes call with privacy without previous registration*/
static void call_with_privacy2(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new2( "pauline_rc",FALSE);
	LinphoneCall *c1,*c2;
	LinphoneCallParams *params;
	LinphoneProxyConfig* pauline_proxy;
	params=linphone_core_create_default_call_parameters(pauline->lc);
	linphone_call_params_set_privacy(params,LinphonePrivacyId);

	linphone_core_get_default_proxy(pauline->lc,&pauline_proxy);
	linphone_proxy_config_edit(pauline_proxy);
	linphone_proxy_config_enable_register(pauline_proxy,FALSE);
	linphone_proxy_config_done(pauline_proxy);

	CU_ASSERT_TRUE(call_with_caller_params(pauline,marie,params));
	linphone_call_params_destroy(params);

	c1=linphone_core_get_current_call(pauline->lc);
	c2=linphone_core_get_current_call(marie->lc);

	CU_ASSERT_PTR_NOT_NULL(c1);
	CU_ASSERT_PTR_NOT_NULL(c2);

	/*make sure local identity is unchanged*/
	CU_ASSERT_TRUE(linphone_address_weak_equal(linphone_call_log_get_from(linphone_call_get_call_log(c1)),pauline->identity));

	/*make sure remote identity is hidden*/
	CU_ASSERT_FALSE(linphone_address_weak_equal(linphone_call_get_remote_address(c2),pauline->identity));

	CU_ASSERT_EQUAL(linphone_call_params_get_privacy(linphone_call_get_current_params(c2)),LinphonePrivacyId);

	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	/*test proxy config privacy*/
	linphone_proxy_config_set_privacy(pauline_proxy,LinphonePrivacyId);

	CU_ASSERT_TRUE(call(pauline,marie));
	c1=linphone_core_get_current_call(pauline->lc);
	c2=linphone_core_get_current_call(marie->lc);

	CU_ASSERT_PTR_NOT_NULL(c1);
	CU_ASSERT_PTR_NOT_NULL(c2);

	/*make sure remote identity is hidden*/
	CU_ASSERT_FALSE(linphone_address_weak_equal(linphone_call_get_remote_address(c2),pauline->identity));

	CU_ASSERT_EQUAL(linphone_call_params_get_privacy(linphone_call_get_current_params(c2)),LinphonePrivacyId);
	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,2));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,2));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_waiting_indication_with_param(bool_t enable_caller_privacy) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	char hellopath[256];
	MSList *iterator;
	MSList* lcs;
	LinphoneCall* pauline_called_by_marie;
	LinphoneCall* pauline_called_by_laure=NULL;
	LinphoneCallParams *laure_params=linphone_core_create_default_call_parameters(laure->lc);
	LinphoneCallParams *marie_params=linphone_core_create_default_call_parameters(marie->lc);

	if (enable_caller_privacy)
		linphone_call_params_set_privacy(marie_params,LinphonePrivacyId);

	lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);

	CU_ASSERT_TRUE(call_with_caller_params(marie,pauline,marie_params));
	pauline_called_by_marie=linphone_core_get_current_call(pauline->lc);


	/*use playfile for callee to avoid locking on capture card*/
	linphone_core_use_files (laure->lc,TRUE);
	linphone_core_use_files (marie->lc,TRUE);
	snprintf(hellopath,sizeof(hellopath), "%s/sounds/hello8000.wav", liblinphone_tester_file_prefix);
	linphone_core_set_play_file(laure->lc,hellopath);
	if (enable_caller_privacy)
		linphone_call_params_set_privacy(laure_params,LinphonePrivacyId);

	CU_ASSERT_PTR_NOT_NULL(linphone_core_invite_address_with_params(laure->lc,pauline->identity,laure_params));

	CU_ASSERT_TRUE(wait_for(laure->lc
							,pauline->lc
							,&pauline->stat.number_of_LinphoneCallIncomingReceived
							,2));

	CU_ASSERT_EQUAL(laure->stat.number_of_LinphoneCallOutgoingProgress,1);


	CU_ASSERT_TRUE(wait_for(laure->lc
							,pauline->lc
							,&laure->stat.number_of_LinphoneCallOutgoingRinging
							,1));

	for (iterator=(MSList *)linphone_core_get_calls(pauline->lc);iterator!=NULL;iterator=iterator->next) {
		LinphoneCall *call=(LinphoneCall *)iterator->data;
		if (call != pauline_called_by_marie) {
			/*fine, this is the call waiting*/
			pauline_called_by_laure=call;
			linphone_core_accept_call(pauline->lc,pauline_called_by_laure);
		}
	}

	CU_ASSERT_TRUE(wait_for(laure->lc
							,pauline->lc
							,&laure->stat.number_of_LinphoneCallConnected
							,1));

	CU_ASSERT_TRUE(wait_for(pauline->lc
								,marie->lc
								,&marie->stat.number_of_LinphoneCallPausedByRemote
								,1));

	if (pauline_called_by_laure && enable_caller_privacy )
		CU_ASSERT_EQUAL(linphone_call_params_get_privacy(linphone_call_get_current_params(pauline_called_by_laure)),LinphonePrivacyId);
	/*wait a bit for ACK to be sent*/
	wait_for_list(lcs,NULL,0,1000);
	linphone_core_terminate_all_calls(pauline->lc);

	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,1,2000));


	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	ms_list_free(lcs);
}
static void call_waiting_indication(void) {
	call_waiting_indication_with_param(FALSE);
}

static void call_waiting_indication_with_privacy(void) {
	call_waiting_indication_with_param(TRUE);
}

static void simple_conference_base(LinphoneCoreManager* marie, LinphoneCoreManager* pauline, LinphoneCoreManager* laure) {

	stats initial_marie_stat;
	stats initial_pauline_stat;
	stats initial_laure_stat;

	LinphoneCall* marie_call_pauline;
	LinphoneCall* pauline_called_by_marie;
	LinphoneCall* marie_call_laure;

	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);

	CU_ASSERT_TRUE(call(marie,pauline));
	marie_call_pauline=linphone_core_get_current_call(marie->lc);
	pauline_called_by_marie=linphone_core_get_current_call(pauline->lc);
	CU_ASSERT_TRUE(pause_call_1(marie,marie_call_pauline,pauline,pauline_called_by_marie));

	CU_ASSERT_TRUE(call(marie,laure));
	initial_marie_stat=marie->stat;
	initial_pauline_stat=pauline->stat;
	initial_laure_stat=laure->stat;

	marie_call_laure=linphone_core_get_current_call(marie->lc);

	CU_ASSERT_PTR_NOT_NULL_FATAL(marie_call_laure);
	linphone_core_add_to_conference(marie->lc,marie_call_laure);
	CU_ASSERT_TRUE(wait_for(marie->lc,laure->lc,&marie->stat.number_of_LinphoneCallUpdating,initial_marie_stat.number_of_LinphoneCallUpdating+1));



	linphone_core_add_to_conference(marie->lc,marie_call_pauline);

	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallResuming,initial_marie_stat.number_of_LinphoneCallResuming+1,2000));

	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,initial_pauline_stat.number_of_LinphoneCallStreamsRunning+1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,initial_laure_stat.number_of_LinphoneCallStreamsRunning+1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,initial_marie_stat.number_of_LinphoneCallStreamsRunning+2,3000));

	CU_ASSERT_TRUE(linphone_core_is_in_conference(marie->lc));
	CU_ASSERT_EQUAL(linphone_core_get_conference_size(marie->lc),3);

	/*
	 * FIXME: check_ice cannot work as it is today because there is no current call for the party that hosts the conference
	if (linphone_core_get_firewall_policy(marie->lc) == LinphonePolicyUseIce) {
		if (linphone_core_get_firewall_policy(pauline->lc) == LinphonePolicyUseIce) {
			check_ice(marie,pauline,LinphoneIceStateHostConnection);
		}
		if (linphone_core_get_firewall_policy(laure->lc) == LinphonePolicyUseIce) {
			check_ice(marie,laure,LinphoneIceStateHostConnection);
		}
	}
	*/

	linphone_core_terminate_conference(marie->lc);

	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,1,2000));



	ms_list_free(lcs);
}
static void simple_conference(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	simple_conference_base(marie,pauline,laure);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
}

static void simple_conference_with_ice(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");

	linphone_core_set_firewall_policy(marie->lc,LinphonePolicyUseIce);
	linphone_core_set_stun_server(marie->lc,"stun.linphone.org");
	linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);
	linphone_core_set_stun_server(pauline->lc,"stun.linphone.org");
	linphone_core_set_firewall_policy(laure->lc,LinphonePolicyUseIce);
	linphone_core_set_stun_server(laure->lc,"stun.linphone.org");

	simple_conference_base(marie,pauline,laure);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
}

static void srtp_call() {
	call_base(LinphoneMediaEncryptionSRTP,FALSE,FALSE,LinphonePolicyNoFirewall);
}

static void zrtp_call() {
	call_base(LinphoneMediaEncryptionZRTP,FALSE,FALSE,LinphonePolicyNoFirewall);
}
static void zrtp_video_call() {
	call_base(LinphoneMediaEncryptionZRTP,TRUE,FALSE,LinphonePolicyNoFirewall);
}

static void call_with_declined_srtp(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	if (linphone_core_media_encryption_supported(marie->lc,LinphoneMediaEncryptionSRTP)) {
		linphone_core_set_media_encryption(pauline->lc,LinphoneMediaEncryptionSRTP);

		CU_ASSERT_TRUE(call(pauline,marie));

		/*just to sleep*/
		linphone_core_terminate_all_calls(marie->lc);
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	} else {
		ms_warning ("not tested because srtp not available");
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void on_eof(LinphonePlayer *player, void *user_data){
	LinphoneCoreManager *marie=(LinphoneCoreManager*)user_data;
	marie->stat.number_of_player_eof++;
}

static void call_with_file_player(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphonePlayer *player;
	char hellopath[256];
	char *recordpath = create_filepath(liblinphone_tester_writable_dir_prefix, "record", "wav");
	double similar;
	const double threshold = 0.9;

	/*make sure the record file doesn't already exists, otherwise this test will append new samples to it*/
	unlink(recordpath);

	snprintf(hellopath,sizeof(hellopath), "%s/sounds/hello8000.wav", liblinphone_tester_file_prefix);

	/*caller uses files instead of soundcard in order to avoid mixing soundcard input with file played using call's player*/
	linphone_core_use_files(marie->lc,TRUE);
	linphone_core_set_play_file(marie->lc,NULL);

	/*callee is recording and plays file*/
	linphone_core_use_files(pauline->lc,TRUE);
	linphone_core_set_play_file(pauline->lc,NULL);
	linphone_core_set_record_file(pauline->lc,recordpath);

	CU_ASSERT_TRUE(call(marie,pauline));

	player=linphone_call_get_player(linphone_core_get_current_call(marie->lc));
	CU_ASSERT_PTR_NOT_NULL(player);
	if (player){
		CU_ASSERT_TRUE(linphone_player_open(player,hellopath,on_eof,marie)==0);
		CU_ASSERT_TRUE(linphone_player_start(player)==0);
	}
	CU_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&marie->stat.number_of_player_eof,1,12000));

	/*just to sleep*/
	linphone_core_terminate_all_calls(marie->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(ms_audio_diff(hellopath,recordpath,&similar,NULL,NULL)==0);
	CU_ASSERT_TRUE(similar>threshold);
	CU_ASSERT_TRUE(similar<=1.0);
	if(similar > threshold && similar <=1.0) {
		remove(recordpath);
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	ms_free(recordpath);
}

static bool_t is_format_supported(LinphoneCore *lc, const char *fmt){
	const char **formats=linphone_core_get_supported_file_formats(lc);
	for(;*formats!=NULL;++formats){
		if (strcasecmp(*formats,fmt)==0) return TRUE;
	}
	return FALSE;
}

static void call_with_mkv_file_player(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphonePlayer *player;
	char hellomkv[256];
	char hellowav[256];
	char *recordpath;
	double similar;
	const double threshold = 0.9;

	if (!is_format_supported(marie->lc,"mkv")){
		ms_warning("Test skipped, no mkv support.");
		goto end;
	}
	recordpath = create_filepath(liblinphone_tester_writable_dir_prefix, "record", "wav");
	/*make sure the record file doesn't already exists, otherwise this test will append new samples to it*/
	unlink(recordpath);

	snprintf(hellowav,sizeof(hellowav), "%s/sounds/hello8000.wav", liblinphone_tester_file_prefix);
	snprintf(hellomkv,sizeof(hellomkv), "%s/sounds/hello8000.mkv", liblinphone_tester_file_prefix);

	/*caller uses files instead of soundcard in order to avoid mixing soundcard input with file played using call's player*/
	linphone_core_use_files(marie->lc,TRUE);
	linphone_core_set_play_file(marie->lc,NULL);
	/*callee is recording and plays file*/
	linphone_core_use_files(pauline->lc,TRUE);
	linphone_core_set_play_file(pauline->lc,hellowav); /*just to send something but we are not testing what is sent by pauline*/
	linphone_core_set_record_file(pauline->lc,recordpath);

	CU_ASSERT_TRUE(call(marie,pauline));

	player=linphone_call_get_player(linphone_core_get_current_call(marie->lc));
	CU_ASSERT_PTR_NOT_NULL(player);
	if (player){
		int res = linphone_player_open(player,hellomkv,on_eof,marie);
		if(!ms_filter_codec_supported("opus")) {
			CU_ASSERT_EQUAL(res, -1);
			goto end;
		}
		CU_ASSERT_EQUAL(res, 0);
		CU_ASSERT_TRUE(linphone_player_start(player)==0);
		CU_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&marie->stat.number_of_player_eof,1,12000));
		linphone_player_close(player);
	}

	/*just to sleep*/
	linphone_core_terminate_all_calls(marie->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(ms_audio_diff(hellowav,recordpath,&similar,NULL,NULL)==0);
	CU_ASSERT_TRUE(similar>threshold);
	CU_ASSERT_TRUE(similar<=1.0);
	if(similar>threshold && similar<=1.0) {
		remove(recordpath);
	}
	ms_free(recordpath);

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);

}


static void call_base(LinphoneMediaEncryption mode, bool_t enable_video,bool_t enable_relay,LinphoneFirewallPolicy policy) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	if (enable_relay) {
		linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
		linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);
	}
	if (linphone_core_media_encryption_supported(marie->lc,mode)) {
		linphone_core_set_media_encryption(marie->lc,mode);
		linphone_core_set_media_encryption(pauline->lc,mode);

		linphone_core_set_firewall_policy(marie->lc,policy);
		linphone_core_set_stun_server(marie->lc,"stun.linphone.org");
		linphone_core_set_firewall_policy(pauline->lc,policy);
		linphone_core_set_stun_server(pauline->lc,"stun.linphone.org");


		CU_ASSERT_TRUE(call(pauline,marie));
		if (linphone_core_get_media_encryption(pauline->lc) == LinphoneMediaEncryptionZRTP
			&& linphone_core_get_media_encryption(pauline->lc) == LinphoneMediaEncryptionZRTP) {
			/*wait for SAS*/
			int i;
			for (i=0;i<10;i++) {
				if (linphone_call_get_authentication_token(linphone_core_get_current_call(pauline->lc))
					&&
					linphone_call_get_authentication_token(linphone_core_get_current_call(marie->lc))) {
					/*check SAS*/
								CU_ASSERT_STRING_EQUAL(linphone_call_get_authentication_token(linphone_core_get_current_call(pauline->lc))
												,linphone_call_get_authentication_token(linphone_core_get_current_call(marie->lc)));
								liblinphone_tester_check_rtcp(pauline,marie);
								break;
				}
				linphone_core_iterate(marie->lc);
				linphone_core_iterate(pauline->lc);
				ms_usleep(200000);
			}

		}

		if (policy == LinphonePolicyUseIce)
			CU_ASSERT_TRUE(check_ice(pauline,marie,LinphoneIceStateHostConnection));
#ifdef VIDEO_ENABLED
		if (enable_video) {
			int i=0;
			if (linphone_core_video_supported(marie->lc)) {
				for (i=0;i<100;i++) { /*fixme to workaround a crash*/
					ms_usleep(20000);
					linphone_core_iterate(marie->lc);
					linphone_core_iterate(pauline->lc);
				}

				add_video(pauline,marie);
				if (policy == LinphonePolicyUseIce)
					CU_ASSERT_TRUE(check_ice(pauline,marie,LinphoneIceStateHostConnection));

				liblinphone_tester_check_rtcp(marie,pauline);
				/*wait for ice to found the direct path*/
				CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_IframeDecoded,1));
			} else {
				ms_warning ("not tested because video not available");
			}

		}
#endif


		/*just to sleep*/
		linphone_core_terminate_all_calls(marie->lc);
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	} else {
		ms_warning ("not tested because %s not available", linphone_media_encryption_to_string(mode));
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#ifdef VIDEO_ENABLED
static void srtp_video_ice_call(void) {
	call_base(LinphoneMediaEncryptionSRTP,TRUE,FALSE,LinphonePolicyUseIce);
}
static void zrtp_video_ice_call(void) {
	call_base(LinphoneMediaEncryptionZRTP,TRUE,FALSE,LinphonePolicyUseIce);
}
#endif

static void srtp_ice_call(void) {
	call_base(LinphoneMediaEncryptionSRTP,FALSE,FALSE,LinphonePolicyUseIce);
}

static void zrtp_ice_call(void) {
	call_base(LinphoneMediaEncryptionZRTP,FALSE,FALSE,LinphonePolicyUseIce);
}
static void zrtp_ice_call_with_relay(void) {
	call_base(LinphoneMediaEncryptionZRTP,FALSE,TRUE,LinphonePolicyUseIce);
}

static void early_media_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_early_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	CU_ASSERT_TRUE(call(pauline,marie));

	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallIncomingEarlyMedia,1);
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallOutgoingEarlyMedia,1);

	wait_for_until(pauline->lc,marie->lc,NULL,0,1000);

	/*added because a bug related to early-media caused the Connected state to be reached two times*/
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallConnected,1);

	/*just to sleep*/
	linphone_core_terminate_all_calls(marie->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));



	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void early_media_call_with_ringing(void){
	char hellopath[256];
	LinphoneCoreManager* marie   = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("pauline_rc");
	MSList* lcs = NULL;
	LinphoneCall* marie_call;
	LinphoneCallLog *marie_call_log;
	time_t connected_time=0;
	time_t ended_time=0;
	int dummy=0;

	lcs = ms_list_append(lcs,marie->lc);
	lcs = ms_list_append(lcs,pauline->lc);
	/*
		Marie calls Pauline, and after the call has rung, transitions to an early_media session
	*/

	/*use playfile for callee to avoid locking on capture card*/
	linphone_core_use_files (pauline->lc,TRUE);
	snprintf(hellopath,sizeof(hellopath), "%s/sounds/hello8000.wav", liblinphone_tester_file_prefix);
	linphone_core_set_play_file(pauline->lc,hellopath);

	marie_call = linphone_core_invite_address(marie->lc, pauline->identity);
	marie_call_log = linphone_call_get_call_log(marie_call);

	CU_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingReceived,1,3000));
	CU_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingRinging,1,1000));


	if (linphone_core_inc_invite_pending(pauline->lc)) {
		/* send a 183 to initiate the early media */

		linphone_core_accept_early_media(pauline->lc, linphone_core_get_current_call(pauline->lc));

		CU_ASSERT_TRUE( wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingEarlyMedia,1,2000) );
		CU_ASSERT_TRUE( wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingEarlyMedia,1,2000) );

		liblinphone_tester_check_rtcp(marie, pauline);

		linphone_core_accept_call(pauline->lc, linphone_core_get_current_call(pauline->lc));

		CU_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallConnected, 1,1000));
		connected_time=time(NULL);
		CU_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallStreamsRunning, 1,1000));

		CU_ASSERT_EQUAL(marie_call, linphone_core_get_current_call(marie->lc));

		liblinphone_tester_check_rtcp(marie, pauline);
		/*just to have a call duration !=0*/
		wait_for_list(lcs,&dummy,1,2000);

		linphone_core_terminate_all_calls(pauline->lc);

		CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));
		ended_time=time(NULL);
		CU_ASSERT_TRUE (labs (linphone_call_log_get_duration(marie_call_log) - (ended_time - connected_time)) <1 );
		ms_list_free(lcs);
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void early_media_call_with_update_base(bool_t media_change){
	char hellopath[256];
	LinphoneCoreManager* marie   = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("pauline_rc");
	MSList* lcs = NULL;
	LinphoneCall *marie_call, *pauline_call;
	LinphoneCallParams *pauline_params;

	lcs = ms_list_append(lcs,marie->lc);
	lcs = ms_list_append(lcs,pauline->lc);
	if (media_change) {
		disable_all_audio_codecs_except_one(marie->lc,"pcmu",-1);
		disable_all_audio_codecs_except_one(pauline->lc,"pcmu",-1);
	}
	/*
		Marie calls Pauline, and after the call has rung, transitions to an early_media session
	*/

	/*use playfile for callee to avoid locking on capture card*/
	linphone_core_use_files (pauline->lc,TRUE);
	snprintf(hellopath,sizeof(hellopath), "%s/sounds/hello8000.wav", liblinphone_tester_file_prefix);
	linphone_core_set_play_file(pauline->lc,hellopath);

	marie_call = linphone_core_invite_address(marie->lc, pauline->identity);

	CU_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingRinging,1,1000));
	/* send a 183 to initiate the early media */
	linphone_core_accept_early_media(pauline->lc, linphone_core_get_current_call(pauline->lc));
	CU_ASSERT_TRUE( wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingEarlyMedia,1,2000) );
	CU_ASSERT_TRUE( wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingEarlyMedia,1,2000) );

	pauline_call = linphone_core_get_current_call(pauline->lc);
	pauline_params = linphone_call_params_copy(linphone_call_get_current_params(pauline_call));

	if (media_change) {
		disable_all_audio_codecs_except_one(marie->lc,"pcma",-1);
		disable_all_audio_codecs_except_one(pauline->lc,"pcma",-1);
	}
	#define UPDATED_SESSION_NAME "nouveau nom de session"

	linphone_call_params_set_session_name(pauline_params,UPDATED_SESSION_NAME);
	linphone_core_update_call(pauline->lc, pauline_call, pauline_params);
	CU_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallEarlyUpdating,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallEarlyUpdatedByRemote,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingEarlyMedia,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingEarlyMedia,1,2000));

	/*just to wait 2s*/
	liblinphone_tester_check_rtcp(marie, pauline);

	CU_ASSERT_STRING_EQUAL(	  linphone_call_params_get_session_name(linphone_call_get_remote_params(marie_call))
							, UPDATED_SESSION_NAME);

	linphone_core_accept_call(pauline->lc, linphone_core_get_current_call(pauline->lc));

	CU_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallConnected, 1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallStreamsRunning, 1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallConnected, 1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallStreamsRunning, 1,1000));

	liblinphone_tester_check_rtcp(marie, pauline);

	linphone_core_terminate_all_calls(pauline->lc);

	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));


	ms_list_free(lcs);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void early_media_call_with_session_update(void){
	early_media_call_with_update_base(FALSE);
}

static void early_media_call_with_codec_update(void){
	early_media_call_with_update_base(TRUE);
}

static void simple_call_transfer(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	LinphoneCall* pauline_called_by_marie;
	LinphoneCall *marie_calling_pauline;
	LinphoneCall *marie_calling_laure;

	char* laure_identity=linphone_address_as_string(laure->identity);
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);


	CU_ASSERT_TRUE(call(marie,pauline));
	marie_calling_pauline=linphone_core_get_current_call(marie->lc);
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

	CU_ASSERT_PTR_NOT_NULL(linphone_call_get_transfer_target_call(marie_calling_pauline));

	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneTransferCallOutgoingInit,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallIncomingReceived,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallOutgoingRinging,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneTransferCallOutgoingProgress,1,2000));
	linphone_core_accept_call(laure->lc,linphone_core_get_current_call(laure->lc));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallConnected,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallConnected,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,1,2000));

	marie_calling_laure=linphone_core_get_current_call(marie->lc);
	CU_ASSERT_PTR_NOT_NULL_FATAL(marie_calling_laure);
	CU_ASSERT_TRUE(linphone_call_get_transferer_call(marie_calling_laure)==marie_calling_pauline);

	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneTransferCallConnected,1,2000));

	/*terminate marie to pauline call*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,2000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	ms_list_free(lcs);
}

static void unattended_call_transfer(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	LinphoneCall* pauline_called_by_marie;

	char* laure_identity=linphone_address_as_string(laure->identity);
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);


	CU_ASSERT_TRUE(call(marie,pauline));
	pauline_called_by_marie=linphone_core_get_current_call(marie->lc);

	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);
	reset_counters(&laure->stat);

	linphone_core_transfer_call(marie->lc,pauline_called_by_marie,laure_identity);
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallRefered,1,2000));

	/*marie ends the call  */
	linphone_core_terminate_call(marie->lc,pauline_called_by_marie);
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));

	/*Pauline starts the transfer*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingInit,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingProgress,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallIncomingReceived,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,2000));
	linphone_core_accept_call(laure->lc,linphone_core_get_current_call(laure->lc));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallConnected,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,2000));

	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	ms_list_free(lcs);
}

static void unattended_call_transfer_with_error(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCall* pauline_called_by_marie;

	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);

	CU_ASSERT_TRUE(call(marie,pauline));
	pauline_called_by_marie=linphone_core_get_current_call(marie->lc);

	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);

	linphone_core_transfer_call(marie->lc,pauline_called_by_marie,"unknown_user");
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallRefered,1,2000));

	/*Pauline starts the transfer*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingInit,1,2000));
	/* and immediately get an error*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallError,1,2000));

	/*the error must be reported back to marie*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransferCallError,1,2000));

	/*and pauline should resume the call automatically*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallResuming,1,2000));

	/*and call should be resumed*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,1,2000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	ms_list_free(lcs);
}


static void call_transfer_existing_call_outgoing_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");

	LinphoneCall* marie_call_pauline;
	LinphoneCall* pauline_called_by_marie;
	LinphoneCall* marie_call_laure;
	LinphoneCall* laure_called_by_marie;
	LinphoneCall* lcall;

	MSList* lcs=ms_list_append(NULL,marie->lc);
	const MSList* calls;

	linphone_core_use_files (pauline->lc,TRUE);
	linphone_core_use_files (laure->lc,TRUE);

	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);

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
	/*marie pause laure*/
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
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransferCallOutgoingProgress,1,2000));

	/*laure accept call*/
	for(calls=linphone_core_get_calls(laure->lc);calls!=NULL;calls=calls->next) {
		lcall = (LinphoneCall*)calls->data;
		if (linphone_call_get_state(lcall) == LinphoneCallIncomingReceived) {
			CU_ASSERT_EQUAL(linphone_call_get_replaced_call(lcall),laure_called_by_marie);
			linphone_core_accept_call(laure->lc,lcall);
			break;
		}
	}
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallConnected,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransferCallConnected,1,2000));

	/*terminate marie to pauline/laure call*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,2,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,1,2000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	ms_list_free(lcs);
}

static void check_call_state(LinphoneCoreManager* mgr,LinphoneCallState state) {
	CU_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(mgr->lc));
	if (linphone_core_get_current_call(mgr->lc))
		CU_ASSERT_EQUAL(linphone_call_get_state(linphone_core_get_current_call(mgr->lc)),state);
}
static void call_established_with_rejected_info(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	int dummy=0;

	CU_ASSERT_TRUE(call(pauline,marie));

	sal_enable_unconditional_answer(marie->lc->sal,TRUE);
	linphone_call_send_info_message(linphone_core_get_current_call(pauline->lc),linphone_core_create_info_message(pauline->lc));

	wait_for_until(marie->lc,pauline->lc,&dummy,1,1000); /*just to sleep while iterating 1s*/

	sal_enable_unconditional_answer(marie->lc->sal,FALSE);

	linphone_call_send_info_message(linphone_core_get_current_call(pauline->lc),linphone_core_create_info_message(pauline->lc));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_inforeceived,1));
	CU_ASSERT_EQUAL(marie->stat.number_of_inforeceived,1);

	check_call_state(pauline,LinphoneCallStreamsRunning);
	check_call_state(marie,LinphoneCallStreamsRunning);

	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


static void call_established_with_rejected_reinvite(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	CU_ASSERT_TRUE(call(pauline,marie));

	linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMU",8000,1),FALSE); /*disable PCMU*/
	linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMA",8000,1),TRUE); /*enable PCMA*/


	linphone_core_update_call(	pauline->lc
								,linphone_core_get_current_call(pauline->lc)
								,linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc)));


	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));

	CU_ASSERT_EQUAL(linphone_call_get_reason(linphone_core_get_current_call(pauline->lc)),LinphoneReasonNotAcceptable);

	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallStreamsRunning,1);
	check_call_state(pauline,LinphoneCallStreamsRunning);
	check_call_state(marie,LinphoneCallStreamsRunning);

	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_established_with_rejected_incoming_reinvite(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	CU_ASSERT_TRUE(call(pauline,marie));

	/*wait for ACK to be transmitted before going to reINVITE*/
	wait_for_until(marie->lc,pauline->lc,NULL,0,1000);

	linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMU",8000,1),FALSE); /*disable PCMU*/
	linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMA",8000,1),TRUE); /*enable PCMA*/

	linphone_core_update_call(marie->lc
				,linphone_core_get_current_call(marie->lc)
				,linphone_call_get_current_params(linphone_core_get_current_call(marie->lc)));


	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallUpdating,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));

	CU_ASSERT_EQUAL(linphone_call_get_reason(linphone_core_get_current_call(marie->lc)),LinphoneReasonNotAcceptable);

	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallStreamsRunning,1);
	check_call_state(pauline,LinphoneCallStreamsRunning);
	check_call_state(marie,LinphoneCallStreamsRunning);

	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_redirect(void){
	char hellopath[256];
	LinphoneCoreManager* marie   = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("pauline_rc");
	LinphoneCoreManager* laure   = linphone_core_manager_new("laure_rc");
	MSList* lcs = NULL;
	char *margaux_url = NULL;
	LinphoneCall* marie_call;

	lcs = ms_list_append(lcs,marie->lc);
	lcs = ms_list_append(lcs,pauline->lc);
	lcs = ms_list_append(lcs,laure->lc);
	/*
		Marie calls Pauline, which will redirect the call to Laure via a 302
	*/

	/*use playfile for callee to avoid locking on capture card*/
	linphone_core_use_files (pauline->lc,TRUE);
	linphone_core_use_files (laure->lc,TRUE);
	snprintf(hellopath,sizeof(hellopath), "%s/sounds/hello8000.wav", liblinphone_tester_file_prefix);
	linphone_core_set_play_file(pauline->lc,hellopath);
	linphone_core_set_play_file(laure->lc,hellopath);

	marie_call = linphone_core_invite_address(marie->lc, pauline->identity);

	CU_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingReceived,1,1000));

	margaux_url = linphone_address_as_string(laure->identity);
	linphone_core_redirect_call(pauline->lc, linphone_core_get_current_call(pauline->lc), margaux_url);
	ms_free(margaux_url);

	/* laure should be ringing now */
	CU_ASSERT_TRUE(wait_for_list(lcs, &laure->stat.number_of_LinphoneCallIncomingReceived,1,6000));
	/* pauline should have ended the call */
	CU_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallEnd,1,1000));
	/* the call should still be ringing on marie's side */
	CU_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingRinging, 1,1000));

	linphone_core_accept_call(laure->lc, linphone_core_get_current_call(laure->lc));

	CU_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallStreamsRunning, 1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs, &laure->stat.number_of_LinphoneCallStreamsRunning, 1,1000));

	CU_ASSERT_EQUAL(marie_call, linphone_core_get_current_call(marie->lc));

	liblinphone_tester_check_rtcp(marie, laure);

	linphone_core_terminate_all_calls(laure->lc);

	CU_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));

	ms_list_free(lcs);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);

}

static void call_established_with_rejected_reinvite_with_error(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	CU_ASSERT_TRUE(call(pauline,marie));

	linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMA",8000,1),TRUE); /*add PCMA*/


	sal_enable_unconditional_answer(marie->lc->sal,TRUE);

	linphone_core_update_call(	pauline->lc
					,linphone_core_get_current_call(pauline->lc)
					,linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc)));


	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));

	CU_ASSERT_EQUAL(linphone_call_get_reason(linphone_core_get_current_call(pauline->lc)),LinphoneReasonTemporarilyUnavailable); /*might be change later*/

	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallStreamsRunning,1);
	check_call_state(pauline,LinphoneCallStreamsRunning);
	check_call_state(marie,LinphoneCallStreamsRunning);

	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_rejected_because_wrong_credentials_with_params(const char* user_agent,bool_t enable_auth_req_cb) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneAuthInfo* good_auth_info=linphone_auth_info_clone(linphone_core_find_auth_info(marie->lc,NULL,linphone_address_get_username(marie->identity),NULL));
	LinphoneAuthInfo* wrong_auth_info=linphone_auth_info_clone(good_auth_info);
	bool_t result=FALSE;
	linphone_auth_info_set_passwd(wrong_auth_info,"passecretdutout");
	linphone_core_clear_all_auth_info(marie->lc);

	if (user_agent) {
		linphone_core_set_user_agent(marie->lc,user_agent,NULL);
	}
	if (!enable_auth_req_cb) {

		((LinphoneCoreVTable*)(marie->lc->vtables->data))->auth_info_requested=NULL;

		linphone_core_add_auth_info(marie->lc,wrong_auth_info);
	}

	CU_ASSERT_PTR_NOT_NULL(linphone_core_invite_address(marie->lc,marie->identity));

	result=wait_for(marie->lc,marie->lc,&marie->stat.number_of_auth_info_requested,1);

	if (enable_auth_req_cb) {
		CU_ASSERT_TRUE(result);
		/*automatically re-inititae the call*/
		linphone_core_add_auth_info(marie->lc,wrong_auth_info);
	}

	CU_ASSERT_TRUE(wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphoneCallError,1));
	if (enable_auth_req_cb) {
		CU_ASSERT_EQUAL(marie->stat.number_of_auth_info_requested,2);
	}
	/*to make sure unregister will work*/
	linphone_core_clear_all_auth_info(marie->lc);
	linphone_core_add_auth_info(marie->lc,good_auth_info);
	linphone_auth_info_destroy(good_auth_info);
	linphone_core_manager_destroy(marie);
}

static void call_rejected_because_wrong_credentials() {
	call_rejected_because_wrong_credentials_with_params(NULL,TRUE);
}

static void call_rejected_without_403_because_wrong_credentials() {
	call_rejected_because_wrong_credentials_with_params("tester-no-403",TRUE);
}

static void call_rejected_without_403_because_wrong_credentials_no_auth_req_cb() {
	call_rejected_because_wrong_credentials_with_params("tester-no-403",FALSE);
}

#ifdef VIDEO_ENABLED
/*this is call forking with early media managed at client side (not by flexisip server)*/
static void multiple_early_media(void) {
	LinphoneCoreManager* marie1 = linphone_core_manager_new("marie_early_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new("marie_early_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("pauline_tcp_rc");
	MSList *lcs=NULL;
	LinphoneCallParams *params=linphone_core_create_default_call_parameters(pauline->lc);
	LinphoneVideoPolicy pol;
	LinphoneCall *marie1_call;
	LinphoneCall *marie2_call;
	LinphoneCall *pauline_call;
	LinphoneInfoMessage *info;
	int dummy=0;
	char ringbackpath[256];
	snprintf(ringbackpath,sizeof(ringbackpath), "%s/sounds/hello8000.wav" /*use hello because rinback is too short*/, liblinphone_tester_file_prefix);

	pol.automatically_accept=1;
	pol.automatically_initiate=1;

	linphone_core_enable_video(pauline->lc,TRUE,TRUE);

	linphone_core_enable_video(marie1->lc,TRUE,TRUE);
	linphone_core_set_video_policy(marie1->lc,&pol);
	/*use playfile for marie1 to avoid locking on capture card*/
	linphone_core_use_files(marie1->lc,TRUE);
	linphone_core_set_play_file(marie1->lc,ringbackpath);

	linphone_core_enable_video(marie2->lc,TRUE,TRUE);
	linphone_core_set_video_policy(marie2->lc,&pol);
	linphone_core_set_audio_port_range(marie2->lc,40200,40300);
	linphone_core_set_video_port_range(marie2->lc,40400,40500);
	/*use playfile for marie2 to avoid locking on capture card*/
	linphone_core_use_files(marie2->lc,TRUE);
	linphone_core_set_play_file(marie2->lc,ringbackpath);


	lcs=ms_list_append(lcs,marie1->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	lcs=ms_list_append(lcs,pauline->lc);

	linphone_call_params_enable_early_media_sending(params,TRUE);
	linphone_call_params_enable_video(params,TRUE);

	linphone_core_invite_address_with_params(pauline->lc,marie1->identity,params);
	linphone_call_params_destroy(params);

	CU_ASSERT_TRUE(wait_for_list(lcs, &marie1->stat.number_of_LinphoneCallIncomingEarlyMedia,1,3000));
	CU_ASSERT_TRUE(wait_for_list(lcs, &marie2->stat.number_of_LinphoneCallIncomingEarlyMedia,1,3000));
	CU_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallOutgoingEarlyMedia,1,3000));

	pauline_call=linphone_core_get_current_call(pauline->lc);
	marie1_call=linphone_core_get_current_call(marie1->lc);
	marie2_call=linphone_core_get_current_call(marie2->lc);

	/*wait a bit that streams are established*/
	wait_for_list(lcs,&dummy,1,3000);
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(pauline_call)->download_bandwidth>70);
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(marie1_call)->download_bandwidth>70);
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(marie2_call)->download_bandwidth>70);

	linphone_core_accept_call(marie1->lc,linphone_core_get_current_call(marie1->lc));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie1->stat.number_of_LinphoneCallStreamsRunning,1,3000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,3000));

	/*marie2 should get her call terminated*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));

	/*wait a bit that streams are established*/
	wait_for_list(lcs,&dummy,1,1000);
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(pauline_call)->download_bandwidth>71);
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(marie1_call)->download_bandwidth>71);

	/*send an INFO in reverse side to check that dialogs are properly established*/
	info=linphone_core_create_info_message(marie1->lc);
	linphone_call_send_info_message(marie1_call,info);
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_inforeceived,1,2000));

	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie1->stat.number_of_LinphoneCallEnd,1,2000));

	ms_list_free(lcs);
	linphone_core_manager_destroy(marie1);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(pauline);
}
#endif

static void profile_call(bool_t avpf1, bool_t srtp1, bool_t avpf2, bool_t srtp2, const char *expected_profile) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_rc");
	LinphoneProxyConfig *lpc;
	const LinphoneCallParams *params;

	if (avpf1) {
		linphone_core_get_default_proxy(marie->lc, &lpc);
		linphone_proxy_config_enable_avpf(lpc, TRUE);
		linphone_proxy_config_set_avpf_rr_interval(lpc, 3);
	}
	if (avpf2) {
		linphone_core_get_default_proxy(pauline->lc, &lpc);
		linphone_proxy_config_enable_avpf(lpc, TRUE);
		linphone_proxy_config_set_avpf_rr_interval(lpc, 3);
	}
	if (srtp1) {
		if (linphone_core_media_encryption_supported(marie->lc, LinphoneMediaEncryptionSRTP)) {
			linphone_core_set_media_encryption(marie->lc, LinphoneMediaEncryptionSRTP);
		}
	}
	if (srtp2) {
		if (linphone_core_media_encryption_supported(pauline->lc, LinphoneMediaEncryptionSRTP)) {
			linphone_core_set_media_encryption(pauline->lc, LinphoneMediaEncryptionSRTP);
		}
	}

	CU_ASSERT_TRUE(call(marie, pauline));
	CU_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 1));
	CU_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 1));
	params = linphone_call_get_current_params(linphone_core_get_current_call(marie->lc));
	CU_ASSERT_STRING_EQUAL(linphone_call_params_get_rtp_profile(params), expected_profile);
	params = linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc));
	CU_ASSERT_STRING_EQUAL(linphone_call_params_get_rtp_profile(params), expected_profile);

	linphone_core_terminate_all_calls(marie->lc);
	CU_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallEnd, 1));
	CU_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallEnd, 1));
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallConnected, 1);
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallConnected, 1);

	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
}

static void avp_to_avp_call(void) {
	profile_call(FALSE, FALSE, FALSE, FALSE, "RTP/AVP");
}

static void avp_to_avpf_call(void) {
	profile_call(FALSE, FALSE, TRUE, FALSE, "RTP/AVP");
}

static void avp_to_savp_call(void) {
	profile_call(FALSE, FALSE, FALSE, TRUE, "RTP/AVP");
}

static void avp_to_savpf_call(void) {
	profile_call(FALSE, FALSE, TRUE, TRUE, "RTP/AVP");
}

static void avpf_to_avp_call(void) {
	profile_call(TRUE, FALSE, FALSE, FALSE, "RTP/AVPF");
}

static void avpf_to_avpf_call(void) {
	profile_call(TRUE, FALSE, TRUE, FALSE, "RTP/AVPF");
}

static void avpf_to_savp_call(void) {
	profile_call(TRUE, FALSE, FALSE, TRUE, "RTP/AVPF");
}

static void avpf_to_savpf_call(void) {
	profile_call(TRUE, FALSE, TRUE, TRUE, "RTP/AVPF");
}

static void savp_to_avp_call(void) {
	profile_call(FALSE, TRUE, FALSE, FALSE, "RTP/SAVP");
}

static void savp_to_avpf_call(void) {
	profile_call(FALSE, TRUE, TRUE, FALSE, "RTP/SAVP");
}

static void savp_to_savp_call(void) {
	profile_call(FALSE, TRUE, FALSE, TRUE, "RTP/SAVP");
}

static void savp_to_savpf_call(void) {
	profile_call(FALSE, TRUE, TRUE, TRUE, "RTP/SAVP");
}

static void savpf_to_avp_call(void) {
	profile_call(TRUE, TRUE, FALSE, FALSE, "RTP/SAVPF");
}

static void savpf_to_avpf_call(void) {
	profile_call(TRUE, TRUE, TRUE, FALSE, "RTP/SAVPF");
}

static void savpf_to_savp_call(void) {
	profile_call(TRUE, TRUE, FALSE, TRUE, "RTP/SAVPF");
}

static void savpf_to_savpf_call(void) {
	profile_call(TRUE, TRUE, TRUE, TRUE, "RTP/SAVPF");
}

static char *create_filepath(const char *dir, const char *filename, const char *ext) {
	return ms_strdup_printf("%s/%s.%s",dir,filename,ext);
}

static void record_call(const char *filename, bool_t enableVideo) {
	LinphoneCoreManager *marie = NULL;
	LinphoneCoreManager *pauline = NULL;
	LinphoneCallParams *marieParams = NULL;
	LinphoneCallParams *paulineParams = NULL;
	LinphoneCall *callInst = NULL;
	const char **formats, *format;
	char *filepath;
	int dummy=0, i;

#if defined(HAVE_OPENH264) && defined(ANDROID)
	ms_init();
	libmsopenh264_init();
#endif


	marie = linphone_core_manager_new("marie_h264_rc");
	pauline = linphone_core_manager_new("pauline_h264_rc");
	marieParams = linphone_core_create_default_call_parameters(marie->lc);
	paulineParams = linphone_core_create_default_call_parameters(pauline->lc);

#ifdef VIDEO_ENABLED
	if(enableVideo) {
		if((linphone_core_find_payload_type(marie->lc, "H264", -1, -1) != NULL)
				&& (linphone_core_find_payload_type(pauline->lc, "H264", -1, -1) != NULL)) {
			linphone_call_params_enable_video(marieParams, TRUE);
			linphone_call_params_enable_video(paulineParams, TRUE);
			disable_all_video_codecs_except_one(marie->lc, "H264");
			disable_all_video_codecs_except_one(pauline->lc, "H264");
		} else {
			ms_warning("call_recording(): the H264 payload has not been found. Only sound will be recorded");
		}
	}
#endif

	formats = linphone_core_get_supported_file_formats(marie->lc);

	for(i=0, format = formats[0]; format != NULL; i++, format = formats[i]) {
		filepath = create_filepath(liblinphone_tester_writable_dir_prefix, filename, format);
		remove(filepath);
		linphone_call_params_set_record_file(marieParams, filepath);
		if((CU_ASSERT_TRUE(call_with_params(marie, pauline, marieParams, paulineParams)))
				&& (CU_ASSERT_PTR_NOT_NULL(callInst = linphone_core_get_current_call(marie->lc)))) {

			ms_message("call_recording(): start recording into %s", filepath);
			linphone_call_start_recording(callInst);
			wait_for_until(marie->lc,pauline->lc,&dummy,1,5000);
			linphone_call_stop_recording(callInst);
			end_call(marie, pauline);
			CU_ASSERT_EQUAL(access(filepath, F_OK), 0);
		}
		remove(filepath);
		ms_free(filepath);
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
#if defined(HAVE_OPENH264) && defined(ANDROID)
	ms_exit();
#endif
}

static void audio_call_recording_test(void) {
	record_call("recording", FALSE);
}

#ifdef VIDEO_ENABLED
static void video_call_recording_test(void) {
	record_call("recording", TRUE);
}

static void video_call_snapshot(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_rc");
	LinphoneCallParams *marieParams = linphone_core_create_default_call_parameters(marie->lc);
	LinphoneCallParams *paulineParams = linphone_core_create_default_call_parameters(pauline->lc);
	LinphoneCall *callInst = NULL;
	char *filename = create_filepath(liblinphone_tester_writable_dir_prefix, "snapshot", "jpeg");
	int dummy = 0;

	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, TRUE);
	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, FALSE);
	linphone_call_params_enable_video(marieParams, TRUE);
	linphone_call_params_enable_video(paulineParams, TRUE);

	if((CU_ASSERT_TRUE(call_with_params(marie, pauline, marieParams, paulineParams)))
			&& (CU_ASSERT_PTR_NOT_NULL(callInst = linphone_core_get_current_call(marie->lc)))) {
		linphone_call_take_video_snapshot(callInst, filename);
		wait_for_until(marie->lc, pauline->lc, &dummy, 1, 5000);
		CU_ASSERT_EQUAL(access(filename, F_OK), 0);
		remove(filename);
	}
	ms_free(filename);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#endif

static void call_with_in_dialog_update(void) {
	int begin;
	int leaked_objects;
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCallParams *params;

	belle_sip_object_enable_leak_detector(TRUE);
	begin=belle_sip_object_get_object_count();

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new( "pauline_rc");
	CU_ASSERT_TRUE(call(pauline,marie));
	liblinphone_tester_check_rtcp(marie,pauline);
	params=linphone_core_create_call_params(marie->lc,linphone_core_get_current_call(marie->lc));
	params->no_user_consent=TRUE;
	linphone_core_update_call(marie->lc,linphone_core_get_current_call(marie->lc),params);
	linphone_call_params_destroy(params);
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallUpdating,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallUpdatedByRemote,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	end_call(marie,pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);

	leaked_objects=belle_sip_object_get_object_count()-begin;
	CU_ASSERT_TRUE(leaked_objects==0);
	if (leaked_objects>0){
		belle_sip_object_dump_active_objects();
	}
}
static void call_with_in_dialog_codec_change_base(bool_t no_sdp) {
	int begin;
	int leaked_objects;
	int dummy=0;
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCallParams *params;

	belle_sip_object_enable_leak_detector(TRUE);
	begin=belle_sip_object_get_object_count();

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new( "pauline_rc");
	CU_ASSERT_TRUE(call(pauline,marie));
	liblinphone_tester_check_rtcp(marie,pauline);
	params=linphone_core_create_call_params(marie->lc,linphone_core_get_current_call(marie->lc));

	linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMU",8000,1),FALSE); /*disable PCMU*/
	linphone_core_enable_payload_type(marie->lc,linphone_core_find_payload_type(marie->lc,"PCMU",8000,1),FALSE); /*disable PCMU*/
	linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMA",8000,1),TRUE); /*enable PCMA*/
	linphone_core_enable_payload_type(marie->lc,linphone_core_find_payload_type(marie->lc,"PCMA",8000,1),TRUE); /*enable PCMA*/
	if (no_sdp) {
		linphone_core_enable_sdp_200_ack(marie->lc,TRUE);
	}
	linphone_core_update_call(marie->lc,linphone_core_get_current_call(marie->lc),params);
	linphone_call_params_destroy(params);
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallUpdating,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallUpdatedByRemote,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	CU_ASSERT_STRING_EQUAL("PCMA",linphone_payload_type_get_mime_type(linphone_call_params_get_used_audio_codec(linphone_call_get_current_params(linphone_core_get_current_call(marie->lc)))));
	wait_for_until(marie->lc, pauline->lc, &dummy, 1, 3000);
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(linphone_core_get_current_call(marie->lc))->download_bandwidth>70);
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(linphone_core_get_current_call(pauline->lc))->download_bandwidth>70);

	end_call(marie,pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);

	leaked_objects=belle_sip_object_get_object_count()-begin;
	CU_ASSERT_TRUE(leaked_objects==0);
	if (leaked_objects>0){
		belle_sip_object_dump_active_objects();
	}
}
static void call_with_in_dialog_codec_change(void) {
	call_with_in_dialog_codec_change_base(FALSE);
}
static void call_with_in_dialog_codec_change_no_sdp(void) {
	call_with_in_dialog_codec_change_base(TRUE);
}
static void call_with_custom_supported_tags(void) {
	int begin;
	int leaked_objects;
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	const LinphoneCallParams *remote_params;
	const char *recv_supported;

	belle_sip_object_enable_leak_detector(TRUE);
	begin=belle_sip_object_get_object_count();

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new( "pauline_rc");

	linphone_core_add_supported_tag(marie->lc,"pouet-tag");
	CU_ASSERT_TRUE(call(pauline,marie));
	liblinphone_tester_check_rtcp(marie,pauline);
	remote_params=linphone_call_get_remote_params(linphone_core_get_current_call(pauline->lc));
	recv_supported=linphone_call_params_get_custom_header(remote_params,"supported");
	CU_ASSERT_PTR_NOT_NULL(recv_supported);
	if (recv_supported){
		CU_ASSERT_TRUE(strstr(recv_supported,"pouet-tag")!=NULL);
	}
	end_call(marie,pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);

	leaked_objects=belle_sip_object_get_object_count()-begin;
	CU_ASSERT_TRUE(leaked_objects==0);
	if (leaked_objects>0){
		belle_sip_object_dump_active_objects();
	}
}

static void call_log_from_taken_from_p_asserted_id(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneCall *c1,*c2;
	LinphoneCallParams *params;
	const char* paulie_asserted_id ="\"Paupauche\" <sip:pauline@super.net>";
	LinphoneAddress *paulie_asserted_id_addr = linphone_address_new(paulie_asserted_id);
	LpConfig *marie_lp;

	params=linphone_core_create_default_call_parameters(pauline->lc);

	linphone_call_params_add_custom_header(params,"P-Asserted-Identity",paulie_asserted_id);
	/*fixme, should be able to add several time the same header linphone_call_params_add_custom_header(params,"P-Asserted-Identity","\"Paupauche\" <tel:+12345>");*/

	marie_lp = linphone_core_get_config(marie->lc);
	lp_config_set_int(marie_lp,"sip","call_logs_use_asserted_id_instead_of_from",1);


	CU_ASSERT_TRUE(call_with_caller_params(pauline,marie,params));
	linphone_call_params_destroy(params);

	c1=linphone_core_get_current_call(pauline->lc);
	c2=linphone_core_get_current_call(marie->lc);

	CU_ASSERT_PTR_NOT_NULL(c1);
	CU_ASSERT_PTR_NOT_NULL(c2);

	/*make sure remote identity is hidden*/
	CU_ASSERT_TRUE(linphone_address_weak_equal(linphone_call_get_remote_address(c2),paulie_asserted_id_addr));


	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void incoming_invite_without_sdp()  {
	LinphoneCoreManager* caller = linphone_core_manager_new( "pauline_rc");
	LinphoneCoreManager* callee = linphone_core_manager_new( "marie_rc");
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params = {0};

	callee_test_params.sdp_removal = TRUE;
	CU_ASSERT_FALSE(call_with_params2(caller,callee,&caller_test_params, &callee_test_params, FALSE));

	CU_ASSERT_PTR_NULL(linphone_core_get_current_call(callee->lc));
	CU_ASSERT_EQUAL(caller->stat.number_of_LinphoneCallError,1);
	/*call will be drop before presented to the application, because it is invalid*/
	CU_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallIncomingReceived,0);

	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);
}

static void outgoing_invite_without_sdp()  {
	LinphoneCoreManager* caller = linphone_core_manager_new( "pauline_rc");
	LinphoneCoreManager* callee = linphone_core_manager_new( "marie_rc");
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params = {0};

	caller_test_params.sdp_removal = TRUE;
	CU_ASSERT_FALSE(call_with_params2(caller,callee,&caller_test_params, &callee_test_params, FALSE));

	CU_ASSERT_PTR_NULL(linphone_core_get_current_call(callee->lc));
	CU_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallIncomingReceived,1);
	CU_ASSERT_EQUAL(caller->stat.number_of_LinphoneCallError,1);
	// actually callee does not receive error, because it just get a BYE from the other part
	CU_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallError,0);
	CU_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallEnd,1);

	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);
}

static void incoming_reinvite_without_ack_sdp()  {
#ifdef VIDEO_ENABLED
	LinphoneCoreManager* caller = linphone_core_manager_new( "pauline_rc");
	LinphoneCoreManager* callee = linphone_core_manager_new( "marie_rc");
	LinphoneCall * inc_call;
	CU_ASSERT_TRUE(call(caller,callee));
	inc_call = linphone_core_get_current_call(callee->lc);

	CU_ASSERT_PTR_NOT_NULL(inc_call);
	if (inc_call) {
		const LinphoneCallParams *caller_params;
		stats initial_caller_stat=caller->stat;
		stats initial_callee_stat=callee->stat;
		sal_call_enable_sdp_removal(inc_call->op, TRUE);
 		CU_ASSERT_PTR_NOT_NULL(setup_video(caller, callee));
		CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallUpdating,initial_callee_stat.number_of_LinphoneCallUpdating+1));
		CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallStreamsRunning,initial_callee_stat.number_of_LinphoneCallStreamsRunning+1));
		CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&caller->stat.number_of_LinphoneCallStreamsRunning,initial_caller_stat.number_of_LinphoneCallStreamsRunning));
		/*Basically the negotiation failed but since the call was already running, we expect it to restore to
		the previous state so error stats should not be changed*/
		CU_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallError,initial_callee_stat.number_of_LinphoneCallError);
		/*and remote should have received an update notification*/
		CU_ASSERT_EQUAL(caller->stat.number_of_LinphoneCallUpdatedByRemote,initial_caller_stat.number_of_LinphoneCallUpdatedByRemote+1);


		CU_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(callee->lc))));
		caller_params = linphone_call_get_current_params(linphone_core_get_current_call(caller->lc));
		CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,(int*)&caller_params->has_video,FALSE));

		sal_call_enable_sdp_removal(inc_call->op, FALSE);
	}
	linphone_core_terminate_all_calls(caller->lc);
	CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&caller->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);
#else
	ms_warning("not tested because video not available");
#endif
}

static void outgoing_reinvite_without_ack_sdp()  {
#ifdef VIDEO_ENABLED
	LinphoneCoreManager* caller = linphone_core_manager_new( "pauline_rc");
	LinphoneCoreManager* callee = linphone_core_manager_new( "marie_rc");
	LinphoneCall * out_call;
	CU_ASSERT_TRUE(call(caller,callee));
	out_call = linphone_core_get_current_call(caller->lc);

	CU_ASSERT_PTR_NOT_NULL(out_call);
	if (out_call) {
		stats initial_caller_stat=caller->stat;
		stats initial_callee_stat=callee->stat;
		sal_call_enable_sdp_removal(out_call->op, TRUE);
 		CU_ASSERT_PTR_NOT_NULL(setup_video(caller, callee));
		CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallUpdating,initial_callee_stat.number_of_LinphoneCallUpdating+1));
		CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallStreamsRunning,initial_callee_stat.number_of_LinphoneCallStreamsRunning+1));
		CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&caller->stat.number_of_LinphoneCallStreamsRunning,initial_caller_stat.number_of_LinphoneCallStreamsRunning));
		/*Basically the negotiation failed but since the call was already running, we expect it to restore to
		the previous state so error stats should not be changed*/
		CU_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallError,initial_callee_stat.number_of_LinphoneCallError);
		/*and remote should not have received any update notification*/
		CU_ASSERT_EQUAL(caller->stat.number_of_LinphoneCallUpdatedByRemote,initial_caller_stat.number_of_LinphoneCallUpdatedByRemote);

		CU_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(callee->lc))));
		CU_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(caller->lc))));

		sal_call_enable_sdp_removal(out_call->op, FALSE);
	}
	linphone_core_terminate_all_calls(caller->lc);
	CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&caller->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);
#else
	ms_warning("not tested because video not available");
#endif
}

test_t call_tests[] = {
	{ "Early declined call", early_declined_call },
	{ "Call declined", call_declined },
	{ "Cancelled call", cancelled_call },
	{ "Early cancelled call", early_cancelled_call},
	{ "Call with DNS timeout", call_with_dns_time_out },
	{ "Cancelled ringing call", cancelled_ringing_call },
	{ "Call failed because of codecs", call_failed_because_of_codecs },
	{ "Simple call", simple_call },
	{ "Direct call over IPv6", direct_call_over_ipv6},
	{ "Outbound call with multiple proxy possible", call_outbound_with_multiple_proxy },
	{ "Audio call recording", audio_call_recording_test },
#if 0 /* not yet activated because not implemented */
	{ "Multiple answers to a call", multiple_answers_call },
#endif
	{ "Multiple answers to a call with media relay", multiple_answers_call_with_media_relay },
	{ "Call with media relay", call_with_media_relay},
	{ "Call with media relay (random ports)", call_with_media_relay_random_ports},
	{ "Simple call compatibility mode", simple_call_compatibility_mode },
	{ "Early-media call", early_media_call },
	{ "Early-media call with ringing", early_media_call_with_ringing },
	{ "Early-media call with updated media session", early_media_call_with_session_update},
	{ "Early-media call with updated codec", early_media_call_with_codec_update},
	{ "Call terminated by caller", call_terminated_by_caller },
	{ "Call without SDP", call_with_no_sdp},
	{ "Call without SDP and ACK without SDP", call_with_no_sdp_ack_without_sdp},
	{ "Call paused resumed", call_paused_resumed },
	{ "Call paused resumed with loss", call_paused_resumed_with_loss },
	{ "Call paused resumed from callee", call_paused_resumed_from_callee },
	{ "SRTP call", srtp_call },
	{ "ZRTP call",zrtp_call},
	{ "ZRTP video call",zrtp_video_call},
	{ "SRTP call with declined srtp", call_with_declined_srtp },
	{ "Call with file player", call_with_file_player},
	{ "Call with mkv file player", call_with_mkv_file_player},
#ifdef VIDEO_ENABLED
	{ "Simple video call",video_call},
	{ "Simple video call using policy",video_call_using_policy},
	{ "Video call without SDP",video_call_no_sdp},
	{ "SRTP ice video call", srtp_video_ice_call },
	{ "ZRTP ice video call", zrtp_video_ice_call },
	{ "Call with video added", call_with_video_added },
	{ "Call with video added (random ports)", call_with_video_added_random_ports },
	{ "Call with several video switches", call_with_several_video_switches },
	{ "SRTP call with several video switches", srtp_call_with_several_video_switches },
	{ "Call with video declined", call_with_declined_video},
	{ "Call with video declined using policy", call_with_declined_video_using_policy},
	{ "Call with multiple early media", multiple_early_media },
	{ "Call with ICE from video to non-video", call_with_ice_video_to_novideo},
	{ "Call with ICE and video added", call_with_ice_video_added },
	{ "Video call with ICE no matching audio codecs", video_call_with_ice_no_matching_audio_codecs },
	{ "Video call recording", video_call_recording_test },
	{ "Snapshot", video_call_snapshot },
#endif
	{ "SRTP ice call", srtp_ice_call },
	{ "ZRTP ice call", zrtp_ice_call },
	{ "ZRTP ice call with relay", zrtp_ice_call_with_relay},
	{ "Call with privacy", call_with_privacy },
	{ "Call with privacy 2", call_with_privacy2 },
	{ "Call rejected because of wrong credential", call_rejected_because_wrong_credentials},
	{ "Call rejected without 403 because of wrong credential", call_rejected_without_403_because_wrong_credentials},
	{ "Call rejected without 403 because of wrong credential and no auth req cb", call_rejected_without_403_because_wrong_credentials_no_auth_req_cb},
	{ "Call waiting indication", call_waiting_indication },
	{ "Call waiting indication with privacy", call_waiting_indication_with_privacy },
	{ "Simple conference", simple_conference },
	{ "Simple conference with ICE",simple_conference_with_ice},
	{ "Simple call transfer", simple_call_transfer },
	{ "Unattended call transfer", unattended_call_transfer },
	{ "Unattended call transfer with error", unattended_call_transfer_with_error },
	{ "Call transfer existing call outgoing call", call_transfer_existing_call_outgoing_call },
	{ "Call with ICE", call_with_ice },
	{ "Call with ICE without SDP", call_with_ice_no_sdp },
	{ "Call with ICE (random ports)", call_with_ice_random_ports },
	{ "Call from ICE to not ICE",ice_to_not_ice},
	{ "Call from not ICE to ICE",not_ice_to_ice},
	{ "Call with custom headers",call_with_custom_headers},
	{ "Call established with rejected INFO",call_established_with_rejected_info},
	{ "Call established with rejected RE-INVITE",call_established_with_rejected_reinvite},
	{ "Call established with rejected incoming RE-INVITE", call_established_with_rejected_incoming_reinvite },
	{ "Call established with rejected RE-INVITE in error", call_established_with_rejected_reinvite_with_error},
	{ "Call redirected by callee", call_redirect},
	{ "Call with specified codec bitrate", call_with_specified_codec_bitrate},
	{ "AVP to AVP call", avp_to_avp_call },
	{ "AVP to AVPF call", avp_to_avpf_call },
	{ "AVP to SAVP call", avp_to_savp_call },
	{ "AVP to SAVPF call", avp_to_savpf_call },
	{ "AVPF to AVP call", avpf_to_avp_call },
	{ "AVPF to AVPF call", avpf_to_avpf_call },
	{ "AVPF to SAVP call", avpf_to_savp_call },
	{ "AVPF to SAVPF call", avpf_to_savpf_call },
	{ "SAVP to AVP call", savp_to_avp_call },
	{ "SAVP to AVPF call", savp_to_avpf_call },
	{ "SAVP to SAVP call", savp_to_savp_call },
	{ "SAVP to SAVPF call", savp_to_savpf_call },
	{ "SAVPF to AVP call", savpf_to_avp_call },
	{ "SAVPF to AVPF call", savpf_to_avpf_call },
	{ "SAVPF to SAVP call", savpf_to_savp_call },
	{ "SAVPF to SAVPF call", savpf_to_savpf_call },
	{ "Call with in-dialog UPDATE request", call_with_in_dialog_update },
	{ "Call with in-dialog codec change", call_with_in_dialog_codec_change },
	{ "Call with in-dialog codec change no sdp", call_with_in_dialog_codec_change_no_sdp },
	{ "Call with custom supported tags", call_with_custom_supported_tags },
	{ "Call log from taken from asserted id",call_log_from_taken_from_p_asserted_id},
	{ "Incoming INVITE without SDP",incoming_invite_without_sdp},
	{ "Outgoing INVITE without ACK SDP",outgoing_invite_without_sdp},
	{ "Incoming REINVITE without SDP",incoming_reinvite_without_ack_sdp},
	{ "Outgoing REINVITE without ACK SDP",outgoing_reinvite_without_ack_sdp},
};

test_suite_t call_test_suite = {
	"Call",
	NULL,
	NULL,
	sizeof(call_tests) / sizeof(call_tests[0]),
	call_tests
};
