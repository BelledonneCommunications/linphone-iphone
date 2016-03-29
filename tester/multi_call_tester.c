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
#endif


static void call_waiting_indication_with_param(bool_t enable_caller_privacy) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	MSList *iterator;
	MSList* lcs;
	LinphoneCall* pauline_called_by_marie;
	LinphoneCall* pauline_called_by_laure=NULL;
	LinphoneCallParams *laure_params=linphone_core_create_call_params(laure->lc, NULL);
	LinphoneCallParams *marie_params=linphone_core_create_call_params(marie->lc, NULL);

	if (enable_caller_privacy)
		linphone_call_params_set_privacy(marie_params,LinphonePrivacyId);

	lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);

	BC_ASSERT_TRUE(call_with_caller_params(marie,pauline,marie_params));
	linphone_call_params_destroy(marie_params);
	pauline_called_by_marie=linphone_core_get_current_call(pauline->lc);

	if (enable_caller_privacy)
		linphone_call_params_set_privacy(laure_params,LinphonePrivacyId);

	BC_ASSERT_PTR_NOT_NULL(linphone_core_invite_address_with_params(laure->lc,pauline->identity,laure_params));
	linphone_call_params_destroy(laure_params);

	BC_ASSERT_TRUE(wait_for(laure->lc
							,pauline->lc
							,&pauline->stat.number_of_LinphoneCallIncomingReceived
							,2));

	BC_ASSERT_EQUAL(laure->stat.number_of_LinphoneCallOutgoingProgress,1, int, "%d");


	BC_ASSERT_TRUE(wait_for(laure->lc
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

	BC_ASSERT_TRUE(wait_for(laure->lc
							,pauline->lc
							,&laure->stat.number_of_LinphoneCallConnected
							,1));

	BC_ASSERT_TRUE(wait_for(pauline->lc
								,marie->lc
								,&marie->stat.number_of_LinphoneCallPausedByRemote
								,1));

	if (pauline_called_by_laure && enable_caller_privacy )
		BC_ASSERT_EQUAL(linphone_call_params_get_privacy(linphone_call_get_current_params(pauline_called_by_laure)),LinphonePrivacyId, int, "%d");
	/*wait a bit for ACK to be sent*/
	wait_for_list(lcs,NULL,0,1000);
	linphone_core_terminate_all_calls(pauline->lc);

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,1,10000));

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

