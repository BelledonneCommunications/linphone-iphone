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

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>

#include <linphone/utils/utils.h>

#include "linphone/core.h"
#include "private.h"
#include "linphone/lpconfig.h"
#include "belle-sip/belle-sip.h"
#include "ortp/b64.h"
#include "linphone/wrapper_utils.h"

#include "c-wrapper/c-wrapper.h"
#include "chat/basic-chat-room.h"
#include "chat/client-group-chat-room.h"
#include "chat/real-time-text-chat-room.h"
#include "chat/real-time-text-chat-room-p.h"
#include "content/content-type.h"

using namespace std;

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

static LinphoneChatRoom *_linphone_core_create_chat_room(LinphoneCore *lc, const LinphoneAddress *addr) {
	LinphoneChatRoom *cr = linphone_chat_room_new(lc, addr);
	lc->chatrooms = bctbx_list_append(lc->chatrooms, (void *)cr);
	return cr;
}

LinphoneChatRoom *_linphone_core_create_chat_room_from_call(LinphoneCall *call){
	LinphoneChatRoom *cr = linphone_chat_room_new(linphone_call_get_core(call),
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

static bool_t linphone_chat_room_matches(LinphoneChatRoom *cr, const LinphoneAddress *from) {
	LinphoneAddress *addr = linphone_address_new(L_GET_CPP_PTR_FROM_C_OBJECT(cr)->getPeerAddress().asString().c_str());
	bool_t result = linphone_address_weak_equal(addr, from);
	linphone_address_unref(addr);
	return result;
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
		ret = _linphone_core_create_chat_room(lc, addr);
	}
	return ret;
}

LinphoneChatRoom * linphone_core_create_client_group_chat_room(LinphoneCore *lc, const char *subject) {
	LinphoneChatRoom *cr = _linphone_client_group_chat_room_new(lc, subject);
	lc->chatrooms = bctbx_list_append(lc->chatrooms, cr);
	return cr;
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

void create_file_transfer_information_from_vnd_gsma_rcs_ft_http_xml(LinphoneChatMessage *msg) {
	xmlChar *file_url = NULL;
	xmlDocPtr xmlMessageBody;
	xmlNodePtr cur;
	/* parse the msg body to get all informations from it */
	xmlMessageBody = xmlParseDoc((const xmlChar *)linphone_chat_message_get_text(msg));
	LinphoneContent *content = linphone_content_new();
	linphone_chat_message_set_file_transfer_information(msg, content);

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
							linphone_content_set_size(content, (size_t)strtol((const char *)fileSizeString, NULL, 10));
							xmlFree(fileSizeString);
						}

						if (!xmlStrcmp(cur->name, (const xmlChar *)"file-name")) {
							xmlChar *filename = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
							linphone_content_set_name(content, (char *)filename);
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
							linphone_content_set_type(content, type);
							linphone_content_set_subtype(content, subtype);
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
							linphone_content_set_key(content, (char *)keyBuffer, keyLength);
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
	LinphoneReason reason = L_GET_PRIVATE_FROM_C_OBJECT(cr)->messageReceived(op, sal_msg);
	linphone_address_unref(addr);
	return reason;
}



void linphone_core_real_time_text_received(LinphoneCore *lc, LinphoneChatRoom *cr, uint32_t character, LinphoneCall *call) {
	if (linphone_core_realtime_text_enabled(lc)) {
		shared_ptr<LinphonePrivate::RealTimeTextChatRoom> rttcr =
			static_pointer_cast<LinphonePrivate::RealTimeTextChatRoom>(L_GET_CPP_PTR_FROM_C_OBJECT(cr));
		L_GET_PRIVATE(rttcr)->realtimeTextReceived(character, call);
		//L_GET_PRIVATE(static_pointer_cast<LinphonePrivate::RealTimeTextChatRoom>(L_GET_CPP_PTR_FROM_C_OBJECT(cr)))->realtimeTextReceived(character, call);
	}
}
