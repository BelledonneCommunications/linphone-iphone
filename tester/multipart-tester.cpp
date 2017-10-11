/*
 * cpim-tester.cpp
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "address/address.h"
#include "chat/chat-room/basic-chat-room.h"
#include "chat/chat-message/chat-message.h"
#include "content/content-type.h"

#include "liblinphone_tester.h"

// =============================================================================

using namespace std;

using namespace LinphonePrivate;

static void chat_message_multipart_modifier_base(bool first_file_transfer, bool second_file_transfer) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	Address paulineAddress(linphone_address_as_string_uri_only(pauline->identity));
	shared_ptr<ChatRoom> marieRoom = ObjectFactory::create<BasicChatRoom>(marie->lc, paulineAddress);

	shared_ptr<ChatMessage> marieMessage;
	if (first_file_transfer) {
		//TODO
		marieMessage = marieRoom->createFileTransferMessage(NULL);
	} else {
		marieMessage = marieRoom->createMessage("Hello Part 1");
	}

	if (second_file_transfer) {
		//TODO
	} else {
		Content content;
		content.setContentType(ContentType::PlainText);
		content.setBody("Hello Part 2");
		marieMessage->addContent(content);
	}
	marieMessage->send();

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageReceived,1));
	BC_ASSERT_STRING_EQUAL(marieMessage->getInternalContent().getContentType().asString().c_str(), "multipart/mixed");

	BC_ASSERT_PTR_NOT_NULL(pauline->stat.last_received_chat_message);
	//TODO

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void multipart_two_text_content(void) {
	chat_message_multipart_modifier_base(false, false);
}

test_t multipart_tests[] = {
	TEST_NO_TAG("Chat message multipart 2 text content", multipart_two_text_content),
};

test_suite_t multipart_test_suite = {
	"Multipart", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(multipart_tests) / sizeof(multipart_tests[0]), multipart_tests
};
