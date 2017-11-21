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

#include "handlers/remote-conference-event-handler.h"
#include "participant-p.h"
#include "remote-conference-p.h"
#include "xml/resource-lists.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

RemoteConference::RemoteConference (
	const shared_ptr<Core> &core,
	const IdentityAddress &myAddress,
	CallListener *listener
) : Conference(*new RemoteConferencePrivate, core, myAddress, listener) {
	L_D();
	d->eventHandler.reset(new RemoteConferenceEventHandler(this));
}

RemoteConference::~RemoteConference () {
	L_D();
	d->eventHandler->unsubscribe();
}

// -----------------------------------------------------------------------------

void RemoteConference::addParticipant (const IdentityAddress &addr, const CallSessionParams *params, bool hasMedia) {
	L_D();
	shared_ptr<Participant> participant = findParticipant(addr);
	if (participant)
		return;
	participant = make_shared<Participant>(addr);
	participant->getPrivate()->createSession(*this, params, hasMedia, this);
	d->participants.push_back(participant);
	if (!d->activeParticipant)
		d->activeParticipant = participant;
}

void RemoteConference::removeParticipant (const shared_ptr<const Participant> &participant) {
	L_D();
	for (const auto &p : d->participants) {
		if (participant->getAddress() == p->getAddress()) {
			d->participants.remove(p);
			return;
		}
	}
}

string RemoteConference::getResourceLists (const list<IdentityAddress> &addresses) const {
	Xsd::ResourceLists::ResourceLists rl = Xsd::ResourceLists::ResourceLists();
	Xsd::ResourceLists::ListType l = Xsd::ResourceLists::ListType();
	for (const auto &addr : addresses) {
		Xsd::ResourceLists::EntryType entry = Xsd::ResourceLists::EntryType(addr.asString());
		l.getEntry().push_back(entry);
	}
	rl.getList().push_back(l);

	Xsd::XmlSchema::NamespaceInfomap map;
	stringstream xmlBody;
	serializeResourceLists(xmlBody, rl, map);
	return xmlBody.str();
}

// -----------------------------------------------------------------------------

void RemoteConference::onConferenceCreated (const IdentityAddress &addr) {}

void RemoteConference::onConferenceTerminated (const IdentityAddress &addr) {
	L_D();
	d->eventHandler->unsubscribe();
}

void RemoteConference::onFirstNotifyReceived (const IdentityAddress &addr) {}

void RemoteConference::onParticipantAdded (const std::shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) {
	(void)event;
	(void)isFullState;
}

void RemoteConference::onParticipantRemoved (const std::shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) {
	(void)event;
	(void)isFullState;
}

void RemoteConference::onParticipantSetAdmin (const std::shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) {
	(void)event;
	(void)isFullState;
}

void RemoteConference::onSubjectChanged (const std::shared_ptr<ConferenceSubjectEvent> &event, bool isFullState) {
	(void)event;
	(void)isFullState;
}

void RemoteConference::onParticipantDeviceAdded (const std::shared_ptr<ConferenceParticipantDeviceEvent> &event, bool isFullState) {
	(void)event;
	(void)isFullState;
}

void RemoteConference::onParticipantDeviceRemoved (const std::shared_ptr<ConferenceParticipantDeviceEvent> &event, bool isFullState) {
	(void)event;
	(void)isFullState;
}

LINPHONE_END_NAMESPACE
