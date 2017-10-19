/*
 * chat-message-event.h
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

#ifndef _CHAT_MESSAGE_EVENT_H_
#define _CHAT_MESSAGE_EVENT_H_

#include <memory>

#include "event-log/event-log.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatMessage;
class ChatMessageEventPrivate;

class LINPHONE_PUBLIC ChatMessageEvent : public EventLog {
public:
	ChatMessageEvent (std::time_t time, const std::shared_ptr<ChatMessage> &chatMessage);

	std::shared_ptr<ChatMessage> getChatMessage () const;

private:
	L_DECLARE_PRIVATE(ChatMessageEvent);
	L_DISABLE_COPY(ChatMessageEvent);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CHAT_MESSAGE_EVENT_H_
