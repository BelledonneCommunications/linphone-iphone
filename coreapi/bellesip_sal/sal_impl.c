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

void sal_enable_logs(){
	belle_sip_set_log_level(BELLE_SIP_LOG_MESSAGE);
}
void sal_disable_logs() {
	belle_sip_set_log_level(BELLE_SIP_LOG_ERROR);
}
static void sal_add_pending_auth(Sal *sal, SalOp *op){
	sal->pending_auths=ms_list_append(sal->pending_auths,op);
}

 void sal_remove_pending_auth(Sal *sal, SalOp *op){
	sal->pending_auths=ms_list_remove(sal->pending_auths,op);
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
static void process_dialog_terminated(void *user_ctx, const belle_sip_dialog_terminated_event_t *event){
	ms_error("process_dialog_terminated not implemented yet");
}
static void process_io_error(void *user_ctx, const belle_sip_io_error_event_t *event){
	ms_error("process_io_error not implemented yet");
}
static void process_request_event(void *user_ctx, const belle_sip_request_event_t *event) {
	belle_sip_server_transaction_t* server_transaction = belle_sip_request_event_get_server_transaction(event);
	SalOp* op = (SalOp*)belle_sip_transaction_get_application_data(BELLE_SIP_TRANSACTION(server_transaction));
	ms_error("sal process_request_event not implemented yet");
}

static void process_response_event(void *user_ctx, const belle_sip_response_event_t *event){
	belle_sip_client_transaction_t* client_transaction = belle_sip_response_event_get_client_transaction(event);
	SalOp* op = (SalOp*)belle_sip_transaction_get_application_data(BELLE_SIP_TRANSACTION(client_transaction));
	belle_sip_response_t* response = belle_sip_response_event_get_response(event);
	belle_sip_header_address_t* contact_address;
	belle_sip_header_via_t* via_header;
	belle_sip_uri_t* contact_uri;
	unsigned int contact_port;
	const char* received;
	int rport;
	bool_t contact_updated=FALSE;
	char* new_contact;
	belle_sip_request_t* old_request=NULL;;
	belle_sip_response_t* old_response=NULL;;
	int response_code = belle_sip_response_get_status_code(response);

	if (op->callbacks.process_response_event) {
		/*Fix contact if needed*/
		via_header= (belle_sip_header_via_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_VIA);
		received = belle_sip_header_via_get_received(via_header);
		rport = belle_sip_header_via_get_rport(via_header);
		if (received!=NULL || rport>0) {
			if (sal_op_get_contact(op)){
				contact_address = BELLE_SIP_HEADER_ADDRESS(sal_address_clone(sal_op_get_contact_address(op)));
				contact_uri=belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(contact_address));
				if (received && strcmp(received,belle_sip_uri_get_host(contact_uri))!=0) {
					/*need to update host*/
					belle_sip_uri_set_host(contact_uri,received);
					contact_updated=TRUE;
				}
				contact_port =  belle_sip_uri_get_port(contact_uri);
				if (rport>0 && rport!=contact_port && (contact_port+rport)!=5060) {
					/*need to update port*/
					belle_sip_uri_set_port(contact_uri,rport);
					contact_updated=TRUE;
				}
				if (contact_updated) {
					new_contact=belle_sip_object_to_string(BELLE_SIP_OBJECT(contact_address));
					ms_message("Updating contact from [%s] to [%s] for [%p]",sal_op_get_contact(op),new_contact,op);
					sal_op_set_contact(op,new_contact);
					belle_sip_free(new_contact);
				}
				belle_sip_object_unref(contact_address);
			}

		}
		/*update request/response
		 * maybe only the transaction should be kept*/
		old_request=op->request;
		op->request=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
		belle_sip_object_ref(op->request);
		if (old_request) belle_sip_object_unref(old_request);

		old_response=op->response;
		op->response=response; /*kept for use at authorization time*/
		belle_sip_object_ref(op->response);
		if (old_response) belle_sip_object_unref(old_response);

		/*handle authozation*/
		switch (response_code) {
		case 200: {
			sal_remove_pending_auth(op->base.root,op);/*just in case*/
			break;
		}
		case 401:
		case 407:{

			process_authentication(op,BELLE_SIP_MESSAGE(response));
			return;
		}
		}
		op->callbacks.process_response_event(op,event);
	} else {
		ms_error("Unhandled event response [%p]",event);
	}

}
static void process_timeout(void *user_ctx, const belle_sip_timeout_event_t *event) {
/*	belle_sip_client_transaction_t* client_transaction = belle_sip_timeout_event_get_client_transaction(event);
	SalOp* op = (SalOp*)belle_sip_transaction_get_application_data(BELLE_SIP_TRANSACTION(client_transaction));
	if (op->callbacks.process_timeout) {
		op->callbacks.process_timeout(op,event);
	} else*/ {
		ms_error("Unhandled event timeout [%p]",event);
	}
}
static void process_transaction_terminated(void *user_ctx, const belle_sip_transaction_terminated_event_t *event) {
/*	belle_sip_client_transaction_t* client_transaction = belle_sip_transaction_terminated_event_get_client_transaction(event);
	SalOp* op = (SalOp*)belle_sip_transaction_get_application_data(client_transaction);
	if (op->calbacks.process_transaction_terminated) {
		op->calbacks.process_transaction_terminated(op,event);
	} else */{
		ms_error("Unhandled transaction terminated [%p]",event);
	}
}

