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

#include "c-wrapper/internal/c-tools.h"
#include "sal/op.h"
#include "bellesip_sal/sal_impl.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

SalOp::SalOp(Sal *sal) {
	mRoot = sal;
	mSdpHandling = sal->mDefaultSdpHandling;
	memset(&mErrorInfo, 0, sizeof(mErrorInfo));
	memset(&mReasonErrorInfo, 0, sizeof(mReasonErrorInfo));
	ref();
}

SalOp::~SalOp() {
	lInfo() << "Destroying op [" << this << "] of type [" << toString(mType) << "]";
	
	if (mPendingAuthTransaction) belle_sip_object_unref(mPendingAuthTransaction);
	mRoot->removePendingAuth(this);
	if (mAuthInfo) {
		sal_auth_info_delete(mAuthInfo);
	}
	if (mSdpAnswer) belle_sip_object_unref(mSdpAnswer);
	if (mRefresher) {
		belle_sip_object_unref(mRefresher);
		mRefresher=NULL;
	}
	if (mResult)
		sal_media_description_unref(mResult);
	if(mReplaces) belle_sip_object_unref(mReplaces);
	if(mReferredBy) belle_sip_object_unref(mReferredBy);

	if (mPendingClientTransaction) belle_sip_object_unref(mPendingClientTransaction);
	if (mPendingServerTransaction) belle_sip_object_unref(mPendingServerTransaction);
	if (mPendingUpdateServerTransaction) belle_sip_object_unref(mPendingUpdateServerTransaction);
	if (mEvent) belle_sip_object_unref(mEvent);
	
	sal_error_info_reset(&mErrorInfo);
	if (mFromAddress){
		sal_address_destroy(mFromAddress);
		mFromAddress=NULL;
	}
	if (mToAddress){
		sal_address_destroy(mToAddress);
		mToAddress=NULL;
	}

	if (mServiceRoute){
		sal_address_destroy(mServiceRoute);
		mServiceRoute=NULL;
	}

	if (mOriginAddress){
		sal_address_destroy(mOriginAddress);
		mOriginAddress=NULL;
	}

	if (mContactAddress) {
		sal_address_destroy(mContactAddress);
	}
	if (mRemoteContactAddress){
		sal_address_destroy(mRemoteContactAddress);
	}
	if (mServiceRoute) {
		sal_address_destroy(mServiceRoute);
	}
	for (auto &addr : mRouteAddresses)
		sal_address_unref(addr);
	if (mRecvCustomHeaders)
		sal_custom_header_free(mRecvCustomHeaders);
	if (mSentCustomHeaders)
		sal_custom_header_free(mSentCustomHeaders);
}

SalOp *SalOp::ref() {
	   mRef++;
	return this;
}

void *SalOp::unref() {
	   mRef--;
	if (mRef==0) {
		delete this;
	} else if (mRef<0) {
		ms_fatal("SalOp [%p]: too many unrefs.",this);
	}
	return NULL;
}

void SalOp::setContactAddress(const SalAddress *address) {
	if (mContactAddress) sal_address_destroy(mContactAddress);
	mContactAddress=address?sal_address_clone(address):NULL;
}

void SalOp::assignAddress (SalAddress **address, const string &value) {
	if (*address) {
		sal_address_destroy(*address);
		*address = nullptr;
	}
	if (!value.empty())
		*address = sal_address_new(value.c_str());
}

void SalOp::setRoute (const string &value) {
	for (auto &address : mRouteAddresses)
		sal_address_unref(address);
	mRouteAddresses.clear();
	if (value.empty()) {
		mRoute.clear();
	} else {
		auto address = sal_address_new(value.c_str());
		mRouteAddresses.push_back(address);
		char *routeStr = sal_address_as_string(address);
		mRoute = routeStr;
		ms_free(routeStr);
	}
}

void SalOp::setRouteAddress(const SalAddress *address){
	char* address_string=sal_address_as_string(address); /*can probably be optimized*/
	   setRoute(address_string);
	ms_free(address_string);
}

void SalOp::addRouteAddress (const SalAddress *address) {
	if (mRouteAddresses.empty())
		setRouteAddress(address);
	else
		mRouteAddresses.push_back(sal_address_clone(address));
}

void SalOp::setFrom (const string &value) {
	assignAddress(&mFromAddress, value);
	if (mFromAddress) {
		char *valueStr = sal_address_as_string(mFromAddress);
		mFrom = valueStr;
		ms_free(valueStr);
	} else {
		mFrom.clear();
	}
}

void SalOp::setFromAddress(const SalAddress *from) {
	char* address_string=sal_address_as_string(from); /*can probably be optimized*/
	   setFrom(address_string);
	ms_free(address_string);
}

