/*
 * call-session.cpp
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

#include <bctoolbox/defs.h>

#include "c-wrapper/c-wrapper.h"

#include "address/address-p.h"
#include "call/call-p.h"
#include "conference/params/call-session-params-p.h"
#include "conference/session/call-session-p.h"
#include "conference/session/call-session.h"
#include "core/core-p.h"

#include "logger/logger.h"

#include "linphone/core.h"

#include "private.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

int CallSessionPrivate::computeDuration () const {
	if (log->connected_date_time == 0)
		return 0;
	return (int)(ms_time(nullptr) - log->connected_date_time);
}

/*
 * Initialize call parameters according to incoming call parameters. This is to avoid to ask later (during reINVITEs) for features that the remote
 * end apparently does not support. This features are: privacy, video...
 */
void CallSessionPrivate::initializeParamsAccordingToIncomingCallParams () {
	currentParams->setPrivacy((LinphonePrivacyMask)op->get_privacy());
}

void CallSessionPrivate::notifyReferState () {
	SalCallOp *refererOp = referer->getPrivate()->getOp();
	if (refererOp)
		refererOp->notify_refer_state(op);
}

void CallSessionPrivate::setState(LinphoneCallState newState, const string &message) {
	L_Q();
	if (state != newState){
		prevState = state;

		/* Make sanity checks with call state changes. Any bad transition can result in unpredictable results
		   or irrecoverable errors in the application. */
		if ((state == LinphoneCallEnd) || (state == LinphoneCallError)) {
			if (newState != LinphoneCallReleased) {
				lFatal() << "Abnormal call resurection from " << linphone_call_state_to_string(state) <<
					" to " << linphone_call_state_to_string(newState) << " , aborting";
				return;
			}
		} else if ((newState == LinphoneCallReleased) && (prevState != LinphoneCallError) && (prevState != LinphoneCallEnd)) {
			lFatal() << "Attempt to move CallSession [" << q << "] to Released state while it was not previously in Error or End state, aborting";
			return;
		}
		lInfo() << "CallSession [" << q << "] moving from state " << linphone_call_state_to_string(state) << " to " << linphone_call_state_to_string(newState);

		if (newState != LinphoneCallRefered) {
			/* LinphoneCallRefered is rather an event, not a state.
			   Indeed it does not change the state of the call (still paused or running). */
			state = newState;
		}

		switch (newState) {
			case LinphoneCallOutgoingInit:
			case LinphoneCallIncomingReceived:
				getPlatformHelpers(q->getCore()->getCCore())->acquireWifiLock();
				getPlatformHelpers(q->getCore()->getCCore())->acquireMcastLock();
				getPlatformHelpers(q->getCore()->getCCore())->acquireCpuLock();
				break;
			case LinphoneCallEnd:
			case LinphoneCallError:
				switch (linphone_error_info_get_reason(q->getErrorInfo())) {
					case LinphoneReasonDeclined:
						log->status = LinphoneCallDeclined;
						break;
					case LinphoneReasonNotAnswered:
						if (log->dir == LinphoneCallIncoming)
							log->status = LinphoneCallMissed;
						break;
					case LinphoneReasonNone:
						if (log->dir == LinphoneCallIncoming) {
							if (ei) {
								int code = linphone_error_info_get_protocol_code(ei);
								if ((code >= 200) && (code < 300))
									log->status = LinphoneCallAcceptedElsewhere;
							}
						}
						break;
					case LinphoneReasonDoNotDisturb:
						if (log->dir == LinphoneCallIncoming) {
							if (ei) {
								int code = linphone_error_info_get_protocol_code(ei);
								if ((code >= 600) && (code < 700))
									log->status = LinphoneCallDeclinedElsewhere;
							}
						}
						break;
					default:
						break;
				}
				setTerminated();
				break;
			case LinphoneCallConnected:
				log->status = LinphoneCallSuccess;
				log->connected_date_time = ms_time(nullptr);
				break;
			case LinphoneCallReleased:
				getPlatformHelpers(q->getCore()->getCCore())->acquireWifiLock();
				getPlatformHelpers(q->getCore()->getCCore())->acquireMcastLock();
				getPlatformHelpers(q->getCore()->getCCore())->acquireCpuLock();
				break;
			default:
				break;
		}

		if (message.empty()) {
			lError() << "You must fill a reason when changing call state (from " <<
				linphone_call_state_to_string(prevState) << " to " << linphone_call_state_to_string(state) << ")";
		}
		if (listener)
			listener->onCallSessionStateChanged(q->getSharedFromThis(), newState, message);
		if (newState == LinphoneCallReleased)
			setReleased(); /* Shall be performed after app notification */
	}
}

void CallSessionPrivate::setTransferState (LinphoneCallState newState) {
	L_Q();
	if (newState == transferState)
		return;
	lInfo() << "Transfer state for CallSession [" << q << "] changed from ["
		<< linphone_call_state_to_string(transferState) << "] to [" << linphone_call_state_to_string(newState) << "]";
	transferState = newState;
	if (listener)
		listener->onCallSessionTransferStateChanged(q->getSharedFromThis(), newState);
}

void CallSessionPrivate::startIncomingNotification () {
	L_Q();
	if (listener)
		listener->onIncomingCallSessionStarted(q->getSharedFromThis());

	setState(LinphoneCallIncomingReceived, "Incoming CallSession");

	// From now on, the application is aware of the call and supposed to take background task or already submitted
	// notification to the user. We can then drop our background task.
	if (listener)
		listener->onBackgroundTaskToBeStopped(q->getSharedFromThis());

	if (state == LinphoneCallIncomingReceived) {
		handleIncomingReceivedStateInIncomingNotification();
	}
}

