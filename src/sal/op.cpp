/*
 * op.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <cstring>

#include "sal/op.h"
#include "bellesip_sal/sal_impl.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

SalOp::SalOp(Sal *sal) {
	this->root = sal;
	this->sdp_handling = sal->default_sdp_handling;
	memset(&this->error_info, 0, sizeof(this->error_info));
	memset(&this->reason_error_info, 0, sizeof(this->reason_error_info));
	ref();
}

SalOp::~SalOp() {
	ms_message("Destroying op [%p] of type [%s]",this,to_string(this->type));
	
	if (this->pending_auth_transaction) belle_sip_object_unref(this->pending_auth_transaction);
	this->root->remove_pending_auth(this);
	if (this->auth_info) {
		sal_auth_info_delete(this->auth_info);
	}
	if (this->sdp_answer) belle_sip_object_unref(this->sdp_answer);
	if (this->refresher) {
		belle_sip_object_unref(this->refresher);
		this->refresher=NULL;
	}
	if (this->result)
		sal_media_description_unref(this->result);
	if(this->replaces) belle_sip_object_unref(this->replaces);
	if(this->referred_by) belle_sip_object_unref(this->referred_by);

	if (this->pending_client_trans) belle_sip_object_unref(this->pending_client_trans);
	if (this->pending_server_trans) belle_sip_object_unref(this->pending_server_trans);
	if (this->pending_update_server_trans) belle_sip_object_unref(this->pending_update_server_trans);
	if (this->event) belle_sip_object_unref(this->event);
	
	sal_error_info_reset(&this->error_info);
	if (this->from_address){
		sal_address_destroy(this->from_address);
		this->from_address=NULL;
	}
	if (this->to_address){
		sal_address_destroy(this->to_address);
		this->to_address=NULL;
	}

	if (this->service_route){
		sal_address_destroy(this->service_route);
		this->service_route=NULL;
	}

	if (this->origin_address){
		sal_address_destroy(this->origin_address);
		this->origin_address=NULL;
	}

	if (this->from) {
		ms_free(this->from);
		this->from=NULL;
	}
	if (this->to) {
		ms_free(this->to);
		this->to=NULL;
	}
	if (this->subject) {
		ms_free(this->subject);
		this->subject = NULL;
	}
	if (this->route) {
		ms_free(this->route);
		this->route=NULL;
	}
	if (this->realm) {
		ms_free(this->realm);
		this->realm=NULL;
	}
	if (this->contact_address) {
		sal_address_destroy(this->contact_address);
	}
	if (this->origin){
		ms_free(this->origin);
		this->origin=NULL;
	}
	if (this->remote_ua){
		ms_free(this->remote_ua);
		this->remote_ua=NULL;
	}
	if (this->remote_contact){
		ms_free(this->remote_contact);
		this->remote_contact=NULL;
	}
	if (this->remote_contact_address){
		sal_address_destroy(this->remote_contact_address);
	}
	if (this->call_id)
		ms_free(this->call_id);
	if (this->service_route) {
		sal_address_destroy(this->service_route);
	}
	if (this->route_addresses){
		bctbx_list_for_each(this->route_addresses,(void (*)(void*)) sal_address_destroy);
		this->route_addresses=bctbx_list_free(this->route_addresses);
	}
	if (this->recv_custom_headers)
		sal_custom_header_free(this->recv_custom_headers);
	if (this->sent_custom_headers)
		sal_custom_header_free(this->sent_custom_headers);

	if (this->entity_tag != NULL){
		ms_free(this->entity_tag);
		this->entity_tag = NULL;
	}
}

SalOp *SalOp::ref() {
	_ref++;
	return this;
}

void *SalOp::unref() {
	_ref--;
	if (_ref==0) {
		delete this;
	} else if (_ref<0) {
		ms_fatal("SalOp [%p]: too many unrefs.",this);
	}
	return NULL;
}

void SalOp::set_contact_address(const SalAddress *address) {
	if (this->contact_address) sal_address_destroy(this->contact_address);
	this->contact_address=address?sal_address_clone(address):NULL;
}

void SalOp::assign_address(SalAddress** address, const char *value) {
	if (*address){
		sal_address_destroy(*address);
		*address=NULL;
	}
	if (value)
		*address=sal_address_new(value);
}

void SalOp::assign_string(char **str, const char *arg) {
	if (*str){
		ms_free(*str);
		*str=NULL;
	}
	if (arg)
		*str=ms_strdup(arg);
}

void SalOp::set_route(const char *route) {
	char* route_string=NULL;
	if (this->route_addresses) {
		bctbx_list_for_each(this->route_addresses,(void (*)(void *))sal_address_destroy);
		this->route_addresses=bctbx_list_free(this->route_addresses);
	}
	if (route) {
		this->route_addresses=bctbx_list_append(NULL,NULL);
		assign_address((SalAddress**)&(this->route_addresses->data),route);
		route_string=sal_address_as_string((SalAddress*)this->route_addresses->data);
	}
	assign_string(&this->route,route_string);
	if(route_string) ms_free(route_string);
}

void SalOp::set_route_address(const SalAddress *address){
	char* address_string=sal_address_as_string(address); /*can probably be optimized*/
	set_route(address_string);
	ms_free(address_string);
}

