/*
 * presence-op.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "c-wrapper/internal/c-tools.h"
#include "sal/presence-op.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

void SalPresenceOp::presenceProcessIoErrorCb(void *user_ctx, const belle_sip_io_error_event_t *event) {
	SalPresenceOp * op = (SalPresenceOp *)user_ctx;
	belle_sip_request_t* request;
	belle_sip_client_transaction_t* client_transaction = NULL;
	
	if (BELLE_SIP_OBJECT_IS_INSTANCE_OF(belle_sip_io_error_event_get_source(event),
		belle_sip_client_transaction_t)){
		 client_transaction = (belle_sip_client_transaction_t*)belle_sip_io_error_event_get_source(event);
	}
	
	if (!client_transaction) return;
	
	request = belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
	
	if (strcmp("SUBSCRIBE",belle_sip_request_get_method(request))==0){
		if (op->mRefresher){
			ms_warning("presence_process_io_error() refresher is present, should not happen");
			return;
		}
		ms_message("subscription to [%s] io error",op->getTo().c_str());
		if (!op->mOpReleased){
			op->mRoot->mCallbacks.notify_presence(op,SalSubscribeTerminated, NULL,NULL); /*NULL = offline*/
		}
	}
}

void SalPresenceOp::presenceRefresherListenerCb(belle_sip_refresher_t* refresher, void* user_pointer, unsigned int status_code, const char* reason_phrase, int will_retry) {
	SalPresenceOp * op = (SalPresenceOp *)user_pointer;
	if (status_code >= 300) {
		ms_message("The SUBSCRIBE dialog no longer works. Let's restart a new one.");
		belle_sip_refresher_stop(op->mRefresher);
		if (op->mDialog) { /*delete previous dialog if any*/
			op->setOrUpdateDialog(NULL);
		}

		if (op->getContactAddress()) {
			/*contact is also probably not good*/
			SalAddress* contact=sal_address_clone(op->getContactAddress());
			sal_address_set_port(contact,-1);
			sal_address_set_domain(contact,NULL);
			op->setContactAddress(contact);
			sal_address_destroy(contact);
		}
		/*send a new SUBSCRIBE, that will attempt to establish a new dialog*/
		op->subscribe(NULL,NULL,-1);
	}
	if (status_code == 0 || status_code == 503){
		/*timeout or io error: the remote doesn't seem reachable.*/
		if (!op->mOpReleased){
			op->mRoot->mCallbacks.notify_presence(op,SalSubscribeActive, NULL,NULL); /*NULL = offline*/
		}
	}
}

