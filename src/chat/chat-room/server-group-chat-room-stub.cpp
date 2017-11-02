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

shared_ptr<Participant> ServerGroupChatRoomPrivate::addParticipant (const Address &) {
	return nullptr;
}

void ServerGroupChatRoomPrivate::confirmCreation () {}

void ServerGroupChatRoomPrivate::confirmJoining (SalCallOp *) {}

shared_ptr<Participant> ServerGroupChatRoomPrivate::findRemovedParticipant (
	const shared_ptr<const CallSession> &
) const {
	return nullptr;
}

string ServerGroupChatRoomPrivate::generateConferenceId () const {
	return "";
}

void ServerGroupChatRoomPrivate::removeParticipant (const shared_ptr<const Participant> &) {}

void ServerGroupChatRoomPrivate::subscribeReceived (LinphoneEvent *) {}

void ServerGroupChatRoomPrivate::update (SalCallOp *) {}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::dispatchMessage (const Address &, const Content &) {}

void ServerGroupChatRoomPrivate::storeOrUpdateMessage (const shared_ptr<ChatMessage> &) {}

LinphoneReason ServerGroupChatRoomPrivate::messageReceived (SalOp *, const SalMessage *) {
	return LinphoneReasonNone;
}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::designateAdmin () {}

bool ServerGroupChatRoomPrivate::isAdminLeft () const {
	return false;
}

// =============================================================================

ServerGroupChatRoom::ServerGroupChatRoom (const shared_ptr<Core> &core, SalCallOp *op) :
  ChatRoom(*new ServerGroupChatRoomPrivate, core, Address(op->get_to())),
	LocalConference(core->getCCore(), Address(op->get_to()), nullptr) {}

int ServerGroupChatRoom::getCapabilities () const {
	return 0;
}

// -----------------------------------------------------------------------------

void ServerGroupChatRoom::onChatMessageReceived(const shared_ptr<ChatMessage> &msg) {}

void ServerGroupChatRoom::addParticipant (const Address &, const CallSessionParams *, bool) {}

void ServerGroupChatRoom::addParticipants (const list<Address> &, const CallSessionParams *, bool) {}

bool ServerGroupChatRoom::canHandleParticipants () const {
	return false;
}

shared_ptr<Participant> ServerGroupChatRoom::findParticipant (const Address &) const {
	return nullptr;
}

const Address &ServerGroupChatRoom::getConferenceAddress () const {
	return LocalConference::getConferenceAddress();
}

int ServerGroupChatRoom::getNbParticipants () const {
	return 0;
}

list<shared_ptr<Participant>> ServerGroupChatRoom::getParticipants () const {
	return LocalConference::getParticipants();
}

const string &ServerGroupChatRoom::getSubject () const {
	return LocalConference::getSubject();
}

void ServerGroupChatRoom::join () {}

void ServerGroupChatRoom::leave () {}

void ServerGroupChatRoom::removeParticipant (const shared_ptr<const Participant> &) {}

void ServerGroupChatRoom::removeParticipants (const list<shared_ptr<Participant>> &) {}

void ServerGroupChatRoom::setParticipantAdminStatus (shared_ptr<Participant> &, bool) {}

void ServerGroupChatRoom::setSubject (const string &) {}

// -----------------------------------------------------------------------------

void ServerGroupChatRoom::onCallSessionStateChanged (
	const shared_ptr<const CallSession> &,
	LinphoneCallState,
	const string &
) {}

LINPHONE_END_NAMESPACE
