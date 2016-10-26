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


void sal_add_presence_info(SalOp *op, belle_sip_message_t *notify, SalPresenceModel *presence) {
	char *contact_info;
	char *content = NULL;
	size_t content_length;

	if (presence){
		belle_sip_header_from_t *from=belle_sip_message_get_header_by_type(notify,belle_sip_header_from_t);
		contact_info=belle_sip_uri_to_string(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(from)));
		op->base.root->callbacks.convert_presence_to_xml_requested(op, presence, contact_info, &content);
		belle_sip_free(contact_info);
		if (content == NULL) return;
	}

	belle_sip_message_remove_header(BELLE_SIP_MESSAGE(notify),BELLE_SIP_CONTENT_TYPE);
	belle_sip_message_remove_header(BELLE_SIP_MESSAGE(notify),BELLE_SIP_CONTENT_LENGTH);
	belle_sip_message_set_body(BELLE_SIP_MESSAGE(notify),NULL,0);

	if (content){
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
								,BELLE_SIP_HEADER(belle_sip_header_content_type_create("application","pidf+xml")));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
								,BELLE_SIP_HEADER(belle_sip_header_content_length_create(content_length=strlen(content))));
		belle_sip_message_set_body(BELLE_SIP_MESSAGE(notify),content,content_length);
		ms_free(content);
	}
}

static void presence_process_io_error(void *user_ctx, const belle_sip_io_error_event_t *event){
	SalOp* op = (SalOp*)user_ctx;
	belle_sip_request_t* request;
	belle_sip_client_transaction_t* client_transaction = NULL;
	
	if (BELLE_SIP_OBJECT_IS_INSTANCE_OF(belle_sip_io_error_event_get_source(event),
		belle_sip_client_transaction_t)){
		 client_transaction = (belle_sip_client_transaction_t*)belle_sip_io_error_event_get_source(event);
	}
	
	if (!client_transaction) return;
	
	request = belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
	
	if (strcmp("SUBSCRIBE",belle_sip_request_get_method(request))==0){
		if (op->refresher){
			ms_warning("presence_process_io_error() refresher is present, should not happen");
			return;
		}
		ms_message("subscription to [%s] io error",sal_op_get_to(op));
		if (!op->op_released){
			op->base.root->callbacks.notify_presence(op,SalSubscribeTerminated, NULL,NULL); /*NULL = offline*/
		}
	}
}

static void presence_process_dialog_terminated(void *ctx, const belle_sip_dialog_terminated_event_t *event) {
	SalOp* op= (SalOp*)ctx;
	if (op->dialog && belle_sip_dialog_is_server(op->dialog)) {
			ms_message("Incoming subscribtion from [%s] terminated",sal_op_get_from(op));
			if (!op->op_released){
				op->base.root->callbacks.subscribe_presence_closed(op, sal_op_get_from(op));
			}
			set_or_update_dialog(op, NULL);
	}/* else client dialog is managed by refresher*/
}

static void presence_refresher_listener(belle_sip_refresher_t* refresher, void* user_pointer, unsigned int status_code, const char* reason_phrase, int will_retry){
	SalOp* op = (SalOp*)user_pointer;
	if (status_code >= 300) {
		ms_message("The SUBSCRIBE dialog no longer works. Let's restart a new one.");
		belle_sip_refresher_stop(op->refresher);
		if (op->dialog) { /*delete previous dialog if any*/
			set_or_update_dialog(op, NULL);
		}

		if (sal_op_get_contact_address(op)) {
			/*contact is also probably not good*/
			SalAddress* contact=sal_address_clone(sal_op_get_contact_address(op));
			sal_address_set_port(contact,-1);
			sal_address_set_domain(contact,NULL);
			sal_op_set_contact_address(op,contact);
			sal_address_destroy(contact);
		}
		/*send a new SUBSCRIBE, that will attempt to establish a new dialog*/
		sal_subscribe_presence(op,NULL,NULL,-1);
	}
	if (status_code == 0 || status_code == 503){
		/*timeout or io error: the remote doesn't seem reachable.*/
		if (!op->op_released){
			op->base.root->callbacks.notify_presence(op,SalSubscribeActive, NULL,NULL); /*NULL = offline*/
		}
	}
}


