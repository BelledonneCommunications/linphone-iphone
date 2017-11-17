/*
 * op.h
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

#ifndef _SAL_OP_H_
#define _SAL_OP_H_

#include <bctoolbox/list.h>
#include <bctoolbox/port.h>
#include <belle-sip/types.h>

#include "c-wrapper/internal/c-sal.h"
#include "content/content.h"
#include "sal/sal.h"

LINPHONE_BEGIN_NAMESPACE

class SalOp {
public:
	SalOp(Sal *sal);
	virtual ~SalOp();

	SalOp *ref();
	void *unref();

	Sal *get_sal() const {return this->root;}

	void set_user_pointer(void *up) {this->user_pointer=up;}
	void *get_user_pointer() const {return this->user_pointer;}

	void set_subject (const char *subject);
	const char *get_subject () const { return this->subject; }

	void set_from(const char *from);
	void set_from_address(const SalAddress *from);
	const char *get_from() const {return this->from;}
	const SalAddress *get_from_address() const {return this->from_address;}

	void set_to(const char *to);
	void set_to_address(const SalAddress *to);
	const char *get_to() const {return this->to;}
	const SalAddress *get_to_address() const {return this->to_address;}

	void set_contact_address(const SalAddress* address);
	const SalAddress *get_contact_address() const {return this->contact_address;}

	void set_route(const char *route);
	void set_route_address(const SalAddress* address);
	const bctbx_list_t *get_route_addresses() const {return this->route_addresses;}
	void add_route_address(const SalAddress* address);

	void set_diversion_address(const SalAddress *diversion);
	const SalAddress *get_diversion_address() const {return this->diversion_address;}

	void set_service_route(const SalAddress* service_route);
	const SalAddress *get_service_route() const {return this->service_route;}

	void set_manual_refresher_mode(bool_t enabled) {this->manual_refresher=enabled;}

	void set_entity_tag(const char* entity_tag);
	const char *get_entity_tag() const {return this->entity_tag;}

	void set_event(const char *eventname);

	void set_privacy(SalPrivacyMask privacy) {this->privacy=privacy;}
	SalPrivacyMask get_privacy() const {return this->privacy;}

	void set_realm(const char *realm);

	void set_sent_custom_header(SalCustomHeader* ch);

	bool getUseGruuInFrom () { return useGruuInFrom; }
	void setUseGruuInFrom (bool value) { useGruuInFrom = value; }

	void enable_cnx_ip_to_0000_if_sendonly(bool_t yesno) {this->_cnx_ip_to_0000_if_sendonly_enabled = yesno;}
	bool_t cnx_ip_to_0000_if_sendonly_enabled() const {return this->_cnx_ip_to_0000_if_sendonly_enabled;}

	const char *get_proxy() const {return this->route;}
	const char *get_network_origin() const {return this->origin;}
	const char* get_call_id() const {return  this->call_id;}
	char* get_dialog_id() const;
	int get_address_family() const;
	const SalCustomHeader *get_recv_custom_header() const {return this->recv_custom_headers;}
	const char *get_remote_contact() const {return this->remote_contact;}
	const SalAddress *get_remote_contact_address() const {return this->remote_contact_address;}
	const char *get_remote_ua() const {return this->remote_ua;}

	const char *get_public_address(int *port) {return this->refresher ? belle_sip_refresher_get_public_address(this->refresher, port) : NULL;}
	const char *get_local_address(int *port) {return this->refresher ? belle_sip_refresher_get_local_address(this->refresher, port) : NULL;}

	const SalErrorInfo *get_error_info() const {return &this->error_info;}
	const SalErrorInfo *get_reason_error_info() const {return &this->reason_error_info;}

	bool_t is_forked_of(const SalOp *op2) const {return this->call_id && op2->call_id && strcmp(this->call_id, op2->call_id) == 0;}
	bool_t is_idle() const ;

	void stop_refreshing() {if (this->refresher) belle_sip_refresher_stop(this->refresher);}
	int refresh();
	void kill_dialog();
	void release();

	virtual void authenticate(const SalAuthInfo *info) {process_authentication();}
	void cancel_authentication() {ms_fatal("sal_op_cancel_authentication not implemented yet");}
	SalAuthInfo *get_auth_requested() {return this->auth_info;}

	int ping(const char *from, const char *to);
	int send_info(const char *from, const char *to, const SalBodyHandler *body_handler);

protected:
	enum class State {
		Early = 0,
		Active,
		Terminating, /*this state is used to wait until a proceeding state, so we can send the cancel*/
		Terminated
	};

	static const char* to_string(const State value);

	enum class Dir {
		Incoming = 0,
		Outgoing
	};

	enum class Type {
		Unknown,
		Register,
		Call,
		Message,
		Presence,
		Publish,
		Subscribe,
		Refer /*for out of dialog refer only*/
	};

	static const char *to_string(const SalOp::Type type);

	typedef void (*ReleaseCb)(SalOp *op);

	virtual void fill_cbs() {}
	void release_impl();
	void process_authentication();
	int process_redirect();

	belle_sip_request_t* build_request(const char* method);
	int send_request(belle_sip_request_t* request);
	int send_request_with_contact(belle_sip_request_t* request, bool_t add_contact);
	int send_request_with_expires(belle_sip_request_t* request,int expires);
	void resend_request(belle_sip_request_t* request);
	int send_and_create_refresher(belle_sip_request_t* req, int expires,belle_sip_refresher_listener_t listener);

	void set_reason_error_info(belle_sip_message_t *msg);
	void set_error_info_from_response(belle_sip_response_t *response);

	void set_referred_by(belle_sip_header_referred_by_t* referred_by);
	void set_replaces(belle_sip_header_replaces_t* replaces);

	void set_remote_contact(const char* remote_contact);
	void set_network_origin(const char *origin);
	void set_network_origin_address(SalAddress *origin);
	void set_privacy_from_message(belle_sip_message_t* msg);
	void set_remote_ua(belle_sip_message_t* message);

	belle_sip_response_t *create_response_from_request(belle_sip_request_t *req, int code) {return this->root->create_response_from_request(req,code);}
	belle_sip_header_contact_t *create_contact();

	void set_or_update_dialog(belle_sip_dialog_t* dialog);
	belle_sip_dialog_t *link_op_with_dialog(belle_sip_dialog_t* dialog);
	void unlink_op_with_dialog(belle_sip_dialog_t* dialog);

	SalBodyHandler *get_body_handler(belle_sip_message_t *msg);

	void assign_recv_headers(belle_sip_message_t *incoming);

	bool_t is_secure() const;
	void add_headers(belle_sip_header_t *h, belle_sip_message_t *msg);
	void add_custom_headers(belle_sip_message_t *msg);
	int unsubscribe();

	void process_incoming_message(const belle_sip_request_event_t *event);
	int reply_message(SalReason reason);
	void add_message_accept(belle_sip_message_t *msg);
	static bool_t is_external_body(belle_sip_header_content_type_t* content_type);

	static void assign_address(SalAddress** address, const char *value);
	static void assign_string(char **str, const char *arg);
	static void add_initial_route_set(belle_sip_request_t *request, const MSList *list);

	// SalOpBase
	Sal *root = NULL;
	char *route = NULL; /*or request-uri for REGISTER*/
	MSList* route_addresses = NULL; /*list of SalAddress* */
	SalAddress* contact_address = NULL;
	char *subject = NULL;
	char *from = NULL;
	SalAddress* from_address = NULL;
	char *to = NULL;
	SalAddress* to_address = NULL;
	char *origin = NULL;
	SalAddress* origin_address = NULL;
	SalAddress* diversion_address = NULL;
	char *remote_ua = NULL;
	SalAddress* remote_contact_address = NULL;
	char *remote_contact = NULL;
	void *user_pointer = NULL;
	char* call_id = NULL;
	char* realm = NULL;
	SalAddress* service_route = NULL; /*as defined by rfc3608, might be a list*/
	SalCustomHeader *sent_custom_headers = NULL;
	SalCustomHeader *recv_custom_headers = NULL;
	char* entity_tag = NULL; /*as defined by rfc3903 (I.E publih)*/
	ReleaseCb release_cb = NULL;

	// BelleSip implementation
	const belle_sip_listener_callbacks_t *callbacks = NULL;
	SalErrorInfo error_info;
	SalErrorInfo reason_error_info;
	belle_sip_client_transaction_t *pending_auth_transaction = NULL;
	belle_sip_server_transaction_t* pending_server_trans = NULL;
	belle_sip_server_transaction_t* pending_update_server_trans = NULL;
	belle_sip_client_transaction_t* pending_client_trans = NULL;
	SalAuthInfo* auth_info = NULL;
	belle_sip_dialog_t* dialog = NULL;
	belle_sip_header_replaces_t *replaces = NULL;
	belle_sip_header_referred_by_t *referred_by = NULL;
	SalMediaDescription *result = NULL;
	belle_sdp_session_description_t *sdp_answer = NULL;
	State state = State::Early;
	Dir dir = Dir::Incoming;
	belle_sip_refresher_t* refresher = NULL;
	int _ref = 0;
	Type type = Type::Unknown;
	SalPrivacyMask privacy = SalPrivacyNone;
	belle_sip_header_event_t *event = NULL; /*used by SalOpSubscribe kinds*/
	SalOpSDPHandling sdp_handling = SalOpSDPNormal;
	int auth_requests = 0; /*number of auth requested for this op*/
	bool_t _cnx_ip_to_0000_if_sendonly_enabled = FALSE;
	bool_t auto_answer_asked = FALSE;
	bool_t sdp_offering = FALSE;
	bool_t call_released = FALSE;
	bool_t manual_refresher = FALSE;
	bool_t has_auth_pending = FALSE;
	bool_t supports_session_timers = FALSE;
	bool_t op_released = FALSE;
	bool useGruuInFrom = false;

	friend class Sal;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _SAL_OP_H_
