/*
 * message-event.h
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

#ifndef _MESSAGE_EVENT_H_
#define _MESSAGE_EVENT_H_

#include <memory>

#include "event-log.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Message;
class MessageEventPrivate;

class LINPHONE_PUBLIC MessageEvent : public EventLog {
public:
	MessageEvent (const std::shared_ptr<Message> &message);
	MessageEvent (const MessageEvent &src);

	MessageEvent &operator= (const MessageEvent &src);

	std::shared_ptr<Message> getMessage () const;

private:
	L_DECLARE_PRIVATE(MessageEvent);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _MESSAGE_EVENT_H_
