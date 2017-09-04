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

#include "chat-room-p.h"

#include "chat-room.h"
#include "imdn.h"
#include "logger/logger.h"
#include "utils/content-type.h"

#include "chat-room.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

ChatRoomPrivate::ChatRoomPrivate (LinphoneCore *core)
	: core(core), isComposingHandler(core, this) {}

ChatRoomPrivate::~ChatRoomPrivate () {
	for (auto it = transientMessages.begin(); it != transientMessages.end(); it++) {
		linphone_chat_message_release(*it);
	}
	if (!receivedRttCharacters.empty()) {
		for (auto it = receivedRttCharacters.begin(); it != receivedRttCharacters.end(); it++)
			bctbx_free(*it);
	}
	if (core) {
		if (bctbx_list_find(core->chatrooms, cBackPointer)) {
			lError() << "LinphoneChatRoom[" << cBackPointer << "] is destroyed while still being used by the LinphoneCore. " <<
				"This is abnormal. linphone_core_get_chat_room() doesn't give a reference, there is no need to call linphone_chat_room_unref(). " <<
				"In order to remove a chat room from the core, use linphone_core_delete_chat_room().";
			core->chatrooms = bctbx_list_remove(core->chatrooms, cBackPointer);
		}
	}
	linphone_address_unref(peerAddress);
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
	isComposingHandler.stopTimers();
	for (auto it = weakMessages.begin(); it != weakMessages.end(); it++) {
		linphone_chat_message_deactivate(*it);
	}
	for (auto it = transientMessages.begin(); it != transientMessages.end(); it++) {
		linphone_chat_message_deactivate(*it);
	}
	core = nullptr;
	linphone_chat_room_unref(cBackPointer);
}

void ChatRoomPrivate::sendImdn (const string &content, LinphoneReason reason) {
	L_Q(ChatRoom);

	const char *identity = nullptr;
	LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(core, peerAddress);
	if (proxy)
		identity = linphone_address_as_string(linphone_proxy_config_get_identity_address(proxy));
	else
		identity = linphone_core_get_primary_contact(core);

	/* Sending out of call */
	SalOp *op = sal_op_new(core->sal);
	linphone_configure_op(core, op, peerAddress, nullptr, lp_config_get_int(core->config, "sip", "chat_msg_with_contact", 0));
	LinphoneChatMessage *msg = q->createMessage(content);
	LinphoneAddress *fromAddr = linphone_address_new(identity);
	linphone_chat_message_set_from_address(msg, fromAddr);
	LinphoneAddress *toAddr = linphone_address_new(peer.c_str());
	linphone_chat_message_set_to_address(msg, toAddr);
	linphone_chat_message_set_content_type(msg, "message/imdn+xml");

	/* Do not try to encrypt the notification when it is reporting an error (maybe it should be bypassed only for some reasons). */
	int retval = -1;
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(core);
	if (imee && (reason == LinphoneReasonNone)) {
		LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsOutgoingMessageCb cbProcessOutgoingMessage = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imeeCbs);
		if (cbProcessOutgoingMessage) {
			retval = cbProcessOutgoingMessage(imee, cBackPointer, msg);
		}
	}

	if (retval <= 0) {
		sal_message_send(op, identity, peer.c_str(), msg->content_type, msg->message, nullptr);
	}

	linphone_chat_message_unref(msg);
	linphone_address_unref(fromAddr);
	linphone_address_unref(toAddr);
	sal_op_unref(op);
}

// -----------------------------------------------------------------------------

