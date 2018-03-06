/*
 * chat-room.cpp
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

#include "linphone/utils/utils.h"

#include "c-wrapper/c-wrapper.h"
#include "chat/chat-message/chat-message-p.h"
#include "chat/chat-room/chat-room-p.h"
#include "core/core-p.h"
#include "sip-tools/sip-headers.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

void ChatRoomPrivate::setState (ChatRoom::State state) {
	if (this->state != state) {
		this->state = state;
		notifyStateChanged();
	}
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::sendChatMessage (const shared_ptr<ChatMessage> &chatMessage) {
	L_Q();

	ChatMessagePrivate *dChatMessage = chatMessage->getPrivate();
	dChatMessage->setTime(ms_time(0));
	dChatMessage->send();

	LinphoneChatRoom *cr = getCChatRoom();
	// TODO: server currently don't stock message, remove condition in the future.
	if (!linphone_core_conference_server_enabled(q->getCore()->getCCore())) {
		shared_ptr<ConferenceChatMessageEvent> event = static_pointer_cast<ConferenceChatMessageEvent>(
			q->getCore()->getPrivate()->mainDb->getEventFromKey(dChatMessage->dbKey)
		);
		if (!event)
			event = make_shared<ConferenceChatMessageEvent>(time(nullptr), chatMessage);

		_linphone_chat_room_notify_chat_message_sent(cr, L_GET_C_BACK_PTR(event));
	}

	if (isComposing)
		isComposing = false;
	isComposingHandler->stopIdleTimer();
	isComposingHandler->stopRefreshTimer();
}

void ChatRoomPrivate::sendIsComposingNotification () {
	L_Q();
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(q->getCore()->getCCore());
	if (!linphone_im_notif_policy_get_send_is_composing(policy))
		return;

	string payload = isComposingHandler->marshal(isComposing);
	if (payload.empty())
		return;

	Content *content = new Content();
	content->setContentType(ContentType::ImIsComposing);
	content->setBody(payload);

	shared_ptr<ChatMessage> chatMessage = createChatMessage(ChatMessage::Direction::Outgoing);
	chatMessage->setToBeStored(false);
	chatMessage->addContent(*content);
	chatMessage->getPrivate()->addSalCustomHeader(PriorityHeader::HeaderName, PriorityHeader::NonUrgent);
	chatMessage->getPrivate()->addSalCustomHeader("Expires", "0");

	chatMessage->getPrivate()->send();
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::addEvent (const shared_ptr<EventLog> &eventLog) {
	L_Q();

	q->getCore()->getPrivate()->mainDb->addEvent(eventLog);
	setLastUpdateTime(eventLog->getCreationTime());
}

void ChatRoomPrivate::addTransientEvent (const shared_ptr<EventLog> &eventLog) {
	auto it = find(transientEvents.begin(), transientEvents.end(), eventLog);
	if (it == transientEvents.end())
		transientEvents.push_back(eventLog);
}

void ChatRoomPrivate::removeTransientEvent (const shared_ptr<EventLog> &eventLog) {
	auto it = find(transientEvents.begin(), transientEvents.end(), eventLog);
	if (it != transientEvents.end())
		transientEvents.erase(it);
}

// -----------------------------------------------------------------------------

shared_ptr<ChatMessage> ChatRoomPrivate::createChatMessage (ChatMessage::Direction direction) {
	L_Q();
	return shared_ptr<ChatMessage>(new ChatMessage(q->getSharedFromThis(), direction));
}

list<shared_ptr<ChatMessage>> ChatRoomPrivate::findChatMessages (const string &messageId) const {
	L_Q();
	return q->getCore()->getPrivate()->mainDb->findChatMessages(q->getChatRoomId(), messageId);
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::notifyChatMessageReceived (const shared_ptr<ChatMessage> &chatMessage) {
	L_Q();
	LinphoneChatRoom *cr = getCChatRoom();
	if (!chatMessage->getPrivate()->getText().empty()) {
		/* Legacy API */
		LinphoneAddress *fromAddress = linphone_address_new(chatMessage->getFromAddress().asString().c_str());
		linphone_core_notify_text_message_received(
			q->getCore()->getCCore(),
			cr,
			fromAddress,
			chatMessage->getPrivate()->getText().c_str()
		);
		linphone_address_unref(fromAddress);
	}
	_linphone_chat_room_notify_message_received(cr, L_GET_C_BACK_PTR(chatMessage));
	linphone_core_notify_message_received(q->getCore()->getCCore(), cr, L_GET_C_BACK_PTR(chatMessage));
}

