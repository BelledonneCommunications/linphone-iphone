/*
 * event-op.cpp
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

#include "sal/event-op.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

void SalSubscribeOp::subscribeProcessIoErrorCb(void *user_ctx, const belle_sip_io_error_event_t *event) {
	SalSubscribeOp *op = (SalSubscribeOp *)user_ctx;
	belle_sip_object_t *src = belle_sip_io_error_event_get_source(event);
	if (BELLE_SIP_OBJECT_IS_INSTANCE_OF(src, belle_sip_client_transaction_t)){
		belle_sip_client_transaction_t *tr = BELLE_SIP_CLIENT_TRANSACTION(src);
		belle_sip_request_t* req = belle_sip_transaction_get_request((belle_sip_transaction_t*)tr);
		const char *method=belle_sip_request_get_method(req);
	
		if (strcmp(method,"NOTIFY")==0){
			SalErrorInfo *ei=&op->mErrorInfo;
			sal_error_info_set(ei,SalReasonIOError, "SIP", 0,NULL,NULL);
			op->mRoot->mCallbacks.on_notify_response(op);
		}
	}
}

void SalSubscribeOp::subscribeResponseEventCb(void *op_base, const belle_sip_response_event_t *event){
	SalSubscribeOp *op = (SalSubscribeOp *)op_base;
	belle_sip_request_t * req;
	const char *method;
	belle_sip_client_transaction_t *tr =  belle_sip_response_event_get_client_transaction(event);

	if (!tr) return;
	req = belle_sip_transaction_get_request((belle_sip_transaction_t*)tr);
	method = belle_sip_request_get_method(req);
	
	if (strcmp(method,"NOTIFY")==0){
		op->setErrorInfoFromResponse(belle_sip_response_event_get_response(event));
		op->mRoot->mCallbacks.on_notify_response(op);
	}
}

void SalSubscribeOp::subscribeProcessTimeoutCb(void *user_ctx, const belle_sip_timeout_event_t *event) {
	SalSubscribeOp *op = (SalSubscribeOp *)user_ctx;
	belle_sip_request_t * req;
	const char *method;
	belle_sip_client_transaction_t *tr =  belle_sip_timeout_event_get_client_transaction(event);

	if (!tr) return;
	req = belle_sip_transaction_get_request((belle_sip_transaction_t*)tr);
	method = belle_sip_request_get_method(req);
	
	if (strcmp(method,"NOTIFY")==0){
		SalErrorInfo *ei=&op->mErrorInfo;
		sal_error_info_set(ei,SalReasonRequestTimeout, "SIP", 0,NULL,NULL);
		op->mRoot->mCallbacks.on_notify_response(op);
	}
}

void SalSubscribeOp::handleNotify(belle_sip_request_t *req, const char *eventname, SalBodyHandler* body_handler){
	SalSubscribeStatus sub_state;
	belle_sip_header_subscription_state_t* subscription_state_header=belle_sip_message_get_header_by_type(req,belle_sip_header_subscription_state_t);
	belle_sip_response_t* resp;
	belle_sip_server_transaction_t* server_transaction = mPendingServerTransaction;
	
	if (!subscription_state_header || strcasecmp(BELLE_SIP_SUBSCRIPTION_STATE_TERMINATED,belle_sip_header_subscription_state_get_state(subscription_state_header)) ==0) {
		sub_state=SalSubscribeTerminated;
		ms_message("Outgoing subscription terminated by remote [%s]",getTo().c_str());
	} else
		sub_state=SalSubscribeActive;
	ref();
	mRoot->mCallbacks.notify(this,sub_state,eventname,body_handler);
	resp=createResponseFromRequest(req,200);
	belle_sip_server_transaction_send_response(server_transaction,resp);
	unref();
}

void SalSubscribeOp::subscribeProcessRequestEventCb(void *op_base, const belle_sip_request_event_t *event) {
	SalSubscribeOp * op = (SalSubscribeOp *)op_base;
	belle_sip_server_transaction_t* server_transaction = belle_sip_provider_create_server_transaction(op->mRoot->mProvider,belle_sip_request_event_get_request(event));
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_dialog_state_t dialog_state;
	belle_sip_header_expires_t* expires = belle_sip_message_get_header_by_type(req,belle_sip_header_expires_t);
	belle_sip_header_event_t *event_header;
	belle_sip_body_handler_t *body_handler;
	belle_sip_response_t* resp;
	const char *eventname=NULL;
	const char *method=belle_sip_request_get_method(req);
	belle_sip_dialog_t *dialog = NULL;
	
	belle_sip_object_ref(server_transaction);
	if (op->mPendingServerTransaction)  belle_sip_object_unref(op->mPendingServerTransaction);
	op->mPendingServerTransaction=server_transaction;

	event_header=belle_sip_message_get_header_by_type(req,belle_sip_header_event_t);
	body_handler = BELLE_SIP_BODY_HANDLER(op->getBodyHandler(BELLE_SIP_MESSAGE(req)));
	
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
	eventname=belle_sip_header_event_get_package_name(event_header);
	
	if (!op->mDialog) {
		if (strcmp(method,"SUBSCRIBE")==0){
			dialog = belle_sip_provider_create_dialog(op->mRoot->mProvider,BELLE_SIP_TRANSACTION(server_transaction));
			if (!dialog){
				resp=op->createResponseFromRequest(req,481);
				belle_sip_server_transaction_send_response(server_transaction,resp);
				op->release();
				return;
			}
			op->setOrUpdateDialog(dialog);
			ms_message("new incoming subscription from [%s] to [%s]",op->getFrom().c_str(),op->getTo().c_str());
		}else{ /*this is a NOTIFY*/
			op->handleNotify(req, eventname, (SalBodyHandler *)body_handler);
			return;
		}
	}
	dialog_state=belle_sip_dialog_get_state(op->mDialog);
	switch(dialog_state) {

	case BELLE_SIP_DIALOG_NULL: {
		const char *type = NULL;
		belle_sip_header_content_type_t *content_type = belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req), belle_sip_header_content_type_t);
		if (content_type) type = belle_sip_header_content_type_get_type(content_type);
		op->mRoot->mCallbacks.subscribe_received(op, eventname, type ? (SalBodyHandler *)body_handler : NULL);
		break;
	}
	case BELLE_SIP_DIALOG_EARLY:
		ms_error("unexpected method [%s] for dialog [%p] in state BELLE_SIP_DIALOG_EARLY ",belle_sip_request_get_method(req),op->mDialog);
		break;

	case BELLE_SIP_DIALOG_CONFIRMED:
		if (strcmp("NOTIFY",method)==0) {
			op->handleNotify(req, eventname, (SalBodyHandler *)body_handler);
		} else if (strcmp("SUBSCRIBE",method)==0) {
			/*either a refresh of an unsubscribe*/
			if (expires && belle_sip_header_expires_get_expires(expires)>0) {
				resp=op->createResponseFromRequest(req,200);
				belle_sip_server_transaction_send_response(server_transaction,resp);
			} else if(expires) {
				ms_message("Unsubscribe received from [%s]",op->getFrom().c_str());
				resp=op->createResponseFromRequest(req,200);
				belle_sip_server_transaction_send_response(server_transaction,resp);
				op->mRoot->mCallbacks.incoming_subscribe_closed(op);
			}
		}
		break;
		default: {
			ms_error("unexpected dialog state [%s]",belle_sip_dialog_state_to_string(dialog_state));
		}
	}
}

