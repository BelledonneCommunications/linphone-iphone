/*
 * remote-conference.cpp
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

#include "handlers/remote-conference-event-handler.h"
#include "logger/logger.h"
#include "participant-p.h"
#include "remote-conference-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

RemoteConference::RemoteConference (
	const shared_ptr<Core> &core,
	const IdentityAddress &myAddress,
	CallSessionListener *listener
) : Conference(*new RemoteConferencePrivate, core, myAddress, listener) {
	L_D();
	d->eventHandler.reset(new RemoteConferenceEventHandler(this));
}

RemoteConference::~RemoteConference () {
	L_D();
	d->eventHandler.reset();
}

// -----------------------------------------------------------------------------

void RemoteConference::addParticipant (const IdentityAddress &addr, const CallSessionParams *params, bool hasMedia) {
	L_D();
	shared_ptr<Participant> participant = findParticipant(addr);
	if (participant) {
		lInfo() << "Not adding participant '" << addr.asString() << "' because it is already a participant of the RemoteConference";
		return;
	}
	participant = make_shared<Participant>(this, addr);
	participant->getPrivate()->createSession(*this, params, hasMedia, d->listener);
	d->participants.push_back(participant);
	if (!d->activeParticipant)
		d->activeParticipant = participant;
}

void RemoteConference::removeParticipant (const shared_ptr<Participant> &participant) {
	L_D();
	for (const auto &p : d->participants) {
		if (participant->getAddress() == p->getAddress()) {
			d->participants.remove(p);
			return;
		}
	}
}

// -----------------------------------------------------------------------------

void RemoteConference::onConferenceCreated (const IdentityAddress &) {}

void RemoteConference::onConferenceTerminated (const IdentityAddress &) {
	L_D();
	d->eventHandler->unsubscribe();
}

void RemoteConference::onFirstNotifyReceived (const IdentityAddress &) {}

void RemoteConference::onParticipantAdded (const std::shared_ptr<ConferenceParticipantEvent> &, bool) {}

void RemoteConference::onParticipantRemoved (const std::shared_ptr<ConferenceParticipantEvent> &, bool) {}

void RemoteConference::onParticipantSetAdmin (const std::shared_ptr<ConferenceParticipantEvent> &, bool) {}

void RemoteConference::onSubjectChanged (const std::shared_ptr<ConferenceSubjectEvent> &, bool) {}

void RemoteConference::onParticipantDeviceAdded (const std::shared_ptr<ConferenceParticipantDeviceEvent> &, bool) {}

void RemoteConference::onParticipantDeviceRemoved (const std::shared_ptr<ConferenceParticipantDeviceEvent> &, bool) {}

LINPHONE_END_NAMESPACE
