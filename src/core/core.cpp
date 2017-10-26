/*
 * core.cpp
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
#include <sstream>

#include "chat/chat-room/basic-chat-room.h"
#include "chat/chat-room/chat-room-p.h"
#include "chat/chat-room/real-time-text-chat-room.h"
#include "core-p.h"
#include "db/main-db.h"
#include "logger/logger.h"
#include "object/object-p.h"
#include "paths/paths.h"

// TODO: Remove me later.
#include "c-wrapper/c-wrapper.h"
#include "private.h"

#define LINPHONE_DB "linphone.db"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Helpers.
// -----------------------------------------------------------------------------

static inline Address getCleanedPeerAddress (const Address &peerAddress) {
	Address cleanedAddress = peerAddress;
	cleanedAddress.clean();
	cleanedAddress.setPort(0);
	return cleanedAddress;
}

// -----------------------------------------------------------------------------
// CorePrivate: ChatRoom.
// -----------------------------------------------------------------------------

void CorePrivate::insertChatRoom (const shared_ptr<ChatRoom> &chatRoom) {
	L_ASSERT(chatRoom);
	L_ASSERT(chatRoom->getState() == ChatRoom::State::Created);

	string peerAddress = getCleanedPeerAddress(chatRoom->getPeerAddress()).asStringUriOnly();
	deleteChatRoom(peerAddress);

	chatRooms.push_back(chatRoom);
	chatRoomsByUri[peerAddress] = chatRoom;
}

void CorePrivate::deleteChatRoom (const string &peerAddress) {
	auto it = chatRoomsByUri.find(peerAddress);
	if (it != chatRoomsByUri.end())
		chatRooms.erase(
			find_if(chatRooms.begin(), chatRooms.end(), [&peerAddress](const shared_ptr<const ChatRoom> &chatRoom) {
				return peerAddress == chatRoom->getPeerAddress().asStringUriOnly();
			})
		);
}

void CorePrivate::insertChatRoomWithDb (const shared_ptr<ChatRoom> &chatRoom) {
	insertChatRoom(chatRoom);
	mainDb->insertChatRoom(
		getCleanedPeerAddress(chatRoom->getPeerAddress()).asStringUriOnly(),
		chatRoom->getCapabilities()
	);
}

void CorePrivate::deleteChatRoomWithDb (const string &peerAddress) {
	deleteChatRoom(peerAddress);
	mainDb->deleteChatRoom(peerAddress);
}

// =============================================================================

Core::Core (LinphoneCore *cCore) : Object(*new CorePrivate) {
	L_D();
	d->cCore = cCore;
	d->mainDb.reset(new MainDb(this));

	AbstractDb::Backend backend;
	string uri = L_C_TO_STRING(lp_config_get_string(linphone_core_get_config(d->cCore), "server", "db_uri", NULL));
	if (!uri.empty())
		backend = strcmp(lp_config_get_string(linphone_core_get_config(d->cCore), "server", "db_backend", NULL), "mysql") == 0
			? MainDb::Mysql
			: MainDb::Sqlite3;
	else {
		backend = AbstractDb::Sqlite3;
		uri = getDataPath() + "/" LINPHONE_DB;
	}

	lInfo() << "Opening " LINPHONE_DB " at: " << uri;
	if (!d->mainDb->connect(backend, uri))
		lFatal() << "Unable to open linphone database.";

	for (auto &chatRoom : d->mainDb->getChatRooms())
		d->insertChatRoom(chatRoom);
}

// -----------------------------------------------------------------------------
// Paths.
// -----------------------------------------------------------------------------

string Core::getDataPath() const {
	L_D();
	return Paths::getPath(Paths::Data, static_cast<PlatformHelpers *>(d->cCore->platform_helper));
}

string Core::getConfigPath() const {
	L_D();
	return Paths::getPath(Paths::Config, static_cast<PlatformHelpers *>(d->cCore->platform_helper));
}

// -----------------------------------------------------------------------------
// ChatRoom.
// -----------------------------------------------------------------------------

const list<shared_ptr<ChatRoom>> &Core::getChatRooms () const {
	L_D();
	return d->chatRooms;
}

shared_ptr<ChatRoom> Core::findChatRoom (const Address &peerAddress) const {
	L_D();
	auto it = d->chatRoomsByUri.find(getCleanedPeerAddress(peerAddress).asStringUriOnly());
	return it == d->chatRoomsByUri.cend() ? shared_ptr<ChatRoom>() : it->second;
}

shared_ptr<ChatRoom> Core::createClientGroupChatRoom (const string &subject) {
	L_D();

	const char *factoryUri = linphone_core_get_conference_factory_uri(d->cCore);
	if (!factoryUri)
		return nullptr;

	return L_GET_CPP_PTR_FROM_C_OBJECT(
		_linphone_client_group_chat_room_new(d->cCore, factoryUri, L_STRING_TO_C(subject))
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

	if (isRtt)
		chatRoom = ObjectFactory::create<RealTimeTextChatRoom>(d->cCore, peerAddress);
	else
		chatRoom = ObjectFactory::create<BasicChatRoom>(d->cCore, peerAddress);

	chatRoom->getPrivate()->setState(ChatRoom::State::Instantiated);
	chatRoom->getPrivate()->setState(ChatRoom::State::Created);

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
	CorePrivate *d = chatRoom->getCore()->cppCore->getPrivate();
	string peerAddress = getCleanedPeerAddress(chatRoom->getPeerAddress()).asStringUriOnly();
	d->deleteChatRoomWithDb(peerAddress);
}

// -----------------------------------------------------------------------------

LINPHONE_END_NAMESPACE