void SalSubscribeOp::subscribeProcessDialogTerminatedCb(void *ctx, const belle_sip_dialog_terminated_event_t *event) {
	belle_sip_dialog_t *dialog = belle_sip_dialog_terminated_event_get_dialog(event);
	SalSubscribeOp * op= (SalSubscribeOp *)ctx;
	if (op->mDialog) {
		if (belle_sip_dialog_terminated_event_is_expired(event)){
			if (!belle_sip_dialog_is_server(dialog)){
				/*notify the app that our subscription is dead*/
				const char *eventname = NULL;
				if (op->mEvent){
					eventname = belle_sip_header_event_get_package_name(op->mEvent);
				}
				op->mRoot->mCallbacks.notify(op, SalSubscribeTerminated, eventname, NULL);
			}else{
				op->mRoot->mCallbacks.incoming_subscribe_closed(op);
			}
		}
		op->setOrUpdateDialog(NULL);
	}
}

void SalSubscribeOp::releaseCb(SalOp *op_base) {
	auto *op =reinterpret_cast<SalSubscribeOp *>(op_base);
	if(op->mRefresher) {
		belle_sip_refresher_stop(op->mRefresher);
		belle_sip_object_unref(op->mRefresher);
		op->mRefresher=NULL;
		op->setOrUpdateDialog(NULL); /*only if we have refresher. else dialog terminated event will remove association*/
	}
}

