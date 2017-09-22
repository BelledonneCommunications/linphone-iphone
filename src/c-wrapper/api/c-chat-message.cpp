/*
 * c-chat-message.cpp
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

#include "linphone/api/c-chat-message.h"
#include "linphone/utils/utils.h"
#include "linphone/wrapper_utils.h"

#include "ortp/b64.h"

#include "c-wrapper/c-wrapper.h"
#include "chat/chat-message-p.h"
#include "chat/chat-room-p.h"
#include "chat/real-time-text-chat-room-p.h"
#include "content/content-type.h"

// =============================================================================

#define GET_CPP_PTR(obj) L_GET_CPP_PTR_FROM_C_OBJECT(obj)
#define GET_CPP_PRIVATE_PTR(obj) L_GET_PRIVATE_FROM_C_OBJECT(obj, ChatMessage, ChatMessage)

using namespace std;

static void _linphone_chat_message_constructor (LinphoneChatMessage *msg);
static void _linphone_chat_message_destructor (LinphoneChatMessage *msg);

L_DECLARE_C_OBJECT_IMPL_WITH_XTORS(ChatMessage,
	_linphone_chat_message_constructor, _linphone_chat_message_destructor,
	LinphoneChatMessageCbs *cbs;
	LinphoneChatRoom* chat_room;
	LinphoneErrorInfo *ei;
	LinphoneChatMessageDir dir;
	char* message;
	void* message_userdata;
	char* appdata;
	char* external_body_url;
	LinphoneAddress *from;
	LinphoneAddress *to;
	time_t time;
	SalCustomHeader *sal_custom_headers;
	LinphoneChatMessageState state;
	bool_t is_read;
	unsigned int storage_id;
	char *message_id;
	SalOp *op;
	LinphoneContent *file_transfer_information; //< used to store file transfer information when the message is of file transfer type
	char *content_type; //< is used to specified the type of message to be sent, used only for file transfer message
	bool_t to_be_stored;
	belle_http_request_t *http_request; //< keep a reference to the http_request in case of file transfer in order to be able to cancel the transfer
	belle_http_request_listener_t *http_listener; // our listener, only owned by us
	char *file_transfer_filepath;
	unsigned long bg_task_id;
	bool_t is_secured;
	LinphoneChatMessageStateChangedCb message_state_changed_cb;
	void* message_state_changed_user_data;
)

static void _linphone_chat_message_constructor (LinphoneChatMessage *msg) {
	msg->cbs = linphone_chat_message_cbs_new();
}

static void _linphone_chat_message_destructor (LinphoneChatMessage *msg) {
	linphone_chat_message_cbs_unref(msg->cbs);
	msg->cbs = nullptr;
}

// =============================================================================
// Reference and user data handling functions.
// =============================================================================

LinphoneChatMessage *linphone_chat_message_ref (LinphoneChatMessage *msg) {
	belle_sip_object_ref(msg);
	return msg;
}

void linphone_chat_message_unref (LinphoneChatMessage *msg) {
	belle_sip_object_unref(msg);
}

void * linphone_chat_message_get_user_data (const LinphoneChatMessage *msg) {
	return L_GET_USER_DATA_FROM_C_OBJECT(msg, ChatMessage);
}

void linphone_chat_message_set_user_data (LinphoneChatMessage *msg, void *ud) {
	L_SET_USER_DATA_FROM_C_OBJECT(msg, ud, ChatMessage);
}


// =============================================================================
// Getter and setters
// =============================================================================

const char *linphone_chat_message_get_external_body_url(const LinphoneChatMessage *msg) {
	return msg->external_body_url;
}

void linphone_chat_message_set_external_body_url(LinphoneChatMessage *msg, const char *url) {
	if (msg->external_body_url) {
		ms_free(msg->external_body_url);
	}
	msg->external_body_url = url ? ms_strdup(url) : NULL;
}

time_t linphone_chat_message_get_time(const LinphoneChatMessage *msg) {
	return msg->time;
}

void linphone_chat_message_set_time(LinphoneChatMessage *msg, time_t time) {
	msg->time = time;
}

void linphone_chat_message_set_is_secured(LinphoneChatMessage *msg, bool_t secured) {
	msg->is_secured = secured;
}

bool_t linphone_chat_message_is_secured(LinphoneChatMessage *msg) {
	return msg->is_secured;
}

bool_t linphone_chat_message_is_outgoing(LinphoneChatMessage *msg) {
	return msg->dir == LinphoneChatMessageOutgoing;
}

void linphone_chat_message_set_incoming(LinphoneChatMessage *msg) {
	msg->dir = LinphoneChatMessageIncoming;
}

void linphone_chat_message_set_outgoing(LinphoneChatMessage *msg) {
	msg->dir = LinphoneChatMessageOutgoing;
}

void linphone_chat_message_set_from_address(LinphoneChatMessage *msg, const LinphoneAddress *from) {
	if (msg->from)
		linphone_address_unref(msg->from);
	msg->from = linphone_address_clone(from);
}

const LinphoneAddress *linphone_chat_message_get_from_address(const LinphoneChatMessage *msg) {
	return msg->from;
}

void linphone_chat_message_set_to_address(LinphoneChatMessage *msg, const LinphoneAddress *to) {
	if (msg->to)
		linphone_address_unref(msg->to);
	msg->to = linphone_address_clone(to);
}

const LinphoneAddress *linphone_chat_message_get_to_address(const LinphoneChatMessage *msg) {
	if (msg->to)
		return msg->to;
	if (msg->dir == LinphoneChatMessageOutgoing) {
		return linphone_chat_room_get_peer_address(msg->chat_room);
	}
	return NULL;
}

void linphone_chat_message_set_message_state_changed_cb(LinphoneChatMessage* msg, LinphoneChatMessageStateChangedCb cb) {
	msg->message_state_changed_cb = cb;
}

void linphone_chat_message_set_message_state_changed_cb_user_data(LinphoneChatMessage* msg, void *user_data) {
	msg->message_state_changed_user_data = user_data;
}

void * linphone_chat_message_get_message_state_changed_cb_user_data(LinphoneChatMessage* msg) {
	return msg->message_state_changed_user_data;
}

const char * linphone_chat_message_get_content_type(const LinphoneChatMessage *msg) {
	return msg->content_type;
}

void linphone_chat_message_set_content_type(LinphoneChatMessage *msg, const char *content_type) {
	if (msg->content_type) {
		ms_free(msg->content_type);
	}
	msg->content_type = content_type ? ms_strdup(content_type) : NULL;
}

const char *linphone_chat_message_get_text(const LinphoneChatMessage *msg) {
	return msg->message;
}

int linphone_chat_message_set_text(LinphoneChatMessage *msg, const char* text) {
	if (msg->message)
		ms_free(msg->message);
	if (text)
		msg->message = ms_strdup(text);
	else
		msg->message = NULL;

	return 0;
}

unsigned int linphone_chat_message_get_storage_id(LinphoneChatMessage *msg) {
	return msg->storage_id;
}

void linphone_chat_message_set_state(LinphoneChatMessage *msg, LinphoneChatMessageState state) {
	/* do not invoke callbacks on orphan messages */
	if (state != msg->state && msg->chat_room != NULL) {
		if (((msg->state == LinphoneChatMessageStateDisplayed) || (msg->state == LinphoneChatMessageStateDeliveredToUser))
			&& ((state == LinphoneChatMessageStateDeliveredToUser) || (state == LinphoneChatMessageStateDelivered) || (state == LinphoneChatMessageStateNotDelivered))) {
			/* If the message has been displayed or delivered to user we must not go back to the delivered or not delivered state. */
			return;
		}
		ms_message("Chat message %p: moving from state %s to %s", msg, linphone_chat_message_state_to_string(msg->state),
				   linphone_chat_message_state_to_string(state));
		msg->state = state;
		if (msg->message_state_changed_cb) {
			msg->message_state_changed_cb(msg, msg->state, msg->message_state_changed_user_data);
		}
		if (linphone_chat_message_cbs_get_msg_state_changed(msg->cbs)) {
			linphone_chat_message_cbs_get_msg_state_changed(msg->cbs)(msg, msg->state);
		}
	}
}

