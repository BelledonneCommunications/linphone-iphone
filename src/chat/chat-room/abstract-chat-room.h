/*
 * abstract-chat-room.h
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

#ifndef _L_ABSTRACT_CHAT_ROOM_H_
#define _L_ABSTRACT_CHAT_ROOM_H_

#include "linphone/utils/enum-mask.h"

#include "chat/chat-message/chat-message.h"
#include "conference/conference-interface.h"
#include "core/core-accessor.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class AbstractChatRoomPrivate;
class ChatRoomId;
class EventLog;

class LINPHONE_PUBLIC AbstractChatRoom : public Object, public CoreAccessor, public ConferenceInterface {
	friend class ChatMessage;
	friend class ChatMessagePrivate;
	friend class ClientGroupToBasicChatRoomPrivate;
	friend class Core;
	friend class CorePrivate;
	friend class MainDb;
	friend class ProxyChatRoomPrivate;

public:
	L_OVERRIDE_SHARED_FROM_THIS(AbstractChatRoom);

	L_DECLARE_ENUM(Capabilities, L_ENUM_VALUES_CHAT_ROOM_CAPABILITIES);
	L_DECLARE_ENUM(State, L_ENUM_VALUES_CHAT_ROOM_STATE);

	typedef EnumMask<Capabilities> CapabilitiesMask;

	virtual void allowCpim (bool value) = 0;
	virtual void allowMultipart (bool value) = 0;
	virtual bool canHandleCpim () const = 0;
	virtual bool canHandleMultipart () const = 0;

	virtual const ChatRoomId &getChatRoomId () const = 0;

	virtual const IdentityAddress &getPeerAddress () const = 0;
	virtual const IdentityAddress &getLocalAddress () const = 0;

	virtual time_t getCreationTime () const = 0;
	virtual time_t getLastUpdateTime () const = 0;

	virtual CapabilitiesMask getCapabilities () const = 0;
	virtual State getState () const = 0;
	virtual bool hasBeenLeft () const = 0;

	virtual std::list<std::shared_ptr<EventLog>> getMessageHistory (int nLast) const = 0;
	virtual std::list<std::shared_ptr<EventLog>> getMessageHistoryRange (int begin, int end) const = 0;
	virtual std::list<std::shared_ptr<EventLog>> getHistory (int nLast) const = 0;
	virtual std::list<std::shared_ptr<EventLog>> getHistoryRange (int begin, int end) const = 0;
	virtual int getHistorySize () const = 0;

	virtual void deleteFromDb () = 0;
	virtual void deleteHistory () = 0;

	virtual std::shared_ptr<ChatMessage> getLastChatMessageInHistory () const = 0;

	virtual int getChatMessageCount () const = 0;
	virtual int getUnreadChatMessageCount () const = 0;

	virtual void compose () = 0;
	virtual bool isRemoteComposing () const = 0;
	virtual std::list<IdentityAddress> getComposingAddresses () const = 0;

	virtual std::shared_ptr<ChatMessage> createChatMessage () = 0;
	virtual std::shared_ptr<ChatMessage> createChatMessage (const std::string &text) = 0;

	virtual std::shared_ptr<ChatMessage> createFileTransferMessage (Content *initialContent) = 0;

	virtual std::shared_ptr<ChatMessage> findChatMessage (const std::string &messageId) const = 0;
	virtual std::shared_ptr<ChatMessage> findChatMessage (
		const std::string &messageId,
		ChatMessage::Direction direction
	) const = 0;

	virtual void markAsRead () = 0;

protected:
	explicit AbstractChatRoom (AbstractChatRoomPrivate &p, const std::shared_ptr<Core> &core);

private:
	L_DECLARE_PRIVATE(AbstractChatRoom);
	L_DISABLE_COPY(AbstractChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_ABSTRACT_CHAT_ROOM_H_
