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
#include "chat/chat-message/chat-message.h"
#include "chat/chat-room/basic-chat-room.h"
#include "chat/cpim/cpim.h"
#include "content/content-type.h"
#include "content/content.h"
#include "core/core.h"
#include "belr/grammarbuilder.h"
// TODO: Remove me later.
#include "private.h"

#include "liblinphone_tester.h"
#include "tester_utils.h"

// =============================================================================

using namespace std;

using namespace LinphonePrivate;

static void parse_minimal_message () {
	const string str = "Subject: the weather will be fine today\r\n"
		"\r\n"
		"Content-Type: text/plain; charset=utf-8\r\n"
		"\r\n";

	shared_ptr<const Cpim::Message> message = Cpim::Message::createFromString(str);
	if (!BC_ASSERT_PTR_NOT_NULL(message)) return;

	const string str2 = message->asString();
	BC_ASSERT_STRING_EQUAL(str2.c_str(), str.c_str());

	const string content = message->getContent();
	BC_ASSERT_STRING_EQUAL(content.c_str(), "");
}

static void set_generic_header_name () {
	const list<pair<string, bool> > entries = {
		// Reserved.
		{ "From", false },
		{ "To", false },
		{ "cc", false },
		{ "DateTime", false },
		{ "Subject", false },
		{ "NS", false },
		{ "Require", false }
	};

	for (const auto &entry : entries) {
		Cpim::GenericHeader genericHeader(entry.first, "");

		const string name = genericHeader.getName();
		BC_ASSERT_STRING_EQUAL(name.c_str(), "");
	}
}

static void check_core_header_names () {
	const list<pair<shared_ptr<Cpim::Header>, string> > entries = {
		{ make_shared<Cpim::FromHeader>(), "From" },
		{ make_shared<Cpim::ToHeader>(), "To" },
		{ make_shared<Cpim::CcHeader>(), "cc" },
		{ make_shared<Cpim::DateTimeHeader>(), "DateTime" },
		{ make_shared<Cpim::SubjectHeader>(), "Subject" },
		{ make_shared<Cpim::NsHeader>(), "NS" },
		{ make_shared<Cpim::RequireHeader>(), "Require" }
	};

	for (const auto &entry : entries) {
		const string name = entry.first->getName();
		BC_ASSERT_STRING_EQUAL(name.c_str(), entry.second.c_str());
	}
}

static void parse_rfc_example () {
	const string body = "<body>"
		"Here is the text of my message."
		"</body>";

	const string str = "From: \"MR SANDERS\"<im:piglet@100akerwood.com>\r\n"
		"To: \"Depressed Donkey\"<im:eeyore@100akerwood.com>\r\n"
		"DateTime: 2000-12-13T13:40:00-08:00\r\n"
		"Subject: the weather will be fine today\r\n"
		"Subject:;lang=fr beau temps prevu pour aujourd'hui\r\n"
		"NS: MyFeatures <mid:MessageFeatures@id.foo.com>\r\n"
		"Require: MyFeatures.VitalMessageOption\r\n"
		"MyFeatures.VitalMessageOption: Confirmation-requested\r\n"
		"MyFeatures.WackyMessageOption: Use-silly-font\r\n"
		"\r\n"
		"Content-Type: text/xml; charset=utf-8\r\n"
		"Content-ID: <1234567890@foo.com>\r\n"
		"\r\n" + body;

	shared_ptr<const Cpim::Message> message = Cpim::Message::createFromString(str);
	if (!BC_ASSERT_PTR_NOT_NULL(message)) return;

	const string str2 = message->asString();
	BC_ASSERT_STRING_EQUAL(str2.c_str(), str.c_str());

	string content = message->getContent();
	BC_ASSERT_STRING_EQUAL(content.c_str(), body.c_str());

	Cpim::Message::HeaderList list = message->getMessageHeaders();
	if (!BC_ASSERT_PTR_NOT_NULL(list)) return;
	BC_ASSERT_EQUAL(list->size(), 7, int, "%d");

	list = message->getMessageHeaders("MyFeatures");
	if (!BC_ASSERT_PTR_NOT_NULL(list)) return;
	BC_ASSERT_EQUAL(list->size(), 2, int, "%d");
}