bool CallSessionPrivate::startPing () {
	L_Q();
	if (q->getCore()->getCCore()->sip_conf.ping_with_options) {
		/* Defer the start of the call after the OPTIONS ping for outgoing call or
		 * send an option request back to the caller so that we get a chance to discover our nat'd address
		 * before answering for incoming call */
		pingReplied = false;
		pingOp = new SalOp(q->getCore()->getCCore()->sal);
		if (direction == LinphoneCallIncoming) {
			const char *from = pingOp->get_from();
			const char *to = pingOp->get_to();
			linphone_configure_op(q->getCore()->getCCore(), pingOp, log->from, nullptr, false);
			pingOp->set_route(op->get_network_origin());
			pingOp->ping(from, to);
		} else if (direction == LinphoneCallOutgoing) {
			char *from = linphone_address_as_string(log->from);
			char *to = linphone_address_as_string(log->to);
			pingOp->ping(from, to);
			ms_free(from);
			ms_free(to);
		}
		pingOp->set_user_pointer(this);
		return true;
	}
	return false;
}

// -----------------------------------------------------------------------------

void CallSessionPrivate::setParams (CallSessionParams *csp) {
	if (params)
		delete params;
	params = csp;
}

bool CallSessionPrivate::isInConference () const {
	return params->getPrivate()->getInConference();
}

// -----------------------------------------------------------------------------

void CallSessionPrivate::abort (const string &errorMsg) {
	op->terminate();
	setState(LinphoneCallError, errorMsg);
}

void CallSessionPrivate::accepted () {
	/* Immediately notify the connected state, even if errors occur after */
	switch (state) {
		case LinphoneCallOutgoingProgress:
		case LinphoneCallOutgoingRinging:
		case LinphoneCallOutgoingEarlyMedia:
			/* Immediately notify the connected state */
			setState(LinphoneCallConnected, "Connected");
			break;
		default:
			break;
	}
	currentParams->setPrivacy((LinphonePrivacyMask)op->get_privacy());
}

void CallSessionPrivate::ackBeingSent (LinphoneHeaders *headers) {
	L_Q();
	if (listener)
		listener->onAckBeingSent(q->getSharedFromThis(), headers);
}

void CallSessionPrivate::ackReceived (LinphoneHeaders *headers) {
	L_Q();
	if (listener)
		listener->onAckReceived(q->getSharedFromThis(), headers);
}

void CallSessionPrivate::cancelDone () {
	if (reinviteOnCancelResponseRequested) {
		reinviteOnCancelResponseRequested = false;
		reinviteToRecoverFromConnectionLoss();
	}
}

bool CallSessionPrivate::failure () {
	L_Q();
	const SalErrorInfo *ei = op->get_error_info();
	switch (ei->reason) {
		case SalReasonRedirect:
			if ((state == LinphoneCallOutgoingInit) || (state == LinphoneCallOutgoingProgress)
				|| (state == LinphoneCallOutgoingRinging) /* Push notification case */ || (state == LinphoneCallOutgoingEarlyMedia)) {
				const SalAddress *redirectionTo = op->get_remote_contact_address();
				if (redirectionTo) {
					char *url = sal_address_as_string(redirectionTo);
					lWarning() << "Redirecting CallSession [" << q << "] to " << url;
					if (log->to)
						linphone_address_unref(log->to);
					log->to = linphone_address_new(url);
					ms_free(url);
					restartInvite();
					return true;
				}
			}
			break;
		default:
			break;
	}

	/* Some call errors are not fatal */
	switch (state) {
		case LinphoneCallUpdating:
		case LinphoneCallPausing:
		case LinphoneCallResuming:
			if (ei->reason != SalReasonNoMatch) {
				lInfo() << "Call error on state [" << linphone_call_state_to_string(state) << "], restoring previous state [" << linphone_call_state_to_string(prevState) << "]";
				setState(prevState, ei->full_string);
				return true;
			}
		default:
			break;
	}

	if ((state != LinphoneCallEnd) && (state != LinphoneCallError)) {
		if (ei->reason == SalReasonDeclined)
			setState(LinphoneCallEnd, "Call declined");
		else {
			if (linphone_call_state_is_early(state))
				setState(LinphoneCallError, ei->full_string ? ei->full_string : "");
			else
				setState(LinphoneCallEnd, ei->full_string ? ei->full_string : "");
		}
		if ((ei->reason != SalReasonNone) && listener)
			listener->onPlayErrorTone(q->getSharedFromThis(), linphone_reason_from_sal(ei->reason));
	}
	if (referer) {
		// Notify referer of the failure
		notifyReferState();
	}
	return false;
}

void CallSessionPrivate::infoReceived (SalBodyHandler *bodyHandler) {
	L_Q();
	LinphoneInfoMessage *info = linphone_core_create_info_message(q->getCore()->getCCore());
	linphone_info_message_set_headers(info, op->get_recv_custom_header());
	if (bodyHandler) {
		LinphoneContent *content = linphone_content_from_sal_body_handler(bodyHandler);
		linphone_info_message_set_content(info, content);
		linphone_content_unref(content);
	}
	if (listener)
		listener->onInfoReceived(q->getSharedFromThis(), info);
	linphone_info_message_unref(info);
}

void CallSessionPrivate::pingReply () {
	L_Q();
	if (state == LinphoneCallOutgoingInit) {
		pingReplied = true;
		if (isReadyForInvite())
			q->startInvite(nullptr, "");
	}
}

void CallSessionPrivate::referred (const Address &referToAddr) {
	L_Q();
	referTo = referToAddr.asString();
	referPending = true;
	setState(LinphoneCallRefered, "Referred");
	if (referPending && listener)
		listener->onCallSessionStartReferred(q->getSharedFromThis());
}

void CallSessionPrivate::remoteRinging () {
	L_Q();
	/* Set privacy */
	currentParams->setPrivacy((LinphonePrivacyMask)op->get_privacy());
	if (listener)
		listener->onStartRinging(q->getSharedFromThis());
	lInfo() << "Remote ringing...";
	setState(LinphoneCallOutgoingRinging, "Remote ringing");
}

