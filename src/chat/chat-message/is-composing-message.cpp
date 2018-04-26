/*
 * is-composing-message.cpp
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

#include "chat/chat-message/notification-message-p.h"
#include "chat/chat-message/is-composing-message.h"
#include "sip-tools/sip-headers.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

IsComposingMessage::IsComposingMessage (
	const shared_ptr<AbstractChatRoom> &chatRoom,
	IsComposing &isComposingHandler,
	bool isComposing
) : NotificationMessage(*new NotificationMessagePrivate(chatRoom, ChatMessage::Direction::Outgoing)) {
	L_D();
	Content *content = new Content();
	content->setContentType(ContentType::ImIsComposing);
	content->setBody(isComposingHandler.createXml(isComposing));
	addContent(content);
	d->addSalCustomHeader(PriorityHeader::HeaderName, PriorityHeader::NonUrgent);
	d->addSalCustomHeader("Expires", "0");
}

LINPHONE_END_NAMESPACE
