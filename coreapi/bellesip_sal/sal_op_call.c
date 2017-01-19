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

static int extract_sdp(SalOp* op,belle_sip_message_t* message,belle_sdp_session_description_t** session_desc, SalReason *error);

/*used for calls terminated before creation of a dialog*/
static void call_set_released(SalOp* op){
	if (!op->call_released){
		op->state=SalOpStateTerminated;
		op->base.root->callbacks.call_released(op);
		op->call_released=TRUE;
		/*be aware that the following line may destroy the op*/
		set_or_update_dialog(op,NULL);
	}
}

static void call_set_error(SalOp* op,belle_sip_response_t* response, bool_t fatal){
	sal_op_set_error_info_from_response(op,response);
	if (fatal) op->state = SalOpStateTerminating;
	op->base.root->callbacks.call_failure(op);
}
static void set_addr_to_0000(char value[]) {
	if (ms_is_ipv6(value)) {
		strcpy(value,"::0");
	} else {
		strcpy(value,"0.0.0.0");
	}
	return;
}
static void sdp_process(SalOp *h){
	ms_message("Doing SDP offer/answer process of type %s",h->sdp_offering ? "outgoing" : "incoming");
	if (h->result){
		sal_media_description_unref(h->result);
		h->result = NULL;
	}

	/* if SDP was invalid */
	if (h->base.remote_media == NULL) return;

	h->result=sal_media_description_new();
	if (h->sdp_offering){
		offer_answer_initiate_outgoing(h->base.root->factory, h->base.local_media,h->base.remote_media,h->result);
	}else{
		int i;
		if (h->sdp_answer){
			belle_sip_object_unref(h->sdp_answer);
		}
		offer_answer_initiate_incoming(h->base.root->factory, h->base.local_media,h->base.remote_media,h->result,h->base.root->one_matching_codec);
		/*for backward compatibility purpose*/
		if(h->cnx_ip_to_0000_if_sendonly_enabled && sal_media_description_has_dir(h->result,SalStreamSendOnly)) {
			set_addr_to_0000(h->result->addr);
			for(i=0;i<SAL_MEDIA_DESCRIPTION_MAX_STREAMS;++i){
				if (h->result->streams[i].dir == SalStreamSendOnly) {
					set_addr_to_0000(h->result->streams[i].rtp_addr);
					set_addr_to_0000(h->result->streams[i].rtcp_addr);
				}
			}
		}
		h->sdp_answer=(belle_sdp_session_description_t *)belle_sip_object_ref(media_description_to_sdp(h->result));
		/*once we have generated the SDP answer, we modify the result description for processing by the upper layer.
		 It should contains media parameters constraint from the remote offer, not our response*/
		strcpy(h->result->addr,h->base.remote_media->addr);
		h->result->bandwidth=h->base.remote_media->bandwidth;

		for(i=0;i<SAL_MEDIA_DESCRIPTION_MAX_STREAMS;++i){
			/*copy back parameters from remote description that we need in our result description*/
			if (h->result->streams[i].rtp_port!=0){ /*if stream was accepted*/
				strcpy(h->result->streams[i].rtp_addr,h->base.remote_media->streams[i].rtp_addr);
				h->result->streams[i].ptime=h->base.remote_media->streams[i].ptime;
				h->result->streams[i].bandwidth=h->base.remote_media->streams[i].bandwidth;
				h->result->streams[i].rtp_port=h->base.remote_media->streams[i].rtp_port;
				strcpy(h->result->streams[i].rtcp_addr,h->base.remote_media->streams[i].rtcp_addr);
				h->result->streams[i].rtcp_port=h->base.remote_media->streams[i].rtcp_port;

				if (sal_stream_description_has_srtp(&h->result->streams[i])) {
					h->result->streams[i].crypto[0] = h->base.remote_media->streams[i].crypto[0];
				}
			}
		}
	}
}

