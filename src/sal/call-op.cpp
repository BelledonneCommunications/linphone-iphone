/*
 * call-op.cpp
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

#include "bellesip_sal/sal_impl.h"
#include "offeranswer.h"
#include "sal/call-op.h"

#include <bctoolbox/defs.h>
#include <belle-sip/provider.h>

#include "content/content-disposition.h"
#include "content/content-type.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

SalCallOp::~SalCallOp() {
	if (mLocalMedia) sal_media_description_unref(mLocalMedia);
	if (mRemoteMedia) sal_media_description_unref(mRemoteMedia);
}

int SalCallOp::setLocalMediaDescription(SalMediaDescription *desc) {
	if (desc) {
		sal_media_description_ref(desc);
		belle_sip_error_code error;
		belle_sdp_session_description_t *sdp = media_description_to_sdp(desc);
		vector<char> buffer = marshalMediaDescription(sdp, error);
		if (error != BELLE_SIP_OK) return -1;

		mLocalBody.setContentType(ContentType::Sdp);
		mLocalBody.setBody(move(buffer));
	} else {
		mLocalBody = Content();
	}

	if (mLocalMedia)
		sal_media_description_unref(mLocalMedia);
	mLocalMedia=desc;

	if (mRemoteMedia){
		/*case of an incoming call where we modify the local capabilities between the time
		 * the call is ringing and it is accepted (for example if you want to accept without video*/
		/*reset the sdp answer so that it is computed again*/
		if (mSdpAnswer){
			belle_sip_object_unref(mSdpAnswer);
			mSdpAnswer=NULL;
		}
	}
	return 0;
}

int SalCallOp::setLocalBody(const Content &body) {
	Content bodyCopy = body;
	return setLocalBody(move(bodyCopy));
}

int SalCallOp::setLocalBody(const Content &&body) {
	if (!body.isValid()) return -1;

	if (body.getContentType() == ContentType::Sdp) {
		SalMediaDescription *desc = NULL;
		if (body.getSize() > 0) {
			belle_sdp_session_description_t *sdp = belle_sdp_session_description_parse(body.getBodyAsString().c_str());
			if (sdp == NULL) return -1;
			desc = sal_media_description_new();
			if (sdp_to_media_description(sdp, desc) != 0) {
				sal_media_description_unref(desc);
				return -1;
			}
		}
		if (mLocalMedia) sal_media_description_unref(mLocalMedia);
		mLocalMedia = desc;
	}

	mLocalBody = body;
	return 0;
}

belle_sip_header_allow_t *SalCallOp::createAllow(bool enable_update) {
	belle_sip_header_allow_t* header_allow;
	char allow [256];
	snprintf(allow,sizeof(allow),"INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO%s",(enable_update?", UPDATE":""));
	header_allow = belle_sip_header_allow_create(allow);
	return header_allow;
}

int SalCallOp::setCustomBody(belle_sip_message_t *msg, const Content &body) {
	ContentType contentType = body.getContentType();
	auto contentDisposition = body.getContentDisposition();
	string contentEncoding = body.getContentEncoding();
	size_t bodySize = body.getBody().size();

	if (bodySize > SIP_MESSAGE_BODY_LIMIT) {
		bctbx_error("trying to add a body greater than %lukB to message [%p]", (unsigned long)SIP_MESSAGE_BODY_LIMIT/1024, msg);
		return -1;
	}

	if (contentType.isValid()) {
		belle_sip_header_content_type_t *content_type = belle_sip_header_content_type_create(contentType.getType().c_str(), contentType.getSubType().c_str());
		belle_sip_message_add_header(msg, BELLE_SIP_HEADER(content_type));
	}
	if (contentDisposition.isValid()) {
		belle_sip_header_content_disposition_t *contentDispositionHeader = belle_sip_header_content_disposition_create(
			contentDisposition.asString().c_str()
		);
		belle_sip_message_add_header(msg, BELLE_SIP_HEADER(contentDispositionHeader));
	}
	if (!contentEncoding.empty())
		belle_sip_message_add_header(msg, belle_sip_header_create("Content-Encoding", contentEncoding.c_str()));
	belle_sip_header_content_length_t *content_length = belle_sip_header_content_length_create(bodySize);
	belle_sip_message_add_header(msg, BELLE_SIP_HEADER(content_length));

	if (bodySize > 0) {
		char *buffer = bctbx_new(char, bodySize + 1);
		memcpy(buffer, body.getBody().data(), bodySize);
		buffer[bodySize] = '\0';
		belle_sip_message_assign_body(msg, buffer, bodySize);
	}

	return 0;
}

std::vector<char> SalCallOp::marshalMediaDescription(belle_sdp_session_description_t *session_desc, belle_sip_error_code &error) {
	size_t length = 0;
	size_t bufLen = 2048;
	vector<char> buff(bufLen);
	error = BELLE_SIP_BUFFER_OVERFLOW;

	/* try to marshal the description. This could go higher than 2k so we iterate */
	while( error != BELLE_SIP_OK && bufLen <= SIP_MESSAGE_BODY_LIMIT) {
		error = belle_sip_object_marshal(BELLE_SIP_OBJECT(session_desc),buff.data(),bufLen,&length);
		if( error != BELLE_SIP_OK ){
			bufLen *= 2;
			length  = 0;
			buff.resize(bufLen);
		}
	}

	/* give up if hard limit reached */
	if (error != BELLE_SIP_OK) {
		ms_error("Buffer too small (%d) or not enough memory, giving up SDP", (int)bufLen);
		return std::vector<char>(); // return a new vector in order to free the buffer held by 'buff' vector
	}

	buff.resize(length);
	return buff;
}

int SalCallOp::setSdp(belle_sip_message_t *msg,belle_sdp_session_description_t* session_desc) {
	belle_sip_error_code error;

	if (session_desc == NULL) return -1;

	vector<char> buff = marshalMediaDescription(session_desc, error);
	if (error != BELLE_SIP_OK) return -1;

	Content body;
	body.setContentType(ContentType::Sdp);
	body.setBody(move(buff));
	   setCustomBody(msg, body);

	return 0;
}

int SalCallOp::setSdpFromDesc(belle_sip_message_t *msg, const SalMediaDescription *desc) {
	int err;
	belle_sdp_session_description_t *sdp=media_description_to_sdp(desc);
	err=setSdp(msg,sdp);
	belle_sip_object_unref(sdp);
	return err;

}

void SalCallOp::fillInvite(belle_sip_request_t* invite) {
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),BELLE_SIP_HEADER(createAllow(mRoot->mEnableSipUpdate)));
	if (mRoot->mSessionExpires!=0){
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),belle_sip_header_create( "Session-expires", "600;refresher=uas"));
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),belle_sip_header_create( "Supported", "timer"));
	}
	mSdpOffering = (mLocalBody.getContentType() == ContentType::Sdp);
	   setCustomBody(BELLE_SIP_MESSAGE(invite), mLocalBody);
}

void SalCallOp::setReleased() {
	if (!mCallReleased){
		mState=State::Terminated;
		mRoot->mCallbacks.call_released(this);
		mCallReleased=TRUE;
		/*be aware that the following line may destroy the op*/
		      setOrUpdateDialog(NULL);
	}
}

void SalCallOp::processIoErrorCb(void *user_ctx, const belle_sip_io_error_event_t *event) {
	SalCallOp *op = (SalCallOp *)user_ctx;

	if (op->mState == State::Terminated) return;

	if (op->mPendingClientTransaction && (belle_sip_transaction_get_state(BELLE_SIP_TRANSACTION(op->mPendingClientTransaction)) == BELLE_SIP_TRANSACTION_INIT)) {

		sal_error_info_set(&op->mErrorInfo, SalReasonIOError, "SIP", 503, "IO error", NULL);
		op->mRoot->mCallbacks.call_failure(op);

		if (!op->mDialog || belle_sip_dialog_get_state(op->mDialog) != BELLE_SIP_DIALOG_CONFIRMED){
			/* Call terminated very very early, before INVITE is even sent, probably DNS resolution timeout. */
			op->mState = State::Terminating;
			op->setReleased();
		}
	} else {
		/* Nothing to be done. If the error comes from a connectivity loss,
		 * the call will be marked as broken, and an attempt to repair it will be done. */
	}
}

void SalCallOp::cancellingInvite(const SalErrorInfo *info) {
	   cancelInvite(info);
	mState=State::Terminating;
}

