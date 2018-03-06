/*
 * basic-to-client-group-chat-room.cpp
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

#include "basic-to-client-group-chat-room.h"
#include "proxy-chat-room-p.h"
#include "client-group-chat-room-p.h"
#include "chat/chat-message/chat-message-p.h"
#include "conference/participant.h"
#include "conference/session/call-session.h"
#include "core/core-p.h"
#include "c-wrapper/c-wrapper.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

class BasicToClientGroupChatRoomPrivate : public ProxyChatRoomPrivate {
public:
	void onChatRoomInsertRequested (const shared_ptr<AbstractChatRoom> &chatRoom) override {
		L_Q();
		// Insert the client group chat room temporarily
		q->getCore()->getPrivate()->insertChatRoom(chatRoom);
	}

	void onChatRoomInsertInDatabaseRequested (const shared_ptr<AbstractChatRoom> &chatRoom) override {
		// Do not insert the client group chat room in database, the migration will do it
	}

	void onChatRoomDeleteRequested (const shared_ptr<AbstractChatRoom> &chatRoom) override {
		L_Q();
		q->getCore()->deleteChatRoom(q->getSharedFromThis());
		setState(AbstractChatRoom::State::Deleted);
	}

	void sendChatMessage (const shared_ptr<ChatMessage> &chatMessage) override {
		L_Q();
		ProxyChatRoomPrivate::sendChatMessage(chatMessage);
		const char *specs = linphone_core_get_linphone_specs(chatMessage->getCore()->getCCore());
		time_t currentRealTime = ms_time(nullptr);
		LinphoneAddress *lAddr = linphone_address_new(
			chatMessage->getChatRoom()->getChatRoomId().getLocalAddress().asString().c_str()
		);
		LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(q->getCore()->getCCore(), lAddr);
		linphone_address_unref(lAddr);
		const char *conferenceFactoryUri = nullptr;
		if (proxy)
			conferenceFactoryUri = linphone_proxy_config_get_conference_factory_uri(proxy);
		if (!conferenceFactoryUri
			|| (chatRoom->getCapabilities() & ChatRoom::Capabilities::Conference)
			|| clientGroupChatRoom
			|| !specs || !strstr(specs, "groupchat")
			|| ((currentRealTime - migrationRealTime) <
				linphone_config_get_int(linphone_core_get_config(chatMessage->getCore()->getCCore()),
					"misc", "basic_to_client_group_chat_room_migration_timer", 86400) // Try migration every 24 hours
				)
		) {
			return;
		}
		migrationRealTime = currentRealTime;
		clientGroupChatRoom = static_pointer_cast<ClientGroupChatRoom>(
			chatRoom->getCore()->getPrivate()->createClientGroupChatRoom(chatRoom->getSubject(), "", false)
		);
		clientGroupChatRoom->getPrivate()->setCallSessionListener(this);
		clientGroupChatRoom->getPrivate()->setChatRoomListener(this);
		clientGroupChatRoom->addParticipant(chatRoom->getPeerAddress(), nullptr, false);
	}

	void onCallSessionStateChanged (
		const shared_ptr<CallSession> &session,
		CallSession::State newState,
		const string &message
	) override {
		if (!clientGroupChatRoom)
			return;
		if ((newState == CallSession::State::Error) && (clientGroupChatRoom->getState() == ChatRoom::State::CreationPending)) {
			Core::deleteChatRoom(clientGroupChatRoom);
			if (session->getReason() == LinphoneReasonNotAcceptable) {
				clientGroupChatRoom = nullptr;
				return;
			}
		}
		clientGroupChatRoom->getPrivate()->onCallSessionStateChanged(session, newState, message);
	}

private:
	shared_ptr<ClientGroupChatRoom> clientGroupChatRoom;
	time_t migrationRealTime = 0;

	L_DECLARE_PUBLIC(BasicToClientGroupChatRoom);
};

// =============================================================================

BasicToClientGroupChatRoom::BasicToClientGroupChatRoom (const shared_ptr<ChatRoom> &chatRoom) :
	ProxyChatRoom(*new BasicToClientGroupChatRoomPrivate, chatRoom) {}

BasicToClientGroupChatRoom::CapabilitiesMask BasicToClientGroupChatRoom::getCapabilities () const {
	L_D();
	CapabilitiesMask capabilities = d->chatRoom->getCapabilities();
	capabilities.set(Capabilities::Proxy);
	if (capabilities.isSet(Capabilities::Basic))
		capabilities.set(Capabilities::Migratable);
	return capabilities;
}

shared_ptr<ChatMessage> BasicToClientGroupChatRoom::createChatMessage () {
	shared_ptr<ChatMessage> msg = ProxyChatRoom::createChatMessage();
	msg->getPrivate()->setChatRoom(getSharedFromThis());
	return msg;
}

shared_ptr<ChatMessage> BasicToClientGroupChatRoom::createChatMessage (const string &text) {
	shared_ptr<ChatMessage> msg = ProxyChatRoom::createChatMessage(text);
	msg->getPrivate()->setChatRoom(getSharedFromThis());
	return msg;
}

void BasicToClientGroupChatRoom::migrate(const std::shared_ptr<ClientGroupChatRoom> &clientGroupChatRoom, const std::shared_ptr<AbstractChatRoom> &chatRoom) {
	clientGroupChatRoom->getCore()->getPrivate()->mainDb->migrateBasicToClientGroupChatRoom(chatRoom, clientGroupChatRoom);

	if (chatRoom->getCapabilities() & ChatRoom::Capabilities::Proxy) {
		shared_ptr<BasicToClientGroupChatRoom> btcgcr = static_pointer_cast<BasicToClientGroupChatRoom>(chatRoom);
		btcgcr->getCore()->getPrivate()->replaceChatRoom(chatRoom, clientGroupChatRoom);
		btcgcr->getPrivate()->chatRoom = clientGroupChatRoom;
	} else {
		LinphoneChatRoom *lcr = L_GET_C_BACK_PTR(chatRoom);
		L_SET_CPP_PTR_FROM_C_OBJECT(lcr, clientGroupChatRoom);
		clientGroupChatRoom->getCore()->getPrivate()->replaceChatRoom(chatRoom, clientGroupChatRoom);
	}
}

LINPHONE_END_NAMESPACE
