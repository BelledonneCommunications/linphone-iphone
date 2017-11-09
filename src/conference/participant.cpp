/*
 * participant.cpp
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

#include <algorithm>

#include "object/object-p.h"
#include "participant-p.h"

#include "participant.h"
#include "params/media-session-params.h"
#include "session/media-session.h"

#include "linphone/event.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

ParticipantPrivate::~ParticipantPrivate () {
	if (conferenceSubscribeEvent)
		linphone_event_unref(conferenceSubscribeEvent);
}

// -----------------------------------------------------------------------------

shared_ptr<CallSession> ParticipantPrivate::createSession (
	const Conference &conference, const CallSessionParams *params, bool hasMedia, CallSessionListener *listener
) {
	if (hasMedia && (!params || dynamic_cast<const MediaSessionParams *>(params))) {
		session = make_shared<MediaSession>(conference, params, listener);
	} else {
		session = make_shared<CallSession>(conference, params, listener);
	}
	return session;
}

// -----------------------------------------------------------------------------

void ParticipantPrivate::setConferenceSubscribeEvent (LinphoneEvent *ev) {
	if (conferenceSubscribeEvent)
		linphone_event_unref(conferenceSubscribeEvent);
	conferenceSubscribeEvent = linphone_event_ref(ev);
}

// -----------------------------------------------------------------------------

const list<ParticipantDevice>::const_iterator ParticipantPrivate::findDevice (const Address &gruu) const {
	ParticipantDevice device(gruu);
	return find(devices.cbegin(), devices.cend(), device);
}

const list<ParticipantDevice> &ParticipantPrivate::getDevices () const {
	return devices;
}

void ParticipantPrivate::addDevice (const Address &gruu) {
	ParticipantDevice device(gruu);
	if(findDevice(gruu) == devices.cend())
		devices.push_back(device);
}

void ParticipantPrivate::removeDevice (const Address &gruu) {
	ParticipantDevice device(gruu);
	if(findDevice(gruu) != devices.cend())
		devices.remove(device);
}

// =============================================================================

Participant::Participant (const Address &address) : Object(*new ParticipantPrivate) {
	L_D();
	d->contactAddr = address;
	d->addr = SimpleAddress(address);
}

Participant::Participant (Address &&address) : Object(*new ParticipantPrivate) {
	L_D();
	d->contactAddr = move(address);
	d->addr = SimpleAddress(address);
}

// -----------------------------------------------------------------------------

const SimpleAddress& Participant::getAddress () const {
	L_D();
	return d->addr;
}

const Address& Participant::getContactAddress () const {
	L_D();
	return d->contactAddr;
}

// -----------------------------------------------------------------------------

bool Participant::isAdmin () const {
	L_D();
	return d->isAdmin;
}

// =============================================================================

ostream & operator<< (ostream &strm, const shared_ptr<Participant> &participant) {
	return strm << "'" << participant->getAddress().asString() << "'";
}

LINPHONE_END_NAMESPACE
