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
#include "core/core-accessor.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatMessage;
class ChatRoom;
class Core;
class EventLog;
class MainDbPrivate;

class LINPHONE_PUBLIC MainDb : public AbstractDb, public CoreAccessor {
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
	bool deleteEvent (const std::shared_ptr<EventLog> &eventLog);
	int getEventsCount (FilterMask mask = NoFilter) const;

	// ---------------------------------------------------------------------------
	// Conference notified events.
	// ---------------------------------------------------------------------------

	std::list<std::shared_ptr<EventLog>> getConferenceNotifiedEvents (
		const std::string &peerAddress,
		unsigned int lastNotifyId
	) const;

	// ---------------------------------------------------------------------------
	// Conference chat message events.
	// ---------------------------------------------------------------------------

	int getChatMessagesCount (const std::string &peerAddress = "") const;
	int getUnreadChatMessagesCount (const std::string &peerAddress = "") const;
	void markChatMessagesAsRead (const std::string &peerAddress = "") const;
	std::list<std::shared_ptr<ChatMessage>> getUnreadChatMessages (const std::string &peerAddress = "") const;

	// ---------------------------------------------------------------------------
	// Conference events.
	// ---------------------------------------------------------------------------

	std::list<std::shared_ptr<EventLog>> getHistory (
		const std::string &peerAddress,
		int nLast,
		FilterMask mask = NoFilter
	) const;
	std::list<std::shared_ptr<EventLog>> getHistoryRange (
		const std::string &peerAddress,
		int begin,
		int end,
		FilterMask mask = NoFilter
	) const;
	void cleanHistory (const std::string &peerAddress = "", FilterMask mask = NoFilter);

	// ---------------------------------------------------------------------------
	// Chat rooms.
	// ---------------------------------------------------------------------------

	std::list<std::shared_ptr<ChatRoom>> getChatRooms () const;
	void insertChatRoom (const std::string &peerAddress, int capabilities);
	void deleteChatRoom (const std::string &peerAddress);

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
