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


#include "linphonecore.h"
#include "lpconfig.h"
#include "private.h"
#include "liblinphone_tester.h"

static void subscribe_forking(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCoreManager* pauline2 = linphone_core_manager_new( transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneContent* content;
	LinphoneEvent *lev;
	int expires=  600;
	MSList* lcs=ms_list_append(NULL,marie->lc);

	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,pauline2->lc);

	content = linphone_core_create_content(marie->lc);
	linphone_content_set_type(content,"application");
	linphone_content_set_subtype(content,"somexml");
	linphone_content_set_buffer(content, liblinphone_tester_get_subscribe_content(), strlen(liblinphone_tester_get_subscribe_content()));

	lev=linphone_core_subscribe(marie->lc,pauline->identity,"dodo",expires,content);

	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionOutgoingInit,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionIncomingReceived,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline2->stat.number_of_LinphoneSubscriptionIncomingReceived,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionActive,1,1000));

	/*make sure marie receives first notification before terminating*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_NotifyReceived,1,1000));

	linphone_event_terminate(lev);

	linphone_content_unref(content);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(pauline2);
	ms_list_free(lcs);
}

static void message_forking(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");
	MSList* lcs=ms_list_append(NULL,marie->lc);
	LinphoneChatRoom* chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(message);

	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,marie2->lc);

	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(chat_room, message);
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneMessageReceived,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneMessageReceived,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneMessageDelivered,1,1000));

	/*wait a bit that 200Ok for MESSAGE are sent to server before shuting down the cores, because otherwise Flexisip will consider the messages
	 * as not delivered and will expedite them in the next test, after receiving the REGISTER from marie's instances*/
	wait_for_list(lcs, NULL, 0, 2000);

	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1, int, "%d");
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(pauline);
	ms_list_free(lcs);
}

static void message_forking_with_unreachable_recipients(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie3 = linphone_core_manager_new( "marie_rc");
	MSList* lcs=ms_list_append(NULL,marie->lc);
	LinphoneChatRoom* chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(message);

	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	lcs=ms_list_append(lcs,marie3->lc);

	/*the following lines are to workaround a problem with messages sent by a previous test (Message forking) that arrive together with REGISTER responses,
	 * because the ForkMessageContext is not terminated at flexisip side if Message forking test is passing fast*/
	wait_for_list(lcs,NULL,0,1000);
	marie->stat.number_of_LinphoneMessageReceived = 0;
	marie2->stat.number_of_LinphoneMessageReceived = 0;
	marie3->stat.number_of_LinphoneMessageReceived = 0;

	/*marie2 and marie3 go offline*/
	linphone_core_set_network_reachable(marie2->lc,FALSE);
	linphone_core_set_network_reachable(marie3->lc,FALSE);

	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(chat_room, message);
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneMessageReceived,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneMessageDelivered,1,1000));
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1, int, "%d");
	BC_ASSERT_EQUAL(marie2->stat.number_of_LinphoneMessageReceived, 0, int, "%d");
	BC_ASSERT_EQUAL(marie3->stat.number_of_LinphoneMessageReceived, 0, int, "%d");
	/*marie 2 goes online */
	linphone_core_set_network_reachable(marie2->lc,TRUE);
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneMessageReceived,1,3000));

	/*wait a long time so that all transactions are expired*/
	wait_for_list(lcs,NULL,0,32000);

	/*marie 3 goes online now*/
	linphone_core_set_network_reachable(marie3->lc,TRUE);
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneMessageReceived,1,3000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(marie3);
	linphone_core_manager_destroy(pauline);
	ms_list_free(lcs);
}

static void message_forking_with_all_recipients_unreachable(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie3 = linphone_core_manager_new( "marie_rc");
	MSList* lcs=ms_list_append(NULL,marie->lc);
	LinphoneChatRoom* chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(message);

	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	lcs=ms_list_append(lcs,marie3->lc);

	/*the following lines are to workaround a problem with messages sent by a previous test (Message forking) that arrive together with REGISTER responses,
	 * because the ForkMessageContext is not terminated at flexisip side if Message forking test is passing fast*/
	wait_for_list(lcs,NULL,0,1000);
	marie->stat.number_of_LinphoneMessageReceived = 0;
	marie2->stat.number_of_LinphoneMessageReceived = 0;
	marie3->stat.number_of_LinphoneMessageReceived = 0;


	/*All marie's device go offline*/
	linphone_core_set_network_reachable(marie->lc,FALSE);
	linphone_core_set_network_reachable(marie2->lc,FALSE);
	linphone_core_set_network_reachable(marie3->lc,FALSE);

	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(chat_room, message);

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneMessageInProgress,1,5000));
	/*flexisip will accept the message with 202 after 16 seconds*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneMessageDelivered,1,18000));
	BC_ASSERT_EQUAL( marie->stat.number_of_LinphoneMessageReceived, 0, int, "%d");
	BC_ASSERT_EQUAL( marie2->stat.number_of_LinphoneMessageReceived, 0, int, "%d");
	BC_ASSERT_EQUAL( marie3->stat.number_of_LinphoneMessageReceived, 0, int, "%d");

	/*marie 1 goes online */
	linphone_core_set_network_reachable(marie->lc,TRUE);
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneMessageReceived,1,3000));

	/*marie 2 goes online */
	linphone_core_set_network_reachable(marie2->lc,TRUE);
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneMessageReceived,1,3000));

	/*wait a long time so that all transactions are expired*/
	wait_for_list(lcs,NULL,0,32000);

	/*marie 3 goes online now*/
	linphone_core_set_network_reachable(marie3->lc,TRUE);
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneMessageReceived,1,3000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(marie3);
	linphone_core_manager_destroy(pauline);
	ms_list_free(lcs);
}

