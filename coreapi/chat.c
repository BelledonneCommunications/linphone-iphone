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
#include "belle-sip/belle-sip.h"

#include <libxml/parser.h>
#include <libxml/xmlwriter.h>

#define COMPOSING_DEFAULT_IDLE_TIMEOUT 15
#define COMPOSING_DEFAULT_REFRESH_TIMEOUT 60
#define COMPOSING_DEFAULT_REMOTE_REFRESH_TIMEOUT 120


static void _linphone_chat_room_send_message(LinphoneChatRoom *cr, LinphoneChatMessage* msg);
#define MULTIPART_BOUNDARY "---------------------------14737809831466499882746641449"
#define MULTIPART_HEADER_1 "--" MULTIPART_BOUNDARY "\r\n" \
			"Content-Disposition: form-data; name=\"File\"; filename=\""
#define MULTIPART_HEADER_2 "\"\r\n" \
			"Content-Type: "
#define MULTIPART_HEADER_3 "\r\n\r\n"
#define MULTIPART_END "\r\n--" MULTIPART_BOUNDARY "--\r\n"
const char *multipart_boundary=MULTIPART_BOUNDARY;

static size_t linphone_chat_message_compute_multipart_header_size(const char *filename, const char *content_type) {
	return strlen(MULTIPART_HEADER_1)+strlen(filename)+strlen(MULTIPART_HEADER_2)+strlen(content_type)+strlen(MULTIPART_HEADER_3);
}
static void process_io_error(void *data, const belle_sip_io_error_event_t *event){
	printf("We have a response io error!\n");
}
static void process_auth_requested(void *data, belle_sip_auth_event_t *event){
	printf("We have a auth requested!\n");
}

/**
 * Callback called during upload or download of a file from server
 * It is just forwarding the call and some parameters to the vtable defined callback
 */
static void linphone_chat_message_file_transfer_on_progress(belle_sip_body_handler_t *bh, belle_sip_message_t *msg, void *data, size_t offset, size_t total){
	LinphoneChatMessage* chatMsg=(LinphoneChatMessage *)data;
	LinphoneCore *lc = chatMsg->chat_room->lc;
	/* call back given by application level */
	if (lc->vtable.file_transfer_progress_indication != NULL) {
		lc->vtable.file_transfer_progress_indication(lc, chatMsg, chatMsg->file_transfer_information, (size_t)(((double)offset/(double)total)*100.0));
	}
	return;
}

/**
 * Callback called when posting a file to server (following rcs5.1 recommendation)
 *
 * @param	bh		the body handler
 * @param	msg		the belle sip message
 * @param	data	the user data associated to the handler, contains the linphoneChatMessage we're working on
 * @param	offset	current position in the input buffer
 * @param	buffer	the ouput buffer we to copy the data to be uploaded
 * @param	size	size in byte of the data requested, as output it will contain the effective copied size
 *
 */
static int linphone_chat_message_file_transfer_on_send_body(belle_sip_user_body_handler_t *bh, belle_sip_message_t *msg, void *data, size_t offset, char *buffer, size_t *size){
	LinphoneChatMessage* chatMsg=(LinphoneChatMessage *)data;
	LinphoneCore *lc = chatMsg->chat_room->lc;

	char *content_type=belle_sip_strdup_printf("%s/%s", chatMsg->file_transfer_information->type, chatMsg->file_transfer_information->subtype);
	size_t end_of_file=linphone_chat_message_compute_multipart_header_size(chatMsg->file_transfer_information->name, content_type)+chatMsg->file_transfer_information->size;

	if (offset==0){
		int partlen=linphone_chat_message_compute_multipart_header_size(chatMsg->file_transfer_information->name, content_type);
		memcpy(buffer,MULTIPART_HEADER_1,strlen(MULTIPART_HEADER_1));
		buffer += strlen(MULTIPART_HEADER_1);
		memcpy(buffer,chatMsg->file_transfer_information->name,strlen(chatMsg->file_transfer_information->name));
		buffer += strlen(chatMsg->file_transfer_information->name);
		memcpy(buffer,MULTIPART_HEADER_2,strlen(MULTIPART_HEADER_2));
		buffer += strlen(MULTIPART_HEADER_2);
		memcpy(buffer,content_type,strlen(content_type));
		buffer += strlen(content_type);
		memcpy(buffer,MULTIPART_HEADER_3,strlen(MULTIPART_HEADER_3));

		*size=partlen;
	}else if (offset<end_of_file){
		/* get data from call back */
		lc->vtable.file_transfer_send(lc, chatMsg, chatMsg->file_transfer_information, buffer, size);
	}else{
		*size=strlen(MULTIPART_END);
		strncpy(buffer,MULTIPART_END,*size);
	}
	belle_sip_free(content_type);
	return BELLE_SIP_CONTINUE;
}

