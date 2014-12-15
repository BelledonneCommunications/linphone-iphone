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
#include "lpconfig.h"
#include "private.h"
#include "liblinphone_tester.h"

static void subscribe_forking(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* pauline2 = linphone_core_manager_new( "pauline_tcp_rc");
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
	
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionOutgoingInit,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneSubscriptionIncomingReceived,1,3000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline2->stat.number_of_LinphoneSubscriptionIncomingReceived,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneSubscriptionActive,1,1000));

	/*make sure marie receives first notification before terminating*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_NotifyReceived,1,1000));

	linphone_event_terminate(lev);

	linphone_content_unref(content);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(pauline2);
	ms_list_free(lcs);
}

static void message_forking(void) {
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");
	MSList* lcs=ms_list_append(NULL,marie->lc);
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	
	linphone_chat_room_send_message2(chat_room,message,liblinphone_tester_chat_message_state_change,pauline->lc);
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneMessageReceived,1,3000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneMessageReceived,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneMessageDelivered,1,1000));
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(pauline);
	ms_free(to);
	ms_list_free(lcs);
}

static void message_forking_with_unreachable_recipients(void) {
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie3 = linphone_core_manager_new( "marie_rc");
	MSList* lcs=ms_list_append(NULL,marie->lc);
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	lcs=ms_list_append(lcs,marie3->lc);
	
	/*marie2 and marie3 go offline*/
	linphone_core_set_network_reachable(marie2->lc,FALSE);
	linphone_core_set_network_reachable(marie3->lc,FALSE);
	
	linphone_chat_room_send_message2(chat_room,message,liblinphone_tester_chat_message_state_change,pauline->lc);
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneMessageReceived,1,3000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneMessageDelivered,1,1000));
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);
	CU_ASSERT_TRUE( marie2->stat.number_of_LinphoneMessageReceived==0);
	CU_ASSERT_TRUE( marie3->stat.number_of_LinphoneMessageReceived==0);
	/*marie 2 goes online */
	linphone_core_set_network_reachable(marie2->lc,TRUE);
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneMessageReceived,1,3000));
	
	/*wait a long time so that all transactions are expired*/
	wait_for_list(lcs,NULL,0,32000);
	
	/*marie 3 goes online now*/
	linphone_core_set_network_reachable(marie3->lc,TRUE);
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneMessageReceived,1,3000));
	
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(pauline);
	ms_free(to);
	ms_list_free(lcs);
}

static void message_forking_with_all_recipients_unreachable(void) {
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie3 = linphone_core_manager_new( "marie_rc");
	MSList* lcs=ms_list_append(NULL,marie->lc);
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	
	lcs=ms_list_append(lcs,pauline->lc);
	lcs=ms_list_append(lcs,marie2->lc);
	lcs=ms_list_append(lcs,marie3->lc);
	
	/*All marie's device go offline*/
	linphone_core_set_network_reachable(marie->lc,FALSE);
	linphone_core_set_network_reachable(marie2->lc,FALSE);
	linphone_core_set_network_reachable(marie3->lc,FALSE);
	
	linphone_chat_room_send_message2(chat_room,message,liblinphone_tester_chat_message_state_change,pauline->lc);
	
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneMessageInProgress,1,1000));
	/*flexisip will accept the message with 202 after 16 seconds*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneMessageDelivered,1,18000));
	CU_ASSERT_TRUE( marie->stat.number_of_LinphoneMessageReceived==0);
	CU_ASSERT_TRUE( marie2->stat.number_of_LinphoneMessageReceived==0);
	CU_ASSERT_TRUE( marie3->stat.number_of_LinphoneMessageReceived==0);
	
	/*marie 1 goes online */
	linphone_core_set_network_reachable(marie->lc,TRUE);
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneMessageReceived,1,3000));
	
	/*marie 2 goes online */
	linphone_core_set_network_reachable(marie2->lc,TRUE);
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneMessageReceived,1,3000));
	
	/*wait a long time so that all transactions are expired*/
	wait_for_list(lcs,NULL,0,32000);
	
	/*marie 3 goes online now*/
	linphone_core_set_network_reachable(marie3->lc,TRUE);
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneMessageReceived,1,3000));
	
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(pauline);
	ms_free(to);
	ms_list_free(lcs);
}

