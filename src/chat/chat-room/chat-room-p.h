/*
 * chat-room-p.h
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

#ifndef _L_CHAT_ROOM_P_H_
#define _L_CHAT_ROOM_P_H_

#include <ctime>

#include "abstract-chat-room-p.h"
#include "chat-room-id.h"
#include "chat-room.h"
#include "chat/notification/is-composing.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatRoomPrivate : public AbstractChatRoomPrivate, public IsComposingListener {
public:
	inline void setProxyChatRoom (AbstractChatRoom *value) { proxyChatRoom = value; }

	inline void setCreationTime (time_t creationTime) override {
		this->creationTime = creationTime;
	}

	inline void setLastUpdateTime (time_t lastUpdateTime) override {
		this->lastUpdateTime = lastUpdateTime;
	}

	void setState (ChatRoom::State state) override;

	void sendChatMessage (const std::shared_ptr<ChatMessage> &chatMessage) override;
	void sendIsComposingNotification ();

	void addEvent (const std::shared_ptr<EventLog> &eventLog) override;

	void addTransientEvent (const std::shared_ptr<EventLog> &eventLog) override;
	void removeTransientEvent (const std::shared_ptr<EventLog> &eventLog) override;

	std::shared_ptr<ChatMessage> createChatMessage (ChatMessage::Direction direction);
	std::list<std::shared_ptr<ChatMessage>> findChatMessages (const std::string &messageId) const;

	void notifyChatMessageReceived (const std::shared_ptr<ChatMessage> &chatMessage) override;
	void notifyIsComposingReceived (const Address &remoteAddress, bool isComposing);
	void notifyStateChanged ();
	void notifyUndecryptableChatMessageReceived (const std::shared_ptr<ChatMessage> &chatMessage) override;

	LinphoneReason onSipMessageReceived (SalOp *op, const SalMessage *message) override;
	void onChatMessageReceived (const std::shared_ptr<ChatMessage> &chatMessage) override;
	void onImdnReceived (const std::shared_ptr<ChatMessage> &chatMessage);
	void onIsComposingReceived (const Address &remoteAddress, const std::string &text);
	void onIsComposingRefreshNeeded () override;
	void onIsComposingStateChanged (bool isComposing) override;
	void onIsRemoteComposingStateChanged (const Address &remoteAddress, bool isComposing) override;

	LinphoneChatRoom *getCChatRoom () const;

	std::list<IdentityAddress> remoteIsComposing;
	std::list<std::shared_ptr<EventLog>> transientEvents;

	ChatRoomId chatRoomId;

private:
	AbstractChatRoom *proxyChatRoom = nullptr;

	ChatRoom::State state = ChatRoom::State::None;

	time_t creationTime = std::time(nullptr);
	time_t lastUpdateTime = std::time(nullptr);

	std::unique_ptr<IsComposing> isComposingHandler;

	bool isComposing = false;

	L_DECLARE_PUBLIC(ChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CHAT_ROOM_P_H_
