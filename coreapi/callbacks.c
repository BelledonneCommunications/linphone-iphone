/*
linphone
Copyright (C) 2010  Simon MORLAT (simon.morlat@free.fr)

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


#include "c-wrapper/internal/c-sal.h"
#include "sal/call-op.h"
#include "sal/message-op.h"
#include "sal/refer-op.h"

#include "linphone/core.h"
#include "linphone/utils/utils.h"
#include "private.h"
#include "mediastreamer2/mediastream.h"
#include "linphone/lpconfig.h"
#include <bctoolbox/defs.h>

// stat
#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "c-wrapper/c-wrapper.h"
#include "call/call-p.h"
#include "chat/chat-message/chat-message-p.h"
#include "chat/chat-room/chat-room.h"
#include "chat/chat-room/server-group-chat-room-p.h"
#include "conference/participant.h"
#include "conference/session/call-session-p.h"
#include "conference/session/call-session.h"
#include "conference/session/media-session-p.h"
#include "conference/session/media-session.h"
#include "core/core-p.h"

using namespace std;

using namespace LinphonePrivate;

static void register_failure(SalOp *op);

static void call_received(SalCallOp *h) {
	/* Look if this INVITE is for a call that has already been notified but broken because of network failure */
	LinphoneCore *lc = reinterpret_cast<LinphoneCore *>(h->get_sal()->get_user_pointer());
	if (L_GET_PRIVATE_FROM_C_OBJECT(lc)->inviteReplacesABrokenCall(h))
		return;

	LinphoneAddress *fromAddr = nullptr;
	const char *pAssertedId = sal_custom_header_find(h->get_recv_custom_header(), "P-Asserted-Identity");
	/* In some situation, better to trust the network rather than the UAC */
	if (lp_config_get_int(linphone_core_get_config(lc), "sip", "call_logs_use_asserted_id_instead_of_from", 0)) {
		if (pAssertedId) {
			LinphoneAddress *pAssertedIdAddr = linphone_address_new(pAssertedId);
			if (pAssertedIdAddr) {
				ms_message("Using P-Asserted-Identity [%s] instead of from [%s] for op [%p]", pAssertedId, h->get_from(), h);
				fromAddr = pAssertedIdAddr;
			} else
				ms_warning("Unsupported P-Asserted-Identity header for op [%p] ", h);
		} else
			ms_warning("No P-Asserted-Identity header found so cannot use it for op [%p] instead of from", h);
	}

	if (!fromAddr)
		fromAddr = linphone_address_new(h->get_from());
	LinphoneAddress *toAddr = linphone_address_new(h->get_to());

	if (_linphone_core_is_conference_creation(lc, toAddr)) {
		linphone_address_unref(toAddr);
		linphone_address_unref(fromAddr);
		if (sal_address_has_param(h->get_remote_contact_address(), "text")) {
			bool oneToOneChatRoom = false;
			const char *oneToOneChatRoomStr = sal_custom_header_find(h->get_recv_custom_header(), "One-To-One-Chat-Room");
			if (oneToOneChatRoomStr && (strcmp(oneToOneChatRoomStr, "true") == 0))
				oneToOneChatRoom = true;
			if (oneToOneChatRoom) {
				IdentityAddress from(h->get_from());
				list<IdentityAddress> identAddresses = ServerGroupChatRoom::parseResourceLists(h->get_remote_body());
				if (identAddresses.size() != 1) {
					h->decline(SalReasonNotAcceptable, nullptr);
					return;
				}
				IdentityAddress confAddr = L_GET_PRIVATE_FROM_C_OBJECT(lc)->mainDb->findOneToOneConferenceChatRoomAddress(from, identAddresses.front());
				if (confAddr.isValid()) {
					shared_ptr<AbstractChatRoom> chatRoom = L_GET_CPP_PTR_FROM_C_OBJECT(lc)->findChatRoom(ChatRoomId(confAddr, confAddr));
					L_GET_PRIVATE(static_pointer_cast<ServerGroupChatRoom>(chatRoom))->confirmRecreation(h);
					return;
				}
			}
			_linphone_core_create_server_group_chat_room(lc, h);
		}
		// TODO: handle media conference creation if the "text" feature tag is not present
		return;
	} else if (sal_address_has_param(h->get_remote_contact_address(), "text")) {
		shared_ptr<AbstractChatRoom> chatRoom = L_GET_CPP_PTR_FROM_C_OBJECT(lc)->findChatRoom(
			ChatRoomId(IdentityAddress(h->get_to()), IdentityAddress(h->get_to()))
		);
		if (chatRoom) {
			L_GET_PRIVATE(static_pointer_cast<ServerGroupChatRoom>(chatRoom))->confirmJoining(h);
			linphone_address_unref(toAddr);
			linphone_address_unref(fromAddr);
		} else {
			//invite is for an unknown chatroom
			h->decline(SalReasonNotFound, nullptr);
		}
		return;
	} else {
		// TODO: handle media conference joining if the "text" feature tag is not present
	}

	/* First check if we can answer successfully to this invite */
	LinphonePresenceActivity *activity = nullptr;
	if ((linphone_presence_model_get_basic_status(lc->presence_model) == LinphonePresenceBasicStatusClosed)
		&& (activity = linphone_presence_model_get_activity(lc->presence_model))) {
		char *altContact = nullptr;
		switch (linphone_presence_activity_get_type(activity)) {
			case LinphonePresenceActivityPermanentAbsence:
				altContact = linphone_presence_model_get_contact(lc->presence_model);
				if (altContact) {
					SalErrorInfo sei;
					memset(&sei, 0, sizeof(sei));
					sal_error_info_set(&sei, SalReasonRedirect, "SIP", 0, nullptr, nullptr);
					SalAddress *altAddr = sal_address_new(altContact);
					h->decline_with_error_info(&sei, altAddr);
					ms_free(altContact);
					sal_address_unref(altAddr);
					LinphoneErrorInfo *ei = linphone_error_info_new();
					linphone_error_info_set(ei, nullptr, LinphoneReasonMovedPermanently, 302, "Moved permanently", nullptr);
					linphone_core_report_early_failed_call(lc, LinphoneCallIncoming, fromAddr, toAddr, ei);
					h->release();
					sal_error_info_reset(&sei);
					return;
				}
				break;
			default:
				/* Nothing special to be done */
				break;
		}
	}

	if (!L_GET_PRIVATE_FROM_C_OBJECT(lc)->canWeAddCall()) { /* Busy */
		h->decline(SalReasonBusy, nullptr);
		LinphoneErrorInfo *ei = linphone_error_info_new();
		linphone_error_info_set(ei, nullptr, LinphoneReasonBusy, 486, "Busy - too many calls", nullptr);
		linphone_core_report_early_failed_call(lc, LinphoneCallIncoming, fromAddr, toAddr, ei);
		h->release();
		return;
	}

	/* Check if I'm the caller */
	LinphoneAddress *fromAddressToSearchIfMe = nullptr;
	if (h->get_privacy() == SalPrivacyNone)
		fromAddressToSearchIfMe = linphone_address_clone(fromAddr);
	else if (pAssertedId)
		fromAddressToSearchIfMe = linphone_address_new(pAssertedId);
	else
		ms_warning("Hidden from identity, don't know if it's me");
	if (fromAddressToSearchIfMe && L_GET_PRIVATE_FROM_C_OBJECT(lc)->isAlreadyInCallWithAddress(*L_GET_CPP_PTR_FROM_C_OBJECT(fromAddressToSearchIfMe))) {
		char *addr = linphone_address_as_string(fromAddr);
		ms_warning("Receiving a call while one with same address [%s] is initiated, refusing this one with busy message", addr);
		h->decline(SalReasonBusy, nullptr);
		LinphoneErrorInfo *ei = linphone_error_info_new();
		linphone_error_info_set(ei, nullptr, LinphoneReasonBusy, 486, "Busy - duplicated call", nullptr);
		linphone_core_report_early_failed_call(lc, LinphoneCallIncoming, fromAddr, toAddr, ei);
		h->release();
		linphone_address_unref(fromAddressToSearchIfMe);
		ms_free(addr);
		return;
	}
	if (fromAddressToSearchIfMe)
		linphone_address_unref(fromAddressToSearchIfMe);

	LinphoneCall *call = linphone_call_new_incoming(lc, fromAddr, toAddr, h);
	linphone_address_unref(fromAddr);
	linphone_address_unref(toAddr);
	L_GET_PRIVATE_FROM_C_OBJECT(call)->startIncomingNotification();
}