static void call_forking(void){
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie3 = linphone_core_manager_new( "marie_rc");
	MSList* lcs=ms_list_append(NULL,pauline->lc);

	lcs=ms_list_append(lcs,marie->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	lcs=ms_list_append(lcs,marie3->lc);

	linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(marie2->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(marie3->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);

	linphone_core_invite_address(pauline->lc,marie->identity);
	/*pauline should hear ringback*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,3000));
	/*all devices from Marie should be ringing*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallIncomingReceived,1,3000));

	/*marie accepts the call on its first device*/
	linphone_core_accept_call(marie->lc,linphone_core_get_current_call(marie->lc));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallConnected,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,1,1000));

	/*other devices should stop ringing*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallEnd,1,1000));

	linphone_core_terminate_call(pauline->lc,linphone_core_get_current_call(pauline->lc));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));

	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(marie3);
	ms_list_free(lcs);
}

static void call_forking_with_urgent_reply(void){
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie3 = linphone_core_manager_new( "marie_rc");
	MSList* lcs=ms_list_append(NULL,pauline->lc);
	lcs=ms_list_append(lcs,marie->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	lcs=ms_list_append(lcs,marie3->lc);

	if (linphone_core_media_encryption_supported(pauline->lc,LinphoneMediaEncryptionSRTP)) {
		linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
		linphone_core_set_user_agent(marie2->lc,"Natted Linphone",NULL);
		linphone_core_set_user_agent(marie3->lc,"Natted Linphone",NULL);
		linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);


		linphone_core_set_media_encryption(pauline->lc,LinphoneMediaEncryptionSRTP);
		linphone_core_set_network_reachable(marie2->lc,FALSE);
		linphone_core_set_network_reachable(marie3->lc,FALSE);

		linphone_core_invite_address(pauline->lc,marie->identity);
		/*pauline should hear ringback, after 5 seconds, when it will retry without SRTP*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,9000));
		/*Marie should be ringing*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,1000));

		/*marie accepts the call on its first device*/
		linphone_core_accept_call(marie->lc,linphone_core_get_current_call(marie->lc));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,1000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,1000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallConnected,1,1000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,1,1000));

		linphone_core_terminate_call(pauline->lc,linphone_core_get_current_call(pauline->lc));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));
	}
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(marie3);
	ms_list_free(lcs);
}

static void call_forking_cancelled(void){
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie3 = linphone_core_manager_new( "marie_rc");
	MSList* lcs=ms_list_append(NULL,pauline->lc);

	lcs=ms_list_append(lcs,marie->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	lcs=ms_list_append(lcs,marie3->lc);

	linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(marie2->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(marie3->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);

	linphone_core_invite_address(pauline->lc,marie->identity);
	/*pauline should hear ringback*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,3000));
	/*all devices from Marie should be ringing*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallIncomingReceived,1,1000));

	/*pauline finally cancels the call*/
	linphone_core_terminate_call(pauline->lc,linphone_core_get_current_call(pauline->lc));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));

	/*all devices should stop ringing*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallEnd,1,1000));

	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(marie3);
	ms_list_free(lcs);
}

static void call_forking_declined(bool_t declined_globaly){
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie3 = linphone_core_manager_new( "marie_rc");
	MSList* lcs=ms_list_append(NULL,pauline->lc);

	lcs=ms_list_append(lcs,marie->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	lcs=ms_list_append(lcs,marie3->lc);

	linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(marie2->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(marie3->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);

	linphone_core_invite_address(pauline->lc,marie->identity);
	/*pauline should hear ringback*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,3000));
	/*all devices from Marie should be ringing*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallIncomingReceived,1,1000));

	/*marie finally declines the call*/
	linphone_core_decline_call(marie->lc,linphone_core_get_current_call(marie->lc),
		declined_globaly ? LinphoneReasonDeclined : LinphoneReasonBusy
	);

	if (declined_globaly){
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));
		/*all devices should stop ringing*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallEnd,1,1000));
	}else{
		/*pauline should continue ringing and be able to hear a call taken by marie2 */
		linphone_core_accept_call(marie2->lc, linphone_core_get_current_call(marie2->lc));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,2000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallStreamsRunning,1,2000));
		liblinphone_tester_check_rtcp(pauline,marie2);
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,3000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallEnd,1,3000));
		linphone_core_terminate_call(marie2->lc,linphone_core_get_current_call(marie2->lc));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,3000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,3000));
	}

	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(marie3);
	ms_list_free(lcs);
}