static int set_sdp(belle_sip_message_t *msg,belle_sdp_session_description_t* session_desc) {
	belle_sip_header_content_type_t* content_type ;
	belle_sip_header_content_length_t* content_length;
	belle_sip_error_code error = BELLE_SIP_BUFFER_OVERFLOW;
	size_t length = 0;

	if (session_desc) {
		size_t bufLen = 2048;
		size_t hardlimit = 16*1024; /* 16k SDP limit seems reasonable */
		char* buff = belle_sip_malloc(bufLen);
		content_type = belle_sip_header_content_type_create("application","sdp");

		/* try to marshal the description. This could go higher than 2k so we iterate */
		while( error != BELLE_SIP_OK && bufLen <= hardlimit && buff != NULL){
			error = belle_sip_object_marshal(BELLE_SIP_OBJECT(session_desc),buff,bufLen,&length);
			if( error != BELLE_SIP_OK ){
				bufLen *= 2;
				length  = 0;
				buff = belle_sip_realloc(buff,bufLen);
			}
		}
		/* give up if hard limit reached */
		if (error != BELLE_SIP_OK || buff == NULL) {
			ms_error("Buffer too small (%d) or not enough memory, giving up SDP", (int)bufLen);
			return -1;
		}

		content_length = belle_sip_header_content_length_create(length);
		belle_sip_message_add_header(msg,BELLE_SIP_HEADER(content_type));
		belle_sip_message_add_header(msg,BELLE_SIP_HEADER(content_length));
		belle_sip_message_assign_body(msg,buff,length);
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

static void call_process_io_error(void *user_ctx, const belle_sip_io_error_event_t *event) {
	SalOp *op = (SalOp *)user_ctx;

	if (op->state == SalOpStateTerminated) return;

	if (op->pending_client_trans && (belle_sip_transaction_get_state(BELLE_SIP_TRANSACTION(op->pending_client_trans)) == BELLE_SIP_TRANSACTION_INIT)) {
		/* Call terminated very very early, before INVITE is even sent, probably DNS resolution timeout. */
		sal_error_info_set(&op->error_info, SalReasonIOError, 503, "IO error", NULL);
		op->base.root->callbacks.call_failure(op);
		op->state = SalOpStateTerminating;
		call_set_released(op);
	} else {
		/* Nothing to be done. If the error comes from a connectivity loss,
		 * the call will be marked as broken, and an attempt to repair it will be done. */
	}
}

static void process_dialog_terminated(void *ctx, const belle_sip_dialog_terminated_event_t *event) {
	SalOp* op=(SalOp*)ctx;

	if (op->dialog && op->dialog==belle_sip_dialog_terminated_event_get_dialog(event))  {
		/*belle_sip_transaction_t* trans=belle_sip_dialog_get_last_transaction(op->dialog);*/
		ms_message("Dialog [%p] terminated for op [%p]",belle_sip_dialog_terminated_event_get_dialog(event),op);

		switch(belle_sip_dialog_get_previous_state(op->dialog)) {
			case BELLE_SIP_DIALOG_EARLY:
			case BELLE_SIP_DIALOG_NULL:
				if (op->state!=SalOpStateTerminated && op->state!=SalOpStateTerminating) {
					/*this is an early termination due to incorrect response received*/
					op->base.root->callbacks.call_failure(op);
					op->state=SalOpStateTerminating;
				}
			break;
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
							,(belle_sip_callback_t) call_set_released
							, op);
	} else {
		ms_error("dialog unknown for op ");
	}
}

static void handle_sdp_from_response(SalOp* op,belle_sip_response_t* response) {
	belle_sdp_session_description_t* sdp;
	SalReason reason;
	if (op->base.remote_media){
		sal_media_description_unref(op->base.remote_media);
		op->base.remote_media=NULL;
	}
	if (extract_sdp(op,BELLE_SIP_MESSAGE(response),&sdp,&reason)==0) {
		if (sdp){
			op->base.remote_media=sal_media_description_new();
			sdp_to_media_description(sdp,op->base.remote_media);
		}/*if no sdp in response, what can we do ?*/
	}
	/* process sdp in any case to reset result media description*/
	if (op->base.local_media) sdp_process(op);
}

void sal_call_cancel_invite(SalOp* op) {
	belle_sip_request_t* cancel;
	ms_message("Cancelling INVITE request from [%s] to [%s] ",sal_op_get_from(op), sal_op_get_to(op));
	cancel = belle_sip_client_transaction_create_cancel(op->pending_client_trans);
	if (cancel){
		sal_op_send_request(op,cancel);
	}else if (op->dialog){
		belle_sip_dialog_state_t state = belle_sip_dialog_get_state(op->dialog);;
		/*case where the response received is invalid (could not establish a dialog), but the transaction is not cancellable 
		 * because already terminated*/
		switch(state){
			case BELLE_SIP_DIALOG_EARLY:
			case BELLE_SIP_DIALOG_NULL:
				/*force kill the dialog*/
				ms_warning("op [%p]: force kill of dialog [%p]", op, op->dialog);
				belle_sip_dialog_delete(op->dialog);
			break;
			default:
			break;
		}
	}
}

static void cancelling_invite(SalOp *op) {
	sal_call_cancel_invite(op);
	op->state=SalOpStateTerminating;
}

static int vfu_retry (void *user_data, unsigned int events) {
	SalOp *op=(SalOp *)user_data;
	sal_call_send_vfu_request(op);
	sal_op_unref(op);
	return BELLE_SIP_STOP;
}

static void call_process_response(void *op_base, const belle_sip_response_event_t *event){
	SalOp* op = (SalOp*)op_base;
	belle_sip_request_t* ack;
	belle_sip_dialog_state_t dialog_state;
	belle_sip_client_transaction_t* client_transaction = belle_sip_response_event_get_client_transaction(event);
	belle_sip_request_t* req;
	belle_sip_response_t* response=belle_sip_response_event_get_response(event);
	int code = belle_sip_response_get_status_code(response);
	belle_sip_header_content_type_t *header_content_type=NULL;
	belle_sip_dialog_t *dialog=belle_sip_response_event_get_dialog(event);
	const char *method;

	if (!client_transaction) {
		ms_warning("Discarding stateless response [%i] on op [%p]",code,op);
		return;
	}
	req=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
	set_or_update_dialog(op,dialog);
	dialog_state=dialog ? belle_sip_dialog_get_state(dialog) : BELLE_SIP_DIALOG_NULL;
	method=belle_sip_request_get_method(req);
	ms_message("Op [%p] receiving call response [%i], dialog is [%p] in state [%s]",op,code,dialog,belle_sip_dialog_state_to_string(dialog_state));
	/*to make sure no cb will destroy op*/
	sal_op_ref(op);
	switch(dialog_state) {
		case BELLE_SIP_DIALOG_NULL:
		case BELLE_SIP_DIALOG_EARLY: {
			if (strcmp("INVITE",method)==0 ) {
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
				} else if (code >= 180 && code<200) {
					belle_sip_response_t *prev_response=belle_sip_object_data_get(BELLE_SIP_OBJECT(dialog),"early_response");
					if (!prev_response || code>belle_sip_response_get_status_code(prev_response)){
						handle_sdp_from_response(op,response);
						op->base.root->callbacks.call_ringing(op);
					}
					belle_sip_object_data_set(BELLE_SIP_OBJECT(dialog),"early_response",belle_sip_object_ref(response),belle_sip_object_unref);
				} else if (code>=300){
					call_set_error(op, response, TRUE);
					if (op->dialog==NULL) call_set_released(op);
				}
			} else if (code >=200 && code<300) {
				if (strcmp("UPDATE",method)==0) {
					handle_sdp_from_response(op,response);
					op->base.root->callbacks.call_accepted(op);
				} else if (strcmp("CANCEL", method) == 0) {
					op->base.root->callbacks.call_cancel_done(op);
				}
			}
		}
		break;
		case BELLE_SIP_DIALOG_CONFIRMED: {
			switch (op->state) {
				case SalOpStateEarly:/*invite case*/
				case SalOpStateActive: /*re-invite, INFO, UPDATE case*/
					if (strcmp("INVITE",method)==0){
						if (code >=200 && code<300) {
							handle_sdp_from_response(op,response);
							ack=belle_sip_dialog_create_ack(op->dialog,belle_sip_dialog_get_local_seq_number(op->dialog));
							if (ack == NULL) {
								ms_error("This call has been already terminated.");
								return ;
							}
							if (op->sdp_answer){
								set_sdp(BELLE_SIP_MESSAGE(ack),op->sdp_answer);
								belle_sip_object_unref(op->sdp_answer);
								op->sdp_answer=NULL;
							}
							belle_sip_message_add_header(BELLE_SIP_MESSAGE(ack),BELLE_SIP_HEADER(op->base.root->user_agent));
							belle_sip_dialog_send_ack(op->dialog,ack);
							op->base.root->callbacks.call_accepted(op); /*INVITE*/
							op->state=SalOpStateActive;
						}else if (code >= 300){
							call_set_error(op,response, FALSE);
						}
					}else if (strcmp("INFO",method)==0){
						if (code == 491
							&& (header_content_type = belle_sip_message_get_header_by_type(req,belle_sip_header_content_type_t))
							&& strcmp("application",belle_sip_header_content_type_get_type(header_content_type))==0
							&& strcmp("media_control+xml",belle_sip_header_content_type_get_subtype(header_content_type))==0) {
							unsigned int retry_in = (unsigned int)(1000*((float)rand()/RAND_MAX));
							belle_sip_source_t *s=sal_create_timer(op->base.root,vfu_retry,sal_op_ref(op), retry_in, "vfu request retry");
							ms_message("Rejected vfu request on op [%p], just retry in [%ui] ms",op,retry_in);
							belle_sip_object_unref(s);
						}else {
								/*ignoring*/
						}
					}else if (strcmp("UPDATE",method)==0){
						op->base.root->callbacks.call_accepted(op); /*INVITE*/
					}else if (strcmp("CANCEL",method)==0){
						op->base.root->callbacks.call_cancel_done(op);
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
			if (strcmp("INVITE",method)==0 && code >= 300){
				call_set_error(op,response, TRUE);
			}
		}
		break;
		default: {
			ms_error("call op [%p] receive answer [%i] not implemented",op,code);
		}
		break;
	}
	sal_op_unref(op);
}

static void call_process_timeout(void *user_ctx, const belle_sip_timeout_event_t *event) {
	SalOp* op=(SalOp*)user_ctx;

	if (op->state==SalOpStateTerminated) return;

	if (!op->dialog)  {
		/*call terminated very early*/
		sal_error_info_set(&op->error_info,SalReasonRequestTimeout,408,"Request timeout",NULL);
		op->base.root->callbacks.call_failure(op);
		op->state = SalOpStateTerminating;
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
	int code = 0;
	bool_t release_call=FALSE;

	if (client_transaction) {
		req=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
		resp=belle_sip_transaction_get_response(BELLE_SIP_TRANSACTION(client_transaction));
	} else {
		req=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(server_transaction));
		resp=belle_sip_transaction_get_response(BELLE_SIP_TRANSACTION(server_transaction));
	}
	if (resp) code = belle_sip_response_get_status_code(resp);
	
	if (op->state == SalOpStateTerminating
			&& strcmp("BYE",belle_sip_request_get_method(req))==0
			&& (!resp || (belle_sip_response_get_status_code(resp) != 401
			&& belle_sip_response_get_status_code(resp) != 407))
			&& op->dialog==NULL) {
		release_call=TRUE;
	}else if (op->state == SalOpStateEarly && code < 200){
		/*call terminated early*/
		sal_error_info_set(&op->error_info,SalReasonIOError,503,"I/O error",NULL);
		op->state = SalOpStateTerminating;
		op->base.root->callbacks.call_failure(op);
		release_call=TRUE;
	}
	if (server_transaction){
		if (op->pending_server_trans==server_transaction){
			belle_sip_object_unref(op->pending_server_trans);
			op->pending_server_trans=NULL;
		}
		if (op->pending_update_server_trans==server_transaction){
			belle_sip_object_unref(op->pending_update_server_trans);
			op->pending_update_server_trans=NULL;
		}
	}
	if (release_call) call_set_released(op);
}

static void call_terminated(SalOp* op,belle_sip_server_transaction_t* server_transaction, belle_sip_request_t* request,int status_code) {
	belle_sip_response_t* resp;
	op->state = SalOpStateTerminating;
	op->base.root->callbacks.call_terminated(op,op->dir==SalOpDirIncoming?sal_op_get_from(op):sal_op_get_to(op));
	resp=sal_op_create_response_from_request(op,request,status_code);
	belle_sip_server_transaction_send_response(server_transaction,resp);
}

static void unsupported_method(belle_sip_server_transaction_t* server_transaction,belle_sip_request_t* request) {
	belle_sip_response_t* resp;
	resp=belle_sip_response_create_from_request(request,501);
	belle_sip_server_transaction_send_response(server_transaction,resp);
	return;
}

/*
 * Extract the sdp from a sip message.
 * If there is no body in the message, the session_desc is set to null, 0 is returned.
 * If body was present is not a SDP or parsing of SDP failed, -1 is returned and SalReason is set appropriately.
 *
**/
static int extract_sdp(SalOp *op, belle_sip_message_t* message,belle_sdp_session_description_t** session_desc, SalReason *error) {
	const char *body;
	belle_sip_header_content_type_t* content_type;

	if (op&&op->sdp_handling == SalOpSDPSimulateError){
		ms_error("Simulating SDP parsing error for op %p", op);
		*session_desc=NULL;
		*error=SalReasonNotAcceptable;
		return -1;
	} else if( op && op->sdp_handling == SalOpSDPSimulateRemove){
		ms_error("Simulating no SDP for op %p", op);
		*session_desc = NULL;
		return 0;
	}

	body = belle_sip_message_get_body(message);
	if(body == NULL) {
		*session_desc = NULL;
		return 0;
	}

	content_type = belle_sip_message_get_header_by_type(message,belle_sip_header_content_type_t);
	if (content_type){
		if (strcmp("application",belle_sip_header_content_type_get_type(content_type))==0
			&& strcmp("sdp",belle_sip_header_content_type_get_subtype(content_type))==0) {
			*session_desc=belle_sdp_session_description_parse(body);
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
	if (md->nb_streams==0){
		ms_warning("Media description does not define any stream.");
		return FALSE;
	}
	return TRUE;
}

static int process_sdp_for_invite(SalOp* op,belle_sip_request_t* invite) {
	belle_sdp_session_description_t* sdp;
	int err=0;
	SalReason reason;

	if (extract_sdp(op,BELLE_SIP_MESSAGE(invite),&sdp,&reason)==0) {
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

static void sal_op_reset_descriptions(SalOp *op) {
	if (op->base.remote_media){
		sal_media_description_unref(op->base.remote_media);
		op->base.remote_media=NULL;
	}
	if (op->result){
		sal_media_description_unref(op->result);
		op->result=NULL;
	}
}
static void process_request_event(void *op_base, const belle_sip_request_event_t *event) {
	SalOp* op = (SalOp*)op_base;
	belle_sip_server_transaction_t* server_transaction=NULL;
	belle_sdp_session_description_t* sdp;
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_dialog_state_t dialog_state;
	belle_sip_response_t* resp;
	belle_sip_header_t* call_info;
	const char *method=belle_sip_request_get_method(req);
	bool_t is_update=FALSE;
	bool_t drop_op = FALSE;

	if (strcmp("ACK",method)!=0){  /*ACK doesn't create a server transaction*/
		server_transaction = belle_sip_provider_create_server_transaction(op->base.root->prov,belle_sip_request_event_get_request(event));
		belle_sip_object_ref(server_transaction);
		belle_sip_transaction_set_application_data(BELLE_SIP_TRANSACTION(server_transaction),sal_op_ref(op));
	}

	if (strcmp("INVITE",method)==0) {
		if (op->pending_server_trans) belle_sip_object_unref(op->pending_server_trans);
		/*updating pending invite transaction*/
		op->pending_server_trans=server_transaction;
		belle_sip_object_ref(op->pending_server_trans);
	}

	if (strcmp("UPDATE",method)==0) {
		if (op->pending_update_server_trans) belle_sip_object_unref(op->pending_update_server_trans);
		/*updating pending update transaction*/
		op->pending_update_server_trans=server_transaction;
		belle_sip_object_ref(op->pending_update_server_trans);
	}

	if (!op->dialog) {
		set_or_update_dialog(op,belle_sip_provider_create_dialog(op->base.root->prov,BELLE_SIP_TRANSACTION(op->pending_server_trans)));
		ms_message("new incoming call from [%s] to [%s]",sal_op_get_from(op),sal_op_get_to(op));
	}
	dialog_state=belle_sip_dialog_get_state(op->dialog);
	switch(dialog_state) {
	case BELLE_SIP_DIALOG_NULL: {
		if (strcmp("INVITE",method)==0) {
			if (!op->replaces && (op->replaces=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_replaces_t))) {
				belle_sip_object_ref(op->replaces);
			} else if(op->replaces) {
				ms_warning("replace header already set");
			}

			if (process_sdp_for_invite(op,req) == 0) {
				if ((call_info=belle_sip_message_get_header(BELLE_SIP_MESSAGE(req),"Call-Info"))) {
					if( strstr(belle_sip_header_get_unparsed_value(call_info),"answer-after=") != NULL) {
						op->auto_answer_asked=TRUE;
						ms_message("The caller asked to automatically answer the call(Emergency?)\n");
					}
				}
				op->base.root->callbacks.call_received(op);
			}else{
				/*the INVITE was declined by process_sdp_for_invite(). As we are not inside an established dialog, we can drop the op immediately*/
				drop_op = TRUE;
			}
			break;
		} /* else same behavior as for EARLY state, thus NO BREAK*/
	}
	case BELLE_SIP_DIALOG_EARLY: {
		if (strcmp("CANCEL",method)==0) {
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
		} else if (strcmp("PRACK",method)==0) {
			resp=sal_op_create_response_from_request(op,req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
		} else if (strcmp("UPDATE",method)==0) {
			sal_op_reset_descriptions(op);
			if (process_sdp_for_invite(op,req)==0)
				op->base.root->callbacks.call_updating(op,TRUE);
		} else {
			belle_sip_error("Unexpected method [%s] for dialog state BELLE_SIP_DIALOG_EARLY",belle_sip_request_get_method(req));
			unsupported_method(server_transaction,req);
		}
		break;
	}
	case BELLE_SIP_DIALOG_CONFIRMED:
		/*great ACK received*/
		if (strcmp("ACK",method)==0) {
			if (!op->pending_client_trans || 
				!belle_sip_transaction_state_is_transient(belle_sip_transaction_get_state((belle_sip_transaction_t*)op->pending_client_trans))){
				if (op->sdp_offering){
					SalReason reason;
					if (extract_sdp(op,BELLE_SIP_MESSAGE(req),&sdp,&reason)==0){
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
				op->base.root->callbacks.call_ack(op);
			}else{
				ms_message("Ignored received ack since a new client transaction has been started since.");
			}
		} else if(strcmp("BYE",method)==0) {
			resp=sal_op_create_response_from_request(op,req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
			op->base.root->callbacks.call_terminated(op,op->dir==SalOpDirIncoming?sal_op_get_from(op):sal_op_get_to(op));
			op->state=SalOpStateTerminating;
			/*call end not notified by dialog deletion because transaction can end before dialog*/
		} else if(strcmp("INVITE",method)==0 || (is_update=(strcmp("UPDATE",method)==0)) ) {
			if (is_update && !belle_sip_message_get_body(BELLE_SIP_MESSAGE(req))) {
				/*session timer case*/
				/*session expire should be handled. to be done when real session timer (rfc4028) will be implemented*/
				resp=sal_op_create_response_from_request(op,req,200);
				belle_sip_server_transaction_send_response(server_transaction,resp);
				belle_sip_object_unref(op->pending_update_server_trans);
				op->pending_update_server_trans=NULL;
			} else {
				/*re-invite*/
				sal_op_reset_descriptions(op);
				if (process_sdp_for_invite(op,req)==0)
					op->base.root->callbacks.call_updating(op,is_update);
			}
		} else if (strcmp("INFO",method)==0){
			if (belle_sip_message_get_body(BELLE_SIP_MESSAGE(req))
				&&	strstr(belle_sip_message_get_body(BELLE_SIP_MESSAGE(req)),"picture_fast_update")) {
				/*vfu request*/
				ms_message("Receiving VFU request on op [%p]",op);
				if (op->base.root->callbacks.vfu_request){
					op->base.root->callbacks.vfu_request(op);

				}
			}else{
				belle_sip_message_t *msg = BELLE_SIP_MESSAGE(req);
				belle_sip_body_handler_t *body_handler = BELLE_SIP_BODY_HANDLER(sal_op_get_body_handler(op, msg));
				if (body_handler) {
					belle_sip_header_content_type_t *content_type = belle_sip_message_get_header_by_type(msg, belle_sip_header_content_type_t);
					if (content_type
						&& (strcmp(belle_sip_header_content_type_get_type(content_type), "application") == 0)
						&& (strcmp(belle_sip_header_content_type_get_subtype(content_type), "dtmf-relay") == 0)) {
						char tmp[10];
						if (sal_lines_get_value(belle_sip_message_get_body(msg), "Signal",tmp, sizeof(tmp))){
							op->base.root->callbacks.dtmf_received(op,tmp[0]);
						}
					}else
						op->base.root->callbacks.info_received(op, (SalBodyHandler *)body_handler);
				} else {
					op->base.root->callbacks.info_received(op,NULL);
				}
			}
			resp=sal_op_create_response_from_request(op,req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
		}else if (strcmp("REFER",method)==0) {
			sal_op_process_refer(op,event,server_transaction);
		} else if (strcmp("NOTIFY",method)==0) {
			sal_op_call_process_notify(op,event,server_transaction);
		} else if (strcmp("OPTIONS",method)==0) {
			resp=sal_op_create_response_from_request(op,req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
		} else if (strcmp("CANCEL",method)==0) {
			belle_sip_transaction_t *last_transaction = belle_sip_dialog_get_last_transaction(op->dialog);
			if (last_transaction == NULL) {
				/*call leg does not exist because 200ok already sent*/
				belle_sip_server_transaction_send_response(server_transaction,sal_op_create_response_from_request(op,req,481));
			} else {
				/* CANCEL on re-INVITE for which a 200ok has not been sent yet */
				belle_sip_server_transaction_send_response(server_transaction, sal_op_create_response_from_request(op, req, 200));
				belle_sip_server_transaction_send_response(BELLE_SIP_SERVER_TRANSACTION(last_transaction),
					sal_op_create_response_from_request(op, belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(last_transaction)), 487));
			}
		} else if (strcmp("MESSAGE",method)==0){
			sal_process_incoming_message(op,event);
		}else{
			ms_error("unexpected method [%s] for dialog [%p]",belle_sip_request_get_method(req),op->dialog);
			unsupported_method(server_transaction,req);
		}
		break;
	default:
		ms_error("unexpected dialog state [%s]",belle_sip_dialog_state_to_string(dialog_state));
		break;
	}

	if (server_transaction) belle_sip_object_unref(server_transaction);
	if (drop_op) sal_op_release(op);
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

static belle_sip_header_allow_t *create_allow(bool_t enable_update){
	belle_sip_header_allow_t* header_allow;
	char allow [256];
	snprintf(allow,sizeof(allow),"INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO%s",(enable_update?", UPDATE":""));
	header_allow = belle_sip_header_allow_create(allow);
	return header_allow;
}

static void sal_op_fill_invite(SalOp *op, belle_sip_request_t* invite) {
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),BELLE_SIP_HEADER(create_allow(op->base.root->enable_sip_update)));

	if (op->base.root->session_expires!=0){
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),belle_sip_header_create( "Session-expires", "600;refresher=uas"));
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

	if( invite == NULL ){
		/* can happen if the op has an invalid address */
		return -1;
	}

	sal_op_fill_invite(op,invite);

	sal_op_call_fill_cbs(op);
	if (op->replaces){
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),BELLE_SIP_HEADER(op->replaces));
	}
	if (op->referred_by)
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),BELLE_SIP_HEADER(op->referred_by));

	return sal_op_send_request(op,invite);
}

static belle_sip_listener_callbacks_t call_op_callbacks={0};

void sal_op_call_fill_cbs(SalOp*op) {
	if (call_op_callbacks.process_response_event==NULL){
		call_op_callbacks.process_io_error=call_process_io_error;
		call_op_callbacks.process_response_event=call_process_response;
		call_op_callbacks.process_timeout=call_process_timeout;
		call_op_callbacks.process_transaction_terminated=call_process_transaction_terminated;
		call_op_callbacks.process_request_event=process_request_event;
		call_op_callbacks.process_dialog_terminated=process_dialog_terminated;
	}
	op->callbacks=&call_op_callbacks;
	op->type=SalOpCall;
}

static void handle_offer_answer_response(SalOp* op, belle_sip_response_t* response) {
	if (op->base.local_media){
		/*this is the case where we received an invite without SDP*/
		if (op->sdp_offering) {
			set_sdp_from_desc(BELLE_SIP_MESSAGE(response),op->base.local_media);
		}else{

			if ( op->sdp_answer==NULL )
			{
				if( op->sdp_handling == SalOpSDPSimulateRemove ){
					ms_warning("Simulating SDP removal in answer for op %p", op);
				} else {
					sdp_process(op);
				}
			}

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
	belle_sip_server_transaction_t* transaction;

	/*first check if an UPDATE transaction need to be accepted*/
	if (h->pending_update_server_trans) {
		transaction=h->pending_update_server_trans;
	} else if (h->pending_server_trans) {
		/*so it must be an invite/re-invite*/
		transaction=h->pending_server_trans;
	} else {
		ms_error("No transaction to accept for op [%p]",h);
		return -1;
	}
	ms_message("Accepting server transaction [%p] on op [%p]", transaction, h);

	/* sends a 200 OK */
	response = sal_op_create_response_from_request(h,belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(transaction)),200);
	if (response==NULL){
		ms_error("Fail to build answer for call");
		return -1;
	}
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_HEADER(create_allow(h->base.root->enable_sip_update)));
	if (h->base.root->session_expires!=0){
/*		if (h->supports_session_timers) {*/
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),belle_sip_header_create("Supported", "timer"));
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),belle_sip_header_create( "Session-expires", "600;refresher=uac"));
		/*}*/
	}

	if ((contact_header=sal_op_create_contact(h))) {
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_HEADER(contact_header));
	}

	_sal_op_add_custom_headers(h, BELLE_SIP_MESSAGE(response));

	handle_offer_answer_response(h,response);

	belle_sip_server_transaction_send_response(transaction,response);
	if (h->pending_update_server_trans) {
		belle_sip_object_unref(h->pending_update_server_trans);
		h->pending_update_server_trans=NULL;
	}
	if (h->state == SalOpStateEarly){
		h->state = SalOpStateActive;
	}
	return 0;
}

