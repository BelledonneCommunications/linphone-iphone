/*
 * main-db.h
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

#ifndef _MAIN_DB_H_
#define _MAIN_DB_H_

#include <list>

#include "abstract/abstract-db.h"
#include "chat/chat-room/chat-room-id.h"
#include "core/core-accessor.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatMessage;
class ChatRoom;
class Core;
class EventLog;
class MainDbPrivate;

class MainDb : public AbstractDb, public CoreAccessor {
	friend class MainDbEventKey;

public:
	enum Filter {
		NoFilter = 0x0,
		ConferenceCallFilter = 0x1,
		ConferenceChatMessageFilter = 0x2,
		ConferenceInfoFilter = 0x4
	};

	typedef int FilterMask;

	MainDb (const std::shared_ptr<Core> &core);

	// ---------------------------------------------------------------------------
	// Generic.
	// ---------------------------------------------------------------------------

	bool addEvent (const std::shared_ptr<EventLog> &eventLog);
	bool updateEvent (const std::shared_ptr<EventLog> &eventLog);
	static bool deleteEvent (const std::shared_ptr<EventLog> &eventLog);
	int getEventsCount (FilterMask mask = NoFilter) const;

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

	int getChatMessagesCount (const ChatRoomId &chatRoomId = ChatRoomId()) const;
	int getUnreadChatMessagesCount (const ChatRoomId &chatRoomId = ChatRoomId()) const;
	void markChatMessagesAsRead (const ChatRoomId &chatRoomId = ChatRoomId()) const;
	std::list<std::shared_ptr<ChatMessage>> getUnreadChatMessages (const ChatRoomId &chatRoomId = ChatRoomId()) const;

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
	void cleanHistory (const ChatRoomId &chatRoomId, FilterMask mask = NoFilter);

	// ---------------------------------------------------------------------------
	// Chat rooms.
	// ---------------------------------------------------------------------------

	std::list<std::shared_ptr<ChatRoom>> getChatRooms () const;
	void insertChatRoom (const ChatRoomId &chatRoomId, int capabilities, const std::string &subject);
	void deleteChatRoom (const ChatRoomId &chatRoomId);

	// ---------------------------------------------------------------------------
	// Other.
	// ---------------------------------------------------------------------------

	// Import legacy messages from old db.
	bool import (Backend backend, const std::string &parameters) override;

protected:
	void init () override;

private:
	L_DECLARE_PRIVATE(MainDb);
	L_DISABLE_COPY(MainDb);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _MAIN_DB_H_