static void call_forking_declined_globaly(void){
	call_forking_declined(TRUE);
}

static void call_forking_declined_localy(void){
	call_forking_declined(FALSE);
}

static void call_forking_with_push_notification_single(void){
	MSList* lcs;
	LinphoneCoreManager* marie = linphone_core_manager_new2( "marie_rc", FALSE);
	LinphoneCoreManager* pauline = linphone_core_manager_new2( transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc",FALSE);
	int dummy=0;

	linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);
	linphone_proxy_config_set_contact_uri_parameters(
		linphone_core_get_default_proxy_config(marie->lc),
		"app-id=org.linphonetester;pn-tok=aaabbb;pn-type=apple;pn-msg-str=33;pn-call-str=34;");

	lcs=ms_list_append(NULL,pauline->lc);
	lcs=ms_list_append(lcs,marie->lc);

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneRegistrationOk,1,5000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneRegistrationOk,1,5000));

	/*unfortunately marie gets unreachable due to crappy 3G operator or iOS bug...*/
	linphone_core_set_network_reachable(marie->lc,FALSE);

	linphone_core_invite_address(pauline->lc,marie->identity);

	/*After 5 seconds the server is expected to send a push notification to marie, this will wake up linphone, that will reconnect:*/
	wait_for_list(lcs,&dummy,1,6000);
	linphone_core_set_network_reachable(marie->lc,TRUE);

	/*Marie shall receive the call immediately*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,5000));
	/*pauline should hear ringback as well*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,1000));

	/*marie accepts the call*/
	linphone_core_accept_call(marie->lc,linphone_core_get_current_call(marie->lc));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,5000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallConnected,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,1,1000));

	liblinphone_tester_check_rtcp(pauline,marie);

	linphone_core_terminate_call(pauline->lc,linphone_core_get_current_call(pauline->lc));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,5000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,5000));

	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	ms_list_free(lcs);
}

static void call_forking_with_push_notification_multiple(void){
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");

	MSList* lcs=ms_list_append(NULL,pauline->lc);

	lcs=ms_list_append(lcs,marie->lc);
	lcs=ms_list_append(lcs,marie2->lc);

	linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(marie2->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);

	/*unfortunately marie gets unreachable due to crappy 3G operator or iOS bug...*/
	linphone_core_set_network_reachable(marie2->lc,FALSE);

	linphone_core_invite_address(pauline->lc,marie->identity);

	/*marie will ring*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,5000));
	/*pauline should hear ringback as well*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,1000));

	/*the server is expected to send a push notification to marie2, this will wake up linphone, that will reconnect:*/
	linphone_core_set_network_reachable(marie2->lc,TRUE);

	/*Marie shall receive the call immediately*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived,1,5000));

	/*marie2 accepts the call*/
	linphone_core_accept_call(marie2->lc,linphone_core_get_current_call(marie2->lc));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallConnected,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallStreamsRunning,1,1000));

	/*call to marie should be cancelled*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));

	liblinphone_tester_check_rtcp(pauline,marie2);

	linphone_core_terminate_call(pauline->lc,linphone_core_get_current_call(pauline->lc));

	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));

	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
}

static void call_forking_not_responded(void){
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie3 = linphone_core_manager_new( "marie_rc");
	MSList* lcs=ms_list_append(NULL,pauline->lc);

	lcs=ms_list_append(lcs,marie->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	lcs=ms_list_append(lcs,marie3->lc);

	linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(marie2->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(marie3->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);

	linphone_core_invite_address(pauline->lc,marie->identity);
	/*pauline should hear ringback*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,3000));
	/*all devices from Marie should be ringing*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallIncomingReceived,1,1000));

	/*nobody answers, flexisip should close the call after XX seconds*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallError,1,22000));
	/*all devices should stop ringing*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallEnd,1,1000));

	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(marie3);
	ms_list_free(lcs);
}

static void early_media_call_forking(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_early_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new("marie_early_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	MSList *lcs=NULL;
	LinphoneCallParams *params=linphone_core_create_call_params(pauline->lc, NULL);
	LinphoneVideoPolicy pol;
	int dummy=0;

	pol.automatically_accept=1;
	pol.automatically_initiate=1;

	linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(marie2->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);

	linphone_core_enable_video_capture(pauline->lc,TRUE);
	linphone_core_enable_video_display(pauline->lc,TRUE);

	linphone_core_enable_video_capture(marie->lc,TRUE);
	linphone_core_enable_video_display(marie->lc,TRUE);
	linphone_core_set_video_policy(marie->lc,&pol);

	linphone_core_enable_video_capture(marie2->lc,TRUE);
	linphone_core_enable_video_display(marie2->lc,TRUE);

	linphone_core_set_video_policy(marie2->lc,&pol);
	linphone_core_set_audio_port_range(marie2->lc,40200,40300);
	linphone_core_set_video_port_range(marie2->lc,40400,40500);

	lcs=ms_list_append(lcs,marie->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	lcs=ms_list_append(lcs,pauline->lc);

	linphone_call_params_enable_early_media_sending(params,TRUE);
	linphone_call_params_enable_video(params,TRUE);

	linphone_core_invite_address_with_params(pauline->lc,marie->identity,params);
	linphone_call_params_destroy(params);

	BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphoneCallIncomingEarlyMedia,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie2->stat.number_of_LinphoneCallIncomingEarlyMedia,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallOutgoingEarlyMedia,1,3000));
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallOutgoingEarlyMedia,1, int, "%d");

	/*wait a bit that streams are established*/
	wait_for_list(lcs,&dummy,1,5000);
	BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_down_bw(pauline), 60, int, "%d");
	BC_ASSERT_LOWER(linphone_core_manager_get_mean_audio_down_bw(pauline), 99, int, "%d");
	BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_down_bw(marie), 60, int, "%d");
	BC_ASSERT_LOWER(linphone_core_manager_get_mean_audio_down_bw(marie), 99, int, "%d");
	BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_down_bw(marie2), 60, int, "%d");
	BC_ASSERT_LOWER(linphone_core_manager_get_mean_audio_down_bw(marie2), 99, int, "%d");

	linphone_core_accept_call(marie->lc,linphone_core_get_current_call(marie->lc));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,1,3000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,3000));

	/*marie2 should get her call terminated*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));

	/*wait a bit that streams are established*/
	wait_for_list(lcs,&dummy,1,3000);
	BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_down_bw(pauline), 60, int, "%d");
	BC_ASSERT_LOWER(linphone_core_manager_get_mean_audio_down_bw(pauline), 99, int, "%d");
	BC_ASSERT_GREATER(linphone_core_manager_get_mean_audio_down_bw(marie), 60, int, "%d");
	BC_ASSERT_LOWER(linphone_core_manager_get_mean_audio_down_bw(marie), 99, int, "%d");

	end_call(pauline, marie);

	ms_list_free(lcs);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(marie);
}

