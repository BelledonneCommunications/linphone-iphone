/*
 * chat-room.cpp
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

#include <algorithm>

#include "linphone/utils/utils.h"

#include "c-wrapper/c-tools.h"
#include "chat-room-p.h"
#include "content/content-type.h"
#include "imdn.h"
#include "logger/logger.h"

#include "chat-room.h"

extern LinphoneChatRoom * _linphone_chat_room_init();
#define GET_BACK_PTR(object) L_GET_C_BACK_PTR(object->shared_from_this(), ChatRoom, chat_room)

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ChatRoomPrivate::ChatRoomPrivate (LinphoneCore *core)
	: core(core), isComposingHandler(core, this) {}

ChatRoomPrivate::~ChatRoomPrivate () {
	for (auto it = transientMessages.begin(); it != transientMessages.end(); it++) {
		linphone_chat_message_release(*it);
	}
	if (pendingMessage)
		linphone_chat_message_destroy(pendingMessage);
}

// -----------------------------------------------------------------------------

int ChatRoomPrivate::createChatMessageFromDb (void *data, int argc, char **argv, char **colName) {
	ChatRoomPrivate *d = reinterpret_cast<ChatRoomPrivate *>(data);
	return d->createChatMessageFromDb(argc, argv, colName);
}

void ChatRoomPrivate::onWeakMessageDestroyed (void *obj, belle_sip_object_t *messageBeingDestroyed) {
	ChatRoomPrivate *d = reinterpret_cast<ChatRoomPrivate *>(obj);
	d->onWeakMessageDestroyed(reinterpret_cast<LinphoneChatMessage *>(messageBeingDestroyed));
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::addTransientMessage (LinphoneChatMessage *msg) {
	auto iter = find(transientMessages.begin(), transientMessages.end(), msg);
	if (iter == transientMessages.end())
		transientMessages.push_back(linphone_chat_message_ref(msg));
}

void ChatRoomPrivate::addWeakMessage (LinphoneChatMessage *msg) {
	auto iter = find(weakMessages.begin(), weakMessages.end(), msg);
	if (iter == weakMessages.end())
		weakMessages.push_back(reinterpret_cast<LinphoneChatMessage *>(belle_sip_object_weak_ref(msg, onWeakMessageDestroyed, this)));
}

void ChatRoomPrivate::moveTransientMessageToWeakMessages (LinphoneChatMessage *msg) {
	auto iter = find(transientMessages.begin(), transientMessages.end(), msg);
	if (iter != transientMessages.end()) {
		/* msg is not transient anymore, we can remove it from our transient list and unref it */
		addWeakMessage(msg);
		removeTransientMessage(msg);
	} else {
		/* msg has already been removed from the transient messages, do nothing */
	}
}

