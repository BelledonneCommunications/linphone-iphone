/*
	belle-sip - SIP (RFC3261) library.
	Copyright (C) 2014  Belledonne Communications SARL

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

#include "linphone/core.h"
#include "private.h"
#include "liblinphone_tester.h"
#include "mediastreamer2/stun.h"
#include "ortp/port.h"


static const char *stun_address = "stun.linphone.org";


static size_t test_stun_encode(char **buffer)
{
	MSStunMessage *req = ms_stun_binding_request_create();
	UInt96 tr_id = ms_stun_message_get_tr_id(req);
	tr_id.octet[0] = 11;
	ms_stun_message_set_tr_id(req, tr_id);
	return ms_stun_message_encode(req, buffer);
}

static void linphone_stun_test_encode(void)
{
	char *buffer = NULL;
	size_t len = test_stun_encode(&buffer);
	BC_ASSERT(len > 0);
	BC_ASSERT_PTR_NOT_NULL(buffer);
	if (buffer != NULL) ms_free(buffer);
	ms_message("STUN message encoded in %i bytes", (int)len);
}

static void linphone_stun_test_grab_ip(void)
{
	
	LinphoneCoreManager* lc_stun = linphone_core_manager_new2("stun_rc", FALSE);
	LinphoneCall dummy_call;
	int ping_time;
	int tmp = 0;

	/*this test verifies the very basic STUN support of liblinphone, which is deprecated.
	 * It works only in IPv4 mode and there is no plan to make it work over ipv6.*/
	if (liblinphone_tester_ipv4_available()){
		goto end;
	}
	linphone_core_enable_ipv6(lc_stun->lc, FALSE);
	
	memset(&dummy_call, 0, sizeof(LinphoneCall));
	dummy_call.main_audio_stream_index = 0;
	dummy_call.main_video_stream_index = 1;
	dummy_call.main_text_stream_index = 2;
	dummy_call.media_ports[dummy_call.main_audio_stream_index].rtp_port = 7078;
	dummy_call.media_ports[dummy_call.main_video_stream_index].rtp_port = 9078;
	dummy_call.media_ports[dummy_call.main_text_stream_index].rtp_port = 11078;

	linphone_core_set_stun_server(lc_stun->lc, stun_address);
	BC_ASSERT_STRING_EQUAL(stun_address, linphone_core_get_stun_server(lc_stun->lc));

	wait_for(lc_stun->lc, lc_stun->lc, &tmp, 1);

	ping_time = linphone_core_run_stun_tests(lc_stun->lc, &dummy_call);
	BC_ASSERT(ping_time != -1);

	ms_message("Round trip to STUN: %d ms", ping_time);

	BC_ASSERT(dummy_call.ac.addr[0] != '\0');
	BC_ASSERT(dummy_call.ac.port != 0);
#ifdef VIDEO_ENABLED
	BC_ASSERT(dummy_call.vc.addr[0] != '\0');
	BC_ASSERT(dummy_call.vc.port != 0);
#endif
	BC_ASSERT(dummy_call.tc.addr[0] != '\0');
	BC_ASSERT(dummy_call.tc.port != 0);

	ms_message("STUN test result: local audio port maps to %s:%i", dummy_call.ac.addr, dummy_call.ac.port);
#ifdef VIDEO_ENABLED
	ms_message("STUN test result: local video port maps to %s:%i", dummy_call.vc.addr, dummy_call.vc.port);
#endif
	ms_message("STUN test result: local text port maps to %s:%i", dummy_call.tc.addr, dummy_call.tc.port);

end:
	linphone_core_manager_destroy(lc_stun);
}

