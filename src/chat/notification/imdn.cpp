/*
 * imdn.cpp
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

#include <algorithm>

#include "chat/chat-message/chat-message-p.h"
#include "chat/chat-room/chat-room-p.h"
#include "core/core.h"
#include "logger/logger.h"
#include "xml/imdn.h"
#include "xml/linphone-imdn.h"

#include "imdn.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

Imdn::Imdn (ChatRoom *chatRoom) : chatRoom(chatRoom) {}

Imdn::~Imdn () {
	stopTimer();
}

// -----------------------------------------------------------------------------

void Imdn::notifyDelivery (const shared_ptr<ChatMessage> &message) {
	if (find(deliveredMessages.begin(), deliveredMessages.end(), message) == deliveredMessages.end()) {
		deliveredMessages.push_back(message);
		startTimer();
	}
}

void Imdn::notifyDeliveryError (const shared_ptr<ChatMessage> &message, LinphoneReason reason) {
	auto it = find_if(nonDeliveredMessages.begin(), nonDeliveredMessages.end(), [message](const MessageReason mr) {
		return message == mr.message;
	});
	if (it == nonDeliveredMessages.end()) {
		nonDeliveredMessages.emplace_back(message, reason);
		startTimer();
	}
}

void Imdn::notifyDisplay (const shared_ptr<ChatMessage> &message) {
	auto it = find(deliveredMessages.begin(), deliveredMessages.end(), message);
	if (it != deliveredMessages.end())
		deliveredMessages.erase(it);

	if (find(displayedMessages.begin(), displayedMessages.end(), message) == displayedMessages.end()) {
		displayedMessages.push_back(message);
		startTimer();
	}
}

// -----------------------------------------------------------------------------

string Imdn::createXml (const string &id, time_t timestamp, Imdn::Type imdnType, LinphoneReason reason) {
	char *datetime = linphone_timestamp_to_rfc3339_string(timestamp);
	Xsd::Imdn::Imdn imdn(id, datetime);
	ms_free(datetime);
	if (imdnType == Imdn::Type::Delivery) {
		Xsd::Imdn::Status status;
		if (reason == LinphoneReasonNone) {
			auto delivered = Xsd::Imdn::Delivered();
			status.setDelivered(delivered);
		} else {
			auto failed = Xsd::Imdn::Failed();
			status.setFailed(failed);
			Xsd::LinphoneImdn::ImdnReason imdnReason(linphone_reason_to_string(reason));
			imdnReason.setCode(linphone_reason_to_error_code(reason));
			status.setReason(imdnReason);
		}
		Xsd::Imdn::DeliveryNotification deliveryNotification(status);
		imdn.setDeliveryNotification(deliveryNotification);
	} else if (imdnType == Imdn::Type::Display) {
		Xsd::Imdn::Status1 status;
		auto displayed = Xsd::Imdn::Displayed();
		status.setDisplayed(displayed);
		Xsd::Imdn::DisplayNotification displayNotification(status);
		imdn.setDisplayNotification(displayNotification);
	}

	stringstream ss;
	Xsd::XmlSchema::NamespaceInfomap map;
	map[""].name = "urn:ietf:params:xml:ns:imdn";
	map["imdn"].name = "http://www.linphone.org/xsds/imdn.xsd";
	Xsd::Imdn::serializeImdn(ss, imdn, map);
	return ss.str();
}

void Imdn::parse (const shared_ptr<ChatMessage> &chatMessage) {
	shared_ptr<AbstractChatRoom> cr = chatMessage->getChatRoom();
	for (const auto &content : chatMessage->getPrivate()->getContents()) {
		istringstream data(content->getBodyAsString());
		unique_ptr<Xsd::Imdn::Imdn> imdn(Xsd::Imdn::parseImdn(data, Xsd::XmlSchema::Flags::dont_validate));
		if (!imdn)
			continue;
		shared_ptr<ChatMessage> cm = cr->findChatMessage(imdn->getMessageId());
		if (!cm) {
			lWarning() << "Received IMDN for unknown message " << imdn->getMessageId();
		} else {
			auto policy = linphone_core_get_im_notif_policy(cr->getCore()->getCCore());
			time_t imdnTime = chatMessage->getTime();
			const IdentityAddress &participantAddress = chatMessage->getFromAddress().getAddressWithoutGruu();
			auto &deliveryNotification = imdn->getDeliveryNotification();
			auto &displayNotification = imdn->getDisplayNotification();
			if (deliveryNotification.present()) {
				auto &status = deliveryNotification.get().getStatus();
				if (status.getDelivered().present() && linphone_im_notif_policy_get_recv_imdn_delivered(policy))
					cm->getPrivate()->setParticipantState(participantAddress, ChatMessage::State::DeliveredToUser, imdnTime);
				else if ((status.getFailed().present() || status.getError().present())
					&& linphone_im_notif_policy_get_recv_imdn_delivered(policy)
				)
					cm->getPrivate()->setParticipantState(participantAddress, ChatMessage::State::NotDelivered, imdnTime);
			} else if (displayNotification.present()) {
				auto &status = displayNotification.get().getStatus();
				if (status.getDisplayed().present() && linphone_im_notif_policy_get_recv_imdn_displayed(policy))
					cm->getPrivate()->setParticipantState(participantAddress, ChatMessage::State::Displayed, imdnTime);
			}
		}
	}
}

// -----------------------------------------------------------------------------

int Imdn::timerExpired (void *data, unsigned int revents) {
	Imdn *d = reinterpret_cast<Imdn *>(data);
	d->stopTimer();
	d->send();
	return BELLE_SIP_STOP;
}

// -----------------------------------------------------------------------------

void Imdn::send () {
	if (!deliveredMessages.empty() || !displayedMessages.empty())
		chatRoom->getPrivate()->createImdnMessage(deliveredMessages, displayedMessages)->send();
	if (!nonDeliveredMessages.empty())
		chatRoom->getPrivate()->createImdnMessage(nonDeliveredMessages)->send();
}

void Imdn::startTimer () {
	unsigned int duration = 500;
	if (!timer)
		timer = chatRoom->getCore()->getCCore()->sal->create_timer(timerExpired, this, duration, "imdn timeout");
	else
		belle_sip_source_set_timeout(timer, duration);
}

void Imdn::stopTimer () {
	if (timer) {
		auto core = chatRoom->getCore()->getCCore();
		if (core && core->sal)
			core->sal->cancel_timer(timer);
		belle_sip_object_unref(timer);
		timer = nullptr;
	}
}

LINPHONE_END_NAMESPACE
