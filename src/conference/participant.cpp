/*
 * participant.cpp
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

#include <algorithm>

#include "object/object-p.h"
#include "participant-device.h"
#include "participant-p.h"

#include "participant.h"
#include "params/media-session-params.h"
#include "session/media-session.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

shared_ptr<CallSession> ParticipantPrivate::createSession (
	const Conference &conference, const CallSessionParams *params, bool hasMedia, CallSessionListener *listener
) {
	L_Q();
	if (hasMedia && (!params || dynamic_cast<const MediaSessionParams *>(params))) {
		session = make_shared<MediaSession>(conference.getCore(), q->getSharedFromThis(), params, listener);
	} else {
		session = make_shared<CallSession>(conference.getCore(), params, listener);
	}
	return session;
}

// -----------------------------------------------------------------------------

shared_ptr<ParticipantDevice> ParticipantPrivate::addDevice (const IdentityAddress &gruu) {
	L_Q();
	shared_ptr<ParticipantDevice> device = findDevice(gruu);
	if (device)
		return device;
	device = make_shared<ParticipantDevice>(q, gruu);
	devices.push_back(device);
	return device;
}

void ParticipantPrivate::clearDevices () {
	devices.clear();
}

shared_ptr<ParticipantDevice> ParticipantPrivate::findDevice (const IdentityAddress &gruu) const {
	for (const auto &device : devices) {
		if (device->getAddress() == gruu)
			return device;
	}
	return nullptr;
}

shared_ptr<ParticipantDevice> ParticipantPrivate::findDevice (const shared_ptr<const CallSession> &session) {
	for (const auto &device : devices) {
		if (device->getSession() == session)
			return device;
	}
	return nullptr;
}

const list<shared_ptr<ParticipantDevice>> &ParticipantPrivate::getDevices () const {
	return devices;
}

void ParticipantPrivate::removeDevice (const IdentityAddress &gruu) {
	for (auto it = devices.begin(); it != devices.end(); it++) {
		if ((*it)->getAddress() == gruu) {
			devices.erase(it);
			return;
		}
	}
}

// =============================================================================

Participant::Participant (Conference *conference, const IdentityAddress &address) : Object(*new ParticipantPrivate) {
	L_D();
	d->mConference = conference;
	d->addr = address.getAddressWithoutGruu();
}

// -----------------------------------------------------------------------------

const IdentityAddress& Participant::getAddress () const {
	L_D();
	return d->addr;
}

// -----------------------------------------------------------------------------

bool Participant::isAdmin () const {
	L_D();
	return d->isAdmin;
}

LINPHONE_END_NAMESPACE
