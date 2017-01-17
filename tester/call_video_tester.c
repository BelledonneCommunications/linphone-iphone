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

#include "linphone/core.h"
#include "liblinphone_tester.h"
#include "private.h"

#ifdef VIDEO_ENABLED
static void call_paused_resumed_with_video_base_call_cb(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *message) {
	if (cstate == LinphoneCallUpdatedByRemote) {
		LinphoneCallParams *params = linphone_core_create_call_params(lc, call);
		linphone_call_params_enable_video(params, TRUE);
		ms_message (" New state LinphoneCallUpdatedByRemote on call [%p], accepting with video on",call);
		BC_ASSERT_NOT_EQUAL(linphone_core_accept_call_update(lc, call, params), 0, int, "%i");
		linphone_call_params_unref(params);
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
	bctbx_list_t *lcs = NULL;
	LinphoneVideoPolicy vpol;
	bool_t call_ok;
	LinphoneCoreVTable *vtable = linphone_core_v_table_new();
	vtable->call_state_changed = call_paused_resumed_with_video_base_call_cb;
	lcs = bctbx_list_append(lcs, pauline->lc);
	lcs = bctbx_list_append(lcs, marie->lc);

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

	if (resume_in_audio_send_only_video_inactive_first) {
		LinphoneCallParams *params = linphone_core_create_call_params(pauline->lc, call_pauline);
		linphone_call_params_set_audio_direction(params,LinphoneMediaDirectionSendOnly);
		linphone_call_params_set_video_direction(params,LinphoneMediaDirectionInactive);
		linphone_core_update_call(pauline->lc, call_pauline, params);
		linphone_call_params_unref(params);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallUpdating,1));
	} else {
		linphone_core_pause_call(pauline->lc,call_pauline);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPausing,1));
	}
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,1));
	BC_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_remote_params(call_marie)));
	if (resume_in_audio_send_only_video_inactive_first) {
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	} else {
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPaused,1));
	}

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
		linphone_call_params_set_audio_direction(params,LinphoneMediaDirectionSendOnly);
		linphone_call_params_set_video_direction(params,LinphoneMediaDirectionInactive);
		linphone_core_update_call(pauline->lc,call_pauline,params);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,2));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallUpdating,1));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,3));
		linphone_call_params_set_audio_direction(params,LinphoneMediaDirectionSendRecv);
		linphone_call_params_set_video_direction(params,LinphoneMediaDirectionSendRecv);
		if (with_call_accept) {
			linphone_core_add_listener(marie->lc, vtable);
		}
		linphone_core_update_call(pauline->lc,call_pauline,params);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,4));
		linphone_call_params_unref(params);
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
	bctbx_list_free(lcs);
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

static void zrtp_video_call(void) {
	call_base(LinphoneMediaEncryptionZRTP,TRUE,FALSE,LinphonePolicyNoFirewall,FALSE);
}

static void call_state_changed_callback_to_accept_video(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState state, const char *message){
	LinphoneCoreVTable *vtable;
	if (state == LinphoneCallUpdatedByRemote){		
		LinphoneCallParams *params = linphone_core_create_call_params(lc, call);
		linphone_call_params_enable_video(params, TRUE);
		linphone_core_accept_call_update(lc, call, params);
		linphone_call_params_unref(params);
	}
	ms_message("video acceptance listener about to be dropped");
	vtable = belle_sip_object_data_get(BELLE_SIP_OBJECT(call),
						"call_state_changed_callback_to_accept_video");
	linphone_core_remove_listener(lc, vtable);
	belle_sip_object_data_set(BELLE_SIP_OBJECT(call), "call_state_changed_callback_to_accept_video", NULL, NULL);
}