const char* linphone_chat_message_get_message_id(const LinphoneChatMessage *msg) {
	return msg->message_id;
}

void linphone_chat_message_set_message_id(LinphoneChatMessage *msg, char *id) {
	msg->message_id = id;
}

void linphone_chat_message_set_is_read(LinphoneChatMessage *msg, bool_t is_read) {
	msg->is_read = is_read;
}

void linphone_chat_message_set_storage_id(LinphoneChatMessage *msg, unsigned int id) {
	msg->storage_id = id;
}

const char *linphone_chat_message_get_appdata(const LinphoneChatMessage *msg) {
	return msg->appdata;
}

void linphone_chat_message_set_appdata(LinphoneChatMessage *msg, const char *data) {
	if (msg->appdata) {
		ms_free(msg->appdata);
	}
	msg->appdata = data ? ms_strdup(data) : NULL;
	linphone_chat_message_store_appdata(msg);
}

SalCustomHeader * linphone_chat_message_get_sal_custom_headers(const LinphoneChatMessage *msg) {
	return msg->sal_custom_headers;
}

void linphone_chat_message_set_sal_custom_headers(LinphoneChatMessage *msg, SalCustomHeader *header) {
	msg->sal_custom_headers = header;
}

belle_http_request_t * linphone_chat_message_get_http_request(LinphoneChatMessage *msg) {
	return msg->http_request;
}

void linphone_chat_message_set_http_request(LinphoneChatMessage *msg, belle_http_request_t *request) {
	msg->http_request = request;
}

void linphone_chat_message_set_file_transfer_information(LinphoneChatMessage *msg, LinphoneContent *content) {
	msg->file_transfer_information = content;
}

LinphoneChatMessageDir linphone_chat_message_get_direction(const LinphoneChatMessage *msg) {
	return msg->dir;
}

LinphoneChatRoom *linphone_chat_message_get_chat_room(const LinphoneChatMessage *msg) {
	return msg->chat_room;
}

SalOp * linphone_chat_message_get_sal_op(const LinphoneChatMessage *msg) {
	return msg->op;
}

void linphone_chat_message_set_sal_op(LinphoneChatMessage *msg, SalOp *op) {
	msg->op = op;
}

void linphone_chat_message_set_chat_room(LinphoneChatMessage *msg, LinphoneChatRoom *room) {
	msg->chat_room = room;
}

const char *linphone_chat_message_get_file_transfer_filepath(LinphoneChatMessage *msg) {
	return msg->file_transfer_filepath;
}

// =============================================================================

void linphone_chat_message_update_state(LinphoneChatMessage *msg, LinphoneChatMessageState new_state) {
	linphone_chat_message_set_state(msg, new_state);
	linphone_chat_message_store_state(msg);

	if (msg->state == LinphoneChatMessageStateDelivered || msg->state == LinphoneChatMessageStateNotDelivered) {
		L_GET_PRIVATE_FROM_C_OBJECT(msg->chat_room, ChatRoom)->moveTransientMessageToWeakMessages(msg);
	}
}

void _linphone_chat_message_resend(LinphoneChatMessage *msg, bool_t ref_msg) {
	LinphoneChatMessageState state = linphone_chat_message_get_state(msg);
	LinphoneChatRoom *cr;

	if (state != LinphoneChatMessageStateNotDelivered) {
		ms_warning("Cannot resend chat message in state %s", linphone_chat_message_state_to_string(state));
		return;
	}

	cr = linphone_chat_message_get_chat_room(msg);
	if (ref_msg) linphone_chat_message_ref(msg);
	L_GET_CPP_PTR_FROM_C_OBJECT(cr)->sendMessage(msg);
}

void linphone_chat_message_resend(LinphoneChatMessage *msg) {
	_linphone_chat_message_resend(msg, FALSE);
}

void linphone_chat_message_resend_2(LinphoneChatMessage *msg) {
	_linphone_chat_message_resend(msg, TRUE);
}

