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
 
#include "linphonecore.h"
#include "private.h"
#include "lpconfig.h"

#include <libxml/xmlwriter.h>

#define COMPOSING_DEFAULT_IDLE_TIMEOUT 15
#define COMPOSING_DEFAULT_REFRESH_TIMEOUT 60
#define COMPOSING_DEFAULT_REMOTE_REFRESH_TIMEOUT 120

/**
 * @addtogroup chatroom
 * @{
 */

/**
 * Returns an array of chat rooms
 * @param lc #LinphoneCore object
 * @return An array of #LinpĥoneChatRoom
**/
MSList* linphone_core_get_chat_rooms(LinphoneCore *lc) {
	return lc->chatrooms;
}

/**
 * Create a new chat room for messaging from a sip uri like sip:joe@sip.linphone.org
 * @param lc #LinphoneCore object
 * @param to destination address for messages
 * @return #LinphoneChatRoom where messaging can take place.
 */
LinphoneChatRoom * linphone_core_create_chat_room(LinphoneCore *lc, const char *to){
	LinphoneAddress *parsed_url=NULL;

	if ((parsed_url=linphone_core_interpret_url(lc,to))!=NULL){
		LinphoneChatRoom *cr=ms_new0(LinphoneChatRoom,1);
		cr->lc=lc;
		cr->peer=linphone_address_as_string(parsed_url);
		cr->peer_url=parsed_url;
		lc->chatrooms=ms_list_append(lc->chatrooms,(void *)cr);
		return cr;
	}
	return NULL;
}

bool_t linphone_chat_room_matches(LinphoneChatRoom *cr, const LinphoneAddress *from){
	return linphone_address_weak_equal(cr->peer_url,from);
}

/**
 * Create a new chat room for messaging from a sip uri like sip:joe@sip.linphone.org if not already existing, else return exisiting one
 * @param lc #LinphoneCore object
 * @param to destination address for messages
 * @return #LinphoneChatRoom where messaging can take place.
 */
LinphoneChatRoom* linphone_core_get_or_create_chat_room(LinphoneCore* lc, const char* to) {
	LinphoneAddress *to_addr=linphone_core_interpret_url(lc,to);
	LinphoneChatRoom *ret;
	
	if (to_addr==NULL){
		ms_error("linphone_core_get_or_create_chat_room(): Cannot make a valid address with %s",to);
		return NULL;
	}
	ret=linphone_core_get_chat_room(lc,to_addr);
	linphone_address_destroy(to_addr);
	if (!ret){
		ret=linphone_core_create_chat_room(lc,to);
	}
	return ret;
}

static void linphone_chat_room_delete_composing_idle_timer(LinphoneChatRoom *cr) {
	if (cr->composing_idle_timer) {
		sal_cancel_timer(cr->lc->sal, cr->composing_idle_timer);
		belle_sip_object_unref(cr->composing_idle_timer);
		cr->composing_idle_timer = NULL;
	}
}

static void linphone_chat_room_delete_composing_refresh_timer(LinphoneChatRoom *cr) {
	if (cr->composing_refresh_timer) {
		sal_cancel_timer(cr->lc->sal, cr->composing_refresh_timer);
		belle_sip_object_unref(cr->composing_refresh_timer);
		cr->composing_refresh_timer = NULL;
	}
}

static void linphone_chat_room_delete_remote_composing_refresh_timer(LinphoneChatRoom *cr) {
	if (cr->remote_composing_refresh_timer) {
		sal_cancel_timer(cr->lc->sal, cr->remote_composing_refresh_timer);
		belle_sip_object_unref(cr->remote_composing_refresh_timer);
		cr->remote_composing_refresh_timer = NULL;
	}
}

/**
 * Destroy a LinphoneChatRoom.
 * @param cr #LinphoneChatRoom object
 */
void linphone_chat_room_destroy(LinphoneChatRoom *cr){
	LinphoneCore *lc=cr->lc;
	linphone_chat_room_delete_composing_idle_timer(cr);
	linphone_chat_room_delete_composing_refresh_timer(cr);
	linphone_chat_room_delete_remote_composing_refresh_timer(cr);
	lc->chatrooms=ms_list_remove(lc->chatrooms,(void *) cr);
	linphone_address_destroy(cr->peer_url);
	ms_free(cr->peer);
	ms_free(cr);
}



