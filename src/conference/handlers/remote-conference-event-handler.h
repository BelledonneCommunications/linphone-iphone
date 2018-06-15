/*
 * remote-conference-event-handler.h
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

#ifndef _L_REMOTE_CONFERENCE_EVENT_HANDLER_H_
#define _L_REMOTE_CONFERENCE_EVENT_HANDLER_H_

#include "object/object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatRoomId;
class RemoteConference;
class RemoteConferenceEventHandlerPrivate;

class LINPHONE_PUBLIC RemoteConferenceEventHandler : public Object {
	friend class ClientGroupChatRoom;

public:
	RemoteConferenceEventHandler (RemoteConference *remoteConference);
	~RemoteConferenceEventHandler ();

	void subscribe (const ChatRoomId &chatRoomId);
	void notifyReceived (const std::string &xmlBody);
	void multipartNotifyReceived (const std::string &xmlBody);
	void unsubscribe ();

	void setChatRoomId (ChatRoomId chatRoomId);
	const ChatRoomId &getChatRoomId () const;

	unsigned int getLastNotify () const;
	void setLastNotify (unsigned int lastNotify);
	void resetLastNotify ();

private:
	L_DECLARE_PRIVATE(RemoteConferenceEventHandler);
	L_DISABLE_COPY(RemoteConferenceEventHandler);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_REMOTE_CONFERENCE_EVENT_HANDLER_H_
