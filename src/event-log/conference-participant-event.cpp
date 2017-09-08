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

#include "conference-event-p.h"

#include "conference-participant-event.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class ConferenceParticipantEventPrivate : public ConferenceEventPrivate {
public:
	shared_ptr<const Address> participantAddress;
};

// -----------------------------------------------------------------------------

ConferenceParticipantEvent::ConferenceParticipantEvent (
	Type type,
	const shared_ptr<const Address> &conferenceAddress,
	const shared_ptr<const Address> &participantAddress
) : ConferenceEvent(*new ConferenceParticipantEventPrivate, type, conferenceAddress) {
	L_D(ConferenceParticipantEvent);
	L_ASSERT(
		type == TypeConferenceParticipantAdded ||
		type == TypeConferenceParticipantRemoved ||
		type == TypeConferenceParticipantSetAdmin ||
		type == TypeConferenceParticipantUnsetAdmin
	);
	L_ASSERT(participantAddress);
	// TODO: Duplicate address.
	d->participantAddress = participantAddress;
}

ConferenceParticipantEvent::ConferenceParticipantEvent (const ConferenceParticipantEvent &src) :
	ConferenceParticipantEvent(src.getType(), src.getAddress(), src.getParticipantAddress()) {}

ConferenceParticipantEvent &ConferenceParticipantEvent::operator= (const ConferenceParticipantEvent &src) {
	L_D(ConferenceParticipantEvent);
	if (this != &src) {
		ConferenceEvent::operator=(src);
		// TODO: Duplicate address.
		d->participantAddress = src.getPrivate()->participantAddress;
	}

	return *this;
}

shared_ptr<const Address> ConferenceParticipantEvent::getParticipantAddress () const {
	// TODO.
	return nullptr;
}

LINPHONE_END_NAMESPACE
