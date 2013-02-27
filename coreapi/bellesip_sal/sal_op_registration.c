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
	SalOp* op = (SalOp*)user_ctx;
	ms_error("register_process_io_error io error reported for [%s]",sal_op_get_proxy(op));
	if (op->registration_refresher) {
		op->base.root->callbacks.register_failure(op,SalErrorFailure,SalErrorFailure,"io error");
	}
}

static void register_refresher_listener ( const belle_sip_refresher_t* refresher
		,void* user_pointer
		,unsigned int status_code
		,const char* reason_phrase) {
	SalOp* op = (SalOp*)user_pointer;
	SalError sal_err;
	SalReason sal_reason;
	ms_message("Register refresher  [%i] reason [%s] for proxy [%s]",status_code,reason_phrase,sal_op_get_proxy(op));
	if(status_code ==200) {
		op->base.root->callbacks.register_success(op,belle_sip_refresher_get_expires(op->registration_refresher)>0);
	} else if (status_code>=400) {
		sal_compute_sal_errors_from_code(status_code,&sal_err,&sal_reason);
		op->base.root->callbacks.register_failure(op,sal_err,sal_reason,reason_phrase);
	} else {
		ms_warning("Register refresher know what to do with this status code");
	}
}

static void register_response_event(void *user_ctx, const belle_sip_response_event_t *event){
	belle_sip_client_transaction_t* client_transaction = belle_sip_response_event_get_client_transaction(event);
	SalOp* op = (SalOp*)belle_sip_transaction_get_application_data(BELLE_SIP_TRANSACTION(client_transaction));
	belle_sip_response_t* response = belle_sip_response_event_get_response(event);
	belle_sip_header_service_route_t* service_route;
	belle_sip_header_address_t* service_route_address=NULL;

	int response_code = belle_sip_response_get_status_code(response);
	if (response_code<200) return;/*nothing to do*/


	switch (response_code) {
	case 200: {

		/*check service route rfc3608*/
		if ((service_route=belle_sip_message_get_header_by_type(response,belle_sip_header_service_route_t))) {
			service_route_address=belle_sip_header_address_create(NULL,belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(service_route)));
		}
		sal_op_set_service_route(op,(const SalAddress*)service_route_address);
		if (service_route_address) belle_sip_object_unref(service_route_address);

		op->base.root->callbacks.register_success(op,TRUE);

		if (/*expires>0 &&*/ !op->registration_refresher) {
			op->registration_refresher = belle_sip_client_transaction_create_refresher(client_transaction);
			belle_sip_refresher_set_listener(op->registration_refresher,register_refresher_listener,op);
		}

		break;
	}

	default:{

		/* from rfc3608, 6.1.
		If the UA refreshes the registration, the stored value of the Service-
		   Route is updated according to the Service-Route header field of the
		   latest 200 class response.  If there is no Service-Route header field
		   in the response, the UA clears any service route for that address-
		   of-record previously stored by the UA.  If the re-registration
		   request is refused or if an existing registration expires and the UA
		   chooses not to re-register, the UA SHOULD discard any stored service
		   route for that address-of-record. */
		sal_op_set_service_route(op,NULL);

		ms_error("Unexpected answer [%s] for registration request bound to [%s]",belle_sip_response_get_reason_phrase(response),op->base.from);
		op->base.root->callbacks.register_failure(op,SalErrorFailure,SalReasonUnknown,belle_sip_response_get_reason_phrase(response));
		break;
	}
}
}
static void register_process_timeout(void *user_ctx, const belle_sip_timeout_event_t *event) {
	SalOp* op = (SalOp*)user_ctx;
	ms_error("register_process_timeout timeout error reported for [%s]",sal_op_get_proxy(op));
	if (!op->registration_refresher) {
		op->base.root->callbacks.register_failure(op,SalErrorNoResponse,SalReasonUnknown,"Request Timeout");
	} else {
		/*refresher will report error*/
	}
}
static void register_process_transaction_terminated(void *user_ctx, const belle_sip_transaction_terminated_event_t *event) {
	/*ms_error("register_process_transaction_terminated not implemented yet");*/
}



/*if expire = -1, does not change expires*/
static int send_register_request_with_expires(SalOp* op, belle_sip_request_t* request,int expires) {
	belle_sip_header_expires_t* expires_header=(belle_sip_header_expires_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_EXPIRES);

	if (!expires_header && expires>=0) {
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(expires_header=belle_sip_header_expires_new()));
	}
	if (expires_header) belle_sip_header_expires_set_expires(expires_header,expires);
	return sal_op_send_request(op,request);
}


int sal_register(SalOp *op, const char *proxy, const char *from, int expires){
	belle_sip_request_t *req;
	belle_sip_uri_t* req_uri;
	sal_op_set_from(op,from);
	sal_op_set_to(op,from);
	sal_op_set_route(op,proxy);
	op->callbacks.process_io_error=register_process_io_error;
	op->callbacks.process_response_event=register_response_event;
	op->callbacks.process_timeout=register_process_timeout;
	op->callbacks.process_transaction_terminated=register_process_transaction_terminated;

	req = sal_op_build_request(op,"REGISTER");
	req_uri = belle_sip_request_get_uri(req);
	belle_sip_uri_set_user(req_uri,NULL); /*remove userinfo if any*/

	return send_register_request_with_expires(op,req,expires);
}

int sal_register_refresh(SalOp *op, int expires){
	if (op->registration_refresher)
		return belle_sip_refresher_refresh(op->registration_refresher,expires);
	else
		return -1;
}
int sal_unregister(SalOp *op){
	return sal_register_refresh(op,0);
}


