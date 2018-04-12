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

// TODO: Remove me later.
#include "private.h"

#include "liblinphone_tester.h"
#include "tester_utils.h"

// =============================================================================

using namespace std;

using namespace LinphonePrivate;

static void parse_minimal_message () {
	const string str = "Content-type: Message/CPIM\r\n"
		"\r\n"
		"Content-Type: text/plain; charset=utf-8\r\n"
		"\r\n";

	shared_ptr<const Cpim::Message> message = Cpim::Message::createFromString(str);
	if (!BC_ASSERT_PTR_NOT_NULL(message)) return;

	const string str2 = message->asString();
	BC_ASSERT_STRING_EQUAL(str2.c_str(), str.c_str());

	BC_ASSERT_STRING_EQUAL(message->getContent().c_str(), "");
}

static void set_generic_header_name () {
	const list<pair<string, bool> > entries = {
		{ "toto", true },
		{ "george.abitbol", true },
		{ "tata/titi", false },
		{ "hey ho", false },
		{ " fail", false },
		{ "fail2 ", false },

		// Reserved.
		{ "From", false },
		{ "To", false },
		{ "cc", false },
		{ "DateTime", false },
		{ "Subject", false },
		{ "NS", false },
		{ "Require", false },

		// Case sensitivity.
		{ "FROM", true },
		{ "to", true },
		{ "cC", true },
		{ "Datetime", true },
		{ "SuBject", true },
		{ "nS", true },
		{ "requirE", true }
	};

	for (const auto &entry : entries) {
		Cpim::GenericHeader genericHeader;

		const bool result = genericHeader.setName(entry.first);
		BC_ASSERT_EQUAL(result, entry.second, bool, "%d");

		const string name = genericHeader.getName();

		if (result)
			BC_ASSERT_STRING_EQUAL(name.c_str(), entry.first.c_str());
		else
			BC_ASSERT_STRING_EQUAL(name.c_str(), "");
	}
}

static void set_generic_header_value () {
	const list<pair<string, bool> > entries = {
		{ "MyFeatures <mid:MessageFeatures@id.foo.com>", true },
		{ "2000-12-13T13:40:00-08:00", true },
		{ "2000-12-13T13:40:00-08:00", true },
		{ "text/xml; charset=utf-8", true },
		{ "text/xml; charset=ut\r\nf-8", false }
	};

	for (const auto &entry : entries) {
		Cpim::GenericHeader genericHeader;

		const bool result = genericHeader.setValue(entry.first);
		BC_ASSERT_EQUAL(result, entry.second, bool, "%d");

		const string value = genericHeader.getValue();

		if (result)
			BC_ASSERT_STRING_EQUAL(value.c_str(), entry.first.c_str());
		else
			BC_ASSERT_STRING_EQUAL(value.c_str(), "");
	}
}

