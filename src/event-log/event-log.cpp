/*
 * event-log.cpp
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

#include "event-log-p.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

EventLog::EventLog () : ClonableObject(*new EventLogPrivate) {}

EventLog::EventLog (const EventLog &) : ClonableObject(*new EventLogPrivate) {}

EventLog::EventLog (EventLogPrivate &p, Type type, const time_t &time) : ClonableObject(*new EventLogPrivate) {
	L_D();
	d->type = type;
	d->time = time;
}

EventLog &EventLog::operator= (const EventLog &src) {
	L_D();
	if (this != &src)
		d->type = src.getPrivate()->type;
	return *this;
}

EventLog::Type EventLog::getType () const {
	L_D();
	return d->type;
}

LINPHONE_END_NAMESPACE
