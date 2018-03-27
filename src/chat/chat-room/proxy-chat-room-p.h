/*
 * proxy-chat-room-p.h
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

#ifndef _L_PROXY_CHAT_ROOM_P_H_
#define _L_PROXY_CHAT_ROOM_P_H_

#include "abstract-chat-room-p.h"
#include "proxy-chat-room.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ProxyChatRoomPrivate : public AbstractChatRoomPrivate {
public:
	inline void setCreationTime (time_t creationTime) override {
		chatRoom->getPrivate()->setCreationTime(creationTime);
	}

	inline void setLastUpdateTime (time_t lastUpdateTime) override {
		chatRoom->getPrivate()->setLastUpdateTime(lastUpdateTime);
	}

	inline void setState (AbstractChatRoom::State state) override {
		chatRoom->getPrivate()->setState(state);
	}

	inline void sendChatMessage (const std::shared_ptr<ChatMessage> &chatMessage) override {
		chatRoom->getPrivate()->sendChatMessage(chatMessage);
	}

	inline void addEvent (const std::shared_ptr<EventLog> &eventLog) override {
		chatRoom->getPrivate()->addEvent(eventLog);
	}

	inline void addTransientEvent (const std::shared_ptr<EventLog> &eventLog) override {
		chatRoom->getPrivate()->addTransientEvent(eventLog);
	}

	inline void removeTransientEvent (const std::shared_ptr<EventLog> &eventLog) override {
		chatRoom->getPrivate()->removeTransientEvent(eventLog);
	}

	inline void notifyChatMessageReceived (const std::shared_ptr<ChatMessage> &chatMessage) override {
		chatRoom->getPrivate()->notifyChatMessageReceived(chatMessage);
	}

	inline void notifyUndecryptableChatMessageReceived (const std::shared_ptr<ChatMessage> &chatMessage) override {
		chatRoom->getPrivate()->notifyUndecryptableChatMessageReceived(chatMessage);
	}

	inline LinphoneReason onSipMessageReceived (SalOp *op, const SalMessage *message) override {
		return chatRoom->getPrivate()->onSipMessageReceived(op, message);
	}

	inline void onChatMessageReceived (const std::shared_ptr<ChatMessage> &chatMessage) override {
		chatRoom->getPrivate()->onChatMessageReceived(chatMessage);
	}

	void setupProxy ();
	void teardownProxy ();

	std::shared_ptr<AbstractChatRoom> chatRoom;

	L_DECLARE_PUBLIC(ProxyChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_PROXY_CHAT_ROOM_P_H_
