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

static int extract_sdp(belle_sip_message_t* message,belle_sdp_session_description_t** session_desc, SalReason *error);

/*used for calls terminated before creation of a dialog*/
static void call_set_released(SalOp* op){
	if (!op->call_released){
		op->state=SalOpStateTerminated;
		op->base.root->callbacks.call_released(op);
		op->call_released=TRUE;
	}
}

/*used when the SalOp was ref'd by the dialog, in which case we rely only on the dialog terminated notification*/
static void call_set_released_and_unref(SalOp* op) {
	call_set_released(op);
	sal_op_unref(op);
}


static void call_set_error(SalOp* op,belle_sip_response_t* response){
	SalError error=SalErrorUnknown;
	SalReason sr=SalReasonUnknown;
	belle_sip_header_t* reason_header = belle_sip_message_get_header(BELLE_SIP_MESSAGE(response),"Reason");
	char* reason=(char*)belle_sip_response_get_reason_phrase(response);
	int code = belle_sip_response_get_status_code(response);
	if (reason_header){
		reason = ms_strdup_printf("%s %s",reason,belle_sip_header_get_unparsed_value(reason_header));
	}
	sal_compute_sal_errors_from_code(code,&error,&sr);
	op->base.root->callbacks.call_failure(op,error,sr,reason,code);
	if (reason_header != NULL){
		ms_free(reason);
	}
}

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
			strcpy(h->result->streams[i].rtp_addr,h->base.remote_media->streams[i].rtp_addr);
			h->result->streams[i].ptime=h->base.remote_media->streams[i].ptime;
			h->result->streams[i].bandwidth=h->base.remote_media->streams[i].bandwidth;
			h->result->streams[i].rtp_port=h->base.remote_media->streams[i].rtp_port;
			strcpy(h->result->streams[i].rtcp_addr,h->base.remote_media->streams[i].rtcp_addr);
			h->result->streams[i].rtcp_port=h->base.remote_media->streams[i].rtcp_port;

			if (h->result->streams[i].proto == SalProtoRtpSavp) {
				h->result->streams[i].crypto[0] = h->base.remote_media->streams[i].crypto[0];
			}
		}
	}
}