void SalOp::setTo (const string &value) {
	assignAddress(&mToAddress, value);
	if (mToAddress) {
		char *valueStr = sal_address_as_string(mToAddress);
		mTo = valueStr;
		ms_free(valueStr);
	} else {
		mTo.clear();
	}
}

void SalOp::setToAddress(const SalAddress *to) {
	char* address_string=sal_address_as_string(to); /*can probably be optimized*/
	   setTo(address_string);
	ms_free(address_string);
}

void SalOp::setDiversionAddress(const SalAddress *diversion) {
	if (mDiversionAddress) sal_address_destroy(mDiversionAddress);
	mDiversionAddress=diversion ? sal_address_clone(diversion) : NULL;
}

int SalOp::refresh() {
	if (mRefresher) {
		belle_sip_refresher_refresh(mRefresher,belle_sip_refresher_get_expires(mRefresher));
		return 0;
	}
	lWarning() << "No refresher on op [" << this << "] of type [" << toString(mType) << "]";
	return -1;
}

void SalOp::killDialog() {
	ms_warning("op [%p]: force kill of dialog [%p]", this, mDialog);
	belle_sip_dialog_delete(mDialog);
}

void SalOp::release() {
	/*if in terminating state, keep this state because it means we are waiting for a response to be able to terminate the operation.*/
	if (mState!=State::Terminating)
		mState=State::Terminated;
	   setUserPointer(NULL);/*mandatory because releasing op doesn't not mean freeing op. Make sure back pointer will not be used later*/
	if (mReleaseCb)
		mReleaseCb(this);
	if (mRefresher) {
		belle_sip_refresher_stop(mRefresher);
	}
	mOpReleased = TRUE;
	unref();
}

int SalOp::sendRequestWithContact(belle_sip_request_t* request, bool add_contact) {
	belle_sip_client_transaction_t* client_transaction;
	belle_sip_provider_t* prov=mRoot->mProvider;
	belle_sip_header_contact_t* contact;
	int result =-1;
	belle_sip_uri_t *next_hop_uri=NULL;

	if (add_contact && !belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(request),belle_sip_header_contact_t)) {
		contact = createContact();
		belle_sip_message_set_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(contact));
	} /*keep existing*/

	   addCustomHeaders((belle_sip_message_t*)request);

	if (!mDialog || belle_sip_dialog_get_state(mDialog) == BELLE_SIP_DIALOG_NULL) {
		/*don't put route header if  dialog is in confirmed state*/
		auto routeAddresses = getRouteAddresses();
		const char *transport;
		const char *method=belle_sip_request_get_method(request);
		belle_sip_listening_point_t *udplp=belle_sip_provider_get_listening_point(prov,"UDP");

		if (routeAddresses.empty())
			next_hop_uri = (belle_sip_uri_t*)belle_sip_object_clone((belle_sip_object_t*)belle_sip_request_get_uri(request));
		else
			next_hop_uri = belle_sip_header_address_get_uri((belle_sip_header_address_t*)routeAddresses.front());
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
	if (mPendingClientTransaction) belle_sip_object_unref(mPendingClientTransaction);
	
	mPendingClientTransaction=client_transaction; /*update pending inv for being able to cancel*/
	belle_sip_object_ref(mPendingClientTransaction);

	if (belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(request),belle_sip_header_user_agent_t)==NULL)
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(mRoot->mUserAgentHeader));

	if (!belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_AUTHORIZATION)
		&& !belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_PROXY_AUTHORIZATION)) {
		/*hmm just in case we already have authentication param in cache*/
		belle_sip_provider_add_authorization(mRoot->mProvider,request,NULL,NULL,NULL,L_STRING_TO_C(mRealm));
	}
	result = belle_sip_client_transaction_send_request_to(client_transaction,next_hop_uri/*might be null*/);

	/*update call id if not set yet for this OP*/
	if (result == 0 && mCallId.empty()) {
		mCallId = belle_sip_header_call_id_get_call_id(
			BELLE_SIP_HEADER_CALL_ID(belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(request), belle_sip_header_call_id_t)));
	}

	return result;

}

int SalOp::sendRequest(belle_sip_request_t* request) {
	bool need_contact=FALSE;
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
		need_contact=true;

	return sendRequestWithContact(request,need_contact);
}

void SalOp::resendRequest(belle_sip_request_t* request) {
	belle_sip_header_cseq_t* cseq=(belle_sip_header_cseq_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_CSEQ);
	belle_sip_header_cseq_set_seq_number(cseq,belle_sip_header_cseq_get_seq_number(cseq)+1);
	   sendRequest(request);
}