void SalOp::add_route_address(const SalAddress *address) {
	if (this->route_addresses) {
		this->route_addresses=bctbx_list_append(this->route_addresses,(void*)sal_address_clone(address));
	} else {
		set_route_address(address);
	}
}

void SalOp::set_realm(const char *realm) {
	if (this->realm != NULL){
		ms_free(this->realm);
	}
	this->realm = ms_strdup(realm);
}

#define SET_PARAM(name) \
		char* name##_string=NULL; \
		assign_address(&this->name##_address,name); \
		if (this->name##_address) { \
			name##_string=sal_address_as_string(this->name##_address); \
		}\
		assign_string(&this->name,name##_string); \
		if(name##_string) ms_free(name##_string);

void SalOp::set_subject (const char *subject) {
	assign_string(&this->subject, subject);
}

void SalOp::set_from(const char *from){
	SET_PARAM(from);
}

void SalOp::set_from_address(const SalAddress *from) {
	char* address_string=sal_address_as_string(from); /*can probably be optimized*/
	set_from(address_string);
	ms_free(address_string);
}

void SalOp::set_to(const char *to) {
	SET_PARAM(to);
}

void SalOp::set_to_address(const SalAddress *to) {
	char* address_string=sal_address_as_string(to); /*can probably be optimized*/
	set_to(address_string);
	ms_free(address_string);
}

void SalOp::set_diversion_address(const SalAddress *diversion) {
	if (this->diversion_address) sal_address_destroy(this->diversion_address);
	this->diversion_address=diversion ? sal_address_clone(diversion) : NULL;
}

int SalOp::refresh() {
	if (this->refresher) {
		belle_sip_refresher_refresh(this->refresher,belle_sip_refresher_get_expires(this->refresher));
		return 0;
	}
	ms_warning("sal_refresh on op [%p] of type [%s] no refresher",this,to_string(this->type));
	return -1;
}

void SalOp::kill_dialog() {
	ms_warning("op [%p]: force kill of dialog [%p]", this, this->dialog);
	belle_sip_dialog_delete(this->dialog);
}

void SalOp::release() {
	/*if in terminating state, keep this state because it means we are waiting for a response to be able to terminate the operation.*/
	if (this->state!=State::Terminating)
		this->state=State::Terminated;
	set_user_pointer(NULL);/*mandatory because releasing op doesn't not mean freeing op. Make sure back pointer will not be used later*/
	if (this->release_cb)
		this->release_cb(this);
	if (this->refresher) {
		belle_sip_refresher_stop(this->refresher);
	}
	this->op_released = TRUE;
	unref();
}

int SalOp::send_request_with_contact(belle_sip_request_t* request, bool_t add_contact) {
	belle_sip_client_transaction_t* client_transaction;
	belle_sip_provider_t* prov=this->root->prov;
	belle_sip_uri_t* outbound_proxy=NULL;
	belle_sip_header_contact_t* contact;
	int result =-1;
	belle_sip_uri_t *next_hop_uri=NULL;

	if (add_contact && !belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(request),belle_sip_header_contact_t)) {
		contact = create_contact();
		belle_sip_message_set_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(contact));
	} /*keep existing*/

	add_custom_headers((belle_sip_message_t*)request);

	if (!this->dialog || belle_sip_dialog_get_state(this->dialog) == BELLE_SIP_DIALOG_NULL) {
		/*don't put route header if  dialog is in confirmed state*/
		const MSList *elem=get_route_addresses();
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
	belle_sip_transaction_set_application_data(BELLE_SIP_TRANSACTION(client_transaction),ref());
	if (this->pending_client_trans) belle_sip_object_unref(this->pending_client_trans);
	
	this->pending_client_trans=client_transaction; /*update pending inv for being able to cancel*/
	belle_sip_object_ref(this->pending_client_trans);

	if (belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(request),belle_sip_header_user_agent_t)==NULL)
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(this->root->user_agent));

	if (!belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_AUTHORIZATION)
		&& !belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_PROXY_AUTHORIZATION)) {
		/*hmm just in case we already have authentication param in cache*/
		belle_sip_provider_add_authorization(this->root->prov,request,NULL,NULL,NULL,this->realm);
	}
	result = belle_sip_client_transaction_send_request_to(client_transaction,next_hop_uri/*might be null*/);

	/*update call id if not set yet for this OP*/
	if (result == 0 && !this->call_id) {
		this->call_id=ms_strdup(belle_sip_header_call_id_get_call_id(BELLE_SIP_HEADER_CALL_ID(belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(request), belle_sip_header_call_id_t))));
	}

	return result;

}

int SalOp::send_request(belle_sip_request_t* request) {
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

	return send_request_with_contact(request,need_contact);
}

