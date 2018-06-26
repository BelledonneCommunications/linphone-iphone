/*
 * real-time-text-chat-room.h
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

#ifndef _L_REAL_TIME_TEXT_CHAT_ROOM_H_
#define _L_REAL_TIME_TEXT_CHAT_ROOM_H_

#include "chat/chat-room/basic-chat-room.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Call;
class RealTimeTextChatRoomPrivate;

class LINPHONE_PUBLIC RealTimeTextChatRoom : public BasicChatRoom {
	friend class CallPrivate;
	friend class CorePrivate;

public:
	~RealTimeTextChatRoom () = default;

	CapabilitiesMask getCapabilities () const override;

	uint32_t getChar () const;
	std::shared_ptr<Call> getCall () const;

private:
	RealTimeTextChatRoom (const std::shared_ptr<Core> &core, const ChatRoomId &chatRoomId);

	L_DECLARE_PRIVATE(RealTimeTextChatRoom);
	L_DISABLE_COPY(RealTimeTextChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_REAL_TIME_TEXT_CHAT_ROOM_H_