void ChatRoomPrivate::notifyIsComposingReceived (const Address &remoteAddress, bool isComposing) {
	L_Q();

	if (isComposing)
		remoteIsComposing.push_back(remoteAddress);
	else
		remoteIsComposing.remove(remoteAddress);

	LinphoneChatRoom *cr = getCChatRoom();
	LinphoneAddress *lAddr = linphone_address_new(remoteAddress.asString().c_str());
	_linphone_chat_room_notify_is_composing_received(cr, lAddr, !!isComposing);
	linphone_address_unref(lAddr);
	// Legacy notification
	linphone_core_notify_is_composing_received(q->getCore()->getCCore(), cr);
}

void ChatRoomPrivate::notifyStateChanged () {
	L_Q();
	LinphoneChatRoom *cr = getCChatRoom();
	linphone_core_notify_chat_room_state_changed(q->getCore()->getCCore(), cr, (LinphoneChatRoomState)state);
	_linphone_chat_room_notify_state_changed(cr, (LinphoneChatRoomState)state);
}

void ChatRoomPrivate::notifyUndecryptableChatMessageReceived (const shared_ptr<ChatMessage> &chatMessage) {
	L_Q();
	LinphoneChatRoom *cr = getCChatRoom();
	_linphone_chat_room_notify_undecryptable_message_received(cr, L_GET_C_BACK_PTR(chatMessage));
	linphone_core_notify_message_received_unable_decrypt(q->getCore()->getCCore(), cr, L_GET_C_BACK_PTR(chatMessage));
}

// -----------------------------------------------------------------------------

LinphoneReason ChatRoomPrivate::onSipMessageReceived (SalOp *op, const SalMessage *message) {
	L_Q();

	LinphoneReason reason = LinphoneReasonNone;
	shared_ptr<ChatMessage> msg;

	shared_ptr<Core> core = q->getCore();
	LinphoneCore *cCore = core->getCCore();

	msg = createChatMessage(
		IdentityAddress(op->get_from()) == q->getLocalAddress()
			? ChatMessage::Direction::Outgoing
			: ChatMessage::Direction::Incoming
	);

	Content content;
	content.setContentType(message->content_type);
	content.setBodyFromUtf8(message->text ? message->text : "");
	msg->setInternalContent(content);

	msg->getPrivate()->setTime(message->time);
	msg->getPrivate()->setImdnMessageId(op->get_call_id());

	const SalCustomHeader *ch = op->get_recv_custom_header();
	if (ch)
		msg->getPrivate()->setSalCustomHeaders(sal_custom_header_clone(ch));

	reason = msg->getPrivate()->receive();

	if (reason == LinphoneReasonNotAcceptable || reason == LinphoneReasonUnknown) {
		/* Return LinphoneReasonNone to avoid flexisip resending us a message we can't decrypt */
		reason = LinphoneReasonNone;
		goto end;
	}

	if (msg->getPrivate()->getContentType() == ContentType::ImIsComposing) {
		onIsComposingReceived(msg->getFromAddress(), msg->getPrivate()->getText());
		if (lp_config_get_int(linphone_core_get_config(cCore), "sip", "deliver_imdn", 0) != 1) {
			goto end;
		}
	} else if (msg->getPrivate()->getContentType() == ContentType::Imdn) {
		onImdnReceived(msg);
		if (lp_config_get_int(linphone_core_get_config(cCore), "sip", "deliver_imdn", 0) != 1) {
			goto end;
		}
	}

	onChatMessageReceived(msg);

end:
	return reason;
}