static char *linphone_chat_message_create_imdn_xml(LinphoneChatMessage *cm, ImdnType imdn_type, LinphoneReason reason) {
	xmlBufferPtr buf;
	xmlTextWriterPtr writer;
	int err;
	char *content = NULL;
	char *datetime = NULL;
	const char *message_id;

	/* Check that the chat message has a message id */
	message_id = linphone_chat_message_get_message_id(cm);
	if (message_id == NULL) return NULL;

	buf = xmlBufferCreate();
	if (buf == NULL) {
		ms_error("Error creating the XML buffer");
		return content;
	}
	writer = xmlNewTextWriterMemory(buf, 0);
	if (writer == NULL) {
		ms_error("Error creating the XML writer");
		return content;
	}

	datetime = linphone_timestamp_to_rfc3339_string(linphone_chat_message_get_time(cm));
	err = xmlTextWriterStartDocument(writer, "1.0", "UTF-8", NULL);
	if (err >= 0) {
		err = xmlTextWriterStartElementNS(writer, NULL, (const xmlChar *)"imdn",
										  (const xmlChar *)"urn:ietf:params:xml:ns:imdn");
	}
	if ((err >= 0) && (reason != LinphoneReasonNone)) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xmlns", (const xmlChar *)"linphoneimdn", NULL, (const xmlChar *)"http://www.linphone.org/xsds/imdn.xsd");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"message-id", (const xmlChar *)message_id);
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"datetime", (const xmlChar *)datetime);
	}
	if (err >= 0) {
		if (imdn_type == ImdnTypeDelivery) {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"delivery-notification");
		} else {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"display-notification");
		}
	}
	if (err >= 0) {
		err = xmlTextWriterStartElement(writer, (const xmlChar *)"status");
	}
	if (err >= 0) {
		if (reason == LinphoneReasonNone) {
			if (imdn_type == ImdnTypeDelivery) {
				err = xmlTextWriterStartElement(writer, (const xmlChar *)"delivered");
			} else {
				err = xmlTextWriterStartElement(writer, (const xmlChar *)"displayed");
			}
		} else {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"error");
		}
	}
	if (err >= 0) {
		/* Close the "delivered", "displayed" or "error" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if ((err >= 0) && (reason != LinphoneReasonNone)) {
		err = xmlTextWriterStartElementNS(writer, (const xmlChar *)"linphoneimdn", (const xmlChar *)"reason", NULL);
		if (err >= 0) {
			char codestr[16];
			snprintf(codestr, 16, "%d", linphone_reason_to_error_code(reason));
			err = xmlTextWriterWriteAttribute(writer, (const xmlChar *)"code", (const xmlChar *)codestr);
		}
		if (err >= 0) {
			err = xmlTextWriterWriteString(writer, (const xmlChar *)linphone_reason_to_string(reason));
		}
		if (err >= 0) {
			err = xmlTextWriterEndElement(writer);
		}
	}
	if (err >= 0) {
		/* Close the "status" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		/* Close the "delivery-notification" or "display-notification" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		/* Close the "imdn" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		err = xmlTextWriterEndDocument(writer);
	}
	if (err > 0) {
		/* xmlTextWriterEndDocument returns the size of the content. */
		content = ms_strdup((char *)buf->content);
	}
	xmlFreeTextWriter(writer);
	xmlBufferFree(buf);
	ms_free(datetime);
	return content;
}

void linphone_chat_message_send_imdn(LinphoneChatMessage *cm, ImdnType imdn_type, LinphoneReason reason) {
	char *content = linphone_chat_message_create_imdn_xml(cm, imdn_type, reason);
	if (content) {
		L_GET_PRIVATE_FROM_C_OBJECT(linphone_chat_message_get_chat_room(cm), ChatRoom)->sendImdn(content, reason);
		ms_free(content);
	}
}

void linphone_chat_message_send_delivery_notification(LinphoneChatMessage *cm, LinphoneReason reason) {
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(cm);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(lc);
	if (linphone_im_notif_policy_get_send_imdn_delivered(policy) == TRUE) {
		linphone_chat_message_send_imdn(cm, ImdnTypeDelivery, reason);
	}
}

void linphone_chat_message_send_display_notification(LinphoneChatMessage *cm) {
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(cm);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(lc);
	if (linphone_im_notif_policy_get_send_imdn_displayed(policy) == TRUE) {
		linphone_chat_message_send_imdn(cm, ImdnTypeDisplay, LinphoneReasonNone);
	}
}LinphoneStatus linphone_chat_message_put_char(LinphoneChatMessage *msg, uint32_t character) {
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(msg);
	if (linphone_core_realtime_text_enabled(linphone_chat_room_get_core(cr))) {
		std::shared_ptr<LinphonePrivate::RealTimeTextChatRoom> rttcr =
			std::static_pointer_cast<LinphonePrivate::RealTimeTextChatRoom>(L_GET_CPP_PTR_FROM_C_OBJECT(cr));
		LinphoneCall *call = rttcr->getCall();
		LinphoneCore *lc = rttcr->getCore();
		const uint32_t new_line = 0x2028;
		const uint32_t crlf = 0x0D0A;
		const uint32_t lf = 0x0A;

		if (!call || !linphone_call_get_stream(call, LinphoneStreamTypeText)) {
			return -1;
		}

		if (character == new_line || character == crlf || character == lf) {
			if (lc && lp_config_get_int(lc->config, "misc", "store_rtt_messages", 1) == 1) {
				ms_debug("New line sent, forge a message with content %s", msg->message);
				msg->time = ms_time(0);
				msg->state = LinphoneChatMessageStateDisplayed;
				msg->dir = LinphoneChatMessageOutgoing;
				if (msg->from) linphone_address_unref(msg->from);
				msg->from = linphone_address_new(linphone_core_get_identity(lc));
				msg->storage_id = linphone_chat_message_store(msg);
				ms_free(msg->message);
				msg->message = NULL;
			}
		} else {
			char *value = LinphonePrivate::Utils::utf8ToChar(character);
			msg->message = ms_strcat_printf(msg->message, value);
			ms_debug("Sent RTT character: %s (%lu), pending text is %s", value, (unsigned long)character, msg->message);
			delete value;
		}

		text_stream_putchar32(reinterpret_cast<TextStream *>(linphone_call_get_stream(call, LinphoneStreamTypeText)), character);
		return 0;
	}
	return -1;
}

const char *linphone_chat_message_state_to_string(const LinphoneChatMessageState state) {
	switch (state) {
	case LinphoneChatMessageStateIdle:
		return "LinphoneChatMessageStateIdle";
	case LinphoneChatMessageStateInProgress:
		return "LinphoneChatMessageStateInProgress";
	case LinphoneChatMessageStateDelivered:
		return "LinphoneChatMessageStateDelivered";
	case LinphoneChatMessageStateNotDelivered:
		return "LinphoneChatMessageStateNotDelivered";
	case LinphoneChatMessageStateFileTransferError:
		return "LinphoneChatMessageStateFileTransferError";
	case LinphoneChatMessageStateFileTransferDone:
		return "LinphoneChatMessageStateFileTransferDone ";
	case LinphoneChatMessageStateDeliveredToUser:
		return "LinphoneChatMessageStateDeliveredToUser";
	case LinphoneChatMessageStateDisplayed:
		return "LinphoneChatMessageStateDisplayed";
	}
	return NULL;
}

