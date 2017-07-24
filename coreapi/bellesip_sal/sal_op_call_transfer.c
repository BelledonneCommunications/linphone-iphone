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
#include "sal_impl.h"
#include "offeranswer.h"



/*call transfer*/
static void sal_op_set_referred_by(SalOp* op,belle_sip_header_referred_by_t* referred_by) {
	if (op->referred_by){
		belle_sip_object_unref(op->referred_by);
	}
	op->referred_by=referred_by;
	belle_sip_object_ref(op->referred_by);
}


int sal_call_refer_to(SalOp *op, belle_sip_header_refer_to_t* refer_to, belle_sip_header_referred_by_t* referred_by){
	char* tmp;
	belle_sip_request_t* req=op->dialog?belle_sip_dialog_create_request(op->dialog,"REFER"):sal_op_build_request(op, "REFER");
	if (!req) {
		tmp=belle_sip_uri_to_string(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(refer_to)));
		ms_error("Cannot refer to [%s] for op [%p]",tmp,op);
		belle_sip_free(tmp);
		return -1;
	}
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(refer_to));
	if (referred_by) belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(referred_by));
	return sal_op_send_request(op,req);
}

int sal_call_refer(SalOp *op, const char *refer_to){
	belle_sip_header_address_t *referred_by;
	belle_sip_header_refer_to_t* refer_to_header;
	if (op->dialog) {
		referred_by=(belle_sip_header_address_t*)belle_sip_object_clone(BELLE_SIP_OBJECT(belle_sip_dialog_get_local_party(op->dialog)));
	}else{
		referred_by=BELLE_SIP_HEADER_ADDRESS(sal_op_get_from_address(op));
	}
	refer_to_header=belle_sip_header_refer_to_create(belle_sip_header_address_parse(refer_to));

	return sal_call_refer_to(op,refer_to_header,belle_sip_header_referred_by_create(referred_by));
}

