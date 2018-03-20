/*
 * call-session-p.h
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

#ifndef _L_CALL_SESSION_P_H_
#define _L_CALL_SESSION_P_H_

#include "object/object-p.h"

#include "call-session.h"
#include "sal/call-op.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSessionPrivate : public ObjectPrivate, public CoreListener {
public:
	int computeDuration () const;
	virtual void initializeParamsAccordingToIncomingCallParams ();
	void notifyReferState ();
	virtual void setState (CallSession::State newState, const std::string &message);
	void setTransferState (CallSession::State newState);
	void startIncomingNotification ();
	bool startPing ();
	void setPingTime (int value) { pingTime = value; }

	void createOp ();
	CallSessionParams *getCurrentParams () const { return currentParams; }
	LinphoneProxyConfig * getDestProxy () const { return destProxy; }
	SalCallOp * getOp () const { return op; }
	bool isBroken () const { return broken; }
	bool isInConference () const;
	void setParams (CallSessionParams *csp);
	void setReferPending (bool value) { referPending = value; }
	void setTransferTarget (std::shared_ptr<CallSession> session) { transferTarget = session; }

	virtual void abort (const std::string &errorMsg);
	virtual void accepted ();
	void ackBeingSent (LinphoneHeaders *headers);
	virtual void ackReceived (LinphoneHeaders *headers);
	void cancelDone ();
	virtual bool failure ();
	void infoReceived (SalBodyHandler *bodyHandler);
	void pingReply ();
	void referred (const Address &referToAddr);
	virtual void remoteRinging ();
	void replaceOp (SalCallOp *newOp);
	virtual void terminated ();
	void updated (bool isUpdate);
	void updatedByRemote ();
	virtual void updating (bool isUpdate);

	void setCallSessionListener (CallSessionListener *listener) { this->listener = listener; }

protected:
	void init ();

	void accept (const CallSessionParams *params);
	virtual LinphoneStatus acceptUpdate (const CallSessionParams *csp, CallSession::State nextState, const std::string &stateInfo);
	LinphoneStatus checkForAcceptation ();
	virtual void handleIncomingReceivedStateInIncomingNotification ();
	virtual bool isReadyForInvite () const;
	bool isUpdateAllowed (CallSession::State &nextState) const;
	virtual int restartInvite ();
	virtual void setReleased ();
	virtual void setTerminated ();
	virtual LinphoneStatus startAcceptUpdate (CallSession::State nextState, const std::string &stateInfo);
	virtual LinphoneStatus startUpdate (const std::string &subject);
	virtual void terminate ();
	virtual void updateCurrentParams () const;

	void setBroken ();
	void setContactOp ();

	// CoreListener
	void onNetworkReachable (bool sipNetworkReachable, bool mediaNetworkReachable) override;
	void onRegistrationStateChanged (LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const std::string &message) override;

private:
	void completeLog ();
	void createOpTo (const LinphoneAddress *to);

	LinphoneAddress * getFixedContact () const;

	virtual void reinviteToRecoverFromConnectionLoss ();
	void repairByInviteWithReplaces ();
	void repairIfBroken ();

protected:
	CallSessionListener *listener = nullptr;

	CallSessionParams *params = nullptr;
	mutable CallSessionParams *currentParams = nullptr;
	CallSessionParams *remoteParams = nullptr;
	mutable Address diversionAddress;
	mutable Address remoteContactAddress;
	mutable Address toAddress;

	std::string subject;
	LinphoneCallDir direction = LinphoneCallOutgoing;
	CallSession::State state = CallSession::State::Idle;
	CallSession::State prevState = CallSession::State::Idle;
	CallSession::State transferState = CallSession::State::Idle;
	LinphoneProxyConfig *destProxy = nullptr;
	LinphoneErrorInfo *ei = nullptr;
	LinphoneCallLog *log = nullptr;
	std::string referTo;

	SalCallOp *op = nullptr;

	SalOp *pingOp = nullptr;
	bool pingReplied = false;
	int pingTime = 0;

	std::shared_ptr<CallSession> referer;
	std::shared_ptr<CallSession> transferTarget;

	bool broken = false;
	bool deferIncomingNotification = false;
	bool deferUpdate = false;
	bool deferUpdateInternal = false;
	bool needLocalIpRefresh = false;
	bool nonOpError = false; /* Set when the LinphoneErrorInfo was set at higher level than sal */
	bool notifyRinging = true;
	bool referPending = false;
	bool reinviteOnCancelResponseRequested = false;

private:
	L_DECLARE_PUBLIC(CallSession);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CALL_SESSION_P_H_