Content SalCallOp::extractBody(belle_sip_message_t *message) {
	Content body;
	belle_sip_header_content_type_t *content_type = belle_sip_message_get_header_by_type(message, belle_sip_header_content_type_t);
	belle_sip_header_content_disposition_t *contentDispositionHeader = belle_sip_message_get_header_by_type(message, belle_sip_header_content_disposition_t);
	belle_sip_header_content_length_t *content_length = belle_sip_message_get_header_by_type(message, belle_sip_header_content_length_t);
	const char *type_str = content_type ? belle_sip_header_content_type_get_type(content_type) : NULL;
	const char *subtype_str = content_type ? belle_sip_header_content_type_get_subtype(content_type) : NULL;
	size_t length = content_length ? belle_sip_header_content_length_get_content_length(content_length) : 0;
	const char *body_str = belle_sip_message_get_body(message);

	if (type_str && subtype_str) body.setContentType(ContentType(type_str, subtype_str));
	if (contentDispositionHeader) {
		auto contentDisposition = ContentDisposition(belle_sip_header_content_disposition_get_content_disposition(contentDispositionHeader));
		if (belle_sip_parameters_has_parameter(BELLE_SIP_PARAMETERS(contentDispositionHeader), "handling")) {
			contentDisposition.setParameter("handling=" + string(belle_sip_parameters_get_parameter(BELLE_SIP_PARAMETERS(contentDispositionHeader), "handling")));
		}
		body.setContentDisposition(contentDisposition);
	}
	if (length > 0 && body_str) body.setBody(body_str, length);
	return body;
}

int SalCallOp::parseSdpBody(const Content &body,belle_sdp_session_description_t** session_desc, SalReason *error) {
	*session_desc = NULL;
	*error = SalReasonNone;

	if (mSdpHandling == SalOpSDPSimulateError) {
		ms_error("Simulating SDP parsing error for op %p", this);
		*error = SalReasonNotAcceptable;
		return -1;
	}

	if (mSdpHandling == SalOpSDPSimulateRemove) {
		ms_error("Simulating no SDP for op %p", this);
		return 0;
	}

	string strBody = body.getBodyAsString();
	if (strBody.empty())
		return 0;
	*session_desc = belle_sdp_session_description_parse(strBody.c_str());
	if (*session_desc == NULL) {
		ms_error("Failed to parse SDP message.");
		*error = SalReasonNotAcceptable;
		return -1;
	}

	return 0;
}

void SalCallOp::setAddrTo0000(char value[], size_t sz) {
	if (ms_is_ipv6(value)) {
		strncpy(value,"::0", sz);
	} else {
		strncpy(value,"0.0.0.0", sz);
	}
	return;
}

void SalCallOp::sdpProcess(){
	ms_message("Doing SDP offer/answer process of type %s", mSdpOffering ? "outgoing" : "incoming");
	if (mResult){
		sal_media_description_unref(mResult);
		mResult = NULL;
	}

	/* if SDP was invalid */
	if (mRemoteMedia == NULL) return;

	mResult=sal_media_description_new();
	if (mSdpOffering){
		offer_answer_initiate_outgoing(mRoot->mFactory, mLocalMedia,mRemoteMedia,mResult);
	}else{
		int i;
		if (mSdpAnswer){
			belle_sip_object_unref(mSdpAnswer);
		}
		offer_answer_initiate_incoming(mRoot->mFactory, mLocalMedia,mRemoteMedia,mResult,mRoot->mOneMatchingCodec);
		/*for backward compatibility purpose*/
		if(mCnxIpTo0000IfSendOnlyEnabled && sal_media_description_has_dir(mResult,SalStreamSendOnly)) {
			         setAddrTo0000(mResult->addr, sizeof(mResult->addr));
			for(i=0;i<SAL_MEDIA_DESCRIPTION_MAX_STREAMS;++i){
				if (mResult->streams[i].dir == SalStreamSendOnly) {
					               setAddrTo0000(mResult->streams[i].rtp_addr, sizeof(mResult->streams[i].rtp_addr));
					               setAddrTo0000(mResult->streams[i].rtcp_addr, sizeof(mResult->streams[i].rtcp_addr));
				}
			}
		}

		mSdpAnswer=(belle_sdp_session_description_t *)belle_sip_object_ref(media_description_to_sdp(mResult));
		/*once we have generated the SDP answer, we modify the result description for processing by the upper layer.
		 It should contains media parameters constraint from the remote offer, not our response*/
		strcpy(mResult->addr,mRemoteMedia->addr);
		mResult->bandwidth=mRemoteMedia->bandwidth;

		for(i=0;i<SAL_MEDIA_DESCRIPTION_MAX_STREAMS;++i){
			/*copy back parameters from remote description that we need in our result description*/
			if (mResult->streams[i].rtp_port!=0){ /*if stream was accepted*/
				strcpy(mResult->streams[i].rtp_addr,mRemoteMedia->streams[i].rtp_addr);
				mResult->streams[i].ptime=mRemoteMedia->streams[i].ptime;
				mResult->streams[i].bandwidth=mRemoteMedia->streams[i].bandwidth;
				mResult->streams[i].rtp_port=mRemoteMedia->streams[i].rtp_port;
				strcpy(mResult->streams[i].rtcp_addr,mRemoteMedia->streams[i].rtcp_addr);
				mResult->streams[i].rtcp_port=mRemoteMedia->streams[i].rtcp_port;

				if (sal_stream_description_has_srtp(&mResult->streams[i])) {
					mResult->streams[i].crypto[0] = mRemoteMedia->streams[i].crypto[0];
				}
			}
		}
	}
}

void SalCallOp::handleBodyFromResponse(belle_sip_response_t* response) {
	SalReason reason;
	belle_sdp_session_description_t* sdp = nullptr;
	Content body = extractBody(BELLE_SIP_MESSAGE(response));
	if (mRemoteMedia){
		sal_media_description_unref(mRemoteMedia);
		mRemoteMedia=NULL;
	}
	if (body.getContentType() == ContentType::Sdp) {
		if (parseSdpBody(body, &sdp, &reason) == 0) {
			if (sdp) {
				mRemoteMedia = sal_media_description_new();
				sdp_to_media_description(sdp, mRemoteMedia);
				mRemoteBody = move(body);
			}/*if no sdp in response, what can we do ?*/
		}
		/* process sdp in any case to reset result media description*/
		if (mLocalMedia) sdpProcess();
	} else {
		mRemoteBody = move(body);
	}
}

void SalCallOp::setError(belle_sip_response_t* response, bool fatal){
	   setErrorInfoFromResponse(response);
	if (fatal) mState = State::Terminating;
	mRoot->mCallbacks.call_failure(this);
}

int SalCallOp::vfuRetryCb (void *user_data, unsigned int events) {
	SalCallOp *op=(SalCallOp *)user_data;
	op->sendVfuRequest();
	op->unref();
	return BELLE_SIP_STOP;
}