void CallSessionPrivate::replaceOp (SalCallOp *newOp) {
	L_Q();
	SalCallOp *oldOp = op;
	LinphoneCallState oldState = state;
	op = newOp;
	op->set_user_pointer(q);
	op->set_local_media_description(oldOp->get_local_media_description());
	switch (state) {
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallIncomingReceived:
			op->notify_ringing((state == LinphoneCallIncomingEarlyMedia) ? true : false);
			break;
		case LinphoneCallConnected:
		case LinphoneCallStreamsRunning:
			op->accept();
			break;
		default:
			lWarning() << "CallSessionPrivate::replaceOp(): don't know what to do in state [" << linphone_call_state_to_string(state) << "]";
			break;
	}
	switch (oldState) {
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallIncomingReceived:
			op->set_user_pointer(nullptr); // In order for the call session to not get terminated by terminating this op
			// Do not terminate a forked INVITE
			if (op->get_replaces())
				oldOp->terminate();
			else
				oldOp->kill_dialog();
			break;
		case LinphoneCallConnected:
		case LinphoneCallStreamsRunning:
			oldOp->terminate();
			oldOp->kill_dialog();
			break;
		default:
			break;
	}
	oldOp->release();
}

void CallSessionPrivate::terminated () {
	L_Q();
	switch (state) {
		case LinphoneCallEnd:
		case LinphoneCallError:
			lWarning() << "terminated: already terminated, ignoring";
			return;
		case LinphoneCallIncomingReceived:
		case LinphoneCallIncomingEarlyMedia:
			if (!op->get_reason_error_info()->protocol || strcmp(op->get_reason_error_info()->protocol, "") == 0) {
				linphone_error_info_set(ei, nullptr, LinphoneReasonNotAnswered, 0, "Incoming call cancelled", nullptr);
				nonOpError = true;
			}
			break;
		default:
			break;
	}
	if (referPending && listener)
		listener->onCallSessionStartReferred(q->getSharedFromThis());
	if (listener)
		listener->onStopRingingIfInCall(q->getSharedFromThis());
	setState(LinphoneCallEnd, "Call ended");
}

void CallSessionPrivate::updated (bool isUpdate) {
	L_Q();
	deferUpdate = !!lp_config_get_int(linphone_core_get_config(q->getCore()->getCCore()), "sip", "defer_update_default", FALSE);
	SalErrorInfo sei;
	memset(&sei, 0, sizeof(sei));
	switch (state) {
		case LinphoneCallPausedByRemote:
			updatedByRemote();
			break;
		/* SIP UPDATE CASE */
		case LinphoneCallOutgoingRinging:
		case LinphoneCallOutgoingEarlyMedia:
		case LinphoneCallIncomingEarlyMedia:
			if (isUpdate) {
				setState(LinphoneCallEarlyUpdatedByRemote, "EarlyUpdatedByRemote");
				acceptUpdate(nullptr, prevState, linphone_call_state_to_string(prevState));
			}
			break;
		case LinphoneCallStreamsRunning:
		case LinphoneCallConnected:
		case LinphoneCallUpdatedByRemote: /* Can happen on UAC connectivity loss */
			updatedByRemote();
			break;
		case LinphoneCallPaused:
			/* We'll remain in pause state but accept the offer anyway according to default parameters */
			acceptUpdate(nullptr, state, linphone_call_state_to_string(state));
			break;
		case LinphoneCallUpdating:
		case LinphoneCallPausing:
		case LinphoneCallResuming:
			sal_error_info_set(&sei, SalReasonInternalError, "SIP", 0, nullptr, nullptr);
			op->decline_with_error_info(&sei, nullptr);
			BCTBX_NO_BREAK; /* no break */
		case LinphoneCallIdle:
		case LinphoneCallOutgoingInit:
		case LinphoneCallEnd:
		case LinphoneCallIncomingReceived:
		case LinphoneCallOutgoingProgress:
		case LinphoneCallRefered:
		case LinphoneCallError:
		case LinphoneCallReleased:
		case LinphoneCallEarlyUpdatedByRemote:
		case LinphoneCallEarlyUpdating:
			lWarning() << "Receiving reINVITE or UPDATE while in state [" << linphone_call_state_to_string(state) << "], should not happen";
		break;
	}
}

void CallSessionPrivate::updatedByRemote () {
	L_Q();
	setState(LinphoneCallUpdatedByRemote,"Call updated by remote");
	if (deferUpdate) {
		if (state == LinphoneCallUpdatedByRemote)
			lInfo() << "CallSession [" << q << "]: UpdatedByRemoted was signaled but defered. LinphoneCore expects the application to call linphone_core_accept_call_update() later.";
	} else {
		if (state == LinphoneCallUpdatedByRemote)
			q->acceptUpdate(nullptr);
		else {
			/* Otherwise it means that the app responded by linphone_core_accept_call_update
			 * within the callback, so job is already done. */
		}
	}
}

void CallSessionPrivate::updating (bool isUpdate) {
	updated(isUpdate);
}

// -----------------------------------------------------------------------------

void CallSessionPrivate::init () {
	currentParams = new CallSessionParams();
	ei = linphone_error_info_new();
}

// -----------------------------------------------------------------------------

void CallSessionPrivate::accept (const CallSessionParams *csp) {
	L_Q();
	/* Try to be best-effort in giving real local or routable contact address */
	setContactOp();
	if (csp)
		setParams(new CallSessionParams(*csp));
	if (params)
		op->set_sent_custom_header(params->getPrivate()->getCustomHeaders());

	op->accept();
	if (listener)
		listener->onSetCurrentSession(q->getSharedFromThis());
	setState(LinphoneCallConnected, "Connected");
}

LinphoneStatus CallSessionPrivate::acceptUpdate (const CallSessionParams *csp, LinphoneCallState nextState, const string &stateInfo) {
	return startAcceptUpdate(nextState, stateInfo);
}

