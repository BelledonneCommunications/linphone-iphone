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

void ConferencePrivate::ackBeingSent (const CallSession &session, LinphoneHeaders *headers) {
	if (callListener)
		callListener->ackBeingSent(headers);
}

void ConferencePrivate::ackReceived (const CallSession &session, LinphoneHeaders *headers) {
	if (callListener)
		callListener->ackReceived(headers);
}

void ConferencePrivate::callSessionAccepted (const CallSession &session) {
	if (callListener)
		callListener->incomingCallToBeAdded();
}

void ConferencePrivate::callSessionSetReleased (const CallSession &session) {
	if (callListener)
		callListener->callSetReleased();
}

void ConferencePrivate::callSessionSetTerminated (const CallSession &session) {
	if (callListener)
		callListener->callSetTerminated();
}

void ConferencePrivate::callSessionStateChanged (const CallSession &session, LinphoneCallState state, const std::string &message) {
	if (callListener)
		callListener->callStateChanged(state, message);
}

void ConferencePrivate::incomingCallSessionStarted (const CallSession &session) {
	if (callListener)
		callListener->incomingCallStarted();
}

void ConferencePrivate::encryptionChanged (const CallSession &session, bool activated, const std::string &authToken) {
	if (callListener)
		callListener->encryptionChanged(activated, authToken);
}

void ConferencePrivate::statsUpdated (const LinphoneCallStats *stats) {
	if (callListener)
		callListener->statsUpdated(stats);
}

void ConferencePrivate::resetCurrentSession (const CallSession &session) {
	if (callListener)
		callListener->resetCurrentCall();
}

void ConferencePrivate::setCurrentSession (const CallSession &session) {
	if (callListener)
		callListener->setCurrentCall();
}

void ConferencePrivate::firstVideoFrameDecoded (const CallSession &session) {
	if (callListener)
		callListener->firstVideoFrameDecoded();
}

void ConferencePrivate::resetFirstVideoFrameDecoded (const CallSession &session) {
	if (callListener)
		callListener->resetFirstVideoFrameDecoded();
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

shared_ptr<Participant> Conference::getActiveParticipant () const {
	L_D(const Conference);
	return d->activeParticipant;
}

shared_ptr<Participant> Conference::getMe () const {
	L_D(const Conference);
	return d->me;
}

LINPHONE_END_NAMESPACE
