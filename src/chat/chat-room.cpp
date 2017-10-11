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

#include "linphone/api/c-chat-message.h"
#include "linphone/utils/utils.h"

#include "c-wrapper/c-wrapper.h"
#include "chat-room-p.h"
#include "imdn.h"
#include "content/content.h"
#include "chat-message-p.h"
#include "chat-room.h"
#include "sal/message-op.h"
#include "logger/logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ChatRoomPrivate::ChatRoomPrivate (LinphoneCore *core)
	: core(core), isComposingHandler(core, this) {}

ChatRoomPrivate::~ChatRoomPrivate () {}

// -----------------------------------------------------------------------------

int ChatRoomPrivate::createChatMessageFromDb (void *data, int argc, char **argv, char **colName) {
	ChatRoomPrivate *d = reinterpret_cast<ChatRoomPrivate *>(data);
	return d->createChatMessageFromDb(argc, argv, colName);
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::addTransientMessage (shared_ptr<ChatMessage> msg) {
	auto iter = find(transientMessages.begin(), transientMessages.end(), msg);
	if (iter == transientMessages.end())
		transientMessages.push_back(msg);
}

void ChatRoomPrivate::addWeakMessage (shared_ptr<ChatMessage> msg) {
	weak_ptr<ChatMessage> weakptr(msg);
	weakMessages.push_back(weakptr);
}

void ChatRoomPrivate::moveTransientMessageToWeakMessages (shared_ptr<ChatMessage> msg) {
	auto iter = find(transientMessages.begin(), transientMessages.end(), msg);
	if (iter != transientMessages.end()) {
		/* msg is not transient anymore, we can remove it from our transient list and unref it */
		addWeakMessage(msg);
		removeTransientMessage(msg);
	} else {
		/* msg has already been removed from the transient messages, do nothing */
	}
}

void ChatRoomPrivate::removeTransientMessage (shared_ptr<ChatMessage> msg) {
	auto iter = find(transientMessages.begin(), transientMessages.end(), msg);
	if (iter != transientMessages.end()) {
		transientMessages.erase(iter);
	}
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::release () {
	L_Q();
	isComposingHandler.stopTimers();

	for (auto &message : weakMessages) {
		try {
			shared_ptr<ChatMessage> msg(message);
			msg->cancelFileTransfer();
			msg->getPrivate()->setChatRoom(nullptr);
		} catch(const std::bad_weak_ptr& e) {}
	}
	for (auto &message : transientMessages) {
		message->cancelFileTransfer();
		message->getPrivate()->setChatRoom(nullptr);
	}

	core = nullptr;
	linphone_chat_room_unref(L_GET_C_BACK_PTR(q));
}

void ChatRoomPrivate::sendImdn (const string &payload, LinphoneReason reason) {
	L_Q();

	const char *identity = nullptr;
	LinphoneAddress *peer = linphone_address_new(peerAddress.asString().c_str());
	LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(core, peer);
	if (proxy)
		identity = linphone_address_as_string(linphone_proxy_config_get_identity_address(proxy));
	else
		identity = linphone_core_get_primary_contact(core);

	/* Sending out of call */
	SalMessageOp *op = new SalMessageOp(core->sal);
	linphone_configure_op(core, op, peer, nullptr, !!lp_config_get_int(core->config, "sip", "chat_msg_with_contact", 0));

	shared_ptr<ChatMessage> msg = q->createMessage();
	msg->setFromAddress(identity);
	msg->setToAddress(peerAddress.asString());

	Content content;
	content.setContentType("message/imdn+xml");
	content.setBody(payload);
	msg->addContent(content);

	/* Do not try to encrypt the notification when it is reporting an error (maybe it should be bypassed only for some reasons). */
	int retval = -1;
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(core);
	if (imee && (reason == LinphoneReasonNone)) {
		LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsOutgoingMessageCb cbProcessOutgoingMessage = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imeeCbs);
		if (cbProcessOutgoingMessage) {
			retval = cbProcessOutgoingMessage(imee, L_GET_C_BACK_PTR(q), L_GET_C_BACK_PTR(msg));
		}
	}

	if (retval <= 0) {
		op->send_message(identity, peerAddress.asString().c_str(), msg->getPrivate()->getContentType().asString().c_str(), msg->getPrivate()->getText().c_str(), nullptr);
	}

	linphone_address_unref(peer);
	op->unref();
}

// -----------------------------------------------------------------------------

int ChatRoomPrivate::getMessagesCount (bool unreadOnly) {
	if (!core->db) return 0;

	/* Optimization: do not read database if the count is already available in memory */
	if (unreadOnly && unreadCount >= 0) return unreadCount;

	string peer = peerAddress.asStringUriOnly();
	char *option = nullptr;
	if (unreadOnly)
		option = bctbx_strdup_printf("AND status!=%i AND direction=%i", LinphoneChatMessageStateDisplayed, LinphoneChatMessageIncoming);
	char *buf = sqlite3_mprintf("SELECT count(*) FROM history WHERE remoteContact = %Q %s;", peer.c_str(), unreadOnly ? option : "");
	sqlite3_stmt *selectStatement;
	int numrows = 0;
	int returnValue = sqlite3_prepare_v2(core->db, buf, -1, &selectStatement, nullptr);
	if (returnValue == SQLITE_OK) {
		if (sqlite3_step(selectStatement) == SQLITE_ROW) {
			numrows = sqlite3_column_int(selectStatement, 0);
		}
	}
	sqlite3_finalize(selectStatement);
	sqlite3_free(buf);

	/* No need to test the sign of unreadCount here because it has been tested above */
	if (unreadOnly) {
		unreadCount = numrows;
	}
	if (option) bctbx_free(option);
	return numrows;
}

void ChatRoomPrivate::setState (ChatRoom::State newState) {
	L_Q();
	if (newState != state) {
		state = newState;
		if (state == ChatRoom::State::Instantiated)
			linphone_core_notify_chat_room_instantiated(core, L_GET_C_BACK_PTR(q));
		notifyStateChanged();
	}
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::sendIsComposingNotification () {
	L_Q();
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(core);
	if (linphone_im_notif_policy_get_send_is_composing(policy)) {
		LinphoneAddress *peer = linphone_address_new(peerAddress.asString().c_str());
		LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(core, peer);
		const char *identity = nullptr;

		if (proxy)
			identity = linphone_address_as_string(linphone_proxy_config_get_identity_address(proxy));
		else
			identity = linphone_core_get_primary_contact(core);

		/* Sending out of call */
		SalMessageOp *op = new SalMessageOp(core->sal);
		linphone_configure_op(core, op, peer, nullptr, !!lp_config_get_int(core->config, "sip", "chat_msg_with_contact", 0));
		string payload = isComposingHandler.marshal(isComposing);
		if (!payload.empty()) {
			int retval = -1;

			shared_ptr<ChatMessage> msg = q->createMessage();
			msg->setFromAddress(identity);
			msg->setToAddress(peerAddress.asString());

			Content content;
			content.setContentType("application/im-iscomposing+xml");
			content.setBody(payload);
			msg->addContent(content);

			LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(core);
			if (imee) {
				LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsOutgoingMessageCb cbProcessOutgoingMessage = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imeeCbs);
				if (cbProcessOutgoingMessage) {
					retval = cbProcessOutgoingMessage(imee, L_GET_C_BACK_PTR(q), L_GET_C_BACK_PTR(msg));
				}
			}

			if (retval <= 0) {
				op->send_message(identity, peerAddress.asString().c_str(), msg->getPrivate()->getContentType().asString().c_str(), msg->getPrivate()->getText().c_str(), nullptr);
			}
			op->unref();
		}
		linphone_address_unref(peer);
	}
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
	L_Q();
	unsigned int storageId = (unsigned int)atoi(argv[0]);

	/* Check if the message exists in the weak messages list, in which case we should return that one. */
	shared_ptr<ChatMessage> message = getWeakMessage(storageId);
	if (!message) {
		/* Check if the message exists in the transient list, in which case we should return that one. */
		message = getTransientMessage(storageId);
	}
	if (!message) {
		message = q->createMessage();

		Content content;
		if (argv[4]) {
			content.setBody(argv[4]);
		}
		if (argv[13]) {
			content.setContentType(argv[13]);
		}
		message->addContent(content);

		Address peer(peerAddress.asString());
		if (atoi(argv[3]) == static_cast<int>(ChatMessage::Direction::Incoming)) {
			message->getPrivate()->setDirection(ChatMessage::Direction::Incoming);
			message->setFromAddress(peer);
		} else {
			message->getPrivate()->setDirection(ChatMessage::Direction::Outgoing);
			message->setToAddress(peer);
		}

		message->getPrivate()->setTime((time_t)atol(argv[9]));
		message->getPrivate()->setState((ChatMessage::State)atoi(argv[7]));
		message->getPrivate()->setStorageId(storageId);
		if (argv[8]) {
			message->setExternalBodyUrl(argv[8]);
		}
		if (argv[10]) {
			message->setAppdata(argv[10]);
		}
		if (argv[12]) {
			message->setId(argv[12]);
		}
		message->setIsSecured((bool)atoi(argv[14]));

		if (argv[11]) {
			int id = atoi(argv[11]);
			if (id >= 0)
				linphone_chat_message_fetch_content_from_database(core->db, L_GET_C_BACK_PTR(message), id);
		}

		/* Fix content type for old messages that were stored without it */
		/* To keep ?
		if (message->getPrivate()->getContentType().empty()) {
			if (message->getPrivate()->getFileTransferInformation()) {
				message->getPrivate()->setContentType("application/vnd.gsma.rcs-ft-http+xml");
			} else if (!message->getExternalBodyUrl().empty()) {
				message->getPrivate()->setContentType("message/external-body");
			} else {
				message->getPrivate()->setContentType("text/plain");
			}
		}*/

		/* Add the new message to the weak messages list. */
		addWeakMessage(message);
	}
	messages.push_front(message);
	return 0;
}

shared_ptr<ChatMessage> ChatRoomPrivate::getTransientMessage (unsigned int storageId) const {
	for (auto &message : transientMessages) {
		if (message->getPrivate()->getStorageId() == storageId)
			return message;
	}
	return nullptr;
}

std::shared_ptr<ChatMessage> ChatRoomPrivate::getWeakMessage (unsigned int storageId) const {
	for (auto &message : weakMessages) {
		try {
			shared_ptr<ChatMessage> msg(message);
			if (msg->getPrivate()->getStorageId() == storageId)
				return msg;
		} catch(const std::bad_weak_ptr& e) {}
	}
	return nullptr;
}

int ChatRoomPrivate::sqlRequest (sqlite3 *db, const string &stmt) {
	char *errmsg = nullptr;
	int ret = sqlite3_exec(db, stmt.c_str(), nullptr, nullptr, &errmsg);
	if (ret != SQLITE_OK) {
		lError() << "ChatRoomPrivate::sqlRequest: statement " << stmt << " -> error sqlite3_exec(): " << errmsg;
		sqlite3_free(errmsg);
	}
	return ret;
}

void ChatRoomPrivate::sqlRequestMessage (sqlite3 *db, const string &stmt) {
	char *errmsg = nullptr;
	int ret = sqlite3_exec(db, stmt.c_str(), createChatMessageFromDb, this, &errmsg);
	if (ret != SQLITE_OK) {
		lError() << "Error in creation: " << errmsg;
		sqlite3_free(errmsg);
	}
}

list<shared_ptr<ChatMessage> > ChatRoomPrivate::findMessages (const string &messageId) {
	if (!core->db)
		return list<shared_ptr<ChatMessage> >();
	string peer = peerAddress.asStringUriOnly();
	char *buf = sqlite3_mprintf("SELECT * FROM history WHERE remoteContact = %Q AND messageId = %Q", peer.c_str(), messageId.c_str());
	messages.clear();
	sqlRequestMessage(core->db, buf);
	sqlite3_free(buf);
	list<shared_ptr<ChatMessage> > result = messages;
	messages.clear();
	return result;
}

void ChatRoomPrivate::storeOrUpdateMessage (shared_ptr<ChatMessage> msg) {
	msg->store();
}

// -----------------------------------------------------------------------------

LinphoneReason ChatRoomPrivate::messageReceived (SalOp *op, const SalMessage *salMsg) {
	L_Q();

	bool increaseMsgCount = true;
	LinphoneReason reason = LinphoneReasonNone;
	shared_ptr<ChatMessage> msg;

	/* Check if this is a duplicate message */
	if ((msg = q->findMessageWithDirection(op->get_call_id(), ChatMessage::Direction::Incoming))) {
		reason = core->chat_deny_code;
		return reason;
	}

	msg = q->createMessage();

	Content content;
	content.setContentType(salMsg->content_type);
	content.setBody(salMsg->text ? salMsg->text : "");
	msg->setInternalContent(content);

	msg->setToAddress(op->get_to() ? op->get_to() : linphone_core_get_identity(core));
	msg->setFromAddress(peerAddress);
	msg->getPrivate()->setTime(salMsg->time);
	msg->getPrivate()->setState(ChatMessage::State::Delivered);
	msg->getPrivate()->setDirection(ChatMessage::Direction::Incoming);
	msg->setId(op->get_call_id());

	const SalCustomHeader *ch = op->get_recv_custom_header();
	if (ch)
		msg->getPrivate()->setSalCustomHeaders(sal_custom_header_clone(ch));
	if (salMsg->url)
		msg->setExternalBodyUrl(salMsg->url);

	reason = msg->getPrivate()->receive();

	if (reason == LinphoneReasonNotAcceptable || reason == LinphoneReasonUnknown) {
		/* Return LinphoneReasonNone to avoid flexisip resending us a message we can't decrypt */
		reason = LinphoneReasonNone;
		goto end;
	}

	if (msg->getPrivate()->getContentType() == ContentType::ImIsComposing) {
		isComposingReceived(msg->getPrivate()->getText());
		increaseMsgCount = FALSE;
		if (lp_config_get_int(core->config, "sip", "deliver_imdn", 0) != 1) {
			goto end;
		}
	} else if (msg->getPrivate()->getContentType() == ContentType::Imdn) {
		imdnReceived(msg->getPrivate()->getText());
		increaseMsgCount = FALSE;
		if (lp_config_get_int(core->config, "sip", "deliver_imdn", 0) != 1) {
			goto end;
		}
	}

	if (increaseMsgCount) {
		if (unreadCount < 0)
			unreadCount = 1;
		else
			unreadCount++;
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

void ChatRoomPrivate::chatMessageReceived (shared_ptr<ChatMessage> msg) {
	L_Q();
	if ((msg->getPrivate()->getContentType() != ContentType::Imdn) && (msg->getPrivate()->getContentType() != ContentType::ImIsComposing)) {
		notifyChatMessageReceived(msg);
		remoteIsComposing = false;
		linphone_core_notify_is_composing_received(core, L_GET_C_BACK_PTR(q));
		msg->sendDeliveryNotification(LinphoneReasonNone);
	}
}

void ChatRoomPrivate::imdnReceived (const string &text) {
	L_Q();
	Imdn::parse(*q, text);
}

void ChatRoomPrivate::isComposingReceived (const string &text) {
	isComposingHandler.parse(text);
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::notifyChatMessageReceived (shared_ptr<ChatMessage> msg) {
	L_Q();
	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(q);
	if (!msg->getPrivate()->getText().empty()) {
		/* Legacy API */
		linphone_core_notify_text_message_received(core, cr, L_GET_C_BACK_PTR(&msg->getFromAddress()), msg->getPrivate()->getText().c_str());
	}
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsMessageReceivedCb cb = linphone_chat_room_cbs_get_message_received(cbs);
	if (cb)
		cb(cr, L_GET_C_BACK_PTR(msg));
	linphone_core_notify_message_received(core, cr, L_GET_C_BACK_PTR(msg));
}

void ChatRoomPrivate::notifyStateChanged () {
	L_Q();
	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(q);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsStateChangedCb cb = linphone_chat_room_cbs_get_state_changed(cbs);
	if (cb)
		cb(cr, (LinphoneChatRoomState)state);
}

void ChatRoomPrivate::notifyUndecryptableMessageReceived (shared_ptr<ChatMessage> msg) {
	L_Q();
	LinphoneChatRoom *cr = L_GET_C_BACK_PTR(q);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsUndecryptableMessageReceivedCb cb = linphone_chat_room_cbs_get_undecryptable_message_received(cbs);
	if (cb)
		cb(cr, L_GET_C_BACK_PTR(msg));
	linphone_core_notify_message_received_unable_decrypt(core, cr, L_GET_C_BACK_PTR(msg));
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::onIsComposingStateChanged (bool isComposing) {
	this->isComposing = isComposing;
	sendIsComposingNotification();
}

void ChatRoomPrivate::onIsRemoteComposingStateChanged (bool isComposing) {
	L_Q();
	remoteIsComposing = isComposing;
	linphone_core_notify_is_composing_received(core, L_GET_C_BACK_PTR(q));
}

void ChatRoomPrivate::onIsComposingRefreshNeeded () {
	sendIsComposingNotification();
}

// =============================================================================

ChatRoom::ChatRoom (LinphoneCore *core) : Object(*new ChatRoomPrivate(core)) {}

ChatRoom::ChatRoom (ChatRoomPrivate &p) : Object(p) {}

// -----------------------------------------------------------------------------

void ChatRoom::compose () {
	L_D();
	if (!d->isComposing) {
		d->isComposing = true;
		d->sendIsComposingNotification();
		d->isComposingHandler.startRefreshTimer();
	}
	d->isComposingHandler.startIdleTimer();
}

shared_ptr<ChatMessage> ChatRoom::createFileTransferMessage (const LinphoneContent *initialContent) {
	L_D();
	shared_ptr<ChatMessage> chatMessage = createMessage();

	/* TODO
	Content content;
	content.setContentType(ContentType::FileTransfer);
	content.setBody(linphone_content_get_string_buffer(initialContent));
	chatMessage->addContent(content);*/

	chatMessage->setToAddress(d->peerAddress);
	chatMessage->setFromAddress(linphone_core_get_identity(d->core));
	chatMessage->getPrivate()->setDirection(ChatMessage::Direction::Outgoing);
	chatMessage->getPrivate()->setFileTransferInformation(linphone_content_copy(initialContent));

	return chatMessage;
}

shared_ptr<ChatMessage> ChatRoom::createMessage (const string &message) {
	L_D();
	shared_ptr<ChatMessage> chatMessage = createMessage();

	Content content;
	content.setContentType(ContentType::PlainText);
	content.setBody(message);
	chatMessage->addContent(content);

	chatMessage->setToAddress(d->peerAddress);
	chatMessage->setFromAddress(linphone_core_get_identity(d->core));

	return chatMessage;
}

shared_ptr<ChatMessage> ChatRoom::createMessage () {
	shared_ptr<ChatMessage> chatMessage = ObjectFactory::create<ChatMessage>(getSharedFromThis());
	chatMessage->getPrivate()->setTime(ms_time(0));
	return chatMessage;
}

void ChatRoom::deleteHistory () {
	L_D();
	if (!d->core->db) return;
	string peer = d->peerAddress.asStringUriOnly();
	char *buf = sqlite3_mprintf("DELETE FROM history WHERE remoteContact = %Q;", peer.c_str());
	d->sqlRequest(d->core->db, buf);
	sqlite3_free(buf);
	if (d->unreadCount > 0) d->unreadCount = 0;
}

void ChatRoom::deleteMessage (shared_ptr<ChatMessage> msg) {
	L_D();
	if (!d->core->db) return;
	char *buf = sqlite3_mprintf("DELETE FROM history WHERE id = %u;", msg->getPrivate()->getStorageId());
	d->sqlRequest(d->core->db, buf);
	sqlite3_free(buf);

	/* Invalidate unread_count when we modify the database, so that next
		 time we need it it will be recomputed from latest database state */
	d->unreadCount = -1;
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
	L_D();
	return d->getMessagesCount(false);
}

list<shared_ptr<ChatMessage> > ChatRoom::getHistoryRange (int startm, int endm) {
	L_D();
	if (!d->core->db) return list<shared_ptr<ChatMessage> >();
	string peer = d->peerAddress.asStringUriOnly();
	d->messages.clear();

	/* Since we want to append query parameters depending on arguments given, we use malloc instead of sqlite3_mprintf */
	const int bufMaxSize = 512;
	char *buf = reinterpret_cast<char *>(ms_malloc(bufMaxSize));
	buf = sqlite3_snprintf(bufMaxSize - 1, buf, "SELECT * FROM history WHERE remoteContact = %Q ORDER BY id DESC", peer.c_str());

	if (startm < 0) startm = 0;
	if (((endm > 0) && (endm >= startm)) || ((startm == 0) && (endm == 0))) {
		char *buf2 = ms_strdup_printf("%s LIMIT %i ", buf, endm + 1 - startm);
		ms_free(buf);
		buf = buf2;
	} else if (startm > 0) {
		lInfo() << __FUNCTION__ << "(): end is lower than start (" << endm << " < " << startm << "). Assuming no end limit.";
		char *buf2 = ms_strdup_printf("%s LIMIT -1", buf);
		ms_free(buf);
		buf = buf2;
	}
	if (startm > 0) {
		char *buf2 = ms_strdup_printf("%s OFFSET %i ", buf, startm);
		ms_free(buf);
		buf = buf2;
	}

	uint64_t begin = ortp_get_cur_time_ms();
	d->sqlRequestMessage(d->core->db, buf);
	uint64_t end = ortp_get_cur_time_ms();

	if ((endm + 1 - startm) > 1) {
		/* Display message only if at least 2 messages are loaded */
		lInfo() << __FUNCTION__ << "(): completed in " << (int)(end - begin) << " ms";
	}
	ms_free(buf);

	if (!d->messages.empty()) {
		/* Fill local addr with core identity instead of per message */
		for (auto &message : d->messages) {
			if (message->isOutgoing()) {
				message->setFromAddress(linphone_core_get_identity(d->core));
			} else {
				message->setToAddress(linphone_core_get_identity(d->core));
			}
		}
	}

	list<shared_ptr<ChatMessage> > result = d->messages;
	d->messages.clear();
	return result;
}

int ChatRoom::getUnreadMessagesCount () {
	L_D();
	return d->getMessagesCount(true);
}

bool ChatRoom::isRemoteComposing () const {
	L_D();
	return d->remoteIsComposing;
}

void ChatRoom::markAsRead () {
	L_D();

	if (!d->core->db) return;

	/* Optimization: do not modify the database if no message is marked as unread */
	if (getUnreadMessagesCount() == 0) return;

	string peer = d->peerAddress.asStringUriOnly();
	char *buf = sqlite3_mprintf("SELECT * FROM history WHERE remoteContact = %Q AND direction = %i AND status != %i", peer.c_str(), ChatMessage::Direction::Incoming, ChatMessage::State::Displayed);
	d->sqlRequestMessage(d->core->db, buf);
	sqlite3_free(buf);
	for (auto &message : d->messages) {
		message->sendDisplayNotification();
	}
	d->messages.clear();
	buf = sqlite3_mprintf("UPDATE history SET status=%i WHERE remoteContact=%Q AND direction=%i;", ChatMessage::State::Displayed, peer.c_str(), ChatMessage::Direction::Incoming);
	d->sqlRequest(d->core->db, buf);
	sqlite3_free(buf);

	if (d->pendingMessage) {
		d->pendingMessage->updateState(ChatMessage::State::Displayed);
		d->pendingMessage->sendDisplayNotification();
	}

	d->unreadCount = 0;
}

void ChatRoom::sendMessage (shared_ptr<ChatMessage> msg) {
	L_D();

	msg->getPrivate()->setDirection(ChatMessage::Direction::Outgoing);

	/* Add to transient list */
	d->addTransientMessage(msg);

	msg->getPrivate()->setTime(ms_time(0));
	msg->getPrivate()->send();

	d->storeOrUpdateMessage(msg);

	if (d->isComposing)
		d->isComposing = false;
	d->isComposingHandler.stopIdleTimer();
	d->isComposingHandler.stopRefreshTimer();
}

// -----------------------------------------------------------------------------

LinphoneCore *ChatRoom::getCore () const {
	L_D();
	return d->core;
}

// -----------------------------------------------------------------------------

const Address& ChatRoom::getPeerAddress () const {
	L_D();
	return d->peerAddress;
}

ChatRoom::State ChatRoom::getState () const {
	L_D();
	return d->state;
}

LINPHONE_END_NAMESPACE