static void configure_nat_policy(LinphoneCore *lc, bool_t turn_enabled) {
	const char *username = "liblinphone-tester";
	const char *password = "retset-enohpnilbil";
	LinphoneAuthInfo *auth_info = linphone_core_create_auth_info(lc, username, NULL, password, NULL, "sip.linphone.org", NULL);
	LinphoneNatPolicy *nat_policy = linphone_core_create_nat_policy(lc);
	linphone_nat_policy_enable_ice(nat_policy, TRUE);
	if (turn_enabled) {
		linphone_nat_policy_enable_turn(nat_policy, TRUE);
		linphone_nat_policy_set_stun_server(nat_policy, "sip1.linphone.org:3479");
		linphone_nat_policy_set_stun_server_username(nat_policy, username);
	} else {
		linphone_nat_policy_enable_stun(nat_policy, TRUE);
		linphone_nat_policy_set_stun_server(nat_policy, "stun.linphone.org");
	}
	linphone_core_set_nat_policy(lc, nat_policy);
	linphone_core_add_auth_info(lc, auth_info);
	linphone_nat_policy_unref(nat_policy);
	linphone_auth_info_destroy(auth_info);
}

static void check_turn_context_statistics(MSTurnContext *turn_context, bool_t forced_relay) {
	BC_ASSERT_TRUE(turn_context->stats.nb_successful_allocate > 1);
	if (forced_relay == TRUE) {
		BC_ASSERT_TRUE(turn_context->stats.nb_send_indication > 0);
		BC_ASSERT_TRUE(turn_context->stats.nb_data_indication > 0);
		BC_ASSERT_TRUE(turn_context->stats.nb_received_channel_msg > 0);
		BC_ASSERT_TRUE(turn_context->stats.nb_sent_channel_msg > 0);
		BC_ASSERT_TRUE(turn_context->stats.nb_successful_refresh > 0);
		BC_ASSERT_TRUE(turn_context->stats.nb_successful_create_permission > 1);
		BC_ASSERT_TRUE(turn_context->stats.nb_successful_channel_bind > 1);
	}
}

static void ice_turn_call_base(bool_t video_enabled, bool_t forced_relay, bool_t caller_turn_enabled, bool_t callee_turn_enabled, bool_t rtcp_mux_enabled, bool_t ipv6) {
	LinphoneCoreManager *marie;
	LinphoneCoreManager *pauline;
	LinphoneCall *lcall;
	LinphoneIceState expected_ice_state = LinphoneIceStateHostConnection;
	LinphoneMediaDirection expected_video_dir = LinphoneMediaDirectionInactive;
	bctbx_list_t *lcs = NULL;

	marie = linphone_core_manager_new2("marie_rc", FALSE);
	lcs = bctbx_list_append(lcs, marie->lc);
	pauline = linphone_core_manager_new2(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc", FALSE);
	lcs = bctbx_list_append(lcs, pauline->lc);

	if (ipv6) {
		linphone_core_enable_ipv6(marie->lc, TRUE);
		linphone_core_enable_ipv6(pauline->lc, TRUE);
	} else {
		linphone_core_enable_ipv6(marie->lc, FALSE);
		linphone_core_enable_ipv6(pauline->lc, FALSE);
	}

	configure_nat_policy(marie->lc, caller_turn_enabled);
	configure_nat_policy(pauline->lc, callee_turn_enabled);
	if (forced_relay == TRUE) {
		linphone_core_enable_forced_ice_relay(marie->lc, TRUE);
		linphone_core_enable_forced_ice_relay(pauline->lc, TRUE);
		linphone_core_enable_short_turn_refresh(marie->lc, TRUE);
		linphone_core_enable_short_turn_refresh(pauline->lc, TRUE);
		expected_ice_state = LinphoneIceStateRelayConnection;
	}
	if (rtcp_mux_enabled == TRUE) {
		lp_config_set_int(linphone_core_get_config(marie->lc), "rtp", "rtcp_mux", 1);
		lp_config_set_int(linphone_core_get_config(pauline->lc), "rtp", "rtcp_mux", 1);
	}

	linphone_core_manager_start(marie, TRUE);
	linphone_core_manager_start(pauline, TRUE);

	if (video_enabled) {
#ifdef VIDEO_ENABLED
		linphone_core_set_video_device(pauline->lc,liblinphone_tester_mire_id);
		linphone_core_set_video_device(marie->lc,liblinphone_tester_mire_id);
		video_call_base_2(marie, pauline, FALSE, LinphoneMediaEncryptionNone, TRUE, TRUE);
		expected_video_dir = LinphoneMediaDirectionSendRecv;
#endif
	} else {
		BC_ASSERT_TRUE(call(marie, pauline));
	}

	/* Wait for the ICE reINVITE to complete */
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 2));
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 2));
	BC_ASSERT_TRUE(check_ice(pauline, marie, expected_ice_state));
	check_nb_media_starts(pauline, marie, 1, 1);
	check_media_direction(marie, linphone_core_get_current_call(marie->lc), lcs, LinphoneMediaDirectionSendRecv, expected_video_dir);
	check_media_direction(pauline, linphone_core_get_current_call(pauline->lc), lcs, LinphoneMediaDirectionSendRecv, expected_video_dir);
	liblinphone_tester_check_rtcp(marie, pauline);
	lcall = linphone_core_get_current_call(marie->lc);
	BC_ASSERT_PTR_NOT_NULL(lcall);
	if (lcall != NULL) {
		BC_ASSERT_PTR_NOT_NULL(lcall->ice_session);
		if (lcall->ice_session != NULL) {
			IceCheckList *cl = ice_session_check_list(lcall->ice_session, 0);
			BC_ASSERT_PTR_NOT_NULL(cl);
			if (cl != NULL) {
				check_turn_context_statistics(cl->rtp_turn_context, forced_relay);
				if (!rtcp_mux_enabled) check_turn_context_statistics(cl->rtcp_turn_context, forced_relay);
			}
		}
	}

	end_call(marie, pauline);

	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	bctbx_list_free(lcs);
}