static LinphoneCall* _request_video(LinphoneCoreManager* caller,LinphoneCoreManager* callee, bool_t accept_with_params) {
	LinphoneCallParams* callee_params;
	LinphoneCall* call_obj;

	if (!linphone_core_get_current_call(callee->lc) || linphone_call_get_state(linphone_core_get_current_call(callee->lc)) != LinphoneCallStreamsRunning
			|| !linphone_core_get_current_call(caller->lc) || linphone_call_get_state(linphone_core_get_current_call(caller->lc)) != LinphoneCallStreamsRunning ) {
		ms_warning("bad state for adding video");
		return NULL;
	}
	/*Assert the sanity of the developer, that is not expected to request video if video is already active.*/
	if (!BC_ASSERT_FALSE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(callee->lc))))){
		BC_FAIL("Video was requested while it was already active. This test doesn't look very sane.");
	}
	
	if (accept_with_params) {
		LinphoneCoreVTable *vtable = linphone_core_v_table_new();
		vtable->call_state_changed = call_state_changed_callback_to_accept_video;
		linphone_core_add_listener(caller->lc, vtable);
		belle_sip_object_data_set(BELLE_SIP_OBJECT(linphone_core_get_current_call(caller->lc)), "call_state_changed_callback_to_accept_video",
					  vtable, (void (*)(void*))linphone_core_v_table_destroy);
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
		linphone_call_params_unref(callee_params);
	}
	return call_obj;
}

/*
 * This function requests the addon of a video stream, initiated by "callee" and potentiall accepted by "caller",
 * and asserts a number of things after this is done.
 * However the video addon may fail due to video policy, so that there is no insurance that video is actually added.
 * This function returns TRUE if video was succesfully added, FALSE otherwise or if video is already there.
 **/
bool_t request_video(LinphoneCoreManager* caller,LinphoneCoreManager* callee, bool_t accept_with_params) {
	stats initial_caller_stat=caller->stat;
	stats initial_callee_stat=callee->stat;
	const LinphoneVideoPolicy *video_policy;
	LinphoneCall *call_obj;
	bool_t video_added = FALSE;
	
	if ((call_obj=_request_video(caller, callee, accept_with_params))){
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&caller->stat.number_of_LinphoneCallUpdatedByRemote,initial_caller_stat.number_of_LinphoneCallUpdatedByRemote+1));
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallUpdating,initial_callee_stat.number_of_LinphoneCallUpdating+1));
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_LinphoneCallStreamsRunning,initial_callee_stat.number_of_LinphoneCallStreamsRunning+1));
		BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&caller->stat.number_of_LinphoneCallStreamsRunning,initial_caller_stat.number_of_LinphoneCallStreamsRunning+1));

		video_policy = linphone_core_get_video_policy(caller->lc);
		if (video_policy->automatically_accept || accept_with_params) {
			video_added = BC_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(callee->lc))));
			video_added = 
			BC_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(caller->lc))))
			&& video_added;
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

		if (video_added) {
			linphone_call_set_next_video_frame_decoded_callback(call_obj,linphone_call_iframe_decoded_cb,callee->lc);
			/*send vfu*/
			linphone_call_send_vfu_request(call_obj);
			BC_ASSERT_TRUE(wait_for(caller->lc,callee->lc,&callee->stat.number_of_IframeDecoded,initial_callee_stat.number_of_IframeDecoded+1));
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
		
		if (!BC_ASSERT_TRUE(linphone_call_params_video_enabled(linphone_call_get_current_params(call_obj)))){
			BC_FAIL("Video was asked to be dropped while it was not active. This test doesn't look very sane.");
			return FALSE;
		}
		
		callee_params = linphone_core_create_call_params(callee->lc, call_obj);
		/* Remove video. */
		linphone_call_params_enable_video(callee_params, FALSE);
		linphone_core_update_call(callee->lc, call_obj, callee_params);
		linphone_call_params_unref(callee_params);

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

	BC_ASSERT_TRUE(request_video(pauline,marie, TRUE));

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

	BC_ASSERT_TRUE(request_video(marie,pauline, TRUE));

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

	BC_ASSERT_TRUE(request_video(pauline,marie, TRUE));
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

	BC_ASSERT_TRUE(request_video(pauline,marie, TRUE));
	wait_for_until(pauline->lc,marie->lc,&dummy,1,1000); /* Wait for VFU request exchanges to be finished. */
	BC_ASSERT_TRUE(remove_video(pauline,marie));
	BC_ASSERT_TRUE(request_video(pauline,marie, TRUE));
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

		BC_ASSERT_TRUE(request_video(pauline,marie, TRUE));
		wait_for_until(pauline->lc,marie->lc,&dummy,1,1000); /* Wait for VFU request exchanges to be finished. */
		BC_ASSERT_TRUE(remove_video(pauline,marie));
		BC_ASSERT_TRUE(request_video(pauline,marie, TRUE));
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

	linphone_call_params_unref(caller_test_params.base);
	if (callee_test_params.base) linphone_call_params_unref(callee_test_params.base);
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

	linphone_call_params_unref(caller_test_params.base);
	if (callee_test_params.base) linphone_call_params_unref(callee_test_params.base);
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

	linphone_call_params_unref(caller_test_params.base);
	if (callee_test_params.base) linphone_call_params_unref(callee_test_params.base);

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

	linphone_call_params_unref(caller_test_params.base);
	if (callee_test_params.base) linphone_call_params_unref(callee_test_params.base);

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
	if(BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(callee->lc))) {
		params = linphone_call_get_current_params(linphone_core_get_current_call(callee->lc));
		BC_ASSERT_STRING_EQUAL(linphone_call_params_get_rtp_profile(params), "RTP/AVP");
	}
	if(BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(caller->lc))) {
		params2 =linphone_call_get_current_params(linphone_core_get_current_call(caller->lc));
		BC_ASSERT_STRING_EQUAL(linphone_call_params_get_rtp_profile(params2), "RTP/AVP");
	}
	end_call(caller, callee);
	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);
}