static int set_sdp(belle_sip_message_t *msg,belle_sdp_session_description_t* session_desc) {
	belle_sip_header_content_type_t* content_type ;
	belle_sip_header_content_length_t* content_length;
	belle_sip_error_code error = BELLE_SIP_OK;
	size_t length = 0;
	char buff[2048];

	if (session_desc) {
		content_type = belle_sip_header_content_type_create("application","sdp");
		error = belle_sip_object_marshal(BELLE_SIP_OBJECT(session_desc),buff,sizeof(buff),&length);
		if (error != BELLE_SIP_OK) {
			ms_error("Buffer too small or sdp too big");
			return -1;
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
	int err;
	belle_sdp_session_description_t *sdp=media_description_to_sdp(desc);
	err=set_sdp(msg,sdp);
	belle_sip_object_unref(sdp);
	return err;

}
static void call_process_io_error(void *user_ctx, const belle_sip_io_error_event_t *event){
	SalOp* op=(SalOp*)user_ctx;
	
	if (op->state==SalOpStateTerminated) return;
	
	if (!op->dialog)  {
		/*call terminated very early*/
		op->base.root->callbacks.call_failure(op,SalErrorNoResponse,SalReasonUnknown,"Service Unavailable",503);
		call_set_released(op);
	} else {
		/*dialog will terminated shortly, nothing to do*/
	}
}
static void process_dialog_terminated(void *ctx, const belle_sip_dialog_terminated_event_t *event) {
	SalOp* op=(SalOp*)ctx;

	if (op->dialog && op->dialog==belle_sip_dialog_terminated_event_get_dialog(event))  {
		/*belle_sip_transaction_t* trans=belle_sip_dialog_get_last_transaction(op->dialog);*/
		ms_message("Dialog [%p] terminated for op [%p]",belle_sip_dialog_terminated_event_get_dialog(event),op);
		
		switch(belle_sip_dialog_get_previous_state(op->dialog)) {
			case BELLE_SIP_DIALOG_CONFIRMED:
				if (op->state!=SalOpStateTerminated && op->state!=SalOpStateTerminating) {
					/*this is probably a normal termination from a BYE*/
					op->base.root->callbacks.call_terminated(op,op->dir==SalOpDirIncoming?sal_op_get_from(op):sal_op_get_to(op));
					op->state=SalOpStateTerminating;
				}
			break;
			default:
			break;
		}
		belle_sip_main_loop_do_later(belle_sip_stack_get_main_loop(op->base.root->stack)
							,(belle_sip_callback_t) call_set_released_and_unref
							, op);
	} else {
		ms_error("dialog unknown for op ");
	}
}

static void handle_sdp_from_response(SalOp* op,belle_sip_response_t* response) {
	belle_sdp_session_description_t* sdp;
	SalReason reason;
	if (extract_sdp(BELLE_SIP_MESSAGE(response),&sdp,&reason)==0) {
		if (sdp){
			op->base.remote_media=sal_media_description_new();
			sdp_to_media_description(sdp,op->base.remote_media);
			if (op->base.local_media) sdp_process(op);
		}/*if no sdp in response, what can we do ?*/
	}
}

static void cancelling_invite(SalOp* op ){
	belle_sip_request_t* cancel;
	ms_message("Cancelling INVITE request from [%s] to [%s] ",sal_op_get_from(op), sal_op_get_to(op));
	cancel = belle_sip_client_transaction_create_cancel(op->pending_client_trans);
	sal_op_send_request(op,cancel);
	op->state=SalOpStateTerminating;
}

static void call_process_response(void *op_base, const belle_sip_response_event_t *event){
	SalOp* op = (SalOp*)op_base;
	belle_sip_request_t* ack;
	belle_sip_dialog_state_t dialog_state;
	belle_sip_client_transaction_t* client_transaction = belle_sip_response_event_get_client_transaction(event);
	belle_sip_request_t* req;
	belle_sip_response_t* response=belle_sip_response_event_get_response(event);
	int code = belle_sip_response_get_status_code(response);


	if (!client_transaction) {
		ms_warning("Discarding stateless response [%i] on op [%p]",code,op);
		return;
	}
	req=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
	set_or_update_dialog(op,belle_sip_response_event_get_dialog(event));
	dialog_state=op->dialog?belle_sip_dialog_get_state(op->dialog):BELLE_SIP_DIALOG_NULL;
	
	ms_message("Op [%p] receiving call response [%i], dialog is [%p] in state [%s]",op,code,op->dialog,belle_sip_dialog_state_to_string(dialog_state));

	switch(dialog_state) {
		case BELLE_SIP_DIALOG_NULL:
		case BELLE_SIP_DIALOG_EARLY: {
			if (strcmp("INVITE",belle_sip_request_get_method(req))==0 ) {
				if (op->state == SalOpStateTerminating) {
					/*check if CANCEL was sent before*/
					if (strcmp("CANCEL",belle_sip_request_get_method(belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(op->pending_client_trans))))!=0) {
						/*it wasn't sent */
						if (code<200) {
							cancelling_invite(op);
						}else{
							/* no need to send the INVITE because the UAS rejected the INVITE*/
							if (op->dialog==NULL) call_set_released(op);
						}
					} else {
						/*it was sent already, so just expect the 487 or any error response to send the call_released() notification*/
						if (code>=300){
							if (op->dialog==NULL) call_set_released(op);
						}
					}
				} else if (code >= 180 && code<300) {
					handle_sdp_from_response(op,response);
					op->base.root->callbacks.call_ringing(op);
				} else if (code>=300){
					call_set_error(op,response);
					if (op->dialog==NULL) call_set_released(op);
				}
			}
		}
		break;
		case BELLE_SIP_DIALOG_CONFIRMED: {
			switch (op->state) {
				case SalOpStateEarly:/*invite case*/
				case SalOpStateActive: /*re-invite case*/
					if (code >=200
						&& code<300
						&& strcmp("INVITE",belle_sip_request_get_method(req))==0) {
						handle_sdp_from_response(op,response);
						ack=belle_sip_dialog_create_ack(op->dialog,belle_sip_dialog_get_local_seq_number(op->dialog));
						if (ack==NULL) {
							ms_error("This call has been already terminated.");
							return ;
						}
						if (op->sdp_answer){
							set_sdp(BELLE_SIP_MESSAGE(ack),op->sdp_answer);
							belle_sip_object_unref(op->sdp_answer);
							op->sdp_answer=NULL;
						}
						belle_sip_dialog_send_ack(op->dialog,ack);
						op->base.root->callbacks.call_accepted(op); /*INVITE*/
						op->state=SalOpStateActive;
					}  else if (code >= 300 && strcmp("INVITE",belle_sip_request_get_method(req))==0){
						call_set_error(op,response);
					} else {
							/*ignoring*/
					}
				break;
				case SalOpStateTerminating:
					sal_op_send_request(op,belle_sip_dialog_create_request(op->dialog,"BYE"));
				break;
				case SalOpStateTerminated:
				default:
					ms_error("Call op [%p] receives unexpected answer [%i] while in state [%s].",op,code, sal_op_state_to_string(op->state));
			}
		}
		break;
		case BELLE_SIP_DIALOG_TERMINATED: {
			if (code >= 300){
				call_set_error(op,response);
			}
		}
		break;
		default: {
			ms_error("call op [%p] receive answer [%i] not implemented",op,code);
		}
		break;
	}
}

static void call_process_timeout(void *user_ctx, const belle_sip_timeout_event_t *event) {
	SalOp* op=(SalOp*)user_ctx;
	
	if (op->state==SalOpStateTerminated) return;
	
	if (!op->dialog)  {
		/*call terminated very early*/
		op->base.root->callbacks.call_failure(op,SalErrorNoResponse,SalReasonUnknown,"Request Timeout",408);
		call_set_released(op);
	} else {
		/*dialog will terminated shortly, nothing to do*/
	}
}

static void call_process_transaction_terminated(void *user_ctx, const belle_sip_transaction_terminated_event_t *event) {
	SalOp* op = (SalOp*)user_ctx;
	belle_sip_client_transaction_t *client_transaction=belle_sip_transaction_terminated_event_get_client_transaction(event);
	belle_sip_server_transaction_t *server_transaction=belle_sip_transaction_terminated_event_get_server_transaction(event);
	belle_sip_request_t* req;
	belle_sip_response_t* resp;
	if (client_transaction) {
		req=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
		resp=belle_sip_transaction_get_response(BELLE_SIP_TRANSACTION(client_transaction));
	} else {
		req=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(server_transaction));
		resp=belle_sip_transaction_get_response(BELLE_SIP_TRANSACTION(server_transaction));
	}
	if (op->state ==SalOpStateTerminating
			&& strcmp("BYE",belle_sip_request_get_method(req))==0
			&& (!resp || (belle_sip_response_get_status_code(resp) !=401
			&& belle_sip_response_get_status_code(resp) !=407))) {
		if (op->dialog==NULL) call_set_released(op);
	}
}

