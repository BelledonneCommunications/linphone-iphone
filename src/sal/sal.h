/*
 * sal.h
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

#ifndef _SAL_H_
#define _SAL_H_

#include "c-wrapper/internal/c-sal.h"
#include "linphone/utils/general.h"

LINPHONE_BEGIN_NAMESPACE

class SalOp;
class SalCallOp;
class SalMessageOp;
class SalSubscribeOp;
class SalPresenceOp;
class SalReferOp;

class Sal{
public:
	typedef void (*OnCallReceivedCb)(SalCallOp *op);
	typedef void (*OnCallRingingCb)(SalOp *op);
	typedef void (*OnCallAcceptedCb)(SalOp *op);
	typedef void (*OnCallAckReceivedCb)(SalOp *op, SalCustomHeader *ack);
	typedef void (*OnCallAckBeingSentCb)(SalOp *op, SalCustomHeader *ack);
	typedef void (*OnCallUpdatingCb)(SalOp *op, bool_t is_update);/* Called when a reINVITE/UPDATE is received*/
	typedef void (*OnCallTerminatedCb)(SalOp *op, const char *from);
	typedef void (*OnCallFailureCb)(SalOp *op);
	typedef void (*OnCallReleasedCb)(SalOp *salop);
	typedef void (*OnCallCancelDoneCb)(SalOp *salop);
	typedef void (*OnAuthRequestedLegacyCb)(SalOp *op, const char *realm, const char *username);
	typedef bool_t (*OnAuthRequestedCb)(Sal *sal,SalAuthInfo* info);
	typedef void (*OnAuthFailureCb)(SalOp *op, SalAuthInfo* info);
	typedef void (*OnRegisterSuccessCb)(SalOp *op, bool_t registered);
	typedef void (*OnRegisterFailureCb)(SalOp *op);
	typedef void (*OnVfuRequestCb)(SalOp *op);
	typedef void (*OnDtmfReceivedCb)(SalOp *op, char dtmf);
	typedef void (*OnCallReferCb)(SalOp *op, const SalAddress *referto);
	typedef void (*OnReferCb)(SalOp *op, const SalAddress *referto);
	typedef void (*OnMessageReceivedCb)(SalOp *op, const SalMessage *msg);
	typedef void (*OnMessageDeliveryUpdateCb)(SalOp *op, SalMessageDeliveryStatus);
	typedef void (*OnNotifyReferCb)(SalOp *op, SalReferStatus state);
	typedef void (*OnSubscribeResponseCb)(SalOp *op, SalSubscribeStatus status, int will_retry);
	typedef void (*OnNotifyCb)(SalSubscribeOp *op, SalSubscribeStatus status, const char *event, SalBodyHandler *body);
	typedef void (*OnSubscribeReceivedCb)(SalSubscribeOp *salop, const char *event, const SalBodyHandler *body);
	typedef void (*OnIncomingSubscribeClosedCb)(SalOp *salop);
	typedef void (*OnParsePresenceRequestedCb)(SalOp *salop, const char *content_type, const char *content_subtype, const char *content, SalPresenceModel **result);
	typedef void (*OnConvertPresenceToXMLRequestedCb)(SalOp *salop, SalPresenceModel *presence, const char *contact, char **content);
	typedef void (*OnNotifyPresenceCb)(SalOp *op, SalSubscribeStatus ss, SalPresenceModel *model, const char *msg);
	typedef void (*OnSubscribePresenceReceivedCb)(SalPresenceOp *salop, const char *from);
	typedef void (*OnSubscribePresenceClosedCb)(SalPresenceOp *salop, const char *from);
	typedef void (*OnPingReplyCb)(SalOp *salop);
	typedef void (*OnInfoReceivedCb)(SalOp *salop, SalBodyHandler *body);
	typedef void (*OnPublishResponseCb)(SalOp *salop);
	typedef void (*OnNotifyResponseCb)(SalOp *salop);
	typedef void (*OnExpireCb)(SalOp *salop);

	struct Callbacks {
		OnCallReceivedCb call_received;
		OnCallReceivedCb call_rejected;
		OnCallRingingCb call_ringing;
		OnCallAcceptedCb call_accepted;
		OnCallAckReceivedCb call_ack_received;
		OnCallAckBeingSentCb call_ack_being_sent;
		OnCallUpdatingCb call_updating;
		OnCallTerminatedCb call_terminated;
		OnCallFailureCb call_failure;
		OnCallReleasedCb call_released;
		OnCallCancelDoneCb call_cancel_done;
		OnCallReferCb call_refer_received;
		OnAuthFailureCb auth_failure;
		OnRegisterSuccessCb register_success;
		OnRegisterFailureCb register_failure;
		OnVfuRequestCb vfu_request;
		OnDtmfReceivedCb dtmf_received;
		
		OnMessageReceivedCb message_received;
		OnMessageDeliveryUpdateCb message_delivery_update;
		OnNotifyReferCb notify_refer;
		OnSubscribeReceivedCb subscribe_received;
		OnIncomingSubscribeClosedCb incoming_subscribe_closed;
		OnSubscribeResponseCb subscribe_response;
		OnNotifyCb notify;
		OnSubscribePresenceReceivedCb subscribe_presence_received;
		OnSubscribePresenceClosedCb subscribe_presence_closed;
		OnParsePresenceRequestedCb parse_presence_requested;
		OnConvertPresenceToXMLRequestedCb convert_presence_to_xml_requested;
		OnNotifyPresenceCb notify_presence;
		OnPingReplyCb ping_reply;
		OnAuthRequestedCb auth_requested;
		OnInfoReceivedCb info_received;
		OnPublishResponseCb on_publish_response;
		OnExpireCb on_expire;
		OnNotifyResponseCb on_notify_response;
		OnReferCb refer_received; /*for out of dialog refer*/
	};

	Sal(MSFactory *factory);
	~Sal();

	void set_user_pointer(void *user_data) {this->up=user_data;}
	void *get_user_pointer() const {return this->up;}

	void set_callbacks(const Callbacks *cbs);

	void *get_stack_impl() {return this->stack;}

	int iterate() {belle_sip_stack_sleep(this->stack,0); return 0;}

	void  set_send_error(int value) {belle_sip_stack_set_send_error(this->stack,value);}
	void  set_recv_error(int value) {belle_sip_provider_set_recv_error(this->prov,value);}


	/******************/
	/* SIP parameters */
	/******************/
	void set_supported_tags(const char* tags);
	const char *get_supported_tags() const {return this->supported ? belle_sip_header_get_unparsed_value(this->supported) : NULL;}
	void add_supported_tag(const char* tag);
	void remove_supported_tag(const char* tag);

	void set_user_agent(const char *user_agent);
	const char* get_user_agent() const;
	void append_stack_string_to_user_agent();

	bool_t content_encoding_available(const char *content_encoding) {return (bool_t)belle_sip_stack_content_encoding_available(this->stack, content_encoding);}
	bool_t is_content_type_supported(const char *content_type) const;
	void add_content_type_support(const char *content_type);

	void set_default_sdp_handling(SalOpSDPHandling sdp_handling_method);

	void set_uuid(const char *uuid);
	int create_uuid(char *uuid, size_t len);
	static int generate_uuid(char *uuid, size_t len);

	void enable_nat_helper(bool_t enable);
	bool_t nat_helper_enabled() const {return this->_nat_helper_enabled;}

	bool_t pending_trans_checking_enabled() const {return this->pending_trans_checking;}
	int enable_pending_trans_checking(bool_t value) {this->pending_trans_checking = value; return 0;}

	void set_refresher_retry_after(int value) {this->refresher_retry_after=value;}
	int get_refresher_retry_after() const {return this->refresher_retry_after;}

	void enable_sip_update_method(bool_t value) {this->enable_sip_update=value;}
	void use_session_timers(int expires) {this->session_expires=expires;}
	void use_dates(bool_t enabled) {this->_use_dates=enabled;}
	void use_one_matching_codec_policy(bool_t one_matching_codec) {this->one_matching_codec=one_matching_codec;}
	void use_rport(bool_t use_rports);
	void enable_auto_contacts(bool_t enabled) {this->auto_contacts=enabled;}
	void enable_test_features(bool_t enabled) {this->_enable_test_features=enabled;}
	void use_no_initial_route(bool_t enabled) {this->no_initial_route=enabled;}
	void enable_unconditional_answer(int value) {belle_sip_provider_enable_unconditional_answer(this->prov,value);}

	bctbx_list_t *get_pending_auths() const {return bctbx_list_copy(this->pending_auths);}

	void set_contact_linphone_specs(const char *specs);

	/**********************/
	/* Network parameters */
	/**********************/
	int set_listen_port(const char *addr, int port, SalTransport tr, bool_t is_tunneled);
	int get_listening_port(SalTransport tr);
	int transport_available(SalTransport t);

	void get_default_local_ip(int address_family, char *ip, size_t iplen);

	void set_transport_timeout(int timeout) {belle_sip_stack_set_transport_timeout(this->stack, timeout);}
	int get_transport_timeout() const {return belle_sip_stack_get_transport_timeout(this->stack);}

	void set_keepalive_period(unsigned int value);
	unsigned int get_keepalive_period() const {return this->keep_alive;}
	void use_tcp_tls_keepalive(bool_t enabled) {this->use_tcp_tls_keep_alive=enabled;}

	void set_dscp(int dscp) {belle_sip_stack_set_default_dscp(this->stack,dscp);}

	int set_tunnel(void *tunnelclient);

	void set_http_proxy_host(const char *host) {belle_sip_stack_set_http_proxy_host(this->stack, host);}
	const char *get_http_proxy_host() const {return belle_sip_stack_get_http_proxy_host(this->stack);}

	void set_http_proxy_port(int port) {belle_sip_stack_set_http_proxy_port(this->stack, port);}
	int get_http_proxy_port() const {return belle_sip_stack_get_http_proxy_port(this->stack);}

	ortp_socket_t get_socket() const;

	int unlisten_ports();
	int reset_transports();


	/******************/
	/* TLS parameters */
	/******************/
	void set_ssl_config(void *ssl_config);
	void set_root_ca(const char* rootCa);
	void set_root_ca_data(const char* data);
	const char *get_root_ca() const {return this->root_ca;}

	void verify_server_certificates(bool_t verify);
	void verify_server_cn(bool_t verify);


	/******************/
	/* DNS resolution */
	/******************/
	void set_dns_timeout(int timeout) {belle_sip_stack_set_dns_timeout(this->stack, timeout);}
	int get_dns_timeout() const {return belle_sip_stack_get_dns_timeout(this->stack);}

	void set_dns_servers(const bctbx_list_t *servers);

	void enable_dns_search(bool_t enable) {belle_sip_stack_enable_dns_search(this->stack, (unsigned char)enable);}
	bool_t dns_search_enabled() const {return (bool_t)belle_sip_stack_dns_search_enabled(this->stack);}

	void enable_dns_srv(bool_t enable) {belle_sip_stack_enable_dns_srv(this->stack, (unsigned char)enable);}
	bool_t dns_srv_enabled() const {return (bool_t)belle_sip_stack_dns_srv_enabled(this->stack);}

	void set_dns_user_hosts_file(const char *hosts_file) {belle_sip_stack_set_dns_user_hosts_file(this->stack, hosts_file);}
	const char *get_dns_user_hosts_file() const {return belle_sip_stack_get_dns_user_hosts_file(this->stack);}

	belle_sip_resolver_context_t *resolve_a(const char *name, int port, int family, belle_sip_resolver_callback_t cb, void *data)
		{return belle_sip_stack_resolve_a(this->stack,name,port,family,cb,data);}
	belle_sip_resolver_context_t *resolve(const char *service, const char *transport, const char *name, int port, int family, belle_sip_resolver_callback_t cb, void *data)
		{return belle_sip_stack_resolve(this->stack, service, transport, name, port, family, cb, data);}


	/**********/
	/* Timers */
	/**********/
	belle_sip_source_t *create_timer(belle_sip_source_func_t func, void *data, unsigned int timeout_value_ms, const char* timer_name);
	void cancel_timer(belle_sip_source_t *timer);