static void _linphone_chat_room_send_message(LinphoneChatRoom *cr, LinphoneChatMessage* msg){
	SalOp *op=NULL;
	LinphoneCall *call;
	char* content_type;
	const char *identity=NULL;
	time_t t=time(NULL);
	
	if (lp_config_get_int(cr->lc->config,"sip","chat_use_call_dialogs",0)){
		if((call = linphone_core_get_call_by_remote_address(cr->lc,cr->peer))!=NULL){
			if (call->state==LinphoneCallConnected ||
			call->state==LinphoneCallStreamsRunning ||
			call->state==LinphoneCallPaused ||
			call->state==LinphoneCallPausing ||
			call->state==LinphoneCallPausedByRemote){
				ms_message("send SIP message through the existing call.");
				op = call->op;
				call->pending_message=msg;
				identity=linphone_core_find_best_identity(cr->lc,linphone_call_get_remote_address(call));
			}
		}
	}
	msg->time=t;
	if (op==NULL){
		LinphoneProxyConfig *proxy=linphone_core_lookup_known_proxy(cr->lc,cr->peer_url);
		if (proxy){
			identity=linphone_proxy_config_get_identity(proxy);
		}else identity=linphone_core_get_primary_contact(cr->lc);
		/*sending out of calls*/
		op = sal_op_new(cr->lc->sal);
		linphone_configure_op(cr->lc,op,cr->peer_url,msg->custom_headers,lp_config_get_int(cr->lc->config,"sip","chat_msg_with_contact",0));
		sal_op_set_user_pointer(op, msg); /*if out of call, directly store msg*/
	}
	if (msg->external_body_url) {
		content_type=ms_strdup_printf("message/external-body; access-type=URL; URL=\"%s\"",msg->external_body_url);
		sal_message_send(op,identity,cr->peer,content_type, NULL);
		ms_free(content_type);
	} else {
		sal_text_send(op, identity, cr->peer,msg->message);
	}
	msg->dir=LinphoneChatMessageOutgoing;
	msg->from=linphone_address_new(identity);
	msg->storage_id=linphone_chat_message_store(msg);

	if (cr->is_composing == LinphoneIsComposingActive) {
		cr->is_composing = LinphoneIsComposingIdle;
	}
	linphone_chat_room_delete_composing_idle_timer(cr);
	linphone_chat_room_delete_composing_refresh_timer(cr);
}

/**
 * Send a message to peer member of this chat room.
 * @deprecated linphone_chat_room_send_message2() gives more control on the message expedition.
 * @param cr #LinphoneChatRoom object
 * @param msg message to be sent
 */
void linphone_chat_room_send_message(LinphoneChatRoom *cr, const char *msg) {
	_linphone_chat_room_send_message(cr,linphone_chat_room_create_message(cr,msg));
}

void linphone_chat_room_message_received(LinphoneChatRoom *cr, LinphoneCore *lc, LinphoneChatMessage *msg){
	if (msg->message)
		//legacy API
		if (lc->vtable.text_received!=NULL) lc->vtable.text_received(lc, cr, msg->from, msg->message);
	if (lc->vtable.message_received!=NULL) lc->vtable.message_received(lc, cr,msg);
	if (cr->lc->vtable.is_composing_received != NULL) {
		cr->remote_is_composing = LinphoneIsComposingIdle;
		cr->lc->vtable.is_composing_received(cr->lc, cr);
	}
}

/**
 * Retrieve an existing chat room whose peer is the supplied address, if exists.
 * @param lc the linphone core
 * @param add a linphone address.
 * @returns the matching chatroom, or NULL if no such chatroom exists.
**/
LinphoneChatRoom *linphone_core_get_chat_room(LinphoneCore *lc, const LinphoneAddress *addr){
	LinphoneChatRoom *cr=NULL;
	MSList *elem;
	for(elem=lc->chatrooms;elem!=NULL;elem=ms_list_next(elem)){
		cr=(LinphoneChatRoom*)elem->data;
		if (linphone_chat_room_matches(cr,addr)){
			break;
		}
		cr=NULL;
	}
	return cr;
}