void SalSubscribeOp::fillCallbacks() {
	static belle_sip_listener_callbacks_t op_subscribe_callbacks={0};
	if (op_subscribe_callbacks.process_io_error==NULL){
		op_subscribe_callbacks.process_io_error=subscribeProcessIoErrorCb;
		op_subscribe_callbacks.process_response_event=subscribeResponseEventCb;
		op_subscribe_callbacks.process_timeout=subscribeProcessTimeoutCb;
		op_subscribe_callbacks.process_transaction_terminated=subscribeProcessTransactionTerminatedCb;
		op_subscribe_callbacks.process_request_event=subscribeProcessRequestEventCb;
		op_subscribe_callbacks.process_dialog_terminated=subscribeProcessDialogTerminatedCb;
	}
	mCallbacks=&op_subscribe_callbacks;
	mType=Type::Subscribe;
	mReleaseCb=releaseCb;
}

void SalSubscribeOp::subscribeRefresherListenerCb (belle_sip_refresher_t* refresher,void* user_pointer,unsigned int status_code,const char* reason_phrase, int will_retry) {
	SalSubscribeOp * op = (SalSubscribeOp *)user_pointer;
	belle_sip_transaction_t *tr=BELLE_SIP_TRANSACTION(belle_sip_refresher_get_transaction(refresher));
	/*belle_sip_response_t* response=belle_sip_transaction_get_response(tr);*/
	SalSubscribeStatus sss=SalSubscribeTerminated;
	
	ms_message("Subscribe refresher  [%i] reason [%s] ",status_code,reason_phrase?reason_phrase:"none");
	if (status_code>=200 && status_code<300){
		if (status_code==200) sss=SalSubscribeActive;
		else if (status_code==202) sss=SalSubscribePending;
		op->setOrUpdateDialog(belle_sip_transaction_get_dialog(tr));
		op->mRoot->mCallbacks.subscribe_response(op,sss, will_retry);
	} else if (status_code >= 300) {
		SalReason reason = SalReasonUnknown;
		if (status_code == 503) { /*refresher returns 503 for IO error*/
			reason = SalReasonIOError;
		}
		sal_error_info_set(&op->mErrorInfo, reason, "SIP", (int)status_code, reason_phrase, NULL);
		op->mRoot->mCallbacks.subscribe_response(op,sss, will_retry);
	}else if (status_code==0){
		op->mRoot->mCallbacks.on_expire(op);
	}
	
}

