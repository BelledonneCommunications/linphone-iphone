/*
 * call-session-p.h
 * Copyright (C) 2010-2017 Belledonne Communications SARL
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

#ifndef _CALL_SESSION_P_H_
#define _CALL_SESSION_P_H_

#include "object/object-p.h"

#include "call-session.h"
#include "sal/call-op.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSessionPrivate : public ObjectPrivate {
public:
	CallSessionPrivate (const Conference &conference, const CallSessionParams *params, CallSessionListener *listener);
	virtual ~CallSessionPrivate ();

	int computeDuration () const;
	virtual void initializeParamsAccordingToIncomingCallParams ();
	virtual void setState (LinphoneCallState newState, const std::string &message);
	void startIncomingNotification ();
	bool startPing ();
	void setPingTime (int value) { pingTime = value; }

	LinphoneProxyConfig * getDestProxy () const { return destProxy; }
	SalCallOp * getOp () const { return op; }

	virtual void abort (const std::string &errorMsg);
	virtual void accepted ();
	void ackBeingSent (LinphoneHeaders *headers);
	virtual void ackReceived (LinphoneHeaders *headers);
	virtual bool failure ();
	void pingReply ();
	virtual void remoteRinging ();
	virtual void terminated ();
	void updated (bool isUpdate);
	void updatedByRemote ();
	virtual void updating (bool isUpdate);

protected:
	void accept (const CallSessionParams *params);
	virtual LinphoneStatus acceptUpdate (const CallSessionParams *csp, LinphoneCallState nextState, const std::string &stateInfo);
	LinphoneStatus checkForAcceptation () const;
	virtual void handleIncomingReceivedStateInIncomingNotification ();
	virtual bool isReadyForInvite () const;
	bool isUpdateAllowed (LinphoneCallState &nextState) const;
	virtual int restartInvite ();
	virtual void setReleased ();
	virtual void setTerminated ();
	virtual LinphoneStatus startAcceptUpdate (LinphoneCallState nextState, const std::string &stateInfo);
	virtual LinphoneStatus startUpdate (const std::string &subject);
	virtual void terminate ();
	virtual void updateCurrentParams () const;

	void setContactOp ();

private:
	void completeLog ();
	void createOp ();
	void createOpTo (const LinphoneAddress *to);

	LinphoneAddress * getFixedContact () const;

protected:
	const Conference &conference;
	LinphoneCore *core = nullptr;
	CallSessionListener *listener = nullptr;

	CallSessionParams *params = nullptr;
	mutable CallSessionParams *currentParams = nullptr;
	CallSessionParams *remoteParams = nullptr;
	mutable Address remoteContactAddress;

	std::string subject;
	LinphoneCallDir direction = LinphoneCallOutgoing;
	LinphoneCallState state = LinphoneCallIdle;
	LinphoneCallState prevState = LinphoneCallIdle;
	//LinphoneCallState transferState = LinphoneCallIdle;
	LinphoneProxyConfig *destProxy = nullptr;
	LinphoneErrorInfo *ei = nullptr;
	LinphoneCallLog *log = nullptr;

	SalCallOp *op = nullptr;

	SalOp *pingOp = nullptr;
	bool pingReplied = false;
	int pingTime = 0;

	bool deferIncomingNotification = false;
	bool deferUpdate = false;
	bool nonOpError = false; /* Set when the LinphoneErrorInfo was set at higher level than sal */

private:
	L_DECLARE_PUBLIC(CallSession);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CALL_SESSION_P_H_