void SalCallOp::processResponseCb(void *op_base, const belle_sip_response_event_t *event) {
	SalCallOp * op = (SalCallOp *)op_base;
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
	op->setOrUpdateDialog(dialog);
	dialog_state=dialog ? belle_sip_dialog_get_state(dialog) : BELLE_SIP_DIALOG_NULL;
	method=belle_sip_request_get_method(req);
	ms_message("Op [%p] receiving call response [%i], dialog is [%p] in state [%s]",op,code,dialog,belle_sip_dialog_state_to_string(dialog_state));
	/*to make sure no cb will destroy op*/
	op->ref();
	switch(dialog_state) {
		case BELLE_SIP_DIALOG_NULL:
		case BELLE_SIP_DIALOG_EARLY: {
			if (strcmp("INVITE",method)==0 ) {
				if (op->mState == State::Terminating) {
					/*check if CANCEL was sent before*/
					if (strcmp("CANCEL",belle_sip_request_get_method(belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(op->mPendingClientTransaction))))!=0) {
						/*it wasn't sent */
						if (code<200) {
							op->cancellingInvite(NULL);
						}else{
							/* no need to send the INVITE because the UAS rejected the INVITE*/
							if (op->mDialog==NULL) op->setReleased();
						}
					} else {
						/*it was sent already, so just expect the 487 or any error response to send the call_released() notification*/
						if (code>=300){
							if (op->mDialog==NULL) op->setReleased();
						}
					}
				} else if (code >= 180 && code<200) {
					belle_sip_response_t *prev_response=reinterpret_cast<belle_sip_response_t *>(belle_sip_object_data_get(BELLE_SIP_OBJECT(dialog),"early_response"));
					if (!prev_response || code>belle_sip_response_get_status_code(prev_response)){
						op->handleBodyFromResponse(response);
						op->mRoot->mCallbacks.call_ringing(op);
					}
					belle_sip_object_data_set(BELLE_SIP_OBJECT(dialog),"early_response",belle_sip_object_ref(response),belle_sip_object_unref);
				} else if (code>=300){
					op->setError(response, TRUE);
					if (op->mDialog==NULL) op->setReleased();
				}
			} else if (code >=200 && code<300) {
				if (strcmp("UPDATE",method)==0) {
					op->handleBodyFromResponse(response);
					op->mRoot->mCallbacks.call_accepted(op);
				} else if (strcmp("CANCEL", method) == 0) {
					op->mRoot->mCallbacks.call_cancel_done(op);
				}
			}
		}
		break;
		case BELLE_SIP_DIALOG_CONFIRMED: {
			switch (op->mState) {
				case State::Early:/*invite case*/
				case State::Active: /*re-invite, INFO, UPDATE case*/
					if (strcmp("INVITE",method)==0){
						if (code >=200 && code<300) {
							op->handleBodyFromResponse(response);
							ack=belle_sip_dialog_create_ack(op->mDialog,belle_sip_dialog_get_local_seq_number(op->mDialog));
							if (ack == NULL) {
								ms_error("This call has been already terminated.");
								return ;
							}
							if (op->mSdpAnswer){
								                setSdp(BELLE_SIP_MESSAGE(ack),op->mSdpAnswer);
								belle_sip_object_unref(op->mSdpAnswer);
								op->mSdpAnswer=NULL;
							}
							belle_sip_message_add_header(BELLE_SIP_MESSAGE(ack),BELLE_SIP_HEADER(op->mRoot->mUserAgentHeader));
							op->mRoot->mCallbacks.call_accepted(op); /*INVITE*/
							op->mRoot->mCallbacks.call_ack_being_sent(op, (SalCustomHeader*)ack);
							belle_sip_dialog_send_ack(op->mDialog,ack);
							op->mState=State::Active;
						}else if (code >= 300){
							op->setError(response, FALSE);
						}
					}else if (strcmp("INFO",method)==0){
						if (code == 491
							&& (header_content_type = belle_sip_message_get_header_by_type(req,belle_sip_header_content_type_t))
							&& strcmp("application",belle_sip_header_content_type_get_type(header_content_type))==0
							&& strcmp("media_control+xml",belle_sip_header_content_type_get_subtype(header_content_type))==0) {
							unsigned int retry_in = rand() % 1001; // [0;1000]
							belle_sip_source_t *s=op->mRoot->createTimer(vfuRetryCb,op->ref(), retry_in, "vfu request retry");
							ms_message("Rejected vfu request on op [%p], just retry in [%u] ms",op,retry_in);
							belle_sip_object_unref(s);
						}else {
								/*ignoring*/
						}
					}else if (strcmp("UPDATE",method)==0){
						op->mRoot->mCallbacks.call_accepted(op); /*INVITE*/
					}else if (strcmp("CANCEL",method)==0){
						op->mRoot->mCallbacks.call_cancel_done(op);
					}
				break;
				case State::Terminating:
					op->sendRequest(belle_sip_dialog_create_request(op->mDialog,"BYE"));
				break;
				case State::Terminated:
				default:
					lError() << "Call op [" << op << "] receives unexpected answer [" << code << "] while in state [" << toString(op->mState) << "]";
			}
		}
		break;
		case BELLE_SIP_DIALOG_TERMINATED: {
			if ((code >= 300)
				&& ((strcmp("INVITE", method) == 0) || (strcmp("BYE", method) == 0))
			) {
				op->setError(response, TRUE);
			}
		}
		break;
		default: {
			ms_error("call op [%p] receive answer [%i] not implemented",op,code);
		}
		break;
	}
	op->unref();
}

void SalCallOp::processTimeoutCb(void *user_ctx, const belle_sip_timeout_event_t *event) {
	SalCallOp * op=(SalCallOp *)user_ctx;

	if (op->mState==State::Terminated) return;

	if (!op->mDialog)  {
		/*call terminated very early*/
		sal_error_info_set(&op->mErrorInfo, SalReasonRequestTimeout, "SIP", 408, "Request timeout", NULL);
		op->mRoot->mCallbacks.call_failure(op);
		op->mState = State::Terminating;
		op->setReleased();
	} else {
		/*dialog will terminated shortly, nothing to do*/
	}
}

void SalCallOp::processTransactionTerminatedCb(void *user_ctx, const belle_sip_transaction_terminated_event_t *event) {
	SalCallOp * op = (SalCallOp *)user_ctx;
	belle_sip_client_transaction_t *client_transaction=belle_sip_transaction_terminated_event_get_client_transaction(event);
	belle_sip_server_transaction_t *server_transaction=belle_sip_transaction_terminated_event_get_server_transaction(event);
	belle_sip_request_t* req;
	belle_sip_response_t* resp;
	int code = 0;
	bool release_call = false;

	if (client_transaction) {
		req=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(client_transaction));
		resp=belle_sip_transaction_get_response(BELLE_SIP_TRANSACTION(client_transaction));
	} else {
		req=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(server_transaction));
		resp=belle_sip_transaction_get_response(BELLE_SIP_TRANSACTION(server_transaction));
	}
	if (resp) code = belle_sip_response_get_status_code(resp);

	if (op->mState == State::Terminating
			&& strcmp("BYE",belle_sip_request_get_method(req))==0
			&& (!resp || (belle_sip_response_get_status_code(resp) != 401
			&& belle_sip_response_get_status_code(resp) != 407))
			&& op->mDialog==NULL) {
		release_call=true;
	}else if (op->mState == State::Early && code < 200){
		/*call terminated early*/
		sal_error_info_set(&op->mErrorInfo, SalReasonIOError, "SIP", 503, "I/O error", NULL);
		op->mState = State::Terminating;
		op->mRoot->mCallbacks.call_failure(op);
		release_call=true;
	}
	if (server_transaction){
		if (op->mPendingServerTransaction==server_transaction){
			belle_sip_object_unref(op->mPendingServerTransaction);
			op->mPendingServerTransaction=NULL;
		}
		if (op->mPendingUpdateServerTransaction==server_transaction){
			belle_sip_object_unref(op->mPendingUpdateServerTransaction);
			op->mPendingUpdateServerTransaction=NULL;
		}
	}
	if (release_call) op->setReleased();
}

int SalCallOp::isMediaDescriptionAcceptable(SalMediaDescription *md) {
	if (md->nb_streams==0){
		ms_warning("Media description does not define any stream.");
		return FALSE;
	}
	return TRUE;
}

SalReason SalCallOp::processBodyForInvite(belle_sip_request_t* invite) {
	SalReason reason = SalReasonNone;

	Content body = extractBody(BELLE_SIP_MESSAGE(invite));
	if (!body.isValid()) return SalReasonUnsupportedContent;

	if ((body.getContentType() == ContentType::Sdp) || (body.getContentType().isEmpty() && body.isEmpty())) {
		belle_sdp_session_description_t* sdp;
		if (parseSdpBody(body, &sdp, &reason) == 0) {
			if (sdp) {
				mSdpOffering = FALSE;
				if (mRemoteMedia) sal_media_description_unref(mRemoteMedia);
				mRemoteMedia = sal_media_description_new();
				sdp_to_media_description(sdp, mRemoteMedia);
				/*make some sanity check about the SDP received*/
				if (!isMediaDescriptionAcceptable(mRemoteMedia)) {
					reason = SalReasonNotAcceptable;
				}
				belle_sip_object_unref(sdp);
			} else mSdpOffering = TRUE; /*INVITE without SDP*/
		}
		if (reason != SalReasonNone) {
			SalErrorInfo sei;
			memset(&sei, 0, sizeof(sei));
			sal_error_info_set(&sei, reason, "SIP", 0, NULL, NULL);
			         declineWithErrorInfo(&sei, NULL);
			sal_error_info_reset(&sei);
		}
	}
	mRemoteBody = move(body);
	return reason;
}

