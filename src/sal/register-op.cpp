/*
 * register-op.cpp
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

#include "sal/register-op.h"
#include "bellesip_sal/sal_impl.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

int SalRegisterOp::sendRegister(const char *proxy, const char *from, int expires, const SalAddress* old_contact) {
	belle_sip_request_t *req;
	belle_sip_uri_t* req_uri;
	belle_sip_header_t* accept_header;

	if (mRefresher){
		belle_sip_refresher_stop(mRefresher);
		belle_sip_object_unref(mRefresher);
		mRefresher=NULL;
	}

	mType=Type::Register;
	setFrom(from);
	setTo(from);
	setRoute(proxy);
	req = buildRequest("REGISTER");
	req_uri = belle_sip_request_get_uri(req);
	belle_sip_uri_set_user(req_uri,NULL); /*remove userinfo if any*/
	if (mRoot->mUseDates) {
		time_t curtime=time(NULL);
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(belle_sip_header_date_create_from_time(&curtime)));
	}
	accept_header = belle_sip_header_create("Accept", "application/sdp, text/plain, application/vnd.gsma.rcs-ft-http+xml");
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req), accept_header);
	belle_sip_message_set_header(BELLE_SIP_MESSAGE(req),(belle_sip_header_t*)createContact());
	if (old_contact) {
		belle_sip_header_contact_t *contact=belle_sip_header_contact_create((const belle_sip_header_address_t *)old_contact);
		if (contact) {
			char * tmp;
			belle_sip_header_contact_set_expires(contact,0); /*remove old aor*/
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(req), BELLE_SIP_HEADER(contact));
			tmp = belle_sip_object_to_string(contact);
			ms_message("Clearing contact [%s] for op [%p]",tmp,this);
			ms_free(tmp);
		} else {
			ms_error("Cannot add old contact header to op [%p]",this);
		}
	}
	return sendRequestAndCreateRefresher(req,expires,registerRefresherListener);
}

void SalRegisterOp::registerRefresherListener(belle_sip_refresher_t* refresher, void* user_pointer, unsigned int status_code, const char* reason_phrase, int will_retry) {
	SalRegisterOp * op = (SalRegisterOp *)user_pointer;
	belle_sip_response_t* response=belle_sip_transaction_get_response(BELLE_SIP_TRANSACTION(belle_sip_refresher_get_transaction(refresher)));
	ms_message("Register refresher [%i] reason [%s] for proxy [%s]",status_code,reason_phrase,op->getProxy().c_str());

	if (belle_sip_refresher_get_auth_events(refresher)) {
		if (op->mAuthInfo) sal_auth_info_delete(op->mAuthInfo);
		/*only take first one for now*/
		op->mAuthInfo=sal_auth_info_create((belle_sip_auth_event_t*)(belle_sip_refresher_get_auth_events(refresher)->data));
	}
	sal_error_info_set(&op->mErrorInfo,SalReasonUnknown, "SIP", (int)status_code, reason_phrase, NULL);
	if (status_code>=200){
		op->assignRecvHeaders((belle_sip_message_t*)response);
	}
	if(status_code == 200) {
		/*check service route rfc3608*/
		belle_sip_header_service_route_t* service_route;
		belle_sip_header_address_t* service_route_address=NULL;
		belle_sip_header_contact_t *contact = belle_sip_refresher_get_contact(refresher);
		if ((service_route=belle_sip_message_get_header_by_type(response,belle_sip_header_service_route_t))) {
			service_route_address=belle_sip_header_address_create(NULL,belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(service_route)));
		}
		op->setServiceRoute((const SalAddress*)service_route_address);
		if (service_route_address) belle_sip_object_unref(service_route_address);

		op->mRoot->removePendingAuth(op); /*just in case*/

		if (contact) {
			const char *gruu;
			belle_sip_parameters_t* p = (BELLE_SIP_PARAMETERS(contact));
			if((gruu = belle_sip_parameters_get_parameter(p, "pub-gruu"))) {
				char *unquoted = belle_sip_unquote_strdup(gruu);
				op->setContactAddress((SalAddress*)belle_sip_header_address_parse(unquoted));
				bctbx_free(unquoted);
				belle_sip_parameters_remove_parameter(p, "pub-gruu");
			} else {
				op->setContactAddress((SalAddress*)(BELLE_SIP_HEADER_ADDRESS(contact))); /*update contact with real value*/
			}
		}
		op->mRoot->mCallbacks.register_success(op,belle_sip_refresher_get_expires(op->mRefresher)>0);
	} else if (status_code>=400) {
		/* from rfc3608, 6.1.
				If the UA refreshes the registration, the stored value of the Service-
				   Route is updated according to the Service-Route header field of the
				   latest 200 class response.  If there is no Service-Route header field
				   in the response, the UA clears any service route for that address-
				   of-record previously stored by the UA.  If the re-registration
				   request is refused or if an existing registration expires and the UA
				   chooses not to re-register, the UA SHOULD discard any stored service
				   route for that address-of-record. */
		op->setServiceRoute(NULL);
		op->ref(); /*take a ref while invoking the callback to make sure the operations done after are valid*/
		op->mRoot->mCallbacks.register_failure(op);
		if (op->mState!=State::Terminated && op->mAuthInfo) {
			/*add pending auth*/
			op->mRoot->addPendingAuth(op);
			if (status_code==403 || status_code==401 || status_code==407 )
				op->mRoot->mCallbacks.auth_failure(op,op->mAuthInfo);
		}
		op->unref();
	}
}

LINPHONE_END_NAMESPACE
