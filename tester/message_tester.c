/*
	belle-sip - SIP (RFC3261) library.
    Copyright (C) 2010  Belledonne Communications SARL

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
#include <stdio.h>
#include "CUnit/Basic.h"
#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"

void text_message_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from_address, const char *message) {
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_LinphoneMessageReceivedLegacy++;
}
void message_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage* message) {
	char* from=linphone_address_as_string(linphone_chat_message_get_from(message));
	ms_message("Message from [%s]  is [%s] , external URL [%s]",from
																,linphone_chat_message_get_text(message)
																,linphone_chat_message_get_external_body_url(message));
	ms_free(from);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_LinphoneMessageReceived++;
	if (linphone_chat_message_get_external_body_url(message))
			counters->number_of_LinphoneMessageExtBodyReceived++;
}

void linphone_chat_message_state_change(LinphoneChatMessage* msg,LinphoneChatMessageState state,void* ud) {
	LinphoneCore* lc=(LinphoneCore*)ud;
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	ms_message("Message [%s] [%s]",linphone_chat_message_get_text(msg),linphone_chat_message_state_to_string(state));
	switch (state) {
	case LinphoneChatMessageStateDelivered:
		counters->number_of_LinphoneMessageDelivered++;
		break;
	case LinphoneChatMessageStateNotDelivered:
		counters->number_of_LinphoneMessageNotDelivered++;
		break;
	case LinphoneChatMessageStateInProgress:
		counters->number_of_LinphoneMessageInProgress++;
		break;
	default:
		ms_error("Unexpected state [%s] for message [%p]",linphone_chat_message_state_to_string(state),msg);
	}

}

static void text_message() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceivedLegacy,1);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_with_ack() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	linphone_chat_room_send_message2(chat_room,message,linphone_chat_message_state_change,pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageDelivered,1));
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void text_message_with_external_body() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	linphone_chat_message_set_external_body_url(message,"http://www.linphone.org");
	linphone_chat_room_send_message2(chat_room,message,linphone_chat_message_state_change,pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageDelivered,1));
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,1);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_with_send_error() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	char* to = linphone_address_as_string(pauline->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(marie->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	/*simultate a network error*/
	sal_set_send_error(marie->lc->sal, -1);
	linphone_chat_room_send_message2(chat_room,message,linphone_chat_message_state_change,marie->lc);

	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageNotDelivered,1));
	/*CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageInProgress,1);*/
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageReceived,0);

	sal_set_send_error(marie->lc->sal, 0);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

int message_test_suite () {
	CU_pSuite 	pSuite = CU_add_suite("Message", NULL, NULL);
	if (NULL == CU_add_test(pSuite, "text_message", text_message)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "text_message_with_ack", text_message_with_ack)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "text_message_with_send_error", text_message_with_send_error)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "text_message_with_external_body", text_message_with_external_body)) {
			return CU_get_error();
	}

	return 0;
}