const LinphoneAddress *linphone_chat_message_get_peer_address(LinphoneChatMessage *msg) {
	return linphone_chat_room_get_peer_address(msg->chat_room);
}

bool_t linphone_chat_message_is_file_transfer(const LinphoneChatMessage *msg) {
	return LinphonePrivate::ContentType::isFileTransfer(msg->content_type);
}

bool_t linphone_chat_message_is_text(const LinphoneChatMessage *msg) {
	return LinphonePrivate::ContentType::isText(msg->content_type);
}

bool_t linphone_chat_message_get_to_be_stored(const LinphoneChatMessage *msg) {
	return msg->to_be_stored;
}

void linphone_chat_message_set_to_be_stored(LinphoneChatMessage *msg, bool_t to_be_stored) {
	msg->to_be_stored = to_be_stored;
}

LinphoneAddress *linphone_chat_message_get_local_address(const LinphoneChatMessage *msg) {
	return msg->dir == LinphoneChatMessageOutgoing ? msg->from : msg->to;
}

LinphoneChatMessageState linphone_chat_message_get_state(const LinphoneChatMessage *msg) {
	return msg->state;
}

void linphone_chat_message_add_custom_header(LinphoneChatMessage *msg, const char *header_name,
											 const char *header_value) {
	msg->sal_custom_headers = sal_custom_header_append(msg->sal_custom_headers, header_name, header_value);
}

const char *linphone_chat_message_get_custom_header(LinphoneChatMessage *msg, const char *header_name) {
	return sal_custom_header_find(msg->sal_custom_headers, header_name);
}

void linphone_chat_message_remove_custom_header(LinphoneChatMessage *msg, const char *header_name) {
	msg->sal_custom_headers = sal_custom_header_remove(msg->sal_custom_headers, header_name);
}

bool_t linphone_chat_message_is_read(LinphoneChatMessage *msg) {
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(msg);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(lc);
	if ((linphone_im_notif_policy_get_recv_imdn_displayed(policy) == TRUE) && (msg->state == LinphoneChatMessageStateDisplayed)) return TRUE;
	if ((linphone_im_notif_policy_get_recv_imdn_delivered(policy) == TRUE) && (msg->state == LinphoneChatMessageStateDeliveredToUser || msg->state == LinphoneChatMessageStateDisplayed)) return TRUE;
	return (msg->state == LinphoneChatMessageStateDelivered || msg->state == LinphoneChatMessageStateDisplayed || msg->state == LinphoneChatMessageStateDeliveredToUser);
}

LinphoneChatMessage *linphone_chat_message_clone(const LinphoneChatMessage *msg) {
	LinphoneChatMessage *new_message = linphone_chat_room_create_message(msg->chat_room, msg->message);
	if (msg->external_body_url)
		new_message->external_body_url = ms_strdup(msg->external_body_url);
	if (msg->appdata)
		new_message->appdata = ms_strdup(msg->appdata);
	new_message->message_state_changed_cb = msg->message_state_changed_cb;
	new_message->message_state_changed_user_data = msg->message_state_changed_user_data;
	new_message->message_userdata = msg->message_userdata;
	new_message->time = msg->time;
	new_message->state = msg->state;
	new_message->storage_id = msg->storage_id;
	if (msg->from)
		new_message->from = linphone_address_clone(msg->from);
	if (msg->file_transfer_filepath)
		new_message->file_transfer_filepath = ms_strdup(msg->file_transfer_filepath);
	if (msg->file_transfer_information)
		new_message->file_transfer_information = linphone_content_copy(msg->file_transfer_information);
	return new_message;
}

void linphone_chat_message_deactivate(LinphoneChatMessage *msg){
	if (msg->file_transfer_information != NULL) {
		_linphone_chat_message_cancel_file_transfer(msg, FALSE);
	}
	/*mark the chat msg as orphan (it has no chat room anymore)*/
	msg->chat_room = NULL;
}

void linphone_chat_message_release(LinphoneChatMessage *msg) {
	linphone_chat_message_deactivate(msg);
	linphone_chat_message_unref(msg);
}

const LinphoneErrorInfo *linphone_chat_message_get_error_info(const LinphoneChatMessage *msg) {
	if (!msg->ei) ((LinphoneChatMessage*)msg)->ei = linphone_error_info_new(); /*let's do it mutable*/
	linphone_error_info_from_sal_op(msg->ei, msg->op);
	return msg->ei;
}

LinphoneReason linphone_chat_message_get_reason(LinphoneChatMessage *msg) {
	return linphone_error_info_get_reason(linphone_chat_message_get_error_info(msg));
}

LinphoneChatMessageCbs *linphone_chat_message_get_callbacks(const LinphoneChatMessage *msg) {
	return msg->cbs;
}

static bool_t file_transfer_in_progress_and_valid(LinphoneChatMessage* msg) {
	return (linphone_chat_message_get_chat_room(msg) &&
		linphone_chat_room_get_core(linphone_chat_message_get_chat_room(msg)) &&
		linphone_chat_message_get_http_request(msg) &&
		!belle_http_request_is_cancelled(linphone_chat_message_get_http_request(msg)));
}

static void _release_http_request(LinphoneChatMessage* msg) {
	if (linphone_chat_message_get_http_request(msg)) {
		belle_sip_object_unref(msg->http_request);
		msg->http_request = NULL;
		if (msg->http_listener){
			belle_sip_object_unref(msg->http_listener);
			msg->http_listener = NULL;
			// unhold the reference that the listener was holding on the message
			linphone_chat_message_unref(msg);
		}
	}
}

static void linphone_chat_message_process_io_error_upload(void *data, const belle_sip_io_error_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("I/O Error during file upload of msg [%p]", msg);
	linphone_chat_message_update_state(msg, LinphoneChatMessageStateNotDelivered);
	_release_http_request(msg);
	linphone_chat_room_remove_transient_message(msg->chat_room, msg);
	linphone_chat_message_unref(msg);
}

static void linphone_chat_message_process_auth_requested_upload(void *data, belle_sip_auth_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("Error during file upload: auth requested for msg [%p]", msg);
	linphone_chat_message_update_state(msg, LinphoneChatMessageStateNotDelivered);
	_release_http_request(msg);
	linphone_chat_room_remove_transient_message(msg->chat_room, msg);
	linphone_chat_message_unref(msg);
}

