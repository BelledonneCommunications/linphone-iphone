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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

typedef struct belle_sip_certificates_chain_t _SalCertificatesChain;
typedef struct belle_sip_signing_key_t _SalSigningKey;

/*
rfc3323
4.2 Expressing Privacy Preferences
When a Privacy header is constructed, it MUST consist of either the
   value 'none', or one or more of the values 'user', 'header' and
   'session' (each of which MUST appear at most once) which MAY in turn
   be followed by the 'critical' indicator.
 */
void sal_op_set_privacy_from_message(SalOp* op,belle_sip_message_t* msg) {
	belle_sip_header_privacy_t* privacy = belle_sip_message_get_header_by_type(msg,belle_sip_header_privacy_t);
	if (!privacy) {
		sal_op_set_privacy(op,SalPrivacyNone);
	} else {
		belle_sip_list_t* privacy_list=belle_sip_header_privacy_get_privacy(privacy);
		sal_op_set_privacy(op,0);
		for (;privacy_list!=NULL;privacy_list=privacy_list->next) {
			char* privacy_value=(char*)privacy_list->data;
			if(strcmp(sal_privacy_to_string(SalPrivacyCritical),privacy_value) == 0)
				sal_op_set_privacy(op,sal_op_get_privacy(op)|SalPrivacyCritical);
			if(strcmp(sal_privacy_to_string(SalPrivacyHeader),privacy_value) == 0)
							sal_op_set_privacy(op,sal_op_get_privacy(op)|SalPrivacyHeader);
			if(strcmp(sal_privacy_to_string(SalPrivacyId),privacy_value) == 0)
							sal_op_set_privacy(op,sal_op_get_privacy(op)|SalPrivacyId);
			if(strcmp(sal_privacy_to_string(SalPrivacyNone),privacy_value) == 0) {
				sal_op_set_privacy(op,SalPrivacyNone);
				break;
			}
			if(strcmp(sal_privacy_to_string(SalPrivacySession),privacy_value) == 0)
							sal_op_set_privacy(op,sal_op_get_privacy(op)|SalPrivacySession);
			if(strcmp(sal_privacy_to_string(SalPrivacyUser),privacy_value) == 0)
										sal_op_set_privacy(op,sal_op_get_privacy(op)|SalPrivacyUser);
		}
	}
}
static void set_tls_properties(Sal *ctx);

void _belle_sip_log(belle_sip_log_level lev, const char *fmt, va_list args) {
	int ortp_level;
	switch(lev) {
		case BELLE_SIP_LOG_FATAL:
			ortp_level=ORTP_FATAL;
		break;
		case BELLE_SIP_LOG_ERROR:
			ortp_level=ORTP_ERROR;
		break;
		case BELLE_SIP_LOG_WARNING:
			ortp_level=ORTP_WARNING;
		break;
		case BELLE_SIP_LOG_MESSAGE:
			ortp_level=ORTP_MESSAGE;
		break;
		case BELLE_SIP_LOG_DEBUG:
		default:
			ortp_level=ORTP_DEBUG;
			break;
	}
	if (ortp_log_level_enabled(ortp_level)){
		ortp_logv(ortp_level,fmt,args);
	}
}

void sal_enable_logs(){
	belle_sip_set_log_level(BELLE_SIP_LOG_MESSAGE);
}

void sal_disable_logs() {
	belle_sip_set_log_level(BELLE_SIP_LOG_ERROR);
}

void sal_add_pending_auth(Sal *sal, SalOp *op){
	if (ms_list_find(sal->pending_auths,op)==NULL){
		sal->pending_auths=ms_list_append(sal->pending_auths,op);
		op->has_auth_pending=TRUE;
	}
}

void sal_remove_pending_auth(Sal *sal, SalOp *op){
	if (op->has_auth_pending){
		op->has_auth_pending=FALSE;
		if (ms_list_find(sal->pending_auths,op)){
			sal->pending_auths=ms_list_remove(sal->pending_auths,op);
		}
	}
}

void sal_process_authentication(SalOp *op) {
	belle_sip_request_t* initial_request=belle_sip_transaction_get_request((belle_sip_transaction_t*)op->pending_auth_transaction);
	belle_sip_request_t* new_request;
	bool_t is_within_dialog=FALSE;
	belle_sip_list_t* auth_list=NULL;
	belle_sip_auth_event_t* auth_event;
	belle_sip_response_t *response=belle_sip_transaction_get_response((belle_sip_transaction_t*)op->pending_auth_transaction);
	belle_sip_header_from_t *from=belle_sip_message_get_header_by_type(initial_request,belle_sip_header_from_t);
	belle_sip_uri_t *from_uri=belle_sip_header_address_get_uri((belle_sip_header_address_t*)from);

	if (strcasecmp(belle_sip_uri_get_host(from_uri),"anonymous.invalid")==0){
		/*prefer using the from from the SalOp*/
		from_uri=belle_sip_header_address_get_uri((belle_sip_header_address_t*)sal_op_get_from_address(op));
	}

	if (op->dialog && belle_sip_dialog_get_state(op->dialog)==BELLE_SIP_DIALOG_CONFIRMED) {
		new_request = belle_sip_dialog_create_request_from(op->dialog,initial_request);
		if (!new_request)
			new_request = belle_sip_dialog_create_queued_request_from(op->dialog,initial_request);
		is_within_dialog=TRUE;
	} else {
		new_request=initial_request;
		belle_sip_message_remove_header(BELLE_SIP_MESSAGE(new_request),BELLE_SIP_AUTHORIZATION);
		belle_sip_message_remove_header(BELLE_SIP_MESSAGE(new_request),BELLE_SIP_PROXY_AUTHORIZATION);
	}
	if (new_request==NULL) {
		ms_error("sal_process_authentication() op=[%p] cannot obtain new request from dialog.",op);
		return;
	}

	if (belle_sip_provider_add_authorization(op->base.root->prov,new_request,response,from_uri,&auth_list,op->base.realm)) {
		if (is_within_dialog) {
			sal_op_send_request(op,new_request);
		} else {
			sal_op_resend_request(op,new_request);
		}
		sal_remove_pending_auth(op->base.root,op);
	}else {
		belle_sip_header_from_t *from=belle_sip_message_get_header_by_type(response,belle_sip_header_from_t);
		char *tmp=belle_sip_object_to_string(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(from)));
		ms_message("No auth info found for [%s]",tmp);
		belle_sip_free(tmp);
		sal_add_pending_auth(op->base.root,op);

		if (is_within_dialog) {
			belle_sip_object_unref(new_request);
		}
	}
	/*always store auth info, for case of wrong credential*/
	if (op->auth_info) {
		sal_auth_info_delete(op->auth_info);
		op->auth_info=NULL;
	}
	if (auth_list){
		auth_event=(belle_sip_auth_event_t*)(auth_list->data);
		op->auth_info=sal_auth_info_create(auth_event);
		belle_sip_list_free_with_data(auth_list,(void (*)(void*))belle_sip_auth_event_destroy);
	}
}

