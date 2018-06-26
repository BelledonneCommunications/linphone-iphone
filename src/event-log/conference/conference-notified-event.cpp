/*
 * conference-notified-event.cpp
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

#include "conference-notified-event-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

ConferenceNotifiedEvent::ConferenceNotifiedEvent (
	Type type,
	time_t creationTime,
	const ChatRoomId &chatRoomId,
	unsigned int notifyId
) : ConferenceEvent(*new ConferenceNotifiedEventPrivate, type, creationTime, chatRoomId) {
	L_D();
	d->notifyId = notifyId;
}

ConferenceNotifiedEvent::ConferenceNotifiedEvent (
	ConferenceNotifiedEventPrivate &p,
	Type type,
	time_t creationTime,
	const ChatRoomId &chatRoomId,
	unsigned int notifyId
) : ConferenceEvent(p, type, creationTime, chatRoomId) {
	L_D();
	d->notifyId = notifyId;
}

unsigned int ConferenceNotifiedEvent::getNotifyId () const {
	L_D();
	return d->notifyId;
}

LINPHONE_END_NAMESPACE
