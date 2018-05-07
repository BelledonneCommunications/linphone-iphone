/*
 * message-op.cpp
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

#include "sal/message-op.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

void SalMessageOp::processError() {
	if (mDir == Dir::Outgoing) {
		mRoot->mCallbacks.message_delivery_update(this, SalMessageDeliveryFailed);
	} else {
		ms_warning("unexpected io error for incoming message on op [%p]", this);
	}
	mState=State::Terminated;
}

void SalMessageOp::processIoErrorCb(void *user_ctx, const belle_sip_io_error_event_t *event) {
	SalMessageOp * op = (SalMessageOp *)user_ctx;
	sal_error_info_set(&op->mErrorInfo,SalReasonIOError, "SIP", 503,"IO Error",NULL);
	op->processError();
}

void SalMessageOp::processResponseEventCb(void *op_base, const belle_sip_response_event_t *event) {
	SalMessageOp * op = (SalMessageOp *)op_base;
	int code = belle_sip_response_get_status_code(belle_sip_response_event_get_response(event));
	SalMessageDeliveryStatus status;
	op->setErrorInfoFromResponse(belle_sip_response_event_get_response(event));
	
	if (code>=100 && code <200)
		status=SalMessageDeliveryInProgress;
	else if (code>=200 && code <300)
		status=SalMessageDeliveryDone;
	else
		status=SalMessageDeliveryFailed;
	
	op->mRoot->mCallbacks.message_delivery_update(op,status);
}

void SalMessageOp::processTimeoutCb(void *user_ctx, const belle_sip_timeout_event_t *event) {
	SalMessageOp * op=(SalMessageOp *)user_ctx;
	sal_error_info_set(&op->mErrorInfo,SalReasonRequestTimeout, "SIP", 408,"Request timeout",NULL);
	op->processError();
}

void SalMessageOp::processRequestEventCb(void *op_base, const belle_sip_request_event_t *event) {
	SalMessageOp * op = (SalMessageOp *)op_base;
	op->processIncomingMessage(event);
}

void SalMessageOp::fillCallbacks() {
	static belle_sip_listener_callbacks_t op_message_callbacks = {0};
	if (op_message_callbacks.process_io_error==NULL) {
		op_message_callbacks.process_io_error=processIoErrorCb;
		op_message_callbacks.process_response_event=processResponseEventCb;
		op_message_callbacks.process_timeout=processTimeoutCb;
		op_message_callbacks.process_request_event=processRequestEventCb;
	}
	mCallbacks=&op_message_callbacks;
	mType=Type::Message;
}

int SalMessageOp::sendMessage (const Content &content) {
	   fillCallbacks();
	mDir = Dir::Outgoing;
	belle_sip_request_t *req = buildRequest("MESSAGE");
	if (!req)
		return -1;
	prepareMessageRequest(req, content);
	return sendRequest(req);
}

LINPHONE_END_NAMESPACE
