/*
 * events-db.h
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

#ifndef _EVENTS_DB_H_
#define _EVENTS_DB_H_

#include "abstract/abstract-db.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Event;
class EventsDbPrivate;

class LINPHONE_PUBLIC EventsDb : public AbstractDb {
public:
	EventsDb ();

	bool writeEvent (const Event &event);

protected:
	void init () override;

private:
	L_DECLARE_PRIVATE(EventsDb);
	L_DISABLE_COPY(EventsDb);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _EVENTS_DB_H_
