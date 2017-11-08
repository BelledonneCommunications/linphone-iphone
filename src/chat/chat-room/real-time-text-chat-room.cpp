/*
 * real-time-text-chat-room.cpp
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
			pendingMessage = q->createMessage("");

		cmc->value = character;
		cmc->has_been_read = FALSE;
		receivedRttCharacters.push_back(cmc);

		remoteIsComposing.insert(peerAddress.asStringUriOnly());
		linphone_core_notify_is_composing_received(cCore, L_GET_C_BACK_PTR(q));

		if ((character == new_line) || (character == crlf) || (character == lf)) {
			/* End of message */
			lDebug() << "New line received, forge a message with content " << pendingMessage->getPrivate()->getText().c_str();
			pendingMessage->setFromAddress(peerAddress);
			pendingMessage->setToAddress(
				Address(
					linphone_call_get_dest_proxy(call)
						? linphone_address_as_string(linphone_call_get_dest_proxy(call)->identity_address)
						: linphone_core_get_identity(cCore)
				)
			);
			pendingMessage->getPrivate()->setState(ChatMessage::State::Delivered);
			pendingMessage->getPrivate()->setDirection(ChatMessage::Direction::Incoming);

			if (lp_config_get_int(cCore->config, "misc", "store_rtt_messages", 1) == 1)
				storeOrUpdateMessage(pendingMessage);

			chatMessageReceived(pendingMessage);
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

void RealTimeTextChatRoomPrivate::sendMessage (const std::shared_ptr<ChatMessage> &msg) {
	if (call && linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(call))) {
		uint32_t new_line = 0x2028;
		msg->putCharacter(new_line);
	}
}

// =============================================================================

RealTimeTextChatRoom::RealTimeTextChatRoom (const shared_ptr<Core> &core, const Address &peerAddress) :
	ChatRoom(*new RealTimeTextChatRoomPrivate, core, peerAddress) {}

int RealTimeTextChatRoom::getCapabilities () const {
	return static_cast<int>(Capabilities::Basic) | static_cast<int>(Capabilities::RealTimeText);
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

void RealTimeTextChatRoom::markAsRead () {
	L_D();
	ChatRoom::markAsRead();
	if (d->pendingMessage) {
		d->pendingMessage->updateState(ChatMessage::State::Displayed);
		d->pendingMessage->sendDisplayNotification();
	}
}

// -----------------------------------------------------------------------------

void RealTimeTextChatRoom::onChatMessageReceived(const shared_ptr<ChatMessage> &msg) {}

void RealTimeTextChatRoom::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	lError() << "addParticipant() is not allowed on a RealTimeTextChatRoom";
}

void RealTimeTextChatRoom::addParticipants (const list<Address> &addresses, const CallSessionParams *params, bool hasMedia) {
	lError() << "addParticipants() is not allowed on a RealTimeTextChatRoom";
}

bool RealTimeTextChatRoom::canHandleParticipants () const {
	return false;
}

shared_ptr<Participant> RealTimeTextChatRoom::findParticipant (const Address &addr) const {
	lError() << "findParticipant() is not allowed on a RealTimeTextChatRoom";
	return nullptr;
}

const Address &RealTimeTextChatRoom::getConferenceAddress () const {
	lError() << "a RealTimeTextChatRoom does not have a conference address";
	return Utils::getEmptyConstRefObject<Address>();
}

shared_ptr<Participant> RealTimeTextChatRoom::getMe () const {
	lError() << "a RealTimeTextChatRoom does not handle participants";
	return nullptr;
}

int RealTimeTextChatRoom::getNbParticipants () const {
	return 1;
}

list<shared_ptr<Participant>> RealTimeTextChatRoom::getParticipants () const {
	L_D();
	list<shared_ptr<Participant>> l;
	l.push_back(make_shared<Participant>(d->peerAddress));
	return l;
}

const string &RealTimeTextChatRoom::getSubject () const {
	L_D();
	return d->subject;
}

void RealTimeTextChatRoom::join () {
	lError() << "join() is not allowed on a RealTimeTextChatRoom";
}

void RealTimeTextChatRoom::leave () {
	lError() << "leave() is not allowed on a RealTimeTextChatRoom";
}

void RealTimeTextChatRoom::removeParticipant (const shared_ptr<const Participant> &participant) {
	lError() << "removeParticipant() is not allowed on a RealTimeTextChatRoom";
}

void RealTimeTextChatRoom::removeParticipants (const list<shared_ptr<Participant>> &participants) {
	lError() << "removeParticipants() is not allowed on a RealTimeTextChatRoom";
}

void RealTimeTextChatRoom::setParticipantAdminStatus (shared_ptr<Participant> &participant, bool isAdmin) {
	lError() << "setParticipantAdminStatus() is not allowed on a RealTimeTextChatRoom";
}

void RealTimeTextChatRoom::setSubject (const string &subject) {
	L_D();
	d->subject = subject;
}

LINPHONE_END_NAMESPACE
