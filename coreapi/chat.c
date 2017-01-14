/***************************************************************************
 *            chat.c
 *
 *  Sun Jun  5 19:34:18 2005
 *  Copyright  2005  Simon Morlat
 *  Email simon dot morlat at linphone dot org
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "linphone/core.h"
#include "private.h"
#include "linphone/lpconfig.h"
#include "belle-sip/belle-sip.h"
#include "ortp/b64.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>

#define COMPOSING_DEFAULT_IDLE_TIMEOUT 15
#define COMPOSING_DEFAULT_REFRESH_TIMEOUT 60
#define COMPOSING_DEFAULT_REMOTE_REFRESH_TIMEOUT 120

static void linphone_chat_message_release(LinphoneChatMessage *msg);
static void linphone_chat_room_delete_composing_idle_timer(LinphoneChatRoom *cr);
static void linphone_chat_room_delete_composing_refresh_timer(LinphoneChatRoom *cr);
static void linphone_chat_room_delete_remote_composing_refresh_timer(LinphoneChatRoom *cr);
static void _linphone_chat_message_destroy(LinphoneChatMessage *msg);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneChatMessageCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneChatMessageCbs, belle_sip_object_t,
						   NULL, // destroy
						   NULL, // clone
						   NULL, // marshal
						   FALSE);

LinphoneChatMessageCbs *linphone_chat_message_cbs_new(void) {
	return belle_sip_object_new(LinphoneChatMessageCbs);
}

LinphoneChatMessageCbs *linphone_chat_message_cbs_ref(LinphoneChatMessageCbs *cbs) {
	belle_sip_object_ref(cbs);
	return cbs;
}

void linphone_chat_message_cbs_unref(LinphoneChatMessageCbs *cbs) {
	belle_sip_object_unref(cbs);
}

void *linphone_chat_message_cbs_get_user_data(const LinphoneChatMessageCbs *cbs) {
	return cbs->user_data;
}

void linphone_chat_message_cbs_set_user_data(LinphoneChatMessageCbs *cbs, void *ud) {
	cbs->user_data = ud;
}

LinphoneChatMessageCbsMsgStateChangedCb
linphone_chat_message_cbs_get_msg_state_changed(const LinphoneChatMessageCbs *cbs) {
	return cbs->msg_state_changed;
}

void linphone_chat_message_cbs_set_msg_state_changed(LinphoneChatMessageCbs *cbs,
													 LinphoneChatMessageCbsMsgStateChangedCb cb) {
	cbs->msg_state_changed = cb;
}

LinphoneChatMessageCbsFileTransferRecvCb linphone_chat_message_cbs_get_file_transfer_recv(const LinphoneChatMessageCbs *cbs) {
	return cbs->file_transfer_recv;
}

void linphone_chat_message_cbs_set_file_transfer_recv(LinphoneChatMessageCbs *cbs,
													  LinphoneChatMessageCbsFileTransferRecvCb cb) {
	cbs->file_transfer_recv = cb;
}

LinphoneChatMessageCbsFileTransferSendCb linphone_chat_message_cbs_get_file_transfer_send(const LinphoneChatMessageCbs *cbs) {
	return cbs->file_transfer_send;
}

void linphone_chat_message_cbs_set_file_transfer_send(LinphoneChatMessageCbs *cbs,
													  LinphoneChatMessageCbsFileTransferSendCb cb) {
	cbs->file_transfer_send = cb;
}

LinphoneChatMessageCbsFileTransferProgressIndicationCb
linphone_chat_message_cbs_get_file_transfer_progress_indication(const LinphoneChatMessageCbs *cbs) {
	return cbs->file_transfer_progress_indication;
}

void linphone_chat_message_cbs_set_file_transfer_progress_indication(
	LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsFileTransferProgressIndicationCb cb) {
	cbs->file_transfer_progress_indication = cb;
}


BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneChatMessage);

static void _linphone_chat_room_destroy(LinphoneChatRoom *cr) {
	bctbx_list_free_with_data(cr->transient_messages, (void (*)(void *))linphone_chat_message_release);
	if (cr->received_rtt_characters) {
		cr->received_rtt_characters = bctbx_list_free_with_data(cr->received_rtt_characters, (void (*)(void *))ms_free);
	}
	linphone_chat_room_delete_composing_idle_timer(cr);
	linphone_chat_room_delete_composing_refresh_timer(cr);
	linphone_chat_room_delete_remote_composing_refresh_timer(cr);
	if (cr->lc != NULL) {
		if (bctbx_list_find(cr->lc->chatrooms, cr)) {
			ms_error("LinphoneChatRoom[%p] is destroyed while still being used by the LinphoneCore. This is abnormal."
					 " linphone_core_get_chat_room() doesn't give a reference, there is no need to call "
					 "linphone_chat_room_unref(). "
					 "In order to remove a chat room from the core, use linphone_core_delete_chat_room().",
					 cr);
			cr->lc->chatrooms = bctbx_list_remove(cr->lc->chatrooms, cr);
		}
	}
	linphone_address_unref(cr->peer_url);
	if (cr->pending_message)
		linphone_chat_message_destroy(cr->pending_message);
	ms_free(cr->peer);
	if (cr->weak_messages != NULL) bctbx_list_free(cr->weak_messages);
}

void linphone_chat_message_set_state(LinphoneChatMessage *msg, LinphoneChatMessageState state) {
	/* do not invoke callbacks on orphan messages */
	if (state != msg->state && msg->chat_room != NULL) {
		if ((msg->state == LinphoneChatMessageStateDisplayed) && (state == LinphoneChatMessageStateDeliveredToUser)) {
			/* If the message has been displayed we must not go back to the delivered to user state. */
			return;
		}
		ms_message("Chat message %p: moving from state %s to %s", msg, linphone_chat_message_state_to_string(msg->state),
				   linphone_chat_message_state_to_string(state));
		msg->state = state;
		if (msg->message_state_changed_cb) {
			msg->message_state_changed_cb(msg, msg->state, msg->message_state_changed_user_data);
		}
		if (linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)) {
			linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)(msg, msg->state);
		}
	}
}

BELLE_SIP_INSTANCIATE_VPTR(LinphoneChatMessage, belle_sip_object_t,
						   (belle_sip_object_destroy_t)_linphone_chat_message_destroy,
						   NULL, // clone
						   NULL, // marshal
						   FALSE);

void linphone_core_disable_chat(LinphoneCore *lc, LinphoneReason deny_reason) {
	lc->chat_deny_code = deny_reason;
}

void linphone_core_enable_chat(LinphoneCore *lc) {
	lc->chat_deny_code = LinphoneReasonNone;
}

bool_t linphone_core_chat_enabled(const LinphoneCore *lc) {
	return lc->chat_deny_code != LinphoneReasonNone;
}

const bctbx_list_t *linphone_core_get_chat_rooms(LinphoneCore *lc) {
	return lc->chatrooms;
}

static bool_t linphone_chat_room_matches(LinphoneChatRoom *cr, const LinphoneAddress *from) {
	return linphone_address_weak_equal(cr->peer_url, from);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneChatRoom);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneChatRoom, belle_sip_object_t,
						   (belle_sip_object_destroy_t)_linphone_chat_room_destroy,
						   NULL, // clone
						   NULL, // marshal
						   FALSE);

static LinphoneChatRoom *_linphone_core_create_chat_room_base(LinphoneCore *lc, LinphoneAddress *addr){
	LinphoneChatRoom *cr = belle_sip_object_new(LinphoneChatRoom);
	cr->lc = lc;
	cr->peer = linphone_address_as_string(addr);
	cr->peer_url = addr;
	cr->unread_count = -1;
	cr->received_rtt_characters = NULL;
	return cr;
}

static LinphoneChatRoom *_linphone_core_create_chat_room(LinphoneCore *lc, LinphoneAddress *addr) {
	LinphoneChatRoom *cr = _linphone_core_create_chat_room_base(lc, addr);
	lc->chatrooms = bctbx_list_append(lc->chatrooms, (void *)cr);
	return cr;
}

