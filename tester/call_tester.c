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


#include <sys/types.h>
#include <sys/stat.h>
#include "linphonecore.h"
#include "lpconfig.h"
#include "private.h"
#include "liblinphone_tester.h"
#include "mediastreamer2/msutils.h"
#include "belle-sip/sipstack.h"

#ifdef _WIN32
#define unlink _unlink
#ifndef F_OK
#define F_OK 00 /*visual studio does not define F_OK*/
#endif
#endif

static void srtp_call(void);
static char *create_filepath(const char *dir, const char *filename, const char *ext);

// prototype definition for call_recording()
#ifdef ANDROID
#ifdef HAVE_OPENH264
extern void libmsopenh264_init(MSFactory *factory);
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
		BC_FAIL("unexpected event");break;
	}
}

void call_stats_updated(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallStats *lstats) {
	stats* counters = get_stats(lc);
	counters->number_of_LinphoneCallStatsUpdated++;
	if (lstats->updated & LINPHONE_CALL_STATS_RECEIVED_RTCP_UPDATE) {
		counters->number_of_rtcp_received++;
		if (lstats->rtcp_received_via_mux){
			counters->number_of_rtcp_received_via_mux++;
		}
	}
	if (lstats->updated & LINPHONE_CALL_STATS_SENT_RTCP_UPDATE ) {
		counters->number_of_rtcp_sent++;
	}
	if (lstats->updated & LINPHONE_CALL_STATS_PERIODICAL_UPDATE ) {
		int tab_size = sizeof (counters->audio_download_bandwidth)/sizeof(int);
		int index = (counters->current_bandwidth_index[lstats->type]++) % tab_size;
		if (lstats->type == LINPHONE_CALL_STATS_AUDIO) {
			counters->audio_download_bandwidth[index] = (int)linphone_call_get_audio_stats(call)->download_bandwidth;
			counters->audio_upload_bandwidth[index] = (int)linphone_call_get_audio_stats(call)->upload_bandwidth;
		} else {
			counters->video_download_bandwidth[index] = (int)linphone_call_get_video_stats(call)->download_bandwidth;
			counters->video_upload_bandwidth[index] = (int)linphone_call_get_video_stats(call)->upload_bandwidth;
		}
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
		BC_FAIL("unexpected event");break;
	}
}


void linphone_call_iframe_decoded_cb(LinphoneCall *call,void * user_data) {
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

void liblinphone_tester_check_rtcp(LinphoneCoreManager* caller, LinphoneCoreManager* callee) {
	LinphoneCall *c1,*c2;
	MSTimeSpec ts;
	int max_time_to_wait;
	c1=linphone_core_get_current_call(caller->lc);
	c2=linphone_core_get_current_call(callee->lc);

	BC_ASSERT_PTR_NOT_NULL(c1);
	BC_ASSERT_PTR_NOT_NULL(c2);

	if (!c1 || !c2) return;
	linphone_call_ref(c1);
	linphone_call_ref(c2);
	liblinphone_tester_clock_start(&ts);
	if (linphone_core_rtcp_enabled(caller->lc) && linphone_core_rtcp_enabled(callee->lc))
		max_time_to_wait = 15000;
	else
		max_time_to_wait = 5000;

	do {
		if (linphone_call_get_audio_stats(c1)->round_trip_delay > 0.0
			&& linphone_call_get_audio_stats(c2)->round_trip_delay > 0.0
			&& (!linphone_call_log_video_enabled(linphone_call_get_call_log(c1)) || linphone_call_get_video_stats(c1)->round_trip_delay>0.0)
			&& (!linphone_call_log_video_enabled(linphone_call_get_call_log(c2))  || linphone_call_get_video_stats(c2)->round_trip_delay>0.0)) {
			break;

		}
		wait_for_until(caller->lc,callee->lc,NULL,0,20); /*just to sleep while iterating*/
	}while (!liblinphone_tester_clock_elapsed(&ts,max_time_to_wait));

	if (linphone_core_rtcp_enabled(caller->lc) && linphone_core_rtcp_enabled(callee->lc)) {
		BC_ASSERT_GREATER(linphone_call_get_audio_stats(c1)->round_trip_delay,0.0,float,"%f");
		BC_ASSERT_GREATER(linphone_call_get_audio_stats(c2)->round_trip_delay,0.0,float,"%f");
		if (linphone_call_log_video_enabled(linphone_call_get_call_log(c1))) {
			BC_ASSERT_GREATER(linphone_call_get_video_stats(c1)->round_trip_delay,0.0,float,"%f");
		}
		if (linphone_call_log_video_enabled(linphone_call_get_call_log(c2))) {
			BC_ASSERT_GREATER(linphone_call_get_video_stats(c2)->round_trip_delay,0.0,float,"%f");
		}
	} else {
		if (linphone_core_rtcp_enabled(caller->lc)) {
			BC_ASSERT_EQUAL(linphone_call_get_audio_stats(c1)->rtp_stats.sent_rtcp_packets, 0, int, "%i");
			BC_ASSERT_EQUAL(linphone_call_get_audio_stats(c2)->rtp_stats.recv_rtcp_packets, 0, int, "%i");
			if (linphone_call_log_video_enabled(linphone_call_get_call_log(c1))) {
				BC_ASSERT_EQUAL(linphone_call_get_video_stats(c1)->rtp_stats.sent_rtcp_packets, 0, int, "%i");
			}
			if (linphone_call_log_video_enabled(linphone_call_get_call_log(c2))) {
				BC_ASSERT_EQUAL(linphone_call_get_video_stats(c2)->rtp_stats.recv_rtcp_packets, 0, int, "%i");
			}
		}
		if (linphone_core_rtcp_enabled(callee->lc)) {
		BC_ASSERT_EQUAL(linphone_call_get_audio_stats(c2)->rtp_stats.sent_rtcp_packets, 0, int, "%i");
		BC_ASSERT_EQUAL(linphone_call_get_audio_stats(c1)->rtp_stats.recv_rtcp_packets, 0, int, "%i");
			if (linphone_call_log_video_enabled(linphone_call_get_call_log(c1))) {
				BC_ASSERT_EQUAL(linphone_call_get_video_stats(c1)->rtp_stats.recv_rtcp_packets, 0, int, "%i");
			}
			if (linphone_call_log_video_enabled(linphone_call_get_call_log(c2))) {
				BC_ASSERT_EQUAL(linphone_call_get_video_stats(c2)->rtp_stats.sent_rtcp_packets, 0, int, "%i");
			}
		}

	}
	linphone_call_unref(c1);
	linphone_call_unref(c2);
}

static void setup_sdp_handling(const LinphoneCallTestParams* params, LinphoneCoreManager* mgr ){
	if( params->sdp_removal ){
		sal_default_set_sdp_handling(mgr->lc->sal, SalOpSDPSimulateRemove);
	} else if( params->sdp_simulate_error ){
		sal_default_set_sdp_handling(mgr->lc->sal, SalOpSDPSimulateError);
	}
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
	LinphoneCallParams *caller_params = caller_test_params->base;
	LinphoneCallParams *callee_params = callee_test_params->base;
	bool_t did_receive_call;
	LinphoneCall *callee_call=NULL;
	LinphoneCall *caller_call=NULL;

	setup_sdp_handling(caller_test_params, caller_mgr);
	setup_sdp_handling(callee_test_params, callee_mgr);

	if (!caller_params){
		BC_ASSERT_PTR_NOT_NULL((caller_call=linphone_core_invite_address(caller_mgr->lc,callee_mgr->identity)));
	}else{
		BC_ASSERT_PTR_NOT_NULL((caller_call=linphone_core_invite_address_with_params(caller_mgr->lc,callee_mgr->identity,caller_params)));
	}

	BC_ASSERT_PTR_NULL(linphone_call_get_remote_params(caller_call)); /*assert that remote params are NULL when no response is received yet*/

	did_receive_call = wait_for(callee_mgr->lc
				,caller_mgr->lc
				,&callee_mgr->stat.number_of_LinphoneCallIncomingReceived
				,initial_callee.number_of_LinphoneCallIncomingReceived+1);
	BC_ASSERT_EQUAL(did_receive_call, !callee_test_params->sdp_simulate_error, int, "%d");

	sal_default_set_sdp_handling(caller_mgr->lc->sal, SalOpSDPNormal);
	sal_default_set_sdp_handling(callee_mgr->lc->sal, SalOpSDPNormal);

	if (!did_receive_call) return 0;


	if (linphone_core_get_calls_nb(callee_mgr->lc)<=1)
		BC_ASSERT_TRUE(linphone_core_inc_invite_pending(callee_mgr->lc));
	BC_ASSERT_EQUAL(caller_mgr->stat.number_of_LinphoneCallOutgoingProgress,initial_caller.number_of_LinphoneCallOutgoingProgress+1, int, "%d");


	while (caller_mgr->stat.number_of_LinphoneCallOutgoingRinging!=(initial_caller.number_of_LinphoneCallOutgoingRinging + 1)
			&& caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia!=(initial_caller.number_of_LinphoneCallOutgoingEarlyMedia +1)
			&& retry++ < 100) {
			linphone_core_iterate(caller_mgr->lc);
			linphone_core_iterate(callee_mgr->lc);
			ms_usleep(20000);
	}


	BC_ASSERT_TRUE((caller_mgr->stat.number_of_LinphoneCallOutgoingRinging==initial_caller.number_of_LinphoneCallOutgoingRinging+1)
							||(caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia==initial_caller.number_of_LinphoneCallOutgoingEarlyMedia+1));


	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call_remote_address(callee_mgr->lc));
	callee_call=linphone_core_get_call_by_remote_address2(callee_mgr->lc,caller_mgr->identity);

	if(!linphone_core_get_current_call(caller_mgr->lc) || !linphone_core_get_current_call(callee_mgr->lc) || !linphone_core_get_current_call_remote_address(callee_mgr->lc)) {
		return 0;
	} else if (caller_mgr->identity){
		LinphoneAddress* callee_from=linphone_address_clone(caller_mgr->identity);
		linphone_address_set_port(callee_from,0); /*remove port because port is never present in from header*/

		if (linphone_call_params_get_privacy(linphone_call_get_current_params(linphone_core_get_current_call(caller_mgr->lc))) == LinphonePrivacyNone) {
			/*don't check in case of p asserted id*/
			if (!lp_config_get_int(callee_mgr->lc->config,"sip","call_logs_use_asserted_id_instead_of_from",0))
				BC_ASSERT_TRUE(linphone_address_weak_equal(callee_from,linphone_call_get_remote_address(callee_call)));
		} else {
			BC_ASSERT_FALSE(linphone_address_weak_equal(callee_from,linphone_call_get_remote_address(linphone_core_get_current_call(callee_mgr->lc))));
		}
		linphone_address_destroy(callee_from);
	}


	if (callee_params){
		linphone_core_accept_call_with_params(callee_mgr->lc,callee_call,callee_params);
	}else if (build_callee_params){
		LinphoneCallParams *default_params=linphone_core_create_call_params(callee_mgr->lc,callee_call);
		ms_message("Created default call params with video=%i", linphone_call_params_video_enabled(default_params));
		linphone_core_accept_call_with_params(callee_mgr->lc,callee_call,default_params);
		linphone_call_params_destroy(default_params);
	}else{
		linphone_core_accept_call(callee_mgr->lc,callee_call);
	}

	BC_ASSERT_TRUE(wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallConnected,initial_callee.number_of_LinphoneCallConnected+1));
	BC_ASSERT_TRUE(wait_for(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallConnected,initial_callee.number_of_LinphoneCallConnected+1));

	result = wait_for_until(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallStreamsRunning,initial_caller.number_of_LinphoneCallStreamsRunning+1, 2000)
			&&
			wait_for_until(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallStreamsRunning,initial_callee.number_of_LinphoneCallStreamsRunning+1, 2000);

	if (linphone_core_get_media_encryption(caller_mgr->lc) != LinphoneMediaEncryptionNone
		|| linphone_core_get_media_encryption(callee_mgr->lc) != LinphoneMediaEncryptionNone) {
		/*wait for encryption to be on, in case of zrtp or dtls, it can take a few seconds*/
		if (	(linphone_core_get_media_encryption(caller_mgr->lc) == LinphoneMediaEncryptionZRTP)
				|| (linphone_core_get_media_encryption(caller_mgr->lc) == LinphoneMediaEncryptionDTLS))
			wait_for(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallEncryptedOn,initial_caller.number_of_LinphoneCallEncryptedOn+1);
		if ((linphone_core_get_media_encryption(callee_mgr->lc) == LinphoneMediaEncryptionZRTP)
			|| (linphone_core_get_media_encryption(callee_mgr->lc) == LinphoneMediaEncryptionDTLS)
			|| (linphone_core_get_media_encryption(caller_mgr->lc) == LinphoneMediaEncryptionDTLS) /*also take care of caller policy*/ )
			wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallEncryptedOn,initial_callee.number_of_LinphoneCallEncryptedOn+1);
		{
			const LinphoneCallParams* call_param = linphone_call_get_current_params(callee_call);
			BC_ASSERT_EQUAL(linphone_call_params_get_media_encryption(call_param),linphone_core_get_media_encryption(caller_mgr->lc), int, "%d");
			call_param = linphone_call_get_current_params(linphone_core_get_current_call(caller_mgr->lc));
			BC_ASSERT_EQUAL(linphone_call_params_get_media_encryption(call_param),linphone_core_get_media_encryption(caller_mgr->lc), int, "%d");

		}
	}
	/*wait ice re-invite*/
	if (linphone_core_get_firewall_policy(caller_mgr->lc) == LinphonePolicyUseIce
			&& linphone_core_get_firewall_policy(callee_mgr->lc) == LinphonePolicyUseIce
			&& !linphone_core_sdp_200_ack_enabled(caller_mgr->lc) /*ice does not work with sdp less invite*/
			&& lp_config_get_int(callee_mgr->lc->config, "sip", "update_call_when_ice_completed", TRUE)
			&& lp_config_get_int(caller_mgr->lc->config, "sip", "update_call_when_ice_completed", TRUE)
			&& linphone_core_get_media_encryption(caller_mgr->lc) != LinphoneMediaEncryptionDTLS /*no ice-reinvite with DTLS*/) {
		BC_ASSERT_TRUE(wait_for(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallStreamsRunning,initial_caller.number_of_LinphoneCallStreamsRunning+2));
		BC_ASSERT_TRUE(wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallStreamsRunning,initial_callee.number_of_LinphoneCallStreamsRunning+2));

	} else if (linphone_core_get_firewall_policy(caller_mgr->lc) == LinphonePolicyUseIce) {
		/* check no ice re-invite received*/
		BC_ASSERT_FALSE(wait_for_until(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallStreamsRunning,initial_caller.number_of_LinphoneCallStreamsRunning+2,2000));
		BC_ASSERT_FALSE(wait_for_until(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallStreamsRunning,initial_callee.number_of_LinphoneCallStreamsRunning+2,2000));

	}
	if (linphone_core_get_media_encryption(caller_mgr->lc) == LinphoneMediaEncryptionDTLS ) {
		if (linphone_core_get_current_call(caller_mgr->lc)->audiostream)
			BC_ASSERT_TRUE(ms_media_stream_sessions_get_encryption_mandatory(&linphone_core_get_current_call(caller_mgr->lc)->audiostream->ms.sessions));
#ifdef VIDEO_ENABLED
		if (linphone_core_get_current_call(caller_mgr->lc)->videostream && video_stream_started(linphone_core_get_current_call(caller_mgr->lc)->videostream))
			BC_ASSERT_TRUE(ms_media_stream_sessions_get_encryption_mandatory(&linphone_core_get_current_call(caller_mgr->lc)->videostream->ms.sessions));
#endif

	}
	return result;
}

bool_t call_with_params(LinphoneCoreManager* caller_mgr
						,LinphoneCoreManager* callee_mgr
						,const LinphoneCallParams *caller_params
						,const LinphoneCallParams *callee_params){
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params =  {0};
	caller_test_params.base = (LinphoneCallParams*)caller_params;
	callee_test_params.base = (LinphoneCallParams*)callee_params;
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
	int previous_count_1 = m1->stat.number_of_LinphoneCallEnd;
	int previous_count_2 = m2->stat.number_of_LinphoneCallEnd;
	linphone_core_terminate_all_calls(m1->lc);
	BC_ASSERT_TRUE(wait_for(m1->lc,m2->lc,&m1->stat.number_of_LinphoneCallEnd,previous_count_1+1));
	BC_ASSERT_TRUE(wait_for(m1->lc,m2->lc,&m2->stat.number_of_LinphoneCallEnd,previous_count_2+1));
	BC_ASSERT_TRUE(wait_for(m1->lc,m2->lc,&m1->stat.number_of_LinphoneCallReleased,previous_count_1+1));
	BC_ASSERT_TRUE(wait_for(m1->lc,m2->lc,&m2->stat.number_of_LinphoneCallReleased,previous_count_2+1));
}

void simple_call_base(bool_t enable_multicast_recv_side) {
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	const LinphoneAddress *from;
	LinphoneCall *pauline_call;
	LinphoneProxyConfig* marie_cfg;

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	/* with the account manager, we might lose the identity */
	marie_cfg = linphone_core_get_default_proxy_config(marie->lc);
	{
		LinphoneAddress* marie_addr = linphone_address_clone(linphone_proxy_config_get_identity_address(marie_cfg));
		char* marie_tmp_id = NULL;
		linphone_address_set_display_name(marie_addr, "Super Marie");
		marie_tmp_id = linphone_address_as_string(marie_addr);

		linphone_proxy_config_edit(marie_cfg);
		linphone_proxy_config_set_identity(marie_cfg,marie_tmp_id);
		linphone_proxy_config_done(marie_cfg);

		ms_free(marie_tmp_id);
		linphone_address_destroy(marie_addr);
	}

	linphone_core_enable_audio_multicast(pauline->lc,enable_multicast_recv_side);

	BC_ASSERT_TRUE(call(marie,pauline));
	pauline_call=linphone_core_get_current_call(pauline->lc);
	BC_ASSERT_PTR_NOT_NULL(pauline_call);
	/*check that display name is correctly propagated in From */
	if (pauline_call){
		from=linphone_call_get_remote_address(linphone_core_get_current_call(pauline->lc));
		BC_ASSERT_PTR_NOT_NULL(from);
		if (from){
			const char *dname=linphone_address_get_display_name(from);
			BC_ASSERT_PTR_NOT_NULL(dname);
			if (dname){
				BC_ASSERT_STRING_EQUAL(dname, "Super Marie");
			}
		}
	}


	liblinphone_tester_check_rtcp(marie,pauline);
	end_call(marie,pauline);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
}

static void simple_call(void) {
	simple_call_base(FALSE);
}

static void automatic_call_termination(void) {
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");


	if (!BC_ASSERT_TRUE(call(marie,pauline))) goto end;

	liblinphone_tester_check_rtcp(marie,pauline);

	linphone_core_destroy(pauline->lc);
	pauline->lc = NULL;
	/*marie shall receive the BYE*/
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallEnd, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallReleased, 1));
end:
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
}

static void call_with_timeouted_bye(void) {
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	belle_sip_timer_config_t timer_config;

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	BC_ASSERT_TRUE(call(marie,pauline));

	sal_set_send_error(pauline->lc->sal,1500); /*to trash the message without generating error*/
	timer_config.T1=50; /*to have timer F = 3s*/
	timer_config.T2=4000;
	timer_config.T3=0;
	timer_config.T4=5000;

	belle_sip_stack_set_timer_config(sal_get_belle_sip_stack(pauline->lc->sal),&timer_config);
	linphone_core_terminate_all_calls(pauline->lc);

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallReleased,1,timer_config.T1*84));

	sal_set_send_error(pauline->lc->sal,0);

	linphone_core_terminate_all_calls(marie->lc);
	BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1,5000));
	BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallReleased,1,5000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void direct_call_over_ipv6(void){
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;

	if (liblinphone_tester_ipv6_available()){
		LCSipTransports pauline_transports;
		LinphoneAddress* pauline_dest = linphone_address_new("sip:[::1];transport=tcp");
		marie = linphone_core_manager_new( "marie_rc");
		pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

		linphone_core_enable_ipv6(marie->lc,TRUE);
		linphone_core_enable_ipv6(pauline->lc,TRUE);
		linphone_core_set_default_proxy_config(marie->lc,NULL);
		/*wait for register in v6 mode, however sip2.linphone.org has an ipv6 address but doesn't listen to it*/
#if 0
		BC_ASSERT_TRUE(wait_for_until(pauline->lc, NULL, &pauline->stat.number_of_LinphoneRegistrationOk, 2, 2000));
		BC_ASSERT_TRUE(wait_for_until(pauline->lc, NULL, &marie->stat.number_of_LinphoneRegistrationOk, 2, 2000));
#endif

		linphone_core_get_sip_transports_used(pauline->lc,&pauline_transports);
		linphone_address_set_port(pauline_dest,pauline_transports.tcp_port);
		linphone_core_invite_address(marie->lc,pauline_dest);

		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallOutgoingRinging,1));
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallIncomingReceived,1));
		linphone_core_accept_call(pauline->lc,linphone_core_get_current_call(pauline->lc));
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,1));
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,1));

		liblinphone_tester_check_rtcp(marie,pauline);
		end_call(marie,pauline);
		linphone_core_manager_destroy(marie);
		linphone_core_manager_destroy(pauline);
		linphone_address_destroy(pauline_dest);
	}else ms_warning("Test skipped, no ipv6 available");
}

static void call_outbound_with_multiple_proxy(void) {
	LinphoneCoreManager* marie   = linphone_core_manager_new2( "marie_rc", FALSE);
	LinphoneCoreManager* pauline = linphone_core_manager_new2( "pauline_tcp_rc", FALSE);

	LinphoneProxyConfig* lpc = NULL;
	LinphoneProxyConfig* registered_lpc = linphone_core_create_proxy_config(marie->lc);

	lpc = linphone_core_get_default_proxy_config(marie->lc);
	linphone_core_set_default_proxy(marie->lc,NULL);

	BC_ASSERT_FATAL(lpc != NULL);
	BC_ASSERT_FATAL(registered_lpc != NULL);

	// create new LPC that will successfully register
	linphone_proxy_config_set_identity(registered_lpc, linphone_proxy_config_get_identity(lpc));
	linphone_proxy_config_set_server_addr(registered_lpc, linphone_proxy_config_get_addr(lpc));
	linphone_proxy_config_set_route(registered_lpc, linphone_proxy_config_get_route(lpc));
	linphone_proxy_config_enable_register(registered_lpc, TRUE);

	linphone_core_add_proxy_config(marie->lc, registered_lpc);
	linphone_proxy_config_unref(registered_lpc);

	// set first LPC to unreacheable proxy addr
	linphone_proxy_config_edit(lpc);
	linphone_proxy_config_set_server_addr(lpc,"sip:linphone.org:9016;transport=udp");
	linphone_proxy_config_set_route(lpc, "sip:linphone.org:9016;transport=udp;lr");
	linphone_proxy_config_done(lpc);

	BC_ASSERT_TRUE(wait_for_until(pauline->lc, NULL, &pauline->stat.number_of_LinphoneRegistrationOk, 1, 10000));

	BC_ASSERT_TRUE(wait_for_until(marie->lc, NULL, &marie->stat.number_of_LinphoneRegistrationProgress, 2, 200));
	BC_ASSERT_TRUE(wait_for_until(marie->lc, NULL, &marie->stat.number_of_LinphoneRegistrationOk, 1, 10000));

	// calling marie should go through the second proxy config
	BC_ASSERT_TRUE(call(marie, pauline));

	end_call(marie, pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#if 0 /* TODO: activate test when the implementation is ready */
static void multiple_answers_call(void) {
	/* Scenario is this: pauline calls marie, which is registered 2 times.
	   Both linphones answer at the same time, and only one should get the
	   call running, the other should be terminated */
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc" );
	LinphoneCoreManager* marie1  = linphone_core_manager_new( "marie_rc" );
	LinphoneCoreManager* marie2  = linphone_core_manager_new( "marie_rc" );

	LinphoneCall* call1, *call2;

	MSList* lcs = ms_list_append(NULL,pauline->lc);
	lcs = ms_list_append(lcs,marie1->lc);
	lcs = ms_list_append(lcs,marie2->lc);


	BC_ASSERT_TRUE(wait_for_until(pauline->lc, NULL, &pauline->stat.number_of_LinphoneRegistrationOk, 1, 2000));

	BC_ASSERT_PTR_NOT_NULL( linphone_core_invite_address(pauline->lc, marie1->identity ) );

	BC_ASSERT_TRUE(wait_for_list(lcs,&marie1->stat.number_of_LinphoneCallIncomingReceived, 1, 2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived, 1, 2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingProgress, 1, 2000));

	// marie 1 and 2 answer at the same time
	call1 = linphone_core_get_current_call(marie1->lc);
	call2 = linphone_core_get_current_call(marie2->lc);

	BC_ASSERT_PTR_NOT_NULL_FATAL(call1);
	BC_ASSERT_PTR_NOT_NULL_FATAL(call2);

	BC_ASSERT_EQUAL( linphone_core_accept_call(marie1->lc, call1), 0, int, "%d");
	BC_ASSERT_EQUAL( linphone_core_accept_call(marie2->lc, call2), 0, int, "%d");

	BC_ASSERT_TRUE( wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallStreamsRunning, 1, 2000) );
	BC_ASSERT_TRUE( wait_for_list(lcs, &marie1->stat.number_of_LinphoneCallStreamsRunning, 1, 2000) );
	BC_ASSERT_TRUE( wait_for_list(lcs, &marie2->stat.number_of_LinphoneCallEnd, 1, 2000) );


	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie1);
	linphone_core_manager_destroy(marie2);
}
#endif

static void multiple_answers_call_with_media_relay(void) {

	/* Scenario is this: pauline calls marie, which is registered 2 times.
	 *   Both linphones answer at the same time, and only one should get the
	 *   call running, the other should be terminated */
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc" );
	LinphoneCoreManager* marie1  = linphone_core_manager_new( "marie_rc" );
	LinphoneCoreManager* marie2  = linphone_core_manager_new( "marie_rc" );

	LinphoneCall* call1, *call2;

	MSList* lcs = ms_list_append(NULL,pauline->lc);
	lcs = ms_list_append(lcs,marie1->lc);
	lcs = ms_list_append(lcs,marie2->lc);

	linphone_core_set_user_agent(pauline->lc, "Natted Linphone", NULL);
	linphone_core_set_user_agent(marie1->lc, "Natted Linphone", NULL);
	linphone_core_set_user_agent(marie2->lc, "Natted Linphone", NULL);

	BC_ASSERT_TRUE(wait_for_until(pauline->lc, NULL, &pauline->stat.number_of_LinphoneRegistrationOk, 1, 2000));

	BC_ASSERT_PTR_NOT_NULL( linphone_core_invite_address(pauline->lc, marie1->identity ) );

	BC_ASSERT_TRUE(wait_for_list(lcs,&marie1->stat.number_of_LinphoneCallIncomingReceived, 1, 2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived, 1, 2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingProgress, 1, 2000));

	// marie 1 and 2 answer at the same time
	call1 = linphone_core_get_current_call(marie1->lc);
	call2 = linphone_core_get_current_call(marie2->lc);

	BC_ASSERT_PTR_NOT_NULL_FATAL(call1);
	BC_ASSERT_PTR_NOT_NULL_FATAL(call2);

	BC_ASSERT_EQUAL( linphone_core_accept_call(marie1->lc, call1), 0, int, "%d");
	ms_sleep(1); /*sleep to make sure that the 200OK of marie1 reaches the server first*/
	BC_ASSERT_EQUAL( linphone_core_accept_call(marie2->lc, call2), 0, int, "%d");

	BC_ASSERT_TRUE( wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallStreamsRunning, 1, 2000) );
	BC_ASSERT_TRUE( wait_for_list(lcs, &marie1->stat.number_of_LinphoneCallStreamsRunning, 1, 2000) );
	/*the server will send a bye to marie2, as is 200Ok arrived second*/
	BC_ASSERT_TRUE( wait_for_list(lcs, &marie2->stat.number_of_LinphoneCallEnd, 1, 4000) );

	end_call(marie1, pauline);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie1);
	linphone_core_manager_destroy(marie2);
	ms_list_free(lcs);
}

static void call_with_specified_codec_bitrate(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok;
	const char * codec = "opus";
	int rate = 48000;
	int min_bw=24;
	int max_bw=50;

#ifdef __arm__
	if (ms_factory_get_cpu_count(marie->lc->factory) <2) { /*2 opus codec channel + resampler is too much for a single core*/
#ifndef ANDROID
		codec = "speex";
		rate = 8000;
		min_bw=20;
		max_bw=35;
#else
		BC_PASS("Test requires at least a dual core");
		goto end;
#endif
	}
#endif
	/*Force marie to play from file: if soundcard is used and it is silient, then vbr mode will drop down the bitrate
	 Note that a play file is already set by linphone_core_manager_new() (but not used)*/
	linphone_core_set_use_files(marie->lc, TRUE);

	if (linphone_core_find_payload_type(marie->lc,codec,rate,-1)==NULL){
		BC_PASS("opus codec not supported, test skipped.");
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

	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));
	if (!call_ok) goto end;
	liblinphone_tester_check_rtcp(marie,pauline);
	/*wait a bit that bitstreams are stabilized*/
	wait_for_until(marie->lc, pauline->lc, NULL, 0, 2000);

	BC_ASSERT_LOWER(linphone_core_manager_get_mean_audio_down_bw(marie), (int)(min_bw+5+min_bw*.1), int, "%i");
	BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_down_bw(marie), 10, int, "%i"); /*check that at least something is received */
	BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_down_bw(pauline), (int)(max_bw-5-max_bw*.1), int, "%i");

	end_call(pauline, marie);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


static void disable_all_codecs(const MSList* elem, LinphoneCoreManager* call){

    PayloadType *pt;

    for(;elem!=NULL;elem=elem->next){
        pt=(PayloadType*)elem->data;
        linphone_core_enable_payload_type(call->lc,pt,FALSE);
    }
}
/***
 Disable all audio codecs , sends an INVITE with RTP port 0 and payload 0.
 Wait for SIP  488 unacceptable.
 ***/
static void call_with_no_audio_codec(void){

	LinphoneCoreManager* callee = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* caller = linphone_core_manager_new(transport_supported(LinphoneTransportTcp) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall* out_call ;

	const MSList* elem =linphone_core_get_audio_codecs(caller->lc);

	disable_all_codecs(elem, caller);


	out_call = linphone_core_invite_address(caller->lc,callee->identity);
	linphone_call_ref(out_call);
	BC_ASSERT_TRUE(wait_for(caller->lc, callee->lc, &caller->stat.number_of_LinphoneCallOutgoingInit, 1));


	BC_ASSERT_TRUE(wait_for_until(caller->lc, callee->lc, &caller->stat.number_of_LinphoneCallError, 1, 6000));
	BC_ASSERT_EQUAL(linphone_call_get_reason(out_call), LinphoneReasonNotAcceptable, int, "%d");
	BC_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallIncomingReceived, 0, int, "%d");

	linphone_call_unref(out_call);
	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);

}

