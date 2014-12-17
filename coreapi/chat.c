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


static LinphoneChatMessageCbs * linphone_chat_message_cbs_new(void) {
	return belle_sip_object_new(LinphoneChatMessageCbs);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneChatMessageCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneChatMessageCbs, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // marshal
	FALSE
);


/**
 * @addtogroup chatroom
 * @{
 */

/**
 * Acquire a reference to the LinphoneChatMessageCbs object.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The same LinphoneChatMessageCbs object.
 */
LinphoneChatMessageCbs * linphone_chat_message_cbs_ref(LinphoneChatMessageCbs *cbs) {
	belle_sip_object_ref(cbs);
	return cbs;
}

/**
 * Release reference to the LinphoneChatMessageCbs object.
 * @param[in] cbs LinphoneChatMessageCbs object.
 */
void linphone_chat_message_cbs_unref(LinphoneChatMessageCbs *cbs) {
	belle_sip_object_unref(cbs);
}

/**
 * Retrieve the user pointer associated with the LinphoneChatMessageCbs object.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The user pointer associated with the LinphoneChatMessageCbs object.
 */
void *linphone_chat_message_cbs_get_user_data(const LinphoneChatMessageCbs *cbs) {
	return cbs->user_data;
}

/**
 * Assign a user pointer to the LinphoneChatMessageCbs object.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] ud The user pointer to associate with the LinphoneChatMessageCbs object.
 */
void linphone_chat_message_cbs_set_user_data(LinphoneChatMessageCbs *cbs, void *ud) {
	cbs->user_data = ud;
}

/**
 * Get the message state changed callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The current message state changed callback.
 */
LinphoneChatMessageCbsMsgStateChangedCb linphone_chat_message_cbs_get_msg_state_changed(const LinphoneChatMessageCbs *cbs) {
	return cbs->msg_state_changed;
}

/**
 * Set the message state changed callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] cb The message state changed callback to be used.
 */
void linphone_chat_message_cbs_set_msg_state_changed(LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsMsgStateChangedCb cb) {
	cbs->msg_state_changed = cb;
}

/**
 * Get the file transfer receive callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The current file transfer receive callback.
 */
LinphoneChatMessageCbsFileTransferRecvCb linphone_chat_message_cbs_get_file_transfer_recv(const LinphoneChatMessageCbs *cbs) {
	return cbs->file_transfer_recv;
}

/**
 * Set the file transfer receive callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] cb The file transfer receive callback to be used.
 */
void linphone_chat_message_cbs_set_file_transfer_recv(LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsFileTransferRecvCb cb) {
	cbs->file_transfer_recv = cb;
}

/**
 * Get the file transfer send callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The current file transfer send callback.
 */
LinphoneChatMessageCbsFileTransferSendCb linphone_chat_message_cbs_get_file_transfer_send(const LinphoneChatMessageCbs *cbs) {
	return cbs->file_transfer_send;
}

/**
 * Set the file transfer send callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] cb The file transfer send callback to be used.
 */
void linphone_chat_message_cbs_set_file_transfer_send(LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsFileTransferSendCb cb) {
	cbs->file_transfer_send = cb;
}

/**
 * Get the file transfer progress indication callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @return The current file transfer progress indication callback.
 */
LinphoneChatMessageCbsFileTransferProgressIndicationCb linphone_chat_message_cbs_get_file_transfer_progress_indication(const LinphoneChatMessageCbs *cbs) {
	return cbs->file_transfer_progress_indication;
}

/**
 * Set the file transfer progress indication callback.
 * @param[in] cbs LinphoneChatMessageCbs object.
 * @param[in] cb The file transfer progress indication callback to be used.
 */
void linphone_chat_message_cbs_set_file_transfer_progress_indication(LinphoneChatMessageCbs *cbs, LinphoneChatMessageCbsFileTransferProgressIndicationCb cb) {
	cbs->file_transfer_progress_indication = cb;
}

/**
 * @}
 */



static void _linphone_chat_room_send_message(LinphoneChatRoom *cr, LinphoneChatMessage* msg);

static void process_io_error_upload(void *data, const belle_sip_io_error_event_t *event){
	LinphoneChatMessage* msg=(LinphoneChatMessage *)data;
	ms_error("I/O Error during file upload to %s - msg [%p] chat room[%p]", linphone_core_get_file_transfer_server(msg->chat_room->lc), msg, msg->chat_room);
	if (msg->cb) {
		msg->cb(msg, LinphoneChatMessageStateNotDelivered, msg->cb_ud);
	}
	if (linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)) {
		linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)(msg, LinphoneChatMessageStateNotDelivered);
	}
}
static void process_auth_requested_upload(void *data, belle_sip_auth_event_t *event){
	LinphoneChatMessage* msg=(LinphoneChatMessage *)data;
	ms_error("Error during file upload : auth requested to connect %s - msg [%p] chat room[%p]", linphone_core_get_file_transfer_server(msg->chat_room->lc), msg, msg->chat_room);
	if (msg->cb) {
		msg->cb(msg, LinphoneChatMessageStateNotDelivered, msg->cb_ud);
	}
	if (linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)) {
		linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)(msg, LinphoneChatMessageStateNotDelivered);
	}
}

