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

// From coreapi.
#include "private.h"

#include "chat-room.h"
#include "object/object-p.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatRoomPrivate : public ObjectPrivate {
public:
	virtual ~ChatRoomPrivate ();

private:
	static int refreshComposing (void *data, unsigned int revents);
	static int remoteRefreshComposing (void *data, unsigned int revents);
	static int stopComposing (void *data, unsigned int revents);

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
	void setCBackPointer (LinphoneChatRoom *cr) {
		this->cBackPointer = cr;
	}

	void setCall (LinphoneCall *call) {
		this->call = call;
	}

private:
	std::string createIsComposingXml () const;
	void deleteComposingIdleTimer ();
	void deleteComposingRefreshTimer ();
	void deleteRemoteComposingRefreshTimer ();
	int refreshComposing (unsigned int revents);
	int remoteRefreshComposing (unsigned int revents);
	void sendIsComposingNotification ();
	int stopComposing (unsigned int revents);
	void processImdn (xmlparsing_context_t *xmlCtx);
	void processIsComposingNotification (xmlparsing_context_t *xmlCtx);

	int createChatMessageFromDb (int argc, char **argv, char **colName);
	void onWeakMessageDestroyed (LinphoneChatMessage *messageBeingDestroyed);
	LinphoneChatMessage *getTransientMessage (unsigned int storageId) const;
	LinphoneChatMessage *getWeakMessage (unsigned int storageId) const;
	int sqlRequest (sqlite3 *db, const std::string &stmt);
	void sqlRequestMessage (sqlite3 *db, const std::string &stmt);
	std::list<LinphoneChatMessage *> findMessages (const std::string &messageId);
	LinphoneChatMessage *findMessageWithDirection (const std::string &messageId, LinphoneChatMessageDir direction);
	void storeOrUpdateMessage (LinphoneChatMessage *msg);

public:
	LinphoneReason messageReceived (SalOp *op, const SalMessage *msg);
	void realtimeTextReceived (uint32_t character, LinphoneCall *call);

private:
	void chatMessageReceived (LinphoneChatMessage *msg);
	void imdnReceived (const std::string &text);
	void isComposingReceived (const std::string &text);

public:
	static const int composingDefaultIdleTimeout = 15;
	static const int composingDefaultRefreshTimeout = 60;
	static const int composingDefaultRemoteRefreshTimeout = 120;
	static const std::string imdnPrefix;
	static const std::string isComposingPrefix;

	LinphoneChatRoom *cBackPointer = nullptr;
	LinphoneCore *core = nullptr;
	LinphoneCall *call = nullptr;
	LinphoneAddress *peerAddress = nullptr;
	std::string peer;
	int unreadCount = -1;
	bool isComposing = false;
	bool remoteIsComposing = false;
	belle_sip_source_t *remoteComposingRefreshTimer = nullptr;
	belle_sip_source_t *composingIdleTimer = nullptr;
	belle_sip_source_t *composingRefreshTimer = nullptr;
	std::list<LinphoneChatMessage *> messages;
	std::list<LinphoneChatMessage *> transientMessages;
	std::list<LinphoneChatMessage *> weakMessages;
	std::list<LinphoneChatMessageCharacter *> receivedRttCharacters;
	LinphoneChatMessage *pendingMessage = nullptr;

private:
	L_DECLARE_PUBLIC(ChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CHAT_ROOM_P_H_
