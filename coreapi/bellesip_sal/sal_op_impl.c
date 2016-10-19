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

/*create an operation */
SalOp * sal_op_new(Sal *sal){
	SalOp *op=ms_new0(SalOp,1);
	__sal_op_init(op,sal);
	op->type=SalOpUnknown;
	op->privacy=SalPrivacyNone;
	op->manual_refresher=FALSE;/*tells that requests with expiry (SUBSCRIBE, PUBLISH) will be automatically refreshed*/
	op->sdp_handling=sal->default_sdp_handling;
	sal_op_ref(op);
	return op;
}

void sal_op_kill_dialog(SalOp *op) {
	ms_warning("op [%p]: force kill of dialog [%p]", op, op->dialog);
	belle_sip_dialog_delete(op->dialog);
}

void sal_op_release(SalOp *op){
	/*if in terminating state, keep this state because it means we are waiting for a response to be able to terminate the operation.*/
	if (op->state!=SalOpStateTerminating)
		op->state=SalOpStateTerminated;
	sal_op_set_user_pointer(op,NULL);/*mandatory because releasing op doesn't not mean freeing op. Make sure back pointer will not be used later*/
	if (op->base.release_cb)
		op->base.release_cb(&op->base);
	if (op->refresher) {
		belle_sip_refresher_stop(op->refresher);
	}
	op->op_released = TRUE;
	sal_op_unref(op);
}

void sal_op_release_impl(SalOp *op){
	ms_message("Destroying op [%p] of type [%s]",op,sal_op_type_to_string(op->type));
	if (op->pending_auth_transaction) belle_sip_object_unref(op->pending_auth_transaction);
	sal_remove_pending_auth(op->base.root,op);
	if (op->auth_info) {
		sal_auth_info_delete(op->auth_info);
	}
	if (op->sdp_answer) belle_sip_object_unref(op->sdp_answer);
	if (op->refresher) {
		belle_sip_object_unref(op->refresher);
		op->refresher=NULL;
	}
	if (op->result)
		sal_media_description_unref(op->result);
	if(op->replaces) belle_sip_object_unref(op->replaces);
	if(op->referred_by) belle_sip_object_unref(op->referred_by);

	if (op->pending_client_trans) belle_sip_object_unref(op->pending_client_trans);
	if (op->pending_server_trans) belle_sip_object_unref(op->pending_server_trans);
	if (op->pending_update_server_trans) belle_sip_object_unref(op->pending_update_server_trans);
	if (op->event) belle_sip_object_unref(op->event);
	sal_error_info_reset(&op->error_info);
	__sal_op_free(op);
	return ;
}

void sal_op_authenticate(SalOp *op, const SalAuthInfo *info){
	if (op->type == SalOpRegister) {
		/*Registration authenticate is just about registering again*/
		sal_register_refresh(op,-1);
	}else {
		/*for sure auth info will be accessible from the provider*/
		sal_process_authentication(op);
	}
	return ;
}

void sal_op_cancel_authentication(SalOp *h){
	ms_fatal("sal_op_cancel_authentication not implemented yet");
	return ;
}

SalAuthInfo * sal_op_get_auth_requested(SalOp *op){
	return op->auth_info;
}

belle_sip_header_contact_t* sal_op_create_contact(SalOp *op){
	belle_sip_header_contact_t* contact_header;
	belle_sip_uri_t* contact_uri;

	if (sal_op_get_contact_address(op)) {
		contact_header = belle_sip_header_contact_create(BELLE_SIP_HEADER_ADDRESS(sal_op_get_contact_address(op)));
	} else {
		contact_header= belle_sip_header_contact_new();
	}

	if (!(contact_uri=belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(contact_header)))) {
		/*no uri, just creating a new one*/
		contact_uri=belle_sip_uri_new();
		belle_sip_header_address_set_uri(BELLE_SIP_HEADER_ADDRESS(contact_header),contact_uri);
	}

	belle_sip_uri_set_user_password(contact_uri,NULL);
	belle_sip_uri_set_secure(contact_uri,sal_op_is_secure(op));
	if (op->privacy!=SalPrivacyNone){
		belle_sip_uri_set_user(contact_uri,NULL);
	}
	belle_sip_header_contact_set_automatic(contact_header,op->base.root->auto_contacts);
	if (op->base.root->uuid){
		if (belle_sip_parameters_has_parameter(BELLE_SIP_PARAMETERS(contact_header),"+sip.instance")==0){
			char *instance_id=belle_sip_strdup_printf("\"<urn:uuid:%s>\"",op->base.root->uuid);
			belle_sip_parameters_set_parameter(BELLE_SIP_PARAMETERS(contact_header),"+sip.instance",instance_id);
			belle_sip_free(instance_id);
		}
	}
	return contact_header;
}



