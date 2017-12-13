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

#include "chat-room-p.h"
#include "conference/session/call-session-listener.h"
#include "server-group-chat-room.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ServerGroupChatRoomPrivate : public ChatRoomPrivate, public CallSessionListener {
public:
	std::shared_ptr<Participant> addParticipant (const IdentityAddress &participantAddress);
	void removeParticipant (const std::shared_ptr<const Participant> &participant);
	std::shared_ptr<Participant> findRemovedParticipant (const std::shared_ptr<const CallSession> &session) const;

	void confirmCreation ();
	void confirmJoining (SalCallOp *op);

	IdentityAddress generateConferenceAddress (const std::shared_ptr<Participant> &me) const;

	void subscribeReceived (LinphoneEvent *event);

	void update (SalCallOp *op);

	void dispatchMessage (const IdentityAddress &fromAddress, const Content &content);

	void setConferenceAddress (const IdentityAddress &conferenceAddress);
	void setParticipantDevices (const IdentityAddress &addr, const std::list<IdentityAddress> &devices);

	LinphoneReason onSipMessageReceived (SalOp *op, const SalMessage *message) override;

private:
	void designateAdmin ();
	void finalizeCreation ();
	bool isAdminLeft () const;

	// CallSessionListener
	void onCallSessionStateChanged (
		const std::shared_ptr<const CallSession> &session,
		LinphoneCallState state,
		const std::string &message
	) override;

	std::list<std::shared_ptr<Participant>> removedParticipants;

	L_DECLARE_PUBLIC(ServerGroupChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _SERVER_GROUP_CHAT_ROOM_P_H_