SalReason SalCallOp::processBodyForAck(belle_sip_request_t *ack) {
	SalReason reason = SalReasonNone;
	Content body = extractBody(BELLE_SIP_MESSAGE(ack));
	if (!body.isValid()) return SalReasonUnsupportedContent;
	if (body.getContentType() == ContentType::Sdp) {
		belle_sdp_session_description_t *sdp;
		if (parseSdpBody(body, &sdp, &reason) == 0) {
			if (sdp) {
				if (mRemoteMedia) sal_media_description_unref(mRemoteMedia);
				mRemoteMedia = sal_media_description_new();
				sdp_to_media_description(sdp, mRemoteMedia);
				sdpProcess();
				belle_sip_object_unref(sdp);
			} else {
				ms_warning("SDP expected in ACK but not found.");
			}
		}
	}
	mRemoteBody = move(body);
	return reason;
}

void SalCallOp::callTerminated(belle_sip_server_transaction_t* server_transaction, int status_code, belle_sip_request_t* cancel_request) {
	belle_sip_response_t* resp;
	belle_sip_request_t* server_req = belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(server_transaction));
	mState = State::Terminating;
	   setReasonErrorInfo(BELLE_SIP_MESSAGE(cancel_request ? cancel_request : server_req));
	resp=createResponseFromRequest(server_req,status_code);
	belle_sip_server_transaction_send_response(server_transaction,resp);
	mRoot->mCallbacks.call_terminated(this,mDir==Dir::Incoming?getFrom().c_str():getTo().c_str());
}

void SalCallOp::resetDescriptions() {
	if (mRemoteMedia){
		sal_media_description_unref(mRemoteMedia);
		mRemoteMedia=NULL;
	}
	if (mResult){
		sal_media_description_unref(mResult);
		mResult=NULL;
	}
}

void SalCallOp::unsupportedMethod(belle_sip_server_transaction_t* server_transaction,belle_sip_request_t* request) {
	belle_sip_response_t* resp;
	resp=belle_sip_response_create_from_request(request,501);
	belle_sip_server_transaction_send_response(server_transaction,resp);
}

bool SalCallOp::isAPendingIncomingInviteTransaction(belle_sip_transaction_t *tr){
	return BELLE_SIP_OBJECT_IS_INSTANCE_OF(tr, belle_sip_ist_t) && belle_sip_transaction_state_is_transient(
		belle_sip_transaction_get_state(tr));
}

