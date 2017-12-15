/*
 * client-group-to-basic-chat-room.cpp
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

#include "client-group-to-basic-chat-room.h"
#include "proxy-chat-room-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

class ClientGroupToBasicChatRoomPrivate : public ProxyChatRoomPrivate {
public:
	inline void sendChatMessage (const shared_ptr<ChatMessage> &chatMessage) override {
		ProxyChatRoomPrivate::sendChatMessage(chatMessage);
		// TODO: Try migration.
	}

	inline void onChatMessageReceived (const shared_ptr<ChatMessage> &chatMessage) override {
		ProxyChatRoomPrivate::onChatMessageReceived(chatMessage);
		// TODO: Try migration.
	}
};

// =============================================================================

ClientGroupToBasicChatRoom::ClientGroupToBasicChatRoom (const shared_ptr<ChatRoom> &chatRoom) :
	ProxyChatRoom(*new ClientGroupToBasicChatRoomPrivate, chatRoom) {}

LINPHONE_END_NAMESPACE