static void presence_response_event(void *op_base, const belle_sip_response_event_t *event){
	SalOp* op = (SalOp*)op_base;
	belle_sip_dialog_state_t dialog_state;
	belle_sip_client_transaction_t* client_transaction = belle_sip_response_event_get_client_transaction(event);
	belle_sip_response_t* response=belle_sip_response_event_get_response(event);
	belle_sip_request_t* request=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
	int code = belle_sip_response_get_status_code(response);
	belle_sip_header_expires_t* expires;

	sal_op_set_error_info_from_response(op,response);

	if (code>=300) {
		if (strcmp("SUBSCRIBE",belle_sip_request_get_method(request))==0){
			ms_message("subscription to [%s] rejected",sal_op_get_to(op));
			if (!op->op_released){
				op->base.root->callbacks.notify_presence(op,SalSubscribeTerminated, NULL,NULL); /*NULL = offline*/
			}
			return;
		}
	}
	set_or_update_dialog(op_base,belle_sip_response_event_get_dialog(event));
	if (!op->dialog) {
		ms_message("presence op [%p] receive out of dialog answer [%i]",op,code);
		return;
	}
	dialog_state=belle_sip_dialog_get_state(op->dialog);

	switch(dialog_state) {
		case BELLE_SIP_DIALOG_NULL:
		case BELLE_SIP_DIALOG_EARLY: {
			ms_error("presence op [%p] receive an unexpected answer [%i]",op,code);
			break;
		}
		case BELLE_SIP_DIALOG_CONFIRMED: {
			if (strcmp("SUBSCRIBE",belle_sip_request_get_method(request))==0) {
				expires=belle_sip_message_get_header_by_type(request,belle_sip_header_expires_t);
				if(op->refresher) {
					belle_sip_refresher_stop(op->refresher);
					belle_sip_object_unref(op->refresher);
					op->refresher=NULL;
				}
				if ((expires != NULL) && (belle_sip_header_expires_get_expires(expires) > 0)) {
					op->refresher=belle_sip_client_transaction_create_refresher(client_transaction);
					belle_sip_refresher_set_listener(op->refresher,presence_refresher_listener,op);
					belle_sip_refresher_set_realm(op->refresher,op->base.realm);
				}
			}
			break;
		}
		default: {
			ms_error("presence op [%p] receive answer [%i] not implemented",op,code);
		}
		/* no break */
	}
}

static void presence_process_timeout(void *user_ctx, const belle_sip_timeout_event_t *event) {
	SalOp* op = (SalOp*)user_ctx;
	belle_sip_client_transaction_t* client_transaction = belle_sip_timeout_event_get_client_transaction(event);
	belle_sip_request_t* request;
	
	if (!client_transaction) return;
	
	request = belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
	
	if (strcmp("SUBSCRIBE",belle_sip_request_get_method(request))==0){
		ms_message("subscription to [%s] timeout",sal_op_get_to(op));
		if (!op->op_released){
			op->base.root->callbacks.notify_presence(op,SalSubscribeTerminated, NULL,NULL); /*NULL = offline*/
		}
	}
}

static void presence_process_transaction_terminated(void *user_ctx, const belle_sip_transaction_terminated_event_t *event) {
	ms_message("presence_process_transaction_terminated not implemented yet");
}

static SalPresenceModel * process_presence_notification(SalOp *op, belle_sip_request_t *req) {
	belle_sip_header_content_type_t *content_type = belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req), belle_sip_header_content_type_t);
	belle_sip_header_content_length_t *content_length = belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req), belle_sip_header_content_length_t);
	const char *body = belle_sip_message_get_body(BELLE_SIP_MESSAGE(req));
	SalPresenceModel *result = NULL;

	if ((content_type == NULL) || (content_length == NULL))
		return NULL;
	if (belle_sip_header_content_length_get_content_length(content_length) == 0)
		return NULL;

	if (body==NULL) return NULL;
	if (!op->op_released){
		op->base.root->callbacks.parse_presence_requested(op,
							  belle_sip_header_content_type_get_type(content_type),
							  belle_sip_header_content_type_get_subtype(content_type),
							  body,
							  &result);
	}

	return result;
}

