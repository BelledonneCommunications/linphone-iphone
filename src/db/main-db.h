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

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatRoom;
class EventLog;
class MainDbPrivate;

class LINPHONE_PUBLIC MainDb : public AbstractDb {
	friend class ChatRoomProvider;

public:
	enum Filter {
		NoFilter = 0x0,
		MessageFilter = 0x1,
		CallFilter = 0x2,
		ConferenceFilter = 0x4
	};

	typedef int FilterMask;

	MainDb ();

	// Generic.
	bool addEvent (const EventLog &eventLog);
	bool deleteEvent (const EventLog &eventLog);
	void cleanEvents (FilterMask mask = NoFilter);
	int getEventsCount (FilterMask mask = NoFilter) const;

	// Messages, calls and conferences.
	int getMessagesCount (const std::string &peerAddress = "") const;
	int getUnreadMessagesCount (const std::string &peerAddress = "") const;
	std::list<std::shared_ptr<EventLog>> getHistory (
		const std::string &peerAddress,
		int nLast,
		FilterMask mask = NoFilter
	) const;
	std::list<std::shared_ptr<EventLog>> getHistory (
		const std::string &peerAddress,
		int begin,
		int end,
		FilterMask mask = NoFilter
	) const;
	void cleanHistory (const std::string &peerAddress = "", FilterMask mask = NoFilter);

	// ChatRooms.
	std::list<std::shared_ptr<ChatRoom>> getChatRooms () const;
	std::shared_ptr<ChatRoom> findChatRoom (const std::string &peerAddress) const;

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
