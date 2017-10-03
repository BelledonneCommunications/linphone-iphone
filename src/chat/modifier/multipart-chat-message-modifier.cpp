/*
 * multipart-chat-message-modifier.cpp
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

#include "multipart-chat-message-modifier.h"
#include "address/address.h"
#include "chat/chat-message-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

int MultipartChatMessageModifier::encode (ChatMessagePrivate *messagePrivate) {
	if (messagePrivate->contents.size() > 1) {
		//TODO
	}
	return 0;
}

int MultipartChatMessageModifier::decode (ChatMessagePrivate *messagePrivate) {
	//TODO
	return 0;
}

LINPHONE_END_NAMESPACE
