/*
 * conference.cpp
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

#include "conference-p.h"
#include "conference/session/call-session-p.h"
#include "logger/logger.h"
#include "participant-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

Conference::Conference (
	ConferencePrivate &p,
	const shared_ptr<Core> &core,
	const IdentityAddress &myAddress,
	CallSessionListener *listener
) : CoreAccessor(core), mPrivate(&p) {
	L_D();
	d->mPublic = this;
	d->me = make_shared<Participant>(myAddress);
	d->listener = listener;
}

Conference::~Conference () {
	delete mPrivate;
}

// -----------------------------------------------------------------------------

shared_ptr<Participant> Conference::getActiveParticipant () const {
	L_D();
	return d->activeParticipant;
}

// -----------------------------------------------------------------------------

void Conference::addParticipant (const IdentityAddress &addr, const CallSessionParams *params, bool hasMedia) {
	lError() << "Conference class does not handle addParticipant() generically";
}

void Conference::addParticipants (const list<IdentityAddress> &addresses, const CallSessionParams *params, bool hasMedia) {
	list<IdentityAddress> sortedAddresses(addresses);
	sortedAddresses.sort();
	sortedAddresses.unique();
	for (const auto &addr: sortedAddresses) {
		shared_ptr<Participant> participant = findParticipant(addr);
		if (!participant)
			addParticipant(addr, params, hasMedia);
	}
}

bool Conference::canHandleParticipants () const {
	return true;
}

const IdentityAddress &Conference::getConferenceAddress () const {
	L_D();
	return d->conferenceAddress;
}

shared_ptr<Participant> Conference::getMe () const {
	L_D();
	return d->me;
}

int Conference::getParticipantCount () const {
	L_D();
	return static_cast<int>(d->participants.size());
}

const list<shared_ptr<Participant>> &Conference::getParticipants () const {
	L_D();
	return d->participants;
}

const string &Conference::getSubject () const {
	L_D();
	return d->subject;
}

void Conference::join () {}

void Conference::leave () {}

void Conference::removeParticipant (const shared_ptr<const Participant> &participant) {
	lError() << "Conference class does not handle removeParticipant() generically";
}

void Conference::removeParticipants (const list<shared_ptr<Participant>> &participants) {
	for (const auto &p : participants)
		removeParticipant(p);
}

void Conference::setParticipantAdminStatus (shared_ptr<Participant> &participant, bool isAdmin) {
	lError() << "Conference class does not handle setParticipantAdminStatus() generically";
}

void Conference::setSubject (const string &subject) {
	L_D();
	d->subject = subject;
}

// -----------------------------------------------------------------------------

shared_ptr<Participant> Conference::findParticipant (const IdentityAddress &addr) const {
	L_D();

	IdentityAddress searchedAddr(addr);
	searchedAddr.setGruu("");
	for (const auto &participant : d->participants) {
		if (participant->getAddress() == searchedAddr)
			return participant;
	}

	return nullptr;
}

shared_ptr<Participant> Conference::findParticipant (const shared_ptr<const CallSession> &session) const {
	L_D();

	for (const auto &participant : d->participants) {
		if (participant->getPrivate()->getSession() == session)
			return participant;
	}

	return nullptr;
}

bool Conference::isMe (const IdentityAddress &addr) const {
	L_D();
	IdentityAddress cleanedAddr(addr);
	cleanedAddr.setGruu("");
	IdentityAddress cleanedMeAddr(d->me->getAddress());
	cleanedMeAddr.setGruu("");
	return cleanedMeAddr == cleanedAddr;
}

LINPHONE_END_NAMESPACE