void SalPresenceOp::presenceResponseEventCb(void *op_base, const belle_sip_response_event_t *event) {
	SalPresenceOp * op = (SalPresenceOp *)op_base;
	belle_sip_dialog_state_t dialog_state;
	belle_sip_client_transaction_t* client_transaction = belle_sip_response_event_get_client_transaction(event);
	belle_sip_response_t* response=belle_sip_response_event_get_response(event);
	belle_sip_request_t* request=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
	int code = belle_sip_response_get_status_code(response);
	belle_sip_header_expires_t* expires;

	op->setErrorInfoFromResponse(response);

	if (code>=300) {
		if (strcmp("SUBSCRIBE",belle_sip_request_get_method(request))==0){
			ms_message("subscription to [%s] rejected",op->getTo().c_str());
			if (!op->mOpReleased){
				op->mRoot->mCallbacks.notify_presence(op,SalSubscribeTerminated, NULL,NULL); /*NULL = offline*/
			}
			return;
		}
	}
	op->setOrUpdateDialog(belle_sip_response_event_get_dialog(event));
	if (!op->mDialog) {
		ms_message("presence op [%p] receive out of dialog answer [%i]",op,code);
		return;
	}
	dialog_state=belle_sip_dialog_get_state(op->mDialog);

	switch(dialog_state) {
		case BELLE_SIP_DIALOG_NULL:
		case BELLE_SIP_DIALOG_EARLY: {
			ms_error("presence op [%p] receive an unexpected answer [%i]",op,code);
			break;
		}
		case BELLE_SIP_DIALOG_CONFIRMED: {
			if (strcmp("SUBSCRIBE",belle_sip_request_get_method(request))==0) {
				expires=belle_sip_message_get_header_by_type(request,belle_sip_header_expires_t);
				if(op->mRefresher) {
					belle_sip_refresher_stop(op->mRefresher);
					belle_sip_object_unref(op->mRefresher);
					op->mRefresher=NULL;
				}
				if ((expires != NULL) && (belle_sip_header_expires_get_expires(expires) > 0)) {
					op->mRefresher=belle_sip_client_transaction_create_refresher(client_transaction);
					belle_sip_refresher_set_listener(op->mRefresher,presenceRefresherListenerCb,op);
					belle_sip_refresher_set_realm(op->mRefresher,L_STRING_TO_C(op->mRealm));
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

void SalPresenceOp::presenceProcessTimeoutCb(void *user_ctx, const belle_sip_timeout_event_t *event) {
	SalPresenceOp * op = (SalPresenceOp *)user_ctx;
	belle_sip_client_transaction_t* client_transaction = belle_sip_timeout_event_get_client_transaction(event);
	belle_sip_request_t* request;
	
	if (!client_transaction) return;
	
	request = belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
	
	if (strcmp("SUBSCRIBE",belle_sip_request_get_method(request))==0){
		ms_message("subscription to [%s] timeout",op->getTo().c_str());
		if (!op->mOpReleased){
			op->mRoot->mCallbacks.notify_presence(op,SalSubscribeTerminated, NULL,NULL); /*NULL = offline*/
		}
	}
}

void SalPresenceOp::presenceProcessTransactionTerminatedCb(void *user_ctx, const belle_sip_transaction_terminated_event_t *event) {
	ms_message("presence_process_transaction_terminated not implemented yet");
}

SalPresenceModel *SalPresenceOp::processPresenceNotification(belle_sip_request_t *req) {
	belle_sip_header_content_type_t *content_type = belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req), belle_sip_header_content_type_t);
	belle_sip_header_content_length_t *content_length = belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req), belle_sip_header_content_length_t);
	const char *body = belle_sip_message_get_body(BELLE_SIP_MESSAGE(req));
	SalPresenceModel *result = NULL;

	if ((content_type == NULL) || (content_length == NULL))
		return NULL;
	if (belle_sip_header_content_length_get_content_length(content_length) == 0)
		return NULL;

	if (body==NULL) return NULL;
	if (!mOpReleased){
		mRoot->mCallbacks.parse_presence_requested(this,
							  belle_sip_header_content_type_get_type(content_type),
							  belle_sip_header_content_type_get_subtype(content_type),
							  body,
							  &result);
	}

	return result;
}

void SalPresenceOp::handleNotify(belle_sip_request_t *req, belle_sip_dialog_t *dialog) {
	belle_sip_response_t* resp=NULL;
	belle_sip_server_transaction_t* server_transaction= mPendingServerTransaction;
	belle_sip_header_subscription_state_t* subscription_state_header=belle_sip_message_get_header_by_type(req,belle_sip_header_subscription_state_t);
	SalSubscribeStatus sub_state;
	
	if (strcmp("NOTIFY",belle_sip_request_get_method(req))==0) {
		SalPresenceModel *presence_model = NULL;
		const char* body = belle_sip_message_get_body(BELLE_SIP_MESSAGE(req));
		
		if (mDialog !=NULL && dialog != mDialog){
			ms_warning("Receiving a NOTIFY from a dialog we haven't stored (op->dialog=%p dialog=%p)", mDialog, dialog);
		}
		if (!subscription_state_header || strcasecmp(BELLE_SIP_SUBSCRIPTION_STATE_TERMINATED,belle_sip_header_subscription_state_get_state(subscription_state_header)) ==0) {
			sub_state=SalSubscribeTerminated;
			ms_message("Outgoing subscription terminated by remote [%s]",getTo().c_str());
		} else {
			sub_state=getSubscriptionState(BELLE_SIP_MESSAGE(req));
		}
		presence_model = processPresenceNotification(req);
		if (presence_model != NULL || body==NULL) {
			/* Presence notification body parsed successfully. */

			resp = createResponseFromRequest(req, 200); /*create first because the op may be destroyed by notify_presence */
			if (!mOpReleased){
				mRoot->mCallbacks.notify_presence(this, sub_state, presence_model, NULL);
			}
		} else if (body){
			/* Formatting error in presence notification body. */
			ms_warning("Wrongly formatted presence document.");
			resp = createResponseFromRequest(req, 488);
		}
		if (resp) belle_sip_server_transaction_send_response(server_transaction,resp);
	}
}

void SalPresenceOp::presenceProcessRequestEventCb(void *op_base, const belle_sip_request_event_t *event) {
	SalPresenceOp * op = (SalPresenceOp *)op_base;
	belle_sip_server_transaction_t* server_transaction = belle_sip_provider_create_server_transaction(op->mRoot->mProvider,belle_sip_request_event_get_request(event));
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_dialog_state_t dialog_state;
	belle_sip_response_t* resp;
	const char *method=belle_sip_request_get_method(req);
	belle_sip_header_event_t *event_header;
	belle_sip_object_ref(server_transaction);
	if (op->mPendingServerTransaction)  belle_sip_object_unref(op->mPendingServerTransaction);
	op->mPendingServerTransaction=server_transaction;
	event_header=belle_sip_message_get_header_by_type(req,belle_sip_header_event_t);
	
	if (event_header==NULL){
		ms_warning("No event header in incoming SUBSCRIBE.");
		resp=op->createResponseFromRequest(req,400);
		belle_sip_server_transaction_send_response(server_transaction,resp);
		if (!op->mDialog) op->release();
		return;
	}
	if (op->mEvent==NULL) {
		op->mEvent=event_header;
		belle_sip_object_ref(op->mEvent);
	}


	if (!op->mDialog) {
		if (strcmp(method,"SUBSCRIBE")==0){
			belle_sip_dialog_t *dialog = belle_sip_provider_create_dialog(op->mRoot->mProvider,BELLE_SIP_TRANSACTION(server_transaction));
			if (!dialog){
				resp=op->createResponseFromRequest(req,481);
				belle_sip_server_transaction_send_response(server_transaction,resp);
				op->release();
				return;
			}
			op->setOrUpdateDialog(dialog);
			ms_message("new incoming subscription from [%s] to [%s]",op->getFrom().c_str(),op->getTo().c_str());
		}else if (strcmp(method,"NOTIFY")==0 && belle_sip_request_event_get_dialog(event)) {
			/*special case of dialog created by notify matching subscribe*/
			op->setOrUpdateDialog(belle_sip_request_event_get_dialog(event));
		} else {/* this is a NOTIFY */
			ms_message("Receiving out of dialog notify");
			op->handleNotify(req, belle_sip_request_event_get_dialog(event));
			return;
		}
	}
	dialog_state=belle_sip_dialog_get_state(op->mDialog);
	switch(dialog_state) {
		case BELLE_SIP_DIALOG_NULL: {
			if (strcmp("NOTIFY",method)==0) {
				op->handleNotify(req, belle_sip_request_event_get_dialog(event));
			} else if (strcmp("SUBSCRIBE",method)==0) {
				op->mRoot->mCallbacks.subscribe_presence_received(op,op->getFrom().c_str());
			}
			break;
		}
		case BELLE_SIP_DIALOG_EARLY:
			ms_error("unexpected method [%s] for dialog [%p] in state BELLE_SIP_DIALOG_EARLY ",method,op->mDialog);
			break;

		case BELLE_SIP_DIALOG_CONFIRMED:
			if (strcmp("NOTIFY",method)==0) {
				op->handleNotify(req, belle_sip_request_event_get_dialog(event));
			} else if (strcmp("SUBSCRIBE",method)==0) {
				/*either a refresh or an unsubscribe.
				 If it is a refresh there is nothing to notify to the app. If it is an unSUBSCRIBE, then the dialog
				 will be terminated shortly, and this will be notified to the app through the dialog_terminated callback.*/
				resp=op->createResponseFromRequest(req,200);
				belle_sip_server_transaction_send_response(server_transaction,resp);
			}
			break;
		default:
			ms_error("unexpected dialog state [%s]",belle_sip_dialog_state_to_string(dialog_state));
			break;
	}
}

void SalPresenceOp::presenceProcessDialogTerminatedCb(void *ctx, const belle_sip_dialog_terminated_event_t *event) {
	SalPresenceOp * op= (SalPresenceOp *)ctx;
	if (op->mDialog && belle_sip_dialog_is_server(op->mDialog)) {
			ms_message("Incoming subscribtion from [%s] terminated",op->getFrom().c_str());
			if (!op->mOpReleased){
				op->mRoot->mCallbacks.subscribe_presence_closed(op, op->getFrom().c_str());
			}
			op->setOrUpdateDialog(NULL);
	}/* else client dialog is managed by refresher*/
}

void SalPresenceOp::releaseCb(SalOp *op_base) {
	SalPresenceOp *op =(SalPresenceOp *)op_base;
	if(op->mRefresher) {
		belle_sip_refresher_stop(op->mRefresher);
		belle_sip_object_unref(op->mRefresher);
		op->mRefresher=NULL;
		op->setOrUpdateDialog(NULL); /*only if we have refresher. else dialog terminated event will remove association*/
	}
}

void SalPresenceOp::fillCallbacks() {
	static belle_sip_listener_callbacks_t op_presence_callbacks={0};
	if (op_presence_callbacks.process_request_event==NULL){
		op_presence_callbacks.process_io_error=presenceProcessIoErrorCb;
		op_presence_callbacks.process_response_event=presenceResponseEventCb;
		op_presence_callbacks.process_timeout= presenceProcessTimeoutCb;
		op_presence_callbacks.process_transaction_terminated=presenceProcessTransactionTerminatedCb;
		op_presence_callbacks.process_request_event=presenceProcessRequestEventCb;
		op_presence_callbacks.process_dialog_terminated=presenceProcessDialogTerminatedCb;
	}
	mCallbacks=&op_presence_callbacks;
	mType=Type::Presence;
	mReleaseCb=releaseCb;
}

int SalPresenceOp::subscribe(const char *from, const char *to, int expires) {
	belle_sip_request_t *req=NULL;
	if (from)
		      setFrom(from);
	if (to)
		      setTo(to);

	   fillCallbacks();

	if (expires==-1){
		if (mRefresher){
			expires=belle_sip_refresher_get_expires(mRefresher);
			belle_sip_object_unref(mRefresher);
			mRefresher=NULL;
		}else{
			ms_error("sal_subscribe_presence(): cannot guess expires from previous refresher.");
			return -1;
		}
	}
	if (!mEvent){
		mEvent=belle_sip_header_event_create("presence");
		belle_sip_object_ref(mEvent);
	}
	belle_sip_parameters_remove_parameter(BELLE_SIP_PARAMETERS(mFromAddress),"tag");
	belle_sip_parameters_remove_parameter(BELLE_SIP_PARAMETERS(mToAddress),"tag");
	req=buildRequest("SUBSCRIBE");
	if( req ){
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(mEvent));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(belle_sip_header_expires_create(expires)));
	}

	return sendRequest(req);
}

