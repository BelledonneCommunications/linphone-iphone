/*
 * client-group-chat-room.h
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

#ifndef _CLIENT_GROUP_CHAT_ROOM_H_
#define _CLIENT_GROUP_CHAT_ROOM_H_

// From coreapi
#include "private.h"

#include "chat/chat-room.h"
#include "conference/remote-conference.h"
#include "conference/session/call-session.h"

#include "linphone/types.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ClientGroupChatRoomPrivate;

class LINPHONE_PUBLIC ClientGroupChatRoom : public ChatRoom, public RemoteConference {
public:
	ClientGroupChatRoom (LinphoneCore *core, const Address &me, const std::string &subject);
	virtual ~ClientGroupChatRoom () = default;

public:
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
	/* ConferenceListener */
	void onConferenceCreated (const Address &addr) override;
	void onConferenceTerminated (const Address &addr) override;
	void onParticipantAdded (const Address &addr) override;
	void onParticipantRemoved (const Address &addr) override;
	void onParticipantSetAdmin (const Address &addr, bool isAdmin) override;

private:
	/* CallSessionListener */
	void onCallSessionStateChanged (const CallSession &session, LinphoneCallState state, const std::string &message) override;

private:
	L_DECLARE_PRIVATE(ClientGroupChatRoom);
	L_DISABLE_COPY(ClientGroupChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CLIENT_GROUP_CHAT_ROOM_H_