Sal * sal_init(){
	Sal * sal=ms_new0(Sal,1);
	sal->stack = belle_sip_stack_new(NULL);
	sal->prov = belle_sip_stack_create_provider(sal->stack,NULL);
	sal->listener_callbacks.process_dialog_terminated=process_dialog_terminated;
	sal->listener_callbacks.process_io_error=process_io_error;
	sal->listener_callbacks.process_request_event=process_request_event;
	sal->listener_callbacks.process_response_event=process_response_event;
	sal->listener_callbacks.process_timeout=process_timeout;
	sal->listener_callbacks.process_transaction_terminated=process_transaction_terminated;
	belle_sip_provider_add_sip_listener(sal->prov,belle_sip_listener_create_from_callbacks(&sal->listener_callbacks,sal));
	return sal;
}
void sal_set_user_pointer(Sal *sal, void *user_data){
	sal->up=user_data;
}

void *sal_get_user_pointer(const Sal *sal){
	return sal->up;
}

static void unimplemented_stub(){
	ms_warning("Unimplemented SAL callback");
}

void sal_set_callbacks(Sal *ctx, const SalCallbacks *cbs){
	memcpy(&ctx->callbacks,cbs,sizeof(*cbs));
	if (ctx->callbacks.call_received==NULL)
		ctx->callbacks.call_received=(SalOnCallReceived)unimplemented_stub;
	if (ctx->callbacks.call_ringing==NULL)
		ctx->callbacks.call_ringing=(SalOnCallRinging)unimplemented_stub;
	if (ctx->callbacks.call_accepted==NULL)
		ctx->callbacks.call_accepted=(SalOnCallAccepted)unimplemented_stub;
	if (ctx->callbacks.call_failure==NULL)
		ctx->callbacks.call_failure=(SalOnCallFailure)unimplemented_stub;
	if (ctx->callbacks.call_terminated==NULL)
		ctx->callbacks.call_terminated=(SalOnCallTerminated)unimplemented_stub;
	if (ctx->callbacks.call_released==NULL)
		ctx->callbacks.call_released=(SalOnCallReleased)unimplemented_stub;
	if (ctx->callbacks.call_updating==NULL)
		ctx->callbacks.call_updating=(SalOnCallUpdating)unimplemented_stub;
	if (ctx->callbacks.auth_requested_legacy==NULL)
		ctx->callbacks.auth_requested_legacy=(SalOnAuthRequestedLegacy)unimplemented_stub;
	if (ctx->callbacks.auth_success==NULL)
		ctx->callbacks.auth_success=(SalOnAuthSuccess)unimplemented_stub;
	if (ctx->callbacks.register_success==NULL)
		ctx->callbacks.register_success=(SalOnRegisterSuccess)unimplemented_stub;
	if (ctx->callbacks.register_failure==NULL)
		ctx->callbacks.register_failure=(SalOnRegisterFailure)unimplemented_stub;
	if (ctx->callbacks.dtmf_received==NULL)
		ctx->callbacks.dtmf_received=(SalOnDtmfReceived)unimplemented_stub;
	if (ctx->callbacks.notify==NULL)
		ctx->callbacks.notify=(SalOnNotify)unimplemented_stub;
	if (ctx->callbacks.notify_presence==NULL)
		ctx->callbacks.notify_presence=(SalOnNotifyPresence)unimplemented_stub;
	if (ctx->callbacks.subscribe_received==NULL)
		ctx->callbacks.subscribe_received=(SalOnSubscribeReceived)unimplemented_stub;
	if (ctx->callbacks.text_received==NULL)
		ctx->callbacks.text_received=(SalOnTextReceived)unimplemented_stub;
	if (ctx->callbacks.ping_reply==NULL)
		ctx->callbacks.ping_reply=(SalOnPingReply)unimplemented_stub;
	if (ctx->callbacks.auth_requested==NULL)
		ctx->callbacks.auth_requested=(SalOnAuthRequested)unimplemented_stub;
}



