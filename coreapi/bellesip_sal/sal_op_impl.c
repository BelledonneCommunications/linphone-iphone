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


/*create an operation */
SalOp * sal_op_new(Sal *sal){
	SalOp *op=ms_new0(SalOp,1);
	__sal_op_init(op,sal);
	return op;
}

void sal_op_release(SalOp *op){
	if (op->request) belle_sip_object_unref(op->request);
	if (op->registration_refresh_timer>0) {
		belle_sip_main_loop_cancel_source(belle_sip_stack_get_main_loop(op->base.root->stack),op->registration_refresh_timer);
	}
	__sal_op_free(op);
	return ;
}
void sal_op_authenticate(SalOp *op, const SalAuthInfo *info){
	/*for sure auth info will be accesible from the provider*/
	sal_process_authentication(op, NULL);
	return ;
}

void sal_op_cancel_authentication(SalOp *h){
	ms_fatal("sal_op_cancel_authentication not implemented yet");
	return ;
}

int sal_op_get_auth_requested(SalOp *op, const char **realm, const char **username){
	*realm=op->auth_info.realm;
	*username=op->auth_info.username;
	return 0;
}

belle_sip_request_t* sal_op_build_request(SalOp *op,const char* method) {
	belle_sip_header_from_t* from_header;
	belle_sip_header_to_t* to_header;
	belle_sip_provider_t* prov=op->base.root->prov;
	belle_sip_request_t *req;
	belle_sip_uri_t* req_uri;
	belle_sip_header_contact_t* contact_header;
	char token[10];

	from_header = belle_sip_header_from_create(BELLE_SIP_HEADER_ADDRESS(sal_op_get_from_address(op))
												,belle_sip_random_token(token,sizeof(token)));
	to_header = belle_sip_header_to_create(BELLE_SIP_HEADER_ADDRESS(sal_op_get_to_address(op)),NULL);
	req_uri = (belle_sip_uri_t*)belle_sip_object_clone((belle_sip_object_t*)belle_sip_header_address_get_uri((belle_sip_header_address_t*)to_header));

	if (sal_op_get_contact_address(op)) {
		contact_header = belle_sip_header_contact_create(BELLE_SIP_HEADER_ADDRESS(sal_op_get_contact_address(op)));
	} else {
		contact_header= belle_sip_header_contact_new();
		belle_sip_header_address_set_uri((belle_sip_header_address_t*)contact_header,belle_sip_uri_new());
		belle_sip_uri_set_user(belle_sip_header_address_get_uri((belle_sip_header_address_t*)contact_header),belle_sip_uri_get_user(req_uri));
	}
	req=belle_sip_request_create(
							req_uri,
							method,
		                    belle_sip_provider_create_call_id(prov),
		                    belle_sip_header_cseq_create(20,method),
		                    from_header,
		                    to_header,
		                    belle_sip_header_via_new(),
		                    70);
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(contact_header));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(op->base.root->user_agent));
	return req;
}


/*Messaging */
int sal_text_send(SalOp *op, const char *from, const char *to, const char *text){
	ms_fatal("sal_text_send not implemented yet");
	return -1;
}

/*presence Subscribe/notify*/
int sal_subscribe_presence(SalOp *op, const char *from, const char *to){
	ms_fatal("sal_subscribe_presence not implemented yet");
	return -1;
}
int sal_unsubscribe(SalOp *op){
	ms_fatal("sal_unsubscribe not implemented yet");
	return -1;
}
int sal_subscribe_accept(SalOp *op){
	ms_fatal("sal_subscribe_accept not implemented yet");
	return -1;
}
int sal_subscribe_decline(SalOp *op){
	ms_fatal("sal_subscribe_decline not implemented yet");
	return -1;
}
int sal_notify_presence(SalOp *op, SalPresenceStatus status, const char *status_message){
	ms_fatal("sal_notify_presence not implemented yet");
	return -1;
}
int sal_notify_close(SalOp *op){
	ms_fatal("sal_notify_close not implemented yet");
	return -1;
}

/*presence publish */
int sal_publish(SalOp *op, const char *from, const char *to, SalPresenceStatus status){
	ms_fatal("sal_publish not implemented yet");
	return -1;
}


/*ping: main purpose is to obtain its own contact address behind firewalls*/
int sal_ping(SalOp *op, const char *from, const char *to){
	ms_fatal("sal_ping not implemented yet");
	return -1;
}

void sal_op_set_remote_ua(SalOp*op,belle_sip_message_t* message) {
	belle_sip_header_user_agent_t* user_agent=belle_sip_message_get_header_by_type(message,belle_sip_header_user_agent_t);
	char user_agent_string[256];
	if(user_agent && belle_sip_header_user_agent_get_products_as_string(user_agent,user_agent_string,sizeof(user_agent_string))>0) {
		op->base.remote_ua=ms_strdup(user_agent_string);
	}
}

void sal_op_resend_request(SalOp* op, belle_sip_request_t* request) {
	belle_sip_header_cseq_t* cseq=(belle_sip_header_cseq_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(op->request),BELLE_SIP_CSEQ);
	belle_sip_header_cseq_set_seq_number(cseq,belle_sip_header_cseq_get_seq_number(cseq)+1);
	sal_op_send_request(op,request);
}
void sal_op_send_request(SalOp* op, belle_sip_request_t* request) {
	belle_sip_client_transaction_t* client_transaction;
	belle_sip_provider_t* prov=op->base.root->prov;
	belle_sip_header_route_t* route_header;
	if (!op->dialog || belle_sip_dialog_get_state(op->dialog)!=BELLE_SIP_DIALOG_CONFIRMED) {
		/*don't put route header if dialog is in confirmed state*/
		if (sal_op_get_route_address(op)) {
			route_header = belle_sip_header_route_create(BELLE_SIP_HEADER_ADDRESS(sal_op_get_route_address(op)));
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(route_header));
		}
	}
	client_transaction = belle_sip_provider_create_client_transaction(prov,request);
	belle_sip_transaction_set_application_data(BELLE_SIP_TRANSACTION(client_transaction),op);
	/*in case DIALOG is in state NULL create a new dialog*/
	if (!op->dialog  && strcmp("INVITE",belle_sip_request_get_method(request))==0) {
		op->dialog=belle_sip_provider_create_dialog(prov,BELLE_SIP_TRANSACTION(client_transaction));
		op->pending_inv_client_trans=client_transaction; /*update pending inv for being able to cancel*/
		belle_sip_dialog_set_application_data(op->dialog,op);
	} else if (!belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_AUTHORIZATION)
		&& !belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_PROXY_AUTHORIZATION)) {
		/*hmm just in case we already have authentication param in cache*/
		belle_sip_provider_add_authorization(op->base.root->prov,request,NULL);
	}
	belle_sip_client_transaction_send_request(client_transaction);

}