static void add_initial_route_set(belle_sip_request_t *request, const MSList *list){
	const MSList *elem;
	for (elem=list;elem!=NULL;elem=elem->next){
		SalAddress *addr=(SalAddress*)elem->data;
		belle_sip_header_route_t *route;
		belle_sip_uri_t *uri;
		/*Optimization: if the initial route set only contains one URI which is the same as the request URI, ommit it*/
		if (elem==list && list->next==NULL){
			belle_sip_uri_t *requri=belle_sip_request_get_uri(request);
			/*skip the first route it is the same as the request uri*/
			if (strcmp(sal_address_get_domain(addr),belle_sip_uri_get_host(requri))==0 ){
				ms_message("Skipping top route of initial route-set because same as request-uri.");
				continue;
			}
		}

		route=belle_sip_header_route_create((belle_sip_header_address_t*)addr);
		uri=belle_sip_header_address_get_uri((belle_sip_header_address_t*)route);
		belle_sip_uri_set_lr_param(uri,1);
		belle_sip_message_add_header((belle_sip_message_t*)request,(belle_sip_header_t*)route);
	}
}

belle_sip_request_t* sal_op_build_request(SalOp *op,const char* method) {
	belle_sip_header_from_t* from_header;
	belle_sip_header_to_t* to_header;
	belle_sip_provider_t* prov=op->base.root->prov;
	belle_sip_request_t *req;
	belle_sip_uri_t* req_uri;
	belle_sip_uri_t* to_uri;

	const SalAddress* to_address;
	const MSList *elem=sal_op_get_route_addresses(op);
	char token[10];

	/* check that the op has a correct to address */
	to_address = sal_op_get_to_address(op);
	if( to_address == NULL ){
		ms_error("No To: address, cannot build request");
		return NULL;
	}

	to_uri = belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(to_address));
	if( to_uri == NULL ){
		ms_error("To: address is invalid, cannot build request");
		return NULL;
	}

	if (strcmp("REGISTER",method)==0 || op->privacy==SalPrivacyNone) {
		from_header = belle_sip_header_from_create(BELLE_SIP_HEADER_ADDRESS(sal_op_get_from_address(op))
						,belle_sip_random_token(token,sizeof(token)));
	} else {
		from_header=belle_sip_header_from_create2("Anonymous <sip:anonymous@anonymous.invalid>",belle_sip_random_token(token,sizeof(token)));
	}
	/*make sure to preserve components like headers or port*/

	req_uri = (belle_sip_uri_t*)belle_sip_object_clone((belle_sip_object_t*)to_uri);
	belle_sip_uri_set_secure(req_uri,sal_op_is_secure(op));

	to_header = belle_sip_header_to_create(BELLE_SIP_HEADER_ADDRESS(to_address),NULL);

	req=belle_sip_request_create(
					req_uri,
					method,
					belle_sip_provider_create_call_id(prov),
					belle_sip_header_cseq_create(20,method),
					from_header,
					to_header,
					belle_sip_header_via_new(),
					70);

	if (op->privacy & SalPrivacyId) {
		belle_sip_header_p_preferred_identity_t* p_preferred_identity=belle_sip_header_p_preferred_identity_create(BELLE_SIP_HEADER_ADDRESS(sal_op_get_from_address(op)));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(p_preferred_identity));
	}

	if (elem && strcmp(method,"REGISTER")!=0 && !op->base.root->no_initial_route){
		add_initial_route_set(req,elem);
	}

	if (strcmp("REGISTER",method)!=0 && op->privacy!=SalPrivacyNone ){
		belle_sip_header_privacy_t* privacy_header=belle_sip_header_privacy_new();
		if (op->privacy&SalPrivacyCritical)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyCritical));
		if (op->privacy&SalPrivacyHeader)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyHeader));
		if (op->privacy&SalPrivacyId)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyId));
		if (op->privacy&SalPrivacyNone)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyNone));
		if (op->privacy&SalPrivacySession)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacySession));
		if (op->privacy&SalPrivacyUser)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyUser));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(privacy_header));
	}
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),op->base.root->supported);
	return req;
}

