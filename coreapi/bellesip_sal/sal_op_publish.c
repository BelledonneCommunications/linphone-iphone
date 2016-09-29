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


static void publish_refresher_listener (belle_sip_refresher_t* refresher
		,void* user_pointer
		,unsigned int status_code
		,const char* reason_phrase, int will_retry) {
	SalOp* op = (SalOp*)user_pointer;
	const belle_sip_client_transaction_t* last_publish_trans=belle_sip_refresher_get_transaction(op->refresher);
	belle_sip_response_t *response=belle_sip_transaction_get_response(BELLE_SIP_TRANSACTION(last_publish_trans));
	ms_message("Publish refresher  [%i] reason [%s] for proxy [%s]",status_code,reason_phrase?reason_phrase:"none",sal_op_get_proxy(op));
	if (status_code==0){
		op->base.root->callbacks.on_expire(op);
	}else if (status_code>=200){
		belle_sip_header_t *sip_etag;
		const char *sip_etag_string = NULL;
		if (response && (sip_etag = belle_sip_message_get_header(BELLE_SIP_MESSAGE(response), "SIP-ETag"))) {
			sip_etag_string = belle_sip_header_get_unparsed_value(sip_etag);
		}
		sal_op_set_entity_tag(op, sip_etag_string);
		sal_error_info_set(&op->error_info,SalReasonUnknown,status_code,reason_phrase,NULL);
		sal_op_assign_recv_headers(op,(belle_sip_message_t*)response);
		op->base.root->callbacks.on_publish_response(op);
	}
}

static void publish_response_event(void *userctx, const belle_sip_response_event_t *event){
	SalOp *op=(SalOp*)userctx;
	sal_op_set_error_info_from_response(op,belle_sip_response_event_get_response(event));
	if (op->error_info.protocol_code>=200){
		op->base.root->callbacks.on_publish_response(op);
	}
}

static belle_sip_listener_callbacks_t op_publish_callbacks={0};

void sal_op_publish_fill_cbs(SalOp *op) {
	if (op_publish_callbacks.process_response_event==NULL){
		op_publish_callbacks.process_response_event=publish_response_event;
	}
	op->callbacks=&op_publish_callbacks;
	op->type=SalOpPublish;
}

int sal_publish(SalOp *op, const char *from, const char *to, const char *eventname, int expires, const SalBodyHandler *body_handler){
	belle_sip_request_t *req=NULL;
	if(!op->refresher || !belle_sip_refresher_get_transaction(op->refresher)) {
		if (from)
			sal_op_set_from(op,from);
		if (to)
			sal_op_set_to(op,to);

		sal_op_publish_fill_cbs(op);
		req=sal_op_build_request(op,"PUBLISH");
		if( req == NULL ){
			return -1;
		}
		
		if (sal_op_get_entity_tag(op)) {
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),belle_sip_header_create("SIP-If-Match", sal_op_get_entity_tag(op)));
		}
		
		if (sal_op_get_contact_address(op)){
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(sal_op_create_contact(op)));
		}
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),belle_sip_header_create("Event",eventname));
		belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(req), BELLE_SIP_BODY_HANDLER(body_handler));
		if (expires!=-1)
			return sal_op_send_and_create_refresher(op,req,expires,publish_refresher_listener);
		else return sal_op_send_request(op,req);
	} else {
		/*update status*/
		const belle_sip_client_transaction_t* last_publish_trans=belle_sip_refresher_get_transaction(op->refresher);
		belle_sip_request_t* last_publish=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(last_publish_trans));
		/*update body*/
		if (expires == 0) {
			belle_sip_message_set_body(BELLE_SIP_MESSAGE(last_publish), NULL, 0);
		} else {
			belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(last_publish), BELLE_SIP_BODY_HANDLER(body_handler));
		}
		return belle_sip_refresher_refresh(op->refresher,expires==-1 ? BELLE_SIP_REFRESHER_REUSE_EXPIRES : expires);
	}
}

int sal_op_unpublish(SalOp *op){
	if (op->refresher){
		const belle_sip_transaction_t *tr=(const belle_sip_transaction_t*) belle_sip_refresher_get_transaction(op->refresher);
		belle_sip_request_t *last_req=belle_sip_transaction_get_request(tr);
		belle_sip_message_set_body(BELLE_SIP_MESSAGE(last_req), NULL, 0);
		belle_sip_refresher_refresh(op->refresher,0);
		return 0;
	}
	return -1;
}
