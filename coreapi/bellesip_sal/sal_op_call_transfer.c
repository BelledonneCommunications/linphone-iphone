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
#include "offeranswer.h"



/*call transfer*/
static void sal_op_set_replaces(SalOp* op,belle_sip_header_replaces_t* replaces) {
	if (op->replaces){
		belle_sip_object_unref(op->replaces);
	}
	op->replaces=replaces;
	belle_sip_object_ref(op->replaces);
}
static void sal_op_set_referred_by(SalOp* op,belle_sip_header_referred_by_t* referred_by) {
	if (op->referred_by){
		belle_sip_object_unref(op->referred_by);
	}
	op->referred_by=referred_by;
	belle_sip_object_ref(op->referred_by);
}


int sal_call_refer_to(SalOp *op, belle_sip_header_refer_to_t* refer_to, belle_sip_header_referred_by_t* referred_by){
	char* tmp;
	belle_sip_request_t* req=op->dialog?belle_sip_dialog_create_request(op->dialog,"REFER"):NULL; /*cannot create request if dialog not set yet*/
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
	belle_sip_header_refer_to_t* refer_to_header=belle_sip_header_refer_to_create(belle_sip_header_address_parse(refer_to));
	return sal_call_refer_to(op,refer_to_header,NULL);
}

int sal_call_refer_with_replaces(SalOp *op, SalOp *other_call_op){
	belle_sip_dialog_state_t other_call_dialod_state=other_call_op->dialog?belle_sip_dialog_get_state(other_call_op->dialog):BELLE_SIP_DIALOG_NULL;
	belle_sip_header_refer_to_t* refer_to;
	belle_sip_header_referred_by_t* referred_by;

	/*first, build refer to*/
	if (other_call_dialod_state!=BELLE_SIP_DIALOG_CONFIRMED) {
		ms_error(" wrong dialog state [%s] for op [%p], sould be BELLE_SIP_DIALOG_CONFIRMED",belle_sip_dialog_state_to_string(other_call_dialod_state)
																							,other_call_op);
		return -1;
	} else {
		refer_to=belle_sip_header_refer_to_create(belle_sip_dialog_get_remote_party(other_call_op->dialog));
		referred_by=belle_sip_header_referred_by_create(belle_sip_dialog_get_local_party(op->dialog));
	}
	return sal_call_refer_to(op,refer_to,referred_by);
}
int sal_call_accept_refer(SalOp *h){
	ms_fatal("sal_call_accept_refer not implemented yet");
	return -1;
}
/*informs this call is consecutive to an incoming refer */
int sal_call_set_referer(SalOp *h, SalOp *refered_call){
	if (refered_call->replaces)
		sal_op_set_replaces(h,refered_call->replaces);
	if (refered_call->referred_by)
		sal_op_set_referred_by(h,refered_call->referred_by);
	return 0;
}
/* returns the SalOp of a call that should be replaced by h, if any */
SalOp *sal_call_get_replaces(SalOp *h){
	if (h!=NULL && h->replaces!=NULL){
		ms_fatal("sal_call_get_replaces not implemented yet");
	}
	return NULL;
}

static int send_notify_for_refer(SalOp* op, const char *sipfrag){
	belle_sip_request_t* notify=belle_sip_dialog_create_request(op->dialog,"NOTIFY");
	size_t content_length=strlen(sipfrag);
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
									,BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_ACTIVE,-1)));

	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),belle_sip_header_create("Event","refer"));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),BELLE_SIP_HEADER(belle_sip_header_content_type_create("message","sipfrag")));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),BELLE_SIP_HEADER(belle_sip_header_content_length_create(content_length)));
	belle_sip_message_set_body(BELLE_SIP_MESSAGE(notify),sipfrag,content_length);

	return sal_op_send_request(op,notify);
}