static void call_rejected(SalCallOp *h){
	LinphoneCore *lc=(LinphoneCore *)h->get_sal()->get_user_pointer();
	LinphoneErrorInfo *ei = linphone_error_info_new();
	linphone_error_info_from_sal_op(ei, h);
	linphone_core_report_early_failed_call(lc, LinphoneCallIncoming, linphone_address_new(h->get_from()), linphone_address_new(h->get_to()), ei);
}

static void call_ringing(SalOp *h) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(h->get_user_pointer());
	if (!session) return;
	L_GET_PRIVATE(session)->remoteRinging();
}

/*
 * could be reach :
 *  - when the call is accepted
 *  - when a request is accepted (pause, resume)
 */
static void call_accepted(SalOp *op) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	if (!session) {
		ms_warning("call_accepted: CallSession no longer exists");
		return;
	}
	L_GET_PRIVATE(session)->accepted();
}

/* this callback is called when an incoming re-INVITE/ SIP UPDATE modifies the session*/
static void call_updating(SalOp *op, bool_t is_update) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	if (!session) {
		ms_warning("call_updating: CallSession no longer exists");
		return;
	}
	L_GET_PRIVATE(session)->updating(!!is_update);
}


static void call_ack_received(SalOp *op, SalCustomHeader *ack) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	if (!session) {
		ms_warning("call_ack_received(): no CallSession for which an ack is expected");
		return;
	}
	L_GET_PRIVATE(session)->ackReceived(reinterpret_cast<LinphoneHeaders *>(ack));
}