static void video_call_disable_implicit_AVPF_on_caller(void) {
	LinphoneCoreManager *callee = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *caller = linphone_core_manager_new(transport_supported(LinphoneTransportTcp) ? "pauline_rc" : "pauline_tcp_rc");
	LpConfig   *caller_lp;
	const LinphoneCallParams *params, *params2;

	caller_lp = linphone_core_get_config(caller->lc);
	lp_config_set_int(caller_lp, "rtp", "rtcp_fb_implicit_rtcp_fb", 0);

	video_call_base_3(caller, callee, TRUE, LinphoneMediaEncryptionNone, TRUE, TRUE);
	params = linphone_call_get_current_params(linphone_core_get_current_call(callee->lc));
	BC_ASSERT_STRING_EQUAL(linphone_call_params_get_rtp_profile(params), "RTP/AVP");
	params2 = linphone_call_get_current_params(linphone_core_get_current_call(caller->lc));
	BC_ASSERT_STRING_EQUAL(linphone_call_params_get_rtp_profile(params2), "RTP/AVP");
	end_call(caller, callee);
	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);

}

static void video_call_AVPF_to_implicit_AVPF(void) {
	LinphoneCoreManager *callee = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *caller = linphone_core_manager_new(transport_supported(LinphoneTransportTcp) ? "pauline_rc" : "pauline_tcp_rc");

	linphone_core_set_avpf_mode(caller->lc, LinphoneAVPFEnabled);
	video_call_base_3(caller, callee, TRUE, LinphoneMediaEncryptionNone, TRUE, TRUE);
	end_call(caller, callee);

	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);

}

static void video_call_implicit_AVPF_to_AVPF(void) {
	LinphoneCoreManager *callee = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *caller = linphone_core_manager_new(transport_supported(LinphoneTransportTcp) ? "pauline_rc" : "pauline_tcp_rc");

	linphone_core_set_avpf_mode(callee->lc, LinphoneAVPFEnabled);
	video_call_base_3(caller, callee, TRUE, LinphoneMediaEncryptionNone, TRUE, TRUE);
	end_call(caller, callee);

	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);

}

static void video_call_using_policy_AVPF_implicit_caller_and_callee(void) {
	LinphoneCoreManager *callee = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *caller = linphone_core_manager_new(transport_supported(LinphoneTransportTcp) ? "pauline_rc" : "pauline_tcp_rc");
	video_call_base_3(caller, callee, FALSE, LinphoneMediaEncryptionNone, TRUE, TRUE);
	end_call(caller, callee);
	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);
}

static void video_call_base_avpf(LinphoneCoreManager *caller, LinphoneCoreManager *callee, bool_t using_policy, LinphoneMediaEncryption mode, bool_t callee_video_enabled, bool_t caller_video_enabled) {
	linphone_core_set_avpf_mode(caller->lc, LinphoneAVPFEnabled);
	linphone_core_set_avpf_mode(callee->lc, LinphoneAVPFEnabled);
	video_call_base_3(caller, callee, using_policy, mode, callee_video_enabled, caller_video_enabled);
	end_call(caller, callee);
}