int sal_call_notify_refer_state(SalOp *op, SalOp *newcall){
	belle_sip_dialog_state_t state=newcall->dialog?belle_sip_dialog_get_state(newcall->dialog):BELLE_SIP_DIALOG_NULL;
	switch(state) {
	case BELLE_SIP_DIALOG_NULL:
	case BELLE_SIP_DIALOG_EARLY:
		send_notify_for_refer(op,"SIP/2.0 100 Trying\r\n");
		break;
	case BELLE_SIP_DIALOG_CONFIRMED:
		if(send_notify_for_refer(op,"SIP/2.0 200 Ok\r\n")) {
			/* we need previous notify transaction to complete, so buffer the request for later*/
			/*op->sipfrag_pending="SIP/2.0 200 Ok\r\n";*/
			ms_error("Cannot notify 200 ok frag to [%p] for new  op [%p]",op,newcall);
		}
		break;
	default:
		break;
	}
	return 0;
}


void sal_op_process_refer(SalOp *op, const belle_sip_request_event_t *event){
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_header_refer_to_t *refer_to= belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_refer_to_t);;
	belle_sip_header_referred_by_t *referred_by= belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_referred_by_t);;
	belle_sip_server_transaction_t* server_transaction = belle_sip_provider_create_server_transaction(op->base.root->prov,req);
	belle_sip_response_t* resp;
	belle_sip_uri_t* refer_to_uri;
	char* refer_to_uri_str;
	ms_message("Receiving REFER request on op [%p]",op);
	if (refer_to) {
		refer_to_uri=belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(refer_to));

		if (refer_to_uri && belle_sip_uri_get_header(refer_to_uri,"Replaces")) {
			sal_op_set_replaces(op,belle_sip_header_replaces_create2(belle_sip_uri_get_header(refer_to_uri,"Replaces")));
		}
		if (referred_by){
			sal_op_set_referred_by(op,referred_by);
		}
		refer_to_uri_str=belle_sip_uri_to_string(refer_to_uri);
		resp = belle_sip_response_create_from_request(req,202);
		belle_sip_server_transaction_send_response(server_transaction,resp);
		op->base.root->callbacks.refer_received(op->base.root,op,refer_to_uri_str);
		belle_sip_free(refer_to_uri_str);
	} else {
		ms_warning("cannot do anything with the refer without destination\n");
		resp = belle_sip_response_create_from_request(req,501);
		belle_sip_server_transaction_send_response(server_transaction,resp);
	}

}

void sal_op_call_process_notify(SalOp *op, const belle_sip_request_event_t *event){

	belle_sip_server_transaction_t* server_transaction = belle_sip_provider_create_server_transaction(op->base.root->prov,belle_sip_request_event_get_request(event));
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	const char* body = belle_sip_message_get_body(BELLE_SIP_MESSAGE(req));
	belle_sip_header_t* header_event=belle_sip_message_get_header(BELLE_SIP_MESSAGE(req),"Event");
	belle_sip_header_content_type_t* content_type = belle_sip_message_get_header_by_type(req,belle_sip_header_content_type_t);
	belle_sip_response_t* resp;

	ms_message("Receiving NOTIFY request on op [%p]",op);
	if (header_event
		&& strcasecmp(belle_sip_header_extension_get_value(BELLE_SIP_HEADER_EXTENSION(header_event)),"refer")==0
		&& content_type
		&& strcmp(belle_sip_header_content_type_get_type(content_type),"message")==0
		&& strcmp(belle_sip_header_content_type_get_subtype(content_type),"sipfrag")==0
		&& body){
			belle_sip_response_t* sipfrag=BELLE_SIP_RESPONSE(belle_sip_message_parse(body));

			if (sipfrag){

					int code=belle_sip_response_get_status_code(sipfrag);
					SalReferStatus status=SalReferFailed;
					if (code==100){
						status=SalReferTrying;
					}else if (code==200){
						status=SalReferSuccess;
					}else if (code>=400){
						status=SalReferFailed;
					}
					belle_sip_object_unref(sipfrag);
					resp = belle_sip_response_create_from_request(req,200);
					belle_sip_server_transaction_send_response(server_transaction,resp);
					op->base.root->callbacks.notify_refer(op,status);
				}
		}else{
			ms_error("Notify without sipfrag, trashing");
			resp = belle_sip_response_create_from_request(req,501);
			belle_sip_server_transaction_send_response(server_transaction,resp);
		}

}

