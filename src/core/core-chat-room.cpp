/*
 * core-chat-room.cpp
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

#include <algorithm>

#include "address/identity-address.h"
#include "chat/chat-room/basic-chat-room.h"
#include "chat/chat-room/basic-to-client-group-chat-room.h"
#include "chat/chat-room/chat-room-p.h"
#include "chat/chat-room/client-group-chat-room-p.h"
#include "chat/chat-room/client-group-to-basic-chat-room.h"
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

shared_ptr<AbstractChatRoom> CorePrivate::createBasicChatRoom (
	const ChatRoomId &chatRoomId,
	ChatRoom::CapabilitiesMask capabilities
) {
	L_Q();

	shared_ptr<AbstractChatRoom> chatRoom;

	if (capabilities & ChatRoom::Capabilities::RealTimeText)
		chatRoom.reset(new RealTimeTextChatRoom(q->getSharedFromThis(), chatRoomId));
	else {
		BasicChatRoom *basicChatRoom = new BasicChatRoom(q->getSharedFromThis(), chatRoomId);
		LinphoneAddress *lAddr = linphone_address_new(chatRoomId.getLocalAddress().asString().c_str());
		LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(q->getCCore(), lAddr);
		linphone_address_unref(lAddr);
		const char *conferenceFactoryUri = nullptr;
		if (proxy)
			conferenceFactoryUri = linphone_proxy_config_get_conference_factory_uri(proxy);
		if (
			capabilities & ChatRoom::Capabilities::Migratable &&
			conferenceFactoryUri &&
			linphone_config_get_bool(linphone_core_get_config(q->getCCore()),
				"misc", "enable_basic_to_client_group_chat_room_migration", FALSE)
		)
			chatRoom.reset(new BasicToClientGroupChatRoom(shared_ptr<BasicChatRoom>(basicChatRoom)));
		else
			chatRoom.reset(basicChatRoom);
	}

	AbstractChatRoomPrivate *dChatRoom = chatRoom->getPrivate();
	dChatRoom->setState(ChatRoom::State::Instantiated);
	dChatRoom->setState(ChatRoom::State::Created);

	return chatRoom;
}

shared_ptr<AbstractChatRoom> CorePrivate::createClientGroupChatRoom (const string &subject, const string &uri, bool fallback) {
	L_Q();

	string usedUri;
	string from;
	{
		LinphoneProxyConfig *proxy = nullptr;
		if (uri.empty()) {
			proxy = linphone_core_get_default_proxy_config(q->getCCore());
			if (!proxy)
				return nullptr;
			const char *conferenceFactoryUri = linphone_proxy_config_get_conference_factory_uri(proxy);
			if (!conferenceFactoryUri)
				return nullptr;
			usedUri = conferenceFactoryUri;
		} else {
			LinphoneAddress *addr = linphone_address_new(uri.c_str());
			proxy = linphone_core_lookup_known_proxy(q->getCCore(), addr);
			linphone_address_unref(addr);
			usedUri = uri;
		}
		if (proxy) {
			const LinphoneAddress *contactAddr = linphone_proxy_config_get_contact(proxy);
			if (contactAddr) {
				char *cFrom = linphone_address_as_string(contactAddr);
				from = string(cFrom);
				bctbx_free(cFrom);
			} else
				from = L_GET_CPP_PTR_FROM_C_OBJECT(linphone_proxy_config_get_identity_address(proxy))->asString();
		}
	}

	shared_ptr<AbstractChatRoom> chatRoom;
	{
		shared_ptr<ClientGroupChatRoom> clientGroupChatRoom = make_shared<ClientGroupChatRoom>(
			q->getSharedFromThis(), usedUri, IdentityAddress(from), subject
		);
		ClientGroupChatRoomPrivate *dClientGroupChatRoom = clientGroupChatRoom->getPrivate();

		if (fallback) {
			// Create a ClientGroupToBasicChatRoom to handle fallback from ClientGroupChatRoom to BasicGroupChatRoom if
			// only one participant is invited and that it does not support group chat
			chatRoom = make_shared<ClientGroupToBasicChatRoom>(clientGroupChatRoom);
			dClientGroupChatRoom->setCallSessionListener(chatRoom->getPrivate());
			dClientGroupChatRoom->setChatRoomListener(chatRoom->getPrivate());
		} else
			chatRoom = clientGroupChatRoom;
	}
	chatRoom->getPrivate()->setState(ChatRoom::State::Instantiated);

	noCreatedClientGroupChatRooms[chatRoom.get()] = chatRoom;

	return chatRoom;
}

void CorePrivate::insertChatRoom (const shared_ptr<AbstractChatRoom> &chatRoom) {
	L_ASSERT(chatRoom);

	const ChatRoomId &chatRoomId = chatRoom->getChatRoomId();
	auto it = chatRoomsById.find(chatRoomId);
	// Chat room not exist or yes but with the same pointer!
	L_ASSERT(it == chatRoomsById.end() || it->second == chatRoom);
	if (it == chatRoomsById.end()) {
		// Remove chat room from workaround cache.
		noCreatedClientGroupChatRooms.erase(chatRoom.get());

		chatRooms.push_back(chatRoom);
		chatRoomsById[chatRoomId] = chatRoom;
	}
}

void CorePrivate::insertChatRoomWithDb (const shared_ptr<AbstractChatRoom> &chatRoom) {
	L_ASSERT(chatRoom->getState() == ChatRoom::State::Created);
	mainDb->insertChatRoom(chatRoom);
}

void CorePrivate::loadChatRooms () {
	chatRooms.clear();
	chatRoomsById.clear();
	for (auto &chatRoom : mainDb->getChatRooms())
		insertChatRoom(chatRoom);
}

void CorePrivate::replaceChatRoom (const shared_ptr<AbstractChatRoom> &replacedChatRoom, const shared_ptr<AbstractChatRoom> &newChatRoom) {
	const ChatRoomId &replacedChatRoomId = replacedChatRoom->getChatRoomId();
	const ChatRoomId &newChatRoomId = newChatRoom->getChatRoomId();
	if (replacedChatRoom->getCapabilities() & ChatRoom::Capabilities::Proxy) {
		chatRooms.remove(newChatRoom);
		chatRoomsById.erase(newChatRoomId);
		chatRoomsById[newChatRoomId] = replacedChatRoom;
	} else {
		chatRooms.remove(replacedChatRoom);
		chatRoomsById.erase(replacedChatRoomId);
		chatRoomsById[newChatRoomId] = newChatRoom;
	}
}

// -----------------------------------------------------------------------------

const list<shared_ptr<AbstractChatRoom>> &Core::getChatRooms () const {
	L_D();
	return d->chatRooms;
}

shared_ptr<AbstractChatRoom> Core::findChatRoom (const ChatRoomId &chatRoomId) const {
	L_D();

	auto it = d->chatRoomsById.find(chatRoomId);
	if (it != d->chatRoomsById.cend())
		return it->second;

	lInfo() << "Unable to find chat room in RAM: " << chatRoomId << ".";
	return nullptr;
}

list<shared_ptr<AbstractChatRoom>> Core::findChatRooms (const IdentityAddress &peerAddress) const {
	L_D();

	list<shared_ptr<AbstractChatRoom>> output;
	copy_if(
		d->chatRooms.begin(), d->chatRooms.end(),
		back_inserter(output), [&peerAddress](const shared_ptr<AbstractChatRoom> &chatRoom) {
			return chatRoom->getPeerAddress() == peerAddress;
		}
	);

	return output;
}

shared_ptr<AbstractChatRoom> Core::findOneToOneChatRoom (
	const IdentityAddress &localAddress,
	const IdentityAddress &participantAddress
) const {
	L_D();
	for (const auto &chatRoom : d->chatRooms) {
		const IdentityAddress &curLocalAddress = chatRoom->getLocalAddress();
		ChatRoom::CapabilitiesMask capabilities = chatRoom->getCapabilities();

		// We are looking for a one to one chatroom
		// Do not return a group chat room that everyone except one person has left
		if (!(capabilities & ChatRoom::Capabilities::OneToOne))
			continue;

		// One to one client group chat room
		// The only participant's address must match the participantAddress argument
		if (
			(capabilities & ChatRoom::Capabilities::Conference) &&
			!chatRoom->getParticipants().empty() &&
			localAddress == curLocalAddress &&
			participantAddress.getAddressWithoutGruu() == chatRoom->getParticipants().front()->getAddress()
		)
			return chatRoom;

		// One to one basic chat room (addresses without gruu)
		// The peer address must match the participantAddress argument
		if (
			(capabilities & ChatRoom::Capabilities::Basic) &&
			localAddress.getAddressWithoutGruu() == curLocalAddress.getAddressWithoutGruu() &&
			participantAddress.getAddressWithoutGruu() == chatRoom->getPeerAddress().getAddressWithoutGruu()
		)
			return chatRoom;
	}
	return nullptr;
}

shared_ptr<AbstractChatRoom> Core::createClientGroupChatRoom (const string &subject) {
	L_D();
	return d->createClientGroupChatRoom(subject);
}

shared_ptr<AbstractChatRoom> Core::getOrCreateBasicChatRoom (const ChatRoomId &chatRoomId, bool isRtt) {
	L_D();

	shared_ptr<AbstractChatRoom> chatRoom = findChatRoom(chatRoomId);
	if (chatRoom)
		return chatRoom;

	chatRoom = d->createBasicChatRoom(chatRoomId,
		isRtt ? ChatRoom::CapabilitiesMask(ChatRoom::Capabilities::RealTimeText) : ChatRoom::CapabilitiesMask()
	);
	d->insertChatRoom(chatRoom);
	d->insertChatRoomWithDb(chatRoom);

	return chatRoom;
}

shared_ptr<AbstractChatRoom> Core::getOrCreateBasicChatRoom (const IdentityAddress &peerAddress, bool isRtt) {
	L_D();

	list<shared_ptr<AbstractChatRoom>> chatRooms = findChatRooms(peerAddress);
	if (!chatRooms.empty())
		return chatRooms.front();

	shared_ptr<AbstractChatRoom> chatRoom = d->createBasicChatRoom(
		ChatRoomId(peerAddress, getDefaultLocalAddress(getSharedFromThis(), peerAddress)),
		isRtt ? ChatRoom::CapabilitiesMask(ChatRoom::Capabilities::RealTimeText) : ChatRoom::CapabilitiesMask()
	);
	d->insertChatRoom(chatRoom);
	d->insertChatRoomWithDb(chatRoom);

	return chatRoom;
}

shared_ptr<AbstractChatRoom> Core::getOrCreateBasicChatRoomFromUri (const string &peerAddress, bool isRtt) {
	LinphoneAddress *address = linphone_core_interpret_url(getCCore(), L_STRING_TO_C(peerAddress));
	if (!address) {
		lError() << "Cannot make a valid address with: `" << peerAddress << "`.";
		return nullptr;
	}

	shared_ptr<AbstractChatRoom> chatRoom = getOrCreateBasicChatRoom(*L_GET_CPP_PTR_FROM_C_OBJECT(address), isRtt);
	linphone_address_unref(address);
	return chatRoom;
}

void Core::deleteChatRoom (const shared_ptr<const AbstractChatRoom> &chatRoom) {
	CorePrivate *d = chatRoom->getCore()->getPrivate();

	d->noCreatedClientGroupChatRooms.erase(chatRoom.get());
	const ChatRoomId &chatRoomId = chatRoom->getChatRoomId();
	auto chatRoomsByIdIt = d->chatRoomsById.find(chatRoomId);
	if (chatRoomsByIdIt != d->chatRoomsById.end()) {
		auto chatRoomsIt = find(d->chatRooms.begin(), d->chatRooms.end(), chatRoom);
		L_ASSERT(chatRoomsIt != d->chatRooms.end());
		d->chatRooms.erase(chatRoomsIt);
		d->chatRoomsById.erase(chatRoomsByIdIt);
		d->mainDb->deleteChatRoom(chatRoomId);
	}
}

LINPHONE_END_NAMESPACE
