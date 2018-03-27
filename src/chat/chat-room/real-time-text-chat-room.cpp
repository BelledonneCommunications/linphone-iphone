/*
 * real-time-text-chat-room.cpp
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

#include "c-wrapper/c-wrapper.h"
#include "call/call.h"
#include "chat/chat-message/chat-message-p.h"
#include "conference/participant.h"
#include "core/core.h"
#include "logger/logger.h"
#include "real-time-text-chat-room-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

void RealTimeTextChatRoomPrivate::realtimeTextReceived (uint32_t character, const shared_ptr<Call> &call) {
	L_Q();
	const uint32_t new_line = 0x2028;
	const uint32_t crlf = 0x0D0A;
	const uint32_t lf = 0x0A;

	shared_ptr<Core> core = q->getCore();
	LinphoneCore *cCore = core->getCCore();

	if (call && call->getCurrentParams()->realtimeTextEnabled()) {
		if (!pendingMessage)
			pendingMessage = q->createChatMessage("");

		Character cmc;
		cmc.value = character;
		cmc.hasBeenRead = false;
		receivedRttCharacters.push_back(cmc);

		remoteIsComposing.push_back(q->getPeerAddress());
		linphone_core_notify_is_composing_received(cCore, getCChatRoom());

		if ((character == new_line) || (character == crlf) || (character == lf)) {
			// End of message
			lDebug() << "New line received, forge a message with content " << pendingMessage->getPrivate()->getText().c_str();
			pendingMessage->getPrivate()->setState(ChatMessage::State::Delivered);
			pendingMessage->getPrivate()->setDirection(ChatMessage::Direction::Incoming);
			pendingMessage->getPrivate()->setTime(::ms_time(0));

			if (lp_config_get_int(linphone_core_get_config(cCore), "misc", "store_rtt_messages", 1) == 1)
				 pendingMessage->getPrivate()->storeInDb();

			onChatMessageReceived(pendingMessage);
			pendingMessage = nullptr;
			receivedRttCharacters.clear();
		} else {
			char *value = Utils::utf8ToChar(character);
			string text(pendingMessage->getPrivate()->getText());
			text += string(value);
			pendingMessage->getPrivate()->setText(text);
			lDebug() << "Received RTT character: " << value << " (" << character << "), pending text is " << pendingMessage->getPrivate()->getText();
			delete[] value;
		}
	}
}

void RealTimeTextChatRoomPrivate::sendChatMessage (const shared_ptr<ChatMessage> &chatMessage) {
	L_Q();
	shared_ptr<Call> call = q->getCall();
	if (call && call->getCurrentParams()->realtimeTextEnabled()) {
		uint32_t newLine = 0x2028;
		chatMessage->putCharacter(newLine);
	}
}

// =============================================================================

RealTimeTextChatRoom::RealTimeTextChatRoom (const shared_ptr<Core> &core, const ChatRoomId &chatRoomId) :
	BasicChatRoom(*new RealTimeTextChatRoomPrivate, core, chatRoomId) {}

RealTimeTextChatRoom::CapabilitiesMask RealTimeTextChatRoom::getCapabilities () const {
	return BasicChatRoom::getCapabilities() | Capabilities::RealTimeText;
}

uint32_t RealTimeTextChatRoom::getChar () const {
	L_D();
	for (const auto &cmc : d->receivedRttCharacters) {
		if (!cmc.hasBeenRead) {
			const_cast<RealTimeTextChatRoomPrivate::Character *>(&cmc)->hasBeenRead = true;
			return cmc.value;
		}
	}
	return 0;
}

// -----------------------------------------------------------------------------

shared_ptr<Call> RealTimeTextChatRoom::getCall () const {
	L_D();
	return d->call.lock();
}

LINPHONE_END_NAMESPACE