static void second_call_rejection(bool_t second_without_audio){
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCall *pauline_call;
	LinphoneCallParams *params;
	LinphoneCall *marie_call;

	/*start a call to pauline*/
	linphone_core_invite_address(marie->lc, pauline->identity);
	BC_ASSERT_TRUE(wait_for(marie->lc
			,pauline->lc
			,&marie->stat.number_of_LinphoneCallOutgoingRinging
			,1));

	/*attempt to send a second call while the first one is not answered.
	 * It must be rejected by the core, since the audio resources are already engaged for the first call*/
	params = linphone_core_create_call_params(marie->lc, NULL);
	linphone_call_params_enable_audio(params, !second_without_audio);
	marie_call = linphone_core_invite_with_params(marie->lc, "sip:laure_non_exstent@test.linphone.org", params);

	linphone_call_params_destroy(params);

	if (second_without_audio){
		BC_ASSERT_PTR_NOT_NULL(marie_call);
		BC_ASSERT_TRUE(wait_for(marie->lc
			,pauline->lc
			,&marie->stat.number_of_LinphoneCallError
			,1));

	}else{
		BC_ASSERT_PTR_NULL(marie_call);
	}

	pauline_call = linphone_core_get_current_call(pauline->lc);
	BC_ASSERT_PTR_NOT_NULL(pauline_call);
	if (pauline_call){
		linphone_core_accept_call(pauline->lc, pauline_call);
	}
	BC_ASSERT_TRUE(wait_for(marie->lc
			,pauline->lc
			,&marie->stat.number_of_LinphoneCallStreamsRunning
			,1));
	BC_ASSERT_TRUE(wait_for(marie->lc
			,pauline->lc
			,&pauline->stat.number_of_LinphoneCallStreamsRunning
			,1));
	end_call(pauline, marie);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void second_call_rejected_if_first_one_in_progress(void){
	second_call_rejection(FALSE);
}

static void second_call_allowed_if_not_using_audio(void){
	second_call_rejection(TRUE);
}

static void incoming_call_accepted_when_outgoing_call_in_state(LinphoneCallState state) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	MSList* lcs;
	LinphoneCallParams *laure_params=linphone_core_create_call_params(laure->lc, NULL);
	LinphoneCallParams *marie_params=linphone_core_create_call_params(marie->lc, NULL);

	lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);


	if (state==LinphoneCallOutgoingRinging || state==LinphoneCallOutgoingEarlyMedia) {
		BC_ASSERT_PTR_NOT_NULL(linphone_core_invite_address_with_params(marie->lc,pauline->identity,marie_params));

		BC_ASSERT_TRUE(wait_for(marie->lc
								,pauline->lc
								,&pauline->stat.number_of_LinphoneCallIncomingReceived
								,1));

		if (state==LinphoneCallOutgoingEarlyMedia)
			linphone_core_accept_early_media(pauline->lc,linphone_core_get_current_call(pauline->lc));

		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallOutgoingProgress,1, int, "%d");
		BC_ASSERT_TRUE(wait_for(marie->lc
									,pauline->lc
									,state==LinphoneCallOutgoingEarlyMedia?&marie->stat.number_of_LinphoneCallOutgoingEarlyMedia:&marie->stat.number_of_LinphoneCallOutgoingRinging
									,1));
	} else if (state==LinphoneCallOutgoingProgress) {
		BC_ASSERT_PTR_NOT_NULL(linphone_core_invite_address(marie->lc,pauline->identity));
	} else {
		ms_error("Unsupported state");
		return;
	}

	BC_ASSERT_TRUE(call_with_caller_params(laure,marie,laure_params));


	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,10000));


	linphone_core_terminate_all_calls(marie->lc);

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,1,10000));


	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	ms_list_free(lcs);
}
static void incoming_call_accepted_when_outgoing_call_in_progress(void) {
	incoming_call_accepted_when_outgoing_call_in_state(LinphoneCallOutgoingProgress);
}
static void incoming_call_accepted_when_outgoing_call_in_outgoing_ringing(void) {
	incoming_call_accepted_when_outgoing_call_in_state(LinphoneCallOutgoingRinging);
}
static void incoming_call_accepted_when_outgoing_call_in_outgoing_ringing_early_media(void) {
	incoming_call_accepted_when_outgoing_call_in_state(LinphoneCallOutgoingEarlyMedia);
}