static void process_io_error_download(void *data, const belle_sip_io_error_event_t *event){
	LinphoneChatMessage* msg=(LinphoneChatMessage *)data;
	ms_error("I/O Error during file download %s - msg [%p] chat room[%p]", msg->external_body_url, msg, msg->chat_room);
	if (msg->cb) {
		msg->cb(msg, LinphoneChatMessageStateFileTransferError, msg->cb_ud);
	}
	if (linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)) {
		linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)(msg, LinphoneChatMessageStateFileTransferError);
	}
}
static void process_auth_requested_download(void *data, belle_sip_auth_event_t *event){
	LinphoneChatMessage* msg=(LinphoneChatMessage *)data;
	ms_error("Error during file download : auth requested to get %s - msg [%p] chat room[%p]", msg->external_body_url, msg, msg->chat_room);
	if (msg->cb) {
		msg->cb(msg, LinphoneChatMessageStateFileTransferError, msg->cb_ud);
	}
	if (linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)) {
		linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)(msg, LinphoneChatMessageStateFileTransferError);
	}
}

/**
 * Callback called during upload or download of a file from server
 * It is just forwarding the call and some parameters to the vtable defined callback
 */
static void linphone_chat_message_file_transfer_on_progress(belle_sip_body_handler_t *bh, belle_sip_message_t *msg, void *data, size_t offset, size_t total){
	LinphoneChatMessage* chatMsg=(LinphoneChatMessage *)data;
	if (linphone_chat_message_cbs_get_file_transfer_progress_indication(chatMsg->callbacks)) {
		linphone_chat_message_cbs_get_file_transfer_progress_indication(chatMsg->callbacks)(chatMsg, chatMsg->file_transfer_information, offset, total);
	} else {
		/* Legacy: call back given by application level */
		linphone_core_notify_file_transfer_progress_indication(chatMsg->chat_room->lc, chatMsg, chatMsg->file_transfer_information, offset, total);
	}
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
static int linphone_chat_message_file_transfer_on_send_body(belle_sip_user_body_handler_t *bh, belle_sip_message_t *msg, void *data, size_t offset, uint8_t *buffer, size_t *size){
	LinphoneChatMessage* chatMsg=(LinphoneChatMessage *)data;
	LinphoneCore *lc = chatMsg->chat_room->lc;
	char *buf = (char *)buffer;

	/* if we've not reach the end of file yet, ask for more data*/
	if (offset<linphone_content_get_size(chatMsg->file_transfer_information)){
		/* get data from call back */
		if (linphone_chat_message_cbs_get_file_transfer_send(chatMsg->callbacks)) {
			LinphoneBuffer *lb = linphone_chat_message_cbs_get_file_transfer_send(chatMsg->callbacks)(chatMsg, chatMsg->file_transfer_information, offset, *size);
			if (lb == NULL) *size = 0;
			else {
				*size = linphone_buffer_get_size(lb);
				memcpy(buffer, linphone_buffer_get_content(lb), *size);
				linphone_buffer_unref(lb);
			}
		} else {
			/* Legacy */
			linphone_core_notify_file_transfer_send(lc, chatMsg, chatMsg->file_transfer_information, buf, size);
		}
	}

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
			belle_sip_multipart_body_handler_t *bh;
			char* ua;
			char *first_part_header;
			belle_sip_body_handler_t *first_part_bh;

			/* temporary storage for the Content-disposition header value */
			first_part_header = belle_sip_strdup_printf("form-data; name=\"File\"; filename=\"%s\"", linphone_content_get_name(msg->file_transfer_information));

			/* create a user body handler to take care of the file and add the content disposition and content-type headers */
			if (msg->file_transfer_filepath != NULL) {
				first_part_bh=(belle_sip_body_handler_t *)belle_sip_file_body_handler_new(msg->file_transfer_filepath,NULL,msg);
			} else if (linphone_content_get_buffer(msg->file_transfer_information) != NULL) {
				first_part_bh=(belle_sip_body_handler_t *)belle_sip_memory_body_handler_new_from_buffer(
					linphone_content_get_buffer(msg->file_transfer_information),
					linphone_content_get_size(msg->file_transfer_information),
					NULL,msg);
			} else {
				first_part_bh=(belle_sip_body_handler_t *)belle_sip_user_body_handler_new(linphone_content_get_size(msg->file_transfer_information),NULL,NULL,linphone_chat_message_file_transfer_on_send_body,msg);
			}
			belle_sip_body_handler_add_header(first_part_bh, belle_sip_header_create("Content-disposition", first_part_header));
			belle_sip_free(first_part_header);
			belle_sip_body_handler_add_header(first_part_bh, (belle_sip_header_t *)belle_sip_header_content_type_create(linphone_content_get_type(msg->file_transfer_information), linphone_content_get_subtype(msg->file_transfer_information)));

			/* insert it in a multipart body handler which will manage the boundaries of multipart message */
			bh=belle_sip_multipart_body_handler_new(linphone_chat_message_file_transfer_on_progress, msg, first_part_bh);

			/* create the http request: do not include the message header at this point, it is done by bellesip when setting the multipart body handler in the message */
			ua = ms_strdup_printf("%s/%s", linphone_core_get_user_agent_name(), linphone_core_get_user_agent_version());
			uri=belle_generic_uri_parse(linphone_core_get_file_transfer_server(msg->chat_room->lc));
			req=belle_http_request_create("POST",
										  uri,
										  belle_sip_header_create("User-Agent",ua),
										  NULL);
			ms_free(ua);
			belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(req),BELLE_SIP_BODY_HANDLER(bh));
			cbs.process_response=linphone_chat_message_process_response_from_post_file;
			cbs.process_io_error=process_io_error_upload;
			cbs.process_auth_requested=process_auth_requested_upload;
			l=belle_http_request_listener_create_from_callbacks(&cbs,msg);
			msg->http_request=req; /* update the reference to the http request to be able to cancel it during upload */
			belle_http_provider_send_request(msg->chat_room->lc->http_provider,req,l);
		}
		if (code == 200 ) { /* file has been uploaded correctly, get server reply and send it */
			const char *body;
			/* TODO Check that the transfer has not been cancelled, note this shall be removed once the belle sip API will provide a cancel request as we shall never reach this part if the transfer is actually cancelled */
			if (msg->http_request == NULL) {
				return;
			}
			body = belle_sip_message_get_body((belle_sip_message_t *)event->response);
			msg->message = ms_strdup(body);
			msg->content_type = ms_strdup("application/vnd.gsma.rcs-ft-http+xml");
			if (msg->cb) {
				msg->cb(msg, LinphoneChatMessageStateFileTransferDone, msg->cb_ud);
			}
			if (linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)) {
				linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)(msg, LinphoneChatMessageStateFileTransferDone);
			}
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
 * Returns an list of chat rooms
 * @param[in] lc #LinphoneCore object
 * @return \mslist{LinphoneChatRoom}
