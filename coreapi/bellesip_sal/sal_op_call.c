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

static void sdp_process(SalOp *h){
	ms_message("Doing SDP offer/answer process of type %s",h->sdp_offering ? "outgoing" : "incoming");
	if (h->result){
		sal_media_description_unref(h->result);
	}
	h->result=sal_media_description_new();
	if (h->sdp_offering){
		offer_answer_initiate_outgoing(h->base.local_media,h->base.remote_media,h->result);
	}else{
		int i;
		if (h->sdp_answer){
			belle_sip_object_unref(h->sdp_answer);
		}
		offer_answer_initiate_incoming(h->base.local_media,h->base.remote_media,h->result,h->base.root->one_matching_codec);
		h->sdp_answer=(belle_sdp_session_description_t *)belle_sip_object_ref(media_description_to_sdp(h->result));
		/*once we have generated the SDP answer, we modify the result description for processing by the upper layer.
		 It should contains media parameters constraint from the remote offer, not our response*/
		strcpy(h->result->addr,h->base.remote_media->addr);
		h->result->bandwidth=h->base.remote_media->bandwidth;

		for(i=0;i<h->result->n_active_streams;++i){
			/*fixme add rtcp*/
			strcpy(h->result->streams[i].rtp_addr,h->base.remote_media->streams[i].rtp_addr);
			h->result->streams[i].ptime=h->base.remote_media->streams[i].ptime;
			h->result->streams[i].bandwidth=h->base.remote_media->streams[i].bandwidth;
			h->result->streams[i].rtp_port=h->base.remote_media->streams[i].rtp_port;

			if (h->result->streams[i].proto == SalProtoRtpSavp) {
				h->result->streams[i].crypto[0] = h->base.remote_media->streams[i].crypto[0];
			}
		}
	}

}
static int set_sdp(belle_sip_message_t *msg,belle_sdp_session_description_t* session_desc) {
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
static int set_sdp_from_desc(belle_sip_message_t *msg, const SalMediaDescription *desc){
	return set_sdp(msg,media_description_to_sdp(desc));

}
static void call_process_io_error(void *user_ctx, const belle_sip_io_error_event_t *event){
	ms_error("call_process_io_error not implemented yet");
}
static void process_dialog_terminated(void *ctx, const belle_sip_dialog_terminated_event_t *event) {
	SalOp* op=(SalOp*)ctx;

	if (op->dialog)  {
		if (belle_sip_dialog_get_previous_state(op->dialog) == BELLE_SIP_DIALOG_CONFIRMED) {
			/*this is probably a "normal termination from a BYE*/
			op->base.root->callbacks.call_terminated(op,op->dir==SalOpDirIncoming?sal_op_get_from(op):sal_op_get_to(op));
			op->state=SalOpStateTerminated;
		} else {
			/*let the process response handle this case*/
		}

		belle_sip_object_unref(op->dialog);
		op->dialog=NULL;
	}
}
static void handle_sdp_from_response(SalOp* op,belle_sip_response_t* response) {
	belle_sdp_session_description_t* sdp;
	if ((sdp=belle_sdp_session_description_create(BELLE_SIP_MESSAGE(response)))) {
		op->base.remote_media=sal_media_description_new();
		sdp_to_media_description(sdp,op->base.remote_media);
		if (op->base.local_media) sdp_process(op);
	}
}
static void cancelling_invite(SalOp* op ){
	belle_sip_request_t* cancel;
	ms_message("Cancelling INVITE requets from [%s] to [%s] ",sal_op_get_from(op), sal_op_get_to(op));
	cancel = belle_sip_client_transaction_create_cancel(op->pending_inv_client_trans);
	sal_op_send_request(op,cancel);
	op->state=SalOpStateTerminated;
}
static void call_response_event(void *op_base, const belle_sip_response_event_t *event){
	SalOp* op = (SalOp*)op_base;
	belle_sip_request_t* ack;
	belle_sip_dialog_state_t dialog_state;
	/*belle_sip_client_transaction_t* client_transaction = belle_sip_response_event_get_client_transaction(event);*/
	belle_sip_response_t* response=belle_sip_response_event_get_response(event);
	int code = belle_sip_response_get_status_code(response);
	char* reason;
	SalError error=SalErrorUnknown;
	SalReason sr=SalReasonUnknown;
	belle_sip_header_t* reason_header = belle_sip_message_get_header(BELLE_SIP_MESSAGE(response),"Reason");
	reason=(char*)belle_sip_response_get_reason_phrase(response);
	if (reason_header){
		reason = ms_strdup_printf("%s %s",reason,belle_sip_header_extension_get_value(BELLE_SIP_HEADER_EXTENSION(reason_header)));
	}
	if (code >=400) {
		sal_compute_sal_errors_from_code(code,&error,&sr);
		op->base.root->callbacks.call_failure(op,error,sr,reason,code);
		op->state=SalOpStateTerminated;
		if (reason_header != NULL){
			ms_free(reason);
		}
		return;
	}
	set_or_update_dialog(op,belle_sip_response_event_get_dialog(event));

	/*check if op is terminating*/


	if (op->state == SalOpStateTerminating
			&& (!op->dialog
					|| belle_sip_dialog_get_state(op->dialog)==BELLE_SIP_DIALOG_NULL
					|| belle_sip_dialog_get_state(op->dialog)==BELLE_SIP_DIALOG_EARLY)) {
		/*FIXME if DIALOG_CONFIRM then ACK+BYE*/
		cancelling_invite(op);

		return;
	}

	if (!op->dialog) {
		ms_message("call op [%p] receive out of dialog answer [%i]",op,code);
		return;
	}
	dialog_state=belle_sip_dialog_get_state(op->dialog);

		switch(dialog_state) {

		case BELLE_SIP_DIALOG_NULL: {
			ms_error("call op [%p] receive an unexpected answer [%i]",op,code);
			break;
		}
		case BELLE_SIP_DIALOG_EARLY: {
			if (code >= 180) {
				handle_sdp_from_response(op,response);
				op->base.root->callbacks.call_ringing(op);
			} else {
				ms_error("call op [%p] receive an unexpected answer [%i]",op,code);
			}
			break;
		}
		case BELLE_SIP_DIALOG_CONFIRMED: {
			switch (op->state) {
			case SalOpStateEarly:/*invite case*/
			case SalOpStateActive: /*re-invite case*/
				if (code >=200) {
					handle_sdp_from_response(op,response);
					ack=belle_sip_dialog_create_ack(op->dialog,belle_sip_dialog_get_local_seq_number(op->dialog));
					if (ack==NULL) {
						ms_error("This call has been already terminated.");

						return ;
					}
					if (op->sdp_answer){
						set_sdp(BELLE_SIP_MESSAGE(response),op->sdp_answer);
						op->sdp_answer=NULL;
					}
					belle_sip_dialog_send_ack(op->dialog,ack);
					/*if (op->state != SalOpStateActive)*/
					op->base.root->callbacks.call_accepted(op);
					op->state=SalOpStateActive;
				}
			break;

			case SalOpStateTerminated:
			default:
				ms_error("call op [%p] receive answer [%i] not implemented",op,code);
			}
		break;
		}
		case BELLE_SIP_DIALOG_TERMINATED:
		default: {
			ms_error("call op [%p] receive answer [%i] not implemented",op,code);
		}
		/* no break */
	}


}
static void call_process_timeout(void *user_ctx, const belle_sip_timeout_event_t *event) {
	SalOp* op=(SalOp*)user_ctx;
	if (!op->dialog)  {
		/*call terminated very early*/
		op->base.root->callbacks.call_failure(op,SalErrorNoResponse,SalReasonUnknown,"Request Timeout",408);
		op->state=SalOpStateTerminated;
	} else {
		/*dialog will terminated shortly, nothing to do*/
	}
}
static void call_process_transaction_terminated(void *user_ctx, const belle_sip_transaction_terminated_event_t *event) {
	ms_error("process_transaction_terminated not implemented yet");
}
static void call_terminated(SalOp* op,belle_sip_server_transaction_t* server_transaction, belle_sip_request_t* request,int status_code) {
	belle_sip_response_t* resp;
	op->base.root->callbacks.call_terminated(op,op->dir==SalOpDirIncoming?sal_op_get_from(op):sal_op_get_to(op));
	resp=belle_sip_response_create_from_request(request,status_code);
	belle_sip_server_transaction_send_response(server_transaction,resp);

	return;
}
static void unsupported_method(belle_sip_server_transaction_t* server_transaction,belle_sip_request_t* request) {
	belle_sip_response_t* resp;
	resp=belle_sip_response_create_from_request(request,500);
	belle_sip_server_transaction_send_response(server_transaction,resp);
	return;
}

static void process_sdp_for_invite(SalOp* op,belle_sip_request_t* invite) {
	belle_sdp_session_description_t* sdp;
	if ((sdp=belle_sdp_session_description_create(BELLE_SIP_MESSAGE(invite)))) {
		op->sdp_offering=FALSE;
		op->base.remote_media=sal_media_description_new();
		sdp_to_media_description(sdp,op->base.remote_media);
		belle_sip_object_unref(sdp);
	}else
		op->sdp_offering=TRUE;
}
static void process_request_event(void *op_base, const belle_sip_request_event_t *event) {
	SalOp* op = (SalOp*)op_base;
	belle_sip_server_transaction_t* server_transaction = belle_sip_provider_create_server_transaction(op->base.root->prov,belle_sip_request_event_get_request(event));
	if (server_transaction) belle_sip_object_ref(server_transaction); /*ACK does'nt create srv transaction*/
	if (op->pending_server_trans)  belle_sip_object_unref(op->pending_server_trans);
	op->pending_server_trans=server_transaction;
	belle_sdp_session_description_t* sdp;
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_header_t* replace_header;
	belle_sip_dialog_state_t dialog_state;
	belle_sip_response_t* resp;
	belle_sip_header_t* call_info;

	if (!op->dialog) {
		set_or_update_dialog(op,belle_sip_provider_create_dialog(op->base.root->prov,BELLE_SIP_TRANSACTION(op->pending_server_trans)));
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

		process_sdp_for_invite(op,req);

		if ((call_info=belle_sip_message_get_header(BELLE_SIP_MESSAGE(req),"Call-Info"))) {
			if( strstr(belle_sip_header_extension_get_value(BELLE_SIP_HEADER_EXTENSION(call_info)),"answer-after=") != NULL) {
				op->auto_answer_asked=TRUE;
				ms_message("The caller asked to automatically answer the call(Emergency?)\n");
			}
		}

		op->base.root->callbacks.call_received(op);

		break;
	}
	case BELLE_SIP_DIALOG_EARLY: {
		//hmm probably a cancel
		if (strcmp("CANCEL",belle_sip_request_get_method(req))==0) {
			if(belle_sip_request_event_get_server_transaction(event)) {
				/*first answer 200 ok to cancel*/
				belle_sip_server_transaction_send_response(server_transaction
															,belle_sip_response_create_from_request(req,200));
				/*terminate invite request*/
				call_terminated(op
								,belle_sip_request_event_get_server_transaction(event)
								,belle_sip_request_event_get_request(event),487);


			} else {
				/*call leg does not exist*/
				belle_sip_server_transaction_send_response(server_transaction
															,belle_sip_response_create_from_request(req,481));
			}
		} else {
			belle_sip_error("Unexpected method [%s] for dialog state BELLE_SIP_DIALOG_EARLY");
			unsupported_method(server_transaction,req);
		}
		break;
	}
	case BELLE_SIP_DIALOG_CONFIRMED:
		/*great ACK received*/
		if (strcmp("ACK",belle_sip_request_get_method(req))==0) {
			if (op->sdp_offering){
				if ((sdp=belle_sdp_session_description_create(BELLE_SIP_MESSAGE(req)))){
					if (op->base.remote_media)
						sal_media_description_unref(op->base.remote_media);
					op->base.remote_media=sal_media_description_new();
					sdp_to_media_description(sdp,op->base.remote_media);
					sdp_process(op);
					belle_sip_object_unref(sdp);
				}
			}
			/*FIXME
		if (op->reinvite){
			op->reinvite=FALSE;
		}*/
			op->base.root->callbacks.call_ack(op);
		} else if(strcmp("BYE",belle_sip_request_get_method(req))==0) {
			resp=belle_sip_response_create_from_request(req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
			/*call end is notified by dialog deletion*/
		} else if(strcmp("INVITE",belle_sip_request_get_method(req))==0) {
			/*re-invite*/
			if (op->base.remote_media){
				sal_media_description_unref(op->base.remote_media);
				op->base.remote_media=NULL;
			}
			if (op->result){
				sal_media_description_unref(op->result);
				op->result=NULL;
			}
			process_sdp_for_invite(op,req);

			op->base.root->callbacks.call_updating(op);
		} else {
			ms_error("unexpected method [%s] for dialog [%p]",belle_sip_request_get_method(req),op->dialog);
			unsupported_method(server_transaction,req);
		}
		break;
	default: {
		ms_error("unexpected dialog state [%s]",belle_sip_dialog_state_to_string(dialog_state));
	}
	/* no break */
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
static void sal_op_fill_invite(SalOp *op, belle_sip_request_t* invite) {
	belle_sip_header_allow_t* header_allow;
	header_allow = belle_sip_header_allow_create("INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO");
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),BELLE_SIP_HEADER(header_allow));

	if (op->base.root->session_expires!=0){
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),belle_sip_header_create( "Session-expires", "200"));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),belle_sip_header_create( "Supported", "timer"));
	}
	if (op->base.local_media){
			op->sdp_offering=TRUE;
			set_sdp_from_desc(BELLE_SIP_MESSAGE(invite),op->base.local_media);
	}else op->sdp_offering=FALSE;
	return;
}
int sal_call(SalOp *op, const char *from, const char *to){
	belle_sip_request_t* req;
	op->dir=SalOpDirOutgoing;
	sal_op_set_from(op,from);
	sal_op_set_to(op,to);

	req=sal_op_build_request(op,"INVITE");

	sal_op_fill_invite(op,req);

	sal_op_call_fill_cbs(op);
	sal_op_send_request_with_contact(op,req);

	return 0;
}
void sal_op_call_fill_cbs(SalOp*op) {
	op->callbacks.process_io_error=call_process_io_error;
	op->callbacks.process_response_event=call_response_event;
	op->callbacks.process_timeout=call_process_timeout;
	op->callbacks.process_transaction_terminated=call_process_transaction_terminated;
	op->callbacks.process_request_event=process_request_event;
	op->callbacks.process_dialog_terminated=process_dialog_terminated;
}
static void handle_offer_answer_response(SalOp* op, belle_sip_response_t* response) {
	if (op->base.local_media){
		/*this is the case where we received an invite without SDP*/
		if (op->sdp_offering) {
			set_sdp_from_desc(BELLE_SIP_MESSAGE(response),op->base.local_media);
		}else{
			if (op->sdp_answer==NULL) sdp_process(op);
			if (op->sdp_answer){
				set_sdp(BELLE_SIP_MESSAGE(response),op->sdp_answer);
				op->sdp_answer=NULL;
			}
		}
	}else{
		ms_error("You are accepting a call but not defined any media capabilities !");
	}
}
int sal_call_notify_ringing(SalOp *op, bool_t early_media){
	int status_code =early_media?183:180;
	belle_sip_response_t* ringing_response = belle_sip_response_create_from_request(belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(op->pending_server_trans)),status_code);
	if (early_media){
		handle_offer_answer_response(op,ringing_response);
	}
	belle_sip_server_transaction_send_response(op->pending_server_trans,ringing_response);
	return 0;
}


