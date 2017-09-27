/*
 * conference-participant-event.cpp
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

#include "address/address.h"
#include "conference-event-p.h"
#include "conference-participant-event.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class ConferenceParticipantEventPrivate : public ConferenceEventPrivate {
public:
	Address participantAddress;
};

// -----------------------------------------------------------------------------

ConferenceParticipantEvent::ConferenceParticipantEvent (
	Type type,
	const Address &conferenceAddress,
	const Address &participantAddress
) : ConferenceEvent(*new ConferenceParticipantEventPrivate, type, conferenceAddress) {
	L_D();
	L_ASSERT(
		type == Type::ConferenceParticipantAdded ||
		type == Type::ConferenceParticipantRemoved ||
		type == Type::ConferenceParticipantSetAdmin ||
		type == Type::ConferenceParticipantUnsetAdmin
	);
	d->participantAddress = participantAddress;
}

ConferenceParticipantEvent::ConferenceParticipantEvent (const ConferenceParticipantEvent &src) :
	ConferenceParticipantEvent(src.getType(), src.getAddress(), src.getParticipantAddress()) {}

ConferenceParticipantEvent &ConferenceParticipantEvent::operator= (const ConferenceParticipantEvent &src) {
	L_D();
	if (this != &src) {
		ConferenceEvent::operator=(src);
		d->participantAddress = src.getPrivate()->participantAddress;
	}

	return *this;
}

const Address &ConferenceParticipantEvent::getParticipantAddress () const {
	L_D();
	return d->participantAddress;
}

LINPHONE_END_NAMESPACE