static void video_call_with_no_audio_and_no_video_codec(void){

	LinphoneCoreManager* callee = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* caller = linphone_core_manager_new(transport_supported(LinphoneTransportTcp) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall* out_call ;
	LinphoneVideoPolicy callee_policy, caller_policy;
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params = {0};

	const MSList* elem_video =linphone_core_get_video_codecs(caller->lc);

	const MSList* elem_audio =linphone_core_get_audio_codecs(caller->lc);

	disable_all_codecs(elem_audio, caller);
	disable_all_codecs(elem_video, caller);

	callee_policy.automatically_initiate=FALSE;
	callee_policy.automatically_accept=TRUE;
	caller_policy.automatically_initiate=TRUE;
	caller_policy.automatically_accept=FALSE;

	linphone_core_set_video_policy(callee->lc,&callee_policy);
	linphone_core_set_video_policy(caller->lc,&caller_policy);


	linphone_core_enable_video_display(callee->lc, TRUE);
	linphone_core_enable_video_capture(callee->lc, TRUE);

	linphone_core_enable_video_display(caller->lc, TRUE);
	linphone_core_enable_video_capture(caller->lc, TRUE);

	/* Create call params */
	caller_test_params.base = linphone_core_create_call_params(caller->lc, NULL);


	out_call = linphone_core_invite_address_with_params(caller->lc, callee->identity,caller_test_params.base);
	linphone_call_ref(out_call);

	linphone_call_params_destroy(caller_test_params.base);
	if (callee_test_params.base) linphone_call_params_destroy(callee_test_params.base);


	BC_ASSERT_TRUE(wait_for(caller->lc, callee->lc, &caller->stat.number_of_LinphoneCallOutgoingInit, 1));

	BC_ASSERT_TRUE(wait_for_until(caller->lc, callee->lc, &caller->stat.number_of_LinphoneCallError, 1, 6000));
	BC_ASSERT_EQUAL(linphone_call_get_reason(out_call), LinphoneReasonNotAcceptable, int, "%d");
	BC_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallIncomingReceived, 0, int, "%d");

	linphone_call_unref(out_call);
	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);

}

static void simple_call_compatibility_mode(void) {
	char route[256];
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	LinphoneCore* lc_marie=marie->lc;
	LinphoneCore* lc_pauline=pauline->lc;
	stats* stat_marie=&marie->stat;
	stats* stat_pauline=&pauline->stat;
	LinphoneProxyConfig* proxy;
	const LinphoneAddress* identity;
	LinphoneAddress* proxy_address;
	char*tmp;
	LCSipTransports transport;

	proxy = linphone_core_get_default_proxy_config(lc_marie);
	BC_ASSERT_PTR_NOT_NULL (proxy);
	identity = linphone_proxy_config_get_identity_address(proxy);


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

	BC_ASSERT_TRUE (wait_for(lc_marie,lc_marie,&stat_marie->number_of_LinphoneRegistrationOk,1));

	linphone_core_invite_address(lc_marie,pauline->identity);

	BC_ASSERT_TRUE (wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallIncomingReceived,1));
	BC_ASSERT_TRUE(linphone_core_inc_invite_pending(lc_pauline));
	BC_ASSERT_EQUAL(stat_marie->number_of_LinphoneCallOutgoingProgress,1, int, "%d");
	BC_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallOutgoingRinging,1));

	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call_remote_address(lc_pauline));
	if (linphone_core_get_current_call_remote_address(lc_pauline)) {
		BC_ASSERT_TRUE(linphone_address_weak_equal(identity,linphone_core_get_current_call_remote_address(lc_pauline)));

		linphone_core_accept_call(lc_pauline,linphone_core_get_current_call(lc_pauline));

		BC_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallConnected,1));
		BC_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallConnected,1));
		BC_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallStreamsRunning,1));
		BC_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallStreamsRunning,1));

		wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallStreamsRunning,3);
		end_call(pauline, marie);
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


static void cancelled_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	LinphoneCall* out_call = linphone_core_invite_address(pauline->lc,marie->identity);
	linphone_call_ref(out_call);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallOutgoingInit,1));

	linphone_core_terminate_call(pauline->lc,out_call);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	//BC_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonCanceled, int, "%d");
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallEnd,1, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallIncomingReceived,0, int, "%d");
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallReleased,1));

	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void disable_all_audio_codecs_except_one(LinphoneCore *lc, const char *mime, int rate){
	const MSList *elem=linphone_core_get_audio_codecs(lc);
	PayloadType *pt;

	for(;elem!=NULL;elem=elem->next){
		pt=(PayloadType*)elem->data;
		linphone_core_enable_payload_type(lc,pt,FALSE);
	}
	pt=linphone_core_find_payload_type(lc,mime,rate,-1);
	BC_ASSERT_PTR_NOT_NULL_FATAL(pt);
	linphone_core_enable_payload_type(lc,pt,TRUE);
}

#ifdef VIDEO_ENABLED
void disable_all_video_codecs_except_one(LinphoneCore *lc, const char *mime) {
	const MSList *codecs = linphone_core_get_video_codecs(lc);
	const MSList *it = NULL;
	PayloadType *pt = NULL;

	for(it = codecs; it != NULL; it = it->next) {
		linphone_core_enable_payload_type(lc, (PayloadType *)it->data, FALSE);
	}
	pt = linphone_core_find_payload_type(lc, mime, -1, -1);
	BC_ASSERT_PTR_NOT_NULL_FATAL(pt);
	linphone_core_enable_payload_type(lc, pt, TRUE);
}
#endif

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
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallOutgoingInit,1, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallOutgoingProgress,1, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallError,1, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallReleased,1, int, "%d");
	linphone_core_manager_destroy(marie);
}

static void early_cancelled_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new2( "empty_rc",FALSE);

	LinphoneCall* out_call = linphone_core_invite_address(pauline->lc,marie->identity);

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallOutgoingInit,1));
	linphone_core_terminate_call(pauline->lc,out_call);

	/*since everything is executed in a row, no response can be received from the server, thus the CANCEL cannot be sent.
	 It will ring at Marie's side.*/

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));

	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallEnd,1, int, "%d");

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallIncomingReceived,1));
	/* now the CANCEL should have been sent and the the call at marie's side should terminate*/
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallReleased,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void cancelled_ringing_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	LinphoneCall* out_call = linphone_core_invite_address(pauline->lc,marie->identity);
	linphone_call_ref(out_call);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallIncomingReceived,1));

	linphone_core_terminate_call(pauline->lc,out_call);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallReleased,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallReleased,1));
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallEnd,1, int, "%d");
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallEnd,1, int, "%d");

	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void early_declined_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCallLog* out_call_log;
	LinphoneCall* out_call;

	linphone_core_set_max_calls(marie->lc,0);
	out_call = linphone_core_invite_address(pauline->lc,marie->identity);
	linphone_call_ref(out_call);

	/*wait until flexisip transfers the busy...*/
	BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallError,1,33000));
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallError,1, int, "%d");
	/* FIXME http://git.linphone.org/mantis/view.php?id=757

	BC_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonBusy, int, "%d");
	 */
	if (ms_list_size(linphone_core_get_call_logs(pauline->lc))>0) {
		BC_ASSERT_PTR_NOT_NULL(out_call_log=(LinphoneCallLog*)(linphone_core_get_call_logs(pauline->lc)->data));
		BC_ASSERT_EQUAL(linphone_call_log_get_status(out_call_log),LinphoneCallAborted, int, "%d");
	}
	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_busy_when_calling_self(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCall *out_call=linphone_core_invite_address(marie->lc,marie->identity);
	linphone_call_ref(out_call);

	/*wait until flexisip transfers the busy...*/
	BC_ASSERT_TRUE(wait_for_until(marie->lc,marie->lc,&marie->stat.number_of_LinphoneCallError,1,33000));
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallError,1, int, "%d");

	BC_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonBusy, int, "%d");
	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
}


static void call_declined(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	LinphoneCall* in_call;
	LinphoneCall* out_call = linphone_core_invite_address(pauline->lc,marie->identity);
	linphone_call_ref(out_call);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallIncomingReceived,1));
	BC_ASSERT_PTR_NOT_NULL(in_call=linphone_core_get_current_call(marie->lc));
	if (in_call) {
		linphone_call_ref(in_call);
		linphone_core_terminate_call(marie->lc,in_call);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallReleased,1));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallReleased,1));
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallEnd,1, int, "%d");
		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallEnd,1, int, "%d");
		BC_ASSERT_EQUAL(linphone_call_get_reason(in_call),LinphoneReasonDeclined, int, "%d");
		BC_ASSERT_EQUAL(linphone_call_log_get_status(linphone_call_get_call_log(in_call)),LinphoneCallDeclined, int, "%d");
		BC_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonDeclined, int, "%d");
		BC_ASSERT_EQUAL(linphone_call_log_get_status(linphone_call_get_call_log(out_call)),LinphoneCallDeclined, int, "%d");
		linphone_call_unref(in_call);
	}
	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_terminated_by_caller(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	BC_ASSERT_TRUE(call(pauline,marie));

	end_call(pauline, marie);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_no_sdp(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	linphone_core_enable_sdp_200_ack(marie->lc,TRUE);

	BC_ASSERT_TRUE(call(marie,pauline));

	end_call(pauline, marie);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_no_sdp_ack_without_sdp(void){
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall *call;

	linphone_core_enable_sdp_200_ack(marie->lc,TRUE);

	linphone_core_invite_address(marie->lc,pauline->identity);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallIncomingReceived,1));
	call=linphone_core_get_current_call(pauline->lc);
	if (call){
		sal_call_set_sdp_handling(call->op, SalOpSDPSimulateError); /*this will have the effect that the SDP received in the ACK will be ignored*/
		linphone_core_accept_call(pauline->lc, call);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallError,1));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void check_nb_media_starts(LinphoneCoreManager *caller, LinphoneCoreManager *callee, unsigned int caller_nb_media_starts, unsigned int callee_nb_media_starts) {
	LinphoneCall *c1 = linphone_core_get_current_call(caller->lc);
	LinphoneCall *c2 = linphone_core_get_current_call(callee->lc);
	BC_ASSERT_PTR_NOT_NULL(c1);
	BC_ASSERT_PTR_NOT_NULL(c2);
	if (c1) {
		BC_ASSERT_EQUAL(c1->nb_media_starts, caller_nb_media_starts, unsigned int, "%u");
	}
	if (c2) {
		BC_ASSERT_EQUAL(c2->nb_media_starts, callee_nb_media_starts, unsigned int, "%u");
	}
}

static void _call_with_ice_base(LinphoneCoreManager* pauline,LinphoneCoreManager* marie, bool_t caller_with_ice, bool_t callee_with_ice, bool_t random_ports, bool_t forced_relay) {

	linphone_core_set_user_agent(pauline->lc, "Natted Linphone", NULL);
	linphone_core_set_user_agent(marie->lc, "Natted Linphone", NULL);

	if (callee_with_ice){
		linphone_core_set_firewall_policy(marie->lc,LinphonePolicyUseIce);
	}
	if (caller_with_ice){
		linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);
	}

	if (random_ports){
		linphone_core_set_audio_port(marie->lc,-1);
		linphone_core_set_video_port(marie->lc,-1);
		linphone_core_set_text_port(marie->lc, -1);
		linphone_core_set_audio_port(pauline->lc,-1);
		linphone_core_set_video_port(pauline->lc,-1);
		linphone_core_set_text_port(pauline->lc, -1);
	}

	if (forced_relay == TRUE) {
		linphone_core_enable_forced_ice_relay(marie->lc, TRUE);
		linphone_core_enable_forced_ice_relay(pauline->lc, TRUE);
	}

	if (!BC_ASSERT_TRUE(call(pauline,marie)))
		return;

	if (callee_with_ice && caller_with_ice) {
		/*wait for the ICE reINVITE to complete*/
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));

		if (forced_relay == TRUE) {
			BC_ASSERT_TRUE(check_ice(pauline, marie, LinphoneIceStateRelayConnection));
		} else {
			BC_ASSERT_TRUE(check_ice(pauline,marie,LinphoneIceStateHostConnection));
		}
		check_nb_media_starts(pauline, marie, 1, 1);
	}

	liblinphone_tester_check_rtcp(marie,pauline);
	/*then close the call*/
	end_call(pauline, marie);
}

static void _call_with_ice(bool_t caller_with_ice, bool_t callee_with_ice, bool_t random_ports, bool_t forced_relay) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	_call_with_ice_base(pauline,marie,caller_with_ice,callee_with_ice,random_ports,forced_relay);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void call_with_ice(void){
	_call_with_ice(TRUE,TRUE,FALSE,FALSE);
}

/*ICE is not expected to work in this case, however this should not crash*/
static void call_with_ice_no_sdp(void){
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	linphone_core_enable_sdp_200_ack(pauline->lc,TRUE);

	linphone_core_set_firewall_policy(marie->lc,LinphonePolicyUseIce);

	linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);

	call(pauline,marie);

	liblinphone_tester_check_rtcp(marie,pauline);

	end_call(pauline, marie);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_ice_random_ports(void){
	_call_with_ice(TRUE,TRUE,TRUE,FALSE);
}

static void call_with_ice_forced_relay(void) {
	_call_with_ice(TRUE, TRUE, TRUE, TRUE);
}

static void ice_to_not_ice(void){
	_call_with_ice(TRUE,FALSE,FALSE,FALSE);
}

static void not_ice_to_ice(void){
	_call_with_ice(FALSE,TRUE,FALSE,FALSE);
}

static void call_with_custom_headers(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
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

	params=linphone_core_create_call_params(marie->lc, NULL);
	linphone_call_params_add_custom_header(params,"Weather","bad");
	linphone_call_params_add_custom_header(params,"Working","yes");

	BC_ASSERT_TRUE(call_with_caller_params(pauline,marie,params));
	linphone_call_params_destroy(params);

	call_marie=linphone_core_get_current_call(marie->lc);
	call_pauline=linphone_core_get_current_call(pauline->lc);

	BC_ASSERT_PTR_NOT_NULL(call_marie);
	BC_ASSERT_PTR_NOT_NULL(call_pauline);

	marie_remote_params=linphone_call_get_remote_params(call_marie);
	hvalue=linphone_call_params_get_custom_header(marie_remote_params,"Weather");
	BC_ASSERT_PTR_NOT_NULL(hvalue);
	BC_ASSERT_STRING_EQUAL(hvalue,"bad");
	hvalue=linphone_call_params_get_custom_header(marie_remote_params,"uriHeader");
	BC_ASSERT_PTR_NOT_NULL(hvalue);
	BC_ASSERT_STRING_EQUAL(hvalue,"myUriHeader");


	// FIXME: we have to strdup because successive calls to get_remote_params erase the returned const char*!!
	pauline_remote_contact        = ms_strdup(linphone_call_get_remote_contact(call_pauline));
	pauline_remote_contact_header = ms_strdup(linphone_call_params_get_custom_header(linphone_call_get_remote_params(call_pauline), "Contact"));

	marie_remote_contact        = ms_strdup(linphone_call_get_remote_contact(call_marie));
	marie_remote_contact_header = ms_strdup(linphone_call_params_get_custom_header(linphone_call_get_remote_params(call_marie), "Contact"));

	BC_ASSERT_PTR_NOT_NULL(pauline_remote_contact);
	BC_ASSERT_PTR_NOT_NULL(pauline_remote_contact_header);
	BC_ASSERT_PTR_NOT_NULL(marie_remote_contact);
	BC_ASSERT_PTR_NOT_NULL(marie_remote_contact_header);
	BC_ASSERT_STRING_EQUAL(pauline_remote_contact,pauline_remote_contact_header);
	BC_ASSERT_STRING_EQUAL(marie_remote_contact,marie_remote_contact_header);

	ms_free(pauline_remote_contact);
	ms_free(pauline_remote_contact_header);
	ms_free(marie_remote_contact);
	ms_free(marie_remote_contact_header);

	end_call(pauline, marie);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_custom_sdp_attributes_cb(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *message) {
	if (cstate == LinphoneCallUpdatedByRemote) {
		LinphoneCallParams *params;
		const LinphoneCallParams *remote_params = linphone_call_get_remote_params(call);
		const char *value = linphone_call_params_get_custom_sdp_attribute(remote_params, "weather");
		BC_ASSERT_PTR_NOT_NULL(value);
		if (value) BC_ASSERT_STRING_EQUAL(value, "sunny");
		params = linphone_core_create_call_params(lc, call);
		linphone_call_params_clear_custom_sdp_attributes(params);
		linphone_call_params_clear_custom_sdp_media_attributes(params, LinphoneStreamTypeAudio);
		linphone_call_params_add_custom_sdp_attribute(params, "working", "no");
		BC_ASSERT_EQUAL(linphone_core_accept_call_update(lc, call, params), 0, int, "%i");
		linphone_call_params_destroy(params);
	}
}

static void call_with_custom_sdp_attributes(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall *call_marie, *call_pauline;
	LinphoneCallParams *pauline_params;
	const LinphoneCallParams *marie_remote_params;
	const LinphoneCallParams *pauline_remote_params;
	const char *value;
	LinphoneCoreVTable *vtable;

	pauline_params = linphone_core_create_call_params(pauline->lc, NULL);
	linphone_call_params_add_custom_sdp_attribute(pauline_params, "weather", "bad");
	linphone_call_params_add_custom_sdp_attribute(pauline_params, "working", "yes");
	linphone_call_params_add_custom_sdp_media_attribute(pauline_params, LinphoneStreamTypeAudio, "sleeping", "almost");
	BC_ASSERT_TRUE(call_with_caller_params(pauline, marie, pauline_params));
	linphone_call_params_destroy(pauline_params);

	call_marie = linphone_core_get_current_call(marie->lc);
	call_pauline = linphone_core_get_current_call(pauline->lc);
	BC_ASSERT_PTR_NOT_NULL(call_marie);
	BC_ASSERT_PTR_NOT_NULL(call_pauline);

	marie_remote_params = linphone_call_get_remote_params(call_marie);
	value = linphone_call_params_get_custom_sdp_attribute(marie_remote_params, "weather");
	BC_ASSERT_PTR_NOT_NULL(value);
	if (value) BC_ASSERT_STRING_EQUAL(value, "bad");
	value = linphone_call_params_get_custom_sdp_media_attribute(marie_remote_params, LinphoneStreamTypeAudio, "sleeping");
	BC_ASSERT_PTR_NOT_NULL(value);
	if (value) BC_ASSERT_STRING_EQUAL(value, "almost");

	vtable = linphone_core_v_table_new();
	vtable->call_state_changed = call_with_custom_sdp_attributes_cb;
	linphone_core_add_listener(marie->lc, vtable);
	pauline_params = linphone_core_create_call_params(pauline->lc, call_pauline);
	linphone_call_params_clear_custom_sdp_attributes(pauline_params);
	linphone_call_params_clear_custom_sdp_media_attributes(pauline_params, LinphoneStreamTypeAudio);
	linphone_call_params_add_custom_sdp_attribute(pauline_params, "weather", "sunny");
	linphone_core_update_call(pauline->lc, call_pauline, pauline_params);
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneCallUpdatedByRemote, 1));
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneCallUpdating, 1));
	linphone_call_params_destroy(pauline_params);
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 2));
	pauline_remote_params = linphone_call_get_remote_params(call_pauline);
	value = linphone_call_params_get_custom_sdp_attribute(pauline_remote_params, "working");
	BC_ASSERT_PTR_NOT_NULL(value);
	if (value) BC_ASSERT_STRING_EQUAL(value, "no");

	end_call(pauline, marie);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void call_paused_resumed_base(bool_t multicast) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall* call_pauline;
	const rtp_stats_t * stats;
	bool_t call_ok;

	linphone_core_enable_audio_multicast(pauline->lc,multicast);

	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));

	if (!call_ok) goto end;

	call_pauline = linphone_core_get_current_call(pauline->lc);

	wait_for_until(pauline->lc, marie->lc, NULL, 5, 3000);

	linphone_core_pause_call(pauline->lc,call_pauline);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausing,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPaused,1));

	/*stay in pause a little while in order to generate traffic*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);

	linphone_core_resume_call(pauline->lc,call_pauline);

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	/*same here: wait a while for a bit of a traffic, we need to receive a RTCP packet*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 5000);

	/*since RTCP streams are reset when call is paused/resumed, there should be no loss at all*/
	stats = rtp_session_get_stats(call_pauline->sessions->rtp_session);
	BC_ASSERT_EQUAL((int)stats->cum_packet_loss, 0, int, "%d");

	end_call(pauline, marie);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void call_paused_resumed(void) {
	call_paused_resumed_base(FALSE);
}

static void call_paused_by_both(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall* call_pauline, *call_marie;
	const rtp_stats_t * stats;
	MSList *lcs = NULL;
	bool_t call_ok;

	lcs = ms_list_append(lcs, pauline->lc);
	lcs = ms_list_append(lcs, marie->lc);
	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));

	if (!call_ok) goto end;

	call_pauline = linphone_core_get_current_call(pauline->lc);
	call_marie = linphone_core_get_current_call(marie->lc);

	wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);

	linphone_core_pause_call(pauline->lc,call_pauline);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausing,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPaused,1));

	/*stay in pause a little while in order to generate traffic*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);

	/*marie pauses the call also*/
	linphone_core_pause_call(marie->lc, call_marie);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausing,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPaused,1));

	wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);
	/*pauline must stay in paused state*/
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallPaused, 1, int, "%i");
	check_media_direction(pauline, call_pauline, lcs, LinphoneMediaDirectionInactive, LinphoneMediaDirectionInvalid);
	check_media_direction(marie, call_marie, lcs, LinphoneMediaDirectionInactive, LinphoneMediaDirectionInvalid);

	/*now pauline wants to resume*/
	linphone_core_resume_call(pauline->lc, call_pauline);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallResuming,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausedByRemote,1));
	/*Marie must stay in paused state*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallPaused, 1, int, "%i");

	/*now marie wants to resume also*/
	linphone_core_resume_call(marie->lc, call_marie);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallResuming,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	/*same here: wait a while for a bit of a traffic, we need to receive a RTCP packet*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 5000);

	/*since RTCP streams are reset when call is paused/resumed, there should be no loss at all*/
	stats = rtp_session_get_stats(call_pauline->sessions->rtp_session);
	BC_ASSERT_EQUAL((int)stats->cum_packet_loss, 0, int, "%d");

	end_call(marie, pauline);

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	ms_list_free(lcs);
}

#ifdef VIDEO_ENABLED
static void call_paused_resumed_with_video_base_call_cb(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *message) {
	if (cstate == LinphoneCallUpdatedByRemote) {
		LinphoneCallParams *params = linphone_core_create_call_params(lc, call);
		linphone_call_params_enable_video(params, TRUE);
		ms_message (" New state LinphoneCallUpdatedByRemote on call [%p], accepting with video on",call);
		BC_ASSERT_NOT_EQUAL(linphone_core_accept_call_update(lc, call, params), 0, int, "%i");
		linphone_call_params_destroy(params);
	}
}
/*this test makes sure that pause/resume will not bring up video by accident*/
static void call_paused_resumed_with_video_base(bool_t sdp_200_ack
												,bool_t use_video_policy_for_re_invite_sdp_200
												,bool_t resume_in_audio_send_only_video_inactive_first
												,bool_t with_call_accept){
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall* call_pauline, *call_marie;
	MSList *lcs = NULL;
	LinphoneVideoPolicy vpol;
	bool_t call_ok;
	LinphoneCoreVTable *vtable = linphone_core_v_table_new();
	vtable->call_state_changed = call_paused_resumed_with_video_base_call_cb;
	lcs = ms_list_append(lcs, pauline->lc);
	lcs = ms_list_append(lcs, marie->lc);

	vpol.automatically_accept = FALSE;
	vpol.automatically_initiate = TRUE; /* needed to present a video mline*/

	linphone_core_set_video_policy(marie->lc, &vpol);
	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, TRUE);

	vpol.automatically_accept = FALSE;
	vpol.automatically_initiate = TRUE;

	linphone_core_set_video_policy(pauline->lc, &vpol);
	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, TRUE);

	BC_ASSERT_TRUE((call_ok=call(marie, pauline)));

	if (!call_ok) goto end;

	call_pauline = linphone_core_get_current_call(pauline->lc);
	call_marie = linphone_core_get_current_call(marie->lc);

	wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);

	linphone_core_pause_call(pauline->lc,call_pauline);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausing,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,1));
	BC_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_remote_params(call_marie)));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPaused,1));

	/*stay in pause a little while in order to generate traffic*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);

	/*check if video stream is still offered even if disabled*/

	BC_ASSERT_EQUAL(call_pauline->localdesc->nb_streams, 2, int, "%i");
	BC_ASSERT_EQUAL(call_marie->localdesc->nb_streams, 2, int, "%i");

	linphone_core_enable_sdp_200_ack(pauline->lc,sdp_200_ack);

	if (use_video_policy_for_re_invite_sdp_200) {
		LpConfig *marie_lp;
		marie_lp = linphone_core_get_config(marie->lc);
		lp_config_set_int(marie_lp,"sip","sdp_200_ack_follow_video_policy",1);
	}
	/*now pauline wants to resume*/
	if (resume_in_audio_send_only_video_inactive_first) {
		LinphoneCallParams *params = linphone_core_create_call_params(pauline->lc, call_pauline);
		linphone_call_params_set_video_direction(params,LinphoneMediaDirectionInactive);
		linphone_call_params_set_audio_direction(params,LinphoneMediaDirectionSendOnly);
		linphone_core_update_call(pauline->lc,call_pauline,params);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,2));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallUpdating,1));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
		linphone_call_params_set_video_direction(params,LinphoneMediaDirectionSendRecv);
		linphone_call_params_set_audio_direction(params,LinphoneMediaDirectionSendRecv);
		if (with_call_accept) {
			linphone_core_add_listener(marie->lc, vtable);
		}
		linphone_core_update_call(pauline->lc,call_pauline,params);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,3));
		linphone_call_params_destroy(params);
	} else {
		linphone_core_resume_call(pauline->lc, call_pauline);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallResuming,1));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	}

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));

	if (use_video_policy_for_re_invite_sdp_200) {
		/*make sure video was offered*/
		BC_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_remote_params(call_pauline)));
	} else {
		BC_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(call_pauline)));
		BC_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(call_marie)));
	}
	end_call(marie, pauline);

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	ms_list_free(lcs);
}
static void call_paused_resumed_with_video(void){
	call_paused_resumed_with_video_base(FALSE, FALSE,FALSE,FALSE);
}

static void call_paused_resumed_with_no_sdp_ack(void){
	call_paused_resumed_with_video_base(TRUE, FALSE,FALSE,FALSE);
}
static void call_paused_resumed_with_no_sdp_ack_using_video_policy(void){
	call_paused_resumed_with_video_base(TRUE, TRUE,FALSE,FALSE);
}
static void call_paused_updated_resumed_with_no_sdp_ack_using_video_policy(void){
	call_paused_resumed_with_video_base(TRUE, TRUE,TRUE,FALSE);
}
static void call_paused_updated_resumed_with_no_sdp_ack_using_video_policy_and_accept_call_update(void){
	call_paused_resumed_with_video_base(TRUE, TRUE,TRUE,TRUE);
}
#endif
#define CHECK_CURRENT_LOSS_RATE() \
	rtcp_count_current = pauline->stat.number_of_rtcp_sent; \
	/*wait for an RTCP packet to have an accurate cumulative lost value*/ \
	BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &pauline->stat.number_of_rtcp_sent, rtcp_count_current+1, 10000)); \
	stats = rtp_session_get_stats(call_pauline->audiostream->ms.sessions.rtp_session); \
	loss_percentage = stats->cum_packet_loss * 100.f / (stats->packet_recv + stats->cum_packet_loss); \
	BC_ASSERT_GREATER(loss_percentage, .75f * params.loss_rate, float, "%f"); \
	BC_ASSERT_LOWER(loss_percentage , 1.25f * params.loss_rate, float, "%f")

static void call_paused_resumed_with_loss(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall* call_pauline;
	const rtp_stats_t * stats;
	float loss_percentage;
	int rtcp_count_current;

	OrtpNetworkSimulatorParams params={0};
	params.enabled=TRUE;
	params.loss_rate=20;

	BC_ASSERT_TRUE(call(pauline,marie));
	call_pauline = linphone_core_get_current_call(pauline->lc);
	if (call_pauline){
		rtp_session_enable_network_simulation(call_pauline->audiostream->ms.sessions.rtp_session,&params);

		/*generate some traffic*/
		wait_for_until(pauline->lc, marie->lc, NULL, 5, 10000);
		CHECK_CURRENT_LOSS_RATE();

		/*pause call*/
		linphone_core_pause_call(pauline->lc,call_pauline);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausing,1));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,1));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPaused,1));
		/*stay in pause a little while in order to generate traffic*/
		wait_for_until(pauline->lc, marie->lc, NULL, 5, 10000);
		CHECK_CURRENT_LOSS_RATE();

		/*resume*/
		linphone_core_resume_call(pauline->lc,call_pauline);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
		wait_for_until(pauline->lc, marie->lc, NULL, 5, 10000);

		/*since stats are NOT totally reset during pause, the stats->packet_recv is computed from
		the start of call. This test ensures that the loss rate is consistent during the entire call.*/
		CHECK_CURRENT_LOSS_RATE();
		end_call(marie, pauline);
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

bool_t pause_call_1(LinphoneCoreManager* mgr_1,LinphoneCall* call_1,LinphoneCoreManager* mgr_2,LinphoneCall* call_2) {
	stats initial_call_stat_1=mgr_1->stat;
	stats initial_call_stat_2=mgr_2->stat;
	linphone_core_pause_call(mgr_1->lc,call_1);
	BC_ASSERT_TRUE(wait_for(mgr_1->lc,mgr_2->lc,&mgr_1->stat.number_of_LinphoneCallPausing,initial_call_stat_1.number_of_LinphoneCallPausing+1));
	BC_ASSERT_TRUE(wait_for(mgr_1->lc,mgr_2->lc,&mgr_1->stat.number_of_LinphoneCallPaused,initial_call_stat_1.number_of_LinphoneCallPaused+1));
	BC_ASSERT_TRUE(wait_for(mgr_1->lc,mgr_2->lc,&mgr_2->stat.number_of_LinphoneCallPausedByRemote,initial_call_stat_2.number_of_LinphoneCallPausedByRemote+1));
	BC_ASSERT_EQUAL(linphone_call_get_state(call_1),LinphoneCallPaused, int, "%d");
	BC_ASSERT_EQUAL(linphone_call_get_state(call_2),LinphoneCallPausedByRemote, int, "%d");
	return linphone_call_get_state(call_1) == LinphoneCallPaused && linphone_call_get_state(call_2)==LinphoneCallPausedByRemote;
}
#if 0
void concurrent_paused_resumed_base(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall* call_pauline,call_marie;
	const rtp_stats_t * stats;


	BC_ASSERT_TRUE(call(pauline,marie));

	call_pauline = linphone_core_get_current_call(pauline->lc);
	call_marie = linphone_core_get_current_call(marie->lc);

	linphone_core_pause_call(pauline->lc,call_pauline);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausing,1));

	linphone_core_pause_call(marie->lc,call_marie);

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPaused,1));

	/*stay in pause a little while in order to generate traffic*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);

	linphone_core_resume_call(pauline->lc,call_pauline);

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	/*same here: wait a while for a bit of a traffic, we need to receive a RTCP packet*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 5000);

	/*since RTCP streams are reset when call is paused/resumed, there should be no loss at all*/
	stats = rtp_session_get_stats(call_pauline->sessions->rtp_session);
	BC_ASSERT_EQUAL(stats->cum_packet_loss, 0, int, "%d");


	linphone_core_terminate_all_calls(pauline->lc);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));


	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
#endif
static void call_paused_resumed_from_callee(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall* call_marie;
	const rtp_stats_t * stats;
	bool_t call_ok;

	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));
	if (!call_ok) goto end;
	call_marie = linphone_core_get_current_call(marie->lc);

	linphone_core_pause_call(marie->lc,call_marie);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausing,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausedByRemote,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPaused,1));

	/*stay in pause a little while in order to generate traffic*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);

	linphone_core_resume_call(marie->lc,call_marie);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	/*same here: wait a while for a bit of a traffic, we need to receive a RTCP packet*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 5000);

	/*since RTCP streams are reset when call is paused/resumed, there should be no loss at all*/
	stats = rtp_session_get_stats(call_marie->sessions->rtp_session);
	BC_ASSERT_EQUAL((int)stats->cum_packet_loss, 0, int, "%d");

	end_call(pauline, marie);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void audio_call_with_ice_no_matching_audio_codecs(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall *out_call;

	linphone_core_enable_payload_type(marie->lc, linphone_core_find_payload_type(marie->lc, "PCMU", 8000, 1), FALSE); /* Disable PCMU */
	linphone_core_enable_payload_type(marie->lc, linphone_core_find_payload_type(marie->lc, "PCMA", 8000, 1), TRUE); /* Enable PCMA */
	linphone_core_set_firewall_policy(marie->lc, LinphonePolicyUseIce);
	linphone_core_set_firewall_policy(pauline->lc, LinphonePolicyUseIce);

	out_call = linphone_core_invite_address(marie->lc, pauline->identity);
	linphone_call_ref(out_call);
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallOutgoingInit, 1));

	/* flexisip will retain the 488 until the "urgent reply" timeout arrives. */
	BC_ASSERT_TRUE(wait_for_until(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallError, 1, 6000));
	BC_ASSERT_EQUAL(linphone_call_get_reason(out_call), LinphoneReasonNotAcceptable, int, "%d");
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallIncomingReceived, 0, int, "%d");

	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#ifdef VIDEO_ENABLED
