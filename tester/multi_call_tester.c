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

#ifdef WIN32
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
	LinphoneCallParams *laure_params=linphone_core_create_default_call_parameters(laure->lc);
	LinphoneCallParams *marie_params=linphone_core_create_default_call_parameters(marie->lc);

	if (enable_caller_privacy)
		linphone_call_params_set_privacy(marie_params,LinphonePrivacyId);

	lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);

	BC_ASSERT_TRUE(call_with_caller_params(marie,pauline,marie_params));
	pauline_called_by_marie=linphone_core_get_current_call(pauline->lc);

	if (enable_caller_privacy)
		linphone_call_params_set_privacy(laure_params,LinphonePrivacyId);

	BC_ASSERT_PTR_NOT_NULL(linphone_core_invite_address_with_params(laure->lc,pauline->identity,laure_params));

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

static void incoming_call_accepted_when_outgoing_call_in_state(LinphoneCallState state) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	MSList* lcs;
	LinphoneCallParams *laure_params=linphone_core_create_default_call_parameters(laure->lc);
	LinphoneCallParams *marie_params=linphone_core_create_default_call_parameters(marie->lc);

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

static void simple_conference_base(LinphoneCoreManager* marie, LinphoneCoreManager* pauline, LinphoneCoreManager* laure) {

	stats initial_marie_stat;
	stats initial_pauline_stat;
	stats initial_laure_stat;

	LinphoneCall* marie_call_pauline;
	LinphoneCall* pauline_called_by_marie;
	LinphoneCall* marie_call_laure;
	const MSList* calls;
	MSList* lcs=ms_list_append(NULL,marie->lc);
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);

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
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallUpdating,initial_marie_stat.number_of_LinphoneCallUpdating+1,5000));

	linphone_core_add_to_conference(marie->lc,marie_call_pauline);

	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallResuming,initial_marie_stat.number_of_LinphoneCallResuming+1,2000));

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,initial_pauline_stat.number_of_LinphoneCallStreamsRunning+1,5000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallStreamsRunning,initial_laure_stat.number_of_LinphoneCallStreamsRunning+1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,initial_marie_stat.number_of_LinphoneCallStreamsRunning+2,3000));

	BC_ASSERT_TRUE(linphone_core_is_in_conference(marie->lc));
	BC_ASSERT_EQUAL(linphone_core_get_conference_size(marie->lc),3, int, "%d");

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

	linphone_core_terminate_conference(marie->lc);

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,10000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&laure->stat.number_of_LinphoneCallEnd,1,10000));



	ms_list_free(lcs);
}
static void simple_conference(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");
	simple_conference_base(marie,pauline,laure);
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
		linphone_core_set_stun_server(marie->lc,"stun.linphone.org");
		linphone_core_set_firewall_policy(pauline->lc,LinphonePolicyUseIce);
		linphone_core_set_stun_server(pauline->lc,"stun.linphone.org");
		linphone_core_set_firewall_policy(laure->lc,LinphonePolicyUseIce);
		linphone_core_set_stun_server(laure->lc,"stun.linphone.org");

		/*work around a to avoid stun resolution to be initiate in  call_received callback leading a mainloop reentrency*/
		/*
			belle_sip_main_loop_iterate() at belle_sip_loop.c:369
			belle_sip_main_loop_run [inlined]() at belle_sip_loop.c:478
			belle_sip_main_loop_sleep() at belle_sip_loop.c:490
			sal_iterate() at sal_impl.c:745
			linphone_core_get_stun_server_addrinfo() at misc.c:585
			linphone_core_gather_ice_candidates() at misc.c:610
			linphone_call_prepare_ice() at linphonecall.c:1 906
			linphone_call_new_incoming() at linphonecall.c:1 101
			call_received() at callbacks.c:347
			...
			linphone_core_iterate() at linphonecore.c:2 620
			...

			linphone_core_set_stun_server() initiates an asynchronous resolution, but it needs a few iteration before it is completed.
			By calling private function linphone_core_get_stun_server_addrinfo() we make sure to wait that the resolution is done before the
			test calls actually start.
		*/
		linphone_core_get_stun_server_addrinfo(marie->lc);
		linphone_core_get_stun_server_addrinfo(pauline->lc);
		linphone_core_get_stun_server_addrinfo(laure->lc);
		/**/

		linphone_core_set_media_encryption(marie->lc,mode);
		linphone_core_set_media_encryption(pauline->lc,mode);
		linphone_core_set_media_encryption(laure->lc,mode);

		simple_conference_base(marie,pauline,laure);
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
	BC_ASSERT_TRUE(linphone_call_get_transferer_call(marie_calling_laure)==marie_calling_pauline);

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneTransferCallConnected,1,2000));

	/*terminate marie to pauline call*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,2000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,2000));

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
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
	ms_list_free(lcs);
}

test_t multi_call_tests[] = {
	{ "Call waiting indication", call_waiting_indication },
	{ "Call waiting indication with privacy", call_waiting_indication_with_privacy },
	{ "Simple conference", simple_conference },
	{ "Simple conference with ICE",simple_conference_with_ice},
	{ "Simple ZRTP conference with ICE",simple_zrtp_conference_with_ice},
	{ "Simple call transfer", simple_call_transfer },
	{ "Unattended call transfer", unattended_call_transfer },
	{ "Unattended call transfer with error", unattended_call_transfer_with_error },
	{ "Call transfer existing call outgoing call", call_transfer_existing_call_outgoing_call },
	{ "Incoming call accepted when outgoing call in progress",incoming_call_accepted_when_outgoing_call_in_progress},
	{ "Incoming call accepted when outgoing call in outgoing ringing",incoming_call_accepted_when_outgoing_call_in_outgoing_ringing},
	{ "Incoming call accepted when outgoing call in outgoing ringing early media",incoming_call_accepted_when_outgoing_call_in_outgoing_ringing_early_media},
};

test_suite_t multi_call_test_suite = {
	"Multi call",
	liblinphone_tester_setup,
	NULL,
	sizeof(multi_call_tests) / sizeof(multi_call_tests[0]),
	multi_call_tests
};