LinphoneStatus CallSessionPrivate::checkForAcceptation () const {
	L_Q();
	switch (state) {
		case LinphoneCallIncomingReceived:
		case LinphoneCallIncomingEarlyMedia:
			break;
		default:
			lError() << "checkForAcceptation() CallSession [" << q << "] is in state [" << linphone_call_state_to_string(state) << "], operation not permitted";
			return -1;
	}
	if (listener)
		listener->onCheckForAcceptation(q->getSharedFromThis());

	/* Check if this call is supposed to replace an already running one */
	SalOp *replaced = op->get_replaces();
	if (replaced) {
		CallSession *session = reinterpret_cast<CallSession *>(replaced->get_user_pointer());
		if (session) {
			lInfo() << "CallSession " << q << " replaces CallSession " << session << ". This last one is going to be terminated automatically";
			session->terminate();
		}
	}
	return 0;
}

void CallSessionPrivate::handleIncomingReceivedStateInIncomingNotification () {
	L_Q();
	/* Try to be best-effort in giving real local or routable contact address for 100Rel case */
	setContactOp();
	op->notify_ringing(false);
	if (op->get_replaces() && lp_config_get_int(linphone_core_get_config(q->getCore()->getCCore()), "sip", "auto_answer_replacing_calls", 1))
		q->accept();
}

bool CallSessionPrivate::isReadyForInvite () const {
	bool pingReady = false;
	if (pingOp) {
		if (pingReplied)
			pingReady = true;
	} else
		pingReady = true;
	return pingReady;
}

bool CallSessionPrivate::isUpdateAllowed (LinphoneCallState &nextState) const {
	switch (state) {
		case LinphoneCallIncomingReceived:
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallOutgoingRinging:
		case LinphoneCallOutgoingEarlyMedia:
			nextState = LinphoneCallEarlyUpdating;
			break;
		case LinphoneCallConnected:
		case LinphoneCallStreamsRunning:
		case LinphoneCallPausedByRemote:
		case LinphoneCallUpdatedByRemote:
			nextState = LinphoneCallUpdating;
			break;
		case LinphoneCallPaused:
			nextState = LinphoneCallPausing;
			break;
		case LinphoneCallOutgoingProgress:
		case LinphoneCallPausing:
		case LinphoneCallResuming:
		case LinphoneCallUpdating:
			nextState = state;
			break;
		default:
			lError() << "Update is not allowed in [" << linphone_call_state_to_string(state) << "] state";
			return false;
	}
	return true;
}

int CallSessionPrivate::restartInvite () {
	L_Q();
	createOp();
	return q->startInvite(nullptr, subject);
}

/*
 * Called internally when reaching the Released state, to perform cleanups to break circular references.
**/
void CallSessionPrivate::setReleased () {
	L_Q();
	if (op) {
		/* Transfer the last error so that it can be obtained even in Released state */
		if (!nonOpError)
			linphone_error_info_from_sal_op(ei, op);
		/* So that we cannot have anymore upcalls for SAL concerning this call */
		op->release();
		op = nullptr;
	}
	referer = nullptr;
	transferTarget = nullptr;
#if 0
	if (call->chat_room){
		linphone_chat_room_unref(call->chat_room);
		call->chat_room = NULL;
	}
#endif
	if (listener)
		listener->onCallSessionSetReleased(q->getSharedFromThis());
}

/* This method is called internally to get rid of a call that was notified to the application,
 * because it reached the end or error state. It performs the following tasks:
 * - remove the call from the internal list of calls
 * - update the call logs accordingly
 */
void CallSessionPrivate::setTerminated() {
	L_Q();
	completeLog();
	if (listener)
		listener->onCallSessionSetTerminated(q->getSharedFromThis());
}

LinphoneStatus CallSessionPrivate::startAcceptUpdate (LinphoneCallState nextState, const std::string &stateInfo) {
	op->accept();
	setState(nextState, stateInfo);
	return 0;
}

LinphoneStatus CallSessionPrivate::startUpdate (const string &subject) {
	L_Q();
	string newSubject(subject);
	if (newSubject.empty()) {
		if (q->getParams()->getPrivate()->getInConference())
			newSubject = "Conference";
		else if (q->getParams()->getPrivate()->getInternalCallUpdate())
			newSubject = "ICE processing concluded";
		else if (q->getParams()->getPrivate()->getNoUserConsent())
			newSubject = "Refreshing";
		else
			newSubject = "Media change";
	}
	if (destProxy && destProxy->op) {
		/* Give a chance to update the contact address if connectivity has changed */
		op->set_contact_address(destProxy->op->get_contact_address());
	} else
		op->set_contact_address(nullptr);
	return op->update(newSubject.c_str(), q->getParams()->getPrivate()->getNoUserConsent());
}

void CallSessionPrivate::terminate () {
	if ((state == LinphoneCallIncomingReceived) && (linphone_error_info_get_reason(ei) != LinphoneReasonNotAnswered)) {
		linphone_error_info_set_reason(ei, LinphoneReasonDeclined);
		nonOpError = true;
	}
	setState(LinphoneCallEnd, "Call terminated");
}

void CallSessionPrivate::updateCurrentParams () const {}

// -----------------------------------------------------------------------------

void CallSessionPrivate::setContactOp () {
	L_Q();
	SalAddress *salAddress = nullptr;
	LinphoneAddress *contact = getFixedContact();
	if (contact) {
		auto contactParams = q->getParams()->getPrivate()->getCustomContactParameters();
		for (auto it = contactParams.begin(); it != contactParams.end(); it++)
			linphone_address_set_param(contact, it->first.c_str(), it->second.empty() ? nullptr : it->second.c_str());
		salAddress = const_cast<SalAddress *>(L_GET_PRIVATE_FROM_C_OBJECT(contact)->getInternalAddress());
		op->set_contact_address(salAddress);
		linphone_address_unref(contact);
	}
}

// -----------------------------------------------------------------------------

