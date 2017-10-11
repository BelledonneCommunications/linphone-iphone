/*
 * remote-conference.cpp
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
	eventHandler->unsubscribe();
	delete eventHandler;
}

// -----------------------------------------------------------------------------

void RemoteConference::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	shared_ptr<Participant> participant = findParticipant(addr);
	if (participant)
		return;
	participant = ObjectFactory::create<Participant>(addr);
	participant->getPrivate()->createSession(*this, params, hasMedia, this);
	participants.push_back(participant);
	if (!activeParticipant)
		activeParticipant = participant;
}

void RemoteConference::removeParticipant (const shared_ptr<const Participant> &participant) {
	for (const auto &p : participants) {
		if (participant->getAddress() == p->getAddress()) {
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

void RemoteConference::onConferenceTerminated (const Address &addr) {
	eventHandler->unsubscribe();
}

void RemoteConference::onParticipantAdded (const Address &addr) {}

void RemoteConference::onParticipantRemoved (const Address &addr) {}

void RemoteConference::onParticipantSetAdmin (const Address &addr, bool isAdmin) {}

void RemoteConference::onSubjectChanged (const std::string &subject) {}

void RemoteConference::onParticipantDeviceAdded (const Address &addr, const Address &gruu) {}

void RemoteConference::onParticipantDeviceRemoved (const Address &addr, const Address &gruu) {}

LINPHONE_END_NAMESPACE