static void call_forking(void){
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
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
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,3000));
	/*all devices from Marie should be ringing*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	
	/*marie accepts the call on its first device*/
	linphone_core_accept_call(marie->lc,linphone_core_get_current_call(marie->lc));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallConnected,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,1,1000));
	
	/*other devices should stop ringing*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallEnd,1,1000));
	
	linphone_core_terminate_call(pauline->lc,linphone_core_get_current_call(pauline->lc));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));
	
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(marie3);
	ms_list_free(lcs);
}

static void call_forking_with_urgent_reply(void){
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
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
	
	CU_ASSERT_TRUE(linphone_core_media_encryption_supported(pauline->lc,LinphoneMediaEncryptionSRTP));
	linphone_core_set_media_encryption(pauline->lc,LinphoneMediaEncryptionSRTP);
	linphone_core_set_network_reachable(marie2->lc,FALSE);
	linphone_core_set_network_reachable(marie3->lc,FALSE);
	
	linphone_core_invite_address(pauline->lc,marie->identity);
	/*pauline should hear ringback, after 5 seconds, when it will retry without SRTP*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,9000));
	/*Marie should be ringing*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	
	/*marie accepts the call on its first device*/
	linphone_core_accept_call(marie->lc,linphone_core_get_current_call(marie->lc));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallConnected,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,1,1000));
	
	linphone_core_terminate_call(pauline->lc,linphone_core_get_current_call(pauline->lc));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));
	
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(marie3);
	ms_list_free(lcs);
}

static void call_forking_cancelled(void){
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
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
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,3000));
	/*all devices from Marie should be ringing*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	
	/*pauline finally cancels the call*/
	linphone_core_terminate_call(pauline->lc,linphone_core_get_current_call(pauline->lc));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));
	
	/*all devices should stop ringing*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallEnd,1,1000));
	
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(marie3);
	ms_list_free(lcs);
}

static void call_forking_declined(bool_t declined_globaly){
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
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
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,3000));
	/*all devices from Marie should be ringing*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	
	/*marie1 finally declines the call*/
	linphone_core_decline_call(marie->lc,linphone_core_get_current_call(marie->lc),
		declined_globaly ? LinphoneReasonDeclined : LinphoneReasonBusy
	);
	
	if (declined_globaly){
		CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));
		/*all devices should stop ringing*/
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallEnd,1,1000));
	}else{
		/*pauline should continue ringing and be able to hear a call taken by marie2 */
		linphone_core_accept_call(marie2->lc, linphone_core_get_current_call(marie2->lc));
		CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,2000));
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallStreamsRunning,1,2000));
		liblinphone_tester_check_rtcp(pauline,marie2);
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,3000));
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallEnd,1,3000));
		linphone_core_terminate_call(marie2->lc,linphone_core_get_current_call(marie2->lc));
		CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,3000));
		CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,3000));
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
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	
	linphone_core_set_user_agent(marie->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);
	
	lcs=ms_list_append(NULL,pauline->lc);
	
	lcs=ms_list_append(lcs,marie->lc);
	
	/*unfortunately marie gets unreachable due to crappy 3G operator or iOS bug...*/
	linphone_core_set_network_reachable(marie->lc,FALSE);
	
	linphone_core_invite_address(pauline->lc,marie->identity);
	
	/*the server is expected to send a push notification to marie, this will wake up linphone, that will reconnect:*/
	linphone_core_set_network_reachable(marie->lc,TRUE);
	
	/*Marie shall receive the call immediately*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,5000));
	/*pauline should hear ringback as well*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,1000));
	
	/*marie accepts the call*/
	linphone_core_accept_call(marie->lc,linphone_core_get_current_call(marie->lc));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallConnected,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallStreamsRunning,1,1000));
	
	liblinphone_tester_check_rtcp(pauline,marie);
	
	linphone_core_terminate_call(pauline->lc,linphone_core_get_current_call(pauline->lc));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));
	
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	ms_list_free(lcs);
}

