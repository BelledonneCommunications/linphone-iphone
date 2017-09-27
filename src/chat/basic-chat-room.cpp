/*
 * basic-chat-room.cpp
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

#include "basic-chat-room-p.h"
#include "logger/logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

BasicChatRoomPrivate::BasicChatRoomPrivate (LinphoneCore *core, const Address &peerAddress) : ChatRoomPrivate(core) {
	this->peerAddress = peerAddress;
}

// =============================================================================

BasicChatRoom::BasicChatRoom (LinphoneCore *core, const Address &peerAddress) : ChatRoom(*new BasicChatRoomPrivate(core, peerAddress)) {}

// -----------------------------------------------------------------------------

shared_ptr<Participant> BasicChatRoom::addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) {
	lError() << "addParticipant() is not allowed on a BasicChatRoom";
	return nullptr;
}

void BasicChatRoom::addParticipants (const list<Address> &addresses, const CallSessionParams *params, bool hasMedia) {
	lError() << "addParticipants() is not allowed on a BasicChatRoom";
}

bool BasicChatRoom::canHandleParticipants () const {
	return false;
}

const Address *BasicChatRoom::getConferenceAddress () const {
	lError() << "a BasicChatRoom does not have a conference address";
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

void BasicChatRoom::removeParticipant (const shared_ptr<const Participant> &participant) {
	lError() << "removeParticipant() is not allowed on a BasicChatRoom";
}

void BasicChatRoom::removeParticipants (const list<shared_ptr<Participant>> &participants) {
	lError() << "removeParticipants() is not allowed on a BasicChatRoom";
}

LINPHONE_END_NAMESPACE
