/*
 * participant-imdn-state.h
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

#ifndef _L_PARTICIPANT_IMDN_STATE_H_
#define _L_PARTICIPANT_IMDN_STATE_H_

#include "chat/chat-message/chat-message.h"
#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Participant;
class ParticipantImdnStatePrivate;

class ParticipantImdnState : public ClonableObject {
public:
	ParticipantImdnState (const std::shared_ptr<Participant> &participant, ChatMessage::State state, time_t stateChangeTime);
	ParticipantImdnState (const ParticipantImdnState &other);

	std::shared_ptr<Participant> getParticipant () const;
	ChatMessage::State getState () const;
	time_t getStateChangeTime () const;

private:
	L_DECLARE_PRIVATE(ParticipantImdnState);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_PARTICIPANT_IMDN_STATE_H_
