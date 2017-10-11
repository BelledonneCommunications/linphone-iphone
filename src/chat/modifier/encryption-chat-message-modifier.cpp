/*
 * encryption-chat-message-modifier.cpp
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

#include "encryption-chat-message-modifier.h"

#include "object/object-p.h"
#include "c-wrapper/c-wrapper.h"

#include "content/content-type.h"
#include "content/content.h"
#include "address/address.h"
#include "chat/chat-room.h"
#include "chat/chat-message.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ChatMessageModifier::Result EncryptionChatMessageModifier::encode (const shared_ptr<ChatMessage> &message, int *errorCode) {
	int retval = -1;
	shared_ptr<ChatRoom> chatRoom = message->getChatRoom();
	LinphoneImEncryptionEngine *imee = chatRoom->getCore()->im_encryption_engine;
	if (imee) {
		LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsOutgoingMessageCb cbProcessOutgoingMessage = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imeeCbs);
		if (cbProcessOutgoingMessage) {
			retval = cbProcessOutgoingMessage(imee, L_GET_C_BACK_PTR(chatRoom), L_GET_C_BACK_PTR(message->getSharedFromThis()));
			if (retval == 0 || retval == 1) {
				message->setIsSecured(true);
				if (retval == 1) {
					return ChatMessageModifier::Result::Suspended;
				}
				return ChatMessageModifier::Result::Done;
			} else if (retval == -1) {
				return ChatMessageModifier::Result::Skipped;
			}
			*errorCode = retval;
			return ChatMessageModifier::Result::Error;
		}
	}
	return ChatMessageModifier::Result::Skipped;
}

ChatMessageModifier::Result EncryptionChatMessageModifier::decode (const shared_ptr<ChatMessage> &message, int *errorCode) {
	int retval = -1;
	shared_ptr<ChatRoom> chatRoom = message->getChatRoom();
	LinphoneImEncryptionEngine *imee = chatRoom->getCore()->im_encryption_engine;
	if (imee) {
		LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsIncomingMessageCb cbProcessIncomingMessage = linphone_im_encryption_engine_cbs_get_process_incoming_message(imeeCbs);
		if (cbProcessIncomingMessage) {
			retval = cbProcessIncomingMessage(imee, L_GET_C_BACK_PTR(chatRoom), L_GET_C_BACK_PTR(message->getSharedFromThis()));
			if (retval == 0) {
				message->setIsSecured(true);
				return ChatMessageModifier::Result::Done;
			} else if (retval == -1) {
				return ChatMessageModifier::Result::Skipped;
			}
			*errorCode = retval;
			return ChatMessageModifier::Result::Error;
		}
	}
	return ChatMessageModifier::Result::Skipped;
}

LINPHONE_END_NAMESPACE