static void video_call_avpf(void) {
	LinphoneCoreManager *callee = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *caller = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	video_call_base_avpf(caller, callee, FALSE, LinphoneMediaEncryptionNone, TRUE, TRUE);
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

/*
 * This function aims at testing ICE together with video enablement policies, and video enablements/disablements by either
 * caller or callee.
 * It doesn't use linphone_core_accept_call_with_params() to accept video despite of default policies.
 */
static void _call_with_ice_video(LinphoneVideoPolicy caller_policy, LinphoneVideoPolicy callee_policy,
	bool_t video_added_by_caller, bool_t video_added_by_callee, bool_t video_removed_by_caller, bool_t video_removed_by_callee, bool_t video_only) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	unsigned int nb_media_starts = 1;
	const LinphoneCallParams *marie_remote_params;
	const LinphoneCallParams *pauline_current_params;
	
	/*
	 * Pauline is the caller
	 * Marie is the callee
	 */

	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, TRUE);
	linphone_core_set_video_policy(pauline->lc, &caller_policy);
	linphone_core_set_video_policy(marie->lc, &callee_policy);
	linphone_core_set_firewall_policy(marie->lc, LinphonePolicyUseIce);
	linphone_core_set_firewall_policy(pauline->lc, LinphonePolicyUseIce);
	if (video_only) {
		linphone_core_enable_payload_type(marie->lc, linphone_core_find_payload_type(marie->lc, "PCMU", 8000, 1), FALSE); /* Disable PCMU */
		linphone_core_enable_payload_type(marie->lc, linphone_core_find_payload_type(marie->lc, "PCMA", 8000, 1), TRUE); /* Enable PCMA */
	}

	linphone_core_manager_wait_for_stun_resolution(marie);
	linphone_core_manager_wait_for_stun_resolution(pauline);

	/* This is to activate media relay on Flexisip server.
	 * Indeed, we want to test ICE with relay candidates as well, even though
	 * they will not be used at the end.*/
	linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);
	
	linphone_core_set_audio_port(marie->lc, -1);
	linphone_core_set_video_port(marie->lc, -1);
	linphone_core_set_audio_port(pauline->lc, -1);
	linphone_core_set_video_port(pauline->lc, -1);

	
	linphone_core_invite_address(pauline->lc, marie->identity);
	if (!BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneCallIncomingReceived, 1))) goto end;
	marie_remote_params = linphone_call_get_remote_params(linphone_core_get_current_call(marie->lc));
	BC_ASSERT_PTR_NOT_NULL(marie_remote_params);
	if (marie_remote_params){
		BC_ASSERT_TRUE(linphone_call_params_video_enabled(marie_remote_params) == caller_policy.automatically_initiate);
	}
	
	linphone_core_accept_call(marie->lc, linphone_core_get_current_call(marie->lc));
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 1)
		&& wait_for(pauline->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 1));
	
	pauline_current_params = linphone_call_get_current_params(linphone_core_get_current_call(pauline->lc));
	BC_ASSERT_PTR_NOT_NULL(pauline_current_params);
	if (pauline_current_params){
		BC_ASSERT_TRUE(linphone_call_params_video_enabled(pauline_current_params) == 
			(caller_policy.automatically_initiate && callee_policy.automatically_accept));
	}
	
	/* Wait for ICE reINVITEs to complete. */
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 2)
		&& wait_for(pauline->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 2));
	BC_ASSERT_TRUE(check_ice(pauline, marie, LinphoneIceStateHostConnection));
	BC_ASSERT_TRUE(check_nb_media_starts(pauline, marie, nb_media_starts, nb_media_starts));
	
	
	if (caller_policy.automatically_initiate && callee_policy.automatically_accept && (video_added_by_caller || video_added_by_callee)){
		BC_FAIL("Tired developer detected. You have requested the test to add video while it is already established from the beginning of the call.");
	}else{
		if (video_added_by_caller) {
			BC_ASSERT_TRUE(request_video(marie, pauline, FALSE) == callee_policy.automatically_accept);
		} else if (video_added_by_callee) {
			BC_ASSERT_TRUE(request_video(pauline, marie, FALSE) == caller_policy.automatically_accept);
		}
		if (video_added_by_caller || video_added_by_callee) {
			BC_ASSERT_TRUE(check_ice(pauline, marie, LinphoneIceStateHostConnection));
			if (linphone_call_params_video_enabled(linphone_call_get_current_params(linphone_core_get_current_call(marie->lc)))){
				/* Wait for ICE reINVITEs to complete if video was really added */
				BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneCallStreamsRunning, 4)
					&& wait_for(pauline->lc, pauline->lc, &marie->stat.number_of_LinphoneCallStreamsRunning, 4));
				/*the video addon should have triggered a media start, but the ICE reINVITE shall not*/
				nb_media_starts++;
				BC_ASSERT_TRUE(check_nb_media_starts(pauline, marie, nb_media_starts, nb_media_starts));
			}
		}
	}

	if (video_removed_by_caller) {
		BC_ASSERT_TRUE(remove_video(marie, pauline));
	} else if (video_removed_by_callee) {
		BC_ASSERT_TRUE(remove_video(pauline, marie));
	}
	if (video_removed_by_caller || video_removed_by_callee) {
		BC_ASSERT_TRUE(check_ice(pauline, marie, LinphoneIceStateHostConnection));
		nb_media_starts++;
		BC_ASSERT_TRUE(check_nb_media_starts(pauline, marie, nb_media_starts, nb_media_starts));
		
	}

	end_call(pauline, marie);