static void process_dialog_terminated(void *sal, const belle_sip_dialog_terminated_event_t *event){
	belle_sip_dialog_t* dialog =  belle_sip_dialog_terminated_event_get_dialog(event);
	SalOp* op = belle_sip_dialog_get_application_data(dialog);
	if (op && op->callbacks && op->callbacks->process_dialog_terminated) {
		op->callbacks->process_dialog_terminated(op,event);
	} else {
		ms_error("sal process_dialog_terminated no op found for this dialog [%p], ignoring",dialog);
	}
}

static void process_io_error(void *user_ctx, const belle_sip_io_error_event_t *event){
	belle_sip_client_transaction_t*client_transaction;
	SalOp* op;
	if (BELLE_SIP_OBJECT_IS_INSTANCE_OF(belle_sip_io_error_event_get_source(event),belle_sip_client_transaction_t)) {
		client_transaction=BELLE_SIP_CLIENT_TRANSACTION(belle_sip_io_error_event_get_source(event));
		op = (SalOp*)belle_sip_transaction_get_application_data(BELLE_SIP_TRANSACTION(client_transaction));
		/*also reset auth count on IO error*/
		op->auth_requests=0;
		if (op->callbacks && op->callbacks->process_io_error) {
			op->callbacks->process_io_error(op,event);
		}
	} else {
		/*ms_error("sal process_io_error not implemented yet for non transaction");*/
		/*nop, because already handle at transaction layer*/
	}
}

