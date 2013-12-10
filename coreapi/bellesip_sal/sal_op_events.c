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

SalSubscribeStatus get_subscription_state(belle_sip_message_t *msg){
	belle_sip_header_subscription_state_t* subscription_state_header=belle_sip_message_get_header_by_type(msg,belle_sip_header_subscription_state_t);
	SalSubscribeStatus sss=SalSubscribeNone;
	if (subscription_state_header){
		if (strcmp(belle_sip_header_subscription_state_get_state(subscription_state_header),BELLE_SIP_SUBSCRIPTION_STATE_TERMINATED)==0)
			sss=SalSubscribeTerminated;
		else if (strcmp(belle_sip_header_subscription_state_get_state(subscription_state_header),BELLE_SIP_SUBSCRIPTION_STATE_PENDING)==0)
			sss=SalSubscribePending;
		else if (strcmp(belle_sip_header_subscription_state_get_state(subscription_state_header),BELLE_SIP_SUBSCRIPTION_STATE_ACTIVE)==0)
			sss=SalSubscribeActive;
	}
	return sss;
}

static void subscribe_refresher_listener (belle_sip_refresher_t* refresher
		,void* user_pointer
		,unsigned int status_code
		,const char* reason_phrase) {
	SalOp* op = (SalOp*)user_pointer;
	SalError error=SalErrorUnknown;
	SalReason sr=SalReasonUnknown;
	belle_sip_transaction_t *tr=BELLE_SIP_TRANSACTION(belle_sip_refresher_get_transaction(refresher));
	/*belle_sip_response_t* response=belle_sip_transaction_get_response(tr);*/
	SalSubscribeStatus sss=SalSubscribeTerminated;
	
	ms_message("Subscribe refresher  [%i] reason [%s] ",status_code,reason_phrase?reason_phrase:"none");
	if (status_code>=200 && status_code<300){
		if (status_code==200) sss=SalSubscribeActive;
		else if (status_code==202) sss=SalSubscribePending;
		set_or_update_dialog(op,belle_sip_transaction_get_dialog(tr));
	}
	if (status_code>=200){
		sal_compute_sal_errors_from_code(status_code,&error,&sr);
		op->base.root->callbacks.subscribe_response(op,sss,error,sr);
	}
	
}

static void subscribe_process_io_error(void *user_ctx, const belle_sip_io_error_event_t *event){
	ms_error("subscribe_process_io_error not implemented yet");
}

static void subscribe_process_dialog_terminated(void *ctx, const belle_sip_dialog_terminated_event_t *event) {
	SalOp* op= (SalOp*)ctx;
	if (op->dialog) {
		op->dialog=NULL;
		sal_op_unref(op);
	}
}

static void subscribe_response_event(void *op_base, const belle_sip_response_event_t *event){
}

static void subscribe_process_timeout(void *user_ctx, const belle_sip_timeout_event_t *event) {
}

static void subscribe_process_transaction_terminated(void *user_ctx, const belle_sip_transaction_terminated_event_t *event) {
}

static void handle_notify(SalOp *op, belle_sip_request_t *req, const char *eventname, SalBody * body){
	SalSubscribeStatus sub_state;
	belle_sip_header_subscription_state_t* subscription_state_header=belle_sip_message_get_header_by_type(req,belle_sip_header_subscription_state_t);
	belle_sip_response_t* resp;
	belle_sip_server_transaction_t* server_transaction = op->pending_server_trans;
	
	if (!subscription_state_header || strcasecmp(BELLE_SIP_SUBSCRIPTION_STATE_TERMINATED,belle_sip_header_subscription_state_get_state(subscription_state_header)) ==0) {
		sub_state=SalSubscribeTerminated;
		ms_message("Outgoing subscription terminated by remote [%s]",sal_op_get_to(op));
	} else
		sub_state=SalSubscribeActive;
	sal_op_ref(op);
	op->base.root->callbacks.notify(op,sub_state,eventname,body);
	resp=sal_op_create_response_from_request(op,req,200);
	belle_sip_server_transaction_send_response(server_transaction,resp);
	sal_op_unref(op);
}

