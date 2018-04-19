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

#include "chat/chat-message/imdn-message-p.h"
#include "chat/chat-room/chat-room-p.h"
#include "content/content-disposition.h"
#include "logger/logger.h"
#include "sip-tools/sip-headers.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

void ImdnMessagePrivate::setState (ChatMessage::State newState, bool force) {
	L_Q();

	if (newState == ChatMessage::State::Delivered) {
		for (const auto &message : context.deliveredMessages)
			message->getPrivate()->updateInDb();
		for (const auto &message : context.displayedMessages)
			message->getPrivate()->updateInDb();
		static_pointer_cast<ChatRoom>(context.chatRoom)->getPrivate()->getImdnHandler()->onImdnMessageDelivered(q->getSharedFromThis());
	} else if (newState == ChatMessage::State::NotDelivered) {
		// TODO: Maybe we should retry sending the IMDN message if we get an error here
	}
}

// -----------------------------------------------------------------------------

ImdnMessage::ImdnMessage (
	const shared_ptr<AbstractChatRoom> &chatRoom,
	const list<shared_ptr<ChatMessage>> &deliveredMessages,
	const list<shared_ptr<ChatMessage>> &displayedMessages
) : ImdnMessage(Context(chatRoom, deliveredMessages, displayedMessages)) {}

ImdnMessage::ImdnMessage (
	const shared_ptr<AbstractChatRoom> &chatRoom,
	const list<Imdn::MessageReason> &nonDeliveredMessages
) : ImdnMessage(Context(chatRoom, nonDeliveredMessages)) {}

ImdnMessage::ImdnMessage (const std::shared_ptr<ImdnMessage> &message) : ImdnMessage(message->getPrivate()->context) {}

ImdnMessage::ImdnMessage (const Context &context) : NotificationMessage(*new ImdnMessagePrivate(context)) {
	L_D();

	for (const auto &message : d->context.deliveredMessages) {
		Content *content = new Content();
		content->setContentDisposition(ContentDisposition::Notification);
		content->setContentType(ContentType::Imdn);
		content->setBody(Imdn::createXml(message->getImdnMessageId(), message->getTime(), Imdn::Type::Delivery, LinphoneReasonNone));
		addContent(content);
	}
	for (const auto &message : d->context.displayedMessages) {
		Content *content = new Content();
		content->setContentDisposition(ContentDisposition::Notification);
		content->setContentType(ContentType::Imdn);
		content->setBody(Imdn::createXml(message->getImdnMessageId(), message->getTime(), Imdn::Type::Display, LinphoneReasonNone));
		addContent(content);
	}
	for (const auto &mr : d->context.nonDeliveredMessages) {
		Content *content = new Content();
		content->setContentDisposition(ContentDisposition::Notification);
		content->setContentType(ContentType::Imdn);
		content->setBody(Imdn::createXml(mr.message->getImdnMessageId(), mr.message->getTime(), Imdn::Type::Delivery, mr.reason));
		addContent(content);
	}

	d->addSalCustomHeader(PriorityHeader::HeaderName, PriorityHeader::NonUrgent);
	if (!d->context.nonDeliveredMessages.empty())
		d->setEncryptionPrevented(true);
}

LINPHONE_END_NAMESPACE