belle_sip_response_t *sal_op_create_response_from_request(SalOp *op, belle_sip_request_t *req, int code){
	return sal_create_response_from_request(op->base.root,req,code);
}

/*ping: main purpose is to obtain its own contact address behind firewalls*/
int sal_ping(SalOp *op, const char *from, const char *to){
	sal_op_set_from(op,from);
	sal_op_set_to(op,to);
	return sal_op_send_request(op,sal_op_build_request(op,"OPTIONS"));
}

void sal_op_set_replaces(SalOp* op,belle_sip_header_replaces_t* replaces) {
	if (op->replaces){
		belle_sip_object_unref(op->replaces);
	}
	op->replaces=replaces;
	belle_sip_object_ref(op->replaces);
}

void sal_op_set_remote_ua(SalOp*op,belle_sip_message_t* message) {
	belle_sip_header_user_agent_t* user_agent=belle_sip_message_get_header_by_type(message,belle_sip_header_user_agent_t);
	char user_agent_string[256];
	if (user_agent && belle_sip_header_user_agent_get_products_as_string(user_agent,user_agent_string,sizeof(user_agent_string))>0) {
		if (op->base.remote_ua!=NULL){
			ms_free(op->base.remote_ua);
		}
		op->base.remote_ua=ms_strdup(user_agent_string);
	}
}

int sal_op_send_request_with_expires(SalOp* op, belle_sip_request_t* request,int expires) {
	belle_sip_header_expires_t* expires_header=(belle_sip_header_expires_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_EXPIRES);

	if (!expires_header && expires>=0) {
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(expires_header=belle_sip_header_expires_new()));
	}
	if (expires_header) belle_sip_header_expires_set_expires(expires_header,expires);
	return sal_op_send_request(op,request);
}

void sal_op_resend_request(SalOp* op, belle_sip_request_t* request) {
	belle_sip_header_cseq_t* cseq=(belle_sip_header_cseq_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_CSEQ);
	belle_sip_header_cseq_set_seq_number(cseq,belle_sip_header_cseq_get_seq_number(cseq)+1);
	sal_op_send_request(op,request);
}

static void add_headers(SalOp *op, belle_sip_header_t *h, belle_sip_message_t *msg){

	if (BELLE_SIP_OBJECT_IS_INSTANCE_OF(h,belle_sip_header_contact_t)){
		belle_sip_header_contact_t* newct;
		/*special case for contact, we want to keep everything from the custom contact but set automatic mode and add our own parameters as well*/
		sal_op_set_contact_address(op,(SalAddress*)BELLE_SIP_HEADER_ADDRESS(h));
		newct = sal_op_create_contact(op);
		belle_sip_message_set_header(BELLE_SIP_MESSAGE(msg),BELLE_SIP_HEADER(newct));
		return;
	}
	/*if a header already exists in the message, replace it*/
	belle_sip_message_set_header(msg,h);

}

void _sal_op_add_custom_headers(SalOp *op, belle_sip_message_t *msg){
	if (op->base.sent_custom_headers){
		belle_sip_message_t *ch=(belle_sip_message_t*)op->base.sent_custom_headers;
		belle_sip_list_t *l=belle_sip_message_get_all_headers(ch);
		belle_sip_list_t *elem;
		for(elem=l;elem!=NULL;elem=elem->next){
			add_headers(op,(belle_sip_header_t*)elem->data,msg);
		}
		belle_sip_list_free(l);
	}
}

