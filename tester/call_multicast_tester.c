/*
	liblinphone_tester - liblinphone test suite
	Copyright (C) 2014  Belledonne Communications SARL

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


#include "liblinphone_tester.h"
#include "linphone/core.h"
#include "belle-sip/belle-sip.h"


static void call_multicast_base(bool_t video) {
	LinphoneCoreManager *marie, *pauline;
	LinphoneVideoPolicy marie_policy, pauline_policy;

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new( "pauline_tcp_rc");

	if (video) {
		linphone_core_enable_video_capture(marie->lc, TRUE);
		linphone_core_enable_video_display(marie->lc, TRUE);
		linphone_core_enable_video_capture(pauline->lc, TRUE);
		linphone_core_enable_video_display(pauline->lc, FALSE);

		marie_policy.automatically_initiate=TRUE;
		marie_policy.automatically_accept=TRUE;
		pauline_policy.automatically_initiate=TRUE;
		pauline_policy.automatically_accept=TRUE;

		linphone_core_set_video_policy(marie->lc,&marie_policy);
		linphone_core_set_video_policy(pauline->lc,&pauline_policy);
		linphone_core_set_video_multicast_addr(pauline->lc,"224.1.2.3");
		linphone_core_enable_video_multicast(pauline->lc,TRUE);
	}
	linphone_core_set_audio_multicast_addr(pauline->lc,"224.1.2.3");
	linphone_core_enable_audio_multicast(pauline->lc,TRUE);

	BC_ASSERT_TRUE(call(pauline,marie));
	wait_for_until(marie->lc, pauline->lc, NULL, 1, 6000);
	if (linphone_core_get_current_call(marie->lc)) {
		BC_ASSERT_GREATER(linphone_core_manager_get_max_audio_down_bw(marie),70,int,"%d");
		if (video) {
			/*check video path*/
			linphone_call_set_next_video_frame_decoded_callback(linphone_core_get_current_call(marie->lc),linphone_call_iframe_decoded_cb,marie->lc);
			linphone_call_send_vfu_request(linphone_core_get_current_call(marie->lc));
			BC_ASSERT_TRUE( wait_for(marie->lc,pauline->lc,&marie->stat.number_of_IframeDecoded,1));
		}

		end_call(marie,pauline);
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_multicast(void)  {
	call_multicast_base(FALSE);
}
static void multicast_audio_with_pause_resume(void) {
	call_paused_resumed_base(TRUE,FALSE);
}
#ifdef VIDEO_ENABLED
static void call_multicast_video(void)  {
	call_multicast_base(TRUE);
}
#endif
static void early_media_with_multicast_base(bool_t video) {
	LinphoneCoreManager *marie, *pauline, *pauline2;
	bctbx_list_t* lcs = NULL;
	int dummy=0;
	LinphoneVideoPolicy marie_policy, pauline_policy;
	LpConfig *marie_lp;
	LinphoneCallParams *params;

	marie   = linphone_core_manager_new("marie_rc");
	pauline = linphone_core_manager_new("pauline_tcp_rc");
	pauline2 = linphone_core_manager_new("pauline_tcp_rc");

	marie_lp=linphone_core_get_config(marie->lc);
	lp_config_set_int(marie_lp,"misc","real_early_media",1);

	if (video) {
		linphone_core_enable_video_capture(pauline->lc, FALSE);
		linphone_core_enable_video_display(pauline->lc, TRUE);
		linphone_core_enable_video_capture(pauline2->lc, FALSE);
		linphone_core_enable_video_display(pauline2->lc, TRUE);
		linphone_core_enable_video_capture(marie->lc, TRUE);
		linphone_core_enable_video_display(marie->lc, FALSE);

		// important: VP8 has really poor performances with the mire camera, at least
		// on iOS - so when ever h264 is available, let's use it instead
		if (linphone_core_find_payload_type(pauline->lc,"h264", -1, -1)!=NULL) {
			disable_all_video_codecs_except_one(pauline->lc,"h264");
			disable_all_video_codecs_except_one(pauline2->lc,"h264");
			disable_all_video_codecs_except_one(marie->lc,"h264");
		}

		linphone_core_set_video_device(pauline->lc, liblinphone_tester_mire_id);
		linphone_core_set_video_device(pauline2->lc, liblinphone_tester_mire_id);
		linphone_core_set_video_device(marie->lc, liblinphone_tester_mire_id);

		linphone_core_set_avpf_mode(pauline->lc, LinphoneAVPFEnabled);
		linphone_core_set_avpf_mode(pauline2->lc, LinphoneAVPFEnabled);
		linphone_core_set_avpf_mode(marie->lc, LinphoneAVPFEnabled);

		marie_policy.automatically_initiate=TRUE;
		marie_policy.automatically_accept=TRUE;
		pauline_policy.automatically_initiate=TRUE;
		pauline_policy.automatically_accept=TRUE;

		linphone_core_set_video_policy(marie->lc,&marie_policy);
		linphone_core_set_video_policy(pauline->lc,&pauline_policy);
		linphone_core_set_video_policy(pauline2->lc,&pauline_policy);
		linphone_core_set_video_multicast_addr(marie->lc,"224.1.2.3");
		linphone_core_enable_video_multicast(marie->lc,TRUE);
	}
	linphone_core_set_audio_multicast_addr(marie->lc,"224.1.2.3");
	linphone_core_enable_audio_multicast(marie->lc,TRUE);


	lcs = bctbx_list_append(lcs,marie->lc);
	lcs = bctbx_list_append(lcs,pauline->lc);
	lcs = bctbx_list_append(lcs,pauline2->lc);
	/*
		Marie calls Pauline, and after the call has rung, transitions to an early_media session
	*/

	linphone_core_invite_address(marie->lc, pauline->identity);

	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingReceived,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingRinging,1,1000));


	if (linphone_core_inc_invite_pending(pauline->lc)) {
		/* send a 183 to initiate the early media */
		if (video) {
			/*check video path*/
			linphone_call_set_next_video_frame_decoded_callback(linphone_core_get_current_call(pauline->lc),linphone_call_iframe_decoded_cb,pauline->lc);
		}
		linphone_core_accept_early_media(pauline->lc, linphone_core_get_current_call(pauline->lc));

		BC_ASSERT_TRUE( wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallIncomingEarlyMedia,1,2000) );
		BC_ASSERT_TRUE( wait_for_list(lcs, &marie->stat.number_of_LinphoneCallOutgoingEarlyMedia,1,2000) );

		if (linphone_core_inc_invite_pending(pauline2->lc)) {
				/* send a 183 to initiate the early media */
				if (video) {
					/*check video path*/
					linphone_call_set_next_video_frame_decoded_callback(linphone_core_get_current_call(pauline2->lc),linphone_call_iframe_decoded_cb,pauline2->lc);
				}
				linphone_core_accept_early_media(pauline2->lc, linphone_core_get_current_call(pauline2->lc));

				BC_ASSERT_TRUE( wait_for_list(lcs, &pauline2->stat.number_of_LinphoneCallIncomingEarlyMedia,1,2000) );
		}

		wait_for_list(lcs, &dummy, 1, 3000);

		BC_ASSERT_GREATER(linphone_core_manager_get_max_audio_down_bw(pauline),70,int,"%i");
		BC_ASSERT_LOWER((int)linphone_call_get_audio_stats(linphone_core_get_current_call(pauline->lc))->download_bandwidth, 90, int, "%i");

		BC_ASSERT_GREATER(linphone_core_manager_get_max_audio_down_bw(pauline2),70,int,"%i");
		BC_ASSERT_LOWER((int)linphone_call_get_audio_stats(linphone_core_get_current_call(pauline2->lc))->download_bandwidth,90, int, "%i");

		BC_ASSERT_TRUE(linphone_call_params_audio_multicast_enabled(linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc))));
		BC_ASSERT_TRUE(linphone_call_params_audio_multicast_enabled(linphone_call_get_current_params(linphone_core_get_current_call(marie->lc))));
		if (video) {
			BC_ASSERT_TRUE(linphone_call_params_video_multicast_enabled(linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc))));
			BC_ASSERT_TRUE(linphone_call_params_video_multicast_enabled(linphone_call_get_current_params(linphone_core_get_current_call(marie->lc))));
		}

		if (video) {
			BC_ASSERT_TRUE( wait_for_list(lcs,&pauline->stat.number_of_IframeDecoded,1,2000));
			BC_ASSERT_TRUE( wait_for_list(lcs,&pauline2->stat.number_of_IframeDecoded,1,2000));
		}

		linphone_core_accept_call(pauline->lc, linphone_core_get_current_call(pauline->lc));

		BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallConnected, 1,1000));
		BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallStreamsRunning, 1,1000));
		BC_ASSERT_TRUE(wait_for_list(lcs, &pauline2->stat.number_of_LinphoneCallEnd, 1,1000));

		BC_ASSERT_TRUE(linphone_call_params_audio_multicast_enabled(linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc))));
		BC_ASSERT_TRUE(linphone_call_params_audio_multicast_enabled(linphone_call_get_current_params(linphone_core_get_current_call(marie->lc))));
		if (video) {
			BC_ASSERT_TRUE(linphone_call_params_video_multicast_enabled(linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc))));
			BC_ASSERT_TRUE(linphone_call_params_video_multicast_enabled(linphone_call_get_current_params(linphone_core_get_current_call(marie->lc))));
		}
		params = linphone_core_create_call_params(pauline->lc, linphone_core_get_current_call(pauline->lc));

		linphone_call_params_enable_audio_multicast(params,FALSE);
		linphone_call_params_enable_video_multicast(params,FALSE);
		linphone_core_enable_video_capture(pauline->lc, TRUE);
		linphone_core_enable_video_display(pauline->lc, TRUE);
		linphone_core_enable_video_capture(marie->lc, TRUE);
		linphone_core_enable_video_display(marie->lc, TRUE);

		linphone_core_update_call(	pauline->lc
									, linphone_core_get_current_call(pauline->lc)
									, params);
		linphone_call_params_unref(params);

		BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallStreamsRunning, 2,1000));

		BC_ASSERT_FALSE(linphone_call_params_audio_multicast_enabled(linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc))));
		BC_ASSERT_FALSE(linphone_call_params_audio_multicast_enabled(linphone_call_get_current_params(linphone_core_get_current_call(marie->lc))));

		check_media_direction(	pauline
								, linphone_core_get_current_call(pauline->lc)
								, lcs
								,LinphoneMediaDirectionSendRecv
								, video?LinphoneMediaDirectionSendRecv:LinphoneMediaDirectionInactive);
		check_media_direction(	marie
								, linphone_core_get_current_call(marie->lc)
								, lcs
								,LinphoneMediaDirectionSendRecv
								, video?LinphoneMediaDirectionSendRecv:LinphoneMediaDirectionInactive);

		if (video) {
			BC_ASSERT_FALSE(linphone_call_params_video_multicast_enabled(linphone_call_get_current_params(linphone_core_get_current_call(marie->lc))));
			BC_ASSERT_FALSE(linphone_call_params_video_multicast_enabled(linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc))));
		}
		end_call(marie,pauline);
	}
	ms_free(lcs);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(pauline2);
}

static void early_media_with_multicast_audio(void) {
	early_media_with_multicast_base(FALSE);
}
static void unicast_incoming_with_multicast_audio_on(void) {
	simple_call_base(TRUE);
}
#ifdef VIDEO_ENABLED
static void early_media_with_multicast_video(void) {
	early_media_with_multicast_base(TRUE);
}
#endif

test_t multicast_call_tests[] = {
	TEST_NO_TAG("Multicast audio call",call_multicast),
	TEST_NO_TAG("Multicast call with pause/resume",multicast_audio_with_pause_resume),
	TEST_NO_TAG("Early media multicast audio call",early_media_with_multicast_audio),
	TEST_NO_TAG("Unicast incoming call with multicast activated",unicast_incoming_with_multicast_audio_on),
#ifdef VIDEO_ENABLED
	TEST_NO_TAG("Multicast video call",call_multicast_video),
	TEST_NO_TAG("Early media multicast video call",early_media_with_multicast_video),
#endif
};

test_suite_t multicast_call_test_suite = {"Multicast Call", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
										  sizeof(multicast_call_tests) / sizeof(multicast_call_tests[0]),
										  multicast_call_tests};
