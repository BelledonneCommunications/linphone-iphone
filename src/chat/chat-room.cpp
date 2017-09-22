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

#include "linphone/api/c-chat-message.h"
#include "linphone/utils/utils.h"

#include "c-wrapper/c-wrapper.h"
#include "chat-room-p.h"
#include "content/content-type.h"
#include "imdn.h"
#include "logger/logger.h"

#include "chat-room.h"

#define GET_BACK_PTR(object) L_GET_C_BACK_PTR(object->shared_from_this(), ChatRoom)

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

ChatRoomPrivate::ChatRoomPrivate (LinphoneCore *core)
	: core(core), isComposingHandler(core, this) {}

ChatRoomPrivate::~ChatRoomPrivate () {
	for (auto &message : transientMessages)
		linphone_chat_message_release(message);
	if (pendingMessage)
		linphone_chat_message_unref(pendingMessage);
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

	for (auto &message : weakMessages)
		linphone_chat_message_deactivate(message);
	for (auto &message : transientMessages)
		linphone_chat_message_deactivate(message);

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
	linphone_configure_op(core, op, peer, nullptr, !!lp_config_get_int(core->config, "sip", "chat_msg_with_contact", 0));
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
		sal_message_send(op, identity, peerAddress.asString().c_str(), linphone_chat_message_get_content_type(msg), linphone_chat_message_get_text(msg), nullptr);
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

void ChatRoomPrivate::setState (ChatRoom::State newState) {
	if (newState != state) {
		state = newState;
		notifyStateChanged();
	}
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
		linphone_configure_op(core, op, peer, nullptr, !!lp_config_get_int(core->config, "sip", "chat_msg_with_contact", 0));
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
				sal_message_send(op, identity, peerAddress.asString().c_str(), linphone_chat_message_get_content_type(msg), linphone_chat_message_get_text(msg), nullptr);
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
			linphone_chat_message_set_incoming(newMessage);
			linphone_chat_message_set_from_address(newMessage, peer);
			linphone_chat_message_set_to_address(newMessage, NULL);
		} else {
			linphone_chat_message_set_outgoing(newMessage);
			linphone_chat_message_set_from_address(newMessage, NULL);
			linphone_chat_message_set_to_address(newMessage, peer);
		}
		linphone_address_unref(peer);

		linphone_chat_message_set_time(newMessage, (time_t)atol(argv[9]));
		linphone_chat_message_set_is_read(newMessage, !!atoi(argv[6]));
		linphone_chat_message_set_state(newMessage, static_cast<LinphoneChatMessageState>(atoi(argv[7])));
		linphone_chat_message_set_storage_id(newMessage, storageId);
		linphone_chat_message_set_external_body_url(newMessage, ms_strdup(argv[8]));
		linphone_chat_message_set_appdata(newMessage, ms_strdup(argv[10]));
		linphone_chat_message_set_message_id(newMessage, ms_strdup(argv[12]));
		linphone_chat_message_set_content_type(newMessage, argv[13]);
		linphone_chat_message_set_is_secured(newMessage, (bool_t)atoi(argv[14]));

		if (argv[11]) {
			int id = atoi(argv[11]);
			if (id >= 0)
				linphone_chat_message_fetch_content_from_database(core->db, newMessage, id);
		}

		/* Fix content type for old messages that were stored without it */
		if (!linphone_chat_message_get_content_type(newMessage)) {
			if (linphone_chat_message_get_file_transfer_information(newMessage)) {
				linphone_chat_message_set_content_type(newMessage, ms_strdup("application/vnd.gsma.rcs-ft-http+xml"));
			} else if (linphone_chat_message_get_external_body_url(newMessage)) {
				linphone_chat_message_set_content_type(newMessage, ms_strdup("message/external-body"));
			} else {
				linphone_chat_message_set_content_type(newMessage, ms_strdup("text/plain"));
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
	for (auto &message : transientMessages) {
		if (linphone_chat_message_get_storage_id(message) == storageId)
			return linphone_chat_message_ref(message);
	}
	return nullptr;
}

LinphoneChatMessage *ChatRoomPrivate::getWeakMessage (unsigned int storageId) const {
	for (auto &message : weakMessages) {
		if (linphone_chat_message_get_storage_id(message) == storageId)
			return linphone_chat_message_ref(message);
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
	if (linphone_chat_message_get_storage_id(msg) != 0) {
		/* The message has already been stored (probably because of file transfer), update it */
		linphone_chat_message_store_update(msg);
	} else {
		/* Store the new message */
		linphone_chat_message_store(msg);
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
	linphone_chat_message_set_from_address(msg, peer);
	linphone_address_unref(peer);

	LinphoneAddress *to = sal_op_get_to(op) ? linphone_address_new(sal_op_get_to(op)) : linphone_address_new(linphone_core_get_identity(core));
	linphone_chat_message_set_to_address(msg, to);
	linphone_chat_message_set_time(msg, salMsg->time);
	linphone_chat_message_set_state(msg, LinphoneChatMessageStateDelivered);
	linphone_chat_message_set_incoming(msg);
	linphone_chat_message_set_message_id(msg, ms_strdup(sal_op_get_call_id(op)));

	const SalCustomHeader *ch = sal_op_get_recv_custom_header(op);
	if (ch)
		linphone_chat_message_set_sal_custom_headers(msg, sal_custom_header_clone(ch));
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
				linphone_chat_message_set_is_secured(msg, TRUE);
			} else if (retval > 0) {
				/* Unable to decrypt message */
				notifyUndecryptableMessageReceived(msg);
				reason = linphone_error_code_to_reason(retval);
				linphone_chat_message_send_delivery_notification(msg, reason);
				/* Return LinphoneReasonNone to avoid flexisip resending us a message we can't decrypt */
				reason = LinphoneReasonNone;
				goto end;
			}
		}
	}

	if ((retval <= 0) && (linphone_core_is_content_type_supported(core, linphone_chat_message_get_content_type(msg)) == FALSE)) {
		retval = 415;
		lError() << "Unsupported MESSAGE (content-type " << linphone_chat_message_get_content_type(msg) << " not recognized)";
	}

	if (retval > 0) {
		reason = linphone_error_code_to_reason(retval);
		linphone_chat_message_send_delivery_notification(msg, reason);
		goto end;
	}

	if (ContentType::isFileTransfer(linphone_chat_message_get_content_type(msg))) {
		create_file_transfer_information_from_vnd_gsma_rcs_ft_http_xml(msg);
		linphone_chat_message_set_to_be_stored(msg, TRUE);
	} else if (ContentType::isImIsComposing(linphone_chat_message_get_content_type(msg))) {
		isComposingReceived(linphone_chat_message_get_text(msg));
		linphone_chat_message_set_to_be_stored(msg, FALSE);
		increaseMsgCount = FALSE;
		if (lp_config_get_int(core->config, "sip", "deliver_imdn", 0) != 1) {
			goto end;
		}
	} else if (ContentType::isImdn(linphone_chat_message_get_content_type(msg))) {
		imdnReceived(linphone_chat_message_get_text(msg));
		linphone_chat_message_set_to_be_stored(msg, FALSE);
		increaseMsgCount = FALSE;
		if (lp_config_get_int(core->config, "sip", "deliver_imdn", 0) != 1) {
			goto end;
		}
	} else if (ContentType::isText(linphone_chat_message_get_content_type(msg))) {
		linphone_chat_message_set_to_be_stored(msg, TRUE);
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

	if (linphone_chat_message_get_to_be_stored(msg)) {
		linphone_chat_message_store(msg);
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
	if (!ContentType::isImdn(linphone_chat_message_get_content_type(msg)) && !ContentType::isImIsComposing(linphone_chat_message_get_content_type(msg))) {
		notifyChatMessageReceived(msg);
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

void ChatRoomPrivate::notifyChatMessageReceived (LinphoneChatMessage *msg) {
	L_Q(ChatRoom);
	LinphoneChatRoom *cr = GET_BACK_PTR(q);
	if (linphone_chat_message_get_text(msg)) {
		/* Legacy API */
		linphone_core_notify_text_message_received(core, cr, linphone_chat_message_get_from_address(msg), linphone_chat_message_get_text(msg));
	}
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsMessageReceivedCb cb = linphone_chat_room_cbs_get_message_received(cbs);
	if (cb)
		cb(cr, msg);
	linphone_core_notify_message_received(core, cr, msg);
}

void ChatRoomPrivate::notifyStateChanged () {
	L_Q(ChatRoom);
	LinphoneChatRoom *cr = GET_BACK_PTR(q);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsStateChangedCb cb = linphone_chat_room_cbs_get_state_changed(cbs);
	if (cb)
		cb(cr, (LinphoneChatRoomState)state);
}

void ChatRoomPrivate::notifyUndecryptableMessageReceived (LinphoneChatMessage *msg) {
	L_Q(ChatRoom);
	LinphoneChatRoom *cr = GET_BACK_PTR(q);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(cr);
	LinphoneChatRoomCbsUndecryptableMessageReceivedCb cb = linphone_chat_room_cbs_get_undecryptable_message_received(cbs);
	if (cb)
		cb(cr, msg);
	linphone_core_notify_message_received_unable_decrypt(core, cr, msg);
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
	LinphoneChatMessage *msg = createMessage("");
	linphone_chat_message_set_chat_room(msg, GET_BACK_PTR(this));
	linphone_chat_message_set_text(msg, NULL);
	linphone_chat_message_set_file_transfer_information(msg, linphone_content_copy(initialContent));
	linphone_chat_message_set_outgoing(msg);
	LinphoneAddress *peer = linphone_address_new(d->peerAddress.asString().c_str());
	linphone_chat_message_set_to_address(msg, peer);
	linphone_address_unref(peer);
	linphone_chat_message_set_from_address(msg, linphone_address_new(linphone_core_get_identity(d->core)));
	/* This will be set to application/vnd.gsma.rcs-ft-http+xml when we will transfer the xml reply from server to the peers */
	linphone_chat_message_set_content_type(msg, NULL);
	/* This will store the http request during file upload to the server */
	linphone_chat_message_set_http_request(msg, NULL);
	linphone_chat_message_set_time(msg, ms_time(0));
	return msg;
}

LinphoneChatMessage *ChatRoom::createMessage (const string &message) {
	LinphoneChatMessage *msg = createMessage("");
	linphone_chat_message_set_chat_room(msg, GET_BACK_PTR(this));
	linphone_chat_message_set_state(msg, LinphoneChatMessageStateIdle);
	linphone_chat_message_set_text(msg, message.empty() ? nullptr : ms_strdup(message.c_str()));
	linphone_chat_message_set_content_type(msg, ms_strdup("text/plain"));
	linphone_chat_message_set_file_transfer_information(msg, nullptr);
	linphone_chat_message_set_http_request(msg, NULL);
	linphone_chat_message_set_time(msg, ms_time(0));
	return msg;
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
	char *buf = sqlite3_mprintf("DELETE FROM history WHERE id = %u;", linphone_chat_message_get_storage_id(msg));
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
		for (auto &message : l)
			linphone_chat_message_unref(message);
	}
	return cm;
}

LinphoneChatMessage * ChatRoom::findMessageWithDirection (const string &messageId, LinphoneChatMessageDir direction) {
	L_D(ChatRoom);
	LinphoneChatMessage *ret = nullptr;
	list<LinphoneChatMessage *> l = d->findMessages(messageId);
	for (auto &message : l) {
		if (linphone_chat_message_get_direction(message) == direction) {
			linphone_chat_message_ref(message);
			ret = message;
			break;
		}
	}
	if (!l.empty()) {
		for (auto &message : l)
			linphone_chat_message_unref(message);
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
		for (auto &message : d->messages) {
			if (linphone_chat_message_is_outgoing(message)) {
				linphone_chat_message_set_from_address(message, linphone_address_ref(localAddr));
			} else {
				linphone_chat_message_set_to_address(message, linphone_address_ref(localAddr));
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
	for (auto &message : d->messages) {
		linphone_chat_message_send_display_notification(message);
		linphone_chat_message_unref(message);
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

	linphone_chat_message_set_outgoing(msg);

	/* Check if we shall upload a file to a server */
	if (linphone_chat_message_get_file_transfer_information(msg) && !linphone_chat_message_get_content_type(msg)) {
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
		SalOp *op = linphone_chat_message_get_sal_op(msg);
		LinphoneCall *call = nullptr;
		string identity;
		char *clearTextMessage = nullptr;
		char *clearTextContentType = nullptr;
		LinphoneAddress *peer = linphone_address_new(d->peerAddress.asString().c_str());

		if (linphone_chat_message_get_text(msg)) {
			clearTextMessage = ms_strdup(linphone_chat_message_get_text(msg));
		}
		if (linphone_chat_message_get_content_type(msg)) {
			clearTextContentType = ms_strdup(linphone_chat_message_get_content_type(msg));
		}

		/* Add to transient list */
		d->addTransientMessage(msg);
		linphone_chat_message_set_time(msg, ms_time(0));
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
				identity = L_GET_CPP_PTR_FROM_C_STRUCT(linphone_proxy_config_get_identity_address(proxy))->asString();
			} else {
				identity = linphone_core_get_primary_contact(d->core);
			}
		}
		linphone_chat_message_set_from_address(msg, linphone_address_new(identity.c_str()));

		int retval = -1;
		LinphoneImEncryptionEngine *imee = d->core->im_encryption_engine;
		if (imee) {
			LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
			LinphoneImEncryptionEngineCbsOutgoingMessageCb cbProcessOutgoingMessage = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imeeCbs);
			if (cbProcessOutgoingMessage) {
				retval = cbProcessOutgoingMessage(imee, GET_BACK_PTR(this), msg);
				if (retval == 0) {
					linphone_chat_message_set_is_secured(msg, TRUE);
				}
			}
		}

		if (!op) {
			/* Sending out of call */
			linphone_chat_message_set_sal_op(msg, op = sal_op_new(d->core->sal));
			linphone_configure_op(
				d->core, op, peer, linphone_chat_message_get_sal_custom_headers(msg),
				!!lp_config_get_int(d->core->config, "sip", "chat_msg_with_contact", 0)
			);
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

		if (linphone_chat_message_get_external_body_url(msg)) {
			char *content_type = ms_strdup_printf("message/external-body; access-type=URL; URL=\"%s\"", linphone_chat_message_get_external_body_url(msg));
			sal_message_send(op, identity.c_str(), d->peerAddress.asString().c_str(), content_type, nullptr, nullptr);
			ms_free(content_type);
		} else {
			if (linphone_chat_message_get_content_type(msg)) {
				sal_message_send(op, identity.c_str(), d->peerAddress.asString().c_str(), linphone_chat_message_get_content_type(msg), linphone_chat_message_get_text(msg), d->peerAddress.asStringUriOnly().c_str());
			} else {
				sal_text_send(op, identity.c_str(), d->peerAddress.asString().c_str(), linphone_chat_message_get_text(msg));
			}
		}

		if (linphone_chat_message_get_text(msg) && clearTextMessage && strcmp(linphone_chat_message_get_text(msg), clearTextMessage) != 0) {
			/* We replace the encrypted message by the original one so it can be correctly stored and displayed by the application */
			linphone_chat_message_set_text(msg, ms_strdup(clearTextMessage));
		}
		if (linphone_chat_message_get_content_type(msg) && clearTextContentType && (strcmp(linphone_chat_message_get_content_type(msg), clearTextContentType) != 0)) {
			/* We replace the encrypted content type by the original one */
			linphone_chat_message_set_content_type(msg, ms_strdup(clearTextContentType));
		}
		linphone_chat_message_set_message_id(msg, ms_strdup(sal_op_get_call_id(op)));     /* must be known at that time */
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
			linphone_chat_room_remove_transient_message(linphone_chat_message_get_chat_room(msg), msg);
			linphone_chat_message_unref(msg);
			return;
		}
	}

	/* If operation failed, we should not change message state */
	if (linphone_chat_message_is_outgoing(msg)) {
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

ChatRoom::State ChatRoom::getState () const {
	L_D(const ChatRoom);
	return d->state;
}

LINPHONE_END_NAMESPACE