LinphoneChatRoom *_linphone_core_create_chat_room_from_call(LinphoneCall *call){
	LinphoneChatRoom *cr = _linphone_core_create_chat_room_base(call->core,
		linphone_address_clone(linphone_call_get_remote_address(call)));
	cr->call = call;
	return cr;
}

static LinphoneChatRoom *_linphone_core_create_chat_room_from_url(LinphoneCore *lc, const char *to) {
	LinphoneAddress *parsed_url = NULL;
	if ((parsed_url = linphone_core_interpret_url(lc, to)) != NULL) {
		return _linphone_core_create_chat_room(lc, parsed_url);
	}
	return NULL;
}

LinphoneChatRoom *_linphone_core_get_chat_room(LinphoneCore *lc, const LinphoneAddress *addr) {
	LinphoneChatRoom *cr = NULL;
	bctbx_list_t *elem;
	for (elem = lc->chatrooms; elem != NULL; elem = bctbx_list_next(elem)) {
		cr = (LinphoneChatRoom *)elem->data;
		if (linphone_chat_room_matches(cr, addr)) {
			break;
		}
		cr = NULL;
	}
	return cr;
}

static LinphoneChatRoom *_linphone_core_get_or_create_chat_room(LinphoneCore *lc, const char *to) {
	LinphoneAddress *to_addr = linphone_core_interpret_url(lc, to);
	LinphoneChatRoom *ret;

	if (to_addr == NULL) {
		ms_error("linphone_core_get_or_create_chat_room(): Cannot make a valid address with %s", to);
		return NULL;
	}
	ret = _linphone_core_get_chat_room(lc, to_addr);
	linphone_address_unref(to_addr);
	if (!ret) {
		ret = _linphone_core_create_chat_room_from_url(lc, to);
	}
	return ret;
}

LinphoneChatRoom *linphone_core_get_chat_room(LinphoneCore *lc, const LinphoneAddress *addr) {
	LinphoneChatRoom *ret = _linphone_core_get_chat_room(lc, addr);
	if (!ret) {
		ret = _linphone_core_create_chat_room(lc, linphone_address_clone(addr));
	}
	return ret;
}

void linphone_core_delete_chat_room(LinphoneCore *lc, LinphoneChatRoom *cr) {
	if (bctbx_list_find(lc->chatrooms, cr)) {
		lc->chatrooms = bctbx_list_remove(cr->lc->chatrooms, cr);
		linphone_chat_room_delete_history(cr);
		linphone_chat_room_unref(cr);
	} else {
		ms_error("linphone_core_delete_chat_room(): chatroom [%p] isn't part of LinphoneCore.", cr);
	}
}

LinphoneChatRoom *linphone_core_get_chat_room_from_uri(LinphoneCore *lc, const char *to) {
	return _linphone_core_get_or_create_chat_room(lc, to);
}

static void linphone_chat_room_delete_composing_idle_timer(LinphoneChatRoom *cr) {
	if (cr->composing_idle_timer) {
		if (cr->lc && cr->lc->sal)
			sal_cancel_timer(cr->lc->sal, cr->composing_idle_timer);
		belle_sip_object_unref(cr->composing_idle_timer);
		cr->composing_idle_timer = NULL;
	}
}

static void linphone_chat_room_delete_composing_refresh_timer(LinphoneChatRoom *cr) {
	if (cr->composing_refresh_timer) {
		if (cr->lc && cr->lc->sal)
			sal_cancel_timer(cr->lc->sal, cr->composing_refresh_timer);
		belle_sip_object_unref(cr->composing_refresh_timer);
		cr->composing_refresh_timer = NULL;
	}
}

static void linphone_chat_room_delete_remote_composing_refresh_timer(LinphoneChatRoom *cr) {
	if (cr->remote_composing_refresh_timer) {
		if (cr->lc && cr->lc->sal)
			sal_cancel_timer(cr->lc->sal, cr->remote_composing_refresh_timer);
		belle_sip_object_unref(cr->remote_composing_refresh_timer);
		cr->remote_composing_refresh_timer = NULL;
	}
}

void linphone_chat_room_destroy(LinphoneChatRoom *cr) {
	linphone_chat_room_unref(cr);
}

void linphone_chat_room_release(LinphoneChatRoom *cr) {
	linphone_chat_room_delete_composing_idle_timer(cr);
	linphone_chat_room_delete_composing_refresh_timer(cr);
	linphone_chat_room_delete_remote_composing_refresh_timer(cr);
	cr->lc = NULL;
	linphone_chat_room_unref(cr);
}

static void on_weak_message_destroy(void *obj, belle_sip_object_t *message_being_destroyed) {
	LinphoneChatRoom *cr = (LinphoneChatRoom *)obj;
	cr->weak_messages = bctbx_list_remove(cr->weak_messages, message_being_destroyed);
}

void linphone_chat_room_add_weak_message(LinphoneChatRoom *cr, LinphoneChatMessage *cm) {
	bctbx_list_t *item = bctbx_list_find(cr->weak_messages, cm);
	if (item == NULL) {
		cr->weak_messages = bctbx_list_append(cr->weak_messages, belle_sip_object_weak_ref(cm, on_weak_message_destroy, cr));
	}
}

LinphoneChatRoom *linphone_chat_room_ref(LinphoneChatRoom *cr) {
	belle_sip_object_ref(cr);
	return cr;
}

void linphone_chat_room_unref(LinphoneChatRoom *cr) {
	belle_sip_object_unref(cr);
}

void *linphone_chat_room_get_user_data(const LinphoneChatRoom *cr) {
	return cr->user_data;
}

void linphone_chat_room_set_user_data(LinphoneChatRoom *cr, void *ud) {
	cr->user_data = ud;
}