void SalCallOp::processRequestEventCb(void *op_base, const belle_sip_request_event_t *event) {
	SalCallOp * op = (SalCallOp *)op_base;
	SalReason reason;
	belle_sip_server_transaction_t* server_transaction=NULL;
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_dialog_state_t dialog_state;
	belle_sip_response_t* resp;
	belle_sip_header_t* call_info;
	const char *method=belle_sip_request_get_method(req);
	bool is_update = false;
	bool drop_op = false;

	if (strcmp("ACK",method)!=0){  /*ACK doesn't create a server transaction*/
		server_transaction = belle_sip_provider_create_server_transaction(op->mRoot->mProvider,belle_sip_request_event_get_request(event));
		belle_sip_object_ref(server_transaction);
		belle_sip_transaction_set_application_data(BELLE_SIP_TRANSACTION(server_transaction),op->ref());
	}

	if (strcmp("INVITE",method)==0) {
		if (op->mPendingServerTransaction) belle_sip_object_unref(op->mPendingServerTransaction);
		/*updating pending invite transaction*/
		op->mPendingServerTransaction=server_transaction;
		belle_sip_object_ref(op->mPendingServerTransaction);
	}

	if (strcmp("UPDATE",method)==0) {
		if (op->mPendingUpdateServerTransaction) belle_sip_object_unref(op->mPendingUpdateServerTransaction);
		/*updating pending update transaction*/
		op->mPendingUpdateServerTransaction=server_transaction;
		belle_sip_object_ref(op->mPendingUpdateServerTransaction);
	}

	if (!op->mDialog) {
		op->setOrUpdateDialog(belle_sip_provider_create_dialog(op->mRoot->mProvider, BELLE_SIP_TRANSACTION(op->mPendingServerTransaction)));
		ms_message("new incoming call from [%s] to [%s]",op->getFrom().c_str(),op->getTo().c_str());
	}
	dialog_state=belle_sip_dialog_get_state(op->mDialog);
	switch(dialog_state) {
	case BELLE_SIP_DIALOG_NULL: {
		if (strcmp("INVITE",method)==0) {
			if (!op->mReplaces && (op->mReplaces=belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_replaces_t))) {
				belle_sip_object_ref(op->mReplaces);
			} else if(op->mReplaces) {
				ms_warning("replace header already set");
			}

			if ( (reason = op->processBodyForInvite(req)) == SalReasonNone) {
				if ((call_info=belle_sip_message_get_header(BELLE_SIP_MESSAGE(req),"Call-Info"))) {
					if( strstr(belle_sip_header_get_unparsed_value(call_info),"answer-after=") != NULL) {
						op->mAutoAnswerAsked=TRUE;
						ms_message("The caller asked to automatically answer the call(Emergency?)\n");
					}
				}
				op->mRoot->mCallbacks.call_received(op);
			}else{
				sal_error_info_set(&op->mErrorInfo, reason, "SIP", 0, NULL, NULL);
				op->mRoot->mCallbacks.call_rejected(op);
				/*the INVITE was declined by process_sdp_for_invite(). As we are not inside an established dialog, we can drop the op immediately*/
				drop_op = true;
			}
			break;
		}BCTBX_NO_BREAK; /* else same behavior as for EARLY state, thus NO BREAK*/
	}
	case BELLE_SIP_DIALOG_EARLY: {
		if (strcmp("CANCEL",method)==0) {
			if(belle_sip_request_event_get_server_transaction(event)) {
				/*first answer 200 ok to cancel*/
				belle_sip_server_transaction_send_response(server_transaction
						,op->createResponseFromRequest(req,200));
				/*terminate invite transaction*/
				op->callTerminated(op->mPendingServerTransaction,487,req);
			} else {
				/*call leg does not exist*/
				belle_sip_server_transaction_send_response(server_transaction
							,op->createResponseFromRequest(req,481));
			}
		} else if (strcmp("PRACK",method)==0) {
			resp=op->createResponseFromRequest(req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
		} else if (strcmp("UPDATE",method)==0) {
			op->resetDescriptions();
			if (op->processBodyForInvite(req)==SalReasonNone)
				op->mRoot->mCallbacks.call_updating(op,TRUE);
		} else {
			belle_sip_error("Unexpected method [%s] for dialog state BELLE_SIP_DIALOG_EARLY",belle_sip_request_get_method(req));
			         unsupportedMethod(server_transaction,req);
		}
		break;
	}
	case BELLE_SIP_DIALOG_CONFIRMED:
		/*great ACK received*/
		if (strcmp("ACK",method)==0) {
			if (!op->mPendingClientTransaction ||
				!belle_sip_transaction_state_is_transient(belle_sip_transaction_get_state((belle_sip_transaction_t*)op->mPendingClientTransaction))){
				if (op->mSdpOffering){
					op->processBodyForAck(req);
				}
				op->mRoot->mCallbacks.call_ack_received(op, (SalCustomHeader*)req);
			}else{
				ms_message("Ignored received ack since a new client transaction has been started since.");
			}
		} else if(strcmp("BYE",method)==0) {
			op->callTerminated(server_transaction,200,req);
			/*call end not notified by dialog deletion because transaction can end before dialog*/
		} else if(strcmp("INVITE",method)==0 || (is_update=(strcmp("UPDATE",method)==0)) ) {
			if (is_update && !belle_sip_message_get_body(BELLE_SIP_MESSAGE(req))) {
				/*session timer case*/
				/*session expire should be handled. to be done when real session timer (rfc4028) will be implemented*/
				resp=op->createResponseFromRequest(req,200);
				belle_sip_server_transaction_send_response(server_transaction,resp);
				belle_sip_object_unref(op->mPendingUpdateServerTransaction);
				op->mPendingUpdateServerTransaction=NULL;
			} else {
				/*re-invite*/
				op->resetDescriptions();
				if (op->processBodyForInvite(req)==SalReasonNone)
					op->mRoot->mCallbacks.call_updating(op,is_update);
			}
		} else if (strcmp("INFO",method)==0){
			if (belle_sip_message_get_body(BELLE_SIP_MESSAGE(req))
				&&	strstr(belle_sip_message_get_body(BELLE_SIP_MESSAGE(req)),"picture_fast_update")) {
				/*vfu request*/
				ms_message("Receiving VFU request on op [%p]",op);
				if (op->mRoot->mCallbacks.vfu_request){
					op->mRoot->mCallbacks.vfu_request(op);

				}
			}else{
				belle_sip_message_t *msg = BELLE_SIP_MESSAGE(req);
				belle_sip_body_handler_t *body_handler = BELLE_SIP_BODY_HANDLER(op->getBodyHandler(msg));
				if (body_handler) {
					belle_sip_header_content_type_t *content_type = belle_sip_message_get_header_by_type(msg, belle_sip_header_content_type_t);
					if (content_type
						&& (strcmp(belle_sip_header_content_type_get_type(content_type), "application") == 0)
						&& (strcmp(belle_sip_header_content_type_get_subtype(content_type), "dtmf-relay") == 0)) {
						char tmp[10];
						if (sal_lines_get_value(belle_sip_message_get_body(msg), "Signal",tmp, sizeof(tmp))){
							op->mRoot->mCallbacks.dtmf_received(op,tmp[0]);
						}
					}else
						op->mRoot->mCallbacks.info_received(op, (SalBodyHandler *)body_handler);
				} else {
					op->mRoot->mCallbacks.info_received(op,NULL);
				}
			}
			resp=op->createResponseFromRequest(req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
		}else if (strcmp("REFER",method)==0) {
			op->processRefer(event,server_transaction);
		} else if (strcmp("NOTIFY",method)==0) {
			op->processNotify(event,server_transaction);
		} else if (strcmp("OPTIONS",method)==0) {
			resp=op->createResponseFromRequest(req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
		} else if (strcmp("CANCEL",method)==0) {
			belle_sip_transaction_t *last_transaction = belle_sip_dialog_get_last_transaction(op->mDialog);
			if (last_transaction == NULL || !isAPendingIncomingInviteTransaction(last_transaction) ) {
				/*call leg does not exist because 200ok already sent*/
				belle_sip_server_transaction_send_response(server_transaction,op->createResponseFromRequest(req,481));
			} else {
				/* CANCEL on re-INVITE for which a 200ok has not been sent yet */
				belle_sip_server_transaction_send_response(server_transaction, op->createResponseFromRequest(req, 200));
				belle_sip_server_transaction_send_response(BELLE_SIP_SERVER_TRANSACTION(last_transaction),
					op->createResponseFromRequest(belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(last_transaction)), 487));
			}
		} else if (strcmp("MESSAGE",method)==0){
			op->processIncomingMessage(event);
		}else{
			ms_error("unexpected method [%s] for dialog [%p]",belle_sip_request_get_method(req),op->mDialog);
			         unsupportedMethod(server_transaction,req);
		}
		break;
	default:
		ms_error("unexpected dialog state [%s]",belle_sip_dialog_state_to_string(dialog_state));
		break;
	}

	if (server_transaction) belle_sip_object_unref(server_transaction);
	if (drop_op) op->release();
}

void SalCallOp::setCallAsReleased(SalCallOp *op) {
	op->setReleased();
}

void SalCallOp::processDialogTerminatedCb(void *ctx, const belle_sip_dialog_terminated_event_t *event) {
	SalCallOp * op=(SalCallOp *)ctx;

	if (op->mDialog && op->mDialog==belle_sip_dialog_terminated_event_get_dialog(event))  {
		/*belle_sip_transaction_t* trans=belle_sip_dialog_get_last_transaction(op->dialog);*/
		ms_message("Dialog [%p] terminated for op [%p]",belle_sip_dialog_terminated_event_get_dialog(event),op);

		switch(belle_sip_dialog_get_previous_state(op->mDialog)) {
			case BELLE_SIP_DIALOG_EARLY:
			case BELLE_SIP_DIALOG_NULL:
				if (op->mState!=State::Terminated && op->mState!=State::Terminating) {
					/*this is an early termination due to incorrect response received*/
					op->mRoot->mCallbacks.call_failure(op);
					op->mState=State::Terminating;
				}
			break;
			case BELLE_SIP_DIALOG_CONFIRMED:
				if (op->mState!=State::Terminated && op->mState!=State::Terminating) {
					/*this is probably a normal termination from a BYE*/
					op->mRoot->mCallbacks.call_terminated(op,op->mDir==Dir::Incoming?op->getFrom().c_str():op->getTo().c_str());
					op->mState=State::Terminating;
				}
			break;
			default:
			break;
		}
		belle_sip_main_loop_do_later(belle_sip_stack_get_main_loop(op->mRoot->mStack)
							,(belle_sip_callback_t) setCallAsReleased
							, op);
	} else {
		ms_error("dialog unknown for op ");
	}
}

void SalCallOp::fillCallbacks() {
	static belle_sip_listener_callbacks_t call_op_callbacks = {0};
	if (call_op_callbacks.process_response_event==NULL){
		call_op_callbacks.process_io_error=processIoErrorCb;
		call_op_callbacks.process_response_event=processResponseCb;
		call_op_callbacks.process_timeout=processTimeoutCb;
		call_op_callbacks.process_transaction_terminated=processTransactionTerminatedCb;
		call_op_callbacks.process_request_event=processRequestEventCb;
		call_op_callbacks.process_dialog_terminated=processDialogTerminatedCb;
	}
	mCallbacks=&call_op_callbacks;
	mType=Type::Call;
}

int SalCallOp::call(const char *from, const char *to, const char *subject) {
	belle_sip_request_t* invite;
	mDir=Dir::Outgoing;

	   setFrom(from);
	   setTo(to);

	ms_message("[%s] calling [%s] on op [%p]", from, to, this);
	invite=buildRequest("INVITE");

	if( invite == NULL ){
		/* can happen if the op has an invalid address */
		return -1;
	}

	   fillInvite(invite);
	if (subject) belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite), belle_sip_header_create("Subject", subject));

	   fillCallbacks();
	if (mReplaces){
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),BELLE_SIP_HEADER(mReplaces));
	}
	if (mReferredBy)
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(invite),BELLE_SIP_HEADER(mReferredBy));

	return sendRequest(invite);
}

int SalCallOp::notifyRinging(bool early_media){
	int status_code =early_media?183:180;
	belle_sip_request_t* req=belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(mPendingServerTransaction));
	belle_sip_response_t* ringing_response = createResponseFromRequest(req,status_code);
	belle_sip_header_t *require;
	const char *tags=NULL;

	if (early_media)
		handleOfferAnswerResponse(ringing_response);
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
		belle_sip_header_address_t* contact= (belle_sip_header_address_t*)getContactAddress();
		belle_sip_header_contact_t* contact_header;
		if (contact && (contact_header=belle_sip_header_contact_create(contact))) {
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(ringing_response),BELLE_SIP_HEADER(contact_header));
		}
	}
	belle_sip_server_transaction_send_response(mPendingServerTransaction,ringing_response);
	return 0;
}

int SalCallOp::accept() {
	belle_sip_response_t *response;
	belle_sip_header_contact_t* contact_header;
	belle_sip_server_transaction_t* transaction;

	/*first check if an UPDATE transaction need to be accepted*/
	if (mPendingUpdateServerTransaction) {
		transaction= mPendingUpdateServerTransaction;
	} else if (mPendingServerTransaction) {
		/*so it must be an invite/re-invite*/
		transaction= mPendingServerTransaction;
	} else {
		ms_error("No transaction to accept for op [%p]", this);
		return -1;
	}
	ms_message("Accepting server transaction [%p] on op [%p]", transaction, this);

	/* sends a 200 OK */
	response = createResponseFromRequest(belle_sip_transaction_get_request(BELLE_SIP_TRANSACTION(transaction)),200);
	if (response==NULL){
		ms_error("Fail to build answer for call");
		return -1;
	}
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_HEADER(createAllow(mRoot->mEnableSipUpdate)));
	if (mRoot->mSessionExpires!=0){
/*		if (h->supports_session_timers) {*/
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),belle_sip_header_create("Supported", "timer"));
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),belle_sip_header_create( "Session-expires", "600;refresher=uac"));
		/*}*/
	}

	if ((contact_header=createContact())) {
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_HEADER(contact_header));
	}

	   addCustomHeaders(BELLE_SIP_MESSAGE(response));

	   handleOfferAnswerResponse(response);

	belle_sip_server_transaction_send_response(transaction,response);
	if (mPendingUpdateServerTransaction) {
		belle_sip_object_unref(mPendingUpdateServerTransaction);
		mPendingUpdateServerTransaction=NULL;
	}
	if (mState == State::Early){
		mState = State::Active;
	}
	return 0;
}