static void simple_conference_base(LinphoneCoreManager* marie, LinphoneCoreManager* pauline, LinphoneCoreManager* laure, LinphoneCoreManager *focus) {
	stats initial_marie_stat;
	stats initial_pauline_stat;
	stats initial_laure_stat;

	LinphoneCall* marie_call_pauline;
	LinphoneCall* pauline_called_by_marie;
	LinphoneCall* marie_call_laure;
	LinphoneConference *conference;
	const MSList* calls;
	bool_t is_remote_conf;
	bool_t focus_is_up = (focus && ((LinphoneConferenceServer *)focus)->reg_state == LinphoneRegistrationOk);
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);
	if(focus) lcs=ms_list_append(lcs,focus->lc);

	is_remote_conf = (strcmp(lp_config_get_string(marie->lc->config, "misc", "conference_type", "local"), "remote") == 0);
	if(is_remote_conf) BC_ASSERT_PTR_NOT_NULL(focus);

	BC_ASSERT_TRUE(call(marie,pauline));
	marie_call_pauline=linphone_core_get_current_call(marie->lc);
	pauline_called_by_marie=linphone_core_get_current_call(pauline->lc);
	BC_ASSERT_TRUE(pause_call_1(marie,marie_call_pauline,pauline,pauline_called_by_marie));

	BC_ASSERT_TRUE(call(marie,laure));
	initial_marie_stat=marie->stat;
	initial_pauline_stat=pauline->stat;
	initial_laure_stat=laure->stat;

	marie_call_laure=linphone_core_get_current_call(marie->lc);

	BC_ASSERT_PTR_NOT_NULL_FATAL(marie_call_laure);
	linphone_core_add_to_conference(marie->lc,marie_call_laure);
	if(!is_remote_conf) {
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallUpdating,initial_marie_stat.number_of_LinphoneCallUpdating+1,5000));
	} else {
		if(focus_is_up) {
			BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,initial_marie_stat.number_of_LinphoneCallStreamsRunning+1,5000));
			BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransferCallConnected,initial_marie_stat.number_of_LinphoneTransferCallConnected+1,5000));
			BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,initial_marie_stat.number_of_LinphoneCallEnd+1,5000));
		} else {
			BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallError, initial_marie_stat.number_of_LinphoneCallError+1, 5000));
			BC_ASSERT_PTR_NULL(linphone_core_get_conference(marie->lc));
			BC_ASSERT_EQUAL(linphone_core_terminate_conference(marie->lc), -1, int, "%d");
			linphone_core_terminate_call(marie->lc, marie_call_pauline);
			linphone_core_terminate_call(marie->lc, marie_call_laure);
			BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallEnd, initial_marie_stat.number_of_LinphoneCallEnd+2, 10000));
			BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallEnd, initial_pauline_stat.number_of_LinphoneCallEnd+1, 5000));
			BC_ASSERT_TRUE(wait_for_list(lcs, &laure->stat.number_of_LinphoneCallEnd, initial_laure_stat.number_of_LinphoneCallEnd+1, 5000));
			goto end;
		}
	}

	linphone_core_add_to_conference(marie->lc,marie_call_pauline);

	if(!is_remote_conf) {
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallResuming,initial_marie_stat.number_of_LinphoneCallResuming+1,2000));
	} else {
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransferCallConnected,initial_marie_stat.number_of_LinphoneTransferCallConnected+2,5000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,initial_marie_stat.number_of_LinphoneCallEnd+2,5000));
	}

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,initial_pauline_stat.number_of_LinphoneCallStreamsRunning+1,5000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,initial_laure_stat.number_of_LinphoneCallStreamsRunning+1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,initial_marie_stat.number_of_LinphoneCallStreamsRunning+2,3000));

	BC_ASSERT_TRUE(linphone_core_is_in_conference(marie->lc));
	BC_ASSERT_EQUAL(linphone_core_get_conference_size(marie->lc),3, int, "%d");

	if(!is_remote_conf) {
		BC_ASSERT_PTR_NULL(linphone_core_get_current_call(marie->lc));
	} else {
		BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(marie->lc));
	}
	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(pauline->lc));
	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(laure->lc));


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
	for (calls=linphone_core_get_calls(marie->lc);calls!=NULL;calls=calls->next) {
		LinphoneCall *call=(LinphoneCall *)calls->data;
		BC_ASSERT_EQUAL(linphone_core_get_media_encryption(marie->lc),linphone_call_params_get_media_encryption(linphone_call_get_current_params(call)),int,"%d");
	}

	BC_ASSERT_PTR_NOT_NULL(conference = linphone_core_get_conference(marie->lc));
	if(conference) {
		MSList *participants = linphone_conference_get_participants(conference);
		BC_ASSERT_EQUAL(ms_list_size(participants), 2, int, "%d");
		ms_list_free_with_data(participants, (void(*)(void *))linphone_address_destroy);
	}

	linphone_core_terminate_conference(marie->lc);
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,is_remote_conf?2:1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,is_remote_conf?3:2,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,is_remote_conf?2:1,10000));
	if(is_remote_conf) BC_ASSERT_TRUE(wait_for_list(lcs,&focus->stat.number_of_LinphoneCallEnd,3,10000));
	
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallReleased,is_remote_conf?2:1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallReleased,is_remote_conf?3:2,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallReleased,is_remote_conf?2:1,10000));
	if(is_remote_conf) BC_ASSERT_TRUE(wait_for_list(lcs,&focus->stat.number_of_LinphoneCallReleased,3,10000));

end:
	ms_list_free(lcs);
}

static void simple_conference(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	simple_conference_base(marie,pauline,laure, NULL);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
}

static void simple_encrypted_conference_with_ice(LinphoneMediaEncryption mode) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");

	if (linphone_core_media_encryption_supported(marie->lc,mode)) {
		linphone_core_set_firewall_policy(marie->lc,LinphonePolicyUseIce);
		linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);
		linphone_core_set_firewall_policy(laure->lc,LinphonePolicyUseIce);

		/**/

		linphone_core_set_media_encryption(marie->lc,mode);
		linphone_core_set_media_encryption(pauline->lc,mode);
		linphone_core_set_media_encryption(laure->lc,mode);

		simple_conference_base(marie,pauline,laure,NULL);
	} else {
		ms_warning("No [%s] support available",linphone_media_encryption_to_string(mode));
		BC_PASS("Passed");
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
}

