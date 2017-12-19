/*
 * sal.cpp
 * Copyright (C) 2010-2017 Belledonne Communications SARL
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

#include "sal/sal.h"
#include "sal/call-op.h"
#include "sal/presence-op.h"
#include "sal/refer-op.h"
#include "sal/event-op.h"
#include "sal/message-op.h"
#include "bellesip_sal/sal_impl.h"
#include "tester_utils.h"
#include "private.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

void Sal::process_dialog_terminated_cb(void *sal, const belle_sip_dialog_terminated_event_t *event) {
	belle_sip_dialog_t* dialog =  belle_sip_dialog_terminated_event_get_dialog(event);
	SalOp* op = reinterpret_cast<SalOp *>(belle_sip_dialog_get_application_data(dialog));
	if (op && op->callbacks && op->callbacks->process_dialog_terminated) {
		op->callbacks->process_dialog_terminated(op,event);
	} else {
		ms_error("sal process_dialog_terminated no op found for this dialog [%p], ignoring",dialog);
	}
}

void Sal::process_io_error_cb(void *user_ctx, const belle_sip_io_error_event_t *event){
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

void Sal::process_request_event_cb(void *ud, const belle_sip_request_event_t *event) {
	Sal *sal=(Sal*)ud;
	SalOp* op=NULL;
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_dialog_t* dialog=belle_sip_request_event_get_dialog(event);
	belle_sip_header_address_t* origin_address;
	belle_sip_header_address_t* address=NULL;
	belle_sip_header_from_t* from_header;
	belle_sip_header_to_t* to;
	belle_sip_header_diversion_t* diversion;
	belle_sip_response_t* resp;
	belle_sip_header_t *evh;
	const char *method=belle_sip_request_get_method(req);
	belle_sip_header_contact_t* remote_contact = belle_sip_message_get_header_by_type(req, belle_sip_header_contact_t);
	belle_sip_header_t *subjectHeader = belle_sip_message_get_header(BELLE_SIP_MESSAGE(req), "Subject");

	from_header=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_from_t);

	if (dialog) {
		op=(SalOp*)belle_sip_dialog_get_application_data(dialog);

		if (op == NULL  && strcmp("NOTIFY",method) == 0) {
			/*special case for Dialog created by notify mathing subscribe*/
			belle_sip_transaction_t * sub_trans = belle_sip_dialog_get_last_transaction(dialog);
			op = (SalOp*)belle_sip_transaction_get_application_data(sub_trans);
		}
		if (op==NULL || op->state==SalOp::State::Terminated){
			ms_warning("Receiving request for null or terminated op [%p], ignored",op);
			return;
		}
	}else{
		/*handle the case where we are receiving a request with to tag but it is not belonging to any dialog*/
		belle_sip_header_to_t *to = belle_sip_message_get_header_by_type(req, belle_sip_header_to_t);
		if ((strcmp("INVITE",method)==0 || strcmp("NOTIFY",method)==0) && (belle_sip_header_to_get_tag(to) != NULL)) {
			ms_warning("Receiving %s with to-tag but no know dialog here. Rejecting.", method);
			resp=belle_sip_response_create_from_request(req,481);
			belle_sip_provider_send_response(sal->prov,resp);
			return;
		/* by default (eg. when a to-tag is present), out of dialog ACK are automatically
		handled in lower layers (belle-sip) but in case it misses, it will be forwarded to us */
		} else if (strcmp("ACK",method)==0 && (belle_sip_header_to_get_tag(to) == NULL)) {
			ms_warning("Receiving ACK without to-tag but no know dialog here. Ignoring");
			return;
		}

		if (strcmp("INVITE",method)==0) {
			op=new SalCallOp(sal);
			op->fill_cbs();
		}else if ((strcmp("SUBSCRIBE",method)==0 || strcmp("NOTIFY",method)==0) && (evh=belle_sip_message_get_header(BELLE_SIP_MESSAGE(req),"Event"))!=NULL) {
			if (strncmp(belle_sip_header_get_unparsed_value(evh),"presence",strlen("presence"))==0){
				op=new SalPresenceOp(sal);
			} else {
				op=new SalSubscribeOp(sal);
			}
			op->fill_cbs();
		}else if (strcmp("MESSAGE",method)==0) {
			op=new SalMessageOp(sal);
			op->fill_cbs();
		}else if (strcmp("REFER",method)==0) {
			op=new SalReferOp(sal);
			op->fill_cbs();
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
		}else if (sal->_enable_test_features && strcmp("PUBLISH",method)==0) {
			resp=belle_sip_response_create_from_request(req,200);/*out of dialog PUBLISH */
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
		op->dir=SalOp::Dir::Incoming;
	}

	if (!op->from_address)  {
		if (belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(from_header)))
			address=belle_sip_header_address_create(belle_sip_header_address_get_displayname(BELLE_SIP_HEADER_ADDRESS(from_header))
					,belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(from_header)));
		else if ((belle_sip_header_address_get_absolute_uri(BELLE_SIP_HEADER_ADDRESS(from_header))))
			address=belle_sip_header_address_create2(belle_sip_header_address_get_displayname(BELLE_SIP_HEADER_ADDRESS(from_header))
					,belle_sip_header_address_get_absolute_uri(BELLE_SIP_HEADER_ADDRESS(from_header)));
		else
			ms_error("Cannot not find from uri from request [%p]",req);
		op->set_from_address((SalAddress*)address);
		belle_sip_object_unref(address);
	}

	if( remote_contact ){
		op->set_remote_contact(belle_sip_header_get_unparsed_value(BELLE_SIP_HEADER(remote_contact)));
	}

	if (!op->to_address) {
		to=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_to_t);
		if (belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(to)))
			address=belle_sip_header_address_create(belle_sip_header_address_get_displayname(BELLE_SIP_HEADER_ADDRESS(to))
					,belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(to)));
		else if ((belle_sip_header_address_get_absolute_uri(BELLE_SIP_HEADER_ADDRESS(to))))
			address=belle_sip_header_address_create2(belle_sip_header_address_get_displayname(BELLE_SIP_HEADER_ADDRESS(to))
					,belle_sip_header_address_get_absolute_uri(BELLE_SIP_HEADER_ADDRESS(to)));
		else
			ms_error("Cannot not find to uri from request [%p]",req);

		op->set_to_address((SalAddress*)address);
		belle_sip_object_unref(address);
	}

	if (subjectHeader) {
		const char *subject = belle_sip_header_get_unparsed_value(subjectHeader);
		op->set_subject(subject);
	}

	if(!op->diversion_address){
		diversion=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_diversion_t);
		if (diversion) {
			if (belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(diversion)))
				address=belle_sip_header_address_create(belle_sip_header_address_get_displayname(BELLE_SIP_HEADER_ADDRESS(diversion))
						,belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(diversion)));
			else if ((belle_sip_header_address_get_absolute_uri(BELLE_SIP_HEADER_ADDRESS(diversion))))
				address=belle_sip_header_address_create2(belle_sip_header_address_get_displayname(BELLE_SIP_HEADER_ADDRESS(diversion))
						,belle_sip_header_address_get_absolute_uri(BELLE_SIP_HEADER_ADDRESS(diversion)));
			else
				ms_warning("Cannot not find diversion header from request [%p]",req);
			if (address) {
				op->set_diversion_address((SalAddress*)address);
				belle_sip_object_unref(address);
			}
		}
	}

	if (!op->origin) {
		/*set origin uri*/
		origin_address=belle_sip_header_address_create(NULL,belle_sip_request_extract_origin(req));
		op->set_network_origin_address((SalAddress*)origin_address);
		belle_sip_object_unref(origin_address);
	}
	if (!op->remote_ua) {
		op->set_remote_ua(BELLE_SIP_MESSAGE(req));
	}

	if (!op->call_id) {
		op->call_id=ms_strdup(belle_sip_header_call_id_get_call_id(BELLE_SIP_HEADER_CALL_ID(belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req), belle_sip_header_call_id_t))));
	}
	/*It is worth noting that proxies can (and
   will) remove this header field*/
	op->set_privacy_from_message((belle_sip_message_t*)req);

	op->assign_recv_headers((belle_sip_message_t*)req);
	if (op->callbacks && op->callbacks->process_request_event) {
		op->callbacks->process_request_event(op,event);
	} else {
		ms_error("sal process_request_event not implemented yet");
	}

}