void CallSessionPrivate::onNetworkReachable (bool reachable) {
	if (reachable) {
		repairIfBroken();
	} else {
		switch(state) {
			// For all the early states, we prefer to drop the call
			case LinphoneCallOutgoingInit:
			case LinphoneCallOutgoingProgress:
			case LinphoneCallOutgoingRinging:
			case LinphoneCallOutgoingEarlyMedia:
			case LinphoneCallIncomingReceived:
			case LinphoneCallIncomingEarlyMedia:
				// During the early states, the SAL layer reports the failure from the dialog or transaction layer,
				// hence, there is nothing special to do
			case LinphoneCallStreamsRunning:
			case LinphoneCallUpdating:
			case LinphoneCallPausing:
			case LinphoneCallResuming:
			case LinphoneCallPaused:
			case LinphoneCallPausedByRemote:
			case LinphoneCallUpdatedByRemote:
				// During these states, the dialog is established. A failure of a transaction is not expected to close it.
				// Instead we have to repair the dialog by sending a reINVITE
				broken = true;
				needLocalIpRefresh = true;
				break;
			default:
				lError() << "CallSessionPrivate::onNetworkReachable(): unimplemented case";
				break;
		}
	}
}

void CallSessionPrivate::onRegistrationStateChanged (LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const std::string &message) {
	repairIfBroken();
}

// -----------------------------------------------------------------------------

void CallSessionPrivate::completeLog () {
	L_Q();
	log->duration = computeDuration(); /* Store duration since connected */
	log->error_info = linphone_error_info_ref(ei);
	if (log->status == LinphoneCallMissed)
		q->getCore()->getCCore()->missed_calls++;
	linphone_core_report_call_log(q->getCore()->getCCore(), log);
}

void CallSessionPrivate::createOp () {
	createOpTo(log->to);
}

void CallSessionPrivate::createOpTo (const LinphoneAddress *to) {
	L_Q();
	if (op)
		op->release();
	op = new SalCallOp(q->getCore()->getCCore()->sal);
	op->set_user_pointer(q);
	if (params->getPrivate()->getReferer())
		op->set_referer(params->getPrivate()->getReferer()->getPrivate()->getOp());
	linphone_configure_op(q->getCore()->getCCore(), op, to, q->getParams()->getPrivate()->getCustomHeaders(), false);
	if (q->getParams()->getPrivacy() != LinphonePrivacyDefault)
		op->set_privacy((SalPrivacyMask)q->getParams()->getPrivacy());
	/* else privacy might be set by proxy */
}

// -----------------------------------------------------------------------------

LinphoneAddress * CallSessionPrivate::getFixedContact () const {
	L_Q();
	LinphoneAddress *result = nullptr;
	if (op && op->get_contact_address()) {
		/* If already choosed, don't change it */
		return nullptr;
	} else if (pingOp && pingOp->get_contact_address()) {
		/* If the ping OPTIONS request succeeded use the contact guessed from the received, rport */
		lInfo() << "Contact has been fixed using OPTIONS";
		char *addr = sal_address_as_string(pingOp->get_contact_address());
		result = linphone_address_new(addr);
		ms_free(addr);
	} else if (destProxy && destProxy->op && linphone_proxy_config_get_contact(destProxy)) {
		/* If using a proxy, use the contact address as guessed with the REGISTERs */
		lInfo() << "Contact has been fixed using proxy";
		result = linphone_address_clone(linphone_proxy_config_get_contact(destProxy));
	} else {
		result = linphone_core_get_primary_contact_parsed(q->getCore()->getCCore());
		if (result) {
			/* Otherwise use supplied localip */
			linphone_address_set_domain(result, nullptr /* localip */);
			linphone_address_set_port(result, -1 /* linphone_core_get_sip_port(core) */);
			lInfo() << "Contact has not been fixed, stack will do";
		}
	}
	return result;
}

// -----------------------------------------------------------------------------

void CallSessionPrivate::reinviteToRecoverFromConnectionLoss () {
	L_Q();
	lInfo() << "CallSession [" << q << "] is going to be updated (reINVITE) in order to recover from lost connectivity";
	q->update(params);
}

void CallSessionPrivate::repairByInviteWithReplaces () {
	L_Q();
	const char *callId = op->get_call_id();
	const char *fromTag = op->get_local_tag();
	const char *toTag = op->get_remote_tag();
	op->kill_dialog();
	createOp();
	op->set_replaces(callId, fromTag, toTag);
	q->startInvite(nullptr);
}

void CallSessionPrivate::repairIfBroken () {
	L_Q();
	LinphoneCore *lc = q->getCore()->getCCore();
	LinphoneConfig *config = linphone_core_get_config(lc);
	if (!lp_config_get_int(config, "sip", "repair_broken_calls", 1) || !lc->media_network_reachable || !broken)
		return;

	// If we are registered and this session has been broken due to a past network disconnection,
	// attempt to repair it

	// Make sure that the proxy from which we received this call, or to which we routed this call is registered first
	if (destProxy) {
		// In all other cases, ie no proxy config, or a proxy config for which no registration was requested,
		// we can start the call session repair immediately.
		if (linphone_proxy_config_register_enabled(destProxy)
			&& (linphone_proxy_config_get_state(destProxy) != LinphoneRegistrationOk))
			return;
	}

	SalErrorInfo sei;
	memset(&sei, 0, sizeof(sei));
	switch (state) {
		case LinphoneCallUpdating:
		case LinphoneCallPausing:
			if (op->dialog_request_pending()) {
				// Need to cancel first re-INVITE as described in section 5.5 of RFC 6141
				op->cancel_invite();
				reinviteOnCancelResponseRequested = true;
			}
			break;
		case LinphoneCallStreamsRunning:
		case LinphoneCallPaused:
		case LinphoneCallPausedByRemote:
			if (!op->dialog_request_pending())
				reinviteToRecoverFromConnectionLoss();
			break;
		case LinphoneCallUpdatedByRemote:
			if (op->dialog_request_pending()) {
				sal_error_info_set(&sei, SalReasonServiceUnavailable, "SIP", 0, nullptr, nullptr);
				op->decline_with_error_info(&sei, nullptr);
			}
			reinviteToRecoverFromConnectionLoss();
			break;
		case LinphoneCallOutgoingInit:
		case LinphoneCallOutgoingProgress:
			op->cancel_invite();
			reinviteOnCancelResponseRequested = true;
			break;
		case LinphoneCallOutgoingEarlyMedia:
		case LinphoneCallOutgoingRinging:
			repairByInviteWithReplaces();
			break;
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallIncomingReceived:
			// Keep the call broken until a forked INVITE is received from the server
			break;
		default:
			lWarning() << "CallSessionPrivate::repairIfBroken: don't know what to do in state ["
				<< linphone_call_state_to_string(state);
			broken = false;
			break;
	}
	sal_error_info_reset(&sei);
}

