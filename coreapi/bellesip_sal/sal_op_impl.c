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

static void sal_add_pending_auth(Sal *sal, SalOp *op){
	sal->pending_auths=ms_list_append(sal->pending_auths,op);
}

static void sal_remove_pending_auth(Sal *sal, SalOp *op){
	sal->pending_auths=ms_list_remove(sal->pending_auths,op);
}

/*create an operation */
SalOp * sal_op_new(Sal *sal){
	SalOp *op=ms_new0(SalOp,1);
	__sal_op_init(op,sal);
	return op;
}

void sal_op_release(SalOp *op){
	__sal_op_free(op);
	if (op->register_request) belle_sip_object_unref(op->register_request);
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
	if (!op->register_response) {
		ms_error("try to authenticate an unchallenged op [%p]",op);
		goto error;
	}
	authenticate = BELLE_SIP_HEADER_WWW_AUTHENTICATE(belle_sip_message_get_header(BELLE_SIP_MESSAGE(op->register_response),BELLE_SIP_WWW_AUTHENTICATE));
	if (authenticate) {
		authorization = belle_sip_auth_helper_create_authorization(authenticate);
	} else {
		/*proxy inerite from www*/
		authenticate = BELLE_SIP_HEADER_WWW_AUTHENTICATE(belle_sip_message_get_header(BELLE_SIP_MESSAGE(op->register_response),BELLE_SIP_PROXY_AUTHENTICATE));
		authorization = BELLE_SIP_HEADER_AUTHORIZATION(belle_sip_auth_helper_create_proxy_authorization(BELLE_SIP_HEADER_PROXY_AUTHENTICATE(authenticate)));
	}
	belle_sip_header_authorization_set_uri(authorization,belle_sip_request_get_uri(op->register_request));
	belle_sip_header_authorization_set_username(authorization,info->userid);

	if (belle_sip_auth_helper_fill_authorization(authorization
												,belle_sip_request_get_method(op->register_request)
												,ha1)) {
		belle_sip_object_unref(authorization);
		goto error;
	}
	belle_sip_message_set_header(BELLE_SIP_MESSAGE(op->register_request),BELLE_SIP_HEADER(authorization));
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
/*Call API*/
int sal_call_set_local_media_description(SalOp *h, SalMediaDescription *desc){
	ms_fatal("sal_call_set_local_media_description not implemented yet");
	return -1;
}
int sal_call(SalOp *h, const char *from, const char *to){
	ms_fatal("sal_call not implemented yet");
	return -1;
}
int sal_call_notify_ringing(SalOp *h, bool_t early_media){
	ms_fatal("sal_call_notify_ringing not implemented yet");
	return -1;
}
/*accept an incoming call or, during a call accept a reINVITE*/
int sal_call_accept(SalOp*h){
	ms_fatal("sal_call_accept not implemented yet");
	return -1;
}
int sal_call_decline(SalOp *h, SalReason reason, const char *redirection /*optional*/){
	ms_fatal("sal_call_decline not implemented yet");
	return -1;
}
int sal_call_update(SalOp *h, const char *subject){
	ms_fatal("sal_call_update not implemented yet");
	return -1;
}
SalMediaDescription * sal_call_get_remote_media_description(SalOp *h){
	ms_fatal("sal_call_get_remote_media_description not implemented yet");
	return NULL;
}
SalMediaDescription * sal_call_get_final_media_description(SalOp *h){
	ms_fatal("sal_call_get_final_media_description not implemented yet");
	return NULL;
}
int sal_call_refer(SalOp *h, const char *refer_to){
	ms_fatal("sal_call_refer not implemented yet");
	return -1;
}
int sal_call_refer_with_replaces(SalOp *h, SalOp *other_call_h){
	ms_fatal("sal_call_refer_with_replaces not implemented yet");
	return -1;
}
int sal_call_accept_refer(SalOp *h){
	ms_fatal("sal_call_accept_refer not implemented yet");
	return -1;
}
/*informs this call is consecutive to an incoming refer */
int sal_call_set_referer(SalOp *h, SalOp *refered_call){
	ms_fatal("sal_call_set_referer not implemented yet");
	return -1;
}
/* returns the SalOp of a call that should be replaced by h, if any */
SalOp *sal_call_get_replaces(SalOp *h){
	ms_fatal("sal_call_get_replaces not implemented yet");
	return NULL;
}
int sal_call_send_dtmf(SalOp *h, char dtmf){
	ms_fatal("sal_call_send_dtmf not implemented yet");
	return -1;
}
int sal_call_terminate(SalOp *h){
	ms_fatal("sal_call_terminate not implemented yet");
	return -1;
}
bool_t sal_call_autoanswer_asked(SalOp *op){
	ms_fatal("sal_call_autoanswer_asked not implemented yet");
	return -1;
}
void sal_call_send_vfu_request(SalOp *h){
	ms_fatal("sal_call_send_vfu_request not implemented yet");
	return ;
}
int sal_call_is_offerer(const SalOp *h){
	ms_fatal("sal_call_is_offerer not implemented yet");
	return -1;
}

static void process_authentication(SalOp *op, belle_sip_message_t *response) {
	/*only process a single header for now*/
	belle_sip_header_www_authenticate_t* authenticate;
	belle_sip_header_address_t* from = BELLE_SIP_HEADER_ADDRESS(belle_sip_message_get_header(response,BELLE_SIP_FROM));
	belle_sip_uri_t* uri = belle_sip_header_address_get_uri(from);
	authenticate = BELLE_SIP_HEADER_WWW_AUTHENTICATE(belle_sip_message_get_header(response,BELLE_SIP_WWW_AUTHENTICATE));
	if (!authenticate) {
		/*search for proxy authenticate*/
		authenticate = BELLE_SIP_HEADER_WWW_AUTHENTICATE(belle_sip_message_get_header(response,BELLE_SIP_PROXY_AUTHENTICATE));

	}


	op->auth_info.realm=(char*)belle_sip_header_www_authenticate_get_realm(authenticate);
	op->auth_info.username=(char*)belle_sip_uri_get_user(uri);
	if (authenticate) {
		if (op->base.root->callbacks.auth_requested(op,&op->auth_info)) {
			sal_op_authenticate(op,&op->auth_info);
		} else {
			ms_message("No auth info found for [%s] at [%s]",op->auth_info.username,op->auth_info.realm);
			sal_add_pending_auth(op->base.root,op);
		}
	} else {
		ms_error(" missing authenticate header");
	}

}

/**************************REGISTRATION***************************///////////
static void send_register_request(SalOp* op, belle_sip_request_t* request);

static void register_process_io_error(void *user_ctx, const belle_sip_io_error_event_t *event){
	ms_error("process_io_error not implemented yet");
}

static void register_refresh(SalOp* op) {
	op->registration_refresh_timer=0;
	belle_sip_header_cseq_t* cseq=(belle_sip_header_cseq_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(op->register_request),BELLE_SIP_CSEQ);
	belle_sip_header_cseq_set_seq_number(cseq,belle_sip_header_cseq_get_seq_number(cseq)+1);
	send_register_request(op,op->register_request);
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
	belle_sip_request_t* old_register_request=NULL;;
	belle_sip_response_t* old_register_response=NULL;;
	const belle_sip_list_t* contact_header_list;
	int response_code = belle_sip_response_get_status_code(response);
	int expires=-1;
	if (response_code<200) return;/*nothing to do*/

	/*begin
	 * maybe only the transaction should be kept*/
	old_register_request=op->register_request;
	op->register_request=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
	belle_sip_object_ref(op->register_request);
	if (old_register_request) belle_sip_object_unref(old_register_request);

	old_register_response=op->register_response;
	op->register_response=response; /*kept for use at authorization time*/
	belle_sip_object_ref(op->register_response);
	if (old_register_response) belle_sip_object_unref(old_register_response);
	/*end*/
	switch (response_code) {
	case 200: {

		contact_header_list = belle_sip_message_get_headers(BELLE_SIP_MESSAGE(response),BELLE_SIP_CONTACT);
		if (contact_header_list) {
			contact_header_list = belle_sip_list_find_custom((belle_sip_list_t*)contact_header_list,(belle_sip_compare_func)is_contact_equal, (const void*)sal_op_get_contact_address(op));
			if (!contact_header_list) {
				ms_error("no matching contact for [%s]", sal_op_get_contact(op));
				return;
			}
			expires=belle_sip_header_contact_get_expires(BELLE_SIP_HEADER_CONTACT(contact_header_list->data));

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

		op->base.root->callbacks.register_success(op,expires_header&&expires>=0);
		if (expires>0) {
			if (op->registration_refresh_timer>0) {
				belle_sip_main_loop_cancel_source(belle_sip_stack_get_main_loop(op->base.root->stack),op->registration_refresh_timer);
			}
			op->registration_refresh_timer = belle_sip_main_loop_add_timeout(belle_sip_stack_get_main_loop(op->base.root->stack),(belle_sip_source_func_t)register_refresh,op,expires*1000);
		}
		sal_remove_pending_auth(op->base.root,op);/*just in case*/
		break;
	}
	case 401:
	case 407:{

		process_authentication(op,BELLE_SIP_MESSAGE(response));
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



static void send_register_request(SalOp* op, belle_sip_request_t* request) {
	belle_sip_client_transaction_t* client_transaction;
	belle_sip_provider_t* prov=op->base.root->prov;
	op->callbacks.process_io_error=register_process_io_error;
	op->callbacks.process_response_event=register_response_event;
	op->callbacks.process_timeout=register_process_timeout;
	op->callbacks.process_transaction_terminated=register_process_transaction_terminated;
	client_transaction = belle_sip_provider_create_client_transaction(prov,request);
	belle_sip_transaction_set_application_data(BELLE_SIP_TRANSACTION(client_transaction),op);
	belle_sip_client_transaction_send_request(client_transaction);

}
/*if expire = -1, does not change expires*/
static void send_register_request_with_expires(SalOp* op, belle_sip_request_t* request,int expires) {
	belle_sip_header_expires_t* expires_header=(belle_sip_header_expires_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_EXPIRES);

	if (!expires_header) {
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(expires_header=belle_sip_header_expires_new()));
	}
	if (expires>=0) belle_sip_header_expires_set_expires(expires_header,expires);
	send_register_request(op,request);
}

int sal_register(SalOp *op, const char *proxy, const char *from, int expires){
	belle_sip_request_t *req;
	belle_sip_provider_t* prov=op->base.root->prov;
	belle_sip_header_contact_t* contact_header =belle_sip_header_contact_new();
	belle_sip_header_from_t* from_header;
	belle_sip_header_to_t* to_header;
	belle_sip_uri_t* req_uri;
	belle_sip_uri_t* contact_uri;
	sal_op_set_from(op,from);
	sal_op_set_to(op,from);
	belle_sip_uri_t* route_uri=NULL;
	belle_sip_header_route_t* route_header;
	char token[10];
	if (expires<0) goto error;
	from_header = belle_sip_header_from_create(from,belle_sip_random_token(token,sizeof(token)));
	if (!from_header) goto error;
	to_header=belle_sip_header_to_create(from,NULL);
	req_uri = (belle_sip_uri_t*)belle_sip_object_clone((belle_sip_object_t*)belle_sip_header_address_get_uri((belle_sip_header_address_t*)to_header));
	belle_sip_uri_set_user(req_uri,NULL);

	if (sal_op_get_contact(op))
		contact_uri= belle_sip_uri_parse(sal_op_get_contact(op));
	else
		contact_uri=belle_sip_uri_new();

	if (!contact_uri) goto error;
	belle_sip_header_address_set_uri((belle_sip_header_address_t*)contact_header,contact_uri);
	sal_op_set_route(op,proxy);
	/*FIXME use route info if needed*/
	if (proxy) {
		route_uri=belle_sip_uri_parse(proxy);
	}

	req=belle_sip_request_create(
							req_uri,
		                    "REGISTER",
		                    belle_sip_provider_create_call_id(prov),
		                    belle_sip_header_cseq_create(20,"REGISTER"),
		                    from_header,
		                    to_header,
		                    belle_sip_header_via_new(),
		                    70);


	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(contact_header));

	if (route_uri && !belle_sip_uri_equals(req_uri,route_uri)) {
		route_header = belle_sip_header_route_new();
		belle_sip_uri_set_lr_param(route_uri,1);
		belle_sip_header_address_set_uri(BELLE_SIP_HEADER_ADDRESS(route_header),route_uri);
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(route_header));
	} else if (route_uri){
		belle_sip_object_unref(route_uri);
		route_uri=NULL;
	}
	send_register_request_with_expires(op,req,expires);

return 0;
error:
	ms_error("Cannot initiate register to [%s] for [%s], expire [%i]",proxy,from,expires);
	if (contact_header) belle_sip_object_unref(contact_header);
	if (from_header) belle_sip_object_unref(from_header);
	if (to_header) belle_sip_object_unref(to_header);
	if (route_uri) belle_sip_object_unref(route_uri);
	return -1;
}

int sal_register_refresh(SalOp *op, int expires){
	belle_sip_header_cseq_t* cseq=(belle_sip_header_cseq_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(op->register_request),BELLE_SIP_CSEQ);
	belle_sip_header_cseq_set_seq_number(cseq,belle_sip_header_cseq_get_seq_number(cseq)+1);
	send_register_request_with_expires(op,op->register_request,expires);
	return 0;
}
int sal_unregister(SalOp *op){
	belle_sip_header_cseq_t* cseq=(belle_sip_header_cseq_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(op->register_request),BELLE_SIP_CSEQ);
	belle_sip_header_cseq_set_seq_number(cseq,belle_sip_header_cseq_get_seq_number(cseq)+1);
	send_register_request_with_expires(op,op->register_request,0);
	return 0;
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