static void call_terminated(SalOp* op,belle_sip_server_transaction_t* server_transaction, belle_sip_request_t* request,int status_code) {
	belle_sip_response_t* resp;
	op->base.root->callbacks.call_terminated(op,op->dir==SalOpDirIncoming?sal_op_get_from(op):sal_op_get_to(op));
	resp=sal_op_create_response_from_request(op,request,status_code);
	belle_sip_server_transaction_send_response(server_transaction,resp);
}

static void unsupported_method(belle_sip_server_transaction_t* server_transaction,belle_sip_request_t* request) {
	belle_sip_response_t* resp;
	resp=belle_sip_response_create_from_request(request,500);
	belle_sip_server_transaction_send_response(server_transaction,resp);
	return;
}

/*
 * Extract the sdp from a sip message.
 * If there is no body in the message, the session_desc is set to null, 0 is returned.
 * If body was present is not a SDP or parsing of SDP failed, -1 is returned and SalReason is set appropriately.
 * 
**/
static int extract_sdp(belle_sip_message_t* message,belle_sdp_session_description_t** session_desc, SalReason *error) {
	belle_sip_header_content_type_t* content_type=belle_sip_message_get_header_by_type(message,belle_sip_header_content_type_t);
	if (content_type){
		if (strcmp("application",belle_sip_header_content_type_get_type(content_type))==0
			&& strcmp("sdp",belle_sip_header_content_type_get_subtype(content_type))==0) {
			*session_desc=belle_sdp_session_description_parse(belle_sip_message_get_body(message));
			if (*session_desc==NULL) {
				ms_error("Failed to parse SDP message.");
				*error=SalReasonNotAcceptable;
				return -1;
			}
		}else{
			*error=SalReasonUnsupportedContent;
			return -1;
		}
	}else *session_desc=NULL;
	return 0;
}

