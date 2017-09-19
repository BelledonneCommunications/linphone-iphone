/*
 * cpim-chat-message-modifier.cpp
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

#include "chat/chat-message-p.h"
#include "chat/cpim/cpim.h"
#include "content/content-type.h"
#include "content/content.h"

#include "cpim-chat-message-modifier.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

void CpimChatMessageModifier::encode (ChatMessagePrivate *messagePrivate) {
	Cpim::Message message;
	Cpim::GenericHeader cpimContentTypeHeader;
	cpimContentTypeHeader.setName("Content-Type");
	cpimContentTypeHeader.setValue("Message/CPIM");
	message.addCpimHeader(cpimContentTypeHeader);

	shared_ptr<Content> content;
	if (messagePrivate->internalContent) {
		// Another ChatMessageModifier was called before this one, we apply our changes on the private content
		content = messagePrivate->internalContent;
	} else {
		// We're the first ChatMessageModifier to be called, we'll create the private content from the public one
		// We take the first one because if there is more of them, the multipart modifier should have been called first
		// So we should not be in this block
		content = messagePrivate->contents.front();
	}

	string contentType = content->getContentType().asString();
	const vector<char> body = content->getBody();
	string contentBody(body.begin(), body.end());

	Cpim::GenericHeader contentTypeHeader;
	contentTypeHeader.setName("Content-Type");
	contentTypeHeader.setValue(contentType);
	message.addContentHeader(contentTypeHeader);

	message.setContent(contentBody);

	if (!message.isValid()) {
		//TODO
	} else {
		shared_ptr<Content> newContent = make_shared<Content>();
		ContentType newContentType("Message/CPIM");
		newContent->setContentType(newContentType);
		newContent->setBody(message.asString());
		messagePrivate->internalContent = newContent;
	}
}

void CpimChatMessageModifier::decode (ChatMessagePrivate *messagePrivate) {
	shared_ptr<Content> content;
	if (messagePrivate->internalContent) {
		content = messagePrivate->internalContent;
	} else {
		content = messagePrivate->contents.front();
	}

	ContentType contentType = content->getContentType();
	if (contentType.asString() == "Message/CPIM") {
		const vector<char> body = content->getBody();
		string contentBody(body.begin(), body.end());
		shared_ptr<const Cpim::Message> message = Cpim::Message::createFromString(contentBody);
		if (message && message->isValid()) {
			shared_ptr<Content> newContent = make_shared<Content>();
			ContentType newContentType(message->getContentHeaders()->front()->getValue());
			newContent->setContentType(newContentType);
			newContent->setBody(message->getContent());
		} else {
			//TODO
		}
	} else {
		//TODO
	}
}

LINPHONE_END_NAMESPACE
