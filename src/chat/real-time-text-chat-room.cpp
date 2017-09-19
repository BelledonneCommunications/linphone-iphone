/*
 * chat-room.cpp
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

#include <algorithm>

#include "linphone/utils/utils.h"

#include "real-time-text-chat-room-p.h"
#include "c-wrapper/c-tools.h"
#include "logger/logger.h"

extern LinphoneChatRoom * _linphone_chat_room_init();
#define GET_BACK_PTR(object) L_GET_C_BACK_PTR(object->shared_from_this(), ChatRoom, chat_room)

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

RealTimeTextChatRoomPrivate::RealTimeTextChatRoomPrivate (LinphoneCore *core, const Address &peerAddress)
	: ChatRoomPrivate(core) {
	this->peerAddress = peerAddress;
}

RealTimeTextChatRoomPrivate::~RealTimeTextChatRoomPrivate () {
	if (!receivedRttCharacters.empty()) {
		for (auto &rttChars : receivedRttCharacters)
			bctbx_free(rttChars);
	}
	if (pendingMessage)
		linphone_chat_message_destroy(pendingMessage);
}

// -----------------------------------------------------------------------------

void RealTimeTextChatRoomPrivate::realtimeTextReceived (uint32_t character, LinphoneCall *call) {
	L_Q(ChatRoom);
	const uint32_t new_line = 0x2028;
	const uint32_t crlf = 0x0D0A;
	const uint32_t lf = 0x0A;

	if (call && linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(call))) {
		LinphoneChatMessageCharacter *cmc = bctbx_new0(LinphoneChatMessageCharacter, 1);

		if (!pendingMessage)
			pendingMessage = q->createMessage("");

		cmc->value = character;
		cmc->has_been_read = FALSE;
		receivedRttCharacters.push_back(cmc);

		remoteIsComposing = true;
		linphone_core_notify_is_composing_received(core, GET_BACK_PTR(q));

		if ((character == new_line) || (character == crlf) || (character == lf)) {
			/* End of message */
			lDebug() << "New line received, forge a message with content " << pendingMessage->message;
			LinphoneAddress *peer = linphone_address_new(peerAddress.asString().c_str());
			linphone_chat_message_set_from(pendingMessage, peer);
			linphone_address_unref(peer);
			if (pendingMessage->to)
				linphone_address_unref(pendingMessage->to);
			pendingMessage->to = linphone_call_get_dest_proxy(call)
				? linphone_address_clone(linphone_call_get_dest_proxy(call)->identity_address)
				: linphone_address_new(linphone_core_get_identity(core));
			pendingMessage->time = ms_time(0);
			pendingMessage->state = LinphoneChatMessageStateDelivered;
			pendingMessage->dir = LinphoneChatMessageIncoming;

			if (lp_config_get_int(core->config, "misc", "store_rtt_messages", 1) == 1)
				storeOrUpdateMessage(pendingMessage);

			if (unreadCount < 0) unreadCount = 1;
			else unreadCount++;

			chatMessageReceived(pendingMessage);
			linphone_chat_message_unref(pendingMessage);
			pendingMessage = nullptr;
			for (auto &rttChars : receivedRttCharacters)
				ms_free(rttChars);
			receivedRttCharacters.clear();
		} else {
			char *value = Utils::utf8ToChar(character);
			pendingMessage->message = ms_strcat_printf(pendingMessage->message, value);
			lDebug() << "Received RTT character: " << value << " (" << character << "), pending text is " << pendingMessage->message;
			delete value;
		}
	}
}

// =============================================================================

RealTimeTextChatRoom::RealTimeTextChatRoom (LinphoneCore *core, const Address &peerAddress) : ChatRoom(*new RealTimeTextChatRoomPrivate(core, peerAddress)) {}

// -----------------------------------------------------------------------------

void RealTimeTextChatRoom::sendMessage (LinphoneChatMessage *msg) {
	L_D(ChatRoom);
	if (d->call && linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(d->call))) {
		uint32_t new_line = 0x2028;
		linphone_chat_message_put_char(msg, new_line);
		linphone_chat_message_unref(msg);
	}
}

// -----------------------------------------------------------------------------

uint32_t RealTimeTextChatRoom::getChar () const {
	L_D(const ChatRoom);
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
	L_D(const ChatRoom);
	return d->call;
}

// -----------------------------------------------------------------------------

shared_ptr<Participant> RealTimeTextChatRoom::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	lError() << "addParticipant() is not allowed on a RealTimeTextChatRoom";
	return nullptr;
}

void RealTimeTextChatRoom::addParticipants (const list<Address> &addresses, const CallSessionParams *params, bool hasMedia) {
	lError() << "addParticipants() is not allowed on a RealTimeTextChatRoom";
}

bool RealTimeTextChatRoom::canHandleParticipants () const {
	return false;
}

const string& RealTimeTextChatRoom::getId () const {
	L_D(const RealTimeTextChatRoom);
	lError() << "a RealTimeTextChatRoom does not have a conference id";
	return d->dummyConferenceId;
}

int RealTimeTextChatRoom::getNbParticipants () const {
	return 1;
}

list<shared_ptr<Participant>> RealTimeTextChatRoom::getParticipants () const {
	L_D(const RealTimeTextChatRoom);
	list<shared_ptr<Participant>> l;
	l.push_back(make_shared<Participant>(d->peerAddress));
	return l;
}

void RealTimeTextChatRoom::removeParticipant (const shared_ptr<const Participant> &participant) {
	lError() << "removeParticipant() is not allowed on a RealTimeTextChatRoom";
}

void RealTimeTextChatRoom::removeParticipants (const list<shared_ptr<Participant>> &participants) {
	lError() << "removeParticipants() is not allowed on a RealTimeTextChatRoom";
}

LINPHONE_END_NAMESPACE