void SalOp::resend_request(belle_sip_request_t* request) {
	belle_sip_header_cseq_t* cseq=(belle_sip_header_cseq_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_CSEQ);
	belle_sip_header_cseq_set_seq_number(cseq,belle_sip_header_cseq_get_seq_number(cseq)+1);
	send_request(request);
}

int SalOp::process_redirect(){
	belle_sip_request_t* request = belle_sip_transaction_get_request((belle_sip_transaction_t*)this->pending_client_trans);
	belle_sip_response_t *response = belle_sip_transaction_get_response((belle_sip_transaction_t*)this->pending_client_trans);
	belle_sip_header_contact_t *redirect_contact = belle_sip_message_get_header_by_type((belle_sip_message_t*)response, belle_sip_header_contact_t);
	belle_sip_uri_t *redirect_uri;
	belle_sip_header_call_id_t *callid = belle_sip_message_get_header_by_type((belle_sip_message_t*)request, belle_sip_header_call_id_t);
	belle_sip_header_to_t *to = belle_sip_message_get_header_by_type((belle_sip_message_t*)request, belle_sip_header_to_t);
	
	if (!redirect_contact){
		ms_warning("Redirect not handled, there is no redirect contact header in response");
		return -1;
	}
	
	redirect_uri = belle_sip_header_address_get_uri((belle_sip_header_address_t*) redirect_contact);
	
	if (!redirect_uri){
		ms_warning("Redirect not handled, there is no usable uri in contact.");
		return -1;
	}
	
	if (this->dialog && belle_sip_dialog_get_state(this->dialog)==BELLE_SIP_DIALOG_CONFIRMED){
		ms_warning("Redirect not handled within established dialogs. Does it make sense ?");
		return -1;
	}
	set_or_update_dialog(NULL);
	belle_sip_message_remove_header_from_ptr((belle_sip_message_t*)request, (belle_sip_header_t*)callid);
	belle_sip_message_add_header((belle_sip_message_t*)request, (belle_sip_header_t*)(callid = belle_sip_provider_create_call_id(this->get_sal()->prov)));
	if (this->call_id){
		/*reset the call-id of op, it will be set when new request will be sent*/
		ms_free(this->call_id);
		this->call_id = NULL;
	}
	belle_sip_request_set_uri(request, redirect_uri);
	redirect_uri = BELLE_SIP_URI(belle_sip_object_clone(BELLE_SIP_OBJECT(redirect_uri)));
	belle_sip_uri_set_port(redirect_uri, 0);
	belle_sip_parameters_remove_parameter(BELLE_SIP_PARAMETERS(redirect_uri), "transport");
	belle_sip_header_address_set_uri((belle_sip_header_address_t*)to, redirect_uri);
	send_request(request);
	return 0;
}

void SalOp::process_authentication() {
	belle_sip_request_t* initial_request=belle_sip_transaction_get_request((belle_sip_transaction_t*)this->pending_auth_transaction);
	belle_sip_request_t* new_request;
	bool_t is_within_dialog=FALSE;
	belle_sip_list_t* auth_list=NULL;
	belle_sip_auth_event_t* auth_event;
	belle_sip_response_t *response=belle_sip_transaction_get_response((belle_sip_transaction_t*)this->pending_auth_transaction);
	belle_sip_header_from_t *from=belle_sip_message_get_header_by_type(initial_request,belle_sip_header_from_t);
	belle_sip_uri_t *from_uri=belle_sip_header_address_get_uri((belle_sip_header_address_t*)from);

	if (strcasecmp(belle_sip_uri_get_host(from_uri),"anonymous.invalid")==0){
		/*prefer using the from from the SalOp*/
		from_uri=belle_sip_header_address_get_uri((belle_sip_header_address_t*)get_from_address());
	}

	if (this->dialog && belle_sip_dialog_get_state(this->dialog)==BELLE_SIP_DIALOG_CONFIRMED) {
		new_request = belle_sip_dialog_create_request_from(this->dialog,initial_request);
		if (!new_request)
			new_request = belle_sip_dialog_create_queued_request_from(this->dialog,initial_request);
		is_within_dialog=TRUE;
	} else {
		new_request=initial_request;
		belle_sip_message_remove_header(BELLE_SIP_MESSAGE(new_request),BELLE_SIP_AUTHORIZATION);
		belle_sip_message_remove_header(BELLE_SIP_MESSAGE(new_request),BELLE_SIP_PROXY_AUTHORIZATION);
	}
	if (new_request==NULL) {
		ms_error("sal_process_authentication() op=[%p] cannot obtain new request from dialog.",this);
		return;
	}

	if (belle_sip_provider_add_authorization(this->root->prov,new_request,response,from_uri,&auth_list,this->realm)) {
		if (is_within_dialog) {
			send_request(new_request);
		} else {
			resend_request(new_request);
		}
		this->root->remove_pending_auth(this);
	}else {
		belle_sip_header_from_t *from=belle_sip_message_get_header_by_type(response,belle_sip_header_from_t);
		char *tmp=belle_sip_object_to_string(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(from)));
		ms_message("No auth info found for [%s]",tmp);
		belle_sip_free(tmp);
		this->root->add_pending_auth(this);

		if (is_within_dialog) {
			belle_sip_object_unref(new_request);
		}
	}
	/*always store auth info, for case of wrong credential*/
	if (this->auth_info) {
		sal_auth_info_delete(this->auth_info);
		this->auth_info=NULL;
	}
	if (auth_list){
		auth_event=(belle_sip_auth_event_t*)(auth_list->data);
		this->auth_info=sal_auth_info_create(auth_event);
		belle_sip_list_free_with_data(auth_list,(void (*)(void*))belle_sip_auth_event_destroy);
	}
}