static int is_media_description_acceptable(SalMediaDescription *md){
	if (md->n_total_streams==0){
		ms_warning("Media description does not define any stream.");
		return FALSE;
	}
	return TRUE;
}

static int process_sdp_for_invite(SalOp* op,belle_sip_request_t* invite) {
	belle_sdp_session_description_t* sdp;
	int err=0;
	SalReason reason;
	if (extract_sdp(BELLE_SIP_MESSAGE(invite),&sdp,&reason)==0) {
		if (sdp){
			op->sdp_offering=FALSE;
			op->base.remote_media=sal_media_description_new();
			sdp_to_media_description(sdp,op->base.remote_media);
			/*make some sanity check about the SDP received*/
			if (!is_media_description_acceptable(op->base.remote_media)){
				err=-1;
				reason=SalReasonNotAcceptable;
			}
			belle_sip_object_unref(sdp);
		}else op->sdp_offering=TRUE; /*INVITE without SDP*/
	}else err=-1;
	
	if (err==-1){
		sal_call_decline(op,reason,NULL);
	}
	return err;
}

static void process_request_event(void *op_base, const belle_sip_request_event_t *event) {
	SalOp* op = (SalOp*)op_base;
	belle_sip_server_transaction_t* server_transaction=NULL;
	belle_sdp_session_description_t* sdp;
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_dialog_state_t dialog_state;
	belle_sip_response_t* resp;
	belle_sip_header_t* call_info;

	if (strcmp("ACK",belle_sip_request_get_method(req))!=0){  /*ACK does'nt create srv transaction*/
		server_transaction = belle_sip_provider_create_server_transaction(op->base.root->prov,belle_sip_request_event_get_request(event));
		belle_sip_object_ref(server_transaction);
		belle_sip_transaction_set_application_data(BELLE_SIP_TRANSACTION(server_transaction),op);
		sal_op_ref(op);
	}

	if (strcmp("INVITE",belle_sip_request_get_method(req))==0) {
		if (op->pending_server_trans) belle_sip_object_unref(op->pending_server_trans);
		/*updating pending invite transaction*/
		op->pending_server_trans=server_transaction;
		belle_sip_object_ref(op->pending_server_trans);
	}

	if (!op->dialog) {
		set_or_update_dialog(op,belle_sip_provider_create_dialog(op->base.root->prov,BELLE_SIP_TRANSACTION(op->pending_server_trans)));
		ms_message("new incoming call from [%s] to [%s]",sal_op_get_from(op),sal_op_get_to(op));
	}
	dialog_state=belle_sip_dialog_get_state(op->dialog);
	switch(dialog_state) {
	case BELLE_SIP_DIALOG_NULL: {
		if (strcmp("INVITE",belle_sip_request_get_method(req))==0) {
			if (!op->replaces && (op->replaces=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_replaces_t))) {
				belle_sip_object_ref(op->replaces);
			} else if(op->replaces) {
				ms_warning("replace header already set");
			}

			process_sdp_for_invite(op,req);

			if ((call_info=belle_sip_message_get_header(BELLE_SIP_MESSAGE(req),"Call-Info"))) {
				if( strstr(belle_sip_header_get_unparsed_value(call_info),"answer-after=") != NULL) {
					op->auto_answer_asked=TRUE;
					ms_message("The caller asked to automatically answer the call(Emergency?)\n");
				}
			}

			op->base.root->callbacks.call_received(op);

			break;
		} /* else same behavior as for EARLY state*/
	}
	case BELLE_SIP_DIALOG_EARLY: {
		//hmm probably a cancel
		if (strcmp("CANCEL",belle_sip_request_get_method(req))==0) {
			if(belle_sip_request_event_get_server_transaction(event)) {
				/*first answer 200 ok to cancel*/
				belle_sip_server_transaction_send_response(server_transaction
						,sal_op_create_response_from_request(op,req,200));
				/*terminate invite transaction*/
				call_terminated(op
						,op->pending_server_trans
						,belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(op->pending_server_trans)),487);


			} else {
				/*call leg does not exist*/
				belle_sip_server_transaction_send_response(server_transaction
							,sal_op_create_response_from_request(op,req,481));
			}
		} else if (strcmp("PRACK",belle_sip_request_get_method(req))==0) {
			resp=sal_op_create_response_from_request(op,req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
		} else {
			belle_sip_error("Unexpected method [%s] for dialog state BELLE_SIP_DIALOG_EARLY",belle_sip_request_get_method(req));
			unsupported_method(server_transaction,req);
		}
		break;
	}
	case BELLE_SIP_DIALOG_CONFIRMED:
		/*great ACK received*/
		if (strcmp("ACK",belle_sip_request_get_method(req))==0) {
			if (op->sdp_offering){
				SalReason reason;
				if (extract_sdp(BELLE_SIP_MESSAGE(req),&sdp,&reason)==0){
					if (sdp){
						if (op->base.remote_media)
							sal_media_description_unref(op->base.remote_media);
						op->base.remote_media=sal_media_description_new();
						sdp_to_media_description(sdp,op->base.remote_media);
						sdp_process(op);
						belle_sip_object_unref(sdp);
					}else{
						ms_warning("SDP expected in ACK but not found.");
					}
				}
			}
			/*FIXME
		if (op->reinvite){
			op->reinvite=FALSE;
		}*/
			op->base.root->callbacks.call_ack(op);
		} else if(strcmp("BYE",belle_sip_request_get_method(req))==0) {
			resp=sal_op_create_response_from_request(op,req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
			op->base.root->callbacks.call_terminated(op,op->dir==SalOpDirIncoming?sal_op_get_from(op):sal_op_get_to(op));
			op->state=SalOpStateTerminating;
			/*call end not notified by dialog deletion because transaction can end before dialog*/
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
			if (process_sdp_for_invite(op,req)==0)
				op->base.root->callbacks.call_updating(op);
		} else if (strcmp("INFO",belle_sip_request_get_method(req))==0){
			if (belle_sip_message_get_body(BELLE_SIP_MESSAGE(req))
				&&	strstr(belle_sip_message_get_body(BELLE_SIP_MESSAGE(req)),"picture_fast_update")) {
				/*vfu request*/
				ms_message("Receiving VFU request on op [%p]",op);
				if (op->base.root->callbacks.vfu_request){
					op->base.root->callbacks.vfu_request(op);

				}
			}else{
				SalBody salbody;
				if (sal_op_get_body(op,(belle_sip_message_t*)req,&salbody)) {
					if (sal_body_has_type(&salbody,"application","dtmf-relay")){
						char tmp[10];
						if (sal_lines_get_value(salbody.data, "Signal",tmp, sizeof(tmp))){
							op->base.root->callbacks.dtmf_received(op,tmp[0]);
						}
					}else
						op->base.root->callbacks.info_received(op,&salbody);
				} else {
					op->base.root->callbacks.info_received(op,NULL);
				}
			}
			resp=sal_op_create_response_from_request(op,req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
		}else if (strcmp("REFER",belle_sip_request_get_method(req))==0) {
			sal_op_process_refer(op,event,server_transaction);
		} else if (strcmp("NOTIFY",belle_sip_request_get_method(req))==0) {
			sal_op_call_process_notify(op,event,server_transaction);
		} else if (strcmp("OPTIONS",belle_sip_request_get_method(req))==0) {
			resp=sal_op_create_response_from_request(op,req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
		} else if (strcmp("CANCEL",belle_sip_request_get_method(req))==0) {
			/*call leg does not exist because 200ok already sent*/
			belle_sip_server_transaction_send_response(	server_transaction
														,sal_op_create_response_from_request(op,req,481));

		} else{
			ms_error("unexpected method [%s] for dialog [%p]",belle_sip_request_get_method(req),op->dialog);
			unsupported_method(server_transaction,req);
		}
		break;
	default:
		ms_error("unexpected dialog state [%s]",belle_sip_dialog_state_to_string(dialog_state));
		break;
	}

	if (server_transaction) belle_sip_object_unref(server_transaction);

}


/*Call API*/
int sal_call_set_local_media_description(SalOp *op, SalMediaDescription *desc){
	if (desc)
		sal_media_description_ref(desc);
	if (op->base.local_media)
		sal_media_description_unref(op->base.local_media);
	op->base.local_media=desc;
	
	if (op->base.remote_media){
		/*case of an incoming call where we modify the local capabilities between the time
		 * the call is ringing and it is accepted (for example if you want to accept without video*/
		/*reset the sdp answer so that it is computed again*/
		if (op->sdp_answer){
			belle_sip_object_unref(op->sdp_answer);
			op->sdp_answer=NULL;
		}
	}
	return 0;
}

static belle_sip_header_allow_t *create_allow(){
	belle_sip_header_allow_t* header_allow;
        header_allow = belle_sip_header_allow_create("INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO");
	return header_allow;
}

static void sal_op_fill_invite(SalOp *op, belle_sip_request_t* invite) {
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),BELLE_SIP_HEADER(create_allow()));

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
	belle_sip_request_t* invite;
	op->dir=SalOpDirOutgoing;

	sal_op_set_from(op,from);
	sal_op_set_to(op,to);

	ms_message("[%s] calling [%s] on op [%p]", from, to, op);
	invite=sal_op_build_request(op,"INVITE");

	sal_op_fill_invite(op,invite);

	sal_op_call_fill_cbs(op);
	if (op->replaces){
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),BELLE_SIP_HEADER(op->replaces));
	}
	if (op->referred_by)
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),BELLE_SIP_HEADER(op->referred_by));
	
	return sal_op_send_request(op,invite);
}

