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

#include "cpim-chat-message-modifier.h"

#include "chat/cpim/cpim.h"
#include "content/content-type.h"
#include "content/content.h"
#include "address/address.h"
#include "logger/logger.h"
#include "chat/chat-message-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ChatMessageModifier::Result CpimChatMessageModifier::encode (shared_ptr<ChatMessage> message, int *errorCode) {
	Cpim::Message cpimMessage;
	Cpim::GenericHeader cpimContentTypeHeader;
	cpimContentTypeHeader.setName("Content-Type");
	cpimContentTypeHeader.setValue("Message/CPIM");
	cpimMessage.addCpimHeader(cpimContentTypeHeader);

	Content content;
	if (!message->getInternalContent().isEmpty()) {
		// Another ChatMessageModifier was called before this one, we apply our changes on the private content
		content = message->getInternalContent();
	} else {
		// We're the first ChatMessageModifier to be called, we'll create the private content from the public one
		// We take the first one because if there is more of them, the multipart modifier should have been called first
		// So we should not be in this block
		content = message->getContents().front();
	}

	string contentType = content.getContentType().asString();
	const vector<char> body = content.getBody();
	string contentBody(body.begin(), body.end());

	Cpim::GenericHeader contentTypeHeader;
	contentTypeHeader.setName("Content-Type");
	contentTypeHeader.setValue(contentType);
	cpimMessage.addContentHeader(contentTypeHeader);

	cpimMessage.setContent(contentBody);

	if (!cpimMessage.isValid()) {
		lError() << "[CPIM] Message is invalid: " << contentBody;
		*errorCode = 500;
		return ChatMessageModifier::Result::Error;
	} else {
		Content newContent;
		ContentType newContentType("Message/CPIM");
		newContent.setContentType(newContentType);
		newContent.setBody(cpimMessage.asString());
		message->setInternalContent(newContent);
	}
	return ChatMessageModifier::Result::Done;
}

ChatMessageModifier::Result CpimChatMessageModifier::decode (shared_ptr<ChatMessage> message, int *errorCode) {
	Content content;
	if (!message->getInternalContent().isEmpty()) {
		content = message->getInternalContent();
	} else {
		content = message->getContents().front();
	}

	if (content.getContentType() == ContentType::Cpim) {
		const vector<char> body = content.getBody();
		string contentBody(body.begin(), body.end());
		shared_ptr<const Cpim::Message> cpimMessage = Cpim::Message::createFromString(contentBody);
		if (cpimMessage && cpimMessage->isValid()) {
			Content newContent;
			ContentType newContentType(cpimMessage->getContentHeaders()->front()->getValue());
			newContent.setContentType(newContentType);
			newContent.setBody(cpimMessage->getContent());
			message->setInternalContent(newContent);
		} else {
			lError() << "[CPIM] Message is invalid: " << contentBody;
			*errorCode = 500;
			return ChatMessageModifier::Result::Error;
		}
	} else {
		lError() << "[CPIM] Message is not CPIM but " << content.getContentType().asString();
		return ChatMessageModifier::Result::Skipped;
	}
	return ChatMessageModifier::Result::Done;
}

LINPHONE_END_NAMESPACE
