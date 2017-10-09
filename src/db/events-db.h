/*
 * events-db.h
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

#ifndef _EVENTS_DB_H_
#define _EVENTS_DB_H_

#include <list>

#include "abstract/abstract-db.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class EventLog;
class EventsDbPrivate;

class LINPHONE_PUBLIC EventsDb : public AbstractDb {
public:
	enum Filter {
		NoFilter = 0x0,
		MessageFilter = 0x1,
		CallFilter = 0x2,
		ConferenceFilter = 0x4
	};

	typedef int FilterMask;

	EventsDb ();

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
	void cleanHistory (const std::string &peerAddress = "");

	bool import (Backend backend, const std::string &parameters) override;

protected:
	void init () override;

private:
	L_DECLARE_PRIVATE(EventsDb);
	L_DISABLE_COPY(EventsDb);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _EVENTS_DB_H_
