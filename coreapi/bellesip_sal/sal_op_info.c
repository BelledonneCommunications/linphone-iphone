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

static void process_io_error(void *user_ctx, const belle_sip_io_error_event_t *event){
}

static void process_timeout(void *user_ctx, const belle_sip_timeout_event_t *event) {
}

static void process_response_event(void *op_base, const belle_sip_response_event_t *event){
}

static void process_request_event(void *op_base, const belle_sip_request_event_t *event) {
	SalOp* op = (SalOp*)op_base;
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_server_transaction_t* server_transaction = belle_sip_provider_create_server_transaction(op->base.root->prov,req);
	belle_sip_header_content_type_t* content_type;
	belle_sip_header_content_length_t *clen=NULL;
	belle_sip_response_t* resp;
	SalBody salbody;
	const char *body = NULL;
	
	content_type=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_content_type_t);
	if (content_type){
		body=belle_sip_message_get_body(BELLE_SIP_MESSAGE(req));
		clen=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_content_length_t);
	}
	
	if (content_type && body && clen) {
		salbody.type=belle_sip_header_content_type_get_type(content_type);
		salbody.subtype=belle_sip_header_content_type_get_subtype(content_type);
		salbody.data=body;
		salbody.size=belle_sip_header_content_length_get_content_length(clen);
		op->base.root->callbacks.info_received(op,&salbody);
	} else {
		op->base.root->callbacks.info_received(op,NULL);
	}
	resp = belle_sip_response_create_from_request(req,200);
	belle_sip_server_transaction_send_response(server_transaction,resp);
	sal_op_release(op);
}

int sal_send_info(SalOp *op, const char *from, const char *to, const SalBody *body){
	belle_sip_request_t *req=sal_op_build_request(op,"INFO");
	sal_op_info_fill_cbs(op);
	if (body && body->type && body->subtype && body->data){
		belle_sip_message_add_header((belle_sip_message_t*)req,
			(belle_sip_header_t*)belle_sip_header_content_type_create(body->type,body->subtype));
		belle_sip_message_add_header((belle_sip_message_t*)req,
			(belle_sip_header_t*)belle_sip_header_content_length_create(body->size));
		belle_sip_message_set_body((belle_sip_message_t*)req,(const char*)body->data,body->size);
	}
	return sal_op_send_request(op,req);
}

void sal_op_info_fill_cbs(SalOp*op) {
	op->callbacks.process_io_error=process_io_error;
	op->callbacks.process_response_event=process_response_event;
	op->callbacks.process_timeout=process_timeout;
	op->callbacks.process_request_event=process_request_event;
	op->type=SalOpInfo;
}
