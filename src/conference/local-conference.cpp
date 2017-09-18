/*
 * local-conference.cpp
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

#include "local-conference.h"
#include "participant-p.h"

using namespace std;
using namespace LinphonePrivate;

// =============================================================================

LocalConference::LocalConference (LinphoneCore *core, const Address &myAddress, CallListener *listener)
	: Conference(core, myAddress, listener) {
	eventHandler = new LocalConferenceEventHandler(core, this);
}

LocalConference::~LocalConference () {
	delete eventHandler;
}

// -----------------------------------------------------------------------------

shared_ptr<Participant> LocalConference::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	shared_ptr<Participant> participant = findParticipant(addr);
	if (participant)
		return participant;
	participant = make_shared<Participant>(addr);
	participant->getPrivate()->createSession(*this, params, hasMedia, this);
	participants.push_back(participant);
	activeParticipant = participant;
	return participant;
}

void LocalConference::addParticipants (const list<Address> &addresses, const CallSessionParams *params, bool hasMedia) {
	for (const auto &addr : addresses)
		addParticipant(addr, params, hasMedia);
}

bool LocalConference::canHandleParticipants () const {
	return true;
}

const string& LocalConference::getId () const {
	return id;
}

int LocalConference::getNbParticipants () const {
	participants.size();
	return 1;
}

list<shared_ptr<Participant>> LocalConference::getParticipants () const {
	return participants;
}

void LocalConference::removeParticipant (const shared_ptr<Participant> participant) {
	for (const auto &p : participants) {
		if (participant->getAddress().equal(p->getAddress())) {
			participants.remove(p);
			return;
		}
	}
}

void LocalConference::removeParticipants (const list<shared_ptr<Participant>> participants) {
	for (const auto &p : participants)
		removeParticipant(p);
}
