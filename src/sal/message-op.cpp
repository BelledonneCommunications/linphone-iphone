/*
 * message-op.cpp
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

#include "sal/message-op.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

void SalMessageOp::process_error() {
	if (this->dir == Dir::Outgoing) {
		this->root->callbacks.message_delivery_update(this, SalMessageDeliveryFailed);
	} else {
		ms_warning("unexpected io error for incoming message on op [%p]", this);
	}
	this->state=State::Terminated;
}

void SalMessageOp::process_io_error_cb(void *user_ctx, const belle_sip_io_error_event_t *event) {
	SalMessageOp * op = (SalMessageOp *)user_ctx;
	sal_error_info_set(&op->error_info,SalReasonIOError, "SIP", 503,"IO Error",NULL);
	op->process_error();
}

void SalMessageOp::process_response_event_cb(void *op_base, const belle_sip_response_event_t *event) {
	SalMessageOp * op = (SalMessageOp *)op_base;
	int code = belle_sip_response_get_status_code(belle_sip_response_event_get_response(event));
	SalMessageDeliveryStatus status;
	op->set_error_info_from_response(belle_sip_response_event_get_response(event));
	
	if (code>=100 && code <200)
		status=SalMessageDeliveryInProgress;
	else if (code>=200 && code <300)
		status=SalMessageDeliveryDone;
	else
		status=SalMessageDeliveryFailed;
	
	op->root->callbacks.message_delivery_update(op,status);
}

void SalMessageOp::process_timeout_cb(void *user_ctx, const belle_sip_timeout_event_t *event) {
	SalMessageOp * op=(SalMessageOp *)user_ctx;
	sal_error_info_set(&op->error_info,SalReasonRequestTimeout, "SIP", 408,"Request timeout",NULL);
	op->process_error();
}

void SalMessageOp::process_request_event_cb(void *op_base, const belle_sip_request_event_t *event) {
	SalMessageOp * op = (SalMessageOp *)op_base;
	op->process_incoming_message(event);
}

void SalMessageOp::fill_cbs() {
	static belle_sip_listener_callbacks_t op_message_callbacks = {0};
	if (op_message_callbacks.process_io_error==NULL) {
		op_message_callbacks.process_io_error=process_io_error_cb;
		op_message_callbacks.process_response_event=process_response_event_cb;
		op_message_callbacks.process_timeout=process_timeout_cb;
		op_message_callbacks.process_request_event=process_request_event_cb;
	}
	this->callbacks=&op_message_callbacks;
	this->type=Type::Message;
}

void SalMessageOpInterface::prepare_message_request(belle_sip_request_t *req, const char* content_type, const char *msg) {
	char content_type_raw[256];
	size_t content_length = msg?strlen(msg):0;
	time_t curtime = ms_time(NULL);
	snprintf(content_type_raw,sizeof(content_type_raw),BELLE_SIP_CONTENT_TYPE ": %s",content_type);
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(belle_sip_header_content_type_parse(content_type_raw)));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(belle_sip_header_content_length_create(content_length)));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(belle_sip_header_date_create_from_time(&curtime)));
	if (msg){
		/*don't call set_body() with null argument because it resets content type and content length*/
		belle_sip_message_set_body(BELLE_SIP_MESSAGE(req), msg, content_length);
	}
}

int SalMessageOp::send_message(const char* content_type, const char *msg) {
	fill_cbs();
	this->dir = Dir::Outgoing;
	belle_sip_request_t *req = build_request("MESSAGE");
	if (!req)
		return -1;
	prepare_message_request(req, content_type, msg);
	return send_request(req);
}

LINPHONE_END_NAMESPACE
