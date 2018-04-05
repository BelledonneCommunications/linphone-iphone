/*
 * proxy-chat-room.h
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

#ifndef _L_PROXY_CHAT_ROOM_H_
#define _L_PROXY_CHAT_ROOM_H_

#include "abstract-chat-room.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatRoom;
class ProxyChatRoomPrivate;

class LINPHONE_PUBLIC ProxyChatRoom : public AbstractChatRoom {
	friend class CorePrivate;

public:
	const ChatRoomId &getChatRoomId () const override;

	const IdentityAddress &getPeerAddress () const override;
	const IdentityAddress &getLocalAddress () const override;

	time_t getCreationTime () const override;
	time_t getLastUpdateTime () const override;

	CapabilitiesMask getCapabilities () const override;
	State getState () const override;
	bool hasBeenLeft () const override;

	std::list<std::shared_ptr<EventLog>> getMessageHistory (int nLast) const override;
	std::list<std::shared_ptr<EventLog>> getMessageHistoryRange (int begin, int end) const override;
	std::list<std::shared_ptr<EventLog>> getHistory (int nLast) const override;
	std::list<std::shared_ptr<EventLog>> getHistoryRange (int begin, int end) const override;
	int getHistorySize () const override;

	void deleteFromDb () override;
	void deleteHistory () override;

	std::shared_ptr<ChatMessage> getLastChatMessageInHistory () const override;

	int getChatMessageCount () const override;
	int getUnreadChatMessageCount () const override;

	void compose () override;
	bool isRemoteComposing () const override;
	std::list<IdentityAddress> getComposingAddresses () const override;

	std::shared_ptr<ChatMessage> createChatMessage () override;
	std::shared_ptr<ChatMessage> createChatMessage (const std::string &text) override;

	// TODO: Remove LinphoneContent by LinphonePrivate::Content.
	std::shared_ptr<ChatMessage> createFileTransferMessage (const LinphoneContent *initialContent) override;

	std::shared_ptr<ChatMessage> findChatMessage (const std::string &messageId) const override;
	std::shared_ptr<ChatMessage> findChatMessage (
		const std::string &messageId,
		ChatMessage::Direction direction
	) const override;

	void markAsRead () override;

	const IdentityAddress &getConferenceAddress () const override;

	void allowCpim (bool value) override;
	void allowMultipart (bool value) override;
	bool canHandleCpim () const override;
	bool canHandleMultipart () const override;

	bool canHandleParticipants () const override;

	void addParticipant (
		const IdentityAddress &participantAddress,
		const CallSessionParams *params,
		bool hasMedia
	) override;
	void addParticipants (
		const std::list<IdentityAddress> &addresses,
		const CallSessionParams *params,
		bool hasMedia
	) override;

	void removeParticipant (const std::shared_ptr<const Participant> &participant) override;
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

	const std::shared_ptr<AbstractChatRoom> &getProxiedChatRoom () const;

protected:
	ProxyChatRoom (ProxyChatRoomPrivate &p, const std::shared_ptr<ChatRoom> &chatRoom);

private:
	L_DECLARE_PRIVATE(ProxyChatRoom);
	L_DISABLE_COPY(ProxyChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_PROXY_CHAT_ROOM_H_