int SalCallOp::decline(SalReason reason, const char *redirection){
	belle_sip_response_t* response;
	belle_sip_header_contact_t* contact=NULL;
	int status=to_sip_code(reason);
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
	trans=(belle_sip_transaction_t*)mPendingServerTransaction;
	if (!trans) trans=(belle_sip_transaction_t*)mPendingUpdateServerTransaction;
	if (!trans){
		ms_error("sal_call_decline(): no pending transaction to decline.");
		return -1;
	}
	response = createResponseFromRequest(belle_sip_transaction_get_request(trans),status);
	if (contact) belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_HEADER(contact));
	belle_sip_server_transaction_send_response(BELLE_SIP_SERVER_TRANSACTION(trans),response);
	return 0;
}

belle_sip_header_reason_t *SalCallOp::makeReasonHeader( const SalErrorInfo *info){
	if (info && info->reason != SalReasonNone) {
		belle_sip_header_reason_t* reason = BELLE_SIP_HEADER_REASON(belle_sip_header_reason_new());
		belle_sip_header_reason_set_text(reason, info->status_string);
		belle_sip_header_reason_set_protocol(reason,info->protocol);
		belle_sip_header_reason_set_cause(reason,info->protocol_code);
		return reason;
	}
	return NULL;
}

int SalCallOp::declineWithErrorInfo(const SalErrorInfo *info, const SalAddress *redirectionAddr){
	belle_sip_response_t* response;
	belle_sip_header_contact_t* contact=NULL;
	int status = info->protocol_code;
	belle_sip_transaction_t *trans;

	if (info->reason==SalReasonRedirect){
		if (redirectionAddr) {
			status = 302;
			contact = belle_sip_header_contact_create(BELLE_SIP_HEADER_ADDRESS(redirectionAddr));
		} else {
			ms_error("Cannot redirect to null");
		}
	}
	trans=(belle_sip_transaction_t*)mPendingServerTransaction;
	if (!trans) trans=(belle_sip_transaction_t*)mPendingUpdateServerTransaction;
	if (!trans){
		ms_error("sal_call_decline_with_error_info(): no pending transaction to decline.");
		return -1;
	}
	response = createResponseFromRequest(belle_sip_transaction_get_request(trans),status);
	belle_sip_header_reason_t* reason_header = makeReasonHeader(info->sub_sei);
	if (reason_header) {
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_HEADER(reason_header));
	}

	if (contact) {
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(response),BELLE_SIP_HEADER(contact));
	}
	belle_sip_server_transaction_send_response(BELLE_SIP_SERVER_TRANSACTION(trans),response);
	return 0;
}

int SalCallOp::update(const char *subject, bool no_user_consent) {
	belle_sip_request_t *update;
	belle_sip_dialog_state_t state;

	if (mDialog == NULL) {
		/* If the dialog does not exist, this is that we are trying to recover from a connection loss
			during a very early state of outgoing call initiation (the dialog has not been created yet). */
		return call(mFrom.c_str(), mTo.c_str(), subject);
	}

	state = belle_sip_dialog_get_state(mDialog);
	belle_sip_dialog_enable_pending_trans_checking(mDialog,mRoot->mPendingTransactionChecking);

	/*check for dialog state*/
	if ( state == BELLE_SIP_DIALOG_CONFIRMED) {
		if (no_user_consent)
			update=belle_sip_dialog_create_request(mDialog,"UPDATE");
		else
			update=belle_sip_dialog_create_request(mDialog,"INVITE");
	} else if (state == BELLE_SIP_DIALOG_EARLY)  {
		update=belle_sip_dialog_create_request(mDialog,"UPDATE");
	} else {
		ms_error("Cannot update op [%p] with dialog [%p] in state [%s]", this, mDialog,belle_sip_dialog_state_to_string(state));
		return  -1;
	}
	if (update){
		belle_sip_message_add_header(BELLE_SIP_MESSAGE(update),belle_sip_header_create( "Subject", subject));
		      fillInvite(update);
		return sendRequest(update);
	}
	/*it failed why ?*/
	if (belle_sip_dialog_request_pending(mDialog))
		sal_error_info_set(&mErrorInfo,SalReasonRequestPending, "SIP", 491,NULL,NULL);
	else
		sal_error_info_set(&mErrorInfo,SalReasonUnknown, "SIP", 500,NULL,NULL);
	return -1;
}

int SalCallOp::cancelInvite(const SalErrorInfo *info) {
	belle_sip_request_t* cancel;
	ms_message("Cancelling INVITE request from [%s] to [%s] ",getFrom().c_str(), getTo().c_str());

	if (mPendingClientTransaction == NULL) {
		ms_warning("There is no transaction to cancel.");
		return -1;
	}

	cancel = belle_sip_client_transaction_create_cancel(mPendingClientTransaction);
	if (cancel) {
		if (info && info->reason != SalReasonNone) {
			belle_sip_header_reason_t* reason = makeReasonHeader(info);
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(cancel),BELLE_SIP_HEADER(reason));
		}
		      sendRequest(cancel);
		return 0;
	} else if (mDialog) {
		belle_sip_dialog_state_t state = belle_sip_dialog_get_state(mDialog);;
		/*case where the response received is invalid (could not establish a dialog), but the transaction is not cancellable
		 * because already terminated*/
		switch(state) {
			case BELLE_SIP_DIALOG_EARLY:
			case BELLE_SIP_DIALOG_NULL:
				/*force kill the dialog*/
				ms_warning("op [%p]: force kill of dialog [%p]", this, mDialog);
				belle_sip_dialog_delete(mDialog);
			break;
			default:
			break;
		}
	}
	return -1;
}

SalMediaDescription *SalCallOp::getFinalMediaDescription() {
	if (mLocalMedia && mRemoteMedia && !mResult){
			     sdpProcess();
	}
	return mResult;
}

int SalCallOp::referTo(belle_sip_header_refer_to_t* refer_to, belle_sip_header_referred_by_t* referred_by) {
	char* tmp;
	belle_sip_request_t* req=mDialog?belle_sip_dialog_create_request(mDialog,"REFER"):buildRequest("REFER");
	if (!req) {
		tmp=belle_sip_uri_to_string(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(refer_to)));
		ms_error("Cannot refer to [%s] for op [%p]",tmp, this);
		belle_sip_free(tmp);
		return -1;
	}
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(refer_to));
	if (referred_by) belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(referred_by));
	return sendRequest(req);
}

int SalCallOp::refer(const char *refer_to_){
	belle_sip_header_address_t *referred_by;
	belle_sip_header_refer_to_t* refer_to_header;
	if (mDialog) {
		referred_by=(belle_sip_header_address_t*)belle_sip_object_clone(BELLE_SIP_OBJECT(belle_sip_dialog_get_local_party(mDialog)));
	}else{
		referred_by=BELLE_SIP_HEADER_ADDRESS(getFromAddress());
	}
	refer_to_header=belle_sip_header_refer_to_create(belle_sip_header_address_parse(refer_to_));

	return referTo(refer_to_header,belle_sip_header_referred_by_create(referred_by));
}