void sal_op_call_fill_cbs(SalOp*op) {
	op->callbacks.process_io_error=call_process_io_error;
	op->callbacks.process_response_event=call_process_response;
	op->callbacks.process_timeout=call_process_timeout;
	op->callbacks.process_transaction_terminated=call_process_transaction_terminated;
	op->callbacks.process_request_event=process_request_event;
	op->callbacks.process_dialog_terminated=process_dialog_terminated;
	op->type=SalOpCall;
}

static void handle_offer_answer_response(SalOp* op, belle_sip_response_t* response) {
	if (op->base.local_media){
		/*this is the case where we received an invite without SDP*/
		if (op->sdp_offering) {
			set_sdp_from_desc(BELLE_SIP_MESSAGE(response),op->base.local_media);
		}else{

			if (op->sdp_answer==NULL)
				sdp_process(op);

			if (op->sdp_answer){
				set_sdp(BELLE_SIP_MESSAGE(response),op->sdp_answer);
				belle_sip_object_unref(op->sdp_answer);
				op->sdp_answer=NULL;
			}
		}
	}else{
		ms_error("You are accepting a call but not defined any media capabilities !");
	}
}

int sal_call_notify_ringing(SalOp *op, bool_t early_media){
	int status_code =early_media?183:180;
	belle_sip_request_t* req=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(op->pending_server_trans));
	belle_sip_response_t* ringing_response = sal_op_create_response_from_request(op,req,status_code);
	belle_sip_header_t *require;
	const char *tags=NULL;
	
	if (early_media){
		handle_offer_answer_response(op,ringing_response);
	}
	require=belle_sip_message_get_header((belle_sip_message_t*)req,"Require");
	if (require) tags=belle_sip_header_get_unparsed_value(require);
	/* if client requires 100rel, then add necessary stuff*/
	if (tags && strstr(tags,"100rel")!=0) {
		belle_sip_message_add_header((belle_sip_message_t*)ringing_response,belle_sip_header_create("Require","100rel"));
		belle_sip_message_add_header((belle_sip_message_t*)ringing_response,belle_sip_header_create("RSeq","1"));
	}