// =============================================================================

CallSession::CallSession (const shared_ptr<Core> &core, const CallSessionParams *params, CallSessionListener *listener)
	: Object(*new CallSessionPrivate), CoreAccessor(core) {
	L_D();
	getCore()->getPrivate()->registerListener(d);
	d->listener = listener;
	if (params)
		d->setParams(new CallSessionParams(*params));
	d->init();
	lInfo() << "New CallSession [" << this << "] initialized (LinphoneCore version: " << linphone_core_get_version() << ")";
}

CallSession::CallSession (CallSessionPrivate &p, const shared_ptr<Core> &core) : Object(p), CoreAccessor(core) {
	L_D();
	getCore()->getPrivate()->registerListener(d);
	d->init();
}

CallSession::~CallSession () {
	L_D();
	getCore()->getPrivate()->unregisterListener(d);
	if (d->currentParams)
		delete d->currentParams;
	if (d->params)
		delete d->params;
	if (d->remoteParams)
		delete d->remoteParams;
	if (d->ei)
		linphone_error_info_unref(d->ei);
	if (d->log)
		linphone_call_log_unref(d->log);
	if (d->op)
		d->op->release();
}

// -----------------------------------------------------------------------------

LinphoneStatus CallSession::accept (const CallSessionParams *csp) {
	L_D();
	LinphoneStatus result = d->checkForAcceptation();
	if (result < 0) return result;
	d->accept(csp);
	return 0;
}

LinphoneStatus CallSession::acceptUpdate (const CallSessionParams *csp) {
	L_D();
	if (d->state != LinphoneCallUpdatedByRemote) {
		lError() << "CallSession::acceptUpdate(): invalid state " << linphone_call_state_to_string(d->state) << " to call this method";
		return -1;
	}
	return d->acceptUpdate(csp, d->prevState, linphone_call_state_to_string(d->prevState));
}

void CallSession::configure (LinphoneCallDir direction, LinphoneProxyConfig *cfg, SalCallOp *op, const Address &from, const Address &to) {
	L_D();
	d->direction = direction;
	d->destProxy = cfg;
	LinphoneAddress *fromAddr = linphone_address_new(from.asString().c_str());
	LinphoneAddress *toAddr = linphone_address_new(to.asString().c_str());
	if (!d->destProxy) {
		/* Try to define the destination proxy if it has not already been done to have a correct contact field in the SIP messages */
		d->destProxy = linphone_core_lookup_known_proxy(getCore()->getCCore(), toAddr);
	}
	d->log = linphone_call_log_new(direction, fromAddr, toAddr);

	if (op) {
		/* We already have an op for incoming calls */
		d->op = op;
		d->op->set_user_pointer(this);
		op->enable_cnx_ip_to_0000_if_sendonly(!!lp_config_get_default_int(linphone_core_get_config(getCore()->getCCore()),
			"sip", "cnx_ip_to_0000_if_sendonly_enabled", 0));
		d->log->call_id = ms_strdup(op->get_call_id()); /* Must be known at that time */
	}

	if (direction == LinphoneCallOutgoing) {
		if (d->params->getPrivate()->getReferer())
			d->referer = d->params->getPrivate()->getReferer();
		d->startPing();
	} else if (direction == LinphoneCallIncoming) {
		d->setParams(new CallSessionParams());
		d->params->initDefault(getCore());
	}
}

LinphoneStatus CallSession::decline (LinphoneReason reason) {
	LinphoneErrorInfo *ei = linphone_error_info_new();
	linphone_error_info_set(ei, "SIP", reason, linphone_reason_to_error_code(reason), nullptr, nullptr);
	LinphoneStatus status = decline(ei);
	linphone_error_info_unref(ei);
	return status;
}

LinphoneStatus CallSession::decline (const LinphoneErrorInfo *ei) {
	L_D();
	SalErrorInfo sei;
	SalErrorInfo sub_sei;
	memset(&sei, 0, sizeof(sei));
	memset(&sub_sei, 0, sizeof(sub_sei));
	sei.sub_sei = &sub_sei;
	if ((d->state != LinphoneCallIncomingReceived) && (d->state != LinphoneCallIncomingEarlyMedia)) {
		lError() << "Cannot decline a CallSession that is in state " << linphone_call_state_to_string(d->state);
		return -1;
	}
	if (ei) {
		linphone_error_info_to_sal(ei, &sei);
		d->op->decline_with_error_info(&sei , nullptr);
	} else
		d->op->decline(SalReasonDeclined, nullptr);
	sal_error_info_reset(&sei);
	sal_error_info_reset(&sub_sei);
	d->terminate();
	return 0;
}

LinphoneStatus CallSession::declineNotAnswered (LinphoneReason reason) {
	L_D();
	d->log->status = LinphoneCallMissed;
	d->nonOpError = true;
	linphone_error_info_set(d->ei, nullptr, reason, linphone_reason_to_error_code(reason), "Not answered", nullptr);
	return decline(reason);
}

