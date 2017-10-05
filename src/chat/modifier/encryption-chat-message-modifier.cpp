/*
 * encryption-chat-message-modifier.cpp
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

#include "encryption-chat-message-modifier.h"

#include "object/object-p.h"
#include "c-wrapper/c-wrapper.h"

#include "content/content-type.h"
#include "content/content.h"
#include "address/address.h"
#include "chat/chat-room.h"
#include "chat/chat-message-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ChatMessageModifier::Result EncryptionChatMessageModifier::encode (ChatMessagePrivate *messagePrivate, int *errorCode) {
	int retval = -1;
	LinphoneImEncryptionEngine *imee = messagePrivate->chatRoom->getCore()->im_encryption_engine;
	if (imee) {
		LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsOutgoingMessageCb cbProcessOutgoingMessage = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imeeCbs);
		if (cbProcessOutgoingMessage) {
			retval = cbProcessOutgoingMessage(imee, L_GET_C_BACK_PTR(messagePrivate->chatRoom), L_GET_C_BACK_PTR(messagePrivate->getPublic()->getSharedFromThis()));
			if (retval == 0 || retval == 1) {
				messagePrivate->isSecured = true;
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

ChatMessageModifier::Result EncryptionChatMessageModifier::decode (ChatMessagePrivate *messagePrivate, int *errorCode) {
	int retval = -1;
	LinphoneImEncryptionEngine *imee = messagePrivate->chatRoom->getCore()->im_encryption_engine;
	if (imee) {
		LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsIncomingMessageCb cbProcessIncomingMessage = linphone_im_encryption_engine_cbs_get_process_incoming_message(imeeCbs);
		if (cbProcessIncomingMessage) {
			retval = cbProcessIncomingMessage(imee, L_GET_C_BACK_PTR(messagePrivate->chatRoom), L_GET_C_BACK_PTR(messagePrivate->getPublic()->getSharedFromThis()));
			if (retval == 0) {
				messagePrivate->isSecured = true;
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
