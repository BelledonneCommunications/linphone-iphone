/*
 * chat-message-event.cpp
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

#include "chat-message-event.h"
#include "event-log-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class ChatMessageEventPrivate : public EventLogPrivate {
public:
	shared_ptr<ChatMessage> chatMessage;
};

// -----------------------------------------------------------------------------

ChatMessageEvent::ChatMessageEvent (const shared_ptr<ChatMessage> &chatMessage) :
	EventLog(*new ChatMessageEventPrivate, EventLog::Type::ChatMessage) {
	L_D();
	L_ASSERT(chatMessage);
	d->chatMessage = chatMessage;
}

ChatMessageEvent::ChatMessageEvent (const ChatMessageEvent &src) : ChatMessageEvent(src.getChatMessage()) {}

ChatMessageEvent &ChatMessageEvent::operator= (const ChatMessageEvent &src) {
	L_D();
	if (this != &src) {
		EventLog::operator=(src);
		d->chatMessage = src.getPrivate()->chatMessage;
	}

	return *this;
}

shared_ptr<ChatMessage> ChatMessageEvent::getChatMessage () const {
	L_D();
	return d->chatMessage;
}

LINPHONE_END_NAMESPACE