static void handle_notify(SalOp *op, belle_sip_request_t *req, belle_sip_dialog_t *dialog){
	belle_sip_response_t* resp=NULL;
	belle_sip_server_transaction_t* server_transaction=op->pending_server_trans;
	belle_sip_header_subscription_state_t* subscription_state_header=belle_sip_message_get_header_by_type(req,belle_sip_header_subscription_state_t);
	SalSubscribeStatus sub_state;
	
	if (strcmp("NOTIFY",belle_sip_request_get_method(req))==0) {
		SalPresenceModel *presence_model = NULL;
		const char* body = belle_sip_message_get_body(BELLE_SIP_MESSAGE(req));
		
		if (op->dialog !=NULL && dialog != op->dialog){
			ms_warning("Receiving a NOTIFY from a dialog we haven't stored (op->dialog=%p dialog=%p)", op->dialog, dialog);
		}
		if (!subscription_state_header || strcasecmp(BELLE_SIP_SUBSCRIPTION_STATE_TERMINATED,belle_sip_header_subscription_state_get_state(subscription_state_header)) ==0) {
			sub_state=SalSubscribeTerminated;
			ms_message("Outgoing subscription terminated by remote [%s]",sal_op_get_to(op));
		} else {
			sub_state=belle_sip_message_get_subscription_state(BELLE_SIP_MESSAGE(req));
		}
		presence_model = process_presence_notification(op, req);
		if (presence_model != NULL || body==NULL) {
			/* Presence notification body parsed successfully. */

			resp = sal_op_create_response_from_request(op, req, 200); /*create first because the op may be destroyed by notify_presence */
			if (!op->op_released){
				op->base.root->callbacks.notify_presence(op, sub_state, presence_model, NULL);
			}
		} else if (body){
			/* Formatting error in presence notification body. */
			ms_warning("Wrongly formatted presence document.");
			resp = sal_op_create_response_from_request(op, req, 488);
		}
		if (resp) belle_sip_server_transaction_send_response(server_transaction,resp);
	}
}

static void presence_process_request_event(void *op_base, const belle_sip_request_event_t *event) {
	SalOp* op = (SalOp*)op_base;
	belle_sip_server_transaction_t* server_transaction = belle_sip_provider_create_server_transaction(op->base.root->prov,belle_sip_request_event_get_request(event));
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_dialog_state_t dialog_state;
	belle_sip_response_t* resp;
	const char *method=belle_sip_request_get_method(req);
	belle_sip_header_event_t *event_header;
	belle_sip_object_ref(server_transaction);
	if (op->pending_server_trans)  belle_sip_object_unref(op->pending_server_trans);
	op->pending_server_trans=server_transaction;
	event_header=belle_sip_message_get_header_by_type(req,belle_sip_header_event_t);
	
	if (event_header==NULL){
		ms_warning("No event header in incoming SUBSCRIBE.");
		resp=sal_op_create_response_from_request(op,req,400);
		belle_sip_server_transaction_send_response(server_transaction,resp);
		if (!op->dialog) sal_op_release(op);
		return;
	}
	if (op->event==NULL) {
		op->event=event_header;
		belle_sip_object_ref(op->event);
	}


	if (!op->dialog) {
		if (strcmp(method,"SUBSCRIBE")==0){
			belle_sip_dialog_t *dialog = belle_sip_provider_create_dialog(op->base.root->prov,BELLE_SIP_TRANSACTION(server_transaction));
			if (!dialog){
				resp=sal_op_create_response_from_request(op,req,481);
				belle_sip_server_transaction_send_response(server_transaction,resp);
				sal_op_release(op);
				return;
			}
			set_or_update_dialog(op, dialog);
			ms_message("new incoming subscription from [%s] to [%s]",sal_op_get_from(op),sal_op_get_to(op));
		}else if (strcmp(method,"NOTIFY")==0 && belle_sip_request_event_get_dialog(event)) {
			/*special case of dialog created by notify matching subscribe*/
			set_or_update_dialog(op, belle_sip_request_event_get_dialog(event));
		} else {/* this is a NOTIFY */
			ms_message("Receiving out of dialog notify");
			handle_notify(op, req, belle_sip_request_event_get_dialog(event));
			return;
		}
	}
	dialog_state=belle_sip_dialog_get_state(op->dialog);
	switch(dialog_state) {
		case BELLE_SIP_DIALOG_NULL: {
			if (strcmp("NOTIFY",method)==0) {
				handle_notify(op, req, belle_sip_request_event_get_dialog(event));
			} else if (strcmp("SUBSCRIBE",method)==0) {
				op->base.root->callbacks.subscribe_presence_received(op,sal_op_get_from(op));
			}
			break;
		}
		case BELLE_SIP_DIALOG_EARLY:
			ms_error("unexpected method [%s] for dialog [%p] in state BELLE_SIP_DIALOG_EARLY ",method,op->dialog);
			break;

		case BELLE_SIP_DIALOG_CONFIRMED:
			if (strcmp("NOTIFY",method)==0) {
				handle_notify(op, req, belle_sip_request_event_get_dialog(event));
			} else if (strcmp("SUBSCRIBE",method)==0) {
				/*either a refresh or an unsubscribe.
				 If it is a refresh there is nothing to notify to the app. If it is an unSUBSCRIBE, then the dialog
				 will be terminated shortly, and this will be notified to the app through the dialog_terminated callback.*/
				resp=sal_op_create_response_from_request(op,req,200);
				belle_sip_server_transaction_send_response(server_transaction,resp);
			}
			break;
		default:
			ms_error("unexpected dialog state [%s]",belle_sip_dialog_state_to_string(dialog_state));
			break;
	}
}