int SalOp::processRedirect(){
	belle_sip_request_t* request = belle_sip_transaction_get_request((belle_sip_transaction_t*)mPendingClientTransaction);
	belle_sip_response_t *response = belle_sip_transaction_get_response((belle_sip_transaction_t*)mPendingClientTransaction);
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
	
	if (mDialog && belle_sip_dialog_get_state(mDialog)==BELLE_SIP_DIALOG_CONFIRMED){
		ms_warning("Redirect not handled within established dialogs. Does it make sense ?");
		return -1;
	}
	   setOrUpdateDialog(NULL);
	belle_sip_message_remove_header_from_ptr((belle_sip_message_t*)request, (belle_sip_header_t*)callid);
	belle_sip_message_add_header((belle_sip_message_t*)request, (belle_sip_header_t*)(callid = belle_sip_provider_create_call_id(getSal()->mProvider)));
	mCallId.clear(); // Reset the call-id of op, it will be set when new request will be sent
	belle_sip_request_set_uri(request, redirect_uri);
	redirect_uri = BELLE_SIP_URI(belle_sip_object_clone(BELLE_SIP_OBJECT(redirect_uri)));
	belle_sip_uri_set_port(redirect_uri, 0);
	belle_sip_parameters_remove_parameter(BELLE_SIP_PARAMETERS(redirect_uri), "transport");
	belle_sip_header_address_set_uri((belle_sip_header_address_t*)to, redirect_uri);
	   sendRequest(request);
	return 0;
}

void SalOp::processAuthentication() {
	belle_sip_request_t* initial_request=belle_sip_transaction_get_request((belle_sip_transaction_t*)mPendingAuthTransaction);
	belle_sip_request_t* new_request;
	bool is_within_dialog=false;
	belle_sip_list_t* auth_list=NULL;
	belle_sip_auth_event_t* auth_event;
	belle_sip_response_t *response=belle_sip_transaction_get_response((belle_sip_transaction_t*)mPendingAuthTransaction);
	belle_sip_header_from_t *from=belle_sip_message_get_header_by_type(initial_request,belle_sip_header_from_t);
	belle_sip_uri_t *from_uri=belle_sip_header_address_get_uri((belle_sip_header_address_t*)from);

	if (strcasecmp(belle_sip_uri_get_host(from_uri),"anonymous.invalid")==0){
		/*prefer using the from from the SalOp*/
		from_uri=belle_sip_header_address_get_uri((belle_sip_header_address_t*)getFromAddress());
	}

	if (mDialog && belle_sip_dialog_get_state(mDialog)==BELLE_SIP_DIALOG_CONFIRMED) {
		new_request = belle_sip_dialog_create_request_from(mDialog,initial_request);
		if (!new_request)
			new_request = belle_sip_dialog_create_queued_request_from(mDialog,initial_request);
		is_within_dialog=true;
	} else {
		new_request=initial_request;
		belle_sip_message_remove_header(BELLE_SIP_MESSAGE(new_request),BELLE_SIP_AUTHORIZATION);
		belle_sip_message_remove_header(BELLE_SIP_MESSAGE(new_request),BELLE_SIP_PROXY_AUTHORIZATION);
	}
	if (new_request==NULL) {
		ms_error("sal_process_authentication() op=[%p] cannot obtain new request from dialog.",this);
		return;
	}

	if (belle_sip_provider_add_authorization(mRoot->mProvider,new_request,response,from_uri,&auth_list,L_STRING_TO_C(mRealm))) {
		if (is_within_dialog)
			sendRequest(new_request);
		else
			resendRequest(new_request);
		mRoot->removePendingAuth(this);
	}else {
		belle_sip_header_from_t *from=belle_sip_message_get_header_by_type(response,belle_sip_header_from_t);
		char *tmp=belle_sip_object_to_string(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(from)));
		ms_message("No auth info found for [%s]",tmp);
		belle_sip_free(tmp);
		mRoot->addPendingAuth(this);

		if (is_within_dialog) {
			belle_sip_object_unref(new_request);
		}
	}
	/*always store auth info, for case of wrong credential*/
	if (mAuthInfo) {
		sal_auth_info_delete(mAuthInfo);
		mAuthInfo=NULL;
	}
	if (auth_list){
		auth_event=(belle_sip_auth_event_t*)(auth_list->data);
		mAuthInfo=sal_auth_info_create(auth_event);
		belle_sip_list_free_with_data(auth_list,(void (*)(void*))belle_sip_auth_event_destroy);
	}
}

string SalOp::getDialogId () const {
	if (!mDialog)
		return string();
	stringstream ss;
	ss << mCallId << ";to-tag=" << belle_sip_dialog_get_remote_tag(mDialog) << ";from-tag=" << belle_sip_dialog_get_local_tag(mDialog);
	return ss.str();
}

