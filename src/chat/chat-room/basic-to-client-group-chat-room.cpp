/*
 * basic-to-client-group-chat-room.cpp
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

#include "abstract-chat-room-p.h"
#include "basic-to-client-group-chat-room.h"
#include "chat-room.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

class BasicToClientGroupChatRoomPrivate : public AbstractChatRoomPrivate {
public:
	shared_ptr<AbstractChatRoom> chatRoom;

private:
	inline void setCreationTime (time_t creationTime) override {
		chatRoom->getPrivate()->setCreationTime(creationTime);
	}

	inline void setLastUpdateTime (time_t lastUpdateTime) override {
		chatRoom->getPrivate()->setLastUpdateTime(lastUpdateTime);
	}

	inline void setState (BasicToClientGroupChatRoom::State state) override {
		chatRoom->getPrivate()->setState(state);
	}

	inline void sendChatMessage (const shared_ptr<ChatMessage> &chatMessage) override {
		chatRoom->getPrivate()->sendChatMessage(chatMessage);
	}

	inline void addTransientEvent (const shared_ptr<EventLog> &eventLog) override {
		chatRoom->getPrivate()->addTransientEvent(eventLog);
	}

	inline void removeTransientEvent (const shared_ptr<EventLog> &eventLog) override {
		chatRoom->getPrivate()->removeTransientEvent(eventLog);
	}

	inline void notifyUndecryptableChatMessageReceived (const shared_ptr<ChatMessage> &chatMessage) override {
		chatRoom->getPrivate()->notifyUndecryptableChatMessageReceived(chatMessage);
	}

	inline LinphoneReason onSipMessageReceived (SalOp *op, const SalMessage *message) override {
		return chatRoom->getPrivate()->onSipMessageReceived(op, message);
	}

	inline void onChatMessageReceived (const shared_ptr<ChatMessage> &chatMessage) override {
		chatRoom->getPrivate()->onChatMessageReceived(chatMessage);
	}
};

// =============================================================================

BasicToClientGroupChatRoom::BasicToClientGroupChatRoom (const shared_ptr<ChatRoom> &chatRoom) :
	AbstractChatRoom(*new BasicToClientGroupChatRoomPrivate, chatRoom->getCore()) {
	L_D();
	d->chatRoom = chatRoom;
}

// -----------------------------------------------------------------------------

const ChatRoomId &BasicToClientGroupChatRoom::getChatRoomId () const {
	L_D();
	return d->chatRoom->getChatRoomId();
}

const IdentityAddress &BasicToClientGroupChatRoom::getPeerAddress () const {
	L_D();
	return d->chatRoom->getPeerAddress();
}

const IdentityAddress &BasicToClientGroupChatRoom::getLocalAddress () const {
	L_D();
	return d->chatRoom->getLocalAddress();
}

// -----------------------------------------------------------------------------

time_t BasicToClientGroupChatRoom::getCreationTime () const {
	L_D();
	return d->chatRoom->getCreationTime();
}

time_t BasicToClientGroupChatRoom::getLastUpdateTime () const {
	L_D();
	return d->chatRoom->getLastUpdateTime();
}

// -----------------------------------------------------------------------------

BasicToClientGroupChatRoom::State BasicToClientGroupChatRoom::getState () const {
	L_D();
	return d->chatRoom->getState();
}

// -----------------------------------------------------------------------------

list<shared_ptr<EventLog>> BasicToClientGroupChatRoom::getHistory (int nLast) const {
	L_D();
	return d->chatRoom->getHistory(nLast);
}

list<shared_ptr<EventLog>> BasicToClientGroupChatRoom::getHistoryRange (int begin, int end) const {
	L_D();
	return d->chatRoom->getHistoryRange(begin, end);
}

int BasicToClientGroupChatRoom::getHistorySize () const {
	L_D();
	return d->chatRoom->getHistorySize();
}

void BasicToClientGroupChatRoom::deleteHistory () {
	L_D();
	d->chatRoom->deleteHistory();
}

shared_ptr<ChatMessage> BasicToClientGroupChatRoom::getLastChatMessageInHistory () const {
	L_D();
	return d->chatRoom->getLastChatMessageInHistory();
}

int BasicToClientGroupChatRoom::getChatMessageCount () const {
	L_D();
	return d->chatRoom->getChatMessageCount();
}

int BasicToClientGroupChatRoom::getUnreadChatMessageCount () const {
	L_D();
	return d->chatRoom->getUnreadChatMessageCount();
}

// -----------------------------------------------------------------------------

void BasicToClientGroupChatRoom::compose () {
	L_D();
	return d->chatRoom->compose();
}

bool BasicToClientGroupChatRoom::isRemoteComposing () const {
	L_D();
	return d->chatRoom->isRemoteComposing();
}

list<IdentityAddress> BasicToClientGroupChatRoom::getComposingAddresses () const {
	L_D();
	return d->chatRoom->getComposingAddresses();
}

// -----------------------------------------------------------------------------

shared_ptr<ChatMessage> BasicToClientGroupChatRoom::createChatMessage () {
	L_D();
	return d->chatRoom->createChatMessage();
}

shared_ptr<ChatMessage> BasicToClientGroupChatRoom::createChatMessage (const string &text) {
	L_D();
	return d->chatRoom->createChatMessage(text);
}

shared_ptr<ChatMessage> BasicToClientGroupChatRoom::createFileTransferMessage (const LinphoneContent *initialContent) {
	L_D();
	return d->chatRoom->createFileTransferMessage(initialContent);
}

// -----------------------------------------------------------------------------

shared_ptr<ChatMessage> BasicToClientGroupChatRoom::findChatMessage (const string &messageId) const {
	L_D();
	return d->chatRoom->findChatMessage(messageId);
}

shared_ptr<ChatMessage> BasicToClientGroupChatRoom::findChatMessage (
	const string &messageId,
	ChatMessage::Direction direction
) const {
	L_D();
	return d->chatRoom->findChatMessage(messageId, direction);
}

void BasicToClientGroupChatRoom::markAsRead () {
	L_D();
	d->chatRoom->markAsRead();
}

LINPHONE_END_NAMESPACE