static void basic_ice_turn_call(void) {
	ice_turn_call_base(FALSE, FALSE, TRUE, TRUE, FALSE, FALSE);
}

static void basic_ipv6_ice_turn_call(void) {
	if (liblinphone_tester_ipv6_available()) {
		ice_turn_call_base(FALSE, FALSE, TRUE, TRUE, FALSE, TRUE);
	} else {
		ms_warning("Test skipped, no ipv6 available");
	}
}

#ifdef VIDEO_ENABLED
static void video_ice_turn_call(void) {
	ice_turn_call_base(TRUE, FALSE, TRUE, TRUE, FALSE, FALSE);
}
#endif

static void relayed_ice_turn_call(void) {
	ice_turn_call_base(FALSE, TRUE, TRUE, TRUE, FALSE, FALSE);
}

#ifdef VIDEO_ENABLED
static void relayed_video_ice_turn_call(void) {
	ice_turn_call_base(TRUE, TRUE, TRUE, TRUE, FALSE, FALSE);
}
#endif

static void relayed_ice_turn_call_with_rtcp_mux(void) {
	ice_turn_call_base(FALSE, TRUE, TRUE, TRUE, TRUE, FALSE);
}

static void relayed_ice_turn_to_ice_stun_call(void) {
	ice_turn_call_base(FALSE, TRUE, TRUE, FALSE, FALSE, FALSE);
}


test_t stun_tests[] = {
	TEST_ONE_TAG("Basic Stun test (Ping/public IP)", linphone_stun_test_grab_ip, "STUN"),
	TEST_ONE_TAG("STUN encode", linphone_stun_test_encode, "STUN"),
	TEST_TWO_TAGS("Basic ICE+TURN call", basic_ice_turn_call, "ICE", "TURN"),
	TEST_TWO_TAGS("Basic IPv6 ICE+TURN call", basic_ipv6_ice_turn_call, "ICE", "TURN"),
#ifdef VIDEO_ENABLED
	TEST_TWO_TAGS("Video ICE+TURN call", video_ice_turn_call, "ICE", "TURN"),
	TEST_TWO_TAGS("Relayed video ICE+TURN call", relayed_video_ice_turn_call, "ICE", "TURN"),
#endif
	TEST_TWO_TAGS("Relayed ICE+TURN call", relayed_ice_turn_call, "ICE", "TURN"),
	TEST_TWO_TAGS("Relayed ICE+TURN call with rtcp-mux", relayed_ice_turn_call_with_rtcp_mux, "ICE", "TURN"),
	TEST_TWO_TAGS("Relayed ICE+TURN to ICE+STUN call", relayed_ice_turn_to_ice_stun_call, "ICE", "TURN")
};

test_suite_t stun_test_suite = {"Stun", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
								sizeof(stun_tests) / sizeof(stun_tests[0]), stun_tests};