/*accept an incoming call or, during a call accept a reINVITE*/
int sal_call_accept(SalOp*h){
	belle_sip_response_t *response;
	belle_sip_header_address_t* contact= (belle_sip_header_address_t*)sal_op_get_contact_address(h);
	belle_sip_header_contact_t* contact_header;

	if (!h->pending_server_trans) {
		ms_error("No transaction to accept for op [%p]",h);
		return -1;
	}

	/* sends a 200 OK */
	response = belle_sip_response_create_from_request(belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(h->pending_server_trans)),200);

	if (response==NULL){
		ms_error("Fail to build answer for call");
		return -1;
	}
	if (h->base.root->session_expires!=0){
		if (h->supports_session_timers) {
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),belle_sip_header_create( "Supported", "timer"));
		}
	}

	if (contact && (contact_header=belle_sip_header_contact_create(contact))) {
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_HEADER(contact_header));
	}

	handle_offer_answer_response(h,response);

	belle_sip_server_transaction_send_response(h->pending_server_trans,response);
	return 0;
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
	case SalReasonDeclined:
		status=603;
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
int sal_call_update(SalOp *op, const char *subject){
	belle_sip_request_t *reinvite=belle_sip_dialog_create_request(op->dialog,"INVITE");
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(reinvite),belle_sip_header_create( "Subject", subject));
	sal_op_fill_invite(op, reinvite);
	return sal_op_send_request_with_contact(op,reinvite);
}
SalMediaDescription * sal_call_get_remote_media_description(SalOp *h){
	return h->base.remote_media;;
}


