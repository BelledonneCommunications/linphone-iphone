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
static void process_request_event(void *op_base, const belle_sip_request_event_t *event) {
	SalOp* op = (SalOp*)op_base;
	belle_sip_server_transaction_t* server_transaction = belle_sip_provider_create_server_transaction(op->base.root->prov,belle_sip_request_event_get_request(event));
	belle_sip_object_ref(server_transaction);
	if (op->pending_server_trans)  belle_sip_object_unref(op->pending_server_trans);
	op->pending_server_trans=server_transaction;

	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_header_t* replace_header;
	belle_sip_dialog_state_t dialog_state;
	belle_sdp_session_description_t* sdp;
	belle_sip_header_t* call_info;
	if (!op->dialog) {
		op->dialog=belle_sip_provider_create_dialog(op->base.root->prov,BELLE_SIP_TRANSACTION(op->pending_server_trans));
		belle_sip_dialog_set_application_data(op->dialog,op);
		ms_message("new incoming call from [%s] to [%s]",sal_op_get_from(op),sal_op_get_to(op));
	}
	dialog_state=belle_sip_dialog_get_state(op->dialog);
	switch(dialog_state) {

	case BELLE_SIP_DIALOG_NULL: {
		if (!op->replaces && (replace_header=belle_sip_message_get_header(BELLE_SIP_MESSAGE(req),"replaces"))) {
			op->replaces=belle_sip_header_address_parse(belle_sip_header_extension_get_value(BELLE_SIP_HEADER_EXTENSION(replace_header)));
			belle_sip_object_ref(op->replaces);
		} else if(op->replaces) {
			ms_warning("replace header already set");
		}
		if ((sdp=belle_sdp_session_description_create(BELLE_SIP_MESSAGE(req)))) {
			op->sdp_offering=FALSE;
			op->base.remote_media=sal_media_description_new();
			sdp_to_media_description(sdp,op->base.remote_media);
			belle_sip_object_unref(sdp);
		}else
			op->sdp_offering=TRUE;

		if ((call_info=belle_sip_message_get_header(BELLE_SIP_MESSAGE(req),"Call-Info"))) {
			if( strstr(belle_sip_header_extension_get_value(BELLE_SIP_HEADER_EXTENSION(call_info)),"answer-after=") != NULL) {
				op->auto_answer_asked=TRUE;
				ms_message("The caller asked to automatically answer the call(Emergency?)\n");
			}
		}

		op->base.root->callbacks.call_received(op);

		break;
	}
	default: {
		ms_error("unexpected dialog state [%s]",belle_sip_dialog_state_to_string(dialog_state));
	}
	}


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
	sal_op_call_fill_cbs(op);
	client_transaction = belle_sip_provider_create_client_transaction(prov,req);
	belle_sip_transaction_set_application_data(BELLE_SIP_TRANSACTION(client_transaction),op);
	op->dialog=belle_sip_provider_create_dialog(prov,BELLE_SIP_TRANSACTION(client_transaction));
	belle_sip_client_transaction_send_request(client_transaction);

	return 0;
}
void sal_op_call_fill_cbs(SalOp*op) {
	op->callbacks.process_io_error=call_process_io_error;
	op->callbacks.process_response_event=call_response_event;
	op->callbacks.process_timeout=call_process_timeout;
	op->callbacks.process_transaction_terminated=call_process_transaction_terminated;
	op->callbacks.process_request_event=process_request_event;
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
int sal_call_decline(SalOp *op, SalReason reason, const char *redirection /*optional*/){
	belle_sip_response_t* response;
	belle_sip_header_contact_t* contact=NULL;
	int status;
	switch(reason) {
	case SalReasonBusy:
		status=486;
		break;
	case SalReasonTemporarilyUnavailable:
		status=480;
		break;
	case SalReasonDoNotDisturb:
		status=600;
		break;
	case SalReasonMedia:
		status=415;
		break;
	case SalReasonRedirect:
		if(redirection!=NULL) {
			if (strstr(redirection,"sip:")!=0) status=302;
			status=380;
			contact= belle_sip_header_contact_new();
			belle_sip_header_address_set_uri(BELLE_SIP_HEADER_ADDRESS(contact),belle_sip_uri_parse(redirection));
			break;
		} else {
			ms_error("Cannot redirect to null");
		}
		/* no break */

	default:
		status=500;
		ms_error("Unexpected decline reason [%i]",reason);
		/* no break */
	}
	response = belle_sip_response_create_from_request(belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(op->pending_server_trans)),status);
	if (contact) belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_HEADER(contact));
	belle_sip_server_transaction_send_response(op->pending_server_trans,response);
	return 0;

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