int SalPresenceOp::checkDialogState() {
	belle_sip_dialog_state_t state= mDialog?belle_sip_dialog_get_state(mDialog): BELLE_SIP_DIALOG_NULL;
	if (state != BELLE_SIP_DIALOG_CONFIRMED) {
		ms_warning("Cannot notify presence for op [%p] because dialog in state [%s]", this, belle_sip_dialog_state_to_string(state));
		return -1;
	} else
		return 0;
}

belle_sip_request_t *SalPresenceOp::createPresenceNotify() {
	belle_sip_request_t* notify=belle_sip_dialog_create_queued_request(mDialog,"NOTIFY");
	if (!notify) return NULL;

	belle_sip_message_add_header((belle_sip_message_t*)notify,belle_sip_header_create("Event","presence"));
	return notify;
}

void SalPresenceOp::addPresenceInfo(belle_sip_message_t *notify, SalPresenceModel *presence) {
	char *contact_info;
	char *content = NULL;
	size_t content_length;

	if (presence){
		belle_sip_header_from_t *from=belle_sip_message_get_header_by_type(notify,belle_sip_header_from_t);
		contact_info=belle_sip_uri_to_string(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(from)));
		mRoot->mCallbacks.convert_presence_to_xml_requested(this, presence, contact_info, &content);
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

int SalPresenceOp::notifyPresence(SalPresenceModel *presence) {
	belle_sip_request_t* notify=NULL;
	if (checkDialogState()) {
		return -1;
	}
	notify=createPresenceNotify();
	if (!notify) return-1;

	   addPresenceInfo(BELLE_SIP_MESSAGE(notify),presence); /*FIXME, what about expires ??*/
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
			,BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_ACTIVE,600)));
	return sendRequest(notify);
}

int SalPresenceOp::notifyPresenceClose() {
	belle_sip_request_t* notify=NULL;
	int status;
	if (checkDialogState()) {
		return -1;
	}
	notify=createPresenceNotify();
	if (!notify) return-1;

	   addPresenceInfo(BELLE_SIP_MESSAGE(notify),NULL); /*FIXME, what about expires ??*/
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
		,BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_TERMINATED,-1)));
	status = sendRequest(notify);
	   setOrUpdateDialog(NULL);  /*because we may be chalanged for the notify, so we must release dialog right now*/
	return status;
}

SalSubscribeStatus SalPresenceOp::getSubscriptionState(const belle_sip_message_t *msg) {
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

LINPHONE_END_NAMESPACE
