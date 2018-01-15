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
#include "chat/chat-message/chat-message-p.h"
#include "conference/participant.h"
#include "core/core.h"
#include "logger/logger.h"
#include "real-time-text-chat-room-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

RealTimeTextChatRoomPrivate::~RealTimeTextChatRoomPrivate () {
	if (!receivedRttCharacters.empty()) {
		for (auto &rttChars : receivedRttCharacters)
			bctbx_free(rttChars);
	}
}

// -----------------------------------------------------------------------------

void RealTimeTextChatRoomPrivate::realtimeTextReceived (uint32_t character, LinphoneCall *call) {
	L_Q();
	const uint32_t new_line = 0x2028;
	const uint32_t crlf = 0x0D0A;
	const uint32_t lf = 0x0A;

	shared_ptr<Core> core = q->getCore();
	LinphoneCore *cCore = core->getCCore();

	if (call && linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(call))) {
		LinphoneChatMessageCharacter *cmc = bctbx_new0(LinphoneChatMessageCharacter, 1);

		if (!pendingMessage)
			pendingMessage = q->createChatMessage("");

		cmc->value = character;
		cmc->has_been_read = FALSE;
		receivedRttCharacters.push_back(cmc);

		remoteIsComposing.push_back(q->getPeerAddress());
		linphone_core_notify_is_composing_received(cCore, L_GET_C_BACK_PTR(q));

		if ((character == new_line) || (character == crlf) || (character == lf)) {
			/* End of message */
			lDebug() << "New line received, forge a message with content " << pendingMessage->getPrivate()->getText().c_str();
			// TODO: REPAIR ME.
			// pendingMessage->setFromAddress(peerAddress);
			// pendingMessage->setToAddress(
			// 	Address(
			// 		linphone_call_get_dest_proxy(call)
			// 			? linphone_address_as_string(linphone_call_get_dest_proxy(call)->identity_address)
			// 			: linphone_core_get_identity(cCore)
			// 	)
			// );
			pendingMessage->getPrivate()->setState(ChatMessage::State::Delivered);
			pendingMessage->getPrivate()->setDirection(ChatMessage::Direction::Incoming);

			if (lp_config_get_int(linphone_core_get_config(cCore), "misc", "store_rtt_messages", 1) == 1)
				 pendingMessage->getPrivate()->store();

			onChatMessageReceived(pendingMessage);
			pendingMessage = nullptr;
			for (auto &rttChars : receivedRttCharacters)
				ms_free(rttChars);
			receivedRttCharacters.clear();
		} else {
			char *value = Utils::utf8ToChar(character);
			char *text = (char *)pendingMessage->getPrivate()->getText().c_str();
			pendingMessage->getPrivate()->setText(ms_strcat_printf(text, value));
			lDebug() << "Received RTT character: " << value << " (" << character << "), pending text is " << pendingMessage->getPrivate()->getText();
			delete value;
		}
	}
}

void RealTimeTextChatRoomPrivate::sendChatMessage (const shared_ptr<ChatMessage> &chatMessage) {
	if (call && linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(call))) {
		uint32_t new_line = 0x2028;
		chatMessage->putCharacter(new_line);
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
	if (!d->receivedRttCharacters.empty()) {
		for (auto &cmc : d->receivedRttCharacters) {
			if (!cmc->has_been_read) {
				cmc->has_been_read = TRUE;
				return cmc->value;
			}
		}
	}
	return 0;
}

// -----------------------------------------------------------------------------

LinphoneCall *RealTimeTextChatRoom::getCall () const {
	L_D();
	return d->call;
}

LINPHONE_END_NAMESPACE