end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_ice_video_added(void) {
	/*
	 * Scenario: video is not active at the beginning of the call, caller requests it but callee declines it
	 */
	LinphoneVideoPolicy vpol = { FALSE, FALSE };
	_call_with_ice_video(vpol, vpol, TRUE, FALSE, FALSE, FALSE, FALSE);
}

static void call_with_ice_video_added_2(void) {
	LinphoneVideoPolicy vpol = {FALSE, FALSE};
	/*
	 * Scenario: video is not active at the beginning of the call, callee requests it but caller declines it
	 */
	_call_with_ice_video(vpol, vpol, FALSE, TRUE, FALSE, FALSE, FALSE);
}

static void call_with_ice_video_added_3(void) {
	LinphoneVideoPolicy caller_policy = { FALSE, FALSE };
	LinphoneVideoPolicy callee_policy = { TRUE, TRUE };
	/*
	 * Scenario: video is not active at the beginning of the call, caller requests it and callee accepts.
	 * Finally caller removes it.
	 */
	_call_with_ice_video(caller_policy, callee_policy, TRUE, FALSE, TRUE, FALSE, FALSE);
}

static void call_with_ice_video_added_4(void) {
	LinphoneVideoPolicy caller_policy = { TRUE, TRUE };
	LinphoneVideoPolicy callee_policy = { FALSE, FALSE };
	/*
	 * Scenario: video is not active at the beginning of the call, callee requests it and caller accepts.
	 * Finally caller removes it.
	 */
	_call_with_ice_video(caller_policy, callee_policy, FALSE, TRUE, TRUE, FALSE, FALSE);
}

static void call_with_ice_video_added_5(void) {
	LinphoneVideoPolicy caller_policy = { TRUE, TRUE };
	LinphoneVideoPolicy callee_policy = { FALSE, FALSE };
	/*
	 * Scenario: video is not active at the beginning of the call, callee requests it and caller accepts.
	 * Finally callee removes it.
	 */
	_call_with_ice_video(caller_policy, callee_policy, FALSE, TRUE, FALSE, TRUE, FALSE);
}

static void call_with_ice_video_added_6(void) {
	LinphoneVideoPolicy caller_policy = { TRUE, TRUE };
	LinphoneVideoPolicy callee_policy = { TRUE, TRUE };
	/*
	 * Scenario: video is active at the beginning of the call, caller removes it.
	 */
	_call_with_ice_video(caller_policy, callee_policy, FALSE, FALSE, TRUE, FALSE, FALSE);
}

static void call_with_ice_video_added_7(void) {
	LinphoneVideoPolicy caller_policy = { TRUE, TRUE };
	LinphoneVideoPolicy callee_policy = { TRUE, TRUE };
	/*
	 * Scenario: video is active at the beginning of the call, callee removes it.
	 */
	_call_with_ice_video(caller_policy, callee_policy, FALSE, FALSE, FALSE, TRUE, FALSE);
}