int SalCallOp::referWithReplaces(SalCallOp *other_call_op) {
	belle_sip_dialog_state_t other_call_dialog_state=other_call_op->mDialog?belle_sip_dialog_get_state(other_call_op->mDialog):BELLE_SIP_DIALOG_NULL;
	belle_sip_dialog_state_t op_dialog_state= mDialog?belle_sip_dialog_get_state(mDialog):BELLE_SIP_DIALOG_NULL;
	belle_sip_header_replaces_t* replaces;
	belle_sip_header_refer_to_t* refer_to_;
	belle_sip_header_referred_by_t* referred_by;
	const char* from_tag;
	const char* to_tag;
	char* escaped_replaces;
	/*first, build refer to*/
	if ((other_call_dialog_state!=BELLE_SIP_DIALOG_CONFIRMED) && (other_call_dialog_state!=BELLE_SIP_DIALOG_EARLY)) {
		ms_error("wrong dialog state [%s] for op [%p], should be BELLE_SIP_DIALOG_CONFIRMED or BELE_SIP_DIALOG_EARLY",
			belle_sip_dialog_state_to_string(other_call_dialog_state),
			other_call_op);
		return -1;
	}
	if (op_dialog_state!=BELLE_SIP_DIALOG_CONFIRMED) {
		ms_error("wrong dialog state [%s] for op [%p], should be BELLE_SIP_DIALOG_CONFIRMED",
			belle_sip_dialog_state_to_string(op_dialog_state),
			     this);
		return -1;
	}

	refer_to_ =belle_sip_header_refer_to_create(belle_sip_dialog_get_remote_party(other_call_op->mDialog));
	belle_sip_parameters_clean(BELLE_SIP_PARAMETERS(refer_to_));
	/*rfc3891
	 ...
	 4.  User Agent Client Behavior: Sending a Replaces Header

	 A User Agent that wishes to replace a single existing early or
	 confirmed dialog with a new dialog of its own, MAY send the target
	 User Agent an INVITE request containing a Replaces header field.  The
	 User Agent Client (UAC) places the Call-ID, to-tag, and from-tag
	 information for the target dialog in a single Replaces header field
	 and sends the new INVITE to the target.*/
	from_tag=belle_sip_dialog_get_local_tag(other_call_op->mDialog);
	to_tag=belle_sip_dialog_get_remote_tag(other_call_op->mDialog);

	replaces=belle_sip_header_replaces_create(belle_sip_header_call_id_get_call_id(belle_sip_dialog_get_call_id(other_call_op->mDialog))
											,from_tag,to_tag);
	escaped_replaces=belle_sip_header_replaces_value_to_escaped_string(replaces);
	belle_sip_uri_set_header(belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(refer_to_)),"Replaces",escaped_replaces);
	belle_sip_free(escaped_replaces);
	referred_by=belle_sip_header_referred_by_create(belle_sip_dialog_get_local_party(mDialog));
	belle_sip_parameters_clean(BELLE_SIP_PARAMETERS(referred_by));
	return referTo(refer_to_,referred_by);
}

int SalCallOp::setReferrer(SalCallOp *refered_call){
	if (refered_call->mReplaces)
		SalOp::setReplaces(refered_call->mReplaces);
	if (refered_call->mReferredBy)
		      setReferredBy(refered_call->mReferredBy);
	return 0;
}

SalCallOp *SalCallOp::getReplaces() const {
	if (mReplaces){
		/*rfc3891
		 3.  User Agent Server Behavior: Receiving a Replaces Header

		 The Replaces header contains information used to match an existing
		 SIP dialog (call-id, to-tag, and from-tag).  Upon receiving an INVITE
		 with a Replaces header, the User Agent (UA) attempts to match this
		 information with a confirmed or early dialog.  The User Agent Server
		 (UAS) matches the to-tag and from-tag parameters as if they were tags
		 present in an incoming request.  In other words, the to-tag parameter
		 is compared to the local tag, and the from-tag parameter is compared
		 to the remote tag.
		 */
		belle_sip_dialog_t* dialog=belle_sip_provider_find_dialog(mRoot->mProvider
								,belle_sip_header_replaces_get_call_id(mReplaces)
								,belle_sip_header_replaces_get_to_tag(mReplaces)
								,belle_sip_header_replaces_get_from_tag(mReplaces));

		if (!dialog) {
			/*for backward compatibility with liblinphone <= 3.10.2-243 */
			dialog=belle_sip_provider_find_dialog(mRoot->mProvider
												  ,belle_sip_header_replaces_get_call_id(mReplaces)
												  ,belle_sip_header_replaces_get_from_tag(mReplaces)
												  ,belle_sip_header_replaces_get_to_tag(mReplaces));
		}
		if (dialog) {
			return (SalCallOp*)belle_sip_dialog_get_application_data(dialog);
		}
	}
	return NULL;
}

int SalCallOp::sendDtmf(char dtmf){
	if (mDialog && (belle_sip_dialog_get_state(mDialog) == BELLE_SIP_DIALOG_CONFIRMED || belle_sip_dialog_get_state(mDialog) == BELLE_SIP_DIALOG_EARLY)){
		belle_sip_request_t *req=belle_sip_dialog_create_queued_request(mDialog,"INFO");
		if (req){
			size_t bodylen;
			char dtmf_body[128]={0};

			snprintf(dtmf_body, sizeof(dtmf_body)-1, "Signal=%c\r\nDuration=250\r\n", dtmf);
			bodylen=strlen(dtmf_body);
			belle_sip_message_set_body((belle_sip_message_t*)req,dtmf_body,bodylen);
			belle_sip_message_add_header((belle_sip_message_t*)req,(belle_sip_header_t*)belle_sip_header_content_length_create(bodylen));
			belle_sip_message_add_header((belle_sip_message_t*)req,(belle_sip_header_t*)belle_sip_header_content_type_create("application", "dtmf-relay"));
			         sendRequest(req);
		}else ms_error("sal_call_send_dtmf(): could not build request");
	}else ms_error("sal_call_send_dtmf(): no dialog");
	return 0;
}

int SalCallOp::terminate(const SalErrorInfo *info) {
	SalErrorInfo sei;
	const SalErrorInfo *p_sei;
	belle_sip_dialog_state_t dialog_state = mDialog ? belle_sip_dialog_get_state(mDialog) : BELLE_SIP_DIALOG_NULL;
	int ret = 0;

	memset(&sei, 0, sizeof(sei));
	if (info == NULL && dialog_state != BELLE_SIP_DIALOG_CONFIRMED && mDir == Dir::Incoming) {
		/*the purpose of this line is to set a default SalErrorInfo for declining an incoming call (not yet established of course) */
		sal_error_info_set(&sei,SalReasonDeclined, "SIP", 0, NULL, NULL);
		p_sei = &sei;
	} else{
		p_sei = info;
	}
	if (mState==State::Terminating || mState==State::Terminated) {
		lError() << "Cannot terminate op [" << this << "] in state [" << toString(mState) << "]";
		ret = -1;
		goto end;
	}
	switch(dialog_state) {
		case BELLE_SIP_DIALOG_CONFIRMED: {
			belle_sip_request_t * req = belle_sip_dialog_create_request(mDialog,"BYE");
			if (info && info->reason != SalReasonNone) {
				belle_sip_header_reason_t* reason = makeReasonHeader(info);
				belle_sip_message_add_header(BELLE_SIP_MESSAGE(req),BELLE_SIP_HEADER(reason));
			}
			     sendRequest(req);
			mState=State::Terminating;
			break;
		}

		case BELLE_SIP_DIALOG_NULL: {
			if (mDir == Dir::Incoming) {
				        declineWithErrorInfo(p_sei, NULL);
				mState=State::Terminated;
			} else if (mPendingClientTransaction){
				if (belle_sip_transaction_get_state(BELLE_SIP_TRANSACTION(mPendingClientTransaction)) == BELLE_SIP_TRANSACTION_PROCEEDING){
					           cancellingInvite(p_sei);
				} else {
					/* Case where the CANCEL cannot be sent because no provisional response was received so far.
					 * The Op must be kept for the time of the transaction in case a response is received later.
					 * The state is passed to Terminating to remember to terminate later.
					 */
					mState=State::Terminating;
					/* However, even if the transaction is kept alive, we can stop sending retransmissions to avoid flowing the network with no longer
					 * necessary messages and avoid confusion in logs.*/
					belle_sip_client_transaction_stop_retransmissions(mPendingClientTransaction);
				}
			}
			break;
		}
		case BELLE_SIP_DIALOG_EARLY: {
			if (mDir == Dir::Incoming) {
				        declineWithErrorInfo(p_sei,NULL);
				mState=State::Terminated;
			} else {
				        cancellingInvite(p_sei);
			}
			break;
		}
		default: {
			ms_error("sal_call_terminate not implemented yet for dialog state [%s]",belle_sip_dialog_state_to_string(dialog_state));
			ret = -1;
			goto end;
		}
	}
end:
	sal_error_info_reset(&sei);
	return ret;
}