static void call_ack_being_sent(SalOp *op, SalCustomHeader *ack) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	if (!session) {
		ms_warning("call_ack_being_sent(): no CallSession for which an ack is supposed to be sent");
		return;
	}
	L_GET_PRIVATE(session)->ackBeingSent(reinterpret_cast<LinphoneHeaders *>(ack));
}

static void call_terminated(SalOp *op, const char *from) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	if (!session)
		return;
	L_GET_PRIVATE(session)->terminated();
}

static void call_failure(SalOp *op) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	if (!session) {
		ms_warning("Failure reported on already terminated CallSession");
		return;
	}
	L_GET_PRIVATE(session)->failure();
}

static void call_released(SalOp *op) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	if (!session) {
		/* We can get here when the core manages call at Sal level without creating a Call object. Typicially,
		 * when declining an incoming call with busy because maximum number of calls is reached. */
		return;
	}
	L_GET_PRIVATE(session)->setState(LinphonePrivate::CallSession::State::Released, "Call released");
}

static void call_cancel_done(SalOp *op) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	if (!session) {
		ms_warning("Cancel done reported on already terminated CallSession");
		return;
	}
	L_GET_PRIVATE(session)->cancelDone();
}

static void auth_failure(SalOp *op, SalAuthInfo* info) {
	LinphoneCore *lc = reinterpret_cast<LinphoneCore *>(op->get_sal()->get_user_pointer());
	LinphoneAuthInfo *ai = NULL;

	if (info != NULL) {
		ai = (LinphoneAuthInfo*)_linphone_core_find_auth_info(lc, info->realm, info->username, info->domain, TRUE);
		if (ai){
			LinphoneAuthMethod method = info->mode == SalAuthModeHttpDigest ? LinphoneAuthHttpDigest : LinphoneAuthTls;
			LinphoneAuthInfo *auth_info = linphone_core_create_auth_info(lc, info->username, NULL, NULL, NULL, info->realm, info->domain);
			ms_message("%s/%s/%s/%s authentication fails.", info->realm, info->username, info->domain, info->mode == SalAuthModeHttpDigest ? "HttpDigest" : "Tls");
			/*ask again for password if auth info was already supplied but apparently not working*/
			linphone_core_notify_authentication_requested(lc, auth_info, method);
			linphone_auth_info_unref(auth_info);
			// Deprecated
			linphone_core_notify_auth_info_requested(lc, info->realm, info->username, info->domain);
		}
	}
}

