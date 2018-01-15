/*
 * conference-participant-event.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
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
	time_t creationTime,
	const ChatRoomId &chatRoomId,
	unsigned int notifyId,
	const IdentityAddress &participantAddress
) : ConferenceNotifiedEvent(
	*new ConferenceParticipantEventPrivate,
	type,
	creationTime,
	chatRoomId,
	notifyId
) {
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
	ConferenceParticipantEventPrivate &p,
	Type type,
	time_t creationTime,
	const ChatRoomId &chatRoomId,
	unsigned int notifyId,
	const IdentityAddress &participantAddress
) : ConferenceNotifiedEvent(
	p,
	type,
	creationTime,
	chatRoomId,
	notifyId
) {
	L_D();
	d->participantAddress = participantAddress;
}

const IdentityAddress &ConferenceParticipantEvent::getParticipantAddress () const {
	L_D();
	return d->participantAddress;
}

LINPHONE_END_NAMESPACE