static int _sal_op_send_request_with_contact(SalOp* op, belle_sip_request_t* request, bool_t add_contact) {
	belle_sip_client_transaction_t* client_transaction;
	belle_sip_provider_t* prov=op->base.root->prov;
	belle_sip_uri_t* outbound_proxy=NULL;
	belle_sip_header_contact_t* contact;
	int result =-1;
	belle_sip_uri_t *next_hop_uri=NULL;

	if (add_contact && !belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(request),belle_sip_header_contact_t)) {
		contact = sal_op_create_contact(op);
		belle_sip_message_set_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(contact));
	} /*keep existing*/

	_sal_op_add_custom_headers(op, (belle_sip_message_t*)request);

	if (!op->dialog || belle_sip_dialog_get_state(op->dialog) == BELLE_SIP_DIALOG_NULL) {
		/*don't put route header if  dialog is in confirmed state*/
		const MSList *elem=sal_op_get_route_addresses(op);
		const char *transport;
		const char *method=belle_sip_request_get_method(request);
		belle_sip_listening_point_t *udplp=belle_sip_provider_get_listening_point(prov,"UDP");

		if (elem) {
			outbound_proxy=belle_sip_header_address_get_uri((belle_sip_header_address_t*)elem->data);
			next_hop_uri=outbound_proxy;
		}else{
			next_hop_uri=(belle_sip_uri_t*)belle_sip_object_clone((belle_sip_object_t*)belle_sip_request_get_uri(request));
		}
		transport=belle_sip_uri_get_transport_param(next_hop_uri);
		if (transport==NULL){
			/*compatibility mode: by default it should be udp as not explicitely set and if no udp listening point is available, then use
			 * the first available transport*/
			if (!belle_sip_uri_is_secure(next_hop_uri)){
				if (udplp==NULL){
					if (belle_sip_provider_get_listening_point(prov,"TCP")!=NULL){
						transport="tcp";
					}else if (belle_sip_provider_get_listening_point(prov,"TLS")!=NULL ){
						transport="tls";
					}
				}
				if (transport){
					belle_sip_message("Transport is not specified, using %s because UDP is not available.",transport);
					belle_sip_uri_set_transport_param(next_hop_uri,transport);
				}
			}
		}else{
#ifdef TUNNEL_ENABLED
			if (udplp && BELLE_SIP_OBJECT_IS_INSTANCE_OF(udplp,belle_sip_tunnel_listening_point_t)){
				/* our tunnel mode only supports UDP. Force transport to be set to UDP */
				belle_sip_uri_set_transport_param(next_hop_uri,"udp");
			}
#endif
		}
		/*because in case of tunnel, transport can be changed*/
		transport=belle_sip_uri_get_transport_param(next_hop_uri);
		
		if ((strcmp(method,"REGISTER")==0 || strcmp(method,"SUBSCRIBE")==0) && transport &&
			(strcasecmp(transport,"TCP")==0 || strcasecmp(transport,"TLS")==0)){
			/*RFC 5923: add 'alias' parameter to tell the server that we want it to keep the connection for future requests*/
			belle_sip_header_via_t *via=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(request),belle_sip_header_via_t);
			belle_sip_parameters_set_parameter(BELLE_SIP_PARAMETERS(via),"alias",NULL);
		}
	}

	client_transaction = belle_sip_provider_create_client_transaction(prov,request);
	belle_sip_transaction_set_application_data(BELLE_SIP_TRANSACTION(client_transaction),sal_op_ref(op));
	if (op->pending_client_trans) belle_sip_object_unref(op->pending_client_trans);
	op->pending_client_trans=client_transaction; /*update pending inv for being able to cancel*/
	belle_sip_object_ref(op->pending_client_trans);

	if (belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(request),belle_sip_header_user_agent_t)==NULL)
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(op->base.root->user_agent));

	if (!belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_AUTHORIZATION)
		&& !belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_PROXY_AUTHORIZATION)) {
		/*hmm just in case we already have authentication param in cache*/
		belle_sip_provider_add_authorization(op->base.root->prov,request,NULL,NULL,NULL,op->base.realm);
	}
	result = belle_sip_client_transaction_send_request_to(client_transaction,next_hop_uri/*might be null*/);

	/*update call id if not set yet for this OP*/
	if (result == 0 && !op->base.call_id) {
		op->base.call_id=ms_strdup(belle_sip_header_call_id_get_call_id(BELLE_SIP_HEADER_CALL_ID(belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(request), belle_sip_header_call_id_t))));
	}

	return result;

}

