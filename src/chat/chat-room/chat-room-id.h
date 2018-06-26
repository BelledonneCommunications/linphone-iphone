/*
 * chat-room-id.h
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

#ifndef _L_CHAT_ROOM_ID_H_
#define _L_CHAT_ROOM_ID_H_

#include "address/identity-address.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatRoomIdPrivate;

class LINPHONE_PUBLIC ChatRoomId : public ClonableObject {
public:
	ChatRoomId ();
	ChatRoomId (const IdentityAddress &peerAddress, const IdentityAddress &localAddress);
	ChatRoomId (const ChatRoomId &other);

	ChatRoomId &operator= (const ChatRoomId &other);

	bool operator== (const ChatRoomId &other) const;
	bool operator!= (const ChatRoomId &other) const;

	bool operator< (const ChatRoomId &other) const;

	const IdentityAddress &getPeerAddress () const;
	const IdentityAddress &getLocalAddress () const;

	bool isValid () const;

private:
	L_DECLARE_PRIVATE(ChatRoomId);
};

inline std::ostream &operator<< (std::ostream &os, const ChatRoomId &chatRoomId) {
	os << "ChatRoomId(" << chatRoomId.getPeerAddress() << ", local=" << chatRoomId.getLocalAddress() << ")";
	return os;
}

LINPHONE_END_NAMESPACE

// Add map key support.
namespace std {
	template<>
	struct hash<LinphonePrivate::ChatRoomId> {
		std::size_t operator() (const LinphonePrivate::ChatRoomId &chatRoomId) const {
			return hash<string>()(chatRoomId.getPeerAddress().asString()) ^
				(hash<string>()(chatRoomId.getLocalAddress().asString()) << 1);
		}
	};
}

#endif // ifndef _L_CHAT_ROOM_ID_H_