void _linphone_chat_room_send_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	int retval = -1;
	LinphoneCore *lc = cr->lc;
	LinphoneImEncryptionEngine *imee = lc->im_encryption_engine;
	
	/*stubed rtt text*/
	if (cr->call && linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(cr->call))) {
		uint32_t new_line = 0x2028;
		linphone_chat_message_put_char(msg, new_line); // New Line
		linphone_chat_message_unref(msg);
		return;
	}

	msg->dir = LinphoneChatMessageOutgoing;


	/* Check if we shall upload a file to a server */
	if (msg->file_transfer_information != NULL && msg->content_type == NULL) {
		/* open a transaction with the server and send an empty request(RCS5.1 section 3.5.4.8.3.1) */
		if (linphone_chat_room_upload_file(msg) == 0) {
			// add to transient list only if message is going out
			cr->transient_messages = bctbx_list_append(cr->transient_messages, linphone_chat_message_ref(msg));
		} else {
			linphone_chat_message_unref(msg);
			return;
		}
	} else {
		SalOp *op = NULL;
		LinphoneCall *call=NULL;
		char *content_type;
		const char *identity = NULL;
		char *message_not_encrypted = NULL;
		
		if (msg->message) {
			message_not_encrypted = ms_strdup(msg->message);
		}
		
		// add to transient list
		cr->transient_messages = bctbx_list_append(cr->transient_messages, linphone_chat_message_ref(msg));
		msg->time = ms_time(0);
		if (lp_config_get_int(cr->lc->config, "sip", "chat_use_call_dialogs", 0) != 0) {
			if ((call = linphone_core_get_call_by_remote_address(cr->lc, cr->peer)) != NULL) {
				if (call->state == LinphoneCallConnected || call->state == LinphoneCallStreamsRunning ||
					call->state == LinphoneCallPaused || call->state == LinphoneCallPausing ||
					call->state == LinphoneCallPausedByRemote) {
					ms_message("send SIP msg through the existing call.");
					op = call->op;
					identity = linphone_core_find_best_identity(cr->lc, linphone_call_get_remote_address(call));
				}
			}
		}
		
		if (!identity) {
			LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(cr->lc, cr->peer_url);
			if (proxy) {
				identity = linphone_proxy_config_get_identity(proxy);
			} else {
				identity = linphone_core_get_primary_contact(cr->lc);
			}
		}
		if (msg->from){
			/*
			 * BUG
			 * the file transfer message constructor sets the from, but doesn't do it as well as here.
			 */
			linphone_address_unref(msg->from);
		}
		msg->from = linphone_address_new(identity);
		
		if (imee) {
			LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
			LinphoneImEncryptionEngineCbsOutgoingMessageCb cb_process_outgoing_message = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imee_cbs);
			if (cb_process_outgoing_message) {
				retval = cb_process_outgoing_message(imee, cr, msg);
			}
		}
		
		if (op == NULL) {
			/*sending out of calls*/
			msg->op = op = sal_op_new(cr->lc->sal);
			linphone_configure_op(cr->lc, op, cr->peer_url, msg->custom_headers,
								  lp_config_get_int(cr->lc->config, "sip", "chat_msg_with_contact", 0));
			sal_op_set_user_pointer(op, msg); /*if out of call, directly store msg*/
		}
		
		if (retval > 0) {
			sal_error_info_set((SalErrorInfo *)sal_op_get_error_info(op), SalReasonNotAcceptable, retval, "Unable to encrypt IM", NULL);
			linphone_chat_message_update_state(msg, LinphoneChatMessageStateNotDelivered);
			linphone_chat_message_unref(msg);
			return;
		}

		if (msg->external_body_url) {
			content_type = ms_strdup_printf("message/external-body; access-type=URL; URL=\"%s\"", msg->external_body_url);
			sal_message_send(op, identity, cr->peer, content_type, NULL, NULL);
			ms_free(content_type);
		} else {
			char *peer_uri = linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(cr));
			const char *content_type = msg->content_type;
			if (content_type == NULL) {
				sal_text_send(op, identity, cr->peer, msg->message);
			} else {
				sal_message_send(op, identity, cr->peer, content_type, msg->message, peer_uri);
			}
			ms_free(peer_uri);
		}

		if (msg->message && message_not_encrypted && strcmp(msg->message, message_not_encrypted) != 0) {
			// We replace the encrypted message by the original one so it can be correctly stored and displayed by the application
			ms_free(msg->message);
			msg->message = ms_strdup(message_not_encrypted);
		}
		msg->message_id = ms_strdup(sal_op_get_call_id(op)); /* must be known at that time */
		msg->storage_id = linphone_chat_message_store(msg);

		if (cr->is_composing == LinphoneIsComposingActive) {
			cr->is_composing = LinphoneIsComposingIdle;
		}
		linphone_chat_room_delete_composing_idle_timer(cr);
		linphone_chat_room_delete_composing_refresh_timer(cr);

		if (message_not_encrypted) {
			ms_free(message_not_encrypted);
		}

		if (call && call->op == op) {
			/*In this case, chat delivery status is not notified, so unrefing chat message right now*/
			/*Might be better fixed by delivering status, but too costly for now*/
			msg->chat_room->transient_messages = bctbx_list_remove(msg->chat_room->transient_messages, msg);
			linphone_chat_message_unref(msg);
			linphone_chat_message_unref(msg);
			return;
		}
	}
	// if operation failed, we should not change message state
	if (msg->dir == LinphoneChatMessageOutgoing) {
		linphone_chat_message_set_state(msg, LinphoneChatMessageStateInProgress);
	}
}

void linphone_chat_message_update_state(LinphoneChatMessage *msg, LinphoneChatMessageState new_state) {
	linphone_chat_message_set_state(msg, new_state);
	linphone_chat_message_store_state(msg);

	if (msg->state == LinphoneChatMessageStateDelivered || msg->state == LinphoneChatMessageStateNotDelivered) {
		if (bctbx_list_find(msg->chat_room->transient_messages, msg) != NULL) {
			// msg is not transient anymore, we can remove it from our transient list and unref it
			msg->chat_room->transient_messages = bctbx_list_remove(msg->chat_room->transient_messages, msg);
			linphone_chat_room_add_weak_message(msg->chat_room, msg);
			linphone_chat_message_unref(msg);
		} else {
			// msg has already been removed from the transient messages, do nothing. */
		}
	}
}

void linphone_chat_room_send_message(LinphoneChatRoom *cr, const char *msg) {
	_linphone_chat_room_send_message(cr, linphone_chat_room_create_message(cr, msg));
}

void linphone_chat_room_message_received(LinphoneChatRoom *cr, LinphoneCore *lc, LinphoneChatMessage *msg) {
	if (msg->message) {
		/*legacy API*/
		linphone_core_notify_text_message_received(lc, cr, msg->from, msg->message);
	}
	linphone_core_notify_message_received(lc, cr, msg);
	cr->remote_is_composing = LinphoneIsComposingIdle;
	linphone_core_notify_is_composing_received(cr->lc, cr);
	linphone_chat_message_send_delivery_notification(msg, LinphoneReasonNone);
}

LinphoneReason linphone_core_message_received(LinphoneCore *lc, SalOp *op, const SalMessage *sal_msg) {
	LinphoneChatRoom *cr = NULL;
	LinphoneAddress *addr;
	LinphoneAddress *to;
	LinphoneChatMessage *msg;
	LinphoneImEncryptionEngine *imee = lc->im_encryption_engine;
	const SalCustomHeader *ch;
	LinphoneReason reason = LinphoneReasonNone;
	int retval = -1;

	addr = linphone_address_new(sal_msg->from);
	linphone_address_clean(addr);
	cr = linphone_core_get_chat_room(lc, addr);

	msg = linphone_chat_room_create_message(cr, sal_msg->text); /* create a msg with empty body */
	msg->content_type = ms_strdup(sal_msg->content_type); /* add the content_type "application/vnd.gsma.rcs-ft-http+xml" */
	linphone_chat_message_set_from(msg, cr->peer_url);

	to = sal_op_get_to(op) ? linphone_address_new(sal_op_get_to(op)) : linphone_address_new(linphone_core_get_identity(lc));
	msg->to = to;
	
	msg->time = sal_msg->time;
	msg->state = LinphoneChatMessageStateDelivered;
	msg->dir = LinphoneChatMessageIncoming;
	msg->message_id = ms_strdup(sal_op_get_call_id(op));
	
	ch = sal_op_get_recv_custom_header(op);
	if (ch) {
		msg->custom_headers = sal_custom_header_clone(ch);
	}

	if (sal_msg->url) {
		linphone_chat_message_set_external_body_url(msg, sal_msg->url);
	}
	
	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsIncomingMessageCb cb_process_incoming_message = linphone_im_encryption_engine_cbs_get_process_incoming_message(imee_cbs);
		if (cb_process_incoming_message) {
			retval = cb_process_incoming_message(imee, cr, msg);
		}
	}
	
	if (retval < 0 && strcmp("text/plain", msg->content_type) != 0 && strcmp("message/external-body", msg->content_type) != 0
			&& strcmp("application/vnd.gsma.rcs-ft-http+xml", msg->content_type) != 0) {
		retval = 415;
		ms_error("Unsupported MESSAGE (content-type %s not recognized)", msg->content_type);
	}
	
	if (retval > 0) {
		reason = linphone_error_code_to_reason(retval);
		linphone_chat_message_send_delivery_notification(msg, reason);
		goto end;
	}

	if (strcmp("application/vnd.gsma.rcs-ft-http+xml", msg->content_type) == 0) {
		xmlChar *file_url = NULL;
		xmlDocPtr xmlMessageBody;
		xmlNodePtr cur;
		/* content_type field is, for now, used only for rcs file transfer but we shall strcmp it with "application/vnd.gsma.rcs-ft-http+xml" */
		/* parse the msg body to get all informations from it */
		xmlMessageBody = xmlParseDoc((const xmlChar *)msg->message);
		msg->file_transfer_information = linphone_content_new();

		cur = xmlDocGetRootElement(xmlMessageBody);
		if (cur != NULL) {
			cur = cur->xmlChildrenNode;
			while (cur != NULL) {
				if (!xmlStrcmp(
						cur->name, (const xmlChar *)"file-info")) { /* we found a file info node, check it has a
																	   type="file" attribute */
					xmlChar *typeAttribute = xmlGetProp(cur, (const xmlChar *)"type");
					if (!xmlStrcmp(typeAttribute, (const xmlChar *)"file")) { /* this is the node we are looking for */
						cur = cur->xmlChildrenNode; /* now loop on the content of the file-info node */
						while (cur != NULL) {
							if (!xmlStrcmp(cur->name, (const xmlChar *)"file-size")) {
								xmlChar *fileSizeString = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
								linphone_content_set_size(msg->file_transfer_information,
														  strtol((const char *)fileSizeString, NULL, 10));
								xmlFree(fileSizeString);
							}

							if (!xmlStrcmp(cur->name, (const xmlChar *)"file-name")) {
								xmlChar *filename = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
								linphone_content_set_name(
									msg->file_transfer_information,
									(char *)filename);
								xmlFree(filename);
							}
							if (!xmlStrcmp(cur->name, (const xmlChar *)"content-type")) {
								xmlChar *contentType = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
								int contentTypeIndex = 0;
								char *type;
								char *subtype;
								while (contentType[contentTypeIndex] != '/' && contentType[contentTypeIndex] != '\0') {
									contentTypeIndex++;
								}
								type = ms_strndup((char *)contentType, contentTypeIndex);
								subtype = ms_strdup(((char *)contentType + contentTypeIndex + 1));
								linphone_content_set_type(msg->file_transfer_information, type);
								linphone_content_set_subtype(msg->file_transfer_information, subtype);
								ms_free(subtype);
								ms_free(type);
								xmlFree(contentType);
							}
							if (!xmlStrcmp(cur->name, (const xmlChar *)"data")) {
								file_url = xmlGetProp(cur, (const xmlChar *)"url");
							}

							if (!xmlStrcmp(cur->name, (const xmlChar *)"file-key")) { 
								/* there is a key in the msg: file has been encrypted */
								/* convert the key from base 64 */
								xmlChar *keyb64 = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
								size_t keyLength = b64_decode((char *)keyb64, strlen((char *)keyb64), NULL, 0);
								uint8_t *keyBuffer = (uint8_t *)malloc(keyLength);
								/* decode the key into local key buffer */
								b64_decode((char *)keyb64, strlen((char *)keyb64), keyBuffer, keyLength);
								linphone_content_set_key(msg->file_transfer_information, (char *)keyBuffer, keyLength); 
								/* duplicate key value into the linphone content private structure */
								xmlFree(keyb64);
								free(keyBuffer);
							}

							cur = cur->next;
						}
						xmlFree(typeAttribute);
						break;
					}
					xmlFree(typeAttribute);
				}
				cur = cur->next;
			}
		}
		xmlFreeDoc(xmlMessageBody);

		linphone_chat_message_set_external_body_url(msg, (const char *)file_url);
		xmlFree(file_url);
	}

	msg->storage_id = linphone_chat_message_store(msg);

	if (cr->unread_count < 0)
		cr->unread_count = 1;
	else
		cr->unread_count++;

	linphone_chat_room_message_received(cr, lc, msg);
	