void ChatRoomPrivate::removeTransientMessage (LinphoneChatMessage *msg) {
	auto iter = find(transientMessages.begin(), transientMessages.end(), msg);
	if (iter != transientMessages.end()) {
		linphone_chat_message_unref(*iter);
		transientMessages.erase(iter);
	}
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::release () {
	L_Q(ChatRoom);
	isComposingHandler.stopTimers();
	for (auto it = weakMessages.begin(); it != weakMessages.end(); it++) {
		linphone_chat_message_deactivate(*it);
	}
	for (auto it = transientMessages.begin(); it != transientMessages.end(); it++) {
		linphone_chat_message_deactivate(*it);
	}
	core = nullptr;
	linphone_chat_room_unref(GET_BACK_PTR(q));
}

void ChatRoomPrivate::sendImdn (const string &content, LinphoneReason reason) {
	L_Q(ChatRoom);

	const char *identity = nullptr;
	LinphoneAddress *peer = linphone_address_new(peerAddress.asString().c_str());
	LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(core, peer);
	if (proxy)
		identity = linphone_address_as_string(linphone_proxy_config_get_identity_address(proxy));
	else
		identity = linphone_core_get_primary_contact(core);

	/* Sending out of call */
	SalOp *op = sal_op_new(core->sal);
	linphone_configure_op(core, op, peer, nullptr, lp_config_get_int(core->config, "sip", "chat_msg_with_contact", 0));
	LinphoneChatMessage *msg = q->createMessage(content);
	LinphoneAddress *fromAddr = linphone_address_new(identity);
	linphone_chat_message_set_from_address(msg, fromAddr);
	LinphoneAddress *toAddr = linphone_address_new(peerAddress.asString().c_str());
	linphone_chat_message_set_to_address(msg, toAddr);
	linphone_chat_message_set_content_type(msg, "message/imdn+xml");

	/* Do not try to encrypt the notification when it is reporting an error (maybe it should be bypassed only for some reasons). */
	int retval = -1;
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(core);
	if (imee && (reason == LinphoneReasonNone)) {
		LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsOutgoingMessageCb cbProcessOutgoingMessage = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imeeCbs);
		if (cbProcessOutgoingMessage) {
			retval = cbProcessOutgoingMessage(imee, GET_BACK_PTR(q), msg);
		}
	}

	if (retval <= 0) {
		sal_message_send(op, identity, peerAddress.asString().c_str(), msg->content_type, msg->message, nullptr);
	}

	linphone_chat_message_unref(msg);
	linphone_address_unref(fromAddr);
	linphone_address_unref(toAddr);
	linphone_address_unref(peer);
	sal_op_unref(op);
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

// -----------------------------------------------------------------------------

void ChatRoomPrivate::sendIsComposingNotification () {
	L_Q(ChatRoom);
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
		SalOp *op = sal_op_new(core->sal);
		linphone_configure_op(core, op, peer, nullptr, lp_config_get_int(core->config, "sip", "chat_msg_with_contact", 0));
		string content = isComposingHandler.marshal(isComposing);
		if (!content.empty()) {
			int retval = -1;
			LinphoneAddress *fromAddr = linphone_address_new(identity);
			LinphoneChatMessage *msg = q->createMessage(content);
			linphone_chat_message_set_from_address(msg, fromAddr);
			linphone_chat_message_set_to_address(msg, peer);
			linphone_chat_message_set_content_type(msg, "application/im-iscomposing+xml");

			LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(core);
			if (imee) {
				LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsOutgoingMessageCb cbProcessOutgoingMessage = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imeeCbs);
				if (cbProcessOutgoingMessage) {
					retval = cbProcessOutgoingMessage(imee, GET_BACK_PTR(q), msg);
				}
			}

			if (retval <= 0) {
				sal_message_send(op, identity, peerAddress.asString().c_str(), msg->content_type, msg->message, nullptr);
			}

			linphone_chat_message_unref(msg);
			linphone_address_unref(fromAddr);
			sal_op_unref(op);
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
	L_Q(ChatRoom);
	unsigned int storageId = (unsigned int)atoi(argv[0]);

	/* Check if the message exists in the weak messages list, in which case we should return that one. */
	LinphoneChatMessage *newMessage = getWeakMessage(storageId);
	if (!newMessage) {
		/* Check if the message exists in the transient list, in which case we should return that one. */
		newMessage = getTransientMessage(storageId);
	}
	if (!newMessage) {
		newMessage = q->createMessage(argv[4] ? argv[4] : "");

		LinphoneAddress *peer = linphone_address_new(peerAddress.asString().c_str());
		if (atoi(argv[3]) == LinphoneChatMessageIncoming) {
			newMessage->dir = LinphoneChatMessageIncoming;
			linphone_chat_message_set_from(newMessage, peer);
			newMessage->to = nullptr; /* Will be filled at the end */
		} else {
			newMessage->dir = LinphoneChatMessageOutgoing;
			newMessage->from = nullptr; /* Will be filled at the end */
			linphone_chat_message_set_to(newMessage, peer);
		}
		linphone_address_unref(peer);

		newMessage->time = (time_t)atol(argv[9]);
		newMessage->is_read = atoi(argv[6]);
		newMessage->state = static_cast<LinphoneChatMessageState>(atoi(argv[7]));
		newMessage->storage_id = storageId;
		newMessage->external_body_url = ms_strdup(argv[8]);
		newMessage->appdata = ms_strdup(argv[10]);
		newMessage->message_id = ms_strdup(argv[12]);
		linphone_chat_message_set_content_type(newMessage, argv[13]);
		newMessage->is_secured = (bool_t)atoi(argv[14]);

		if (argv[11]) {
			int id = atoi(argv[11]);
			if (id >= 0)
				linphone_chat_message_fetch_content_from_database(core->db, newMessage, id);
		}

		/* Fix content type for old messages that were stored without it */
		if (!newMessage->content_type) {
			if (newMessage->file_transfer_information) {
				newMessage->content_type = ms_strdup("application/vnd.gsma.rcs-ft-http+xml");
			} else if (newMessage->external_body_url) {
				newMessage->content_type = ms_strdup("message/external-body");
			} else {
				newMessage->content_type = ms_strdup("text/plain");
			}
		}

		/* Add the new message to the weak messages list. */
		addWeakMessage(newMessage);
	}
	messages.push_front(newMessage);
	return 0;
}

void ChatRoomPrivate::onWeakMessageDestroyed (LinphoneChatMessage *messageBeingDestroyed) {
	auto iter = find(weakMessages.begin(), weakMessages.end(), messageBeingDestroyed);
	if (iter != transientMessages.end())
		weakMessages.erase(iter);
}

LinphoneChatMessage *ChatRoomPrivate::getTransientMessage (unsigned int storageId) const {
	for (auto it = transientMessages.begin(); it != transientMessages.end(); it++) {
		if (linphone_chat_message_get_storage_id(*it) == storageId)
			return linphone_chat_message_ref(*it);
	}
	return nullptr;
}

LinphoneChatMessage *ChatRoomPrivate::getWeakMessage (unsigned int storageId) const {
	for (auto it = weakMessages.begin(); it != weakMessages.end(); it++) {
		if (linphone_chat_message_get_storage_id(*it) == storageId)
			return linphone_chat_message_ref(*it);
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

list<LinphoneChatMessage *> ChatRoomPrivate::findMessages (const string &messageId) {
	if (!core->db)
		return list<LinphoneChatMessage *>();
	string peer = peerAddress.asStringUriOnly();
	char *buf = sqlite3_mprintf("SELECT * FROM history WHERE remoteContact = %Q AND messageId = %Q", peer.c_str(), messageId.c_str());
	messages.clear();
	sqlRequestMessage(core->db, buf);
	sqlite3_free(buf);
	list<LinphoneChatMessage *> result = messages;
	messages.clear();
	return result;
}

/**
 * TODO: Should be handled directly by the LinphoneChatMessage object!
 */
void ChatRoomPrivate::storeOrUpdateMessage (LinphoneChatMessage *msg) {
	if (msg->storage_id != 0) {
		/* The message has already been stored (probably because of file transfer), update it */
		linphone_chat_message_store_update(msg);
	} else {
		/* Store the new message */
		msg->storage_id = linphone_chat_message_store(msg);
	}
}

// -----------------------------------------------------------------------------

LinphoneReason ChatRoomPrivate::messageReceived (SalOp *op, const SalMessage *salMsg) {
	L_Q(ChatRoom);

	bool increaseMsgCount = true;
	LinphoneReason reason = LinphoneReasonNone;
	LinphoneChatMessage *msg;

	/* Check if this is a duplicate message */
	if ((msg = q->findMessageWithDirection(sal_op_get_call_id(op), LinphoneChatMessageIncoming))) {
		reason = core->chat_deny_code;
		if (msg)
			linphone_chat_message_unref(msg);
		return reason;
	}

	msg = q->createMessage(salMsg->text ? salMsg->text : "");
	linphone_chat_message_set_content_type(msg, salMsg->content_type);
	LinphoneAddress *peer = linphone_address_new(peerAddress.asString().c_str());
	linphone_chat_message_set_from(msg, peer);
	linphone_address_unref(peer);

	LinphoneAddress *to = sal_op_get_to(op) ? linphone_address_new(sal_op_get_to(op)) : linphone_address_new(linphone_core_get_identity(core));
	msg->to = to;
	msg->time = salMsg->time;
	msg->state = LinphoneChatMessageStateDelivered;
	msg->dir = LinphoneChatMessageIncoming;
	msg->message_id = ms_strdup(sal_op_get_call_id(op));

	const SalCustomHeader *ch = sal_op_get_recv_custom_header(op);
	if (ch)
		msg->custom_headers = sal_custom_header_clone(ch);
	if (salMsg->url)
		linphone_chat_message_set_external_body_url(msg, salMsg->url);

	int retval = -1;
	LinphoneImEncryptionEngine *imee = core->im_encryption_engine;
	if (imee) {
		LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsIncomingMessageCb cbProcessIncomingMessage = linphone_im_encryption_engine_cbs_get_process_incoming_message(imeeCbs);
		if (cbProcessIncomingMessage) {
			retval = cbProcessIncomingMessage(imee, GET_BACK_PTR(q), msg);
			if (retval == 0) {
				msg->is_secured = TRUE;
			} else if (retval > 0) {
				/* Unable to decrypt message */
				linphone_core_notify_message_received_unable_decrypt(core, GET_BACK_PTR(q), msg);
				reason = linphone_error_code_to_reason(retval);
				linphone_chat_message_send_delivery_notification(msg, reason);
				/* Return LinphoneReasonNone to avoid flexisip resending us a message we can't decrypt */
				reason = LinphoneReasonNone;
				goto end;
			}
		}
	}

	if ((retval <= 0) && (linphone_core_is_content_type_supported(core, msg->content_type) == FALSE)) {
		retval = 415;
		lError() << "Unsupported MESSAGE (content-type " << msg->content_type << " not recognized)";
	}

	if (retval > 0) {
		reason = linphone_error_code_to_reason(retval);
		linphone_chat_message_send_delivery_notification(msg, reason);
		goto end;
	}

	if (ContentType::isFileTransfer(msg->content_type)) {
		create_file_transfer_information_from_vnd_gsma_rcs_ft_http_xml(msg);
		linphone_chat_message_set_to_be_stored(msg, TRUE);
	} else if (ContentType::isImIsComposing(msg->content_type)) {
		isComposingReceived(msg->message);
		linphone_chat_message_set_to_be_stored(msg, FALSE);
		increaseMsgCount = FALSE;
		if (lp_config_get_int(core->config, "sip", "deliver_imdn", 0) != 1) {
			goto end;
		}
	} else if (ContentType::isImdn(msg->content_type)) {
		imdnReceived(msg->message);
		linphone_chat_message_set_to_be_stored(msg, FALSE);
		increaseMsgCount = FALSE;
		if (lp_config_get_int(core->config, "sip", "deliver_imdn", 0) != 1) {
			goto end;
		}
	} else if (ContentType::isText(msg->content_type)) {
		linphone_chat_message_set_to_be_stored(msg, TRUE);
	}

	if (increaseMsgCount) {
		if (unreadCount < 0)
			unreadCount = 1;
		else
			unreadCount++;
		/* Mark the message as pending so that if linphone_core_chat_room_mark_as_read() is called
			 in the linphone_chat_room_message_received() callback, it will effectively be marked as
			 being read before being stored. */
		pendingMessage = msg;
	}

	chatMessageReceived(msg);

	if (linphone_chat_message_get_to_be_stored(msg)) {
		msg->storage_id = linphone_chat_message_store(msg);
	}

	pendingMessage = nullptr;

end:
	if (msg)
		linphone_chat_message_unref(msg);
	return reason;
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::chatMessageReceived (LinphoneChatMessage *msg) {
	L_Q(ChatRoom);
	if (msg->message) {
		/* Legacy API */
		linphone_core_notify_text_message_received(core, GET_BACK_PTR(q), msg->from, msg->message);
	}
	linphone_core_notify_message_received(core, GET_BACK_PTR(q), msg);
	if (!ContentType::isImdn(msg->content_type) && !ContentType::isImIsComposing(msg->content_type)) {
		remoteIsComposing = false;
		linphone_core_notify_is_composing_received(core, GET_BACK_PTR(q));
		linphone_chat_message_send_delivery_notification(msg, LinphoneReasonNone);
	}
}

void ChatRoomPrivate::imdnReceived (const string &text) {
	L_Q(ChatRoom);
	Imdn::parse(*q, text);
}

void ChatRoomPrivate::isComposingReceived (const string &text) {
	isComposingHandler.parse(text);
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::onIsComposingStateChanged (bool isComposing) {
	this->isComposing = isComposing;
	sendIsComposingNotification();
}

void ChatRoomPrivate::onIsRemoteComposingStateChanged (bool isComposing) {
	L_Q(ChatRoom);
	remoteIsComposing = isComposing;
	linphone_core_notify_is_composing_received(core, GET_BACK_PTR(q));
}

void ChatRoomPrivate::onIsComposingRefreshNeeded () {
	sendIsComposingNotification();
}

// =============================================================================

ChatRoom::ChatRoom (LinphoneCore *core) : Object(*new ChatRoomPrivate(core)) {}

ChatRoom::ChatRoom (ChatRoomPrivate &p) : Object(p) {}

// -----------------------------------------------------------------------------

void ChatRoom::compose () {
	L_D(ChatRoom);
	if (!d->isComposing) {
		d->isComposing = true;
		d->sendIsComposingNotification();
		d->isComposingHandler.startRefreshTimer();
	}
	d->isComposingHandler.startIdleTimer();
}

LinphoneChatMessage *ChatRoom::createFileTransferMessage (const LinphoneContent *initialContent) {
	L_D(ChatRoom);
	LinphoneChatMessage *cm = belle_sip_object_new(LinphoneChatMessage);
	cm->callbacks = linphone_chat_message_cbs_new();
	cm->chat_room = GET_BACK_PTR(this);
	cm->message = nullptr;
	cm->file_transfer_information = linphone_content_copy(initialContent);
	cm->dir = LinphoneChatMessageOutgoing;
	LinphoneAddress *peer = linphone_address_new(d->peerAddress.asString().c_str());
	linphone_chat_message_set_to(cm, peer);
	linphone_address_unref(peer);
	cm->from = linphone_address_new(linphone_core_get_identity(d->core));
	/* This will be set to application/vnd.gsma.rcs-ft-http+xml when we will transfer the xml reply from server to the peers */
	cm->content_type = nullptr;
	/* This will store the http request during file upload to the server */
	cm->http_request = nullptr;
	cm->time = ms_time(0);
	return cm;
}

LinphoneChatMessage *ChatRoom::createMessage (const string &msg) {
	LinphoneChatMessage *cm = belle_sip_object_new(LinphoneChatMessage);
	cm->state = LinphoneChatMessageStateIdle;
	cm->callbacks = linphone_chat_message_cbs_new();
	cm->chat_room = GET_BACK_PTR(this);
	cm->message = msg.empty() ? nullptr : ms_strdup(msg.c_str());
	cm->content_type = ms_strdup("text/plain");
	cm->file_transfer_information = nullptr; /* this property is used only when transfering file */
	cm->http_request = nullptr;
	cm->time = ms_time(0);
	cm->is_secured = FALSE;
	return cm;
}

void ChatRoom::deleteHistory () {
	L_D(ChatRoom);
	if (!d->core->db) return;
	string peer = d->peerAddress.asStringUriOnly();
	char *buf = sqlite3_mprintf("DELETE FROM history WHERE remoteContact = %Q;", peer.c_str());
	d->sqlRequest(d->core->db, buf);
	sqlite3_free(buf);
	if (d->unreadCount > 0) d->unreadCount = 0;
}

void ChatRoom::deleteMessage (LinphoneChatMessage *msg) {
	L_D(ChatRoom);
	if (!d->core->db) return;
	char *buf = sqlite3_mprintf("DELETE FROM history WHERE id = %u;", msg->storage_id);
	d->sqlRequest(d->core->db, buf);
	sqlite3_free(buf);

	/* Invalidate unread_count when we modify the database, so that next
		 time we need it it will be recomputed from latest database state */
	d->unreadCount = -1;
}

LinphoneChatMessage *ChatRoom::findMessage (const string &messageId) {
	L_D(ChatRoom);
	LinphoneChatMessage *cm = nullptr;
	list<LinphoneChatMessage *> l = d->findMessages(messageId);
	if (!l.empty()) {
		cm = l.front();
		linphone_chat_message_ref(cm);
		for (auto it = l.begin(); it != l.end(); it++)
			linphone_chat_message_unref(*it);
	}
	return cm;
}

LinphoneChatMessage * ChatRoom::findMessageWithDirection (const string &messageId, LinphoneChatMessageDir direction) {
	L_D(ChatRoom);
	LinphoneChatMessage *ret = nullptr;
	list<LinphoneChatMessage *> l = d->findMessages(messageId);
	for (auto it = l.begin(); it != l.end(); it++) {
		LinphoneChatMessage *cm = *it;
		if (cm->dir == direction) {
			linphone_chat_message_ref(cm);
			ret = cm;
			break;
		}
	}
	if (!l.empty()) {
		for (auto it = l.begin(); it != l.end(); it++)
			linphone_chat_message_unref(*it);
	}
	return ret;
}

list<LinphoneChatMessage *> ChatRoom::getHistory (int nbMessages) {
	return getHistoryRange(0, nbMessages - 1);
}

int ChatRoom::getHistorySize () {
	L_D(ChatRoom);
	return d->getMessagesCount(false);
}

list<LinphoneChatMessage *> ChatRoom::getHistoryRange (int startm, int endm) {
	L_D(ChatRoom);
	if (!d->core->db) return list<LinphoneChatMessage *>();
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
		ms_message("%s(): end is lower than start (%d < %d). Assuming no end limit.", __FUNCTION__, endm, startm);
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
		ms_message("%s(): completed in %i ms", __FUNCTION__, (int)(end - begin));
	}
	ms_free(buf);

	if (!d->messages.empty()) {
		/* Fill local addr with core identity instead of per message */
		LinphoneAddress *localAddr = linphone_address_new(linphone_core_get_identity(d->core));
		for (auto it = d->messages.begin(); it != d->messages.end(); it++) {
			LinphoneChatMessage *msg = *it;
			if (msg->dir == LinphoneChatMessageOutgoing) {
				if (msg->from != NULL) linphone_address_unref(msg->from);
				msg->from = linphone_address_ref(localAddr);
			} else {
				msg->to = linphone_address_ref(localAddr);
			}
		}
		linphone_address_unref(localAddr);
	}

	list<LinphoneChatMessage *> result = d->messages;
	d->messages.clear();
	return result;
}

int ChatRoom::getUnreadMessagesCount () {
	L_D(ChatRoom);
	return d->getMessagesCount(true);
}

bool ChatRoom::isRemoteComposing () const {
	L_D(const ChatRoom);
	return d->remoteIsComposing;
}

void ChatRoom::markAsRead () {
	L_D(ChatRoom);

	if (!d->core->db) return;

	/* Optimization: do not modify the database if no message is marked as unread */
	if (getUnreadMessagesCount() == 0) return;

	string peer = d->peerAddress.asStringUriOnly();
	char *buf = sqlite3_mprintf("SELECT * FROM history WHERE remoteContact = %Q AND direction = %i AND status != %i", peer.c_str(), LinphoneChatMessageIncoming, LinphoneChatMessageStateDisplayed);
	d->sqlRequestMessage(d->core->db, buf);
	sqlite3_free(buf);
	for (auto it = d->messages.begin(); it != d->messages.end(); it++) {
		linphone_chat_message_send_display_notification(*it);
		linphone_chat_message_unref(*it);
	}
	d->messages.clear();
	buf = sqlite3_mprintf("UPDATE history SET status=%i WHERE remoteContact=%Q AND direction=%i;", LinphoneChatMessageStateDisplayed, peer.c_str(), LinphoneChatMessageIncoming);
	d->sqlRequest(d->core->db, buf);
	sqlite3_free(buf);

	if (d->pendingMessage) {
		linphone_chat_message_set_state(d->pendingMessage, LinphoneChatMessageStateDisplayed);
		linphone_chat_message_send_display_notification(d->pendingMessage);
	}

	d->unreadCount = 0;
}

void ChatRoom::sendMessage (LinphoneChatMessage *msg) {
	L_D(ChatRoom);

	msg->dir = LinphoneChatMessageOutgoing;

	/* Check if we shall upload a file to a server */
	if (msg->file_transfer_information && !msg->content_type) {
		/* Open a transaction with the server and send an empty request(RCS5.1 section 3.5.4.8.3.1) */
		if (linphone_chat_room_upload_file(msg) == 0) {
			/* Add to transient list only if message is going out */
			d->addTransientMessage(msg);
			/* Store the message so that even if the upload is stopped, it can be done again */
			d->storeOrUpdateMessage(msg);
		} else {
			linphone_chat_message_unref(msg);
			return;
		}
	} else {
		SalOp *op = msg->op;
		LinphoneCall *call = nullptr;
		string identity;
		char *clearTextMessage = nullptr;
		char *clearTextContentType = nullptr;
		LinphoneAddress *peer = linphone_address_new(d->peerAddress.asString().c_str());

		if (msg->message) {
			clearTextMessage = ms_strdup(msg->message);
		}
		if (msg->content_type) {
			clearTextContentType = ms_strdup(msg->content_type);
		}

		/* Add to transient list */
		d->addTransientMessage(msg);
		msg->time = ms_time(0);
		if (lp_config_get_int(d->core->config, "sip", "chat_use_call_dialogs", 0) != 0) {
			call = linphone_core_get_call_by_remote_address(d->core, d->peerAddress.asString().c_str());
			if (call) {
				if (linphone_call_get_state(call) == LinphoneCallConnected || linphone_call_get_state(call) == LinphoneCallStreamsRunning ||
					linphone_call_get_state(call) == LinphoneCallPaused || linphone_call_get_state(call) == LinphoneCallPausing ||
					linphone_call_get_state(call) == LinphoneCallPausedByRemote) {
					ms_message("send SIP msg through the existing call.");
					op = linphone_call_get_op(call);
					identity = linphone_core_find_best_identity(d->core, linphone_call_get_remote_address(call));
				}
			}
		}

		if (identity.empty()) {
			LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(d->core, peer);
			if (proxy) {
				identity = L_GET_CPP_PTR_FROM_C_STRUCT(linphone_proxy_config_get_identity_address(proxy), Address, Address)->asString();
			} else {
				identity = linphone_core_get_primary_contact(d->core);
			}
		}
		if (msg->from) {
			/* BUG: the file transfer message constructor sets the from, but doesn't do it as well as here */
			linphone_address_unref(msg->from);
		}
		msg->from = linphone_address_new(identity.c_str());

		int retval = -1;
		LinphoneImEncryptionEngine *imee = d->core->im_encryption_engine;
		if (imee) {
			LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
			LinphoneImEncryptionEngineCbsOutgoingMessageCb cbProcessOutgoingMessage = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imeeCbs);
			if (cbProcessOutgoingMessage) {
				retval = cbProcessOutgoingMessage(imee, GET_BACK_PTR(this), msg);
				if (retval == 0) {
					msg->is_secured = TRUE;
				}
			}
		}

		if (!op) {
			/* Sending out of call */
			msg->op = op = sal_op_new(d->core->sal);
			linphone_configure_op(d->core, op, peer, msg->custom_headers,
				lp_config_get_int(d->core->config, "sip", "chat_msg_with_contact", 0));
			sal_op_set_user_pointer(op, msg); /* If out of call, directly store msg */
		}

		if (retval > 0) {
			sal_error_info_set((SalErrorInfo *)sal_op_get_error_info(op), SalReasonNotAcceptable, "SIP", retval, "Unable to encrypt IM", nullptr);
			d->storeOrUpdateMessage(msg);
			linphone_chat_message_update_state(msg, LinphoneChatMessageStateNotDelivered);
			linphone_chat_message_unref(msg);
			linphone_address_unref(peer);
			return;
		}

		if (msg->external_body_url) {
			char *content_type = ms_strdup_printf("message/external-body; access-type=URL; URL=\"%s\"", msg->external_body_url);
			sal_message_send(op, identity.c_str(), d->peerAddress.asString().c_str(), content_type, nullptr, nullptr);
			ms_free(content_type);
		} else {
			if (msg->content_type) {
				sal_message_send(op, identity.c_str(), d->peerAddress.asString().c_str(), msg->content_type, msg->message, d->peerAddress.asStringUriOnly().c_str());
			} else {
				sal_text_send(op, identity.c_str(), d->peerAddress.asString().c_str(), msg->message);
			}
		}

		if (msg->message && clearTextMessage && strcmp(msg->message, clearTextMessage) != 0) {
			/* We replace the encrypted message by the original one so it can be correctly stored and displayed by the application */
			ms_free(msg->message);
			msg->message = ms_strdup(clearTextMessage);
		}
		if (msg->content_type && clearTextContentType && (strcmp(msg->content_type, clearTextContentType) != 0)) {
			/* We replace the encrypted content type by the original one */
			ms_free(msg->content_type);
			msg->content_type = ms_strdup(clearTextContentType);
		}
		msg->message_id = ms_strdup(sal_op_get_call_id(op));     /* must be known at that time */
		d->storeOrUpdateMessage(msg);

		if (d->isComposing)
			d->isComposing = false;
		d->isComposingHandler.stopIdleTimer();
		d->isComposingHandler.stopRefreshTimer();

		if (clearTextMessage) {
			ms_free(clearTextMessage);
		}
		if (clearTextContentType) {
			ms_free(clearTextContentType);
		}
		linphone_address_unref(peer);

		if (call && linphone_call_get_op(call) == op) {
			/* In this case, chat delivery status is not notified, so unrefing chat message right now */
			/* Might be better fixed by delivering status, but too costly for now */
			linphone_chat_room_remove_transient_message(msg->chat_room, msg);
			linphone_chat_message_unref(msg);
			return;
		}
	}

	/* If operation failed, we should not change message state */
	if (msg->dir == LinphoneChatMessageOutgoing) {
		linphone_chat_message_set_state(msg, LinphoneChatMessageStateInProgress);
	}
}

// -----------------------------------------------------------------------------

LinphoneCore *ChatRoom::getCore () const {
	L_D(const ChatRoom);
	return d->core;
}

// -----------------------------------------------------------------------------

const Address& ChatRoom::getPeerAddress () const {
	L_D(const ChatRoom);
	return d->peerAddress;
}

LINPHONE_END_NAMESPACE