int sal_op_send_request(SalOp* op, belle_sip_request_t* request)  {
	bool_t need_contact=FALSE;
	if (request==NULL) {
		return -1; /*sanity check*/
	}
	/*
	  Header field          where   proxy ACK BYE CAN INV OPT REG
	  ___________________________________________________________
	  Contact                 R            o   -   -   m   o   o
	 */
	if (strcmp(belle_sip_request_get_method(request),"INVITE")==0
			||strcmp(belle_sip_request_get_method(request),"REGISTER")==0
			||strcmp(belle_sip_request_get_method(request),"SUBSCRIBE")==0
			||strcmp(belle_sip_request_get_method(request),"OPTIONS")==0
			||strcmp(belle_sip_request_get_method(request),"REFER")==0) /* Despite contact seems not mandatory, call flow example show a Contact in REFER requests*/
		need_contact=TRUE;

	return _sal_op_send_request_with_contact(op, request,need_contact);
}

int sal_reason_to_sip_code(SalReason r){
	int ret=500;
	switch(r){
		case SalReasonNone:
			ret=200;
			break;
		case SalReasonIOError:
			ret=503;
			break;
		case SalReasonUnknown:
			ret=400;
			break;
		case SalReasonBusy:
			ret=486;
			break;
		case SalReasonDeclined:
			ret=603;
			break;
		case SalReasonDoNotDisturb:
			ret=600;
			break;
		case SalReasonForbidden:
			ret=403;
			break;
		case SalReasonUnsupportedContent:
			ret=415;
			break;
		case SalReasonNotFound:
			ret=404;
			break;
		case SalReasonRedirect:
			ret=302;
			break;
		case SalReasonTemporarilyUnavailable:
			ret=480;
			break;
		case SalReasonServiceUnavailable:
			ret=503;
			break;
		case SalReasonRequestPending:
			ret=491;
			break;
		case SalReasonUnauthorized:
			ret=401;
			break;
		case SalReasonNotAcceptable:
			ret=488; /*or maybe 606 Not Acceptable ?*/
			break;
		case SalReasonNoMatch:
			ret=481;
			break;
		case SalReasonRequestTimeout:
			ret=408;
			break;
		case SalReasonMovedPermanently:
			ret=301;
			break;
		case SalReasonGone:
			ret=410;
			break;
		case SalReasonAddressIncomplete:
			ret=484;
			break;
		case SalReasonNotImplemented:
			ret=501;
			break;
		case SalReasonServerTimeout:
			ret=504;
			break;
		case SalReasonBadGateway:
			ret=502;
			break;
		case SalReasonInternalError:
			ret=500;
			break;
	}
	return ret;
}

SalReason _sal_reason_from_sip_code(int code) {
	if (code>=100 && code<300) return SalReasonNone;

	switch(code) {
	case 0:
		return SalReasonIOError;
	case 301:
		return SalReasonMovedPermanently;
	case 302:
		return SalReasonRedirect;
	case 401:
	case 407:
		return SalReasonUnauthorized;
	case 403:
		return SalReasonForbidden;
	case 404:
		return SalReasonNotFound;
	case 408:
		return SalReasonRequestTimeout;
	case 410:
		return SalReasonGone;
	case 415:
		return SalReasonUnsupportedContent;
	case 422:
		ms_error ("422 not implemented yet");;
		break;
	case 480:
		return SalReasonTemporarilyUnavailable;
	case 481:
		return SalReasonNoMatch;
	case 484:
		return SalReasonAddressIncomplete;
	case 486:
		return SalReasonBusy;
	case 487:
		return SalReasonNone;
	case 488:
		return SalReasonNotAcceptable;
	case 491:
		return SalReasonRequestPending;
	case 500:
		return SalReasonInternalError;
	case 501:
		return SalReasonNotImplemented;
	case 502:
		return SalReasonBadGateway;
	case 503:
		return SalReasonServiceUnavailable;
	case 504:
		return SalReasonServerTimeout;
	case 600:
		return SalReasonDoNotDisturb;
	case 603:
		return SalReasonDeclined;
	default:
		return SalReasonUnknown;
	}
	return SalReasonUnknown;
}

const SalErrorInfo *sal_error_info_none(void){
	static SalErrorInfo none={
		SalReasonNone,
		"Ok",
		200,
		NULL,
		NULL
	};
	return &none;
}

