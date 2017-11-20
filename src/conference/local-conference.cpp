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

#include "handlers/local-conference-event-handler.h"
#include "local-conference-p.h"
#include "participant-p.h"
#include "xml/resource-lists.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

LocalConference::LocalConference (const shared_ptr<Core> &core, const Address &myAddress, CallListener *listener)
	: Conference(*new LocalConferencePrivate, core, myAddress, listener) {
	L_D();
	d->eventHandler.reset(new LocalConferenceEventHandler(this));
}

// -----------------------------------------------------------------------------

void LocalConference::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	L_D();
	shared_ptr<Participant> participant = findParticipant(addr);
	if (participant)
		return;
	participant = make_shared<Participant>(addr);
	d->participants.push_back(participant);
	if (!d->activeParticipant)
		d->activeParticipant = participant;
}

void LocalConference::removeParticipant (const shared_ptr<const Participant> &participant) {
	L_D();
	for (const auto &p : d->participants) {
		if (participant->getAddress() == p->getAddress()) {
			d->participants.remove(p);
			return;
		}
	}
}

list<Address> LocalConference::parseResourceLists (const string &xmlBody) {
	istringstream data(xmlBody);
	unique_ptr<Xsd::ResourceLists::ResourceLists> rl = LinphonePrivate::Xsd::ResourceLists::parseResourceLists(
		data,
		Xsd::XmlSchema::Flags::dont_validate
	);
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