static void process_request_event(void *ud, const belle_sip_request_event_t *event) {
	Sal *sal=(Sal*)ud;
	SalOp* op=NULL;
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_dialog_t* dialog=belle_sip_request_event_get_dialog(event);
	belle_sip_header_address_t* origin_address;
	belle_sip_header_address_t* address=NULL;
	belle_sip_header_from_t* from_header;
	belle_sip_header_to_t* to;
	belle_sip_response_t* resp;
	belle_sip_header_t *evh;
	const char *method=belle_sip_request_get_method(req);
	belle_sip_header_contact_t* remote_contact = belle_sip_message_get_header_by_type(req, belle_sip_header_contact_t);

	from_header=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_from_t);

	if (dialog) {
		op=(SalOp*)belle_sip_dialog_get_application_data(dialog);
		if (op==NULL || op->state==SalOpStateTerminated){
			ms_warning("Receiving request for null or terminated op [%p], ignored",op);
			return;
		}
	}else if (strcmp("INVITE",method)==0) {
		op=sal_op_new(sal);
		op->dir=SalOpDirIncoming;
		sal_op_call_fill_cbs(op);
	}else if ((strcmp("SUBSCRIBE",method)==0 || strcmp("NOTIFY",method)==0) && (evh=belle_sip_message_get_header(BELLE_SIP_MESSAGE(req),"Event"))!=NULL) {
		op=sal_op_new(sal);
		op->dir=SalOpDirIncoming;
		if (strncmp(belle_sip_header_get_unparsed_value(evh),"presence",strlen("presence"))==0){
			sal_op_presence_fill_cbs(op);
		}else
			sal_op_subscribe_fill_cbs(op);
	}else if (strcmp("MESSAGE",method)==0) {
		op=sal_op_new(sal);
		op->dir=SalOpDirIncoming;
		sal_op_message_fill_cbs(op);
	}else if (strcmp("OPTIONS",method)==0) {
		resp=belle_sip_response_create_from_request(req,200);
		belle_sip_provider_send_response(sal->prov,resp);
		return;
	}else if (strcmp("INFO",method)==0) {
		resp=belle_sip_response_create_from_request(req,481);/*INFO out of call dialogs are not allowed*/
		belle_sip_provider_send_response(sal->prov,resp);
		return;
	}else if (strcmp("BYE",method)==0) {
		resp=belle_sip_response_create_from_request(req,481);/*out of dialog BYE */
		belle_sip_provider_send_response(sal->prov,resp);
		return;
	}else if (strcmp("CANCEL",method)==0) {
		resp=belle_sip_response_create_from_request(req,481);/*out of dialog CANCEL */
		belle_sip_provider_send_response(sal->prov,resp);
		return;
	}else if (sal->enable_test_features && strcmp("PUBLISH",method)==0) {
		resp=belle_sip_response_create_from_request(req,200);/*out of dialog BYE */
		belle_sip_message_add_header((belle_sip_message_t*)resp,belle_sip_header_create("SIP-Etag","4441929FFFZQOA"));
		belle_sip_provider_send_response(sal->prov,resp);
		return;
	}else {
		ms_error("sal process_request_event not implemented yet for method [%s]",belle_sip_request_get_method(req));
		resp=belle_sip_response_create_from_request(req,405);
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(resp)
									,BELLE_SIP_HEADER(belle_sip_header_allow_create("INVITE, CANCEL, ACK, BYE, SUBSCRIBE, NOTIFY, MESSAGE, OPTIONS, INFO")));
		belle_sip_provider_send_response(sal->prov,resp);
		return;
	}

	if (!op->base.from_address)  {
		if (belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(from_header)))
			address=belle_sip_header_address_create(belle_sip_header_address_get_displayname(BELLE_SIP_HEADER_ADDRESS(from_header))
					,belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(from_header)));
		else if ((belle_sip_header_address_get_absolute_uri(BELLE_SIP_HEADER_ADDRESS(from_header))))
			address=belle_sip_header_address_create2(belle_sip_header_address_get_displayname(BELLE_SIP_HEADER_ADDRESS(from_header))
					,belle_sip_header_address_get_absolute_uri(BELLE_SIP_HEADER_ADDRESS(from_header)));
		else
			ms_error("Cannot not find from uri from request [%p]",req);
		sal_op_set_from_address(op,(SalAddress*)address);
		belle_sip_object_unref(address);
	}

	if( remote_contact ){
		__sal_op_set_remote_contact(op, belle_sip_header_get_unparsed_value(BELLE_SIP_HEADER(remote_contact)));
	}

	if (!op->base.to_address) {
		to=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_to_t);
		if (belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(to)))
			address=belle_sip_header_address_create(belle_sip_header_address_get_displayname(BELLE_SIP_HEADER_ADDRESS(to))
					,belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(to)));
		else if ((belle_sip_header_address_get_absolute_uri(BELLE_SIP_HEADER_ADDRESS(to))))
			address=belle_sip_header_address_create2(belle_sip_header_address_get_displayname(BELLE_SIP_HEADER_ADDRESS(to))
					,belle_sip_header_address_get_absolute_uri(BELLE_SIP_HEADER_ADDRESS(to)));
		else
			ms_error("Cannot not find to uri from request [%p]",req);

		sal_op_set_to_address(op,(SalAddress*)address);
		belle_sip_object_unref(address);
	}

	if (!op->base.origin) {
		/*set origin uri*/
		origin_address=belle_sip_header_address_create(NULL,belle_sip_request_extract_origin(req));
		__sal_op_set_network_origin_address(op,(SalAddress*)origin_address);
		belle_sip_object_unref(origin_address);
	}
	if (!op->base.remote_ua) {
		sal_op_set_remote_ua(op,BELLE_SIP_MESSAGE(req));
	}

	if (!op->base.call_id) {
		op->base.call_id=ms_strdup(belle_sip_header_call_id_get_call_id(BELLE_SIP_HEADER_CALL_ID(belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req), belle_sip_header_call_id_t))));
	}
	/*It is worth noting that proxies can (and
   will) remove this header field*/
	sal_op_set_privacy_from_message(op,(belle_sip_message_t*)req);

	sal_op_assign_recv_headers(op,(belle_sip_message_t*)req);
	if (op->callbacks && op->callbacks->process_request_event) {
		op->callbacks->process_request_event(op,event);
	} else {
		ms_error("sal process_request_event not implemented yet");
	}

}

static void process_response_event(void *user_ctx, const belle_sip_response_event_t *event){
	belle_sip_client_transaction_t* client_transaction = belle_sip_response_event_get_client_transaction(event);
	belle_sip_response_t* response = belle_sip_response_event_get_response(event);
	int response_code = belle_sip_response_get_status_code(response);

	if (!client_transaction) {
		ms_warning("Discarding stateless response [%i]",response_code);
		return;
	} else {
		SalOp* op = (SalOp*)belle_sip_transaction_get_application_data(BELLE_SIP_TRANSACTION(client_transaction));
		belle_sip_request_t* request=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
		belle_sip_header_contact_t* remote_contact = belle_sip_message_get_header_by_type(response, belle_sip_header_contact_t);

		if (op->state == SalOpStateTerminated) {
			belle_sip_message("Op is terminated, nothing to do with this [%i]",response_code);
			return;
		}
		/*do it all the time, since we can receive provisional responses from a different instance than the final one*/
		sal_op_set_remote_ua(op,BELLE_SIP_MESSAGE(response));

		if(remote_contact) {
			__sal_op_set_remote_contact(op, belle_sip_header_get_unparsed_value(BELLE_SIP_HEADER(remote_contact)));
		}

		if (!op->base.call_id) {
			op->base.call_id=ms_strdup(belle_sip_header_call_id_get_call_id(BELLE_SIP_HEADER_CALL_ID(belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(response), belle_sip_header_call_id_t))));
		}

		sal_op_assign_recv_headers(op,(belle_sip_message_t*)response);

		if (op->callbacks && op->callbacks->process_response_event) {
			/*handle authorization*/
			switch (response_code) {
				case 200:
					break;
				case 401:
				case 407:
					if (op->state == SalOpStateTerminating && strcmp("BYE",belle_sip_request_get_method(request))!=0) {
						/*only bye are completed*/
						belle_sip_message("Op is in state terminating, nothing else to do ");
					} else {
						if (op->pending_auth_transaction){
							belle_sip_object_unref(op->pending_auth_transaction);
							op->pending_auth_transaction=NULL;
						}
						if (++op->auth_requests > 2) {
							ms_warning("Auth info cannot be found for op [%s/%s] after 2 attempts, giving up",sal_op_get_from(op)
																												,sal_op_get_to(op));
							op->base.root->callbacks.auth_failure(op,op->auth_info);
							sal_remove_pending_auth(op->base.root,op);
						} else {
							op->pending_auth_transaction=(belle_sip_client_transaction_t*)belle_sip_object_ref(client_transaction);
							sal_process_authentication(op);
							return;
						}
					}
					break;
				case 403:
					if (op->auth_info) op->base.root->callbacks.auth_failure(op,op->auth_info);
					break;
			}
			if (response_code >= 180 && response_code !=401 && response_code !=407 && response_code !=403) {
				/*not an auth request*/
				op->auth_requests=0;
			}
			op->callbacks->process_response_event(op,event);
		} else {
			ms_error("Unhandled event response [%p]",event);
		}
	}
}