private:
	struct sal_uuid_t {
		unsigned int time_low;
		unsigned short time_mid;
		unsigned short time_hi_and_version;
		unsigned char clock_seq_hi_and_reserved;
		unsigned char clock_seq_low;
		unsigned char node[6];
	};

	void set_tls_properties();
	int add_listen_port(SalAddress* addr, bool_t is_tunneled);
	void make_supported_header();
	void add_pending_auth(SalOp *op);
	void remove_pending_auth(SalOp *op);
	belle_sip_response_t* create_response_from_request (belle_sip_request_t* req, int code );

	static void unimplemented_stub() {ms_warning("Unimplemented SAL callback");}
	static void remove_listening_point(belle_sip_listening_point_t* lp,belle_sip_provider_t* prov) {belle_sip_provider_remove_listening_point(prov,lp);}

	/* Internal callbacks */
	static void process_dialog_terminated_cb(void *sal, const belle_sip_dialog_terminated_event_t *event);
	static void process_io_error_cb(void *user_ctx, const belle_sip_io_error_event_t *event);
	static void process_request_event_cb(void *ud, const belle_sip_request_event_t *event);
	static void process_response_event_cb(void *user_ctx, const belle_sip_response_event_t *event);
	static void process_timeout_cb(void *user_ctx, const belle_sip_timeout_event_t *event);
	static void process_transaction_terminated_cb(void *user_ctx, const belle_sip_transaction_terminated_event_t *event);
	static void process_auth_requested_cb(void *sal, belle_sip_auth_event_t *event);

	MSFactory *factory = NULL;
	Callbacks callbacks = {0};
	MSList *pending_auths = NULL;/*MSList of SalOp */
	belle_sip_stack_t* stack = NULL;
	belle_sip_provider_t *prov = NULL;
	belle_sip_header_user_agent_t* user_agent = NULL;
	belle_sip_listener_t *listener = NULL;
	void *tunnel_client = NULL;
	void *up = NULL; /*user pointer*/
	int session_expires = 0;
	unsigned int keep_alive = 0;
	char *root_ca = NULL;
	char *root_ca_data = NULL;
	char *uuid = NULL;
	int refresher_retry_after = 60000; /*retry after value for refresher*/
	MSList *supported_tags = NULL;/*list of char * */
	belle_sip_header_t *supported = NULL;
	bool_t one_matching_codec = FALSE;
	bool_t use_tcp_tls_keep_alive = FALSE;
	bool_t _nat_helper_enabled = FALSE;
	bool_t tls_verify = TRUE;
	bool_t tls_verify_cn = TRUE;
	bool_t _use_dates = FALSE;
	bool_t auto_contacts = TRUE;
	bool_t _enable_test_features = FALSE;
	bool_t no_initial_route = FALSE;
	bool_t enable_sip_update = TRUE; /*true by default*/
	SalOpSDPHandling default_sdp_handling = SalOpSDPNormal;
	bool_t pending_trans_checking = TRUE; /*testing purpose*/
	void *ssl_config = NULL;
	bctbx_list_t *supported_content_types = NULL; /* list of char* */
	char *linphone_specs = NULL;

	friend class SalOp;
	friend class SalCallOp;
	friend class SalRegisterOp;
	friend class SalMessageOp;
	friend class SalPresenceOp;
	friend class SalSubscribeOp;
	friend class SalPublishOp;
	friend class SalReferOp;
};

int to_sip_code(SalReason r);

LINPHONE_END_NAMESPACE

#endif // ifndef _SAL_H_