void ChatRoomPrivate::onChatMessageReceived (const shared_ptr<ChatMessage> &chatMessage) {
	const IdentityAddress &fromAddress = chatMessage->getFromAddress();
	if ((chatMessage->getPrivate()->getContentType() != ContentType::ImIsComposing)
		&& (chatMessage->getPrivate()->getContentType() != ContentType::Imdn)
	) {
		isComposingHandler->stopRemoteRefreshTimer(fromAddress.asString());
		notifyIsComposingReceived(fromAddress, false);
	}
	chatMessage->getPrivate()->notifyReceiving();
}

void ChatRoomPrivate::onImdnReceived (const shared_ptr<ChatMessage> &chatMessage) {
	Imdn::parse(chatMessage);
}

void ChatRoomPrivate::onIsComposingReceived (const Address &remoteAddress, const string &text) {
	isComposingHandler->parse(remoteAddress, text);
}

void ChatRoomPrivate::onIsComposingRefreshNeeded () {
	sendIsComposingNotification();
}

void ChatRoomPrivate::onIsComposingStateChanged (bool isComposing) {
	this->isComposing = isComposing;
	sendIsComposingNotification();
}

void ChatRoomPrivate::onIsRemoteComposingStateChanged (const Address &remoteAddress, bool isComposing) {
	notifyIsComposingReceived(remoteAddress, isComposing);
}

// -----------------------------------------------------------------------------

LinphoneChatRoom *ChatRoomPrivate::getCChatRoom () const {
	L_Q();
	if (proxyChatRoom)
		return L_GET_C_BACK_PTR(proxyChatRoom);
	else
		return L_GET_C_BACK_PTR(q);
}

// =============================================================================

ChatRoom::ChatRoom (ChatRoomPrivate &p, const shared_ptr<Core> &core, const ChatRoomId &chatRoomId) :
	AbstractChatRoom(p, core) {
	L_D();

	d->chatRoomId = chatRoomId;
	d->isComposingHandler.reset(new IsComposing(core->getCCore(), d));
}

// -----------------------------------------------------------------------------

const ChatRoomId &ChatRoom::getChatRoomId () const {
	L_D();
	return d->chatRoomId;
}

const IdentityAddress &ChatRoom::getPeerAddress () const {
	L_D();
	return d->chatRoomId.getPeerAddress();
}

const IdentityAddress &ChatRoom::getLocalAddress () const {
	L_D();
	return d->chatRoomId.getLocalAddress();
}

// -----------------------------------------------------------------------------

time_t ChatRoom::getCreationTime () const {
	L_D();
	return d->creationTime;
}

time_t ChatRoom::getLastUpdateTime () const {
	L_D();
	return d->lastUpdateTime;
}

// -----------------------------------------------------------------------------

ChatRoom::State ChatRoom::getState () const {
	L_D();
	return d->state;
}

// -----------------------------------------------------------------------------

list<shared_ptr<EventLog>> ChatRoom::getMessageHistory (int nLast) const {
	return getCore()->getPrivate()->mainDb->getHistory(getChatRoomId(), nLast, MainDb::Filter::ConferenceChatMessageFilter);
}

list<shared_ptr<EventLog>> ChatRoom::getMessageHistoryRange (int begin, int end) const {
	return getCore()->getPrivate()->mainDb->getHistoryRange(getChatRoomId(), begin, end, MainDb::Filter::ConferenceChatMessageFilter);
}

list<shared_ptr<EventLog>> ChatRoom::getHistory (int nLast) const {
	return getCore()->getPrivate()->mainDb->getHistory(
		getChatRoomId(),
		nLast,
		MainDb::FilterMask({ MainDb::Filter::ConferenceChatMessageFilter, MainDb::Filter::ConferenceInfoNoDeviceFilter })
	);
}