/**
 * Callback function called when we have a response from server during a file upload to server (rcs5.1 recommandation)
 * Note: The first post is empty and the server shall reply a 204 (No content) message, this will trigger a new post request to the server
 * to upoad the file. The server response to this second post is processed by this same function
 *
 * @param	data	the user define pointer associated with the request, it contains the linphoneChatMessage we're trying to send
 * @param	event	the response from server
 */
static void linphone_chat_message_process_response_from_post_file(void *data, const belle_http_response_event_t *event){
	LinphoneChatMessage* msg=(LinphoneChatMessage *)data;

	/* check the answer code */
	if (event->response){
		int code=belle_http_response_get_status_code(event->response);
		if (code == 204) { /* this is the reply to the first post to the server - an empty message */
			/* start uploading the file */
			belle_http_request_listener_callbacks_t cbs={0};
			belle_http_request_listener_t *l;
			belle_generic_uri_t *uri;
			belle_http_request_t *req;
			char *content_type=belle_sip_strdup_printf("%s/%s", msg->file_transfer_information->type, msg->file_transfer_information->subtype);
			belle_sip_user_body_handler_t *bh=belle_sip_user_body_handler_new(msg->file_transfer_information->size+linphone_chat_message_compute_multipart_header_size(msg->file_transfer_information->name, content_type)+strlen(MULTIPART_END), linphone_chat_message_file_transfer_on_progress, NULL, linphone_chat_message_file_transfer_on_send_body, msg);
			char* ua = ms_strdup_printf("%s/%s", linphone_core_get_user_agent_name(), linphone_core_get_user_agent_version());

			belle_sip_free(content_type);
			content_type=belle_sip_strdup_printf("multipart/form-data; boundary=%s",multipart_boundary);

			uri=belle_generic_uri_parse(msg->chat_room->lc->file_transfer_server);

			req=belle_http_request_create("POST",
										  uri,
										  belle_sip_header_create("User-Agent",ua),
										  belle_sip_header_create("Content-type",content_type),
										  NULL);
			ms_free(ua);
			belle_sip_free(content_type);
			belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(req),BELLE_SIP_BODY_HANDLER(bh));
			cbs.process_response=linphone_chat_message_process_response_from_post_file;
			cbs.process_io_error=process_io_error;
			cbs.process_auth_requested=process_auth_requested;
			l=belle_http_request_listener_create_from_callbacks(&cbs,msg);
			belle_http_provider_send_request(msg->chat_room->lc->http_provider,req,l);
		}
		if (code == 200 ) { /* file has been uplaoded correctly, get server reply and send it */
			const char *body = belle_sip_message_get_body((belle_sip_message_t *)event->response);
			msg->message = ms_strdup(body);
			linphone_content_uninit(msg->file_transfer_information);
			ms_free(msg->file_transfer_information);
			msg->file_transfer_information = NULL;
			msg->content_type = ms_strdup("application/vnd.gsma.rcs-ft-http+xml");
			_linphone_chat_room_send_message(msg->chat_room, msg);
		}
	}

}


