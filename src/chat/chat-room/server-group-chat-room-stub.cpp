/*
 * server-group-chat-room.cpp
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

#include <algorithm>

#include "address/address-p.h"
#include "address/address.h"
#include "c-wrapper/c-wrapper.h"
#include "chat/chat-message/chat-message-p.h"
#include "chat/modifier/cpim-chat-message-modifier.h"
#include "conference/local-conference-event-handler.h"
#include "conference/local-conference-p.h"
#include "conference/participant-p.h"
#include "conference/session/call-session-p.h"
#include "content/content-type.h"
#include "logger/logger.h"
#include "sal/refer-op.h"
#include "server-group-chat-room-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ServerGroupChatRoomPrivate::ServerGroupChatRoomPrivate (LinphoneCore *core) : ChatRoomPrivate(core) {}

// -----------------------------------------------------------------------------

shared_ptr<Participant> ServerGroupChatRoomPrivate::addParticipant (const Address &addr) {
	return nullptr;
}

void ServerGroupChatRoomPrivate::confirmCreation () {
}

void ServerGroupChatRoomPrivate::confirmJoining (SalCallOp *op) {
}

shared_ptr<Participant> ServerGroupChatRoomPrivate::findRemovedParticipant (const shared_ptr<const CallSession> &session) const {
	return nullptr;
}

string ServerGroupChatRoomPrivate::generateConferenceId () const {
	return "";
}

void ServerGroupChatRoomPrivate::removeParticipant (const shared_ptr<const Participant> &participant) {
}

void ServerGroupChatRoomPrivate::subscribeReceived (LinphoneEvent *event) {
}

void ServerGroupChatRoomPrivate::update (SalCallOp *op) {
}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::dispatchMessage (const Address &fromAddr, const Content &content) {
}

void ServerGroupChatRoomPrivate::storeOrUpdateMessage (const std::shared_ptr<ChatMessage> &msg) {
}

LinphoneReason ServerGroupChatRoomPrivate::messageReceived (SalOp *op, const SalMessage *salMsg) {
	return LinphoneReasonNone;
}

// -----------------------------------------------------------------------------

void ServerGroupChatRoomPrivate::designateAdmin () {
}

bool ServerGroupChatRoomPrivate::isAdminLeft () const {
	return false;
}

// =============================================================================

ServerGroupChatRoom::ServerGroupChatRoom (LinphoneCore *core, SalCallOp *op)
	: ChatRoom(*new ServerGroupChatRoomPrivate(core)), LocalConference(core, Address(op->get_to()), nullptr) {
}

int ServerGroupChatRoom::getCapabilities () const {
	return 0;
}

// -----------------------------------------------------------------------------

void ServerGroupChatRoom::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
}

void ServerGroupChatRoom::addParticipants (const list<Address> &addresses, const CallSessionParams *params, bool hasMedia) {
}

bool ServerGroupChatRoom::canHandleParticipants () const {
	return FALSE;
}

shared_ptr<Participant> ServerGroupChatRoom::findParticipant (const Address &addr) const {
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

void ServerGroupChatRoom::removeParticipant (const shared_ptr<const Participant> &participant) {
}

void ServerGroupChatRoom::removeParticipants (const list<shared_ptr<Participant>> &participants) {
}

void ServerGroupChatRoom::setParticipantAdminStatus (shared_ptr<Participant> &participant, bool isAdmin) {
}

void ServerGroupChatRoom::setSubject (const std::string &subject) {
}

// -----------------------------------------------------------------------------

void ServerGroupChatRoom::onCallSessionStateChanged (const std::shared_ptr<const CallSession> &session, LinphoneCallState state, const std::string &message) {
}

LINPHONE_END_NAMESPACE
