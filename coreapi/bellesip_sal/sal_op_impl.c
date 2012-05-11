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
	__sal_op_free(op);
	if (op->request) belle_sip_object_unref(op->request);
	if (op->registration_refresh_timer>0) {
		belle_sip_main_loop_cancel_source(belle_sip_stack_get_main_loop(op->base.root->stack),op->registration_refresh_timer);
	}
	return ;
}
void sal_op_authenticate(SalOp *op, const SalAuthInfo *info){
	belle_sip_header_www_authenticate_t* authenticate;
	belle_sip_header_authorization_t* authorization;
	const char* ha1;
	char computed_ha1[33];

	if (info->ha1) {
		ha1=info->ha1;
	} else {
		if(belle_sip_auth_helper_compute_ha1(info->userid,info->realm,info->password, computed_ha1)) {
			goto error;
		} else
			ha1=computed_ha1;
	}
	if (!op->response) {
		ms_error("try to authenticate an unchallenged op [%p]",op);
		goto error;
	}
	authenticate = BELLE_SIP_HEADER_WWW_AUTHENTICATE(belle_sip_message_get_header(BELLE_SIP_MESSAGE(op->response),BELLE_SIP_WWW_AUTHENTICATE));
	if (authenticate) {
		authorization = belle_sip_auth_helper_create_authorization(authenticate);
	} else {
		/*proxy inerite from www*/
		authenticate = BELLE_SIP_HEADER_WWW_AUTHENTICATE(belle_sip_message_get_header(BELLE_SIP_MESSAGE(op->response),BELLE_SIP_PROXY_AUTHENTICATE));
		authorization = BELLE_SIP_HEADER_AUTHORIZATION(belle_sip_auth_helper_create_proxy_authorization(BELLE_SIP_HEADER_PROXY_AUTHENTICATE(authenticate)));
	}
	belle_sip_header_authorization_set_uri(authorization,belle_sip_request_get_uri(op->request));
	belle_sip_header_authorization_set_username(authorization,info->userid);

	if (belle_sip_auth_helper_fill_authorization(authorization
												,belle_sip_request_get_method(op->request)
												,ha1)) {
		belle_sip_object_unref(authorization);
		goto error;
	}
	belle_sip_message_set_header(BELLE_SIP_MESSAGE(op->request),BELLE_SIP_HEADER(authorization));
	sal_register_refresh(op,-1);
	return;

error:
	ms_error("cannot generate authorization for [%s] at [%s]",info->userid,info->realm);

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


