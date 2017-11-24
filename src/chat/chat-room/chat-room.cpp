/*
 * chat-room.cpp
 * Copyright (C) 2010-2017 Belledonne Communications SARL
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

#include "c-wrapper/c-wrapper.h"
#include "event-log/conference/conference-chat-message-event.h"
#include "chat/chat-message/chat-message-p.h"
#include "chat/chat-room/chat-room-p.h"
#include "chat/notification/imdn.h"
#include "logger/logger.h"
#include "core/core-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

int ChatRoomPrivate::createChatMessageFromDb (void *data, int argc, char **argv, char **colName) {
	ChatRoomPrivate *d = reinterpret_cast<ChatRoomPrivate *>(data);
	return d->createChatMessageFromDb(argc, argv, colName);
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::addTransientEvent (const shared_ptr<EventLog> &log) {
	auto iter = find(transientEvents.begin(), transientEvents.end(), log);
	if (iter == transientEvents.end())
		transientEvents.push_back(log);
}

void ChatRoomPrivate::removeTransientEvent (const shared_ptr<EventLog> &log) {
	auto iter = find(transientEvents.begin(), transientEvents.end(), log);
	if (iter != transientEvents.end()) {
		transientEvents.erase(iter);
	}
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::release () {
	isComposingHandler->stopTimers();
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::setState (ChatRoom::State newState) {
	L_Q();
	if (newState != state) {
		state = newState;
		if (state == ChatRoom::State::Instantiated)
			linphone_core_notify_chat_room_instantiated(q->getCore()->getCCore(), L_GET_C_BACK_PTR(q));
		notifyStateChanged();
	}
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::sendIsComposingNotification () {
	L_Q();
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(q->getCore()->getCCore());
	if (linphone_im_notif_policy_get_send_is_composing(policy)) {
		string payload = isComposingHandler->marshal(isComposing);
		if (!payload.empty()) {
			shared_ptr<ChatMessage> msg = createChatMessage(ChatMessage::Direction::Outgoing);
			Content *content = new Content();
			content->setContentType(ContentType::ImIsComposing);
			content->setBody(payload);
			msg->addContent(*content);
			msg->getPrivate()->send();
		}
	}
}

// -----------------------------------------------------------------------------

shared_ptr<ChatMessage> ChatRoomPrivate::createChatMessage (ChatMessage::Direction direction) {
	L_Q();
	return shared_ptr<ChatMessage>(new ChatMessage(q->getSharedFromThis(), direction));
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

/**
 * DB layout:
 *
 * | 0  | storage_id
 * | 1  | localContact
 * | 2  | remoteContact
 * | 3  | direction flag (LinphoneChatMessageDir)
 * | 4  | message (text content of the message)
 * | 5  | time (unused now, used to be string-based timestamp, replaced by the utc timestamp)
 * | 6  | read flag (no longer used, replaced by the LinphoneChatMessageStateDisplayed state)
 * | 7  | status (LinphoneChatMessageState)
 * | 8  | external body url (deprecated file transfer system)
 * | 9  | utc timestamp
 * | 10 | app data text
 * | 11 | linphone content id (LinphoneContent describing a file transfer)
 * | 12 | message id (used for IMDN)
 * | 13 | content type (of the message field [must be text representable])
 * | 14 | secured flag
 */
int ChatRoomPrivate::createChatMessageFromDb (int argc, char **argv, char **colName) {
	// TODO: history.
	return 0;
}

list<shared_ptr<ChatMessage> > ChatRoomPrivate::findMessages (const string &messageId) {
	// TODO: history.
	return list<shared_ptr<ChatMessage>>();
}

void ChatRoomPrivate::sendMessage (const shared_ptr<ChatMessage> &msg) {
	L_Q();

	// TODO: Check direction.

	msg->getPrivate()->setTime(ms_time(0));
	msg->getPrivate()->send();

	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(q);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsParticipantAddedCb cb = linphone_chat_room_cbs_get_chat_message_sent(cbs);
	shared_ptr<ConferenceChatMessageEvent> event = make_shared<ConferenceChatMessageEvent>(msg->getTime(), msg);
	if (cb) {
		cb(cr, L_GET_C_BACK_PTR(event));
	}

	if (isComposing)
		isComposing = false;
	isComposingHandler->stopIdleTimer();
	isComposingHandler->stopRefreshTimer();
}

// -----------------------------------------------------------------------------