int ChatRoomPrivate::getMessagesCount (bool unreadOnly) {
	if (!core->db) return 0;

	/* Optimization: do not read database if the count is already available in memory */
	if (unreadOnly && unreadCount >= 0) return unreadCount;

	char *peer = linphone_address_as_string_uri_only(peerAddress);
	char *option = nullptr;
	if (unreadOnly)
		option = bctbx_strdup_printf("AND status!=%i AND direction=%i", LinphoneChatMessageStateDisplayed, LinphoneChatMessageIncoming);
	char *buf = sqlite3_mprintf("SELECT count(*) FROM history WHERE remoteContact = %Q %s;", peer, unreadOnly ? option : "");
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
	ms_free(peer);

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
		LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(core, peerAddress);
		const char *identity = nullptr;

		if (proxy)
			identity = linphone_address_as_string(linphone_proxy_config_get_identity_address(proxy));
		else
			identity = linphone_core_get_primary_contact(core);

		/* Sending out of call */
		SalOp *op = sal_op_new(core->sal);
		linphone_configure_op(core, op, peerAddress, nullptr, lp_config_get_int(core->config, "sip", "chat_msg_with_contact", 0));
		string content = isComposingHandler.marshal(isComposing);
		if (!content.empty()) {
			int retval = -1;
			LinphoneAddress *fromAddr = linphone_address_new(identity);
			LinphoneChatMessage *msg = q->createMessage(content);
			linphone_chat_message_set_from_address(msg, fromAddr);
			linphone_chat_message_set_to_address(msg, peerAddress);
			linphone_chat_message_set_content_type(msg, "application/im-iscomposing+xml");

			LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(core);
			if (imee) {
				LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsOutgoingMessageCb cbProcessOutgoingMessage = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imeeCbs);
				if (cbProcessOutgoingMessage) {
					retval = cbProcessOutgoingMessage(imee, cBackPointer, msg);
				}
			}

			if (retval <= 0) {
				sal_message_send(op, identity, peer.c_str(), msg->content_type, msg->message, nullptr);
			}

			linphone_chat_message_unref(msg);
			linphone_address_unref(fromAddr);
			sal_op_unref(op);
		}
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

		if (atoi(argv[3]) == LinphoneChatMessageIncoming) {
			newMessage->dir = LinphoneChatMessageIncoming;
			linphone_chat_message_set_from(newMessage, peerAddress);
			newMessage->to = nullptr; /* Will be filled at the end */
		} else {
			newMessage->dir = LinphoneChatMessageOutgoing;
			newMessage->from = nullptr; /* Will be filled at the end */
			linphone_chat_message_set_to(newMessage, peerAddress);
		}

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
	char *peer = linphone_address_as_string_uri_only(peerAddress);
	char *buf = sqlite3_mprintf("SELECT * FROM history WHERE remoteContact = %Q AND messageId = %Q", peer, messageId.c_str());
	messages.clear();
	sqlRequestMessage(core->db, buf);
	sqlite3_free(buf);
	ms_free(peer);
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
	linphone_chat_message_set_from(msg, peerAddress);

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
			retval = cbProcessIncomingMessage(imee, cBackPointer, msg);
			if (retval == 0) {
				msg->is_secured = TRUE;
			} else if (retval > 0) {
				/* Unable to decrypt message */
				linphone_core_notify_message_received_unable_decrypt(core, cBackPointer, msg);
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

void ChatRoomPrivate::realtimeTextReceived (uint32_t character, LinphoneCall *call) {
	L_Q(ChatRoom);
	const uint32_t new_line = 0x2028;
	const uint32_t crlf = 0x0D0A;
	const uint32_t lf = 0x0A;

	if (call && linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(call))) {
		LinphoneChatMessageCharacter *cmc = bctbx_new0(LinphoneChatMessageCharacter, 1);

		if (!pendingMessage)
			pendingMessage = q->createMessage("");

		cmc->value = character;
		cmc->has_been_read = FALSE;
		receivedRttCharacters.push_back(cmc);

		remoteIsComposing = true;
		linphone_core_notify_is_composing_received(core, cBackPointer);

		if ((character == new_line) || (character == crlf) || (character == lf)) {
			/* End of message */
			lDebug() << "New line received, forge a message with content " << pendingMessage->message;
			linphone_chat_message_set_from(pendingMessage, peerAddress);
			if (pendingMessage->to)
				linphone_address_unref(pendingMessage->to);
			pendingMessage->to = call->dest_proxy
				? linphone_address_clone(call->dest_proxy->identity_address)
				: linphone_address_new(linphone_core_get_identity(core));
			pendingMessage->time = ms_time(0);
			pendingMessage->state = LinphoneChatMessageStateDelivered;
			pendingMessage->dir = LinphoneChatMessageIncoming;

			if (lp_config_get_int(core->config, "misc", "store_rtt_messages", 1) == 1)
				storeOrUpdateMessage(pendingMessage);

			if (unreadCount < 0) unreadCount = 1;
			else unreadCount++;

			chatMessageReceived(pendingMessage);
			linphone_chat_message_unref(pendingMessage);
			pendingMessage = nullptr;
			for (auto it = receivedRttCharacters.begin(); it != receivedRttCharacters.end(); it++)
				ms_free(*it);
			receivedRttCharacters.clear();
		} else {
			char *value = Utils::utf8ToChar(character);
			pendingMessage->message = ms_strcat_printf(pendingMessage->message, value);
			lDebug() << "Received RTT character: " << value << " (" << character << "), pending text is " << pendingMessage->message;
			delete value;
		}
	}
}

