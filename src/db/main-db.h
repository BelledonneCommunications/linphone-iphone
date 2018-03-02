/*
 * main-db.h
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

#ifndef _L_MAIN_DB_H_
#define _L_MAIN_DB_H_

#include <list>

#include "linphone/utils/enum-mask.h"

#include "abstract/abstract-db.h"
#include "chat/chat-message/chat-message.h"
#include "chat/chat-room/chat-room-id.h"
#include "core/core-accessor.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class AbstractChatRoom;
class ChatMessage;
class Core;
class EventLog;
class MainDbKey;
class MainDbPrivate;
class ParticipantDevice;

class MainDb : public AbstractDb, public CoreAccessor {
	template<typename Function>
	friend class DbTransaction;

	friend class MainDbChatMessageKey;
	friend class MainDbEventKey;

public:
	enum Filter {
		NoFilter = 0x0,
		ConferenceCallFilter = 0x1,
		ConferenceChatMessageFilter = 0x2,
		ConferenceInfoFilter = 0x4,
		ConferenceInfoNoDeviceFilter = 0x8
	};

	typedef EnumMask<Filter> FilterMask;

	MainDb (const std::shared_ptr<Core> &core);

	// ---------------------------------------------------------------------------
	// Generic.
	// ---------------------------------------------------------------------------

	bool addEvent (const std::shared_ptr<EventLog> &eventLog);
	bool updateEvent (const std::shared_ptr<EventLog> &eventLog);
	static bool deleteEvent (const std::shared_ptr<const EventLog> &eventLog);
	int getEventCount (FilterMask mask = NoFilter) const;

	static std::shared_ptr<EventLog> getEventFromKey (const MainDbKey &dbKey);

	// ---------------------------------------------------------------------------
	// Conference notified events.
	// ---------------------------------------------------------------------------

	std::list<std::shared_ptr<EventLog>> getConferenceNotifiedEvents (
		const ChatRoomId &chatRoomId,
		unsigned int lastNotifyId
	) const;

	// ---------------------------------------------------------------------------
	// Conference chat message events.
	// ---------------------------------------------------------------------------

	int getChatMessageCount (const ChatRoomId &chatRoomId = ChatRoomId()) const;
	int getUnreadChatMessageCount (const ChatRoomId &chatRoomId = ChatRoomId()) const;
	void markChatMessagesAsRead (const ChatRoomId &chatRoomId) const;
	std::list<std::shared_ptr<ChatMessage>> getUnreadChatMessages (const ChatRoomId &chatRoomId) const;

	std::list<ChatMessage::State> getChatMessageParticipantStates (const std::shared_ptr<EventLog> &eventLog) const;
	ChatMessage::State getChatMessageParticipantState (
		const std::shared_ptr<EventLog> &eventLog,
		const IdentityAddress &participantAddress
	) const;
	void setChatMessageParticipantState (
		const std::shared_ptr<EventLog> &eventLog,
		const IdentityAddress &participantAddress,
		ChatMessage::State state
	);

	std::shared_ptr<ChatMessage> getLastChatMessage (const ChatRoomId &chatRoomId) const;

	std::list<std::shared_ptr<ChatMessage>> findChatMessages (
		const ChatRoomId &chatRoomId,
		const std::string &imdnMessageId
	) const;

	// ---------------------------------------------------------------------------
	// Conference events.
	// ---------------------------------------------------------------------------

	std::list<std::shared_ptr<EventLog>> getHistory (
		const ChatRoomId &chatRoomId,
		int nLast,
		FilterMask mask = NoFilter
	) const;
	std::list<std::shared_ptr<EventLog>> getHistoryRange (
		const ChatRoomId &chatRoomId,
		int begin,
		int end,
		FilterMask mask = NoFilter
	) const;

	int getHistorySize (const ChatRoomId &chatRoomId, FilterMask mask = NoFilter) const;

	void cleanHistory (const ChatRoomId &chatRoomId, FilterMask mask = NoFilter);

	// ---------------------------------------------------------------------------
	// Chat messages.
	// ---------------------------------------------------------------------------

	void loadChatMessageContents (const std::shared_ptr<ChatMessage> &chatMessage);

	// ---------------------------------------------------------------------------
	// Chat rooms.
	// ---------------------------------------------------------------------------

	std::list<std::shared_ptr<AbstractChatRoom>> getChatRooms () const;
	void insertChatRoom (const std::shared_ptr<AbstractChatRoom> &chatRoom);
	void deleteChatRoom (const ChatRoomId &chatRoomId);
	void enableChatRoomMigration (const ChatRoomId &chatRoomId, bool enable);

	void migrateBasicToClientGroupChatRoom (
		const std::shared_ptr<AbstractChatRoom> &basicChatRoom,
		const std::shared_ptr<AbstractChatRoom> &clientGroupChatRoom
	);

	IdentityAddress findMissingOneToOneConferenceChatRoomParticipantAddress (
		const std::shared_ptr<AbstractChatRoom> &chatRoom,
		const IdentityAddress &presentParticipantAddr
	);
	IdentityAddress findOneToOneConferenceChatRoomAddress (
		const IdentityAddress &participantA,
		const IdentityAddress &participantB
	) const;
	void insertOneToOneConferenceChatRoom (const std::shared_ptr<AbstractChatRoom> &chatRoom);

	void updateChatRoomParticipantDevice (
		const std::shared_ptr<AbstractChatRoom> &chatRoom,
		const std::shared_ptr<ParticipantDevice> &device
	);

	// ---------------------------------------------------------------------------
	// Other.
	// ---------------------------------------------------------------------------

	// Import legacy calls/messages from old db.
	bool import (Backend backend, const std::string &parameters) override;

protected:
	void init () override;

private:
	L_DECLARE_PRIVATE(MainDb);
	L_DISABLE_COPY(MainDb);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_MAIN_DB_H_