static void register_success(SalOp *op, bool_t registered){
	LinphoneProxyConfig *cfg=(LinphoneProxyConfig *)op->get_user_pointer();
	if (!cfg){
		ms_message("Registration success for deleted proxy config, ignored");
		return;
	}
	linphone_proxy_config_set_state(cfg, registered ? LinphoneRegistrationOk : LinphoneRegistrationCleared ,
									registered ? "Registration successful" : "Unregistration done");
}

static void register_failure(SalOp *op){
	LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)op->get_user_pointer();
	const SalErrorInfo *ei=op->get_error_info();
	const char *details=ei->full_string;

	if (cfg==NULL){
		ms_warning("Registration failed for unknown proxy config.");
		return ;
	}
	if (details==NULL)
		details="no response timeout";

	if ((ei->reason == SalReasonServiceUnavailable || ei->reason == SalReasonIOError)
			&& linphone_proxy_config_get_state(cfg) == LinphoneRegistrationOk) {
		linphone_proxy_config_set_state(cfg,LinphoneRegistrationProgress,"Service unavailable, retrying");
	} else {
		linphone_proxy_config_set_state(cfg,LinphoneRegistrationFailed,details);
	}
	if (cfg->presence_publish_event){
		/*prevent publish to be sent now until registration gets successful*/
		linphone_event_terminate(cfg->presence_publish_event);
		cfg->presence_publish_event=NULL;
		cfg->send_publish=cfg->publish;
	}
}

static void vfu_request(SalOp *op) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	if (!session)
		return;
	LinphonePrivate::MediaSession *mediaSession = dynamic_cast<LinphonePrivate::MediaSession *>(session);
	if (!mediaSession) {
		ms_warning("VFU request but no MediaSession!");
		return;
	}
	L_GET_PRIVATE(mediaSession)->sendVfu();
}

static void dtmf_received(SalOp *op, char dtmf) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	if (!session)
		return;
	LinphonePrivate::MediaSession *mediaSession = dynamic_cast<LinphonePrivate::MediaSession *>(session);
	if (!mediaSession) {
		ms_warning("DTMF received but no MediaSession!");
		return;
	}
	L_GET_PRIVATE(mediaSession)->dtmfReceived(dtmf);
}

static void call_refer_received(SalOp *op, const SalAddress *referTo) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	char *addrStr = sal_address_as_string_uri_only(referTo);
	Address referToAddr(addrStr);
	string method;
	if (referToAddr.isValid())
		method = referToAddr.getMethodParam();
	if (session && (method.empty() || (method == "INVITE"))) {
		L_GET_PRIVATE(session)->referred(referToAddr);
	} else {
		LinphoneCore *lc = reinterpret_cast<LinphoneCore *>(op->get_sal()->get_user_pointer());
		linphone_core_notify_refer_received(lc, addrStr);
	}
	bctbx_free(addrStr);
}

static void message_received(SalOp *op, const SalMessage *msg){
	LinphoneCore *lc=(LinphoneCore *)op->get_sal()->get_user_pointer();
	LinphoneCall *call=(LinphoneCall*)op->get_user_pointer();
	LinphoneReason reason = lc->chat_deny_code;
	if (reason == LinphoneReasonNone) {
		linphone_core_message_received(lc, op, msg);
	}
	auto messageOp = dynamic_cast<SalMessageOpInterface *>(op);
	messageOp->reply(linphone_reason_to_sal(reason));
	if (!call) op->release();
}

static void parse_presence_requested(SalOp *op, const char *content_type, const char *content_subtype, const char *body, SalPresenceModel **result) {
	linphone_notify_parse_presence(content_type, content_subtype, body, result);
}

static void convert_presence_to_xml_requested(SalOp *op, SalPresenceModel *presence, const char *contact, char **content) {
	/*for backward compatibility because still used by notify. No loguer used for publish*/

	if(linphone_presence_model_get_presentity((LinphonePresenceModel*)presence) == NULL) {
		LinphoneAddress * presentity = linphone_address_new(contact);
		linphone_presence_model_set_presentity((LinphonePresenceModel*)presence, presentity);
		linphone_address_unref(presentity);
	}
	*content = linphone_presence_model_to_xml((LinphonePresenceModel*)presence);
}