int SalSubscribeOp::subscribe(const char *from, const char *to, const char *eventname, int expires, const SalBodyHandler *body_handler) {
	belle_sip_request_t *req=NULL;
	
	if (from)
		      setFrom(from);
	if (to)
		      setTo(to);
	
	if (!mDialog){
		      fillCallbacks();
		req=buildRequest("SUBSCRIBE");
		if( req == NULL ) {
			return -1;
		}
		      setEvent(eventname);
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(mEvent));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(belle_sip_header_expires_create(expires)));
		belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(req), BELLE_SIP_BODY_HANDLER(body_handler));
		return sendRequestAndCreateRefresher(req,expires,subscribeRefresherListenerCb);
	}else if (mRefresher){
		const belle_sip_transaction_t *tr=(const belle_sip_transaction_t*) belle_sip_refresher_get_transaction(mRefresher);
		belle_sip_request_t *last_req=belle_sip_transaction_get_request(tr);
		/* modify last request to update body*/
		belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(last_req), BELLE_SIP_BODY_HANDLER(body_handler));
		return belle_sip_refresher_refresh(mRefresher,expires);
	}
	ms_warning("sal_subscribe(): no dialog and no refresher ?");
	return -1;
}

int SalSubscribeOp::accept() {
	belle_sip_request_t* req=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(mPendingServerTransaction));
	belle_sip_header_expires_t* expires = belle_sip_message_get_header_by_type(req,belle_sip_header_expires_t);
	belle_sip_response_t* resp = createResponseFromRequest(req,200);
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(resp),BELLE_SIP_HEADER(expires));
	belle_sip_server_transaction_send_response(mPendingServerTransaction,resp);
	return 0;
}

int SalSubscribeOp::decline(SalReason reason) {
	belle_sip_response_t*  resp = belle_sip_response_create_from_request(belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(mPendingServerTransaction)),
									   to_sip_code(reason));
	belle_sip_server_transaction_send_response(mPendingServerTransaction,resp);
	return 0;
}

int SalSubscribeOp::notifyPendingState() {
	
	if (mDialog != NULL && mPendingServerTransaction) {
		belle_sip_request_t* notify;
		belle_sip_header_subscription_state_t* sub_state;
		ms_message("Sending NOTIFY with subscription state pending for op [%p]", this);
		if (!(notify=belle_sip_dialog_create_request(mDialog,"NOTIFY"))) {
			ms_error("Cannot create NOTIFY on op [%p]", this);
			return -1;
		}
		if (mEvent) belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),BELLE_SIP_HEADER(mEvent));
		sub_state=belle_sip_header_subscription_state_new();
		belle_sip_header_subscription_state_set_state(sub_state,BELLE_SIP_SUBSCRIPTION_STATE_PENDING);
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify), BELLE_SIP_HEADER(sub_state));
		return sendRequest(notify);
	} else {
		ms_warning("NOTIFY with subscription state pending for op [%p] not implemented in this case (either dialog pending trans does not exist", this);
	}
	
	return 0;
}

int SalSubscribeOp::notify(const SalBodyHandler *body_handler) {
	belle_sip_request_t* notify;
	
	if (mDialog){
		if (!(notify=belle_sip_dialog_create_queued_request(mDialog,"NOTIFY"))) return -1;
	}else{
		      fillCallbacks();
		notify = buildRequest("NOTIFY");
	}

	if (mEvent) belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),BELLE_SIP_HEADER(mEvent));

	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
			, mDialog ? 
				BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_ACTIVE,600)) :
				BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_TERMINATED,0))
				);
	belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(notify), BELLE_SIP_BODY_HANDLER(body_handler));
	return sendRequest(notify);
}

int SalSubscribeOp::closeNotify() {
	belle_sip_request_t* notify;
	if (!mDialog) return -1;
	if (!(notify=belle_sip_dialog_create_queued_request(mDialog,"NOTIFY"))) return -1;
	if (mEvent) belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),BELLE_SIP_HEADER(mEvent));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
		,BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_TERMINATED,-1)));
	return sendRequest(notify);
}

void SalPublishOp::publishResponseEventCb(void *userctx, const belle_sip_response_event_t *event) {
	SalPublishOp *op=(SalPublishOp *)userctx;
	op->setErrorInfoFromResponse(belle_sip_response_event_get_response(event));
	if (op->mErrorInfo.protocol_code>=200){
		op->mRoot->mCallbacks.on_publish_response(op);
	}
}

