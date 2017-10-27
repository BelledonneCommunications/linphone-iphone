/*
 * server-group-chat-room-p.h
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

#ifndef _SERVER_GROUP_CHAT_ROOM_P_H_
#define _SERVER_GROUP_CHAT_ROOM_P_H_

// From coreapi.
#include "private.h"

#include "address/address.h"
#include "chat-room-p.h"
#include "server-group-chat-room.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Participant;

class ServerGroupChatRoomPrivate : public ChatRoomPrivate {
public:
	ServerGroupChatRoomPrivate (LinphoneCore *core);
	virtual ~ServerGroupChatRoomPrivate () = default;

	std::shared_ptr<Participant> addParticipant (const Address &addr);
	void confirmCreation ();
	void confirmJoining (SalCallOp *op);
	std::shared_ptr<Participant> findRemovedParticipant (const std::shared_ptr<const CallSession> &session) const;
	std::string generateConferenceId () const;
	void removeParticipant (const std::shared_ptr<const Participant> &participant);
	void subscribeReceived (LinphoneEvent *event);
	void update (SalCallOp *op);

	void dispatchMessage (const Address &fromAddr, const Content &content);
	void storeOrUpdateMessage (const std::shared_ptr<ChatMessage> &msg) override;
	LinphoneReason messageReceived (SalOp *op, const SalMessage *msg) override;

private:
	void designateAdmin ();
	bool isAdminLeft () const;

private:
	L_DECLARE_PUBLIC(ServerGroupChatRoom);

	std::list<std::shared_ptr<Participant>> removedParticipants;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _SERVER_GROUP_CHAT_ROOM_P_H_