end:
	linphone_address_unref(addr);
	linphone_chat_message_unref(msg);
	return reason;
}

static int linphone_chat_room_remote_refresh_composing_expired(void *data, unsigned int revents) {
	LinphoneChatRoom *cr = (LinphoneChatRoom *)data;
	belle_sip_object_unref(cr->remote_composing_refresh_timer);
	cr->remote_composing_refresh_timer = NULL;
	cr->remote_is_composing = LinphoneIsComposingIdle;
	linphone_core_notify_is_composing_received(cr->lc, cr);
	return BELLE_SIP_STOP;
}

static const char *iscomposing_prefix = "/xsi:isComposing";

static void process_im_is_composing_notification(LinphoneChatRoom *cr, xmlparsing_context_t *xml_ctx) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr iscomposing_object;
	const char *state_str = NULL;
	const char *refresh_str = NULL;
	int refresh_duration = lp_config_get_int(cr->lc->config, "sip", "composing_remote_refresh_timeout",
											 COMPOSING_DEFAULT_REMOTE_REFRESH_TIMEOUT);
	int i;
	LinphoneIsComposingState state = LinphoneIsComposingIdle;

	if (linphone_create_xml_xpath_context(xml_ctx) < 0)
		return;

	xmlXPathRegisterNs(xml_ctx->xpath_ctx, (const xmlChar *)"xsi",
					   (const xmlChar *)"urn:ietf:params:xml:ns:im-iscomposing");
	iscomposing_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, iscomposing_prefix);
	if (iscomposing_object != NULL) {
		if (iscomposing_object->nodesetval != NULL) {
			for (i = 1; i <= iscomposing_object->nodesetval->nodeNr; i++) {
				snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/xsi:state", iscomposing_prefix, i);
				state_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
				if (state_str == NULL)
					continue;
				snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/xsi:refresh", iscomposing_prefix, i);
				refresh_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
			}
		}
		xmlXPathFreeObject(iscomposing_object);
	}

	if (state_str != NULL) {
		if (strcmp(state_str, "active") == 0) {
			state = LinphoneIsComposingActive;
			if (refresh_str != NULL) {
				refresh_duration = atoi(refresh_str);
			}
			if (!cr->remote_composing_refresh_timer) {
				cr->remote_composing_refresh_timer =
					sal_create_timer(cr->lc->sal, linphone_chat_room_remote_refresh_composing_expired, cr,
									 refresh_duration * 1000, "composing remote refresh timeout");
			} else {
				belle_sip_source_set_timeout(cr->remote_composing_refresh_timer, refresh_duration * 1000);
			}
		} else {
			linphone_chat_room_delete_remote_composing_refresh_timer(cr);
		}

		cr->remote_is_composing = state;
		linphone_core_notify_is_composing_received(cr->lc, cr);
		linphone_free_xml_text_content(state_str);
	}
	if (refresh_str != NULL) {
		linphone_free_xml_text_content(refresh_str);
	}
}

static void linphone_chat_room_notify_is_composing(LinphoneChatRoom *cr, const char *text) {
	xmlparsing_context_t *xml_ctx = linphone_xmlparsing_context_new();
	xmlSetGenericErrorFunc(xml_ctx, linphone_xmlparsing_genericxml_error);
	xml_ctx->doc = xmlReadDoc((const unsigned char *)text, 0, NULL, 0);
	if (xml_ctx->doc != NULL) {
		process_im_is_composing_notification(cr, xml_ctx);
	} else {
		ms_warning("Wrongly formatted presence XML: %s", xml_ctx->errorBuffer);
	}
	linphone_xmlparsing_context_destroy(xml_ctx);
}

LinphoneReason linphone_core_is_composing_received(LinphoneCore *lc, SalOp *op, const SalIsComposing *is_composing) {
	LinphoneAddress *addr = linphone_address_new(is_composing->from);
	LinphoneChatRoom *cr = _linphone_core_get_chat_room(lc, addr);
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(lc);
	LinphoneReason reason = LinphoneReasonNone;

	if (cr != NULL) {
		int retval = -1;
		LinphoneChatMessage *msg = linphone_chat_room_create_message(cr, is_composing->text);
		linphone_chat_message_set_from_address(msg, addr);
		msg->content_type = ms_strdup("application/im-iscomposing+xml");
		if (imee) {
			LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
			LinphoneImEncryptionEngineCbsIncomingMessageCb cb_process_incoming_message = linphone_im_encryption_engine_cbs_get_process_incoming_message(imee_cbs);
			if (cb_process_incoming_message) {
				retval = cb_process_incoming_message(imee, cr, msg);
			}
		}
		if (retval <= 0) {
			linphone_chat_room_notify_is_composing(cr, msg->message);
		} else {
			reason = linphone_error_code_to_reason(retval);
		}
		linphone_chat_message_unref(msg);
	}
	linphone_address_unref(addr);
	
	return reason;
}

bool_t linphone_chat_room_is_remote_composing(const LinphoneChatRoom *cr) {
	return (cr->remote_is_composing == LinphoneIsComposingActive) ? TRUE : FALSE;
}

static const char *imdn_prefix = "/imdn:imdn";

