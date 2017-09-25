/*
 * remote-conference.cpp
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

#include "remote-conference.h"
#include "participant-p.h"
#include "xml/resource-lists.h"

using namespace std;
using namespace LinphonePrivate::Xsd::ResourceLists;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

RemoteConference::RemoteConference (LinphoneCore *core, const Address &myAddress, CallListener *listener)
	: Conference(core, myAddress, listener) {
	eventHandler = new RemoteConferenceEventHandler(core, this);
}

RemoteConference::~RemoteConference () {
	delete eventHandler;
}

// -----------------------------------------------------------------------------

shared_ptr<Participant> RemoteConference::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	shared_ptr<Participant> participant = findParticipant(addr);
	if (participant)
		return participant;
	participant = make_shared<Participant>(addr);
	participant->getPrivate()->createSession(*this, params, hasMedia, this);
	participants.push_back(participant);
	if (!activeParticipant)
		activeParticipant = participant;
	return participant;
}

void RemoteConference::removeParticipant (const shared_ptr<const Participant> &participant) {
	for (const auto &p : participants) {
		if (participant->getAddress().equal(p->getAddress())) {
			participants.remove(p);
			return;
		}
	}
}


string RemoteConference::getResourceLists (const list<Address> &addresses) {
	ResourceLists rl = ResourceLists();
	ListType l = ListType();
	for (const auto &addr : addresses) {
		EntryType entry = EntryType(addr.asStringUriOnly());
		if (!addr.getDisplayName().empty())
			entry.setDisplayName(DisplayName(addr.getDisplayName()));
		l.getEntry().push_back(entry);
	}
	rl.getList().push_back(l);

	Xsd::XmlSchema::NamespaceInfomap map;
	stringstream xmlBody;
	serializeResourceLists(xmlBody, rl, map);
	return xmlBody.str();
}

// -----------------------------------------------------------------------------

void RemoteConference::onConferenceCreated (const Address &addr) {}

void RemoteConference::onConferenceTerminated (const Address &addr) {}

void RemoteConference::onParticipantAdded (const Address &addr) {}

void RemoteConference::onParticipantRemoved (const Address &addr) {}

void RemoteConference::onParticipantSetAdmin (const Address &addr, bool isAdmin) {}

LINPHONE_END_NAMESPACE
