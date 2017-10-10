/*
 * real-time-text-chat-room.h
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

	void sendMessage (std::shared_ptr<ChatMessage> msg) override;

	uint32_t getChar () const;
	LinphoneCall *getCall () const;

	/* ConferenceInterface */
	void addParticipant (const Address &addr, const CallSessionParams *params, bool hasMedia) override;
	void addParticipants (const std::list<Address> &addresses, const CallSessionParams *params, bool hasMedia) override;
	bool canHandleParticipants () const override;
	std::shared_ptr<Participant> findParticipant (const Address &addr) const override;
	const Address *getConferenceAddress () const override;
	int getNbParticipants () const override;
	std::list<std::shared_ptr<Participant>> getParticipants () const override;
	const std::string &getSubject () const override;
	void join () override;
	void leave () override;
	void removeParticipant (const std::shared_ptr<const Participant> &participant) override;
	void removeParticipants (const std::list<std::shared_ptr<Participant>> &participants) override;
	void setParticipantAdminStatus (std::shared_ptr<Participant> &participant, bool isAdmin) override;
	void setSubject (const std::string &subject) override;

private:
	L_DECLARE_PRIVATE(RealTimeTextChatRoom);
	L_DISABLE_COPY(RealTimeTextChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _REAL_TIME_TEXT_CHAT_ROOM_H_
