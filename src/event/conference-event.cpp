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

#include "event-p.h"

#include "conference-event.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class ConferenceEventPrivate : public EventPrivate {
public:
	shared_ptr<Address> address;
};

// -----------------------------------------------------------------------------

ConferenceEvent::ConferenceEvent (Type type, const shared_ptr<Address> &address) : Event(*new ConferenceEventPrivate, type) {
	L_D(ConferenceEvent);
	L_ASSERT(type == ConferenceCreatedEvent || type == ConferenceDestroyedEvent);
	L_ASSERT(address);
	d->address = address;
}

ConferenceEvent::ConferenceEvent (const ConferenceEvent &src) : ConferenceEvent(src.getType(), src.getAddress()) {}

ConferenceEvent &ConferenceEvent::operator= (const ConferenceEvent &src) {
	L_D(ConferenceEvent);
	if (this != &src) {
		Event::operator=(src);
		d->address = src.getPrivate()->address;
	}

	return *this;
}

LINPHONE_END_NAMESPACE
