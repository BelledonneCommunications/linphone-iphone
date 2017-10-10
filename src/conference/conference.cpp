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

#include "participant-p.h"

#include "conference.h"
#include "conference/session/call-session-p.h"
#include "logger/logger.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

Conference::Conference (LinphoneCore *core, const Address &myAddress, CallListener *listener)
	: core(core), callListener(listener) {
	me = ObjectFactory::create<Participant>(myAddress);
}

// -----------------------------------------------------------------------------

shared_ptr<Participant> Conference::getActiveParticipant () const {
	return activeParticipant;
}

// -----------------------------------------------------------------------------

void Conference::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	lError() << "Conference class does not handle addParticipant() generically";
}

void Conference::addParticipants (const list<Address> &addresses, const CallSessionParams *params, bool hasMedia) {
	list<Address> sortedAddresses(addresses);
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

const Address *Conference::getConferenceAddress () const {
	return &conferenceAddress;
}

int Conference::getNbParticipants () const {
	return static_cast<int>(participants.size());
}

list<shared_ptr<Participant>> Conference::getParticipants () const {
	return participants;
}

const string &Conference::getSubject () const {
	return subject;
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

void Conference::setSubject (const string &subject) {
	this->subject = subject;
}

// -----------------------------------------------------------------------------

void Conference::onAckBeingSent (const std::shared_ptr<const CallSession> &session, LinphoneHeaders *headers) {
	if (callListener)
		callListener->onAckBeingSent(headers);
}

void Conference::onAckReceived (const std::shared_ptr<const CallSession> &session, LinphoneHeaders *headers) {
	if (callListener)
		callListener->onAckReceived(headers);
}

void Conference::onCallSessionAccepted (const std::shared_ptr<const CallSession> &session) {
	if (callListener)
		callListener->onIncomingCallToBeAdded();
}

void Conference::onCallSessionSetReleased (const std::shared_ptr<const CallSession> &session) {
	if (callListener)
		callListener->onCallSetReleased();
}

void Conference::onCallSessionSetTerminated (const std::shared_ptr<const CallSession> &session) {
	if (callListener)
		callListener->onCallSetTerminated();
}

void Conference::onCallSessionStateChanged (const std::shared_ptr<const CallSession> &session, LinphoneCallState state, const string &message) {
	if (callListener)
		callListener->onCallStateChanged(state, message);
}

void Conference::onCheckForAcceptation (const std::shared_ptr<const CallSession> &session) {
	if (callListener)
		callListener->onCheckForAcceptation();
}

void Conference::onIncomingCallSessionStarted (const std::shared_ptr<const CallSession> &session) {
	if (callListener)
		callListener->onIncomingCallStarted();
}

void Conference::onEncryptionChanged (const std::shared_ptr<const CallSession> &session, bool activated, const string &authToken) {
	if (callListener)
		callListener->onEncryptionChanged(activated, authToken);
}

void Conference::onStatsUpdated (const LinphoneCallStats *stats) {
	if (callListener)
		callListener->onStatsUpdated(stats);
}

void Conference::onResetCurrentSession (const std::shared_ptr<const CallSession> &session) {
	if (callListener)
		callListener->onResetCurrentCall();
}

void Conference::onSetCurrentSession (const std::shared_ptr<const CallSession> &session) {
	if (callListener)
		callListener->onSetCurrentCall();
}

void Conference::onFirstVideoFrameDecoded (const std::shared_ptr<const CallSession> &session) {
	if (callListener)
		callListener->onFirstVideoFrameDecoded();
}

void Conference::onResetFirstVideoFrameDecoded (const std::shared_ptr<const CallSession> &session) {
	if (callListener)
		callListener->onResetFirstVideoFrameDecoded();
}

// -----------------------------------------------------------------------------

shared_ptr<Participant> Conference::findParticipant (const Address &addr) const {
	Address testedAddr = addr;
	testedAddr.setPort(0);
	for (const auto &participant : participants) {
		Address participantAddr = participant->getAddress();
		participantAddr.setPort(0);
		if (testedAddr.weakEqual(participantAddr))
			return participant;
	}
	return nullptr;
}

shared_ptr<Participant> Conference::findParticipant (const shared_ptr<const CallSession> &session) const {
	for (const auto &participant : participants) {
		if (participant->getPrivate()->getSession() == session)
			return participant;
	}
	return nullptr;
}

bool Conference::isMe (const Address &addr) const {
	Address cleanedMe = me->getAddress();
	cleanedMe.setPort(0);
	Address cleanedAddr = addr;
	cleanedAddr.setPort(0);
	return cleanedAddr == cleanedMe;
}

LINPHONE_END_NAMESPACE
