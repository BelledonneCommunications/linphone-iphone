/*
 * cpim-chat-message-modifier.cpp
 * Copyright (C) 2010-2017 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "address/address.h"
#include "chat/chat-message/chat-message-p.h"
#include "chat/cpim/cpim.h"
#include "content/content-type.h"
#include "content/content.h"
#include "logger/logger.h"

#include "cpim-chat-message-modifier.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ChatMessageModifier::Result CpimChatMessageModifier::encode (const shared_ptr<ChatMessage> &message, int &errorCode) {
	Cpim::Message cpimMessage;
	Cpim::GenericHeader cpimContentTypeHeader;
	cpimContentTypeHeader.setName("Content-Type");
	cpimContentTypeHeader.setValue(ContentType::Cpim.asString());
	cpimMessage.addCpimHeader(cpimContentTypeHeader);

	Cpim::FromHeader cpimFromHeader;
	cpimFromHeader.setValue(cpimAddressAsString(message->getFromAddress()));
	cpimMessage.addMessageHeader(cpimFromHeader);
	Cpim::ToHeader cpimToHeader;
	cpimToHeader.setValue(cpimAddressAsString(message->getToAddress()));
	cpimMessage.addMessageHeader(cpimToHeader);
	char token[13];
	belle_sip_random_token(token, sizeof(token));
	Cpim::MessageIdHeader cpimMessageIdHeader;
	cpimMessageIdHeader.setValue(token);
	cpimMessage.addMessageHeader(cpimMessageIdHeader);
	message->setImdnMessageId(token);

	const Content *content;
	if (!message->getInternalContent().isEmpty()) {
		// Another ChatMessageModifier was called before this one, we apply our changes on the private content
		content = &(message->getInternalContent());
	} else {
		// We're the first ChatMessageModifier to be called, we'll create the private content from the public one
		// We take the first one because if there is more of them, the multipart modifier should have been called first
		// So we should not be in this block
		content = message->getContents().front();
	}

	Cpim::GenericHeader contentTypeHeader;
	contentTypeHeader.setName("Content-Type");
	contentTypeHeader.setValue(content->getContentType().asString());
	cpimMessage.addContentHeader(contentTypeHeader);

	const string contentBody = content->getBodyAsString();
	cpimMessage.setContent(contentBody);

	if (!cpimMessage.isValid()) {
		lError() << "[CPIM] Message is invalid: " << contentBody;
		errorCode = 500;
		return ChatMessageModifier::Result::Error;
	}

	Content newContent;
	newContent.setContentType(ContentType("Message/CPIM"));
	newContent.setBody(cpimMessage.asString());
	message->setInternalContent(newContent);

	return ChatMessageModifier::Result::Done;
}

ChatMessageModifier::Result CpimChatMessageModifier::decode (const shared_ptr<ChatMessage> &message, int &errorCode) {
	const Content *content;
	if (!message->getInternalContent().isEmpty())
		content = &(message->getInternalContent());
	else
		content = message->getContents().front();

	if (content->getContentType() != ContentType::Cpim) {
		lError() << "[CPIM] Message is not CPIM but " << content->getContentType().asString();
		return ChatMessageModifier::Result::Skipped;
	}

	const string contentBody = content->getBodyAsString();
	const shared_ptr<const Cpim::Message> cpimMessage = Cpim::Message::createFromString(contentBody);
	if (!cpimMessage || !cpimMessage->isValid()) {
		lError() << "[CPIM] Message is invalid: " << contentBody;
		errorCode = 500;
		return ChatMessageModifier::Result::Error;
	}

	Content newContent;
	bool contentTypeFound = false;
	Cpim::Message::HeaderList l = cpimMessage->getContentHeaders();
	if (l) {
		for (const auto &header : *l.get()) {
			if (header->getName() == "Content-Type") {
				contentTypeFound = true;
				newContent.setContentType(ContentType(header->getValue()));
				break;
			}
		}
	}
	if (!contentTypeFound) {
		lError() << "[CPIM] No Content-type for the content of the message";
		errorCode = 500;
		return ChatMessageModifier::Result::Error;
	}
	newContent.setBody(cpimMessage->getContent());

	Address cpimFromAddress;
	Address cpimToAddress;
	l = cpimMessage->getMessageHeaders();
	if (l) {
		for (const auto &header : *l.get()) {
			if (header->getName() == "From")
				cpimFromAddress = Address(header->getValue());
			else if (header->getName() == "To")
				cpimToAddress = Address(header->getValue());
			else if (header->getName() == "Message-Id")
				message->setImdnMessageId(header->getValue());
		}
	}

	// Modify the initial message since there was no error
	message->setInternalContent(newContent);
	if (cpimFromAddress.isValid())
		message->getPrivate()->forceFromAddress(cpimFromAddress);

	return ChatMessageModifier::Result::Done;
}

string CpimChatMessageModifier::cpimAddressAsString (const Address &addr) const {
	ostringstream os;
	if (!addr.getDisplayName().empty())
		os << addr.getDisplayName() << " ";
	os << "<" << addr.asStringUriOnly() << ">";
	return os.str();
}

LINPHONE_END_NAMESPACE
