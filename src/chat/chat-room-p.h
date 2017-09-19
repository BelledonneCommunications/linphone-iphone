/*
 * chat-room-p.h
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CHAT_ROOM_P_H_
#define _CHAT_ROOM_P_H_

#include "linphone/enums/chat-room-enums.h"
#include "linphone/utils/enum-generator.h"

// From coreapi.
#include "private.h"

#include "chat-room.h"
#include "is-composing-listener.h"
#include "is-composing.h"
#include "object/object-p.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatRoomPrivate : public ObjectPrivate, public IsComposingListener {
public:
	ChatRoomPrivate (LinphoneCore *core);
	virtual ~ChatRoomPrivate ();

private:
	static int createChatMessageFromDb (void *data, int argc, char **argv, char **colName);
	static void onWeakMessageDestroyed (void *obj, belle_sip_object_t *messageBeingDestroyed);

public:
	void addTransientMessage (LinphoneChatMessage *msg);
	void addWeakMessage (LinphoneChatMessage *msg);
	std::list<LinphoneChatMessage *> getTransientMessages () const {
		return transientMessages;
	}
	void moveTransientMessageToWeakMessages (LinphoneChatMessage *msg);
	void removeTransientMessage (LinphoneChatMessage *msg);

	void release ();
	void sendImdn (const std::string &content, LinphoneReason reason);

	int getMessagesCount (bool unreadOnly);
	void setState (ChatRoom::State newState);

protected:
	void sendIsComposingNotification ();

	int createChatMessageFromDb (int argc, char **argv, char **colName);
	void onWeakMessageDestroyed (LinphoneChatMessage *messageBeingDestroyed);
	LinphoneChatMessage *getTransientMessage (unsigned int storageId) const;
	LinphoneChatMessage *getWeakMessage (unsigned int storageId) const;
	int sqlRequest (sqlite3 *db, const std::string &stmt);
	void sqlRequestMessage (sqlite3 *db, const std::string &stmt);
	std::list<LinphoneChatMessage *> findMessages (const std::string &messageId);
	void storeOrUpdateMessage (LinphoneChatMessage *msg);

public:
	LinphoneReason messageReceived (SalOp *op, const SalMessage *msg);
	void realtimeTextReceived (uint32_t character, LinphoneCall *call);

protected:
	void chatMessageReceived (LinphoneChatMessage *msg);
	void imdnReceived (const std::string &text);
	void isComposingReceived (const std::string &text);

private:
	void notifyChatMessageReceived (LinphoneChatMessage *msg);
	void notifyStateChanged ();
	void notifyUndecryptableMessageReceived (LinphoneChatMessage *msg);

private:
	/* IsComposingListener */
	void onIsComposingStateChanged (bool isComposing) override;
	void onIsRemoteComposingStateChanged (bool isComposing) override;
	void onIsComposingRefreshNeeded () override;

public:
	LinphoneCore *core = nullptr;
	LinphoneCall *call = nullptr;
	ChatRoom::State state = ChatRoom::State::None;
	Address peerAddress;
	int unreadCount = -1;
	bool isComposing = false;
	bool remoteIsComposing = false;
	std::list<LinphoneChatMessage *> messages;
	std::list<LinphoneChatMessage *> transientMessages;
	std::list<LinphoneChatMessage *> weakMessages;
	std::list<LinphoneChatMessageCharacter *> receivedRttCharacters;
	LinphoneChatMessage *pendingMessage = nullptr;
	IsComposing isComposingHandler;

private:
	L_DECLARE_PUBLIC(ChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CHAT_ROOM_P_H_