void Sal::process_response_event_cb(void *user_ctx, const belle_sip_response_event_t *event) {
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

		if (op->state == SalOp::State::Terminated) {
			belle_sip_message("Op [%p] is terminated, nothing to do with this [%i]", op, response_code);
			return;
		}
		/*do it all the time, since we can receive provisional responses from a different instance than the final one*/
		op->set_remote_ua(BELLE_SIP_MESSAGE(response));

		if(remote_contact) {
			op->set_remote_contact(belle_sip_header_get_unparsed_value(BELLE_SIP_HEADER(remote_contact)));
		}

		if (!op->call_id) {
			op->call_id=ms_strdup(belle_sip_header_call_id_get_call_id(BELLE_SIP_HEADER_CALL_ID(belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(response), belle_sip_header_call_id_t))));
		}

		op->assign_recv_headers((belle_sip_message_t*)response);

		if (op->callbacks && op->callbacks->process_response_event) {
			/*handle authorization*/
			switch (response_code) {
				case 200:
					break;
				case 401:
				case 407:
					if (op->state == SalOp::State::Terminating && strcmp("BYE",belle_sip_request_get_method(request))!=0) {
						/*only bye are completed*/
						belle_sip_message("Op is in state terminating, nothing else to do ");
					} else {
						if (op->pending_auth_transaction){
							belle_sip_object_unref(op->pending_auth_transaction);
							op->pending_auth_transaction=NULL;
						}
						if (++op->auth_requests > 2) {
							ms_warning("Auth info cannot be found for op [%s/%s] after 2 attempts, giving up",op->get_from()
																												,op->get_to());
							op->root->callbacks.auth_failure(op,op->auth_info);
							op->root->remove_pending_auth(op);
						} else {
							op->pending_auth_transaction=(belle_sip_client_transaction_t*)belle_sip_object_ref(client_transaction);
							op->process_authentication();
							return;
						}
					}
					break;
				case 403:
					if (op->auth_info) op->root->callbacks.auth_failure(op,op->auth_info);
					break;
				case 302:
				case 301:
					if (op->process_redirect() == 0)
						return;
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

void Sal::process_timeout_cb(void *user_ctx, const belle_sip_timeout_event_t *event) {
	belle_sip_client_transaction_t* client_transaction = belle_sip_timeout_event_get_client_transaction(event);
	SalOp* op = (SalOp*)belle_sip_transaction_get_application_data(BELLE_SIP_TRANSACTION(client_transaction));
	if (op && op->callbacks && op->callbacks->process_timeout) {
		op->callbacks->process_timeout(op,event);
	} else {
		ms_error("Unhandled event timeout [%p]",event);
	}
}

void Sal::process_transaction_terminated_cb(void *user_ctx, const belle_sip_transaction_terminated_event_t *event) {
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
	if (op) {
		op->unref(); /*because every transaction ref op*/
		belle_sip_transaction_set_application_data(trans,NULL); /*no longuer reference something we do not ref to avoid futur access of a released op*/
	}
}

void Sal::process_auth_requested_cb(void *sal, belle_sip_auth_event_t *event) {
	SalAuthInfo* auth_info = sal_auth_info_create(event);
	((Sal*)sal)->callbacks.auth_requested(reinterpret_cast<Sal *>(sal),auth_info);
	belle_sip_auth_event_set_passwd(event,(const char*)auth_info->password);
	belle_sip_auth_event_set_ha1(event,(const char*)auth_info->ha1);
	belle_sip_auth_event_set_userid(event,(const char*)auth_info->userid);
	belle_sip_auth_event_set_signing_key(event,(belle_sip_signing_key_t *)auth_info->key);
	belle_sip_auth_event_set_client_certificates_chain(event,(belle_sip_certificates_chain_t* )auth_info->certificates);
	sal_auth_info_delete(auth_info);
}

Sal::Sal(MSFactory *factory){
	belle_sip_listener_callbacks_t listener_callbacks = {0};

	/*belle_sip_object_enable_marshal_check(TRUE);*/
	this->factory = factory;
	/*first create the stack, which initializes the belle-sip object's pool for this thread*/
	this->stack = belle_sip_stack_new(NULL);

	this->user_agent=belle_sip_header_user_agent_new();
#if defined(PACKAGE_NAME) && defined(LIBLINPHONE_VERSION)
	belle_sip_header_user_agent_add_product(this->user_agent, PACKAGE_NAME "/" LIBLINPHONE_VERSION);
#else
	belle_sip_header_user_agent_add_product(this->user_agent, "Unknown");
#endif
	append_stack_string_to_user_agent();
	belle_sip_object_ref(this->user_agent);

	this->prov = belle_sip_stack_create_provider(this->stack,NULL);
	enable_nat_helper(TRUE);

	listener_callbacks.process_dialog_terminated=process_dialog_terminated_cb;
	listener_callbacks.process_io_error=process_io_error_cb;
	listener_callbacks.process_request_event=process_request_event_cb;
	listener_callbacks.process_response_event=process_response_event_cb;
	listener_callbacks.process_timeout=process_timeout_cb;
	listener_callbacks.process_transaction_terminated=process_transaction_terminated_cb;
	listener_callbacks.process_auth_requested=process_auth_requested_cb;
	this->listener=belle_sip_listener_create_from_callbacks(&listener_callbacks, this);
	belle_sip_provider_add_sip_listener(this->prov, this->listener);
}

Sal::~Sal() {
	belle_sip_object_unref(this->user_agent);
	belle_sip_object_unref(this->prov);
	belle_sip_object_unref(this->stack);
	belle_sip_object_unref(this->listener);
	if (this->supported) belle_sip_object_unref(this->supported);
	bctbx_list_free_with_data(this->supported_tags,ms_free);
	bctbx_list_free_with_data(this->supported_content_types, ms_free);
	if (this->uuid) ms_free(this->uuid);
	if (this->root_ca) ms_free(this->root_ca);
	if (this->root_ca_data) ms_free(this->root_ca_data);
	if (this->linphone_specs) ms_free(this->linphone_specs);
}

void Sal::set_callbacks(const Callbacks *cbs) {
	memcpy(&this->callbacks,cbs,sizeof(*cbs));
	if (this->callbacks.call_received==NULL)
		this->callbacks.call_received=(OnCallReceivedCb)unimplemented_stub;
	if (this->callbacks.call_ringing==NULL)
		this->callbacks.call_ringing=(OnCallRingingCb)unimplemented_stub;
	if (this->callbacks.call_accepted==NULL)
		this->callbacks.call_accepted=(OnCallAcceptedCb)unimplemented_stub;
	if (this->callbacks.call_failure==NULL)
		this->callbacks.call_failure=(OnCallFailureCb)unimplemented_stub;
	if (this->callbacks.call_terminated==NULL)
		this->callbacks.call_terminated=(OnCallTerminatedCb)unimplemented_stub;
	if (this->callbacks.call_released==NULL)
		this->callbacks.call_released=(OnCallReleasedCb)unimplemented_stub;
	if (this->callbacks.call_updating==NULL)
		this->callbacks.call_updating=(OnCallUpdatingCb)unimplemented_stub;
	if (this->callbacks.auth_failure==NULL)
		this->callbacks.auth_failure=(OnAuthFailureCb)unimplemented_stub;
	if (this->callbacks.register_success==NULL)
		this->callbacks.register_success=(OnRegisterSuccessCb)unimplemented_stub;
	if (this->callbacks.register_failure==NULL)
		this->callbacks.register_failure=(OnRegisterFailureCb)unimplemented_stub;
	if (this->callbacks.dtmf_received==NULL)
		this->callbacks.dtmf_received=(OnDtmfReceivedCb)unimplemented_stub;
	if (this->callbacks.notify==NULL)
		this->callbacks.notify=(OnNotifyCb)unimplemented_stub;
	if (this->callbacks.subscribe_received==NULL)
		this->callbacks.subscribe_received=(OnSubscribeReceivedCb)unimplemented_stub;
	if (this->callbacks.incoming_subscribe_closed==NULL)
		this->callbacks.incoming_subscribe_closed=(OnIncomingSubscribeClosedCb)unimplemented_stub;
	if (this->callbacks.parse_presence_requested==NULL)
		this->callbacks.parse_presence_requested=(OnParsePresenceRequestedCb)unimplemented_stub;
	if (this->callbacks.convert_presence_to_xml_requested==NULL)
		this->callbacks.convert_presence_to_xml_requested=(OnConvertPresenceToXMLRequestedCb)unimplemented_stub;
	if (this->callbacks.notify_presence==NULL)
		this->callbacks.notify_presence=(OnNotifyPresenceCb)unimplemented_stub;
	if (this->callbacks.subscribe_presence_received==NULL)
		this->callbacks.subscribe_presence_received=(OnSubscribePresenceReceivedCb)unimplemented_stub;
	if (this->callbacks.message_received==NULL)
		this->callbacks.message_received=(OnMessageReceivedCb)unimplemented_stub;
	if (this->callbacks.ping_reply==NULL)
		this->callbacks.ping_reply=(OnPingReplyCb)unimplemented_stub;
	if (this->callbacks.auth_requested==NULL)
		this->callbacks.auth_requested=(OnAuthRequestedCb)unimplemented_stub;
	if (this->callbacks.info_received==NULL)
		this->callbacks.info_received=(OnInfoReceivedCb)unimplemented_stub;
	if (this->callbacks.on_publish_response==NULL)
		this->callbacks.on_publish_response=(OnPublishResponseCb)unimplemented_stub;
	if (this->callbacks.on_expire==NULL)
		this->callbacks.on_expire=(OnExpireCb)unimplemented_stub;
}

void Sal::set_tls_properties(){
	belle_sip_listening_point_t *lp=belle_sip_provider_get_listening_point(this->prov,"TLS");
	if (lp){
		belle_sip_tls_listening_point_t *tlp=BELLE_SIP_TLS_LISTENING_POINT(lp);
		belle_tls_crypto_config_t *crypto_config = belle_tls_crypto_config_new();
		int verify_exceptions = BELLE_TLS_VERIFY_NONE;
		if (!this->tls_verify) verify_exceptions = BELLE_TLS_VERIFY_ANY_REASON;
		else if (!this->tls_verify_cn) verify_exceptions = BELLE_TLS_VERIFY_CN_MISMATCH;
		belle_tls_crypto_config_set_verify_exceptions(crypto_config, verify_exceptions);
		if (this->root_ca != NULL) belle_tls_crypto_config_set_root_ca(crypto_config, this->root_ca);
		if (this->root_ca_data != NULL) belle_tls_crypto_config_set_root_ca_data(crypto_config, this->root_ca_data);
		if (this->ssl_config != NULL) belle_tls_crypto_config_set_ssl_config(crypto_config, this->ssl_config);
		belle_sip_tls_listening_point_set_crypto_config(tlp, crypto_config);
		belle_sip_object_unref(crypto_config);
	}
}

int Sal::add_listen_port(SalAddress* addr, bool_t is_tunneled) {
	int result;
	belle_sip_listening_point_t* lp;
	if (is_tunneled){
#ifdef TUNNEL_ENABLED
		if (sal_address_get_transport(addr)!=SalTransportUDP){
			ms_error("Tunneled mode is only available for UDP kind of transports.");
			return -1;
		}
		lp = belle_sip_tunnel_listening_point_new(this->stack, this->tunnel_client);
		if (!lp){
			ms_error("Could not create tunnel listening point.");
			return -1;
		}
#else
		ms_error("No tunnel support in library.");
		return -1;
#endif
	}else{
		lp = belle_sip_stack_create_listening_point(this->stack,
									sal_address_get_domain(addr),
									sal_address_get_port(addr),
									sal_transport_to_string(sal_address_get_transport(addr)));
	}
	if (lp) {
		belle_sip_listening_point_set_keep_alive(lp,(int)this->keep_alive);
		result = belle_sip_provider_add_listening_point(this->prov,lp);
		if (sal_address_get_transport(addr)==SalTransportTLS) {
			set_tls_properties();
		}
	} else {
		return -1;
	}
	return result;
}

int Sal::set_listen_port(const char *addr, int port, SalTransport tr, bool_t is_tunneled) {
	SalAddress* sal_addr = sal_address_new(NULL);
	int result;
	sal_address_set_domain(sal_addr,addr);
	sal_address_set_port(sal_addr,port);
	sal_address_set_transport(sal_addr,tr);
	result = add_listen_port(sal_addr, is_tunneled);
	sal_address_destroy(sal_addr);
	return result;
}

int Sal::get_listening_port(SalTransport tr){
	const char *tpn=sal_transport_to_string(tr);
	belle_sip_listening_point_t *lp=belle_sip_provider_get_listening_point(this->prov, tpn);
	if (lp){
		return belle_sip_listening_point_get_port(lp);
	}
	return 0;
}

int Sal::unlisten_ports(){
	const belle_sip_list_t * lps = belle_sip_provider_get_listening_points(this->prov);
	belle_sip_list_t * tmp_list = belle_sip_list_copy(lps);
	belle_sip_list_for_each2 (tmp_list,(void (*)(void*,void*))remove_listening_point,this->prov);
	belle_sip_list_free(tmp_list);
	ms_message("sal_unlisten_ports done");
	return 0;
}

int Sal::transport_available(SalTransport t) {
	switch(t){
		case SalTransportUDP:
		case SalTransportTCP:
			return TRUE;
		case SalTransportTLS:
			return belle_sip_stack_tls_available(this->stack);
		case SalTransportDTLS:
			return FALSE;
	}
	return FALSE;
}

void Sal::make_supported_header(){
	bctbx_list_t *it;
	char *alltags=NULL;
	size_t buflen=64;
	size_t written=0;

	if (this->supported){
		belle_sip_object_unref(this->supported);
		this->supported=NULL;
	}
	for(it=this->supported_tags;it!=NULL;it=it->next){
		const char *tag=(const char*)it->data;
		size_t taglen=strlen(tag);
		if (alltags==NULL || (written+taglen+1>=buflen)) alltags=reinterpret_cast<char *>(ms_realloc(alltags,(buflen=buflen*2)));
		written+=(size_t)snprintf(alltags+written,buflen-written,it->next ? "%s, " : "%s",tag);
	}
	if (alltags){
		this->supported=belle_sip_header_create("Supported",alltags);
		if (this->supported){
			belle_sip_object_ref(this->supported);
		}
		ms_free(alltags);
	}
}

void Sal::set_supported_tags(const char* tags){
	this->supported_tags=bctbx_list_free_with_data(this->supported_tags,ms_free);
	if (tags){
		char *iter;
		char *buffer=ms_strdup(tags);
		char *tag;
		char *context=NULL;
		iter=buffer;
		while((tag=strtok_r(iter,", ",&context))!=NULL){
			iter=NULL;
			this->supported_tags=bctbx_list_append(this->supported_tags,ms_strdup(tag));
		}
		ms_free(buffer);
	}
	make_supported_header();
}

void Sal::add_supported_tag(const char* tag){
	bctbx_list_t *elem=bctbx_list_find_custom(this->supported_tags,(bctbx_compare_func)strcasecmp,tag);
	if (!elem){
		this->supported_tags=bctbx_list_append(this->supported_tags,ms_strdup(tag));
		make_supported_header();
	}
}

void Sal::remove_supported_tag(const char* tag){
	bctbx_list_t *elem=bctbx_list_find_custom(this->supported_tags,(bctbx_compare_func)strcasecmp,tag);
	if (elem){
		ms_free(elem->data);
		this->supported_tags=bctbx_list_erase_link(this->supported_tags,elem);
		make_supported_header();
	}
}

int Sal::reset_transports() {
	ms_message("Reseting transports");
	belle_sip_provider_clean_channels(this->prov);
	return 0;
}

ortp_socket_t Sal::get_socket() const {
	ms_warning("sal_get_socket is deprecated");
	return -1;
}

void Sal::set_user_agent(const char *user_agent) {
	belle_sip_header_user_agent_set_products(this->user_agent,NULL);
	belle_sip_header_user_agent_add_product(this->user_agent,user_agent);
}

const char* Sal::get_user_agent() const {
	static char user_agent[255];
	belle_sip_header_user_agent_get_products_as_string(this->user_agent, user_agent, 254);
	return user_agent;
}

void Sal::append_stack_string_to_user_agent() {
	char stack_string[64];
	snprintf(stack_string, sizeof(stack_string) - 1, "(belle-sip/%s)", belle_sip_version_to_string());
	belle_sip_header_user_agent_add_product(this->user_agent, stack_string);
}

void Sal::set_keepalive_period(unsigned int value) {
	const belle_sip_list_t* iterator;
	belle_sip_listening_point_t* lp;
	this->keep_alive=value;
	for (iterator=belle_sip_provider_get_listening_points(this->prov);iterator!=NULL;iterator=iterator->next) {
		lp=(belle_sip_listening_point_t*)iterator->data;
		if (this->use_tcp_tls_keep_alive || strcasecmp(belle_sip_listening_point_get_transport(lp),"udp")==0) {
			belle_sip_listening_point_set_keep_alive(lp,(int)this->keep_alive);
		}
	}
}

int Sal::set_tunnel(void *tunnelclient) {
#ifdef TUNNEL_ENABLED
	this->tunnel_client=tunnelclient;
	return 0;
#else
	return -1;
#endif
}

bool_t Sal::is_content_type_supported(const char *content_type) const {
	bctbx_list_t *item;
	for (item = this->supported_content_types; item != NULL; item = bctbx_list_next(item)) {
		const char *item_content_type = (const char *)bctbx_list_get_data(item);
		if (strcmp(item_content_type, content_type) == 0) return TRUE;
	}
	return FALSE;
}

void Sal::add_content_type_support(const char *content_type) {
	if ((content_type != NULL) && (is_content_type_supported(content_type) == FALSE)) {
		this->supported_content_types = bctbx_list_append(this->supported_content_types, ms_strdup(content_type));
	}
}

void Sal::use_rport(bool_t use_rports) {
	belle_sip_provider_enable_rport(this->prov,use_rports);
	ms_message("Sal use rport [%s]", use_rports ? "enabled" : "disabled");
}

void Sal::set_contact_linphone_specs(const char *specs) {
	if (this->linphone_specs) {
		ms_free(this->linphone_specs);
		this->linphone_specs = NULL;
	}
	if (specs) {
		this->linphone_specs = ms_strdup(specs);
	}
}

void Sal::set_root_ca(const char* rootCa) {
	if (this->root_ca) {
		ms_free(this->root_ca);
		this->root_ca = NULL;
	}
	if (rootCa)
		this->root_ca = ms_strdup(rootCa);
	set_tls_properties();
}

void Sal::set_root_ca_data(const char* data) {
	if (this->root_ca_data) {
		ms_free(this->root_ca_data);
		this->root_ca_data = NULL;
	}
	if (data)
		this->root_ca_data = ms_strdup(data);
	set_tls_properties();
}

void Sal::verify_server_certificates(bool_t verify) {
	this->tls_verify=verify;
	set_tls_properties();
}

void Sal::verify_server_cn(bool_t verify) {
	this->tls_verify_cn = verify;
	set_tls_properties();
}

void Sal::set_ssl_config(void *ssl_config) {
	this->ssl_config = ssl_config;
	set_tls_properties();
}

void Sal::set_uuid(const char *uuid){
	if (this->uuid){
		ms_free(this->uuid);
		this->uuid=NULL;
	}
	if (uuid)
		this->uuid=ms_strdup(uuid);
}

int Sal::create_uuid(char *uuid, size_t len) {
	if (generate_uuid(uuid, len) == 0) {
		set_uuid(uuid);
		return 0;
	}
	return -1;
}

int Sal::generate_uuid(char *uuid, size_t len) {
	sal_uuid_t uuid_struct;
	int i;
	int written;

	if (len==0) return -1;
	/*create an UUID as described in RFC4122, 4.4 */
	belle_sip_random_bytes((unsigned char*)&uuid_struct, sizeof(sal_uuid_t));
	uuid_struct.clock_seq_hi_and_reserved&=(unsigned char)~(1<<6);
	uuid_struct.clock_seq_hi_and_reserved|=(unsigned char)1<<7;
	uuid_struct.time_hi_and_version&=(unsigned char)~(0xf<<12);
	uuid_struct.time_hi_and_version|=(unsigned char)4<<12;

	written=snprintf(uuid,len,"%8.8x-%4.4x-%4.4x-%2.2x%2.2x-", uuid_struct.time_low, uuid_struct.time_mid,
			uuid_struct.time_hi_and_version, uuid_struct.clock_seq_hi_and_reserved,
			uuid_struct.clock_seq_low);
	if ((written < 0) || ((size_t)written > (len +13))) {
		ms_error("sal_create_uuid(): buffer is too short !");
		return -1;
	}
	for (i = 0; i < 6; i++)
		written+=snprintf(uuid+written,len-(unsigned long)written,"%2.2x", uuid_struct.node[i]);
	uuid[len-1]='\0';
	return 0;
}

void Sal::add_pending_auth(SalOp *op){
	if (bctbx_list_find(this->pending_auths,op)==NULL){
		this->pending_auths=bctbx_list_append(this->pending_auths,op);
		op->has_auth_pending=TRUE;
	}
}

void Sal::remove_pending_auth(SalOp *op){
	if (op->has_auth_pending){
		op->has_auth_pending=FALSE;
		if (bctbx_list_find(this->pending_auths,op)){
			this->pending_auths=bctbx_list_remove(this->pending_auths,op);
		}
	}
}

void Sal::set_default_sdp_handling(SalOpSDPHandling sdp_handling_method)  {
	if (sdp_handling_method != SalOpSDPNormal ) ms_message("Enabling special SDP handling for all new SalOp in Sal[%p]!", this);
	this->default_sdp_handling = sdp_handling_method;
}

void Sal::enable_nat_helper(bool_t enable) {
	this->_nat_helper_enabled=enable;
	belle_sip_provider_enable_nat_helper(this->prov,enable);
	ms_message("Sal nat helper [%s]",enable?"enabled":"disabled");
}

void Sal::get_default_local_ip(int address_family, char *ip, size_t iplen) {
	strncpy(ip,address_family==AF_INET6 ? "::1" : "127.0.0.1",iplen);
	ms_error("sal_get_default_local_ip() is deprecated.");
}

void Sal::set_dns_servers(const bctbx_list_t *servers){
	belle_sip_list_t *l = NULL;

	/*we have to convert the bctbx_list_t into a belle_sip_list_t first*/
	for (; servers != NULL; servers = servers->next){
		l = belle_sip_list_append(l, servers->data);
	}
	belle_sip_stack_set_dns_servers(this->stack, l);
	belle_sip_list_free(l);
}

belle_sip_source_t *Sal::create_timer(belle_sip_source_func_t func, void *data, unsigned int timeout_value_ms, const char* timer_name) {
	belle_sip_main_loop_t *ml = belle_sip_stack_get_main_loop(this->stack);
	return belle_sip_main_loop_create_timeout(ml, func, data, timeout_value_ms, timer_name);
}

void Sal::cancel_timer(belle_sip_source_t *timer) {
	belle_sip_main_loop_t *ml = belle_sip_stack_get_main_loop(this->stack);
	belle_sip_main_loop_remove_source(ml, timer);
}

belle_sip_response_t* Sal::create_response_from_request (belle_sip_request_t* req, int code ) {
	belle_sip_response_t *resp=belle_sip_response_create_from_request(req,code);
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(resp),BELLE_SIP_HEADER(this->user_agent));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(resp), this->supported);
	return resp;
}