static void process_imdn(LinphoneChatRoom *cr, xmlparsing_context_t *xml_ctx) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr imdn_object;
	xmlXPathObjectPtr delivery_status_object;
	xmlXPathObjectPtr display_status_object;
	const char *message_id_str = NULL;
	const char *datetime_str = NULL;
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(lc);

	if (linphone_create_xml_xpath_context(xml_ctx) < 0)
		return;

	xmlXPathRegisterNs(xml_ctx->xpath_ctx, (const xmlChar *)"imdn",
					   (const xmlChar *)"urn:ietf:params:xml:ns:imdn");
	imdn_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, imdn_prefix);
	if (imdn_object != NULL) {
		if ((imdn_object->nodesetval != NULL) && (imdn_object->nodesetval->nodeNr >= 1)) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[1]/imdn:message-id", imdn_prefix);
			message_id_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
			snprintf(xpath_str, sizeof(xpath_str), "%s[1]/imdn:datetime", imdn_prefix);
			datetime_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
		}
		xmlXPathFreeObject(imdn_object);
	}

	if ((message_id_str != NULL) && (datetime_str != NULL)) {
		LinphoneChatMessage *cm = linphone_chat_room_find_message(cr, message_id_str);
		if (cm == NULL) {
			ms_warning("Received IMDN for unknown message %s", message_id_str);
		} else {
			snprintf(xpath_str, sizeof(xpath_str), "%s[1]/imdn:delivery-notification/imdn:status", imdn_prefix);
			delivery_status_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
			snprintf(xpath_str, sizeof(xpath_str), "%s[1]/imdn:display-notification/imdn:status", imdn_prefix);
			display_status_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
			if ((delivery_status_object != NULL) && (linphone_im_notif_policy_get_recv_imdn_delivered(policy) == TRUE)) {
				if ((delivery_status_object->nodesetval != NULL) && (delivery_status_object->nodesetval->nodeNr >= 1)) {
					xmlNodePtr node = delivery_status_object->nodesetval->nodeTab[0];
					if ((node->children != NULL) && (node->children->name != NULL)) {
						if (strcmp((const char *)node->children->name, "delivered") == 0) {
							linphone_chat_message_update_state(cm, LinphoneChatMessageStateDeliveredToUser);
						} else if (strcmp((const char *)node->children->name, "error") == 0) {
							linphone_chat_message_update_state(cm, LinphoneChatMessageStateNotDelivered);
						}
					}
				}
				xmlXPathFreeObject(delivery_status_object);
			}
			if ((display_status_object != NULL) && (linphone_im_notif_policy_get_recv_imdn_displayed(policy) == TRUE)) {
				if ((display_status_object->nodesetval != NULL) && (display_status_object->nodesetval->nodeNr >= 1)) {
					xmlNodePtr node = display_status_object->nodesetval->nodeTab[0];
					if ((node->children != NULL) && (node->children->name != NULL)) {
						if (strcmp((const char *)node->children->name, "displayed") == 0) {
							linphone_chat_message_update_state(cm, LinphoneChatMessageStateDisplayed);
						}
					}
				}
				xmlXPathFreeObject(display_status_object);
			}
			linphone_chat_message_unref(cm);
		}
	}
	if (message_id_str != NULL) linphone_free_xml_text_content(message_id_str);
	if (datetime_str != NULL) linphone_free_xml_text_content(datetime_str);
}

static void linphone_chat_room_notify_imdn(LinphoneChatRoom *cr, const char *text) {
	xmlparsing_context_t *xml_ctx = linphone_xmlparsing_context_new();
	xmlSetGenericErrorFunc(xml_ctx, linphone_xmlparsing_genericxml_error);
	xml_ctx->doc = xmlReadDoc((const unsigned char *)text, 0, NULL, 0);
	if (xml_ctx->doc != NULL) {
		process_imdn(cr, xml_ctx);
	} else {
		ms_warning("Wrongly formatted IMDN XML: %s", xml_ctx->errorBuffer);
	}
	linphone_xmlparsing_context_destroy(xml_ctx);
}

LinphoneReason linphone_core_imdn_received(LinphoneCore *lc, SalOp *op, const SalImdn *imdn) {
	LinphoneAddress *addr = linphone_address_new(imdn->from);
	LinphoneChatRoom *cr = _linphone_core_get_chat_room(lc, addr);
	if (cr != NULL) {
		linphone_chat_room_notify_imdn(cr, imdn->content);
	}
	linphone_address_unref(addr);
	return LinphoneReasonNone;
}

LinphoneCore *linphone_chat_room_get_lc(LinphoneChatRoom *cr) {
	return linphone_chat_room_get_core(cr);
}

LinphoneCore *linphone_chat_room_get_core(LinphoneChatRoom *cr) {
	return cr->lc;
}

const LinphoneAddress *linphone_chat_room_get_peer_address(LinphoneChatRoom *cr) {
	return cr->peer_url;
}

LinphoneChatMessage *linphone_chat_room_create_message(LinphoneChatRoom *cr, const char *message) {
	LinphoneChatMessage *msg = belle_sip_object_new(LinphoneChatMessage);
	msg->state = LinphoneChatMessageStateIdle;
	msg->callbacks = linphone_chat_message_cbs_new();
	msg->chat_room = (LinphoneChatRoom *)cr;
	msg->message = message ? ms_strdup(message) : NULL;
	msg->content_type = NULL;			   /* this property is used only when transfering file */
	msg->file_transfer_information = NULL; /* this property is used only when transfering file */
	msg->http_request = NULL;
	msg->time = ms_time(0);
	return msg;
}

LinphoneChatMessage *linphone_chat_room_create_message_2(LinphoneChatRoom *cr, const char *message,
														 const char *external_body_url, LinphoneChatMessageState state,
														 time_t time, bool_t is_read, bool_t is_incoming) {
	LinphoneChatMessage *msg = linphone_chat_room_create_message(cr, message);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	msg->external_body_url = external_body_url ? ms_strdup(external_body_url) : NULL;
	msg->time = time;
	linphone_chat_message_set_state(msg, state);
	if (is_incoming) {
		msg->dir = LinphoneChatMessageIncoming;
		linphone_chat_message_set_from(msg, linphone_chat_room_get_peer_address(cr));
		msg->to = linphone_address_new(linphone_core_get_identity(lc)); /*direct assignment*/
	} else {
		msg->dir = LinphoneChatMessageOutgoing;
		linphone_chat_message_set_to(msg, linphone_chat_room_get_peer_address(cr));
		msg->from = linphone_address_new(linphone_core_get_identity(lc));/*direct assignment*/
	}
	return msg;
}

void linphone_chat_room_send_message2(LinphoneChatRoom *cr, LinphoneChatMessage *msg,
									  LinphoneChatMessageStateChangedCb status_cb, void *ud) {
	msg->message_state_changed_cb = status_cb;
	msg->message_state_changed_user_data = ud;
	_linphone_chat_room_send_message(cr, msg);
}

void linphone_chat_room_send_chat_message_2(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	linphone_chat_message_ref(msg);
	_linphone_chat_room_send_message(cr, msg);
}

void linphone_chat_room_send_chat_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	_linphone_chat_room_send_message(cr, msg);
}

static char *linphone_chat_room_create_is_composing_xml(LinphoneChatRoom *cr) {
	xmlBufferPtr buf;
	xmlTextWriterPtr writer;
	int err;
	char *content = NULL;

	buf = xmlBufferCreate();
	if (buf == NULL) {
		ms_error("Error creating the XML buffer");
		return content;
	}
	writer = xmlNewTextWriterMemory(buf, 0);
	if (writer == NULL) {
		ms_error("Error creating the XML writer");
		return content;
	}

	err = xmlTextWriterStartDocument(writer, "1.0", "UTF-8", NULL);
	if (err >= 0) {
		err = xmlTextWriterStartElementNS(writer, NULL, (const xmlChar *)"isComposing",
										  (const xmlChar *)"urn:ietf:params:xml:ns:im-iscomposing");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xmlns", (const xmlChar *)"xsi", NULL,
											(const xmlChar *)"http://www.w3.org/2001/XMLSchema-instance");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xsi", (const xmlChar *)"schemaLocation", NULL,
											(const xmlChar *)"urn:ietf:params:xml:ns:im-composing iscomposing.xsd");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"state",
										(cr->is_composing == LinphoneIsComposingActive) ? (const xmlChar *)"active"
																						: (const xmlChar *)"idle");
	}
	if ((err >= 0) && (cr->is_composing == LinphoneIsComposingActive)) {
		char refresh_str[4] = {0};
		int refresh_timeout =
			lp_config_get_int(cr->lc->config, "sip", "composing_refresh_timeout", COMPOSING_DEFAULT_REFRESH_TIMEOUT);
		snprintf(refresh_str, sizeof(refresh_str), "%u", refresh_timeout);
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"refresh", (const xmlChar *)refresh_str);
	}
	if (err >= 0) {
		/* Close the "isComposing" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		err = xmlTextWriterEndDocument(writer);
	}
	if (err > 0) {
		/* xmlTextWriterEndDocument returns the size of the content. */
		content = ms_strdup((char *)buf->content);
	}
	xmlFreeTextWriter(writer);
	xmlBufferFree(buf);
	return content;
}

