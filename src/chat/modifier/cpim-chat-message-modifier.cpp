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

 #include <vector>

 #include "content/content-type.h"
 #include "content/content.h"
 #include "chat/chat-message-p.h"
 #include "chat/cpim/cpim.h"

 #include "cpim-chat-message-modifier.h"

 LINPHONE_BEGIN_NAMESPACE
 
 using namespace std;

 void CpimChatMessageModifier::encode(LinphonePrivate::ChatMessagePrivate* msg) {
    Cpim::Message message;
    Cpim::GenericHeader contentTypeHeader;
	contentTypeHeader.setName("Content-Type");
    contentTypeHeader.setValue("Message/CPIM");
    message.addCpimHeader(contentTypeHeader);
    
    shared_ptr<Content> content;
    if (msg->internalContent) {
        // Another ChatMessageModifier was called before this one, we apply our changes on the private content
        content = msg->internalContent;
    } else {
        // We're the first ChatMessageModifier to be called, we'll create the private content from the public one
        // We take the first one because if there is more of them, the multipart modifier should have been called first
        // So we should not be in this block
        content = msg->contents.front();
    }

    string contentType = content->getContentType().asString();
    const vector<char> body = content->getBody();
    string contentBody(body.begin(), body.end());
    message.setContent("ContentType: " + contentType + "\r\n" + contentBody);

    if (!message.isValid()) {
        //TODO
    } else {
        shared_ptr<Content> newContent = make_shared<Content>();
        ContentType newContentType("Message/CPIM");
        newContent->setContentType(newContentType);
        newContent->setBody(message.asString());
        msg->internalContent = newContent;
    }
 }

 void CpimChatMessageModifier::decode(LinphonePrivate::ChatMessagePrivate* msg) {
    //TODO
 }
 
LINPHONE_END_NAMESPACE