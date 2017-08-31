/*
 * chat-room.h
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

#ifndef _CHAT_ROOM_H_
#define _CHAT_ROOM_H_

#include <list>

#include "object/object.h"

#include "linphone/types.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatRoomPrivate;

class ChatRoom : public Object {
public:
	ChatRoom (LinphoneCore *core, LinphoneAddress *peerAddress);
	virtual ~ChatRoom () = default;

	void compose ();
	LinphoneChatMessage *createFileTransferMessage (const LinphoneContent *initialContent);
	LinphoneChatMessage *createMessage (const std::string &msg);
	void deleteHistory ();
	void deleteMessage (LinphoneChatMessage *msg);
	LinphoneChatMessage *findMessage (const std::string &messageId);
	uint32_t getChar () const;
	std::list<LinphoneChatMessage *> getHistory (int nbMessages);
	int getHistorySize ();
	std::list<LinphoneChatMessage *> getHistoryRange (int startm, int endm);
	int getUnreadMessagesCount ();
	bool isRemoteComposing () const;
	void markAsRead ();
	void sendMessage (LinphoneChatMessage *msg);

	LinphoneCall *getCall () const;
	LinphoneCore *getCore () const;

	const LinphoneAddress *getPeerAddress () const;

private:
	L_DECLARE_PRIVATE(ChatRoom);
	L_DISABLE_COPY(ChatRoom);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CHAT_ROOM_H_