int sal_call_refer_with_replaces(SalOp *op, SalOp *other_call_op){
	belle_sip_dialog_state_t other_call_dialog_state=other_call_op->dialog?belle_sip_dialog_get_state(other_call_op->dialog):BELLE_SIP_DIALOG_NULL;
	belle_sip_dialog_state_t op_dialog_state=op->dialog?belle_sip_dialog_get_state(op->dialog):BELLE_SIP_DIALOG_NULL;
	belle_sip_header_replaces_t* replaces;
	belle_sip_header_refer_to_t* refer_to;
	belle_sip_header_referred_by_t* referred_by;
	const char* from_tag;
	const char* to_tag;
	char* escaped_replaces;
	/*first, build refer to*/
	if ((other_call_dialog_state!=BELLE_SIP_DIALOG_CONFIRMED) && (other_call_dialog_state!=BELLE_SIP_DIALOG_EARLY)) {
		ms_error("wrong dialog state [%s] for op [%p], should be BELLE_SIP_DIALOG_CONFIRMED or BELE_SIP_DIALOG_EARLY",
			belle_sip_dialog_state_to_string(other_call_dialog_state),
			other_call_op);
		return -1;
	}
	if (op_dialog_state!=BELLE_SIP_DIALOG_CONFIRMED) {
		ms_error("wrong dialog state [%s] for op [%p], should be BELLE_SIP_DIALOG_CONFIRMED",
			belle_sip_dialog_state_to_string(op_dialog_state),
			op);
		return -1;
	}

	refer_to=belle_sip_header_refer_to_create(belle_sip_dialog_get_remote_party(other_call_op->dialog));
	belle_sip_parameters_clean(BELLE_SIP_PARAMETERS(refer_to));
	/*rfc3891
	 ...
	 4.  User Agent Client Behavior: Sending a Replaces Header
	 
	 A User Agent that wishes to replace a single existing early or
	 confirmed dialog with a new dialog of its own, MAY send the target
	 User Agent an INVITE request containing a Replaces header field.  The
	 User Agent Client (UAC) places the Call-ID, to-tag, and from-tag
	 information for the target dialog in a single Replaces header field
	 and sends the new INVITE to the target.*/
	from_tag=belle_sip_dialog_get_local_tag(other_call_op->dialog);
	to_tag=belle_sip_dialog_get_remote_tag(other_call_op->dialog);
	
	replaces=belle_sip_header_replaces_create(belle_sip_header_call_id_get_call_id(belle_sip_dialog_get_call_id(other_call_op->dialog))
											,from_tag,to_tag);
	escaped_replaces=belle_sip_header_replaces_value_to_escaped_string(replaces);
	belle_sip_uri_set_header(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(refer_to)),"Replaces",escaped_replaces);
	belle_sip_free(escaped_replaces);
	referred_by=belle_sip_header_referred_by_create(belle_sip_dialog_get_local_party(op->dialog));
	belle_sip_parameters_clean(BELLE_SIP_PARAMETERS(referred_by));
	return sal_call_refer_to(op,refer_to,referred_by);
}
/*
int sal_call_accept_refer(SalOp *h){
	ms_fatal("sal_call_accept_refer not implemented yet");
	return -1;
}*/
/*informs this call is consecutive to an incoming refer */
int sal_call_set_referer(SalOp *h, SalOp *refered_call){
	if (refered_call->replaces)
		sal_op_set_replaces(h,refered_call->replaces);
	if (refered_call->referred_by)
		sal_op_set_referred_by(h,refered_call->referred_by);
	return 0;
}
/* returns the SalOp of a call that should be replaced by h, if any */
SalOp *sal_call_get_replaces(SalOp *op){
	if (op && op->replaces){
		/*rfc3891
		 3.  User Agent Server Behavior: Receiving a Replaces Header
			
		 The Replaces header contains information used to match an existing
		 SIP dialog (call-id, to-tag, and from-tag).  Upon receiving an INVITE
		 with a Replaces header, the User Agent (UA) attempts to match this
		 information with a confirmed or early dialog.  The User Agent Server
		 (UAS) matches the to-tag and from-tag parameters as if they were tags
		 present in an incoming request.  In other words, the to-tag parameter
		 is compared to the local tag, and the from-tag parameter is compared
		 to the remote tag.
		 */
		belle_sip_dialog_t* dialog=belle_sip_provider_find_dialog(op->base.root->prov
								,belle_sip_header_replaces_get_call_id(op->replaces)
								,belle_sip_header_replaces_get_to_tag(op->replaces)
								,belle_sip_header_replaces_get_from_tag(op->replaces));

		if (!dialog) {
			/*for backward compatibility with liblinphone <= 3.10.2-243 */
			dialog=belle_sip_provider_find_dialog(op->base.root->prov
												  ,belle_sip_header_replaces_get_call_id(op->replaces)
												  ,belle_sip_header_replaces_get_from_tag(op->replaces)
												  ,belle_sip_header_replaces_get_to_tag(op->replaces));
		}
		if (dialog) {
			return (SalOp*)belle_sip_dialog_get_application_data(dialog);
		}
	}
	return NULL;
}

static int send_notify_for_refer(SalOp* op, int code, const char *reason){
	belle_sip_request_t* notify=belle_sip_dialog_create_queued_request(op->dialog,"NOTIFY");
	char *sipfrag=belle_sip_strdup_printf("SIP/2.0 %i %s\r\n",code,reason);
	size_t content_length=strlen(sipfrag);
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
		,BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_ACTIVE,-1)));

	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),belle_sip_header_create("Event","refer"));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),BELLE_SIP_HEADER(belle_sip_header_content_type_create("message","sipfrag")));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),BELLE_SIP_HEADER(belle_sip_header_content_length_create(content_length)));
	belle_sip_message_assign_body(BELLE_SIP_MESSAGE(notify),sipfrag,content_length);
	return sal_op_send_request(op,notify);
}

static void notify_last_response(SalOp *op, SalOp *newcall){
	belle_sip_client_transaction_t *tr=newcall->pending_client_trans;
	belle_sip_response_t *resp=NULL;
	if (tr){
		resp=belle_sip_transaction_get_response((belle_sip_transaction_t*)tr);
	}
	if (resp==NULL){
		send_notify_for_refer(op, 100, "Trying");
	}else{
		send_notify_for_refer(op, belle_sip_response_get_status_code(resp), belle_sip_response_get_reason_phrase(resp));
	}
}