static void notify_presence(SalOp *op, SalSubscribeStatus ss, SalPresenceModel *model, const char *msg){
	LinphoneCore *lc=(LinphoneCore *)op->get_sal()->get_user_pointer();
	linphone_notify_recv(lc,op,ss,model);
}

static void subscribe_presence_received(SalPresenceOp *op, const char *from){
	LinphoneCore *lc=(LinphoneCore *)op->get_sal()->get_user_pointer();
	linphone_subscription_new(lc,op,from);
}

static void subscribe_presence_closed(SalPresenceOp *op, const char *from){
	LinphoneCore *lc=(LinphoneCore *)op->get_sal()->get_user_pointer();
	linphone_subscription_closed(lc,op);
}

static void ping_reply(SalOp *op) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	if (!session) {
		ms_warning("Ping reply without CallSession attached...");
		return;
	}
	L_GET_PRIVATE(session)->pingReply();
}

static bool_t fill_auth_info_with_client_certificate(LinphoneCore *lc, SalAuthInfo* sai) {
	const char *chain_file = linphone_core_get_tls_cert_path(lc);
	const char *key_file = linphone_core_get_tls_key_path(lc);

	if (key_file && chain_file) {
#ifndef _WIN32
		// optinal check for files
		struct stat st;
		if (stat(key_file, &st)) {
			ms_warning("No client certificate key found in %s", key_file);
			return FALSE;
		}
		if (stat(chain_file, &st)) {
			ms_warning("No client certificate chain found in %s", chain_file);
			return FALSE;
		}
#endif
		sal_certificates_chain_parse_file(sai, chain_file, SAL_CERTIFICATE_RAW_FORMAT_PEM);
		sal_signing_key_parse_file(sai, key_file, "");
	} else if (lc->tls_cert && lc->tls_key) {
		sal_certificates_chain_parse(sai, lc->tls_cert, SAL_CERTIFICATE_RAW_FORMAT_PEM);
		sal_signing_key_parse(sai, lc->tls_key, "");
	}
	return sai->certificates && sai->key;
}

static bool_t fill_auth_info(LinphoneCore *lc, SalAuthInfo* sai) {
	LinphoneAuthInfo *ai = NULL;
	if (sai->mode == SalAuthModeTls) {
		ai = (LinphoneAuthInfo*)_linphone_core_find_tls_auth_info(lc);
	} else {
		ai = (LinphoneAuthInfo*)_linphone_core_find_auth_info(lc,sai->realm,sai->username,sai->domain, FALSE);
	}
	if (ai) {
		if (sai->mode == SalAuthModeHttpDigest) {
			/*
			 * Compare algorithm of server(sai) with algorithm of client(ai), if they are not correspondant,
			 * exit. The default algorithm is MD5 if it's NULL.
			 */
			if (sai->algorithm && ai->algorithm) {
				if (strcmp(ai->algorithm, sai->algorithm))
					return TRUE;
			} else if (
				(ai->algorithm && strcmp(ai->algorithm, "MD5")) ||
				(sai->algorithm && strcmp(sai->algorithm, "MD5"))
			)
				return TRUE;

			sai->userid = ms_strdup(ai->userid ? ai->userid : ai->username);
			sai->password = ai->passwd?ms_strdup(ai->passwd) : NULL;
			sai->ha1 = ai->ha1 ? ms_strdup(ai->ha1) : NULL;
		} else if (sai->mode == SalAuthModeTls) {
			if (ai->tls_cert && ai->tls_key) {
				sal_certificates_chain_parse(sai, ai->tls_cert, SAL_CERTIFICATE_RAW_FORMAT_PEM);
				sal_signing_key_parse(sai, ai->tls_key, "");
			} else if (ai->tls_cert_path && ai->tls_key_path) {
				sal_certificates_chain_parse_file(sai, ai->tls_cert_path, SAL_CERTIFICATE_RAW_FORMAT_PEM);
				sal_signing_key_parse_file(sai, ai->tls_key_path, "");
			} else {
				fill_auth_info_with_client_certificate(lc, sai);
			}
		}

		if (sai->realm && !ai->realm){
			/*if realm was not known, then set it so that ha1 may eventually be calculated and clear text password dropped*/
			linphone_auth_info_set_realm(ai, sai->realm);
			linphone_core_write_auth_info(lc, ai);
		}
		return TRUE;
	} else {
		if (sai->mode == SalAuthModeTls) {
			return fill_auth_info_with_client_certificate(lc, sai);
		}
		return FALSE;
	}
}
static bool_t auth_requested(Sal* sal, SalAuthInfo* sai) {
	LinphoneCore *lc = (LinphoneCore *)sal->get_user_pointer();
	if (fill_auth_info(lc,sai)) {
		return TRUE;
	} else {
		LinphoneAuthMethod method = sai->mode == SalAuthModeHttpDigest ? LinphoneAuthHttpDigest : LinphoneAuthTls;
		LinphoneAuthInfo *ai = linphone_core_create_auth_info(lc, sai->username, NULL, NULL, NULL, sai->realm, sai->domain);
		linphone_core_notify_authentication_requested(lc, ai, method);
		linphone_auth_info_unref(ai);
		// Deprecated
		linphone_core_notify_auth_info_requested(lc, sai->realm, sai->username, sai->domain);
		if (fill_auth_info(lc, sai)) {
			return TRUE;
		}
		return FALSE;
	}
}