static belle_sip_listener_callbacks_t op_presence_callbacks={0};

/*Invoke when sal_op_release is called by upper layer*/
static void sal_op_release_cb(struct SalOpBase* op_base) {
	SalOp *op =(SalOp*)op_base;
	if(op->refresher) {
		belle_sip_refresher_stop(op->refresher);
		belle_sip_object_unref(op->refresher);
		op->refresher=NULL;
		set_or_update_dialog(op,NULL); /*only if we have refresher. else dialog terminated event will remove association*/
	}
	
}
void sal_op_presence_fill_cbs(SalOp*op) {
	if (op_presence_callbacks.process_request_event==NULL){
		op_presence_callbacks.process_io_error=presence_process_io_error;
		op_presence_callbacks.process_response_event=presence_response_event;
		op_presence_callbacks.process_timeout=presence_process_timeout;
		op_presence_callbacks.process_transaction_terminated=presence_process_transaction_terminated;
		op_presence_callbacks.process_request_event=presence_process_request_event;
		op_presence_callbacks.process_dialog_terminated=presence_process_dialog_terminated;
	}
	op->callbacks=&op_presence_callbacks;
	op->type=SalOpPresence;
	op->base.release_cb=sal_op_release_cb;
}


/*presence Subscribe/notify*/
int sal_subscribe_presence(SalOp *op, const char *from, const char *to, int expires){
	belle_sip_request_t *req=NULL;
	if (from)
		sal_op_set_from(op,from);
	if (to)
		sal_op_set_to(op,to);

	sal_op_presence_fill_cbs(op);

	if (expires==-1){
		if (op->refresher){
			expires=belle_sip_refresher_get_expires(op->refresher);
			belle_sip_object_unref(op->refresher);
			op->refresher=NULL;
		}else{
			ms_error("sal_subscribe_presence(): cannot guess expires from previous refresher.");
			return -1;
		}
	}
	if (!op->event){
		op->event=belle_sip_header_event_create("presence");
		belle_sip_object_ref(op->event);
	}
	belle_sip_parameters_remove_parameter(BELLE_SIP_PARAMETERS(op->base.from_address),"tag");
	belle_sip_parameters_remove_parameter(BELLE_SIP_PARAMETERS(op->base.to_address),"tag");
	req=sal_op_build_request(op,"SUBSCRIBE");
	if( req ){
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(op->event));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(belle_sip_header_expires_create(expires)));
	}

	return sal_op_send_request(op,req);
}


static belle_sip_request_t *create_presence_notify(SalOp *op){
	belle_sip_request_t* notify=belle_sip_dialog_create_queued_request(op->dialog,"NOTIFY");
	if (!notify) return NULL;

	belle_sip_message_add_header((belle_sip_message_t*)notify,belle_sip_header_create("Event","presence"));
	return notify;
}

static int sal_op_check_dialog_state(SalOp *op) {
	belle_sip_dialog_state_t state=op->dialog?belle_sip_dialog_get_state(op->dialog): BELLE_SIP_DIALOG_NULL;
	if (state != BELLE_SIP_DIALOG_CONFIRMED) {
		ms_warning("Cannot notify presence for op [%p] because dialog in state [%s]",op, belle_sip_dialog_state_to_string(state));
		return -1;
	} else
		return 0;
}

int sal_notify_presence(SalOp *op, SalPresenceModel *presence){
	belle_sip_request_t* notify=NULL;
	if (sal_op_check_dialog_state(op)) {
		return -1;
	}
	notify=create_presence_notify(op);
	if (!notify) return-1;

	sal_add_presence_info(op,BELLE_SIP_MESSAGE(notify),presence); /*FIXME, what about expires ??*/
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
			,BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_ACTIVE,600)));
	return sal_op_send_request(op,notify);
}

int sal_notify_presence_close(SalOp *op){
	belle_sip_request_t* notify=NULL;
	int status;
	if (sal_op_check_dialog_state(op)) {
		return -1;
	}
	notify=create_presence_notify(op);
	if (!notify) return-1;

	sal_add_presence_info(op,BELLE_SIP_MESSAGE(notify),NULL); /*FIXME, what about expires ??*/
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
		,BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_TERMINATED,-1)));
	status = sal_op_send_request(op,notify);
	set_or_update_dialog(op,NULL);  /*because we may be chalanged for the notify, so we must release dialog right now*/
	return status;
}