LinphoneStatus CallSession::deferUpdate () {
	L_D();
	if (d->state != LinphoneCallUpdatedByRemote) {
		lError() << "CallSession::deferUpdate() not done in state LinphoneCallUpdatedByRemote";
		return -1;
	}
	d->deferUpdate = true;
	return 0;
}

bool CallSession::hasTransferPending () {
	L_D();
	return d->referPending;
}

void CallSession::initiateIncoming () {}

bool CallSession::initiateOutgoing () {
	L_D();
	bool defer = false;
	d->setState(LinphoneCallOutgoingInit, "Starting outgoing call");
	d->log->start_date_time = ms_time(nullptr);
	if (!d->destProxy)
		defer = d->startPing();
 	if (d->direction == LinphoneCallOutgoing) {
		d->createOpTo(d->log->to);
	}
	return defer;
}

void CallSession::iterate (time_t currentRealTime, bool oneSecondElapsed) {
	L_D();
	int elapsed = (int)(currentRealTime - d->log->start_date_time);
	if ((d->state == LinphoneCallOutgoingInit) && (elapsed >= getCore()->getCCore()->sip_conf.delayed_timeout)) {
		/* Start the call even if the OPTIONS reply did not arrive */
		startInvite(nullptr, "");
	}
	if ((d->state == LinphoneCallIncomingReceived) || (d->state == LinphoneCallIncomingEarlyMedia)) {
		if (d->listener)
			d->listener->onIncomingCallSessionTimeoutCheck(getSharedFromThis(), elapsed, oneSecondElapsed);
	}
	if ((getCore()->getCCore()->sip_conf.in_call_timeout > 0) && (d->log->connected_date_time != 0)
		&& ((currentRealTime - d->log->connected_date_time) > getCore()->getCCore()->sip_conf.in_call_timeout)) {
		lInfo() << "In call timeout (" << getCore()->getCCore()->sip_conf.in_call_timeout << ")";
		terminate();
	}
}

LinphoneStatus CallSession::redirect (const string &redirectUri) {
	LinphoneAddress *realParsedAddr = linphone_core_interpret_url(getCore()->getCCore(), redirectUri.c_str());
	if (!realParsedAddr) {
		/* Bad url */
		lError() << "Bad redirect URI: " << redirectUri;
		return -1;
	}
	char *realParsedUri = linphone_address_as_string(realParsedAddr);
	Address redirectAddr(realParsedUri);
	bctbx_free(realParsedUri);
	linphone_address_unref(realParsedAddr);
	return redirect(redirectAddr);
}

LinphoneStatus CallSession::redirect (const Address &redirectAddr) {
	L_D();
	if (d->state != LinphoneCallIncomingReceived) {
		lError() << "Bad state for CallSession redirection";
		return -1;
	}
	SalErrorInfo sei;
	memset(&sei, 0, sizeof(sei));
	sal_error_info_set(&sei, SalReasonRedirect, "SIP", 0, nullptr, nullptr);
	d->op->decline_with_error_info(&sei, redirectAddr.getPrivate()->getInternalAddress());
	linphone_error_info_set(d->ei, nullptr, LinphoneReasonMovedPermanently, 302, "Call redirected", nullptr);
	d->nonOpError = true;
	d->terminate();
	sal_error_info_reset(&sei);
	return 0;
}

void CallSession::startIncomingNotification () {
	L_D();
	if (d->listener) {
		d->listener->onIncomingCallSessionNotified(getSharedFromThis());
		d->listener->onBackgroundTaskToBeStarted(getSharedFromThis());
	}
	/* Prevent the CallSession from being destroyed while we are notifying, if the user declines within the state callback */
	shared_ptr<CallSession> ref = getSharedFromThis();
	if (d->deferIncomingNotification) {
		lInfo() << "Defer incoming notification";
		return;
	}

	d->startIncomingNotification();
}

int CallSession::startInvite (const Address *destination, const string &subject, const Content *content) {
	L_D();
	d->subject = subject;
	/* Try to be best-effort in giving real local or routable contact address */
	d->setContactOp();
	string destinationStr;
	char *realUrl = nullptr;
	if (destination)
		destinationStr = destination->asString();
	else {
		realUrl = linphone_address_as_string(d->log->to);
		destinationStr = realUrl;
		ms_free(realUrl);
	}
	char *from = linphone_address_as_string(d->log->from);
	/* Take a ref because sal_call() may destroy the CallSession if no SIP transport is available */
	shared_ptr<CallSession> ref = getSharedFromThis();
	if (content)
		d->op->set_local_body(*content);
	int result = d->op->call(from, destinationStr.c_str(), subject.empty() ? nullptr : subject.c_str());
	ms_free(from);
	if (result < 0) {
		if ((d->state != LinphoneCallError) && (d->state != LinphoneCallReleased)) {
			/* sal_call() may invoke call_failure() and call_released() SAL callbacks synchronously,
			   in which case there is no need to perform a state change here. */
			d->setState(LinphoneCallError, "Call failed");
		}
	} else {
		d->log->call_id = ms_strdup(d->op->get_call_id()); /* Must be known at that time */
		d->setState(LinphoneCallOutgoingProgress, "Outgoing call in progress");
	}
	return result;
}

LinphoneStatus CallSession::terminate (const LinphoneErrorInfo *ei) {
	L_D();
	lInfo() << "Terminate CallSession [" << this << "] which is currently in state [" << linphone_call_state_to_string(d->state) << "]";
	SalErrorInfo sei;
	memset(&sei, 0, sizeof(sei));
	switch (d->state) {
		case LinphoneCallReleased:
		case LinphoneCallEnd:
		case LinphoneCallError:
			lWarning() << "No need to terminate CallSession [" << this << "] in state [" << linphone_call_state_to_string(d->state) << "]";
			return -1;
		case LinphoneCallIncomingReceived:
		case LinphoneCallIncomingEarlyMedia:
			return decline(ei);
		case LinphoneCallOutgoingInit:
			/* In state OutgoingInit, op has to be destroyed */
			d->op->release();
			d->op = nullptr;
			break;
		default:
			if (ei) {
				linphone_error_info_to_sal(ei, &sei);
				d->op->terminate_with_error(&sei);
				sal_error_info_reset(&sei);
			} else
				d->op->terminate();
			break;
	}

	d->terminate();
	return 0;
}