#ifndef SAL_OP_CALL_FORCE_CONTACT_IN_RINGING
	if (tags && strstr(tags,"100rel")!=0) 
#endif
	{
		belle_sip_header_address_t* contact= (belle_sip_header_address_t*)sal_op_get_contact_address(op);
		belle_sip_header_contact_t* contact_header;
		if (contact && (contact_header=belle_sip_header_contact_create(contact))) {
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(ringing_response),BELLE_SIP_HEADER(contact_header));
		}
	}
	belle_sip_server_transaction_send_response(op->pending_server_trans,ringing_response);
	return 0;
}


/*accept an incoming call or, during a call accept a reINVITE*/
int sal_call_accept(SalOp*h){
	belle_sip_response_t *response;
	belle_sip_header_contact_t* contact_header;

	if (!h->pending_server_trans) {
		ms_error("No transaction to accept for op [%p]",h);
		return -1;
	}

	/* sends a 200 OK */
	response = sal_op_create_response_from_request(h,belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(h->pending_server_trans)),200);

	if (response==NULL){
		ms_error("Fail to build answer for call");
		return -1;
	}
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_HEADER(create_allow()));
	if (h->base.root->session_expires!=0){
		if (h->supports_session_timers) {
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),belle_sip_header_create("Supported", "timer"));
		}
	}

	if ((contact_header=sal_op_create_contact(h))) {
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_HEADER(contact_header));
	}
	
	_sal_op_add_custom_headers(h, BELLE_SIP_MESSAGE(response));

	handle_offer_answer_response(h,response);

	belle_sip_server_transaction_send_response(h->pending_server_trans,response);
	return 0;
}