void sal_error_info_reset(SalErrorInfo *ei){
	if (ei->status_string){
		ms_free(ei->status_string);
		ei->status_string=NULL;
	}
	if (ei->warnings){
		ms_free(ei->warnings);
		ei->warnings=NULL;

	}
	if (ei->full_string){
		ms_free(ei->full_string);
		ei->full_string=NULL;
	}
	ei->protocol_code=0;
	ei->reason=SalReasonNone;
}

void sal_error_info_set(SalErrorInfo *ei, SalReason reason, int code, const char *status_string, const char *warning){
	sal_error_info_reset(ei);
	if (reason==SalReasonUnknown) ei->reason=_sal_reason_from_sip_code(code);
	else ei->reason=reason;
	ei->protocol_code=code;
	ei->status_string=status_string ? ms_strdup(status_string) : NULL;
	ei->warnings=warning ? ms_strdup(warning) : NULL;
	if (ei->status_string){
		if (ei->warnings)
			ei->full_string=ms_strdup_printf("%s %s",ei->status_string,ei->warnings);
		else ei->full_string=ms_strdup(ei->status_string);
	}
}

void sal_op_set_error_info_from_response(SalOp *op, belle_sip_response_t *response){
	int code = belle_sip_response_get_status_code(response);
	const char *reason_phrase=belle_sip_response_get_reason_phrase(response);
	/*Remark: the reason header is to be used mainly in SIP requests, thus the use and prototype of this function should be changed.*/
	belle_sip_header_t* reason_header = belle_sip_message_get_header(BELLE_SIP_MESSAGE(response),"Reason");
	belle_sip_header_t *warning=belle_sip_message_get_header(BELLE_SIP_MESSAGE(response),"Warning");
	SalErrorInfo *ei=&op->error_info;
	const char *warnings;

	warnings=warning ? belle_sip_header_get_unparsed_value(warning) : NULL;
	if (warnings==NULL) warnings=reason_header ? belle_sip_header_get_unparsed_value(reason_header) : NULL;
	sal_error_info_set(ei,SalReasonUnknown,code,reason_phrase,warnings);
}

const SalErrorInfo *sal_op_get_error_info(const SalOp *op){
	return &op->error_info;
}

static void unlink_op_with_dialog(SalOp *op, belle_sip_dialog_t* dialog){
	belle_sip_dialog_set_application_data(dialog,NULL);
	sal_op_unref(op);
	belle_sip_object_unref(dialog);
}

static belle_sip_dialog_t *link_op_with_dialog(SalOp *op, belle_sip_dialog_t* dialog){
	belle_sip_dialog_set_application_data(dialog,sal_op_ref(op));
	belle_sip_object_ref(dialog);
	return dialog;
}

void set_or_update_dialog(SalOp* op, belle_sip_dialog_t* dialog) {
	ms_message("op [%p] : set_or_update_dialog() current=[%p] new=[%p]",op,op->dialog,dialog);
	sal_op_ref(op);
	if (op->dialog!=dialog){
		if (op->dialog){
			/*FIXME: shouldn't we delete unconfirmed dialogs ?*/
			unlink_op_with_dialog(op,op->dialog);
			op->dialog=NULL;
		}
		if (dialog) {
			op->dialog=link_op_with_dialog(op,dialog);
			belle_sip_dialog_enable_pending_trans_checking(dialog,op->base.root->pending_trans_checking);
		}
	}
	sal_op_unref(op);
}
/*return reffed op*/
SalOp* sal_op_ref(SalOp* op) {
	op->ref++;
	return op;
}
/*return null, destroy op if ref count =0*/
void* sal_op_unref(SalOp* op) {
	op->ref--;
	if (op->ref==0) {
		sal_op_release_impl(op);
	}else if (op->ref<0){
		ms_fatal("SalOp [%p]: too many unrefs.",op);
	}
	return NULL;
}