static LinphoneCall* setup_video(LinphoneCoreManager* caller,LinphoneCoreManager* callee, bool_t change_video_policy) {
	LinphoneVideoPolicy  caller_policy;
	LinphoneCallParams* callee_params;
	LinphoneCall* call_obj;

	if (!linphone_core_get_current_call(callee->lc) || linphone_call_get_state(linphone_core_get_current_call(callee->lc)) != LinphoneCallStreamsRunning
			|| !linphone_core_get_current_call(caller->lc) || linphone_call_get_state(linphone_core_get_current_call(caller->lc)) != LinphoneCallStreamsRunning ) {
		ms_warning("bad state for adding video");
		return NULL;
	}

	if (change_video_policy) {
		caller_policy.automatically_accept=TRUE;
		caller_policy.automatically_initiate=TRUE;
		linphone_core_set_video_policy(caller->lc,&caller_policy);
	}
	linphone_core_enable_video_capture(callee->lc, TRUE);
	linphone_core_enable_video_display(callee->lc, TRUE);
	linphone_core_enable_video_capture(caller->lc, TRUE);
	linphone_core_enable_video_display(caller->lc, FALSE);

	if ((call_obj = linphone_core_get_current_call(callee->lc))) {
		callee_params = linphone_core_create_call_params(callee->lc, call_obj);
		/*add video*/
		linphone_call_params_enable_video(callee_params,TRUE);
		linphone_core_update_call(callee->lc,call_obj,callee_params);
		linphone_call_params_destroy(callee_params);
	}
	return call_obj;
}

bool_t add_video(LinphoneCoreManager* caller,LinphoneCoreManager* callee, bool_t change_video_policy) {
	stats initial_caller_stat=caller->stat;
	stats initial_callee_stat=callee->stat;
	const LinphoneVideoPolicy *video_policy;
	LinphoneCall *call_obj;
	if ((call_obj=setup_video(caller, callee, change_video_policy))){
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&caller->stat.number_of_LinphoneCallUpdatedByRemote,initial_caller_stat.number_of_LinphoneCallUpdatedByRemote+1));
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallUpdating,initial_callee_stat.number_of_LinphoneCallUpdating+1));
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallStreamsRunning,initial_callee_stat.number_of_LinphoneCallStreamsRunning+1));
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&caller->stat.number_of_LinphoneCallStreamsRunning,initial_caller_stat.number_of_LinphoneCallStreamsRunning+1));

		video_policy = linphone_core_get_video_policy(caller->lc);
		if (video_policy->automatically_accept) {
			BC_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(callee->lc))));
			BC_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(caller->lc))));
		} else {
			BC_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(callee->lc))));
			BC_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(caller->lc))));
		}
		if (linphone_core_get_media_encryption(caller->lc) != LinphoneMediaEncryptionNone
				&& linphone_core_get_media_encryption(callee->lc) != LinphoneMediaEncryptionNone) {
			const LinphoneCallParams* call_param;

			switch (linphone_core_get_media_encryption(caller->lc)) {
			case LinphoneMediaEncryptionZRTP:
			case LinphoneMediaEncryptionDTLS:
				/*wait for encryption to be on, in case of zrtp/dtls, it can take a few seconds*/
				wait_for(callee->lc,caller->lc,&caller->stat.number_of_LinphoneCallEncryptedOn,initial_caller_stat.number_of_LinphoneCallEncryptedOn+1);
			break;
			case LinphoneMediaEncryptionNone:
			case LinphoneMediaEncryptionSRTP:
				break;
			}
			switch (linphone_core_get_media_encryption(callee->lc)) {
			case LinphoneMediaEncryptionZRTP:
			case LinphoneMediaEncryptionDTLS:
				wait_for(callee->lc,caller->lc,&callee->stat.number_of_LinphoneCallEncryptedOn,initial_callee_stat.number_of_LinphoneCallEncryptedOn+1);
			break;
			case LinphoneMediaEncryptionNone:
			case LinphoneMediaEncryptionSRTP:
				break;
			}

			call_param = linphone_call_get_current_params(linphone_core_get_current_call(callee->lc));
			BC_ASSERT_EQUAL(linphone_call_params_get_media_encryption(call_param),linphone_core_get_media_encryption(caller->lc), int, "%d");
			call_param = linphone_call_get_current_params(linphone_core_get_current_call(caller->lc));
			BC_ASSERT_EQUAL(linphone_call_params_get_media_encryption(call_param),linphone_core_get_media_encryption(caller->lc), int, "%d");

		}

		if (video_policy->automatically_accept) {
			linphone_call_set_next_video_frame_decoded_callback(call_obj,linphone_call_iframe_decoded_cb,callee->lc);
			/*send vfu*/
			linphone_call_send_vfu_request(call_obj);
			return wait_for(caller->lc,callee->lc,&callee->stat.number_of_IframeDecoded,initial_callee_stat.number_of_IframeDecoded+1);
		} else {
			return TRUE;
		}
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
		callee_params = linphone_core_create_call_params(callee->lc, call_obj);
		/* Remove video. */
		linphone_call_params_enable_video(callee_params, FALSE);
		linphone_core_update_call(callee->lc, call_obj, callee_params);
		linphone_call_params_destroy(callee_params);

		BC_ASSERT_TRUE(wait_for(caller->lc, callee->lc, &caller->stat.number_of_LinphoneCallUpdatedByRemote, initial_caller_stat.number_of_LinphoneCallUpdatedByRemote + 1));
		BC_ASSERT_TRUE(wait_for(caller->lc, callee->lc, &callee->stat.number_of_LinphoneCallUpdating, initial_callee_stat.number_of_LinphoneCallUpdating + 1));
		BC_ASSERT_TRUE(wait_for(caller->lc, callee->lc, &callee->stat.number_of_LinphoneCallStreamsRunning, initial_callee_stat.number_of_LinphoneCallStreamsRunning + 1));
		BC_ASSERT_TRUE(wait_for(caller->lc, callee->lc, &caller->stat.number_of_LinphoneCallStreamsRunning, initial_caller_stat.number_of_LinphoneCallStreamsRunning + 1));

		BC_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(callee->lc))));
		BC_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(caller->lc))));

		return TRUE;
	}
	return FALSE;
}

static void call_with_video_added(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok;

	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));
	if (!call_ok) goto end;

	BC_ASSERT_TRUE(add_video(pauline,marie, TRUE));

	end_call(pauline, marie);

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_video_added_2(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok;
	/*in this variant marie is already in automatically accept*/
	LinphoneVideoPolicy  marie_policy;
	marie_policy.automatically_accept=TRUE;


	linphone_core_set_video_policy(marie->lc,&marie_policy);
	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, FALSE);

	BC_ASSERT_TRUE(call_ok=call(pauline,marie));
	if (!call_ok) goto end;

	BC_ASSERT_TRUE(add_video(marie,pauline, TRUE));

	end_call(pauline, marie);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_video_added_random_ports(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok;

	linphone_core_set_audio_port(marie->lc,-1);
	linphone_core_set_video_port(marie->lc,-1);
	linphone_core_set_audio_port(pauline->lc,-1);
	linphone_core_set_video_port(pauline->lc,-1);

	BC_ASSERT_TRUE(call_ok=call(pauline,marie));
	if (!call_ok) goto end;

	BC_ASSERT_TRUE(add_video(pauline,marie, TRUE));
	end_call(pauline, marie);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_several_video_switches(void) {
	int dummy = 0;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok;
	BC_ASSERT_TRUE(call_ok=call(pauline,marie));

	if (!call_ok) goto end;

	BC_ASSERT_TRUE(add_video(pauline,marie, TRUE));
	wait_for_until(pauline->lc,marie->lc,&dummy,1,1000); /* Wait for VFU request exchanges to be finished. */
	BC_ASSERT_TRUE(remove_video(pauline,marie));
	BC_ASSERT_TRUE(add_video(pauline,marie, TRUE));
	wait_for_until(pauline->lc,marie->lc,&dummy,1,1000); /* Wait for VFU request exchanges to be finished. */
	BC_ASSERT_TRUE(remove_video(pauline,marie));
	/**/
	end_call(pauline, marie);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void srtp_call_with_several_video_switches(void) {
	int dummy = 0;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok;

	if (linphone_core_media_encryption_supported(marie->lc, LinphoneMediaEncryptionSRTP)) {
		linphone_core_set_media_encryption(marie->lc, LinphoneMediaEncryptionSRTP);
		linphone_core_set_media_encryption(pauline->lc, LinphoneMediaEncryptionSRTP);

		BC_ASSERT_TRUE(call_ok=call(pauline,marie));
		if (!call_ok) goto end;

		BC_ASSERT_TRUE(add_video(pauline,marie, TRUE));
		wait_for_until(pauline->lc,marie->lc,&dummy,1,1000); /* Wait for VFU request exchanges to be finished. */
		BC_ASSERT_TRUE(remove_video(pauline,marie));
		BC_ASSERT_TRUE(add_video(pauline,marie, TRUE));
		wait_for_until(pauline->lc,marie->lc,&dummy,1,1000); /* Wait for VFU request exchanges to be finished. */
		BC_ASSERT_TRUE(remove_video(pauline,marie));
		/**/
		end_call(pauline, marie);
	} else {
		ms_warning("Not tested because SRTP is not available.");
	}
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_declined_video_base(bool_t using_policy) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall* marie_call;
	LinphoneCall* pauline_call;
	LinphoneVideoPolicy marie_policy, pauline_policy;
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params = {0};
	bool_t call_ok;

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

	caller_test_params.base=linphone_core_create_call_params(pauline->lc, NULL);
	if (!using_policy)
		linphone_call_params_enable_video(caller_test_params.base,TRUE);

	if (!using_policy){
		callee_test_params.base=linphone_core_create_call_params(marie->lc, NULL);
		linphone_call_params_enable_video(callee_test_params.base,FALSE);
	}

	BC_ASSERT_TRUE((call_ok=call_with_params2(pauline,marie,&caller_test_params,&callee_test_params,using_policy)));
	if (!call_ok) goto end;

	linphone_call_params_destroy(caller_test_params.base);
	if (callee_test_params.base) linphone_call_params_destroy(callee_test_params.base);
	marie_call=linphone_core_get_current_call(marie->lc);
	pauline_call=linphone_core_get_current_call(pauline->lc);

	BC_ASSERT_FALSE(linphone_call_log_video_enabled(linphone_call_get_call_log(marie_call)));
	BC_ASSERT_FALSE(linphone_call_log_video_enabled(linphone_call_get_call_log(pauline_call)));

	end_call(pauline, marie);

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void call_with_declined_video(void) {
	call_with_declined_video_base(FALSE);
}

static void call_with_declined_video_despite_policy(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall* marie_call;
	LinphoneCall* pauline_call;
	LinphoneVideoPolicy marie_policy, pauline_policy;
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params = {0};
	bool_t call_ok;

	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, TRUE);
	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, FALSE);

	pauline_policy.automatically_initiate=TRUE;
	pauline_policy.automatically_accept=TRUE;
	marie_policy.automatically_initiate=TRUE;
	marie_policy.automatically_accept=TRUE;

	linphone_core_set_video_policy(marie->lc,&marie_policy);
	linphone_core_set_video_policy(pauline->lc,&pauline_policy);

	caller_test_params.base=linphone_core_create_call_params(pauline->lc, NULL);

	callee_test_params.base=linphone_core_create_call_params(marie->lc, NULL);
	linphone_call_params_enable_video(callee_test_params.base,FALSE);

	BC_ASSERT_TRUE((call_ok=call_with_params2(pauline,marie,&caller_test_params,&callee_test_params,FALSE)));
	if (!call_ok) goto end;

	linphone_call_params_destroy(caller_test_params.base);
	if (callee_test_params.base) linphone_call_params_destroy(callee_test_params.base);
	marie_call=linphone_core_get_current_call(marie->lc);
	pauline_call=linphone_core_get_current_call(pauline->lc);

	BC_ASSERT_FALSE(linphone_call_log_video_enabled(linphone_call_get_call_log(marie_call)));
	BC_ASSERT_FALSE(linphone_call_log_video_enabled(linphone_call_get_call_log(pauline_call)));

	end_call(pauline, marie);

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_declined_video_using_policy(void) {
	call_with_declined_video_base(TRUE);
}


void video_call_base_2(LinphoneCoreManager* caller,LinphoneCoreManager* callee, bool_t using_policy,LinphoneMediaEncryption mode, bool_t callee_video_enabled, bool_t caller_video_enabled) {
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params = {0};
	LinphoneCall* callee_call;
	LinphoneCall* caller_call;
	LinphoneVideoPolicy callee_policy, caller_policy;

	if (using_policy) {
		callee_policy.automatically_initiate=FALSE;
		callee_policy.automatically_accept=TRUE;
		caller_policy.automatically_initiate=TRUE;
		caller_policy.automatically_accept=FALSE;

		linphone_core_set_video_policy(callee->lc,&callee_policy);
		linphone_core_set_video_policy(caller->lc,&caller_policy);
	}

	linphone_core_enable_video_display(callee->lc, callee_video_enabled);
	linphone_core_enable_video_capture(callee->lc, callee_video_enabled);

	linphone_core_enable_video_display(caller->lc, caller_video_enabled);
	linphone_core_enable_video_capture(caller->lc, caller_video_enabled);

	if (mode==LinphoneMediaEncryptionDTLS) { /* for DTLS we must access certificates or at least have a directory to store them */
		char *path = bc_tester_file("certificates-marie");
		callee->lc->user_certificates_path = ms_strdup(path);
		bc_free(path);
		path = bc_tester_file("certificates-pauline");
		caller->lc->user_certificates_path = ms_strdup(path);
		bc_free(path);
		belle_sip_mkdir(callee->lc->user_certificates_path);
		belle_sip_mkdir(caller->lc->user_certificates_path);
	}

	linphone_core_set_media_encryption(callee->lc,mode);
	linphone_core_set_media_encryption(caller->lc,mode);

	caller_test_params.base=linphone_core_create_call_params(caller->lc, NULL);
	if (!using_policy)
		linphone_call_params_enable_video(caller_test_params.base,TRUE);

	if (!using_policy){
		callee_test_params.base=linphone_core_create_call_params(callee->lc, NULL);
		linphone_call_params_enable_video(callee_test_params.base,TRUE);
	}

	BC_ASSERT_TRUE(call_with_params2(caller,callee,&caller_test_params,&callee_test_params,using_policy));
	callee_call=linphone_core_get_current_call(callee->lc);
	caller_call=linphone_core_get_current_call(caller->lc);

	linphone_call_params_destroy(caller_test_params.base);
	if (callee_test_params.base) linphone_call_params_destroy(callee_test_params.base);

	if (callee_call && caller_call ) {
		if (callee_video_enabled && caller_video_enabled) {
			BC_ASSERT_TRUE(linphone_call_log_video_enabled(linphone_call_get_call_log(callee_call)));
			BC_ASSERT_TRUE(linphone_call_log_video_enabled(linphone_call_get_call_log(caller_call)));

			/*check video path*/
			linphone_call_set_next_video_frame_decoded_callback(callee_call,linphone_call_iframe_decoded_cb,callee->lc);
			linphone_call_send_vfu_request(callee_call);
			BC_ASSERT_TRUE( wait_for(callee->lc,caller->lc,&callee->stat.number_of_IframeDecoded,1));
		} else {
			BC_ASSERT_FALSE(linphone_call_log_video_enabled(linphone_call_get_call_log(callee_call)));
			BC_ASSERT_FALSE(linphone_call_log_video_enabled(linphone_call_get_call_log(caller_call)));
		}
		liblinphone_tester_check_rtcp(callee,caller);
	}
}


static void check_fir(LinphoneCoreManager* caller,LinphoneCoreManager* callee ){
	LinphoneCall* callee_call;
	LinphoneCall* caller_call;

	callee_call=linphone_core_get_current_call(callee->lc);
	caller_call=linphone_core_get_current_call(caller->lc);

	/*check video path is established in both directions.
	 Indeed, FIR are ignored until the first RTP packet is received, because SSRC is not known.*/
	linphone_call_set_next_video_frame_decoded_callback(callee_call,linphone_call_iframe_decoded_cb,callee->lc);
	linphone_call_set_next_video_frame_decoded_callback(caller_call,linphone_call_iframe_decoded_cb,caller->lc);

	BC_ASSERT_TRUE( wait_for(callee->lc,caller->lc,&callee->stat.number_of_IframeDecoded,1));
	BC_ASSERT_TRUE( wait_for(callee->lc,caller->lc,&caller->stat.number_of_IframeDecoded,1));

	linphone_call_send_vfu_request(callee_call);

	if (rtp_session_avpf_enabled(callee_call->sessions->rtp_session)){
		BC_ASSERT_TRUE(wait_for(callee->lc,caller->lc,&caller_call->videostream->ms_video_stat.counter_rcvd_fir, 1));
	}else{
		BC_ASSERT_TRUE(wait_for(callee->lc,caller->lc,&caller_call->videostream->ms_video_stat.counter_rcvd_fir, 0));
	}
	ms_message ("check_fir : [%p] received  %d FIR  ",&caller_call ,caller_call->videostream->ms_video_stat.counter_rcvd_fir);
	ms_message ("check_fir : [%p] stat number of iframe decoded  %d ",&callee_call, callee->stat.number_of_IframeDecoded);

	linphone_call_set_next_video_frame_decoded_callback(caller_call,linphone_call_iframe_decoded_cb,caller->lc);
	linphone_call_send_vfu_request(caller_call);
	BC_ASSERT_TRUE( wait_for(callee->lc,caller->lc,&caller->stat.number_of_IframeDecoded,1));

	if (rtp_session_avpf_enabled(caller_call->sessions->rtp_session)) {
		if (rtp_session_avpf_enabled(callee_call->sessions->rtp_session)){
			BC_ASSERT_TRUE(wait_for(callee->lc,caller->lc,&callee_call->videostream->ms_video_stat.counter_rcvd_fir, 1));
		}
	}else{
		BC_ASSERT_TRUE(wait_for(callee->lc,caller->lc,&callee_call->videostream->ms_video_stat.counter_rcvd_fir, 0));
	}
	ms_message ("check_fir : [%p] received  %d FIR  ",&callee_call ,callee_call->videostream->ms_video_stat.counter_rcvd_fir);
	ms_message ("check_fir : [%p] stat number of iframe decoded  %d ",&caller_call, caller->stat.number_of_IframeDecoded);

}

void video_call_base_3(LinphoneCoreManager* caller,LinphoneCoreManager* callee, bool_t using_policy,LinphoneMediaEncryption mode, bool_t callee_video_enabled, bool_t caller_video_enabled) {
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params = {0};

	LinphoneCall* callee_call;
	LinphoneCall* caller_call;
	LinphoneVideoPolicy callee_policy, caller_policy;

	if (using_policy) {
		callee_policy.automatically_initiate=FALSE;
		callee_policy.automatically_accept=TRUE;
		caller_policy.automatically_initiate=TRUE;
		caller_policy.automatically_accept=FALSE;

		linphone_core_set_video_policy(callee->lc,&callee_policy);
		linphone_core_set_video_policy(caller->lc,&caller_policy);
	}

	linphone_core_enable_video_display(callee->lc, callee_video_enabled);
	linphone_core_enable_video_capture(callee->lc, callee_video_enabled);

	linphone_core_enable_video_display(caller->lc, caller_video_enabled);
	linphone_core_enable_video_capture(caller->lc, caller_video_enabled);

	if (mode==LinphoneMediaEncryptionDTLS) { /* for DTLS we must access certificates or at least have a directory to store them */
		char *path = bc_tester_file("certificates-marie");
		callee->lc->user_certificates_path = ms_strdup(path);
		bc_free(path);
		path = bc_tester_file("certificates-pauline");
		caller->lc->user_certificates_path = ms_strdup(path);
		bc_free(path);
		belle_sip_mkdir(callee->lc->user_certificates_path);
		belle_sip_mkdir(caller->lc->user_certificates_path);
	}

	linphone_core_set_media_encryption(callee->lc,mode);
	linphone_core_set_media_encryption(caller->lc,mode);
	/* Create call params */
	caller_test_params.base=linphone_core_create_call_params(caller->lc, NULL);

	if (!using_policy)
		linphone_call_params_enable_video(caller_test_params.base,TRUE);

	if (!using_policy){
		callee_test_params.base=linphone_core_create_call_params(callee->lc, NULL);
		linphone_call_params_enable_video(callee_test_params.base,TRUE);
	}

	BC_ASSERT_TRUE(call_with_params2(caller,callee,&caller_test_params,&callee_test_params,using_policy));
	callee_call=linphone_core_get_current_call(callee->lc);
	caller_call=linphone_core_get_current_call(caller->lc);

	linphone_call_params_destroy(caller_test_params.base);
	if (callee_test_params.base) linphone_call_params_destroy(callee_test_params.base);

	if (callee_call && caller_call ) {
		if (callee_video_enabled && caller_video_enabled) {
			check_fir(caller,callee);
		} else {
			BC_ASSERT_FALSE(linphone_call_log_video_enabled(linphone_call_get_call_log(callee_call)));
			BC_ASSERT_FALSE(linphone_call_log_video_enabled(linphone_call_get_call_log(caller_call)));
		}
		liblinphone_tester_check_rtcp(callee,caller);
	}
}



static void video_call_base(LinphoneCoreManager* pauline,LinphoneCoreManager* marie, bool_t using_policy,LinphoneMediaEncryption mode, bool_t callee_video_enabled, bool_t caller_video_enabled) {
	video_call_base_2(pauline,marie,using_policy,mode,callee_video_enabled,caller_video_enabled);
	end_call(pauline, marie);
}

static void video_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	video_call_base(marie,pauline,FALSE,LinphoneMediaEncryptionNone,TRUE,TRUE);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void video_call_without_rtcp(void) {
	LpConfig   *lp;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	lp = linphone_core_get_config(marie->lc);
	lp_config_set_int(lp,"rtp","rtcp_enabled",0);

	lp = linphone_core_get_config(pauline->lc);
	lp_config_set_int(lp,"rtp","rtcp_enabled",0);

	video_call_base(marie,pauline,FALSE,LinphoneMediaEncryptionNone,TRUE,TRUE);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void video_call_disable_implicit_AVPF_on_callee(void) {
    LinphoneCoreManager* callee = linphone_core_manager_new("marie_rc");
    LinphoneCoreManager* caller = linphone_core_manager_new(transport_supported(LinphoneTransportTcp) ? "pauline_rc" : "pauline_tcp_rc");
    LpConfig   *callee_lp;
    const LinphoneCallParams *params, *params2;

    callee_lp = linphone_core_get_config(callee->lc);
    lp_config_set_int(callee_lp,"rtp","rtcp_fb_implicit_rtcp_fb",0);

    video_call_base_3(caller,callee,TRUE,LinphoneMediaEncryptionNone,TRUE,TRUE);
    params = linphone_call_get_current_params(linphone_core_get_current_call(callee->lc));
    BC_ASSERT_STRING_EQUAL(linphone_call_params_get_rtp_profile(params), "RTP/AVP");
    params2 =linphone_call_get_current_params(linphone_core_get_current_call(caller->lc));
    BC_ASSERT_STRING_EQUAL(linphone_call_params_get_rtp_profile(params2), "RTP/AVP");
    end_call(caller, callee);
    linphone_core_manager_destroy(callee);
    linphone_core_manager_destroy(caller);

}


static void video_call_disable_implicit_AVPF_on_caller(void) {
    LinphoneCoreManager* callee = linphone_core_manager_new("marie_rc");
    LinphoneCoreManager* caller = linphone_core_manager_new(transport_supported(LinphoneTransportTcp) ? "pauline_rc" : "pauline_tcp_rc");
    LpConfig   *caller_lp;
    const LinphoneCallParams *params, *params2;

    caller_lp = linphone_core_get_config(caller->lc);
    lp_config_set_int(caller_lp,"rtp","rtcp_fb_implicit_rtcp_fb",0);

    video_call_base_3(caller,callee,TRUE,LinphoneMediaEncryptionNone,TRUE,TRUE);
    params = linphone_call_get_current_params(linphone_core_get_current_call(callee->lc));
    BC_ASSERT_STRING_EQUAL(linphone_call_params_get_rtp_profile(params), "RTP/AVP");
    params2 =linphone_call_get_current_params(linphone_core_get_current_call(caller->lc));
    BC_ASSERT_STRING_EQUAL(linphone_call_params_get_rtp_profile(params2), "RTP/AVP");
    end_call(caller, callee);
    linphone_core_manager_destroy(callee);
    linphone_core_manager_destroy(caller);

}

static void video_call_AVPF_to_implicit_AVPF(void)
{
    LinphoneCoreManager* callee = linphone_core_manager_new("marie_rc");
    LinphoneCoreManager* caller = linphone_core_manager_new(transport_supported(LinphoneTransportTcp) ? "pauline_rc" : "pauline_tcp_rc");

    linphone_core_set_avpf_mode(caller->lc,LinphoneAVPFEnabled);
    video_call_base_3(caller,callee,TRUE,LinphoneMediaEncryptionNone,TRUE,TRUE);
    end_call(caller,callee);

    linphone_core_manager_destroy(callee);
    linphone_core_manager_destroy(caller);

}

static void video_call_implicit_AVPF_to_AVPF(void)
{
    LinphoneCoreManager* callee = linphone_core_manager_new("marie_rc");
    LinphoneCoreManager* caller = linphone_core_manager_new(transport_supported(LinphoneTransportTcp) ? "pauline_rc" : "pauline_tcp_rc");

    linphone_core_set_avpf_mode(callee->lc,LinphoneAVPFEnabled);
    video_call_base_3(caller,callee,TRUE,LinphoneMediaEncryptionNone,TRUE,TRUE);
    end_call(caller,callee);

    linphone_core_manager_destroy(callee);
    linphone_core_manager_destroy(caller);

}

static void video_call_using_policy_AVPF_implicit_caller_and_callee(void) {
    LinphoneCoreManager* callee = linphone_core_manager_new("marie_rc");
    LinphoneCoreManager* caller = linphone_core_manager_new(transport_supported(LinphoneTransportTcp) ? "pauline_rc" : "pauline_tcp_rc");
    video_call_base_3(caller,callee,FALSE,LinphoneMediaEncryptionNone,TRUE,TRUE);
    end_call(caller, callee);
    linphone_core_manager_destroy(callee);
    linphone_core_manager_destroy(caller);
}
static void video_call_base_avpf(LinphoneCoreManager* caller,LinphoneCoreManager* callee, bool_t using_policy,LinphoneMediaEncryption mode, bool_t callee_video_enabled, bool_t caller_video_enabled) {
    linphone_core_set_avpf_mode(caller->lc,LinphoneAVPFEnabled);
    linphone_core_set_avpf_mode(callee->lc,LinphoneAVPFEnabled);
    video_call_base_3(caller,callee,using_policy,mode,callee_video_enabled,caller_video_enabled);
    end_call(caller, callee);
}

static void video_call_avpf(void) {
    LinphoneCoreManager* callee = linphone_core_manager_new("marie_rc");
    LinphoneCoreManager* caller = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

    video_call_base_avpf(caller,callee,FALSE,LinphoneMediaEncryptionNone,TRUE,TRUE);
    linphone_core_manager_destroy(callee);
    linphone_core_manager_destroy(caller);

}

static void video_call_zrtp(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	if (linphone_core_media_encryption_supported(marie->lc,LinphoneMediaEncryptionZRTP)) {
		video_call_base(marie,pauline,FALSE,LinphoneMediaEncryptionZRTP,TRUE,TRUE);
	} else
		ms_message("Skipping video_call_zrtp");
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void video_call_dtls(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	if (linphone_core_media_encryption_supported(pauline->lc,LinphoneMediaEncryptionDTLS)) {
		video_call_base(marie,pauline,FALSE,LinphoneMediaEncryptionDTLS,TRUE,TRUE);
	} else
		ms_message("Skipping video_call_dtls");
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);

}



static void video_call_using_policy(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTcp) ? "pauline_rc" : "pauline_tcp_rc");
	video_call_base(pauline,marie,TRUE,LinphoneMediaEncryptionNone,TRUE,TRUE);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void video_call_using_policy_with_callee_video_disabled(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	video_call_base(marie,pauline,TRUE,LinphoneMediaEncryptionNone,FALSE,TRUE);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void video_call_using_policy_with_caller_video_disabled(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	video_call_base(marie,pauline,TRUE,LinphoneMediaEncryptionNone,TRUE,FALSE);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void video_call_no_sdp(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	linphone_core_enable_sdp_200_ack(pauline->lc,TRUE);
	video_call_base(pauline,marie,FALSE,LinphoneMediaEncryptionNone,TRUE,TRUE);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_ice_video_to_novideo(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneVideoPolicy vpol={0};
	vpol.automatically_initiate=TRUE;
	linphone_core_set_video_policy(pauline->lc,&vpol);
	vpol.automatically_initiate=FALSE;
	linphone_core_set_video_policy(marie->lc,&vpol);
	_call_with_ice_base(pauline,marie,TRUE,TRUE,TRUE,FALSE);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void _call_with_ice_video(LinphoneVideoPolicy caller_policy, LinphoneVideoPolicy callee_policy,
	bool_t video_added_by_caller, bool_t video_added_by_callee, bool_t video_removed_by_caller, bool_t video_removed_by_callee) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok;
	unsigned int nb_media_starts = 1;

	linphone_core_set_video_policy(pauline->lc, &caller_policy);
	linphone_core_set_video_policy(marie->lc, &callee_policy);
	linphone_core_set_firewall_policy(marie->lc, LinphonePolicyUseIce);
	linphone_core_set_firewall_policy(pauline->lc, LinphonePolicyUseIce);

	linphone_core_set_audio_port(marie->lc, -1);
	linphone_core_set_video_port(marie->lc, -1);
	linphone_core_set_audio_port(pauline->lc, -1);
	linphone_core_set_video_port(pauline->lc, -1);

	BC_ASSERT_TRUE(call_ok = call(pauline, marie));
	if (!call_ok) goto end;
	BC_ASSERT_TRUE(check_ice(pauline, marie, LinphoneIceStateHostConnection));

	/* Wait for ICE reINVITEs to complete. */
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 2)
		&& wait_for(pauline->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 2));
	check_nb_media_starts(pauline, marie, nb_media_starts, nb_media_starts);
	nb_media_starts++;

	if (video_added_by_caller) {
		BC_ASSERT_TRUE(add_video(marie, pauline, FALSE));
	} else if (video_added_by_callee) {
		BC_ASSERT_TRUE(add_video(pauline, marie, FALSE));
	}
	if (video_added_by_caller || video_added_by_callee) {
		BC_ASSERT_TRUE(check_ice(pauline, marie, LinphoneIceStateHostConnection));
		if (linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(marie->lc)))){
			/* Wait for ICE reINVITEs to complete if video was really added */
			BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 4)
				&& wait_for(pauline->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 4));
			check_nb_media_starts(pauline, marie, nb_media_starts, nb_media_starts);
			nb_media_starts++;
		}
	}

	if (video_removed_by_caller) {
		BC_ASSERT_TRUE(remove_video(marie, pauline));
	} else if (video_removed_by_callee) {
		BC_ASSERT_TRUE(remove_video(pauline, marie));
	}
	if (video_removed_by_caller || video_removed_by_callee) {
		BC_ASSERT_TRUE(check_ice(pauline, marie, LinphoneIceStateHostConnection));
		check_nb_media_starts(pauline, marie, nb_media_starts, nb_media_starts);
		nb_media_starts++;
	}

	end_call(pauline, marie);

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_ice_video_added(void) {
	LinphoneVideoPolicy vpol = { TRUE, TRUE };
	_call_with_ice_video(vpol, vpol, TRUE, FALSE, TRUE, FALSE);
}