static void call_with_ice_video_and_rtt(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	bool_t call_ok;
	LinphoneVideoPolicy policy;
	LinphoneCallParams *params = NULL;
	LinphoneCall *marie_call = NULL;

	policy.automatically_initiate = policy.automatically_accept = TRUE;
	linphone_core_set_video_policy(pauline->lc, &policy);
	linphone_core_set_video_policy(marie->lc, &policy);
	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, FALSE);
	linphone_core_enable_video_capture(pauline->lc, FALSE);
	linphone_core_enable_video_display(pauline->lc, TRUE);
	linphone_core_set_firewall_policy(marie->lc, LinphonePolicyUseIce);
	linphone_core_set_firewall_policy(pauline->lc, LinphonePolicyUseIce);

	linphone_core_manager_wait_for_stun_resolution(marie);
	linphone_core_manager_wait_for_stun_resolution(pauline);

	linphone_core_set_audio_port(marie->lc, -1);
	linphone_core_set_video_port(marie->lc, -1);
	linphone_core_set_text_port(marie->lc, -1);
	linphone_core_set_audio_port(pauline->lc, -1);
	linphone_core_set_video_port(pauline->lc, -1);
	linphone_core_set_text_port(pauline->lc, -1);

	params = linphone_core_create_call_params(pauline->lc, NULL);
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
	linphone_call_params_unref(params);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_with_ice_video_only(void) {
	LinphoneVideoPolicy caller_policy = { TRUE, TRUE };
	LinphoneVideoPolicy callee_policy = { TRUE, TRUE };
	/*
	 * Scenario: video is active at the beginning of the call, but no audio codecs match.
	 */
	_call_with_ice_video(caller_policy, callee_policy, FALSE, FALSE, FALSE, FALSE, TRUE);
}

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

static void dtls_srtp_video_call(void) {
	call_base(LinphoneMediaEncryptionDTLS,TRUE,FALSE,LinphonePolicyNoFirewall,FALSE);
}

static void dtls_srtp_ice_video_call(void) {
	call_base(LinphoneMediaEncryptionDTLS,TRUE,FALSE,LinphonePolicyUseIce,FALSE);
}
static void dtls_srtp_ice_video_call_with_relay(void) {
	call_base(LinphoneMediaEncryptionDTLS,TRUE,TRUE,LinphonePolicyUseIce,FALSE);
}
static void srtp_video_ice_call(void) {
	call_base(LinphoneMediaEncryptionSRTP,TRUE,FALSE,LinphonePolicyUseIce,FALSE);
}
static void zrtp_video_ice_call(void) {
	call_base(LinphoneMediaEncryptionZRTP,TRUE,FALSE,LinphonePolicyUseIce,FALSE);
}

static void accept_call_in_send_only_base(LinphoneCoreManager* pauline, LinphoneCoreManager *marie, bctbx_list_t *lcs) {
#define DEFAULT_WAIT_FOR 10000
	LinphoneCallParams *params;
	LinphoneVideoPolicy pol;
	LinphoneCall *call;
	pol.automatically_accept=1;
	pol.automatically_initiate=1;

	// important: VP8 has really poor performances with the mire camera, at least
	// on iOS - so when ever h264 is available, let's use it instead
	if (linphone_core_find_payload_type(pauline->lc,"h264", -1, -1)!=NULL) {
		disable_all_video_codecs_except_one(pauline->lc,"h264");
		disable_all_video_codecs_except_one(marie->lc,"h264");
	}

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
		linphone_call_params_unref(params);

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
	bctbx_list_t *lcs=NULL;;

	marie = linphone_core_manager_new("marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	if (caller_has_ice) {
		linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);
	}

	lcs=bctbx_list_append(lcs,pauline->lc);
	lcs=bctbx_list_append(lcs,marie->lc);

	accept_call_in_send_only_base(pauline,marie,lcs);


	end_call(marie,pauline);
	bctbx_list_free(lcs);
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
	bctbx_list_t *lcs=NULL;

	marie = linphone_core_manager_new("marie_rc");
	linphone_core_use_files(marie->lc, TRUE);
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	laure = linphone_core_manager_new("laure_rc_udp");

	lcs=bctbx_list_append(lcs,pauline->lc);
	lcs=bctbx_list_append(lcs,marie->lc);
	lcs=bctbx_list_append(lcs,laure->lc);

	accept_call_in_send_only_base(pauline,marie,lcs);

	reset_counters(&marie->stat);
	accept_call_in_send_only_base(laure,marie,lcs);

	end_call(pauline, marie);
	end_call(laure, marie);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	bctbx_list_free(lcs);
}

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
	bctbx_list_t *lcs=NULL;
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

	lcs=bctbx_list_append(lcs,marie1->lc);
	lcs=bctbx_list_append(lcs,marie2->lc);
	lcs=bctbx_list_append(lcs,pauline->lc);

	linphone_call_params_enable_early_media_sending(params,TRUE);
	linphone_call_params_enable_video(params,TRUE);

	linphone_core_invite_address_with_params(pauline->lc,marie1->identity,params);
	linphone_call_params_unref(params);

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

	bctbx_list_free(lcs);
	linphone_core_manager_destroy(marie1);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(pauline);
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