int SalOp::getAddressFamily() const {
	belle_sip_transaction_t *tr=NULL;
	belle_sip_header_address_t *contact;
	

	if (mRefresher)
		tr=(belle_sip_transaction_t *)belle_sip_refresher_get_transaction(mRefresher);

	if (tr==NULL)
		tr=(belle_sip_transaction_t *)mPendingClientTransaction;
	if (tr==NULL)
		tr=(belle_sip_transaction_t *)mPendingServerTransaction;
	
	if (tr==NULL){
		ms_error("Unable to determine IP version from signaling operation.");
		return AF_UNSPEC;
	}
	
	
	if (mRefresher) {
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

bool SalOp::isIdle() const {
	if (mDialog)
		return !belle_sip_dialog_request_pending(mDialog);
	return true;
}

void SalOp::setEvent (const string &eventName) {
	belle_sip_header_event_t *header = nullptr;
	if (mEvent)
		belle_sip_object_unref(mEvent);
	if (!eventName.empty()) {
		header = belle_sip_header_event_create(eventName.c_str());
		belle_sip_object_ref(header);
	}
	mEvent = header;
}

void SalOp::addInitialRouteSet (belle_sip_request_t *request, const list<SalAddress *> &routeAddresses) {
	bool uniqueRoute = routeAddresses.size() == 1;
	for (const auto &address : routeAddresses) {
		// Optimization: if the initial route set only contains one URI which is the same as the request URI, ommit it
		if (uniqueRoute) {
			belle_sip_uri_t *requestUri = belle_sip_request_get_uri(request);
			// Skip the first route it is the same as the request uri
			if (strcmp(sal_address_get_domain(address), belle_sip_uri_get_host(requestUri)) == 0) {
				ms_message("Skipping top route of initial route-set because same as request-uri");
				continue;
			}
		}

		belle_sip_header_route_t *route = belle_sip_header_route_create((belle_sip_header_address_t *)address);
		belle_sip_uri_t *uri = belle_sip_header_address_get_uri((belle_sip_header_address_t *)route);
		belle_sip_uri_set_lr_param(uri, 1);
		belle_sip_message_add_header((belle_sip_message_t *)request, (belle_sip_header_t *)route);
	}
}

belle_sip_request_t* SalOp::buildRequest (const string &method) {
	belle_sip_header_from_t* from_header;
	belle_sip_header_to_t* to_header;
	belle_sip_provider_t* prov=mRoot->mProvider;
	belle_sip_request_t *req;
	belle_sip_uri_t* req_uri;
	belle_sip_uri_t* to_uri;
	belle_sip_header_call_id_t *call_id_header;

	const SalAddress* to_address;
	char token[10];

	/* check that the op has a correct to address */
	to_address = getToAddress();
	if( to_address == NULL ){
		ms_error("No To: address, cannot build request");
		return NULL;
	}

	to_uri = belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(to_address));
	if( to_uri == NULL ){
		ms_error("To: address is invalid, cannot build request");
		return NULL;
	}

	if ((method == "REGISTER") || (mPrivacy == SalPrivacyNone)) {
		from_header = belle_sip_header_from_create(BELLE_SIP_HEADER_ADDRESS(getFromAddress())
						,belle_sip_random_token(token,sizeof(token)));
	} else {
		from_header=belle_sip_header_from_create2("Anonymous <sip:anonymous@anonymous.invalid>",belle_sip_random_token(token,sizeof(token)));
	}
	/*make sure to preserve components like headers or port*/

	req_uri = (belle_sip_uri_t*)belle_sip_object_clone((belle_sip_object_t*)to_uri);
	belle_sip_uri_set_secure(req_uri,isSecure());

	to_header = belle_sip_header_to_create(BELLE_SIP_HEADER_ADDRESS(to_address),NULL);
	call_id_header = belle_sip_provider_create_call_id(prov);
	if (!mCallId.empty())
		belle_sip_header_call_id_set_call_id(call_id_header, mCallId.c_str());

	req=belle_sip_request_create(
					req_uri,
					method.c_str(),
					call_id_header,
					belle_sip_header_cseq_create(20,method.c_str()),
					from_header,
					to_header,
					belle_sip_header_via_new(),
					70);

	if (mPrivacy & SalPrivacyId) {
		belle_sip_header_p_preferred_identity_t* p_preferred_identity=belle_sip_header_p_preferred_identity_create(BELLE_SIP_HEADER_ADDRESS(getFromAddress()));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(p_preferred_identity));
	}

	auto routeAddresses = getRouteAddresses();
	if (!routeAddresses.empty() && (method != "REGISTER") && !mRoot->mNoInitialRoute)
		addInitialRouteSet(req, routeAddresses);

	if ((method != "REGISTER") && (mPrivacy != SalPrivacyNone)) {
		belle_sip_header_privacy_t* privacy_header=belle_sip_header_privacy_new();
		if (mPrivacy&SalPrivacyCritical)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyCritical));
		if (mPrivacy&SalPrivacyHeader)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyHeader));
		if (mPrivacy&SalPrivacyId)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyId));
		if (mPrivacy&SalPrivacyNone)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyNone));
		if (mPrivacy&SalPrivacySession)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacySession));
		if (mPrivacy&SalPrivacyUser)
			belle_sip_header_privacy_add_privacy(privacy_header,sal_privacy_to_string(SalPrivacyUser));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(privacy_header));
	}
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),mRoot->mSupportedHeader);
	return req;
}

