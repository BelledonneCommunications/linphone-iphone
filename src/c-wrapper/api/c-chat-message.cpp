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
#include "address/address.h"
#include "content/content.h"
#include "content/content-type.h"
#include "chat/chat-message-p.h"
#include "chat/chat-message.h"
#include "chat/chat-room-p.h"
#include "chat/real-time-text-chat-room-p.h"

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
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getExternalBodyUrl().c_str();
}

void linphone_chat_message_set_external_body_url(LinphoneChatMessage *msg, const char *url) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->setExternalBodyUrl(string(url));
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
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->setIsSecured(secured);
}

bool_t linphone_chat_message_is_outgoing(LinphoneChatMessage *msg) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->isOutgoing();
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
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getId().c_str();
}

void linphone_chat_message_set_message_id(LinphoneChatMessage *msg, char *id) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->setId(id);
}

void linphone_chat_message_set_storage_id(LinphoneChatMessage *msg, unsigned int id) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setStorageId(id);
}

bool_t linphone_chat_message_is_read(LinphoneChatMessage *msg) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->isRead();
}

const char *linphone_chat_message_get_appdata(const LinphoneChatMessage *msg) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getAppdata().c_str();
}

void linphone_chat_message_set_appdata(LinphoneChatMessage *msg, const char *data) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->setAppdata(data);
}

void linphone_chat_message_set_from_address(LinphoneChatMessage *msg, const LinphoneAddress *from) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->setFromAddress(make_shared<LinphonePrivate::Address>(linphone_address_as_string(from)));
}

const LinphoneAddress *linphone_chat_message_get_from_address(LinphoneChatMessage *msg) {
	if (msg->from)
		linphone_address_unref(msg->from);
	msg->from = linphone_address_new(L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getFromAddress()->asString().c_str());
	return msg->from;
}

void linphone_chat_message_set_to_address(LinphoneChatMessage *msg, const LinphoneAddress *to) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->setToAddress(make_shared<LinphonePrivate::Address>(linphone_address_as_string(to)));
}

const LinphoneAddress *linphone_chat_message_get_to_address(LinphoneChatMessage *msg) {
	if (msg->to)
		linphone_address_unref(msg->to);
	msg->to = linphone_address_new(L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getToAddress()->asString().c_str());
	return msg->to;
}

const char *linphone_chat_message_get_file_transfer_filepath(LinphoneChatMessage *msg) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getFileTransferFilepath().c_str();
}

void linphone_chat_message_set_file_transfer_filepath(LinphoneChatMessage *msg, const char *filepath) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->setFileTransferFilepath(filepath);
}

bool_t linphone_chat_message_get_to_be_stored(const LinphoneChatMessage *msg) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->isToBeStored();
}

void linphone_chat_message_set_to_be_stored(LinphoneChatMessage *msg, bool_t to_be_stored) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->setIsToBeStored(to_be_stored);
}

belle_http_request_t * linphone_chat_message_get_http_request(LinphoneChatMessage *msg) {
	return L_GET_PRIVATE_FROM_C_OBJECT(msg)->getHttpRequest();
}

void linphone_chat_message_set_http_request(LinphoneChatMessage *msg, belle_http_request_t *request) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setHttpRequest(request);
}

SalOp * linphone_chat_message_get_sal_op(const LinphoneChatMessage *msg) {
	return L_GET_PRIVATE_FROM_C_OBJECT(msg)->getSalOp();
}

void linphone_chat_message_set_sal_op(LinphoneChatMessage *msg, SalOp *op) {
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
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->addSalCustomHeader(header_name, header_value);
}

void linphone_chat_message_remove_custom_header(LinphoneChatMessage *msg, const char *header_name) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->removeSalCustomHeader(header_name);
}

const char *linphone_chat_message_get_custom_header(LinphoneChatMessage *msg, const char *header_name) {
	return L_GET_PRIVATE_FROM_C_OBJECT(msg)->getSalCustomHeaderValue(header_name).c_str();
}

const LinphoneErrorInfo *linphone_chat_message_get_error_info(const LinphoneChatMessage *msg) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(msg)->getErrorInfo();
}

// =============================================================================
// Methods
// =============================================================================

int linphone_chat_room_upload_file(LinphoneChatMessage *msg) {
	return ((LinphoneStatus)L_GET_CPP_PTR_FROM_C_OBJECT(msg)->uploadFile());
}

LinphoneStatus linphone_chat_message_download_file(LinphoneChatMessage *msg) {
	return ((LinphoneStatus)L_GET_CPP_PTR_FROM_C_OBJECT(msg)->downloadFile());
}

void linphone_chat_message_cancel_file_transfer(LinphoneChatMessage *msg) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->cancelFileTransfer();
}

void linphone_chat_message_resend(LinphoneChatMessage *msg) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->reSend();
}

void linphone_chat_message_resend_2(LinphoneChatMessage *msg) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->reSend();
}

void linphone_chat_message_update_state(LinphoneChatMessage *msg, LinphoneChatMessageState new_state) {
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->updateState((LinphonePrivate::ChatMessage::State) new_state);
}
void linphone_chat_message_send_imdn(LinphoneChatMessage *msg, ImdnType imdn_type, LinphoneReason reason) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->sendImdn(imdn_type, reason);
}

void linphone_chat_message_deactivate(LinphoneChatMessage *msg){
	L_GET_CPP_PTR_FROM_C_OBJECT(msg)->cancelFileTransfer();
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setChatRoom(nullptr);
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

const char * linphone_chat_message_get_content_type(const LinphoneChatMessage *msg) {
	return L_GET_PRIVATE_FROM_C_OBJECT(msg)->getContentType().c_str();
}

void linphone_chat_message_set_content_type(LinphoneChatMessage *msg, const char *content_type) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setContentType(content_type);
}

const char *linphone_chat_message_get_text(const LinphoneChatMessage *msg) {
	return L_GET_PRIVATE_FROM_C_OBJECT(msg)->getText().c_str();
}

int linphone_chat_message_set_text(LinphoneChatMessage *msg, const char* text) {
	L_GET_PRIVATE_FROM_C_OBJECT(msg)->setText(text);
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
	if (L_GET_CPP_PTR_FROM_C_OBJECT(msg)->isOutgoing()) {
		return linphone_chat_message_get_from_address(msg);
	}
	return linphone_chat_message_get_to_address(msg);
}

LinphoneReason linphone_chat_message_get_reason(LinphoneChatMessage *msg) {
	return linphone_error_info_get_reason(linphone_chat_message_get_error_info(msg));
}

bool_t linphone_chat_message_is_file_transfer(const LinphoneChatMessage *msg) {
	return LinphonePrivate::ContentType::isFileTransfer(linphone_chat_message_get_content_type(msg));
}

bool_t linphone_chat_message_is_text(const LinphoneChatMessage *msg) {
	return LinphonePrivate::ContentType::isText(linphone_chat_message_get_content_type(msg));
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
