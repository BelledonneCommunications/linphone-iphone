/*
linphone
Copyright (C) 2012  Belledonne Communications, Grenoble, France

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "sal_impl.h"



static void register_process_io_error(void *user_ctx, const belle_sip_io_error_event_t *event){
	ms_error("process_io_error not implemented yet");
}

static void register_refresh(SalOp* op) {
	op->registration_refresh_timer=0;
	belle_sip_header_cseq_t* cseq=(belle_sip_header_cseq_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(op->request),BELLE_SIP_CSEQ);
	belle_sip_header_cseq_set_seq_number(cseq,belle_sip_header_cseq_get_seq_number(cseq)+1);
	sal_op_send_request(op,op->request);
}
static bool_t is_contact_equal(belle_sip_header_contact_t* a,belle_sip_header_contact_t* b) {
	if (!a | !b) return FALSE;
	return !belle_sip_uri_equals(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(a))
								,belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(b)));
}
static void register_response_event(void *user_ctx, const belle_sip_response_event_t *event){
	belle_sip_client_transaction_t* client_transaction = belle_sip_response_event_get_client_transaction(event);
	SalOp* op = (SalOp*)belle_sip_transaction_get_application_data(BELLE_SIP_TRANSACTION(client_transaction));
	belle_sip_response_t* response = belle_sip_response_event_get_response(event);
	belle_sip_header_expires_t* expires_header;

	const belle_sip_list_t* contact_header_list;
	int response_code = belle_sip_response_get_status_code(response);
	int expires=-1;
	if (response_code<200) return;/*nothing to do*/


	switch (response_code) {
	case 200: {

		contact_header_list = belle_sip_message_get_headers(BELLE_SIP_MESSAGE(response),BELLE_SIP_CONTACT);
		if (contact_header_list) {
			contact_header_list = belle_sip_list_find_custom((belle_sip_list_t*)contact_header_list,(belle_sip_compare_func)is_contact_equal, (const void*)sal_op_get_contact_address(op));
			if (!contact_header_list) {
				ms_error("no matching contact for [%s]", sal_op_get_contact(op));
			} else {
				expires=belle_sip_header_contact_get_expires(BELLE_SIP_HEADER_CONTACT(contact_header_list->data));
			}
		}

		if (expires<0 ) {
			/*no contact with expire, looking for Expires header*/
			if ((expires_header=(belle_sip_header_expires_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_EXPIRES))) {
				expires = belle_sip_header_expires_get_expires(expires_header);
			}
		}
		if (expires<0) {
			ms_message("Neither Expires header nor corresponding Contact header found");
			expires=0;
		}

		op->base.root->callbacks.register_success(op,expires>0);
		/*always cancel pending refresh if any*/
		if (op->registration_refresh_timer>0) {
			belle_sip_main_loop_cancel_source(belle_sip_stack_get_main_loop(op->base.root->stack),op->registration_refresh_timer);
			op->registration_refresh_timer=0;
		}
		if (expires>0) {
			op->registration_refresh_timer = belle_sip_main_loop_add_timeout(belle_sip_stack_get_main_loop(op->base.root->stack),(belle_sip_source_func_t)register_refresh,op,expires*1000);
		}

		break;
	}

	default:{
		ms_error("Unexpected answer [%s] for registration request bound to [%s]",belle_sip_response_get_reason_phrase(response),op->base.from);
		op->base.root->callbacks.register_failure(op,SalErrorFailure,SalReasonUnknown,belle_sip_response_get_reason_phrase(response));
		break;
	}
}
}
static void register_process_timeout(void *user_ctx, const belle_sip_timeout_event_t *event) {
	ms_error("process_timeout not implemented yet");
}
static void register_process_transaction_terminated(void *user_ctx, const belle_sip_transaction_terminated_event_t *event) {
	ms_error("process_transaction_terminated not implemented yet");
}



/*if expire = -1, does not change expires*/
static void send_register_request_with_expires(SalOp* op, belle_sip_request_t* request,int expires) {
	belle_sip_header_expires_t* expires_header=(belle_sip_header_expires_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_EXPIRES);

	if (!expires_header && expires>=0) {
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(expires_header=belle_sip_header_expires_new()));
	}
	if (expires_header) belle_sip_header_expires_set_expires(expires_header,expires);
	sal_op_send_request(op,request);
}


int sal_register(SalOp *op, const char *proxy, const char *from, int expires){
	belle_sip_request_t *req;

	sal_op_set_from(op,from);
	sal_op_set_to(op,from);
	sal_op_set_route(op,proxy);
	op->callbacks.process_io_error=register_process_io_error;
	op->callbacks.process_response_event=register_response_event;
	op->callbacks.process_timeout=register_process_timeout;
	op->callbacks.process_transaction_terminated=register_process_transaction_terminated;

	req = sal_op_build_request(op,"REGISTER");
	belle_sip_uri_t* req_uri = belle_sip_request_get_uri(req);
	belle_sip_uri_set_user(req_uri,NULL); /*remove userinfo if any*/

	send_register_request_with_expires(op,req,expires);
	return 0;
}

int sal_register_refresh(SalOp *op, int expires){
	belle_sip_header_cseq_t* cseq=(belle_sip_header_cseq_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(op->request),BELLE_SIP_CSEQ);
	belle_sip_header_cseq_set_seq_number(cseq,belle_sip_header_cseq_get_seq_number(cseq)+1);
	send_register_request_with_expires(op,op->request,expires);
	return 0;
}
int sal_unregister(SalOp *op){
	belle_sip_header_cseq_t* cseq=(belle_sip_header_cseq_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(op->request),BELLE_SIP_CSEQ);
	belle_sip_header_cseq_set_seq_number(cseq,belle_sip_header_cseq_get_seq_number(cseq)+1);
	send_register_request_with_expires(op,op->request,0);
	return 0;
}


