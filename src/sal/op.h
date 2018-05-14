/*
 * op.h
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

#ifndef _L_SAL_OP_H_
#define _L_SAL_OP_H_

#include <bctoolbox/list.h>
#include <bctoolbox/port.h>
#include <belle-sip/types.h>

#include "c-wrapper/internal/c-sal.h"
#include "content/content.h"
#include "logger/logger.h"
#include "sal/sal.h"

LINPHONE_BEGIN_NAMESPACE

class SalOp {
public:
	SalOp (Sal *sal);
	virtual ~SalOp ();

	SalOp *ref ();
	void *unref ();

	Sal *getSal () const { return mRoot; }

	void setUserPointer (void *value) { mUserPointer = value; }
	void *getUserPointer () const { return mUserPointer; }

	void setSubject (const std::string &value) { mSubject = value; }
	const std::string &getSubject () const { return mSubject; }

	void setFrom (const std::string &value);
	void setFromAddress (const SalAddress *value);
	const std::string &getFrom () const { return mFrom; }
	const SalAddress *getFromAddress () const { return mFromAddress; }

	void setTo (const std::string &value);
	void setToAddress (const SalAddress *value);
	const std::string &getTo () const { return mTo; }
	const SalAddress *getToAddress () const { return mToAddress; }

	void setContactAddress (const SalAddress* value);
	const SalAddress *getContactAddress() const { return mContactAddress; }

	void setRoute (const std::string &value);
	void setRouteAddress (const SalAddress *value);
	const std::list<SalAddress *> &getRouteAddresses () const { return mRouteAddresses; }
	void addRouteAddress (const SalAddress *address);

	void setDiversionAddress (const SalAddress *value);
	const SalAddress *getDiversionAddress () const { return mDiversionAddress; }

	void setServiceRoute (const SalAddress *value);
	const SalAddress *getServiceRoute () const { return mServiceRoute; }

	void setManualRefresherMode (bool value) { mManualRefresher = value; }

	void setEntityTag (const std::string &value) { mEntityTag = value; }
	const std::string &getEntityTag() const { return mEntityTag; }

	void setEvent (const std::string &eventName);

	void setPrivacy (SalPrivacyMask value) { mPrivacy = value; }
	SalPrivacyMask getPrivacy() const { return mPrivacy; }

	void setRealm (const std::string &value) { mRealm = value; }

	void setSentCustomHeaders (SalCustomHeader *ch);

	void enableCnxIpTo0000IfSendOnly (bool value) { mCnxIpTo0000IfSendOnlyEnabled = value; }
	bool cnxIpTo0000IfSendOnlyEnabled () const { return mCnxIpTo0000IfSendOnlyEnabled; }

	const std::string &getProxy () const { return mRoute; }
	const std::string &getNetworkOrigin () const { return mOrigin; }
	const std::string &getCallId () const { return  mCallId; }
	std::string getDialogId () const;
	int getAddressFamily () const;
	const SalCustomHeader *getRecvCustomHeaders () const { return mRecvCustomHeaders; }
	const std::string &getRemoteContact () const { return mRemoteContact; }
	const SalAddress *getRemoteContactAddress () const { return mRemoteContactAddress; }
	const std::string &getRemoteUserAgent () const { return mRemoteUserAgent; }

	const char *getPublicAddress (int *port) {
		return mRefresher ? belle_sip_refresher_get_public_address(mRefresher, port) : nullptr;
	}
	const char *getLocalAddress (int *port) {
		return mRefresher ? belle_sip_refresher_get_local_address(mRefresher, port) : nullptr;
	}

	const SalErrorInfo *getErrorInfo () const { return &mErrorInfo; }
	const SalErrorInfo *getReasonErrorInfo () const { return &mReasonErrorInfo; }

	bool isForkedOf (const SalOp *op) const {
		return !mCallId.empty() && !op->mCallId.empty() && (mCallId == op->mCallId);
	}
	bool isIdle () const;

	void stopRefreshing () {
		if (mRefresher)
			belle_sip_refresher_stop(mRefresher);
	}
	int refresh ();
	void killDialog ();
	void release ();

	virtual void authenticate (const SalAuthInfo *info) { 
        processAuthentication(); }
	void cancelAuthentication () { lFatal() << "SalOp::cancelAuthentication not implemented yet"; }
	SalAuthInfo *getAuthRequested () { return mAuthInfo; }

	int ping (const std::string &from, const std::string &to);
	int sendInfo (const SalBodyHandler *bodyHandler);

protected:
	enum class State {
		Early = 0,
		Active,
		Terminating, // This state is used to wait until a proceeding state, so we can send the cancel
		Terminated
	};

	static std::string toString (const State value);

	enum class Dir {
		Incoming = 0,
		Outgoing
	};

	enum class Type {
		Unknown,
		Register,
		Call,
		Message,
		Presence,
		Publish,
		Subscribe,
		Refer // For out of dialog refer only
	};

	static std::string toString (const Type type);

	using ReleaseCb = void (*) (SalOp *op);

	virtual void fillCallbacks () {}
	void releaseImpl ();
	void processAuthentication ();
	int processRedirect ();

	belle_sip_request_t *buildRequest (const std::string &method);
	int sendRequest (belle_sip_request_t *request);
	int sendRequestWithContact (belle_sip_request_t *request, bool addContact);
	int sendRequestWithExpires (belle_sip_request_t *request, int expires);
	void resendRequest (belle_sip_request_t *request);
	int sendRequestAndCreateRefresher (belle_sip_request_t *request, int expires, belle_sip_refresher_listener_t listener);

	void setReasonErrorInfo (belle_sip_message_t *message);
	void setErrorInfoFromResponse (belle_sip_response_t *response);

	void setReferredBy (belle_sip_header_referred_by_t *referredByHeader);
	void setReplaces (belle_sip_header_replaces_t *replacesHeader);

	void setRemoteContact (const std::string &value);
	void setNetworkOrigin (const std::string &value);
	void setNetworkOriginAddress (SalAddress *value);
	void setPrivacyFromMessage (belle_sip_message_t *message);
	void setRemoteUserAgent (belle_sip_message_t *message);

	belle_sip_response_t *createResponseFromRequest (belle_sip_request_t *request, int code) {
		return mRoot->createResponseFromRequest(request, code);
	}
	belle_sip_header_contact_t *createContact ();

	void setOrUpdateDialog (belle_sip_dialog_t *dialog);
	belle_sip_dialog_t *linkOpWithDialog (belle_sip_dialog_t *dialog);
	void unlinkOpFromDialog (belle_sip_dialog_t *dialog);

	SalBodyHandler *getBodyHandler (belle_sip_message_t *message);

	void assignRecvHeaders (belle_sip_message_t *message);

	bool isSecure () const;
	void addHeaders (belle_sip_header_t *h, belle_sip_message_t *message);
	void addCustomHeaders (belle_sip_message_t *message);
	int unsubscribe ();

	void processIncomingMessage (const belle_sip_request_event_t *event);
	int replyMessage (SalReason reason);
	void addMessageAccept (belle_sip_message_t *message);

	static bool isExternalBody (belle_sip_header_content_type_t* contentType);

	static void assignAddress (SalAddress **address, const std::string &value);
	static void addInitialRouteSet (belle_sip_request_t *request, const std::list<SalAddress *> &routeAddresses);

	// SalOpBase
	Sal *mRoot = nullptr;
	std::string mRoute; // Or request-uri for REGISTER
	std::list<SalAddress *> mRouteAddresses;
	SalAddress *mContactAddress = nullptr;
	std::string mSubject;
	std::string mFrom;
	SalAddress* mFromAddress = nullptr;
	std::string mTo;
	SalAddress *mToAddress = nullptr;
	std::string mOrigin;
	SalAddress *mOriginAddress = nullptr;
	SalAddress *mDiversionAddress = nullptr;
	std::string mRemoteUserAgent;
	SalAddress *mRemoteContactAddress = nullptr;
	std::string mRemoteContact;
	void *mUserPointer = nullptr;
	std::string mCallId;
	std::string mRealm;
	SalAddress *mServiceRoute = nullptr; // As defined by rfc3608, might be a list
	SalCustomHeader *mSentCustomHeaders = nullptr;
	SalCustomHeader *mRecvCustomHeaders = nullptr;
	std::string mEntityTag; // As defined by rfc3903 (I.E publih)
	ReleaseCb mReleaseCb = nullptr;

	const belle_sip_listener_callbacks_t *mCallbacks = nullptr;
	SalErrorInfo mErrorInfo;
	SalErrorInfo mReasonErrorInfo;
	belle_sip_client_transaction_t *mPendingAuthTransaction = nullptr;
	belle_sip_server_transaction_t *mPendingServerTransaction = nullptr;
	belle_sip_server_transaction_t *mPendingUpdateServerTransaction = nullptr;
	belle_sip_client_transaction_t *mPendingClientTransaction = nullptr;
	SalAuthInfo *mAuthInfo = nullptr;
	belle_sip_dialog_t *mDialog = nullptr;
	belle_sip_header_replaces_t *mReplaces = nullptr;
	belle_sip_header_referred_by_t *mReferredBy = nullptr;
	SalMediaDescription *mResult = nullptr;
	belle_sdp_session_description_t *mSdpAnswer = nullptr;
	State mState = State::Early;
	Dir mDir = Dir::Incoming;
	belle_sip_refresher_t *mRefresher = nullptr;
	int mRef = 0;
	Type mType = Type::Unknown;
	SalPrivacyMask mPrivacy = SalPrivacyNone;
	belle_sip_header_event_t *mEvent = nullptr; // Used by SalOpSubscribe kinds
	SalOpSDPHandling mSdpHandling = SalOpSDPNormal;
	int mAuthRequests = 0; // number of auth requested for this op
	bool mCnxIpTo0000IfSendOnlyEnabled = false;
	bool mAutoAnswerAsked = false;
	bool mSdpOffering = false;
	bool mCallReleased = false;
	bool mManualRefresher = false;
	bool mHasAuthPending = false;
	bool mSupportsSessionTimers = false;
	bool mOpReleased = false;

	friend class Sal;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_SAL_OP_H_