static void call_with_ice_video_added_2(void) {
	LinphoneVideoPolicy vpol = { TRUE, TRUE };
	_call_with_ice_video(vpol, vpol, TRUE, FALSE, FALSE, TRUE);
}

static void call_with_ice_video_added_3(void) {
	LinphoneVideoPolicy vpol = { TRUE, TRUE };
	_call_with_ice_video(vpol, vpol, FALSE, TRUE, TRUE, FALSE);
}

static void call_with_ice_video_added_and_refused(void) {
	LinphoneVideoPolicy caller_policy = { TRUE, TRUE };
	LinphoneVideoPolicy callee_policy = { FALSE, FALSE };
	_call_with_ice_video(caller_policy, callee_policy, TRUE, FALSE, FALSE, FALSE);
}

static void call_with_ice_video_added_with_video_policies_to_false(void) {
	LinphoneVideoPolicy vpol = { FALSE, FALSE };
	_call_with_ice_video(vpol, vpol, FALSE, TRUE, FALSE, FALSE);
}

#if ICE_WAS_WORKING_WITH_REAL_TIME_TEXT /*which is not the case at the moment*/

static void call_with_ice_video_and_rtt(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok;
	LinphoneVideoPolicy policy = { TRUE, TRUE };
	LinphoneCallParams *params = NULL;
	LinphoneCall *marie_call = NULL;

	linphone_core_set_video_policy(pauline->lc, &policy);
	linphone_core_set_video_policy(marie->lc, &policy);
	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, FALSE);
	linphone_core_enable_video_capture(pauline->lc, FALSE);
	linphone_core_enable_video_display(pauline->lc, TRUE);
	linphone_core_set_firewall_policy(marie->lc, LinphonePolicyUseIce);
	linphone_core_set_firewall_policy(pauline->lc, LinphonePolicyUseIce);

	linphone_core_set_audio_port(marie->lc, -1);
	linphone_core_set_video_port(marie->lc, -1);
	linphone_core_set_text_port(marie->lc, -1);
	linphone_core_set_audio_port(pauline->lc, -1);
	linphone_core_set_video_port(pauline->lc, -1);
	linphone_core_set_text_port(pauline->lc, -1);

	params = linphone_core_create_default_call_parameters(pauline->lc);
	linphone_call_params_enable_realtime_text(params, TRUE);
	BC_ASSERT_TRUE(call_ok = call_with_caller_params(pauline, marie, params));
	if (!call_ok) goto end;
	BC_ASSERT_TRUE(check_ice(pauline, marie, LinphoneIceStateHostConnection));

	marie_call = linphone_core_get_current_call(marie->lc);
	BC_ASSERT_TRUE(linphone_call_params_audio_enabled(linphone_call_get_current_params(marie_call)));
	BC_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(marie_call)));
	BC_ASSERT_TRUE(linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(marie_call)));

	end_call(pauline, marie);
end:
	linphone_call_params_destroy(params);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#endif

static void video_call_with_early_media_no_matching_audio_codecs(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall *out_call, *pauline_call;
	LinphoneVideoPolicy vpol={0};

	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, TRUE);
	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, FALSE);

	vpol.automatically_initiate=TRUE;
	vpol.automatically_accept=TRUE;
	linphone_core_set_video_policy(pauline->lc,&vpol);
	linphone_core_set_video_policy(marie->lc,&vpol);

	linphone_core_enable_payload_type(marie->lc, linphone_core_find_payload_type(marie->lc, "PCMU", 8000, 1), FALSE); /* Disable PCMU */
	linphone_core_enable_payload_type(marie->lc, linphone_core_find_payload_type(marie->lc, "PCMA", 8000, 1), TRUE); /* Enable PCMA */

	out_call = linphone_core_invite_address(marie->lc, pauline->identity);
	linphone_call_ref(out_call);
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallOutgoingInit, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallIncomingReceived, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallOutgoingRinging, 1));

	pauline_call = linphone_core_get_current_call(pauline->lc);
	if (!pauline_call) goto end;

	linphone_core_accept_early_media(pauline->lc, pauline_call);

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallIncomingEarlyMedia, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallOutgoingEarlyMedia, 1));
	/*audio stream shall not have been requested to start*/
	BC_ASSERT_PTR_NULL(pauline_call->audiostream->soundread);

	BC_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(out_call)));
	BC_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(pauline_call)));

	linphone_core_accept_call(pauline->lc, pauline_call);

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 1));

	end_call(marie, pauline);

end:
	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void video_call_limited_bandwidth(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	linphone_core_set_download_bandwidth(pauline->lc, 100);
	video_call_base(marie,pauline,FALSE,LinphoneMediaEncryptionNone,TRUE,TRUE);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#endif /*VIDEO_ENABLED*/

static void _call_with_media_relay(bool_t random_ports) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok;

	linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);

	if (random_ports){
		linphone_core_set_audio_port(marie->lc,-1);
		linphone_core_set_video_port(marie->lc,-1);
		linphone_core_set_audio_port(pauline->lc,-1);
		linphone_core_set_video_port(pauline->lc,-1);
	}

	BC_ASSERT_TRUE(call_ok=call(pauline,marie));
	if (!call_ok) goto end;
	liblinphone_tester_check_rtcp(pauline,marie);

#ifdef VIDEO_ENABLED
	BC_ASSERT_TRUE(add_video(pauline,marie, TRUE));
	liblinphone_tester_check_rtcp(pauline,marie);
#endif
	end_call(pauline, marie);
end:
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
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall *c1,*c2;
	LinphoneCallParams *params;
	LinphoneProxyConfig* pauline_proxy;
	params=linphone_core_create_call_params(pauline->lc, NULL);
	linphone_call_params_set_privacy(params,LinphonePrivacyId);

	BC_ASSERT_TRUE(call_with_caller_params(pauline,marie,params));
	linphone_call_params_destroy(params);

	c1=linphone_core_get_current_call(pauline->lc);
	c2=linphone_core_get_current_call(marie->lc);

	BC_ASSERT_PTR_NOT_NULL(c1);
	BC_ASSERT_PTR_NOT_NULL(c2);
	if (c1 && c2){
		/*make sure local identity is unchanged*/
		BC_ASSERT_TRUE(linphone_address_weak_equal(linphone_call_log_get_from(linphone_call_get_call_log(c1)),pauline->identity));

		/*make sure remote identity is hidden*/
		BC_ASSERT_FALSE(linphone_address_weak_equal(linphone_call_get_remote_address(c2),pauline->identity));

		BC_ASSERT_EQUAL(linphone_call_params_get_privacy(linphone_call_get_current_params(c2)),LinphonePrivacyId, int, "%d");
	}

	end_call(pauline, marie);

	/*test proxy config privacy*/
	pauline_proxy = linphone_core_get_default_proxy_config(pauline->lc);
	linphone_proxy_config_set_privacy(pauline_proxy,LinphonePrivacyId);

	BC_ASSERT_TRUE(call(pauline,marie));

	c1=linphone_core_get_current_call(pauline->lc);
	c2=linphone_core_get_current_call(marie->lc);

	BC_ASSERT_PTR_NOT_NULL(c1);
	BC_ASSERT_PTR_NOT_NULL(c2);
	if (c1 && c2){

		/*make sure remote identity is hidden*/
		BC_ASSERT_FALSE(linphone_address_weak_equal(linphone_call_get_remote_address(c2),pauline->identity));

		BC_ASSERT_EQUAL(linphone_call_params_get_privacy(linphone_call_get_current_params(c2)),LinphonePrivacyId, int, "%d");
	}

	end_call(pauline, marie);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

/*this ones makes call with privacy without previous registration*/
static void call_with_privacy2(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new2(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc", FALSE);
	LinphoneCall *c1,*c2;
	LinphoneCallParams *params;
	LinphoneProxyConfig* pauline_proxy;
	params=linphone_core_create_call_params(pauline->lc, NULL);
	linphone_call_params_set_privacy(params,LinphonePrivacyId);

	pauline_proxy = linphone_core_get_default_proxy_config(pauline->lc);
	linphone_proxy_config_edit(pauline_proxy);
	linphone_proxy_config_enable_register(pauline_proxy,FALSE);
	linphone_proxy_config_done(pauline_proxy);

	BC_ASSERT_TRUE(call_with_caller_params(pauline,marie,params));
	linphone_call_params_destroy(params);

	c1=linphone_core_get_current_call(pauline->lc);
	c2=linphone_core_get_current_call(marie->lc);

	BC_ASSERT_PTR_NOT_NULL(c1);
	BC_ASSERT_PTR_NOT_NULL(c2);

	if (c1 && c2){
		/*make sure local identity is unchanged*/
		BC_ASSERT_TRUE(linphone_address_weak_equal(linphone_call_log_get_from(linphone_call_get_call_log(c1)),pauline->identity));
		/*make sure remote identity is hidden*/
		BC_ASSERT_FALSE(linphone_address_weak_equal(linphone_call_get_remote_address(c2),pauline->identity));

		BC_ASSERT_EQUAL(linphone_call_params_get_privacy(linphone_call_get_current_params(c2)),LinphonePrivacyId, int, "%d");
	}

	end_call(pauline, marie);

	/*test proxy config privacy*/
	linphone_proxy_config_set_privacy(pauline_proxy,LinphonePrivacyId);

	BC_ASSERT_TRUE(call(pauline,marie));
	c1=linphone_core_get_current_call(pauline->lc);
	c2=linphone_core_get_current_call(marie->lc);

	BC_ASSERT_PTR_NOT_NULL(c1);
	BC_ASSERT_PTR_NOT_NULL(c2);

	if (c1 && c2){
		/*make sure remote identity is hidden*/
		BC_ASSERT_FALSE(linphone_address_weak_equal(linphone_call_get_remote_address(c2),pauline->identity));
		BC_ASSERT_EQUAL(linphone_call_params_get_privacy(linphone_call_get_current_params(c2)),LinphonePrivacyId, int, "%d");
	}
	end_call(marie, pauline);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void srtp_call(void) {
	call_base(LinphoneMediaEncryptionSRTP,FALSE,FALSE,LinphonePolicyNoFirewall,FALSE);
}

static void zrtp_call(void) {
	call_base(LinphoneMediaEncryptionZRTP,FALSE,FALSE,LinphonePolicyNoFirewall,FALSE);
}

static void zrtp_sas_call(void) {
	call_base_with_configfile(LinphoneMediaEncryptionZRTP,FALSE,FALSE,LinphonePolicyNoFirewall,FALSE, "marie_zrtp_b256_rc", "pauline_zrtp_b256_rc");
	call_base_with_configfile(LinphoneMediaEncryptionZRTP,FALSE,FALSE,LinphonePolicyNoFirewall,FALSE, "marie_zrtp_b256_rc", "pauline_tcp_rc");
}

static void zrtp_cipher_call(void) {
	call_base_with_configfile(LinphoneMediaEncryptionZRTP,FALSE,FALSE,LinphonePolicyNoFirewall,FALSE, "marie_zrtp_srtpsuite_aes256_rc", "pauline_zrtp_srtpsuite_aes256_rc");
	call_base_with_configfile(LinphoneMediaEncryptionZRTP,FALSE,FALSE,LinphonePolicyNoFirewall,FALSE, "marie_zrtp_aes256_rc", "pauline_zrtp_aes256_rc");
	call_base_with_configfile(LinphoneMediaEncryptionZRTP,FALSE,FALSE,LinphonePolicyNoFirewall,FALSE, "marie_zrtp_aes256_rc", "pauline_tcp_rc");
}

static void zrtp_video_call(void) {
	call_base(LinphoneMediaEncryptionZRTP,TRUE,FALSE,LinphonePolicyNoFirewall,FALSE);
}

static void dtls_srtp_call(void) {
	call_base(LinphoneMediaEncryptionDTLS,FALSE,FALSE,LinphonePolicyNoFirewall,FALSE);
}

static void dtls_srtp_call_with_media_realy(void) {
	call_base(LinphoneMediaEncryptionDTLS,FALSE,TRUE,LinphonePolicyNoFirewall,FALSE);
}

static void dtls_srtp_ice_call(void) {
	call_base(LinphoneMediaEncryptionDTLS,FALSE,FALSE,LinphonePolicyUseIce,FALSE);
}
#ifdef VIDEO_ENABLED
static void dtls_srtp_video_call(void) {
	call_base(LinphoneMediaEncryptionDTLS,TRUE,FALSE,LinphonePolicyNoFirewall,FALSE);
}

static void dtls_srtp_ice_video_call(void) {
	call_base(LinphoneMediaEncryptionDTLS,TRUE,FALSE,LinphonePolicyUseIce,FALSE);
}
static void dtls_srtp_ice_video_call_with_relay(void) {
	call_base(LinphoneMediaEncryptionDTLS,TRUE,TRUE,LinphonePolicyUseIce,FALSE);
}
#endif
static void call_with_declined_srtp(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	if (linphone_core_media_encryption_supported(marie->lc,LinphoneMediaEncryptionSRTP)) {
		linphone_core_set_media_encryption(pauline->lc,LinphoneMediaEncryptionSRTP);

		BC_ASSERT_TRUE(call(pauline,marie));

		end_call(marie, pauline);
	} else {
		ms_warning ("not tested because srtp not available");
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_srtp_paused_and_resumed(void) {
	/*
	 * This test was made to evidence a bug due to internal usage of current_params while not yet filled by linphone_call_get_current_params().
	 * As a result it must not use the call() function because it calls linphone_call_get_current_params().
	 */
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	const LinphoneCallParams *params;
	LinphoneCall *pauline_call;

	if (!linphone_core_media_encryption_supported(marie->lc,LinphoneMediaEncryptionSRTP)) goto end;
	linphone_core_set_media_encryption(pauline->lc,LinphoneMediaEncryptionSRTP);

	linphone_core_invite_address(pauline->lc, marie->identity);

	if (!BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallIncomingReceived,1))) goto end;
	pauline_call = linphone_core_get_current_call(pauline->lc);
	linphone_core_accept_call(marie->lc, linphone_core_get_current_call(marie->lc));

	if (!BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,1))) goto end;
	if (!BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,1))) goto end;

	linphone_core_pause_call(pauline->lc, pauline_call);

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPaused,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,1));

	linphone_core_resume_call(pauline->lc, pauline_call);
	if (!BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2))) goto end;
	if (!BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2))) goto end;

	/*assert that after pause and resume, SRTP is still being used*/
	params = linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc));
	BC_ASSERT_EQUAL(linphone_call_params_get_media_encryption(params) , LinphoneMediaEncryptionSRTP, int, "%d");
	params = linphone_call_get_current_params(linphone_core_get_current_call(marie->lc));
	BC_ASSERT_EQUAL(linphone_call_params_get_media_encryption(params) , LinphoneMediaEncryptionSRTP, int, "%d");

	end_call(pauline, marie);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void on_eof(LinphonePlayer *player, void *user_data){
	LinphoneCoreManager *marie=(LinphoneCoreManager*)user_data;
	marie->stat.number_of_player_eof++;
}

static void call_with_file_player(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphonePlayer *player;
	char *hellopath = bc_tester_res("sounds/ahbahouaismaisbon.wav");
	char *recordpath = create_filepath(bc_tester_get_writable_dir_prefix(), "record-call_with_file_player", "wav");
	bool_t call_ok;
	int attempts;
	double similar=1;
	const double threshold = 0.9;

	/*this test is actually attempted three times in case of failure, because the audio comparison at the end is very sensitive to
	 * jitter buffer drifts, which sometimes happen if the machine is unable to run the test in good realtime conditions */
	for (attempts=0; attempts<3; attempts++){
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
		/*make sure the record file doesn't already exists, otherwise this test will append new samples to it*/
		unlink(recordpath);
		/*caller uses files instead of soundcard in order to avoid mixing soundcard input with file played using call's player*/
		linphone_core_use_files(marie->lc,TRUE);
		linphone_core_set_play_file(marie->lc,NULL);

		/*callee is recording and plays file*/
		linphone_core_use_files(pauline->lc,TRUE);
		linphone_core_set_play_file(pauline->lc,NULL);
		linphone_core_set_record_file(pauline->lc,recordpath);

		BC_ASSERT_TRUE((call_ok=call(marie,pauline)));
		if (!call_ok) goto end;
		player=linphone_call_get_player(linphone_core_get_current_call(marie->lc));
		BC_ASSERT_PTR_NOT_NULL(player);
		if (player){
			BC_ASSERT_EQUAL(linphone_player_open(player,hellopath,on_eof,marie),0, int, "%d");
			BC_ASSERT_EQUAL(linphone_player_start(player),0, int, "%d");
		}
		/* This assert should be modified to be at least as long as the WAV file */
		BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&marie->stat.number_of_player_eof,1,10000));
		/*wait one second more for transmission to be fully ended (transmission time + jitter buffer)*/
		wait_for_until(pauline->lc,marie->lc,NULL,0,1000);

		end_call(marie, pauline);
		/*cannot run on iphone simulator because locks main loop beyond permitted time (should run
		on another thread) */
		BC_ASSERT_EQUAL(ms_audio_diff(hellopath,recordpath,&similar,&audio_cmp_params,NULL,NULL), 0, int, "%d");
		if (similar>=threshold)
			break;
	}
	BC_ASSERT_GREATER(similar, threshold, double, "%g");
	BC_ASSERT_LOWER(similar, 1.0, double, "%g");
	if (similar >= threshold && similar <= 1.0) {
		remove(recordpath);
	}

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	ms_free(recordpath);
	ms_free(hellopath);
}

static void call_with_mkv_file_player(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphonePlayer *player;
	char *hellomkv;
	char *hellowav;
	char *recordpath;
	bool_t call_ok;
#if !defined(__arm__) && !defined(__arm64__) && !TARGET_IPHONE_SIMULATOR && !defined(ANDROID)
	double similar=0.0;
	const double threshold = 0.9;
#define DO_AUDIO_CMP
#endif
	hellowav = bc_tester_res("sounds/hello8000_mkv_ref.wav");
	hellomkv = bc_tester_res("sounds/hello8000.mkv");

	if (!linphone_core_file_format_supported(marie->lc,"mkv")){
		ms_warning("Test skipped, no mkv support.");
		goto end;
	}
	recordpath = create_filepath(bc_tester_get_writable_dir_prefix(), "record-call_with_mkv_file_player", "wav");
	/*make sure the record file doesn't already exists, otherwise this test will append new samples to it*/
	unlink(recordpath);


	/*caller uses files instead of soundcard in order to avoid mixing soundcard input with file played using call's player*/
	linphone_core_use_files(marie->lc,TRUE);
	linphone_core_set_play_file(marie->lc,NULL);
	/*callee is recording and plays file*/
	linphone_core_use_files(pauline->lc,TRUE);
	linphone_core_set_play_file(pauline->lc,hellowav); /*just to send something but we are not testing what is sent by pauline*/
	linphone_core_set_record_file(pauline->lc,recordpath);

	BC_ASSERT_TRUE((call_ok=call(marie,pauline)));
	if (!call_ok) goto end;
	player=linphone_call_get_player(linphone_core_get_current_call(marie->lc));
	BC_ASSERT_PTR_NOT_NULL(player);
	if (player){
		int res = linphone_player_open(player,hellomkv,on_eof,marie);
		//if(!ms_filter_codec_supported("opus")) {
		if(!ms_factory_codec_supported(marie->lc->factory, "opus") && !ms_factory_codec_supported(pauline->lc->factory, "opus")){
			BC_ASSERT_EQUAL(res, -1, int, "%d");
			end_call(marie, pauline);
			goto end;
		}
		BC_ASSERT_EQUAL(res, 0, int, "%d");
		BC_ASSERT_EQUAL(linphone_player_start(player),0,int,"%d");
		BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&marie->stat.number_of_player_eof,1,12000));
		linphone_player_close(player);
		/*wait for one second more so that last RTP packets can arrive*/
		wait_for_until(pauline->lc,marie->lc,NULL,0,1000);
	}
	end_call(marie, pauline);
#ifdef DO_AUDIO_CMP
	BC_ASSERT_EQUAL(ms_audio_diff(hellowav,recordpath,&similar,&audio_cmp_params,NULL,NULL),0,int,"%d");
	BC_ASSERT_GREATER(similar,threshold,double,"%f");
	BC_ASSERT_LOWER(similar,1.0,double,"%f");
	if(similar>threshold && similar<=1.0) {
		remove(recordpath);
	}
#else
	/*inter-correlation process is too much CPU consuming ending in a 20 minutes test on arm...*/
	remove(recordpath);
#endif
	ms_free(recordpath);

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	ms_free(hellomkv);
	ms_free(hellowav);
}

void call_base_with_configfile(LinphoneMediaEncryption mode, bool_t enable_video,bool_t enable_relay,LinphoneFirewallPolicy policy,bool_t enable_tunnel, const char *marie_rc, const char *pauline_rc) {
	LinphoneCoreManager* marie = linphone_core_manager_new(marie_rc);
	LinphoneCoreManager* pauline = linphone_core_manager_new(pauline_rc);
	bool_t call_ok;

	linphone_core_set_video_device(pauline->lc,liblinphone_tester_mire_id);
	linphone_core_set_video_device(marie->lc,liblinphone_tester_mire_id);

	if (enable_relay) {
		linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
		linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);
	}
	if (enable_tunnel) {
		int i;
		LinphoneTunnelConfig * tunnel_config = linphone_tunnel_config_new();
		linphone_tunnel_config_set_host(tunnel_config, "tunnel.linphone.org");
		linphone_tunnel_config_set_port(tunnel_config, 443);
		linphone_tunnel_add_server(linphone_core_get_tunnel(marie->lc),tunnel_config);
		linphone_tunnel_enable_sip(linphone_core_get_tunnel(marie->lc),FALSE);
		linphone_tunnel_set_mode(linphone_core_get_tunnel(marie->lc),LinphoneTunnelModeEnable);
		for (i=0;i<100;i++) {
			if (linphone_tunnel_connected(linphone_core_get_tunnel(marie->lc))) {
				break;
			}
			linphone_core_iterate(marie->lc);
			ms_usleep(20000);
		}
		BC_ASSERT_TRUE(linphone_tunnel_connected(linphone_core_get_tunnel(marie->lc)));
		linphone_tunnel_config_unref(tunnel_config);
	}

	if (linphone_core_media_encryption_supported(marie->lc,mode)) {
		linphone_core_set_media_encryption(marie->lc,mode);
		linphone_core_set_media_encryption(pauline->lc,mode);
		if (mode==LinphoneMediaEncryptionDTLS) { /* for DTLS we must access certificates or at least have a directory to store them */
			char *path = bc_tester_file("certificates-marie");
			marie->lc->user_certificates_path = ms_strdup(path);
			bc_free(path);
			path = bc_tester_file("certificates-pauline");
			pauline->lc->user_certificates_path = ms_strdup(path);
			bc_free(path);
			belle_sip_mkdir(marie->lc->user_certificates_path);
			belle_sip_mkdir(pauline->lc->user_certificates_path);
		}

		linphone_core_set_firewall_policy(marie->lc,policy);
		linphone_core_set_firewall_policy(pauline->lc,policy);

		BC_ASSERT_TRUE((call_ok=call(pauline,marie)));
		if (!call_ok) goto end;
		if (linphone_core_get_media_encryption(pauline->lc) == LinphoneMediaEncryptionZRTP
			&& linphone_core_get_media_encryption(pauline->lc) == LinphoneMediaEncryptionZRTP) {
			/*wait for SAS*/
			int i;
			for (i=0;i<100;i++) {
				LinphoneCall *pauline_call = linphone_core_get_current_call(pauline->lc);
				LinphoneCall *marie_call = linphone_core_get_current_call(marie->lc);

				if (!pauline_call || !marie_call){
					/*if one of the two calls was disapeering, don't crash, but report it*/
					BC_ASSERT_PTR_NOT_NULL(pauline_call);
					BC_ASSERT_PTR_NOT_NULL(marie_call);
					break;
				}

				if (linphone_call_get_authentication_token(pauline_call)
					&&
					linphone_call_get_authentication_token(marie_call)) {
					/*check SAS*/
					BC_ASSERT_STRING_EQUAL(linphone_call_get_authentication_token(linphone_core_get_current_call(pauline->lc))
								,linphone_call_get_authentication_token(linphone_core_get_current_call(marie->lc)));
					liblinphone_tester_check_rtcp(pauline,marie);
					break;
				}
				linphone_core_iterate(marie->lc);
				linphone_core_iterate(pauline->lc);
				ms_usleep(20000);
			}
		}

		if (policy == LinphonePolicyUseIce){
			BC_ASSERT_TRUE(check_ice(pauline,marie,enable_tunnel?LinphoneIceStateReflexiveConnection:LinphoneIceStateHostConnection));
			wait_for_until(marie->lc, pauline->lc, NULL, 0, 2000);/*fixme to workaround a crash*/
		}
#ifdef VIDEO_ENABLED
		if (enable_video) {
			if (linphone_core_video_supported(marie->lc)) {
				BC_ASSERT_TRUE(add_video(pauline,marie, TRUE));
				if (policy == LinphonePolicyUseIce){
					BC_ASSERT_TRUE(check_ice(pauline, marie, enable_tunnel ? LinphoneIceStateReflexiveConnection
																		   : LinphoneIceStateHostConnection));
				}
				liblinphone_tester_check_rtcp(marie,pauline);

			} else {
				ms_warning ("not tested because video not available");
			}
		}
#endif
		end_call(marie, pauline);
	} else {
		ms_warning ("not tested because %s not available", linphone_media_encryption_to_string(mode));
	}
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void call_base(LinphoneMediaEncryption mode, bool_t enable_video,bool_t enable_relay,LinphoneFirewallPolicy policy,bool_t enable_tunnel) {
	call_base_with_configfile(mode, enable_video, enable_relay, policy, enable_tunnel, "marie_rc", "pauline_tcp_rc");
}

#ifdef VIDEO_ENABLED
static void srtp_video_ice_call(void) {
	call_base(LinphoneMediaEncryptionSRTP,TRUE,FALSE,LinphonePolicyUseIce,FALSE);
}
static void zrtp_video_ice_call(void) {
	call_base(LinphoneMediaEncryptionZRTP,TRUE,FALSE,LinphonePolicyUseIce,FALSE);
}
#endif

static void srtp_ice_call(void) {
	call_base(LinphoneMediaEncryptionSRTP,FALSE,FALSE,LinphonePolicyUseIce,FALSE);
}

static void zrtp_ice_call(void) {
	call_base(LinphoneMediaEncryptionZRTP,FALSE,FALSE,LinphonePolicyUseIce,FALSE);
}
static void zrtp_ice_call_with_relay(void) {
	call_base(LinphoneMediaEncryptionZRTP,FALSE,TRUE,LinphonePolicyUseIce,FALSE);
}

static void dtls_ice_call_with_relay(void) {
	call_base(LinphoneMediaEncryptionDTLS,FALSE,TRUE,LinphonePolicyUseIce,FALSE);
}

static void early_media_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_early_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok;

	BC_ASSERT_TRUE(call_ok=call(pauline,marie));

	if (!call_ok) goto end;
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallIncomingEarlyMedia,1, int, "%d");
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallOutgoingEarlyMedia,1, int, "%d");

	wait_for_until(pauline->lc,marie->lc,NULL,0,1000);

	/*added because a bug related to early-media caused the Connected state to be reached two times*/
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallConnected,1, int, "%d");

	end_call(pauline, marie);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void early_media_call_with_ice(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_early_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall *marie_call;
	MSList *lcs = NULL;
	
	lcs = ms_list_append(lcs, marie->lc);
	lcs = ms_list_append(lcs, pauline->lc);

	/*in this test, pauline has ICE activated, marie not, but marie proposes early media.
	 * We want to check that ICE processing is not disturbing early media*/
	linphone_core_set_firewall_policy(pauline->lc, LinphonePolicyUseIce);
	
	linphone_core_invite_address(pauline->lc, marie->identity);
	
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallIncomingReceived,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallIncomingEarlyMedia,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallOutgoingEarlyMedia,1,1000));
	
	wait_for_until(pauline->lc,marie->lc,NULL,0,1000);
	
	marie_call = linphone_core_get_current_call(marie->lc);

	if (!marie_call) goto end;
	
	linphone_core_accept_call(marie->lc, marie_call);
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallConnected,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallConnected,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallStreamsRunning,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallStreamsRunning,1,3000));

	end_call(marie, pauline);
end:
	ms_list_free(lcs);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void early_media_call_with_ringing(void){
	LinphoneCoreManager* marie   = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("pauline_tcp_rc");
	MSList* lcs = NULL;
	LinphoneCall* marie_call;
	LinphoneCallLog *marie_call_log;
	uint64_t connected_time=0;
	uint64_t ended_time=0;
	int dummy=0;

	lcs = ms_list_append(lcs,marie->lc);
	lcs = ms_list_append(lcs,pauline->lc);
	/*
		Marie calls Pauline, and after the call has rung, transitions to an early_media session
	*/

	marie_call = linphone_core_invite_address(marie->lc, pauline->identity);
	marie_call_log = linphone_call_get_call_log(marie_call);

	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingReceived,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingRinging,1,1000));


	if (linphone_core_inc_invite_pending(pauline->lc)) {
		/* send a 183 to initiate the early media */

		linphone_core_accept_early_media(pauline->lc, linphone_core_get_current_call(pauline->lc));

		BC_ASSERT_TRUE( wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingEarlyMedia,1,2000) );
		BC_ASSERT_TRUE( wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingEarlyMedia,1,2000) );

		liblinphone_tester_check_rtcp(marie, pauline);

		linphone_core_accept_call(pauline->lc, linphone_core_get_current_call(pauline->lc));

		BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallConnected, 1,1000));
		connected_time=ms_get_cur_time_ms();
		BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallStreamsRunning, 1,1000));

		BC_ASSERT_PTR_EQUAL(marie_call, linphone_core_get_current_call(marie->lc));

		liblinphone_tester_check_rtcp(marie, pauline);
		/*just to have a call duration !=0*/
		wait_for_list(lcs,&dummy,1,2000);

		end_call(pauline, marie);
		ended_time=ms_get_cur_time_ms();
		BC_ASSERT_LOWER( labs((long)((linphone_call_log_get_duration(marie_call_log)*1000) - (int64_t)(ended_time - connected_time))), 1000, long, "%ld");
		ms_list_free(lcs);
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void early_media_call_with_update_base(bool_t media_change){
	LinphoneCoreManager* marie   = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
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

	marie_call = linphone_core_invite_address(marie->lc, pauline->identity);

	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingReceived,1,5000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingRinging,1,5000));

	pauline_call = linphone_core_get_current_call(pauline->lc);
	if (!pauline_call) goto end;
	/* send a 183 to initiate the early media */
	linphone_core_accept_early_media(pauline->lc, pauline_call);
	BC_ASSERT_TRUE( wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingEarlyMedia,1,1000) );
	BC_ASSERT_TRUE( wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingEarlyMedia,1,5000) );


	pauline_params = linphone_call_params_copy(linphone_call_get_current_params(pauline_call));

	if (media_change) {
		disable_all_audio_codecs_except_one(marie->lc,"pcma",-1);
		disable_all_audio_codecs_except_one(pauline->lc,"pcma",-1);
	}
	#define UPDATED_SESSION_NAME "nouveau nom de session"

	linphone_call_params_set_session_name(pauline_params,UPDATED_SESSION_NAME);
	linphone_core_update_call(pauline->lc, pauline_call, pauline_params);
	linphone_call_params_destroy(pauline_params);
	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallEarlyUpdating,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallEarlyUpdatedByRemote,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingEarlyMedia,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingEarlyMedia,1,2000));

	/*just to wait 2s*/
	liblinphone_tester_check_rtcp(marie, pauline);

	BC_ASSERT_STRING_EQUAL(	  linphone_call_params_get_session_name(linphone_call_get_remote_params(marie_call))
							, UPDATED_SESSION_NAME);

	linphone_core_accept_call(pauline->lc, linphone_core_get_current_call(pauline->lc));

	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallConnected, 1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallStreamsRunning, 1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallConnected, 1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallStreamsRunning, 1,1000));

	liblinphone_tester_check_rtcp(marie, pauline);

	end_call(pauline, marie);