static void linphone_chat_room_send_is_composing_notification(LinphoneChatRoom *cr) {
	SalOp *op = NULL;
	const char *identity = NULL;
	char *content = NULL;
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(lc);
	if (linphone_im_notif_policy_get_send_is_composing(policy) == TRUE) {
		LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(lc, cr->peer_url);
		LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(lc);
		LinphoneChatMessage *msg = NULL;

		if (proxy)
			identity = linphone_proxy_config_get_identity(proxy);
		else
			identity = linphone_core_get_primary_contact(lc);
		/*sending out of calls*/
		op = sal_op_new(lc->sal);
		linphone_configure_op(lc, op, cr->peer_url, NULL,
							lp_config_get_int(lc->config, "sip", "chat_msg_with_contact", 0));

		content = linphone_chat_room_create_is_composing_xml(cr);
		if (content != NULL) {
			int retval = -1;
			LinphoneAddress *from_addr = linphone_address_new(identity);
			LinphoneAddress *to_addr = linphone_address_new(cr->peer);
			msg = linphone_chat_room_create_message(cr, content);
			linphone_chat_message_set_from_address(msg, from_addr);
			linphone_chat_message_set_to_address(msg, to_addr);
			msg->content_type = ms_strdup("application/im-iscomposing+xml");

			if (imee) {
				LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsOutgoingMessageCb cb_process_outgoing_message = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imee_cbs);
				if (cb_process_outgoing_message) {
					retval = cb_process_outgoing_message(imee, cr, msg);
				}
			}

			if (retval <= 0) {
				sal_message_send(op, identity, cr->peer, msg->content_type, msg->message, NULL);
			}

			linphone_chat_message_unref(msg);
			linphone_address_unref(from_addr);
			linphone_address_unref(to_addr);
			ms_free(content);
			sal_op_unref(op);
		}
	}
}

enum ImdnType {
	ImdnTypeDelivery,
	ImdnTypeDisplay
};

static char *linphone_chat_message_create_imdn_xml(LinphoneChatMessage *cm, enum ImdnType imdn_type, LinphoneReason reason) {
	xmlBufferPtr buf;
	xmlTextWriterPtr writer;
	int err;
	char *content = NULL;
	char *datetime = NULL;

	buf = xmlBufferCreate();
	if (buf == NULL) {
		ms_error("Error creating the XML buffer");
		return content;
	}
	writer = xmlNewTextWriterMemory(buf, 0);
	if (writer == NULL) {
		ms_error("Error creating the XML writer");
		return content;
	}

	datetime = linphone_timestamp_to_rfc3339_string(linphone_chat_message_get_time(cm));
	err = xmlTextWriterStartDocument(writer, "1.0", "UTF-8", NULL);
	if (err >= 0) {
		err = xmlTextWriterStartElementNS(writer, NULL, (const xmlChar *)"imdn",
										  (const xmlChar *)"urn:ietf:params:xml:ns:imdn");
	}
	if ((err >= 0) && (reason != LinphoneReasonNone)) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xmlns", (const xmlChar *)"linphoneimdn", NULL, (const xmlChar *)"http://www.linphone.org/xsds/imdn.xsd");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"message-id", (const xmlChar *)linphone_chat_message_get_message_id(cm));
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"datetime", (const xmlChar *)datetime);
	}
	if (err >= 0) {
		if (imdn_type == ImdnTypeDelivery) {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"delivery-notification");
		} else {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"display-notification");
		}
	}
	if (err >= 0) {
		err = xmlTextWriterStartElement(writer, (const xmlChar *)"status");
	}
	if (err >= 0) {
		if (reason == LinphoneReasonNone) {
			if (imdn_type == ImdnTypeDelivery) {
				err = xmlTextWriterStartElement(writer, (const xmlChar *)"delivered");
			} else {
				err = xmlTextWriterStartElement(writer, (const xmlChar *)"displayed");
			}
		} else {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"error");
		}
	}
	if (err >= 0) {
		/* Close the "delivered", "displayed" or "error" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if ((err >= 0) && (reason != LinphoneReasonNone)) {
		err = xmlTextWriterStartElementNS(writer, (const xmlChar *)"linphoneimdn", (const xmlChar *)"reason", NULL);
		if (err >= 0) {
			char codestr[16];
			snprintf(codestr, 16, "%d", linphone_reason_to_error_code(reason));
			err = xmlTextWriterWriteAttribute(writer, (const xmlChar *)"code", (const xmlChar *)codestr);
		}
		if (err >= 0) {
			err = xmlTextWriterWriteString(writer, (const xmlChar *)linphone_reason_to_string(reason));
		}
		if (err >= 0) {
			err = xmlTextWriterEndElement(writer);
		}
	}
	if (err >= 0) {
		/* Close the "status" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		/* Close the "delivery-notification" or "display-notification" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		/* Close the "imdn" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		err = xmlTextWriterEndDocument(writer);
	}
	if (err > 0) {
		/* xmlTextWriterEndDocument returns the size of the content. */
		content = ms_strdup((char *)buf->content);
	}
	xmlFreeTextWriter(writer);
	xmlBufferFree(buf);
	ms_free(datetime);
	return content;
}

static void linphone_chat_message_send_imdn(LinphoneChatMessage *cm, enum ImdnType imdn_type, LinphoneReason reason) {
	SalOp *op = NULL;
	const char *identity = NULL;
	char *content = NULL;
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(cm);
	LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(cr->lc, cr->peer_url);
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(cr->lc);
	LinphoneChatMessage *msg;

	if (proxy)
		identity = linphone_proxy_config_get_identity(proxy);
	else
		identity = linphone_core_get_primary_contact(cr->lc);
	/* Sending out of calls */
	op = sal_op_new(cr->lc->sal);
	linphone_configure_op(cr->lc, op, cr->peer_url, NULL,
		lp_config_get_int(cr->lc->config, "sip", "chat_msg_with_contact", 0));

	content = linphone_chat_message_create_imdn_xml(cm, imdn_type, reason);
	if (content != NULL) {
		int retval = -1;
		LinphoneAddress *from_addr = linphone_address_new(identity);
		LinphoneAddress *to_addr = linphone_address_new(cr->peer);
		msg = linphone_chat_room_create_message(cr, content);
		linphone_chat_message_set_from_address(msg, from_addr);
		linphone_chat_message_set_to_address(msg, to_addr);
		msg->content_type = ms_strdup("message/imdn+xml");

		/* Do not try to encrypt the notification when it is reporting an error (maybe it should be bypassed only for some reasons). */
		if (imee && (reason == LinphoneReasonNone)) {
			LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
			LinphoneImEncryptionEngineCbsOutgoingMessageCb cb_process_outgoing_message = linphone_im_encryption_engine_cbs_get_process_outgoing_message(imee_cbs);
			if (cb_process_outgoing_message) {
				retval = cb_process_outgoing_message(imee, cr, msg);
			}
		}

		if (retval <= 0) {
			sal_message_send(op, identity, cr->peer, msg->content_type, msg->message, NULL);
		}
		
		linphone_chat_message_unref(msg);
		linphone_address_unref(from_addr);
		linphone_address_unref(to_addr);
		ms_free(content);
	}
	sal_op_unref(op);
}

void linphone_chat_message_send_delivery_notification(LinphoneChatMessage *cm, LinphoneReason reason) {
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(cm);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(lc);
	if (linphone_im_notif_policy_get_send_imdn_delivered(policy) == TRUE) {
		linphone_chat_message_send_imdn(cm, ImdnTypeDelivery, reason);
	}
}

