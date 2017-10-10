/*
 * local-conference.cpp
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

#include "local-conference.h"
#include "participant-p.h"
#include "xml/resource-lists.h"

using namespace std;
using namespace LinphonePrivate::Xsd::ResourceLists;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

LocalConference::LocalConference (LinphoneCore *core, const Address &myAddress, CallListener *listener)
	: Conference(core, myAddress, listener) {
	eventHandler = new LocalConferenceEventHandler(core, this);
}

LocalConference::~LocalConference () {
	delete eventHandler;
}

// -----------------------------------------------------------------------------

void LocalConference::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	shared_ptr<Participant> participant = findParticipant(addr);
	if (participant)
		return;
	participant = ObjectFactory::create<Participant>(addr);
	participants.push_back(participant);
	if (!activeParticipant)
		activeParticipant = participant;
}

void LocalConference::removeParticipant (const shared_ptr<const Participant> &participant) {
	for (const auto &p : participants) {
		if (participant->getAddress() == p->getAddress()) {
			participants.remove(p);
			return;
		}
	}
}

list<Address> LocalConference::parseResourceLists (string xmlBody) {
	istringstream data(xmlBody);
	unique_ptr<ResourceLists> rl = LinphonePrivate::Xsd::ResourceLists::parseResourceLists(data, Xsd::XmlSchema::Flags::dont_validate);
	list<Address> addresses = list<Address>();
	for (const auto &l : rl->getList()) {
		for (const auto &entry : l.getEntry()) {
			Address addr(entry.getUri());
			if (entry.getDisplayName().present())
				addr.setDisplayName(entry.getDisplayName().get());
			addresses.push_back(addr);
		}
	}
	return addresses;
}

LINPHONE_END_NAMESPACE