char *SalOp::get_dialog_id() const {
	if (this->dialog != NULL) {
		return ms_strdup_printf("%s;to-tag=%s;from-tag=%s", this->call_id,
			belle_sip_dialog_get_remote_tag(this->dialog), belle_sip_dialog_get_local_tag(this->dialog));
	}
	return NULL;
}

int SalOp::get_address_family() const {
	belle_sip_transaction_t *tr=NULL;
	belle_sip_header_address_t *contact;
	

	if (this->refresher)
		tr=(belle_sip_transaction_t *)belle_sip_refresher_get_transaction(this->refresher);

	if (tr==NULL)
		tr=(belle_sip_transaction_t *)this->pending_client_trans;
	if (tr==NULL)
		tr=(belle_sip_transaction_t *)this->pending_server_trans;
	
	if (tr==NULL){
		ms_error("Unable to determine IP version from signaling operation.");
		return AF_UNSPEC;
	}
	
	
	if (this->refresher) {
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

bool_t SalOp::is_idle() const {
	if (this->dialog){
		return !belle_sip_dialog_request_pending(this->dialog);
	}
	return TRUE;
}

void SalOp::set_entity_tag(const char* entity_tag) {
	if (this->entity_tag != NULL) ms_free(this->entity_tag);
	this->entity_tag = entity_tag ? ms_strdup(entity_tag) : NULL;
}

void SalOp::set_event(const char *eventname) {
	belle_sip_header_event_t *header = NULL;
	if (this->event) belle_sip_object_unref(this->event);
	if (eventname){
		header = belle_sip_header_event_create(eventname);
		belle_sip_object_ref(header);
	}
	this->event = header;
}

void SalOp::add_initial_route_set(belle_sip_request_t *request, const MSList *list) {
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

belle_sip_request_t* SalOp::build_request(const char* method) {
	belle_sip_header_from_t* from_header;
	belle_sip_header_to_t* to_header;
	belle_sip_provider_t* prov=this->root->prov;
	belle_sip_request_t *req;
	belle_sip_uri_t* req_uri;
	belle_sip_uri_t* to_uri;
	belle_sip_header_call_id_t *call_id_header;

	const SalAddress* to_address;
	const MSList *elem=get_route_addresses();
	char token[10];

	/* check that the op has a correct to address */
	to_address = get_to_address();
	if( to_address == NULL ){
		ms_error("No To: address, cannot build request");
		return NULL;
	}

	to_uri = belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(to_address));
	if( to_uri == NULL ){
		ms_error("To: address is invalid, cannot build request");
		return NULL;
	}

	if (strcmp("REGISTER",method)==0 || this->privacy==SalPrivacyNone) {
		from_header = belle_sip_header_from_create(BELLE_SIP_HEADER_ADDRESS(get_from_address())
						,belle_sip_random_token(token,sizeof(token)));
	} else {
		from_header=belle_sip_header_from_create2("Anonymous <sip:anonymous@anonymous.invalid>",belle_sip_random_token(token,sizeof(token)));
	}
	/*make sure to preserve components like headers or port*/

	req_uri = (belle_sip_uri_t*)belle_sip_object_clone((belle_sip_object_t*)to_uri);
	belle_sip_uri_set_secure(req_uri,is_secure());

	to_header = belle_sip_header_to_create(BELLE_SIP_HEADER_ADDRESS(to_address),NULL);
	call_id_header = belle_sip_provider_create_call_id(prov);
	if (get_call_id()) {
		belle_sip_header_call_id_set_call_id(call_id_header, get_call_id());
	}

	req=belle_sip_request_create(
					req_uri,
					method,
					call_id_header,
					belle_sip_header_cseq_create(20,method),
					from_header,
					to_header,
					belle_sip_header_via_new(),
					70);

	if (this->privacy & SalPrivacyId) {
		belle_sip_header_p_preferred_identity_t* p_preferred_identity=belle_sip_header_p_preferred_identity_create(BELLE_SIP_HEADER_ADDRESS(get_from_address()));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(p_preferred_identity));
	}

	if (elem && strcmp(method,"REGISTER")!=0 && !this->root->no_initial_route){
		add_initial_route_set(req,elem);
	}

	if (strcmp("REGISTER",method)!=0 && this->privacy!=SalPrivacyNone ){
		belle_sip_header_privacy_t* privacy_header=belle_sip_header_privacy_new();
		if (this->privacy&SalPrivacyCritical)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyCritical));
		if (this->privacy&SalPrivacyHeader)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyHeader));
		if (this->privacy&SalPrivacyId)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyId));
		if (this->privacy&SalPrivacyNone)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyNone));
		if (this->privacy&SalPrivacySession)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacySession));
		if (this->privacy&SalPrivacyUser)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyUser));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(privacy_header));
	}
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),this->root->supported);
	return req;
}