int sal_op_send_and_create_refresher(SalOp* op,belle_sip_request_t* req, int expires,belle_sip_refresher_listener_t listener ) {
	if (sal_op_send_request_with_expires(op,req,expires)==0) {
		if (op->refresher) {
			belle_sip_refresher_stop(op->refresher);
			belle_sip_object_unref(op->refresher);
		}
		if ((op->refresher = belle_sip_client_transaction_create_refresher(op->pending_client_trans))) {
			/*since refresher acquires the transaction, we should remove our context from the transaction, because we won't be notified
			 * that it is terminated anymore.*/
			sal_op_unref(op);/*loose the reference that was given to the transaction when creating it*/
			/* Note that the refresher will replace our data with belle_sip_transaction_set_application_data().
			 Something in the design is not very good here, it makes things complicated to the belle-sip user.
			 Possible ideas to improve things: refresher shall not use belle_sip_transaction_set_application_data() internally, refresher should let the first transaction
			 notify the user as a normal transaction*/
			belle_sip_refresher_set_listener(op->refresher,listener,op);
			belle_sip_refresher_set_retry_after(op->refresher,op->base.root->refresher_retry_after);
			belle_sip_refresher_set_realm(op->refresher,op->base.realm);
			belle_sip_refresher_enable_manual_mode(op->refresher,op->manual_refresher);
			return 0;
		} else {
			return -1;
		}
	}
	return -1;
}

const char* sal_op_state_to_string(const SalOpState value) {
	switch(value) {
	case SalOpStateEarly: return"SalOpStateEarly";
	case SalOpStateActive: return "SalOpStateActive";
	case SalOpStateTerminating: return "SalOpStateTerminating";
	case SalOpStateTerminated: return "SalOpStateTerminated";
	default:
		return "Unknown";
	}
}

/*
 * Warning: this function takes owneship of the custom headers
 */
void sal_op_set_sent_custom_header(SalOp *op, SalCustomHeader* ch){
	SalOpBase *b=(SalOpBase *)op;
	if (b->sent_custom_headers){
		sal_custom_header_free(b->sent_custom_headers);
		b->sent_custom_headers=NULL;
	}
	if (ch) belle_sip_object_ref((belle_sip_message_t*)ch);
	b->sent_custom_headers=ch;
}

void sal_op_assign_recv_headers(SalOp *op, belle_sip_message_t *incoming){
	if (incoming) belle_sip_object_ref(incoming);
	if (op->base.recv_custom_headers){
		belle_sip_object_unref(op->base.recv_custom_headers);
		op->base.recv_custom_headers=NULL;
	}
	if (incoming){
		op->base.recv_custom_headers=(SalCustomHeader*)incoming;
	}
}

const char *sal_op_get_remote_contact(const SalOp *op){
	/*
	 * remote contact is filled in process_response
	 */
	return op->base.remote_contact;
}

SalBodyHandler * sal_op_get_body_handler(SalOp *op, belle_sip_message_t *msg) {
	belle_sip_body_handler_t *body_handler = belle_sip_message_get_body_handler(msg);
	if (body_handler != NULL) {
		belle_sip_header_content_type_t *content_type = belle_sip_message_get_header_by_type(msg, belle_sip_header_content_type_t);
		belle_sip_header_content_length_t *content_length = belle_sip_message_get_header_by_type(msg, belle_sip_header_content_length_t);
		belle_sip_header_t *content_encoding = belle_sip_message_get_header(msg, "Content-Encoding");
		if (content_type != NULL) belle_sip_body_handler_add_header(body_handler, BELLE_SIP_HEADER(content_type));
		if (content_length != NULL) belle_sip_body_handler_add_header(body_handler, BELLE_SIP_HEADER(content_length));
		if (content_encoding != NULL) belle_sip_body_handler_add_header(body_handler, content_encoding);
	}
	return (SalBodyHandler *)body_handler;
}

void sal_op_set_privacy(SalOp* op,SalPrivacyMask privacy) {
	op->privacy=privacy;
}
SalPrivacyMask sal_op_get_privacy(const SalOp* op) {
	return op->privacy;
}

bool_t sal_op_is_secure(const SalOp* op) {
	const SalAddress* from = sal_op_get_from_address(op);
	const SalAddress* to = sal_op_get_to_address(op);

	return from && to && strcasecmp("sips",sal_address_get_scheme(from))==0 && strcasecmp("sips",sal_address_get_scheme(to))==0;
}