LinphoneReason ChatRoomPrivate::messageReceived (SalOp *op, const SalMessage *salMsg) {
	L_Q();

	bool increaseMsgCount = true;
	LinphoneReason reason = LinphoneReasonNone;
	shared_ptr<ChatMessage> msg;

	shared_ptr<Core> core = q->getCore();
	if (!core)
		return reason;
	LinphoneCore *cCore = core->getCCore();

	msg = createChatMessage(
		IdentityAddress(op->get_from()) == q->getLocalAddress()
			? ChatMessage::Direction::Outgoing
			: ChatMessage::Direction::Incoming
	);

	Content content;
	content.setContentType(salMsg->content_type);
	content.setBody(salMsg->text ? salMsg->text : "");
	msg->setInternalContent(content);

	msg->getPrivate()->setTime(salMsg->time);
	msg->getPrivate()->setState(ChatMessage::State::Delivered);
	msg->setImdnMessageId(op->get_call_id());

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
		isComposingReceived(msg->getFromAddress(), msg->getPrivate()->getText());
		increaseMsgCount = FALSE;
		if (lp_config_get_int(cCore->config, "sip", "deliver_imdn", 0) != 1) {
			goto end;
		}
	} else if (msg->getPrivate()->getContentType() == ContentType::Imdn) {
		imdnReceived(msg->getPrivate()->getText());
		increaseMsgCount = FALSE;
		if (lp_config_get_int(cCore->config, "sip", "deliver_imdn", 0) != 1) {
			goto end;
		}
	}

	if (increaseMsgCount) {
		/* Mark the message as pending so that if ChatRoom::markAsRead() is called in the
		 * ChatRoomPrivate::chatMessageReceived() callback, it will effectively be marked as
		 * being read before being stored. */
		pendingMessage = msg;
	}

	chatMessageReceived(msg);

	pendingMessage = nullptr;

end:
	return reason;
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::chatMessageReceived (const shared_ptr<ChatMessage> &msg) {
	L_Q();

	if ((msg->getPrivate()->getContentType() != ContentType::Imdn) && (msg->getPrivate()->getContentType() != ContentType::ImIsComposing)) {
		onChatMessageReceived(msg);

		LinphoneChatRoom *cr = L_GET_C_BACK_PTR(q);
		LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
		LinphoneChatRoomCbsParticipantAddedCb cb = linphone_chat_room_cbs_get_chat_message_received(cbs);
		shared_ptr<ConferenceChatMessageEvent> event = make_shared<ConferenceChatMessageEvent>(msg->getTime(), msg);
		if (cb) {
			cb(cr, L_GET_C_BACK_PTR(event));
		}
		// Legacy
		notifyChatMessageReceived(msg);

		const string fromAddress = msg->getFromAddress().asString();
		isComposingHandler->stopRemoteRefreshTimer(fromAddress);
		notifyIsComposingReceived(msg->getFromAddress(), false);
		msg->sendDeliveryNotification(LinphoneReasonNone);
	}
}

void ChatRoomPrivate::imdnReceived (const string &text) {
	L_Q();
	Imdn::parse(*q, text);
}

