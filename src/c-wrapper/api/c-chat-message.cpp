/*
 * c-chat-message.cpp
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

#include "linphone/api/c-chat-message.h"
#include "linphone/utils/utils.h"
#include "linphone/wrapper_utils.h"

#include "ortp/b64.h"

#include "c-wrapper/c-wrapper.h"
#include "address/address.h"
#include "content/content.h"
#include "content/content-type.h"
#include "chat/chat-message/chat-message-p.h"
#include "chat/chat-room/chat-room-p.h"
#include "chat/chat-room/real-time-text-chat-room-p.h"
#include "chat/notification/imdn.h"

// =============================================================================

using namespace std;

static void _linphone_chat_message_constructor (LinphoneChatMessage *msg);
static void _linphone_chat_message_destructor (LinphoneChatMessage *msg);

L_DECLARE_C_OBJECT_IMPL_WITH_XTORS(ChatMessage,
	_linphone_chat_message_constructor, _linphone_chat_message_destructor,
	LinphoneChatMessageCbs *cbs;
	LinphoneAddress *from; // cache for shared_ptr<Address>
	LinphoneAddress *to; // cache for shared_ptr<Address>
	LinphoneChatMessageStateChangedCb message_state_changed_cb;
	void* message_state_changed_user_data;
	mutable char *contentTypeCache;
)

static void _linphone_chat_message_constructor (LinphoneChatMessage *msg) {
	msg->cbs = linphone_chat_message_cbs_new();
}

static void _linphone_chat_message_destructor (LinphoneChatMessage *msg) {
	linphone_chat_message_cbs_unref(msg->cbs);
	msg->cbs = nullptr;
	if (msg->from)
		linphone_address_unref(msg->from);
	if (msg->to)
		linphone_address_unref(msg->to);
	if (msg->contentTypeCache)
		ms_free(msg->contentTypeCache);
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
	return L_GET_USER_DATA_FROM_C_OBJECT(msg);
}

void linphone_chat_message_set_user_data (LinphoneChatMessage *msg, void *ud) {
	L_SET_USER_DATA_FROM_C_OBJECT(msg, ud);
}

LinphoneChatMessageCbs *linphone_chat_message_get_callbacks(const LinphoneChatMessage *msg) {
	return msg->cbs;
}

// =============================================================================
// Getter and setters
// =============================================================================

LinphoneChatRoom *linphone_chat_message_get_chat_room(const LinphoneChatMessage *msg) {
	return L_GET_C_BACK_PTR(L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getChatRoom());
}

const char *linphone_chat_message_get_external_body_url(const LinphoneChatMessage *msg) {
	return L_STRING_TO_C(L_GET_PRIVATE_FROM_C_OBJECT(msg)->getExternalBodyUrl());
}

void linphone_chat_message_set_external_body_url(LinphoneChatMessage *msg, const char *url) {

}

time_t linphone_chat_message_get_time(const LinphoneChatMessage *msg) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getTime();
}

void linphone_chat_message_set_time(LinphoneChatMessage *msg, time_t time) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setTime(time);
}

bool_t linphone_chat_message_is_secured(LinphoneChatMessage *msg) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->isSecured();
}

void linphone_chat_message_set_is_secured(LinphoneChatMessage *msg, bool_t secured) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->setIsSecured(!!secured);
}

bool_t linphone_chat_message_is_outgoing(LinphoneChatMessage *msg) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getDirection() == LinphonePrivate::ChatMessage::Direction::Outgoing;
}

LinphoneChatMessageDir linphone_chat_message_get_direction(const LinphoneChatMessage *msg) {
	return ((LinphoneChatMessageDir)L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getDirection());
}

void linphone_chat_message_set_incoming(LinphoneChatMessage *msg) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setDirection(LinphonePrivate::ChatMessage::Direction::Incoming);
}

void linphone_chat_message_set_outgoing(LinphoneChatMessage *msg) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setDirection(LinphonePrivate::ChatMessage::Direction::Outgoing);
}

unsigned int linphone_chat_message_get_storage_id(LinphoneChatMessage *msg) {
	return L_GET_PRIVATE_FROM_C_OBJECT(msg)->getStorageId();
}

LinphoneChatMessageState linphone_chat_message_get_state(const LinphoneChatMessage *msg) {
	return ((LinphoneChatMessageState)L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getState());
}

void linphone_chat_message_set_state(LinphoneChatMessage *msg, LinphoneChatMessageState state) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setState((LinphonePrivate::ChatMessage::State)state);
}

const char* linphone_chat_message_get_message_id(const LinphoneChatMessage *msg) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getImdnMessageId().c_str();
}

void linphone_chat_message_set_message_id(LinphoneChatMessage *msg, char *id) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->setImdnMessageId(L_C_TO_STRING(id));
}

void linphone_chat_message_set_storage_id(LinphoneChatMessage *msg, unsigned int id) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setStorageId(id);
}

bool_t linphone_chat_message_is_read(LinphoneChatMessage *msg) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->isRead();
}

const char *linphone_chat_message_get_appdata(const LinphoneChatMessage *msg) {
	return L_STRING_TO_C(L_GET_PRIVATE_FROM_C_OBJECT(msg)->getAppdata());
}

void linphone_chat_message_set_appdata(LinphoneChatMessage *msg, const char *data) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setAppdata(L_C_TO_STRING(data));
}

const LinphoneAddress *linphone_chat_message_get_from_address(LinphoneChatMessage *msg) {
	if (msg->from)
		linphone_address_unref(msg->from);
	msg->from = linphone_address_new(L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getFromAddress().asString().c_str());
	return msg->from;
}

const LinphoneAddress *linphone_chat_message_get_to_address(LinphoneChatMessage *msg) {
	if (msg->to)
		linphone_address_unref(msg->to);
	msg->to = linphone_address_new(L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getToAddress().asString().c_str());
	return msg->to;
}

const char *linphone_chat_message_get_file_transfer_filepath(LinphoneChatMessage *msg) {
	return L_STRING_TO_C(L_GET_PRIVATE_FROM_C_OBJECT(msg)->getFileTransferFilepath());
}

void linphone_chat_message_set_file_transfer_filepath(LinphoneChatMessage *msg, const char *filepath) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setFileTransferFilepath(L_C_TO_STRING(filepath));
}

belle_http_request_t * linphone_chat_message_get_http_request(LinphoneChatMessage *msg) {
	return L_GET_PRIVATE_FROM_C_OBJECT(msg)->getHttpRequest();
}

void linphone_chat_message_set_http_request(LinphoneChatMessage *msg, belle_http_request_t *request) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setHttpRequest(request);
}

LinphonePrivate::SalOp * linphone_chat_message_get_sal_op(const LinphoneChatMessage *msg) {
	return L_GET_PRIVATE_FROM_C_OBJECT(msg)->getSalOp();
}

void linphone_chat_message_set_sal_op(LinphoneChatMessage *msg, LinphonePrivate::SalOp *op) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setSalOp(op);
}

SalCustomHeader * linphone_chat_message_get_sal_custom_headers(const LinphoneChatMessage *msg) {
	return L_GET_PRIVATE_FROM_C_OBJECT(msg)->getSalCustomHeaders();
}

void linphone_chat_message_set_sal_custom_headers(LinphoneChatMessage *msg, SalCustomHeader *header) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setSalCustomHeaders(header);
}

void linphone_chat_message_add_custom_header(LinphoneChatMessage *msg, const char *header_name,
											 const char *header_value) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->addSalCustomHeader(L_C_TO_STRING(header_name), L_C_TO_STRING(header_value));
}

void linphone_chat_message_remove_custom_header(LinphoneChatMessage *msg, const char *header_name) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->removeSalCustomHeader(L_C_TO_STRING(header_name));
}

const char *linphone_chat_message_get_custom_header(LinphoneChatMessage *msg, const char *header_name) {
	return L_STRING_TO_C(L_GET_PRIVATE_FROM_C_OBJECT(msg)->getSalCustomHeaderValue(L_C_TO_STRING(header_name)));
}

const LinphoneErrorInfo *linphone_chat_message_get_error_info(const LinphoneChatMessage *msg) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getErrorInfo();
}

// =============================================================================
// Methods
// =============================================================================

LinphoneStatus linphone_chat_message_download_file(LinphoneChatMessage *msg) {
	return ((LinphoneStatus)L_GET_PRIVATE_FROM_C_OBJECT(msg)->downloadFile());
}

void linphone_chat_message_cancel_file_transfer(LinphoneChatMessage *msg) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->cancelFileTransfer();
}

void linphone_chat_message_send (LinphoneChatMessage *msg) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->send();
}

void linphone_chat_message_resend(LinphoneChatMessage *msg) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->send();
}

void linphone_chat_message_resend_2(LinphoneChatMessage *msg) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->send();
}

void linphone_chat_message_update_state(LinphoneChatMessage *msg, LinphoneChatMessageState new_state) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->updateState((LinphonePrivate::ChatMessage::State) new_state);
}

void linphone_chat_message_deactivate(LinphoneChatMessage *msg){
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->cancelFileTransfer();
}

void linphone_chat_message_send_delivery_notification(LinphoneChatMessage *msg, LinphoneReason reason) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->sendDeliveryNotification(reason);
}

void linphone_chat_message_send_display_notification(LinphoneChatMessage *msg) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->sendDisplayNotification();
}

LinphoneStatus linphone_chat_message_put_char(LinphoneChatMessage *msg, uint32_t character) {
	return ((LinphoneStatus)L_GET_CPP_PTR_FROM_C_OBJECT(msg)->putCharacter(character));
}

void linphone_chat_message_add_text_content(LinphoneChatMessage *msg, const char *c_content) {
	LinphonePrivate::Content *content = new LinphonePrivate::Content();
	LinphonePrivate::ContentType contentType = LinphonePrivate::ContentType::PlainText;
	content->setContentType(contentType);
	content->setBody(L_C_TO_STRING(c_content));
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->addContent(*content);
}

bool_t linphone_chat_message_has_text_content(const LinphoneChatMessage *msg) {
	return L_GET_PRIVATE_FROM_C_OBJECT(msg)->hasTextContent();
}

const char *linphone_chat_message_get_text_content(const LinphoneChatMessage *msg) {
	const LinphonePrivate::Content *content = L_GET_PRIVATE_FROM_C_OBJECT(msg)->getTextContent();
	if (content->isEmpty())
		return nullptr;
	return L_STRING_TO_C(content->getBodyAsString());
}

// =============================================================================
// Old listener
// =============================================================================

LinphoneChatMessageStateChangedCb linphone_chat_message_get_message_state_changed_cb(LinphoneChatMessage* msg) {
	return msg->message_state_changed_cb;
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

// =============================================================================
// Structure has changed, hard to keep the behavior
// =============================================================================

const char * linphone_chat_message_get_content_type(LinphoneChatMessage *msg) {
	if (msg->contentTypeCache) {
		ms_free(msg->contentTypeCache);
	}
	msg->contentTypeCache = ms_strdup(L_STRING_TO_C(L_GET_PRIVATE_FROM_C_OBJECT(msg)->getContentType().asString()));
	return msg->contentTypeCache;
}

void linphone_chat_message_set_content_type(LinphoneChatMessage *msg, const char *content_type) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setContentType(LinphonePrivate::ContentType(L_C_TO_STRING(content_type)));
}

const char *linphone_chat_message_get_text(LinphoneChatMessage *msg) {
	return L_STRING_TO_C(L_GET_PRIVATE_FROM_C_OBJECT(msg)->getText());
}

int linphone_chat_message_set_text(LinphoneChatMessage *msg, const char* text) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setText(L_C_TO_STRING(text));
	return 0;
}

LinphoneContent *linphone_chat_message_get_file_transfer_information(LinphoneChatMessage *msg) {
	return L_GET_PRIVATE_FROM_C_OBJECT(msg)->getFileTransferInformation();
}

void linphone_chat_message_set_file_transfer_information(LinphoneChatMessage *msg, LinphoneContent *content) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setFileTransferInformation(content);
}

// =============================================================================
// Nothing to do, they call other C API methods
// =============================================================================

const LinphoneAddress *linphone_chat_message_get_peer_address(LinphoneChatMessage *msg) {
	return linphone_chat_room_get_peer_address(linphone_chat_message_get_chat_room(msg));
}

const LinphoneAddress *linphone_chat_message_get_local_address(LinphoneChatMessage *msg) {
	if (L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getDirection() == LinphonePrivate::ChatMessage::Direction::Outgoing)
		return linphone_chat_message_get_from_address(msg);
	return linphone_chat_message_get_to_address(msg);
}

LinphoneReason linphone_chat_message_get_reason(LinphoneChatMessage *msg) {
	return linphone_error_info_get_reason(linphone_chat_message_get_error_info(msg));
}

bool_t linphone_chat_message_is_file_transfer(LinphoneChatMessage *msg) {
	return LinphonePrivate::ContentType(linphone_chat_message_get_content_type(msg)) == LinphonePrivate::ContentType::FileTransfer;
}

bool_t linphone_chat_message_is_text(LinphoneChatMessage *msg) {
	return LinphonePrivate::ContentType(linphone_chat_message_get_content_type(msg)) == LinphonePrivate::ContentType::PlainText;
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

void linphone_chat_message_start_file_download(LinphoneChatMessage *msg,
											   LinphoneChatMessageStateChangedCb status_cb, void *ud) {
	msg->message_state_changed_cb = status_cb;
	msg->message_state_changed_user_data = ud;
	linphone_chat_message_download_file(msg);
}

void linphone_chat_message_release(LinphoneChatMessage *msg) {
	linphone_chat_message_deactivate(msg);
	linphone_chat_message_unref(msg);
}
