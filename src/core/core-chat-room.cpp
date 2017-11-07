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
static inline string resolveWorkaroundClientGroupChatRoomAddress (
	const CorePrivate &corePrivate,
	const SimpleAddress &peerAddr
) {
	const char *uri = linphone_core_get_conference_factory_uri(corePrivate.cCore);
	if (!uri)
		return "";

	SimpleAddress workaroundAddr(peerAddr);
	workaroundAddr.setDomain(Address(uri).getDomain());
	return workaroundAddr.asString();
}

// -----------------------------------------------------------------------------

shared_ptr<ChatRoom> CorePrivate::createBasicChatRoom (const Address &peerAddress, bool isRtt) {
	L_Q();

	shared_ptr<ChatRoom> chatRoom;

	if (isRtt)
		chatRoom = make_shared<RealTimeTextChatRoom>(q->getSharedFromThis(), peerAddress);
	else
		chatRoom = make_shared<BasicChatRoom>(q->getSharedFromThis(), peerAddress);

	ChatRoomPrivate *dChatRoom = chatRoom->getPrivate();

	dChatRoom->setState(ChatRoom::State::Instantiated);
	dChatRoom->setState(ChatRoom::State::Created);

	return chatRoom;
}

void CorePrivate::insertChatRoom (const shared_ptr<ChatRoom> &chatRoom) {
	L_ASSERT(chatRoom);

	const SimpleAddress simpleAddr(chatRoom->getPeerAddress());
	const string peerAddress = chatRoom->getCapabilities() & static_cast<int>(ChatRoom::Capabilities::Conference)
		? resolveWorkaroundClientGroupChatRoomAddress(*this, simpleAddr)
		: simpleAddr.asString();

	deleteChatRoom(peerAddress);
	chatRooms.push_back(chatRoom);
	chatRoomsByUri[peerAddress] = chatRoom;
}

void CorePrivate::deleteChatRoom (const string &peerAddr) {
	const SimpleAddress simpleAddr(peerAddr);
	auto it = chatRoomsByUri.find(simpleAddr.asString());
	if (it != chatRoomsByUri.end()) {
		auto it = find_if(chatRooms.begin(), chatRooms.end(), [&peerAddr, &simpleAddr](const shared_ptr<const ChatRoom> &chatRoom) {
			return peerAddr == simpleAddr.asString();
		});
		if (it != chatRooms.end()) {
			chatRooms.erase(it);
			return;
		}
		lError() << "Unable to remove chat room: " << peerAddr;
	}
}

void CorePrivate::insertChatRoomWithDb (const shared_ptr<ChatRoom> &chatRoom) {
	L_ASSERT(chatRoom->getState() == ChatRoom::State::Created);

	const SimpleAddress simpleAddr(chatRoom->getPeerAddress());
	ChatRoom::CapabilitiesMask capabilities = chatRoom->getCapabilities();
	mainDb->insertChatRoom(
		capabilities & static_cast<int>(ChatRoom::Capabilities::Conference)
			? resolveWorkaroundClientGroupChatRoomAddress(*this, simpleAddr)
			: simpleAddr.asString(),
		capabilities
	);
}

void CorePrivate::deleteChatRoomWithDb (const string &peerAddr) {
	const SimpleAddress simpleAddr(peerAddr);
	deleteChatRoom(simpleAddr.asString());
	mainDb->deleteChatRoom(simpleAddr.asString());
}

// -----------------------------------------------------------------------------

const list<shared_ptr<ChatRoom>> &Core::getChatRooms () const {
	L_D();
	return d->chatRooms;
}

shared_ptr<ChatRoom> Core::findChatRoom (const Address &peerAddr) const {
	L_D();

	const SimpleAddress simpleAddr(peerAddr);
	auto it = d->chatRoomsByUri.find(simpleAddr.asString());
	if (it != d->chatRoomsByUri.cend())
		return it->second;

	lInfo() << "Unable to find chat room: `" << simpleAddr.asString() << "`";

	// TODO: Remove me, temp workaround.
	const string workaroundAddress = resolveWorkaroundClientGroupChatRoomAddress(*d, simpleAddr);
	if (!workaroundAddress.empty()) {
		lWarning() << "Workaround: searching chat room with: `" << workaroundAddress << "`";
		it = d->chatRoomsByUri.find(workaroundAddress);
		return it == d->chatRoomsByUri.cend() ? shared_ptr<ChatRoom>() : it->second;
	}
	return shared_ptr<ChatRoom>();
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

shared_ptr<ChatRoom> Core::getOrCreateBasicChatRoom (const Address &peerAddress, bool isRtt) {
	L_D();

	if (!peerAddress.isValid()) {
		lWarning() << "Cannot find get or create chat room with invalid peer address.";
		return nullptr;
	}

	shared_ptr<ChatRoom> chatRoom = findChatRoom(peerAddress);
	if (chatRoom)
		return chatRoom;

	chatRoom = d->createBasicChatRoom(peerAddress, isRtt);
	d->insertChatRoom(chatRoom);
	d->insertChatRoomWithDb(chatRoom);

	return chatRoom;
}

shared_ptr<ChatRoom> Core::getOrCreateBasicChatRoom (const string &peerAddress, bool isRtt) {
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
	const SimpleAddress simpleAddr(chatRoom->getPeerAddress());
	d->deleteChatRoomWithDb(
		chatRoom->getCapabilities() & static_cast<int>(ChatRoom::Capabilities::Conference)
			? resolveWorkaroundClientGroupChatRoomAddress(*d, simpleAddr)
			: simpleAddr.asString()
	);
}

LINPHONE_END_NAMESPACE