static void notify_refer(SalOp *op, SalReferStatus status) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	if (!session) {
		ms_warning("Receiving notify_refer for unknown CallSession");
		return;
	}
	LinphonePrivate::CallSession::State cstate;
	switch (status) {
		case SalReferTrying:
			cstate = LinphonePrivate::CallSession::State::OutgoingProgress;
			break;
		case SalReferSuccess:
			cstate = LinphonePrivate::CallSession::State::Connected;
			break;
		case SalReferFailed:
		default:
			cstate = LinphonePrivate::CallSession::State::Error;
			break;
	}
	L_GET_PRIVATE(session)->setTransferState(cstate);
	if (cstate == LinphonePrivate::CallSession::State::Connected)
		session->terminate(); // Automatically terminate the call as the transfer is complete
}

static LinphoneChatMessageState chatStatusSal2Linphone(SalMessageDeliveryStatus status){
	switch(status){
		case SalMessageDeliveryInProgress:
			return LinphoneChatMessageStateInProgress;
		case SalMessageDeliveryDone:
			return LinphoneChatMessageStateDelivered;
		case SalMessageDeliveryFailed:
			return LinphoneChatMessageStateNotDelivered;
	}
	return LinphoneChatMessageStateIdle;
}

static void message_delivery_update(SalOp *op, SalMessageDeliveryStatus status) {
	LinphonePrivate::ChatMessage *msg = reinterpret_cast<LinphonePrivate::ChatMessage *>(op->get_user_pointer());
	if (!msg)
		return; // Do not handle delivery status for isComposing messages.

	// Check that the message does not belong to an already destroyed chat room - if so, do not invoke callbacks
	if (msg->getChatRoom())
		L_GET_PRIVATE(msg)->setState((LinphonePrivate::ChatMessage::State)chatStatusSal2Linphone(status));
}

static void info_received(SalOp *op, SalBodyHandler *body_handler) {
	LinphonePrivate::CallSession *session = reinterpret_cast<LinphonePrivate::CallSession *>(op->get_user_pointer());
	if (!session)
		return;
	L_GET_PRIVATE(session)->infoReceived(body_handler);
}

static void subscribe_response(SalOp *op, SalSubscribeStatus status, int will_retry){
	LinphoneEvent *lev=(LinphoneEvent*)op->get_user_pointer();

	if (lev==NULL) return;

	if (status==SalSubscribeActive){
		linphone_event_set_state(lev,LinphoneSubscriptionActive);
	}else if (status==SalSubscribePending){
		linphone_event_set_state(lev,LinphoneSubscriptionPending);
	}else{
		if (will_retry){
			linphone_event_set_state(lev,LinphoneSubscriptionOutgoingProgress);
		}
		else linphone_event_set_state(lev,LinphoneSubscriptionError);
	}
}