static void check_core_header_names () {
	const list<pair<shared_ptr<Cpim::CoreHeader>, string> > entries = {
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

static void set_core_header_values () {
	const list<pair<shared_ptr<Cpim::Header>, list<pair<string, bool> > > > entries = {
		{ make_shared<Cpim::FromHeader>(), {
				{ "Winnie the Pooh <im:pooh@100akerwood.com>", true },
				{ "<im:tigger@100akerwood.com>", true },
				{ "<im:tigger@100akerwood.com", false },
				{ "<im:tigger>", true },
				{ "toto", false }
			} },
		{ make_shared<Cpim::ToHeader>(), {
				{ "<im:tigger@100akerwood.com", false },
				{ "Winnie the Pooh <im:pooh@100akerwood.com>", true },
				{ "toto", false },
				{ "<im:tigger>", true },
				{ "<im:tigger@100akerwood.com>", true }
			} },
		{ make_shared<Cpim::CcHeader>(), {
				{ "<im:tigger@100akerwood.com>", true },
				{ "<im:tigger@100akerwood.com", false },
				{ "Winnie the Pooh <im:pooh@100akerwood.com>", true },
				{ "<im:tigger>", true },
				{ "toto", false }
			} },
		{ make_shared<Cpim::DateTimeHeader>(), {
				{ "abcd", false },
				{ "1985-04-12T23:20:50.52Z", true },
				{ "1996-12-19T16:39:57-08:00", true },
				{ "1990-12-31T23:59:60Z", true },
				{ "1990-12-31T15:59:60-08:00", true },
				{ "2001-02-29T10:10:10Z", false },
				{ "2000-02-29T10:10:10Z", true },
				{ "1937-01-01T12:00:27.87+00:20", true },
				{ "1937-01-01T12:00:27.87Z", true },
				{ "1956", false }
			} },
		{ make_shared<Cpim::SubjectHeader>(), {
				{ "Eeyore's feeling very depressed today", true },
				{ "🤣", true },
				{ "hello", true }
			} },
		{ make_shared<Cpim::NsHeader>(), {
				{ "MyAlias <mid:MessageFeatures@id.foo.com>", true },
				{ "What is this? - Barry Burton", false },
				{ "<mid:MessageFeatures@id.foo.com>", true },
				{ "<mid:MessageFeatures@id.foo.com", false }
			} },
		{ make_shared<Cpim::RequireHeader>(), {
				{ "MyAlias.VitalHeader", true },
				{ "MyAlias.VitalHeader,Test", true },
				{ "MyAlias.VitalHeader,🤣", false }
			} }
	};

	for (const auto &entry : entries) {
		const shared_ptr<Cpim::Header> header = entry.first;
		string previousValue;

		for (const auto &test : entry.second) {
			const bool result = header->setValue(test.first);
			BC_ASSERT_EQUAL(result, test.second, bool, "%d");

			const string value = header->getValue();

			if (result)
				BC_ASSERT_STRING_EQUAL(value.c_str(), test.first.c_str());
			else
				BC_ASSERT_STRING_EQUAL(value.c_str(), previousValue.c_str());

			previousValue = value;
		}
	}
}

static void check_subject_header_language () {
	Cpim::SubjectHeader subjectHeader;

	// Check for not defined language.
	{
		const string language = subjectHeader.getLanguage();
		BC_ASSERT_STRING_EQUAL(language.c_str(), "");
	}

	// Set valid language.
	{
		const string languageToSet = "fr";

		BC_ASSERT_TRUE(subjectHeader.setLanguage(languageToSet));
		BC_ASSERT_TRUE(languageToSet == subjectHeader.getLanguage());

		const string str = subjectHeader.asString();
		const string expected = "Subject:;lang=" + languageToSet + " \r\n";
		BC_ASSERT_STRING_EQUAL(str.c_str(), expected.c_str());
	}

	// Set invalid language.
	{
		const string languageToSet = "fr--";
		BC_ASSERT_FALSE(subjectHeader.setLanguage(languageToSet));
		BC_ASSERT_FALSE(languageToSet == subjectHeader.getLanguage());
		BC_ASSERT_FALSE(subjectHeader.isValid());
	}
}

static void parse_rfc_example () {
	const string body = "<body>"
		"Here is the text of my message."
		"</body>";

	const string str = "Content-type: Message/CPIM\r\n"
		"\r\n"
		"From: MR SANDERS <im:piglet@100akerwood.com>\r\n"
		"To: Depressed Donkey <im:eeyore@100akerwood.com>\r\n"
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
}

static void parse_message_with_generic_header_parameters () {
	const string body = "<body>"
		"Here is the text of my message."
		"</body>";

	const string str = "Content-type: Message/CPIM\r\n"
		"\r\n"
		"From: MR SANDERS <im:piglet@100akerwood.com>\r\n"
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
	if (!BC_ASSERT_FALSE(message.isValid()))
		return;

	// Set CPIM headers.
	Cpim::GenericHeader cpimContentTypeHeader;
	if (!BC_ASSERT_TRUE(cpimContentTypeHeader.setName("Content-Type"))) return;
	if (!BC_ASSERT_TRUE(cpimContentTypeHeader.setValue("Message/CPIM"))) return;

	if (!BC_ASSERT_TRUE(message.addCpimHeader(cpimContentTypeHeader))) return;

	// Set message headers.
	Cpim::FromHeader fromHeader;
	if (!BC_ASSERT_TRUE(fromHeader.setValue("MR SANDERS <im:piglet@100akerwood.com>"))) return;

	Cpim::ToHeader toHeader;
	if (!BC_ASSERT_TRUE(toHeader.setValue("Depressed Donkey <im:eeyore@100akerwood.com>"))) return;

	Cpim::DateTimeHeader dateTimeHeader;
	if (!BC_ASSERT_TRUE(dateTimeHeader.setValue("2000-12-13T13:40:00-08:00"))) return;

	Cpim::SubjectHeader subjectHeader;
	if (!BC_ASSERT_TRUE(subjectHeader.setValue("the weather will be fine today"))) return;

	Cpim::SubjectHeader subjectWithLanguageHeader;
	if (!BC_ASSERT_TRUE(subjectWithLanguageHeader.setValue("beau temps prevu pour aujourd'hui"))) return;
	if (!BC_ASSERT_TRUE(subjectWithLanguageHeader.setLanguage("fr"))) return;

	Cpim::NsHeader nsHeader;
	if (!BC_ASSERT_TRUE(nsHeader.setValue("MyFeatures <mid:MessageFeatures@id.foo.com>"))) return;

	Cpim::RequireHeader requireHeader;
	if (!BC_ASSERT_TRUE(requireHeader.setValue("MyFeatures.VitalMessageOption"))) return;

	Cpim::GenericHeader vitalMessageHeader;
	if (!BC_ASSERT_TRUE(vitalMessageHeader.setName("MyFeatures.VitalMessageOption"))) return;
	if (!BC_ASSERT_TRUE(vitalMessageHeader.setValue("Confirmation-requested"))) return;

	Cpim::GenericHeader wackyMessageHeader;
	if (!BC_ASSERT_TRUE(wackyMessageHeader.setName("MyFeatures.WackyMessageOption"))) return;
	if (!BC_ASSERT_TRUE(wackyMessageHeader.setValue("Use-silly-font"))) return;

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
    Cpim::GenericHeader contentTypeHeader;
	if (!BC_ASSERT_TRUE(contentTypeHeader.setName("Content-Type"))) return;
	if (!BC_ASSERT_TRUE( contentTypeHeader.setValue("text/xml; charset=utf-8"))) return;
	if (!BC_ASSERT_TRUE(message.addContentHeader(contentTypeHeader))) return;

    Cpim::GenericHeader contentIdHeader;
	if (!BC_ASSERT_TRUE(contentIdHeader.setName("Content-ID"))) return;
	if (!BC_ASSERT_TRUE( contentIdHeader.setValue("<1234567890@foo.com>"))) return;
    if (!BC_ASSERT_TRUE(message.addContentHeader(contentIdHeader))) return;

	const string content = "<body>"
		"Here is the text of my message."
		"</body>";

	if (!BC_ASSERT_TRUE(message.setContent(content))) return;
	if (!BC_ASSERT_TRUE(message.isValid())) return;

	const string strMessage = message.asString();
	const string expectedMessage = "Content-Type: Message/CPIM\r\n"
		"\r\n"
		"From: MR SANDERS <im:piglet@100akerwood.com>\r\n"
		"To: Depressed Donkey <im:eeyore@100akerwood.com>\r\n"
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
		"\r\n"
		"<body>"
		"Here is the text of my message."
		"</body>";

	BC_ASSERT_STRING_EQUAL(strMessage.c_str(), expectedMessage.c_str());
}

static int fake_im_encryption_engine_process_incoming_message_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg) {
	BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_content_type(msg), ContentType::Cpim.asString().c_str()); // Encryption is the first receiving step, so this message should be CPIM
	return -1;
}

