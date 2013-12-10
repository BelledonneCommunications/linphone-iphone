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


static void publish_refresher_listener (belle_sip_refresher_t* refresher
		,void* user_pointer
		,unsigned int status_code
		,const char* reason_phrase) {
	SalOp* op = (SalOp*)user_pointer;
	/*belle_sip_response_t* response=belle_sip_transaction_get_response(BELLE_SIP_TRANSACTION(belle_sip_refresher_get_transaction(refresher)));*/
	ms_message("Publish refresher  [%i] reason [%s] for proxy [%s]",status_code,reason_phrase?reason_phrase:"none",sal_op_get_proxy(op));
	if (status_code==412){
		/*resubmit the request after removing the SIP-If-Match*/
		const belle_sip_client_transaction_t* last_publish_trans=belle_sip_refresher_get_transaction(op->refresher);
		belle_sip_request_t* last_publish=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(last_publish_trans));
		belle_sip_message_remove_header((belle_sip_message_t*)last_publish,"SIP-If-Match");
		belle_sip_refresher_refresh(op->refresher,BELLE_SIP_REFRESHER_REUSE_EXPIRES);
	}else if (status_code==0){
		op->base.root->callbacks.on_expire(op);
	}else if (status_code>=200){
		SalError err;
		SalReason reason;
		sal_compute_sal_errors_from_code(status_code,&err,&reason);
		op->base.root->callbacks.on_publish_response(op,err,reason);
	}
}

static void publish_response_event(void *userctx, const belle_sip_response_event_t *event){
	SalOp *op=(SalOp*)userctx;
	int code=belle_sip_response_get_status_code(belle_sip_response_event_get_response(event));
	SalError err;
	SalReason reason;
	sal_compute_sal_errors_from_code(code,&err,&reason);
	op->base.root->callbacks.on_publish_response(op,err,reason);
}

void sal_op_publish_fill_cbs(SalOp*op) {
	op->callbacks.process_response_event=publish_response_event;
	op->type=SalOpPublish;
}

/*
 * Sending a publish with 0 expires removes the event state and such request shall not contain a body.
 * See RFC3903, section 4.5
 */

/*presence publish */
int sal_publish_presence(SalOp *op, const char *from, const char *to, int expires, SalPresenceModel *presence){
	belle_sip_request_t *req=NULL;
	if(!op->refresher || !belle_sip_refresher_get_transaction(op->refresher)) {
		if (from)
			sal_op_set_from(op,from);
		if (to)
			sal_op_set_to(op,to);

		op->type=SalOpPublish;
		req=sal_op_build_request(op,"PUBLISH");
		if (sal_op_get_contact(op)){
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(sal_op_create_contact(op)));
		}
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),belle_sip_header_create("Event","presence"));
		sal_add_presence_info(op,BELLE_SIP_MESSAGE(req),presence);
		return sal_op_send_and_create_refresher(op,req,expires,publish_refresher_listener);
	} else {
		/*update presence status*/
		const belle_sip_client_transaction_t* last_publish_trans=belle_sip_refresher_get_transaction(op->refresher);
		belle_sip_request_t* last_publish=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(last_publish_trans));
		sal_add_presence_info(op,BELLE_SIP_MESSAGE(last_publish),expires!=0 ? presence : NULL);
		return belle_sip_refresher_refresh(op->refresher,expires);
	}
}

int sal_publish(SalOp *op, const char *from, const char *to, const char *eventname, int expires, const SalBody *body){
	belle_sip_request_t *req=NULL;
	if(!op->refresher || !belle_sip_refresher_get_transaction(op->refresher)) {
		if (from)
			sal_op_set_from(op,from);
		if (to)
			sal_op_set_to(op,to);

		sal_op_publish_fill_cbs(op);
		req=sal_op_build_request(op,"PUBLISH");
		if (sal_op_get_contact(op)){
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(sal_op_create_contact(op)));
		}
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),belle_sip_header_create("Event",eventname));
		sal_op_add_body(op,BELLE_SIP_MESSAGE(req),body);
		return sal_op_send_and_create_refresher(op,req,expires,publish_refresher_listener);
	} else {
		/*update status*/
		const belle_sip_client_transaction_t* last_publish_trans=belle_sip_refresher_get_transaction(op->refresher);
		belle_sip_request_t* last_publish=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(last_publish_trans));
		/*update body*/
		sal_op_add_body(op,BELLE_SIP_MESSAGE(last_publish),expires!=0 ? body : NULL);
		return belle_sip_refresher_refresh(op->refresher,expires==-1 ? BELLE_SIP_REFRESHER_REUSE_EXPIRES : expires);
	}
}
