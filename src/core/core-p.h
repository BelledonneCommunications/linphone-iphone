/*
 * core-p.h
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

#ifndef _CORE_P_H_
#define _CORE_P_H_

#include "chat/chat-room/chat-room-id.h"
#include "core.h"
#include "db/main-db.h"
#include "object/object-p.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CorePrivate : public ObjectPrivate {
public:
	void insertChatRoom (const std::shared_ptr<ChatRoom> &chatRoom);
	void insertChatRoomWithDb (const std::shared_ptr<ChatRoom> &chatRoom);
	std::shared_ptr<ChatRoom> createBasicChatRoom (const ChatRoomId &chatRoomId, bool isRtt);

	std::unique_ptr<MainDb> mainDb;
	LinphoneCore *cCore = nullptr;

private:
	void deleteChatRoom (const ChatRoomId &chatRoomId);
	void deleteChatRoomWithDb (const ChatRoomId &chatRoomId);

	std::list<std::shared_ptr<ChatRoom>> chatRooms;
	std::unordered_map<ChatRoomId, std::shared_ptr<ChatRoom>> chatRoomsById;

	L_DECLARE_PUBLIC(Core);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CORE_P_H_