/***********************************/
/* Global functions implementation */
/***********************************/

int to_sip_code(SalReason r) {
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

LINPHONE_END_NAMESPACE

/*******************************/
/* C++ to C wrapping functions */
/*******************************/

using namespace LinphonePrivate;

extern "C" {

Sal *linphone_core_get_sal(const LinphoneCore *lc) {
	return lc->sal;
}

SalOp *linphone_proxy_config_get_sal_op(const LinphoneProxyConfig *cfg) {
	return cfg->op;
}

SalOp *linphone_call_get_op_as_sal_op(const LinphoneCall *call) {
	return linphone_call_get_op(call);
}

Sal *sal_init(MSFactory *factory) {
	return new Sal(factory);
}

void sal_uninit(Sal* sal) {
	delete sal;
}

int sal_create_uuid(Sal *ctx, char *uuid, size_t len) {
	return ctx->create_uuid(uuid, len);
}

void sal_set_uuid(Sal *ctx, const char *uuid) {
	ctx->set_uuid(uuid);
}

void sal_default_set_sdp_handling(Sal* h, SalOpSDPHandling handling_method) {
	h->set_default_sdp_handling(handling_method);
}

void sal_set_send_error(Sal *sal,int value) {
	sal->set_send_error(value);
}

void sal_set_recv_error(Sal *sal,int value) {
	sal->set_recv_error(value);
}

int sal_enable_pending_trans_checking(Sal *sal, bool_t value) {
	return sal->enable_pending_trans_checking(value);
}

void sal_enable_unconditional_answer(Sal *sal,int value) {
	sal->enable_unconditional_answer(value);
}

void sal_set_dns_timeout(Sal* sal,int timeout) {
	sal->set_dns_timeout(timeout);
}

void sal_set_dns_user_hosts_file(Sal *sal, const char *hosts_file) {
	sal->set_dns_user_hosts_file(hosts_file);
}

void *sal_get_stack_impl(Sal *sal) {
	return sal->get_stack_impl();
}

void sal_set_refresher_retry_after(Sal *sal,int value) {
	sal->set_refresher_retry_after(value);
}

int sal_get_refresher_retry_after(const Sal *sal) {
	return sal->get_refresher_retry_after();
}

void sal_set_transport_timeout(Sal* sal,int timeout) {
	sal->set_transport_timeout(timeout);
}

void sal_enable_test_features(Sal*ctx, bool_t enabled) {
	ctx->enable_test_features(enabled);
}

int sal_transport_available(Sal *ctx, SalTransport t) {
	return ctx->transport_available(t);
}

const SalErrorInfo *sal_op_get_error_info(const SalOp *op) {
	return op->get_error_info();
}

bool_t sal_call_dialog_request_pending(const SalOp *op) {
	auto callOp = dynamic_cast<const SalCallOp *>(op);
	return callOp->dialog_request_pending();
}

void sal_call_set_sdp_handling(SalOp *h, SalOpSDPHandling handling) {
	auto callOp = dynamic_cast<SalCallOp *>(h);
	callOp->set_sdp_handling(handling);
}

SalMediaDescription * sal_call_get_final_media_description(SalOp *h) {
	auto callOp = dynamic_cast<SalCallOp *>(h);
	return callOp->get_final_media_description();
}

belle_sip_resolver_context_t *sal_resolve_a(Sal *sal, const char *name, int port, int family, belle_sip_resolver_callback_t cb, void *data) {
	return sal->resolve_a(name, port, family, cb, data);
}

Sal *sal_op_get_sal(SalOp *op) {
	return op->get_sal();
}

SalOp *sal_create_refer_op(Sal *sal) {
	return new SalReferOp(sal);
}

void sal_release_op(SalOp *op) {
	op->release();
}

void sal_op_set_from(SalOp *sal_refer_op, const char* from) {
	auto referOp = dynamic_cast<SalReferOp *>(sal_refer_op);
	referOp->set_from(from);
}

void sal_op_set_to(SalOp *sal_refer_op, const char* to) {
	auto referOp = dynamic_cast<SalReferOp *>(sal_refer_op);
	referOp->set_to(to);
}

void sal_op_send_refer(SalOp *sal_refer_op, SalAddress* refer_to) {
	auto referOp = dynamic_cast<SalReferOp *>(sal_refer_op);
	referOp->send_refer(refer_to);
}

void sal_set_user_pointer(Sal *sal, void *user_pointer) {
	sal->set_user_pointer(user_pointer);
}

void *sal_get_user_pointer(Sal *sal) {
	return sal->get_user_pointer();
}

void sal_set_call_refer_callback(Sal *sal, void (*OnReferCb)(SalOp *op, const SalAddress *referto)) {
	struct Sal::Callbacks cbs = {NULL};
	cbs.refer_received = OnReferCb;
	sal->set_callbacks(&cbs);
}

}
