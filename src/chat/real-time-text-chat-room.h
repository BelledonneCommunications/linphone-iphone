/*
 * real-time-text-chat-room.h
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

#ifndef _REAL_TIME_TEXT_CHAT_ROOM_H_
#define _REAL_TIME_TEXT_CHAT_ROOM_H_

// From coreapi
#include "private.h"

#include "chat/chat-room.h"

#include "linphone/types.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class RealTimeTextChatRoomPrivate;

class LINPHONE_PUBLIC RealTimeTextChatRoom : public ChatRoom {
public:
	RealTimeTextChatRoom (LinphoneCore *core, const Address &peerAddress);
	virtual ~RealTimeTextChatRoom () = default;

	void sendMessage (LinphoneChatMessage *msg) override;

	uint32_t getChar () const;
	LinphoneCall *getCall () const;

	/* ConferenceInterface */
	std::shared_ptr<Participant> addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) override;
	void addParticipants (const std::list<Address> &addresses, const CallSessionParams *params, bool hasMedia) override;
	bool canHandleParticipants () const override;
	const Address *getConferenceAddress () const override;
	int getNbParticipants () const override;
	std::list<std::shared_ptr<Participant>> getParticipants () const override;
	void removeParticipant (const std::shared_ptr<const Participant> &participant) override;
	void removeParticipants (const std::list<std::shared_ptr<Participant>> &participants) override;

private:
	L_DECLARE_PRIVATE(RealTimeTextChatRoom);
	L_DISABLE_COPY(RealTimeTextChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _REAL_TIME_TEXT_CHAT_ROOM_H_
