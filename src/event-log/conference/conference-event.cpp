/*
 * conference-event.cpp
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

#include "conference-event-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

ConferenceEvent::ConferenceEvent (Type type, time_t creationTime, const ChatRoomId &chatRoomId) :
	EventLog(*new ConferenceEventPrivate, type, creationTime) {
	L_D();
	L_ASSERT(type == Type::ConferenceCreated || type == Type::ConferenceTerminated);
	d->chatRoomId = chatRoomId;
}

ConferenceEvent::ConferenceEvent (
	ConferenceEventPrivate &p,
	Type type,
	time_t creationTime,
	const ChatRoomId &chatRoomId
) : EventLog(p, type, creationTime) {
	L_D();
	d->chatRoomId = chatRoomId;
}

const ChatRoomId &ConferenceEvent::getChatRoomId () const {
	L_D();
	return d->chatRoomId;
}

LINPHONE_END_NAMESPACE