void SalOp::setErrorInfoFromResponse(belle_sip_response_t *response) {
	int code = belle_sip_response_get_status_code(response);
	const char *reason_phrase=belle_sip_response_get_reason_phrase(response);
	belle_sip_header_t *warning=belle_sip_message_get_header(BELLE_SIP_MESSAGE(response),"Warning");
	SalErrorInfo *ei=&mErrorInfo;
	const char *warnings;

	warnings=warning ? belle_sip_header_get_unparsed_value(warning) : NULL;
	sal_error_info_set(ei,SalReasonUnknown,"SIP", code,reason_phrase,warnings);
	   setReasonErrorInfo(BELLE_SIP_MESSAGE(response));
}

string SalOp::toString (const State value) {
	switch (value) {
		case State::Early:
			return"SalOpStateEarly";
		case State::Active:
			return "SalOpStateActive";
		case State::Terminating:
			return "SalOpStateTerminating";
		case State::Terminated:
			return "SalOpStateTerminated";
		default:
			return "Unknown";
	}
}

void SalOp::setReasonErrorInfo(belle_sip_message_t *msg) {
	belle_sip_header_reason_t* reason_header = belle_sip_message_get_header_by_type(msg,belle_sip_header_reason_t);
	if (reason_header){
		SalErrorInfo *ei=&mReasonErrorInfo; // ?//
		const char *protocol = belle_sip_header_reason_get_protocol(reason_header);
		int code = belle_sip_header_reason_get_cause(reason_header);
		const char *text = belle_sip_header_reason_get_text(reason_header);
		sal_error_info_set(ei, SalReasonUnknown, protocol, code, text, NULL);
	}
}

void SalOp::setReferredBy(belle_sip_header_referred_by_t* referred_by) {
	if (mReferredBy){
		belle_sip_object_unref(mReferredBy);
	}
	
	mReferredBy=referred_by;
	belle_sip_object_ref(mReferredBy);
}

void SalOp::setReplaces(belle_sip_header_replaces_t* replaces) {
	if (mReplaces){
		belle_sip_object_unref(mReplaces);
	}
	
	mReplaces=replaces;
	belle_sip_object_ref(mReplaces);
}

int SalOp::sendRequestWithExpires(belle_sip_request_t* request,int expires) {
	belle_sip_header_expires_t* expires_header=(belle_sip_header_expires_t*)belle_sip_message_get_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_EXPIRES);

	if (!expires_header && expires>=0) {
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(request),BELLE_SIP_HEADER(expires_header=belle_sip_header_expires_new()));
	}
	if (expires_header) belle_sip_header_expires_set_expires(expires_header,expires);
	return sendRequest(request);
}

int SalOp::sendRequestAndCreateRefresher(belle_sip_request_t* req, int expires,belle_sip_refresher_listener_t listener) {
	if (sendRequestWithExpires(req,expires)==0) {
		if (mRefresher) {
			belle_sip_refresher_stop(mRefresher);
			belle_sip_object_unref(mRefresher);
		}
		if ((mRefresher = belle_sip_client_transaction_create_refresher(mPendingClientTransaction))) {
			/*since refresher acquires the transaction, we should remove our context from the transaction, because we won't be notified
			 * that it is terminated anymore.*/
			unref();/*loose the reference that was given to the transaction when creating it*/
			/* Note that the refresher will replace our data with belle_sip_transaction_set_application_data().
			 Something in the design is not very good here, it makes things complicated to the belle-sip user.
			 Possible ideas to improve things: refresher shall not use belle_sip_transaction_set_application_data() internally, refresher should let the first transaction
			 notify the user as a normal transaction*/
			belle_sip_refresher_set_listener(mRefresher,listener, this);
			belle_sip_refresher_set_retry_after(mRefresher,mRoot->mRefresherRetryAfter);
			belle_sip_refresher_set_realm(mRefresher,L_STRING_TO_C(mRealm));
			belle_sip_refresher_enable_manual_mode(mRefresher, mManualRefresher);
			return 0;
		} else {
			return -1;
		}
	}
	return -1;
}

