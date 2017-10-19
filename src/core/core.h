/*
 * core.h
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

#ifndef _CORE_H_
#define _CORE_H_

#include <list>

#include "chat/chat-room/chat-room.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatRoom;
class CorePrivate;

class LINPHONE_PUBLIC Core : public Object {
friend class ClientGroupChatRoom;
public:
	Core (LinphoneCore *cCore);

	std::shared_ptr<ChatRoom> createClientGroupChatRoom (const std::string &subject);
	std::shared_ptr<ChatRoom> getOrCreateChatRoom (const std::string &peerAddress, bool isRtt = false) const;
	const std::list<std::shared_ptr<ChatRoom>> &getChatRooms () const;
	const std::string &getDataPath() const;
	const std::string &getConfigPath() const;

private:
	L_DECLARE_PRIVATE(Core);
	L_DISABLE_COPY(Core);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CORE_H_