**/
MSList* linphone_core_get_chat_rooms(LinphoneCore *lc) {
	return lc->chatrooms;
}

static bool_t linphone_chat_room_matches(LinphoneChatRoom *cr, const LinphoneAddress *from){
	return linphone_address_weak_equal(cr->peer_url,from);
}

static void _linphone_chat_room_destroy(LinphoneChatRoom *obj);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneChatRoom);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneChatRoom, belle_sip_object_t,
	(belle_sip_object_destroy_t)_linphone_chat_room_destroy,
	NULL, // clone
	NULL, // marshal
	FALSE
);

static LinphoneChatRoom * _linphone_core_create_chat_room(LinphoneCore *lc, LinphoneAddress *addr) {
	LinphoneChatRoom *cr = belle_sip_object_new(LinphoneChatRoom);
	cr->lc = lc;
	cr->peer = linphone_address_as_string(addr);
	cr->peer_url = addr;
	lc->chatrooms = ms_list_append(lc->chatrooms, (void *)cr);
	return cr;
}

static LinphoneChatRoom * _linphone_core_create_chat_room_from_url(LinphoneCore *lc, const char *to) {
	LinphoneAddress *parsed_url = NULL;
	if ((parsed_url = linphone_core_interpret_url(lc, to)) != NULL) {
		return _linphone_core_create_chat_room(lc, parsed_url);
	}
	return NULL;
}