void linphone_core_message_received(LinphoneCore *lc, SalOp *op, const SalMessage *sal_msg){
	
	LinphoneChatRoom *cr=NULL;
	LinphoneAddress *addr;
	char *cleanfrom;
	char *from;
	LinphoneChatMessage* msg;
	const SalCustomHeader *ch;
	
	addr=linphone_address_new(sal_msg->from);
	linphone_address_clean(addr);
	cr=linphone_core_get_chat_room(lc,addr);
	cleanfrom=linphone_address_as_string(addr);
	from=linphone_address_as_string_uri_only(addr);
	if (cr==NULL){
		/* create a new chat room */
		cr=linphone_core_create_chat_room(lc,cleanfrom);
	}
	msg = linphone_chat_room_create_message(cr, sal_msg->text);
	linphone_chat_message_set_from(msg, cr->peer_url);
	
	{
		LinphoneAddress *to;
		to=sal_op_get_to(op) ? linphone_address_new(sal_op_get_to(op)) : linphone_address_new(linphone_core_get_identity(lc));
		msg->to=to;
	}
	
	msg->time=sal_msg->time;
	msg->state=LinphoneChatMessageStateDelivered;
	msg->is_read=FALSE;
	msg->dir=LinphoneChatMessageIncoming;
	ch=sal_op_get_recv_custom_header(op);
	if (ch) msg->custom_headers=sal_custom_header_clone(ch);
	
	if (sal_msg->url) {
		linphone_chat_message_set_external_body_url(msg, sal_msg->url);
	}
	linphone_address_destroy(addr);
	msg->storage_id=linphone_chat_message_store(msg);
	linphone_chat_room_message_received(cr,lc,msg);
	ms_free(cleanfrom);
	ms_free(from);
}

static int linphone_chat_room_remote_refresh_composing_expired(void *data, unsigned int revents) {
	LinphoneChatRoom *cr = (LinphoneChatRoom *)data;
	belle_sip_object_unref(cr->remote_composing_refresh_timer);
	cr->remote_composing_refresh_timer = NULL;
	cr->remote_is_composing = LinphoneIsComposingIdle;
	if (cr->lc->vtable.is_composing_received != NULL)
		cr->lc->vtable.is_composing_received(cr->lc, cr);
	return BELLE_SIP_STOP;
}

static const char *iscomposing_prefix = "/xsi:isComposing";

static void process_im_is_composing_notification(LinphoneChatRoom *cr, xmlparsing_context_t *xml_ctx) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr iscomposing_object;
	const char *state_str = NULL;
	const char *refresh_str = NULL;
	int refresh_duration = lp_config_get_int(cr->lc->config, "sip", "composing_remote_refresh_timeout", COMPOSING_DEFAULT_REMOTE_REFRESH_TIMEOUT);
	int i;
	LinphoneIsComposingState state = LinphoneIsComposingIdle;

	if (linphone_create_xml_xpath_context(xml_ctx) < 0) return;

	xmlXPathRegisterNs(xml_ctx->xpath_ctx, (const xmlChar *)"xsi", (const xmlChar *)"urn:ietf:params:xml:ns:im-iscomposing");
	iscomposing_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, iscomposing_prefix);
	if ((iscomposing_object != NULL) && (iscomposing_object->nodesetval != NULL)) {
		for (i = 1; i <= iscomposing_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/xsi:state", iscomposing_prefix, i);
			state_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
			if (state_str == NULL) continue;
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/xsi:refresh", iscomposing_prefix, i);
			refresh_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
		}
	}

	if (state_str != NULL) {
		if (strcmp(state_str, "active") == 0) {
			state = LinphoneIsComposingActive;
			if (refresh_str != NULL) {
				refresh_duration = atoi(refresh_str);
			}
			if (!cr->remote_composing_refresh_timer) {
				cr->remote_composing_refresh_timer = sal_create_timer(cr->lc->sal, linphone_chat_room_remote_refresh_composing_expired, cr, refresh_duration * 1000, "composing remote refresh timeout");
			} else {
				belle_sip_source_set_timeout(cr->remote_composing_refresh_timer, refresh_duration * 1000);
			}
		} else {
			linphone_chat_room_delete_remote_composing_refresh_timer(cr);
		}

		cr->remote_is_composing = state;
		if (cr->lc->vtable.is_composing_received != NULL)
			cr->lc->vtable.is_composing_received(cr->lc, cr);
	}
}