void linphone_chat_message_send_display_notification(LinphoneChatMessage *cm) {
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(cm);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(lc);
	if (linphone_im_notif_policy_get_send_imdn_displayed(policy) == TRUE) {
		linphone_chat_message_send_imdn(cm, ImdnTypeDisplay, LinphoneReasonNone);
	}
}

static char* utf8_to_char(uint32_t ic) {
	char *result = ms_malloc(sizeof(char) * 5);
	int size = 0;
	if (ic < 0x80) {
		result[0] = ic;
		size = 1;
	} else if (ic < 0x800) {
		result[1] = 0x80 + ((ic & 0x3F));
		result[0] = 0xC0 + ((ic >> 6) & 0x1F);
		size = 2;
	} else if (ic < 0x100000) {
		result[2] = 0x80 + (ic & 0x3F);
		result[1] = 0x80 + ((ic >> 6) & 0x3F);
		result[0] = 0xE0 + ((ic >> 12) & 0xF);
		size = 3;
	} else if (ic < 0x110000) {
		result[3] = 0x80 + (ic & 0x3F);
		result[2] = 0x80 + ((ic >> 6) & 0x3F);
		result[1] = 0x80 + ((ic >> 12) & 0x3F);
		result[0] = 0xF0 + ((ic >> 18) & 0x7);
		size = 4;
	}
	result[size] = '\0';
	return result;
}

void linphone_core_real_time_text_received(LinphoneCore *lc, LinphoneChatRoom *cr, uint32_t character, LinphoneCall *call) {
	uint32_t new_line = 0x2028;
	uint32_t crlf = 0x0D0A;
	uint32_t lf = 0x0A;

	if (call && linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(call))) {
		LinphoneChatMessageCharacter *cmc = ms_new0(LinphoneChatMessageCharacter, 1);

		if (cr->pending_message == NULL) {
			cr->pending_message = linphone_chat_room_create_message(cr, "");
		}

		cmc->value = character;
		cmc->has_been_read = FALSE;
		cr->received_rtt_characters = bctbx_list_append(cr->received_rtt_characters, (void *)cmc);

		cr->remote_is_composing = LinphoneIsComposingActive;
		linphone_core_notify_is_composing_received(cr->lc, cr);

		if (character == new_line || character == crlf || character == lf) {
			// End of message
			LinphoneChatMessage *msg = cr->pending_message;
			ms_debug("New line received, forge a message with content %s", cr->pending_message->message);

			linphone_chat_message_set_from(msg, cr->peer_url);
			if (msg->to)
				linphone_address_unref(msg->to);
			msg->to = call->dest_proxy ? linphone_address_clone(call->dest_proxy->identity_address) :
					linphone_address_new(linphone_core_get_identity(lc));
			msg->time = ms_time(0);
			msg->state = LinphoneChatMessageStateDelivered;
			msg->dir = LinphoneChatMessageIncoming;

			if (lp_config_get_int(lc->config, "misc", "store_rtt_messages", 1) == 1) {
				msg->storage_id = linphone_chat_message_store(msg);
			}

			if (cr->unread_count < 0) cr->unread_count = 1;
			else cr->unread_count++;

			linphone_chat_room_message_received(cr, lc, msg);
			linphone_chat_message_unref(msg);
			cr->pending_message = NULL;
			cr->received_rtt_characters = bctbx_list_free_with_data(cr->received_rtt_characters, (void (*)(void *))ms_free);
		} else {
			char *value = utf8_to_char(character);
			cr->pending_message->message = ms_strcat_printf(cr->pending_message->message, value);
			ms_debug("Received RTT character: %s (%lu), pending text is %s", value, (unsigned long)character, cr->pending_message->message);
			ms_free(value);
		}
	}
}

uint32_t linphone_chat_room_get_char(const LinphoneChatRoom *cr) {
	if (cr && cr->received_rtt_characters) {
		bctbx_list_t *characters = cr->received_rtt_characters;
		while (characters != NULL) {
			LinphoneChatMessageCharacter *cmc = (LinphoneChatMessageCharacter *)characters->data;
			if (!cmc->has_been_read) {
				cmc->has_been_read = TRUE;
				return cmc->value;
			}
			characters = bctbx_list_next(characters);
		}
	}
	return 0;
}

int linphone_chat_message_put_char(LinphoneChatMessage *msg, uint32_t character) {
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(msg);
	LinphoneCall *call = cr->call;
	LinphoneCore *lc = cr->lc;
	uint32_t new_line = 0x2028;
	uint32_t crlf = 0x0D0A;
	uint32_t lf = 0x0A;

	if (!call || !call->textstream) {
		return -1;
	}

	if (character == new_line || character == crlf || character == lf) {
		if (lc && lp_config_get_int(lc->config, "misc", "store_rtt_messages", 1) == 1) {
			ms_debug("New line sent, forge a message with content %s", msg->message);
			msg->time = ms_time(0);
			msg->state = LinphoneChatMessageStateDisplayed;
			msg->dir = LinphoneChatMessageOutgoing;
			if (msg->from) linphone_address_unref(msg->from);
			msg->from = linphone_address_new(linphone_core_get_identity(lc));
			msg->storage_id = linphone_chat_message_store(msg);
			ms_free(msg->message);
			msg->message = NULL;
		}
	} else {
		char *value = utf8_to_char(character);
		msg->message = ms_strcat_printf(msg->message, value);
		ms_debug("Sent RTT character: %s (%lu), pending text is %s", value, (unsigned long)character, msg->message);
		ms_free(value);
	}

	text_stream_putchar32(call->textstream, character);
	return 0;
}

const char* linphone_chat_message_get_message_id(const LinphoneChatMessage *cm) {
	return cm->message_id;
}

static int linphone_chat_room_stop_composing(void *data, unsigned int revents) {
	LinphoneChatRoom *cr = (LinphoneChatRoom *)data;
	cr->is_composing = LinphoneIsComposingIdle;
	linphone_chat_room_send_is_composing_notification(cr);
	linphone_chat_room_delete_composing_refresh_timer(cr);
	belle_sip_object_unref(cr->composing_idle_timer);
	cr->composing_idle_timer = NULL;
	return BELLE_SIP_STOP;
}

static int linphone_chat_room_refresh_composing(void *data, unsigned int revents) {
	LinphoneChatRoom *cr = (LinphoneChatRoom *)data;
	linphone_chat_room_send_is_composing_notification(cr);
	return BELLE_SIP_CONTINUE;
}

void linphone_chat_room_compose(LinphoneChatRoom *cr) {
	int idle_timeout =
		lp_config_get_int(cr->lc->config, "sip", "composing_idle_timeout", COMPOSING_DEFAULT_IDLE_TIMEOUT);
	int refresh_timeout =
		lp_config_get_int(cr->lc->config, "sip", "composing_refresh_timeout", COMPOSING_DEFAULT_REFRESH_TIMEOUT);
	if (cr->is_composing == LinphoneIsComposingIdle) {
		cr->is_composing = LinphoneIsComposingActive;
		linphone_chat_room_send_is_composing_notification(cr);
		if (!cr->composing_refresh_timer) {
			cr->composing_refresh_timer = sal_create_timer(cr->lc->sal, linphone_chat_room_refresh_composing, cr,
														   refresh_timeout * 1000, "composing refresh timeout");
		} else {
			belle_sip_source_set_timeout(cr->composing_refresh_timer, refresh_timeout * 1000);
		}
		if (!cr->composing_idle_timer) {
			cr->composing_idle_timer = sal_create_timer(cr->lc->sal, linphone_chat_room_stop_composing, cr,
														idle_timeout * 1000, "composing idle timeout");
		}
	}
	belle_sip_source_set_timeout(cr->composing_idle_timer, idle_timeout * 1000);
}

