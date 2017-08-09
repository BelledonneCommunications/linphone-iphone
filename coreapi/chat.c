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
#include "linphone/wrapper_utils.h"

#include "chat/chat-room.h"
#include "chat/chat-room-p.h"
#include "utils/content-type.h"
#include "utils/utils.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>

struct _LinphoneChatRoom{
	belle_sip_object_t base;
	void *user_data;
	LinphonePrivate::ChatRoom *cr;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneChatRoom);

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

void linphone_chat_message_set_state(LinphoneChatMessage *msg, LinphoneChatMessageState state) {
	/* do not invoke callbacks on orphan messages */
	if (state != msg->state && msg->chat_room != NULL) {
		if (((msg->state == LinphoneChatMessageStateDisplayed) || (msg->state == LinphoneChatMessageStateDeliveredToUser))
			&& ((state == LinphoneChatMessageStateDeliveredToUser) || (state == LinphoneChatMessageStateDelivered) || (state == LinphoneChatMessageStateNotDelivered))) {
			/* If the message has been displayed or delivered to user we must not go back to the delivered or not delivered state. */
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
	return linphone_address_weak_equal(cr->cr->getPeerAddress(), from);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneChatRoom);

static void _linphone_chat_room_destroy(LinphoneChatRoom *cr) {
	delete cr->cr;
}

BELLE_SIP_INSTANCIATE_VPTR(LinphoneChatRoom, belle_sip_object_t,
						   (belle_sip_object_destroy_t)_linphone_chat_room_destroy,
						   NULL, // clone
						   NULL, // marshal
						   FALSE);

LinphoneChatRoom *_linphone_core_create_chat_room_base(LinphoneCore *lc, LinphoneAddress *addr){
	LinphoneChatRoom *cr = belle_sip_object_new(LinphoneChatRoom);
	cr->cr = new LinphonePrivate::ChatRoom(lc, addr);
	cr->cr->getPrivate()->setCBackPointer(cr);
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
	linphone_chat_room_set_call(cr, call);
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
		lc->chatrooms = bctbx_list_remove(lc->chatrooms, cr);
		linphone_chat_room_delete_history(cr);
		linphone_chat_room_unref(cr);
	} else {
		ms_error("linphone_core_delete_chat_room(): chatroom [%p] isn't part of LinphoneCore.", cr);
	}
}

LinphoneChatRoom *linphone_core_get_chat_room_from_uri(LinphoneCore *lc, const char *to) {
	return _linphone_core_get_or_create_chat_room(lc, to);
}

void linphone_chat_room_destroy(LinphoneChatRoom *cr) {
	linphone_chat_room_unref(cr);
}

void linphone_chat_room_release(LinphoneChatRoom *cr) {
	cr->cr->getPrivate()->release();
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

void linphone_chat_room_remove_transient_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	cr->cr->getPrivate()->removeTransientMessage(msg);
}

void linphone_chat_message_update_state(LinphoneChatMessage *msg, LinphoneChatMessageState new_state) {
	linphone_chat_message_set_state(msg, new_state);
	linphone_chat_message_store_state(msg);

	if (msg->state == LinphoneChatMessageStateDelivered || msg->state == LinphoneChatMessageStateNotDelivered) {
		msg->chat_room->cr->getPrivate()->moveTransientMessageToWeakMessages(msg);
	}
}

void linphone_chat_room_send_message(LinphoneChatRoom *cr, const char *msg) {
	cr->cr->sendMessage(cr->cr->createMessage(msg));
}

void create_file_transfer_information_from_vnd_gsma_rcs_ft_http_xml(LinphoneChatMessage *msg) {
	xmlChar *file_url = NULL;
	xmlDocPtr xmlMessageBody;
	xmlNodePtr cur;
	/* parse the msg body to get all informations from it */
	xmlMessageBody = xmlParseDoc((const xmlChar *)msg->message);
	msg->file_transfer_information = linphone_content_new();

	cur = xmlDocGetRootElement(xmlMessageBody);
	if (cur != NULL) {
		cur = cur->xmlChildrenNode;
		while (cur != NULL) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"file-info")) {
				/* we found a file info node, check if it has a type="file" attribute */
				xmlChar *typeAttribute = xmlGetProp(cur, (const xmlChar *)"type");
				if (!xmlStrcmp(typeAttribute, (const xmlChar *)"file")) { /* this is the node we are looking for */
					cur = cur->xmlChildrenNode; /* now loop on the content of the file-info node */
					while (cur != NULL) {
						if (!xmlStrcmp(cur->name, (const xmlChar *)"file-size")) {
							xmlChar *fileSizeString = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
							linphone_content_set_size(msg->file_transfer_information, strtol((const char *)fileSizeString, NULL, 10));
							xmlFree(fileSizeString);
						}

						if (!xmlStrcmp(cur->name, (const xmlChar *)"file-name")) {
							xmlChar *filename = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
							linphone_content_set_name(msg->file_transfer_information, (char *)filename);
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
							size_t keyLength = b64::b64_decode((char *)keyb64, strlen((char *)keyb64), NULL, 0);
							uint8_t *keyBuffer = (uint8_t *)malloc(keyLength);
							/* decode the key into local key buffer */
							b64::b64_decode((char *)keyb64, strlen((char *)keyb64), keyBuffer, keyLength);
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

int linphone_core_message_received(LinphoneCore *lc, SalOp *op, const SalMessage *sal_msg) {
	LinphoneAddress *addr = linphone_address_new(sal_msg->from);
	linphone_address_clean(addr);
	LinphoneChatRoom *cr = linphone_core_get_chat_room(lc, addr);
	LinphoneReason reason = cr->cr->getPrivate()->messageReceived(op, sal_msg);
	linphone_address_unref(addr);
	return reason;
}

bool_t linphone_chat_room_is_remote_composing(const LinphoneChatRoom *cr) {
	return cr->cr->isRemoteComposing();
}

LinphoneCore *linphone_chat_room_get_lc(LinphoneChatRoom *cr) {
	return linphone_chat_room_get_core(cr);
}

LinphoneCore *linphone_chat_room_get_core(LinphoneChatRoom *cr) {
	return cr->cr->getCore();
}

const LinphoneAddress *linphone_chat_room_get_peer_address(LinphoneChatRoom *cr) {
	return cr->cr->getPeerAddress();
}

LinphoneChatMessage *linphone_chat_room_create_message(LinphoneChatRoom *cr, const char *message) {
	return cr->cr->createMessage(message ? message : "");
}

LinphoneChatMessage *linphone_chat_room_create_message_2(LinphoneChatRoom *cr, const char *message,
														 const char *external_body_url, LinphoneChatMessageState state,
														 time_t time, bool_t is_read, bool_t is_incoming) {
	LinphoneChatMessage *msg = linphone_chat_room_create_message(cr, message);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	msg->external_body_url = external_body_url ? ms_strdup(external_body_url) : NULL;
	msg->time = time;
	msg->is_secured = FALSE;
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
	cr->cr->sendMessage(msg);
}

void linphone_chat_room_send_chat_message_2(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	linphone_chat_message_ref(msg);
	cr->cr->sendMessage(msg);
}

void linphone_chat_room_send_chat_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	cr->cr->sendMessage(msg);
}

LinphonePrivate::ChatRoom& linphone_chat_room_get_cpp_obj(LinphoneChatRoom *cr) {
	return *cr->cr;
}

void _linphone_chat_message_resend(LinphoneChatMessage *msg, bool_t ref_msg) {
	LinphoneChatMessageState state = linphone_chat_message_get_state(msg);
	LinphoneChatRoom *cr;

	if (state != LinphoneChatMessageStateNotDelivered) {
		ms_warning("Cannot resend chat message in state %s", linphone_chat_message_state_to_string(state));
		return;
	}

	cr = linphone_chat_message_get_chat_room(msg);
	if (ref_msg) linphone_chat_message_ref(msg);
	cr->cr->sendMessage(msg);
}

void linphone_chat_message_resend(LinphoneChatMessage *msg) {
	_linphone_chat_message_resend(msg, FALSE);
}

void linphone_chat_message_resend_2(LinphoneChatMessage *msg) {
	_linphone_chat_message_resend(msg, TRUE);
}

static char *linphone_chat_message_create_imdn_xml(LinphoneChatMessage *cm, ImdnType imdn_type, LinphoneReason reason) {
	xmlBufferPtr buf;
	xmlTextWriterPtr writer;
	int err;
	char *content = NULL;
	char *datetime = NULL;
	const char *message_id;

	/* Check that the chat message has a message id */
	message_id = linphone_chat_message_get_message_id(cm);
	if (message_id == NULL) return NULL;

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
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"message-id", (const xmlChar *)message_id);
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

void linphone_chat_message_send_imdn(LinphoneChatMessage *cm, ImdnType imdn_type, LinphoneReason reason) {
	char *content = linphone_chat_message_create_imdn_xml(cm, imdn_type, reason);
	if (content) {
		linphone_chat_message_get_chat_room(cm)->cr->getPrivate()->sendImdn(content, reason);
		ms_free(content);
	}
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

void linphone_core_real_time_text_received(LinphoneCore *lc, LinphoneChatRoom *cr, uint32_t character, LinphoneCall *call) {
	cr->cr->getPrivate()->realtimeTextReceived(character, call);
}

uint32_t linphone_chat_room_get_char(const LinphoneChatRoom *cr) {
	return cr->cr->getChar();
}

LinphoneStatus linphone_chat_message_put_char(LinphoneChatMessage *msg, uint32_t character) {
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(msg);
	LinphoneCall *call = cr->cr->getCall();
	LinphoneCore *lc = cr->cr->getCore();
	const uint32_t new_line = 0x2028;
	const uint32_t crlf = 0x0D0A;
	const uint32_t lf = 0x0A;

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
		char *value = LinphonePrivate::Utils::utf8ToChar(character);
		msg->message = ms_strcat_printf(msg->message, value);
		ms_debug("Sent RTT character: %s (%lu), pending text is %s", value, (unsigned long)character, msg->message);
		delete value;
	}

	text_stream_putchar32(call->textstream, character);
	return 0;
}

const char* linphone_chat_message_get_message_id(const LinphoneChatMessage *cm) {
	return cm->message_id;
}

void linphone_chat_room_compose(LinphoneChatRoom *cr) {
	cr->cr->compose();
}

LinphoneCall *linphone_chat_room_get_call(const LinphoneChatRoom *room) {
	return room->cr->getCall();
}

void linphone_chat_room_set_call(LinphoneChatRoom *cr, LinphoneCall *call) {
	cr->cr->getPrivate()->setCall(call);
}

bctbx_list_t * linphone_chat_room_get_transient_messages(const LinphoneChatRoom *cr) {
	std::list<LinphoneChatMessage *> l = cr->cr->getPrivate()->getTransientMessages();
	bctbx_list_t *result = nullptr;
	for (auto it = l.begin(); it != l.end(); it++)
		result = bctbx_list_append(result, *it);
	return result;
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

const char * linphone_chat_message_get_content_type(const LinphoneChatMessage *msg) {
	return msg->content_type;
}

void linphone_chat_message_set_content_type(LinphoneChatMessage *msg, const char *content_type) {
	if (msg->content_type) {
		ms_free(msg->content_type);
	}
	msg->content_type = content_type ? ms_strdup(content_type) : NULL;
}

bool_t linphone_chat_message_is_file_transfer(const LinphoneChatMessage *msg) {
	return LinphonePrivate::ContentType::isFileTransfer(msg->content_type);
}

bool_t linphone_chat_message_is_text(const LinphoneChatMessage *msg) {
	return LinphonePrivate::ContentType::isText(msg->content_type);
}

bool_t linphone_chat_message_get_to_be_stored(const LinphoneChatMessage *msg) {
	return msg->to_be_stored;
}

void linphone_chat_message_set_to_be_stored(LinphoneChatMessage *msg, bool_t to_be_stored) {
	msg->to_be_stored = to_be_stored;
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
		return msg->chat_room->cr->getPeerAddress();
	}
	return NULL;
}

void linphone_chat_message_set_is_secured(LinphoneChatMessage *msg, bool_t secured) {
	msg->is_secured = secured;
}

bool_t linphone_chat_message_is_secured(LinphoneChatMessage *msg) {
	return msg->is_secured;
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

int linphone_chat_message_set_text(LinphoneChatMessage *msg, const char* text) {
	if (msg->message)
		ms_free(msg->message);
	if (text)
		msg->message = ms_strdup(text);
	else
		msg->message = NULL;
	
	return 0;
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
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(msg);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(lc);
	if ((linphone_im_notif_policy_get_recv_imdn_displayed(policy) == TRUE) && (msg->state == LinphoneChatMessageStateDisplayed)) return TRUE;
	if ((linphone_im_notif_policy_get_recv_imdn_delivered(policy) == TRUE) && (msg->state == LinphoneChatMessageStateDeliveredToUser || msg->state == LinphoneChatMessageStateDisplayed)) return TRUE;
	return (msg->state == LinphoneChatMessageStateDelivered || msg->state == LinphoneChatMessageStateDisplayed || msg->state == LinphoneChatMessageStateDeliveredToUser);
}

bool_t linphone_chat_message_is_outgoing(LinphoneChatMessage *msg) {
	return msg->dir == LinphoneChatMessageOutgoing;
}

unsigned int linphone_chat_message_get_storage_id(LinphoneChatMessage *msg) {
	return msg->storage_id;
}

LinphoneChatMessage *linphone_chat_message_clone(const LinphoneChatMessage *msg) {
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
	if (msg->ei)
		linphone_error_info_unref(msg->ei);
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

void linphone_chat_message_deactivate(LinphoneChatMessage *msg){
	if (msg->file_transfer_information != NULL) {
		_linphone_chat_message_cancel_file_transfer(msg, FALSE);
	}
	/*mark the chat msg as orphan (it has no chat room anymore)*/
	msg->chat_room = NULL;
}

void linphone_chat_message_release(LinphoneChatMessage *msg) {
	linphone_chat_message_deactivate(msg);
	linphone_chat_message_unref(msg);
}

const LinphoneErrorInfo *linphone_chat_message_get_error_info(const LinphoneChatMessage *msg) {
	if (!msg->ei) ((LinphoneChatMessage*)msg)->ei = linphone_error_info_new(); /*let's do it mutable*/
	linphone_error_info_from_sal_op(msg->ei, msg->op);
	return msg->ei;
}

LinphoneReason linphone_chat_message_get_reason(LinphoneChatMessage *msg) {
	return linphone_error_info_get_reason(linphone_chat_message_get_error_info(msg));
}

LinphoneChatMessageCbs *linphone_chat_message_get_callbacks(const LinphoneChatMessage *msg) {
	return msg->callbacks;
}