end:

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


static void check_call_state(LinphoneCoreManager* mgr,LinphoneCallState state) {
	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(mgr->lc));
	if (linphone_core_get_current_call(mgr->lc))
		BC_ASSERT_EQUAL(linphone_call_get_state(linphone_core_get_current_call(mgr->lc)),state, int, "%d");
}

static void call_established_with_rejected_info(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	int dummy=0;
	bool_t call_ok=FALSE;

	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));
	if (call_ok){

		sal_enable_unconditional_answer(marie->lc->sal,TRUE);
		linphone_call_send_info_message(linphone_core_get_current_call(pauline->lc),linphone_core_create_info_message(pauline->lc));

		wait_for_until(marie->lc,pauline->lc,&dummy,1,1000); /*just to sleep while iterating 1s*/

		sal_enable_unconditional_answer(marie->lc->sal,FALSE);

		linphone_call_send_info_message(linphone_core_get_current_call(pauline->lc),linphone_core_create_info_message(pauline->lc));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_inforeceived,1));
		BC_ASSERT_EQUAL(marie->stat.number_of_inforeceived,1, int, "%d");

		check_call_state(pauline,LinphoneCallStreamsRunning);
		check_call_state(marie,LinphoneCallStreamsRunning);
		end_call(pauline, marie);
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_established_with_complex_rejected_operation(void) {

	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok=FALSE;
	LinphoneCallParams *params;

	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));
	if (call_ok){

		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,1));
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,1));

		linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMU",8000,1),FALSE); /*disable PCMU*/
		linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMA",8000,1),TRUE); /*enable PCMA*/
		linphone_core_enable_payload_type(marie->lc,linphone_core_find_payload_type(marie->lc,"PCMU",8000,1),FALSE); /*disable PCMU*/
		linphone_core_enable_payload_type(marie->lc,linphone_core_find_payload_type(marie->lc,"PCMA",8000,1),TRUE); /*enable PCMA*/

		/*just to authenticate marie*/
		linphone_call_send_info_message(linphone_core_get_current_call(marie->lc),linphone_core_create_info_message(marie->lc));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_inforeceived,1));
		BC_ASSERT_EQUAL(pauline->stat.number_of_inforeceived,1, int, "%d");
		/*to give time for 200ok to arrive*/
		wait_for_until(marie->lc,pauline->lc,NULL,0,1000);



		linphone_core_update_call(	pauline->lc
									,linphone_core_get_current_call(pauline->lc)
									,linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc)));


		linphone_core_update_call(	marie->lc
											,linphone_core_get_current_call(marie->lc)
											,linphone_call_get_current_params(linphone_core_get_current_call(marie->lc)));

		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));

		BC_ASSERT_EQUAL(linphone_call_get_reason(linphone_core_get_current_call(pauline->lc)),LinphoneReasonTemporarilyUnavailable, int, "%d");
		BC_ASSERT_EQUAL(linphone_call_get_reason(linphone_core_get_current_call(pauline->lc)),LinphoneReasonTemporarilyUnavailable, int, "%d");

		check_call_state(pauline,LinphoneCallStreamsRunning);
		check_call_state(marie,LinphoneCallStreamsRunning);

		linphone_core_update_call(	pauline->lc
										,linphone_core_get_current_call(pauline->lc)
										,linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc)));


		linphone_call_send_info_message(linphone_core_get_current_call(marie->lc),linphone_core_create_info_message(marie->lc));


		params=linphone_core_create_call_params(marie->lc,linphone_core_get_current_call(marie->lc));
		sal_enable_pending_trans_checking(marie->lc->sal,FALSE); /*to allow // transactions*/
		linphone_core_enable_payload_type(marie->lc,linphone_core_find_payload_type(marie->lc,"PCMU",8000,1),TRUE);
		linphone_core_enable_payload_type(marie->lc,linphone_core_find_payload_type(marie->lc,"PCMA",8000,1),FALSE);

		linphone_core_update_call(	marie->lc
											,linphone_core_get_current_call(marie->lc)
											,params);

		linphone_call_params_destroy(params);

		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,3));
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,3));

		BC_ASSERT_EQUAL(linphone_call_get_reason(linphone_core_get_current_call(pauline->lc)),LinphoneReasonTemporarilyUnavailable, int, "%d");
		BC_ASSERT_EQUAL(linphone_call_get_reason(linphone_core_get_current_call(pauline->lc)),LinphoneReasonTemporarilyUnavailable, int, "%d");

		end_call(pauline, marie);
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_established_with_rejected_info_during_reinvite(void) {

	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok=FALSE;

	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));
	if (call_ok){

		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,1));
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,1));

		linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMU",8000,1),FALSE); /*disable PCMU*/
		linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMA",8000,1),TRUE); /*enable PCMA*/
		linphone_core_enable_payload_type(marie->lc,linphone_core_find_payload_type(marie->lc,"PCMU",8000,1),FALSE); /*disable PCMU*/
		linphone_core_enable_payload_type(marie->lc,linphone_core_find_payload_type(marie->lc,"PCMA",8000,1),TRUE); /*enable PCMA*/

		/*just to authenticate marie*/
		linphone_call_send_info_message(linphone_core_get_current_call(marie->lc),linphone_core_create_info_message(marie->lc));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_inforeceived,1));
		BC_ASSERT_EQUAL(pauline->stat.number_of_inforeceived,1, int, "%d");
		/*to give time for 200ok to arrive*/
		wait_for_until(marie->lc,pauline->lc,NULL,0,1000);


		//sal_enable_pending_trans_checking(marie->lc->sal,FALSE); /*to allow // transactions*/

		linphone_call_send_info_message(linphone_core_get_current_call(marie->lc),linphone_core_create_info_message(marie->lc));

		//sal_set_send_error(marie->lc->sal, -1); /*to avoid 491 pending to be sent*/

		linphone_core_update_call(	pauline->lc
									,linphone_core_get_current_call(pauline->lc)
									,linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc)));



		wait_for_until(pauline->lc,pauline->lc,NULL,0,2000); /*to avoid 491 pending to be sent to early*/


		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
		end_call(pauline, marie);
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


static void call_established_with_rejected_reinvite(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok=FALSE;

	BC_ASSERT_TRUE(call_ok=call(pauline,marie));
	if (call_ok){
		linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMU",8000,1),FALSE); /*disable PCMU*/
		linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMA",8000,1),TRUE); /*enable PCMA*/


		linphone_core_update_call(	pauline->lc
									,linphone_core_get_current_call(pauline->lc)
									,linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc)));


		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));

		BC_ASSERT_EQUAL(linphone_call_get_reason(linphone_core_get_current_call(pauline->lc)),LinphoneReasonNotAcceptable, int, "%d");

		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallStreamsRunning,1, int, "%d");
		check_call_state(pauline,LinphoneCallStreamsRunning);
		check_call_state(marie,LinphoneCallStreamsRunning);
		end_call(pauline, marie);
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_established_with_rejected_incoming_reinvite(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok=FALSE;

	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));

	if (call_ok){

		/*wait for ACK to be transmitted before going to reINVITE*/
		wait_for_until(marie->lc,pauline->lc,NULL,0,1000);

		linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMU",8000,1),FALSE); /*disable PCMU*/
		linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMA",8000,1),TRUE); /*enable PCMA*/

		linphone_core_update_call(marie->lc
					,linphone_core_get_current_call(marie->lc)
					,linphone_call_get_current_params(linphone_core_get_current_call(marie->lc)));


		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallUpdating,1));
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));

		BC_ASSERT_EQUAL(linphone_call_get_reason(linphone_core_get_current_call(marie->lc)),LinphoneReasonNotAcceptable, int, "%d");

		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallStreamsRunning,1, int, "%d");
		check_call_state(pauline,LinphoneCallStreamsRunning);
		check_call_state(marie,LinphoneCallStreamsRunning);
		end_call(pauline, marie);
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_redirect(void){
	LinphoneCoreManager* marie   = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCoreManager* laure   = linphone_core_manager_new("laure_rc");
	MSList* lcs = NULL;
	char *laure_url = NULL;
	LinphoneCall* marie_call;

	lcs = ms_list_append(lcs,marie->lc);
	lcs = ms_list_append(lcs,pauline->lc);
	lcs = ms_list_append(lcs,laure->lc);
	/*
		Marie calls Pauline, which will redirect the call to Laure via a 302
	*/

	marie_call = linphone_core_invite_address(marie->lc, pauline->identity);

	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingReceived,1,6000));

	if (linphone_core_get_current_call(pauline->lc)){
		laure_url = linphone_address_as_string(laure->identity);
		linphone_core_redirect_call(pauline->lc, linphone_core_get_current_call(pauline->lc), laure_url);
		ms_free(laure_url);

		/* laure should be ringing now */
		BC_ASSERT_TRUE(wait_for_list(lcs, &laure->stat.number_of_LinphoneCallIncomingReceived,1,6000));
		/* pauline should have ended the call */
		BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallEnd,1,1000));
		/* the call should still be ringing on marie's side */
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallOutgoingRinging, 1, int, "%i");

		linphone_core_accept_call(laure->lc, linphone_core_get_current_call(laure->lc));

		BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallStreamsRunning, 1,5000));
		BC_ASSERT_TRUE(wait_for_list(lcs, &laure->stat.number_of_LinphoneCallStreamsRunning, 1,5000));

		BC_ASSERT_PTR_EQUAL(marie_call, linphone_core_get_current_call(marie->lc));

		liblinphone_tester_check_rtcp(marie, laure);

		end_call(laure, marie);
	}

	ms_list_free(lcs);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);

}

static void call_established_with_rejected_reinvite_with_error_base(bool_t trans_pending) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok=TRUE;
	int result;

	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));

	if (call_ok){
		linphone_core_enable_payload_type(pauline->lc,linphone_core_find_payload_type(pauline->lc,"PCMA",8000,1),TRUE); /*add PCMA*/

		if (trans_pending) {
			LinphoneInfoMessage * info = linphone_core_create_info_message(pauline->lc);
			linphone_call_send_info_message(linphone_core_get_current_call(pauline->lc),info);

		} else
			sal_enable_unconditional_answer(marie->lc->sal,TRUE);

		result = linphone_core_update_call(	pauline->lc
						,linphone_core_get_current_call(pauline->lc)
						,linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc)));

		if (trans_pending)
			BC_ASSERT_NOT_EQUAL(result,0, int, "%d");
		else
			BC_ASSERT_EQUAL(result,0,int, "%d");

		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));

		BC_ASSERT_EQUAL(linphone_call_get_reason(linphone_core_get_current_call(pauline->lc)),LinphoneReasonTemporarilyUnavailable, int, "%d"); /*might be change later*/

		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallStreamsRunning,1, int, "%d");
		check_call_state(pauline,LinphoneCallStreamsRunning);
		check_call_state(marie,LinphoneCallStreamsRunning);

		if (!trans_pending)
			sal_enable_unconditional_answer(marie->lc->sal,FALSE);

		end_call(pauline, marie);
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_established_with_rejected_reinvite_with_error(void) {
	call_established_with_rejected_reinvite_with_error_base(FALSE);
}

static void call_established_with_rejected_reinvite_with_trans_pending_error(void) {
	call_established_with_rejected_reinvite_with_error_base(TRUE);
}

static void call_rejected_because_wrong_credentials_with_params(const char* user_agent,bool_t enable_auth_req_cb) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneAuthInfo* good_auth_info=linphone_auth_info_clone(linphone_core_find_auth_info(marie->lc,NULL,linphone_address_get_username(marie->identity),NULL));
	LinphoneAuthInfo* wrong_auth_info=linphone_auth_info_clone(good_auth_info);
	bool_t result=FALSE;
	linphone_auth_info_set_passwd(wrong_auth_info,"passecretdutout");
	linphone_auth_info_set_ha1(wrong_auth_info, NULL);
	linphone_core_clear_all_auth_info(marie->lc);

	if (user_agent) {
		linphone_core_set_user_agent(marie->lc,user_agent,NULL);
	}
	if (!enable_auth_req_cb) {
		((VTableReference*)(marie->lc->vtable_refs->data))->vtable->auth_info_requested=NULL;
		linphone_core_add_auth_info(marie->lc,wrong_auth_info);
	}

	BC_ASSERT_PTR_NOT_NULL(linphone_core_invite_address(marie->lc,marie->identity));

	result=wait_for(marie->lc,marie->lc,&marie->stat.number_of_auth_info_requested,1);

	if (enable_auth_req_cb) {
		BC_ASSERT_TRUE(result);
		/*automatically re-inititae the call*/
		linphone_core_add_auth_info(marie->lc,wrong_auth_info);
	}

	BC_ASSERT_TRUE(wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphoneCallError,1));
	if (enable_auth_req_cb) {
		BC_ASSERT_EQUAL(marie->stat.number_of_auth_info_requested,2, int, "%d");
	}
	/*to make sure unregister will work*/
	linphone_core_clear_all_auth_info(marie->lc);
	linphone_core_add_auth_info(marie->lc,good_auth_info);
	linphone_auth_info_destroy(good_auth_info);
	linphone_core_manager_destroy(marie);
}

static void call_rejected_because_wrong_credentials(void) {
	call_rejected_because_wrong_credentials_with_params(NULL,TRUE);
}

static void call_rejected_without_403_because_wrong_credentials(void) {
	call_rejected_because_wrong_credentials_with_params("tester-no-403",TRUE);
}

static void call_rejected_without_403_because_wrong_credentials_no_auth_req_cb(void) {
	call_rejected_because_wrong_credentials_with_params("tester-no-403",FALSE);
}

#ifdef VIDEO_ENABLED
static void video_early_media_call(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_early_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_rc");
	LinphoneCall *pauline_to_marie;
	
	linphone_core_set_video_device(pauline->lc, "Mire: Mire (synthetic moving picture)");
	
	video_call_base_3(pauline, marie, TRUE, LinphoneMediaEncryptionNone, TRUE, TRUE);
	
	BC_ASSERT_PTR_NOT_NULL(pauline_to_marie = linphone_core_get_current_call(pauline->lc));
	if(pauline_to_marie) {
		BC_ASSERT_EQUAL(pauline_to_marie->videostream->source->desc->id, MS_MIRE_ID, int, "%d");
	}
	
	end_call(pauline, marie);
	
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

/*this is call forking with early media managed at client side (not by flexisip server)*/
static void multiple_early_media(void) {
	LinphoneCoreManager* pauline = linphone_core_manager_new("pauline_tcp_rc");
	LinphoneCoreManager* marie1 = linphone_core_manager_new("marie_early_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new("marie_early_rc");
	MSList *lcs=NULL;
	LinphoneCallParams *params=linphone_core_create_call_params(pauline->lc, NULL);
	LinphoneVideoPolicy pol;
	LinphoneCall *marie1_call;
	LinphoneCall *marie2_call;
	LinphoneCall *pauline_call;
	LinphoneInfoMessage *info;
	int dummy=0;
	pol.automatically_accept=1;
	pol.automatically_initiate=1;

	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, TRUE);

	linphone_core_enable_video_capture(marie1->lc, TRUE);
	linphone_core_enable_video_display(marie1->lc, TRUE);
	linphone_core_set_video_policy(marie1->lc,&pol);

	linphone_core_enable_video_capture(marie2->lc, TRUE);
	linphone_core_enable_video_display(marie2->lc, TRUE);
	linphone_core_set_video_policy(marie2->lc,&pol);
	linphone_core_set_audio_port_range(marie2->lc,40200,40300);
	linphone_core_set_video_port_range(marie2->lc,40400,40500);

	lcs=ms_list_append(lcs,marie1->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	lcs=ms_list_append(lcs,pauline->lc);

	linphone_call_params_enable_early_media_sending(params,TRUE);
	linphone_call_params_enable_video(params,TRUE);

	linphone_core_invite_address_with_params(pauline->lc,marie1->identity,params);
	linphone_call_params_destroy(params);

	BC_ASSERT_TRUE(wait_for_list(lcs, &marie1->stat.number_of_LinphoneCallIncomingEarlyMedia,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie2->stat.number_of_LinphoneCallIncomingEarlyMedia,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallOutgoingEarlyMedia,1,3000));

	pauline_call=linphone_core_get_current_call(pauline->lc);
	marie1_call=linphone_core_get_current_call(marie1->lc);
	marie2_call=linphone_core_get_current_call(marie2->lc);

	BC_ASSERT_PTR_NOT_NULL(pauline_call);
	BC_ASSERT_PTR_NOT_NULL(marie1_call);
	BC_ASSERT_PTR_NOT_NULL(marie2_call);

	if (pauline_call && marie1_call && marie2_call){

		/*wait a bit that streams are established*/
		wait_for_list(lcs,&dummy,1,6000);
		BC_ASSERT_GREATER(linphone_core_manager_get_max_audio_down_bw(pauline),70,int,"%i");
		BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_down_bw(marie1), 70, int, "%i");
		BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_down_bw(marie2), 70, int, "%i");

		linphone_core_accept_call(marie1->lc,linphone_core_get_current_call(marie1->lc));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie1->stat.number_of_LinphoneCallStreamsRunning,1,3000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,3000));

		/*marie2 should get her call terminated*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));

		/*wait a bit that streams are established*/
		wait_for_list(lcs,&dummy,1,3000);
		BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_down_bw(pauline), 71, int, "%i");
		BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_down_bw(marie1), 71, int, "%i");

		/*send an INFO in reverse side to check that dialogs are properly established*/
		info=linphone_core_create_info_message(marie1->lc);
		linphone_call_send_info_message(marie1_call,info);
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_inforeceived,1,3000));
	}

	end_call(pauline, marie1);

	ms_list_free(lcs);
	linphone_core_manager_destroy(marie1);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(pauline);
}
#endif

void check_media_direction(LinphoneCoreManager* mgr, LinphoneCall *call, MSList* lcs,LinphoneMediaDirection audio_dir, LinphoneMediaDirection video_dir) {
	BC_ASSERT_PTR_NOT_NULL(call);
	if  (call) {
		const LinphoneCallParams *params;
		wait_for_list(lcs,NULL,0,5000); /*on some device, it may take 3 to 4s to get audio from mic*/
		params = linphone_call_get_current_params(call);
#ifdef VIDEO_ENABLED
		if (video_dir != LinphoneMediaDirectionInvalid){
			int current_recv_iframe = mgr->stat.number_of_IframeDecoded;
			int expected_recv_iframe=0;

			if (video_dir != LinphoneMediaDirectionInactive){
				BC_ASSERT_TRUE(linphone_call_params_video_enabled(params));
			}
			BC_ASSERT_EQUAL(linphone_call_params_get_video_direction(params), video_dir, int, "%d");
			linphone_call_set_next_video_frame_decoded_callback(call,linphone_call_iframe_decoded_cb,mgr->lc);
			linphone_call_send_vfu_request(call);

			switch (video_dir) {
			case LinphoneMediaDirectionInactive:
				BC_ASSERT_LOWER((int)linphone_call_get_video_stats(call)->upload_bandwidth, 5, int, "%i");
				break;
			case LinphoneMediaDirectionSendOnly:
				expected_recv_iframe = 0;
				BC_ASSERT_LOWER((int)linphone_call_get_video_stats(call)->download_bandwidth, 5, int, "%i");
				break;
			case LinphoneMediaDirectionRecvOnly:
				BC_ASSERT_LOWER((int)linphone_call_get_video_stats(call)->upload_bandwidth, 5, int, "%i");
			case LinphoneMediaDirectionSendRecv:
				expected_recv_iframe = 1;
				break;
			default:
				break;
			}

			BC_ASSERT_TRUE(wait_for_list(lcs, &mgr->stat.number_of_IframeDecoded,current_recv_iframe + expected_recv_iframe,3000));
		}
#endif
		if (audio_dir != LinphoneMediaDirectionInvalid){
			BC_ASSERT_EQUAL(linphone_call_params_get_audio_direction(params), audio_dir, int, "%d");
			switch (audio_dir) {
				case LinphoneMediaDirectionInactive:
					BC_ASSERT_LOWER(linphone_core_manager_get_mean_audio_up_bw(mgr), 5, int, "%i");
					BC_ASSERT_LOWER(linphone_core_manager_get_mean_audio_down_bw(mgr), 5, int, "%i");
					break;
				case LinphoneMediaDirectionSendOnly:
					BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_up_bw(mgr), 70, int, "%i");
					break;
				case LinphoneMediaDirectionRecvOnly:
					BC_ASSERT_LOWER(linphone_core_manager_get_mean_audio_up_bw(mgr), 5, int, "%i");
					break;
				case LinphoneMediaDirectionSendRecv:
					BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_down_bw(mgr), 70, int, "%i");
					BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_up_bw(mgr), 70, int, "%i");
					break;
				default:
					break;
			}
		}
	}

}
#ifdef VIDEO_ENABLED
static void accept_call_in_send_only_base(LinphoneCoreManager* pauline, LinphoneCoreManager *marie, MSList *lcs) {
#define DEFAULT_WAIT_FOR 10000
	LinphoneCallParams *params;
	LinphoneVideoPolicy pol;
	LinphoneCall *call;
	pol.automatically_accept=1;
	pol.automatically_initiate=1;

	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, TRUE);
	linphone_core_set_video_policy(pauline->lc,&pol);
	linphone_core_set_video_device(pauline->lc,liblinphone_tester_mire_id);

	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, TRUE);
	linphone_core_set_video_policy(marie->lc,&pol);
	linphone_core_set_video_device(marie->lc,liblinphone_tester_mire_id);

	linphone_call_set_next_video_frame_decoded_callback(linphone_core_invite_address(pauline->lc,marie->identity)
														,linphone_call_iframe_decoded_cb
														,pauline->lc);


	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallIncomingReceived,1,DEFAULT_WAIT_FOR));

	{
		char* remote_uri = linphone_address_as_string_uri_only(pauline->identity);
		call = linphone_core_find_call_from_uri(marie->lc,remote_uri);
		ms_free(remote_uri);
	}

	if  (call) {
		params=linphone_core_create_call_params(marie->lc, NULL);
		linphone_call_params_set_audio_direction(params,LinphoneMediaDirectionSendOnly);
		linphone_call_params_set_video_direction(params,LinphoneMediaDirectionSendOnly);
		linphone_core_accept_call_with_params(marie->lc,call,params);
		linphone_call_params_destroy(params);

		BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallStreamsRunning,1,DEFAULT_WAIT_FOR));
		BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallPausedByRemote,1,DEFAULT_WAIT_FOR));

		check_media_direction(marie,call,lcs,LinphoneMediaDirectionSendOnly,LinphoneMediaDirectionSendOnly);
	}


	call=linphone_core_get_current_call(pauline->lc);
	if  (call) {
		check_media_direction(pauline,call,lcs,LinphoneMediaDirectionRecvOnly,LinphoneMediaDirectionRecvOnly);
	}

}
static void accept_call_in_send_base(bool_t caller_has_ice) {
	LinphoneCoreManager *pauline, *marie;
	MSList *lcs=NULL;;

	marie = linphone_core_manager_new("marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	if (caller_has_ice) {
		linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);
	}

	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,marie->lc);

	accept_call_in_send_only_base(pauline,marie,lcs);


	end_call(marie,pauline);
	ms_list_free(lcs);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void accept_call_in_send_only(void)  {
	accept_call_in_send_base(FALSE);
}

static void accept_call_in_send_only_with_ice(void)  {
	accept_call_in_send_base(TRUE);
}

void two_accepted_call_in_send_only(void) {
	LinphoneCoreManager *pauline, *marie, *laure;
	MSList *lcs=NULL;

	marie = linphone_core_manager_new("marie_rc");
	linphone_core_use_files(marie->lc, TRUE);
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	laure = linphone_core_manager_new("laure_rc");

	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,marie->lc);
	lcs=ms_list_append(lcs,laure->lc);

	accept_call_in_send_only_base(pauline,marie,lcs);

	reset_counters(&marie->stat);
	accept_call_in_send_only_base(laure,marie,lcs);

	end_call(pauline, marie);
	end_call(laure, marie);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	ms_list_free(lcs);
}
#endif

static char *create_filepath(const char *dir, const char *filename, const char *ext) {
	return ms_strdup_printf("%s/%s.%s",dir,filename,ext);
}

static void record_call(const char *filename, bool_t enableVideo, const char *video_codec) {
	LinphoneCoreManager *marie = NULL;
	LinphoneCoreManager *pauline = NULL;
	LinphoneCallParams *marieParams = NULL;
	LinphoneCallParams *paulineParams = NULL;
	LinphoneCall *callInst = NULL;
	const char **formats, *format;
	char *filepath;
	int dummy=0, i;
	bool_t call_succeeded = FALSE;

	marie = linphone_core_manager_new("marie_h264_rc");
	pauline = linphone_core_manager_new("pauline_h264_rc");

#if defined(HAVE_OPENH264) && defined(ANDROID)
	libmsopenh264_init(linphone_core_get_ms_factory(marie->lc));
	linphone_core_reload_ms_plugins(marie->lc, NULL);
	libmsopenh264_init(linphone_core_get_ms_factory(pauline->lc));
	linphone_core_reload_ms_plugins(pauline->lc, NULL);
#endif
	marieParams = linphone_core_create_call_params(marie->lc, NULL);
	paulineParams = linphone_core_create_call_params(pauline->lc, NULL);

#ifdef VIDEO_ENABLED
	linphone_core_set_video_device(pauline->lc, liblinphone_tester_mire_id);
	if(enableVideo) {
		if(linphone_core_find_payload_type(marie->lc, video_codec, -1, -1)
				&& linphone_core_find_payload_type(pauline->lc, video_codec, -1, -1)) {
			linphone_call_params_enable_video(marieParams, TRUE);
			linphone_call_params_enable_video(paulineParams, TRUE);
			disable_all_video_codecs_except_one(marie->lc, video_codec);
			disable_all_video_codecs_except_one(pauline->lc, video_codec);
		} else {
			ms_warning("call_recording(): the H264 payload has not been found. Only sound will be recorded");
		}
	}
#endif

	formats = linphone_core_get_supported_file_formats(marie->lc);

	for(i=0, format = formats[0]; format != NULL; i++, format = formats[i]) {
		filepath = create_filepath(bc_tester_get_writable_dir_prefix(), filename, format);
		remove(filepath);
		linphone_call_params_set_record_file(marieParams, filepath);
		BC_ASSERT_TRUE(call_succeeded = call_with_params(marie, pauline, marieParams, paulineParams));
		BC_ASSERT_PTR_NOT_NULL(callInst = linphone_core_get_current_call(marie->lc));
		if ((call_succeeded == TRUE) && (callInst != NULL)) {
			ms_message("call_recording(): start recording into %s", filepath);
			linphone_call_start_recording(callInst);
			wait_for_until(marie->lc,pauline->lc,&dummy,1,5000);
			linphone_call_stop_recording(callInst);
			end_call(marie, pauline);
			BC_ASSERT_EQUAL(ortp_file_exist(filepath), 0, int, "%d");
		}
		remove(filepath);
		ms_free(filepath);
	}
	linphone_call_params_destroy(paulineParams);
	linphone_call_params_destroy(marieParams);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void audio_call_recording_test(void) {
	record_call("recording", FALSE, NULL);
}

#ifdef VIDEO_ENABLED
static void video_call_recording_h264_test(void) {
	record_call("recording", TRUE, "H264");
}

static void video_call_recording_vp8_test(void) {
	record_call("recording", TRUE, "VP8");
}

static void video_call_snapshot(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCallParams *marieParams = linphone_core_create_call_params(marie->lc, NULL);
	LinphoneCallParams *paulineParams = linphone_core_create_call_params(pauline->lc, NULL);
	LinphoneCall *callInst = NULL;
	char *filename = create_filepath(bc_tester_get_writable_dir_prefix(), "snapshot", "jpeg");
	int dummy = 0;
	bool_t call_succeeded = FALSE;

	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, TRUE);
	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, FALSE);
	linphone_call_params_enable_video(marieParams, TRUE);
	linphone_call_params_enable_video(paulineParams, TRUE);

	BC_ASSERT_TRUE(call_succeeded = call_with_params(marie, pauline, marieParams, paulineParams));
	BC_ASSERT_PTR_NOT_NULL(callInst = linphone_core_get_current_call(marie->lc));
	if((call_succeeded == TRUE) && (callInst != NULL)) {
		int jpeg_support = linphone_call_take_video_snapshot(callInst, filename);
		if (jpeg_support < 0) {
			ms_warning("No jpegwriter support!");
		} else {
			wait_for_until(marie->lc, pauline->lc, &dummy, 1, 5000);
			BC_ASSERT_EQUAL(ortp_file_exist(filename), 0, int, "%d");
			remove(filename);
		}
		end_call(marie, pauline);
	}
	ms_free(filename);
	linphone_call_params_unref(marieParams);
	linphone_call_params_unref(paulineParams);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#endif

