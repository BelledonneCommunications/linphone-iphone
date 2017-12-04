/*
 * core-chat-room.cpp
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

#include <algorithm>

#include "address/identity-address.h"
#include "chat/chat-room/basic-chat-room.h"
#include "chat/chat-room/chat-room-p.h"
#include "chat/chat-room/real-time-text-chat-room.h"
#include "conference/participant.h"
#include "core-p.h"
#include "logger/logger.h"

// TODO: Remove me later.
#include "c-wrapper/c-wrapper.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Helpers.
// -----------------------------------------------------------------------------

// Return the better local address to talk with peer address.
static IdentityAddress getDefaultLocalAddress (const shared_ptr<Core> &core, const IdentityAddress &peerAddress) {
	LinphoneCore *cCore = core->getCCore();

	LinphoneAddress *cPeerAddress = linphone_address_new(peerAddress.asString().c_str());
	LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(cCore, cPeerAddress);
	linphone_address_unref(cPeerAddress);

	IdentityAddress localAddress;
	if (proxy) {
		char *identity = linphone_address_as_string(linphone_proxy_config_get_identity_address(proxy));
		localAddress = IdentityAddress(identity);
		bctbx_free(identity);
	} else
		localAddress = IdentityAddress(linphone_core_get_primary_contact(cCore));

	return localAddress;
}

// -----------------------------------------------------------------------------

shared_ptr<ChatRoom> CorePrivate::createBasicChatRoom (const ChatRoomId &chatRoomId, bool isRtt) {
	L_Q();

	shared_ptr<ChatRoom> chatRoom;

	if (isRtt)
		chatRoom.reset(new RealTimeTextChatRoom(q->getSharedFromThis(), chatRoomId));
	else
		chatRoom.reset(new BasicChatRoom(q->getSharedFromThis(), chatRoomId));

	ChatRoomPrivate *dChatRoom = chatRoom->getPrivate();

	dChatRoom->setState(ChatRoom::State::Instantiated);
	dChatRoom->setState(ChatRoom::State::Created);

	return chatRoom;
}

void CorePrivate::insertChatRoom (const shared_ptr<ChatRoom> &chatRoom) {
	L_ASSERT(chatRoom);
	L_Q();

	q->deleteChatRoom(chatRoom);
	chatRooms.push_back(chatRoom);
	chatRoomsById[chatRoom->getChatRoomId()] = chatRoom;
}

void CorePrivate::insertChatRoomWithDb (const shared_ptr<ChatRoom> &chatRoom) {
	L_ASSERT(chatRoom->getState() == ChatRoom::State::Created);
	mainDb->insertChatRoom(chatRoom);
}

// -----------------------------------------------------------------------------

const list<shared_ptr<ChatRoom>> &Core::getChatRooms () const {
	L_D();
	return d->chatRooms;
}

shared_ptr<ChatRoom> Core::findChatRoom (const ChatRoomId &chatRoomId) const {
	L_D();

	auto it = d->chatRoomsById.find(chatRoomId);
	if (it != d->chatRoomsById.cend())
		return it->second;

	lInfo() << "Unable to find chat room in RAM: (peer=" <<
		chatRoomId.getPeerAddress().asString() << ", local=" << chatRoomId.getLocalAddress().asString() << ").";

	return shared_ptr<ChatRoom>();
}

list<shared_ptr<ChatRoom>> Core::findChatRooms (const IdentityAddress &peerAddress) const {
	L_D();

	// TODO: Improve performance if necessary.
	list<shared_ptr<ChatRoom>> output;
	copy_if(
		d->chatRooms.begin(), d->chatRooms.end(),
		back_inserter(output), [&peerAddress](const shared_ptr<ChatRoom> &chatRoom) {
			return chatRoom->getPeerAddress() == peerAddress;
		}
	);

	return output;
}

shared_ptr<ChatRoom> Core::findOneToOneChatRoom (
	const IdentityAddress &localAddress,
	const IdentityAddress &participantAddress
) const {
	L_D();
	for (const auto &chatRoom : d->chatRooms) {
		if (
			chatRoom->getNbParticipants() == 1 &&
			chatRoom->getLocalAddress() == localAddress &&
			participantAddress == chatRoom->getParticipants().front()->getAddress()
		)
			return chatRoom;
	}
	return nullptr;
}

shared_ptr<ChatRoom> Core::createClientGroupChatRoom (const string &subject) {
	L_D();
	return L_GET_CPP_PTR_FROM_C_OBJECT(
		_linphone_client_group_chat_room_new(
			d->cCore,
			linphone_core_get_conference_factory_uri(d->cCore),
			L_STRING_TO_C(subject)
		)
	);
}

shared_ptr<ChatRoom> Core::getOrCreateBasicChatRoom (const ChatRoomId &chatRoomId, bool isRtt) {
	L_D();

	shared_ptr<ChatRoom> chatRoom = findChatRoom(chatRoomId);
	if (chatRoom)
		return chatRoom;

	chatRoom = d->createBasicChatRoom(chatRoomId, isRtt);
	d->insertChatRoom(chatRoom);
	d->insertChatRoomWithDb(chatRoom);

	return chatRoom;
}

shared_ptr<ChatRoom> Core::getOrCreateBasicChatRoom (const IdentityAddress &peerAddress, bool isRtt) {
	L_D();

	list<shared_ptr<ChatRoom>> chatRooms = findChatRooms(peerAddress);
	if (!chatRooms.empty())
		return chatRooms.front();

	shared_ptr<ChatRoom> chatRoom = d->createBasicChatRoom(
		ChatRoomId(peerAddress, getDefaultLocalAddress(getSharedFromThis(), peerAddress)),
		isRtt
	);
	d->insertChatRoom(chatRoom);
	d->insertChatRoomWithDb(chatRoom);

	return chatRoom;
}

shared_ptr<ChatRoom> Core::getOrCreateBasicChatRoomFromUri (const string &peerAddress, bool isRtt) {
	L_D();

	LinphoneAddress *address = linphone_core_interpret_url(d->cCore, L_STRING_TO_C(peerAddress));
	if (!address) {
		lError() << "Cannot make a valid address with: `" << peerAddress << "`.";
		return nullptr;
	}

	shared_ptr<ChatRoom> chatRoom = getOrCreateBasicChatRoom(*L_GET_CPP_PTR_FROM_C_OBJECT(address), isRtt);
	linphone_address_unref(address);
	return chatRoom;
}

void Core::deleteChatRoom (const shared_ptr<const ChatRoom> &chatRoom) {
	CorePrivate *d = chatRoom->getCore()->getPrivate();

	const ChatRoomId &chatRoomId = chatRoom->getChatRoomId();
	auto it = d->chatRoomsById.find(chatRoomId);
	if (it != d->chatRoomsById.end()) {
		auto it = find(d->chatRooms.begin(), d->chatRooms.end(), chatRoom);
		L_ASSERT(it != d->chatRooms.end());
		d->chatRooms.erase(it);
		d->mainDb->deleteChatRoom(chatRoomId);
	}
}

LINPHONE_END_NAMESPACE