static void process_timeout(void *user_ctx, const belle_sip_timeout_event_t *event) {
	belle_sip_client_transaction_t* client_transaction = belle_sip_timeout_event_get_client_transaction(event);
	SalOp* op = (SalOp*)belle_sip_transaction_get_application_data(BELLE_SIP_TRANSACTION(client_transaction));
	if (op && op->callbacks && op->callbacks->process_timeout) {
		op->callbacks->process_timeout(op,event);
	} else {
		ms_error("Unhandled event timeout [%p]",event);
	}
}

static void process_transaction_terminated(void *user_ctx, const belle_sip_transaction_terminated_event_t *event) {
	belle_sip_client_transaction_t* client_transaction = belle_sip_transaction_terminated_event_get_client_transaction(event);
	belle_sip_server_transaction_t* server_transaction = belle_sip_transaction_terminated_event_get_server_transaction(event);
	belle_sip_transaction_t* trans;
	SalOp* op;

	if(client_transaction)
		trans=BELLE_SIP_TRANSACTION(client_transaction);
	 else
		trans=BELLE_SIP_TRANSACTION(server_transaction);

	op = (SalOp*)belle_sip_transaction_get_application_data(trans);
	if (op && op->callbacks && op->callbacks->process_transaction_terminated) {
		op->callbacks->process_transaction_terminated(op,event);
	} else {
		ms_message("Unhandled transaction terminated [%p]",trans);
	}
	if (op) sal_op_unref(op); /*because every transaction ref op*/
}


static void process_auth_requested(void *sal, belle_sip_auth_event_t *event) {
	SalAuthInfo* auth_info = sal_auth_info_create(event);
	((Sal*)sal)->callbacks.auth_requested(sal,auth_info);
	belle_sip_auth_event_set_passwd(event,(const char*)auth_info->password);
	belle_sip_auth_event_set_ha1(event,(const char*)auth_info->ha1);
	belle_sip_auth_event_set_userid(event,(const char*)auth_info->userid);
	belle_sip_auth_event_set_signing_key(event,(belle_sip_signing_key_t *)auth_info->key);
	belle_sip_auth_event_set_client_certificates_chain(event,(belle_sip_certificates_chain_t* )auth_info->certificates);
	sal_auth_info_delete(auth_info);
}

Sal * sal_init(){
	belle_sip_listener_callbacks_t listener_callbacks;
	Sal * sal=ms_new0(Sal,1);

	/*belle_sip_object_enable_marshal_check(TRUE);*/
	sal->auto_contacts=TRUE;

	/*first create the stack, which initializes the belle-sip object's pool for this thread*/
	belle_sip_set_log_handler(_belle_sip_log);
	sal->stack = belle_sip_stack_new(NULL);

	sal->user_agent=belle_sip_header_user_agent_new();
#if defined(PACKAGE_NAME) && defined(LIBLINPHONE_VERSION)
	belle_sip_header_user_agent_add_product(sal->user_agent, PACKAGE_NAME "/" LIBLINPHONE_VERSION);
#endif
	sal_append_stack_string_to_user_agent(sal);
	belle_sip_object_ref(sal->user_agent);

	sal->prov = belle_sip_stack_create_provider(sal->stack,NULL);
	sal_nat_helper_enable(sal,TRUE);
	memset(&listener_callbacks,0,sizeof(listener_callbacks));
	listener_callbacks.process_dialog_terminated=process_dialog_terminated;
	listener_callbacks.process_io_error=process_io_error;
	listener_callbacks.process_request_event=process_request_event;
	listener_callbacks.process_response_event=process_response_event;
	listener_callbacks.process_timeout=process_timeout;
	listener_callbacks.process_transaction_terminated=process_transaction_terminated;
	listener_callbacks.process_auth_requested=process_auth_requested;
	sal->listener=belle_sip_listener_create_from_callbacks(&listener_callbacks,sal);
	belle_sip_provider_add_sip_listener(sal->prov,sal->listener);
	sal->tls_verify=TRUE;
	sal->tls_verify_cn=TRUE;
	sal->refresher_retry_after=60000; /*default value in ms*/
	sal->enable_sip_update=TRUE;
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
	if (ctx->callbacks.auth_failure==NULL)
		ctx->callbacks.auth_failure=(SalOnAuthFailure)unimplemented_stub;
	if (ctx->callbacks.register_success==NULL)
		ctx->callbacks.register_success=(SalOnRegisterSuccess)unimplemented_stub;
	if (ctx->callbacks.register_failure==NULL)
		ctx->callbacks.register_failure=(SalOnRegisterFailure)unimplemented_stub;
	if (ctx->callbacks.dtmf_received==NULL)
		ctx->callbacks.dtmf_received=(SalOnDtmfReceived)unimplemented_stub;
	if (ctx->callbacks.notify==NULL)
		ctx->callbacks.notify=(SalOnNotify)unimplemented_stub;
	if (ctx->callbacks.subscribe_received==NULL)
		ctx->callbacks.subscribe_received=(SalOnSubscribeReceived)unimplemented_stub;
	if (ctx->callbacks.subscribe_closed==NULL)
		ctx->callbacks.subscribe_closed=(SalOnSubscribeClosed)unimplemented_stub;
	if (ctx->callbacks.parse_presence_requested==NULL)
		ctx->callbacks.parse_presence_requested=(SalOnParsePresenceRequested)unimplemented_stub;
	if (ctx->callbacks.convert_presence_to_xml_requested==NULL)
		ctx->callbacks.convert_presence_to_xml_requested=(SalOnConvertPresenceToXMLRequested)unimplemented_stub;
	if (ctx->callbacks.notify_presence==NULL)
		ctx->callbacks.notify_presence=(SalOnNotifyPresence)unimplemented_stub;
	if (ctx->callbacks.subscribe_presence_received==NULL)
		ctx->callbacks.subscribe_presence_received=(SalOnSubscribePresenceReceived)unimplemented_stub;
	if (ctx->callbacks.text_received==NULL)
		ctx->callbacks.text_received=(SalOnTextReceived)unimplemented_stub;
	if (ctx->callbacks.is_composing_received==NULL)
		ctx->callbacks.is_composing_received=(SalOnIsComposingReceived)unimplemented_stub;
	if (ctx->callbacks.ping_reply==NULL)
		ctx->callbacks.ping_reply=(SalOnPingReply)unimplemented_stub;
	if (ctx->callbacks.auth_requested==NULL)
		ctx->callbacks.auth_requested=(SalOnAuthRequested)unimplemented_stub;
	if (ctx->callbacks.info_received==NULL)
		ctx->callbacks.info_received=(SalOnInfoReceived)unimplemented_stub;
	if (ctx->callbacks.on_publish_response==NULL)
		ctx->callbacks.on_publish_response=(SalOnPublishResponse)unimplemented_stub;
	if (ctx->callbacks.on_expire==NULL)
		ctx->callbacks.on_expire=(SalOnExpire)unimplemented_stub;
}