void SalPublishOp::fillCallbacks() {
	static belle_sip_listener_callbacks_t op_publish_callbacks={0};
	if (op_publish_callbacks.process_response_event==NULL){
		op_publish_callbacks.process_response_event=publishResponseEventCb;
	}
	
	mCallbacks=&op_publish_callbacks;
	mType=Type::Publish;
}

void SalPublishOp::publishRefresherListenerCb (belle_sip_refresher_t* refresher,void* user_pointer,unsigned int status_code,const char* reason_phrase, int will_retry) {
	SalPublishOp * op = (SalPublishOp *)user_pointer;
	const belle_sip_client_transaction_t* last_publish_trans=belle_sip_refresher_get_transaction(op->mRefresher);
	belle_sip_response_t *response=belle_sip_transaction_get_response(BELLE_SIP_TRANSACTION(last_publish_trans));
	ms_message("Publish refresher  [%i] reason [%s] for proxy [%s]",status_code,reason_phrase?reason_phrase:"none",op->getProxy().c_str());
	if (status_code==0){
		op->mRoot->mCallbacks.on_expire(op);
	}else if (status_code>=200){
		belle_sip_header_t *sip_etag;
		string sipEtagStr;
		if (response && (sip_etag = belle_sip_message_get_header(BELLE_SIP_MESSAGE(response), "SIP-ETag"))) {
			sipEtagStr = belle_sip_header_get_unparsed_value(sip_etag);
		}
		op->setEntityTag(sipEtagStr);
		sal_error_info_set(&op->mErrorInfo,SalReasonUnknown, "SIP", (int)status_code, reason_phrase, NULL);
		op->assignRecvHeaders((belle_sip_message_t*)response);
		op->mRoot->mCallbacks.on_publish_response(op);
	}
}

int SalPublishOp::publish(const char *from, const char *to, const char *eventname, int expires, const SalBodyHandler *body_handler) {
	belle_sip_request_t *req=NULL;
	if(!mRefresher || !belle_sip_refresher_get_transaction(mRefresher)) {
		if (from)
			         setFrom(from);
		if (to)
			         setTo(to);

		      fillCallbacks();
		req=buildRequest("PUBLISH");
		if( req == NULL ){
			return -1;
		}

		if (!mEntityTag.empty())
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(req), belle_sip_header_create("SIP-If-Match", mEntityTag.c_str()));

		if (getContactAddress()){
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(createContact()));
		}
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),belle_sip_header_create("Event",eventname));
		belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(req), BELLE_SIP_BODY_HANDLER(body_handler));
		if (expires!=-1)
			return sendRequestAndCreateRefresher(req,expires,publishRefresherListenerCb);
		else return sendRequest(req);
	} else {
		/*update status*/
		const belle_sip_client_transaction_t* last_publish_trans=belle_sip_refresher_get_transaction(mRefresher);
		belle_sip_request_t* last_publish=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(last_publish_trans));
		/*update body*/
		if (expires == 0) {
			belle_sip_message_set_body(BELLE_SIP_MESSAGE(last_publish), NULL, 0);
		} else {
			belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(last_publish), BELLE_SIP_BODY_HANDLER(body_handler));
		}
		return belle_sip_refresher_refresh(mRefresher,expires==-1 ? BELLE_SIP_REFRESHER_REUSE_EXPIRES : expires);
	}
}

int SalPublishOp::unpublish() {
	if (mRefresher){
		const belle_sip_transaction_t *tr=(const belle_sip_transaction_t*) belle_sip_refresher_get_transaction(mRefresher);
		belle_sip_request_t *last_req=belle_sip_transaction_get_request(tr);
		belle_sip_message_set_body(BELLE_SIP_MESSAGE(last_req), NULL, 0);
		belle_sip_refresher_refresh(mRefresher,0);
		return 0;
	}
	return -1;
}

LINPHONE_END_NAMESPACE