belle_sip_header_contact_t *SalOp::createContact() {
	belle_sip_header_contact_t* contact_header;
	belle_sip_uri_t* contact_uri;

	if (getContactAddress()) {
		contact_header = belle_sip_header_contact_create(BELLE_SIP_HEADER_ADDRESS(getContactAddress()));
	} else {
		contact_header= belle_sip_header_contact_new();
	}

	if (!(contact_uri=belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(contact_header)))) {
		/*no uri, just creating a new one*/
		contact_uri=belle_sip_uri_new();
		belle_sip_header_address_set_uri(BELLE_SIP_HEADER_ADDRESS(contact_header),contact_uri);
	}

	belle_sip_uri_set_user_password(contact_uri,NULL);
	belle_sip_uri_set_secure(contact_uri,isSecure());
	if (mPrivacy!=SalPrivacyNone){
		belle_sip_uri_set_user(contact_uri,NULL);
	}
	/*don't touch contact in case of gruu*/
	if (!belle_sip_parameters_has_parameter(BELLE_SIP_PARAMETERS(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(contact_header))),"gr")) {
		belle_sip_header_contact_set_automatic(contact_header,mRoot->mAutoContacts);
		if (!mRoot->mUuid.empty()
			&& !belle_sip_parameters_has_parameter(BELLE_SIP_PARAMETERS(contact_header), "+sip.instance")
		) {
			stringstream ss;
			ss << "\"<urn:uuid:" << mRoot->mUuid << ">\"";
			string instanceId = ss.str();
			belle_sip_parameters_set_parameter(BELLE_SIP_PARAMETERS(contact_header), "+sip.instance", instanceId.c_str());
		}
	}
	if (!mRoot->mLinphoneSpecs.empty()
		&& !belle_sip_parameters_has_parameter(BELLE_SIP_PARAMETERS(contact_header), "+org.linphone.specs")
	) {
		belle_sip_parameters_set_parameter(BELLE_SIP_PARAMETERS(contact_header), "+org.linphone.specs", mRoot->mLinphoneSpecs.c_str());
	}
	return contact_header;
}

void SalOp::unlinkOpFromDialog(belle_sip_dialog_t* dialog) {
	belle_sip_dialog_set_application_data(dialog,NULL);
	unref();
	belle_sip_object_unref(dialog);
}

belle_sip_dialog_t *SalOp::linkOpWithDialog(belle_sip_dialog_t* dialog) {
	belle_sip_dialog_set_application_data(dialog,ref());
	belle_sip_object_ref(dialog);
	return dialog;
}

void SalOp::setOrUpdateDialog(belle_sip_dialog_t* dialog) {
	ms_message("op [%p] : set_or_update_dialog() current=[%p] new=[%p]", this, mDialog,dialog);
	ref();
	if (mDialog!=dialog){
		if (mDialog){
			/*FIXME: shouldn't we delete unconfirmed dialogs ?*/
			         unlinkOpFromDialog(mDialog);
			mDialog=NULL;
		}
		if (dialog) {
			mDialog=linkOpWithDialog(dialog);
			belle_sip_dialog_enable_pending_trans_checking(dialog,mRoot->mPendingTransactionChecking);
		}
	}
	unref();
}

int SalOp::ping (const string &from, const string &to) {
	setFrom(from);
	setTo(to);
	return sendRequest(buildRequest("OPTIONS"));
}

int SalOp::sendInfo (const SalBodyHandler *bodyHandler) {
	if (mDialog && (belle_sip_dialog_get_state(mDialog) == BELLE_SIP_DIALOG_CONFIRMED)) {
		belle_sip_dialog_enable_pending_trans_checking(mDialog, mRoot->mPendingTransactionChecking);
		belle_sip_request_t *request = belle_sip_dialog_create_queued_request(mDialog, "INFO");
		belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(request), BELLE_SIP_BODY_HANDLER(bodyHandler));
		return sendRequest(request);
	} else {
		lError() << "Cannot send INFO message on op [" << this << "] because dialog is not in confirmed state yet";
	}
	return -1;
}

