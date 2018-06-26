/*
 * encryption-chat-message-modifier.cpp
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

#include "address/address.h"
#include "c-wrapper/c-wrapper.h"
#include "chat/chat-message/chat-message.h"
#include "chat/chat-room/chat-room.h"
#include "content/content-type.h"
#include "content/content.h"
#include "core/core.h"

#include "encryption-chat-message-modifier.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ChatMessageModifier::Result EncryptionChatMessageModifier::encode (
	const shared_ptr<ChatMessage> &message,
	int &errorCode
) {
	shared_ptr<AbstractChatRoom> chatRoom = message->getChatRoom();
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(chatRoom->getCore()->getCCore());
	if (!imee)
		return ChatMessageModifier::Result::Skipped;

	LinphoneImEncryptionEngineCbsOutgoingMessageCb cbProcessOutgoingMessage =
		linphone_im_encryption_engine_cbs_get_process_outgoing_message(
			linphone_im_encryption_engine_get_callbacks(imee)
		);

	if (!cbProcessOutgoingMessage)
		return ChatMessageModifier::Result::Skipped;

	int retval = cbProcessOutgoingMessage(imee, L_GET_C_BACK_PTR(chatRoom), L_GET_C_BACK_PTR(message));
	if (retval == -1)
		return ChatMessageModifier::Result::Skipped;

	if (retval != 0 && retval != 1) {
		errorCode = retval;
		return ChatMessageModifier::Result::Error;
	}

	message->setIsSecured(true);
	if (retval == 1)
		return ChatMessageModifier::Result::Suspended;

	return ChatMessageModifier::Result::Done;
}

ChatMessageModifier::Result EncryptionChatMessageModifier::decode (
	const shared_ptr<ChatMessage> &message,
	int &errorCode
) {
	shared_ptr<AbstractChatRoom> chatRoom = message->getChatRoom();
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(chatRoom->getCore()->getCCore());
	if (!imee)
		return ChatMessageModifier::Result::Skipped;

	LinphoneImEncryptionEngineCbsIncomingMessageCb cbProcessIncomingMessage =
		linphone_im_encryption_engine_cbs_get_process_incoming_message(
			linphone_im_encryption_engine_get_callbacks(imee)
		);

	if (!cbProcessIncomingMessage)
		return ChatMessageModifier::Result::Skipped;

	int retval = cbProcessIncomingMessage(imee, L_GET_C_BACK_PTR(chatRoom), L_GET_C_BACK_PTR(message));
	if (retval == -1)
		return ChatMessageModifier::Result::Skipped;

	if (retval != 0 && retval != 1) {
		errorCode = retval;
		return ChatMessageModifier::Result::Error;
	}

	message->setIsSecured(true);
	if (retval == 1)
		return ChatMessageModifier::Result::Suspended;

	return ChatMessageModifier::Result::Done;
}

LINPHONE_END_NAMESPACE
