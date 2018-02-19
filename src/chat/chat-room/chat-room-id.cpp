/*
 * chat-room-id.cpp
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

#include "object/clonable-object-p.h"

#include "chat-room-id.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class ChatRoomIdPrivate : public ClonableObjectPrivate {
public:
	IdentityAddress peerAddress;
	IdentityAddress localAddress;
};

// -----------------------------------------------------------------------------

ChatRoomId::ChatRoomId () : ClonableObject(*new ChatRoomIdPrivate) {}

ChatRoomId::ChatRoomId (
	const IdentityAddress &peerAddress,
	const IdentityAddress &localAddress
) : ClonableObject(*new ChatRoomIdPrivate) {
	L_D();
	d->peerAddress = peerAddress;
	d->localAddress = localAddress;
}

L_USE_DEFAULT_CLONABLE_OBJECT_SHARED_IMPL(ChatRoomId);

bool ChatRoomId::operator== (const ChatRoomId &other) const {
	L_D();
	const ChatRoomIdPrivate *dChatRoomId = other.getPrivate();
	return d->peerAddress == dChatRoomId->peerAddress && d->localAddress == dChatRoomId->localAddress;
}

bool ChatRoomId::operator!= (const ChatRoomId &other) const {
	return !(*this == other);
}

bool ChatRoomId::operator< (const ChatRoomId &other) const {
	L_D();
	const ChatRoomIdPrivate *dChatRoomId = other.getPrivate();
	return d->peerAddress < dChatRoomId->peerAddress
		|| (d->peerAddress == dChatRoomId->peerAddress && d->localAddress < dChatRoomId->localAddress);
}

const IdentityAddress &ChatRoomId::getPeerAddress () const {
	L_D();
	return d->peerAddress;
}

const IdentityAddress &ChatRoomId::getLocalAddress () const {
	L_D();
	return d->localAddress;
}

bool ChatRoomId::isValid () const {
	L_D();
	return d->peerAddress.isValid() && d->localAddress.isValid();
}

LINPHONE_END_NAMESPACE
