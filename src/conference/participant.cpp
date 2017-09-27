/*
 * participant.cpp
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

#include "object/object-p.h"
#include "participant-p.h"

#include "participant.h"
#include "params/media-session-params.h"
#include "session/media-session.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

shared_ptr<CallSession> ParticipantPrivate::createSession (const Conference &conference, const CallSessionParams *params, bool hasMedia, CallSessionListener *listener) {
	if (hasMedia && (!params || dynamic_cast<const MediaSessionParams *>(params))) {
		session = make_shared<MediaSession>(conference, params, listener);
	} else {
		session = make_shared<CallSession>(conference, params, listener);
	}
	return session;
}

shared_ptr<CallSession> ParticipantPrivate::getSession () const {
	return session;
}

// =============================================================================

Participant::Participant (const Address &addr) : Object(*new ParticipantPrivate) {
	L_D();
	d->addr = addr;
}

// -----------------------------------------------------------------------------

const Address& Participant::getAddress () const {
	L_D();
	return d->addr;
}

// -----------------------------------------------------------------------------

bool Participant::isAdmin () const {
	L_D();
	return d->isAdmin;
}

void Participant::setAdmin (bool isAdmin) {
	L_D();
	d->isAdmin = isAdmin;
}

// =============================================================================

ostream & operator<< (ostream &strm, const shared_ptr<Participant> &participant) {
	return strm << "'" << participant->getAddress().asString() << "'";
}

LINPHONE_END_NAMESPACE
