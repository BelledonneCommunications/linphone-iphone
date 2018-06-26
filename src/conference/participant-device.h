/*
 * participant-device.h
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

#ifndef _L_PARTICIPANT_DEVICE_H_
#define _L_PARTICIPANT_DEVICE_H_

#include <string>

#include "address/identity-address.h"

#include "linphone/types.h"
#include "linphone/utils/general.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSession;
class Core;
class Participant;

class ParticipantDevice {
public:
	enum class State {
		Joining,
		Present,
		Leaving,
		Left
	};

	ParticipantDevice ();
	explicit ParticipantDevice (Participant *participant, const IdentityAddress &gruu);
	virtual ~ParticipantDevice ();

	bool operator== (const ParticipantDevice &device) const;

	std::shared_ptr<Core> getCore () const;

	inline const IdentityAddress &getAddress () const { return mGruu; }
	Participant *getParticipant () const { return mParticipant; }
	inline std::shared_ptr<CallSession> getSession () const { return mSession; }
	inline void setSession (std::shared_ptr<CallSession> session) { mSession = session; }
	inline State getState () const { return mState; }
	inline void setState (State newState) { mState = newState; }

	inline bool isSubscribedToConferenceEventPackage () const { return mConferenceSubscribeEvent != nullptr; }
	LinphoneEvent *getConferenceSubscribeEvent () const { return mConferenceSubscribeEvent; }
	void setConferenceSubscribeEvent (LinphoneEvent *ev);

	bool isValid () const { return mGruu.isValid(); }

private:
	Participant *mParticipant = nullptr;
	IdentityAddress mGruu;
	std::shared_ptr<CallSession> mSession;
	LinphoneEvent *mConferenceSubscribeEvent = nullptr;
	State mState = State::Joining;

	L_DISABLE_COPY(ParticipantDevice);
};

std::ostream &operator<< (std::ostream &stream, ParticipantDevice::State state);

LINPHONE_END_NAMESPACE

#endif // ifndef _L_PARTICIPANT_DEVICE_H_
