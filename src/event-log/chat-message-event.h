/*
 * chat-message-event.h
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

#ifndef _CHAT_MESSAGE_EVENT_H_
#define _CHAT_MESSAGE_EVENT_H_

#include <memory>

#include "event-log.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatMessage;
class ChatMessageEventPrivate;

class LINPHONE_PUBLIC ChatMessageEvent : public EventLog {
public:
	ChatMessageEvent (const std::shared_ptr<ChatMessage> &chatMessage);
	ChatMessageEvent (const ChatMessageEvent &src);

	ChatMessageEvent &operator= (const ChatMessageEvent &src);

	std::shared_ptr<ChatMessage> getChatMessage () const;

private:
	L_DECLARE_PRIVATE(ChatMessageEvent);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CHAT_MESSAGE_EVENT_H_
