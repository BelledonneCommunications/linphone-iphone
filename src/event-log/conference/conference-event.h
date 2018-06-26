/*
 * conference-event.h
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

#ifndef _L_CONFERENCE_EVENT_H_
#define _L_CONFERENCE_EVENT_H_

#include "event-log/event-log.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ConferenceEventPrivate;
class ChatRoomId;

class LINPHONE_PUBLIC ConferenceEvent : public EventLog {
public:
	ConferenceEvent (Type type, time_t creationTime, const ChatRoomId &chatRoomId);

	const ChatRoomId &getChatRoomId () const;

protected:
	ConferenceEvent (ConferenceEventPrivate &p, Type type, time_t creationTime, const ChatRoomId &chatRoomId);

private:
	L_DECLARE_PRIVATE(ConferenceEvent);
	L_DISABLE_COPY(ConferenceEvent);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CONFERENCE_EVENT_H_
