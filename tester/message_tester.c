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
	char* from=linphone_address_as_string(from_address);
	ms_message("Message from [%s]  is [%s]",from,message);
	ms_free(from);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_LinphoneMessageReceived++;
}


static void text_message() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

int message_test_suite () {
	CU_pSuite 	pSuite = CU_add_suite("Message", NULL, NULL);
	if (NULL == CU_add_test(pSuite, "text_message", text_message)) {
			return CU_get_error();
	}
	return 0;
}