void sal_uninit(Sal* sal){
	belle_sip_object_unref(sal->prov);
	belle_sip_object_unref(sal->stack);
	ms_free(sal);
	return ;
};

int sal_listen_port(Sal *ctx, const char *addr, int port, SalTransport tr, int is_secure){
	int result;
	belle_sip_listening_point_t* lp = belle_sip_stack_create_listening_point(ctx->stack,addr,port,sal_transport_to_string(tr));
	if (lp) {
		result = belle_sip_provider_add_listening_point(ctx->prov,lp);
		belle_sip_object_unref(lp);
	} else {
		return -1;
	}
	return result;
}
static void remove_listening_point(belle_sip_listening_point_t* lp,belle_sip_provider_t* prov) {
	belle_sip_provider_remove_listening_point(prov,lp);
}
int sal_unlisten_ports(Sal *ctx){
	const belle_sip_list_t * lps = belle_sip_provider_get_listening_points(ctx->prov);
	belle_sip_list_t * tmp_list = belle_sip_list_copy(lps);
	belle_sip_list_for_each2 (tmp_list,(void (*)(void*,void*))remove_listening_point,ctx->prov);
	belle_sip_list_free(tmp_list);

	ms_message("sal_unlisten_ports done");
	return 0;
}
ortp_socket_t sal_get_socket(Sal *ctx){
	ms_fatal("sal_get_socket not implemented yet");
	return -1;
}
void sal_set_user_agent(Sal *ctx, const char *user_agent){
	ms_error("sal_set_user_agent not implemented yet");
	return ;
}
/*keepalive period in ms*/
void sal_set_keepalive_period(Sal *ctx,unsigned int value){
	ms_error("sal_set_keepalive_period not implemented yet");
	return ;
}
/**
 * returns keepalive period in ms
 * 0 desactiaved
 * */
unsigned int sal_get_keepalive_period(Sal *ctx){
	ms_fatal("sal_get_keepalive_period not implemented yet");
	return -1;
}
void sal_use_session_timers(Sal *ctx, int expires){
	ctx->session_expires=expires;
	return ;
}
void sal_use_double_registrations(Sal *ctx, bool_t enabled){
	ms_error("sal_use_double_registrations not implemented yet");
	return ;
}
void sal_reuse_authorization(Sal *ctx, bool_t enabled){
	ms_error("sal_reuse_authorization not implemented yet");
	return ;
}
void sal_use_one_matching_codec_policy(Sal *ctx, bool_t one_matching_codec){
	ms_error("sal_use_one_matching_codec_policy not implemented yet");
	return ;
}
void sal_use_rport(Sal *ctx, bool_t use_rports){
	ms_error("sal_use_rport not implemented yet");
	return ;
}
void sal_use_101(Sal *ctx, bool_t use_101){
	ms_error("sal_use_101 not implemented yet");
	return ;
}
void sal_set_root_ca(Sal* ctx, const char* rootCa){
	ms_error("sal_set_root_ca not implemented yet");
	return ;
}
void sal_verify_server_certificates(Sal *ctx, bool_t verify){
	ms_error("sal_verify_server_certificates not implemented yet");
	return ;
}

int sal_iterate(Sal *sal){
	/*FIXME should be zero*/
	belle_sip_stack_sleep(sal->stack,0);
	return 0;
}
MSList * sal_get_pending_auths(Sal *sal){
	return ms_list_copy(sal->pending_auths);
}

#define payload_type_set_number(pt,n)	(pt)->user_data=(void*)((long)n);
#define payload_type_get_number(pt)		((int)(long)(pt)->user_data)

/*misc*/
void sal_get_default_local_ip(Sal *sal, int address_family, char *ip, size_t iplen){
	ms_fatal("sal_get_default_local_ip not implemented yet");
	return ;
}