static void audio_call_with_ice_with_video_policy_enabled(void){
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
	linphone_call_params_unref(params);*/

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
	LinphoneVideoPolicy vpol;
	bctbx_list_t *lcs = NULL;
	int retry = 0;
	bool_t ok;

	vpol.automatically_initiate = vpol.automatically_accept = TRUE;
	lcs = bctbx_list_append(lcs, caller_mgr->lc);
	lcs = bctbx_list_append(lcs, callee_mgr->lc);

	linphone_core_enable_video_capture(caller_mgr->lc, TRUE);
	linphone_core_enable_video_display(caller_mgr->lc, TRUE);
	linphone_core_enable_video_capture(callee_mgr->lc, TRUE);
	linphone_core_enable_video_display(callee_mgr->lc, TRUE);
	linphone_core_set_avpf_mode(caller_mgr->lc, LinphoneAVPFEnabled);
	linphone_core_set_avpf_mode(callee_mgr->lc, LinphoneAVPFEnabled);
	linphone_core_set_video_policy(caller_mgr->lc, &vpol);
	linphone_core_set_video_policy(callee_mgr->lc, &vpol);

	// important: VP8 has really poor performances with the mire camera, at least
	// on iOS - so when ever h264 is available, let's use it instead
	if (linphone_core_find_payload_type(caller_mgr->lc,"h264", -1, -1)!=NULL) {
		disable_all_video_codecs_except_one(caller_mgr->lc,"h264");
		disable_all_video_codecs_except_one(callee_mgr->lc,"h264");
	}

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
	linphone_call_params_unref(early_media_params);

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
	linphone_call_params_unref(in_call_params);

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
	linphone_call_params_unref(in_call_params);

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
	bctbx_list_free(lcs);
}
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
	char *filename = bc_tester_file("snapshot.jpeg");
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

