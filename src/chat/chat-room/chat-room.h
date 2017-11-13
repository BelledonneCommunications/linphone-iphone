/*
 * chat-room.h
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

#ifndef _CHAT_ROOM_H_
#define _CHAT_ROOM_H_

#include "chat/chat-message/chat-message.h"
#include "chat/chat-room/chat-room-id.h"
#include "conference/conference-interface.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatRoomPrivate;

class LINPHONE_PUBLIC ChatRoom : public Object, public CoreAccessor, public ConferenceInterface {
	friend class Core;
	friend class CorePrivate;
	friend class ChatMessage;
	friend class ChatMessagePrivate;
	friend class FileTransferChatMessageModifier;

public:
	L_OVERRIDE_SHARED_FROM_THIS(ChatRoom);

	L_DECLARE_ENUM(Capabilities, L_ENUM_VALUES_CHAT_ROOM_CAPABILITIES);
	L_DECLARE_ENUM(State, L_ENUM_VALUES_CHAT_ROOM_STATE);

	typedef int CapabilitiesMask;

	virtual ~ChatRoom () = default;

	const ChatRoomId &getChatRoomId () const;

	const SimpleAddress &getPeerAddress () const;
	const SimpleAddress &getLocalAddress () const;

	virtual CapabilitiesMask getCapabilities () const = 0;

	// TODO: Remove useless functions.
	void compose ();
	std::shared_ptr<ChatMessage> createFileTransferMessage (const LinphoneContent *initialContent);
	std::shared_ptr<ChatMessage> createMessage (const std::string &msg);
	std::shared_ptr<ChatMessage> createMessage ();
	void deleteHistory ();
	void deleteMessage (const std::shared_ptr<ChatMessage> &msg);
	std::shared_ptr<ChatMessage> findMessage (const std::string &messageId);
	std::shared_ptr<ChatMessage> findMessageWithDirection (const std::string &messageId, ChatMessage::Direction direction);
	std::list<std::shared_ptr<ChatMessage>> getHistory (int nbMessages);
	int getHistorySize ();
	std::list<std::shared_ptr<ChatMessage>> getHistoryRange (int startm, int endm);
	int getUnreadChatMessagesCount ();
	bool isRemoteComposing () const;

	virtual void markAsRead ();

	State getState () const;

protected:
	explicit ChatRoom (ChatRoomPrivate &p, const std::shared_ptr<Core> &core, const ChatRoomId &chatRoomId);

	virtual void onChatMessageReceived (const std::shared_ptr<ChatMessage> &msg) = 0;

private:
	L_DECLARE_PRIVATE(ChatRoom);
	L_DISABLE_COPY(ChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CHAT_ROOM_H_