static void linphone_chat_room_notify_is_composing(LinphoneChatRoom *cr, const char *text) {
	xmlparsing_context_t *xml_ctx = linphone_xmlparsing_context_new();
	xmlSetGenericErrorFunc(xml_ctx, linphone_xmlparsing_genericxml_error);
	xml_ctx->doc = xmlReadDoc((const unsigned char*)text, 0, NULL, 0);
	if (xml_ctx->doc != NULL) {
		process_im_is_composing_notification(cr, xml_ctx);
	} else {
		ms_warning("Wrongly formatted presence XML: %s", xml_ctx->errorBuffer);
	}
	linphone_xmlparsing_context_destroy(xml_ctx);
}

void linphone_core_is_composing_received(LinphoneCore *lc, SalOp *op, const SalIsComposing *is_composing) {
	LinphoneChatRoom *cr = linphone_core_get_or_create_chat_room(lc, is_composing->from);
	if (cr != NULL) {
		linphone_chat_room_notify_is_composing(cr, is_composing->text);
	}
}

bool_t linphone_chat_room_is_remote_composing(const LinphoneChatRoom *cr) {
	return (cr->remote_is_composing == LinphoneIsComposingActive) ? TRUE : FALSE;
}

/**
 * Returns back pointer to LinphoneCore object.
**/
LinphoneCore* linphone_chat_room_get_lc(LinphoneChatRoom *cr){
	return cr->lc;
}

/**
 * Assign a user pointer to the chat room.
**/
void linphone_chat_room_set_user_data(LinphoneChatRoom *cr, void * ud){
	cr->user_data=ud;
}

/**
 * Retrieve the user pointer associated with the chat room.
**/
void * linphone_chat_room_get_user_data(LinphoneChatRoom *cr){
	return cr->user_data;
}

/**
 * get peer address \link linphone_core_create_chat_room() associated to \endlink this #LinphoneChatRoom
 * @param cr #LinphoneChatRoom object
 * @return #LinphoneAddress peer address
 */
const LinphoneAddress* linphone_chat_room_get_peer_address(LinphoneChatRoom *cr) {
	return cr->peer_url;
}

/**
 * Create a message attached to a dedicated chat room;
 * @param cr the chat room.
 * @param message text message, NULL if absent.
 * @return a new #LinphoneChatMessage
 */
LinphoneChatMessage* linphone_chat_room_create_message(LinphoneChatRoom *cr, const char* message) {
	LinphoneChatMessage* msg = ms_new0(LinphoneChatMessage,1);
	msg->chat_room=(LinphoneChatRoom*)cr;
	msg->message=message?ms_strdup(message):NULL;
	msg->is_read=TRUE;
	return msg;
}

/**
 * Create a message attached to a dedicated chat room;
 * @param cr the chat room.
 * @param message text message, NULL if absent.
 * @param external_body_url the URL given in external body or NULL.
 * @param state the LinphoneChatMessage.State of the message.
 * @param time the time_t at which the message has been received/sent.
 * @param is_read TRUE if the message should be flagged as read, FALSE otherwise.
 * @param is_incoming TRUE if the message has been received, FALSE otherwise.
 * @return a new #LinphoneChatMessage
 */