static void call_forking_with_push_notification_multiple(void){
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
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
	
	/*marie1 will ring*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,5000));
	/*pauline should hear ringback as well*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,1000));
	
	/*the server is expected to send a push notification to marie2, this will wake up linphone, that will reconnect:*/
	linphone_core_set_network_reachable(marie2->lc,TRUE);
	
	/*Marie shall receive the call immediately*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived,1,5000));
	
	/*marie2 accepts the call*/
	linphone_core_accept_call(marie2->lc,linphone_core_get_current_call(marie2->lc));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallConnected,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallConnected,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallStreamsRunning,1,1000));
	
	/*call to marie1 should be cancelled*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));
	
	liblinphone_tester_check_rtcp(pauline,marie2);
	
	linphone_core_terminate_call(pauline->lc,linphone_core_get_current_call(pauline->lc));
	
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));
	
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
}

void call_forking_not_responded(void){
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
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
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallOutgoingRinging,1,3000));
	/*all devices from Marie should be ringing*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallIncomingReceived,1,1000));
	
	/*nobody answers, flexisip should close the call after XX seconds*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallError,1,22000));
	/*all devices should stop ringing*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphoneCallEnd,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie3->stat.number_of_LinphoneCallEnd,1,1000));
	
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(marie3);
	ms_list_free(lcs);
}

static void early_media_call_forking(void) {
	LinphoneCoreManager* pauline = linphone_core_manager_new("pauline_tcp_rc");
	LinphoneCoreManager* marie1 = linphone_core_manager_new("marie_early_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new("marie_early_rc");
	MSList *lcs=NULL;
	LinphoneCallParams *params=linphone_core_create_default_call_parameters(pauline->lc);
	LinphoneVideoPolicy pol;
	LinphoneCall *marie1_call;
	LinphoneCall *marie2_call;
	LinphoneCall *pauline_call;
	int dummy=0;

	pol.automatically_accept=1;
	pol.automatically_initiate=1;
	
	linphone_core_set_user_agent(marie1->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(marie2->lc,"Natted Linphone",NULL);
	linphone_core_set_user_agent(pauline->lc,"Natted Linphone",NULL);
	
	linphone_core_enable_video(pauline->lc,TRUE,TRUE);
	
	linphone_core_enable_video(marie1->lc,TRUE,TRUE);
	linphone_core_set_video_policy(marie1->lc,&pol);
	
	linphone_core_enable_video(marie2->lc,TRUE,TRUE);
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

	CU_ASSERT_TRUE(wait_for_list(lcs, &marie1->stat.number_of_LinphoneCallIncomingEarlyMedia,1,3000));
	CU_ASSERT_TRUE(wait_for_list(lcs, &marie2->stat.number_of_LinphoneCallIncomingEarlyMedia,1,3000));
	CU_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallOutgoingEarlyMedia,1,3000));
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallOutgoingEarlyMedia,1);
	
	pauline_call=linphone_core_get_current_call(pauline->lc);
	marie1_call=linphone_core_get_current_call(marie1->lc);
	marie2_call=linphone_core_get_current_call(marie2->lc);
	
	/*wait a bit that streams are established*/
	wait_for_list(lcs,&dummy,1,3000);
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(pauline_call)->download_bandwidth>70
					&& linphone_call_get_audio_stats(pauline_call)->download_bandwidth<90);
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(marie1_call)->download_bandwidth>70
					&& linphone_call_get_audio_stats(marie1_call)->download_bandwidth<90);
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(marie2_call)->download_bandwidth>70
					&& linphone_call_get_audio_stats(marie2_call)->download_bandwidth<90);
	
	linphone_core_accept_call(marie1->lc,linphone_core_get_current_call(marie1->lc));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie1->stat.number_of_LinphoneCallStreamsRunning,1,3000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallStreamsRunning,1,3000));
	
	/*marie2 should get her call terminated*/
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie2->stat.number_of_LinphoneCallEnd,1,1000));
	
	/*wait a bit that streams are established*/
	wait_for_list(lcs,&dummy,1,3000);
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(pauline_call)->download_bandwidth>71
					&& linphone_call_get_audio_stats(pauline_call)->download_bandwidth<91 );
	CU_ASSERT_TRUE(linphone_call_get_audio_stats(marie1_call)->download_bandwidth>71
					&& linphone_call_get_audio_stats(marie1_call)->download_bandwidth<91 );
	
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphoneCallEnd,1,1000));
	CU_ASSERT_TRUE(wait_for_list(lcs,&marie1->stat.number_of_LinphoneCallEnd,1,1000));

	ms_list_free(lcs);
	linphone_core_manager_destroy(marie1);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(pauline);
}


test_t flexisip_tests[] = {
	{ "Subscribe forking", subscribe_forking },
	{ "Message forking", message_forking },
	{ "Message forking with unreachable recipients", message_forking_with_unreachable_recipients },
	{ "Message forking with all recipients unreachable", message_forking_with_all_recipients_unreachable},
	{ "Call forking", call_forking },
	{ "Call forking cancelled", call_forking_cancelled },
	{ "Call forking declined globaly", call_forking_declined_globaly },
	{ "Call forking declined localy", call_forking_declined_localy },
	{ "Call forking with urgent reply", call_forking_with_urgent_reply },
	{ "Call forking with push notification (single)", call_forking_with_push_notification_single },
	{ "Call forking with push notification (multiple)", call_forking_with_push_notification_multiple },
	{ "Call forking not responded", call_forking_not_responded },
	{ "Early-media call forking", early_media_call_forking },
};


test_suite_t flexisip_test_suite = {
	"Flexisip",
	NULL,
	NULL,
	sizeof(flexisip_tests) / sizeof(flexisip_tests[0]),
	flexisip_tests
};