LinphoneStatus CallSession::transfer (const shared_ptr<CallSession> &dest) {
	L_D();
	int result = d->op->refer_with_replaces(dest->getPrivate()->op);
	d->setTransferState(LinphoneCallOutgoingInit);
	return result;
}

LinphoneStatus CallSession::transfer (const string &dest) {
	L_D();
	LinphoneAddress *destAddr = linphone_core_interpret_url(getCore()->getCCore(), dest.c_str());
	if (!destAddr)
		return -1;
	char *addrStr = linphone_address_as_string(destAddr);
	d->op->refer(addrStr);
	bctbx_free(addrStr);
	linphone_address_unref(destAddr);
	d->setTransferState(LinphoneCallOutgoingInit);
	return 0;
}

LinphoneStatus CallSession::update (const CallSessionParams *csp, const string &subject, const Content *content) {
	L_D();
	LinphoneCallState nextState;
	LinphoneCallState initialState = d->state;
	if (!d->isUpdateAllowed(nextState))
		return -1;
	if (d->currentParams == csp)
		lWarning() << "CallSession::update() is given the current params, this is probably not what you intend to do!";
	if (csp)
		d->setParams(new CallSessionParams(*csp));
	d->op->set_local_body(content ? *content : Content());
	LinphoneStatus result = d->startUpdate(subject);
	if (result && (d->state != initialState)) {
		/* Restore initial state */
		d->setState(initialState, "Restore initial state");
	}
	return result;
}

// -----------------------------------------------------------------------------

LinphoneCallDir CallSession::getDirection () const {
	L_D();
	return d->direction;
}

const Address& CallSession::getDiversionAddress () const {
	L_D();
	if (d->op) {
		char *addrStr = sal_address_as_string(d->op->get_diversion_address());
		d->diversionAddress = Address(addrStr);
		bctbx_free(addrStr);
	} else {
		d->diversionAddress = Address();
	}
	return d->diversionAddress;
}

int CallSession::getDuration () const {
	L_D();
	switch (d->state) {
		case LinphoneCallEnd:
		case LinphoneCallError:
		case LinphoneCallReleased:
			return d->log->duration;
		default:
			return d->computeDuration();
	}
}

const LinphoneErrorInfo * CallSession::getErrorInfo () const {
	L_D();
	if (!d->nonOpError)
		linphone_error_info_from_sal_op(d->ei, d->op);
	return d->ei;
}

LinphoneCallLog * CallSession::getLog () const {
	L_D();
	return d->log;
}

LinphoneReason CallSession::getReason () const {
	return linphone_error_info_get_reason(getErrorInfo());
}

shared_ptr<CallSession> CallSession::getReferer () const {
	L_D();
	return d->referer;
}

string CallSession::getReferTo () const {
	L_D();
	return d->referTo;
}

const Address& CallSession::getRemoteAddress () const {
	L_D();
	return *L_GET_CPP_PTR_FROM_C_OBJECT((d->direction == LinphoneCallIncoming)
		? linphone_call_log_get_from(d->log) : linphone_call_log_get_to(d->log));
}

string CallSession::getRemoteAddressAsString () const {
	return getRemoteAddress().asString();
}

string CallSession::getRemoteContact () const {
	L_D();
	if (d->op) {
		/* sal_op_get_remote_contact preserves header params */
		return d->op->get_remote_contact();
	}
	return string();
}

const Address *CallSession::getRemoteContactAddress () const {
	L_D();
	if (!d->op) {
		return nullptr;
	}
	char *addrStr = sal_address_as_string(d->op->get_remote_contact_address());
	d->remoteContactAddress = Address(addrStr);
	bctbx_free(addrStr);
	return &d->remoteContactAddress;
}

const CallSessionParams * CallSession::getRemoteParams () {
	L_D();
	if (d->op){
		const SalCustomHeader *ch = d->op->get_recv_custom_header();
		if (ch) {
			/* Instanciate a remote_params only if a SIP message was received before (custom headers indicates this) */
			if (!d->remoteParams)
				d->remoteParams = new CallSessionParams();
			d->remoteParams->getPrivate()->setCustomHeaders(ch);
		}
		return d->remoteParams;
	}
	return nullptr;
}

LinphoneCallState CallSession::getState () const {
	L_D();
	return d->state;
}

const Address& CallSession::getToAddress () const {
	L_D();
	d->toAddress = Address(d->op->get_to());
	return d->toAddress;
}

LinphoneCallState CallSession::getTransferState () const {
	L_D();
	return d->transferState;
}

shared_ptr<CallSession> CallSession::getTransferTarget () const {
	L_D();
	return d->transferTarget;
}

string CallSession::getToHeader (const string &name) const {
	L_D();
	return L_C_TO_STRING(sal_custom_header_find(d->op->get_recv_custom_header(), name.c_str()));
}

// -----------------------------------------------------------------------------

string CallSession::getRemoteUserAgent () const {
	L_D();
	if (d->op && d->op->get_remote_ua())
		return d->op->get_remote_ua();
	return string();
}

shared_ptr<CallSession> CallSession::getReplacedCallSession () const {
	L_D();
	SalOp *replacedOp = d->op->get_replaces();
	if (!replacedOp)
		return nullptr;
	return reinterpret_cast<CallSession *>(replacedOp->get_user_pointer())->getSharedFromThis();
}

CallSessionParams * CallSession::getCurrentParams () const {
	L_D();
	d->updateCurrentParams();
	return d->currentParams;
}

// -----------------------------------------------------------------------------

const CallSessionParams * CallSession::getParams () const {
	L_D();
	return d->params;
}

LINPHONE_END_NAMESPACE
