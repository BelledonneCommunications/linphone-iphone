/*
 * basic-chat-room.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
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

#include "linphone/utils/utils.h"

#include "basic-chat-room-p.h"
#include "conference/participant.h"
#include "logger/logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

BasicChatRoom::BasicChatRoom (const shared_ptr<Core> &core, const ChatRoomId &chatRoomId) :
	BasicChatRoom(*new BasicChatRoomPrivate, core, chatRoomId) {}

BasicChatRoom::BasicChatRoom (
	BasicChatRoomPrivate &p,
	const std::shared_ptr<Core> &core,
	const ChatRoomId &chatRoomId
) : ChatRoom(p, core, chatRoomId) {
	L_D();
	d->me = make_shared<Participant>(nullptr, getLocalAddress());
	d->participants.push_back(make_shared<Participant>(nullptr, getPeerAddress()));
}

void BasicChatRoom::allowCpim (bool value) {
	L_D();
	d->cpimAllowed = value;
}

void BasicChatRoom::allowMultipart (bool value) {
	L_D();
	d->multipartAllowed = value;
}

bool BasicChatRoom::canHandleCpim () const {
	L_D();
	return d->cpimAllowed;
}

bool BasicChatRoom::canHandleMultipart () const {
	L_D();
	return d->multipartAllowed;
}

BasicChatRoom::CapabilitiesMask BasicChatRoom::getCapabilities () const {
	return { Capabilities::Basic, Capabilities::OneToOne };
}

bool BasicChatRoom::hasBeenLeft () const {
	return false;
}

bool BasicChatRoom::canHandleParticipants () const {
	return false;
}

const IdentityAddress &BasicChatRoom::getConferenceAddress () const {
	lError() << "a BasicChatRoom does not have a conference address";
	return Utils::getEmptyConstRefObject<IdentityAddress>();
}

void BasicChatRoom::addParticipant (const IdentityAddress &, const CallSessionParams *, bool) {
	lError() << "addParticipant() is not allowed on a BasicChatRoom";
}

void BasicChatRoom::addParticipants (const list<IdentityAddress> &, const CallSessionParams *, bool) {
	lError() << "addParticipants() is not allowed on a BasicChatRoom";
}

void BasicChatRoom::removeParticipant (const shared_ptr<Participant> &) {
	lError() << "removeParticipant() is not allowed on a BasicChatRoom";
}

void BasicChatRoom::removeParticipants (const list<shared_ptr<Participant>> &) {
	lError() << "removeParticipants() is not allowed on a BasicChatRoom";
}

shared_ptr<Participant> BasicChatRoom::findParticipant (const IdentityAddress &) const {
	lError() << "findParticipant() is not allowed on a BasicChatRoom";
	return nullptr;
}

shared_ptr<Participant> BasicChatRoom::getMe () const {
	L_D();
	return d->me;
}

int BasicChatRoom::getParticipantCount () const {
	return 1;
}

const list<shared_ptr<Participant>> &BasicChatRoom::getParticipants () const {
	L_D();
	return d->participants;
}

void BasicChatRoom::setParticipantAdminStatus (const shared_ptr<Participant> &, bool) {
	lError() << "setParticipantAdminStatus() is not allowed on a BasicChatRoom";
}

const string &BasicChatRoom::getSubject () const {
	L_D();
	return d->subject;
}

void BasicChatRoom::setSubject (const string &subject) {
	L_D();
	d->subject = subject;
}

void BasicChatRoom::join () {
	lError() << "join() is not allowed on a BasicChatRoom";
}

void BasicChatRoom::leave () {
	lError() << "leave() is not allowed on a BasicChatRoom";
}

LINPHONE_END_NAMESPACE