int sal_call_notify_refer_state(SalOp *op, SalOp *newcall){
	belle_sip_dialog_state_t state;
	if(belle_sip_dialog_get_state(op->dialog) == BELLE_SIP_DIALOG_TERMINATED){
		return 0;
	}
	state = newcall->dialog?belle_sip_dialog_get_state(newcall->dialog):BELLE_SIP_DIALOG_NULL;
	switch(state) {
		case BELLE_SIP_DIALOG_EARLY:
			send_notify_for_refer(op, 100, "Trying");
			break;
		case BELLE_SIP_DIALOG_CONFIRMED:
			send_notify_for_refer(op, 200, "Ok");
			break;
		case BELLE_SIP_DIALOG_TERMINATED:
		case BELLE_SIP_DIALOG_NULL:
			notify_last_response(op,newcall);
			break;
	}
	return 0;
}


void sal_op_process_refer(SalOp *op, const belle_sip_request_event_t *event, belle_sip_server_transaction_t *server_transaction){
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_header_refer_to_t *refer_to= belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_refer_to_t);
	belle_sip_header_referred_by_t *referred_by= belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_referred_by_t);
	belle_sip_response_t* resp;
	belle_sip_uri_t* refer_to_uri;
	char* refer_to_uri_str;
	
	ms_message("Receiving REFER request on op [%p]",op);
	if (refer_to) {
		refer_to_uri=belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(refer_to));

		if (refer_to_uri && belle_sip_uri_get_header(refer_to_uri,"Replaces")) {
			sal_op_set_replaces(op,belle_sip_header_replaces_create2(belle_sip_uri_get_header(refer_to_uri,"Replaces")));
			belle_sip_uri_remove_header(refer_to_uri,"Replaces");
		}
		if (referred_by){
			sal_op_set_referred_by(op,referred_by);
		}
		refer_to_uri_str=belle_sip_uri_to_string(refer_to_uri);
		resp = sal_op_create_response_from_request(op,req,202);
		belle_sip_server_transaction_send_response(server_transaction,resp);
		op->base.root->callbacks.refer_received(op->base.root,op,refer_to_uri_str);
		belle_sip_free(refer_to_uri_str);
	} else {
		ms_warning("cannot do anything with the refer without destination\n");
		resp = sal_op_create_response_from_request(op,req,400);
		belle_sip_server_transaction_send_response(server_transaction,resp);
	}

}

void sal_op_call_process_notify(SalOp *op, const belle_sip_request_event_t *event, belle_sip_server_transaction_t* server_transaction){
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	const char* body = belle_sip_message_get_body(BELLE_SIP_MESSAGE(req));
	belle_sip_header_t* header_event=belle_sip_message_get_header(BELLE_SIP_MESSAGE(req),"Event");
	belle_sip_header_content_type_t* content_type = belle_sip_message_get_header_by_type(req,belle_sip_header_content_type_t);
	belle_sip_response_t* resp;

	ms_message("Receiving NOTIFY request on op [%p]",op);
	if (header_event
	&& strncasecmp(belle_sip_header_get_unparsed_value(header_event),"refer",strlen("refer"))==0
	&& content_type
	&& strcmp(belle_sip_header_content_type_get_type(content_type),"message")==0
	&& strcmp(belle_sip_header_content_type_get_subtype(content_type),"sipfrag")==0
	&& body){
		belle_sip_response_t* sipfrag=BELLE_SIP_RESPONSE(belle_sip_message_parse(body));

		if (sipfrag){
			int code=belle_sip_response_get_status_code(sipfrag);
			SalReferStatus status=SalReferFailed;
			if (code<200){
				status=SalReferTrying;
			}else if (code<300){
				status=SalReferSuccess;
			}else if (code>=400){
				status=SalReferFailed;
			}
			belle_sip_object_unref(sipfrag);
			resp = sal_op_create_response_from_request(op,req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
			op->base.root->callbacks.notify_refer(op,status);
		}
	}else{
		ms_error("Notify without sipfrag, trashing");
		resp = sal_op_create_response_from_request(op,req,501);
		belle_sip_server_transaction_send_response(server_transaction,resp);
	}
}