void SalOp::set_error_info_from_response(belle_sip_response_t *response) {
	int code = belle_sip_response_get_status_code(response);
	const char *reason_phrase=belle_sip_response_get_reason_phrase(response);
	belle_sip_header_t *warning=belle_sip_message_get_header(BELLE_SIP_MESSAGE(response),"Warning");
	SalErrorInfo *ei=&this->error_info;
	const char *warnings;

	warnings=warning ? belle_sip_header_get_unparsed_value(warning) : NULL;
	sal_error_info_set(ei,SalReasonUnknown,"SIP", code,reason_phrase,warnings);
	set_reason_error_info(BELLE_SIP_MESSAGE(response));
}

const char* SalOp::to_string(const State value) {
	switch(value) {
		case State::Early: return"SalOpStateEarly";
		case State::Active: return "SalOpStateActive";
		case State::Terminating: return "SalOpStateTerminating";
		case State::Terminated: return "SalOpStateTerminated";
	default:
		return "Unknown";
	}
}

void SalOp::set_reason_error_info(belle_sip_message_t *msg) {
	belle_sip_header_reason_t* reason_header = belle_sip_message_get_header_by_type(msg,belle_sip_header_reason_t);
	if (reason_header){
		SalErrorInfo *ei=&this->reason_error_info; // ?//
		const char *protocol = belle_sip_header_reason_get_protocol(reason_header);
		int code = belle_sip_header_reason_get_cause(reason_header);
		const char *text = belle_sip_header_reason_get_text(reason_header);
		sal_error_info_set(ei, SalReasonUnknown, protocol, code, text, NULL);
	}
}

void SalOp::set_referred_by(belle_sip_header_referred_by_t* referred_by) {
	if (this->referred_by){
		belle_sip_object_unref(this->referred_by);
	}
	
	this->referred_by=referred_by;
	belle_sip_object_ref(this->referred_by);
}

void SalOp::set_replaces(belle_sip_header_replaces_t* replaces) {
	if (this->replaces){
		belle_sip_object_unref(this->replaces);
	}
	
	this->replaces=replaces;
	belle_sip_object_ref(this->replaces);
}

int SalOp::send_request_with_expires(belle_sip_request_t* request,int expires) {
	belle_sip_header_expires_t* expires_header=(belle_sip_header_expires_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_EXPIRES);

	if (!expires_header && expires>=0) {
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(expires_header=belle_sip_header_expires_new()));
	}
	if (expires_header) belle_sip_header_expires_set_expires(expires_header,expires);
	return send_request(request);
}

int SalOp::send_and_create_refresher(belle_sip_request_t* req, int expires,belle_sip_refresher_listener_t listener) {
	if (send_request_with_expires(req,expires)==0) {
		if (this->refresher) {
			belle_sip_refresher_stop(this->refresher);
			belle_sip_object_unref(this->refresher);
		}
		if ((this->refresher = belle_sip_client_transaction_create_refresher(this->pending_client_trans))) {
			/*since refresher acquires the transaction, we should remove our context from the transaction, because we won't be notified
			 * that it is terminated anymore.*/
			unref();/*loose the reference that was given to the transaction when creating it*/
			/* Note that the refresher will replace our data with belle_sip_transaction_set_application_data().
			 Something in the design is not very good here, it makes things complicated to the belle-sip user.
			 Possible ideas to improve things: refresher shall not use belle_sip_transaction_set_application_data() internally, refresher should let the first transaction
			 notify the user as a normal transaction*/
			belle_sip_refresher_set_listener(this->refresher,listener, this);
			belle_sip_refresher_set_retry_after(this->refresher,this->root->refresher_retry_after);
			belle_sip_refresher_set_realm(this->refresher,this->realm);
			belle_sip_refresher_enable_manual_mode(this->refresher, this->manual_refresher);
			return 0;
		} else {
			return -1;
		}
	}
	return -1;
}

