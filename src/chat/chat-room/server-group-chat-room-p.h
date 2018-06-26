/*
 * server-group-chat-room-p.h
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

#ifndef _L_SERVER_GROUP_CHAT_ROOM_P_H_
#define _L_SERVER_GROUP_CHAT_ROOM_P_H_

#include <chrono>
#include <queue>
#include <unordered_map>

#include "chat-room-p.h"
#include "server-group-chat-room.h"
#include "conference/participant-device.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ParticipantDevice;

class ServerGroupChatRoomPrivate : public ChatRoomPrivate {
public:
	void setState (ChatRoom::State state) override;

	std::shared_ptr<Participant> addParticipant (const IdentityAddress &participantAddress);
	void removeParticipant (const std::shared_ptr<const Participant> &participant);

	std::shared_ptr<Participant> findFilteredParticipant (const std::shared_ptr<const CallSession> &session) const;
	std::shared_ptr<Participant> findFilteredParticipant (const IdentityAddress &participantAddress) const;

	ParticipantDevice::State getParticipantDeviceState (const std::shared_ptr<const ParticipantDevice> &device) const;
	void setParticipantDeviceState (const std::shared_ptr<ParticipantDevice> &device, ParticipantDevice::State state);

	void acceptSession (const std::shared_ptr<CallSession> &session);
	void confirmCreation ();
	void confirmJoining (SalCallOp *op);
	void confirmRecreation (SalCallOp *op);
	void declineSession (const std::shared_ptr<CallSession> &session, LinphoneReason reason);
	void dispatchQueuedMessages ();

	void subscribeReceived (LinphoneEvent *event);

	bool update (SalCallOp *op);

	void setConferenceAddress (const IdentityAddress &conferenceAddress);
	void setParticipantDevices (const IdentityAddress &addr, const std::list<IdentityAddress> &devices);
	void addParticipantDevice (const IdentityAddress &participantAddress, const IdentityAddress &deviceAddress);
	void addCompatibleParticipants (const IdentityAddress &deviceAddr, const std::list<IdentityAddress> &compatibleParticipants);
	void checkCompatibleParticipants (const IdentityAddress &deviceAddr, const std::list<IdentityAddress> &addressesToCheck);

	LinphoneReason onSipMessageReceived (SalOp *op, const SalMessage *message) override;

private:
	struct Message {
		Message (const std::string &from, const ContentType &contentType, const std::string &text, const SalCustomHeader *salCustomHeaders)
			: fromAddr(from)
		{
			content.setContentType(contentType);
			if (!text.empty())
				content.setBodyFromUtf8(text);
			if (salCustomHeaders)
				customHeaders = sal_custom_header_clone(salCustomHeaders);
		}

		~Message () {
			if (customHeaders)
				sal_custom_header_free(customHeaders);
		}

		IdentityAddress fromAddr;
		Content content;
		std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
		SalCustomHeader *customHeaders = nullptr;
	};

	static void copyMessageHeaders (const std::shared_ptr<Message> &fromMessage, const std::shared_ptr<ChatMessage> &toMessage);

	void byeDevice (const std::shared_ptr<ParticipantDevice> &device);
	void designateAdmin ();
	void dispatchMessage (const std::shared_ptr<Message> &message, const std::string &uri);
	void finalizeCreation ();
	void inviteDevice (const std::shared_ptr<ParticipantDevice> &device);
	bool isAdminLeft () const;
	void queueMessage (const std::shared_ptr<Message> &message);
	void queueMessage (const std::shared_ptr<Message> &msg, const IdentityAddress &deviceAddress);
	void removeNonPresentParticipants (const std::list <IdentityAddress> &compatibleParticipants);

	void onParticipantDeviceLeft (const std::shared_ptr<ParticipantDevice> &device);

	// ChatRoomListener
	void onChatRoomInsertRequested (const std::shared_ptr<AbstractChatRoom> &chatRoom) override;
	void onChatRoomInsertInDatabaseRequested (const std::shared_ptr<AbstractChatRoom> &chatRoom) override;
	void onChatRoomDeleteRequested (const std::shared_ptr<AbstractChatRoom> &chatRoom) override;

	// CallSessionListener
	void onCallSessionStateChanged (
		const std::shared_ptr<CallSession> &session,
		CallSession::State newState,
		const std::string &message
	) override;
	void onCallSessionSetReleased (const std::shared_ptr<CallSession> &session) override;

	std::list<std::shared_ptr<Participant>> filteredParticipants;
	ChatRoomListener *chatRoomListener = this;
	ServerGroupChatRoom::CapabilitiesMask capabilities = ServerGroupChatRoom::Capabilities::Conference;
	bool joiningPendingAfterCreation = false;
	std::unordered_map<std::string, std::queue<std::shared_ptr<Message>>> queuedMessages;

	L_DECLARE_PUBLIC(ServerGroupChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_SERVER_GROUP_CHAT_ROOM_P_H_