static void call_with_in_dialog_update(void) {
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCallParams *params;
	bool_t call_ok;

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	BC_ASSERT_TRUE(call_ok=call(pauline,marie));
	if (!call_ok) goto end;

	liblinphone_tester_check_rtcp(marie,pauline);
	params=linphone_core_create_call_params(marie->lc,linphone_core_get_current_call(marie->lc));
	params->no_user_consent=TRUE;
	linphone_core_update_call(marie->lc,linphone_core_get_current_call(marie->lc),params);
	linphone_call_params_destroy(params);
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallUpdating,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallUpdatedByRemote,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	end_call(marie,pauline);

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void call_with_in_dialog_codec_change_base(bool_t no_sdp) {
	int dummy=0;
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCallParams *params;
	bool_t call_ok;

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	BC_ASSERT_TRUE(call_ok=call(pauline,marie));
	if (!call_ok) goto end;

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
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallUpdating,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallUpdatedByRemote,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	BC_ASSERT_STRING_EQUAL("PCMA",linphone_payload_type_get_mime_type(linphone_call_params_get_used_audio_codec(linphone_call_get_current_params(linphone_core_get_current_call(marie->lc)))));
	wait_for_until(marie->lc, pauline->lc, &dummy, 1, 5000);
	BC_ASSERT_GREATER(linphone_core_manager_get_max_audio_down_bw(marie),70,int,"%i");
	BC_ASSERT_GREATER(linphone_core_manager_get_max_audio_down_bw(pauline),70,int,"%i");

	end_call(marie,pauline);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void call_with_in_dialog_codec_change(void) {
	call_with_in_dialog_codec_change_base(FALSE);
}
static void call_with_in_dialog_codec_change_no_sdp(void) {
	call_with_in_dialog_codec_change_base(TRUE);
}

static void call_with_custom_supported_tags(void) {
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	const LinphoneCallParams *remote_params;
	const char *recv_supported;
	LinphoneCall *pauline_call;

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	linphone_core_add_supported_tag(marie->lc,"pouet-tag");
	linphone_core_add_supported_tag(marie->lc,"truc-tag");
	linphone_core_add_supported_tag(marie->lc,"machin-tag");
	
	linphone_core_invite_address(marie->lc, pauline->identity);
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallIncomingReceived,1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallOutgoingRinging,1));
	pauline_call = linphone_core_get_current_call(pauline->lc);
	if (!pauline_call) goto end;
	
	remote_params=linphone_call_get_remote_params(pauline_call);
	recv_supported=linphone_call_params_get_custom_header(remote_params,"supported");
	BC_ASSERT_PTR_NOT_NULL(recv_supported);
	if (recv_supported){
		BC_ASSERT_PTR_NOT_NULL(strstr(recv_supported,"pouet-tag, truc-tag, machin-tag"));
	}
	
	end_call(marie,pauline);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_log_from_taken_from_p_asserted_id(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall *c1,*c2;
	LinphoneCallParams *params;
	const char* pauline_asserted_id ="\"Paupauche\" <sip:pauline@super.net>";
	LinphoneAddress *pauline_asserted_id_addr = linphone_address_new(pauline_asserted_id);
	LpConfig *marie_lp;
	bool_t call_ok;

	params=linphone_core_create_call_params(pauline->lc, NULL);

	linphone_call_params_add_custom_header(params,"P-Asserted-Identity",pauline_asserted_id);
	/*fixme, should be able to add several time the same header linphone_call_params_add_custom_header(params,"P-Asserted-Identity","\"Paupauche\" <tel:+12345>");*/

	marie_lp = linphone_core_get_config(marie->lc);
	lp_config_set_int(marie_lp,"sip","call_logs_use_asserted_id_instead_of_from",1);


	BC_ASSERT_TRUE(call_ok=call_with_caller_params(pauline,marie,params));

	if (!call_ok) goto end;

	c1=linphone_core_get_current_call(pauline->lc);
	c2=linphone_core_get_current_call(marie->lc);

	BC_ASSERT_PTR_NOT_NULL(c1);
	BC_ASSERT_PTR_NOT_NULL(c2);

	/*make sure remote identity is hidden*/
	BC_ASSERT_TRUE(linphone_address_weak_equal(linphone_call_get_remote_address(c2),pauline_asserted_id_addr));
	linphone_address_destroy(pauline_asserted_id_addr);
	end_call(pauline, marie);
end:
	linphone_call_params_destroy(params);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void incoming_invite_with_invalid_sdp(void) {
	LinphoneCoreManager* caller = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* callee = linphone_core_manager_new( "marie_rc");
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params = {0};

	callee_test_params.sdp_simulate_error = TRUE;
	BC_ASSERT_FALSE(call_with_params2(caller,callee,&caller_test_params, &callee_test_params, FALSE));

	BC_ASSERT_PTR_NULL(linphone_core_get_current_call(callee->lc));
	BC_ASSERT_EQUAL(caller->stat.number_of_LinphoneCallError,1, int, "%d");
	/*call will be drop before presented to the application, because it is invalid*/
	BC_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallIncomingReceived,0, int, "%d");

	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);
}

static void outgoing_invite_with_invalid_sdp(void) {
	LinphoneCoreManager* caller = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* callee = linphone_core_manager_new( "marie_rc");
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params = {0};

	caller_test_params.sdp_simulate_error = TRUE;
	BC_ASSERT_FALSE(call_with_params2(caller,callee,&caller_test_params, &callee_test_params, FALSE));

	BC_ASSERT_PTR_NULL(linphone_core_get_current_call(callee->lc));
	BC_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallIncomingReceived,1, int, "%d");
	BC_ASSERT_EQUAL(caller->stat.number_of_LinphoneCallError,1, int, "%d");
	// actually callee does not receive error, because it just get a BYE from the other part
	BC_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallError,0, int, "%d");
	BC_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallEnd,1, int, "%d");

	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);
}

static void incoming_reinvite_with_invalid_ack_sdp(void){
#ifdef VIDEO_ENABLED
	LinphoneCoreManager* caller = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* callee = linphone_core_manager_new( "marie_rc");
	LinphoneCall * inc_call;
	BC_ASSERT_TRUE(call(caller,callee));
	inc_call = linphone_core_get_current_call(callee->lc);

	BC_ASSERT_PTR_NOT_NULL(inc_call);
	if (inc_call) {
		const LinphoneCallParams *caller_params;
		stats initial_caller_stat=caller->stat;
		stats initial_callee_stat=callee->stat;
		sal_call_set_sdp_handling(inc_call->op, SalOpSDPSimulateError); /* will force a parse error for the ACK SDP*/
		BC_ASSERT_PTR_NOT_NULL(setup_video(caller, callee, TRUE));
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallUpdating,initial_callee_stat.number_of_LinphoneCallUpdating+1));
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallStreamsRunning,initial_callee_stat.number_of_LinphoneCallStreamsRunning+1));
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&caller->stat.number_of_LinphoneCallStreamsRunning,initial_caller_stat.number_of_LinphoneCallStreamsRunning));
		/*Basically the negotiation failed but since the call was already running, we expect it to restore to
		the previous state so error stats should not be changed*/
		BC_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallError,initial_callee_stat.number_of_LinphoneCallError, int, "%d");
		/*and remote should have received an update notification*/
		BC_ASSERT_EQUAL(caller->stat.number_of_LinphoneCallUpdatedByRemote,initial_caller_stat.number_of_LinphoneCallUpdatedByRemote+1, int, "%d");


		BC_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(callee->lc))));
		caller_params = linphone_call_get_current_params(linphone_core_get_current_call(caller->lc));
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,(int*)&caller_params->has_video,FALSE));

		sal_call_set_sdp_handling(inc_call->op, SalOpSDPNormal);
	}
	end_call(caller, callee);

	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);
#else
	ms_warning("not tested because video not available");
#endif
}

static void outgoing_reinvite_with_invalid_ack_sdp(void)  {
#ifdef VIDEO_ENABLED
	LinphoneCoreManager* caller = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* callee = linphone_core_manager_new( "marie_rc");
	LinphoneCall * out_call;
	BC_ASSERT_TRUE(call(caller,callee));
	out_call = linphone_core_get_current_call(caller->lc);

	BC_ASSERT_PTR_NOT_NULL(out_call);
	if (out_call) {
		stats initial_caller_stat=caller->stat;
		stats initial_callee_stat=callee->stat;
		sal_call_set_sdp_handling(out_call->op, SalOpSDPSimulateError); /* will force a parse error for the ACK SDP*/
		BC_ASSERT_PTR_NOT_NULL(setup_video(caller, callee, TRUE));
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallUpdating,initial_callee_stat.number_of_LinphoneCallUpdating+1));
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallStreamsRunning,initial_callee_stat.number_of_LinphoneCallStreamsRunning+1));
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&caller->stat.number_of_LinphoneCallStreamsRunning,initial_caller_stat.number_of_LinphoneCallStreamsRunning));
		/*Basically the negotiation failed but since the call was already running, we expect it to restore to
		the previous state so error stats should not be changed*/
		BC_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallError,initial_callee_stat.number_of_LinphoneCallError, int, "%d");
		/*and remote should not have received any update notification*/
		BC_ASSERT_EQUAL(caller->stat.number_of_LinphoneCallUpdatedByRemote,initial_caller_stat.number_of_LinphoneCallUpdatedByRemote, int, "%d");

		BC_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(callee->lc))));
		BC_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(caller->lc))));

		sal_call_set_sdp_handling(out_call->op, SalOpSDPNormal);
	}
	end_call(caller, callee);

	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);
#else
	ms_warning("not tested because video not available");
#endif
}


static void call_with_paused_no_sdp_on_resume(void) {
	int dummy=0;
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCall* call_marie = NULL;
	bool_t call_ok;

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	BC_ASSERT_TRUE(call_ok=call(pauline,marie));
	if (!call_ok) goto end;

	liblinphone_tester_check_rtcp(marie,pauline);

	call_marie = linphone_core_get_current_call(marie->lc);
	BC_ASSERT_PTR_NOT_NULL(call_marie);

	ms_message("== Call is OK ==");

	/* the called party pause the call */
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 3000);

	linphone_core_pause_call(marie->lc,call_marie);
	ms_message("== Call pausing ==");
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausing,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausedByRemote,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPaused,1));

	/*stay in pause a little while in order to generate traffic*/
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);

	ms_message("== Call paused, marie call: %p ==", call_marie);

	linphone_core_enable_sdp_200_ack(marie->lc,TRUE);

	linphone_core_resume_call(marie->lc,call_marie);

	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));

	wait_for_until(marie->lc, pauline->lc, &dummy, 1, 3000);
	BC_ASSERT_GREATER(linphone_core_manager_get_max_audio_down_bw(marie),70,int,"%i");
	BC_ASSERT_TRUE(linphone_call_get_audio_stats(linphone_core_get_current_call(pauline->lc))->download_bandwidth>70);
	end_call(marie,pauline);
end:

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void early_media_without_sdp_in_200_base( bool_t use_video, bool_t use_ice ){
	LinphoneCoreManager* marie   = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	MSList* lcs = NULL;
	LinphoneCall* marie_call;
	LinphoneCallParams* params = NULL;
	LinphoneCallLog *marie_call_log;
	uint64_t connected_time=0;
	uint64_t ended_time=0;
	int dummy=0;

	lcs = ms_list_append(lcs,marie->lc);
	lcs = ms_list_append(lcs,pauline->lc);
	if (use_ice){
		linphone_core_set_firewall_policy(marie->lc, LinphonePolicyUseIce);
	}
	/*
		Marie calls Pauline, and after the call has rung, transitions to an early_media session
	*/
	params = linphone_core_create_call_params(marie->lc, NULL);

	if( use_video){

		linphone_call_params_enable_video(params, TRUE);

		linphone_core_enable_video_capture(pauline->lc, TRUE);
		linphone_core_enable_video_display(pauline->lc, TRUE);
		linphone_core_enable_video_capture(marie->lc, TRUE);
		linphone_core_enable_video_display(marie->lc, FALSE);
	}

	marie_call = linphone_core_invite_address_with_params(marie->lc, pauline->identity, params);
	linphone_call_params_destroy(params);
	marie_call_log = linphone_call_get_call_log(marie_call);

	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingReceived,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingRinging,1,1000));

	if (linphone_core_inc_invite_pending(pauline->lc)) {
		LinphoneCall* pauline_call = linphone_core_get_current_call(pauline->lc);

		/* send a 183 to initiate the early media */
		linphone_core_accept_early_media(pauline->lc, pauline_call);

		BC_ASSERT_TRUE( wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingEarlyMedia,1,2000) );
		BC_ASSERT_TRUE( wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingEarlyMedia,1,2000) );

		liblinphone_tester_check_rtcp(marie, pauline);

		/* will send the 200OK _without_ SDP. We expect the early-media SDP to be used instead */
		sal_call_set_sdp_handling(pauline_call->op, SalOpSDPSimulateRemove);
		linphone_core_accept_call(pauline->lc, pauline_call);

		BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallConnected, 1,1000));
		connected_time=ms_get_cur_time_ms();
		BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallStreamsRunning, 1,3000));

		BC_ASSERT_PTR_EQUAL(marie_call, linphone_core_get_current_call(marie->lc));

		liblinphone_tester_check_rtcp(marie, pauline);
		/*just to have a call duration !=0*/
		wait_for_list(lcs,&dummy,1,2000);

		end_call(pauline, marie);
		ended_time=ms_get_cur_time_ms();
		BC_ASSERT_LOWER(labs((long)((linphone_call_log_get_duration(marie_call_log)*1000) - (int64_t)(ended_time - connected_time))), 1000, long, "%ld");
	}
	ms_list_free(lcs);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_early_media_and_no_sdp_in_200_with_video(void){
	early_media_without_sdp_in_200_base(TRUE, FALSE);
}

static void call_with_early_media_and_no_sdp_in_200(void){
	early_media_without_sdp_in_200_base(FALSE, FALSE);
}

static void call_with_early_media_ice_and_no_sdp_in_200(void){
	early_media_without_sdp_in_200_base(FALSE, TRUE);
}

static void call_with_generic_cn(void) {
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCall *pauline_call;
	char *audio_file_with_silence=bc_tester_res("sounds/ahbahouaismaisbon.wav");
	char *recorded_file=bc_tester_file("result.wav");

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	remove(recorded_file);

	linphone_core_use_files(marie->lc,TRUE);
	linphone_core_use_files(pauline->lc,TRUE);
	linphone_core_set_play_file(marie->lc, audio_file_with_silence);
	/*linphone_core_set_play_file(pauline->lc, NULL);*/
	linphone_core_set_record_file(pauline->lc, recorded_file);
	linphone_core_enable_generic_confort_noise(marie->lc, TRUE);
	linphone_core_enable_generic_confort_noise(pauline->lc, TRUE);
	BC_ASSERT_TRUE(call(marie,pauline));
	pauline_call=linphone_core_get_current_call(pauline->lc);
	BC_ASSERT_PTR_NOT_NULL(pauline_call);
	if (pauline_call){
		const rtp_stats_t *rtps;

		wait_for_until(marie->lc, pauline->lc, NULL, 0, 8000);
		rtps=rtp_session_get_stats(pauline_call->audiostream->ms.sessions.rtp_session);
		BC_ASSERT_TRUE(rtps->packet_recv<=300 && rtps->packet_recv>=200);
	}
	end_call(marie,pauline);

	if (pauline_call){
		struct stat stbuf;
		int err;

		err=stat(recorded_file,&stbuf);
		BC_ASSERT_EQUAL(err, 0, int, "%d");
		if (err==0){
			BC_ASSERT_GREATER(stbuf.st_size,120000,int, "%d");
		}
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);

	ms_free(audio_file_with_silence);
	bc_free(recorded_file);
}
void static call_state_changed_2(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg){
	LCSipTransports sip_tr;
	if (cstate==LinphoneCallReleased) {
		/*to make sure transport is changed*/
		sip_tr.udp_port = 0;
		sip_tr.tcp_port = 45876;
		sip_tr.tls_port = 0;

		linphone_core_set_sip_transports(lc,&sip_tr);
	}
}
void static call_state_changed_3(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg){
/*just to check multi listener in such situation*/
	char* to=linphone_address_as_string(linphone_call_get_call_log(call)->to);
	char* from=linphone_address_as_string(linphone_call_get_call_log(call)->from);
	ms_message("Third call listener reports: %s call from [%s] to [%s], new state is [%s]"	,linphone_call_get_call_log(call)->dir==LinphoneCallIncoming?"Incoming":"Outgoing"
																,from
																,to
																,linphone_call_state_to_string(cstate));
	ms_free(to);
	ms_free(from);
}


static void call_with_transport_change_base(bool_t succesfull_call) {
	LCSipTransports sip_tr;
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCoreVTable * v_table;
	v_table = linphone_core_v_table_new();
	v_table->call_state_changed=call_state_changed_2;
	marie = linphone_core_manager_new("marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	linphone_core_add_listener(marie->lc,v_table);
	v_table = linphone_core_v_table_new();
	v_table->call_state_changed=call_state_changed_3;
	linphone_core_add_listener(marie->lc,v_table);

	sip_tr.udp_port = 0;
	sip_tr.tcp_port = 45875;
	sip_tr.tls_port = 0;
	linphone_core_set_sip_transports(marie->lc,&sip_tr);
	if (succesfull_call) {
		BC_ASSERT_TRUE(call(marie,pauline));
		end_call(marie, pauline);
	}
	else
		linphone_core_invite(marie->lc,"nexiste_pas");

	if (succesfull_call)
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallReleased,1));
	if (succesfull_call) {
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallReleased,1));
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void call_with_transport_change_after_released(void) {
	call_with_transport_change_base(TRUE);
}
static void unsucessfull_call_with_transport_change_after_released(void) {
	call_with_transport_change_base(FALSE);
}
#ifdef VIDEO_ENABLED

static void video_call_with_re_invite_inactive_followed_by_re_invite_base(LinphoneMediaEncryption mode, bool_t no_sdp) {
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCallParams *params;
	const LinphoneCallParams *current_params;
	MSList *lcs=NULL;
	bool_t calls_ok;

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	linphone_core_set_avpf_mode(pauline->lc,TRUE);
	linphone_core_set_video_device(pauline->lc,liblinphone_tester_mire_id);
	linphone_core_set_video_device(marie->lc,liblinphone_tester_mire_id);
	linphone_core_set_avpf_mode(marie->lc,TRUE);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,marie->lc);

	video_call_base_2(marie,pauline,TRUE,mode,TRUE,TRUE);

	calls_ok = linphone_core_get_current_call(marie->lc) != NULL && linphone_core_get_current_call(pauline->lc) != NULL;
	BC_ASSERT_TRUE(calls_ok);

	if (calls_ok) {
		params=linphone_core_create_call_params(marie->lc,linphone_core_get_current_call(marie->lc));
		linphone_call_params_set_audio_direction(params,LinphoneMediaDirectionInactive);
		linphone_call_params_set_video_direction(params,LinphoneMediaDirectionInactive);

		linphone_core_update_call(marie->lc, linphone_core_get_current_call(marie->lc),params);
		linphone_call_params_destroy(params);

		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallUpdating,1));
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallPausedByRemote,1));

		check_media_direction(marie,linphone_core_get_current_call(marie->lc),lcs,LinphoneMediaDirectionInactive,LinphoneMediaDirectionInactive);
		check_media_direction(pauline,linphone_core_get_current_call(pauline->lc), lcs, LinphoneMediaDirectionInactive, LinphoneMediaDirectionInactive);

		if (no_sdp) {
			linphone_core_enable_sdp_200_ack(marie->lc,TRUE);
		}

		params=linphone_core_create_call_params(marie->lc,linphone_core_get_current_call(marie->lc));
		linphone_call_params_set_audio_direction(params,LinphoneMediaDirectionSendRecv);
		linphone_call_params_set_video_direction(params,LinphoneMediaDirectionSendRecv);
		linphone_core_update_call(marie->lc,linphone_core_get_current_call(marie->lc),params);
		linphone_call_params_destroy(params);

		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,3));
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));

		check_media_direction(marie,linphone_core_get_current_call(marie->lc),lcs,LinphoneMediaDirectionSendRecv,LinphoneMediaDirectionSendRecv);
		check_media_direction(pauline,linphone_core_get_current_call(pauline->lc),lcs,LinphoneMediaDirectionSendRecv,LinphoneMediaDirectionSendRecv);

		/*assert that after pause and resume, SRTP is still being used*/
		current_params = linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc));
		BC_ASSERT_EQUAL(linphone_call_params_get_media_encryption(current_params) , mode, int, "%d");
		current_params = linphone_call_get_current_params(linphone_core_get_current_call(marie->lc));
		BC_ASSERT_EQUAL(linphone_call_params_get_media_encryption(current_params) , mode, int, "%d");

	}
	end_call(marie,pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void video_call_with_re_invite_inactive_followed_by_re_invite(void) {
	video_call_with_re_invite_inactive_followed_by_re_invite_base(LinphoneMediaEncryptionNone,FALSE);
}

static void video_call_with_re_invite_inactive_followed_by_re_invite_no_sdp(void) {
	video_call_with_re_invite_inactive_followed_by_re_invite_base(LinphoneMediaEncryptionNone, TRUE);
}

static void srtp_video_call_with_re_invite_inactive_followed_by_re_invite(void) {
	if (ms_srtp_supported())
		video_call_with_re_invite_inactive_followed_by_re_invite_base(LinphoneMediaEncryptionSRTP,FALSE);
	else
		ms_message("srtp_video_call_with_re_invite_inactive_followed_by_re_invite skipped, missing srtp support");
}

static void srtp_video_call_with_re_invite_inactive_followed_by_re_invite_no_sdp(void) {
	if (ms_srtp_supported())
		video_call_with_re_invite_inactive_followed_by_re_invite_base(LinphoneMediaEncryptionSRTP, TRUE);
	else
		ms_message("srtp_video_call_with_re_invite_inactive_followed_by_re_invite_no_sdp skipped, missing srtp support");
}

static void video_call_ice_params(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	linphone_core_set_firewall_policy(marie->lc,LinphonePolicyUseIce);
	linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);
	video_call_base(marie,pauline,FALSE,LinphoneMediaEncryptionNone,TRUE,TRUE);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void audio_call_with_video_policy_enabled(void){
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneVideoPolicy vpol;


	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, TRUE);
	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, TRUE);
	vpol.automatically_accept = vpol.automatically_initiate = TRUE;
	linphone_core_set_video_policy(marie->lc, &vpol);
	vpol.automatically_accept = vpol.automatically_initiate = FALSE;
	linphone_core_set_video_policy(pauline->lc, &vpol);

	linphone_core_set_firewall_policy(marie->lc, LinphonePolicyUseIce);
	linphone_core_set_firewall_policy(pauline->lc, LinphonePolicyUseIce);

	linphone_core_invite_address(pauline->lc, marie->identity);
	if (!BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallIncomingReceived, 1))) goto end;
	linphone_core_accept_call(marie->lc, linphone_core_get_current_call(marie->lc));
	/*
	LinphoneCallParams *params;
	params = linphone_core_create_call_params(marie->lc, linphone_core_get_current_call(marie->lc));
	linphone_call_params_enable_video(params, TRUE);
	linphone_core_accept_call_with_params(marie->lc, linphone_core_get_current_call(marie->lc), params);
	linphone_call_params_destroy(params);*/

	/*wait for call to be established and ICE reINVITEs to be done */
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 2));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 2));

	linphone_core_pause_call(marie->lc, linphone_core_get_current_call(marie->lc));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallPausedByRemote, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallPaused, 1));

	end_call(marie, pauline);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


static void classic_video_entry_phone_setup(void) {
	LinphoneCoreManager *callee_mgr = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *caller_mgr = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCallParams *early_media_params = NULL;
	LinphoneCallParams *in_call_params = NULL;
	LinphoneCall *callee_call = NULL;
	LinphoneVideoPolicy vpol = { TRUE, TRUE };
	MSList *lcs = NULL;
	int retry = 0;
	bool_t ok;

	lcs = ms_list_append(lcs, caller_mgr->lc);
	lcs = ms_list_append(lcs, callee_mgr->lc);

	linphone_core_enable_video_capture(caller_mgr->lc, TRUE);
	linphone_core_enable_video_display(caller_mgr->lc, TRUE);
	linphone_core_enable_video_capture(callee_mgr->lc, TRUE);
	linphone_core_enable_video_display(callee_mgr->lc, TRUE);
	linphone_core_set_avpf_mode(caller_mgr->lc, LinphoneAVPFEnabled);
	linphone_core_set_avpf_mode(callee_mgr->lc, LinphoneAVPFEnabled);
	linphone_core_set_video_policy(caller_mgr->lc, &vpol);
	linphone_core_set_video_policy(callee_mgr->lc, &vpol);
	linphone_core_set_video_device(caller_mgr->lc, liblinphone_tester_mire_id);
	linphone_core_set_video_device(callee_mgr->lc, liblinphone_tester_mire_id);

	BC_ASSERT_PTR_NOT_NULL(linphone_core_invite_address(caller_mgr->lc, callee_mgr->identity));

	ok = wait_for(callee_mgr->lc, caller_mgr->lc, &callee_mgr->stat.number_of_LinphoneCallIncomingReceived, 1);
	BC_ASSERT_TRUE(ok);
	if (!ok) goto end;
	BC_ASSERT_TRUE(caller_mgr->stat.number_of_LinphoneCallOutgoingProgress == 1);

	callee_call = linphone_core_get_call_by_remote_address2(callee_mgr->lc, caller_mgr->identity);
	early_media_params = linphone_core_create_call_params(callee_mgr->lc, callee_call);
	linphone_call_params_set_audio_direction(early_media_params, LinphoneMediaDirectionInactive);
	linphone_call_params_set_video_direction(early_media_params, LinphoneMediaDirectionRecvOnly);
	linphone_core_accept_early_media_with_params(callee_mgr->lc, callee_call, early_media_params);
	linphone_call_params_destroy(early_media_params);

	while ((caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia != 1) && (retry++ < 100)) {
		linphone_core_iterate(caller_mgr->lc);
		linphone_core_iterate(callee_mgr->lc);
		ms_usleep(20000);
	}
	BC_ASSERT_TRUE(caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia == 1);
	BC_ASSERT_TRUE(callee_mgr->stat.number_of_LinphoneCallIncomingEarlyMedia == 1);

	check_media_direction(callee_mgr, callee_call, lcs, LinphoneMediaDirectionInactive, LinphoneMediaDirectionRecvOnly);
	callee_call = linphone_core_get_call_by_remote_address2(callee_mgr->lc, caller_mgr->identity);
	in_call_params = linphone_core_create_call_params(callee_mgr->lc, callee_call);
	linphone_call_params_set_audio_direction(in_call_params, LinphoneMediaDirectionSendRecv);
	linphone_call_params_set_video_direction(in_call_params, LinphoneMediaDirectionSendRecv);
	linphone_core_accept_call_with_params(callee_mgr->lc, callee_call, in_call_params);
	linphone_call_params_destroy(in_call_params);

	BC_ASSERT_TRUE(wait_for(callee_mgr->lc, caller_mgr->lc, &callee_mgr->stat.number_of_LinphoneCallConnected, 1));
	BC_ASSERT_TRUE(wait_for(callee_mgr->lc, caller_mgr->lc, &caller_mgr->stat.number_of_LinphoneCallConnected, 1));

	ok = wait_for_until(callee_mgr->lc, caller_mgr->lc, &caller_mgr->stat.number_of_LinphoneCallStreamsRunning, 1, 2000)
		&& wait_for_until(callee_mgr->lc, caller_mgr->lc, &callee_mgr->stat.number_of_LinphoneCallStreamsRunning, 1, 2000);
	BC_ASSERT_TRUE(ok);
	if (!ok) goto end;
	check_media_direction(callee_mgr, callee_call, lcs, LinphoneMediaDirectionSendRecv, LinphoneMediaDirectionSendRecv);

	callee_call = linphone_core_get_current_call(callee_mgr->lc);
	in_call_params = linphone_core_create_call_params(callee_mgr->lc, callee_call);
	linphone_call_params_set_audio_direction(in_call_params, LinphoneMediaDirectionRecvOnly);
	linphone_call_params_set_video_direction(in_call_params, LinphoneMediaDirectionSendOnly);
	linphone_core_update_call(callee_mgr->lc, callee_call, in_call_params);
	linphone_call_params_destroy(in_call_params);

	ok = wait_for_until(callee_mgr->lc, caller_mgr->lc, &caller_mgr->stat.number_of_LinphoneCallStreamsRunning, 2, 2000)
		&& wait_for_until(callee_mgr->lc, caller_mgr->lc, &callee_mgr->stat.number_of_LinphoneCallStreamsRunning, 2, 2000);
	BC_ASSERT_TRUE(ok);
	if (!ok) goto end;
	callee_call = linphone_core_get_current_call(callee_mgr->lc);
	check_media_direction(callee_mgr, callee_call, lcs, LinphoneMediaDirectionRecvOnly, LinphoneMediaDirectionSendOnly);

	end_call(caller_mgr, callee_mgr);

end:
	linphone_core_manager_destroy(callee_mgr);
	linphone_core_manager_destroy(caller_mgr);
	ms_list_free(lcs);
}
#endif

#if !defined(__arm__) && !defined(__arm64__) && !TARGET_IPHONE_SIMULATOR && !defined(ANDROID)
static void completion_cb(void *user_data, int percentage){
	fprintf(stdout,"%i %% completed\r",percentage);
	fflush(stdout);
}
#endif

static void simple_stereo_call(const char *codec_name, int clock_rate, int bitrate_override, bool_t stereo) {
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	PayloadType *pt;
	char *stereo_file = bc_tester_res("sounds/vrroom.wav");
	char *recordpath = bc_tester_file("stereo-record.wav");
	bool_t audio_cmp_failed = FALSE;

	unlink(recordpath);

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	/*make sure we have opus*/
	pt = linphone_core_find_payload_type(marie->lc, codec_name, clock_rate, 2);
	if (!pt) {
		ms_warning("%s not available, stereo with %s not tested.",codec_name, codec_name);
		goto end;
	}
	if (stereo) payload_type_set_recv_fmtp(pt, "stereo=1;sprop-stereo=1");
	if (bitrate_override) linphone_core_set_payload_type_bitrate(marie->lc, pt, bitrate_override);
	pt = linphone_core_find_payload_type(pauline->lc, codec_name, clock_rate, 2);
	if (stereo) payload_type_set_recv_fmtp(pt, "stereo=1;sprop-stereo=1");
	if (bitrate_override) linphone_core_set_payload_type_bitrate(pauline->lc, pt, bitrate_override);

	disable_all_audio_codecs_except_one(marie->lc, codec_name, clock_rate);
	disable_all_audio_codecs_except_one(pauline->lc, codec_name, clock_rate);

	linphone_core_set_use_files(marie->lc, TRUE);
	linphone_core_set_play_file(marie->lc, stereo_file);
	linphone_core_set_use_files(pauline->lc, TRUE);
	linphone_core_set_record_file(pauline->lc, recordpath);

	/*stereo is supported only without volume control, echo canceller...*/
	lp_config_set_string(marie->lc->config,"sound","features","REMOTE_PLAYING");
	lp_config_set_string(pauline->lc->config,"sound","features","REMOTE_PLAYING");

	if (!BC_ASSERT_TRUE(call(pauline,marie))) goto end;
	wait_for_until(marie->lc, pauline->lc, NULL, 0, 6000);
	end_call(pauline, marie);


	if (clock_rate!=48000) {
		ms_warning("Similarity checking not implemented for files not having the same sampling rate");
	}else{
#if !defined(__arm__) && !defined(__arm64__) && !TARGET_IPHONE_SIMULATOR && !defined(ANDROID)
		double similar;
		double min_threshold = .75f; /*should be above 0.8 in best conditions*/
		double max_threshold = 1.f;
		if (!stereo){
			/*when opus doesn't transmit stereo, the cross correlation is around 0.6 : as expected, it is not as good as in full stereo mode*/
			min_threshold = .4f;
			max_threshold = .68f;
		}
		BC_ASSERT_EQUAL(ms_audio_diff(stereo_file, recordpath,&similar,&audio_cmp_params,completion_cb,NULL), 0, int, "%d");
		BC_ASSERT_GREATER(similar, min_threshold, double, "%g");
		BC_ASSERT_LOWER(similar, max_threshold, double, "%g");
		if (similar<min_threshold || similar>max_threshold){
			audio_cmp_failed = TRUE;
		}
#endif
	}
	if (!audio_cmp_failed) unlink(recordpath);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	ms_free(stereo_file);
	bc_free(recordpath);
}

static void simple_stereo_call_l16(void){
	simple_stereo_call("L16", 44100, 0, TRUE);
}

static void simple_stereo_call_opus(void){
	simple_stereo_call("opus", 48000, 150, TRUE);
}

