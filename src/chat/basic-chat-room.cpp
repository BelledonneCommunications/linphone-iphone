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

shared_ptr<Participant> BasicChatRoom::addParticipant (const Address &addr, const shared_ptr<CallSessionParams> params, bool hasMedia) {
	lError() << "addParticipant() is not allowed on a BasicChatRoom";
	return nullptr;
}

void BasicChatRoom::addParticipants (const list<Address> &addresses, const shared_ptr<CallSessionParams> params, bool hasMedia) {
	lError() << "addParticipants() is not allowed on a BasicChatRoom";
}

const string& BasicChatRoom::getId () const {
	L_D(const BasicChatRoom);
	lError() << "a BasicChatRoom does not have a conference id";
	return d->dummyConferenceId;
}

int BasicChatRoom::getNbParticipants () const {
	return 1;
}

list<shared_ptr<Participant>> BasicChatRoom::getParticipants () const {
	L_D(const BasicChatRoom);
	list<shared_ptr<Participant>> l;
	l.push_back(make_shared<Participant>(d->peerAddress));
	return l;
}

void BasicChatRoom::removeParticipant (const shared_ptr<Participant> participant) {
	lError() << "removeParticipant() is not allowed on a BasicChatRoom";
}

void BasicChatRoom::removeParticipants (const list<shared_ptr<Participant>> participants) {
	lError() << "removeParticipants() is not allowed on a BasicChatRoom";
}

LINPHONE_END_NAMESPACE