void sal_uninit(Sal* sal){
	belle_sip_object_unref(sal->user_agent);
	belle_sip_object_unref(sal->prov);
	belle_sip_object_unref(sal->stack);
	belle_sip_object_unref(sal->listener);
	if (sal->supported) belle_sip_object_unref(sal->supported);
	ms_list_free_with_data(sal->supported_tags,ms_free);
	if (sal->uuid) ms_free(sal->uuid);
	if (sal->root_ca) ms_free(sal->root_ca);
	ms_free(sal);
};

int sal_transport_available(Sal *sal, SalTransport t){
	switch(t){
		case SalTransportUDP:
		case SalTransportTCP:
			return TRUE;
		case SalTransportTLS:
			return belle_sip_stack_tls_available(sal->stack);
		case SalTransportDTLS:
			return FALSE;
	}
	return FALSE;
}

static int sal_add_listen_port(Sal *ctx, SalAddress* addr, bool_t is_tunneled){
	int result;
	belle_sip_listening_point_t* lp;
	if (is_tunneled){
#ifdef TUNNEL_ENABLED
		if (sal_address_get_transport(addr)!=SalTransportUDP){
			ms_error("Tunneled mode is only available for UDP kind of transports.");
			return -1;
		}
		lp = belle_sip_tunnel_listening_point_new(ctx->stack, ctx->tunnel_client);
		if (!lp){
			ms_error("Could not create tunnel listening point.");
			return -1;
		}
#else
		ms_error("No tunnel support in library.");
		return -1;
#endif
	}else{
		lp = belle_sip_stack_create_listening_point(ctx->stack,
									sal_address_get_domain(addr),
									sal_address_get_port(addr),
									sal_transport_to_string(sal_address_get_transport(addr)));
	}
	if (lp) {
		belle_sip_listening_point_set_keep_alive(lp,ctx->keep_alive);
		result = belle_sip_provider_add_listening_point(ctx->prov,lp);
		if (sal_address_get_transport(addr)==SalTransportTLS) set_tls_properties(ctx);
	} else {
		return -1;
	}
	return result;
}

int sal_listen_port(Sal *ctx, const char *addr, int port, SalTransport tr, int is_tunneled) {
	SalAddress* sal_addr = sal_address_new(NULL);
	int result;
	sal_address_set_domain(sal_addr,addr);
	sal_address_set_port(sal_addr,port);
	sal_address_set_transport(sal_addr,tr);
	result = sal_add_listen_port(ctx, sal_addr, is_tunneled);
	sal_address_destroy(sal_addr);
	return result;
}

static void remove_listening_point(belle_sip_listening_point_t* lp,belle_sip_provider_t* prov) {
	belle_sip_provider_remove_listening_point(prov,lp);
}

int sal_get_listening_port(Sal *ctx, SalTransport tr){
	const char *tpn=sal_transport_to_string(tr);
	belle_sip_listening_point_t *lp=belle_sip_provider_get_listening_point(ctx->prov, tpn);
	if (lp){
		return belle_sip_listening_point_get_port(lp);
	}
	return 0;
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
	ms_warning("sal_get_socket is deprecated");
	return -1;
}

void sal_set_user_agent(Sal *ctx, const char *user_agent){
	belle_sip_header_user_agent_set_products(ctx->user_agent,NULL);
	belle_sip_header_user_agent_add_product(ctx->user_agent,user_agent);
	return ;
}

const char* sal_get_user_agent(Sal *ctx){
	static char user_agent[255];
	belle_sip_header_user_agent_get_products_as_string(ctx->user_agent, user_agent, 254);
	return user_agent;
}

void sal_append_stack_string_to_user_agent(Sal *ctx) {
	char stack_string[64];
	snprintf(stack_string, sizeof(stack_string) - 1, "(belle-sip/%s)", belle_sip_version_to_string());
	belle_sip_header_user_agent_add_product(ctx->user_agent, stack_string);
}