list<shared_ptr<EventLog>> ChatRoom::getHistoryRange (int begin, int end) const {
	return getCore()->getPrivate()->mainDb->getHistoryRange(
		getChatRoomId(),
		begin,
		end,
		MainDb::FilterMask({ MainDb::Filter::ConferenceChatMessageFilter, MainDb::Filter::ConferenceInfoNoDeviceFilter })
	);
}

int ChatRoom::getHistorySize () const {
	return getCore()->getPrivate()->mainDb->getHistorySize(getChatRoomId());
}

void ChatRoom::deleteFromDb () {
	L_D();
	// Keep a ref, otherwise the object might be destroyed before we can set the Deleted state
	shared_ptr<AbstractChatRoom> ref = this->getSharedFromThis();
	Core::deleteChatRoom(ref);
	d->setState(ChatRoom::State::Deleted);
}

void ChatRoom::deleteHistory () {
	getCore()->getPrivate()->mainDb->cleanHistory(getChatRoomId());
}

shared_ptr<ChatMessage> ChatRoom::getLastChatMessageInHistory () const {
	return getCore()->getPrivate()->mainDb->getLastChatMessage(getChatRoomId());
}

int ChatRoom::getChatMessageCount () const {
	return getCore()->getPrivate()->mainDb->getChatMessageCount(getChatRoomId());
}

int ChatRoom::getUnreadChatMessageCount () const {
	return getCore()->getPrivate()->mainDb->getUnreadChatMessageCount(getChatRoomId());
}

// -----------------------------------------------------------------------------

void ChatRoom::compose () {
	L_D();
	if (!d->isComposing) {
		d->isComposing = true;
		d->sendIsComposingNotification();
		d->isComposingHandler->startRefreshTimer();
	}
	d->isComposingHandler->startIdleTimer();
}

bool ChatRoom::isRemoteComposing () const {
	L_D();
	return !d->remoteIsComposing.empty();
}

list<IdentityAddress> ChatRoom::getComposingAddresses () const {
	L_D();
	return d->remoteIsComposing;
}

// -----------------------------------------------------------------------------

shared_ptr<ChatMessage> ChatRoom::createChatMessage () {
	L_D();
	return d->createChatMessage(ChatMessage::Direction::Outgoing);
}

shared_ptr<ChatMessage> ChatRoom::createChatMessage (const string &text) {
	shared_ptr<ChatMessage> chatMessage = createChatMessage();
	Content *content = new Content();
	content->setContentType(ContentType::PlainText);
	content->setBody(text);
	chatMessage->addContent(*content);
	return chatMessage;
}

shared_ptr<ChatMessage> ChatRoom::createFileTransferMessage (const LinphoneContent *initialContent) {
	shared_ptr<ChatMessage> chatMessage = createChatMessage();
	chatMessage->getPrivate()->setFileTransferInformation(initialContent);
	return chatMessage;
}

// -----------------------------------------------------------------------------

shared_ptr<ChatMessage> ChatRoom::findChatMessage (const string &messageId) const {
	L_D();
	list<shared_ptr<ChatMessage>> chatMessages = d->findChatMessages(messageId);
	return chatMessages.empty() ? nullptr : chatMessages.front();
}

shared_ptr<ChatMessage> ChatRoom::findChatMessage (const string &messageId, ChatMessage::Direction direction) const {
	L_D();
	for (auto &chatMessage : d->findChatMessages(messageId))
		if (chatMessage->getDirection() == direction)
			return chatMessage;
	return nullptr;
}

void ChatRoom::markAsRead () {
	L_D();

	CorePrivate *dCore = getCore()->getPrivate();
	for (auto &chatMessage : dCore->mainDb->getUnreadChatMessages(d->chatRoomId)) {
		chatMessage->sendDisplayNotification();
		chatMessage->getPrivate()->setState(ChatMessage::State::Displayed, true);
	}

	dCore->mainDb->markChatMessagesAsRead(d->chatRoomId);
}

LINPHONE_END_NAMESPACE
