/*
 * basic-chat-room.cpp
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

#include "linphone/utils/utils.h"

#include "basic-chat-room-p.h"
#include "conference/participant.h"
#include "logger/logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

BasicChatRoom::BasicChatRoom (const shared_ptr<Core> &core, const Address &peerAddress) :
	ChatRoom(*new BasicChatRoomPrivate, core, peerAddress) {}

// -----------------------------------------------------------------------------

void BasicChatRoom::onChatMessageReceived (const shared_ptr<ChatMessage> &msg) {

}

int BasicChatRoom::getCapabilities () const {
	return static_cast<int>(Capabilities::Basic);
}

void BasicChatRoom::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	lError() << "addParticipant() is not allowed on a BasicChatRoom";
}

void BasicChatRoom::addParticipants (const list<Address> &addresses, const CallSessionParams *params, bool hasMedia) {
	lError() << "addParticipants() is not allowed on a BasicChatRoom";
}

bool BasicChatRoom::canHandleParticipants () const {
	return false;
}

shared_ptr<Participant> BasicChatRoom::findParticipant (const Address &addr) const {
	lError() << "findParticipant() is not allowed on a BasicChatRoom";
	return nullptr;
}

const Address &BasicChatRoom::getConferenceAddress () const {
	lError() << "a BasicChatRoom does not have a conference address";
	return Utils::getEmptyConstRefObject<Address>();
}

shared_ptr<Participant> BasicChatRoom::getMe () const {
	lError() << "a BasicChatRoom does not handle participants";
	return nullptr;
}

int BasicChatRoom::getNbParticipants () const {
	return 1;
}

list<shared_ptr<Participant>> BasicChatRoom::getParticipants () const {
	L_D();
	list<shared_ptr<Participant>> l;
	l.push_back(make_shared<Participant>(d->peerAddress));
	return l;
}

const string &BasicChatRoom::getSubject () const {
	L_D();
	return d->subject;
}

void BasicChatRoom::join () {
	lError() << "join() is not allowed on a BasicChatRoom";
}

void BasicChatRoom::leave () {
	lError() << "leave() is not allowed on a BasicChatRoom";
}

void BasicChatRoom::removeParticipant (const shared_ptr<const Participant> &participant) {
	lError() << "removeParticipant() is not allowed on a BasicChatRoom";
}

void BasicChatRoom::removeParticipants (const list<shared_ptr<Participant>> &participants) {
	lError() << "removeParticipants() is not allowed on a BasicChatRoom";
}

void BasicChatRoom::setParticipantAdminStatus (shared_ptr<Participant> &participant, bool isAdmin) {
	lError() << "setParticipantAdminStatus() is not allowed on a BasicChatRoom";
}

void BasicChatRoom::setSubject (const string &subject) {
	L_D();
	d->subject = subject;
}

LINPHONE_END_NAMESPACE