LinphoneChatMessage* linphone_chat_room_create_message_2(
        LinphoneChatRoom *cr, const char* message, const char* external_body_url,
        LinphoneChatMessageState state, time_t time, bool_t is_read, bool_t is_incoming) {
	LinphoneCore *lc=linphone_chat_room_get_lc(cr);

	LinphoneChatMessage* msg = ms_new0(LinphoneChatMessage,1);
	msg->chat_room=(LinphoneChatRoom*)cr;
	msg->message=message?ms_strdup(message):NULL;
	msg->external_body_url=external_body_url?ms_strdup(external_body_url):NULL;
	msg->time=time;
	msg->state=state;
	msg->is_read=is_read;
	if (is_incoming) {
		msg->dir=LinphoneChatMessageIncoming;
		linphone_chat_message_set_from(msg, linphone_chat_room_get_peer_address(cr));
		linphone_chat_message_set_to(msg, linphone_address_new(linphone_core_get_identity(lc)));
	} else {
		msg->dir=LinphoneChatMessageOutgoing;
		linphone_chat_message_set_to(msg, linphone_chat_room_get_peer_address(cr));
		linphone_chat_message_set_from(msg, linphone_address_new(linphone_core_get_identity(lc)));
	}
	return msg;
}

/**
 * Send a message to peer member of this chat room.
 * @param cr #LinphoneChatRoom object
 * @param msg #LinphoneChatMessage message to be sent
 * @param status_cb LinphoneChatMessageStateChangeCb status callback invoked when message is delivered or could not be delivered. May be NULL
 * @param ud user data for the status cb.
 * @note The LinphoneChatMessage must not be destroyed until the the callback is called.
 */
void linphone_chat_room_send_message2(LinphoneChatRoom *cr, LinphoneChatMessage* msg,LinphoneChatMessageStateChangedCb status_cb, void* ud) {
	msg->cb=status_cb;
	msg->cb_ud=ud;
	msg->state=LinphoneChatMessageStateInProgress;
	_linphone_chat_room_send_message(cr, msg);
}

static char * linphone_chat_room_create_is_composing_xml(LinphoneChatRoom *cr) {
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
		err = xmlTextWriterStartElementNS(writer, NULL, (const xmlChar *)"isComposing", (const xmlChar *)"urn:ietf:params:xml:ns:im-iscomposing");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xmlns", (const xmlChar *)"xsi",
			NULL, (const xmlChar *)"http://www.w3.org/2001/XMLSchema-instance");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xsi", (const xmlChar *)"schemaLocation",
			NULL, (const xmlChar *)"urn:ietf:params:xml:ns:im-composing iscomposing.xsd");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"state",
			(cr->is_composing == LinphoneIsComposingActive) ? (const xmlChar *)"active" : (const xmlChar *)"idle");
	}
	if ((err >= 0) && (cr->is_composing == LinphoneIsComposingActive)) {
		char refresh_str[4] = { 0 };
		int refresh_timeout = lp_config_get_int(cr->lc->config, "sip", "composing_refresh_timeout", COMPOSING_DEFAULT_REFRESH_TIMEOUT);
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
	LinphoneCall *call;
	const char *identity = NULL;
	char *content = NULL;

	if (lp_config_get_int(cr->lc->config, "sip", "chat_use_call_dialogs", 0)) {
		if ((call = linphone_core_get_call_by_remote_address(cr->lc, cr->peer)) != NULL) {
			if (call->state == LinphoneCallConnected ||
			call->state == LinphoneCallStreamsRunning ||
			call->state == LinphoneCallPaused ||
			call->state == LinphoneCallPausing ||
			call->state == LinphoneCallPausedByRemote) {
				ms_message("send SIP message through the existing call.");
				op = call->op;
				identity = linphone_core_find_best_identity(cr->lc, linphone_call_get_remote_address(call));
			}
		}
	}
	if (op == NULL) {
		LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(cr->lc, cr->peer_url);
		if (proxy)
			identity = linphone_proxy_config_get_identity(proxy);
		else
			identity = linphone_core_get_primary_contact(cr->lc);
		/*sending out of calls*/
		op = sal_op_new(cr->lc->sal);
		linphone_configure_op(cr->lc, op, cr->peer_url, NULL, lp_config_get_int(cr->lc->config, "sip", "chat_msg_with_contact", 0));
	}
	content = linphone_chat_room_create_is_composing_xml(cr);
	if (content != NULL) {
		sal_message_send(op, identity, cr->peer, "application/im-iscomposing+xml", content);
		ms_free(content);
	}
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
	int idle_timeout = lp_config_get_int(cr->lc->config, "sip", "composing_idle_timeout", COMPOSING_DEFAULT_IDLE_TIMEOUT);
	int refresh_timeout = lp_config_get_int(cr->lc->config, "sip", "composing_refresh_timeout", COMPOSING_DEFAULT_REFRESH_TIMEOUT);
	if (cr->is_composing == LinphoneIsComposingIdle) {
		cr->is_composing = LinphoneIsComposingActive;
		linphone_chat_room_send_is_composing_notification(cr);
		if (!cr->composing_refresh_timer) {
			cr->composing_refresh_timer = sal_create_timer(cr->lc->sal, linphone_chat_room_refresh_composing, cr, refresh_timeout * 1000, "composing refresh timeout");
		} else {
			belle_sip_source_set_timeout(cr->composing_refresh_timer, refresh_timeout * 1000);
		}
		if (!cr->composing_idle_timer) {
			cr->composing_idle_timer = sal_create_timer(cr->lc->sal, linphone_chat_room_stop_composing, cr, idle_timeout * 1000, "composing idle timeout");
		}
	}
	belle_sip_source_set_timeout(cr->composing_idle_timer, idle_timeout * 1000);
}

