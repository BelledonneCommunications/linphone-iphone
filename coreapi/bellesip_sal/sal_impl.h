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

#ifndef SAL_IMPL_H_
#define SAL_IMPL_H_

#include "sal/sal.h"
#include "belle-sip/belle-sip.h"
#include "belle-sip/belle-sdp.h"

struct Sal{
	MSFactory *factory;
	SalCallbacks callbacks;
	MSList *pending_auths;/*MSList of SalOp */
	belle_sip_stack_t* stack;
	belle_sip_provider_t *prov;
	belle_sip_header_user_agent_t* user_agent;
	belle_sip_listener_t *listener;
	void *tunnel_client;
	void *up; /*user pointer*/
	int session_expires;
	unsigned int keep_alive;
	char *root_ca;
	char *root_ca_data;
	char *uuid;
	int refresher_retry_after; /*retry after value for refresher*/
	MSList *supported_tags;/*list of char * */
	belle_sip_header_t *supported;
	bool_t one_matching_codec;
	bool_t use_tcp_tls_keep_alive;
	bool_t nat_helper_enabled;
	bool_t tls_verify;
	bool_t tls_verify_cn;
	bool_t use_dates;
	bool_t auto_contacts;
	bool_t enable_test_features;
	bool_t no_initial_route;
	bool_t enable_sip_update; /*true by default*/
	SalOpSDPHandling default_sdp_handling;
	bool_t pending_trans_checking; /*testing purpose*/
	void *ssl_config;
};

typedef enum SalOpState {
	SalOpStateEarly=0
	,SalOpStateActive
	,SalOpStateTerminating /*this state is used to wait until a proceeding state, so we can send the cancel*/
	,SalOpStateTerminated
}SalOpState;

const char* sal_op_state_to_string(SalOpState value);

typedef enum SalOpDir {
	SalOpDirIncoming=0
	,SalOpDirOutgoing
}SalOpDir;
typedef enum SalOpType {
	SalOpUnknown,
	SalOpRegister,
	SalOpCall,
	SalOpMessage,
	SalOpPresence,
	SalOpPublish,
	SalOpSubscribe
}SalOpType;

const char* sal_op_type_to_string(SalOpType type);

struct SalOp{
	SalOpBase base;
	const belle_sip_listener_callbacks_t *callbacks;
	SalErrorInfo error_info;
	belle_sip_client_transaction_t *pending_auth_transaction;
	belle_sip_server_transaction_t* pending_server_trans;
	belle_sip_server_transaction_t* pending_update_server_trans;
	belle_sip_client_transaction_t* pending_client_trans;
	SalAuthInfo* auth_info;
	belle_sip_dialog_t* dialog;
	belle_sip_header_replaces_t *replaces;
	belle_sip_header_referred_by_t *referred_by;
	SalMediaDescription *result;
	belle_sdp_session_description_t *sdp_answer;
	SalOpState state;
	SalOpDir dir;
	belle_sip_refresher_t* refresher;
	int ref;
	SalOpType type;
	SalPrivacyMask privacy;
	belle_sip_header_event_t *event; /*used by SalOpSubscribe kinds*/
	SalOpSDPHandling sdp_handling;
	int auth_requests; /*number of auth requested for this op*/
	bool_t cnx_ip_to_0000_if_sendonly_enabled;
	bool_t auto_answer_asked;
	bool_t sdp_offering;
	bool_t call_released;
	bool_t manual_refresher;
	bool_t has_auth_pending;
	bool_t supports_session_timers;
	bool_t op_released;
};


belle_sdp_session_description_t * media_description_to_sdp(const SalMediaDescription *sal);
int sdp_to_media_description(belle_sdp_session_description_t  *sdp, SalMediaDescription *desc);
belle_sip_request_t* sal_op_build_request(SalOp *op,const char* method);


void sal_op_call_fill_cbs(SalOp*op);
void set_or_update_dialog(SalOp* op, belle_sip_dialog_t* dialog);

/*return reffed op*/
SalOp* sal_op_ref(SalOp* op);
/*return null, destroy op if ref count =0*/
void* sal_op_unref(SalOp* op);
void sal_op_release_impl(SalOp *op);

void sal_op_set_replaces(SalOp* op,belle_sip_header_replaces_t* replaces);
void sal_op_set_remote_ua(SalOp*op,belle_sip_message_t* message);
int sal_op_send_request(SalOp* op, belle_sip_request_t* request);
int sal_op_send_request_with_expires(SalOp* op, belle_sip_request_t* request,int expires);
void sal_op_resend_request(SalOp* op, belle_sip_request_t* request);
int sal_op_send_and_create_refresher(SalOp* op,belle_sip_request_t* req, int expires,belle_sip_refresher_listener_t listener );
belle_sip_response_t *sal_op_create_response_from_request(SalOp *op, belle_sip_request_t *req, int code);

/*
 * return true if both from and to uri are sips
 * */
bool_t sal_op_is_secure(const SalOp* op);

void sal_process_authentication(SalOp *op);
belle_sip_header_contact_t* sal_op_create_contact(SalOp *op) ;

bool_t _sal_compute_sal_errors(belle_sip_response_t* response, SalReason* sal_reason, char* reason, size_t reason_size);
SalReason _sal_reason_from_sip_code(int code);

void sal_op_set_error_info_from_response(SalOp *op, belle_sip_response_t *response);
/*presence*/
void sal_op_presence_fill_cbs(SalOp*op);
/*messaging*/
void sal_op_message_fill_cbs(SalOp*op);
void sal_process_incoming_message(SalOp *op,const belle_sip_request_event_t *event);
void sal_op_subscribe_fill_cbs(SalOp*op);

/*call transfer*/
void sal_op_process_refer(SalOp *op, const belle_sip_request_event_t *event, belle_sip_server_transaction_t *tr);
void sal_op_call_process_notify(SalOp *op, const belle_sip_request_event_t *event, belle_sip_server_transaction_t *tr);
/*create SalAuthInfo by copying username and realm from suth event*/
SalAuthInfo* sal_auth_info_create(belle_sip_auth_event_t* event) ;
void sal_add_pending_auth(Sal *sal, SalOp *op);
void sal_remove_pending_auth(Sal *sal, SalOp *op);
void sal_add_presence_info(SalOp *op, belle_sip_message_t *notify, SalPresenceModel *presence);

belle_sip_response_t *sal_create_response_from_request(Sal *sal, belle_sip_request_t *req, int code);

void sal_op_assign_recv_headers(SalOp *op, belle_sip_message_t *incoming);

SalBodyHandler * sal_op_get_body_handler(SalOp *op, belle_sip_message_t *msg);

int sal_reason_to_sip_code(SalReason r);

void _sal_op_add_custom_headers(SalOp *op, belle_sip_message_t *msg);

SalSubscribeStatus belle_sip_message_get_subscription_state(const belle_sip_message_t *msg);

#endif /* SAL_IMPL_H_ */