static void linphone_chat_message_process_io_error_download(void *data, const belle_sip_io_error_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("I/O Error during file download msg [%p]", msg);
	linphone_chat_message_update_state(msg, LinphoneChatMessageStateFileTransferError);
	_release_http_request(msg);
}

static void linphone_chat_message_process_auth_requested_download(void *data, belle_sip_auth_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("Error during file download : auth requested for msg [%p]", msg);
	linphone_chat_message_update_state(msg, LinphoneChatMessageStateFileTransferError);
	_release_http_request(msg);
}

static void linphone_chat_message_file_transfer_on_progress(belle_sip_body_handler_t *bh, belle_sip_message_t *m,
															void *data, size_t offset, size_t total) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	if (!file_transfer_in_progress_and_valid(msg)) {
		ms_warning("Cancelled request for %s msg [%p], ignoring %s", msg->chat_room?"":"ORPHAN", msg, __FUNCTION__);
		_release_http_request(msg);
		return;
	}
	if (linphone_chat_message_cbs_get_file_transfer_progress_indication(msg->cbs)) {
		linphone_chat_message_cbs_get_file_transfer_progress_indication(msg->cbs)(
			msg, msg->file_transfer_information, offset, total);
	} else {
		/* Legacy: call back given by application level */
		linphone_core_notify_file_transfer_progress_indication(linphone_chat_room_get_core(msg->chat_room), msg, msg->file_transfer_information,
															offset, total);
	}
}

static int on_send_body(belle_sip_user_body_handler_t *bh, belle_sip_message_t *m,
															void *data, size_t offset, uint8_t *buffer, size_t *size) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	LinphoneCore *lc = NULL;
	LinphoneImEncryptionEngine *imee = NULL;
	int retval = -1;

	if (!file_transfer_in_progress_and_valid(msg)) {
		if (msg->http_request) {
			ms_warning("Cancelled request for %s msg [%p], ignoring %s", msg->chat_room?"":"ORPHAN", msg, __FUNCTION__);
			_release_http_request(msg);
		}
		return BELLE_SIP_STOP;
	}

	lc = linphone_chat_room_get_core(msg->chat_room);
	/* if we've not reach the end of file yet, ask for more data */
	/* in case of file body handler, won't be called */
	if (msg->file_transfer_filepath == NULL && offset < linphone_content_get_size(msg->file_transfer_information)) {
		/* get data from call back */
		LinphoneChatMessageCbsFileTransferSendCb file_transfer_send_cb = linphone_chat_message_cbs_get_file_transfer_send(msg->cbs);
		if (file_transfer_send_cb) {
			LinphoneBuffer *lb = file_transfer_send_cb(msg, msg->file_transfer_information, offset, *size);
			if (lb == NULL) {
				*size = 0;
			} else {
				*size = linphone_buffer_get_size(lb);
				memcpy(buffer, linphone_buffer_get_content(lb), *size);
				linphone_buffer_unref(lb);
			}
		} else {
			/* Legacy */
			linphone_core_notify_file_transfer_send(lc, msg, msg->file_transfer_information, (char *)buffer, size);
		}
	}

	imee = linphone_core_get_im_encryption_engine(lc);
	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsUploadingFileCb cb_process_uploading_file = linphone_im_encryption_engine_cbs_get_process_uploading_file(imee_cbs);
		if (cb_process_uploading_file) {
			size_t max_size = *size;
			uint8_t *encrypted_buffer = (uint8_t *)ms_malloc0(max_size);
			retval = cb_process_uploading_file(imee, msg, offset, (const uint8_t *)buffer, size, encrypted_buffer);
			if (retval == 0) {
				if (*size > max_size) {
					ms_error("IM encryption engine process upload file callback returned a size bigger than the size of the buffer, so it will be truncated !");
					*size = max_size;
				}
				memcpy(buffer, encrypted_buffer, *size);
			}
			ms_free(encrypted_buffer);
		}
	}

	return retval <= 0 ? BELLE_SIP_CONTINUE : BELLE_SIP_STOP;
}

static void on_send_end(belle_sip_user_body_handler_t *bh, void *data) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	LinphoneCore *lc = linphone_chat_room_get_core(msg->chat_room);
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(lc);

	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsUploadingFileCb cb_process_uploading_file = linphone_im_encryption_engine_cbs_get_process_uploading_file(imee_cbs);
		if (cb_process_uploading_file) {
			cb_process_uploading_file(imee, msg, 0, NULL, NULL, NULL);
		}
	}
}

static void file_upload_end_background_task(LinphoneChatMessage *obj){
	if (obj->bg_task_id){
		ms_message("channel [%p]: ending file upload background task with id=[%lx].",obj,obj->bg_task_id);
		sal_end_background_task(obj->bg_task_id);
		obj->bg_task_id=0;
	}
}

static void file_upload_background_task_ended(LinphoneChatMessage *obj){
	ms_warning("channel [%p]: file upload background task has to be ended now, but work isn't finished.",obj);
	file_upload_end_background_task(obj);
}

static void file_upload_begin_background_task(LinphoneChatMessage *obj){
	if (obj->bg_task_id==0){
		obj->bg_task_id=sal_begin_background_task("file transfer upload",(void (*)(void*))file_upload_background_task_ended, obj);
		if (obj->bg_task_id) ms_message("channel [%p]: starting file upload background task with id=[%lx].",obj,obj->bg_task_id);
	}
}