/*keepalive period in ms*/
void sal_set_keepalive_period(Sal *ctx,unsigned int value){
	const belle_sip_list_t* iterator;
	belle_sip_listening_point_t* lp;
	ctx->keep_alive=value;
	for (iterator=belle_sip_provider_get_listening_points(ctx->prov);iterator!=NULL;iterator=iterator->next) {
		lp=(belle_sip_listening_point_t*)iterator->data;
		if (ctx->use_tcp_tls_keep_alive || strcasecmp(belle_sip_listening_point_get_transport(lp),"udp")==0) {
			belle_sip_listening_point_set_keep_alive(lp,ctx->keep_alive);
		}
	}
}
int sal_set_tunnel(Sal *ctx, void *tunnelclient) {
#ifdef TUNNEL_ENABLED
	ctx->tunnel_client=tunnelclient;
	return 0;
#else
	return -1;
#endif
}

/**
 * returns keepalive period in ms
 * 0 desactiaved
 * */
unsigned int sal_get_keepalive_period(Sal *ctx){
	return ctx->keep_alive;
}
void sal_use_session_timers(Sal *ctx, int expires){
	ctx->session_expires=expires;
	return ;
}

void sal_use_one_matching_codec_policy(Sal *ctx, bool_t one_matching_codec){
	ctx->one_matching_codec=one_matching_codec;
}

void sal_use_rport(Sal *ctx, bool_t use_rports){
	belle_sip_provider_enable_rport(ctx->prov,use_rports);
	ms_message("Sal use rport [%s]",use_rports?"enabled":"disabled");
	return ;
}

static void set_tls_properties(Sal *ctx){
	belle_sip_listening_point_t *lp=belle_sip_provider_get_listening_point(ctx->prov,"TLS");
	if (lp){
		belle_sip_tls_listening_point_t *tlp=BELLE_SIP_TLS_LISTENING_POINT(lp);
		int verify_exceptions=0;

		if (!ctx->tls_verify) verify_exceptions=BELLE_SIP_TLS_LISTENING_POINT_BADCERT_ANY_REASON;
		else if (!ctx->tls_verify_cn) verify_exceptions=BELLE_SIP_TLS_LISTENING_POINT_BADCERT_CN_MISMATCH;

		belle_sip_tls_listening_point_set_root_ca(tlp,ctx->root_ca); /*root_ca might be NULL */
		belle_sip_tls_listening_point_set_verify_exceptions(tlp,verify_exceptions);
	}
}

void sal_set_root_ca(Sal* ctx, const char* rootCa){
	if (ctx->root_ca){
		ms_free(ctx->root_ca);
		ctx->root_ca=NULL;
	}
	if (rootCa)
		ctx->root_ca=ms_strdup(rootCa);
	set_tls_properties(ctx);
	return ;
}

void sal_verify_server_certificates(Sal *ctx, bool_t verify){
	ctx->tls_verify=verify;
	set_tls_properties(ctx);
	return ;
}

void sal_verify_server_cn(Sal *ctx, bool_t verify){
	ctx->tls_verify_cn=verify;
	set_tls_properties(ctx);
	return ;
}

void sal_use_tcp_tls_keepalive(Sal *ctx, bool_t enabled) {
	ctx->use_tcp_tls_keep_alive=enabled;
}

int sal_iterate(Sal *sal){
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
	strncpy(ip,address_family==AF_INET6 ? "::1" : "127.0.0.1",iplen);
	ms_error("sal_get_default_local_ip() is deprecated.");
}

const char *sal_get_root_ca(Sal* ctx) {
	return ctx->root_ca;
}

int sal_reset_transports(Sal *ctx){
	ms_message("Reseting transports");
	belle_sip_provider_clean_channels(ctx->prov);
	return 0;
}

void sal_set_dscp(Sal *ctx, int dscp){
	belle_sip_stack_set_default_dscp(ctx->stack,dscp);
}

void  sal_set_send_error(Sal *sal,int value) {
	 belle_sip_stack_set_send_error(sal->stack,value);
}
void  sal_set_recv_error(Sal *sal,int value) {
	 belle_sip_provider_set_recv_error(sal->prov,value);
}
void sal_nat_helper_enable(Sal *sal,bool_t enable) {
	sal->nat_helper_enabled=enable;
	belle_sip_provider_enable_nat_helper(sal->prov,enable);
	ms_message("Sal nat helper [%s]",enable?"enabled":"disabled");
}
bool_t sal_nat_helper_enabled(Sal *sal) {
	return sal->nat_helper_enabled;
}
void sal_set_dns_timeout(Sal* sal,int timeout) {
	belle_sip_stack_set_dns_timeout(sal->stack, timeout);
}
int sal_get_dns_timeout(const Sal* sal)  {
	return belle_sip_stack_get_dns_timeout(sal->stack);
}

void sal_set_transport_timeout(Sal* sal,int timeout) {
	belle_sip_stack_set_transport_timeout(sal->stack, timeout);
}
int sal_get_transport_timeout(const Sal* sal)  {
	return belle_sip_stack_get_transport_timeout(sal->stack);
}
void sal_enable_dns_srv(Sal *sal, bool_t enable) {
	belle_sip_stack_enable_dns_srv(sal->stack, (unsigned char)enable);
}
bool_t sal_dns_srv_enabled(const Sal *sal) {
	return (bool_t)belle_sip_stack_dns_srv_enabled(sal->stack);
}

void sal_set_dns_user_hosts_file(Sal *sal, const char *hosts_file) {
	belle_sip_stack_set_dns_user_hosts_file(sal->stack, hosts_file);
}

const char * sal_get_dns_user_hosts_file(const Sal *sal) {
	return belle_sip_stack_get_dns_user_hosts_file(sal->stack);
}

