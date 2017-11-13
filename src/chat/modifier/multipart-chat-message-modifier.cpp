/*
 * multipart-chat-message-modifier.cpp
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

// TODO: Remove me later.
#include "private.h"

#include "address/address.h"
#include "chat/chat-message/chat-message.h"
#include "chat/chat-room/chat-room.h"
#include "content/content-type.h"
#include "content/file-transfer-content.h"
#include "logger/logger.h"
#include "core/core.h"

#include "multipart-chat-message-modifier.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ChatMessageModifier::Result MultipartChatMessageModifier::encode (
	const shared_ptr<ChatMessage> &message,
	int &errorCode
) {
	if (message->getContents().size() <= 1)
		return ChatMessageModifier::Result::Skipped;

	LinphoneCore *lc = message->getChatRoom()->getCore()->getCCore();
	char tmp[64];
	lc->sal->create_uuid(tmp, sizeof(tmp));
	string boundary = tmp;
	stringstream multipartMessage;

	multipartMessage << "--" << boundary;
	for (Content *content : message->getContents()) {
		multipartMessage << "\r\n";
		multipartMessage << "Content-Type: " << content->getContentType().asString() << "\r\n\r\n";
		multipartMessage << content->getBodyAsString() << "\r\n\r\n";
		multipartMessage << "--" << boundary;
	}
	multipartMessage << "--";

	Content newContent;
	ContentType newContentType("multipart/mixed");
	newContentType.setParameter("boundary=" + boundary);
	newContent.setContentType(newContentType);
	newContent.setBody(multipartMessage.str());
	message->setInternalContent(newContent);

	return ChatMessageModifier::Result::Done;
}

ChatMessageModifier::Result MultipartChatMessageModifier::decode (const shared_ptr<ChatMessage> &message, int &errorCode) {
	if (message->getInternalContent().getContentType().getType() == "multipart") {
		string boundary = message->getInternalContent().getContentType().getParameter();
		if (boundary.empty()) {
			lError() << "Boundary parameter of content-type not found: " << message->getInternalContent().getContentType().asString();
			return ChatMessageModifier::Result::Error;
		}

		size_t pos = boundary.find("=");
		if (pos == string::npos) {
			lError() << "Parameter seems invalid: " << boundary;
			return ChatMessageModifier::Result::Error;
		}
		boundary = "--" + boundary.substr(pos + 1);
		lInfo() << "Multipart boundary is " << boundary;

		const vector<char> body = message->getInternalContent().getBody();
		string contentsString(body.begin(), body.end());

		pos = contentsString.find(boundary);
		if (pos == string::npos) {
			lError() << "Boundary not found in body !";
			return ChatMessageModifier::Result::Error;
		}

		size_t start = pos + boundary.length() + 2; // 2 is the size of \r\n
		size_t end;
		do {
			end = contentsString.find(boundary, start);
			if (end != string::npos) {
				string contentString = contentsString.substr(start, end - start);

				size_t contentTypePos = contentString.find(": ") + 2; // 2 is the size of :
				size_t endOfLinePos = contentString.find("\r\n");
				if (contentTypePos >= endOfLinePos) {
					lError() << "Content should start by a 'Content-Type: ' line !";
					continue;
				}
				string contentTypeString = contentString.substr(contentTypePos, endOfLinePos - contentTypePos);
				ContentType contentType(contentTypeString);

				endOfLinePos += 4; // 4 is two time the size of \r\n
				string contentBody = contentString.substr(endOfLinePos, contentString.length() - (endOfLinePos + 4)); // 4 is two time the size of \r\n

				Content *content;
				if (contentType == ContentType::FileTransfer) {
					content = new FileTransferContent();
				} else {
					content = new Content();
				}
				content->setContentType(contentType);
				content->setBody(contentBody);
				message->addContent(*content);

				lInfo() << "Parsed and added content with type " << contentType.asString();
			}
			start = end + boundary.length() + 2; // 2 is the size of \r\n
		} while (end != string::npos);

		return ChatMessageModifier::Result::Done;
	}
	return ChatMessageModifier::Result::Skipped;
}

LINPHONE_END_NAMESPACE