static void linphone_chat_message_process_response_from_post_file(void *data, const belle_http_response_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;

	if (msg->http_request && !file_transfer_in_progress_and_valid(msg)) {
		ms_warning("Cancelled request for %s msg [%p], ignoring %s", msg->chat_room?"":"ORPHAN", msg, __FUNCTION__);
		_release_http_request(msg);
		return;
	}

	/* check the answer code */
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code == 204) { /* this is the reply to the first post to the server - an empty msg */
			/* start uploading the file */
			belle_sip_multipart_body_handler_t *bh;
			char *first_part_header;
			belle_sip_body_handler_t *first_part_bh;

			bool_t is_file_encryption_enabled = FALSE;
			LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(linphone_chat_room_get_core(msg->chat_room));
			if (imee) {
				LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb is_encryption_enabled_for_file_transfer_cb =
					linphone_im_encryption_engine_cbs_get_is_encryption_enabled_for_file_transfer(imee_cbs);
				if (is_encryption_enabled_for_file_transfer_cb) {
					is_file_encryption_enabled = is_encryption_enabled_for_file_transfer_cb(imee, msg->chat_room);
				}
			}
			/* shall we encrypt the file */
			if (is_file_encryption_enabled) {
				LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsGenerateFileTransferKeyCb generate_file_transfer_key_cb =
					linphone_im_encryption_engine_cbs_get_generate_file_transfer_key(imee_cbs);
				if (generate_file_transfer_key_cb) {
					generate_file_transfer_key_cb(imee, msg->chat_room, msg);
				}
				/* temporary storage for the Content-disposition header value : use a generic filename to not leak it
				* Actual filename stored in msg->file_transfer_information->name will be set in encrypted msg
				* sended to the  */
				first_part_header = belle_sip_strdup_printf("form-data; name=\"File\"; filename=\"filename.txt\"");
			} else {
				/* temporary storage for the Content-disposition header value */
				first_part_header = belle_sip_strdup_printf("form-data; name=\"File\"; filename=\"%s\"",
															linphone_content_get_name(msg->file_transfer_information));
			}

			/* create a user body handler to take care of the file and add the content disposition and content-type
			 * headers */
			first_part_bh = (belle_sip_body_handler_t *)belle_sip_user_body_handler_new(
					linphone_content_get_size(msg->file_transfer_information),
					linphone_chat_message_file_transfer_on_progress, NULL, NULL,
					on_send_body, on_send_end, msg);
			if (msg->file_transfer_filepath != NULL) {
				belle_sip_user_body_handler_t *body_handler = (belle_sip_user_body_handler_t *)first_part_bh;
				first_part_bh = (belle_sip_body_handler_t *)belle_sip_file_body_handler_new(msg->file_transfer_filepath,
					NULL, msg); // No need to add again the callback for progression, otherwise it will be called twice
				linphone_content_set_size(msg->file_transfer_information, belle_sip_file_body_handler_get_file_size((belle_sip_file_body_handler_t *)first_part_bh));
				belle_sip_file_body_handler_set_user_body_handler((belle_sip_file_body_handler_t *)first_part_bh, body_handler);
			} else if (linphone_content_get_buffer(msg->file_transfer_information) != NULL) {
				first_part_bh = (belle_sip_body_handler_t *)belle_sip_memory_body_handler_new_from_buffer(
					linphone_content_get_buffer(msg->file_transfer_information),
					linphone_content_get_size(msg->file_transfer_information), linphone_chat_message_file_transfer_on_progress, msg);
			}

			belle_sip_body_handler_add_header(first_part_bh,
											  belle_sip_header_create("Content-disposition", first_part_header));
			belle_sip_free(first_part_header);
			belle_sip_body_handler_add_header(first_part_bh,
											  (belle_sip_header_t *)belle_sip_header_content_type_create(
												  linphone_content_get_type(msg->file_transfer_information),
												  linphone_content_get_subtype(msg->file_transfer_information)));

			/* insert it in a multipart body handler which will manage the boundaries of multipart msg */
			bh = belle_sip_multipart_body_handler_new(linphone_chat_message_file_transfer_on_progress, msg, first_part_bh, NULL);

			linphone_chat_message_ref(msg);
			_release_http_request(msg);
			file_upload_begin_background_task(msg);
			linphone_chat_room_upload_file(msg);
			belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(msg->http_request), BELLE_SIP_BODY_HANDLER(bh));
			linphone_chat_message_unref(msg);
		} else if (code == 200) { /* file has been uploaded correctly, get server reply and send it */
			const char *body = belle_sip_message_get_body((belle_sip_message_t *)event->response);
			if (body && strlen(body) > 0) {
				/* if we have an encryption key for the file, we must insert it into the msg and restore the correct
				 * filename */
				const char *content_key = linphone_content_get_key(msg->file_transfer_information);
				size_t content_key_size = linphone_content_get_key_size(msg->file_transfer_information);
				if (content_key != NULL) {
					/* parse the msg body */
					xmlDocPtr xmlMessageBody = xmlParseDoc((const xmlChar *)body);

					xmlNodePtr cur = xmlDocGetRootElement(xmlMessageBody);
					if (cur != NULL) {
						cur = cur->xmlChildrenNode;
						while (cur != NULL) {
							if (!xmlStrcmp(cur->name, (const xmlChar *)"file-info")) { /* we found a file info node, check
																						  it has a type="file" attribute */
								xmlChar *typeAttribute = xmlGetProp(cur, (const xmlChar *)"type");
								if (!xmlStrcmp(typeAttribute,
											   (const xmlChar *)"file")) { /* this is the node we are looking for : add a
																			  file-key children node */
									xmlNodePtr fileInfoNodeChildren =
										cur
											->xmlChildrenNode; /* need to parse the children node to update the file-name
																  one */
									/* convert key to base64 */
									size_t b64Size = b64::b64_encode(NULL, content_key_size, NULL, 0);
									char *keyb64 = (char *)ms_malloc0(b64Size + 1);
									int xmlStringLength;

									b64Size = b64::b64_encode(content_key, content_key_size, keyb64, b64Size);
									keyb64[b64Size] = '\0'; /* libxml need a null terminated string */

									/* add the node containing the key to the file-info node */
									xmlNewTextChild(cur, NULL, (const xmlChar *)"file-key", (const xmlChar *)keyb64);
									xmlFree(typeAttribute);
									ms_free(keyb64);

									/* look for the file-name node and update its content */
									while (fileInfoNodeChildren != NULL) {
										if (!xmlStrcmp(
												fileInfoNodeChildren->name,
												(const xmlChar *)"file-name")) { /* we found a the file-name node, update
																					its content with the real filename */
											/* update node content */
											xmlNodeSetContent(fileInfoNodeChildren,
															  (const xmlChar *)(linphone_content_get_name(
																  msg->file_transfer_information)));
											break;
										}
										fileInfoNodeChildren = fileInfoNodeChildren->next;
									}

									/* dump the xml into msg->message */
									xmlDocDumpFormatMemoryEnc(xmlMessageBody, (xmlChar **)&msg->message, &xmlStringLength,
															  "UTF-8", 0);

									break;
								}
								xmlFree(typeAttribute);
							}
							cur = cur->next;
						}
					}
					xmlFreeDoc(xmlMessageBody);
				} else { /* no encryption key, transfer in plain, just copy the msg sent by server */
					msg->message = ms_strdup(body);
				}
				linphone_chat_message_set_content_type(msg, "application/vnd.gsma.rcs-ft-http+xml");
				linphone_chat_message_ref(msg);
				linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferDone);
				_release_http_request(msg);
				L_GET_CPP_PTR_FROM_C_OBJECT(msg->chat_room)->sendMessage(msg);
				file_upload_end_background_task(msg);
				linphone_chat_message_unref(msg);
			} else {
				ms_warning("Received empty response from server, file transfer failed");
				linphone_chat_message_update_state(msg, LinphoneChatMessageStateNotDelivered);
				_release_http_request(msg);
				file_upload_end_background_task(msg);
				linphone_chat_message_unref(msg);
			}
		} else {
			ms_warning("Unhandled HTTP code response %d for file transfer", code);
			linphone_chat_message_update_state(msg, LinphoneChatMessageStateNotDelivered);
			_release_http_request(msg);
			file_upload_end_background_task(msg);
			linphone_chat_message_unref(msg);
		}
	}
}

