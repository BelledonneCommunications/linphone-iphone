/*
 * real-time-text-chat-room-p.h
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

#ifndef _REAL_TIME_TEXT_CHAT_ROOM_P_H_
#define _REAL_TIME_TEXT_CHAT_ROOM_P_H_

// From coreapi.
#include "private.h"

#include "real-time-text-chat-room.h"
#include "chat/chat-room-p.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class RealTimeTextChatRoomPrivate : public ChatRoomPrivate {
public:
	RealTimeTextChatRoomPrivate (LinphoneCore *core, const Address &peerAddress);
	virtual ~RealTimeTextChatRoomPrivate ();

public:
	void setCall (LinphoneCall *call) { this->call = call; }
	void realtimeTextReceived (uint32_t character, LinphoneCall *call);

	void sendMessage (const std::shared_ptr<ChatMessage> &msg) override;

public:
	LinphoneCall *call = nullptr;
	std::list<LinphoneChatMessageCharacter *> receivedRttCharacters;
	std::shared_ptr<ChatMessage> pendingMessage = nullptr;

private:
	std::string subject;

	L_DECLARE_PUBLIC(RealTimeTextChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _REAL_TIME_TEXT_CHAT_ROOM_P_H_