const char *linphone_chat_message_state_to_string(const LinphoneChatMessageState state) {
	switch (state) {
	case LinphoneChatMessageStateIdle:
		return "LinphoneChatMessageStateIdle";
	case LinphoneChatMessageStateInProgress:
		return "LinphoneChatMessageStateInProgress";
	case LinphoneChatMessageStateDelivered:
		return "LinphoneChatMessageStateDelivered";
	case LinphoneChatMessageStateNotDelivered:
		return "LinphoneChatMessageStateNotDelivered";
	case LinphoneChatMessageStateFileTransferError:
		return "LinphoneChatMessageStateFileTransferError";
	case LinphoneChatMessageStateFileTransferDone:
		return "LinphoneChatMessageStateFileTransferDone ";
	case LinphoneChatMessageStateDeliveredToUser:
		return "LinphoneChatMessageStateDeliveredToUser";
	case LinphoneChatMessageStateDisplayed:
		return "LinphoneChatMessageStateDisplayed";
	}
	return NULL;
}

LinphoneChatRoom *linphone_chat_message_get_chat_room(LinphoneChatMessage *msg) {
	return msg->chat_room;
}

const LinphoneAddress *linphone_chat_message_get_peer_address(LinphoneChatMessage *msg) {
	return linphone_chat_room_get_peer_address(msg->chat_room);
}

void linphone_chat_message_set_user_data(LinphoneChatMessage *msg, void *ud) {
	msg->message_userdata = ud;
}

void *linphone_chat_message_get_user_data(const LinphoneChatMessage *msg) {
	return msg->message_userdata;
}

const char *linphone_chat_message_get_external_body_url(const LinphoneChatMessage *msg) {
	return msg->external_body_url;
}

void linphone_chat_message_set_external_body_url(LinphoneChatMessage *msg, const char *url) {
	if (msg->external_body_url) {
		ms_free(msg->external_body_url);
	}
	msg->external_body_url = url ? ms_strdup(url) : NULL;
}

const char *linphone_chat_message_get_appdata(const LinphoneChatMessage *msg) {
	return msg->appdata;
}

void linphone_chat_message_set_appdata(LinphoneChatMessage *msg, const char *data) {
	if (msg->appdata) {
		ms_free(msg->appdata);
	}
	msg->appdata = data ? ms_strdup(data) : NULL;
	linphone_chat_message_store_appdata(msg);
}

void linphone_chat_message_set_from_address(LinphoneChatMessage *msg, const LinphoneAddress *from) {
	if (msg->from)
		linphone_address_unref(msg->from);
	msg->from = linphone_address_clone(from);
}

const LinphoneAddress *linphone_chat_message_get_from_address(const LinphoneChatMessage *msg) {
	return msg->from;
}

void linphone_chat_message_set_to_address(LinphoneChatMessage *msg, const LinphoneAddress *to) {
	if (msg->to)
		linphone_address_unref(msg->to);
	msg->to = linphone_address_clone(to);
}

const LinphoneAddress *linphone_chat_message_get_to_address(const LinphoneChatMessage *msg) {
	if (msg->to)
		return msg->to;
	if (msg->dir == LinphoneChatMessageOutgoing) {
		return msg->chat_room->peer_url;
	}
	return NULL;
}

LinphoneAddress *linphone_chat_message_get_local_address(const LinphoneChatMessage *msg) {
	return msg->dir == LinphoneChatMessageOutgoing ? msg->from : msg->to;
}

time_t linphone_chat_message_get_time(const LinphoneChatMessage *msg) {
	return msg->time;
}

LinphoneChatMessageState linphone_chat_message_get_state(const LinphoneChatMessage *msg) {
	return msg->state;
}

const char *linphone_chat_message_get_text(const LinphoneChatMessage *msg) {
	return msg->message;
}

void linphone_chat_message_add_custom_header(LinphoneChatMessage *msg, const char *header_name,
											 const char *header_value) {
	msg->custom_headers = sal_custom_header_append(msg->custom_headers, header_name, header_value);
}

const char *linphone_chat_message_get_custom_header(LinphoneChatMessage *msg, const char *header_name) {
	return sal_custom_header_find(msg->custom_headers, header_name);
}

void linphone_chat_message_remove_custom_header(LinphoneChatMessage *msg, const char *header_name) {
	msg->custom_headers = sal_custom_header_remove(msg->custom_headers, header_name);
}

bool_t linphone_chat_message_is_read(LinphoneChatMessage *msg) {
	return (msg->state == LinphoneChatMessageStateDisplayed) ? TRUE : FALSE;
}

bool_t linphone_chat_message_is_outgoing(LinphoneChatMessage *msg) {
	return msg->dir == LinphoneChatMessageOutgoing;
}

unsigned int linphone_chat_message_get_storage_id(LinphoneChatMessage *msg) {
	return msg->storage_id;
}

LinphoneChatMessage *linphone_chat_message_clone(const LinphoneChatMessage *msg) {
	/*struct _LinphoneChatMessage {
	 char* msg;
	 LinphoneChatRoom* chat_room;
	 LinphoneChatMessageStateChangeCb cb;
	 void* cb_ud;
	 void* message_userdata;
	 char* external_body_url;
	 LinphoneAddress* from;
	 time_t time;
	 SalCustomHeader *custom_headers;
	 LinphoneChatMessageState state;
	 };*/
	LinphoneChatMessage *new_message = linphone_chat_room_create_message(msg->chat_room, msg->message);
	if (msg->external_body_url)
		new_message->external_body_url = ms_strdup(msg->external_body_url);
	if (msg->appdata)
		new_message->appdata = ms_strdup(msg->appdata);
	new_message->message_state_changed_cb = msg->message_state_changed_cb;
	new_message->message_state_changed_user_data = msg->message_state_changed_user_data;
	new_message->message_userdata = msg->message_userdata;
	new_message->time = msg->time;
	new_message->state = msg->state;
	new_message->storage_id = msg->storage_id;
	if (msg->from)
		new_message->from = linphone_address_clone(msg->from);
	if (msg->file_transfer_filepath)
		new_message->file_transfer_filepath = ms_strdup(msg->file_transfer_filepath);
	if (msg->file_transfer_information)
		new_message->file_transfer_information = linphone_content_copy(msg->file_transfer_information);
	return new_message;
}

void linphone_chat_message_destroy(LinphoneChatMessage *msg) {
	belle_sip_object_unref(msg);
}

static void _linphone_chat_message_destroy(LinphoneChatMessage *msg) {
	if (msg->op)
		sal_op_release(msg->op);
	if (msg->message)
		ms_free(msg->message);
	if (msg->external_body_url)
		ms_free(msg->external_body_url);
	if (msg->appdata)
		ms_free(msg->appdata);
	if (msg->from)
		linphone_address_unref(msg->from);
	if (msg->to)
		linphone_address_unref(msg->to);
	if (msg->message_id)
		ms_free(msg->message_id);
	if (msg->custom_headers)
		sal_custom_header_free(msg->custom_headers);
	if (msg->content_type)
		ms_free(msg->content_type);
	if (msg->file_transfer_information) {
		linphone_content_unref(msg->file_transfer_information);
	}
	if (msg->file_transfer_filepath != NULL) {
		ms_free(msg->file_transfer_filepath);
	}
	if (msg->callbacks) {
		linphone_chat_message_cbs_unref(msg->callbacks);
	}
}

LinphoneChatMessage *linphone_chat_message_ref(LinphoneChatMessage *msg) {
	belle_sip_object_ref(msg);
	return msg;
}

void linphone_chat_message_unref(LinphoneChatMessage *msg) {
	belle_sip_object_unref(msg);
}

static void linphone_chat_message_release(LinphoneChatMessage *msg) {
	/*mark the chat msg as orphan (it has no chat room anymore), and unref it*/
	msg->chat_room = NULL;
	if (msg->file_transfer_information != NULL) {
		linphone_chat_message_cancel_file_transfer(msg);
	}
	linphone_chat_message_unref(msg);
}

const LinphoneErrorInfo *linphone_chat_message_get_error_info(const LinphoneChatMessage *msg) {
	return linphone_error_info_from_sal_op(msg->op);
}

LinphoneReason linphone_chat_message_get_reason(LinphoneChatMessage *msg) {
	return linphone_error_info_get_reason(linphone_chat_message_get_error_info(msg));
}

LinphoneChatMessageCbs *linphone_chat_message_get_callbacks(const LinphoneChatMessage *msg) {
	return msg->callbacks;
}

LinphoneCall *linphone_chat_room_get_call(const LinphoneChatRoom *room) {
	return room->call;
}
