/*
 * call-session.cpp
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <bctoolbox/defs.h>

#include "c-wrapper/c-wrapper.h"

#include "address/address-p.h"
#include "conference/session/call-session-p.h"
#include "call/call-p.h"
#include "conference/params/call-session-params-p.h"

#include "conference/session/call-session.h"

#include "logger/logger.h"

#include "linphone/core.h"

#include "private.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

CallSessionPrivate::CallSessionPrivate (const Conference &conference, const CallSessionParams *params, CallSessionListener *listener)
	: conference(conference), listener(listener) {
	if (params)
		this->params = new CallSessionParams(*params);
	currentParams = new CallSessionParams();
	core = conference.getCore();
	ei = linphone_error_info_new();
}

CallSessionPrivate::~CallSessionPrivate () {
	if (currentParams)
		delete currentParams;
	if (params)
		delete params;
	if (remoteParams)
		delete remoteParams;
	if (ei)
		linphone_error_info_unref(ei);
	if (log)
		linphone_call_log_unref(log);
	if (op)
		sal_op_release(op);
}

// -----------------------------------------------------------------------------

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
	currentParams->setPrivacy((LinphonePrivacyMask)sal_op_get_privacy(op));
}

void CallSessionPrivate::setState(LinphoneCallState newState, const string &message) {
	L_Q(CallSession);
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
#ifdef __ANDROID__
				lInfo() << "CallSession [" << q << "] acquires both wifi and multicast lock";
				linphone_core_wifi_lock_acquire(core);
				linphone_core_multicast_lock_acquire(core); /* Does no affect battery more than regular rtp traffic */
#endif
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
#ifdef __ANDROID__
				lInfo() << "CallSession [" << q << "] releases wifi/multicast lock";
				linphone_core_wifi_lock_release(core);
				linphone_core_multicast_lock_release(core);
#endif
				break;
			default:
				break;
		}

		if (newState != LinphoneCallStreamsRunning) {
#if 0 // TODO
			if (call->dtmfs_timer!=NULL){
				/*cancelling DTMF sequence, if any*/
				linphone_call_cancel_dtmfs(call);
			}
#endif
		}
		if (message.empty()) {
			lError() << "You must fill a reason when changing call state (from " <<
				linphone_call_state_to_string(prevState) << " to " << linphone_call_state_to_string(state) << ")";
		}
		if (listener)
			listener->onCallSessionStateChanged(*q, state, message);
		if (newState == LinphoneCallReleased)
			setReleased(); /* Shall be performed after app notification */
	}
}

bool CallSessionPrivate::startPing () {
	if (core->sip_conf.ping_with_options) {
		/* Defer the start of the call after the OPTIONS ping for outgoing call or
		 * send an option request back to the caller so that we get a chance to discover our nat'd address
		 * before answering for incoming call */
		pingReplied = false;
		pingOp = sal_op_new(core->sal);
		if (direction == LinphoneCallIncoming) {
			const char *from = sal_op_get_from(pingOp);
			const char *to = sal_op_get_to(pingOp);
			linphone_configure_op(core, pingOp, log->from, nullptr, false);
			sal_op_set_route(pingOp, sal_op_get_network_origin(op));
			sal_ping(pingOp, from, to);
		} else if (direction == LinphoneCallOutgoing) {
			char *from = linphone_address_as_string(log->from);
			char *to = linphone_address_as_string(log->to);
			sal_ping(pingOp, from, to);
			ms_free(from);
			ms_free(to);
		}
		sal_op_set_user_pointer(pingOp, this);
		return true;
	}
	return false;
}

// -----------------------------------------------------------------------------

void CallSessionPrivate::abort (const string &errorMsg) {
	sal_call_terminate(op);
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
	currentParams->setPrivacy((LinphonePrivacyMask)sal_op_get_privacy(op));
}

void CallSessionPrivate::ackBeingSent (LinphoneHeaders *headers) {
	L_Q(CallSession);
	if (listener)
		listener->onAckBeingSent(*q, headers);
}

void CallSessionPrivate::ackReceived (LinphoneHeaders *headers) {
	L_Q(CallSession);
	if (listener)
		listener->onAckReceived(*q, headers);
}

