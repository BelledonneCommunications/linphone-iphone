/*
 * message-event.cpp
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

#include "event-p.h"

#include "message-event.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class MessageEventPrivate : public EventPrivate {
public:
	shared_ptr<Message> message;
};

// -----------------------------------------------------------------------------

MessageEvent::MessageEvent (const shared_ptr<Message> &message) : Event(*new MessageEventPrivate, Event::MessageEvent) {
	L_D(MessageEvent);
	L_ASSERT(message);
	d->message = message;
}

MessageEvent::MessageEvent (const MessageEvent &src) : MessageEvent(src.getMessage()) {}

shared_ptr<Message> MessageEvent::getMessage () const {
	L_D(const MessageEvent);
	return d->message;
}

LINPHONE_END_NAMESPACE