static void subscribe_process_request_event(void *op_base, const belle_sip_request_event_t *event) {
	SalOp* op = (SalOp*)op_base;
	belle_sip_server_transaction_t* server_transaction = belle_sip_provider_create_server_transaction(op->base.root->prov,belle_sip_request_event_get_request(event));
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_dialog_state_t dialog_state;
	belle_sip_header_expires_t* expires = belle_sip_message_get_header_by_type(req,belle_sip_header_expires_t);
	belle_sip_header_t *event_header;
	SalBody body;
	belle_sip_response_t* resp;
	const char *eventname=NULL;
	const char *method=belle_sip_request_get_method(req);
	
	belle_sip_object_ref(server_transaction);
	if (op->pending_server_trans)  belle_sip_object_unref(op->pending_server_trans);
	op->pending_server_trans=server_transaction;

	event_header=belle_sip_message_get_header((belle_sip_message_t*)req,"Event");
	sal_op_get_body(op,(belle_sip_message_t*)req,&body);
	
	if (event_header==NULL){
		ms_warning("No event header in incoming SUBSCRIBE.");
		resp=sal_op_create_response_from_request(op,req,400);
		belle_sip_server_transaction_send_response(server_transaction,resp);
		return;
	}
	if (op->event==NULL) {
		op->event=event_header;
		belle_sip_object_ref(op->event);
	}
	eventname=belle_sip_header_get_unparsed_value(event_header);
	
	if (!op->dialog) {
		if (strcmp(method,"SUBSCRIBE")==0){
			op->dialog=belle_sip_provider_create_dialog(op->base.root->prov,BELLE_SIP_TRANSACTION(server_transaction));
			belle_sip_dialog_set_application_data(op->dialog,op);
			sal_op_ref(op);
			ms_message("new incoming subscription from [%s] to [%s]",sal_op_get_from(op),sal_op_get_to(op));
		}else{ /*this is a NOTIFY*/
			handle_notify(op,req,eventname,&body);
			return;
		}
	}
	dialog_state=belle_sip_dialog_get_state(op->dialog);
	switch(dialog_state) {

	case BELLE_SIP_DIALOG_NULL: {
		op->base.root->callbacks.subscribe_received(op,eventname,body.type ? &body : NULL);
		break;
	}
	case BELLE_SIP_DIALOG_EARLY:
		ms_error("unexpected method [%s] for dialog [%p] in state BELLE_SIP_DIALOG_EARLY ",belle_sip_request_get_method(req),op->dialog);
		break;

	case BELLE_SIP_DIALOG_CONFIRMED:
		if (strcmp("NOTIFY",method)==0) {
			handle_notify(op,req,eventname,&body);
		} else if (strcmp("SUBSCRIBE",method)==0) {
			/*either a refresh of an unsubscribe*/
			if (expires && belle_sip_header_expires_get_expires(expires)>0) {
				resp=sal_op_create_response_from_request(op,req,200);
				belle_sip_server_transaction_send_response(server_transaction,resp);
			} else if(expires) {
				ms_message("Unsubscribe received from [%s]",sal_op_get_from(op));
				resp=sal_op_create_response_from_request(op,req,200);
				belle_sip_server_transaction_send_response(server_transaction,resp);
				op->base.root->callbacks.subscribe_closed(op);
			}
		}
		break;
		default: {
			ms_error("unexpected dialog state [%s]",belle_sip_dialog_state_to_string(dialog_state));
		}
	}
}

