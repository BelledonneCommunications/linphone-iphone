/*
 * event-log.h
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

#ifndef _EVENT_LOG_H_
#define _EVENT_LOG_H_

#include "object/clonable-object.h"
#include "event-log-enums.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class EventLogPrivate;

class LINPHONE_PUBLIC EventLog : public ClonableObject {
public:
	enum Type {
		L_ENUM_VALUES_EVENT_LOG_TYPE
	};

	EventLog ();
	EventLog (const EventLog &src);
	virtual ~EventLog () = default;

	EventLog &operator= (const EventLog &src);

	Type getType () const;

protected:
	EventLog (EventLogPrivate &p, Type type);

private:
	L_DECLARE_PRIVATE(EventLog);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _EVENT_LOG_H_