static void call_with_complex_late_offering(void){
	LinphoneCallParams *params;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline =  linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall* call_pauline;
	LinphoneCall* call_marie;
	LinphoneVideoPolicy vpol = {TRUE, TRUE};
	bool_t call_ok;

	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, TRUE);
	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, TRUE);
	linphone_core_set_video_policy(pauline->lc, &vpol);
	linphone_core_set_video_policy(marie->lc, &vpol);
	linphone_core_set_video_device(pauline->lc,liblinphone_tester_mire_id);
	linphone_core_set_video_device(marie->lc,liblinphone_tester_mire_id);

	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));
	if (!call_ok) goto end;

	call_pauline = linphone_core_get_current_call(pauline->lc);
	call_marie = linphone_core_get_current_call(marie->lc);

	//Invite inactive Audio/video (Marie pause Pauline)
	ms_message("CONTEXT: Marie sends INVITE with SDP with all streams inactive");
	params=linphone_core_create_call_params(marie->lc,call_marie);
	linphone_call_params_set_audio_direction(params,LinphoneMediaDirectionInactive);
	linphone_call_params_set_video_direction(params,LinphoneMediaDirectionInactive);

	linphone_core_update_call(marie->lc, call_marie ,params);
	linphone_call_params_destroy(params);

	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallUpdating,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallPausedByRemote,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));

	//Marie sends INVITE without SDP
	ms_message("CONTEXT: Marie sends INVITE without SDP for setting streams in send-only mode");
	linphone_core_enable_sdp_200_ack(marie->lc,TRUE);
	params=linphone_core_create_call_params(marie->lc,call_marie);
	linphone_call_params_set_audio_direction(params,LinphoneMediaDirectionSendOnly);
	linphone_call_params_set_video_direction(params,LinphoneMediaDirectionSendOnly);
	linphone_core_update_call(marie->lc, call_marie , params);
	linphone_call_params_destroy(params);

	//Pauline OK with sendonly
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallUpdating,2));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,3));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallPausedByRemote,2));

	linphone_core_enable_sdp_200_ack(marie->lc,FALSE);

	//Pauline pause Marie
	ms_message("CONTEXT: Pauline pauses the call");
	linphone_core_pause_call(pauline->lc,call_pauline);
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallPausing,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallPaused,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,1));

	//Pauline resume Marie
	ms_message("CONTEXT: Pauline resumes the call");
	wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);
	linphone_core_resume_call(pauline->lc,call_pauline);
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,4));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallResuming,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallPausedByRemote,3));

	wait_for_until(pauline->lc, marie->lc, NULL, 0, 2000);

	//Marie invite inactive Audio/Video
	ms_message("CONTEXT: Marie sends INVITE with SDP with all streams inactive");
	params=linphone_core_create_call_params(marie->lc,call_marie);
	linphone_call_params_set_audio_direction(params,LinphoneMediaDirectionInactive);
	linphone_call_params_set_video_direction(params,LinphoneMediaDirectionInactive);

	linphone_core_update_call(marie->lc, call_marie,params);
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallUpdating,3));
	linphone_call_params_destroy(params);

	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallPausedByRemote,4));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,5));

	//Marie sends INVITE without SDP
	ms_message("CONTEXT: Marie sends INVITE without SDP in the purpose of re-enabling streams in sendrecv mode");
	linphone_core_enable_sdp_200_ack(marie->lc,TRUE);
	params=linphone_core_create_call_params(marie->lc,call_marie);
	linphone_call_params_set_audio_direction(params,LinphoneMediaDirectionSendRecv);
	linphone_call_params_set_video_direction(params,LinphoneMediaDirectionSendRecv);
	linphone_core_update_call(marie->lc, call_marie,params);
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallUpdating,3));
	linphone_call_params_destroy(params);
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallUpdatedByRemote,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,5));

	linphone_core_enable_sdp_200_ack(marie->lc,FALSE);

	end_call(marie,pauline);

end:

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void simple_mono_call_opus(void){
	/*actually a call where input/output is made with stereo but opus transmits everything as mono*/
	simple_stereo_call("opus", 48000, 150, FALSE);
}

/* because SIP ALG (like in android phones) crash when seing a domain name in SDP, we prefer using SIP/TLS for both participants*/
static void call_with_fqdn_in_sdp(void) {
	bool_t tls_supported = transport_supported(LinphoneTransportTls);
	LinphoneCoreManager* marie = linphone_core_manager_new(tls_supported ? "marie_sips_rc" : "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(tls_supported ? "pauline_rc" : "pauline_tcp_rc");
	LpConfig *lp;
	bool_t call_ok;

	lp = linphone_core_get_config(marie->lc);
	lp_config_set_string(lp,"rtp","bind_address","localhost");
	lp = linphone_core_get_config(pauline->lc);
	lp_config_set_string(lp,"rtp","bind_address","localhost");


	BC_ASSERT_TRUE(call_ok=call(marie,pauline));
	if (!call_ok) goto end;
	liblinphone_tester_check_rtcp(pauline,marie);

#ifdef VIDEO_ENABLED
	BC_ASSERT_TRUE(add_video(pauline,marie, TRUE));
	liblinphone_tester_check_rtcp(pauline,marie);
#endif
	end_call(pauline, marie);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_rtp_io_mode(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphonePlayer *player;
	char *hellopath = bc_tester_res("sounds/ahbahouaismaisbon.wav");
	char *recordpath = create_filepath(bc_tester_get_writable_dir_prefix(), "record-call_with_rtp_io_mode", "wav");
	bool_t call_ok;
	int attempts;
	double similar=1;
	const double threshold = 0.85;

	/*this test is actually attempted three times in case of failure, because the audio comparison at the end is very sensitive to
	 * jitter buffer drifts, which sometimes happen if the machine is unable to run the test in good realtime conditions */
	for (attempts=0; attempts<3; attempts++){
		/* Make sure that the record file doesn't already exists, otherwise this test will append new samples to it. */
		unlink(recordpath);
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);

		/* The caller uses files instead of soundcard in order to avoid mixing soundcard input with file played using call's player. */
		linphone_core_use_files(marie->lc, TRUE);
		linphone_core_set_play_file(marie->lc, NULL);
		linphone_core_set_record_file(marie->lc, recordpath);
		linphone_core_use_files(pauline->lc, FALSE);

		/* The callee uses the RTP IO mode with the PCMU codec to send back audio to the caller. */
		disable_all_audio_codecs_except_one(pauline->lc, "pcmu", -1);
		lp_config_set_int(pauline->lc->config, "sound", "rtp_io", 1);
		lp_config_set_string(pauline->lc->config, "sound", "rtp_local_addr", "127.0.0.1");
		lp_config_set_string(pauline->lc->config, "sound", "rtp_remote_addr", "127.0.0.1");
		lp_config_set_int(pauline->lc->config, "sound", "rtp_local_port", 17076);
		lp_config_set_int(pauline->lc->config, "sound", "rtp_remote_port", 17076);
		lp_config_set_string(pauline->lc->config, "sound", "rtp_map", "pcmu/8000/1");

		BC_ASSERT_TRUE((call_ok = call(marie, pauline)));
		if (!call_ok) goto end;
		player = linphone_call_get_player(linphone_core_get_current_call(marie->lc));
		BC_ASSERT_PTR_NOT_NULL(player);
		if (player) {
			BC_ASSERT_EQUAL(linphone_player_open(player, hellopath, on_eof, marie) , 0, int, "%d");
			BC_ASSERT_EQUAL(linphone_player_start(player) , 0, int, "%d");
		}

		/* This assert should be modified to be at least as long as the WAV file */
		BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &marie->stat.number_of_player_eof, 1, 10000));
		/*wait for one second more so that last RTP packets can arrive*/
		wait_for_until(pauline->lc,marie->lc,NULL,0,1000);
		end_call(pauline,marie);

		BC_ASSERT_EQUAL(ms_audio_diff(hellopath, recordpath, &similar, &audio_cmp_params, NULL, NULL), 0, int, "%d");
		if (similar>=threshold) break;
	}
	BC_ASSERT_GREATER(similar, threshold, double, "%g");
	BC_ASSERT_LOWER(similar, 1.0, double, "%g");
	if (similar >= threshold && similar <= 1.0) {
		remove(recordpath);
	}

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	ms_free(recordpath);
	ms_free(hellopath);
}

static void generic_nack_received(const OrtpEventData *evd, stats *st) {
	if (rtcp_is_RTPFB(evd->packet)) {
		switch (rtcp_RTPFB_get_type(evd->packet)) {
			case RTCP_RTPFB_NACK:
				st->number_of_rtcp_generic_nack++;
				break;
			default:
				break;
		}
	}
}

static void call_with_generic_nack_rtcp_feedback(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LpConfig *lp;
	LinphoneCall *call_marie;
	bool_t call_ok;
	OrtpNetworkSimulatorParams params = { 0 };

	params.enabled = TRUE;
	params.loss_rate = 10;
	params.consecutive_loss_probability = 0.75;
	params.mode = OrtpNetworkSimulatorOutbound;
	linphone_core_set_avpf_mode(marie->lc, LinphoneAVPFEnabled);
	linphone_core_set_avpf_mode(pauline->lc, LinphoneAVPFEnabled);
	lp = linphone_core_get_config(pauline->lc);
	lp_config_set_int(lp, "rtp", "rtcp_fb_generic_nack_enabled", 1);


	BC_ASSERT_TRUE(call_ok = call(pauline, marie));
	if (!call_ok) goto end;
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 1));
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 1));
	call_marie = linphone_core_get_current_call(marie->lc);
	if (call_marie) {
		rtp_session_enable_network_simulation(call_marie->audiostream->ms.sessions.rtp_session, &params);
		ortp_ev_dispatcher_connect(media_stream_get_event_dispatcher(&call_marie->audiostream->ms),
			ORTP_EVENT_RTCP_PACKET_RECEIVED, RTCP_RTPFB, (OrtpEvDispatcherCb)generic_nack_received, &marie->stat);
	}

	BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &marie->stat.number_of_rtcp_generic_nack, 5, 8000));
	end_call(pauline, marie);

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

// This is a custom structure used for the tests using custom RTP transport modifier.
// It is only used to count the number of sent and received packets
typedef struct _RtpTransportModifierData {
	uint64_t packetSentCount;
	uint64_t packetReceivedCount;
	MSQueue to_send;
	MSQueue to_recv;
} RtpTransportModifierData;

const char *XOR_KEY = "BELLEDONNECOMMUNICATIONS";

// Callback called when a packet is on it's way to be sent
// This is where we can do some changes on it, like encrypt it
static int rtptm_on_send(RtpTransportModifier *rtptm, mblk_t *msg) {
	RtpTransportModifierData *data = rtptm->data;
	rtp_header_t *rtp = (rtp_header_t*)msg->b_rptr;

	if (rtp->version == 0) {
		// This is probably a STUN packet, so don't count it (oRTP won't) and don't encrypt it either
		return (int)msgdsize(msg);
	}
	/*ms_message("rtptm_on_send: rtpm=%p seq=%u", rtptm, (int)ntohs(rtp_get_seqnumber(msg)));*/

	data->packetSentCount += 1;
	ms_queue_put(&data->to_send, dupmsg(msg));
	return 0; // Return 0 to drop the packet, it will be injected later
}

// Callback called when a packet is on it's way to be received
// This is where we can do some changes on it, like decrypt it
static int rtptm_on_receive(RtpTransportModifier *rtptm, mblk_t *msg) {
	RtpTransportModifierData *data = rtptm->data;
	rtp_header_t *rtp = (rtp_header_t*)msg->b_rptr;

	if (rtp->version == 0) {
		// This is probably a STUN packet, so don't count it (oRTP won't) and don't decrypt it either
		return (int)msgdsize(msg);
	}

	data->packetReceivedCount += 1;
	ms_queue_put(&data->to_recv, dupmsg(msg));
	return 0; // Return 0 to drop the packet, it will be injected later
}

static void rtptm_on_schedule(RtpTransportModifier *rtptm) {
	RtpTransportModifierData *data = rtptm->data;
	mblk_t *msg = NULL;

	while ((msg = ms_queue_get(&data->to_send)) != NULL) {
		int size = 0;
		unsigned char *src;
		int i = 0;

		// Mediastream can create a mblk_t with only the RTP header and setting the b_cont pointer to the actual RTP content buffer
		// In this scenario, the result of rtp_get_payload will be 0, and we won't be able to do our XOR encryption on the payload
		// The call to msgpullup will trigger a memcpy of the header and the payload in the same buffer in the msg mblk_t
		msgpullup(msg, -1);
		// Now that the mblk_t buffer directly contains the header and the payload, we can get the size of the payload and a pointer to it's start (we don't encrypt the RTP header)
		size = rtp_get_payload(msg, &src);

		// Just for fun, let's do a XOR encryption
		for (i = 0; i < size; i++) {
			src[i] ^= (unsigned char) XOR_KEY[i % strlen(XOR_KEY)];
		}

		meta_rtp_transport_modifier_inject_packet_to_send(rtptm->transport, rtptm, msg, 0);
	}

	msg = NULL;
	while ((msg = ms_queue_get(&data->to_recv)) != NULL) {
		int size = 0;
		unsigned char *src;
		int i = 0;

		// On the receiving side, there is no need for a msgpullup, the mblk_t contains the header and the payload in the same buffer
		// We just ask for the size and a pointer to the payload buffer
		size = rtp_get_payload(msg, &src);

		// Since we did a XOR encryption on the send side, we have to do it again to decrypt the payload
		for (i = 0; i < size; i++) {
			src[i] ^= (unsigned char) XOR_KEY[i % strlen(XOR_KEY)];
		}

		meta_rtp_transport_modifier_inject_packet_to_recv(rtptm->transport, rtptm, msg, 0);
	}
}

// This callback is called when the transport modifier is being destroyed
// It is a good place to free the resources allocated for the transport modifier
static void rtptm_destroy(RtpTransportModifier *rtptm)  {
	// Do nothing, we'll free it later because we need to access the RtpTransportModifierData structure after the call is ended
}

// This is the callback called when the state of the call change
void static call_state_changed_4(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg) {
	int i = 0;

	// To add a custom RTP transport modifier, we have to do it before the call is running, but after the RTP session is created.
	if (cstate == LinphoneCallIncomingReceived || cstate == LinphoneCallOutgoingProgress) {
		RtpTransport *rtpt = NULL;
		RtpTransportModifierData *data = ms_new0(RtpTransportModifierData, 1);
		RtpTransportModifier *rtptm = ms_new0(RtpTransportModifier, 1);
		ms_queue_init(&data->to_send);
		ms_queue_init(&data->to_recv);
		rtptm->data = data;
		rtptm->t_process_on_send = rtptm_on_send;
		rtptm->t_process_on_receive = rtptm_on_receive;
		rtptm->t_process_on_schedule = rtptm_on_schedule;
		rtptm->t_destroy = rtptm_destroy;

		// Here we iterate on each meta rtp transport available
		for (i = 0; i < linphone_call_get_stream_count(call); i++) {
			MSFormatType type;

			rtpt = linphone_call_get_meta_rtp_transport(call, i);

			// If we wanted, we also could get the RTCP meta transports like this:
			// rtcpt = linphone_call_get_meta_rtcp_transport(call, i);

			// If you want to know which stream meta RTP transport is the current one, you can use
			type = linphone_call_get_stream_type(call, i);
			// Currently there is only MSAudio and MSVideo types, but this could change later
			if (type == MSAudio) {
				// And now we append our RTP transport modifier to the current list of modifiers
				meta_rtp_transport_append_modifier(rtpt, rtptm);
			} else if (type == MSVideo) {
				// Because the call of this test is audio only, we don't have to append our modifier to the meta RTP transport from the video stream
			}
		}
		// We save the pointer to our RtpTransportModifier in the call user_data to be able to get to it later
		call->user_data = rtptm;
	}
}

static void custom_rtp_modifier(bool_t pauseResumeTest, bool_t recordTest) {
	// This will initialize two linphone core using information contained in the marie_rc and pauline_rc files and wait for them to be correctly registered
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall* call_pauline = NULL;
	LinphoneCall* call_marie = NULL;
	const rtp_stats_t * stats;
	bool_t call_ok;
	LinphoneCoreVTable * v_table;
	RtpTransportModifier *rtptm_marie = NULL;
	RtpTransportModifier *rtptm_pauline = NULL;
	RtpTransportModifierData *data_marie = NULL;
	RtpTransportModifierData *data_pauline = NULL;
	// The following are only used for the record test
	LinphonePlayer *player;
	char *hellopath = bc_tester_res("sounds/ahbahouaismaisbon.wav"); // File to be played
	char *recordpath = create_filepath(bc_tester_get_writable_dir_prefix(), "record-call_with_file_player", "wav"); // File to record the received sound
	double similar = 1; // The factor of similarity between the played file and the one recorded
	const double threshold = 0.85; // Minimum similarity value to consider the record file equal to the one sent

	// We create a new vtable to listen only to the call state changes, in order to plug our RTP Transport Modifier when the call will be established
	v_table = linphone_core_v_table_new();
	v_table->call_state_changed = call_state_changed_4;
	linphone_core_add_listener(pauline->lc,v_table);
	v_table = linphone_core_v_table_new();
	v_table->call_state_changed = call_state_changed_4;
	linphone_core_add_listener(marie->lc,v_table);


	if (recordTest) { // When we do the record test, we need a file player to play the content of a sound file
		/*make sure the record file doesn't already exists, otherwise this test will append new samples to it*/
		unlink(recordpath);

		linphone_core_use_files(pauline->lc,TRUE);
		linphone_core_set_play_file(pauline->lc,NULL);
		linphone_core_set_record_file(pauline->lc,NULL);

		/*callee is recording and plays file*/
		linphone_core_use_files(marie->lc,TRUE);
		linphone_core_set_play_file(marie->lc,NULL);
		linphone_core_set_record_file(marie->lc,recordpath);
	}

	// Now the the call should be running (call state StreamsRunning)
	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));

	if (!call_ok) goto end;

	// Ref the call to keep the pointer valid even after the call is release
	call_pauline = linphone_call_ref(linphone_core_get_current_call(pauline->lc));
	call_marie = linphone_call_ref(linphone_core_get_current_call(marie->lc));

	// This is for the pause/resume test, we don't do it in the call record test to be able to check the recorded call matches the file played
	if (pauseResumeTest) {
		// This only wait for 3 seconds in order to generate traffic for the test
		wait_for_until(pauline->lc, marie->lc, NULL, 5, 3000);

		linphone_core_pause_call(pauline->lc,call_pauline);
		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneCallPausing, 1));
		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneCallPausedByRemote, 1));
		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneCallPaused, 1));

		/*stay in pause a little while in order to generate traffic*/
		wait_for_until(pauline->lc, marie->lc, NULL, 5, 2000);

		linphone_core_resume_call(pauline->lc,call_pauline);

		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 2));
		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 2));

		/*same here: wait a while for a bit of a traffic, we need to receive a RTCP packet*/
		wait_for_until(pauline->lc, marie->lc, NULL, 5, 5000);

		/*since RTCP streams are reset when call is paused/resumed, there should be no loss at all*/
		stats = rtp_session_get_stats(call_pauline->sessions->rtp_session);
		BC_ASSERT_EQUAL((int)stats->cum_packet_loss, 0, int, "%d");

		end_call(pauline, marie);
	} else if (recordTest) {
		player = linphone_call_get_player(call_pauline);
		BC_ASSERT_PTR_NOT_NULL(player);
		if (player) {
			// This will ask pauline to play the file
			BC_ASSERT_EQUAL(linphone_player_open(player, hellopath, on_eof, pauline),0, int, "%d");
			BC_ASSERT_EQUAL(linphone_player_start(player), 0, int, "%d");
		}
		/* This assert should be modified to be at least as long as the WAV file */
		BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &pauline->stat.number_of_player_eof, 1, 10000));
		/*wait one second more for transmission to be fully ended (transmission time + jitter buffer)*/
		wait_for_until(pauline->lc, marie->lc, NULL, 0, 1000);

		end_call(pauline, marie);

		// Now we compute a similarity factor between the original file and the one we recorded on the callee side
		BC_ASSERT_EQUAL(ms_audio_diff(hellopath, recordpath, &similar, &audio_cmp_params, NULL, NULL), 0, int, "%d");

		BC_ASSERT_GREATER(similar, threshold, double, "%g");
		BC_ASSERT_LOWER(similar, 1.0, double, "%g");
		if (similar >= threshold && similar <= 1.0) {
			// If the similarity value is between perfect (1) and our threshold (0.9), then we delete the file used for the record
			remove(recordpath);
		}
	} else {
		// This only wait for 3 seconds in order to generate traffic for the test
		wait_for_until(pauline->lc, marie->lc, NULL, 5, 3000);

		// We termine the call and check the stats to see if the call is correctly ended on both sides
		end_call(pauline, marie);
	}

	// Now we can go fetch our custom structure and check the number of packets sent/received is the same on both sides
	rtptm_marie = (RtpTransportModifier *)call_marie->user_data;
	rtptm_pauline = (RtpTransportModifier *)call_pauline->user_data;
	data_marie = (RtpTransportModifierData *)rtptm_marie->data;
	data_pauline = (RtpTransportModifierData *)rtptm_pauline->data;

	BC_ASSERT_PTR_NOT_NULL(data_marie);
	BC_ASSERT_PTR_NOT_NULL(data_pauline);
	ms_message("Marie sent %i RTP packets and received %i (through our modifier)", (int)data_marie->packetSentCount, (int)data_marie->packetReceivedCount);
	ms_message("Pauline sent %i RTP packets and received %i (through our modifier)", (int)data_pauline->packetSentCount, (int)data_pauline->packetReceivedCount);
	// There will be a few RTP packets sent on marie's side before the call is ended at pauline's request, so we need the threshold
	BC_ASSERT_TRUE(data_marie->packetSentCount - data_pauline->packetReceivedCount < 50);
	BC_ASSERT_TRUE(data_pauline->packetSentCount - data_marie->packetReceivedCount < 50);
	// At this point, we know each packet that has been processed in the send callback of our RTP modifier also go through the recv callback of the remote.

	// Now we want to ensure that all sent RTP packets actually go through our RTP transport modifier and thus no packet leave without being processed (by any operation we might want to do on it)
	{
		const LinphoneCallStats *marie_stats = linphone_call_get_audio_stats(call_marie);
		const LinphoneCallStats *pauline_stats = linphone_call_get_audio_stats(call_pauline);
		rtp_stats_t marie_rtp_stats = linphone_call_stats_get_rtp_stats(marie_stats);
		rtp_stats_t pauline_rtp_stats = linphone_call_stats_get_rtp_stats(pauline_stats);
		ms_message("Marie sent %i RTP packets and received %i (for real)", (int)marie_rtp_stats.packet_sent, (int)marie_rtp_stats.packet_recv);
		ms_message("Pauline sent %i RTP packets and received %i (for real)", (int)pauline_rtp_stats.packet_sent, (int)pauline_rtp_stats.packet_recv);
		BC_ASSERT_TRUE(data_marie->packetReceivedCount == marie_rtp_stats.packet_recv);
		BC_ASSERT_TRUE(data_marie->packetSentCount == marie_rtp_stats.packet_sent);
		// There can be a small difference between the number of packets received in the modifier and the number processed in reception because the processing is asynchronous
		BC_ASSERT_TRUE(data_pauline->packetReceivedCount - pauline_rtp_stats.packet_recv < 20);
		BC_ASSERT_TRUE(data_pauline->packetSentCount == pauline_rtp_stats.packet_sent);
	}

end:
	// Since we didn't free the resources of our RTP transport modifier in the rtptm_destroy callback, we'll do it here
	if (data_pauline) {
		ms_free(data_pauline);
	}
	ms_free(rtptm_pauline);
	if (data_marie) {
		ms_free(data_marie);
	}
	ms_free(rtptm_marie);

	// Unref the previously ref calls
	if (call_marie) {
		linphone_call_unref(call_marie);
	}
	if (call_pauline) {
		linphone_call_unref(call_pauline);
	}

	// The test is finished, the linphone core are no longer needed, we can safely free them
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);

	ms_free(recordpath);
	ms_free(hellopath);
}

static void call_with_custom_rtp_modifier(void) {
	custom_rtp_modifier(FALSE, FALSE);
}

static void call_paused_resumed_with_custom_rtp_modifier(void) {
	custom_rtp_modifier(TRUE, FALSE);
}

static void call_record_with_custom_rtp_modifier(void) {
	custom_rtp_modifier(FALSE, TRUE);
}

static void _call_with_network_switch_in_early_state(bool_t network_loosed_by_caller){
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	linphone_core_invite_address(marie->lc, pauline->identity);
	if (!BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallIncomingReceived, 1))) goto end;
	if (!BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallOutgoingRinging, 1))) goto end;
	if (network_loosed_by_caller){
		linphone_core_set_network_reachable(marie->lc, FALSE);
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallError, 1));
		linphone_core_set_network_reachable(marie->lc, TRUE);
		linphone_core_terminate_call(pauline->lc, linphone_core_get_current_call(pauline->lc));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallEnd, 1));
	}else{
		linphone_core_set_network_reachable(pauline->lc, FALSE);
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallError, 1));
		linphone_core_set_network_reachable(pauline->lc, TRUE);
		linphone_core_terminate_call(marie->lc, linphone_core_get_current_call(marie->lc));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallEnd, 1));
	}
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallReleased, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallReleased, 1));
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_network_switch_in_early_state_1(void){
	_call_with_network_switch_in_early_state(TRUE);
}

static void call_with_network_switch_in_early_state_2(void){
	_call_with_network_switch_in_early_state(FALSE);
}

static void _call_with_network_switch(bool_t use_ice, bool_t with_socket_refresh, bool_t enable_rtt) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCallParams *pauline_params = NULL;
	MSList *lcs = NULL;
	bool_t call_ok;

	lcs = ms_list_append(lcs, marie->lc);
	lcs = ms_list_append(lcs, pauline->lc);

	if (use_ice){
		linphone_core_set_firewall_policy(marie->lc,LinphonePolicyUseIce);
		linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);
	}
	if (with_socket_refresh){
		lp_config_set_int(linphone_core_get_config(marie->lc), "net", "recreate_sockets_when_network_is_up", 1);
		lp_config_set_int(linphone_core_get_config(pauline->lc), "net", "recreate_sockets_when_network_is_up", 1);
	}
	if (enable_rtt) {
		pauline_params = linphone_core_create_call_params(pauline->lc, NULL);
		linphone_call_params_enable_realtime_text(pauline_params, TRUE);
	}
	
	BC_ASSERT_TRUE((call_ok=call_with_params(pauline, marie, pauline_params, NULL)));
	if (!call_ok) goto end;

	wait_for_until(marie->lc, pauline->lc, NULL, 0, 2000);
	if (use_ice) {
		BC_ASSERT_TRUE(check_ice(pauline,marie,LinphoneIceStateHostConnection));
		/*wait for ICE reINVITE to complete*/
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 2));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 2));
	}

	/*marie looses the network and reconnects*/
	linphone_core_set_network_reachable(marie->lc, FALSE);
	wait_for_until(marie->lc, pauline->lc, NULL, 0, 1000);

	/*marie will reconnect and register*/
	linphone_core_set_network_reachable(marie->lc, TRUE);
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneRegistrationOk, 2));

	if (use_ice){
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallUpdating, 1));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallUpdatedByRemote, 1));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 3));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 3));
		/*now comes the ICE reINVITE*/
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallUpdating, 2));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallUpdatedByRemote, 2));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 4));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 4));
	}else{
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallUpdating, 1));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallUpdatedByRemote, 1));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 2));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 2));
	}

	/*check that media is back*/
	check_media_direction(marie, linphone_core_get_current_call(marie->lc), lcs, LinphoneMediaDirectionSendRecv, LinphoneMediaDirectionInvalid);
	liblinphone_tester_check_rtcp(pauline, marie);
	if (use_ice) BC_ASSERT_TRUE(check_ice(pauline,marie,LinphoneIceStateHostConnection));

	/*pauline shall be able to end the call without problem now*/
	end_call(pauline, marie);
end:
	ms_list_free(lcs);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_network_switch(void){
	_call_with_network_switch(FALSE, FALSE, FALSE);
}

static void call_with_network_switch_and_ice(void){
	_call_with_network_switch(TRUE, FALSE, FALSE);
}

static void call_with_network_switch_ice_and_rtt(void) {
	_call_with_network_switch(TRUE, FALSE, TRUE);
}

static void call_with_network_switch_and_socket_refresh(void){
	_call_with_network_switch(TRUE, TRUE, FALSE);
}

static void call_with_sip_and_rtp_independant_switches(void){
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	MSList *lcs = NULL;
	bool_t call_ok;
	bool_t use_ice = TRUE;
	bool_t with_socket_refresh = TRUE;

	lcs = ms_list_append(lcs, marie->lc);
	lcs = ms_list_append(lcs, pauline->lc);

	if (use_ice){
		linphone_core_set_firewall_policy(marie->lc,LinphonePolicyUseIce);
		linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);
	}
	if (with_socket_refresh){
		lp_config_set_int(linphone_core_get_config(marie->lc), "net", "recreate_sockets_when_network_is_up", 1);
		lp_config_set_int(linphone_core_get_config(pauline->lc), "net", "recreate_sockets_when_network_is_up", 1);
	}

	linphone_core_set_media_network_reachable(marie->lc, TRUE);

	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));
	if (!call_ok) goto end;

	wait_for_until(marie->lc, pauline->lc, NULL, 0, 2000);
	if (use_ice) {
		BC_ASSERT_TRUE(check_ice(pauline,marie,LinphoneIceStateHostConnection));
		/*wait for ICE reINVITE to complete*/
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 2));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 2));
	}
	/*marie looses the SIP network and reconnects*/
	linphone_core_set_sip_network_reachable(marie->lc, FALSE);
	linphone_core_set_media_network_reachable(marie->lc, FALSE);
	wait_for_until(marie->lc, pauline->lc, NULL, 0, 1000);

	/*marie will reconnect and register*/
	linphone_core_set_sip_network_reachable(marie->lc, TRUE);
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneRegistrationOk, 2));
	wait_for_until(marie->lc, pauline->lc, NULL, 0, 5000);
	/*at this stage, no reINVITE is expected to be send*/
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallUpdating, 0, int, "%i");

	/*now we notify the a reconnection of media network*/
	linphone_core_set_media_network_reachable(marie->lc, TRUE);

	if (use_ice){
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallUpdating, 1));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallUpdatedByRemote, 1));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 3));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 3));
		/*now comes the ICE reINVITE*/
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallUpdating, 2));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallUpdatedByRemote, 2));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 4));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 4));
	}else{
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallUpdating, 1));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallUpdatedByRemote, 1));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 2));
		BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 2));
	}

	/*check that media is back*/
	check_media_direction(marie, linphone_core_get_current_call(marie->lc), lcs, LinphoneMediaDirectionSendRecv, LinphoneMediaDirectionInvalid);
	liblinphone_tester_check_rtcp(pauline, marie);
	if (use_ice) BC_ASSERT_TRUE(check_ice(pauline,marie,LinphoneIceStateHostConnection));

	/*pauline shall be able to end the call without problem now*/
	end_call(pauline, marie);
end:
	ms_list_free(lcs);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


#ifdef CALL_LOGS_STORAGE_ENABLED

static void call_logs_if_no_db_set(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new("laure_call_logs_rc");
	BC_ASSERT_TRUE(ms_list_size(laure->lc->call_logs) == 10);

	BC_ASSERT_TRUE(call(marie, laure));
	wait_for_until(marie->lc, laure->lc, NULL, 5, 1000);
	end_call(marie, laure);

	BC_ASSERT_TRUE(ms_list_size(laure->lc->call_logs) == 11);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(laure);
}

static void call_logs_migrate(void) {
	LinphoneCoreManager* laure = linphone_core_manager_new("laure_call_logs_rc");
	char *logs_db = create_filepath(bc_tester_get_writable_dir_prefix(), "call_logs", "db");
	int i = 0;
	int incoming_count = 0, outgoing_count = 0, missed_count = 0, aborted_count = 0, decline_count = 0, video_enabled_count = 0;

	unlink(logs_db);
	BC_ASSERT_TRUE(ms_list_size(laure->lc->call_logs) == 10);

	linphone_core_set_call_logs_database_path(laure->lc, logs_db);
	BC_ASSERT_TRUE(linphone_core_get_call_history_size(laure->lc) == 10);

	for (; i < ms_list_size(laure->lc->call_logs); i++) {
		LinphoneCallLog *log = ms_list_nth_data(laure->lc->call_logs, i);
		LinphoneCallStatus state = linphone_call_log_get_status(log);
		LinphoneCallDir direction = linphone_call_log_get_dir(log);

		if (state == LinphoneCallAborted) {
			aborted_count += 1;
		} else if (state == LinphoneCallMissed) {
			missed_count += 1;
		} else if (state == LinphoneCallDeclined) {
			decline_count += 1;
		}

		if (direction == LinphoneCallOutgoing) {
			outgoing_count += 1;
		} else {
			incoming_count += 1;
		}

		if (linphone_call_log_video_enabled(log)) {
			video_enabled_count += 1;
		}
	}
	BC_ASSERT_TRUE(incoming_count == 5);
	BC_ASSERT_TRUE(outgoing_count == 5);
	BC_ASSERT_TRUE(missed_count == 1);
	BC_ASSERT_TRUE(aborted_count == 3);
	BC_ASSERT_TRUE(decline_count == 2);
	BC_ASSERT_TRUE(video_enabled_count == 3);

	{
		LinphoneCallLog *log = linphone_core_get_last_outgoing_call_log(laure->lc);
		BC_ASSERT_PTR_NOT_NULL(log);
		if (log) {
			BC_ASSERT_EQUAL((int)log->start_date_time, 1441738272, int, "%d");
			linphone_call_log_unref(log);
			log = NULL;
		}
	}

	laure->lc->call_logs = ms_list_free_with_data(laure->lc->call_logs, (void (*)(void*))linphone_call_log_unref);
	call_logs_read_from_config_file(laure->lc);
	BC_ASSERT_TRUE(ms_list_size(laure->lc->call_logs) == 0);

	unlink(logs_db);
	ms_free(logs_db);
	linphone_core_manager_destroy(laure);
}