void sal_op_subscribe_fill_cbs(SalOp*op) {
	op->callbacks.process_io_error=subscribe_process_io_error;
	op->callbacks.process_response_event=subscribe_response_event;
	op->callbacks.process_timeout=subscribe_process_timeout;
	op->callbacks.process_transaction_terminated=subscribe_process_transaction_terminated;
	op->callbacks.process_request_event=subscribe_process_request_event;
	op->callbacks.process_dialog_terminated=subscribe_process_dialog_terminated;
	op->type=SalOpSubscribe;
}


int sal_subscribe(SalOp *op, const char *from, const char *to, const char *eventname, int expires, const SalBody *body){
	belle_sip_request_t *req=NULL;
	
	if (from)
		sal_op_set_from(op,from);
	if (to)
		sal_op_set_to(op,to);
	
	if (!op->dialog){
		sal_op_subscribe_fill_cbs(op);
		/*???sal_exosip_fix_route(op); make sure to ha ;lr*/
		req=sal_op_build_request(op,"SUBSCRIBE");
		if (eventname){
			if (op->event) belle_sip_object_unref(op->event);
			op->event=belle_sip_header_create("Event",eventname);
			belle_sip_object_ref(op->event);
		}
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),op->event);
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(belle_sip_header_expires_create(expires)));
		sal_op_add_body(op,(belle_sip_message_t*)req,body);
		return sal_op_send_and_create_refresher(op,req,expires,subscribe_refresher_listener);
	}else if (op->refresher){
		const belle_sip_transaction_t *tr=(const belle_sip_transaction_t*) belle_sip_refresher_get_transaction(op->refresher);
		belle_sip_request_t *last_req=belle_sip_transaction_get_request(tr);
		/* modify last request to update body*/
		sal_op_add_body(op,(belle_sip_message_t*)last_req,body);
		return belle_sip_refresher_refresh(op->refresher,expires);
	}
	ms_warning("sal_subscribe(): no dialog and no refresher ?");
	return -1;
}

int sal_unsubscribe(SalOp *op){
	if (op->refresher){
		const belle_sip_transaction_t *tr=(const belle_sip_transaction_t*) belle_sip_refresher_get_transaction(op->refresher);
		belle_sip_request_t *last_req=belle_sip_transaction_get_request(tr);
		sal_op_add_body(op,(belle_sip_message_t*)last_req,NULL);
		belle_sip_refresher_refresh(op->refresher,0);
		return 0;
	}
	return -1;
}

int sal_subscribe_accept(SalOp *op){
	belle_sip_request_t* req=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(op->pending_server_trans));
	belle_sip_header_expires_t* expires = belle_sip_message_get_header_by_type(req,belle_sip_header_expires_t);
	belle_sip_response_t* resp = sal_op_create_response_from_request(op,req,200);
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(resp),BELLE_SIP_HEADER(expires));
	belle_sip_server_transaction_send_response(op->pending_server_trans,resp);
	return 0;
}

int sal_subscribe_decline(SalOp *op, SalReason reason){
	belle_sip_response_t*  resp = belle_sip_response_create_from_request(belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(op->pending_server_trans)),
									   sal_reason_to_sip_code(reason));
	belle_sip_server_transaction_send_response(op->pending_server_trans,resp);
	return 0;
}

int sal_notify(SalOp *op, const SalBody *body){
	belle_sip_request_t* notify;
	
	if (!op->dialog) return -1;
	
	if (!(notify=belle_sip_dialog_create_queued_request(op->dialog,"NOTIFY"))) return -1;

	if (op->event) belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),op->event);

	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
			,BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_ACTIVE,600)));
	
	sal_op_add_body(op,(belle_sip_message_t*)notify, body);
	return sal_op_send_request(op,notify);
}

int sal_notify_close(SalOp *op){
	belle_sip_request_t* notify;
	if (!op->dialog) return -1;
	if (!(notify=belle_sip_dialog_create_queued_request(op->dialog,"NOTIFY"))) return -1;
	if (op->event) belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),op->event);
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
		,BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_TERMINATED,-1)));
	return sal_op_send_request(op,notify);
}