static void video_call_with_re_invite_inactive_followed_by_re_invite_base(LinphoneMediaEncryption mode, bool_t no_sdp) {
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCallParams *params;
	const LinphoneCallParams *current_params;
	bctbx_list_t *lcs=NULL;
	bool_t calls_ok;

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	linphone_core_set_avpf_mode(pauline->lc,TRUE);

	// important: VP8 has really poor performances with the mire camera, at least
	// on iOS - so when ever h264 is available, let's use it instead
	if (linphone_core_find_payload_type(pauline->lc,"h264", -1, -1)!=NULL) {
		disable_all_video_codecs_except_one(pauline->lc,"h264");
		disable_all_video_codecs_except_one(marie->lc,"h264");
	}
	linphone_core_set_video_device(pauline->lc,liblinphone_tester_mire_id);
	linphone_core_set_video_device(marie->lc,liblinphone_tester_mire_id);
	linphone_core_set_avpf_mode(marie->lc,TRUE);
	lcs=bctbx_list_append(lcs,pauline->lc);
	lcs=bctbx_list_append(lcs,marie->lc);

	video_call_base_2(marie,pauline,TRUE,mode,TRUE,TRUE);

	calls_ok = linphone_core_get_current_call(marie->lc) != NULL && linphone_core_get_current_call(pauline->lc) != NULL;
	BC_ASSERT_TRUE(calls_ok);

	if (calls_ok) {
		params=linphone_core_create_call_params(marie->lc,linphone_core_get_current_call(marie->lc));
		linphone_call_params_set_audio_direction(params,LinphoneMediaDirectionInactive);
		linphone_call_params_set_video_direction(params,LinphoneMediaDirectionInactive);

		linphone_core_update_call(marie->lc, linphone_core_get_current_call(marie->lc),params);
		linphone_call_params_unref(params);

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
		linphone_call_params_unref(params);

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

static void incoming_reinvite_with_invalid_ack_sdp(void){
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
		BC_ASSERT_PTR_NOT_NULL(_request_video(caller, callee, TRUE));
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
}

static void outgoing_reinvite_with_invalid_ack_sdp(void)  {
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
		BC_ASSERT_PTR_NOT_NULL(_request_video(caller, callee, TRUE));
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
}

#endif
static void video_call_with_no_audio_and_no_video_codec(void){
	LinphoneCoreManager* callee = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* caller = linphone_core_manager_new(transport_supported(LinphoneTransportTcp) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCall* out_call ;
	LinphoneVideoPolicy callee_policy, caller_policy;
	LinphoneCallTestParams caller_test_params = {0}, callee_test_params = {0};

	const bctbx_list_t* elem_video =linphone_core_get_video_codecs(caller->lc);

	const bctbx_list_t* elem_audio =linphone_core_get_audio_codecs(caller->lc);

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

	linphone_call_params_unref(caller_test_params.base);
	if (callee_test_params.base) linphone_call_params_unref(callee_test_params.base);


	BC_ASSERT_TRUE(wait_for(caller->lc, callee->lc, &caller->stat.number_of_LinphoneCallOutgoingInit, 1));

	BC_ASSERT_TRUE(wait_for_until(caller->lc, callee->lc, &caller->stat.number_of_LinphoneCallError, 1, 6000));
	BC_ASSERT_EQUAL(linphone_call_get_reason(out_call), LinphoneReasonNotAcceptable, int, "%d");
	BC_ASSERT_EQUAL(callee->stat.number_of_LinphoneCallIncomingReceived, 0, int, "%d");

	linphone_call_unref(out_call);
	linphone_core_manager_destroy(callee);
	linphone_core_manager_destroy(caller);
}

static void call_with_early_media_and_no_sdp_in_200_with_video(void){
	early_media_without_sdp_in_200_base(TRUE, FALSE);
}

test_t call_video_tests[] = {
#ifdef VIDEO_ENABLED
	TEST_NO_TAG("Call paused resumed with video", call_paused_resumed_with_video),
	TEST_NO_TAG("Call paused resumed with video no sdp ack", call_paused_resumed_with_no_sdp_ack),
	TEST_NO_TAG("Call paused resumed with video no sdk ack using video policy for resume offers", call_paused_resumed_with_no_sdp_ack_using_video_policy),
	TEST_NO_TAG("Call paused, updated and resumed with video no sdk ack using video policy for resume offers", call_paused_updated_resumed_with_no_sdp_ack_using_video_policy),
	TEST_NO_TAG("Call paused, updated and resumed with video no sdk ack using video policy for resume offers with accept call update", call_paused_updated_resumed_with_no_sdp_ack_using_video_policy_and_accept_call_update),
	TEST_NO_TAG("ZRTP video call", zrtp_video_call),
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
	TEST_ONE_TAG("Call with ICE and video added 4", call_with_ice_video_added_4, "ICE"),
	TEST_ONE_TAG("Call with ICE and video added 5", call_with_ice_video_added_5, "ICE"),
	TEST_ONE_TAG("Call with ICE and video added 6", call_with_ice_video_added_6, "ICE"),
	TEST_ONE_TAG("Call with ICE and video added 7", call_with_ice_video_added_7, "ICE"),
	TEST_ONE_TAG("Call with ICE, video and realtime text", call_with_ice_video_and_rtt, "ICE"),
	TEST_ONE_TAG("Call with ICE, video only", call_with_ice_video_only, "ICE"),
	TEST_ONE_TAG("Video call with ICE accepted using call params", video_call_ice_params, "ICE"),
	TEST_ONE_TAG("Audio call with ICE paused with caller video policy enabled", audio_call_with_ice_with_video_policy_enabled, "ICE"),
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
	TEST_NO_TAG("Incoming REINVITE with invalid SDP in ACK", incoming_reinvite_with_invalid_ack_sdp),
	TEST_NO_TAG("Outgoing REINVITE with invalid SDP in ACK", outgoing_reinvite_with_invalid_ack_sdp),
#endif
	TEST_NO_TAG("Video call with no audio and no video codec", video_call_with_no_audio_and_no_video_codec),
	TEST_NO_TAG("Call with early media and no SDP in 200 Ok with video", call_with_early_media_and_no_sdp_in_200_with_video),
};

test_suite_t call_video_test_suite = {"Video Call", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
								sizeof(call_video_tests) / sizeof(call_video_tests[0]), call_video_tests};
