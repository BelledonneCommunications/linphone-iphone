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
	op->type=SalOpUnknown;
	op->privacy=SalPrivacyNone;
	op->manual_refresher=FALSE;/*tells that requests with expiry (SUBSCRIBE, PUBLISH) will be automatically refreshed*/
	sal_op_ref(op);
	return op;
}

void sal_op_release(SalOp *op){
	/*if in terminating state, keep this state because it means we are waiting for a response to be able to terminate the operation.*/
	if (op->state!=SalOpStateTerminating)
		op->state=SalOpStateTerminated;
	sal_op_set_user_pointer(op,NULL);/*mandatory because releasing op doesn't not mean freeing op. Make sure back pointer will not be used later*/
	if (op->refresher) {
		belle_sip_refresher_stop(op->refresher);
	}
	sal_op_unref(op);
}

void sal_op_release_impl(SalOp *op){
	ms_message("Destroying op [%p] of type [%s]",op,sal_op_type_to_string(op->type));
	if (op->pending_auth_transaction) belle_sip_object_unref(op->pending_auth_transaction);
	if (op->auth_info) {
		sal_remove_pending_auth(op->base.root,op);
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
	if (op->event) belle_sip_object_unref(op->event);
	__sal_op_free(op);
	return ;
}

void sal_op_authenticate(SalOp *op, const SalAuthInfo *info){
	if (op->type == SalOpRegister) {
		/*Registration authenticate is just about registering again*/
		sal_register_refresh(op,-1);
	}else {
		/*for sure auth info will be accesible from the provider*/
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
	belle_sip_uri_set_secure(contact_uri,sal_op_is_secure(op));

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

belle_sip_request_t* sal_op_build_request(SalOp *op,const char* method) {
	belle_sip_header_from_t* from_header;
	belle_sip_header_to_t* to_header;
	belle_sip_provider_t* prov=op->base.root->prov;
	belle_sip_request_t *req;
	belle_sip_uri_t* req_uri;
	char token[10];


	if (strcmp("REGISTER",method)==0 || op->privacy==SalPrivacyNone) {
		from_header = belle_sip_header_from_create(BELLE_SIP_HEADER_ADDRESS(sal_op_get_from_address(op))
												,belle_sip_random_token(token,sizeof(token)));
	} else {
		from_header=belle_sip_header_from_create2("Anonymous <sip:anonymous@anonymous.invalid>",belle_sip_random_token(token,sizeof(token)));
	}
	/*make sure to preserve components like headers or port*/
	req_uri = (belle_sip_uri_t*)belle_sip_object_clone((belle_sip_object_t*)belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(sal_op_get_to_address(op))));
	belle_sip_uri_set_secure(req_uri,sal_op_is_secure(op));

	to_header = belle_sip_header_to_create(BELLE_SIP_HEADER_ADDRESS(sal_op_get_to_address(op)),NULL);

	req=belle_sip_request_create(
							req_uri,
							method,
		                    belle_sip_provider_create_call_id(prov),
		                    belle_sip_header_cseq_create(20,method),
		                    from_header,
		                    to_header,
		                    belle_sip_header_via_new(),
		                    70);

	if (op->privacy&SalPrivacyId) {
		belle_sip_header_p_preferred_identity_t* p_preferred_identity=belle_sip_header_p_preferred_identity_create(BELLE_SIP_HEADER_ADDRESS(sal_op_get_from_address(op)));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(p_preferred_identity));
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

void sal_op_set_remote_ua(SalOp*op,belle_sip_message_t* message) {
	belle_sip_header_user_agent_t* user_agent=belle_sip_message_get_header_by_type(message,belle_sip_header_user_agent_t);
	char user_agent_string[256];
	if(user_agent && belle_sip_header_user_agent_get_products_as_string(user_agent,user_agent_string,sizeof(user_agent_string))>0) {
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

static void add_headers(belle_sip_header_t *h, belle_sip_message_t *msg){
	if (belle_sip_message_get_header(msg,belle_sip_header_get_name(h))==NULL)
		belle_sip_message_add_header(msg,h);
}

static void _sal_op_add_custom_headers(SalOp *op, belle_sip_message_t *msg){
	if (op->base.sent_custom_headers){
		belle_sip_message_t *ch=(belle_sip_message_t*)op->base.sent_custom_headers;
		belle_sip_list_t *l=belle_sip_message_get_all_headers(ch);
		belle_sip_list_for_each2(l,(void (*)(void *, void *))add_headers,msg);
		belle_sip_list_free(l);
	}
}

static int _sal_op_send_request_with_contact(SalOp* op, belle_sip_request_t* request,bool_t add_contact) {
	belle_sip_client_transaction_t* client_transaction;
	belle_sip_provider_t* prov=op->base.root->prov;
	belle_sip_uri_t* outbound_proxy=NULL;
	belle_sip_header_contact_t* contact;
	int result =-1;
	
	_sal_op_add_custom_headers(op, (belle_sip_message_t*)request);
	
	if (!op->dialog || belle_sip_dialog_get_state(op->dialog) == BELLE_SIP_DIALOG_NULL) {
		/*don't put route header if  dialog is in confirmed state*/
		const MSList *elem=sal_op_get_route_addresses(op);
		belle_sip_uri_t *next_hop_uri;
		const char *transport;
		const char *method=belle_sip_request_get_method(request);
		
		if (elem) {
			outbound_proxy=belle_sip_header_address_get_uri((belle_sip_header_address_t*)elem->data);
			next_hop_uri=outbound_proxy;
		}else{
			next_hop_uri=belle_sip_request_get_uri(request);
		}
		transport=belle_sip_uri_get_transport_param(next_hop_uri);
		if (transport==NULL){
			/*compatibility mode: by default it should be udp as not explicitely set and if no udp listening point is available, then use
			 * the first available transport*/
			if (belle_sip_provider_get_listening_point(prov,"UDP")==0){
				if (belle_sip_provider_get_listening_point(prov,"TCP")!=NULL){
					transport="tcp";
				}else if (belle_sip_provider_get_listening_point(prov,"TLS")!=NULL){
					transport="tls";
				}
			}
			if (transport){
				belle_sip_message("Transport is not specified, using %s because UDP is not available.",transport);
				belle_sip_uri_set_transport_param(next_hop_uri,transport);
			}
			/* not really usefull belle_sip_uri_fix(next_hop_uri);*/
		}
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

	if (add_contact) {
		contact = sal_op_create_contact(op);
		belle_sip_message_set_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(contact));
	}
	if (!belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_AUTHORIZATION)
		&& !belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_PROXY_AUTHORIZATION)) {
		/*hmm just in case we already have authentication param in cache*/
		belle_sip_provider_add_authorization(op->base.root->prov,request,NULL,NULL);
	}
	result = belle_sip_client_transaction_send_request_to(client_transaction,outbound_proxy/*might be null*/);
	
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

SalReason sal_reason_to_sip_code(SalReason r){
	int ret=500;
	switch(r){
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
		case SalReasonMedia:
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
			ret=488;
			break;
	}
	return ret;
}

void sal_compute_sal_errors_from_code(int code ,SalError* sal_err,SalReason* sal_reason) {
	switch(code) {
	case 400:
		*sal_err=SalErrorUnknown;
		break;
	case 401:
	case 407:
		*sal_err=SalErrorFailure;
		*sal_reason=SalReasonUnauthorized;
		break;
	case 403:
		*sal_err=SalErrorFailure;
		*sal_reason=SalReasonForbidden;
		break;
	case 404:
		*sal_err=SalErrorFailure;
		*sal_reason=SalReasonNotFound;
		break;
	case 415:
		*sal_err=SalErrorFailure;
		*sal_reason=SalReasonMedia;
		break;
	case 422:
		ms_error ("422 not implemented yet");;
		break;
	case 480:
		*sal_err=SalErrorFailure;
		*sal_reason=SalReasonTemporarilyUnavailable;
		break;
	case 486:
		*sal_err=SalErrorFailure;
		*sal_reason=SalReasonBusy;
		break;
	case 487:
		break;
	case 488:
		*sal_err=SalErrorFailure;
		*sal_reason=SalReasonNotAcceptable;
		break;
	case 491:
		*sal_err=SalErrorFailure;
		*sal_reason=SalReasonRequestPending;
		break;
	case 600:
		*sal_err=SalErrorFailure;
		*sal_reason=SalReasonDoNotDisturb;
		break;
	case 603:
		*sal_err=SalErrorFailure;
		*sal_reason=SalReasonDeclined;
		break;
	case 503:
		*sal_err=SalErrorFailure;
		*sal_reason=SalReasonServiceUnavailable;
		break;
	default:
		if (code>=300){
			*sal_err=SalErrorFailure;
			*sal_reason=SalReasonUnknown;
		}else if (code>=100){
			*sal_err=SalErrorNone;
			*sal_reason=SalReasonUnknown;
		}else if (code==0){
			*sal_err=SalErrorNoResponse;
		}
		/* no break */
	}
}
/*return TRUE if error code*/
bool_t sal_compute_sal_errors(belle_sip_response_t* response,SalError* sal_err,SalReason* sal_reason,char* reason, size_t reason_size) {
	int code = belle_sip_response_get_status_code(response);
	belle_sip_header_t* reason_header = belle_sip_message_get_header(BELLE_SIP_MESSAGE(response),"Reason");
	*sal_err=SalErrorUnknown;
	*sal_reason = SalReasonUnknown;

	if (reason_header){
		snprintf(reason
				,reason_size
				,"%s %s"
				,belle_sip_response_get_reason_phrase(response)
				,belle_sip_header_extension_get_value(BELLE_SIP_HEADER_EXTENSION(reason_header)));
	} else {
		strncpy(reason,belle_sip_response_get_reason_phrase(response),reason_size);
	}
	if (code>=400) {
		sal_compute_sal_errors_from_code(code,sal_err,sal_reason);
		return TRUE;
	} else {
		return FALSE;
	}
}

void set_or_update_dialog(SalOp* op, belle_sip_dialog_t* dialog) {
	/*check if dialog has changed*/
	if (dialog && dialog != op->dialog) {
		ms_message("Dialog set from [%p] to [%p] for op [%p]",op->dialog,dialog,op);
		/*fixme, shouldn't we cancel previous dialog*/
		if (op->dialog) {
			belle_sip_dialog_set_application_data(op->dialog,NULL);
			belle_sip_object_unref(op->dialog);
			sal_op_unref(op);
		}
		op->dialog=dialog;
		belle_sip_dialog_set_application_data(op->dialog,op);
		sal_op_ref(op);
		belle_sip_object_ref(op->dialog);
	}
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
			belle_sip_refresher_set_listener(op->refresher,listener,op);
			belle_sip_refresher_set_retry_after(op->refresher,op->base.root->refresher_retry_after);
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
	return sal_custom_header_find(op->base.recv_custom_headers,"Contact");
}

void sal_op_add_body(SalOp *op, belle_sip_message_t *req, const SalBody *body){
	belle_sip_message_remove_header((belle_sip_message_t*)req,"Content-type");
	belle_sip_message_remove_header((belle_sip_message_t*)req,"Content-length");
	belle_sip_message_set_body((belle_sip_message_t*)req,NULL,0);
	if (body && body->type && body->subtype && body->data){
		belle_sip_message_add_header((belle_sip_message_t*)req,
			(belle_sip_header_t*)belle_sip_header_content_type_create(body->type,body->subtype));
		belle_sip_message_add_header((belle_sip_message_t*)req,
			(belle_sip_header_t*)belle_sip_header_content_length_create(body->size));
		belle_sip_message_set_body((belle_sip_message_t*)req,(const char*)body->data,body->size);
	}
}


bool_t sal_op_get_body(SalOp *op, belle_sip_message_t *msg, SalBody *salbody){
	const char *body = NULL;
	belle_sip_header_content_type_t *content_type;
	belle_sip_header_content_length_t *clen=NULL;
	
	content_type=belle_sip_message_get_header_by_type(msg,belle_sip_header_content_type_t);
	if (content_type){
		body=belle_sip_message_get_body(msg);
		clen=belle_sip_message_get_header_by_type(msg,belle_sip_header_content_length_t);
	}
	
	if (content_type && body && clen) {
		salbody->type=belle_sip_header_content_type_get_type(content_type);
		salbody->subtype=belle_sip_header_content_type_get_subtype(content_type);
		salbody->data=body;
		salbody->size=belle_sip_header_content_length_get_content_length(clen);
		return TRUE;
	}
	memset(salbody,0,sizeof(SalBody));
	return FALSE;
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