int sal_call_decline(SalOp *op, SalReason reason, const char *redirection /*optional*/){
	belle_sip_response_t* response;
	belle_sip_header_contact_t* contact=NULL;
	int status=sal_reason_to_sip_code(reason);
	
	if (reason==SalReasonRedirect){
		if (redirection!=NULL) {
			if (strstr(redirection,"sip:")!=0) status=302;
			else status=380;
			contact= belle_sip_header_contact_new();
			belle_sip_header_address_set_uri(BELLE_SIP_HEADER_ADDRESS(contact),belle_sip_uri_parse(redirection));
		} else {
			ms_error("Cannot redirect to null");
		}
	}
	response = sal_op_create_response_from_request(op,belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(op->pending_server_trans)),status);
	if (contact) belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_HEADER(contact));
	belle_sip_server_transaction_send_response(op->pending_server_trans,response);
	return 0;
}

int sal_call_update(SalOp *op, const char *subject){
	belle_sip_request_t *reinvite=belle_sip_dialog_create_request(op->dialog,"INVITE");
	if (reinvite){
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(reinvite),belle_sip_header_create( "Subject", subject));
		sal_op_fill_invite(op, reinvite);
		return sal_op_send_request(op,reinvite);
	}
	return -1;
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

int sal_call_send_dtmf(SalOp *h, char dtmf){
	if (h->dialog && (belle_sip_dialog_get_state(h->dialog) == BELLE_SIP_DIALOG_CONFIRMED || belle_sip_dialog_get_state(h->dialog) == BELLE_SIP_DIALOG_EARLY)){
		belle_sip_request_t *req=belle_sip_dialog_create_queued_request(h->dialog,"INFO");
		if (req){
			int bodylen;
			char dtmf_body[128]={0};
			
			snprintf(dtmf_body, sizeof(dtmf_body)-1, "Signal=%c\r\nDuration=250\r\n", dtmf);
			bodylen=strlen(dtmf_body);
			belle_sip_message_set_body((belle_sip_message_t*)req,dtmf_body,bodylen);
			belle_sip_message_add_header((belle_sip_message_t*)req,(belle_sip_header_t*)belle_sip_header_content_length_create(bodylen));
			belle_sip_message_add_header((belle_sip_message_t*)req,(belle_sip_header_t*)belle_sip_header_content_type_create("application", "dtmf-relay"));
			sal_op_send_request(h,req);
		}else ms_error("sal_call_send_dtmf(): could not build request");
	}else ms_error("sal_call_send_dtmf(): no dialog");
	return 0;
}