void sal_op_set_manual_refresher_mode(SalOp *op, bool_t enabled){
	op->manual_refresher=enabled;
}

int sal_op_get_address_family(SalOp *op){
	belle_sip_transaction_t *tr=NULL;
	belle_sip_header_address_t *contact;
	

	if (op->refresher)
		tr=(belle_sip_transaction_t *)belle_sip_refresher_get_transaction(op->refresher);

	if (tr==NULL)
		tr=(belle_sip_transaction_t *)op->pending_client_trans;
	if (tr==NULL)
		tr=(belle_sip_transaction_t *)op->pending_server_trans;
	
	if (tr==NULL){
		ms_error("Unable to determine IP version from signaling operation.");
		return AF_UNSPEC;
	}
	
	
	if (op->refresher) {
		belle_sip_response_t *resp = belle_sip_transaction_get_response(tr);
		belle_sip_header_via_t *via = resp ?belle_sip_message_get_header_by_type(resp,belle_sip_header_via_t):NULL;
		if (!via){
			ms_error("Unable to determine IP version from signaling operation, no via header found.");
			return AF_UNSPEC;
		}
		return (strchr(belle_sip_header_via_get_host(via),':') != NULL) ? AF_INET6 : AF_INET;
	} else {
		belle_sip_request_t *req = belle_sip_transaction_get_request(tr);
		contact=(belle_sip_header_address_t*)belle_sip_message_get_header_by_type(req,belle_sip_header_contact_t);
		if (!contact){
			ms_error("Unable to determine IP version from signaling operation, no contact header found.");
		}
		return sal_address_is_ipv6((SalAddress*)contact) ? AF_INET6 : AF_INET;
	}
}

bool_t sal_op_is_idle(SalOp *op){
	if (op->dialog){
		return !belle_sip_dialog_request_pending(op->dialog);
	}
	return TRUE;
}

void sal_op_stop_refreshing(SalOp *op){
	if (op->refresher){
		belle_sip_refresher_stop(op->refresher);
	}
}

void sal_call_set_sdp_handling(SalOp *h, SalOpSDPHandling handling)  {
	if (handling != SalOpSDPNormal) ms_message("Enabling special SDP handling for SalOp[%p]!", h);
	h->sdp_handling = handling;
}
void sal_op_cnx_ip_to_0000_if_sendonly_enable(SalOp *op,bool_t yesno) {
	op->cnx_ip_to_0000_if_sendonly_enabled = yesno;
}

bool_t sal_op_cnx_ip_to_0000_if_sendonly_enabled(SalOp *op) {
	return op->cnx_ip_to_0000_if_sendonly_enabled;
}

bool_t sal_op_is_forked_of(const SalOp *op1, const SalOp *op2){
	return op1->base.call_id && op2->base.call_id && strcmp(op1->base.call_id, op2->base.call_id) == 0;
}
int sal_op_refresh(SalOp *op) {
	if (op->refresher) {
		belle_sip_refresher_refresh(op->refresher,belle_sip_refresher_get_expires(op->refresher));
		return 0;
	}
	ms_warning("sal_refresh on op [%p] of type [%s] no refresher",op,sal_op_type_to_string(op->type));
	return -1;
}

void sal_op_set_event(SalOp *op, const char *eventname){
	belle_sip_header_event_t *header = NULL;
	if (op->event) belle_sip_object_unref(op->event);
	if (eventname){
		header = belle_sip_header_event_create(eventname);
		belle_sip_object_ref(header);
	}
	op->event = header;
}

const char* sal_op_get_public_address(SalOp *op, int *port) {
	if (op && op->refresher) {
		return belle_sip_refresher_get_public_address(op->refresher, port);
	}
	return NULL;
}

const char* sal_op_get_local_address(SalOp *op, int *port) {
	if (op && op->refresher) {
		return belle_sip_refresher_get_local_address(op->refresher, port);
	}
	return NULL;
}

char* sal_op_get_dialog_id(const SalOp *op) {
	if (op->dialog != NULL) {
		return ms_strdup_printf("%s;to-tag=%s;from-tag=%s", ((SalOpBase*)op)->call_id,
			belle_sip_dialog_get_remote_tag(op->dialog), belle_sip_dialog_get_local_tag(op->dialog));
	}
	return NULL;
}