int sal_call_decline(SalOp *op, SalReason reason, const char *redirection /*optional*/){
	belle_sip_response_t* response;
	belle_sip_header_contact_t* contact=NULL;
	int status=sal_reason_to_sip_code(reason);
	belle_sip_transaction_t *trans;

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
	trans=(belle_sip_transaction_t*)op->pending_server_trans;
	if (!trans) trans=(belle_sip_transaction_t*)op->pending_update_server_trans;
	if (!trans){
		ms_error("sal_call_decline(): no pending transaction to decline.");
		return -1;
	}
	response = sal_op_create_response_from_request(op,belle_sip_transaction_get_request(trans),status);
	if (contact) belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_HEADER(contact));
	belle_sip_server_transaction_send_response(BELLE_SIP_SERVER_TRANSACTION(trans),response);
	return 0;
}

int sal_call_update(SalOp *op, const char *subject, bool_t no_user_consent){
	belle_sip_request_t *update;
	belle_sip_dialog_state_t state;

	if (op->dialog == NULL) {
		/* If the dialog does not exist, this is that we are trying to recover from a connection loss
			during a very early state of outgoing call initiation (the dialog has not been created yet). */
		const char *from = sal_op_get_from(op);
		const char *to = sal_op_get_to(op);
		return sal_call(op, from, to);
	}

	state = belle_sip_dialog_get_state(op->dialog);
	belle_sip_dialog_enable_pending_trans_checking(op->dialog,op->base.root->pending_trans_checking);

	/*check for dialog state*/
	if ( state == BELLE_SIP_DIALOG_CONFIRMED) {
		if (no_user_consent)
			update=belle_sip_dialog_create_request(op->dialog,"UPDATE");
		else
			update=belle_sip_dialog_create_request(op->dialog,"INVITE");
	} else if (state == BELLE_SIP_DIALOG_EARLY)  {
		update=belle_sip_dialog_create_request(op->dialog,"UPDATE");
	} else {
		ms_error("Cannot update op [%p] with dialog [%p] in state [%s]",op, op->dialog,belle_sip_dialog_state_to_string(state));
		return  -1;
	}
	if (update){
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(update),belle_sip_header_create( "Subject", subject));
		sal_op_fill_invite(op, update);
		return sal_op_send_request(op,update);
	}
	/*it failed why ?*/
	if (belle_sip_dialog_request_pending(op->dialog))
		sal_error_info_set(&op->error_info,SalReasonRequestPending,491,NULL,NULL);
	else
		sal_error_info_set(&op->error_info,SalReasonUnknown,500,NULL,NULL);
	return -1;
}

SalMediaDescription * sal_call_get_remote_media_description(SalOp *h){
	return h->base.remote_media;
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
			size_t bodylen;
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

bool_t sal_call_compare_op(const SalOp *op1, const SalOp *op2) {
	if (strcmp(op1->base.call_id, op2->base.call_id) == 0) return TRUE;
	return FALSE;
}

bool_t sal_call_dialog_request_pending(const SalOp *op) {
	return belle_sip_dialog_request_pending(op->dialog) ? TRUE : FALSE;
}

const char * sal_call_get_local_tag(SalOp *op) {
	return belle_sip_dialog_get_local_tag(op->dialog);
}

const char * sal_call_get_remote_tag(SalOp *op) {
	return belle_sip_dialog_get_remote_tag(op->dialog);
}

void sal_call_set_replaces(SalOp *op, const char *call_id, const char *from_tag, const char *to_tag) {
	belle_sip_header_replaces_t *replaces = belle_sip_header_replaces_create(call_id, from_tag, to_tag);
	sal_op_set_replaces(op, replaces);
}