void ChatRoomPrivate::isComposingReceived (const Address &remoteAddr, const string &text) {
	isComposingHandler->parse(remoteAddr, text);
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::notifyChatMessageReceived (const shared_ptr<ChatMessage> &msg) {
	L_Q();
	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(q);
	if (!msg->getPrivate()->getText().empty()) {
		/* Legacy API */
		LinphoneAddress *fromAddress = linphone_address_new(msg->getFromAddress().asString().c_str());
		linphone_core_notify_text_message_received(
			q->getCore()->getCCore(),
			cr,
			fromAddress,
			msg->getPrivate()->getText().c_str()
		);
		linphone_address_unref(fromAddress);
	}
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsMessageReceivedCb cb = linphone_chat_room_cbs_get_message_received(cbs);
	if (cb)
		cb(cr, L_GET_C_BACK_PTR(msg));
	linphone_core_notify_message_received(q->getCore()->getCCore(), cr, L_GET_C_BACK_PTR(msg));
}

void ChatRoomPrivate::notifyIsComposingReceived (const Address &remoteAddr, bool isComposing) {
	L_Q();

	if (isComposing)
		remoteIsComposing.push_back(remoteAddr);
	else
		remoteIsComposing.remove(remoteAddr);

	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(q);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsIsComposingReceivedCb cb = linphone_chat_room_cbs_get_is_composing_received(cbs);
	if (cb) {
		LinphoneAddress *lAddr = linphone_address_new(remoteAddr.asString().c_str());
		cb(cr, lAddr, !!isComposing);
		linphone_address_unref(lAddr);
	}
	// Legacy notification
	linphone_core_notify_is_composing_received(q->getCore()->getCCore(), cr);
}

void ChatRoomPrivate::notifyStateChanged () {
	L_Q();
	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(q);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsStateChangedCb cb = linphone_chat_room_cbs_get_state_changed(cbs);
	if (cb)
		cb(cr, (LinphoneChatRoomState)state);
}

void ChatRoomPrivate::notifyUndecryptableMessageReceived (const shared_ptr<ChatMessage> &msg) {
	L_Q();
	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(q);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsUndecryptableMessageReceivedCb cb = linphone_chat_room_cbs_get_undecryptable_message_received(cbs);
	if (cb)
		cb(cr, L_GET_C_BACK_PTR(msg));
	linphone_core_notify_message_received_unable_decrypt(q->getCore()->getCCore(), cr, L_GET_C_BACK_PTR(msg));
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::onIsComposingStateChanged (bool isComposing) {
	this->isComposing = isComposing;
	sendIsComposingNotification();
}

void ChatRoomPrivate::onIsRemoteComposingStateChanged (const Address &remoteAddr, bool isComposing) {
	notifyIsComposingReceived(remoteAddr, isComposing);
}

void ChatRoomPrivate::onIsComposingRefreshNeeded () {
	sendIsComposingNotification();
}

// =============================================================================

ChatRoom::ChatRoom (ChatRoomPrivate &p, const shared_ptr<Core> &core, const ChatRoomId &chatRoomId) :
	Object(p), CoreAccessor(core) {
	L_D();

	d->chatRoomId = chatRoomId;
	d->isComposingHandler.reset(new IsComposing(core->getCCore(), d));
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

shared_ptr<ChatMessage> ChatRoom::createFileTransferMessage (const LinphoneContent *initialContent) {
	shared_ptr<ChatMessage> chatMessage = createMessage();
	chatMessage->getPrivate()->setFileTransferInformation(initialContent);
	return chatMessage;
}

shared_ptr<ChatMessage> ChatRoom::createMessage (const string &message) {
	shared_ptr<ChatMessage> chatMessage = createMessage();
	Content *content = new Content();
	content->setContentType(ContentType::PlainText);
	content->setBody(message);
	chatMessage->addContent(*content);
	return chatMessage;
}

shared_ptr<ChatMessage> ChatRoom::createMessage () {
	L_D();
	return d->createChatMessage(ChatMessage::Direction::Outgoing);
}

void ChatRoom::deleteHistory () {
	// TODO: history.
}

void ChatRoom::deleteMessage (const shared_ptr<ChatMessage> &msg) {
	// TODO: history.
}

shared_ptr<ChatMessage> ChatRoom::findMessage (const string &messageId) {
	L_D();
	shared_ptr<ChatMessage> cm = nullptr;
	list<shared_ptr<ChatMessage> > l = d->findMessages(messageId);
	if (!l.empty()) {
		cm = l.front();
	}
	return cm;
}

shared_ptr<ChatMessage> ChatRoom::findMessageWithDirection (const string &messageId, ChatMessage::Direction direction) {
	L_D();
	shared_ptr<ChatMessage> ret = nullptr;
	list<shared_ptr<ChatMessage> > l = d->findMessages(messageId);
	for (auto &message : l) {
		if (message->getDirection() == direction) {
			ret = message;
			break;
		}
	}
	return ret;
}

list<shared_ptr<ChatMessage> > ChatRoom::getHistory (int nbMessages) {
	return getHistoryRange(0, nbMessages - 1);
}

int ChatRoom::getHistorySize () {
	return getCore()->getPrivate()->mainDb->getChatMessagesCount(getChatRoomId());
}

list<shared_ptr<ChatMessage> > ChatRoom::getHistoryRange (int startm, int endm) {
	// TODO: history.
	return list<shared_ptr<ChatMessage>>();
}

int ChatRoom::getUnreadChatMessagesCount () {
	return getCore()->getPrivate()->mainDb->getUnreadChatMessagesCount(getChatRoomId());
}

bool ChatRoom::isRemoteComposing () const {
	L_D();
	return d->remoteIsComposing.size() > 0;
}

std::list<Address> ChatRoom::getComposingAddresses () const {
	L_D();
	return d->remoteIsComposing;
}

void ChatRoom::markAsRead () {
	L_D();

	if (getUnreadChatMessagesCount() == 0)
		return;

	shared_ptr<Core> core = getCore();
	if (!core)
		return;

	CorePrivate *dCore = core->getPrivate();
	const string peerAddress = getPeerAddress().asString();
	list<shared_ptr<ChatMessage>> chatMessages = dCore->mainDb->getUnreadChatMessages(d->chatRoomId);

	for (auto &chatMessage : chatMessages)
		chatMessage->sendDisplayNotification();

	dCore->mainDb->markChatMessagesAsRead(d->chatRoomId);

	if (d->pendingMessage) {
		d->pendingMessage->updateState(ChatMessage::State::Displayed);
		d->pendingMessage->sendDisplayNotification();
	}
}

// -----------------------------------------------------------------------------

ChatRoom::State ChatRoom::getState () const {
	L_D();
	return d->state;
}

LINPHONE_END_NAMESPACE
