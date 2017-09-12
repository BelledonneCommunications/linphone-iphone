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

#include "conference-p.h"
#include "participant-p.h"

#include "conference.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

void ConferencePrivate::onAckBeingSent (const CallSession &session, LinphoneHeaders *headers) {
	if (callListener)
		callListener->onAckBeingSent(headers);
}

void ConferencePrivate::onAckReceived (const CallSession &session, LinphoneHeaders *headers) {
	if (callListener)
		callListener->onAckReceived(headers);
}

void ConferencePrivate::onCallSessionAccepted (const CallSession &session) {
	if (callListener)
		callListener->onIncomingCallToBeAdded();
}

void ConferencePrivate::onCallSessionSetReleased (const CallSession &session) {
	if (callListener)
		callListener->onCallSetReleased();
}

void ConferencePrivate::onCallSessionSetTerminated (const CallSession &session) {
	if (callListener)
		callListener->onCallSetTerminated();
}

void ConferencePrivate::onCallSessionStateChanged (const CallSession &session, LinphoneCallState state, const string &message) {
	if (callListener)
		callListener->onCallStateChanged(state, message);
}

void ConferencePrivate::onIncomingCallSessionStarted (const CallSession &session) {
	if (callListener)
		callListener->onIncomingCallStarted();
}

void ConferencePrivate::onEncryptionChanged (const CallSession &session, bool activated, const string &authToken) {
	if (callListener)
		callListener->onEncryptionChanged(activated, authToken);
}

void ConferencePrivate::onStatsUpdated (const LinphoneCallStats *stats) {
	if (callListener)
		callListener->onStatsUpdated(stats);
}

void ConferencePrivate::onResetCurrentSession (const CallSession &session) {
	if (callListener)
		callListener->onResetCurrentCall();
}

void ConferencePrivate::onSetCurrentSession (const CallSession &session) {
	if (callListener)
		callListener->onSetCurrentCall();
}

void ConferencePrivate::onFirstVideoFrameDecoded (const CallSession &session) {
	if (callListener)
		callListener->onFirstVideoFrameDecoded();
}

void ConferencePrivate::onResetFirstVideoFrameDecoded (const CallSession &session) {
	if (callListener)
		callListener->onResetFirstVideoFrameDecoded();
}

// =============================================================================

Conference::Conference (ConferencePrivate &p, LinphoneCore *core, const Address &myAddress, CallListener *listener)
	: Object(p) {
	L_D(Conference);
	d->core = core;
	d->callListener = listener;
	d->me = make_shared<Participant>(myAddress);
}

shared_ptr<Participant> Conference::addParticipant (const Address &addr, const shared_ptr<CallSessionParams> params, bool hasMedia) {
	L_D(Conference);
	d->activeParticipant = make_shared<Participant>(addr);
	d->activeParticipant->getPrivate()->createSession(*this, params, hasMedia, d);
	return d->activeParticipant;
}

void Conference::addParticipants (const list<Address> &addresses, const shared_ptr<CallSessionParams> params, bool hasMedia) {
	// TODO
}

const string& Conference::getId () const {
	L_D(const Conference);
	return d->id;
}

int Conference::getNbParticipants () const {
	// TODO
	return 1;
}

list<shared_ptr<Participant>> Conference::getParticipants () const {
	L_D(const Conference);
	return d->participants;
}

void Conference::removeParticipant (const shared_ptr<Participant> participant) {
	// TODO
}

void Conference::removeParticipants (const list<shared_ptr<Participant>> participants) {
	// TODO
}

shared_ptr<Participant> Conference::getActiveParticipant () const {
	L_D(const Conference);
	return d->activeParticipant;
}

shared_ptr<Participant> Conference::getMe () const {
	L_D(const Conference);
	return d->me;
}

LINPHONE_END_NAMESPACE