/**
 * Returns a #LinphoneChatMessageState as a string.
 */
const char* linphone_chat_message_state_to_string(const LinphoneChatMessageState state) {
	switch (state) {
		case LinphoneChatMessageStateIdle:return "LinphoneChatMessageStateIdle"; 
		case LinphoneChatMessageStateInProgress:return "LinphoneChatMessageStateInProgress";
		case LinphoneChatMessageStateDelivered:return "LinphoneChatMessageStateDelivered";
		case LinphoneChatMessageStateNotDelivered:return "LinphoneChatMessageStateNotDelivered";
		default: return "Unknown state";
	}
	
}

/**
 * Returns the chatroom this message belongs to.
**/
LinphoneChatRoom* linphone_chat_message_get_chat_room(LinphoneChatMessage *msg){
	return msg->chat_room;
}

/**
 * Returns the peer (remote) address for the message.
**/
const LinphoneAddress* linphone_chat_message_get_peer_address(LinphoneChatMessage *msg) {
	return linphone_chat_room_get_peer_address(msg->chat_room);
}

/**
 *User pointer set function
 */
void linphone_chat_message_set_user_data(LinphoneChatMessage* message,void* ud) {
	message->message_userdata=ud;
}

/**
 * User pointer get function
 */
void* linphone_chat_message_get_user_data(const LinphoneChatMessage* message) {
	return message->message_userdata;
}

/**
 * Linphone message can carry external body as defined by rfc2017
 * @param message #LinphoneChatMessage
 * @return external body url or NULL if not present.
 */
const char* linphone_chat_message_get_external_body_url(const LinphoneChatMessage* message) {
	return message->external_body_url;
}

/**
 * Linphone message can carry external body as defined by rfc2017
 * 
 * @param message a LinphoneChatMessage  
 * @param url ex: access-type=URL; URL="http://www.foo.com/file"
 */
void linphone_chat_message_set_external_body_url(LinphoneChatMessage* message,const char* url) {
	if (message->external_body_url) {
		ms_free(message->external_body_url);
	}
	message->external_body_url=url?ms_strdup(url):NULL;
}

/**
 * Set origin of the message
 *@param message #LinphoneChatMessage obj
 *@param from #LinphoneAddress origin of this message (copied)
 */
void linphone_chat_message_set_from(LinphoneChatMessage* message, const LinphoneAddress* from) {
	if(message->from) linphone_address_destroy(message->from);
	message->from=linphone_address_clone(from);
}

/**
 * Get origin of the message 
 *@param message #LinphoneChatMessage obj
 *@return #LinphoneAddress
 */
const LinphoneAddress* linphone_chat_message_get_from(const LinphoneChatMessage* message) {
	return message->from;
}

/**
 * Set destination of the message
 *@param message #LinphoneChatMessage obj
 *@param to #LinphoneAddress destination of this message (copied)
 */
void linphone_chat_message_set_to(LinphoneChatMessage* message, const LinphoneAddress* to) {
	if(message->to) linphone_address_destroy(message->to);
	message->to=linphone_address_clone(to);
}