static void call_with_sips(void){
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_sips_rc");
		LinphoneCoreManager* pauline1 = linphone_core_manager_new( "pauline_sips_rc");
		LinphoneCoreManager* pauline2 = linphone_core_manager_new( "pauline_tcp_rc");
		MSList* lcs=ms_list_append(NULL,marie->lc);

		lcs=ms_list_append(lcs,pauline1->lc);
		lcs=ms_list_append(lcs,pauline2->lc);

		linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
		linphone_core_set_user_agent(pauline1->lc,"Natted Linphone",NULL);
		linphone_core_set_user_agent(pauline2->lc,"Natted Linphone",NULL);

		linphone_core_invite_address(marie->lc,pauline1->identity);

		/*marie should hear ringback*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallOutgoingRinging,1,3000));
		/*Only the sips registered device from pauline should ring*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline1->stat.number_of_LinphoneCallIncomingReceived,1,1000));

		/*pauline accepts the call */
		linphone_core_accept_call(pauline1->lc,linphone_core_get_current_call(pauline1->lc));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline1->stat.number_of_LinphoneCallConnected,1,1000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline1->stat.number_of_LinphoneCallStreamsRunning,1,1000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallConnected,1,1000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,1,1000));

		/*pauline2 should not have ring*/
		BC_ASSERT_EQUAL(pauline2->stat.number_of_LinphoneCallIncomingReceived, 0, int, "%d");

		linphone_core_terminate_call(pauline1->lc,linphone_core_get_current_call(pauline1->lc));
		BC_ASSERT_TRUE(wait_for_list(lcs,&pauline1->stat.number_of_LinphoneCallEnd,1,3000));
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,3000));

		linphone_core_manager_destroy(marie);
		linphone_core_manager_destroy(pauline1);
		linphone_core_manager_destroy(pauline2);
		ms_list_free(lcs);
	}
}

