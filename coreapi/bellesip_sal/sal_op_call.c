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

static void call_process_io_error(void *user_ctx, const belle_sip_io_error_event_t *event){
	ms_error("process_io_error not implemented yet");
}
static void call_response_event(void *user_ctx, const belle_sip_response_event_t *event){
	ms_error("response_event not implemented yet");
}
static void call_process_timeout(void *user_ctx, const belle_sip_timeout_event_t *event) {
	ms_error("process_timeout not implemented yet");
}
static void call_process_transaction_terminated(void *user_ctx, const belle_sip_transaction_terminated_event_t *event) {
	ms_error("process_transaction_terminated not implemented yet");
}
static int set_sdp_from_desc(belle_sip_message_t *msg, const SalMediaDescription *desc){
	belle_sdp_session_description_t* session_desc=media_description_to_sdp(desc);
	belle_sip_header_content_type_t* content_type ;
	belle_sip_header_content_length_t* content_length;
	int length;
	char buff[1024];

	if (session_desc) {
		content_type = belle_sip_header_content_type_create("application","sdp");
		length = belle_sip_object_marshal(BELLE_SIP_OBJECT(session_desc),buff,0,sizeof(buff));
		if (length==sizeof(buff)) {
			ms_error("Buffer too small or sdp too big");
		}

		content_length= belle_sip_header_content_length_create(length);
		belle_sip_message_add_header(msg,BELLE_SIP_HEADER(content_type));
		belle_sip_message_add_header(msg,BELLE_SIP_HEADER(content_length));
		belle_sip_message_set_body(msg,buff,length);
		return 0;
	} else {
		return -1;
	}
}
/*Call API*/
int sal_call_set_local_media_description(SalOp *op, SalMediaDescription *desc){
	if (desc)
		sal_media_description_ref(desc);
	if (op->base.local_media)
		sal_media_description_unref(op->base.local_media);
	op->base.local_media=desc;
	return 0;
}
int sal_call(SalOp *op, const char *from, const char *to){
	belle_sip_request_t* req;
	belle_sip_header_allow_t* header_allow;
	belle_sip_client_transaction_t* client_transaction;
	belle_sip_provider_t* prov=op->base.root->prov;
	belle_sip_header_route_t* route_header;

	sal_op_set_from(op,from);
	sal_op_set_to(op,to);

	req=sal_op_build_request(op,"INVITE");
	header_allow = belle_sip_header_allow_create("INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO");
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(header_allow));

	if (op->base.root->session_expires!=0){
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),belle_sip_header_create( "Session-expires", "200"));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),belle_sip_header_create( "Supported", "timer"));
	}
	if (op->base.local_media){
			op->sdp_offering=TRUE;
			set_sdp_from_desc(BELLE_SIP_MESSAGE(req),op->base.local_media);
	}else op->sdp_offering=FALSE;

	if (sal_op_get_route_address(op)) {
		route_header = belle_sip_header_route_create(BELLE_SIP_HEADER_ADDRESS(sal_op_get_route_address(op)));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(route_header));
	}

	op->callbacks.process_io_error=call_process_io_error;
	op->callbacks.process_response_event=call_response_event;
	op->callbacks.process_timeout=call_process_timeout;
	op->callbacks.process_transaction_terminated=call_process_transaction_terminated;
	client_transaction = belle_sip_provider_create_client_transaction(prov,req);
	belle_sip_transaction_set_application_data(BELLE_SIP_TRANSACTION(client_transaction),op);
	belle_sip_client_transaction_send_request(client_transaction);

	return 0;
}
int sal_call_notify_ringing(SalOp *h, bool_t early_media){
	ms_fatal("sal_call_notify_ringing not implemented yet");
	return -1;
}
/*accept an incoming call or, during a call accept a reINVITE*/
int sal_call_accept(SalOp*h){
	ms_fatal("sal_call_accept not implemented yet");
	return -1;
}
int sal_call_decline(SalOp *h, SalReason reason, const char *redirection /*optional*/){
	ms_fatal("sal_call_decline not implemented yet");
	return -1;
}
int sal_call_update(SalOp *h, const char *subject){
	ms_fatal("sal_call_update not implemented yet");
	return -1;
}
SalMediaDescription * sal_call_get_remote_media_description(SalOp *h){
	ms_fatal("sal_call_get_remote_media_description not implemented yet");
	return NULL;
}
SalMediaDescription * sal_call_get_final_media_description(SalOp *h){
	ms_fatal("sal_call_get_final_media_description not implemented yet");
	return NULL;
}
int sal_call_refer(SalOp *h, const char *refer_to){
	ms_fatal("sal_call_refer not implemented yet");
	return -1;
}
int sal_call_refer_with_replaces(SalOp *h, SalOp *other_call_h){
	ms_fatal("sal_call_refer_with_replaces not implemented yet");
	return -1;
}
int sal_call_accept_refer(SalOp *h){
	ms_fatal("sal_call_accept_refer not implemented yet");
	return -1;
}
/*informs this call is consecutive to an incoming refer */
int sal_call_set_referer(SalOp *h, SalOp *refered_call){
	ms_fatal("sal_call_set_referer not implemented yet");
	return -1;
}
/* returns the SalOp of a call that should be replaced by h, if any */
SalOp *sal_call_get_replaces(SalOp *h){
	ms_fatal("sal_call_get_replaces not implemented yet");
	return NULL;
}
int sal_call_send_dtmf(SalOp *h, char dtmf){
	ms_fatal("sal_call_send_dtmf not implemented yet");
	return -1;
}
int sal_call_terminate(SalOp *h){
	ms_fatal("sal_call_terminate not implemented yet");
	return -1;
}
bool_t sal_call_autoanswer_asked(SalOp *op){
	ms_fatal("sal_call_autoanswer_asked not implemented yet");
	return -1;
}
void sal_call_send_vfu_request(SalOp *h){
	ms_fatal("sal_call_send_vfu_request not implemented yet");
	return ;
}
int sal_call_is_offerer(const SalOp *h){
	return h->sdp_offering;
}