static void parse_message_with_generic_header_parameters () {
	const string body = "<body>"
		"Here is the text of my message."
		"</body>";

	const string str = "From: \"MR SANDERS\"<im:piglet@100akerwood.com>\r\n"
		"Test:;aaa=bbb;yes=no CheckMe\r\n"
		"yaya: coucou\r\n"
		"yepee:;good=bad ugly\r\n"
		"\r\n"
		"Content-Type: text/xml; charset=utf-8\r\n"
		"Content-ID: <1234567890@foo.com>\r\n"
		"\r\n" + body;

	shared_ptr<const Cpim::Message> message = Cpim::Message::createFromString(str);
	if (!BC_ASSERT_PTR_NOT_NULL(message)) return;

	const string str2 = message->asString();
	BC_ASSERT_STRING_EQUAL(str2.c_str(), str.c_str());

	string content = message->getContent();
	BC_ASSERT_STRING_EQUAL(content.c_str(), body.c_str());
}

static void build_message () {

	Cpim::Message message;

	// Set message headers.
	Cpim::FromHeader fromHeader("im:piglet@100akerwood.com", "MR SANDERS");

	Cpim::ToHeader toHeader("im:eeyore@100akerwood.com", "Depressed Donkey");

	// 976686000 is 2000-12-13T13:40:00-08:00
	Cpim::DateTimeHeader dateTimeHeader(976686000);
	BC_ASSERT_EQUAL((int)dateTimeHeader.getTime(), 976686000, int, "%d");

	Cpim::SubjectHeader subjectHeader("the weather will be fine today");

	Cpim::SubjectHeader subjectWithLanguageHeader("beau temps prevu pour aujourd'hui", "fr");

	Cpim::NsHeader nsHeader("mid:MessageFeatures@id.foo.com", "MyFeatures");

	Cpim::RequireHeader requireHeader("MyFeatures.VitalMessageOption");

	Cpim::GenericHeader vitalMessageHeader("MyFeatures.VitalMessageOption", "Confirmation-requested");

	Cpim::GenericHeader wackyMessageHeader("MyFeatures.WackyMessageOption", "Use-silly-font");

	if (!BC_ASSERT_TRUE(message.addMessageHeader(fromHeader))) return;
	if (!BC_ASSERT_TRUE(message.addMessageHeader(toHeader))) return;
	if (!BC_ASSERT_TRUE(message.addMessageHeader(dateTimeHeader))) return;
	if (!BC_ASSERT_TRUE(message.addMessageHeader(subjectHeader))) return;
	if (!BC_ASSERT_TRUE(message.addMessageHeader(subjectWithLanguageHeader))) return;
	if (!BC_ASSERT_TRUE(message.addMessageHeader(nsHeader))) return;
	if (!BC_ASSERT_TRUE(message.addMessageHeader(requireHeader))) return;
	if (!BC_ASSERT_TRUE(message.addMessageHeader(vitalMessageHeader))) return;
	if (!BC_ASSERT_TRUE(message.addMessageHeader(wackyMessageHeader))) return;

	// Set Content headers.
    Cpim::GenericHeader contentTypeHeader("Content-Type", "text/xml; charset=utf-8");
	if (!BC_ASSERT_TRUE(message.addContentHeader(contentTypeHeader))) return;

	Cpim::GenericHeader contentIdHeader("Content-ID", "<1234567890@foo.com>");
	if (!BC_ASSERT_TRUE(message.addContentHeader(contentIdHeader))) return;

	// Add a wrong message header and a wrong content header
	Cpim::FromHeader wrongFromHeader("", "");
	if (!BC_ASSERT_FALSE(message.addMessageHeader(wrongFromHeader))) return;

	Cpim::GenericHeader wrongContentHeader("", "");
	if (!BC_ASSERT_FALSE(message.addContentHeader(wrongContentHeader))) return;

	const string content = "<body>"
		"Here is the text of my message."
		"</body>";

	if (!BC_ASSERT_TRUE(message.setContent(content))) return;

	const string strMessage = message.asString();
	const string expectedMessage = "From: \"MR SANDERS\"<im:piglet@100akerwood.com>\r\n"
		"To: \"Depressed Donkey\"<im:eeyore@100akerwood.com>\r\n"
		"DateTime: 2000-12-13T05:40:00Z\r\n"
		"Subject: the weather will be fine today\r\n"
		"Subject:;lang=fr beau temps prevu pour aujourd'hui\r\n"
		"NS: MyFeatures <mid:MessageFeatures@id.foo.com>\r\n"
		"Require: MyFeatures.VitalMessageOption\r\n"
		"MyFeatures.VitalMessageOption: Confirmation-requested\r\n"
		"MyFeatures.WackyMessageOption: Use-silly-font\r\n"
		"\r\n"
		"Content-Type: text/xml; charset=utf-8\r\n"
		"Content-ID: <1234567890@foo.com>\r\n"
		"\r\n"
		"<body>"
		"Here is the text of my message."
		"</body>";

	BC_ASSERT_STRING_EQUAL(strMessage.c_str(), expectedMessage.c_str());
}

