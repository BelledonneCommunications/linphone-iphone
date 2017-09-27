/*
 * conference.cpp
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

#include "participant-p.h"

#include "conference.h"
#include "logger/logger.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

Conference::Conference (LinphoneCore *core, const Address &myAddress, CallListener *listener)
	: core(core), callListener(listener) {
	me = make_shared<Participant>(myAddress);
}

// -----------------------------------------------------------------------------

shared_ptr<Participant> Conference::getActiveParticipant () const {
	return activeParticipant;
}

// -----------------------------------------------------------------------------

shared_ptr<Participant> Conference::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	lError() << "Conference class does not handle addParticipant() generically";
	return nullptr;
}

void Conference::addParticipants (const list<Address> &addresses, const CallSessionParams *params, bool hasMedia) {
	for (const auto &addr : addresses)
		addParticipant(addr, params, hasMedia);
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

void Conference::removeParticipant (const shared_ptr<const Participant> &participant) {
	lError() << "Conference class does not handle removeParticipant() generically";
}

void Conference::removeParticipants (const list<shared_ptr<Participant>> &participants) {
	for (const auto &p : participants)
		removeParticipant(p);
}

// -----------------------------------------------------------------------------

void Conference::onAckBeingSent (const CallSession &session, LinphoneHeaders *headers) {
	if (callListener)
		callListener->onAckBeingSent(headers);
}

void Conference::onAckReceived (const CallSession &session, LinphoneHeaders *headers) {
	if (callListener)
		callListener->onAckReceived(headers);
}

void Conference::onCallSessionAccepted (const CallSession &session) {
	if (callListener)
		callListener->onIncomingCallToBeAdded();
}

void Conference::onCallSessionSetReleased (const CallSession &session) {
	if (callListener)
		callListener->onCallSetReleased();
}

void Conference::onCallSessionSetTerminated (const CallSession &session) {
	if (callListener)
		callListener->onCallSetTerminated();
}

void Conference::onCallSessionStateChanged (const CallSession &session, LinphoneCallState state, const string &message) {
	if (callListener)
		callListener->onCallStateChanged(state, message);
}

void Conference::onCheckForAcceptation (const CallSession &session) {
	if (callListener)
		callListener->onCheckForAcceptation();
}

void Conference::onIncomingCallSessionStarted (const CallSession &session) {
	if (callListener)
		callListener->onIncomingCallStarted();
}

void Conference::onEncryptionChanged (const CallSession &session, bool activated, const string &authToken) {
	if (callListener)
		callListener->onEncryptionChanged(activated, authToken);
}

void Conference::onStatsUpdated (const LinphoneCallStats *stats) {
	if (callListener)
		callListener->onStatsUpdated(stats);
}

void Conference::onResetCurrentSession (const CallSession &session) {
	if (callListener)
		callListener->onResetCurrentCall();
}

void Conference::onSetCurrentSession (const CallSession &session) {
	if (callListener)
		callListener->onSetCurrentCall();
}

void Conference::onFirstVideoFrameDecoded (const CallSession &session) {
	if (callListener)
		callListener->onFirstVideoFrameDecoded();
}

void Conference::onResetFirstVideoFrameDecoded (const CallSession &session) {
	if (callListener)
		callListener->onResetFirstVideoFrameDecoded();
}

// -----------------------------------------------------------------------------

shared_ptr<Participant> Conference::findParticipant (const Address &addr) {
	for (const auto &participant : participants) {
		if (addr.equal(participant->getAddress()))
			return participant;
	}
	return nullptr;
}

LINPHONE_END_NAMESPACE