static void notify(SalSubscribeOp *op, SalSubscribeStatus st, const char *eventname, SalBodyHandler *body_handler){
	LinphoneEvent *lev=(LinphoneEvent*)op->get_user_pointer();
	LinphoneCore *lc=(LinphoneCore *)op->get_sal()->get_user_pointer();
	bool_t out_of_dialog = (lev==NULL);
	if (out_of_dialog) {
		/*out of dialog notify */
		lev = linphone_event_new_with_out_of_dialog_op(lc,op,LinphoneSubscriptionOutgoing,eventname);
	}
	{
		LinphoneContent *ct=linphone_content_from_sal_body_handler(body_handler);
		if (ct) {
			linphone_core_notify_notify_received(lc,lev,eventname,ct);
			linphone_content_unref(ct);
		}
	}
	if (out_of_dialog){
		/*out of dialog NOTIFY do not create an implicit subscription*/
		linphone_event_set_state(lev, LinphoneSubscriptionTerminated);
	}else if (st!=SalSubscribeNone){
		linphone_event_set_state(lev,linphone_subscription_state_from_sal(st));
	}
}

static void subscribe_received(SalSubscribeOp *op, const char *eventname, const SalBodyHandler *body_handler){
	LinphoneEvent *lev=(LinphoneEvent*)op->get_user_pointer();
	LinphoneCore *lc=(LinphoneCore *)op->get_sal()->get_user_pointer();

	if (lev==NULL) {
		lev=linphone_event_new_with_op(lc,op,LinphoneSubscriptionIncoming,eventname);
		linphone_event_set_state(lev,LinphoneSubscriptionIncomingReceived);
	}else{
		/*subscribe refresh, unhandled*/
	}

}

static void incoming_subscribe_closed(SalOp *op){
	LinphoneEvent *lev=(LinphoneEvent*)op->get_user_pointer();

	linphone_event_set_state(lev,LinphoneSubscriptionTerminated);
}

static void on_publish_response(SalOp* op){
	LinphoneEvent *lev=(LinphoneEvent*)op->get_user_pointer();
	const SalErrorInfo *ei=op->get_error_info();

	if (lev==NULL) return;
	if (ei->reason==SalReasonNone){
		if (!lev->terminating)
			linphone_event_set_publish_state(lev,LinphonePublishOk);
		else
			linphone_event_set_publish_state(lev,LinphonePublishCleared);
	}else{
		if (lev->publish_state==LinphonePublishOk){
			linphone_event_set_publish_state(lev,LinphonePublishProgress);
		}else{
			linphone_event_set_publish_state(lev,LinphonePublishError);
		}
	}
}


static void on_expire(SalOp *op){
	LinphoneEvent *lev=(LinphoneEvent*)op->get_user_pointer();

	if (lev==NULL) return;

	if (linphone_event_get_publish_state(lev)==LinphonePublishOk){
		linphone_event_set_publish_state(lev,LinphonePublishExpiring);
	}else if (linphone_event_get_subscription_state(lev)==LinphoneSubscriptionActive){
		linphone_event_set_state(lev,LinphoneSubscriptionExpiring);
	}
}

static void on_notify_response(SalOp *op){
	LinphoneEvent *lev=(LinphoneEvent*)op->get_user_pointer();
	if (!lev)
		return;

	if (lev->is_out_of_dialog_op) {
		switch (linphone_event_get_subscription_state(lev)) {
			case LinphoneSubscriptionIncomingReceived:
				if (op->get_error_info()->reason == SalReasonNone)
					linphone_event_set_state(lev, LinphoneSubscriptionTerminated);
				else
					linphone_event_set_state(lev, LinphoneSubscriptionError);
				break;
			default:
				ms_warning("Unhandled on_notify_response() case %s",
					linphone_subscription_state_to_string(linphone_event_get_subscription_state(lev)));
				break;
		}
	} else {
		ms_warning("on_notify_response in dialog");
		_linphone_event_notify_notify_response(lev);
	}
}

