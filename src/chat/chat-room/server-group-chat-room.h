/*
 * server-group-chat-room.h
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

#ifndef _SERVER_GROUP_CHAT_ROOM_H_
#define _SERVER_GROUP_CHAT_ROOM_H_

// From coreapi
#include "private.h"

#include "chat/chat-room/chat-room.h"
#include "conference/local-conference.h"

#include "linphone/types.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ServerGroupChatRoomPrivate;

class ServerGroupChatRoom : public ChatRoom, public LocalConference {
public:
	// TODO: Make me private!
	ServerGroupChatRoom (const std::shared_ptr<Core> &core, SalCallOp *op);

	CapabilitiesMask getCapabilities () const override;
	bool isReadOnly () const override;

	const IdentityAddress &getConferenceAddress () const override;

	bool canHandleParticipants () const override;

	void addParticipant (const IdentityAddress &addr, const CallSessionParams *params, bool hasMedia) override;
	void addParticipants (const std::list<IdentityAddress> &addresses, const CallSessionParams *params, bool hasMedia) override;

	void removeParticipant (const std::shared_ptr<const Participant> &participant) override;
	void removeParticipants (const std::list<std::shared_ptr<Participant>> &participants) override;

	std::shared_ptr<Participant> findParticipant (const IdentityAddress &addr) const override;

	std::shared_ptr<Participant> getMe () const override;
	int getNbParticipants () const override;
	const std::list<std::shared_ptr<Participant>> &getParticipants () const override;

	void setParticipantAdminStatus (std::shared_ptr<Participant> &participant, bool isAdmin) override;

	const std::string &getSubject () const override;
	void setSubject (const std::string &subject) override;

	void join () override;
	void leave () override;

private:
	// TODO: Move me in ServerGroupChatRoomPrivate.
	void onCallSessionStateChanged (const std::shared_ptr<const CallSession> &session, LinphoneCallState state, const std::string &message) override;

	L_DECLARE_PRIVATE(ServerGroupChatRoom);
	L_DISABLE_COPY(ServerGroupChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _SERVER_GROUP_CHAT_ROOM_H_