belle_sip_header_contact_t *SalOp::create_contact() {
	belle_sip_header_contact_t* contact_header;
	belle_sip_uri_t* contact_uri;

	if (get_contact_address()) {
		contact_header = belle_sip_header_contact_create(BELLE_SIP_HEADER_ADDRESS(get_contact_address()));
	} else {
		contact_header= belle_sip_header_contact_new();
	}

	if (!(contact_uri=belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(contact_header)))) {
		/*no uri, just creating a new one*/
		contact_uri=belle_sip_uri_new();
		belle_sip_header_address_set_uri(BELLE_SIP_HEADER_ADDRESS(contact_header),contact_uri);
	}

	belle_sip_uri_set_user_password(contact_uri,NULL);
	belle_sip_uri_set_secure(contact_uri,is_secure());
	if (this->privacy!=SalPrivacyNone){
		belle_sip_uri_set_user(contact_uri,NULL);
	}
	/*don't touch contact in case of gruu*/
	if (!belle_sip_parameters_has_parameter(BELLE_SIP_PARAMETERS(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(contact_header))),"gr")) {
		belle_sip_header_contact_set_automatic(contact_header,this->root->auto_contacts);
		if (this->root->uuid) {
			if (belle_sip_parameters_has_parameter(BELLE_SIP_PARAMETERS(contact_header),"+sip.instance")==0){
				char *instance_id=belle_sip_strdup_printf("\"<urn:uuid:%s>\"",this->root->uuid);
				belle_sip_parameters_set_parameter(BELLE_SIP_PARAMETERS(contact_header),"+sip.instance",instance_id);
				belle_sip_free(instance_id);
			}
		}
	}
	if (this->root->linphone_specs && strlen(this->root->linphone_specs) > 0) {
		if (belle_sip_parameters_has_parameter(BELLE_SIP_PARAMETERS(contact_header),"+org.linphone.specs") == 0) {
			belle_sip_parameters_set_parameter(BELLE_SIP_PARAMETERS(contact_header), "+org.linphone.specs", this->root->linphone_specs);
		}
	}
	return contact_header;
}

void SalOp::unlink_op_with_dialog(belle_sip_dialog_t* dialog) {
	belle_sip_dialog_set_application_data(dialog,NULL);
	unref();
	belle_sip_object_unref(dialog);
}

belle_sip_dialog_t *SalOp::link_op_with_dialog(belle_sip_dialog_t* dialog) {
	belle_sip_dialog_set_application_data(dialog,ref());
	belle_sip_object_ref(dialog);
	return dialog;
}

void SalOp::set_or_update_dialog(belle_sip_dialog_t* dialog) {
	ms_message("op [%p] : set_or_update_dialog() current=[%p] new=[%p]", this, this->dialog,dialog);
	ref();
	if (this->dialog!=dialog){
		if (this->dialog){
			/*FIXME: shouldn't we delete unconfirmed dialogs ?*/
			unlink_op_with_dialog(this->dialog);
			this->dialog=NULL;
		}
		if (dialog) {
			this->dialog=link_op_with_dialog(dialog);
			belle_sip_dialog_enable_pending_trans_checking(dialog,this->root->pending_trans_checking);
		}
	}
	unref();
}

int SalOp::ping(const char *from, const char *to) {
	set_from(from);
	set_to(to);
	return send_request(build_request("OPTIONS"));
}

int SalOp::send_info(const char *from, const char *to, const SalBodyHandler *body_handler) {
	if (this->dialog && belle_sip_dialog_get_state(this->dialog) == BELLE_SIP_DIALOG_CONFIRMED) {
		belle_sip_request_t *req;
		belle_sip_dialog_enable_pending_trans_checking(this->dialog,this->root->pending_trans_checking);
		req=belle_sip_dialog_create_queued_request(this->dialog,"INFO");
		belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(req), BELLE_SIP_BODY_HANDLER(body_handler));
		return send_request(req);
	}else{
		ms_error("Cannot send INFO message on op [%p] because dialog is not in confirmed state yet.", this);
	}
	return -1;
}

