/*
 * server-group-chat-room-stub.cpp
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

#include "core/core.h"
#include "server-group-chat-room-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::setState (ChatRoom::State state) {
	ChatRoomPrivate::setState(state);
}

shared_ptr<Participant> ServerGroupChatRoomPrivate::addParticipant (const IdentityAddress &) {
	return nullptr;
}

void ServerGroupChatRoomPrivate::removeParticipant (const shared_ptr<const Participant> &) {}

shared_ptr<Participant> ServerGroupChatRoomPrivate::findFilteredParticipant (const shared_ptr<const CallSession> &session) const {
	return nullptr;
}

shared_ptr<Participant> ServerGroupChatRoomPrivate::findFilteredParticipant (const IdentityAddress &participantAddress) const {
	return nullptr;
}

ParticipantDevice::State ServerGroupChatRoomPrivate::getParticipantDeviceState (const shared_ptr<const ParticipantDevice> &device) const {
	return device->getState();
}

void ServerGroupChatRoomPrivate::setParticipantDeviceState (const shared_ptr<ParticipantDevice> &device, ParticipantDevice::State state) {
	device->setState(state);
}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::acceptSession (const shared_ptr<CallSession> &session) {}

void ServerGroupChatRoomPrivate::confirmCreation () {}

void ServerGroupChatRoomPrivate::confirmJoining (SalCallOp *) {}

void ServerGroupChatRoomPrivate::confirmRecreation (SalCallOp *) {}

void ServerGroupChatRoomPrivate::declineSession (const shared_ptr<CallSession> &session, LinphoneReason reason) {}

void ServerGroupChatRoomPrivate::dispatchQueuedMessages () {}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::subscribeReceived (LinphoneEvent *) {}

bool ServerGroupChatRoomPrivate::update (SalCallOp *) { return true; }

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::setConferenceAddress (const IdentityAddress &) {}

void ServerGroupChatRoomPrivate::setParticipantDevices (const IdentityAddress &addr, const list<IdentityAddress> &devices) {}

void ServerGroupChatRoomPrivate::addParticipantDevice (const IdentityAddress &participantAddress, const IdentityAddress &deviceAddress) {}

void ServerGroupChatRoomPrivate::addCompatibleParticipants (const IdentityAddress &deviceAddr, const list<IdentityAddress> &participantCompatible) {}

void ServerGroupChatRoomPrivate::checkCompatibleParticipants (const IdentityAddress &deviceAddr, const list<IdentityAddress> &addressesToCheck) {}

// -----------------------------------------------------------------------------

LinphoneReason ServerGroupChatRoomPrivate::onSipMessageReceived (SalOp *, const SalMessage *) {
	return LinphoneReasonNone;
}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::byeDevice (const shared_ptr<ParticipantDevice> &device) {}

void ServerGroupChatRoomPrivate::designateAdmin () {}

void ServerGroupChatRoomPrivate::dispatchMessage (const shared_ptr<Message> &message, const string &uri) {}

void ServerGroupChatRoomPrivate::finalizeCreation () {}

void ServerGroupChatRoomPrivate::inviteDevice (const shared_ptr<ParticipantDevice> &device) {}

bool ServerGroupChatRoomPrivate::isAdminLeft () const {
	return false;
}

void ServerGroupChatRoomPrivate::queueMessage (const shared_ptr<Message> &message) {}

void ServerGroupChatRoomPrivate::queueMessage (const shared_ptr<Message> &msg, const IdentityAddress &deviceAddress) {}

void ServerGroupChatRoomPrivate::removeNonPresentParticipants (const list <IdentityAddress> &compatibleParticipants) {}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::onParticipantDeviceLeft (const shared_ptr<ParticipantDevice> &device) {}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::onChatRoomInsertRequested (const shared_ptr<AbstractChatRoom> &chatRoom) {}

void ServerGroupChatRoomPrivate::onChatRoomInsertInDatabaseRequested (const shared_ptr<AbstractChatRoom> &chatRoom) {}

void ServerGroupChatRoomPrivate::onChatRoomDeleteRequested (const shared_ptr<AbstractChatRoom> &chatRoom) {}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::onCallSessionStateChanged (
	const shared_ptr<CallSession> &,
	CallSession::State,
	const string &
) {}

void ServerGroupChatRoomPrivate::onCallSessionSetReleased (const shared_ptr<CallSession> &session) {}

// =============================================================================

ServerGroupChatRoom::ServerGroupChatRoom (const shared_ptr<Core> &core, SalCallOp *op)
: ChatRoom(*new ServerGroupChatRoomPrivate, core, ChatRoomId(IdentityAddress(op->getTo()), IdentityAddress(op->getTo()))),
LocalConference(core, IdentityAddress(op->getTo()), nullptr) {
	L_D();
	d->chatRoomListener = d;
}

ServerGroupChatRoom::ServerGroupChatRoom (
	const shared_ptr<Core> &core,
	const IdentityAddress &peerAddress,
	AbstractChatRoom::CapabilitiesMask capabilities,
	const string &subject,
	list<shared_ptr<Participant>> &&participants,
	unsigned int lastNotifyId
) : ChatRoom(*new ServerGroupChatRoomPrivate, core, ChatRoomId(peerAddress, peerAddress)),
	LocalConference(core, peerAddress, nullptr) {}

ServerGroupChatRoom::~ServerGroupChatRoom () {};

ServerGroupChatRoom::CapabilitiesMask ServerGroupChatRoom::getCapabilities () const {
	return 0;
}

void ServerGroupChatRoom::allowCpim (bool value) {

}

void ServerGroupChatRoom::allowMultipart (bool value) {

}

bool ServerGroupChatRoom::canHandleCpim () const {
	return true;
}

bool ServerGroupChatRoom::canHandleMultipart () const {
	return true;
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

void ServerGroupChatRoom::removeParticipant (const shared_ptr<Participant> &participant) {}

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

void ServerGroupChatRoom::onFirstNotifyReceived (const IdentityAddress &addr) {}

// -----------------------------------------------------------------------------

ostream &operator<< (ostream &stream, const ServerGroupChatRoom *chatRoom) {
	return stream << "ServerGroupChatRoom [" << reinterpret_cast<const void *>(chatRoom) << "]";
}

LINPHONE_END_NAMESPACE
