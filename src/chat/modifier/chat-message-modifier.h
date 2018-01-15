/*
 * chat-message-modifier.h
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

#ifndef _L_CHAT_MESSAGE_MODIFIER_H_
#define _L_CHAT_MESSAGE_MODIFIER_H_

#include "object/object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatMessage;

class ChatMessageModifier {
public:
	enum class Result {
		Skipped = -1,
		Done = 0,
		Suspended = 1,
		Error = 2
	};

	virtual ~ChatMessageModifier () = default;

	/**
	 * This method will be called when the message is about to be sent.
	 * It should check first if the internalContent is filled.
	 * If so, it should apply it's changes to it, otherwise it should use the contentsList.
	 */
	virtual Result encode (const std::shared_ptr<ChatMessage> &message, int &errorCode) = 0;

	/**
	 * This method will be called when the message is about to be received.
	 * It should apply it's changes to the internal content, the last modifier will take care of filling the contentsList.
	 */
	virtual Result decode (const std::shared_ptr<ChatMessage> &message, int &errorCode) = 0;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CPIM_CHAT_MESSAGE_MODIFIER_H_