static int fake_im_encryption_engine_process_outgoing_message_cb(LinphoneImEncryptionEngine *engine, LinphoneChatRoom *room, LinphoneChatMessage *msg) {
	BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_content_type(msg), ContentType::Cpim.asString().c_str()); // Encryption is the last sending step, so this message should be CPIM
	return -1;
}

static void cpim_chat_message_modifier_base(bool_t use_multipart) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	// We use a fake encryption engine just to check the internal content type during the sending/receiving process
	LinphoneImEncryptionEngine *marie_imee = linphone_im_encryption_engine_new();
	LinphoneImEncryptionEngineCbs *marie_cbs = linphone_im_encryption_engine_get_callbacks(marie_imee);
	LinphoneImEncryptionEngine *pauline_imee = linphone_im_encryption_engine_new();
	LinphoneImEncryptionEngineCbs *pauline_cbs = linphone_im_encryption_engine_get_callbacks(pauline_imee);
	linphone_im_encryption_engine_cbs_set_process_outgoing_message(marie_cbs, fake_im_encryption_engine_process_outgoing_message_cb);
	linphone_im_encryption_engine_cbs_set_process_incoming_message(pauline_cbs, fake_im_encryption_engine_process_incoming_message_cb);
	linphone_core_set_im_encryption_engine(marie->lc, marie_imee);
	linphone_core_set_im_encryption_engine(pauline->lc, pauline_imee);

	IdentityAddress paulineAddress(linphone_address_as_string_uri_only(pauline->identity));
	shared_ptr<AbstractChatRoom> marieRoom = marie->lc->cppPtr->getOrCreateBasicChatRoom(paulineAddress);
	marieRoom->allowCpim(true);

	shared_ptr<ChatMessage> marieMessage = marieRoom->createChatMessage("Hello CPIM");
	if (use_multipart) {
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
		BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_content_type(pauline->stat.last_received_chat_message), ContentType::PlainText.asString().c_str());
	}

	linphone_im_encryption_engine_unref(marie_imee);
	linphone_im_encryption_engine_unref(pauline_imee);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void cpim_chat_message_modifier(void) {
	cpim_chat_message_modifier_base(FALSE);
}

static void cpim_chat_message_modifier_with_multipart_body(void) {
	cpim_chat_message_modifier_base(TRUE);
}

test_t cpim_tests[] = {
	TEST_NO_TAG("Parse minimal CPIM message", parse_minimal_message),
	TEST_NO_TAG("Set generic header name", set_generic_header_name),
	TEST_NO_TAG("Set generic header value", set_generic_header_value),
	TEST_NO_TAG("Check core header names", check_core_header_names),
	TEST_NO_TAG("Set core header values", set_core_header_values),
	TEST_NO_TAG("Check Subject header language", check_subject_header_language),
	TEST_NO_TAG("Parse RFC example", parse_rfc_example),
	TEST_NO_TAG("Parse Message with generic header parameters", parse_message_with_generic_header_parameters),
	TEST_NO_TAG("Build Message", build_message),
	TEST_NO_TAG("CPIM chat message modifier", cpim_chat_message_modifier),
	TEST_NO_TAG("CPIM chat message modifier with multipart body", cpim_chat_message_modifier_with_multipart_body),
};

test_suite_t cpim_test_suite = {
	"Cpim", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(cpim_tests) / sizeof(cpim_tests[0]), cpim_tests
};