static void call_with_sips_not_achievable(void){
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* pauline2 = linphone_core_manager_new( "pauline_tcp_rc");
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_sips_rc");
		LinphoneCoreManager* pauline1 = linphone_core_manager_new( "pauline_rc");
		MSList* lcs=ms_list_append(NULL,marie->lc);
		LinphoneAddress *dest;
		LinphoneCall *call;
		const LinphoneErrorInfo *ei;

		lcs=ms_list_append(lcs,pauline1->lc);
		lcs=ms_list_append(lcs,pauline2->lc);


		dest=linphone_address_clone(pauline1->identity);
		linphone_address_set_secure(dest,TRUE);
		call=linphone_core_invite_address(marie->lc,dest);
		linphone_call_ref(call);
		linphone_address_unref(dest);

		/*Call should be rejected by server with 480*/
		BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallError,1,6000));
		ei=linphone_call_get_error_info(call);
		BC_ASSERT_PTR_NOT_NULL(ei);
		if (ei){
			BC_ASSERT_EQUAL(linphone_error_info_get_reason(ei), LinphoneReasonTemporarilyUnavailable, int, "%d");
		}

		linphone_core_manager_destroy(marie);
		linphone_core_manager_destroy(pauline1);
		linphone_core_manager_destroy(pauline2);
		ms_list_free(lcs);
	}
}

static void call_with_ipv6(void) {
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCall *pauline_call;

	/*calling ortp_init() here is done to have WSAStartup() done, otherwise liblinphone_tester_ipv6_available() will not work.*/
	ortp_init();

	if (!liblinphone_tester_ipv6_available()){
		ms_warning("Call with ipv6 not tested, no ipv6 connectivity");
		return;
	}

	liblinphone_tester_enable_ipv6(TRUE);
	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new( transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);
	BC_ASSERT_TRUE(call(marie,pauline));
	pauline_call=linphone_core_get_current_call(pauline->lc);
	BC_ASSERT_PTR_NOT_NULL(pauline_call);
	if (pauline_call){
		/*check that the remote contact is IPv6*/
		const char *contact=linphone_call_get_remote_contact(pauline_call);
		LinphoneAddress *ct_addr;

		BC_ASSERT_PTR_NOT_NULL(contact);
		if (contact){
			ct_addr=linphone_address_new(contact);
			BC_ASSERT_PTR_NOT_NULL(ct_addr);
			if (ct_addr){
				BC_ASSERT_PTR_NOT_NULL(strchr(linphone_address_get_domain(ct_addr),':'));
			}
			linphone_address_destroy(ct_addr);
		}

	}

	liblinphone_tester_check_rtcp(marie,pauline);
	end_call(marie,pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	liblinphone_tester_enable_ipv6(FALSE);

	ortp_exit();
}

static void file_transfer_message_rcs_to_external_body_client(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneChatRoom* chat_room;
		LinphoneChatMessage* message;
		LinphoneChatMessageCbs *cbs;
		LinphoneContent* content;
		FILE *file_to_send = NULL;
		size_t file_size;
		char *send_filepath = bc_tester_res("images/nowebcamCIF.jpg");
		char *receive_filepath = bc_tester_file("receive_file.dump");
		LinphoneCoreManager* marie = linphone_core_manager_new2( "marie_rc", FALSE);
		LinphoneCoreManager* pauline = linphone_core_manager_new2( "pauline_rc", FALSE);
		// This is done to prevent register to be sent before the custom header is set
		linphone_core_set_network_reachable(marie->lc, FALSE);
		linphone_core_set_network_reachable(pauline->lc, FALSE);

		linphone_proxy_config_set_custom_header(marie->lc->default_proxy, "Accept", "application/sdp");
		linphone_core_set_network_reachable(marie->lc, TRUE);
		linphone_core_manager_start(marie, TRUE);
		

		linphone_proxy_config_set_custom_header(pauline->lc->default_proxy, "Accept", "application/sdp, text/plain, application/vnd.gsma.rcs-ft-http+xml");
		linphone_core_set_network_reachable(pauline->lc, TRUE);
		linphone_core_manager_start(pauline, TRUE);

		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);

		file_to_send = fopen(send_filepath, "rb");
		fseek(file_to_send, 0, SEEK_END);
		file_size = ftell(file_to_send);
		fseek(file_to_send, 0, SEEK_SET);

		/* Globally configure an http file transfer server. */
		linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

		/* create a chatroom on pauline's side */

		chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);
		/* create a file transfer message */
		content = linphone_core_create_content(pauline->lc);
		linphone_content_set_type(content,"image");
		linphone_content_set_subtype(content,"jpeg");
		linphone_content_set_size(content,file_size); /*total size to be transfered*/
		linphone_content_set_name(content,"nowebcamCIF.jpg");
		message = linphone_chat_room_create_file_transfer_message(chat_room, content);
		linphone_chat_message_set_user_data(message, file_to_send);
		cbs = linphone_chat_message_get_callbacks(message);
		{
			int dummy=0;
			wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
			reset_counters(&marie->stat);
			reset_counters(&pauline->stat);
		}
		linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
		linphone_chat_message_cbs_set_file_transfer_send(cbs, tester_file_transfer_send);
		linphone_chat_room_send_chat_message(chat_room,message);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageExtBodyReceived,1));

		if (marie->stat.last_received_chat_message ) {
			cbs = linphone_chat_message_get_callbacks(marie->stat.last_received_chat_message);
			linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
			linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_received);
			linphone_chat_message_download_file(marie->stat.last_received_chat_message);
		}
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageFileTransferDone,1));

		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,2, int, "%d");
		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,1, int, "%d");
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,1, int, "%d");
		compare_files(send_filepath, receive_filepath);

		linphone_content_unref(content);
		linphone_core_manager_destroy(marie);
		linphone_core_manager_destroy(pauline);
		ms_free(send_filepath);
		bc_free(receive_filepath);
	}
}