int sal_call_terminate(SalOp *op){
	belle_sip_dialog_state_t dialog_state=op->dialog?belle_sip_dialog_get_state(op->dialog):BELLE_SIP_DIALOG_NULL;
	if (op->state==SalOpStateTerminating || op->state==SalOpStateTerminated) {
		ms_error("Cannot terminate op [%p] in state [%s]",op,sal_op_state_to_string(op->state));
		return -1;
	}
	switch(dialog_state) {
		case BELLE_SIP_DIALOG_CONFIRMED: {
			sal_op_send_request(op,belle_sip_dialog_create_request(op->dialog,"BYE"));
			op->state=SalOpStateTerminating;
			break;
		}
		case BELLE_SIP_DIALOG_NULL: {
			if (op->dir == SalOpDirIncoming) {
				sal_call_decline(op, SalReasonDeclined,NULL);
				op->state=SalOpStateTerminated;
			} else if (op->pending_client_trans){
				if (belle_sip_transaction_get_state(BELLE_SIP_TRANSACTION(op->pending_client_trans)) == BELLE_SIP_TRANSACTION_PROCEEDING){
					cancelling_invite(op);
				}else{
					/* Case where the CANCEL cannot be sent because no provisional response was received so far.
					 * The Op must be kept for the time of the transaction in case a response is received later.
					 * The state is passed to Terminating to remember to terminate later.
					 */
					op->state=SalOpStateTerminating;
				}
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
			ms_error("sal_call_terminate not implemented yet for dialog state [%s]",belle_sip_dialog_state_to_string(dialog_state));
			return -1;
		}
	}
	return 0;
}

bool_t sal_call_autoanswer_asked(SalOp *op){
	return op->auto_answer_asked;
}

void sal_call_send_vfu_request(SalOp *op){
	char info_body[] =
			"<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
			 "<media_control>"
			 "  <vc_primitive>"
			 "    <to_encoder>"
			 "      <picture_fast_update></picture_fast_update>"
			 "    </to_encoder>"
			 "  </vc_primitive>"
			 "</media_control>";
	size_t content_lenth = sizeof(info_body) - 1;
	belle_sip_dialog_state_t dialog_state=op->dialog?belle_sip_dialog_get_state(op->dialog):BELLE_SIP_DIALOG_NULL; /*no dialog = dialog in NULL state*/
	if (dialog_state == BELLE_SIP_DIALOG_CONFIRMED) {
		belle_sip_request_t* info =	belle_sip_dialog_create_queued_request(op->dialog,"INFO");
		int error=TRUE;
		if (info) {
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(info),BELLE_SIP_HEADER(belle_sip_header_content_type_create("application","media_control+xml")));
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(info),BELLE_SIP_HEADER(belle_sip_header_content_length_create(content_lenth)));
			belle_sip_message_set_body(BELLE_SIP_MESSAGE(info),info_body,content_lenth);
			error=sal_op_send_request(op,info);
		}
		if (error)
			ms_warning("Cannot send vfu request to [%s] ", sal_op_get_to(op));

	} else {
		ms_warning("Cannot send vfu request to [%s] because dialog [%p] in wrong state [%s]",sal_op_get_to(op)
																							,op->dialog
																							,belle_sip_dialog_state_to_string(dialog_state));
	}

	return ;
}

int sal_call_is_offerer(const SalOp *h){
	return h->sdp_offering;
}




