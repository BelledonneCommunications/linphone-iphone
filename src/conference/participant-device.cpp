/*
 * participant-device.cpp
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

#include "participant-device.h"
#include "participant-p.h"

#include "linphone/event.h"

using namespace std;

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

ParticipantDevice::ParticipantDevice () {}

ParticipantDevice::ParticipantDevice (Participant *participant, const IdentityAddress &gruu)
	: mParticipant(participant), mGruu(gruu) {}

ParticipantDevice::~ParticipantDevice () {
	if (mConferenceSubscribeEvent)
		linphone_event_unref(mConferenceSubscribeEvent);
}

bool ParticipantDevice::operator== (const ParticipantDevice &device) const {
	return (mGruu == device.getAddress());
}

shared_ptr<Core> ParticipantDevice::getCore () const {
	return mParticipant ? mParticipant->getPrivate()->getCore() : nullptr;
}

void ParticipantDevice::setConferenceSubscribeEvent (LinphoneEvent *ev) {
	if (mConferenceSubscribeEvent)
		linphone_event_unref(mConferenceSubscribeEvent);
	mConferenceSubscribeEvent = linphone_event_ref(ev);
}

ostream &operator<< (ostream &stream, ParticipantDevice::State state) {
	switch (state) {
		case ParticipantDevice::State::Joining:
			return stream << "Joining";
		case ParticipantDevice::State::Present:
			return stream << "Present";
		case ParticipantDevice::State::Leaving:
			return stream << "Leaving";
		case ParticipantDevice::State::Left:
			return stream << "Left";
	}
	return stream;
}

LINPHONE_END_NAMESPACE