LinphoneContent *linphone_chat_message_get_file_transfer_information(LinphoneChatMessage *msg) {
	return msg->file_transfer_information;
}

static void on_recv_body(belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, void *data, size_t offset, uint8_t *buffer, size_t size) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	LinphoneCore *lc = NULL;
	LinphoneImEncryptionEngine *imee = NULL;
	int retval = -1;
	uint8_t *decrypted_buffer = NULL;

	if (!msg->chat_room) {
		linphone_chat_message_cancel_file_transfer(msg);
		return;
	}
	lc = linphone_chat_room_get_core(msg->chat_room);

	if (lc == NULL){
		return; /*might happen during linphone_core_destroy()*/
	}

	if (!msg->http_request || belle_http_request_is_cancelled(msg->http_request)) {
		ms_warning("Cancelled request for msg [%p], ignoring %s", msg, __FUNCTION__);
		return;
	}

	/* first call may be with a zero size, ignore it */
	if (size == 0) {
		return;
	}

	decrypted_buffer = (uint8_t *)ms_malloc0(size);
	imee = linphone_core_get_im_encryption_engine(lc);
	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsDownloadingFileCb cb_process_downloading_file = linphone_im_encryption_engine_cbs_get_process_downloading_file(imee_cbs);
		if (cb_process_downloading_file) {
			retval = cb_process_downloading_file(imee, msg, offset, (const uint8_t *)buffer, size, decrypted_buffer);
			if (retval == 0) {
				memcpy(buffer, decrypted_buffer, size);
			}
		}
	}
	ms_free(decrypted_buffer);

	if (retval <= 0) {
		if (msg->file_transfer_filepath == NULL) {
			if (linphone_chat_message_cbs_get_file_transfer_recv(msg->cbs)) {
				LinphoneBuffer *lb = linphone_buffer_new_from_data(buffer, size);
				linphone_chat_message_cbs_get_file_transfer_recv(msg->cbs)(msg, msg->file_transfer_information, lb);
				linphone_buffer_unref(lb);
			} else {
				/* Legacy: call back given by application level */
				linphone_core_notify_file_transfer_recv(lc, msg, msg->file_transfer_information, (const char *)buffer, size);
			}
		}
	} else {
		ms_warning("File transfer decrypt failed with code %d", (int)retval);
		linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferError);
	}

	return;
}

static void on_recv_end(belle_sip_user_body_handler_t *bh, void *data) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	LinphoneCore *lc = linphone_chat_room_get_core(msg->chat_room);
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(lc);
	int retval = -1;

	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsDownloadingFileCb cb_process_downloading_file = linphone_im_encryption_engine_cbs_get_process_downloading_file(imee_cbs);
		if (cb_process_downloading_file) {
			retval = cb_process_downloading_file(imee, msg, 0, NULL, 0, NULL);
		}
	}

	if (retval <= 0) {
		if (msg->file_transfer_filepath == NULL) {
			if (linphone_chat_message_cbs_get_file_transfer_recv(msg->cbs)) {
				LinphoneBuffer *lb = linphone_buffer_new();
				linphone_chat_message_cbs_get_file_transfer_recv(msg->cbs)(msg, msg->file_transfer_information, lb);
				linphone_buffer_unref(lb);
			} else {
				/* Legacy: call back given by application level */
				linphone_core_notify_file_transfer_recv(lc, msg, msg->file_transfer_information, NULL, 0);
			}
		}
	}

	if (retval <= 0 && linphone_chat_message_get_state(msg) != LinphoneChatMessageStateFileTransferError) {
		linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferDone);
	}
}

static LinphoneContent *linphone_chat_create_file_transfer_information_from_headers(const belle_sip_message_t *m) {
	LinphoneContent *content = linphone_content_new();

	belle_sip_header_content_length_t *content_length_hdr =
		BELLE_SIP_HEADER_CONTENT_LENGTH(belle_sip_message_get_header(m, "Content-Length"));
	belle_sip_header_content_type_t *content_type_hdr =
		BELLE_SIP_HEADER_CONTENT_TYPE(belle_sip_message_get_header(m, "Content-Type"));
	const char *type = NULL, *subtype = NULL;

	linphone_content_set_name(content, "");

	if (content_type_hdr) {
		type = belle_sip_header_content_type_get_type(content_type_hdr);
		subtype = belle_sip_header_content_type_get_subtype(content_type_hdr);
		ms_message("Extracted content type %s / %s from header", type ? type : "", subtype ? subtype : "");
		if (type)
			linphone_content_set_type(content, type);
		if (subtype)
			linphone_content_set_subtype(content, subtype);
	}

	if (content_length_hdr) {
		linphone_content_set_size(content, belle_sip_header_content_length_get_content_length(content_length_hdr));
		ms_message("Extracted content length %i from header", (int)linphone_content_get_size(content));
	}

	return content;
}