static void refer_received(SalOp *op, const SalAddress *refer_to){
	if (sal_address_has_param(refer_to, "text")) {
		char *refer_uri = sal_address_as_string(refer_to);
		LinphonePrivate::Address addr(refer_uri);
		bctbx_free(refer_uri);
		if (addr.isValid()) {
			LinphoneCore *lc = reinterpret_cast<LinphoneCore *>(op->get_sal()->get_user_pointer());
			if (addr.hasUriParam("method") && (addr.getUriParamValue("method") == "BYE")) {
				if (linphone_core_conference_server_enabled(lc)) {
					// Removal of a participant at the server side
					shared_ptr<AbstractChatRoom> chatRoom = L_GET_CPP_PTR_FROM_C_OBJECT(lc)->findChatRoom(
						ChatRoomId(IdentityAddress(op->get_to()), IdentityAddress(op->get_to()))
					);
					if (chatRoom) {
						std::shared_ptr<Participant> participant = chatRoom->findParticipant(IdentityAddress(op->get_from()));
						if (!participant || !participant->isAdmin()) {
							static_cast<SalReferOp *>(op)->reply(SalReasonDeclined);
							return;
						}
						participant = chatRoom->findParticipant(addr);
						if (participant)
							chatRoom->removeParticipant(participant);
						static_cast<SalReferOp *>(op)->reply(SalReasonNone);
						return;
					}
				} else {
					// The server asks a participant to leave a chat room
					LinphoneChatRoom *cr = L_GET_C_BACK_PTR(
						L_GET_CPP_PTR_FROM_C_OBJECT(lc)->findChatRoom(ChatRoomId(addr, IdentityAddress(op->get_to())))
					);
					if (cr) {
						L_GET_CPP_PTR_FROM_C_OBJECT(cr)->leave();
						static_cast<SalReferOp *>(op)->reply(SalReasonNone);
						return;
					}
					static_cast<SalReferOp *>(op)->reply(SalReasonDeclined);
				}
			} else if (addr.hasParam("admin")) {
				LinphoneChatRoom *cr = L_GET_C_BACK_PTR(L_GET_CPP_PTR_FROM_C_OBJECT(lc)->findChatRoom(
					ChatRoomId(IdentityAddress(op->get_to()), IdentityAddress(op->get_to()))
				));
				if (cr) {
					Address fromAddr(op->get_from());
					std::shared_ptr<Participant> participant = L_GET_CPP_PTR_FROM_C_OBJECT(cr)->findParticipant(fromAddr);
					if (!participant || !participant->isAdmin()) {
						static_cast<SalReferOp *>(op)->reply(SalReasonDeclined);
						return;
					}
					participant = L_GET_CPP_PTR_FROM_C_OBJECT(cr)->findParticipant(addr);
					if (participant) {
						bool value = Utils::stob(addr.getParamValue("admin"));
						L_GET_CPP_PTR_FROM_C_OBJECT(cr)->setParticipantAdminStatus(participant, value);
					}
					static_cast<SalReferOp *>(op)->reply(SalReasonNone);
					return;
				}
			} else {
				shared_ptr<AbstractChatRoom> chatRoom = L_GET_CPP_PTR_FROM_C_OBJECT(lc)->findChatRoom(
					ChatRoomId(addr, IdentityAddress(op->get_to()))
				);
				if (!chatRoom)
					chatRoom = L_GET_PRIVATE_FROM_C_OBJECT(lc)->createClientGroupChatRoom("", addr.asString(), false);
				chatRoom->join();
				static_cast<SalReferOp *>(op)->reply(SalReasonNone);
				return;
			}
		}
	}
	static_cast<SalReferOp *>(op)->reply(SalReasonDeclined);
}

Sal::Callbacks linphone_sal_callbacks={
	call_received,
	call_rejected,
	call_ringing,
	call_accepted,
	call_ack_received,
	call_ack_being_sent,
	call_updating,
	call_terminated,
	call_failure,
	call_released,
	call_cancel_done,
	call_refer_received,
	auth_failure,
	register_success,
	register_failure,
	vfu_request,
	dtmf_received,
	message_received,
	message_delivery_update,
	notify_refer,
	subscribe_received,
	incoming_subscribe_closed,
	subscribe_response,
	notify,
	subscribe_presence_received,
	subscribe_presence_closed,
	parse_presence_requested,
	convert_presence_to_xml_requested,
	notify_presence,
	ping_reply,
	auth_requested,
	info_received,
	on_publish_response,
	on_expire,
	on_notify_response,
	refer_received,
};
