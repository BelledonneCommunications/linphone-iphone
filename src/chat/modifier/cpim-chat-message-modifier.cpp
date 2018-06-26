/*
 * cpim-chat-message-modifier.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
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

#include "linphone/utils/utils.h"

#include "address/address.h"
#include "chat/chat-message/chat-message-p.h"
#include "chat/cpim/cpim.h"
#include "content/content.h"
#include "content/content-disposition.h"
#include "content/content-type.h"
#include "logger/logger.h"

#include "cpim-chat-message-modifier.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ChatMessageModifier::Result CpimChatMessageModifier::encode (const shared_ptr<ChatMessage> &message, int &errorCode) {
	Cpim::Message cpimMessage;

	cpimMessage.addMessageHeader(
		Cpim::FromHeader(cpimAddressUri(message->getFromAddress()), cpimAddressDisplayName(message->getFromAddress()))
	);
	cpimMessage.addMessageHeader(
		Cpim::ToHeader(cpimAddressUri(message->getToAddress()), cpimAddressDisplayName(message->getToAddress()))
	);
	cpimMessage.addMessageHeader(
		Cpim::DateTimeHeader(message->getTime())
	);

	if (message->getPrivate()->getPositiveDeliveryNotificationRequired()
		|| message->getPrivate()->getNegativeDeliveryNotificationRequired()
		|| message->getPrivate()->getDisplayNotificationRequired()
	) {
		const string imdnNamespace = "imdn";
		cpimMessage.addMessageHeader(Cpim::NsHeader("urn:ietf:params:imdn", imdnNamespace));

		char token[13];
		belle_sip_random_token(token, sizeof(token));
		cpimMessage.addMessageHeader(
			Cpim::GenericHeader("Message-ID", token) // TODO: Replace by imdnNamespace + ".Message-ID");
		);
		message->getPrivate()->setImdnMessageId(token);

		vector<string> dispositionNotificationValues;
		if (message->getPrivate()->getPositiveDeliveryNotificationRequired())
			dispositionNotificationValues.emplace_back("positive-delivery");
		if (message->getPrivate()->getNegativeDeliveryNotificationRequired())
			dispositionNotificationValues.emplace_back("negative-delivery");
		if (message->getPrivate()->getDisplayNotificationRequired())
			dispositionNotificationValues.emplace_back("display");
		cpimMessage.addMessageHeader(
			Cpim::GenericHeader(
				imdnNamespace + ".Disposition-Notification",
				Utils::join(dispositionNotificationValues, ", ")
			)
		);
	}

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

	const string contentBody = content->getBodyAsString();
	if (content->getContentDisposition().isValid()) {
		cpimMessage.addContentHeader(
			Cpim::GenericHeader("Content-Disposition", content->getContentDisposition().asString())
		);
	}
	cpimMessage.addContentHeader(
		Cpim::GenericHeader("Content-Type", content->getContentType().asString())
	);
	cpimMessage.addContentHeader(
		Cpim::GenericHeader("Content-Length", Utils::toString(contentBody.size()))
	);
	cpimMessage.setContent(contentBody);

	Content newContent;
	newContent.setContentType(ContentType::Cpim);
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
		lError() << "[CPIM] Message is not CPIM but " << content->getContentType();
		return ChatMessageModifier::Result::Skipped;
	}

	const string contentBody = content->getBodyAsString();
	const shared_ptr<const Cpim::Message> cpimMessage = Cpim::Message::createFromString(contentBody);
	if (!cpimMessage || !cpimMessage->getMessageHeader("From") || !cpimMessage->getMessageHeader("To")) {
		lError() << "[CPIM] Message is invalid: " << contentBody;
		errorCode = 488; // Not Acceptable
		return ChatMessageModifier::Result::Error;
	}

	Content newContent;
	auto contentTypeHeader = cpimMessage->getContentHeader("Content-Type");
	if (!contentTypeHeader) {
		lError() << "[CPIM] No Content-type for the content of the message";
		errorCode = 488; // Not Acceptable
		return ChatMessageModifier::Result::Error;
	}
	newContent.setContentType(ContentType(contentTypeHeader->getValue()));
	auto contentDispositionHeader = cpimMessage->getContentHeader("Content-Disposition");
	if (contentDispositionHeader)
		newContent.setContentDisposition(ContentDisposition(contentDispositionHeader->getValue()));
	newContent.setBody(cpimMessage->getContent());

	message->getPrivate()->setPositiveDeliveryNotificationRequired(false);
	message->getPrivate()->setNegativeDeliveryNotificationRequired(false);
	message->getPrivate()->setDisplayNotificationRequired(false);

	string imdnNamespace = "";
	auto messageHeaders = cpimMessage->getMessageHeaders();
	if (messageHeaders) {
		for (const auto &header : *messageHeaders.get()) {
			if (header->getName() != "NS")
				continue;
			auto nsHeader = static_pointer_cast<const Cpim::NsHeader>(header);
			if (nsHeader->getUri() == "urn:ietf:params:imdn") {
				imdnNamespace = nsHeader->getPrefixName();
				break;
			}
		}
	}

	auto fromHeader = static_pointer_cast<const Cpim::FromHeader>(cpimMessage->getMessageHeader("From"));
	Address cpimFromAddress(fromHeader->getValue());
	auto toHeader = static_pointer_cast<const Cpim::ToHeader>(cpimMessage->getMessageHeader("To"));
	Address cpimToAddress(toHeader->getValue());
	auto dateTimeHeader = static_pointer_cast<const Cpim::DateTimeHeader>(cpimMessage->getMessageHeader("DateTime"));
	if (dateTimeHeader)
		message->getPrivate()->setTime(dateTimeHeader->getTime());

	auto messageIdHeader = cpimMessage->getMessageHeader("Message-ID"); // TODO: For compatibility, to remove
	if (!imdnNamespace.empty()) {
		if (!messageIdHeader)
			messageIdHeader = cpimMessage->getMessageHeader("Message-ID", imdnNamespace);
		auto dispositionNotificationHeader = cpimMessage->getMessageHeader("Disposition-Notification", imdnNamespace);
		if (dispositionNotificationHeader) {
			vector<string> values = Utils::split(dispositionNotificationHeader->getValue(), ", ");
			for (const auto &value : values)
				if (value == "positive-delivery")
					message->getPrivate()->setPositiveDeliveryNotificationRequired(true);
				else if (value == "negative-delivery")
					message->getPrivate()->setNegativeDeliveryNotificationRequired(true);
				else if (value == "display")
					message->getPrivate()->setDisplayNotificationRequired(true);
		}
	}
	if (messageIdHeader)
		message->getPrivate()->setImdnMessageId(messageIdHeader->getValue());

	// Modify the initial message since there was no error
	message->setInternalContent(newContent);
	if (cpimFromAddress.isValid())
		message->getPrivate()->forceFromAddress(cpimFromAddress);

	return ChatMessageModifier::Result::Done;
}

string CpimChatMessageModifier::cpimAddressDisplayName (const Address &addr) const {
	return addr.getDisplayName();
}

string CpimChatMessageModifier::cpimAddressUri (const Address &addr) const {
	return addr.asStringUriOnly();
}

LINPHONE_END_NAMESPACE
