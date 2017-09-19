/*
 * cpim-chat-message-modifier.h
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

#ifndef _CPIM_CHAT_MESSAGE_MODIFIER_H_
#define _CPIM_CHAT_MESSAGE_MODIFIER_H_

#include "chat-message-modifier.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CpimChatMessageModifier : public ChatMessageModifier {
public:
	CpimChatMessageModifier () = default;

	void encode (ChatMessagePrivate *messagePrivate) override;
	void decode (ChatMessagePrivate *messagePrivate) override;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CPIM_CHAT_MESSAGE_MODIFIER_H_