static void linphone_chat_process_response_headers_from_get_file(void *data, const belle_http_response_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	if (event->response) {
		/*we are receiving a response, set a specific body handler to acquire the response.
		 * if not done, belle-sip will create a memory body handler, the default*/
		belle_sip_message_t *response = BELLE_SIP_MESSAGE(event->response);
		belle_sip_body_handler_t *body_handler = NULL;
		size_t body_size = 0;

		if (msg->file_transfer_information == NULL) {
			ms_warning("No file transfer information for msg %p: creating...", msg);
			msg->file_transfer_information = linphone_chat_create_file_transfer_information_from_headers(response);
		}

		if (msg->file_transfer_information) {
			body_size = linphone_content_get_size(msg->file_transfer_information);
		}


		body_handler = (belle_sip_body_handler_t *)belle_sip_user_body_handler_new(body_size, linphone_chat_message_file_transfer_on_progress, NULL, on_recv_body, NULL, on_recv_end, msg);
		if (msg->file_transfer_filepath != NULL) {
			belle_sip_user_body_handler_t *bh = (belle_sip_user_body_handler_t *)body_handler;
			body_handler = (belle_sip_body_handler_t *)belle_sip_file_body_handler_new(
				msg->file_transfer_filepath, linphone_chat_message_file_transfer_on_progress, msg);
			if (belle_sip_body_handler_get_size((belle_sip_body_handler_t *)body_handler) == 0) {
				/* If the size of the body has not been initialized from the file stat, use the one from the
				 * file_transfer_information. */
				belle_sip_body_handler_set_size((belle_sip_body_handler_t *)body_handler, body_size);
			}
			belle_sip_file_body_handler_set_user_body_handler((belle_sip_file_body_handler_t *)body_handler, bh);
		}
		belle_sip_message_set_body_handler((belle_sip_message_t *)event->response, body_handler);
	}
}

static void linphone_chat_process_response_from_get_file(void *data, const belle_http_response_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	/* check the answer code */
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code >= 400 && code < 500) {
			ms_warning("File transfer failed with code %d", code);
			linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferError);
		} else if (code != 200) {
			ms_warning("Unhandled HTTP code response %d for file transfer", code);
		}
		_release_http_request(msg);
	}
}

int _linphone_chat_room_start_http_transfer(LinphoneChatMessage *msg, const char* url, const char* action, const belle_http_request_listener_callbacks_t *cbs) {
	belle_generic_uri_t *uri = NULL;
	const char* ua = linphone_core_get_user_agent(linphone_chat_room_get_core(msg->chat_room));

	if (url == NULL) {
		ms_warning("Cannot process file transfer msg: no file remote URI configured.");
		goto error;
	}
	uri = belle_generic_uri_parse(url);
	if (uri == NULL || belle_generic_uri_get_host(uri)==NULL) {
		ms_warning("Cannot process file transfer msg: incorrect file remote URI configured '%s'.", url);
		goto error;
	}

	msg->http_request = belle_http_request_create(action, uri, belle_sip_header_create("User-Agent", ua), NULL);

	if (msg->http_request == NULL) {
		ms_warning("Could not create http request for uri %s", url);
		goto error;
	}
	/* keep a reference to the http request to be able to cancel it during upload */
	belle_sip_object_ref(msg->http_request);

	/* give msg to listener to be able to start the actual file upload when server answer a 204 No content */
	msg->http_listener = belle_http_request_listener_create_from_callbacks(cbs, linphone_chat_message_ref(msg));
	belle_http_provider_send_request(linphone_chat_room_get_core(msg->chat_room)->http_provider, msg->http_request, msg->http_listener);
	return 0;
error:
	if (uri) {
		belle_sip_object_unref(uri);
	}
	return -1;
}

int linphone_chat_room_upload_file(LinphoneChatMessage *msg) {
	belle_http_request_listener_callbacks_t cbs = {0};
	int err;

	if (msg->http_request){
		ms_error("linphone_chat_room_upload_file(): there is already an upload in progress.");
		return -1;
	}

	cbs.process_response = linphone_chat_message_process_response_from_post_file;
	cbs.process_io_error = linphone_chat_message_process_io_error_upload;
	cbs.process_auth_requested = linphone_chat_message_process_auth_requested_upload;
	err = _linphone_chat_room_start_http_transfer(msg, linphone_core_get_file_transfer_server(linphone_chat_room_get_core(msg->chat_room)), "POST", &cbs);
	if (err == -1){
		linphone_chat_message_set_state(msg, LinphoneChatMessageStateNotDelivered);
	}
	return err;
}

LinphoneStatus linphone_chat_message_download_file(LinphoneChatMessage *msg) {
	belle_http_request_listener_callbacks_t cbs = {0};
	int err;

	if (msg->http_request){
		ms_error("linphone_chat_message_download_file(): there is already a download in progress");
		return -1;
	}
	cbs.process_response_headers = linphone_chat_process_response_headers_from_get_file;
	cbs.process_response = linphone_chat_process_response_from_get_file;
	cbs.process_io_error = linphone_chat_message_process_io_error_download;
	cbs.process_auth_requested = linphone_chat_message_process_auth_requested_download;
	err = _linphone_chat_room_start_http_transfer(msg, msg->external_body_url, "GET", &cbs);
	if (err == -1) return -1;
	/* start the download, status is In Progress */
	linphone_chat_message_set_state(msg, LinphoneChatMessageStateInProgress);
	return 0;
}

void linphone_chat_message_start_file_download(LinphoneChatMessage *msg,
											   LinphoneChatMessageStateChangedCb status_cb, void *ud) {
	msg->message_state_changed_cb = status_cb;
	msg->message_state_changed_user_data = ud;
	linphone_chat_message_download_file(msg);
}

void _linphone_chat_message_cancel_file_transfer(LinphoneChatMessage *msg, bool_t unref) {
	if (msg->http_request) {
		if (msg->state == LinphoneChatMessageStateInProgress) {
			linphone_chat_message_set_state(msg, LinphoneChatMessageStateNotDelivered);
		}
		if (!belle_http_request_is_cancelled(msg->http_request)) {
			if (msg->chat_room) {
				ms_message("Canceling file transfer %s - msg [%p] chat room[%p]"
								, (msg->external_body_url == NULL) ? linphone_core_get_file_transfer_server(linphone_chat_room_get_core(msg->chat_room)) : msg->external_body_url
								, msg
								, msg->chat_room);
				belle_http_provider_cancel_request(linphone_chat_room_get_core(msg->chat_room)->http_provider, msg->http_request);
				if ((msg->dir == LinphoneChatMessageOutgoing) && unref) {
					// must release it
					linphone_chat_message_unref(msg);
				}
			} else {
				ms_message("Warning: http request still running for ORPHAN msg [%p]: this is a memory leak", msg);
			}
		}
		_release_http_request(msg);
	} else {
		ms_message("No existing file transfer - nothing to cancel");
	}
}

void linphone_chat_message_cancel_file_transfer(LinphoneChatMessage *msg) {
	_linphone_chat_message_cancel_file_transfer(msg, TRUE);
}

void linphone_chat_message_set_file_transfer_filepath(LinphoneChatMessage *msg, const char *filepath) {
	if (msg->file_transfer_filepath != NULL) {
		ms_free(msg->file_transfer_filepath);
	}
	msg->file_transfer_filepath = ms_strdup(filepath);
}