SalMediaDescription * sal_call_get_final_media_description(SalOp *h){
	if (h->base.local_media && h->base.remote_media && !h->result){
			sdp_process(h);
	}
	return h->result;
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
	if (h!=NULL && h->replaces!=NULL){
		ms_fatal("sal_call_get_replaces not implemented yet");
	}
	return NULL;
}
int sal_call_send_dtmf(SalOp *h, char dtmf){
	ms_fatal("sal_call_send_dtmf not implemented yet");
	return -1;
}
int sal_call_terminate(SalOp *op){
	belle_sip_dialog_state_t dialog_state=op->dialog?belle_sip_dialog_get_state(op->dialog):BELLE_SIP_DIALOG_NULL; /*no dialog = dialog in NULL state*/
	op->state=SalOpStateTerminating;
	switch(dialog_state) {
		case BELLE_SIP_DIALOG_CONFIRMED: {
			sal_op_send_request(op,belle_sip_dialog_create_request(op->dialog,"BYE"));
			op->state=SalOpStateTerminated;
			break;
		}
		case BELLE_SIP_DIALOG_NULL: {
			if (op->dir == SalOpDirIncoming) {
				sal_call_decline(op, SalReasonDeclined,NULL);
				op->state=SalOpStateTerminated;
			} else if (op->pending_inv_client_trans
					&& belle_sip_transaction_get_state(BELLE_SIP_TRANSACTION(op->pending_inv_client_trans)) == BELLE_SIP_TRANSACTION_PROCEEDING){
				cancelling_invite(op);
				break;
			} else {
				ms_error("Don't know how to termination NUlL dialog for op [%p]",op);
			}
			break;
		}
		case BELLE_SIP_DIALOG_EARLY: {
			if (op->dir == SalOpDirIncoming) {
				sal_call_decline(op, SalReasonDeclined,NULL);
				op->state=SalOpStateTerminated;
			} else  {
				cancelling_invite(op);
			}
			break;
		}
		default: {
			ms_fatal("sal_call_terminate not implemented yet for dialog state [%s]",belle_sip_dialog_state_to_string(dialog_state));
			return -1;
		}
	}
	return 0;
}
bool_t sal_call_autoanswer_asked(SalOp *op){
	return op->auto_answer_asked;
}
void sal_call_send_vfu_request(SalOp *h){
	ms_fatal("sal_call_send_vfu_request not implemented yet");
	return ;
}
int sal_call_is_offerer(const SalOp *h){
	return h->sdp_offering;
}

int sal_call_notify_refer_state(SalOp *h, SalOp *newcall){
	ms_fatal("sal_call_notify_refer_state not implemented yet");
	return -1;
}
void sal_expire_old_registration_contacts(Sal *ctx, bool_t enabled){
	ms_warning("sal_expire_old_registration_contacts not implemented ");
}

void sal_use_dates(Sal *ctx, bool_t enabled){
	ms_warning("sal_use_dates not implemented yet");
}

