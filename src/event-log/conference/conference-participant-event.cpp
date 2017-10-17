/*
 * conference-participant-event.cpp
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

#include "conference-participant-event-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

ConferenceParticipantEvent::ConferenceParticipantEvent (
	Type type,
	time_t time,
	const Address &conferenceAddress,
	const Address &participantAddress
) : ConferenceEvent(*new ConferenceParticipantEventPrivate, type, time, conferenceAddress) {
	L_D();
	L_ASSERT(
		type == Type::ConferenceParticipantAdded ||
		type == Type::ConferenceParticipantRemoved ||
		type == Type::ConferenceParticipantSetAdmin ||
		type == Type::ConferenceParticipantUnsetAdmin
	);
	d->participantAddress = participantAddress;
}

ConferenceParticipantEvent::ConferenceParticipantEvent (
	const ConferenceParticipantEvent &src
) : ConferenceParticipantEvent(
	src.getType(),
	src.getTime(),
	src.getConferenceAddress(),
	src.getParticipantAddress()
) {}

ConferenceParticipantEvent::ConferenceParticipantEvent (
	ConferenceParticipantEventPrivate &p,
	Type type,
	time_t time,
	const Address &conferenceAddress,
	const Address &participantAddress
) : ConferenceEvent(p, type, time, conferenceAddress) {
	L_D();
	d->participantAddress = participantAddress;
}

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