void send_file_transfer_message_using_external_body_url(LinphoneCoreManager *marie, LinphoneCoreManager *pauline) {
	LinphoneChatMessageCbs *cbs;
	LinphoneChatRoom *chat_room;
	LinphoneChatMessage *message;

	/* create a chatroom on pauline's side */
	chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);

	message = linphone_chat_room_create_message(chat_room, NULL);

	cbs = linphone_chat_message_get_callbacks(message);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);

	linphone_chat_message_set_external_body_url(message, "https://www.linphone.org:444//tmp/54ec58280ace9_c30709218df8eaba61d1.jpg");
	linphone_chat_room_send_chat_message(chat_room, message);

	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceived, 1));
	if (marie->stat.last_received_chat_message) {
		linphone_chat_message_download_file(marie->stat.last_received_chat_message);
	}
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageExtBodyReceived, 1));

	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress, 1, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived, 1, int, "%d");
	
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneMessageDelivered, 1));
	
}

static void file_transfer_message_external_body_to_external_body_client(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

		linphone_proxy_config_set_custom_header(marie->lc->default_proxy, "Accept", "application/sdp");
		linphone_core_manager_start(marie, TRUE);

		linphone_proxy_config_set_custom_header(pauline->lc->default_proxy, "Accept", "application/sdp");
		linphone_core_manager_start(pauline, TRUE);

		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);

		linphone_core_refresh_registers(marie->lc);
		linphone_core_refresh_registers(pauline->lc);

		send_file_transfer_message_using_external_body_url(marie, pauline);

		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}
}

static void file_transfer_message_external_body_to_rcs_client(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

		linphone_proxy_config_set_custom_header(marie->lc->default_proxy, "Accept", "application/sdp");
		linphone_core_manager_start(marie, TRUE);

		linphone_proxy_config_set_custom_header(pauline->lc->default_proxy, "Accept", "application/sdp, text/plain, application/vnd.gsma.rcs-ft-http+xml");
		linphone_core_manager_start(pauline, TRUE);

		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);

		send_file_transfer_message_using_external_body_url(marie, pauline);

		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}
}

