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
#include "sal.h"
#include "belle-sip/belle-sip.h"
/**/


/* Address manipulation API*/
SalAddress * sal_address_new(const char *uri){
	belle_sip_header_address_t*  result;
	if (uri) {
		return (SalAddress *)belle_sip_header_address_parse (uri);
	} else {
		result = belle_sip_header_address_new();
		belle_sip_header_address_set_uri(result,belle_sip_uri_new());
		return (SalAddress *)result;
	}
}
SalAddress * sal_address_clone(const SalAddress *addr){
	return (SalAddress *) belle_sip_object_clone(BELLE_SIP_OBJECT(addr));
}
const char *sal_address_get_scheme(const SalAddress *addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);
	if (uri) {
		if (belle_sip_uri_is_secure(uri)) return "sips";
		else return "sip";
	} else
		return NULL;
}
const char *sal_address_get_display_name(const SalAddress* addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	return belle_sip_header_address_get_displayname(header_addr);

}
const char *sal_address_get_display_name_unquoted(const SalAddress *addr){
	return sal_address_get_display_name(addr);
}
#define SAL_ADDRESS_GET(addr,param) \
belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);\
belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);\
if (uri) {\
	return belle_sip_uri_get_##param(uri);\
} else\
	return NULL;

#define SAL_ADDRESS_SET(addr,param,value) \
belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);\
belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);\
belle_sip_uri_set_##param(uri,value);

const char *sal_address_get_username(const SalAddress *addr){
	SAL_ADDRESS_GET(addr,user)
}
const char *sal_address_get_domain(const SalAddress *addr){
	SAL_ADDRESS_GET(addr,host)
}
const char * sal_address_get_port(const SalAddress *addr){
	ms_fatal("sal_address_get_port not implemented yet");
	return NULL;
}
int sal_address_get_port_int(const SalAddress *addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);
	if (uri) {
		return belle_sip_uri_get_port(uri);
	} else
		return -1;
}
SalTransport sal_address_get_transport(const SalAddress* addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);
	if (uri) {
		return sal_transport_parse(belle_sip_uri_get_transport_param(uri));
	} else
		return SalTransportUDP;
};

void sal_address_set_display_name(SalAddress *addr, const char *display_name){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_header_address_set_displayname(header_addr,display_name);
}
void sal_address_set_username(SalAddress *addr, const char *username){
	SAL_ADDRESS_SET(addr,user,username);
}
void sal_address_set_domain(SalAddress *addr, const char *host){
	SAL_ADDRESS_SET(addr,host,host);
}
void sal_address_set_port(SalAddress *addr, const char *port){
	SAL_ADDRESS_SET(addr,port,atoi(port));
}
void sal_address_set_port_int(SalAddress *addr, int port){
	SAL_ADDRESS_SET(addr,port,port);
}
void sal_address_clean(SalAddress *addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_header_address_set_displayname(header_addr,NULL);
	belle_sip_object_unref(belle_sip_header_address_get_uri(header_addr));
	belle_sip_header_address_set_uri(header_addr,belle_sip_uri_new());
	return ;
}
char *sal_address_as_string(const SalAddress *addr){
	return belle_sip_object_to_string(BELLE_SIP_OBJECT(addr));
}
char *sal_address_as_string_uri_only(const SalAddress *addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_uri_t* uri = belle_sip_header_address_get_uri(header_addr);
	return belle_sip_object_to_string(BELLE_SIP_OBJECT(uri));
}
void sal_address_destroy(SalAddress *addr){
	belle_sip_header_address_t* header_addr = BELLE_SIP_HEADER_ADDRESS(addr);
	belle_sip_object_unref(header_addr);
	return ;
}
void sal_address_set_param(SalAddress *addr,const char* name,const char* value){
	belle_sip_parameters_t* parameters = BELLE_SIP_PARAMETERS(addr);
	belle_sip_parameters_set_parameter(parameters,name,value);
	return ;
}
void sal_address_set_transport(SalAddress* addr,SalTransport transport){
	SAL_ADDRESS_SET(addr,transport_param,sal_transport_to_string(transport));
}


struct Sal{
	SalCallbacks callbacks;
	belle_sip_listener_callbacks_t listener_callbacks;
	belle_sip_stack_t* stack;
	belle_sip_provider_t *prov;
	void *up; /*user pointer*/
};


struct SalOp{
	SalOpBase base;
	belle_sip_listener_callbacks_t callbacks;
	belle_sip_request_t* register_request;
	unsigned long int  registration_refresh_timer;
};

