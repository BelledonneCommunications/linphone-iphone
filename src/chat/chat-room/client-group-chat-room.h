/*
 * client-group-chat-room.h
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

#ifndef _L_CLIENT_GROUP_CHAT_ROOM_H_
#define _L_CLIENT_GROUP_CHAT_ROOM_H_

#include "chat/chat-room/chat-room.h"
#include "conference/remote-conference.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ClientGroupChatRoomPrivate;

class LINPHONE_PUBLIC ClientGroupChatRoom : public ChatRoom, public RemoteConference {
	friend class BasicToClientGroupChatRoomPrivate;
	friend class ClientGroupToBasicChatRoomPrivate;
	friend class Core;
	friend class CorePrivate;

public:
	L_OVERRIDE_SHARED_FROM_THIS(ClientGroupChatRoom);

	// TODO: Make me private.
	ClientGroupChatRoom (
		const std::shared_ptr<Core> &core,
		const std::string &factoryUri,
		const IdentityAddress &me,
		const std::string &subject
	);

	ClientGroupChatRoom (
		const std::shared_ptr<Core> &core,
		const ChatRoomId &chatRoomId,
		std::shared_ptr<Participant> &me,
		AbstractChatRoom::CapabilitiesMask capabilities,
		const std::string &subject,
		std::list<std::shared_ptr<Participant>> &&participants,
		unsigned int lastNotifyId
	);

	~ClientGroupChatRoom ();

	std::shared_ptr<Core> getCore () const;

	void allowCpim (bool value) override;
	void allowMultipart (bool value) override;
	bool canHandleCpim () const override;
	bool canHandleMultipart () const override;

	CapabilitiesMask getCapabilities () const override;
	bool hasBeenLeft () const override;

	const IdentityAddress &getConferenceAddress () const override;

	bool canHandleParticipants () const override;

	void deleteFromDb () override;

	void addParticipant (const IdentityAddress &addr, const CallSessionParams *params, bool hasMedia) override;
	void addParticipants (const std::list<IdentityAddress> &addresses, const CallSessionParams *params, bool hasMedia) override;

	void removeParticipant (const std::shared_ptr<const Participant> &participant) override;
	void removeParticipants (const std::list<std::shared_ptr<Participant>> &participants) override;

	std::shared_ptr<Participant> findParticipant (const IdentityAddress &addr) const override;

	std::shared_ptr<Participant> getMe () const override;
	int getParticipantCount () const override;
	const std::list<std::shared_ptr<Participant>> &getParticipants () const override;

	void setParticipantAdminStatus (const std::shared_ptr<Participant> &participant, bool isAdmin) override;

	const std::string &getSubject () const override;
	void setSubject (const std::string &subject) override;

	void join () override;
	void leave () override;

private:
	// TODO: Move me in ClientGroupChatRoomPrivate.
	// ALL METHODS AFTER THIS POINT.

	void onConferenceCreated (const IdentityAddress &addr) override;
	void onConferenceKeywordsChanged (const std::vector<std::string> &keywords) override;
	void onConferenceTerminated (const IdentityAddress &addr) override;
	void onFirstNotifyReceived (const IdentityAddress &addr) override;
	void onParticipantAdded (const std::shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) override;
	void onParticipantDeviceAdded (const std::shared_ptr<ConferenceParticipantDeviceEvent> &event, bool isFullState) override;
	void onParticipantDeviceRemoved (const std::shared_ptr<ConferenceParticipantDeviceEvent> &event, bool isFullState) override;
	void onParticipantRemoved (const std::shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) override;
	void onParticipantSetAdmin (const std::shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) override;
	void onSubjectChanged (const std::shared_ptr<ConferenceSubjectEvent> &event, bool isFullState) override;

	L_DECLARE_PRIVATE(ClientGroupChatRoom);
	L_DISABLE_COPY(ClientGroupChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CLIENT_GROUP_CHAT_ROOM_H_