bool CallSessionPrivate::failure () {
	L_Q(CallSession);
	const SalErrorInfo *ei = sal_op_get_error_info(op);
	switch (ei->reason) {
		case SalReasonRedirect:
			if ((state == LinphoneCallOutgoingInit) || (state == LinphoneCallOutgoingProgress)
				|| (state == LinphoneCallOutgoingRinging) /* Push notification case */ || (state == LinphoneCallOutgoingEarlyMedia)) {
				const SalAddress *redirectionTo = sal_op_get_remote_contact_address(op);
				if (redirectionTo) {
					char *url = sal_address_as_string(redirectionTo);
					lWarning() << "Redirecting CallSession [" << q << "] to " << url;
					if (log->to)
						linphone_address_unref(log->to);
					log->to = linphone_address_new(url);
					ms_free(url);
#if 0
					linphone_call_restart_invite(call);
#endif
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
				setState(LinphoneCallError, ei->full_string);
			else
				setState(LinphoneCallEnd, ei->full_string);
		}
#if 0 // TODO: handle in Call class
		if (ei->reason != SalReasonNone)
			linphone_core_play_call_error_tone(core, linphone_reason_from_sal(ei->reason));
#endif
	}
#if 0
	LinphoneCall *referer=call->referer;
	if (referer){
		/*notify referer of the failure*/
		linphone_core_notify_refer_state(lc,referer,call);
		/*schedule automatic resume of the call. This must be done only after the notifications are completed due to dialog serialization of requests.*/
		linphone_core_queue_task(lc,(belle_sip_source_func_t)resume_call_after_failed_transfer,linphone_call_ref(referer),"Automatic call resuming after failed transfer");
	}
#endif
	return false;
}

void CallSessionPrivate::pingReply () {
	L_Q(CallSession);
	if (state == LinphoneCallOutgoingInit) {
		pingReplied = true;
		if (isReadyForInvite())
			q->startInvite(nullptr);
	}
}

void CallSessionPrivate::remoteRinging () {
	L_Q(CallSession);
	/* Set privacy */
	q->getCurrentParams()->setPrivacy((LinphonePrivacyMask)sal_op_get_privacy(op));
#if 0
	if (lc->ringstream == NULL) start_remote_ring(lc, call);
#endif
	lInfo() << "Remote ringing...";
	setState(LinphoneCallOutgoingRinging, "Remote ringing");
}

void CallSessionPrivate::terminated () {
	switch (state) {
		case LinphoneCallEnd:
		case LinphoneCallError:
			lWarning() << "terminated: already terminated, ignoring";
			return;
		case LinphoneCallIncomingReceived:
		case LinphoneCallIncomingEarlyMedia:
			if (!sal_op_get_reason_error_info(op)->protocol || strcmp(sal_op_get_reason_error_info(op)->protocol, "") == 0) {
				linphone_error_info_set(ei, nullptr, LinphoneReasonNotAnswered, 0, "Incoming call cancelled", nullptr);
				nonOpError = true;
			}
			break;
		default:
			break;
	}
#if 0
	if (call->refer_pending)
		linphone_core_start_refered_call(lc,call,NULL);
	//we stop the call only if we have this current call or if we are in call
	if ((bctbx_list_size(lc->calls)  == 1) || linphone_core_in_call(lc)) {
		linphone_core_stop_ringing(lc);
	}
#endif
	setState(LinphoneCallEnd, "Call ended");
}

void CallSessionPrivate::updated (bool isUpdate) {
	deferUpdate = lp_config_get_int(linphone_core_get_config(core), "sip", "defer_update_default", FALSE);
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
			sal_call_decline_with_error_info(op, &sei, nullptr);
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
	L_Q(CallSession);
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

void CallSessionPrivate::accept (const CallSessionParams *params) {
	L_Q(CallSession);
	/* Try to be best-effort in giving real local or routable contact address */
	setContactOp();
	if (params) {
		this->params = new CallSessionParams(*params);
		sal_op_set_sent_custom_header(op, this->params->getPrivate()->getCustomHeaders());
	}

	sal_call_accept(op);
	if (listener)
		listener->onSetCurrentSession(*q);
	setState(LinphoneCallConnected, "Connected");
}

LinphoneStatus CallSessionPrivate::acceptUpdate (const CallSessionParams *csp, LinphoneCallState nextState, const string &stateInfo) {
	return startAcceptUpdate(nextState, stateInfo);
}

LinphoneStatus CallSessionPrivate::checkForAcceptation () const {
	L_Q(const CallSession);
	switch (state) {
		case LinphoneCallIncomingReceived:
		case LinphoneCallIncomingEarlyMedia:
			break;
		default:
			lError() << "checkForAcceptation() CallSession [" << q << "] is in state [" << linphone_call_state_to_string(state) << "], operation not permitted";
			return -1;
	}
	if (listener)
		listener->onCheckForAcceptation(*q);

	/* Check if this call is supposed to replace an already running one */
	SalOp *replaced = sal_call_get_replaces(op);
	if (replaced) {
		CallSession *session = reinterpret_cast<CallSession *>(sal_op_get_user_pointer(replaced));
		if (session) {
			lInfo() << "CallSession " << q << " replaces CallSession " << session << ". This last one is going to be terminated automatically";
			session->terminate();
		}
	}
	return 0;
}

void CallSessionPrivate::handleIncomingReceivedStateInIncomingNotification () {
	L_Q(CallSession);
	/* Try to be best-effort in giving real local or routable contact address for 100Rel case */
	setContactOp();
	sal_call_notify_ringing(op, false);
	if (sal_call_get_replaces(op) && lp_config_get_int(linphone_core_get_config(core), "sip", "auto_answer_replacing_calls", 1))
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

/*
 * Called internally when reaching the Released state, to perform cleanups to break circular references.
**/
void CallSessionPrivate::setReleased () {
	L_Q(CallSession);
	if (op) {
		/* Transfer the last error so that it can be obtained even in Released state */
		if (!nonOpError)
			linphone_error_info_from_sal_op(ei, op);
		/* So that we cannot have anymore upcalls for SAL concerning this call */
		sal_op_release(op);
		op = nullptr;
	}
#if 0
	/* It is necessary to reset pointers to other call to prevent circular references that would result in memory never freed */
	if (call->referer){
		linphone_call_unref(call->referer);
		call->referer=NULL;
	}
	if (call->transfer_target){
		linphone_call_unref(call->transfer_target);
		call->transfer_target=NULL;
	}
	if (call->chat_room){
		linphone_chat_room_unref(call->chat_room);
		call->chat_room = NULL;
	}
#endif
	if (listener)
		listener->onCallSessionSetReleased(*q);
}

/* This method is called internally to get rid of a call that was notified to the application,
 * because it reached the end or error state. It performs the following tasks:
 * - remove the call from the internal list of calls
 * - update the call logs accordingly
 */
void CallSessionPrivate::setTerminated() {
	L_Q(CallSession);
	completeLog();
	if (listener)
		listener->onCallSessionSetTerminated(*q);
}

LinphoneStatus CallSessionPrivate::startAcceptUpdate (LinphoneCallState nextState, const std::string &stateInfo) {
	sal_call_accept(op);
	setState(nextState, stateInfo);
	return 0;
}

LinphoneStatus CallSessionPrivate::startUpdate () {
	L_Q(CallSession);
	string subject;
	if (q->getParams()->getPrivate()->getInConference())
		subject = "Conference";
	else if (q->getParams()->getPrivate()->getInternalCallUpdate())
		subject = "ICE processing concluded";
	else if (q->getParams()->getPrivate()->getNoUserConsent())
		subject = "Refreshing";
	else
		subject = "Media change";
	if (destProxy && destProxy->op) {
		/* Give a chance to update the contact address if connectivity has changed */
		sal_op_set_contact_address(op, sal_op_get_contact_address(destProxy->op));
	} else
		sal_op_set_contact_address(op, nullptr);
	return sal_call_update(op, subject.c_str(), q->getParams()->getPrivate()->getNoUserConsent());
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
	SalAddress *salAddress = nullptr;
	LinphoneAddress *contact = getFixedContact();
	if (contact) {
		salAddress = const_cast<SalAddress *>(L_GET_PRIVATE_FROM_C_OBJECT(contact)->getInternalAddress());
		sal_address_ref(salAddress);
		linphone_address_unref(contact);
	}
	sal_op_set_and_clean_contact_address(op, salAddress);
}

// -----------------------------------------------------------------------------

void CallSessionPrivate::completeLog () {
	log->duration = computeDuration(); /* Store duration since connected */
	log->error_info = linphone_error_info_ref(ei);
	if (log->status == LinphoneCallMissed)
		core->missed_calls++;
	linphone_core_report_call_log(core, log);
}

void CallSessionPrivate::createOpTo (const LinphoneAddress *to) {
	L_Q(CallSession);
	if (op)
		sal_op_release(op);
	op = sal_op_new(core->sal);
	sal_op_set_user_pointer(op, q);
#if 0
	if (linphone_call_params_get_referer(call->params))
		sal_call_set_referer(call->op,linphone_call_params_get_referer(call->params)->op);
#endif
	linphone_configure_op(core, op, to, q->getParams()->getPrivate()->getCustomHeaders(), false);
	if (q->getParams()->getPrivacy() != LinphonePrivacyDefault)
		sal_op_set_privacy(op, (SalPrivacyMask)q->getParams()->getPrivacy());
	/* else privacy might be set by proxy */
}

// -----------------------------------------------------------------------------

LinphoneAddress * CallSessionPrivate::getFixedContact () const {
	LinphoneAddress *result = nullptr;
	if (op && sal_op_get_contact_address(op)) {
		/* If already choosed, don't change it */
		return nullptr;
	} else if (pingOp && sal_op_get_contact_address(pingOp)) {
		/* If the ping OPTIONS request succeeded use the contact guessed from the received, rport */
		lInfo() << "Contact has been fixed using OPTIONS";
		char *addr = sal_address_as_string(sal_op_get_contact_address(pingOp));
		result = linphone_address_new(addr);
		ms_free(addr);
	} else if (destProxy && destProxy->op && sal_op_get_contact_address(destProxy->op)) {
		/* If using a proxy, use the contact address as guessed with the REGISTERs */
		lInfo() << "Contact has been fixed using proxy";
		char *addr = sal_address_as_string(sal_op_get_contact_address(destProxy->op));
		result = linphone_address_new(addr);
		ms_free(addr);
	} else {
		result = linphone_core_get_primary_contact_parsed(core);
		if (result) {
			/* Otherwise use supplied localip */
			linphone_address_set_domain(result, nullptr /* localip */);
			linphone_address_set_port(result, -1 /* linphone_core_get_sip_port(core) */);
			lInfo() << "Contact has not been fixed, stack will do";
		}
	}
	return result;
}

// =============================================================================

CallSession::CallSession (const Conference &conference, const CallSessionParams *params, CallSessionListener *listener)
	: Object(*new CallSessionPrivate(conference, params, listener)) {
	lInfo() << "New CallSession [" << this << "] initialized (LinphoneCore version: " << linphone_core_get_version() << ")";
}

CallSession::CallSession (CallSessionPrivate &p) : Object(p) {}

// -----------------------------------------------------------------------------

LinphoneStatus CallSession::accept (const CallSessionParams *csp) {
	L_D(CallSession);
	LinphoneStatus result = d->checkForAcceptation();
	if (result < 0) return result;
	d->accept(csp);
	return 0;
}

LinphoneStatus CallSession::acceptUpdate (const CallSessionParams *csp) {
	L_D(CallSession);
	if (d->state != LinphoneCallUpdatedByRemote) {
		lError() << "CallSession::acceptUpdate(): invalid state " << linphone_call_state_to_string(d->state) << " to call this method";
		return -1;
	}
	return d->acceptUpdate(csp, d->prevState, linphone_call_state_to_string(d->prevState));
}

void CallSession::configure (LinphoneCallDir direction, LinphoneProxyConfig *cfg, SalOp *op, const Address &from, const Address &to) {
	L_D(CallSession);
	d->direction = direction;
	d->destProxy = cfg;
	LinphoneAddress *fromAddr = linphone_address_new(from.asString().c_str());
	LinphoneAddress *toAddr = linphone_address_new(to.asString().c_str());
	if (!d->destProxy) {
		/* Try to define the destination proxy if it has not already been done to have a correct contact field in the SIP messages */
		d->destProxy = linphone_core_lookup_known_proxy(d->core, toAddr);
	}
	d->log = linphone_call_log_new(direction, fromAddr, toAddr);

	if (op) {
		/* We already have an op for incoming calls */
		d->op = op;
		sal_op_set_user_pointer(d->op, this);
		sal_op_cnx_ip_to_0000_if_sendonly_enable(op, !!lp_config_get_default_int(linphone_core_get_config(d->core),
			"sip", "cnx_ip_to_0000_if_sendonly_enabled", 0));
		d->log->call_id = ms_strdup(sal_op_get_call_id(op)); /* Must be known at that time */
	}

	if (direction == LinphoneCallOutgoing) {
		d->startPing();
	} else if (direction == LinphoneCallIncoming) {
		d->params = new CallSessionParams();
		d->params->initDefault(d->core);
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
	L_D(CallSession);
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
		sal_call_decline_with_error_info(d->op, &sei , nullptr);
	} else
		sal_call_decline(d->op, SalReasonDeclined, nullptr);
	sal_error_info_reset(&sei);
	sal_error_info_reset(&sub_sei);
	d->terminate();
	return 0;
}

void CallSession::initiateIncoming () {}

bool CallSession::initiateOutgoing () {
	L_D(CallSession);
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
	L_D(CallSession);
	int elapsed = (int)(currentRealTime - d->log->start_date_time);
	if ((d->state == LinphoneCallOutgoingInit) && (elapsed >= d->core->sip_conf.delayed_timeout)) {
		/* Start the call even if the OPTIONS reply did not arrive */
		startInvite(nullptr);
	}
	if ((d->state == LinphoneCallIncomingReceived) || (d->state == LinphoneCallIncomingEarlyMedia)) {
		if (oneSecondElapsed)
			lInfo() << "Incoming call ringing for " << elapsed << " seconds";
		if (elapsed > d->core->sip_conf.inc_timeout) {
			lInfo() << "Incoming call timeout (" << d->core->sip_conf.inc_timeout << ")";
#if 0
			LinphoneReason declineReason = (core->current_call != call) ? LinphoneReasonBusy : LinphoneReasonDeclined;
#endif
			d->log->status = LinphoneCallMissed;
#if 0
			call->non_op_error = TRUE;
			linphone_error_info_set(call->ei, NULL, decline_reason, linphone_reason_to_error_code(decline_reason), "Not answered", NULL);
			linphone_call_decline(call, decline_reason);
#endif
		}
	}
	if ((d->core->sip_conf.in_call_timeout > 0) && (d->log->connected_date_time != 0)
		&& ((currentRealTime - d->log->connected_date_time) > d->core->sip_conf.in_call_timeout)) {
		lInfo() << "In call timeout (" << d->core->sip_conf.in_call_timeout << ")";
		terminate();
	}
}

void CallSession::startIncomingNotification () {
	L_D(CallSession);
	if (d->listener)
		d->listener->onCallSessionAccepted(*this);
	/* Prevent the CallSession from being destroyed while we are notifying, if the user declines within the state callback */
	shared_ptr<CallSession> ref = static_pointer_cast<CallSession>(shared_from_this());
#if 0
	call->bg_task_id=sal_begin_background_task("liblinphone call notification", NULL, NULL);
#endif
	if (d->deferIncomingNotification) {
		lInfo() << "Defer incoming notification";
		return;
	}

	if (d->listener)
		d->listener->onIncomingCallSessionStarted(*this);

	d->setState(LinphoneCallIncomingReceived, "Incoming CallSession");

	/* From now on, the application is aware of the call and supposed to take background task or already submitted notification to the user.
	 * We can then drop our background task. */
#if 0
	if (call->bg_task_id!=0) {
		sal_end_background_task(call->bg_task_id);
		call->bg_task_id=0;
	}
#endif

	if (d->state == LinphoneCallIncomingReceived) {
		d->handleIncomingReceivedStateInIncomingNotification();
	}
}

int CallSession::startInvite (const Address *destination) {
	L_D(CallSession);
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
	shared_ptr<CallSession> ref = static_pointer_cast<CallSession>(shared_from_this());
	int result = sal_call(d->op, from, destinationStr.c_str());
	ms_free(from);
	if (result < 0) {
		if ((d->state != LinphoneCallError) && (d->state != LinphoneCallReleased)) {
			/* sal_call() may invoke call_failure() and call_released() SAL callbacks synchronously,
			   in which case there is no need to perform a state change here. */
			d->setState(LinphoneCallError, "Call failed");
		}
	} else {
		d->log->call_id = ms_strdup(sal_op_get_call_id(d->op)); /* Must be known at that time */
		d->setState(LinphoneCallOutgoingProgress, "Outgoing call in progress");
	}
	return result;
}

LinphoneStatus CallSession::terminate (const LinphoneErrorInfo *ei) {
	L_D(CallSession);
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
			sal_op_release(d->op);
			d->op = nullptr;
			break;
		default:
			if (ei) {
				linphone_error_info_to_sal(ei, &sei);
				sal_call_terminate_with_error(d->op, &sei);
				sal_error_info_reset(&sei);
			} else
				sal_call_terminate(d->op);
			break;
	}

	d->terminate();
	return 0;
}

LinphoneStatus CallSession::update (const CallSessionParams *csp) {
	L_D(CallSession);
	LinphoneCallState nextState;
	LinphoneCallState initialState = d->state;
	if (!d->isUpdateAllowed(nextState))
		return -1;
	if (d->currentParams == csp)
		lWarning() << "CallSession::update() is given the current params, this is probably not what you intend to do!";
	LinphoneStatus result = d->startUpdate();
	if (result && (d->state != initialState)) {
		/* Restore initial state */
		d->setState(initialState, "Restore initial state");
	}
	return result;
}

// -----------------------------------------------------------------------------

LinphoneCallDir CallSession::getDirection () const {
	L_D(const CallSession);
	return d->direction;
}

int CallSession::getDuration () const {
	L_D(const CallSession);
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
	L_D(const CallSession);
	if (!d->nonOpError)
		linphone_error_info_from_sal_op(d->ei, d->op);
	return d->ei;
}

LinphoneCallLog * CallSession::getLog () const {
	L_D(const CallSession);
	return d->log;
}

LinphoneReason CallSession::getReason () const {
	return linphone_error_info_get_reason(getErrorInfo());
}

const Address& CallSession::getRemoteAddress () const {
	L_D(const CallSession);
	return *L_GET_CPP_PTR_FROM_C_OBJECT((d->direction == LinphoneCallIncoming)
		? linphone_call_log_get_from(d->log) : linphone_call_log_get_to(d->log));
}

string CallSession::getRemoteAddressAsString () const {
	return getRemoteAddress().asString();
}

string CallSession::getRemoteContact () const {
	L_D(const CallSession);
	if (d->op) {
		/* sal_op_get_remote_contact preserves header params */
		return sal_op_get_remote_contact(d->op);
	}
	return string();
}

const CallSessionParams * CallSession::getRemoteParams () {
	L_D(CallSession);
	if (d->op){
		const SalCustomHeader *ch = sal_op_get_recv_custom_header(d->op);
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
	L_D(const CallSession);
	return d->state;
}

// -----------------------------------------------------------------------------

string CallSession::getRemoteUserAgent () const {
	L_D(const CallSession);
	if (d->op)
		return sal_op_get_remote_ua(d->op);
	return string();
}

CallSessionParams * CallSession::getCurrentParams () const {
	L_D(const CallSession);
	d->updateCurrentParams();
	return d->currentParams;
}

// -----------------------------------------------------------------------------

const CallSessionParams * CallSession::getParams () const {
	L_D(const CallSession);
	return d->params;
}

LINPHONE_END_NAMESPACE
