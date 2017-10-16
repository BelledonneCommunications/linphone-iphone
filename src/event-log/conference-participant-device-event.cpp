/*
 * conference-participant-device-event.cpp
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

#include "conference-participant-device-event.h"
#include "conference-participant-event-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class ConferenceParticipantDeviceEventPrivate : public ConferenceParticipantEventPrivate {
public:
	Address gruuAddress;
};

// -----------------------------------------------------------------------------

ConferenceParticipantDeviceEvent::ConferenceParticipantDeviceEvent (
	Type type,
	const Address &conferenceAddress,
	const Address &participantAddress,
	const Address &gruuAddress
) : ConferenceParticipantEvent(
	*new ConferenceParticipantDeviceEventPrivate,
	type,
	conferenceAddress,
	participantAddress
) {
	L_D();
	L_ASSERT(
		type == Type::ConferenceParticipantDeviceAdded ||
		type == Type::ConferenceParticipantDeviceRemoved
	);
	d->gruuAddress = gruuAddress;
}

ConferenceParticipantDeviceEvent::ConferenceParticipantDeviceEvent (const ConferenceParticipantDeviceEvent &src) :
	ConferenceParticipantDeviceEvent(
		src.getType(),
		src.getAddress(),
		src.getParticipantAddress(),
		src.getGruuAddress()
	) {}

ConferenceParticipantDeviceEvent &ConferenceParticipantDeviceEvent::operator= (
	const ConferenceParticipantDeviceEvent &src
) {
	L_D();
	if (this != &src) {
		ConferenceParticipantEvent::operator=(src);
		d->gruuAddress = src.getPrivate()->gruuAddress;
	}

	return *this;
}

const Address &ConferenceParticipantDeviceEvent::getGruuAddress () const {
	L_D();
	return d->gruuAddress;
}

LINPHONE_END_NAMESPACE