static void call_logs_sqlite_storage(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	char *logs_db = create_filepath(bc_tester_get_writable_dir_prefix(), "call_logs", "db");
	MSList *logs = NULL;
	LinphoneCallLog *call_log = NULL;
	LinphoneAddress *laure = NULL;
	time_t user_data_time = time(NULL);
	time_t start_time = 0;
	unlink(logs_db);

	linphone_core_set_call_logs_database_path(marie->lc, logs_db);
	BC_ASSERT_TRUE(linphone_core_get_call_history_size(marie->lc) == 0);

	BC_ASSERT_TRUE(call(marie, pauline));
	wait_for_until(marie->lc, pauline->lc, NULL, 5, 500);
	call_log = linphone_call_get_call_log(linphone_core_get_current_call(marie->lc));
	start_time = linphone_call_log_get_start_date(call_log);
	linphone_call_log_set_user_data(call_log, &user_data_time);
	linphone_call_log_set_ref_key(call_log, "ref_key");
	end_call(marie, pauline);
	BC_ASSERT_TRUE(linphone_core_get_call_history_size(marie->lc) == 1);

	logs = linphone_core_get_call_history_for_address(marie->lc, linphone_proxy_config_get_identity_address(linphone_core_get_default_proxy_config(pauline->lc)));
	BC_ASSERT_TRUE(ms_list_size(logs) == 1);
	ms_list_free_with_data(logs, (void (*)(void*))linphone_call_log_unref);

	laure = linphone_address_new("\"Laure\" <sip:laure@sip.example.org>");
	logs = linphone_core_get_call_history_for_address(marie->lc, laure);
	BC_ASSERT_TRUE(ms_list_size(logs) == 0);
	linphone_address_destroy(laure);

	logs = linphone_core_get_call_history_for_address(marie->lc, linphone_proxy_config_get_identity_address(linphone_core_get_default_proxy_config(pauline->lc)));
	if (BC_ASSERT_TRUE(ms_list_size(logs) == 1)) {
		const char *call_id;
		const char *ref_key = linphone_call_log_get_ref_key(call_log);
		call_log = logs->data;
		BC_ASSERT_EQUAL(linphone_call_log_get_dir(call_log), LinphoneCallOutgoing, int, "%d");
		BC_ASSERT_LOWER(linphone_call_log_get_duration(call_log), 2, float, "%.1f");
		BC_ASSERT_TRUE(linphone_address_equal(
			linphone_call_log_get_from_address(call_log),
			linphone_proxy_config_get_identity_address(linphone_core_get_default_proxy_config(marie->lc))));
		BC_ASSERT_TRUE(linphone_address_equal(
			linphone_call_log_get_to_address(call_log),
			linphone_proxy_config_get_identity_address(linphone_core_get_default_proxy_config(pauline->lc))));
		BC_ASSERT_PTR_NOT_NULL(linphone_call_log_get_local_stats(call_log));
		BC_ASSERT_GREATER(linphone_call_log_get_quality(call_log), -1, int, "%d");
		BC_ASSERT_PTR_NOT_NULL(ref_key);
		if (ref_key) {
			BC_ASSERT_STRING_EQUAL(ref_key, "ref_key");
		}
		BC_ASSERT_PTR_EQUAL(linphone_call_log_get_user_data(call_log), &user_data_time);

		call_id = linphone_call_log_get_call_id(call_log);
		BC_ASSERT_PTR_NOT_NULL(call_id);
		{
			LinphoneCallLog* find_call_log = linphone_core_find_call_log_from_call_id(marie->lc, call_id);
			BC_ASSERT_PTR_NOT_NULL(find_call_log);
			if (find_call_log) linphone_call_log_unref(find_call_log);
		}

		BC_ASSERT_TRUE(linphone_address_equal(
			linphone_call_log_get_remote_address(call_log),
			linphone_proxy_config_get_identity_address(linphone_core_get_default_proxy_config(pauline->lc))));
		BC_ASSERT_PTR_NOT_NULL(linphone_call_log_get_remote_stats(call_log));

		BC_ASSERT_EQUAL(linphone_call_log_get_start_date(call_log), start_time, int, "%d");
		BC_ASSERT_EQUAL(linphone_call_log_get_status(call_log), LinphoneCallSuccess, int, "%d");
	}

	linphone_core_delete_call_log(marie->lc, (LinphoneCallLog *)ms_list_nth_data(logs, 0));
	ms_list_free_with_data(logs, (void (*)(void*))linphone_call_log_unref);
	BC_ASSERT_TRUE(linphone_core_get_call_history_size(marie->lc) == 0);

	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);
	BC_ASSERT_TRUE(call(marie, pauline));
	end_call(marie, pauline);

	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);
	BC_ASSERT_TRUE(call(marie, pauline));
	end_call(marie, pauline);
	BC_ASSERT_TRUE(linphone_core_get_call_history_size(marie->lc) == 2);

	linphone_core_delete_call_history(marie->lc);
	BC_ASSERT_TRUE(linphone_core_get_call_history_size(marie->lc) == 0);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	unlink(logs_db);
	ms_free(logs_db);
}

#endif


static void call_with_http_proxy(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("pauline_rc");
	bool_t call_ok;
	LinphoneCall *marie_call;
	LinphoneAddress *contact_addr;
	struct addrinfo *res=NULL;
	struct addrinfo hints = {0};
	char ip[NI_MAXHOST];
	int err;

	if (!transport_supported(LinphoneTransportTls)) {
		ms_message("Test skipped because no tls support");
		goto end;
	}
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	err = getaddrinfo("sip.linphone.org","8888", &hints, &res);
	if (err !=0){
		ms_error("call_with_http_proxy(): getaddrinfo() error: %s", gai_strerror(err));
	}
	BC_ASSERT_PTR_NOT_NULL(res);
	if (!res) goto end;

	BC_ASSERT_EQUAL(err=getnameinfo(res->ai_addr, (socklen_t)res->ai_addrlen, ip, sizeof(ip)-1, NULL, 0, NI_NUMERICHOST), 0, int, "%i");
	if (err != 0){
		ms_error("call_with_http_proxy(): getnameinfo() error: %s", gai_strerror(err));
		goto end;
	}

	freeaddrinfo(res);

	linphone_core_set_http_proxy_host(pauline->lc,"sip.linphone.org");
	linphone_core_set_network_reachable(pauline->lc, FALSE); /*to make sure channel is restarted*/
	linphone_core_set_network_reachable(pauline->lc, TRUE);

	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));
	if (!call_ok) goto end;

	marie_call = linphone_core_get_current_call(marie->lc);
	contact_addr = linphone_address_new(linphone_call_get_remote_contact(marie_call));
	BC_ASSERT_STRING_EQUAL(linphone_address_get_domain(contact_addr),ip);
	linphone_address_destroy(contact_addr);
	end_call(marie, pauline);
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void _call_with_rtcp_mux(bool_t caller_rtcp_mux, bool_t callee_rtcp_mux, bool_t with_ice,bool_t with_ice_reinvite){
	LinphoneCoreManager * marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	const LinphoneCallParams *params;
	MSList *lcs = NULL;

	lcs = ms_list_append(lcs, marie->lc);
	lcs = ms_list_append(lcs, pauline->lc);

	if (caller_rtcp_mux){
		lp_config_set_int(linphone_core_get_config(marie->lc), "rtp", "rtcp_mux", 1);
	}
	if (callee_rtcp_mux){
		lp_config_set_int(linphone_core_get_config(pauline->lc), "rtp", "rtcp_mux", 1);
	}
	if (with_ice){
		linphone_core_set_user_agent(pauline->lc, "Natted Linphone", NULL);
		linphone_core_set_user_agent(marie->lc, "Natted Linphone", NULL);
		linphone_core_set_firewall_policy(marie->lc, LinphonePolicyUseIce);
		linphone_core_set_firewall_policy(pauline->lc, LinphonePolicyUseIce);
	}
	if (!with_ice_reinvite) {
		lp_config_set_int(linphone_core_get_config(pauline->lc), "sip", "update_call_when_ice_completed", 0);
		lp_config_set_int(linphone_core_get_config(marie->lc), "sip", "update_call_when_ice_completed", 0);
	}

	if (!BC_ASSERT_TRUE(call(marie,pauline))) goto end;

	params = linphone_call_get_remote_params(linphone_core_get_current_call(pauline->lc));
	BC_ASSERT_TRUE(caller_rtcp_mux == (linphone_call_params_get_custom_sdp_media_attribute(params, LinphoneStreamTypeAudio, "rtcp-mux") != NULL));
	if (caller_rtcp_mux){
		params = linphone_call_get_remote_params(linphone_core_get_current_call(marie->lc));
		BC_ASSERT_TRUE(callee_rtcp_mux == (linphone_call_params_get_custom_sdp_media_attribute(params, LinphoneStreamTypeAudio, "rtcp-mux") != NULL));
	}

	if (with_ice){
		check_ice(marie, pauline, LinphoneIceStateHostConnection);
	}
	liblinphone_tester_check_rtcp(marie,pauline);

	if (caller_rtcp_mux && callee_rtcp_mux){
		BC_ASSERT_EQUAL(marie->stat.number_of_rtcp_received_via_mux, marie->stat.number_of_rtcp_received, int, "%i");
		BC_ASSERT_GREATER(marie->stat.number_of_rtcp_received, 0, int, "%i");
		BC_ASSERT_EQUAL(pauline->stat.number_of_rtcp_received_via_mux, pauline->stat.number_of_rtcp_received, int, "%i");
		BC_ASSERT_GREATER(pauline->stat.number_of_rtcp_received, 0, int, "%i");
	}else{
		BC_ASSERT_TRUE(marie->stat.number_of_rtcp_received_via_mux == 0);
		BC_ASSERT_TRUE(pauline->stat.number_of_rtcp_received_via_mux == 0);
	}

	check_media_direction(pauline, linphone_core_get_current_call(pauline->lc), lcs, LinphoneMediaDirectionSendRecv, LinphoneMediaDirectionInvalid);
	end_call(marie,pauline);

end:
	ms_list_free(lcs);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
}

static void call_with_rtcp_mux(void){
	_call_with_rtcp_mux(TRUE, TRUE, FALSE,TRUE);
}

static void call_with_rtcp_mux_not_accepted(void){
	_call_with_rtcp_mux(TRUE, FALSE, FALSE,TRUE);
}

static void call_with_ice_and_rtcp_mux(void){
	_call_with_rtcp_mux(TRUE, TRUE, TRUE,TRUE);
}

static void call_with_ice_and_rtcp_mux_without_reinvite(void){
	_call_with_rtcp_mux(TRUE, TRUE, TRUE,FALSE);
}

static void call_with_zrtp_configured_calling_base(LinphoneCoreManager *marie, LinphoneCoreManager *pauline) {
	bool_t call_ok;

	linphone_core_set_media_encryption(marie->lc, LinphoneMediaEncryptionZRTP);
	BC_ASSERT_TRUE((call_ok=call(pauline,marie)));

	liblinphone_tester_check_rtcp(marie,pauline);

	BC_ASSERT_EQUAL(linphone_call_params_get_media_encryption(linphone_call_get_current_params(linphone_core_get_current_call(marie->lc)))
					, LinphoneMediaEncryptionNone, int, "%i");
	BC_ASSERT_EQUAL(linphone_call_params_get_media_encryption(linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc)))
					, LinphoneMediaEncryptionNone, int, "%i");
	end_call(pauline, marie);

}

/*
 * this test checks the 'dont_default_to_stun_candidates' mode, where the c= line is left to host
 * ip instead of stun candidate when ice is enabled*/
static void call_with_ice_with_default_candidate_not_stun(void){
	LinphoneCoreManager * marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	char localip[LINPHONE_IPADDR_SIZE];
	bool_t call_ok;

	lp_config_set_int(marie->lc->config, "net", "dont_default_to_stun_candidates", 1);
	linphone_core_set_firewall_policy(marie->lc, LinphonePolicyUseIce);
	linphone_core_set_firewall_policy(pauline->lc, LinphonePolicyUseIce);
	linphone_core_get_local_ip(marie->lc, AF_INET, NULL, localip);
	call_ok = call(marie, pauline);
	if (call_ok){
		check_ice(marie, pauline, LinphoneIceStateHostConnection);
		BC_ASSERT_STRING_EQUAL(marie->lc->current_call->localdesc->addr, localip);
		BC_ASSERT_STRING_EQUAL(pauline->lc->current_call->resultdesc->addr, localip);
		BC_ASSERT_STRING_EQUAL(marie->lc->current_call->localdesc->streams[0].rtp_addr, localip);
		BC_ASSERT_STRING_EQUAL(pauline->lc->current_call->resultdesc->streams[0].rtp_addr, "");
	}
	end_call(marie, pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_ice_without_stun(void){
	LinphoneCoreManager * marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	linphone_core_set_stun_server(marie->lc, NULL);
	linphone_core_set_stun_server(pauline->lc, NULL);
	_call_with_ice_base(marie, pauline, TRUE, TRUE, TRUE, FALSE);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_zrtp_configured_calling_side(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	call_with_zrtp_configured_calling_base(marie,pauline);

	linphone_core_set_user_agent(pauline->lc, "Natted Linphone", NULL);
	linphone_core_set_user_agent(marie->lc, "Natted Linphone", NULL);
	call_with_zrtp_configured_calling_base(marie,pauline);

	linphone_core_set_firewall_policy(marie->lc,LinphonePolicyUseIce);
	linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);
	call_with_zrtp_configured_calling_base(marie,pauline);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);


}
test_t call_tests[] = {
	TEST_NO_TAG("Early declined call", early_declined_call),
	TEST_NO_TAG("Call declined", call_declined),
	TEST_NO_TAG("Cancelled call", cancelled_call),
	TEST_NO_TAG("Early cancelled call", early_cancelled_call),
	TEST_NO_TAG("Call with DNS timeout", call_with_dns_time_out),
	TEST_NO_TAG("Cancelled ringing call", cancelled_ringing_call),
	TEST_NO_TAG("Call busy when calling self", call_busy_when_calling_self),
	TEST_NO_TAG("Simple call", simple_call),
	TEST_ONE_TAG("Call terminated automatically by linphone_core_destroy", automatic_call_termination, "LeaksMemory"),
	TEST_NO_TAG("Call with http proxy", call_with_http_proxy),
	TEST_NO_TAG("Call with timeouted bye", call_with_timeouted_bye),
	TEST_NO_TAG("Direct call over IPv6", direct_call_over_ipv6),
	TEST_NO_TAG("Outbound call with multiple proxy possible", call_outbound_with_multiple_proxy),
	TEST_NO_TAG("Audio call recording", audio_call_recording_test),
#if 0 /* not yet activated because not implemented */
	TEST_NO_TAG("Multiple answers to a call", multiple_answers_call),
#endif
	TEST_NO_TAG("Multiple answers to a call with media relay", multiple_answers_call_with_media_relay),
	TEST_NO_TAG("Call with media relay", call_with_media_relay),
	TEST_NO_TAG("Call with media relay (random ports)", call_with_media_relay_random_ports),
	TEST_NO_TAG("Simple call compatibility mode", simple_call_compatibility_mode),
	TEST_NO_TAG("Early-media call", early_media_call),
	TEST_ONE_TAG("Early-media call with ICE", early_media_call_with_ice, "ICE"),
	TEST_NO_TAG("Early-media call with ringing", early_media_call_with_ringing),
	TEST_NO_TAG("Early-media call with updated media session", early_media_call_with_session_update),
	TEST_NO_TAG("Early-media call with updated codec", early_media_call_with_codec_update),
	TEST_NO_TAG("Call terminated by caller", call_terminated_by_caller),
	TEST_NO_TAG("Call without SDP", call_with_no_sdp),
	TEST_NO_TAG("Call without SDP and ACK without SDP", call_with_no_sdp_ack_without_sdp),
	TEST_NO_TAG("Call paused resumed", call_paused_resumed),
#ifdef VIDEO_ENABLED
	TEST_NO_TAG("Call paused resumed with video", call_paused_resumed_with_video),
	TEST_NO_TAG("Call paused resumed with video no sdp ack", call_paused_resumed_with_no_sdp_ack),
	TEST_NO_TAG("Call paused resumed with video no sdk ack using video policy for resume offers", call_paused_resumed_with_no_sdp_ack_using_video_policy),
	TEST_NO_TAG("Call paused, updated and resumed with video no sdk ack using video policy for resume offers", call_paused_updated_resumed_with_no_sdp_ack_using_video_policy),
	TEST_NO_TAG("Call paused, updated and resumed with video no sdk ack using video policy for resume offers with accept call update", call_paused_updated_resumed_with_no_sdp_ack_using_video_policy_and_accept_call_update),
#endif
	TEST_NO_TAG("Call paused by both parties", call_paused_by_both),
	TEST_NO_TAG("Call paused resumed with loss", call_paused_resumed_with_loss),
	TEST_NO_TAG("Call paused resumed from callee", call_paused_resumed_from_callee),
	TEST_NO_TAG("SRTP call", srtp_call),
	TEST_NO_TAG("ZRTP call", zrtp_call),
	TEST_NO_TAG("ZRTP SAS call", zrtp_sas_call),
	TEST_NO_TAG("ZRTP Cipher call", zrtp_cipher_call),
	TEST_NO_TAG("DTLS SRTP call", dtls_srtp_call),
	TEST_NO_TAG("DTLS SRTP call with media relay", dtls_srtp_call_with_media_realy),
	TEST_NO_TAG("ZRTP video call", zrtp_video_call),
	TEST_NO_TAG("SRTP call with declined srtp", call_with_declined_srtp),
	TEST_NO_TAG("SRTP call paused and resumed", call_srtp_paused_and_resumed),
	TEST_NO_TAG("Call with file player", call_with_file_player),
	TEST_NO_TAG("Call with mkv file player", call_with_mkv_file_player),
	TEST_ONE_TAG("Audio call with ICE no matching audio codecs", audio_call_with_ice_no_matching_audio_codecs, "ICE"),
#ifdef VIDEO_ENABLED
	TEST_NO_TAG("Simple video call AVPF", video_call_avpf),
    TEST_NO_TAG("Simple video call implicit AVPF both", video_call_using_policy_AVPF_implicit_caller_and_callee),
    TEST_NO_TAG("Simple video call disable implicit AVPF on callee", video_call_disable_implicit_AVPF_on_callee),
    TEST_NO_TAG("Simple video call disable implicit AVPF on caller", video_call_disable_implicit_AVPF_on_caller),
    TEST_NO_TAG("Simple video call AVPF to implicit AVPF", video_call_AVPF_to_implicit_AVPF),
    TEST_NO_TAG("Simple video call implicit AVPF to AVPF", video_call_implicit_AVPF_to_AVPF),
    TEST_NO_TAG("Simple video call", video_call),
	TEST_NO_TAG("Simple video call without rtcp",video_call_without_rtcp),
	TEST_NO_TAG("Simple ZRTP video call", video_call_zrtp),
	TEST_NO_TAG("Simple DTLS video call", video_call_dtls),
	TEST_NO_TAG("Simple video call using policy", video_call_using_policy),
	TEST_NO_TAG("Video call using policy with callee video disabled", video_call_using_policy_with_callee_video_disabled),
	TEST_NO_TAG("Video call using policy with caller video disabled", video_call_using_policy_with_caller_video_disabled),
	TEST_NO_TAG("Video call without SDP", video_call_no_sdp),
	TEST_ONE_TAG("SRTP ice video call", srtp_video_ice_call, "ICE"),
	TEST_ONE_TAG("ZRTP ice video call", zrtp_video_ice_call, "ICE"),
	TEST_NO_TAG("Call with video added", call_with_video_added),
	TEST_NO_TAG("Call with video added 2", call_with_video_added_2),
	TEST_NO_TAG("Call with video added (random ports)", call_with_video_added_random_ports),
	TEST_NO_TAG("Call with several video switches", call_with_several_video_switches),
	TEST_NO_TAG("SRTP call with several video switches", srtp_call_with_several_video_switches),
	TEST_NO_TAG("Call with video declined", call_with_declined_video),
	TEST_NO_TAG("Call with video declined despite policy", call_with_declined_video_despite_policy),
	TEST_NO_TAG("Call with video declined using policy", call_with_declined_video_using_policy),
	TEST_NO_TAG("Video early-media call", video_early_media_call),
	TEST_NO_TAG("Call with multiple early media", multiple_early_media),
	TEST_ONE_TAG("Call with ICE from video to non-video", call_with_ice_video_to_novideo, "ICE"),
	TEST_ONE_TAG("Call with ICE and video added", call_with_ice_video_added, "ICE"),
	TEST_ONE_TAG("Call with ICE and video added 2", call_with_ice_video_added_2, "ICE"),
	TEST_ONE_TAG("Call with ICE and video added 3", call_with_ice_video_added_3, "ICE"),
	TEST_ONE_TAG("Call with ICE and video added and refused", call_with_ice_video_added_and_refused, "ICE"),
	TEST_ONE_TAG("Call with ICE and video added with video policies to false", call_with_ice_video_added_with_video_policies_to_false, "ICE"),
#if ICE_WAS_WORKING_WITH_REAL_TIME_TEXT
	TEST_ONE_TAG("Call with ICE, video and realtime text", call_with_ice_video_and_rtt, "ICE"),
#endif
	TEST_ONE_TAG("Video call with ICE accepted using call params", video_call_ice_params, "ICE"),
	TEST_NO_TAG("Audio call paused with caller video policy enabled", audio_call_with_video_policy_enabled),
	TEST_NO_TAG("Video call recording (H264)", video_call_recording_h264_test),
	TEST_NO_TAG("Video call recording (VP8)", video_call_recording_vp8_test),
	TEST_NO_TAG("Snapshot", video_call_snapshot),
	TEST_NO_TAG("Video call with early media and no matching audio codecs", video_call_with_early_media_no_matching_audio_codecs),
	TEST_NO_TAG("DTLS SRTP video call", dtls_srtp_video_call),
	TEST_ONE_TAG("DTLS SRTP ice video call", dtls_srtp_ice_video_call, "ICE"),
	TEST_ONE_TAG("DTLS SRTP ice video call with relay", dtls_srtp_ice_video_call_with_relay, "ICE"),
	TEST_NO_TAG("Video call with limited bandwidth", video_call_limited_bandwidth),
	TEST_NO_TAG("Video call accepted in send only", accept_call_in_send_only),
	TEST_ONE_TAG("Video call accepted in send only with ice", accept_call_in_send_only_with_ice, "ICE"),
	TEST_NO_TAG("2 Video call accepted in send only", two_accepted_call_in_send_only),
	TEST_NO_TAG("Video call with re-invite(inactive) followed by re-invite", video_call_with_re_invite_inactive_followed_by_re_invite),
	TEST_NO_TAG("Video call with re-invite(inactive) followed by re-invite(no sdp)", video_call_with_re_invite_inactive_followed_by_re_invite_no_sdp),
	TEST_NO_TAG("SRTP Video call with re-invite(inactive) followed by re-invite", srtp_video_call_with_re_invite_inactive_followed_by_re_invite),
	TEST_NO_TAG("SRTP Video call with re-invite(inactive) followed by re-invite(no sdp)", srtp_video_call_with_re_invite_inactive_followed_by_re_invite_no_sdp),
	TEST_NO_TAG("Classic video entry phone setup", classic_video_entry_phone_setup),
#endif
	TEST_ONE_TAG("SRTP ice call", srtp_ice_call, "ICE"),
	TEST_ONE_TAG("ZRTP ice call", zrtp_ice_call, "ICE"),
	TEST_ONE_TAG("ZRTP ice call with relay", zrtp_ice_call_with_relay, "ICE"),
	TEST_ONE_TAG("DTLS SRTP ice call", dtls_srtp_ice_call, "ICE"),
	TEST_ONE_TAG("DTLS ice call with relay", dtls_ice_call_with_relay, "ICE"),
	TEST_NO_TAG("Call with privacy", call_with_privacy),
	TEST_NO_TAG("Call with privacy 2", call_with_privacy2),
	TEST_NO_TAG("Call rejected because of wrong credential", call_rejected_because_wrong_credentials),
	TEST_NO_TAG("Call rejected without 403 because of wrong credential", call_rejected_without_403_because_wrong_credentials),
	TEST_NO_TAG("Call rejected without 403 because of wrong credential and no auth req cb", call_rejected_without_403_because_wrong_credentials_no_auth_req_cb),
	TEST_ONE_TAG("Call with ICE", call_with_ice, "ICE"),
	TEST_ONE_TAG("Call with ICE without SDP", call_with_ice_no_sdp, "ICE"),
	TEST_ONE_TAG("Call with ICE (random ports)", call_with_ice_random_ports, "ICE"),
	TEST_ONE_TAG("Call with ICE (forced relay)", call_with_ice_forced_relay, "ICE"),
	TEST_ONE_TAG("Call from ICE to not ICE", ice_to_not_ice, "ICE"),
	TEST_ONE_TAG("Call from not ICE to ICE", not_ice_to_ice, "ICE"),
	TEST_NO_TAG("Call with custom headers", call_with_custom_headers),
	TEST_NO_TAG("Call with custom SDP attributes", call_with_custom_sdp_attributes),
	TEST_NO_TAG("Call established with rejected INFO", call_established_with_rejected_info),
	TEST_NO_TAG("Call established with rejected RE-INVITE", call_established_with_rejected_reinvite),
	TEST_NO_TAG("Call established with rejected incoming RE-INVITE", call_established_with_rejected_incoming_reinvite),
	TEST_NO_TAG("Call established with rejected RE-INVITE in error", call_established_with_rejected_reinvite_with_error),
	TEST_NO_TAG("Call established with rejected RE-INVITE with trans pending error", call_established_with_rejected_reinvite_with_trans_pending_error),
	TEST_NO_TAG("Call established with complex rejected operation", call_established_with_complex_rejected_operation),
	TEST_NO_TAG("Call established with rejected info during re-invite", call_established_with_rejected_info_during_reinvite),
	TEST_NO_TAG("Call redirected by callee", call_redirect),
	TEST_NO_TAG("Call with specified codec bitrate", call_with_specified_codec_bitrate),
	TEST_NO_TAG("Call with no audio codec", call_with_no_audio_codec),
	TEST_NO_TAG("Video call with no audio and no video codec", video_call_with_no_audio_and_no_video_codec),
	TEST_NO_TAG("Call with in-dialog UPDATE request", call_with_in_dialog_update),
	TEST_NO_TAG("Call with in-dialog codec change", call_with_in_dialog_codec_change),
	TEST_NO_TAG("Call with in-dialog codec change no sdp", call_with_in_dialog_codec_change_no_sdp),
	TEST_NO_TAG("Call with pause no SDP on resume", call_with_paused_no_sdp_on_resume),
	TEST_NO_TAG("Call with early media and no SDP in 200 Ok", call_with_early_media_and_no_sdp_in_200),
	TEST_NO_TAG("Call with early media and no SDP in 200 Ok with video", call_with_early_media_and_no_sdp_in_200_with_video),
	TEST_ONE_TAG("Call with ICE and no SDP in 200 OK", call_with_early_media_ice_and_no_sdp_in_200, "ICE"),
	TEST_NO_TAG("Call with custom supported tags", call_with_custom_supported_tags),
	TEST_NO_TAG("Call log from taken from asserted id", call_log_from_taken_from_p_asserted_id),
	TEST_NO_TAG("Incoming INVITE with invalid SDP", incoming_invite_with_invalid_sdp),
	TEST_NO_TAG("Outgoing INVITE with invalid ACK SDP", outgoing_invite_with_invalid_sdp),
	TEST_NO_TAG("Incoming REINVITE with invalid SDP in ACK", incoming_reinvite_with_invalid_ack_sdp),
	TEST_NO_TAG("Outgoing REINVITE with invalid SDP in ACK", outgoing_reinvite_with_invalid_ack_sdp),
	TEST_NO_TAG("Call with generic CN", call_with_generic_cn),
	TEST_NO_TAG("Call with transport change after released", call_with_transport_change_after_released),
	TEST_NO_TAG("Unsuccessful call with transport change after released", unsucessfull_call_with_transport_change_after_released),
	TEST_NO_TAG("Simple stereo call with L16", simple_stereo_call_l16),
	TEST_NO_TAG("Simple stereo call with opus", simple_stereo_call_opus),
	TEST_NO_TAG("Simple mono call with opus", simple_mono_call_opus),
	TEST_NO_TAG("Call with FQDN in SDP", call_with_fqdn_in_sdp),
	TEST_NO_TAG("Call with RTP IO mode", call_with_rtp_io_mode),
	TEST_NO_TAG("Call with generic NACK RTCP feedback", call_with_generic_nack_rtcp_feedback),
	TEST_NO_TAG("Call with complex late offering", call_with_complex_late_offering),
#ifdef CALL_LOGS_STORAGE_ENABLED
	TEST_NO_TAG("Call log working if no db set", call_logs_if_no_db_set),
	TEST_NO_TAG("Call log storage migration from rc to db", call_logs_migrate),
	TEST_NO_TAG("Call log storage in sqlite database", call_logs_sqlite_storage),
#endif
	TEST_NO_TAG("Call with custom RTP Modifier", call_with_custom_rtp_modifier),
	TEST_NO_TAG("Call paused resumed with custom RTP Modifier", call_paused_resumed_with_custom_rtp_modifier),
	TEST_NO_TAG("Call record with custom RTP Modifier", call_record_with_custom_rtp_modifier),
	TEST_NO_TAG("Call with network switch", call_with_network_switch),
	TEST_NO_TAG("Call with network switch in early state 1", call_with_network_switch_in_early_state_1),
	TEST_NO_TAG("Call with network switch in early state 2", call_with_network_switch_in_early_state_2),
	TEST_ONE_TAG("Call with network switch and ICE", call_with_network_switch_and_ice, "ICE"),
	TEST_ONE_TAG("Call with network switch, ICE and RTT", call_with_network_switch_ice_and_rtt, "ICE"),
	TEST_NO_TAG("Call with network switch with socket refresh", call_with_network_switch_and_socket_refresh),
	TEST_NO_TAG("Call with SIP and RTP independant switches", call_with_sip_and_rtp_independant_switches),
	TEST_NO_TAG("Call with rtcp-mux", call_with_rtcp_mux),
	TEST_NO_TAG("Call with rtcp-mux not accepted", call_with_rtcp_mux_not_accepted),
	TEST_ONE_TAG("Call with ICE and rtcp-mux", call_with_ice_and_rtcp_mux, "ICE"),
	TEST_ONE_TAG("Call with ICE and rtcp-mux without ICE re-invite", call_with_ice_and_rtcp_mux_without_reinvite, "ICE"),
	TEST_ONE_TAG("Call with ICE with default candidate not stun", call_with_ice_with_default_candidate_not_stun, "ICE"),
	TEST_ONE_TAG("Call with ICE without stun server", call_with_ice_without_stun, "ICE"),
	TEST_NO_TAG("call with ZRTP configured calling side only", call_with_zrtp_configured_calling_side)
};

test_suite_t call_test_suite = {"Single Call", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
								sizeof(call_tests) / sizeof(call_tests[0]), call_tests};
