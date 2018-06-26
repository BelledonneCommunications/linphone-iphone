/*
 * participant-imdn-state.cpp
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

#include "participant-imdn-state-p.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

ParticipantImdnState::ParticipantImdnState (const shared_ptr<Participant> &participant, ChatMessage::State state, time_t stateChangeTime)
	: ClonableObject(*new ParticipantImdnStatePrivate)
{
	L_D();
	d->participant = participant;
	d->state = state;
	d->stateChangeTime = stateChangeTime;
}

ParticipantImdnState::ParticipantImdnState(const ParticipantImdnState &other) : ClonableObject(*new ParticipantImdnStatePrivate) {
	L_D();
	d->participant = other.getParticipant();
	d->state = other.getState();
	d->stateChangeTime = other.getStateChangeTime();
}

// -----------------------------------------------------------------------------

shared_ptr<Participant> ParticipantImdnState::getParticipant () const {
	L_D();
	return d->participant;
}

ChatMessage::State ParticipantImdnState::getState () const {
	L_D();
	return d->state;
}

time_t ParticipantImdnState::getStateChangeTime () const {
	L_D();
	return d->stateChangeTime;
}

LINPHONE_END_NAMESPACE
