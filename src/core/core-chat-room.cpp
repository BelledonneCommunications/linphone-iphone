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

#include "address/simple-address.h"
#include "chat/chat-room/basic-chat-room.h"
#include "chat/chat-room/chat-room-p.h"
#include "chat/chat-room/real-time-text-chat-room.h"
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

// TODO: Remove me later.
static inline ChatRoomId resolveWorkaroundClientGroupChatRoomId (
	const CorePrivate &corePrivate,
	const ChatRoomId &chatRoomId
) {
	const char *uri = linphone_core_get_conference_factory_uri(corePrivate.cCore);
	if (!uri)
		return ChatRoomId();

	SimpleAddress peerAddress = chatRoomId.getPeerAddress();
	peerAddress.setDomain(Address(uri).getDomain());
	return ChatRoomId(peerAddress, chatRoomId.getLocalAddress());
}

// TODO: Remove me later.
static inline ChatRoomId resolveWorkaroundClientGroupChatRoomId (
	const CorePrivate &corePrivate,
	const shared_ptr<ChatRoom> &chatRoom
) {
	if (!(chatRoom->getCapabilities() & static_cast<int>(ChatRoom::Capabilities::Conference)))
		return chatRoom->getChatRoomId();
	return resolveWorkaroundClientGroupChatRoomId(corePrivate, chatRoom->getChatRoomId());
}

// Return the better local address to talk with peer address.
static SimpleAddress getDefaultLocalAddress (const shared_ptr<Core> &core, const SimpleAddress &peerAddress) {
	LinphoneCore *cCore = core->getCCore();

	LinphoneAddress *cPeerAddress = linphone_address_new(peerAddress.asString().c_str());
	LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(cCore, cPeerAddress);
	linphone_address_unref(cPeerAddress);

	SimpleAddress localAddress;
	if (proxy) {
		char *identity = linphone_address_as_string(linphone_proxy_config_get_identity_address(proxy));
		localAddress = SimpleAddress(identity);
		bctbx_free(identity);
	} else
		localAddress = SimpleAddress(linphone_core_get_primary_contact(cCore));

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

	const ChatRoomId &chatRoomId = resolveWorkaroundClientGroupChatRoomId(*this, chatRoom);

	deleteChatRoom(chatRoomId);
	chatRooms.push_back(chatRoom);
	chatRoomsById[chatRoomId] = chatRoom;
}

void CorePrivate::deleteChatRoom (const ChatRoomId &chatRoomId) {
	auto it = chatRoomsById.find(chatRoomId);
	if (it != chatRoomsById.end()) {
		auto it = find_if(chatRooms.begin(), chatRooms.end(), [&chatRoomId](const shared_ptr<ChatRoom> &chatRoom) {
			return chatRoomId == chatRoom->getChatRoomId();
		});
		if (it != chatRooms.end()) {
			chatRooms.erase(it);
			return;
		}
		lError() << "Unable to remove chat room: (peer=" <<
			chatRoomId.getPeerAddress().asString() << ", local=" << chatRoomId.getLocalAddress().asString() << ").";
	}
}

void CorePrivate::insertChatRoomWithDb (const shared_ptr<ChatRoom> &chatRoom) {
	L_ASSERT(chatRoom->getState() == ChatRoom::State::Created);

	const ChatRoomId &chatRoomId = resolveWorkaroundClientGroupChatRoomId(*this, chatRoom);

	ChatRoom::CapabilitiesMask capabilities = chatRoom->getCapabilities();
	mainDb->insertChatRoom(chatRoomId, capabilities);
}

void CorePrivate::deleteChatRoomWithDb (const ChatRoomId &chatRoomId) {
	deleteChatRoom(chatRoomId);
	mainDb->deleteChatRoom(chatRoomId);
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

	lInfo() << "Unable to find chat room: (peer=" <<
		chatRoomId.getPeerAddress().asString() << ", local=" << chatRoomId.getLocalAddress().asString() << ").";

	// TODO: Remove me, temp workaround.
	ChatRoomId workaroundChatRoomId = resolveWorkaroundClientGroupChatRoomId(*d, chatRoomId);
	lWarning() << "Workaround: searching chat room with: (peer=" <<
		chatRoomId.getPeerAddress().asString() << ", local=" << chatRoomId.getLocalAddress().asString() << ").";

	it = d->chatRoomsById.find(workaroundChatRoomId);
	return it == d->chatRoomsById.cend() ? shared_ptr<ChatRoom>() : it->second;
}

list<shared_ptr<ChatRoom>> Core::findChatRooms (const SimpleAddress &peerAddress) const {
	// TODO: DEV GROUP CHAT.
	return list<shared_ptr<ChatRoom>>();
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

shared_ptr<ChatRoom> Core::getOrCreateBasicChatRoom (const SimpleAddress &peerAddress, bool isRtt) {
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
	d->deleteChatRoomWithDb(
		resolveWorkaroundClientGroupChatRoomId(*d, chatRoom->getChatRoomId())
	);
}

LINPHONE_END_NAMESPACE