SalBodyHandler *SalOp::getBodyHandler(belle_sip_message_t *msg) {
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

void SalOp::assignRecvHeaders(belle_sip_message_t *incoming) {
	if (incoming) belle_sip_object_ref(incoming);
	if (mRecvCustomHeaders){
		belle_sip_object_unref(mRecvCustomHeaders);
		mRecvCustomHeaders=NULL;
	}
	if (incoming){
		mRecvCustomHeaders=(SalCustomHeader*)incoming;
	}
}

void SalOp::setRemoteContact (const string &value) {
	assignAddress(&mRemoteContactAddress, value);
	mRemoteContact = value; // To preserve header params
}

void SalOp::setNetworkOrigin (const string &value) {
	assignAddress(&mOriginAddress, value);
	if (mOriginAddress) {
		char *valueStr = sal_address_as_string(mOriginAddress);
		mOrigin = valueStr;
		ms_free(valueStr);
	} else {
		mOrigin.clear();
	}
}

void SalOp::setNetworkOriginAddress (SalAddress *value) {
	char *valueStr = sal_address_as_string(value); // Can probably be optimized
	setNetworkOrigin(valueStr);
	ms_free(valueStr);
}

/*
rfc3323
4.2 Expressing Privacy Preferences
When a Privacy header is constructed, it MUST consist of either the
   value 'none', or one or more of the values 'user', 'header' and
   'session' (each of which MUST appear at most once) which MAY in turn
   be followed by the 'critical' indicator.
 */
void SalOp::setPrivacyFromMessage(belle_sip_message_t* msg) {
	belle_sip_header_privacy_t* privacy = belle_sip_message_get_header_by_type(msg,belle_sip_header_privacy_t);
	if (!privacy) {
		      setPrivacy(SalPrivacyNone);
	} else {
		belle_sip_list_t* privacy_list=belle_sip_header_privacy_get_privacy(privacy);
		      setPrivacy(0);
		for (;privacy_list!=NULL;privacy_list=privacy_list->next) {
			char* privacy_value=(char*)privacy_list->data;
			if(strcmp(sal_privacy_to_string(SalPrivacyCritical),privacy_value) == 0)
				            setPrivacy(getPrivacy()|SalPrivacyCritical);
			if(strcmp(sal_privacy_to_string(SalPrivacyHeader),privacy_value) == 0)
							         setPrivacy(getPrivacy()|SalPrivacyHeader);
			if(strcmp(sal_privacy_to_string(SalPrivacyId),privacy_value) == 0)
							         setPrivacy(getPrivacy()|SalPrivacyId);
			if(strcmp(sal_privacy_to_string(SalPrivacyNone),privacy_value) == 0) {
				            setPrivacy(SalPrivacyNone);
				break;
			}
			if(strcmp(sal_privacy_to_string(SalPrivacySession),privacy_value) == 0)
							         setPrivacy(getPrivacy()|SalPrivacySession);
			if(strcmp(sal_privacy_to_string(SalPrivacyUser),privacy_value) == 0)
										      setPrivacy(getPrivacy()|SalPrivacyUser);
		}
	}
}

void SalOp::setRemoteUserAgent (belle_sip_message_t *message) {
	belle_sip_header_user_agent_t *userAgentHeader = belle_sip_message_get_header_by_type(message, belle_sip_header_user_agent_t);
	char userAgentStr[256];
	if (userAgentHeader
		&& belle_sip_header_user_agent_get_products_as_string(userAgentHeader, userAgentStr, sizeof(userAgentStr)) > 0
	) {
		mRemoteUserAgent = userAgentStr;
	}
}

string SalOp::toString (const Type type) {
	switch (type) {
		case Type::Register:
			return "SalOpRegister";
		case Type::Call:
			return "SalOpCall";
		case Type::Message:
			return "SalOpMessage";
		case Type::Presence:
			return "SalOpPresence";
		default:
			return "SalOpUnknown";
	}
}

bool SalOp::isSecure() const {
	const SalAddress* from = getFromAddress();
	const SalAddress* to = getToAddress();
	return from && to && strcasecmp("sips",sal_address_get_scheme(from))==0 && strcasecmp("sips",sal_address_get_scheme(to))==0;
}

/*
 * Warning: this function takes owneship of the custom headers
 */
void SalOp::setSentCustomHeaders(SalCustomHeader* ch){
	if (mSentCustomHeaders){
		sal_custom_header_free(mSentCustomHeaders);
		mSentCustomHeaders=NULL;
	}
	if (ch) belle_sip_object_ref((belle_sip_message_t*)ch);
	mSentCustomHeaders=ch;
}

void SalOp::addHeaders(belle_sip_header_t *h, belle_sip_message_t *msg){

	if (BELLE_SIP_OBJECT_IS_INSTANCE_OF(h,belle_sip_header_contact_t)){
		belle_sip_header_contact_t* newct;
		/*special case for contact, we want to keep everything from the custom contact but set automatic mode and add our own parameters as well*/
		      setContactAddress((SalAddress*)BELLE_SIP_HEADER_ADDRESS(h));
		newct = createContact();
		belle_sip_message_set_header(BELLE_SIP_MESSAGE(msg),BELLE_SIP_HEADER(newct));
		return;
	}
	/*if a header already exists in the message, replace it*/
	belle_sip_message_set_header(msg,h);

}

void SalOp::addCustomHeaders(belle_sip_message_t *msg){
	if (mSentCustomHeaders){
		belle_sip_message_t *ch=(belle_sip_message_t*)mSentCustomHeaders;
		belle_sip_list_t *l=belle_sip_message_get_all_headers(ch);
		belle_sip_list_t *elem;
		for(elem=l;elem!=NULL;elem=elem->next){
			         addHeaders((belle_sip_header_t*)elem->data,msg);
		}
		belle_sip_list_free(l);
	}
}

int SalOp::unsubscribe(){
	if (mRefresher){
		const belle_sip_transaction_t *tr=(const belle_sip_transaction_t*) belle_sip_refresher_get_transaction(mRefresher);
		belle_sip_request_t *last_req=belle_sip_transaction_get_request(tr);
		belle_sip_message_set_body(BELLE_SIP_MESSAGE(last_req), NULL, 0);
		belle_sip_refresher_refresh(mRefresher,0);
		return 0;
	}
	return -1;
}

void SalOp::processIncomingMessage(const belle_sip_request_event_t *event) {
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_server_transaction_t* server_transaction = belle_sip_provider_create_server_transaction(mRoot->mProvider,req);
	belle_sip_header_address_t* address;
	belle_sip_header_from_t* from_header;
	belle_sip_header_content_type_t* content_type;
	belle_sip_response_t* resp;
	int errcode = 500;
	belle_sip_header_call_id_t* call_id = belle_sip_message_get_header_by_type(req,belle_sip_header_call_id_t);
	belle_sip_header_cseq_t* cseq = belle_sip_message_get_header_by_type(req,belle_sip_header_cseq_t);
	belle_sip_header_date_t *date=belle_sip_message_get_header_by_type(req,belle_sip_header_date_t);
	char* from;
	bool external_body = false;

	from_header=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_from_t);
	content_type=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_content_type_t);
	
	if (content_type) {
		SalMessage salmsg;
		char message_id[256]={0};

		if (mPendingServerTransaction) belle_sip_object_unref(mPendingServerTransaction);
		
		mPendingServerTransaction=server_transaction;
		belle_sip_object_ref(mPendingServerTransaction);

		external_body=isExternalBody(content_type);
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
		mRoot->mCallbacks.message_received(this,&salmsg);

		belle_sip_object_unref(address);
		belle_sip_free(from);
		if (salmsg.url) ms_free((char*)salmsg.url);
		ms_free((char *)salmsg.content_type);
	} else {
		ms_error("Unsupported MESSAGE (no Content-Type)");
		resp = belle_sip_response_create_from_request(req, errcode);
		      addMessageAccept((belle_sip_message_t*)resp);
		belle_sip_server_transaction_send_response(server_transaction,resp);
		release();
	}
}

