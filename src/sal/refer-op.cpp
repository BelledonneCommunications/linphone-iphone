/*
 Linphone library
 Copyright (C) 2017  Belledonne Communications SARL

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "sal/refer-op.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

void SalReferOp::process_error() {
	this->state=State::Terminated;
}

void SalReferOp::process_io_error_cb(void *user_ctx, const belle_sip_io_error_event_t *event) {
	SalReferOp * op = (SalReferOp *)user_ctx;
	sal_error_info_set(&op->error_info,SalReasonIOError, "SIP", 503,"IO Error",NULL);
	op->process_error();
}

void SalReferOp::process_response_event_cb(void *op_base, const belle_sip_response_event_t *event) {
	SalReferOp * op = (SalReferOp *)op_base;
	op->set_error_info_from_response(belle_sip_response_event_get_response(event));
	/*the response is not notified to the app*/
	/*To be done when necessary*/
}

void SalReferOp::process_timeout_cb(void *user_ctx, const belle_sip_timeout_event_t *event) {
	SalReferOp * op=(SalReferOp *)user_ctx;
	sal_error_info_set(&op->error_info,SalReasonRequestTimeout, "SIP", 408,"Request timeout",NULL);
	op->process_error();
}

void SalReferOp::process_request_event_cb(void *op_base, const belle_sip_request_event_t *event) {
	SalReferOp * op = (SalReferOp *)op_base;
	belle_sip_request_t *req = belle_sip_request_event_get_request(event);
	belle_sip_header_refer_to_t *refer_to= belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_refer_to_t);
	belle_sip_server_transaction_t *server_transaction = belle_sip_provider_create_server_transaction(op->root->prov,belle_sip_request_event_get_request(event));
	
	belle_sip_object_ref(server_transaction);
	belle_sip_transaction_set_application_data(BELLE_SIP_TRANSACTION(server_transaction),op->ref());
	op->pending_server_trans = server_transaction;
	
	if (!refer_to){
		ms_warning("cannot do anything with the refer without destination");
		op->reply(SalReasonUnknown);/*is mapped on bad request*/
		op->unref();
		return;
	}
	SalAddress *referToAddr = sal_address_new(belle_sip_header_get_unparsed_value(BELLE_SIP_HEADER(refer_to)));
	op->root->callbacks.refer_received(op, referToAddr);
	/*the app is expected to reply in the callback*/
	sal_address_unref(referToAddr);
	op->unref();
}

void SalReferOp::fill_cbs() {
	static belle_sip_listener_callbacks_t op_refer_callbacks = {0};
	if (op_refer_callbacks.process_io_error==NULL) {
		op_refer_callbacks.process_io_error=process_io_error_cb;
		op_refer_callbacks.process_response_event=process_response_event_cb;
		op_refer_callbacks.process_timeout=process_timeout_cb;
		op_refer_callbacks.process_request_event=process_request_event_cb;
	}
	this->callbacks=&op_refer_callbacks;
	this->type=Type::Refer;
}

SalReferOp::SalReferOp(Sal *sal) : SalOp(sal){
	fill_cbs();
}

int SalReferOp::send_refer(const SalAddress *refer_to) {
	this->dir=Dir::Outgoing;

	belle_sip_request_t* req=build_request("REFER");
	if (req == NULL ) return -1;
	if (get_contact_address()) belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(create_contact()));
	belle_sip_header_address_t *address = BELLE_SIP_HEADER_ADDRESS(refer_to);
	belle_sip_uri_t *uri = belle_sip_header_address_get_uri(address);
	if (!belle_sip_uri_get_host(uri))
		belle_sip_header_address_set_automatic(address, true);
	belle_sip_header_refer_to_t *refer_to_header = belle_sip_header_refer_to_create(address);
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req), BELLE_SIP_HEADER(refer_to_header));
	return send_request(req);
}

int SalReferOp::reply(SalReason reason){
	if (this->pending_server_trans){
		int code=to_sip_code(reason);
		belle_sip_response_t *resp = belle_sip_response_create_from_request(
			belle_sip_transaction_get_request((belle_sip_transaction_t*)this->pending_server_trans),code);
		belle_sip_server_transaction_send_response(this->pending_server_trans,resp);
		return 0;
	}else ms_error("SalReferOp::reply: no server transaction");
	return -1;
}

LINPHONE_END_NAMESPACE
