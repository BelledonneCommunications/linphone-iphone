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

#include "multipart-chat-message-modifier.h"
#include "address/address.h"
#include "chat/chat-message-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ChatMessageModifier::Result MultipartChatMessageModifier::encode (shared_ptr<ChatMessage> message, int *errorCode) {
	if (message->getContents().size() > 1) {
		//TODO
		return ChatMessageModifier::Result::Done;
	}
	return ChatMessageModifier::Result::Skipped;
}

ChatMessageModifier::Result MultipartChatMessageModifier::decode (shared_ptr<ChatMessage> message, int *errorCode) {
	//TODO
	if (false) { // Multipart required

		return ChatMessageModifier::Result::Done;
	} else if (message->getContents().size() == 0) {
		// All previous modifiers only altered the internal content, let's fill the content list because we're the last modifier to be called
		message->addContent(message->getInternalContent());
	}
	return ChatMessageModifier::Result::Skipped;
}

LINPHONE_END_NAMESPACE
