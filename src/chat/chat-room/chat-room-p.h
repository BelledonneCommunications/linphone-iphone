/*
 * chat-room-p.h
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

#ifndef _CHAT_ROOM_P_H_
#define _CHAT_ROOM_P_H_

#include "abstract-chat-room-p.h"
#include "chat-room-id.h"
#include "chat-room.h"
#include "chat/notification/is-composing.h"
#include "event-log/event-log.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatRoomPrivate : public AbstractChatRoomPrivate, public IsComposingListener {
public:
	void setCreationTime (time_t creationTime) override {
		this->creationTime = creationTime;
	}

	void setLastUpdateTime (time_t lastUpdateTime) override {
		this->lastUpdateTime = lastUpdateTime;
	}

	void setState (ChatRoom::State state) override;

	void sendChatMessage (const std::shared_ptr<ChatMessage> &chatMessage) override;

	void addTransientEvent (const std::shared_ptr<EventLog> &eventLog) override;
	void removeTransientEvent (const std::shared_ptr<EventLog> &eventLog) override;

	void notifyUndecryptableMessageReceived (const std::shared_ptr<ChatMessage> &chatMessage) override;

	void onChatMessageReceived (const std::shared_ptr<ChatMessage> &chatMessage) override;
	LinphoneReason onSipMessageReceived (SalOp *op, const SalMessage *message) override;

	// TODO: After this point clean and order methods (AND set members private if possible)!!!

	void sendIsComposingNotification ();

	std::list<std::shared_ptr<ChatMessage>> findMessages (const std::string &messageId) const;

	void imdnReceived (const std::string &text);
	void isComposingReceived (const Address &remoteAddr, const std::string &text);

	void notifyChatMessageReceived (const std::shared_ptr<ChatMessage> &msg);
	void notifyIsComposingReceived (const Address &remoteAddr, bool isComposing);
	void notifyStateChanged ();

	/* IsComposingListener */
	void onIsComposingStateChanged (bool isComposing) override;
	void onIsRemoteComposingStateChanged (const Address &remoteAddr, bool isComposing) override;
	void onIsComposingRefreshNeeded () override;

	std::shared_ptr<ChatMessage> createChatMessage (ChatMessage::Direction direction);

	LinphoneCall *call = nullptr;
	bool isComposing = false;
	std::list<IdentityAddress> remoteIsComposing;
	std::list<std::shared_ptr<EventLog>> transientEvents;

	// TODO: Remove me. Must be present only in rtt chat room.
	std::shared_ptr<ChatMessage> pendingMessage;

	// TODO: Use CoreAccessor on IsComposing. And avoid pointer if possible.
	std::unique_ptr<IsComposing> isComposingHandler;

	// TODO: Check all fields before this point.

	ChatRoom::State state = ChatRoom::State::None;

	ChatRoomId chatRoomId;

	time_t creationTime = std::time(nullptr);
	time_t lastUpdateTime = std::time(nullptr);

private:
	L_DECLARE_PUBLIC(ChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CHAT_ROOM_P_H_