static void dos_module_trigger(void) {
	LinphoneChatRoom *chat_room;
	int i = 0;
	const char* passmsg = "This one should pass through";
	int number_of_messge_to_send = 100;
	LinphoneChatMessage * chat_msg = NULL;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);

	chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);

	do {
		char msg[128];
		sprintf(msg, "Flood message number %i", i);
		chat_msg = linphone_chat_room_create_message(chat_room, msg);
		linphone_chat_room_send_chat_message(chat_room, chat_msg);
		ms_usleep(10000);
		i++;
	} while (i < number_of_messge_to_send);
	// At this point we should be banned for a minute

	ms_usleep(65000000); // Wait several seconds to ensure we are not banned anymore
	BC_ASSERT_LOWER(marie->stat.number_of_LinphoneMessageReceived, number_of_messge_to_send, int, "%d");

	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);
	chat_msg = linphone_chat_room_create_message(chat_room, passmsg);
	linphone_chat_room_send_chat_message(chat_room, chat_msg);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived, 1));
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceived, 1, int, "%d");
	if (marie->stat.last_received_chat_message) {
		BC_ASSERT_NSTRING_EQUAL(linphone_chat_message_get_text(marie->stat.last_received_chat_message), passmsg, strlen(passmsg));
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


static void test_subscribe_notify_with_sipp_publisher(void) {
	char *scen;
	FILE * sipp_out;
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	/*just to get an identity*/
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneAddress *sip_example_org;
	const LinphoneAuthInfo	*marie_auth = linphone_core_find_auth_info(marie->lc, NULL, linphone_address_get_username(marie->identity), NULL);
	LpConfig *pauline_lp = linphone_core_get_config(pauline->lc);
	char* lf_identity=linphone_address_as_string_uri_only(marie->identity);
	LinphoneFriend *lf = linphone_core_create_friend_with_address(pauline->lc,lf_identity);

	linphone_core_set_user_agent(marie->lc, "full-presence-support", NULL);
	linphone_core_set_user_agent(pauline->lc, "full-presence-support", NULL);

	ms_free(lf_identity);

	lp_config_set_int(pauline_lp,"sip","subscribe_expires",5);

	linphone_core_add_friend(pauline->lc,lf);

	/*wait for subscribe acknowledgment*/
	BC_ASSERT_TRUE(wait_for_until(pauline->lc,pauline->lc,&pauline->stat.number_of_NotifyPresenceReceived,1,2000));
	BC_ASSERT_EQUAL(LinphoneStatusOffline,linphone_friend_get_status(lf), int, "%d");

	scen = bc_tester_res("sipp/simple_publish.xml");

	sip_example_org = linphone_core_manager_resolve(marie, marie->identity);
	sipp_out = sip_start(scen, linphone_address_get_username(marie->identity), linphone_auth_info_get_passwd(marie_auth), sip_example_org);
	linphone_address_destroy(sip_example_org);

	if (sipp_out) {
		/*wait for marie status*/
		BC_ASSERT_TRUE(wait_for_until(pauline->lc,pauline->lc,&pauline->stat.number_of_NotifyPresenceReceived,2,3000));
		BC_ASSERT_EQUAL(LinphoneStatusOnline,linphone_friend_get_status(lf), int, "%d");
		BC_ASSERT_EQUAL(0,pclose(sipp_out),int,"%d");
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
	//does not work because sipp seams not able to manage 2 call  id in case file
#if 0
static void test_subscribe_notify_with_sipp_publisher_double_publish(void) {
	char *scen;
	FILE * sipp_out;
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	/*just to get an identity*/
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneAddress *sip_example_org;

	linphone_core_set_user_agent(marie->lc, "full-presence-support", NULL);
	linphone_core_set_user_agent(pauline->lc, "full-presence-support", NULL);

	LpConfig *pauline_lp = linphone_core_get_config(pauline->lc);
	char* lf_identity=linphone_address_as_string_uri_only(marie->identity);
	LinphoneFriend *lf = linphone_core_create_friend_with_address(pauline->lc,lf_identity);
	ms_free(lf_identity);
	lp_config_set_int(pauline_lp,"sip","subscribe_expires",5);

	linphone_core_add_friend(pauline->lc,lf);

	/*wait for subscribe acknowledgment*/
	BC_ASSERT_TRUE(wait_for_until(pauline->lc,pauline->lc,&pauline->stat.number_of_NotifyPresenceReceived,1,2000));
	BC_ASSERT_EQUAL(LinphoneStatusOffline,linphone_friend_get_status(lf), int, "%d");

	scen = bc_tester_res("sipp/double_publish_with_error.xml");

	sip_example_org = linphone_core_manager_resolve(marie, marie->identity);
	sipp_out = sip_start(scen, linphone_address_get_username(marie->identity), sip_example_org);

	if (sipp_out) {
		/*wait for marie status*/
		BC_ASSERT_TRUE(wait_for_until(pauline->lc,pauline->lc,&pauline->stat.number_of_NotifyPresenceReceived,2,3000));
		BC_ASSERT_EQUAL(LinphoneStatusOnline,linphone_friend_get_status(lf), int, "%d");
		pclose(sipp_out);
		BC_ASSERT_EQUAL(pauline->stat.number_of_NotifyPresenceReceived,2,int, "%d");
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
#endif

static void test_publish_unpublish(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneProxyConfig* proxy;

	proxy = linphone_core_get_default_proxy_config(marie->lc);
	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_enable_publish(proxy,TRUE);
	linphone_proxy_config_done(proxy);
	wait_core(marie->lc);
	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_enable_publish(proxy,FALSE);
	linphone_proxy_config_done(proxy);
	wait_core(marie->lc);
	linphone_core_manager_destroy(marie);
}

static void test_list_subscribe (void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* laure = linphone_core_manager_new( "laure_rc");

	char *list =	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
					"<resource-lists xmlns=\"urn:ietf:params:xml:ns:resource-lists\"\n"
					"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
					"<list>\n"
						"\t<entry uri=\"%s\" />\n"
						"\t<entry uri=\"%s\" />\n"
					"</list>\n"
					"</resource-lists>\n";


	LinphoneEvent *lev;
	MSList* lcs=ms_list_append(NULL,marie->lc);
	char * pauline_uri=linphone_address_as_string_uri_only(pauline->identity);
	char * laure_uri=linphone_address_as_string_uri_only(laure->identity);
	char * subscribe_content = ms_strdup_printf(list,pauline_uri,laure_uri);
	LinphoneContent* content = linphone_core_create_content(marie->lc);
	LinphoneAddress *list_name = linphone_address_new("sip:mescops@sip.example.org");
	LinphoneProxyConfig* proxy_config;
	int dummy=0;

	ms_free(pauline_uri);
	ms_free(laure_uri);

	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,laure->lc);

	linphone_content_set_type(content,"application");
	linphone_content_set_subtype(content,"resource-lists+xml");
	linphone_content_set_buffer(content,subscribe_content,strlen(subscribe_content));

	lev=linphone_core_create_subscribe(marie->lc,list_name,"presence",60);

	linphone_event_add_custom_header(lev,"Supported","eventlist");
	linphone_event_add_custom_header(lev,"Accept","application/pidf+xml, application/rlmi+xml");
	linphone_event_add_custom_header(lev,"Content-Disposition", "recipient-list");
	linphone_event_add_custom_header(lev,"Require", "recipient-list-subscribe");

	linphone_event_send_subscribe(lev,content);

	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionOutgoingInit,1,1000));
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionActive,1,5000));

	/*make sure marie receives first notification before terminating*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_NotifyReceived,1,5000));
	/*dummy wait to avoid derred notify*/
	wait_for_list(lcs,&dummy,1,2000);
	proxy_config = linphone_core_get_default_proxy_config(pauline->lc);
	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_enable_publish(proxy_config,TRUE);
	linphone_proxy_config_done(proxy_config);

	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_NotifyReceived,2,5000));

	proxy_config = linphone_core_get_default_proxy_config(laure->lc);
	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_enable_publish(proxy_config,TRUE);
	linphone_proxy_config_done(proxy_config);
	/*make sure notify is not sent "imadiatly but defered*/
	BC_ASSERT_FALSE(wait_for_list(lcs,&marie->stat.number_of_NotifyReceived,3,1000));

	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_NotifyReceived,3,5000));

	linphone_event_terminate(lev);

	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionTerminated,1,5000));

	ms_free(subscribe_content);
	linphone_address_destroy(list_name);
	linphone_content_unref(content);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
}

