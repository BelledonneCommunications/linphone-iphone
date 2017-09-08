/*
 * conference-event.cpp
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

#include "conference-event-p.h"

#include "conference-event.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ConferenceEvent::ConferenceEvent (Type type, const shared_ptr<const Address> &address) :
	EventLog(*new ConferenceEventPrivate, type) {
	L_D(ConferenceEvent);
	L_ASSERT(type == TypeConferenceCreated || type == TypeConferenceDestroyed);
	L_ASSERT(address);
	// TODO: Duplicate address.
	d->address = address;
}

ConferenceEvent::ConferenceEvent (const ConferenceEvent &src) : ConferenceEvent(src.getType(), src.getAddress()) {}

ConferenceEvent::ConferenceEvent (ConferenceEventPrivate &p, Type type, const shared_ptr<const Address> &address) :
	EventLog(p, type) {
	L_D(ConferenceEvent);
	L_ASSERT(address);
	// TODO: Duplicate address.
	d->address = address;
}

ConferenceEvent &ConferenceEvent::operator= (const ConferenceEvent &src) {
	L_D(ConferenceEvent);
	if (this != &src) {
		EventLog::operator=(src);
		// TODO: Duplicate address.
		d->address = src.getPrivate()->address;
	}

	return *this;
}

shared_ptr<const Address> ConferenceEvent::getAddress () const {
	// TODO.
	return nullptr;
}

LINPHONE_END_NAMESPACE