SalAuthInfo* sal_auth_info_create(belle_sip_auth_event_t* event) {
	SalAuthInfo* auth_info = sal_auth_info_new();
	auth_info->realm = ms_strdup(belle_sip_auth_event_get_realm(event));
	auth_info->username = ms_strdup(belle_sip_auth_event_get_username(event));
	auth_info->domain = ms_strdup(belle_sip_auth_event_get_domain(event));
	auth_info->mode = (SalAuthMode)belle_sip_auth_event_get_mode(event);
	return auth_info;
}

SalAuthMode sal_auth_info_get_mode(const SalAuthInfo* auth_info) { return auth_info->mode; }
SalSigningKey *sal_auth_info_get_signing_key(const SalAuthInfo* auth_info) { return auth_info->key; }
SalCertificatesChain *sal_auth_info_get_certificates_chain(const SalAuthInfo* auth_info) { return auth_info->certificates; }
void sal_auth_info_set_mode(SalAuthInfo* auth_info, SalAuthMode mode) { auth_info->mode = mode; }
void sal_certificates_chain_delete(SalCertificatesChain *chain) {
	belle_sip_object_unref((belle_sip_object_t *)chain);
}
void sal_signing_key_delete(SalSigningKey *key) {
	belle_sip_object_unref((belle_sip_object_t *)key);
}

const char* sal_op_type_to_string(const SalOpType type) {
	switch(type) {
	case SalOpRegister: return "SalOpRegister";
	case SalOpCall: return "SalOpCall";
	case SalOpMessage: return "SalOpMessage";
	case SalOpPresence: return "SalOpPresence";
	default:
		return "SalOpUnknown";
	}
}

void sal_use_dates(Sal *ctx, bool_t enabled){
	ctx->use_dates=enabled;
}

int sal_auth_compute_ha1(const char* userid,const char* realm,const char* password, char ha1[33]) {
	return belle_sip_auth_helper_compute_ha1(userid, realm, password, ha1);
}


SalCustomHeader *sal_custom_header_append(SalCustomHeader *ch, const char *name, const char *value){
	belle_sip_message_t *msg=(belle_sip_message_t*)ch;
	belle_sip_header_t *h;

	if (msg==NULL){
		msg=(belle_sip_message_t*)belle_sip_request_new();
		belle_sip_object_ref(msg);
	}
	h=belle_sip_header_create(name,value);
	if (h==NULL){
		belle_sip_error("Fail to parse custom header.");
		return (SalCustomHeader*)msg;
	}
	belle_sip_message_add_header(msg,h);
	return (SalCustomHeader*)msg;
}

const char *sal_custom_header_find(const SalCustomHeader *ch, const char *name){
	if (ch){
		belle_sip_header_t *h=belle_sip_message_get_header((belle_sip_message_t*)ch,name);

		if (h){
			return belle_sip_header_get_unparsed_value(h);
		}
	}
	return NULL;
}

void sal_custom_header_free(SalCustomHeader *ch){
	if (ch==NULL) return;
	belle_sip_object_unref((belle_sip_message_t*)ch);
}

SalCustomHeader *sal_custom_header_clone(const SalCustomHeader *ch){
	if (ch==NULL) return NULL;
	return (SalCustomHeader*)belle_sip_object_ref((belle_sip_message_t*)ch);
}

const SalCustomHeader *sal_op_get_recv_custom_header(SalOp *op){
	SalOpBase *b=(SalOpBase *)op;
	return b->recv_custom_headers;
}

void sal_set_uuid(Sal *sal, const char *uuid){
	if (sal->uuid){
		ms_free(sal->uuid);
		sal->uuid=NULL;
	}
	if (uuid)
		sal->uuid=ms_strdup(uuid);
}

typedef struct {
	unsigned int time_low;
	unsigned short time_mid;
	unsigned short time_hi_and_version;
	unsigned char clock_seq_hi_and_reserved;
	unsigned char clock_seq_low;
	unsigned char node[6];
} sal_uuid_t;


int sal_create_uuid(Sal*ctx, char *uuid, size_t len){
	sal_uuid_t uuid_struct;
	int i;
	int written;

	if (len==0) return -1;
	/*create an UUID as described in RFC4122, 4.4 */
	belle_sip_random_bytes((unsigned char*)&uuid_struct, sizeof(sal_uuid_t));
	uuid_struct.clock_seq_hi_and_reserved&=~(1<<6);
	uuid_struct.clock_seq_hi_and_reserved|=1<<7;
	uuid_struct.time_hi_and_version&=~(0xf<<12);
	uuid_struct.time_hi_and_version|=4<<12;

	written=snprintf(uuid,len,"%8.8x-%4.4x-%4.4x-%2.2x%2.2x-", uuid_struct.time_low, uuid_struct.time_mid,
			uuid_struct.time_hi_and_version, uuid_struct.clock_seq_hi_and_reserved,
			uuid_struct.clock_seq_low);
	if (written>len+13){
		ms_error("sal_create_uuid(): buffer is too short !");
		return -1;
	}
	for (i = 0; i < 6; i++)
		written+=snprintf(uuid+written,len-written,"%2.2x", uuid_struct.node[i]);
	uuid[len-1]='\0';
	sal_set_uuid(ctx,uuid);
	return 0;
}

static void make_supported_header(Sal *sal){
	MSList *it;
	char *alltags=NULL;
	size_t buflen=64;
	size_t written=0;

	if (sal->supported){
		belle_sip_object_unref(sal->supported);
		sal->supported=NULL;
	}
	for(it=sal->supported_tags;it!=NULL;it=it->next){
		const char *tag=(const char*)it->data;
		size_t taglen=strlen(tag);
		if (alltags==NULL || (written+taglen+1>=buflen)) alltags=ms_realloc(alltags,(buflen=buflen*2));
		snprintf(alltags+written,buflen-written,it->next ? "%s, " : "%s",tag);
	}
	if (alltags){
		sal->supported=belle_sip_header_create("Supported",alltags);
		if (sal->supported){
			belle_sip_object_ref(sal->supported);
		}
		ms_free(alltags);
	}
}