static void test_subscribe_on_wrong_dialog(void) {
	char *scen;
	FILE * sipp_out;
	/*just to get an identity*/
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	const LinphoneAuthInfo	*marie_auth = linphone_core_find_auth_info(marie->lc, NULL, linphone_address_get_username(marie->identity), NULL);
	LinphoneAddress *sip_example_org;

	scen = bc_tester_res("sipp/subscribe_on_wrong_dialog.xml");
	sip_example_org = linphone_core_manager_resolve(marie, marie->identity);
	sipp_out = sip_start(scen, linphone_address_get_username(marie->identity),linphone_auth_info_get_passwd(marie_auth), sip_example_org);
	linphone_address_destroy(sip_example_org);

	if (sipp_out) {
		/*wait for marie status*/
		BC_ASSERT_EQUAL(0, pclose(sipp_out),int,"%d");
	}

	linphone_core_manager_destroy(marie);
}


test_t flexisip_tests[] = {
	TEST_ONE_TAG("Subscribe forking", subscribe_forking, "LeaksMemory"),
	TEST_NO_TAG("Message forking", message_forking),
	TEST_NO_TAG("Message forking with unreachable recipients", message_forking_with_unreachable_recipients),
	TEST_NO_TAG("Message forking with all recipients unreachable", message_forking_with_all_recipients_unreachable),
	TEST_NO_TAG("Call forking", call_forking),
	TEST_NO_TAG("Call forking cancelled", call_forking_cancelled),
	TEST_NO_TAG("Call forking declined globaly", call_forking_declined_globaly),
	TEST_NO_TAG("Call forking declined localy", call_forking_declined_localy),
	TEST_NO_TAG("Call forking with urgent reply", call_forking_with_urgent_reply),
	TEST_NO_TAG("Call forking with push notification (single)", call_forking_with_push_notification_single),
	TEST_NO_TAG("Call forking with push notification (multiple)", call_forking_with_push_notification_multiple),
	TEST_NO_TAG("Call forking not responded", call_forking_not_responded),
	TEST_NO_TAG("Early-media call forking", early_media_call_forking),
	TEST_NO_TAG("Call with sips", call_with_sips),
	TEST_ONE_TAG("Call with sips not achievable", call_with_sips_not_achievable, "LeaksMemory"),
	TEST_NO_TAG("Call with ipv6", call_with_ipv6),
	TEST_ONE_TAG("Subscribe Notify with sipp publisher", test_subscribe_notify_with_sipp_publisher, "LeaksMemory"),
	/*TEST_ONE_TAG("Subscribe Notify with sipp double publish", test_subscribe_notify_with_sipp_publisher_double_publish, "LeaksMemory"),*/
	TEST_NO_TAG("Publish/unpublish", test_publish_unpublish),
	TEST_ONE_TAG("List subscribe", test_list_subscribe,"LeaksMemory"),
	TEST_NO_TAG("File transfer message rcs to external body client", file_transfer_message_rcs_to_external_body_client),
	TEST_ONE_TAG("File transfer message external body to rcs client", file_transfer_message_external_body_to_rcs_client, "LeaksMemory"),
	TEST_ONE_TAG("File transfer message external body to external body client", file_transfer_message_external_body_to_external_body_client, "LeaksMemory"),
	TEST_NO_TAG("DoS module trigger by sending a lot of chat messages", dos_module_trigger),
	TEST_NO_TAG("Subscribe on wrong dialog", test_subscribe_on_wrong_dialog)
};

test_suite_t flexisip_test_suite = {"Flexisip", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
									sizeof(flexisip_tests) / sizeof(flexisip_tests[0]), flexisip_tests};
