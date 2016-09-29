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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "sal_impl.h"


static void register_refresher_listener (belle_sip_refresher_t* refresher
		,void* user_pointer
		,unsigned int status_code
		,const char* reason_phrase, int will_retry) {
	SalOp* op = (SalOp*)user_pointer;
	belle_sip_response_t* response=belle_sip_transaction_get_response(BELLE_SIP_TRANSACTION(belle_sip_refresher_get_transaction(refresher)));
	ms_message("Register refresher [%i] reason [%s] for proxy [%s]",status_code,reason_phrase,sal_op_get_proxy(op));

	if (belle_sip_refresher_get_auth_events(refresher)) {
		if (op->auth_info) sal_auth_info_delete(op->auth_info);
		/*only take first one for now*/
		op->auth_info=sal_auth_info_create((belle_sip_auth_event_t*)(belle_sip_refresher_get_auth_events(refresher)->data));
	}
	sal_error_info_set(&op->error_info,SalReasonUnknown,status_code,reason_phrase,NULL);
	if (status_code>=200){
		sal_op_assign_recv_headers(op,(belle_sip_message_t*)response);
	}
	if(status_code == 200) {
		/*check service route rfc3608*/
		belle_sip_header_service_route_t* service_route;
		belle_sip_header_address_t* service_route_address=NULL;
		belle_sip_header_contact_t *contact = belle_sip_refresher_get_contact(refresher);
		if ((service_route=belle_sip_message_get_header_by_type(response,belle_sip_header_service_route_t))) {
			service_route_address=belle_sip_header_address_create(NULL,belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(service_route)));
		}
		sal_op_set_service_route(op,(const SalAddress*)service_route_address);
		if (service_route_address) belle_sip_object_unref(service_route_address);

		sal_remove_pending_auth(op->base.root,op); /*just in case*/
		if (contact) {
			sal_op_set_contact_address(op,(SalAddress*)(BELLE_SIP_HEADER_ADDRESS(contact))); /*update contact with real value*/
		}
		op->base.root->callbacks.register_success(op,belle_sip_refresher_get_expires(op->refresher)>0);
	} else if (status_code>=400) {
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
		sal_op_ref(op); /*take a ref while invoking the callback to make sure the operations done after are valid*/
		op->base.root->callbacks.register_failure(op);
		if (op->state!=SalOpStateTerminated && op->auth_info) {
			/*add pending auth*/
			sal_add_pending_auth(op->base.root,op);
			if (status_code==403 || status_code==401 || status_code==407 )
				op->base.root->callbacks.auth_failure(op,op->auth_info);
		}
		sal_op_unref(op);
	}
}

int sal_register(SalOp *op, const char *proxy, const char *from, int expires,SalAddress* old_contact){
	belle_sip_request_t *req;
	belle_sip_uri_t* req_uri;
	belle_sip_header_t* accept_header;

	if (op->refresher){
		belle_sip_refresher_stop(op->refresher);
		belle_sip_object_unref(op->refresher);
		op->refresher=NULL;
	}

	op->type=SalOpRegister;
	sal_op_set_from(op,from);
	sal_op_set_to(op,from);
	sal_op_set_route(op,proxy);
	req = sal_op_build_request(op,"REGISTER");
	req_uri = belle_sip_request_get_uri(req);
	belle_sip_uri_set_user(req_uri,NULL); /*remove userinfo if any*/
	if (op->base.root->use_dates){
		time_t curtime=time(NULL);
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(belle_sip_header_date_create_from_time(&curtime)));
	}
	accept_header = belle_sip_header_create("Accept", "application/sdp, text/plain, application/vnd.gsma.rcs-ft-http+xml");
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req), accept_header);
	belle_sip_message_set_header(BELLE_SIP_MESSAGE(req),(belle_sip_header_t*)sal_op_create_contact(op));
	if (old_contact) {
		belle_sip_header_contact_t *contact=belle_sip_header_contact_create((const belle_sip_header_address_t *)old_contact);
		if (contact) {
			char * tmp;
			belle_sip_header_contact_set_expires(contact,0); /*remove old aor*/
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(req), BELLE_SIP_HEADER(contact));
			tmp = belle_sip_object_to_string(contact);
			ms_message("Clearing contact [%s] for op [%p]",tmp,op);
			ms_free(tmp);
		} else {
			ms_error("Cannot add old contact header to op [%p]",op);
		}
	}
	return sal_op_send_and_create_refresher(op,req,expires,register_refresher_listener);
}

int sal_register_refresh(SalOp *op, int expires){
	if (op->refresher)
		return belle_sip_refresher_refresh(op->refresher,expires);
	else
		return -1;
}

int sal_unregister(SalOp *op){
	return sal_register_refresh(op,0);
}


