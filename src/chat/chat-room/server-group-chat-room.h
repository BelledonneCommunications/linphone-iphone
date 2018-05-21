/*
 * server-group-chat-room.h
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

#ifndef _L_SERVER_GROUP_CHAT_ROOM_H_
#define _L_SERVER_GROUP_CHAT_ROOM_H_

#include "chat/chat-room/chat-room.h"
#include "conference/local-conference.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class SalCallOp;
class ServerGroupChatRoomPrivate;

class ServerGroupChatRoom : public ChatRoom, public LocalConference {
public:
	// TODO: Make me private!
	ServerGroupChatRoom (const std::shared_ptr<Core> &core, SalCallOp *op);

	// TODO: Same idea.
	ServerGroupChatRoom (
		const std::shared_ptr<Core> &core,
		const IdentityAddress &peerAddress,
		AbstractChatRoom::CapabilitiesMask capabilities,
		const std::string &subject,
		std::list<std::shared_ptr<Participant>> &&participants,
		unsigned int lastNotifyId
	);

	~ServerGroupChatRoom ();

	std::shared_ptr<Core> getCore () const;

	void allowCpim (bool value) override;
	void allowMultipart (bool value) override;
	bool canHandleCpim () const override;
	bool canHandleMultipart () const override;

	std::shared_ptr<Participant> findParticipant (const std::shared_ptr<const CallSession> &session) const;

	CapabilitiesMask getCapabilities () const override;
	bool hasBeenLeft () const override;

	const IdentityAddress &getConferenceAddress () const override;

	bool canHandleParticipants () const override;

	void addParticipant (const IdentityAddress &address, const CallSessionParams *params, bool hasMedia) override;
	void addParticipants (
		const std::list<IdentityAddress> &addresses,
		const CallSessionParams *params,
		bool hasMedia
	) override;

	void removeParticipant (const std::shared_ptr<Participant> &participant) override;
	void removeParticipants (const std::list<std::shared_ptr<Participant>> &participants) override;

	std::shared_ptr<Participant> findParticipant (const IdentityAddress &participantAddress) const override;

	std::shared_ptr<Participant> getMe () const override;
	int getParticipantCount () const override;
	const std::list<std::shared_ptr<Participant>> &getParticipants () const override;

	void setParticipantAdminStatus (const std::shared_ptr<Participant> &participant, bool isAdmin) override;

	const std::string &getSubject () const override;
	void setSubject (const std::string &subject) override;

	void join () override;
	void leave () override;

	/* ConferenceListener */
	void onFirstNotifyReceived (const IdentityAddress &addr) override;

private:
	L_DECLARE_PRIVATE(ServerGroupChatRoom);
	L_DISABLE_COPY(ServerGroupChatRoom);
};

std::ostream &operator<< (std::ostream &stream, const ServerGroupChatRoom *chatRoom);

LINPHONE_END_NAMESPACE

#endif // ifndef _L_SERVER_GROUP_CHAT_ROOM_H_