LinphoneChatRoom * _linphone_core_get_chat_room(LinphoneCore *lc, const LinphoneAddress *addr){
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

static LinphoneChatRoom * _linphone_core_get_or_create_chat_room(LinphoneCore* lc, const char* to) {
	LinphoneAddress *to_addr=linphone_core_interpret_url(lc,to);
	LinphoneChatRoom *ret;

	if (to_addr==NULL){
		ms_error("linphone_core_get_or_create_chat_room(): Cannot make a valid address with %s",to);
		return NULL;
	}
	ret=_linphone_core_get_chat_room(lc,to_addr);
	linphone_address_destroy(to_addr);
	if (!ret){
		ret=_linphone_core_create_chat_room_from_url(lc,to);
	}
	return ret;
}

/**
 * Create a new chat room for messaging from a sip uri like sip:joe@sip.linphone.org if not already existing, else return exisiting one
 * @param lc #LinphoneCore object
 * @param to destination address for messages
 * @return #LinphoneChatRoom where messaging can take place.
 * @deprecated Use linphone_core_get_chat_room() or linphone_core_get_chat_room_from_uri() instead.
 */
LinphoneChatRoom* linphone_core_get_or_create_chat_room(LinphoneCore* lc, const char* to) {
	return _linphone_core_get_or_create_chat_room(lc, to);
}

/**
 * Create a new chat room for messaging from a sip uri like sip:joe@sip.linphone.org
 * @param lc #LinphoneCore object
 * @param to destination address for messages
 * @return #LinphoneChatRoom where messaging can take place.
 * @deprecated Use linphone_core_get_chat_room() or linphone_core_get_chat_room_from_uri() instead.
 */
LinphoneChatRoom * linphone_core_create_chat_room(LinphoneCore *lc, const char *to) {
	return _linphone_core_get_or_create_chat_room(lc, to);
}

/**
 * Get a chat room whose peer is the supplied address. If it does not exist yet, it will be created.
 * @param lc the linphone core
 * @param addr a linphone address.
 * @returns #LinphoneChatRoom where messaging can take place.
**/
LinphoneChatRoom *linphone_core_get_chat_room(LinphoneCore *lc, const LinphoneAddress *addr){
	LinphoneChatRoom *ret = _linphone_core_get_chat_room(lc, addr);
	if (!ret) {
		ret = _linphone_core_create_chat_room(lc, linphone_address_clone(addr));
	}
	return ret;
}

/**
 * Get a chat room for messaging from a sip uri like sip:joe@sip.linphone.org. If it does not exist yet, it will be created.
 * @param lc The linphone core
 * @param to The destination address for messages.
 * @returns #LinphoneChatRoom where messaging can take place.
**/
LinphoneChatRoom * linphone_core_get_chat_room_from_uri(LinphoneCore *lc, const char *to) {
	return _linphone_core_get_or_create_chat_room(lc, to);
}

static void linphone_chat_room_delete_composing_idle_timer(LinphoneChatRoom *cr) {
	if (cr->composing_idle_timer) {
		if(cr-> lc && cr->lc->sal)
			sal_cancel_timer(cr->lc->sal, cr->composing_idle_timer);
		belle_sip_object_unref(cr->composing_idle_timer);
		cr->composing_idle_timer = NULL;
	}
}

static void linphone_chat_room_delete_composing_refresh_timer(LinphoneChatRoom *cr) {
	if (cr->composing_refresh_timer) {
		if(cr->lc && cr->lc->sal)
			sal_cancel_timer(cr->lc->sal, cr->composing_refresh_timer);
		belle_sip_object_unref(cr->composing_refresh_timer);
		cr->composing_refresh_timer = NULL;
	}
}

static void linphone_chat_room_delete_remote_composing_refresh_timer(LinphoneChatRoom *cr) {
	if (cr->remote_composing_refresh_timer) {
		if(cr->lc && cr->lc->sal)
			sal_cancel_timer(cr->lc->sal, cr->remote_composing_refresh_timer);
		belle_sip_object_unref(cr->remote_composing_refresh_timer);
		cr->remote_composing_refresh_timer = NULL;
	}
}

static void _linphone_chat_room_destroy(LinphoneChatRoom *cr){
	ms_list_free_with_data(cr->transient_messages, (void (*)(void*))linphone_chat_message_unref);
	linphone_chat_room_delete_composing_idle_timer(cr);
	linphone_chat_room_delete_composing_refresh_timer(cr);
	linphone_chat_room_delete_remote_composing_refresh_timer(cr);
	if (cr->lc != NULL) {
		cr->lc->chatrooms=ms_list_remove(cr->lc->chatrooms,(void *) cr);
	}
	linphone_address_destroy(cr->peer_url);
	ms_free(cr->peer);
}

/**
 * Destroy a LinphoneChatRoom.
 * @param cr #LinphoneChatRoom object
 * @deprecated Use linphone_chat_room_unref() instead.
 */
void linphone_chat_room_destroy(LinphoneChatRoom *cr) {
	linphone_chat_room_unref(cr);
}

void linphone_chat_room_release(LinphoneChatRoom *cr) {
	cr->lc = NULL;
	linphone_chat_room_unref(cr);
}

LinphoneChatRoom * linphone_chat_room_ref(LinphoneChatRoom *cr) {
	belle_sip_object_ref(cr);
	return cr;
}

void linphone_chat_room_unref(LinphoneChatRoom *cr) {
	belle_sip_object_unref(cr);
}

void * linphone_chat_room_get_user_data(const LinphoneChatRoom *cr) {
	return cr->user_data;
}

void linphone_chat_room_set_user_data(LinphoneChatRoom *cr, void *ud) {
	cr->user_data = ud;
}


static void _linphone_chat_room_send_message(LinphoneChatRoom *cr, LinphoneChatMessage* msg){
	SalOp *op=NULL;
	LinphoneCall *call;
	char* content_type;
	const char *identity=NULL;
	time_t t=time(NULL);
	linphone_chat_message_ref(msg);
	/* Check if we shall upload a file to a server */
	if (msg->file_transfer_information != NULL && msg->content_type == NULL) {
		/* open a transaction with the server and send an empty request(RCS5.1 section 3.5.4.8.3.1) */
		belle_http_request_listener_callbacks_t cbs={0};
		belle_http_request_listener_t *l;
		belle_generic_uri_t *uri;
		belle_http_request_t *req;
		const char *transfer_server = linphone_core_get_file_transfer_server(cr->lc);

		if (transfer_server == NULL) {
			ms_warning("Cannot send file transfer message: no file transfer server configured.");
			return;
		}
		uri=belle_generic_uri_parse(transfer_server);

		req=belle_http_request_create("POST",
				uri,
				NULL,
				NULL,
				NULL);
		cbs.process_response=linphone_chat_message_process_response_from_post_file;
		cbs.process_io_error=process_io_error_upload;
		cbs.process_auth_requested=process_auth_requested_upload;
		l=belle_http_request_listener_create_from_callbacks(&cbs,msg); /* give msg to listener to be able to start the actual file upload when server answer a 204 No content */
		msg->http_request = req; /* keep a reference on the request to be able to cancel it */
		belle_http_provider_send_request(cr->lc->http_provider,req,l);
		linphone_chat_message_unref(msg);
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
	} else { /* the message is either text or have a file transfer using RCS recommendation */
		if (msg->content_type == NULL) { /* if no content type is specified, it is a text message */
			sal_text_send(op, identity, cr->peer,msg->message);
		} else {
			sal_message_send(op, identity, cr->peer, msg->content_type, msg->message);
			// Remove the message to prevent the xml from the file uplaod to be stored in the database
			ms_free(msg->message);
			msg->message = NULL;
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
	linphone_chat_message_unref(msg);
}

void linphone_chat_message_update_state(LinphoneChatMessage* chat_msg ) {
	linphone_chat_message_store_state(chat_msg);

	if( chat_msg->state == LinphoneChatMessageStateDelivered
			|| chat_msg->state == LinphoneChatMessageStateNotDelivered ){
		// message is not transient anymore, we can remove it from our transient list and unref it :
		chat_msg->chat_room->transient_messages = ms_list_remove(chat_msg->chat_room->transient_messages, chat_msg);
		linphone_chat_message_unref(chat_msg);
	}
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
	if (msg->message){
		/*legacy API*/
		linphone_core_notify_text_message_received(lc, cr, msg->from, msg->message);
	}
	linphone_core_notify_message_received(lc, cr,msg);
	cr->remote_is_composing = LinphoneIsComposingIdle;
	linphone_core_notify_is_composing_received(cr->lc, cr);
}

void linphone_core_message_received(LinphoneCore *lc, SalOp *op, const SalMessage *sal_msg){
	LinphoneChatRoom *cr=NULL;
	LinphoneAddress *addr;
	LinphoneChatMessage* msg;
	const SalCustomHeader *ch;

	addr=linphone_address_new(sal_msg->from);
	linphone_address_clean(addr);
	cr=linphone_core_get_chat_room(lc,addr);

	if (sal_msg->content_type != NULL) { /* content_type field is, for now, used only for rcs file transfer but we shall strcmp it with "application/vnd.gsma.rcs-ft-http+xml" */
		xmlChar *file_url = NULL;
		xmlDocPtr xmlMessageBody;
		xmlNodePtr cur;

		msg = linphone_chat_room_create_message(cr, NULL); /* create a message with empty body */
		msg->content_type = ms_strdup(sal_msg->content_type); /* add the content_type "application/vnd.gsma.rcs-ft-http+xml" */
		msg->file_transfer_information = linphone_content_new();

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
								linphone_content_set_size(msg->file_transfer_information, strtol((const char*)fileSizeString, NULL, 10));
								xmlFree(fileSizeString);
							}

							if (!xmlStrcmp(cur->name, (const xmlChar *)"file-name")) {
								linphone_content_set_name(msg->file_transfer_information, (const char *)xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1));
							}
							if (!xmlStrcmp(cur->name, (const xmlChar *)"content-type")) {
								xmlChar *contentType = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
								int contentTypeIndex = 0;
								char *type;
								char *subtype;
								while (contentType[contentTypeIndex]!='/' && contentType[contentTypeIndex]!='\0') {
									contentTypeIndex++;
								}
								type = ms_strndup((char *)contentType, contentTypeIndex);
								subtype = ms_strdup(((char *)contentType+contentTypeIndex+1));
								linphone_content_set_type(msg->file_transfer_information, type);
								linphone_content_set_subtype(msg->file_transfer_information, subtype);
								ms_free(subtype);
								ms_free(type);
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
	linphone_chat_message_unref(msg);
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
		linphone_core_notify_is_composing_received(cr->lc, cr);
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
	msg->callbacks=linphone_chat_message_cbs_new();
	msg->chat_room=(LinphoneChatRoom*)cr;
	msg->message=message?ms_strdup(message):NULL;
	msg->is_read=TRUE;
	msg->content_type = NULL; /* this property is used only when transfering file */
	msg->file_transfer_information = NULL; /* this property is used only when transfering file */
	msg->http_request = NULL;
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
	msg->callbacks=linphone_chat_message_cbs_new();
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
 * @deprecated Use linphone_chat_room_send_chat_message() instead.
 * @note The LinphoneChatMessage must not be destroyed until the the callback is called.
 */
void linphone_chat_room_send_message2(LinphoneChatRoom *cr, LinphoneChatMessage* msg,LinphoneChatMessageStateChangedCb status_cb, void* ud) {
	msg->cb=status_cb;
	msg->cb_ud=ud;
	msg->state=LinphoneChatMessageStateInProgress;
	_linphone_chat_room_send_message(cr, msg);
}

/**
 * Send a message to peer member of this chat room.
 * @param[in] cr LinphoneChatRoom object
 * @param[in] msg LinphoneChatMessage object
 * The state of the message sending will be notified via the callbacks defined in the LinphoneChatMessageCbs object that can be obtained
 * by calling linphone_chat_message_get_callbacks().
 */
void linphone_chat_room_send_chat_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	msg->state = LinphoneChatMessageStateInProgress;
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
		case LinphoneChatMessageStateFileTransferError:return "LinphoneChatMessageStateFileTransferError";
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

static void on_recv_body(belle_sip_user_body_handler_t *bh, belle_sip_message_t *msg, void *data, size_t offset, const uint8_t *buffer, size_t size){
	LinphoneChatMessage* chatMsg=(LinphoneChatMessage *)data;
	LinphoneCore *lc = chatMsg->chat_room->lc;
	/* TODO: while belle sip doesn't implement the cancel http request method, test if a request is still linked to the message before forwarding the data to callback */
	if (chatMsg->http_request == NULL) {
		return;
	}
	if (linphone_chat_message_cbs_get_file_transfer_recv(chatMsg->callbacks)) {
		LinphoneBuffer *lb = linphone_buffer_new_from_data(buffer, size);
		linphone_chat_message_cbs_get_file_transfer_recv(chatMsg->callbacks)(chatMsg, chatMsg->file_transfer_information, lb);
		linphone_buffer_unref(lb);
	} else {
		/* Legacy: call back given by application level */
		linphone_core_notify_file_transfer_recv(lc, chatMsg, chatMsg->file_transfer_information, (char *)buffer, size);
	}
	return;
}


static LinphoneContent* linphone_chat_create_file_transfer_information_from_headers(const belle_sip_message_t* message ){
	LinphoneContent *content = linphone_content_new();

	belle_sip_header_content_length_t* content_length_hdr = BELLE_SIP_HEADER_CONTENT_LENGTH(belle_sip_message_get_header(message, "Content-Length"));
	belle_sip_header_content_type_t* content_type_hdr = BELLE_SIP_HEADER_CONTENT_TYPE(belle_sip_message_get_header(message, "Content-Type"));
	const char* type = NULL,*subtype = NULL;

	linphone_content_set_name(content, "");

	if( content_type_hdr ){
		type = belle_sip_header_content_type_get_type(content_type_hdr);
		subtype = belle_sip_header_content_type_get_subtype(content_type_hdr);
		ms_message("Extracted content type %s / %s from header", type?type:"", subtype?subtype:"");
		if( type ) linphone_content_set_type(content, type);
		if( subtype ) linphone_content_set_subtype(content, subtype);
	}

	if( content_length_hdr ){
		linphone_content_set_size(content, belle_sip_header_content_length_get_content_length(content_length_hdr));
		ms_message("Extracted content length %i from header", (int)linphone_content_get_size(content));
	}

	return content;
}

static void linphone_chat_process_response_headers_from_get_file(void *data, const belle_http_response_event_t *event){
	if (event->response){
		/*we are receiving a response, set a specific body handler to acquire the response.
		 * if not done, belle-sip will create a memory body handler, the default*/
		LinphoneChatMessage *message=(LinphoneChatMessage *)belle_sip_object_data_get(BELLE_SIP_OBJECT(event->request),"message");
		belle_sip_message_t* response = BELLE_SIP_MESSAGE(event->response);
		size_t body_size = 0;

		if( message->file_transfer_information == NULL ){
			ms_warning("No file transfer information for message %p: creating...", message);
			message->file_transfer_information = linphone_chat_create_file_transfer_information_from_headers(response);
		}

		if( message->file_transfer_information ){
			body_size = linphone_content_get_size(message->file_transfer_information);
		}

		if (message->file_transfer_filepath == NULL) {
			belle_sip_message_set_body_handler(
				(belle_sip_message_t*)event->response,
				(belle_sip_body_handler_t*)belle_sip_user_body_handler_new(body_size, linphone_chat_message_file_transfer_on_progress,on_recv_body,NULL,message)
			);
		} else {
			belle_sip_message_set_body_handler(
				(belle_sip_message_t *)event->response,
				(belle_sip_body_handler_t *)belle_sip_file_body_handler_new(message->file_transfer_filepath, linphone_chat_message_file_transfer_on_progress, message)
			);
		}
	}
}

static void linphone_chat_process_response_from_get_file(void *data, const belle_http_response_event_t *event){
	/* check the answer code */
	if (event->response){
		int code=belle_http_response_get_status_code(event->response);
		if (code==200) {
			LinphoneChatMessage* chatMsg=(LinphoneChatMessage *)data;
			LinphoneCore *lc = chatMsg->chat_room->lc;
			/* file downloaded succesfully, call again the callback with size at zero */
			if (linphone_chat_message_cbs_get_file_transfer_recv(chatMsg->callbacks)) {
				LinphoneBuffer *lb = linphone_buffer_new();
				linphone_chat_message_cbs_get_file_transfer_recv(chatMsg->callbacks)(chatMsg, chatMsg->file_transfer_information, lb);
				linphone_buffer_unref(lb);
			} else {
				linphone_core_notify_file_transfer_recv(lc, chatMsg, chatMsg->file_transfer_information, NULL, 0);
			}
			if (chatMsg->cb) {
				chatMsg->cb(chatMsg, LinphoneChatMessageStateFileTransferDone, chatMsg->cb_ud);
			}
			if (linphone_chat_message_cbs_get_msg_state_changed(chatMsg->callbacks)) {
				linphone_chat_message_cbs_get_msg_state_changed(chatMsg->callbacks)(chatMsg, LinphoneChatMessageStateFileTransferDone);
			}
		}
	}
}

/**
 * Start the download of the file referenced in a LinphoneChatMessage from remote server.
 * @param[in] message LinphoneChatMessage object.
 */
void linphone_chat_message_download_file(LinphoneChatMessage *message) {
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
	cbs.process_io_error=process_io_error_download;
	cbs.process_auth_requested=process_auth_requested_download;
	l=belle_http_request_listener_create_from_callbacks(&cbs, (void *)message);
	belle_sip_object_data_set(BELLE_SIP_OBJECT(req),"message",(void *)message,NULL);
	message->http_request = req; /* keep a reference on the request to be able to cancel the download */
	message->state = LinphoneChatMessageStateInProgress; /* start the download, status is In Progress */
	belle_http_provider_send_request(message->chat_room->lc->http_provider,req,l);
}

/**
 * Start the download of the file from remote server
 *
 * @param message #LinphoneChatMessage
 * @param status_cb LinphoneChatMessageStateChangeCb status callback invoked when file is downloaded or could not be downloaded
 * @deprecated Use linphone_chat_message_download_file() instead.
 */
void linphone_chat_message_start_file_download(LinphoneChatMessage *message, LinphoneChatMessageStateChangedCb status_cb, void *ud) {
	message->cb = status_cb;
	message->cb_ud = ud;
	linphone_chat_message_download_file(message);
}

/**
 * Cancel an ongoing file transfer attached to this message.(upload or download)
 * @param msg	#LinphoneChatMessage
 */
void linphone_chat_message_cancel_file_transfer(LinphoneChatMessage *msg) {
	ms_message("Cancelled file transfer %s - msg [%p] chat room[%p]", (msg->external_body_url==NULL)?linphone_core_get_file_transfer_server(msg->chat_room->lc):msg->external_body_url, msg, msg->chat_room);
	/* TODO: here we shall call the cancel http request from bellesip API when it is available passing msg->http_request */
	/* waiting for this API, just set to NULL the reference to the request in the message and any request */
	msg->http_request = NULL;
	if (msg->cb) {
		msg->cb(msg, LinphoneChatMessageStateNotDelivered, msg->cb_ud);
	}
	if (linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)) {
		linphone_chat_message_cbs_get_msg_state_changed(msg->callbacks)(msg, LinphoneChatMessageStateNotDelivered);
	}
}


/**
 * Set origin of the message
 * @param[in] message #LinphoneChatMessage obj
 * @param[in] from #LinphoneAddress origin of this message (copied)
 */
void linphone_chat_message_set_from_address(LinphoneChatMessage* message, const LinphoneAddress* from) {
	if(message->from) linphone_address_destroy(message->from);
	message->from=linphone_address_clone(from);
}

/**
 * Get origin of the message
 * @param[in] message #LinphoneChatMessage obj
 * @return #LinphoneAddress
 */
const LinphoneAddress* linphone_chat_message_get_from_address(const LinphoneChatMessage* message) {
	return message->from;
}

/**
 * Set destination of the message
 * @param[in] message #LinphoneChatMessage obj
 * @param[in] to #LinphoneAddress destination of this message (copied)
 */
void linphone_chat_message_set_to_address(LinphoneChatMessage* message, const LinphoneAddress* to) {
	if(message->to) linphone_address_destroy(message->to);
	message->to=linphone_address_clone(to);
}

/**
 * Get destination of the message
 * @param[in] message #LinphoneChatMessage obj
 * @return #LinphoneAddress
 */
const LinphoneAddress* linphone_chat_message_get_to_address(const LinphoneChatMessage* message){
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
	if (msg->file_transfer_filepath) new_message->file_transfer_filepath=ms_strdup(msg->file_transfer_filepath);
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
		linphone_content_unref(msg->file_transfer_information);
	}
	if (msg->file_transfer_filepath != NULL) {
		ms_free(msg->file_transfer_filepath);
	}
	linphone_chat_message_cbs_unref(msg->callbacks);
	ms_message("LinphoneChatMessage [%p] destroyed.",msg);
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
 * Set the path to the file to read from or write to during the file transfer.
 * @param[in] msg LinphoneChatMessage object
 * @param[in] filepath The path to the file to use for the file transfer.
 */
void linphone_chat_message_set_file_transfer_filepath(LinphoneChatMessage *msg, const char *filepath) {
	if (msg->file_transfer_filepath != NULL) {
		ms_free(msg->file_transfer_filepath);
	}
	msg->file_transfer_filepath = ms_strdup(filepath);
}

/**
 * Get the path to the file to read from or write to during the file transfer.
 * @param[in] msg LinphoneChatMessage object
 * @return The path to the file to use for the file transfer.
 */
const char * linphone_chat_message_get_file_transfer_filepath(LinphoneChatMessage *msg) {
	return msg->file_transfer_filepath;
}

/**
 * Get the LinphoneChatMessageCbs object associated with the LinphoneChatMessage.
 * @param[in] msg LinphoneChatMessage object
 * @return The LinphoneChatMessageCbs object associated with the LinphoneChatMessage.
 */
LinphoneChatMessageCbs * linphone_chat_message_get_callbacks(const LinphoneChatMessage *msg) {
	return msg->callbacks;
}


/**
 * Create a message attached to a dedicated chat room with a particular content. Use #linphone_chat_room_send_message2 to initiate the transfer
 * @param cr the chat room.
 * @param initial_content #LinphoneContent initial content. #LinphoneCoreVTable.file_transfer_send is invoked later to notify file transfer progress and collect next chunk of the message if #LinphoneContent.data is NULL.
 * @return a new #LinphoneChatMessage
 */

LinphoneChatMessage* linphone_chat_room_create_file_transfer_message(LinphoneChatRoom *cr, LinphoneContent* initial_content) {
	LinphoneChatMessage* msg = belle_sip_object_new(LinphoneChatMessage);
	msg->callbacks=linphone_chat_message_cbs_new();
	msg->chat_room=(LinphoneChatRoom*)cr;
	msg->message = NULL;
	msg->file_transfer_information = linphone_content_copy(initial_content);
	msg->dir=LinphoneChatMessageOutgoing;
	linphone_chat_message_set_to(msg, linphone_chat_room_get_peer_address(cr));
	linphone_chat_message_set_from(msg, linphone_address_new(linphone_core_get_identity(cr->lc)));
	msg->content_type=NULL; /* this will be set to application/vnd.gsma.rcs-ft-http+xml when we will transfer the xml reply from server to the peers */
	msg->http_request=NULL; /* this will store the http request during file upload to the server */
	return msg;
}

/**
 * @}
 */