void sal_enable_logs(){
	belle_sip_set_log_level(BELLE_SIP_LOG_MESSAGE);
}
void sal_disable_logs() {
	belle_sip_set_log_level(BELLE_SIP_LOG_ERROR);
}
static void process_dialog_terminated(void *user_ctx, const belle_sip_dialog_terminated_event_t *event){
	ms_error("process_dialog_terminated not implemented yet");
}
static void process_io_error(void *user_ctx, const belle_sip_io_error_event_t *event){
	ms_error("process_io_error not implemented yet");
}
static void process_request_event(void *user_ctx, const belle_sip_request_event_t *event) {
	ms_error("process_request_event not implemented yet");
}
static void process_response_event(void *user_ctx, const belle_sip_response_event_t *event){
	belle_sip_client_transaction_t* client_transaction = belle_sip_response_event_get_client_transaction(event);
	SalOp* op = (SalOp*)belle_sip_transaction_get_application_data(BELLE_SIP_TRANSACTION(client_transaction));
	if (op->callbacks.process_response_event) {
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
	if (ctx->callbacks.auth_requested==NULL)
		ctx->callbacks.auth_requested=(SalOnAuthRequested)unimplemented_stub;
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
	ms_error("sal_use_session_timers not implemented yet");
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
	belle_sip_stack_sleep(sal->stack,1);
	return 0;
}
MSList * sal_get_pending_auths(Sal *sal){
	ms_fatal("sal_get_pending_auths not implemented yet");
	return NULL;
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
void sal_op_authenticate(SalOp *h, const SalAuthInfo *info){
	ms_fatal("sal_op_authenticate not implemented yet");
	return ;
}
void sal_op_cancel_authentication(SalOp *h){
	ms_fatal("sal_op_cancel_authentication not implemented yet");
	return ;
}

int sal_op_get_auth_requested(SalOp *h, const char **realm, const char **username){
	ms_fatal("sal_op_get_auth_requested not implemented yet");
	return -1;
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
static void register_response_event(void *user_ctx, const belle_sip_response_event_t *event){
	belle_sip_client_transaction_t* client_transaction = belle_sip_response_event_get_client_transaction(event);
	SalOp* op = (SalOp*)belle_sip_transaction_get_application_data(BELLE_SIP_TRANSACTION(client_transaction));
	belle_sip_response_t* response = belle_sip_response_event_get_response(event);
	belle_sip_header_expires_t* expires_header;
	belle_sip_request_t* old_register_request;
	int response_code = belle_sip_response_get_status_code(response);
	if (response_code<200) return;/*nothing to do*/
	switch (response_code) {
	case 200: {
		expires_header=(belle_sip_header_expires_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_EXPIRES);
		op->base.root->callbacks.register_success(op,expires_header&&belle_sip_header_expires_get_expires(expires_header)>0);
		old_register_request=op->register_request;
		op->register_request=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
		belle_sip_object_ref(op->register_request);
		if (old_register_request) belle_sip_object_unref(old_register_request);
		/*FIXME schedule refresh cb*/
		if (belle_sip_header_expires_get_expires(expires_header)>0) {
			if (op->registration_refresh_timer>0) {
				belle_sip_main_loop_cancel_source(belle_sip_stack_get_main_loop(op->base.root->stack),op->registration_refresh_timer);
			}
			op->registration_refresh_timer = belle_sip_main_loop_add_timeout(belle_sip_stack_get_main_loop(op->base.root->stack),(belle_sip_source_func_t)register_refresh,op,belle_sip_header_expires_get_expires(expires_header)*1000);
		}
		break;
	}
	default:{
		ms_error("Unexpected answer [%s] for registration request bound to [%s]",belle_sip_response_get_reason_phrase(response),op->base.from);
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
static void send_register_request_with_expires(SalOp* op, belle_sip_request_t* request,int expires) {
	belle_sip_header_expires_t* expires_header=(belle_sip_header_expires_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_EXPIRES);

	if (!expires_header) {
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(expires_header=belle_sip_header_expires_new()));
	}
	belle_sip_header_expires_set_expires(expires_header,expires);
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
	send_register_request_with_expires(op,req,expires);

return 0;
error:
	ms_error("Cannot initiate register to [%s] for [%s], expire [%i]",proxy,from,expires);
	if (contact_header) belle_sip_object_unref(contact_header);
	if (from_header) belle_sip_object_unref(from_header);
	if (to_header) belle_sip_object_unref(to_header);
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



#define payload_type_set_number(pt,n)	(pt)->user_data=(void*)((long)n);
#define payload_type_get_number(pt)		((int)(long)(pt)->user_data)

/*misc*/
void sal_get_default_local_ip(Sal *sal, int address_family, char *ip, size_t iplen){
	ms_fatal("sal_get_default_local_ip not implemented yet");
	return ;
}