// -----------------------------------------------------------------------------

void ChatRoomPrivate::chatMessageReceived (LinphoneChatMessage *msg) {
	if (msg->message) {
		/* Legacy API */
		linphone_core_notify_text_message_received(core, cBackPointer, msg->from, msg->message);
	}
	linphone_core_notify_message_received(core, cBackPointer, msg);
	if (!ContentType::isImdn(msg->content_type) && !ContentType::isImIsComposing(msg->content_type)) {
		remoteIsComposing = false;
		linphone_core_notify_is_composing_received(core, cBackPointer);
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

void ChatRoomPrivate::isComposingStateChanged (bool isComposing) {
	this->isComposing = isComposing;
	sendIsComposingNotification();
}

void ChatRoomPrivate::isRemoteComposingStateChanged (bool isComposing) {
	remoteIsComposing = isComposing;
	linphone_core_notify_is_composing_received(core, cBackPointer);
}

void ChatRoomPrivate::isComposingRefreshNeeded () {
	sendIsComposingNotification();
}

// =============================================================================

ChatRoom::ChatRoom (LinphoneCore *core, LinphoneAddress *peerAddress) : Object(*new ChatRoomPrivate(core)) {
	L_D(ChatRoom);
	d->peerAddress = peerAddress;
	char *peerStr = linphone_address_as_string(d->peerAddress);
	d->peer = peerStr;
	ms_free(peerStr);
}

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
	cm->chat_room = d->cBackPointer;
	cm->message = nullptr;
	cm->file_transfer_information = linphone_content_copy(initialContent);
	cm->dir = LinphoneChatMessageOutgoing;
	linphone_chat_message_set_to(cm, d->peerAddress);
	cm->from = linphone_address_new(linphone_core_get_identity(d->core));
	/* This will be set to application/vnd.gsma.rcs-ft-http+xml when we will transfer the xml reply from server to the peers */
	cm->content_type = nullptr;
	/* This will store the http request during file upload to the server */
	cm->http_request = nullptr;
	cm->time = ms_time(0);
	return cm;
}

LinphoneChatMessage *ChatRoom::createMessage (const string &msg) {
	L_D(ChatRoom);
	LinphoneChatMessage *cm = belle_sip_object_new(LinphoneChatMessage);
	cm->state = LinphoneChatMessageStateIdle;
	cm->callbacks = linphone_chat_message_cbs_new();
	cm->chat_room = d->cBackPointer;
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
	char *peer = linphone_address_as_string_uri_only(d->peerAddress);
	char *buf = sqlite3_mprintf("DELETE FROM history WHERE remoteContact = %Q;", peer);
	d->sqlRequest(d->core->db, buf);
	sqlite3_free(buf);
	ms_free(peer);
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

uint32_t ChatRoom::getChar () const {
	L_D(const ChatRoom);
	if (!d->receivedRttCharacters.empty()) {
		for (auto it = d->receivedRttCharacters.begin(); it != d->receivedRttCharacters.end(); it++) {
			LinphoneChatMessageCharacter *cmc = *it;
			if (!cmc->has_been_read) {
				cmc->has_been_read = TRUE;
				return cmc->value;
			}
		}
	}
	return 0;
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
	char *peer = linphone_address_as_string_uri_only(d->peerAddress);
	d->messages.clear();

	/* Since we want to append query parameters depending on arguments given, we use malloc instead of sqlite3_mprintf */
	const int bufMaxSize = 512;
	char *buf = reinterpret_cast<char *>(ms_malloc(bufMaxSize));
	buf = sqlite3_snprintf(bufMaxSize - 1, buf, "SELECT * FROM history WHERE remoteContact = %Q ORDER BY id DESC", peer);

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
	ms_free(peer);
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

	char *peer = linphone_address_as_string_uri_only(d->peerAddress);
	char *buf = sqlite3_mprintf("SELECT * FROM history WHERE remoteContact = %Q AND direction = %i AND status != %i", peer, LinphoneChatMessageIncoming, LinphoneChatMessageStateDisplayed);
	d->sqlRequestMessage(d->core->db, buf);
	sqlite3_free(buf);
	for (auto it = d->messages.begin(); it != d->messages.end(); it++) {
		linphone_chat_message_send_display_notification(*it);
		linphone_chat_message_unref(*it);
	}
	d->messages.clear();
	buf = sqlite3_mprintf("UPDATE history SET status=%i WHERE remoteContact=%Q AND direction=%i;", LinphoneChatMessageStateDisplayed, peer, LinphoneChatMessageIncoming);
	d->sqlRequest(d->core->db, buf);
	sqlite3_free(buf);
	ms_free(peer);

	if (d->pendingMessage) {
		linphone_chat_message_set_state(d->pendingMessage, LinphoneChatMessageStateDisplayed);
		linphone_chat_message_send_display_notification(d->pendingMessage);
	}

	d->unreadCount = 0;
}

void ChatRoom::sendMessage (LinphoneChatMessage *msg) {
	L_D(ChatRoom);

	/* Stubed rtt */
	if (d->call && linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(d->call))) {
		uint32_t new_line = 0x2028;
		linphone_chat_message_put_char(msg, new_line);
		linphone_chat_message_unref(msg);
		return;
	}

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
		const char *identity = nullptr;
		char *clearTextMessage = nullptr;
		char *clearTextContentType = nullptr;

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
			call = linphone_core_get_call_by_remote_address(d->core, d->peer.c_str());
			if (call) {
				if (call->state == LinphoneCallConnected || call->state == LinphoneCallStreamsRunning ||
						call->state == LinphoneCallPaused || call->state == LinphoneCallPausing ||
						call->state == LinphoneCallPausedByRemote) {
					ms_message("send SIP msg through the existing call.");
					op = call->op;
					identity = linphone_core_find_best_identity(d->core, linphone_call_get_remote_address(call));
				}
			}
		}

		if (!identity) {
			LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(d->core, d->peerAddress);
			if (proxy) {
				identity = linphone_address_as_string(linphone_proxy_config_get_identity_address(proxy));
			} else {
				identity = linphone_core_get_primary_contact(d->core);
			}
		}
		if (msg->from) {
			/* BUG: the file transfer message constructor sets the from, but doesn't do it as well as here */
			linphone_address_unref(msg->from);
		}
		msg->from = linphone_address_new(identity);

		int retval = -1;
		LinphoneImEncryptionEngine *imee = d->core->im_encryption_engine;
		if (imee) {
			LinphoneImEncryptionEngineCbs *imeeCbs = linphone_im_encryption_engine_get_callbacks(imee);
			LinphoneImEncryptionEngineCbsOutgoingMessageCb cbProcessOutgoingMessage = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imeeCbs);
			if (cbProcessOutgoingMessage) {
				retval = cbProcessOutgoingMessage(imee, d->cBackPointer, msg);
				if (retval == 0) {
					msg->is_secured = TRUE;
				}
			}
		}

		if (!op) {
			/* Sending out of call */
			msg->op = op = sal_op_new(d->core->sal);
			linphone_configure_op(d->core, op, d->peerAddress, msg->custom_headers,
				lp_config_get_int(d->core->config, "sip", "chat_msg_with_contact", 0));
			sal_op_set_user_pointer(op, msg); /* If out of call, directly store msg */
		}

		if (retval > 0) {
			sal_error_info_set((SalErrorInfo *)sal_op_get_error_info(op), SalReasonNotAcceptable, "SIP", retval, "Unable to encrypt IM", nullptr);
			d->storeOrUpdateMessage(msg);
			linphone_chat_message_update_state(msg, LinphoneChatMessageStateNotDelivered);
			linphone_chat_message_unref(msg);
			return;
		}

		if (msg->external_body_url) {
			char *content_type = ms_strdup_printf("message/external-body; access-type=URL; URL=\"%s\"", msg->external_body_url);
			sal_message_send(op, identity, d->peer.c_str(), content_type, nullptr, nullptr);
			ms_free(content_type);
		} else {
			char *peerUri = linphone_address_as_string_uri_only(d->peerAddress);
			if (msg->content_type) {
				sal_message_send(op, identity, d->peer.c_str(), msg->content_type, msg->message, peerUri);
			} else {
				sal_text_send(op, identity, d->peer.c_str(), msg->message);
			}
			ms_free(peerUri);
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

		if (call && call->op == op) {
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

LinphoneCall *ChatRoom::getCall () const {
	L_D(const ChatRoom);
	return d->call;
}

LinphoneCore *ChatRoom::getCore () const {
	L_D(const ChatRoom);
	return d->core;
}

// -----------------------------------------------------------------------------

const LinphoneAddress *ChatRoom::getPeerAddress () const {
	L_D(const ChatRoom);
	return d->peerAddress;
}

LINPHONE_END_NAMESPACE