SalBodyHandler *SalOp::get_body_handler(belle_sip_message_t *msg) {
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

void SalOp::assign_recv_headers(belle_sip_message_t *incoming) {
	if (incoming) belle_sip_object_ref(incoming);
	if (this->recv_custom_headers){
		belle_sip_object_unref(this->recv_custom_headers);
		this->recv_custom_headers=NULL;
	}
	if (incoming){
		this->recv_custom_headers=(SalCustomHeader*)incoming;
	}
}

void SalOp::set_remote_contact(const char* remote_contact) {
	assign_address(&this->remote_contact_address,remote_contact);
	/*to preserve header params*/
	assign_string(&this->remote_contact,remote_contact);
}

void SalOp::set_network_origin(const char *origin) {
	SET_PARAM(origin);
}

void SalOp::set_network_origin_address(SalAddress *origin){
	char* address_string=sal_address_as_string(origin); /*can probably be optimized*/
	set_network_origin(address_string);
	ms_free(address_string);
}

/*
rfc3323
4.2 Expressing Privacy Preferences
When a Privacy header is constructed, it MUST consist of either the
   value 'none', or one or more of the values 'user', 'header' and
   'session' (each of which MUST appear at most once) which MAY in turn
   be followed by the 'critical' indicator.
 */
void SalOp::set_privacy_from_message(belle_sip_message_t* msg) {
	belle_sip_header_privacy_t* privacy = belle_sip_message_get_header_by_type(msg,belle_sip_header_privacy_t);
	if (!privacy) {
		set_privacy(SalPrivacyNone);
	} else {
		belle_sip_list_t* privacy_list=belle_sip_header_privacy_get_privacy(privacy);
		set_privacy(0);
		for (;privacy_list!=NULL;privacy_list=privacy_list->next) {
			char* privacy_value=(char*)privacy_list->data;
			if(strcmp(sal_privacy_to_string(SalPrivacyCritical),privacy_value) == 0)
				set_privacy(get_privacy()|SalPrivacyCritical);
			if(strcmp(sal_privacy_to_string(SalPrivacyHeader),privacy_value) == 0)
							set_privacy(get_privacy()|SalPrivacyHeader);
			if(strcmp(sal_privacy_to_string(SalPrivacyId),privacy_value) == 0)
							set_privacy(get_privacy()|SalPrivacyId);
			if(strcmp(sal_privacy_to_string(SalPrivacyNone),privacy_value) == 0) {
				set_privacy(SalPrivacyNone);
				break;
			}
			if(strcmp(sal_privacy_to_string(SalPrivacySession),privacy_value) == 0)
							set_privacy(get_privacy()|SalPrivacySession);
			if(strcmp(sal_privacy_to_string(SalPrivacyUser),privacy_value) == 0)
										set_privacy(get_privacy()|SalPrivacyUser);
		}
	}
}

void SalOp::set_remote_ua(belle_sip_message_t* message) {
	belle_sip_header_user_agent_t* user_agent=belle_sip_message_get_header_by_type(message,belle_sip_header_user_agent_t);
	char user_agent_string[256];
	if (user_agent && belle_sip_header_user_agent_get_products_as_string(user_agent,user_agent_string,sizeof(user_agent_string))>0) {
		if (this->remote_ua!=NULL){
			ms_free(this->remote_ua);
		}
		this->remote_ua=ms_strdup(user_agent_string);
	}
}

const char *SalOp::to_string(const Type type) {
	switch(type) {
		case Type::Register: return "SalOpRegister";
		case Type::Call: return "SalOpCall";
		case Type::Message: return "SalOpMessage";
		case Type::Presence: return "SalOpPresence";
		default: return "SalOpUnknown";
	}
}

bool_t SalOp::is_secure() const {
	const SalAddress* from = get_from_address();
	const SalAddress* to = get_to_address();
	return from && to && strcasecmp("sips",sal_address_get_scheme(from))==0 && strcasecmp("sips",sal_address_get_scheme(to))==0;
}

/*
 * Warning: this function takes owneship of the custom headers
 */
void SalOp::set_sent_custom_header(SalCustomHeader* ch){
	if (this->sent_custom_headers){
		sal_custom_header_free(this->sent_custom_headers);
		this->sent_custom_headers=NULL;
	}
	if (ch) belle_sip_object_ref((belle_sip_message_t*)ch);
	this->sent_custom_headers=ch;
}

void SalOp::add_headers(belle_sip_header_t *h, belle_sip_message_t *msg){

	if (BELLE_SIP_OBJECT_IS_INSTANCE_OF(h,belle_sip_header_contact_t)){
		belle_sip_header_contact_t* newct;
		/*special case for contact, we want to keep everything from the custom contact but set automatic mode and add our own parameters as well*/
		set_contact_address((SalAddress*)BELLE_SIP_HEADER_ADDRESS(h));
		newct = create_contact();
		belle_sip_message_set_header(BELLE_SIP_MESSAGE(msg),BELLE_SIP_HEADER(newct));
		return;
	}
	/*if a header already exists in the message, replace it*/
	belle_sip_message_set_header(msg,h);

}

void SalOp::add_custom_headers(belle_sip_message_t *msg){
	if (this->sent_custom_headers){
		belle_sip_message_t *ch=(belle_sip_message_t*)this->sent_custom_headers;
		belle_sip_list_t *l=belle_sip_message_get_all_headers(ch);
		belle_sip_list_t *elem;
		for(elem=l;elem!=NULL;elem=elem->next){
			add_headers((belle_sip_header_t*)elem->data,msg);
		}
		belle_sip_list_free(l);
	}
}

int SalOp::unsubscribe(){
	if (this->refresher){
		const belle_sip_transaction_t *tr=(const belle_sip_transaction_t*) belle_sip_refresher_get_transaction(this->refresher);
		belle_sip_request_t *last_req=belle_sip_transaction_get_request(tr);
		belle_sip_message_set_body(BELLE_SIP_MESSAGE(last_req), NULL, 0);
		belle_sip_refresher_refresh(this->refresher,0);
		return 0;
	}
	return -1;
}

void SalOp::process_incoming_message(const belle_sip_request_event_t *event) {
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_server_transaction_t* server_transaction = belle_sip_provider_create_server_transaction(this->root->prov,req);
	belle_sip_header_address_t* address;
	belle_sip_header_from_t* from_header;
	belle_sip_header_content_type_t* content_type;
	belle_sip_response_t* resp;
	int errcode = 500;
	belle_sip_header_call_id_t* call_id = belle_sip_message_get_header_by_type(req,belle_sip_header_call_id_t);
	belle_sip_header_cseq_t* cseq = belle_sip_message_get_header_by_type(req,belle_sip_header_cseq_t);
	belle_sip_header_date_t *date=belle_sip_message_get_header_by_type(req,belle_sip_header_date_t);
	char* from;
	bool_t external_body=FALSE;

	from_header=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_from_t);
	content_type=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_content_type_t);
	
	if (content_type) {
		SalMessage salmsg;
		char message_id[256]={0};

		if (this->pending_server_trans) belle_sip_object_unref(this->pending_server_trans);
		
		this->pending_server_trans=server_transaction;
		belle_sip_object_ref(this->pending_server_trans);

		external_body=is_external_body(content_type);
		address=belle_sip_header_address_create(belle_sip_header_address_get_displayname(BELLE_SIP_HEADER_ADDRESS(from_header))
				,belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(from_header)));
		from=belle_sip_object_to_string(BELLE_SIP_OBJECT(address));
		snprintf(message_id,sizeof(message_id)-1,"%s%i"
				,belle_sip_header_call_id_get_call_id(call_id)
				,belle_sip_header_cseq_get_seq_number(cseq));
		salmsg.from=from;
		/* if we just deciphered a message, use the deciphered part(which can be a rcs xml body pointing to the file to retreive from server)*/
		salmsg.text=(!external_body)?belle_sip_message_get_body(BELLE_SIP_MESSAGE(req)):NULL;
		salmsg.url=NULL;

		char buffer[1024];
		size_t offset = 0;
		belle_sip_parameters_marshal(BELLE_SIP_PARAMETERS(content_type), buffer, 1024, &offset);
		buffer[offset] = '\0';
		salmsg.content_type = ms_strdup_printf("%s/%s%s", belle_sip_header_content_type_get_type(content_type), belle_sip_header_content_type_get_subtype(content_type), buffer);
		if (external_body && belle_sip_parameters_get_parameter(BELLE_SIP_PARAMETERS(content_type),"URL")) {
			size_t url_length=strlen(belle_sip_parameters_get_parameter(BELLE_SIP_PARAMETERS(content_type),"URL"));
			salmsg.url = ms_strdup(belle_sip_parameters_get_parameter(BELLE_SIP_PARAMETERS(content_type),"URL")+1); /* skip first "*/
			((char*)salmsg.url)[url_length-2]='\0'; /*remove trailing "*/
		}
		
		salmsg.message_id=message_id;
		salmsg.time=date ? belle_sip_header_date_get_time(date) : time(NULL);
		this->root->callbacks.message_received(this,&salmsg);

		belle_sip_object_unref(address);
		belle_sip_free(from);
		if (salmsg.url) ms_free((char*)salmsg.url);
		ms_free((char *)salmsg.content_type);
	} else {
		ms_error("Unsupported MESSAGE (no Content-Type)");
		resp = belle_sip_response_create_from_request(req, errcode);
		add_message_accept((belle_sip_message_t*)resp);
		belle_sip_server_transaction_send_response(server_transaction,resp);
		release();
	}
}