/**
 * Get destination of the message 
 *@param message #LinphoneChatMessage obj
 *@return #LinphoneAddress
 */
const LinphoneAddress* linphone_chat_message_get_to(const LinphoneChatMessage* message){
	if (message->to) return message->to;
	if (message->dir==LinphoneChatMessageOutgoing){
		return message->chat_room->peer_url;
	}
	return NULL;
}

/**
 * Returns the origin address of a message if it was a outgoing message, or the destination address if it was an incoming message.
 *@param message #LinphoneChatMessage obj
 *@return #LinphoneAddress
 */
LinphoneAddress *linphone_chat_message_get_local_address(const LinphoneChatMessage* message){
	return message->dir==LinphoneChatMessageOutgoing ? message->from : message->to;
}

/**
 * Get the time the message was sent.
 */
time_t linphone_chat_message_get_time(const LinphoneChatMessage* message) {
	return message->time;
}

/**
 * Get the state of the message
 *@param message #LinphoneChatMessage obj
 *@return #LinphoneChatMessageState
 */
LinphoneChatMessageState linphone_chat_message_get_state(const LinphoneChatMessage* message) {
	return message->state;
}

/**
 * Get text part of this message
 * @return text or NULL if no text.
 */
const char * linphone_chat_message_get_text(const LinphoneChatMessage* message) {
	return message->message;
}

/**
 * Add custom headers to the message.
 * @param message the message
 * @param header_name name of the header_name
 * @param header_value header value
**/
void linphone_chat_message_add_custom_header(LinphoneChatMessage* message, const char *header_name, const char *header_value){
	message->custom_headers=sal_custom_header_append(message->custom_headers,header_name,header_value);
}

/**
 * Retrieve a custom header value given its name.
 * @param message the message
 * @param header_name header name searched
**/
const char * linphone_chat_message_get_custom_header(LinphoneChatMessage* message, const char *header_name){
	return sal_custom_header_find(message->custom_headers,header_name);
}

/**
 * Returns TRUE if the message has been read, otherwise returns FALSE.
 * @param message the message
**/
bool_t linphone_chat_message_is_read(LinphoneChatMessage* message) {
	return message->is_read;
}

/**
 * Returns TRUE if the message has been sent, returns FALSE if the message has been received.
 * @param message the message
**/
bool_t linphone_chat_message_is_outgoing(LinphoneChatMessage* message) {
	return message->dir == LinphoneChatMessageOutgoing;
}

/**
 * Returns the id used to identify this message in the storage database
 * @param message the message
 * @return the id
 */
unsigned int linphone_chat_message_get_storage_id(LinphoneChatMessage* message) {
    return message->storage_id;
}

/**
 * Duplicate a LinphoneChatMessage
**/
LinphoneChatMessage* linphone_chat_message_clone(const LinphoneChatMessage* msg) {
	/*struct _LinphoneChatMessage {
	 char* message;
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
	LinphoneChatMessage* new_message = linphone_chat_room_create_message(msg->chat_room,msg->message);
	if (msg->external_body_url) new_message->external_body_url=ms_strdup(msg->external_body_url);
	new_message->cb=msg->cb;
	new_message->cb_ud=msg->cb_ud;
	new_message->message_userdata=msg->message_userdata;
	new_message->cb=msg->cb;
	new_message->time=msg->time;
	new_message->state=msg->state;
	new_message->storage_id=msg->storage_id;
	if (msg->from) new_message->from=linphone_address_clone(msg->from);
	return new_message;
}

/**
 * Destroys a LinphoneChatMessage.
**/
void linphone_chat_message_destroy(LinphoneChatMessage* msg) {
	if (msg->message) ms_free(msg->message);
	if (msg->external_body_url) ms_free(msg->external_body_url);
	if (msg->from) linphone_address_destroy(msg->from);
	if (msg->to) linphone_address_destroy(msg->to);
	if (msg->custom_headers) sal_custom_header_free(msg->custom_headers);
	ms_free(msg);
}


LinphoneReason linphone_chat_message_get_reason(LinphoneChatMessage* msg) {
	return linphone_reason_from_sal(msg->reason);
}

/**
 * @}
 */


