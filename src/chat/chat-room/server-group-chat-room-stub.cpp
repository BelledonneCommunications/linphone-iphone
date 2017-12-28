/*
 * server-group-chat-room-stub.cpp
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

#include "core/core.h"
#include "server-group-chat-room-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

shared_ptr<Participant> ServerGroupChatRoomPrivate::addParticipant (const IdentityAddress &) {
	return nullptr;
}

void ServerGroupChatRoomPrivate::removeParticipant (const shared_ptr<const Participant> &) {}

shared_ptr<Participant> ServerGroupChatRoomPrivate::findRemovedParticipant (
	const shared_ptr<const CallSession> &
) const {
	return nullptr;
}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::confirmCreation () {}

void ServerGroupChatRoomPrivate::confirmJoining (SalCallOp *) {}

// -----------------------------------------------------------------------------

IdentityAddress ServerGroupChatRoomPrivate::generateConferenceAddress (const shared_ptr<Participant> &me) const {
	return IdentityAddress();
}

void ServerGroupChatRoomPrivate::subscribeReceived (LinphoneEvent *) {}

void ServerGroupChatRoomPrivate::update (SalCallOp *) {}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::dispatchMessage (const IdentityAddress &, const Content &) {}

void ServerGroupChatRoomPrivate::setConferenceAddress (const IdentityAddress &) {}

void ServerGroupChatRoomPrivate::setParticipantDevices (const IdentityAddress &addr, const list<IdentityAddress> &devices) {}

void ServerGroupChatRoomPrivate::addCompatibleParticipants (const IdentityAddress &deviceAddr, const list<IdentityAddress> &participantCompatible) {}

// -----------------------------------------------------------------------------

LinphoneReason ServerGroupChatRoomPrivate::onSipMessageReceived (SalOp *, const SalMessage *) {
	return LinphoneReasonNone;
}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::designateAdmin () {}

void ServerGroupChatRoomPrivate::finalizeCreation () {}

bool ServerGroupChatRoomPrivate::isAdminLeft () const {
	return false;
}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::onChatRoomInsertRequested (const shared_ptr<AbstractChatRoom> &chatRoom) {}

void ServerGroupChatRoomPrivate::onChatRoomInsertInDatabaseRequested (const shared_ptr<AbstractChatRoom> &chatRoom) {}

void ServerGroupChatRoomPrivate::onChatRoomDeleteRequested (const shared_ptr<AbstractChatRoom> &chatRoom) {}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::onCallSessionStateChanged (
	const shared_ptr<const CallSession> &,
	CallSession::State,
	const string &
) {}

// =============================================================================

ServerGroupChatRoom::ServerGroupChatRoom (const shared_ptr<Core> &core, SalCallOp *op)
: ChatRoom(*new ServerGroupChatRoomPrivate, core, ChatRoomId(IdentityAddress(op->get_to()), IdentityAddress(op->get_to()))),
LocalConference(core, IdentityAddress(op->get_to()), nullptr) {
	L_D();
	d->chatRoomListener = d;
}

ServerGroupChatRoom::ServerGroupChatRoom (
	const shared_ptr<Core> &core,
	const IdentityAddress &peerAddress,
	const string &subject,
	list<shared_ptr<Participant>> &&participants,
	unsigned int lastNotifyId
) : ChatRoom(*new ServerGroupChatRoomPrivate, core, ChatRoomId(peerAddress, peerAddress)),
	LocalConference(core, peerAddress, nullptr) {}

ServerGroupChatRoom::CapabilitiesMask ServerGroupChatRoom::getCapabilities () const {
	return 0;
}

bool ServerGroupChatRoom::hasBeenLeft () const {
	return true;
}

const IdentityAddress &ServerGroupChatRoom::getConferenceAddress () const {
	return LocalConference::getConferenceAddress();
}

bool ServerGroupChatRoom::canHandleParticipants () const {
	return false;
}

void ServerGroupChatRoom::addParticipant (const IdentityAddress &, const CallSessionParams *, bool) {}

void ServerGroupChatRoom::addParticipants (const list<IdentityAddress> &, const CallSessionParams *, bool) {}

void ServerGroupChatRoom::removeParticipant (const shared_ptr<const Participant> &) {}

void ServerGroupChatRoom::removeParticipants (const list<shared_ptr<Participant>> &) {}

shared_ptr<Participant> ServerGroupChatRoom::findParticipant (const IdentityAddress &) const {
	return nullptr;
}

shared_ptr<Participant> ServerGroupChatRoom::getMe () const {
	return nullptr;
}

int ServerGroupChatRoom::getParticipantCount () const {
	return 0;
}

const list<shared_ptr<Participant>> &ServerGroupChatRoom::getParticipants () const {
	return LocalConference::getParticipants();
}

void ServerGroupChatRoom::setParticipantAdminStatus (const shared_ptr<Participant> &, bool) {}

const string &ServerGroupChatRoom::getSubject () const {
	return LocalConference::getSubject();
}

void ServerGroupChatRoom::setSubject (const string &) {}

void ServerGroupChatRoom::join () {}

void ServerGroupChatRoom::leave () {}

LINPHONE_END_NAMESPACE