void SalCallOp::sendVfuRequest() {
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
	belle_sip_dialog_state_t dialog_state= mDialog?belle_sip_dialog_get_state(mDialog):BELLE_SIP_DIALOG_NULL; /*no dialog = dialog in NULL state*/
	if (dialog_state == BELLE_SIP_DIALOG_CONFIRMED) {
		belle_sip_request_t* info =	belle_sip_dialog_create_queued_request(mDialog,"INFO");
		int error=TRUE;
		if (info) {
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(info),BELLE_SIP_HEADER(belle_sip_header_content_type_create("application","media_control+xml")));
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(info),BELLE_SIP_HEADER(belle_sip_header_content_length_create(content_lenth)));
			belle_sip_message_set_body(BELLE_SIP_MESSAGE(info),info_body,content_lenth);
			error=sendRequest(info);
		}
		if (error)
			ms_warning("Cannot send vfu request to [%s] ", getTo().c_str());

	} else {
		ms_warning("Cannot send vfu request to [%s] because dialog [%p] in wrong state [%s]",getTo().c_str()
																							,mDialog
																							,belle_sip_dialog_state_to_string(dialog_state));
	}

	return ;
}

int SalCallOp::sendNotifyForRefer(int code, const char *reason) {
	belle_sip_request_t* notify=belle_sip_dialog_create_queued_request(mDialog,"NOTIFY");
	char *sipfrag=belle_sip_strdup_printf("SIP/2.0 %i %s\r\n",code,reason);
	size_t content_length=strlen(sipfrag);
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify)
		,BELLE_SIP_HEADER(belle_sip_header_subscription_state_create(BELLE_SIP_SUBSCRIPTION_STATE_ACTIVE,-1)));

	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),belle_sip_header_create("Event","refer"));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),BELLE_SIP_HEADER(belle_sip_header_content_type_create("message","sipfrag")));
	belle_sip_message_add_header(BELLE_SIP_MESSAGE(notify),BELLE_SIP_HEADER(belle_sip_header_content_length_create(content_length)));
	belle_sip_message_assign_body(BELLE_SIP_MESSAGE(notify),sipfrag,content_length);
	return sendRequest(notify);
}

void SalCallOp::notifyLastResponse(SalCallOp *newcall) {
	belle_sip_client_transaction_t *tr=newcall->mPendingClientTransaction;
	belle_sip_response_t *resp=NULL;
	if (tr){
		resp=belle_sip_transaction_get_response((belle_sip_transaction_t*)tr);
	}
	if (resp==NULL){
		      sendNotifyForRefer(100, "Trying");
	}else{
		      sendNotifyForRefer(belle_sip_response_get_status_code(resp), belle_sip_response_get_reason_phrase(resp));
	}
}

int SalCallOp::notifyReferState(SalCallOp *newcall) {
	belle_sip_dialog_state_t state;
	if(belle_sip_dialog_get_state(mDialog) == BELLE_SIP_DIALOG_TERMINATED){
		return 0;
	}
	state = newcall->mDialog?belle_sip_dialog_get_state(newcall->mDialog):BELLE_SIP_DIALOG_NULL;
	switch(state) {
		case BELLE_SIP_DIALOG_EARLY:
			     sendNotifyForRefer(100, "Trying");
			break;
		case BELLE_SIP_DIALOG_CONFIRMED:
			     sendNotifyForRefer(200, "Ok");
			break;
		case BELLE_SIP_DIALOG_TERMINATED:
		case BELLE_SIP_DIALOG_NULL:
			     notifyLastResponse(newcall);
			break;
	}
	return 0;
}

void SalCallOp::setReplaces(const char *call_id, const char *from_tag, const char *to_tag) {
	belle_sip_header_replaces_t *replaces = belle_sip_header_replaces_create(call_id, from_tag, to_tag);
	SalOp::setReplaces(replaces);
}

void SalCallOp::setSdpHandling(SalOpSDPHandling handling) {
	if (handling != SalOpSDPNormal) ms_message("Enabling special SDP handling for SalOp[%p]!", this);
	mSdpHandling = handling;
}

void SalCallOp::processRefer(const belle_sip_request_event_t *event, belle_sip_server_transaction_t *server_transaction) {
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	belle_sip_header_refer_to_t *refer_to= belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_refer_to_t);
	belle_sip_header_referred_by_t *referred_by= belle_sip_message_get_header_by_type(BELLE_SIP_MESSAGE(req),belle_sip_header_referred_by_t);
	belle_sip_response_t* resp;
	belle_sip_uri_t* refer_to_uri;

	ms_message("Receiving REFER request on op [%p]", this);
	if (refer_to) {
		refer_to_uri=belle_sip_header_address_get_uri(BELLE_SIP_HEADER_ADDRESS(refer_to));

		if (refer_to_uri && belle_sip_uri_get_header(refer_to_uri,"Replaces")) {
			SalOp::setReplaces(belle_sip_header_replaces_create2(belle_sip_uri_get_header(refer_to_uri,"Replaces")));
			belle_sip_uri_remove_header(refer_to_uri,"Replaces");
		}
		if (referred_by){
			         setReferredBy(referred_by);
		}
		resp = createResponseFromRequest(req,202);
		belle_sip_server_transaction_send_response(server_transaction,resp);
		mRoot->mCallbacks.call_refer_received(this,(SalAddress*)BELLE_SIP_HEADER_ADDRESS(refer_to));
	} else {
		ms_warning("cannot do anything with the refer without destination");
		resp = createResponseFromRequest(req,400);
		belle_sip_server_transaction_send_response(server_transaction,resp);
	}

}

void SalCallOp::processNotify(const belle_sip_request_event_t *event, belle_sip_server_transaction_t* server_transaction) {
	belle_sip_request_t* req = belle_sip_request_event_get_request(event);
	const char* body = belle_sip_message_get_body(BELLE_SIP_MESSAGE(req));
	belle_sip_header_t* header_event=belle_sip_message_get_header(BELLE_SIP_MESSAGE(req),"Event");
	belle_sip_header_content_type_t* content_type = belle_sip_message_get_header_by_type(req,belle_sip_header_content_type_t);
	belle_sip_response_t* resp;

	ms_message("Receiving NOTIFY request on op [%p]", this);
	if (header_event
	&& strncasecmp(belle_sip_header_get_unparsed_value(header_event),"refer",strlen("refer"))==0
	&& content_type
	&& strcmp(belle_sip_header_content_type_get_type(content_type),"message")==0
	&& strcmp(belle_sip_header_content_type_get_subtype(content_type),"sipfrag")==0
	&& body){
		belle_sip_response_t* sipfrag=BELLE_SIP_RESPONSE(belle_sip_message_parse(body));

		if (sipfrag){
			int code=belle_sip_response_get_status_code(sipfrag);
			SalReferStatus status=SalReferFailed;
			if (code<200){
				status=SalReferTrying;
			}else if (code<300){
				status=SalReferSuccess;
			}else if (code>=400){
				status=SalReferFailed;
			}
			belle_sip_object_unref(sipfrag);
			resp = createResponseFromRequest(req,200);
			belle_sip_server_transaction_send_response(server_transaction,resp);
			mRoot->mCallbacks.notify_refer(this,status);
		}
	}else{
		ms_error("Notify without sipfrag or not for 'refer' event package, rejecting");
		resp = createResponseFromRequest(req, 489);
		belle_sip_server_transaction_send_response(server_transaction,resp);
	}
}

int SalCallOp::sendMessage (const Content &content) {
	if (!mDialog)
		return -1;
	belle_sip_request_t *req = belle_sip_dialog_create_queued_request(mDialog, "MESSAGE");
	prepareMessageRequest(req, content);
	return sendRequest(req);
}

bool SalCallOp::compareOp(const SalCallOp *op2) const {
	return mCallId == op2->mCallId;
}

void SalCallOp::handleOfferAnswerResponse(belle_sip_response_t* response) {
	if (mLocalMedia){
		/*this is the case where we received an invite without SDP*/
		if (mSdpOffering) {
			         setSdpFromDesc(BELLE_SIP_MESSAGE(response),mLocalMedia);
		}else{

			if ( mSdpAnswer==NULL )
			{
				if( mSdpHandling == SalOpSDPSimulateRemove ){
					ms_warning("Simulating SDP removal in answer for op %p", this);
				} else {
					               sdpProcess();
				}
			}

			if (mSdpAnswer){
				            setSdp(BELLE_SIP_MESSAGE(response),mSdpAnswer);
				belle_sip_object_unref(mSdpAnswer);
				mSdpAnswer=NULL;
			}
		}
	}else{
		ms_error("You are accepting a call but not defined any media capabilities !");
	}
}

LINPHONE_END_NAMESPACE
