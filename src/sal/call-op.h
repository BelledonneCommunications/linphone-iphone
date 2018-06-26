/*
 * call-op.h
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

#ifndef _L_SAL_CALL_OP_H_
#define _L_SAL_CALL_OP_H_

#include "sal/op.h"
#include "sal/message-op-interface.h"

LINPHONE_BEGIN_NAMESPACE

class SalCallOp : public SalOp, public SalMessageOpInterface {
public:
	SalCallOp (Sal *sal) : SalOp(sal) {}
	~SalCallOp () override;

	SalMediaDescription *getLocalMediaDescription () const { return mLocalMedia; }
	int setLocalMediaDescription (SalMediaDescription *desc);
	int setLocalBody (const Content &body);
	int setLocalBody (const Content &&body);

	SalMediaDescription *getRemoteMediaDescription () { return mRemoteMedia; }
	const Content &getRemoteBody () const { return mRemoteBody; }
	SalMediaDescription *getFinalMediaDescription ();

	int call (const char *from, const char *to, const char *subject);
	int notifyRinging (bool earlyMedia);
	int accept ();
	int decline (SalReason reason, const char *redirection = nullptr);
	int declineWithErrorInfo (const SalErrorInfo *info, const SalAddress *redirectionAddr = nullptr);
	int update (const char *subject, bool noUserConsent);
	int cancelInvite (const SalErrorInfo *info = nullptr);
	int refer (const char *referTo);
	int referWithReplaces (SalCallOp *otherCallOp);
	int setReferrer (SalCallOp *referredCall);
	SalCallOp *getReplaces () const;
	int sendDtmf (char dtmf);
	int terminate (const SalErrorInfo *info = nullptr);
	bool autoAnswerAsked () const { return mAutoAnswerAsked; }
	void sendVfuRequest ();
	int isOfferer () const { return mSdpOffering; }
	int notifyReferState (SalCallOp *newCallOp);
	bool compareOp (const SalCallOp *otherCallOp) const;
	bool dialogRequestPending () const { return (belle_sip_dialog_request_pending(mDialog) != 0); }
	const char *getLocalTag () { return belle_sip_dialog_get_local_tag(mDialog); }
	const char *getRemoteTag () { return belle_sip_dialog_get_remote_tag(mDialog); }
	void setReplaces (const char *callId, const char *fromTag, const char *toTag);
	void setSdpHandling (SalOpSDPHandling handling);

	// Implementation of SalMessageOpInterface
	int sendMessage (const Content &content) override;
	int reply (SalReason reason) override { return SalOp::replyMessage(reason); }

private:
	virtual void fillCallbacks () override;
	void setReleased ();

	void setError (belle_sip_response_t *response, bool fatal);
	void callTerminated (belle_sip_server_transaction_t *serverTransaction, int statusCode, belle_sip_request_t *cancelRequest);
	void resetDescriptions ();

	int parseSdpBody (const Content &body, belle_sdp_session_description_t **sessionDesc, SalReason *error);
	void sdpProcess ();
	void handleBodyFromResponse (belle_sip_response_t *response);
	SalReason processBodyForInvite (belle_sip_request_t *invite);
	SalReason processBodyForAck (belle_sip_request_t *ack);
	void handleOfferAnswerResponse (belle_sip_response_t *response);

	void fillInvite (belle_sip_request_t *invite);
	void cancellingInvite (const SalErrorInfo *info);
	int referTo (belle_sip_header_refer_to_t *referToHeader, belle_sip_header_referred_by_t *referredByHeader);
	int sendNotifyForRefer (int code, const char *reason);
	void notifyLastResponse (SalCallOp *newCallOp);
	void processRefer (const belle_sip_request_event_t *event, belle_sip_server_transaction_t *serverTransaction);
	void processNotify (const belle_sip_request_event_t *event, belle_sip_server_transaction_t *serverTransaction);

	static void setAddrTo0000 (char value[], size_t sz);
	static int isMediaDescriptionAcceptable (SalMediaDescription *md);
	static bool isAPendingIncomingInviteTransaction (belle_sip_transaction_t *tr);
	static void setCallAsReleased (SalCallOp *op);
	static void unsupportedMethod (belle_sip_server_transaction_t *serverTransaction, belle_sip_request_t *request);
	static belle_sip_header_reason_t *makeReasonHeader (const SalErrorInfo *info);
	static belle_sip_header_allow_t *createAllow (bool enableUpdate);
	static std::vector<char> marshalMediaDescription (belle_sdp_session_description_t *sessionDesc, belle_sip_error_code &error);

	// belle_sip_message handlers
	static int setCustomBody (belle_sip_message_t *message, const Content &body);
	static int setSdp (belle_sip_message_t *message, belle_sdp_session_description_t *sessionDesc);
	static int setSdpFromDesc (belle_sip_message_t *message, const SalMediaDescription *desc);
	static void processIoErrorCb (void *userCtx, const belle_sip_io_error_event_t *event);
	static Content extractBody (belle_sip_message_t *message);

	// Callbacks
	static int vfuRetryCb (void *userCtx, unsigned int events);
	static void processResponseCb (void *userCtx, const belle_sip_response_event_t *event);
	static void processTimeoutCb (void *userCtx, const belle_sip_timeout_event_t *event);
	static void processTransactionTerminatedCb (void *userCtx, const belle_sip_transaction_terminated_event_t *event);
	static void processRequestEventCb (void *userCtx, const belle_sip_request_event_t *event);
	static void processDialogTerminatedCb (void *userCtx, const belle_sip_dialog_terminated_event_t *event);

	// Private constants
	static const size_t SIP_MESSAGE_BODY_LIMIT = 16 * 1024; // 16kB

	// Attributes
	SalMediaDescription *mLocalMedia = nullptr;
	SalMediaDescription *mRemoteMedia = nullptr;
	Content mLocalBody;
	Content mRemoteBody;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_SAL_CALL_OP_H_
