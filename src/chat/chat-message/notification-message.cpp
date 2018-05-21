/*
 * notification-message.cpp
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

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

NotificationMessage::NotificationMessage (const shared_ptr<AbstractChatRoom> &chatRoom, ChatMessage::Direction direction) :
	NotificationMessage(*new NotificationMessagePrivate(chatRoom, direction)) {
}

NotificationMessage::NotificationMessage (NotificationMessagePrivate &p) : ChatMessage(p) {
	L_D();
	d->displayNotificationRequired = false;
	d->negativeDeliveryNotificationRequired = false;
	d->positiveDeliveryNotificationRequired = false;
	d->toBeStored = false;
	d->contentEncoding = "deflate";
}

void NotificationMessage::setToBeStored (bool value) {
}

LINPHONE_END_NAMESPACE