bool_t SalOp::is_external_body(belle_sip_header_content_type_t* content_type) {
	return strcmp("message",belle_sip_header_content_type_get_type(content_type))==0
			&&	strcmp("external-body",belle_sip_header_content_type_get_subtype(content_type))==0;
}

int SalOp::reply_message(SalReason reason) {
	if (this->pending_server_trans){
		int code=to_sip_code(reason);
		belle_sip_response_t *resp = belle_sip_response_create_from_request(
			belle_sip_transaction_get_request((belle_sip_transaction_t*)this->pending_server_trans),code);
		belle_sip_server_transaction_send_response(this->pending_server_trans,resp);
		return 0;
	}else ms_error("sal_message_reply(): no server transaction");
	return -1;
}

void SalOp::add_message_accept(belle_sip_message_t *msg) {
	bctbx_list_t *item;
	const char *str;
	char *old;
	char *header = ms_strdup("xml/cipher, application/cipher.vnd.gsma.rcs-ft-http+xml");

	for (item = this->root->supported_content_types; item != NULL; item = bctbx_list_next(item)) {
		str = (const char *)bctbx_list_get_data(item);
		old = header;
		header = ms_strdup_printf("%s, %s", old, str);
		ms_free(old);
	}

	belle_sip_message_add_header(msg, belle_sip_header_create("Accept", header));
	ms_free(header);
}

void SalOp::set_service_route(const SalAddress* service_route) {
	if (this->service_route)
		sal_address_destroy(this->service_route);

	this->service_route=service_route?sal_address_clone(service_route):NULL;
}

LINPHONE_END_NAMESPACE