static void _linphone_chat_message_destroy(LinphoneChatMessage* msg);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneChatMessage);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneChatMessage,belle_sip_object_t,
	(belle_sip_object_destroy_t)_linphone_chat_message_destroy,
	NULL, // clone
	NULL, // marshal
	FALSE
);


/**
 * @addtogroup chatroom
 * @{
 */

/**
 * Inconditionnaly disable incoming chat messages.
 * @param lc the core
 * @param deny_reason the deny reason (#LinphoneReasonNone has no effect).
**/
void linphone_core_disable_chat(LinphoneCore *lc, LinphoneReason deny_reason){
	lc->chat_deny_code=deny_reason;
}

/**
 * Enable reception of incoming chat messages.
 * By default it is enabled but it can be disabled with linphone_core_disable_chat().
 * @param lc the core
**/
void linphone_core_enable_chat(LinphoneCore *lc){
	lc->chat_deny_code=LinphoneReasonNone;
}

/**
 * Returns whether chat is enabled.
**/
bool_t linphone_core_chat_enabled(const LinphoneCore *lc){
	return lc->chat_deny_code!=LinphoneReasonNone;
}

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
	ms_list_free_with_data(cr->transient_messages, (void (*)(void*))linphone_chat_message_unref);
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

	/* Check if we shall upload a file to a server */
	if (msg->file_transfer_information != NULL) {
		/* open a transaction with the server and send an empty request(RCS5.1 section 3.5.4.8.3.1) */
		belle_http_request_listener_callbacks_t cbs={0};
		belle_http_request_listener_t *l;
		belle_generic_uri_t *uri;
		belle_http_request_t *req;

		uri=belle_generic_uri_parse(cr->lc->file_transfer_server);

		req=belle_http_request_create("POST",
				uri,
				NULL,
				NULL,
				NULL);
		cbs.process_response=linphone_chat_message_process_response_from_post_file;
		cbs.process_io_error=process_io_error;
		cbs.process_auth_requested=process_auth_requested;
		l=belle_http_request_listener_create_from_callbacks(&cbs,msg); /* give msg to listener to be able to start the actual file upload when server answer a 204 No content */
		belle_http_provider_send_request(cr->lc->http_provider,req,l);

		return;
	}

	if (lp_config_get_int(cr->lc->config,"sip","chat_use_call_dialogs",0)){
		if((call = linphone_core_get_call_by_remote_address(cr->lc,cr->peer))!=NULL){
			if (call->state==LinphoneCallConnected ||
			call->state==LinphoneCallStreamsRunning ||
			call->state==LinphoneCallPaused ||
			call->state==LinphoneCallPausing ||
			call->state==LinphoneCallPausedByRemote){
				ms_message("send SIP message through the existing call.");
				op = call->op;
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
		msg->op = op = sal_op_new(cr->lc->sal);
		linphone_configure_op(cr->lc,op,cr->peer_url,msg->custom_headers,lp_config_get_int(cr->lc->config,"sip","chat_msg_with_contact",0));
		sal_op_set_user_pointer(op, msg); /*if out of call, directly store msg*/
	}
	if (msg->external_body_url) {
		content_type=ms_strdup_printf("message/external-body; access-type=URL; URL=\"%s\"",msg->external_body_url);
		sal_message_send(op,identity,cr->peer,content_type, NULL);
		ms_free(content_type);
	} else {
		if (msg->content_type == NULL) {
			sal_text_send(op, identity, cr->peer,msg->message);
		} else {
			sal_message_send(op, identity, cr->peer, msg->content_type, msg->message);
		}
	}
	msg->dir=LinphoneChatMessageOutgoing;
	msg->from=linphone_address_new(identity);
	msg->storage_id=linphone_chat_message_store(msg);

	// add to transient list
	cr->transient_messages = ms_list_append(cr->transient_messages, linphone_chat_message_ref(msg));

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
 * @param addr a linphone address.
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
	if (sal_msg->content_type != NULL) { /* content_type field is, for now, used only for rcs file transfer bu twe shall strcmp it with "application/vnd.gsma.rcs-ft-http+xml" */
		xmlChar *file_url = NULL;
		xmlDocPtr xmlMessageBody;
		xmlNodePtr cur;

		msg = linphone_chat_room_create_message(cr, NULL); /* create a message with empty body */
		msg->content_type = ms_strdup(sal_msg->content_type); /* add the content_type "application/vnd.gsma.rcs-ft-http+xml" */
		msg->file_transfer_information = (LinphoneContent *)malloc(sizeof(LinphoneContent));
		memset(msg->file_transfer_information, 0, sizeof(*(msg->file_transfer_information)));

		/* parse the message body to get all informations from it */
		xmlMessageBody = xmlParseDoc((const xmlChar *)sal_msg->text);

		cur = xmlDocGetRootElement(xmlMessageBody);
		if (cur != NULL) {
			cur = cur->xmlChildrenNode;
			while (cur!=NULL) {
				if (!xmlStrcmp(cur->name, (const xmlChar *)"file-info")) { /* we found a file info node, check it has a type="file" attribute */
					xmlChar *typeAttribute = xmlGetProp(cur, (const xmlChar *)"type");
					if(!xmlStrcmp(typeAttribute, (const xmlChar *)"file")) { /* this is the node we are looking for */
						cur = cur->xmlChildrenNode; /* now loop on the content of the file-info node */
						while (cur!=NULL) {
							if (!xmlStrcmp(cur->name, (const xmlChar *)"file-size")) {
								xmlChar *fileSizeString = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
								msg->file_transfer_information->size = strtol((const char*)fileSizeString, NULL, 10);
								xmlFree(fileSizeString);
							}

							if (!xmlStrcmp(cur->name, (const xmlChar *)"file-name")) {
								msg->file_transfer_information->name = (char *)xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
							}
							if (!xmlStrcmp(cur->name, (const xmlChar *)"content-type")) {
								xmlChar *contentType = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
								int contentTypeIndex = 0;
								while (contentType[contentTypeIndex]!='/' && contentType[contentTypeIndex]!='\0') {
									contentTypeIndex++;
								}
								msg->file_transfer_information->type = ms_strndup((char *)contentType, contentTypeIndex);
								msg->file_transfer_information->subtype = ms_strdup(((char *)contentType+contentTypeIndex+1));
								xmlFree(contentType);
							}
							if (!xmlStrcmp(cur->name, (const xmlChar *)"data")) {
								file_url = 	xmlGetProp(cur, (const xmlChar *)"url");
							}

							cur=cur->next;
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
	} else { /* message is not rcs file transfer, create it with provided sal_msg->text as ->message */
		msg = linphone_chat_room_create_message(cr, sal_msg->text);
	}
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
 * @deprecated use linphone_chat_room_get_core()
**/
LinphoneCore* linphone_chat_room_get_lc(LinphoneChatRoom *cr){
	return cr->lc;
}

/**
 * Returns back pointer to LinphoneCore object.
**/
LinphoneCore* linphone_chat_room_get_core(LinphoneChatRoom *cr){
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
	LinphoneChatMessage* msg = belle_sip_object_new(LinphoneChatMessage);
	msg->chat_room=(LinphoneChatRoom*)cr;
	msg->message=message?ms_strdup(message):NULL;
	msg->is_read=TRUE;
	msg->content_type = NULL; /* this property is used only when transfering file */
	msg->file_transfer_information = NULL; /* this property is used only when transfering file */
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

	LinphoneChatMessage* msg = belle_sip_object_new(LinphoneChatMessage);
	msg->chat_room=(LinphoneChatRoom*)cr;
	msg->message=message?ms_strdup(message):NULL;
	msg->external_body_url=external_body_url?ms_strdup(external_body_url):NULL;
	msg->time=time;
	msg->state=state;
	msg->is_read=is_read;
	msg->content_type = NULL; /* this property is used only when transfering file */
	msg->file_transfer_information = NULL; /* this property is used only when transfering file */
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
 * Linphone message has an app-specific field that can store a text. The application might want
 * to use it for keeping data over restarts, like thumbnail image path.
 * @param message #LinphoneChatMessage
 * @return the application-specific data or NULL if none has been stored.
 */
const char* linphone_chat_message_get_appdata(const LinphoneChatMessage* message){
	return message->appdata;
}

/**
 * Linphone message has an app-specific field that can store a text. The application might want
 * to use it for keeping data over restarts, like thumbnail image path.
 *
 * Invoking this function will attempt to update the message storage to reflect the changeif it is
 * enabled.
 *
 * @param message #LinphoneChatMessage
 * @param data the data to store into the message
 */
void linphone_chat_message_set_appdata(LinphoneChatMessage* message, const char* data){
	if( message->appdata ){
		ms_free(message->appdata);
	}
	message->appdata = data? ms_strdup(data) : NULL;
	linphone_chat_message_store_appdata(message);
}


/**
 * Get the file_transfer_information (used by call backs to recover informations during a rcs file transfer)
 *
 * @param message #LinphoneChatMessage
 * @return a pointer to the LinphoneContent structure or NULL if not present.
 */
const LinphoneContent *linphone_chat_message_get_file_transfer_information(const LinphoneChatMessage*message) {
	return message->file_transfer_information;
}

static void on_recv_body(belle_sip_user_body_handler_t *bh, belle_sip_message_t *msg, void *data, size_t offset, const char *buffer, size_t size){
	//printf("Receive %ld bytes\n\n%s\n\n", size, (char *)buffer);
	LinphoneChatMessage* chatMsg=(LinphoneChatMessage *)data;
	LinphoneCore *lc = chatMsg->chat_room->lc;
	/* call back given by application level */
	if (lc->vtable.file_transfer_received != NULL) {
		lc->vtable.file_transfer_received(lc, chatMsg, chatMsg->file_transfer_information, buffer, size);
	}
	return;

	/* feed the callback with the received data */


}

static void linphone_chat_process_response_headers_from_get_file(void *data, const belle_http_response_event_t *event){
	if (event->response){
		/*we are receiving a response, set a specific body handler to acquire the response.
		 * if not done, belle-sip will create a memory body handler, the default*/
		LinphoneChatMessage *message=(LinphoneChatMessage *)belle_sip_object_data_get(BELLE_SIP_OBJECT(event->request),"message");
		belle_sip_message_set_body_handler(
			(belle_sip_message_t*)event->response,
			(belle_sip_body_handler_t*)belle_sip_user_body_handler_new(message->file_transfer_information->size, linphone_chat_message_file_transfer_on_progress,on_recv_body,NULL,message)
		);
	}
}

static void linphone_chat_process_response_from_get_file(void *data, const belle_http_response_event_t *event){
	//LinphoneChatMessage* msg=(LinphoneChatMessage *)data;

	/* check the answer code */
	if (event->response){
		int code=belle_http_response_get_status_code(event->response);
		if (code==200) {
			LinphoneChatMessage* chatMsg=(LinphoneChatMessage *)data;
			LinphoneCore *lc = chatMsg->chat_room->lc;
			/* file downloaded succesfully, call again the callback with size at zero */
			if (lc->vtable.file_transfer_received != NULL) {
				lc->vtable.file_transfer_received(lc, chatMsg, chatMsg->file_transfer_information, NULL, 0);
			}
		}
	}
}

/**
 * Start the download of the file from remote server
 *
 * @param message #LinphoneChatMessage
 */
void linphone_chat_message_start_file_download(const LinphoneChatMessage *message) {
	belle_http_request_listener_callbacks_t cbs={0};
	belle_http_request_listener_t *l;
	belle_generic_uri_t *uri;
	belle_http_request_t *req;
	const char *url=message->external_body_url;
	char* ua = ms_strdup_printf("%s/%s", linphone_core_get_user_agent_name(), linphone_core_get_user_agent_version());

	uri=belle_generic_uri_parse(url);

	req=belle_http_request_create("GET",
				uri,
				belle_sip_header_create("User-Agent",ua),
				NULL);

	ms_free(ua);

	cbs.process_response_headers=linphone_chat_process_response_headers_from_get_file;
	cbs.process_response=linphone_chat_process_response_from_get_file;
	cbs.process_io_error=process_io_error;
	cbs.process_auth_requested=process_auth_requested;
	l=belle_http_request_listener_create_from_callbacks(&cbs, (void *)message);
	belle_sip_object_data_set(BELLE_SIP_OBJECT(req),"message",(void *)message,NULL);
	belle_http_provider_send_request(message->chat_room->lc->http_provider,req,l);
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
	if (msg->appdata) new_message->appdata = ms_strdup(msg->appdata);
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
void linphone_chat_message_destroy(LinphoneChatMessage* msg){
	belle_sip_object_unref(msg);
}


/**
 * Destroys a LinphoneChatMessage.
**/
static void _linphone_chat_message_destroy(LinphoneChatMessage* msg) {
	if (msg->op) sal_op_release(msg->op);
	if (msg->message) ms_free(msg->message);
	if (msg->external_body_url) ms_free(msg->external_body_url);
	if (msg->appdata) ms_free(msg->appdata);
	if (msg->from) linphone_address_destroy(msg->from);
	if (msg->to) linphone_address_destroy(msg->to);
	if (msg->custom_headers) sal_custom_header_free(msg->custom_headers);
	if (msg->content_type) ms_free(msg->content_type);
	if (msg->file_transfer_information) {
		linphone_content_uninit(msg->file_transfer_information);
		ms_free(msg->file_transfer_information);
	}
}


/**
 * Acquire a reference to the chat message.
 * @param msg the chat message
 * @return the same chat message
**/
LinphoneChatMessage * linphone_chat_message_ref(LinphoneChatMessage *msg){
	belle_sip_object_ref(msg);
	return msg;
}

/**
 * Release reference to the chat message.
 * @param msg the chat message.
**/
void linphone_chat_message_unref(LinphoneChatMessage *msg){
	belle_sip_object_unref(msg);
}

/**
 * Get full details about delivery error of a chat message.
 * @param msg a LinphoneChatMessage
 * @return a LinphoneErrorInfo describing the details.
**/
const LinphoneErrorInfo *linphone_chat_message_get_error_info(const LinphoneChatMessage *msg){
	return linphone_error_info_from_sal_op(msg->op);
}

LinphoneReason linphone_chat_message_get_reason(LinphoneChatMessage* msg) {
	return linphone_error_info_get_reason(linphone_chat_message_get_error_info(msg));
}



/**
 * Create a message attached to a dedicated chat room with a particular content. Use #linphone_chat_room_send_message2 to initiate the transfer
 * @param cr the chat room.
 * @param a #LinphoneContent initial content. #LinphoneCoreVTable.file_transfer_send is invoked later to notify file transfer progress and collect next chunk of the message if #LinphoneContent.data is NULL.
 * @return a new #LinphoneChatMessage
 */

LinphoneChatMessage* linphone_chat_room_create_file_transfer_message(LinphoneChatRoom *cr, LinphoneContent* initial_content) {
	LinphoneChatMessage* msg = belle_sip_object_new(LinphoneChatMessage);
	msg->chat_room=(LinphoneChatRoom*)cr;
	msg->message = NULL;
	msg->file_transfer_information = (LinphoneContent *)malloc(sizeof(LinphoneContent));
	memset(msg->file_transfer_information, 0, sizeof(LinphoneContent));
	linphone_content_copy(msg->file_transfer_information, initial_content);
	msg->dir=LinphoneChatMessageOutgoing;
	linphone_chat_message_set_to(msg, linphone_chat_room_get_peer_address(cr));
	linphone_chat_message_set_from(msg, linphone_address_new(linphone_core_get_identity(cr->lc)));
	msg->content_type=NULL; /* this will be set to application/vnd.gsma.rcs-ft-http+xml when we will transfer the xml reply from server to the peers */

	return msg;
}
/**
 * @}
 */


