/*
 * basic-chat-room.h
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

#ifndef _BASIC_CHAT_ROOM_H_
#define _BASIC_CHAT_ROOM_H_

#include "chat/chat-room.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class BasicChatRoomPrivate;

class LINPHONE_PUBLIC BasicChatRoom : public ChatRoom {
public:
	BasicChatRoom (LinphoneCore *core, const Address &peerAddress);
	virtual ~BasicChatRoom () = default;

	/* ConferenceInterface. */
	std::shared_ptr<Participant> addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) override;
	void addParticipants (const std::list<Address> &addresses, const CallSessionParams *params, bool hasMedia) override;
	bool canHandleParticipants () const override;
	const Address *getConferenceAddress () const override;
	int getNbParticipants () const override;
	std::list<std::shared_ptr<Participant>> getParticipants () const override;
	void removeParticipant (const std::shared_ptr<const Participant> &participant) override;
	void removeParticipants (const std::list<std::shared_ptr<Participant>> &participants) override;

private:
	L_DECLARE_PRIVATE(BasicChatRoom);
	L_DISABLE_COPY(BasicChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _BASIC_CHAT_ROOM_H_