static void simple_conference_with_ice(void) {
	simple_encrypted_conference_with_ice(LinphoneMediaEncryptionNone);
}
static void simple_zrtp_conference_with_ice(void) {
	simple_encrypted_conference_with_ice(LinphoneMediaEncryptionZRTP);
}


static void simple_call_transfer(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	LinphoneCall* pauline_called_by_marie;
	LinphoneCall *marie_calling_pauline;
	LinphoneCall *marie_calling_laure;

	char* laure_identity=linphone_address_as_string(laure->identity);
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);


	BC_ASSERT_TRUE(call(marie,pauline));
	marie_calling_pauline=linphone_core_get_current_call(marie->lc);
	pauline_called_by_marie=linphone_core_get_current_call(pauline->lc);

	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);
	reset_counters(&laure->stat);


	linphone_core_transfer_call(pauline->lc,pauline_called_by_marie,laure_identity);
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallRefered,1,2000));
	/*marie pausing pauline*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallPausing,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallPausedByRemote,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallPaused,1,2000));
	/*marie calling laure*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallOutgoingProgress,1,2000));

	BC_ASSERT_PTR_NOT_NULL(linphone_call_get_transfer_target_call(marie_calling_pauline));

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneTransferCallOutgoingInit,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallIncomingReceived,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallOutgoingRinging,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneTransferCallOutgoingProgress,1,2000));
	linphone_core_accept_call(laure->lc,linphone_core_get_current_call(laure->lc));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallConnected,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallConnected,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,1,2000));

	marie_calling_laure=linphone_core_get_current_call(marie->lc);
	BC_ASSERT_PTR_NOT_NULL_FATAL(marie_calling_laure);
	BC_ASSERT_PTR_EQUAL(linphone_call_get_transferer_call(marie_calling_laure),marie_calling_pauline);

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneTransferCallConnected,1,2000));

	/*terminate marie to pauline call*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallReleased,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallReleased,1,2000));

	end_call(marie, laure);
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallReleased,1,2000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	ms_list_free(lcs);
}

static void unattended_call_transfer(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	LinphoneCall* pauline_called_by_marie;

	char* laure_identity=linphone_address_as_string(laure->identity);
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);


	BC_ASSERT_TRUE(call(marie,pauline));
	pauline_called_by_marie=linphone_core_get_current_call(marie->lc);

	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);
	reset_counters(&laure->stat);

	linphone_core_transfer_call(marie->lc,pauline_called_by_marie,laure_identity);
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallRefered,1,2000));

	/*marie ends the call  */
	linphone_core_terminate_call(marie->lc,pauline_called_by_marie);
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));

	/*Pauline starts the transfer*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingInit,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingProgress,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallIncomingReceived,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,2000));
	linphone_core_accept_call(laure->lc,linphone_core_get_current_call(laure->lc));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallConnected,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,2000));

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));

	end_call(laure, pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	ms_list_free(lcs);
}

static void unattended_call_transfer_with_error(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCall* pauline_called_by_marie;
	bool_t call_ok=TRUE;
	MSList* lcs=ms_list_append(NULL,marie->lc);

	lcs=ms_list_append(lcs,pauline->lc);

	BC_ASSERT_TRUE((call_ok=call(marie,pauline)));
	if (call_ok){
		pauline_called_by_marie=linphone_core_get_current_call(marie->lc);

		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);

		linphone_core_transfer_call(marie->lc,pauline_called_by_marie,"unknown_user");
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallRefered,1,2000));

		/*Pauline starts the transfer*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingInit,1,2000));
		/* and immediately get an error*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallError,1,2000));

		/*the error must be reported back to marie*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransferCallError,1,2000));

		/*and pauline should resume the call automatically*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallResuming,1,2000));

		/*and call should be resumed*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,1,2000));

		end_call(marie, pauline);
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	ms_list_free(lcs);
}


static void call_transfer_existing_call_outgoing_call(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	LinphoneCall* marie_call_pauline;
	LinphoneCall* pauline_called_by_marie;
	LinphoneCall* marie_call_laure;
	LinphoneCall* laure_called_by_marie;
	LinphoneCall* lcall;
	bool_t call_ok=TRUE;
	const MSList* calls;
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);

	/*marie call pauline*/
	BC_ASSERT_TRUE((call_ok=call(marie,pauline)));
	if (call_ok){
		marie_call_pauline=linphone_core_get_current_call(marie->lc);
		pauline_called_by_marie=linphone_core_get_current_call(pauline->lc);
		/*marie pause pauline*/
		BC_ASSERT_TRUE(pause_call_1(marie,marie_call_pauline,pauline,pauline_called_by_marie));

		/*marie call laure*/
		BC_ASSERT_TRUE(call(marie,laure));
		marie_call_laure=linphone_core_get_current_call(marie->lc);
		laure_called_by_marie=linphone_core_get_current_call(laure->lc);
		/*marie pause laure*/
		BC_ASSERT_TRUE(pause_call_1(marie,marie_call_laure,laure,laure_called_by_marie));

		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
		reset_counters(&laure->stat);


		linphone_core_transfer_call_to_another(marie->lc,marie_call_pauline,marie_call_laure);
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallRefered,1,2000));

		/*pauline pausing marie*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallPausing,1,4000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallPaused,1,4000));
		/*pauline calling laure*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingProgress,1,2000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransferCallOutgoingInit,1,2000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallIncomingReceived,1,2000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,2000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransferCallOutgoingProgress,1,2000));

		/*laure accept call*/
		for(calls=linphone_core_get_calls(laure->lc);calls!=NULL;calls=calls->next) {
			lcall = (LinphoneCall*)calls->data;
			if (linphone_call_get_state(lcall) == LinphoneCallIncomingReceived) {
				BC_ASSERT_PTR_EQUAL(linphone_call_get_replaced_call(lcall),laure_called_by_marie);
				linphone_core_accept_call(laure->lc,lcall);
				break;
			}
		}
		BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallConnected,1,2000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,1,2000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,2000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,2000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransferCallConnected,1,2000));

		/*terminate marie to pauline/laure call*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,2,2000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,1,2000));

		end_call(pauline, laure);
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(laure);
	linphone_core_manager_destroy(pauline);
	ms_list_free(lcs);
}

static void eject_from_3_participants_conference(LinphoneCoreManager *marie, LinphoneCoreManager *pauline, LinphoneCoreManager *laure, LinphoneCoreManager *focus) {
	stats initial_marie_stat;
	stats initial_pauline_stat;
	stats initial_laure_stat;

	LinphoneCall* marie_call_pauline;
	LinphoneCall* pauline_called_by_marie;
	LinphoneCall* marie_call_laure;
	bool_t is_remote_conf;
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);
	if(focus) lcs=ms_list_append(lcs,focus->lc);

	is_remote_conf = (strcmp(lp_config_get_string(marie->lc->config, "misc", "conference_type", "local"), "remote") == 0);
	if(is_remote_conf) BC_ASSERT_PTR_NOT_NULL(focus);

	BC_ASSERT_TRUE(call(marie,pauline));
	marie_call_pauline=linphone_core_get_current_call(marie->lc);
	pauline_called_by_marie=linphone_core_get_current_call(pauline->lc);
	BC_ASSERT_TRUE(pause_call_1(marie,marie_call_pauline,pauline,pauline_called_by_marie));

	BC_ASSERT_TRUE(call(marie,laure));
	initial_marie_stat=marie->stat;
	initial_pauline_stat=pauline->stat;
	initial_laure_stat=laure->stat;

	marie_call_laure=linphone_core_get_current_call(marie->lc);

	BC_ASSERT_PTR_NOT_NULL_FATAL(marie_call_laure);

	linphone_core_add_to_conference(marie->lc,marie_call_laure);

	if(!is_remote_conf) BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallUpdating,initial_marie_stat.number_of_LinphoneCallUpdating+1,5000));
	else {
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransferCallConnected,initial_marie_stat.number_of_LinphoneTransferCallConnected+1,5000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,initial_marie_stat.number_of_LinphoneCallEnd+1,5000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,initial_laure_stat.number_of_LinphoneCallEnd+1,5000));
	}

	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_conference(marie->lc));

	linphone_core_add_to_conference(marie->lc,marie_call_pauline);

	if(!is_remote_conf) BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallResuming,initial_marie_stat.number_of_LinphoneCallResuming+1,2000));
	else {
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneTransferCallConnected,initial_marie_stat.number_of_LinphoneTransferCallConnected+2,5000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,initial_marie_stat.number_of_LinphoneCallEnd+2,5000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,initial_pauline_stat.number_of_LinphoneCallEnd+1,5000));
	}

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,initial_pauline_stat.number_of_LinphoneCallStreamsRunning+1,5000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,initial_laure_stat.number_of_LinphoneCallStreamsRunning+1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,initial_marie_stat.number_of_LinphoneCallStreamsRunning+2,3000));

	BC_ASSERT_TRUE(linphone_core_is_in_conference(marie->lc));
	BC_ASSERT_EQUAL(linphone_core_get_conference_size(marie->lc),3, int, "%d");

	if(!is_remote_conf) BC_ASSERT_PTR_NULL(linphone_core_get_current_call(marie->lc));

	if(!is_remote_conf) linphone_core_remove_from_conference(marie->lc, marie_call_pauline);
	else {
		LinphoneConference *conference = linphone_core_get_conference(marie->lc);
		const LinphoneAddress *uri = linphone_call_get_remote_address(marie_call_pauline);
		linphone_conference_remove_participant(conference, uri);
	}

	if(!is_remote_conf) {
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallPausedByRemote,1,10000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,3,10000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,5,10000));
		BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(marie->lc));
		BC_ASSERT_EQUAL(ms_list_size(linphone_core_get_calls(marie->lc)), 2, int, "%d");
		BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(pauline->lc));
		BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(laure->lc));
	} else {
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,initial_pauline_stat.number_of_LinphoneCallEnd+2,5000));
	}

	if(!is_remote_conf) {
		end_call(laure, marie);
		end_call(pauline, marie);

		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,10000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,10000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,1,10000));
	} else {
		linphone_core_terminate_conference(marie->lc);
		BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,initial_laure_stat.number_of_LinphoneCallEnd+2,3000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,initial_marie_stat.number_of_LinphoneCallEnd+3,3000));
	}

	ms_list_free(lcs);
}

static void eject_from_3_participants_local_conference(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");

	eject_from_3_participants_conference(marie, pauline, laure, NULL);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
}

static void eject_from_4_participants_conference(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	LinphoneCoreManager* michelle = linphone_core_manager_new( "michelle_rc");
	int timeout_ms = 5000;
	stats initial_laure_stat;
	stats initial_michelle_stat;

	LinphoneCall* marie_call_pauline;
	LinphoneCall* pauline_called_by_marie;
	LinphoneCall* marie_call_laure;
	LinphoneCall* marie_call_michelle;
	LinphoneCall* michelle_called_by_marie;
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);
	lcs=ms_list_append(lcs,michelle->lc);

	BC_ASSERT_TRUE(call(marie,pauline));
	marie_call_pauline=linphone_core_get_current_call(marie->lc);
	pauline_called_by_marie=linphone_core_get_current_call(pauline->lc);
	BC_ASSERT_TRUE(pause_call_1(marie,marie_call_pauline,pauline,pauline_called_by_marie));

	BC_ASSERT_TRUE(call(marie,michelle));
	marie_call_michelle=linphone_core_get_current_call(marie->lc);
	michelle_called_by_marie=linphone_core_get_current_call(michelle->lc);
	BC_ASSERT_TRUE(pause_call_1(marie,marie_call_michelle,michelle,michelle_called_by_marie));

	BC_ASSERT_TRUE(call(marie,laure));
	initial_laure_stat=laure->stat;
	initial_michelle_stat=michelle->stat;

	marie_call_laure=linphone_core_get_current_call(marie->lc);

	BC_ASSERT_PTR_NOT_NULL_FATAL(marie_call_laure);

	linphone_core_add_to_conference(marie->lc,marie_call_laure);
	linphone_core_add_to_conference(marie->lc,marie_call_michelle);
	linphone_core_add_to_conference(marie->lc,marie_call_pauline);

	while (linphone_core_get_conference_size(marie->lc)!=4&&timeout_ms) {
		wait_for_list(lcs, NULL, 0, 100);
		timeout_ms -= 100;
	}
	BC_ASSERT_TRUE(linphone_core_is_in_conference(marie->lc));
	BC_ASSERT_EQUAL(linphone_core_get_conference_size(marie->lc),4, int, "%d");

	BC_ASSERT_PTR_NULL(linphone_core_get_current_call(marie->lc));

	linphone_core_remove_from_conference(marie->lc, marie_call_pauline);

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallPausedByRemote,1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,initial_laure_stat.number_of_LinphoneCallStreamsRunning+1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&michelle->stat.number_of_LinphoneCallStreamsRunning,initial_michelle_stat.number_of_LinphoneCallStreamsRunning+1,10000));

	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,5,10000));
	BC_ASSERT_PTR_NULL(linphone_core_get_current_call(marie->lc));
	BC_ASSERT_TRUE(linphone_core_is_in_conference(marie->lc));
	BC_ASSERT_EQUAL(linphone_core_get_conference_size(marie->lc),3, int, "%d");
	BC_ASSERT_EQUAL(ms_list_size(linphone_core_get_calls(marie->lc)), 3, int, "%d");
	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(pauline->lc));
	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(laure->lc));
	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call(michelle->lc));
	end_call(laure, marie);
	end_call(pauline, marie);
	end_call(michelle, marie);

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&michelle->stat.number_of_LinphoneCallEnd,1,10000));
	
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallReleased,1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallReleased,1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallReleased,1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&michelle->stat.number_of_LinphoneCallReleased,1,10000));

	ms_list_free(lcs);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	linphone_core_manager_destroy(michelle);
}


void simple_remote_conference(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_tcp_rc");
	LinphoneCoreManager *laure = linphone_core_manager_new("laure_rc");
	LinphoneConferenceServer *focus = linphone_conference_server_new("conference_focus_rc", TRUE);
	LpConfig *marie_config = linphone_core_get_config(marie->lc);
	LinphoneProxyConfig *focus_proxy_config = linphone_core_get_default_proxy_config(((LinphoneCoreManager *)focus)->lc);
	LinphoneProxyConfig *laure_proxy_config = linphone_core_get_default_proxy_config(((LinphoneCoreManager *)laure)->lc);
	const char *laure_proxy_uri = linphone_proxy_config_get_server_addr(laure_proxy_config);
	const char *focus_uri = linphone_proxy_config_get_identity(focus_proxy_config);
	int laure_n_register = laure->stat.number_of_LinphoneRegistrationOk;
	MSList *lcs = NULL;

	lp_config_set_string(marie_config, "misc", "conference_type", "remote");
	lp_config_set_string(marie_config, "misc", "conference_focus_addr", focus_uri);

	linphone_proxy_config_edit(laure_proxy_config);
	linphone_proxy_config_set_route(laure_proxy_config, laure_proxy_uri);
	linphone_proxy_config_done(laure_proxy_config);
	lcs = ms_list_append(lcs, laure->lc);
	BC_ASSERT_TRUE(wait_for_list(lcs, &laure->stat.number_of_LinphoneRegistrationOk, laure_n_register+1, 5000));
	ms_list_free(lcs);

	simple_conference_base(marie, pauline, laure, (LinphoneCoreManager *)focus);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	linphone_conference_server_destroy(focus);
}

void simple_remote_conference_shut_down_focus(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_tcp_rc");
	LinphoneCoreManager *laure = linphone_core_manager_new("laure_rc");
	LinphoneConferenceServer *focus = linphone_conference_server_new("conference_focus_rc", FALSE);
	LpConfig *marie_config = linphone_core_get_config(marie->lc);
	LinphoneProxyConfig *focus_proxy_config = linphone_core_get_default_proxy_config(((LinphoneCoreManager *)focus)->lc);
	LinphoneProxyConfig *laure_proxy_config = linphone_core_get_default_proxy_config(((LinphoneCoreManager *)laure)->lc);
	const char *laure_proxy_uri = linphone_proxy_config_get_server_addr(laure_proxy_config);
	const char *focus_uri = linphone_proxy_config_get_identity(focus_proxy_config);
	int laure_n_register = laure->stat.number_of_LinphoneRegistrationOk;
	MSList *lcs = NULL;

	lp_config_set_string(marie_config, "misc", "conference_type", "remote");
	lp_config_set_string(marie_config, "misc", "conference_focus_addr", focus_uri);

	linphone_proxy_config_edit(laure_proxy_config);
	linphone_proxy_config_set_route(laure_proxy_config, laure_proxy_uri);
	linphone_proxy_config_done(laure_proxy_config);
	lcs = ms_list_append(lcs, laure->lc);
	BC_ASSERT_TRUE(wait_for_list(lcs, &laure->stat.number_of_LinphoneRegistrationOk, laure_n_register+1, 5000));
	ms_list_free(lcs);

	simple_conference_base(marie, pauline, laure, (LinphoneCoreManager *)focus);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	linphone_conference_server_destroy(focus);
}

void eject_from_3_participants_remote_conference(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	LinphoneConferenceServer *focus = linphone_conference_server_new("conference_focus_rc", TRUE);
	LpConfig *marie_config = linphone_core_get_config(marie->lc);
	LinphoneProxyConfig *focus_proxy_config = linphone_core_get_default_proxy_config(((LinphoneCoreManager *)focus)->lc);
	LinphoneProxyConfig *laure_proxy_config = linphone_core_get_default_proxy_config(((LinphoneCoreManager *)laure)->lc);
	const char *laure_proxy_uri = linphone_proxy_config_get_server_addr(laure_proxy_config);
	const char *focus_uri = linphone_proxy_config_get_identity(focus_proxy_config);
	int laure_n_register = laure->stat.number_of_LinphoneRegistrationOk;
	MSList *lcs = NULL;

	lp_config_set_string(marie_config, "misc", "conference_type", "remote");
	lp_config_set_string(marie_config, "misc", "conference_focus_addr", focus_uri);

	linphone_proxy_config_edit(laure_proxy_config);
	linphone_proxy_config_set_route(laure_proxy_config, laure_proxy_uri);
	linphone_proxy_config_done(laure_proxy_config);
	lcs = ms_list_append(lcs, laure->lc);
	BC_ASSERT_TRUE(wait_for_list(lcs, &laure->stat.number_of_LinphoneRegistrationOk, laure_n_register+1, 5000));
	ms_list_free(lcs);

	eject_from_3_participants_conference(marie, pauline, laure, (LinphoneCoreManager *)focus);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	linphone_conference_server_destroy(focus);
}

test_t multi_call_tests[] = {
	TEST_NO_TAG("Call waiting indication", call_waiting_indication),
	TEST_NO_TAG("Call waiting indication with privacy", call_waiting_indication_with_privacy),
	TEST_NO_TAG("Second call rejected if first one in progress", second_call_rejected_if_first_one_in_progress),
	TEST_NO_TAG("Second call allowed if not using audio", second_call_allowed_if_not_using_audio),
	TEST_ONE_TAG("Incoming call accepted when outgoing call in progress", incoming_call_accepted_when_outgoing_call_in_progress, "LeaksMemory"),
	TEST_ONE_TAG("Incoming call accepted when outgoing call in outgoing ringing", incoming_call_accepted_when_outgoing_call_in_outgoing_ringing, "LeaksMemory"),
	TEST_ONE_TAG("Incoming call accepted when outgoing call in outgoing ringing early media", incoming_call_accepted_when_outgoing_call_in_outgoing_ringing_early_media, "LeaksMemory"),
	TEST_NO_TAG("Simple conference", simple_conference),
	TEST_TWO_TAGS("Simple conference with ICE", simple_conference_with_ice, "ICE",),
	TEST_TWO_TAGS("Simple ZRTP conference with ICE", simple_zrtp_conference_with_ice, "ICE",),
	TEST_NO_TAG("Eject from 3 participants conference", eject_from_3_participants_local_conference),
	TEST_ONE_TAG("Eject from 4 participants conference", eject_from_4_participants_conference, "LeaksMemory"),
	TEST_NO_TAG("Simple call transfer", simple_call_transfer),
	TEST_NO_TAG("Unattended call transfer", unattended_call_transfer),
	TEST_NO_TAG("Unattended call transfer with error", unattended_call_transfer_with_error),
	TEST_NO_TAG("Call transfer existing call outgoing call", call_transfer_existing_call_outgoing_call),
	TEST_NO_TAG("Simple remote conference", simple_remote_conference),
	TEST_ONE_TAG("Simple remote conference with shut down focus", simple_remote_conference_shut_down_focus, "LeaksMemory"),
	TEST_NO_TAG("Eject from 3 participants in remote conference", eject_from_3_participants_remote_conference),
};

test_suite_t multi_call_test_suite = {"Multi call", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
									  sizeof(multi_call_tests) / sizeof(multi_call_tests[0]), multi_call_tests};
