/*
 * imdn-message.cpp
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
#include "chat/chat-message/imdn-message.h"
#include "content/content-disposition.h"
#include "sip-tools/sip-headers.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

ImdnMessage::ImdnMessage (
	const shared_ptr<AbstractChatRoom> &chatRoom,
	const list<const shared_ptr<ChatMessage>> &deliveredMessages,
	const list<const shared_ptr<ChatMessage>> &displayedMessages
) : NotificationMessage(*new NotificationMessagePrivate(chatRoom, ChatMessage::Direction::Outgoing)) {
	L_D();

	for (const auto &message : deliveredMessages) {
		Content *content = new Content();
		content->setContentDisposition(ContentDisposition::Notification);
		content->setContentType(ContentType::Imdn);
		content->setBody(Imdn::createXml(message->getImdnMessageId(), message->getTime(), Imdn::Type::Delivery, LinphoneReasonNone));
		addContent(content);
	}
	for (const auto &message : displayedMessages) {
		Content *content = new Content();
		content->setContentDisposition(ContentDisposition::Notification);
		content->setContentType(ContentType::Imdn);
		content->setBody(Imdn::createXml(message->getImdnMessageId(), message->getTime(), Imdn::Type::Display, LinphoneReasonNone));
		addContent(content);
	}

	d->addSalCustomHeader(PriorityHeader::HeaderName, PriorityHeader::NonUrgent);
}

ImdnMessage::ImdnMessage (
	const shared_ptr<AbstractChatRoom> &chatRoom,
	const list<Imdn::MessageReason> &nonDeliveredMessages
) : NotificationMessage(*new NotificationMessagePrivate(chatRoom, ChatMessage::Direction::Outgoing)) {
	L_D();

	for (const auto &mr : nonDeliveredMessages) {
		Content *content = new Content();
		content->setContentDisposition(ContentDisposition::Notification);
		content->setContentType(ContentType::Imdn);
		content->setBody(Imdn::createXml(mr.message->getImdnMessageId(), mr.message->getTime(), Imdn::Type::Delivery, mr.reason));
		addContent(content);
	}

	d->addSalCustomHeader(PriorityHeader::HeaderName, PriorityHeader::NonUrgent);
	d->setEncryptionPrevented(true);
}

LINPHONE_END_NAMESPACE