static int fake_im_encryption_engine_process_incoming_message_cb (
	LinphoneImEncryptionEngine *engine,
	LinphoneChatRoom *room,
	LinphoneChatMessage *msg
) {
	// Encryption is the first receiving step, so this message should be CPIM.
	const string expected = ContentType::Cpim.asString();
	BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_content_type(msg), expected.c_str());
	return -1;
}

static int fake_im_encryption_engine_process_outgoing_message_cb (
	LinphoneImEncryptionEngine *engine,
	LinphoneChatRoom *room,
	LinphoneChatMessage *msg
) {
	// Encryption is the last sending step, so this message should be CPIM.
	const string expected = ContentType::Cpim.asString();
	BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_content_type(msg), expected.c_str());
	return -1;
}

static void cpim_chat_message_modifier_base (bool useMultipart) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	// We use a fake encryption engine just to check the internal content type during the sending/receiving process.
	LinphoneImEncryptionEngine *marie_imee = linphone_im_encryption_engine_new();
	LinphoneImEncryptionEngineCbs *marie_cbs = linphone_im_encryption_engine_get_callbacks(marie_imee);
	LinphoneImEncryptionEngine *pauline_imee = linphone_im_encryption_engine_new();
	LinphoneImEncryptionEngineCbs *pauline_cbs = linphone_im_encryption_engine_get_callbacks(pauline_imee);
	linphone_im_encryption_engine_cbs_set_process_outgoing_message(marie_cbs, fake_im_encryption_engine_process_outgoing_message_cb);
	linphone_im_encryption_engine_cbs_set_process_incoming_message(pauline_cbs, fake_im_encryption_engine_process_incoming_message_cb);
	linphone_core_set_im_encryption_engine(marie->lc, marie_imee);
	linphone_core_set_im_encryption_engine(pauline->lc, pauline_imee);

	char *paulineUri = linphone_address_as_string_uri_only(pauline->identity);
	IdentityAddress paulineAddress(paulineUri);
	bctbx_free(paulineUri);

	shared_ptr<AbstractChatRoom> marieRoom = marie->lc->cppPtr->getOrCreateBasicChatRoom(paulineAddress);
	marieRoom->allowCpim(true);

	shared_ptr<ChatMessage> marieMessage = marieRoom->createChatMessage("Hello CPIM");
	if (useMultipart) {
		marieRoom->allowMultipart(true);
		Content *content = new Content();
		content->setContentType(ContentType::PlainText);
		content->setBody("Hello Part 2");
		marieMessage->addContent(content);
	}
	marieMessage->send();

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageReceived,1));
	BC_ASSERT_TRUE(marieMessage->getInternalContent().getContentType().isEmpty()); // Internal content is cleaned after message is sent or received

	BC_ASSERT_PTR_NOT_NULL(pauline->stat.last_received_chat_message);
	if (pauline->stat.last_received_chat_message != NULL) {
		BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_text(pauline->stat.last_received_chat_message), "Hello CPIM");
		const string expected = ContentType::PlainText.asString();
		BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_content_type(pauline->stat.last_received_chat_message), expected.c_str());
	}

	marieMessage.reset();
	marieRoom.reset();

	linphone_im_encryption_engine_unref(marie_imee);
	linphone_im_encryption_engine_unref(pauline_imee);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void cpim_chat_message_modifier () {
	cpim_chat_message_modifier_base(FALSE);
}

static void cpim_chat_message_modifier_with_multipart_body () {
	cpim_chat_message_modifier_base(TRUE);
}

test_t cpim_tests[] = {
	TEST_NO_TAG("Parse minimal CPIM message", parse_minimal_message),
	TEST_NO_TAG("Set generic header name", set_generic_header_name),
	TEST_NO_TAG("Check core header names", check_core_header_names),
	TEST_NO_TAG("Parse RFC example", parse_rfc_example),
	TEST_NO_TAG("Parse Message with generic header parameters", parse_message_with_generic_header_parameters),
	TEST_NO_TAG("Build Message", build_message),
	TEST_NO_TAG("CPIM chat message modifier", cpim_chat_message_modifier),
	TEST_NO_TAG("CPIM chat message modifier with multipart body", cpim_chat_message_modifier_with_multipart_body)
};

static int suite_begin(void) {
	//Supposed to be done by platform helper, but in this case, we don't have it"
	belr::GrammarLoader::get().addPath(std::string(bc_tester_get_resource_dir_prefix()).append("/share/belr/grammars"));
	return 0;
}
test_suite_t cpim_test_suite = {
	"Cpim", suite_begin, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(cpim_tests) / sizeof(cpim_tests[0]), cpim_tests
};
