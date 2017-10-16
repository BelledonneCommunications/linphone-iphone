/*
 * main-db-p.h
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

#ifndef _MAIN_DB_P_H_
#define _MAIN_DB_P_H_

#include "abstract/abstract-db-p.h"
#include "chat/chat-message/chat-message.h"
#include "event-log/event-log.h"
#include "main-db.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatRoom;
class Content;

struct MessageEventReferences {
	long eventId;
	long localSipAddressId;
	long remoteSipAddressId;
	long chatRoomId;
};

class MainDbPrivate : public AbstractDbPrivate {
public:

private:
	long insertSipAddress (const std::string &sipAddress);
	void insertContent (long messageEventId, const Content &content);
	long insertContentType (const std::string &contentType);
	long insertEvent (EventLog::Type type, const tm &date);
	long insertChatRoom (long sipAddressId, int capabilities, const tm &date);
	void insertChatRoomParticipant (long chatRoomId, long sipAddressId, bool isAdmin);

	long insertMessageEvent (
		const MessageEventReferences &references,
		ChatMessage::State state,
		ChatMessage::Direction direction,
		const std::string &imdnMessageId,
		bool isSecured,
		const std::list<Content> &contents
	);

	void insertMessageParticipant (long messageEventId, long sipAddressId, ChatMessage::State state);

	std::unordered_map<std::string, std::weak_ptr<ChatRoom>> chatRooms;

	L_DECLARE_PUBLIC(MainDb);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _MAIN_DB_P_H_
