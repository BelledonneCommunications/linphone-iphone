/*
 * chat-message-modifier.h
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

#ifndef _CHAT_MESSAGE_MODIFIER_H_
#define _CHAT_MESSAGE_MODIFIER_H_

#include "linphone/utils/general.h"
#include "private.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatMessagePrivate;

class ChatMessageModifier {
public:
	virtual ~ChatMessageModifier () = default;

	/**
	 * This method will be called when the message is about to be sent.
	 * It should check first if the internalContent is filled. 
	 * If so, it should apply it's changes to it, otherwise it should use the contentsList.
	 * If it returns 0, it means everything went well. 
	 * If it returns -1, it means it didn't change anything. 
	 * If it returns 1, it means to abort the sending process and it will be resumed later.
	 * If it returns any other value > 1, it's an error code.
	 */
	virtual int encode (ChatMessagePrivate *messagePrivate) = 0;

	/**
	 * This method will be called when the message is about to be received.
	 * It should apply it's changes to the internal content, the last modifier will take care of filling the contentsList.
	 * If it returns 0, it means everything went well. 
	 * If it returns -1, it means it didn't change anything. 
	 * If it returns any other value > 0, it's an error code.
	 */
	virtual int decode (ChatMessagePrivate *messagePrivate) = 0;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CPIM_CHAT_MESSAGE_MODIFIER_H_