bool SalOp::isExternalBody(belle_sip_header_content_type_t* content_type) {
	return strcmp("message",belle_sip_header_content_type_get_type(content_type))==0
			&&	strcmp("external-body",belle_sip_header_content_type_get_subtype(content_type))==0;
}

int SalOp::replyMessage(SalReason reason) {
	if (mPendingServerTransaction){
		int code=to_sip_code(reason);
		belle_sip_response_t *resp = belle_sip_response_create_from_request(
			belle_sip_transaction_get_request((belle_sip_transaction_t*)mPendingServerTransaction),code);
		belle_sip_server_transaction_send_response(mPendingServerTransaction,resp);
		return 0;
	}else ms_error("sal_message_reply(): no server transaction");
	return -1;
}

void SalOp::addMessageAccept (belle_sip_message_t *message) {
	stringstream ss;
	ss << "xml/cipher, application/cipher.vnd.gsma.rcs-ft-http+xml";
	for (const auto &supportedContentType : mRoot->mSupportedContentTypes)
		ss << ", " << supportedContentType;
	string headerValue = ss.str();
	belle_sip_message_add_header(message, belle_sip_header_create("Accept", headerValue.c_str()));
}

void SalOp::setServiceRoute(const SalAddress* service_route) {
	if (mServiceRoute)
		sal_address_destroy(mServiceRoute);

	mServiceRoute=service_route?sal_address_clone(service_route):NULL;
}

LINPHONE_END_NAMESPACE