void sal_set_supported_tags(Sal *ctx, const char* tags){
	ctx->supported_tags=ms_list_free_with_data(ctx->supported_tags,ms_free);
	if (tags){
		char *iter;
		char *buffer=ms_strdup(tags);
		char *tag;
		char *context=NULL;
		iter=buffer;
		while((tag=strtok_r(iter,", ",&context))!=NULL){
			iter=NULL;
			ctx->supported_tags=ms_list_append(ctx->supported_tags,ms_strdup(tag));
		}
		ms_free(buffer);
	}
	make_supported_header(ctx);
}

const char *sal_get_supported_tags(Sal *ctx){
	if (ctx->supported){
		return belle_sip_header_get_unparsed_value(ctx->supported);
	}
	return NULL;
}

void sal_add_supported_tag(Sal *ctx, const char* tag){
	MSList *elem=ms_list_find_custom(ctx->supported_tags,(MSCompareFunc)strcasecmp,tag);
	if (!elem){
		ctx->supported_tags=ms_list_append(ctx->supported_tags,ms_strdup(tag));
		make_supported_header(ctx);
	}

}

void sal_remove_supported_tag(Sal *ctx, const char* tag){
	MSList *elem=ms_list_find_custom(ctx->supported_tags,(MSCompareFunc)strcasecmp,tag);
	if (elem){
		ms_free(elem->data);
		ctx->supported_tags=ms_list_remove_link(ctx->supported_tags,elem);
		make_supported_header(ctx);
	}
}



belle_sip_response_t* sal_create_response_from_request ( Sal* sal, belle_sip_request_t* req, int code ) {
	belle_sip_response_t *resp=belle_sip_response_create_from_request(req,code);
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(resp),BELLE_SIP_HEADER(sal->user_agent));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(resp),sal->supported);
	return resp;
}

void sal_set_refresher_retry_after(Sal *sal,int value) {
	sal->refresher_retry_after=value;
}

int sal_get_refresher_retry_after(const Sal *sal) {
	return sal->refresher_retry_after;
}

void sal_enable_auto_contacts(Sal *ctx, bool_t enabled){
	ctx->auto_contacts=enabled;
}

void sal_enable_test_features(Sal*ctx, bool_t enabled){
	ctx->enable_test_features=enabled;
}

void sal_use_no_initial_route(Sal *ctx, bool_t enabled){
	ctx->no_initial_route=enabled;
}

SalResolverContext * sal_resolve_a(Sal* sal, const char *name, int port, int family, SalResolverCallback cb, void *data){
	return (SalResolverContext*)belle_sip_stack_resolve_a(sal->stack,name,port,family,(belle_sip_resolver_callback_t)cb,data);
}

/*
void sal_resolve_cancel(Sal *sal, SalResolverContext* ctx){
	belle_sip_stack_resolve_cancel(sal->stack,ctx);
}
*/

void sal_enable_unconditional_answer(Sal *sal,int value) {
	belle_sip_provider_enable_unconditional_answer(sal->prov,value);
}

/** Parse a file containing either a certificate chain order in PEM format or a single DER cert
 * @param auth_info structure where to store the result of parsing
 * @param path path to certificate chain file
 * @param format either PEM or DER
 */
void sal_certificates_chain_parse_file(SalAuthInfo* auth_info, const char* path, SalCertificateRawFormat format) {
	auth_info->certificates = (SalCertificatesChain*) belle_sip_certificates_chain_parse_file(path, (belle_sip_certificate_raw_format_t)format); //
	if (auth_info->certificates) belle_sip_object_ref((belle_sip_object_t *) auth_info->certificates);
}

/**
 * Parse a file containing either a private or public rsa key
 * @param auth_info structure where to store the result of parsing
 * @param passwd password (optionnal)
 */
void sal_signing_key_parse_file(SalAuthInfo* auth_info, const char* path, const char *passwd) {
	auth_info->key = (SalSigningKey *) belle_sip_signing_key_parse_file(path, passwd);
	if (auth_info->key) belle_sip_object_ref((belle_sip_object_t *) auth_info->key);
}

unsigned char * sal_get_random_bytes(unsigned char *ret, size_t size){
	return belle_sip_random_bytes(ret,size);
}

char *sal_get_random_token(int size){
	return belle_sip_random_token(ms_malloc(size),size);
}

unsigned int sal_get_random(void){
	unsigned int ret=0;
	belle_sip_random_bytes((unsigned char*)&ret,4);
	return ret;
}

belle_sip_source_t * sal_create_timer(Sal *sal, belle_sip_source_func_t func, void *data, unsigned int timeout_value_ms, const char* timer_name) {
	belle_sip_main_loop_t *ml = belle_sip_stack_get_main_loop(sal->stack);
	return belle_sip_main_loop_create_timeout(ml, func, data, timeout_value_ms, timer_name);
}

void sal_cancel_timer(Sal *sal, belle_sip_source_t *timer) {
	belle_sip_main_loop_t *ml = belle_sip_stack_get_main_loop(sal->stack);
	belle_sip_main_loop_remove_source(ml, timer);
}
void sal_enable_sip_update_method(Sal *ctx,bool_t value) {
	ctx->enable_sip_update=value;
}

void sal_default_enable_sdp_removal(Sal *sal, bool_t enable)  {
	if (enable) ms_message("Enabling SDP removal feature by default for all new SalOp in Sal[%p]!", sal);
	sal->default_sdp_removal = enable;
}
