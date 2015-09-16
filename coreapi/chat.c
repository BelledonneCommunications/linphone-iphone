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
#include "lime.h"
#include "ortp/b64.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>

#define COMPOSING_DEFAULT_IDLE_TIMEOUT 15
#define COMPOSING_DEFAULT_REFRESH_TIMEOUT 60
#define COMPOSING_DEFAULT_REMOTE_REFRESH_TIMEOUT 120

#define FILE_TRANSFER_KEY_SIZE 32

static void linphone_chat_message_release(LinphoneChatMessage *msg);

static LinphoneChatMessageCbs *linphone_chat_message_cbs_new(void) {
	return belle_sip_object_new(LinphoneChatMessageCbs);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneChatMessageCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneChatMessageCbs, belle_sip_object_t,
						   NULL, // destroy
						   NULL, // clone
						   NULL, // marshal
						   FALSE);

static void linphone_chat_message_set_state(LinphoneChatMessage *msg, LinphoneChatMessageState state) {
	if (state != msg->state) {
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

LinphoneChatMessageCbsFileTransferRecvCb
linphone_chat_message_cbs_get_file_transfer_recv(const LinphoneChatMessageCbs *cbs) {
	return cbs->file_transfer_recv;
}

void linphone_chat_message_cbs_set_file_transfer_recv(LinphoneChatMessageCbs *cbs,
													  LinphoneChatMessageCbsFileTransferRecvCb cb) {
	cbs->file_transfer_recv = cb;
}

LinphoneChatMessageCbsFileTransferSendCb
linphone_chat_message_cbs_get_file_transfer_send(const LinphoneChatMessageCbs *cbs) {
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

static void _linphone_chat_room_send_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg);

static void process_io_error_upload(void *data, const belle_sip_io_error_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("I/O Error during file upload to %s - msg [%p] chat room[%p]",
			 linphone_core_get_file_transfer_server(msg->chat_room->lc), msg, msg->chat_room);
	linphone_chat_message_cancel_file_transfer(msg);
}
static void process_auth_requested_upload(void *data, belle_sip_auth_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("Error during file upload: auth requested to connect %s - msg [%p] chat room[%p]",
			 linphone_core_get_file_transfer_server(msg->chat_room->lc), msg, msg->chat_room);
	linphone_chat_message_cancel_file_transfer(msg);
}

static void process_io_error_download(void *data, const belle_sip_io_error_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("I/O Error during file download %s - msg [%p] chat room[%p]", msg->external_body_url, msg, msg->chat_room);
	linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferError);
}
static void process_auth_requested_download(void *data, belle_sip_auth_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferError);
	ms_error("Error during file download : auth requested to get %s - msg [%p] chat room[%p]", msg->external_body_url,
			 msg, msg->chat_room);
}

static void linphone_chat_message_file_transfer_on_progress(belle_sip_body_handler_t *bh, belle_sip_message_t *m,
															void *data, size_t offset, size_t total) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	if (!msg->http_request || belle_http_request_is_cancelled(msg->http_request)) {
		ms_warning("Cancelled request for msg [%p], ignoring %s", msg, __FUNCTION__);
		return;
	}
	if (linphone_chat_message_cbs_get_file_transfer_progress_indication(msg->callbacks)) {
		linphone_chat_message_cbs_get_file_transfer_progress_indication(msg->callbacks)(
			msg, msg->file_transfer_information, offset, total);
	} else {
		/* Legacy: call back given by application level */
		linphone_core_notify_file_transfer_progress_indication(msg->chat_room->lc, msg, msg->file_transfer_information,
															   offset, total);
	}
}

static int linphone_chat_message_file_transfer_on_send_body(belle_sip_user_body_handler_t *bh, belle_sip_message_t *m,
															void *data, size_t offset, uint8_t *buffer, size_t *size) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	LinphoneCore *lc = msg->chat_room->lc;
	char *buf = (char *)buffer;

	if (!msg->http_request || belle_http_request_is_cancelled(msg->http_request)) {
		ms_warning("Cancelled request for msg [%p], ignoring %s", msg, __FUNCTION__);
		return BELLE_SIP_STOP;
	}

	/* if we've not reach the end of file yet, ask for more data*/
	if (offset < linphone_content_get_size(msg->file_transfer_information)) {
		char *plainBuffer = NULL;

		if (linphone_content_get_key(msg->file_transfer_information) !=
			NULL) { /* if we have a key to cipher the msg, use it! */
			/* if this chunk is not the last one, the lenght must be a multiple of block cipher size(16 bytes)*/
			if (offset + *size < linphone_content_get_size(msg->file_transfer_information)) {
				*size -= (*size % 16);
			}
			plainBuffer = (char *)malloc(*size);
		}

		/* get data from call back */
		if (linphone_chat_message_cbs_get_file_transfer_send(msg->callbacks)) {
			LinphoneBuffer *lb = linphone_chat_message_cbs_get_file_transfer_send(msg->callbacks)(
				msg, msg->file_transfer_information, offset, *size);
			if (lb == NULL) {
				*size = 0;
			} else {
				*size = linphone_buffer_get_size(lb);
				memcpy(plainBuffer ? plainBuffer : buf, linphone_buffer_get_content(lb), *size);
				linphone_buffer_unref(lb);
			}
		} else {
			/* Legacy */
			linphone_core_notify_file_transfer_send(lc, msg, msg->file_transfer_information,
													plainBuffer ? plainBuffer : buf, size);
		}

		if (linphone_content_get_key(msg->file_transfer_information) !=
			NULL) { /* if we have a key to cipher the msg, use it! */
			lime_encryptFile(linphone_content_get_cryptoContext_address(msg->file_transfer_information),
							 (unsigned char *)linphone_content_get_key(msg->file_transfer_information), *size,
							 plainBuffer, (char *)buffer);
			free(plainBuffer);
			/* check if we reach the end of file */
			if (offset + *size >= linphone_content_get_size(msg->file_transfer_information)) {
				/* conclude file ciphering by calling it context with a zero size */
				lime_encryptFile(linphone_content_get_cryptoContext_address(msg->file_transfer_information), NULL, 0,
								 NULL, NULL);
			}
		}
	}

	return BELLE_SIP_CONTINUE;
}

static void linphone_chat_message_process_response_from_post_file(void *data,
																  const belle_http_response_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;

	/* check the answer code */
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code == 204) { /* this is the reply to the first post to the server - an empty msg */
			/* start uploading the file */
			belle_http_request_listener_callbacks_t cbs = {0};
			belle_http_request_listener_t *l;
			belle_generic_uri_t *uri;
			belle_sip_multipart_body_handler_t *bh;
			char *ua;
			char *first_part_header;
			belle_sip_body_handler_t *first_part_bh;

			/* shall we encrypt the file */
			if (linphone_core_lime_for_file_sharing_enabled(msg->chat_room->lc)) {
				char keyBuffer
					[FILE_TRANSFER_KEY_SIZE]; /* temporary storage of generated key: 192 bits of key + 64 bits of
												 initial vector */
				/* generate a random 192 bits key + 64 bits of initial vector and store it into the
				 * file_transfer_information->key field of the msg */
				sal_get_random_bytes((unsigned char *)keyBuffer, FILE_TRANSFER_KEY_SIZE);
				linphone_content_set_key(
					msg->file_transfer_information, keyBuffer,
					FILE_TRANSFER_KEY_SIZE); /* key is duplicated in the content private structure */
				/* temporary storage for the Content-disposition header value : use a generic filename to not leak it
				 * Actual filename stored in msg->file_transfer_information->name will be set in encrypted msg
				 * sended to the  */
				first_part_header = belle_sip_strdup_printf("form-data; name=\"File\"; filename=\"filename.txt\"");
			} else {
				/* temporary storage for the Content-disposition header value */
				first_part_header = belle_sip_strdup_printf("form-data; name=\"File\"; filename=\"%s\"",
															linphone_content_get_name(msg->file_transfer_information));
			}

			/* create a user body handler to take care of the file and add the content disposition and content-type
			 * headers */
			if (msg->file_transfer_filepath != NULL) {
				first_part_bh =
					(belle_sip_body_handler_t *)belle_sip_file_body_handler_new(msg->file_transfer_filepath, NULL, msg);
			} else if (linphone_content_get_buffer(msg->file_transfer_information) != NULL) {
				first_part_bh = (belle_sip_body_handler_t *)belle_sip_memory_body_handler_new_from_buffer(
					linphone_content_get_buffer(msg->file_transfer_information),
					linphone_content_get_size(msg->file_transfer_information), NULL, msg);
			} else {
				first_part_bh = (belle_sip_body_handler_t *)belle_sip_user_body_handler_new(
					linphone_content_get_size(msg->file_transfer_information), NULL, NULL,
					linphone_chat_message_file_transfer_on_send_body, msg);
			}
			belle_sip_body_handler_add_header(first_part_bh,
											  belle_sip_header_create("Content-disposition", first_part_header));
			belle_sip_free(first_part_header);
			belle_sip_body_handler_add_header(first_part_bh,
											  (belle_sip_header_t *)belle_sip_header_content_type_create(
												  linphone_content_get_type(msg->file_transfer_information),
												  linphone_content_get_subtype(msg->file_transfer_information)));

			/* insert it in a multipart body handler which will manage the boundaries of multipart msg */
			bh = belle_sip_multipart_body_handler_new(linphone_chat_message_file_transfer_on_progress, msg,
													  first_part_bh);

			/* create the http request: do not include the msg header at this point, it is done by bellesip when
			 * setting the multipart body handler in the msg */
			ua = ms_strdup_printf("%s/%s", linphone_core_get_user_agent_name(), linphone_core_get_user_agent_version());
			uri = belle_generic_uri_parse(linphone_core_get_file_transfer_server(msg->chat_room->lc));
			if (msg->http_request)
				belle_sip_object_unref(msg->http_request);
			msg->http_request = belle_http_request_create("POST", uri, belle_sip_header_create("User-Agent", ua), NULL);
			belle_sip_object_ref(
				msg->http_request); /* keep a reference to the http request to be able to cancel it during upload */
			ms_free(ua);
			belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(msg->http_request), BELLE_SIP_BODY_HANDLER(bh));
			cbs.process_response = linphone_chat_message_process_response_from_post_file;
			cbs.process_io_error = process_io_error_upload;
			cbs.process_auth_requested = process_auth_requested_upload;
			l = belle_http_request_listener_create_from_callbacks(&cbs, msg);
			belle_http_provider_send_request(msg->chat_room->lc->http_provider, msg->http_request, l);
		}

		if (code == 200) { /* file has been uplaoded correctly, get server reply and send it */
			const char *body = belle_sip_message_get_body((belle_sip_message_t *)event->response);
			belle_sip_object_unref(msg->http_request);
			msg->http_request = NULL;

			/* if we have an encryption key for the file, we must insert it into the msg and restore the correct
			 * filename */
			if (linphone_content_get_key(msg->file_transfer_information) != NULL) {
				/* parse the msg body */
				xmlDocPtr xmlMessageBody = xmlParseDoc((const xmlChar *)body);

				xmlNodePtr cur = xmlDocGetRootElement(xmlMessageBody);
				if (cur != NULL) {
					cur = cur->xmlChildrenNode;
					while (cur != NULL) {
						if (!xmlStrcmp(cur->name, (const xmlChar *)"file-info")) { /* we found a file info node, check
																					  it has a type="file" attribute */
							xmlChar *typeAttribute = xmlGetProp(cur, (const xmlChar *)"type");
							if (!xmlStrcmp(typeAttribute,
										   (const xmlChar *)"file")) { /* this is the node we are looking for : add a
																		  file-key children node */
								xmlNodePtr fileInfoNodeChildren =
									cur
										->xmlChildrenNode; /* need to parse the children node to update the file-name
															  one */
								/* convert key to base64 */
								int b64Size = b64_encode(NULL, FILE_TRANSFER_KEY_SIZE, NULL, 0);
								char *keyb64 = (char *)malloc(b64Size + 1);
								int xmlStringLength;

								b64Size = b64_encode(linphone_content_get_key(msg->file_transfer_information),
													 FILE_TRANSFER_KEY_SIZE, keyb64, b64Size);
								keyb64[b64Size] = '\0'; /* libxml need a null terminated string */

								/* add the node containing the key to the file-info node */
								xmlNewTextChild(cur, NULL, (const xmlChar *)"file-key", (const xmlChar *)keyb64);
								xmlFree(typeAttribute);
								free(keyb64);

								/* look for the file-name node and update its content */
								while (fileInfoNodeChildren != NULL) {
									if (!xmlStrcmp(
											fileInfoNodeChildren->name,
											(const xmlChar *)"file-name")) { /* we found a the file-name node, update
																				its content with the real filename */
										/* update node content */
										xmlNodeSetContent(fileInfoNodeChildren,
														  (const xmlChar *)(linphone_content_get_name(
															  msg->file_transfer_information)));
										break;
									}
									fileInfoNodeChildren = fileInfoNodeChildren->next;
								}

								/* dump the xml into msg->message */
								xmlDocDumpFormatMemoryEnc(xmlMessageBody, (xmlChar **)&msg->message, &xmlStringLength,
														  "UTF-8", 0);

								break;
							}
							xmlFree(typeAttribute);
						}
						cur = cur->next;
					}
				}
				xmlFreeDoc(xmlMessageBody);

			} else { /* no encryption key, transfer in plain, just copy the msg sent by server */
				msg->message = ms_strdup(body);
			}

			msg->content_type = ms_strdup("application/vnd.gsma.rcs-ft-http+xml");
			linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferDone);
			_linphone_chat_room_send_message(msg->chat_room, msg);
		}
	}
}

static void _linphone_chat_message_destroy(LinphoneChatMessage *msg);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneChatMessage);

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

const MSList *linphone_core_get_chat_rooms(LinphoneCore *lc) {
	return lc->chatrooms;
}

static bool_t linphone_chat_room_matches(LinphoneChatRoom *cr, const LinphoneAddress *from) {
	return linphone_address_weak_equal(cr->peer_url, from);
}

static void _linphone_chat_room_destroy(LinphoneChatRoom *obj);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneChatRoom);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneChatRoom, belle_sip_object_t,
						   (belle_sip_object_destroy_t)_linphone_chat_room_destroy,
						   NULL, // clone
						   NULL, // marshal
						   FALSE);

static LinphoneChatRoom *_linphone_core_create_chat_room(LinphoneCore *lc, LinphoneAddress *addr) {
	LinphoneChatRoom *cr = belle_sip_object_new(LinphoneChatRoom);
	cr->lc = lc;
	cr->peer = linphone_address_as_string(addr);
	cr->peer_url = addr;
	cr->unread_count = -1;
	lc->chatrooms = ms_list_append(lc->chatrooms, (void *)cr);
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
	MSList *elem;
	for (elem = lc->chatrooms; elem != NULL; elem = ms_list_next(elem)) {
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
	linphone_address_destroy(to_addr);
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
	if (ms_list_find(lc->chatrooms, cr)) {
		lc->chatrooms = ms_list_remove(cr->lc->chatrooms, cr);
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

static void _linphone_chat_room_destroy(LinphoneChatRoom *cr) {
	ms_list_free_with_data(cr->transient_messages, (void (*)(void *))linphone_chat_message_release);
	linphone_chat_room_delete_composing_idle_timer(cr);
	linphone_chat_room_delete_composing_refresh_timer(cr);
	linphone_chat_room_delete_remote_composing_refresh_timer(cr);
	if (cr->lc != NULL) {
		if (ms_list_find(cr->lc->chatrooms, cr)) {
			ms_error("LinphoneChatRoom[%p] is destroyed while still being used by the LinphoneCore. This is abnormal."
					 " linphone_core_get_chat_room() doesn't give a reference, there is no need to call "
					 "linphone_chat_room_unref(). "
					 "In order to remove a chat room from the core, use linphone_core_delete_chat_room().",
					 cr);
			cr->lc->chatrooms = ms_list_remove(cr->lc->chatrooms, cr);
		}
	}
	linphone_address_destroy(cr->peer_url);
	ms_free(cr->peer);
}

void linphone_chat_room_destroy(LinphoneChatRoom *cr) {
	linphone_chat_room_unref(cr);
}

void linphone_chat_room_release(LinphoneChatRoom *cr) {
	cr->lc = NULL;
	linphone_chat_room_unref(cr);
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

static void _linphone_chat_room_send_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	SalOp *op = NULL;
	LinphoneCall *call;
	char *content_type;
	const char *identity = NULL;
	time_t t = time(NULL);
	linphone_chat_message_ref(msg);
	/* Check if we shall upload a file to a server */
	if (msg->file_transfer_information != NULL && msg->content_type == NULL) {
		/* open a transaction with the server and send an empty request(RCS5.1 section 3.5.4.8.3.1) */
		belle_http_request_listener_callbacks_t cbs = {0};
		belle_http_request_listener_t *l;
		belle_generic_uri_t *uri;
		const char *transfer_server = linphone_core_get_file_transfer_server(cr->lc);

		if (transfer_server == NULL) {
			ms_warning("Cannot send file transfer msg: no file transfer server configured.");
			return;
		}
		uri = belle_generic_uri_parse(transfer_server);

		msg->http_request = belle_http_request_create("POST", uri, NULL, NULL, NULL);
		belle_sip_object_ref(msg->http_request); /* keep a reference on the request to be able to cancel it */
		cbs.process_response = linphone_chat_message_process_response_from_post_file;
		cbs.process_io_error = process_io_error_upload;
		cbs.process_auth_requested = process_auth_requested_upload;
		l = belle_http_request_listener_create_from_callbacks(
			&cbs, msg); /* give msg to listener to be able to start the actual file upload when server answer a 204 No
						   content */
		belle_http_provider_send_request(cr->lc->http_provider, msg->http_request, l);
		linphone_chat_message_unref(msg);
		return;
	}

	if (lp_config_get_int(cr->lc->config, "sip", "chat_use_call_dialogs", 0)) {
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
	msg->time = t;
	if (op == NULL) {
		LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(cr->lc, cr->peer_url);
		if (proxy) {
			identity = linphone_proxy_config_get_identity(proxy);
		} else
			identity = linphone_core_get_primary_contact(cr->lc);
		/*sending out of calls*/
		msg->op = op = sal_op_new(cr->lc->sal);
		linphone_configure_op(cr->lc, op, cr->peer_url, msg->custom_headers,
							  lp_config_get_int(cr->lc->config, "sip", "chat_msg_with_contact", 0));
		sal_op_set_user_pointer(op, msg); /*if out of call, directly store msg*/
	}

	if (msg->external_body_url) {
		content_type = ms_strdup_printf("message/external-body; access-type=URL; URL=\"%s\"", msg->external_body_url);
		sal_message_send(op, identity, cr->peer, content_type, NULL, NULL);
		ms_free(content_type);
	} else {
		char *peer_uri = linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(cr));
		const char *content_type;

		if (linphone_core_lime_enabled(cr->lc)) {
			linphone_chat_message_ref(
				msg); /* ref the msg or it may be destroyed by callback if the encryption failed */
			if (msg->content_type && strcmp(msg->content_type, "application/vnd.gsma.rcs-ft-http+xml") == 0) {
				content_type =
					"application/cipher.vnd.gsma.rcs-ft-http+xml"; /* it's a file transfer, content type shall be set to
																	  application/cipher.vnd.gsma.rcs-ft-http+xml*/
			} else {
				content_type = "xml/cipher";
			}
		} else {
			content_type = msg->content_type;
		}

		if (content_type == NULL) {
			sal_text_send(op, identity, cr->peer, msg->message);
		} else {
			sal_message_send(op, identity, cr->peer, content_type, msg->message, peer_uri);
		}
		ms_free(peer_uri);
	}

	msg->dir = LinphoneChatMessageOutgoing;
	msg->from = linphone_address_new(identity);
	msg->storage_id = linphone_chat_message_store(msg);

	if (cr->unread_count >= 0 && !msg->is_read)
		cr->unread_count++;

	// add to transient list
	cr->transient_messages = ms_list_append(cr->transient_messages, linphone_chat_message_ref(msg));

	if (cr->is_composing == LinphoneIsComposingActive) {
		cr->is_composing = LinphoneIsComposingIdle;
	}
	linphone_chat_room_delete_composing_idle_timer(cr);
	linphone_chat_room_delete_composing_refresh_timer(cr);
	linphone_chat_message_unref(msg);
}

void linphone_chat_message_update_state(LinphoneChatMessage *msg, LinphoneChatMessageState new_state) {
	linphone_chat_message_set_state(msg, new_state);
	linphone_chat_message_store_state(msg);

	if (msg->state == LinphoneChatMessageStateDelivered || msg->state == LinphoneChatMessageStateNotDelivered) {
		// msg is not transient anymore, we can remove it from our transient list and unref it :
		msg->chat_room->transient_messages = ms_list_remove(msg->chat_room->transient_messages, msg);
		linphone_chat_message_unref(msg);
	}
}

/**
 * Send a msg to peer member of this chat room.
 * @deprecated linphone_chat_room_send_message2() gives more control on the msg expedition.
 * @param cr #LinphoneChatRoom object
 * @param msg msg to be sent
 */
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
}

void linphone_core_message_received(LinphoneCore *lc, SalOp *op, const SalMessage *sal_msg) {
	LinphoneChatRoom *cr = NULL;
	LinphoneAddress *addr;
	LinphoneChatMessage *msg;
	const SalCustomHeader *ch;

	addr = linphone_address_new(sal_msg->from);
	linphone_address_clean(addr);
	cr = linphone_core_get_chat_room(lc, addr);

	if (sal_msg->content_type !=
		NULL) { /* content_type field is, for now, used only for rcs file transfer but we shall strcmp it with
				   "application/vnd.gsma.rcs-ft-http+xml" */
		xmlChar *file_url = NULL;
		xmlDocPtr xmlMessageBody;
		xmlNodePtr cur;

		msg = linphone_chat_room_create_message(cr, NULL); /* create a msg with empty body */
		msg->content_type =
			ms_strdup(sal_msg->content_type); /* add the content_type "application/vnd.gsma.rcs-ft-http+xml" */
		msg->file_transfer_information = linphone_content_new();

		/* parse the msg body to get all informations from it */
		xmlMessageBody = xmlParseDoc((const xmlChar *)sal_msg->text);

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
								linphone_content_set_name(
									msg->file_transfer_information,
									(const char *)xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1));
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

							if (!xmlStrcmp(cur->name,
										   (const xmlChar *)"file-key")) { /* there is a key in the msg: file has
																			  been encrypted */
								/* convert the key from base 64 */
								xmlChar *keyb64 = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
								int keyLength = b64_decode((char *)keyb64, strlen((char *)keyb64), NULL, 0);
								uint8_t *keyBuffer = (uint8_t *)malloc(keyLength);
								/* decode the key into local key buffer */
								b64_decode((char *)keyb64, strlen((char *)keyb64), keyBuffer, keyLength);
								linphone_content_set_key(
									msg->file_transfer_information, (char *)keyBuffer,
									keyLength); /* duplicate key value into the linphone content private structure */
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
	} else { /* msg is not rcs file transfer, create it with provided sal_msg->text as ->msg */
		msg = linphone_chat_room_create_message(cr, sal_msg->text);
	}
	linphone_chat_message_set_from(msg, cr->peer_url);

	{
		LinphoneAddress *to;
		to = sal_op_get_to(op) ? linphone_address_new(sal_op_get_to(op))
							   : linphone_address_new(linphone_core_get_identity(lc));
		msg->to = to;
	}

	msg->time = sal_msg->time;
	linphone_chat_message_set_state(msg, LinphoneChatMessageStateDelivered);
	msg->is_read = FALSE;
	msg->dir = LinphoneChatMessageIncoming;
	ch = sal_op_get_recv_custom_header(op);
	if (ch)
		msg->custom_headers = sal_custom_header_clone(ch);

	if (sal_msg->url) {
		linphone_chat_message_set_external_body_url(msg, sal_msg->url);
	}

	linphone_address_destroy(addr);
	msg->storage_id = linphone_chat_message_store(msg);

	if (cr->unread_count < 0)
		cr->unread_count = 1;
	else
		cr->unread_count++;

	linphone_chat_room_message_received(cr, lc, msg);
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

void linphone_core_is_composing_received(LinphoneCore *lc, SalOp *op, const SalIsComposing *is_composing) {
	LinphoneAddress *addr = linphone_address_new(is_composing->from);
	LinphoneChatRoom *cr = _linphone_core_get_chat_room(lc, addr);
	if (cr != NULL) {
		linphone_chat_room_notify_is_composing(cr, is_composing->text);
	}
	linphone_address_destroy(addr);
}

bool_t linphone_chat_room_is_remote_composing(const LinphoneChatRoom *cr) {
	return (cr->remote_is_composing == LinphoneIsComposingActive) ? TRUE : FALSE;
}

LinphoneCore *linphone_chat_room_get_lc(LinphoneChatRoom *cr) {
	return cr->lc;
}

LinphoneCore *linphone_chat_room_get_core(LinphoneChatRoom *cr) {
	return cr->lc;
}

const LinphoneAddress *linphone_chat_room_get_peer_address(LinphoneChatRoom *cr) {
	return cr->peer_url;
}

LinphoneChatMessage *linphone_chat_room_create_message(LinphoneChatRoom *cr, const char *message) {
	LinphoneChatMessage *msg = belle_sip_object_new(LinphoneChatMessage);
	msg->callbacks = linphone_chat_message_cbs_new();
	msg->chat_room = (LinphoneChatRoom *)cr;
	msg->message = message ? ms_strdup(message) : NULL;
	msg->is_read = TRUE;
	msg->content_type = NULL;			   /* this property is used only when transfering file */
	msg->file_transfer_information = NULL; /* this property is used only when transfering file */
	msg->http_request = NULL;
	return msg;
}

LinphoneChatMessage *linphone_chat_room_create_message_2(LinphoneChatRoom *cr, const char *message,
														 const char *external_body_url, LinphoneChatMessageState state,
														 time_t time, bool_t is_read, bool_t is_incoming) {
	LinphoneCore *lc = linphone_chat_room_get_lc(cr);

	LinphoneChatMessage *msg = belle_sip_object_new(LinphoneChatMessage);
	msg->callbacks = linphone_chat_message_cbs_new();
	msg->chat_room = (LinphoneChatRoom *)cr;
	msg->message = message ? ms_strdup(message) : NULL;
	msg->external_body_url = external_body_url ? ms_strdup(external_body_url) : NULL;
	msg->time = time;
	linphone_chat_message_set_state(msg, state);
	msg->is_read = is_read;
	msg->content_type = NULL;			   /* this property is used only when transfering file */
	msg->file_transfer_information = NULL; /* this property is used only when transfering file */
	if (is_incoming) {
		msg->dir = LinphoneChatMessageIncoming;
		linphone_chat_message_set_from(msg, linphone_chat_room_get_peer_address(cr));
		linphone_chat_message_set_to(msg, linphone_address_new(linphone_core_get_identity(lc)));
	} else {
		msg->dir = LinphoneChatMessageOutgoing;
		linphone_chat_message_set_to(msg, linphone_chat_room_get_peer_address(cr));
		linphone_chat_message_set_from(msg, linphone_address_new(linphone_core_get_identity(lc)));
	}
	return msg;
}

void linphone_chat_room_send_message2(LinphoneChatRoom *cr, LinphoneChatMessage *msg,
									  LinphoneChatMessageStateChangedCb status_cb, void *ud) {
	msg->message_state_changed_cb = status_cb;
	msg->message_state_changed_user_data = ud;
	linphone_chat_message_set_state(msg, LinphoneChatMessageStateInProgress);
	_linphone_chat_room_send_message(cr, msg);
}

void linphone_chat_room_send_chat_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	linphone_chat_message_set_state(msg, LinphoneChatMessageStateInProgress);
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
	LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(cr->lc, cr->peer_url);
	if (proxy)
		identity = linphone_proxy_config_get_identity(proxy);
	else
		identity = linphone_core_get_primary_contact(cr->lc);
	/*sending out of calls*/
	op = sal_op_new(cr->lc->sal);
	linphone_configure_op(cr->lc, op, cr->peer_url, NULL,
						  lp_config_get_int(cr->lc->config, "sip", "chat_msg_with_contact", 0));

	content = linphone_chat_room_create_is_composing_xml(cr);
	if (content != NULL) {
		sal_message_send(op, identity, cr->peer, "application/im-iscomposing+xml", content, NULL);
		ms_free(content);
		sal_op_unref(op);
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

const LinphoneContent *linphone_chat_message_get_file_transfer_information(const LinphoneChatMessage *msg) {
	return msg->file_transfer_information;
}

static void on_recv_body(belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, void *data, size_t offset,
						 const uint8_t *buffer, size_t size) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	LinphoneCore *lc = msg->chat_room->lc;

	if (!msg->http_request || belle_http_request_is_cancelled(msg->http_request)) {
		ms_warning("Cancelled request for msg [%p], ignoring %s", msg, __FUNCTION__);
		return;
	}

	/* first call may be with a zero size, ignore it */
	if (size == 0) {
		return;
	}

	if (linphone_content_get_key(msg->file_transfer_information) !=
		NULL) { /* we have a key, we must decrypt the file */
		/* get data from callback to a plainBuffer */
		char *plainBuffer = (char *)malloc(size);
		lime_decryptFile(linphone_content_get_cryptoContext_address(msg->file_transfer_information),
						 (unsigned char *)linphone_content_get_key(msg->file_transfer_information), size, plainBuffer,
						 (char *)buffer);
		if (linphone_chat_message_cbs_get_file_transfer_recv(msg->callbacks)) {
			LinphoneBuffer *lb = linphone_buffer_new_from_data((unsigned char *)plainBuffer, size);
			linphone_chat_message_cbs_get_file_transfer_recv(msg->callbacks)(msg, msg->file_transfer_information, lb);
			linphone_buffer_unref(lb);
		} else {
			/* legacy: call back given by application level */
			linphone_core_notify_file_transfer_recv(lc, msg, msg->file_transfer_information, plainBuffer, size);
		}
		free(plainBuffer);
	} else { /* regular file, no deciphering */
		if (linphone_chat_message_cbs_get_file_transfer_recv(msg->callbacks)) {
			LinphoneBuffer *lb = linphone_buffer_new_from_data(buffer, size);
			linphone_chat_message_cbs_get_file_transfer_recv(msg->callbacks)(msg, msg->file_transfer_information, lb);
			linphone_buffer_unref(lb);
		} else {
			/* Legacy: call back given by application level */
			linphone_core_notify_file_transfer_recv(lc, msg, msg->file_transfer_information, (char *)buffer, size);
		}
	}

	return;
}

static LinphoneContent *linphone_chat_create_file_transfer_information_from_headers(const belle_sip_message_t *m) {
	LinphoneContent *content = linphone_content_new();

	belle_sip_header_content_length_t *content_length_hdr =
		BELLE_SIP_HEADER_CONTENT_LENGTH(belle_sip_message_get_header(m, "Content-Length"));
	belle_sip_header_content_type_t *content_type_hdr =
		BELLE_SIP_HEADER_CONTENT_TYPE(belle_sip_message_get_header(m, "Content-Type"));
	const char *type = NULL, *subtype = NULL;

	linphone_content_set_name(content, "");

	if (content_type_hdr) {
		type = belle_sip_header_content_type_get_type(content_type_hdr);
		subtype = belle_sip_header_content_type_get_subtype(content_type_hdr);
		ms_message("Extracted content type %s / %s from header", type ? type : "", subtype ? subtype : "");
		if (type)
			linphone_content_set_type(content, type);
		if (subtype)
			linphone_content_set_subtype(content, subtype);
	}

	if (content_length_hdr) {
		linphone_content_set_size(content, belle_sip_header_content_length_get_content_length(content_length_hdr));
		ms_message("Extracted content length %i from header", (int)linphone_content_get_size(content));
	}

	return content;
}

static void linphone_chat_process_response_headers_from_get_file(void *data, const belle_http_response_event_t *event) {
	if (event->response) {
		/*we are receiving a response, set a specific body handler to acquire the response.
		 * if not done, belle-sip will create a memory body handler, the default*/
		LinphoneChatMessage *msg =
			(LinphoneChatMessage *)belle_sip_object_data_get(BELLE_SIP_OBJECT(event->request), "msg");
		belle_sip_message_t *response = BELLE_SIP_MESSAGE(event->response);
		size_t body_size = 0;

		if (msg->file_transfer_information == NULL) {
			ms_warning("No file transfer information for msg %p: creating...", msg);
			msg->file_transfer_information = linphone_chat_create_file_transfer_information_from_headers(response);
		}

		if (msg->file_transfer_information) {
			body_size = linphone_content_get_size(msg->file_transfer_information);
		}

		if (msg->file_transfer_filepath == NULL) {
			belle_sip_message_set_body_handler(
				(belle_sip_message_t *)event->response,
				(belle_sip_body_handler_t *)belle_sip_user_body_handler_new(
					body_size, linphone_chat_message_file_transfer_on_progress, on_recv_body, NULL, msg));
		} else {
			belle_sip_body_handler_t *bh = (belle_sip_body_handler_t *)belle_sip_file_body_handler_new(
				msg->file_transfer_filepath, linphone_chat_message_file_transfer_on_progress, msg);
			if (belle_sip_body_handler_get_size(bh) == 0) {
				/* If the size of the body has not been initialized from the file stat, use the one from the
				 * file_transfer_information. */
				belle_sip_body_handler_set_size(bh, body_size);
			}
			belle_sip_message_set_body_handler((belle_sip_message_t *)event->response, bh);
		}
	}
}

static void linphone_chat_process_response_from_get_file(void *data, const belle_http_response_event_t *event) {
	/* check the answer code */
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code == 200) {
			LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
			LinphoneCore *lc = msg->chat_room->lc;
			/* if the file was encrypted, finish the decryption and free context */
			if (linphone_content_get_key(msg->file_transfer_information) != NULL) {
				lime_decryptFile(linphone_content_get_cryptoContext_address(msg->file_transfer_information), NULL, 0,
								 NULL, NULL);
			}
			/* file downloaded succesfully, call again the callback with size at zero */
			if (linphone_chat_message_cbs_get_file_transfer_recv(msg->callbacks)) {
				LinphoneBuffer *lb = linphone_buffer_new();
				linphone_chat_message_cbs_get_file_transfer_recv(msg->callbacks)(msg, msg->file_transfer_information,
																				 lb);
				linphone_buffer_unref(lb);
			} else {
				linphone_core_notify_file_transfer_recv(lc, msg, msg->file_transfer_information, NULL, 0);
			}
			linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferDone);
		}
	}
}

void linphone_chat_message_download_file(LinphoneChatMessage *msg) {
	belle_http_request_listener_callbacks_t cbs = {0};
	belle_http_request_listener_t *l;
	belle_generic_uri_t *uri;
	const char *url = msg->external_body_url;
	char *ua;

	if (url == NULL) {
		ms_error("Cannot download file from chat msg [%p] because url is NULL", msg);
		return;
	}
	ua = ms_strdup_printf("%s/%s", linphone_core_get_user_agent_name(), linphone_core_get_user_agent_version());
	uri = belle_generic_uri_parse(url);

	msg->http_request = belle_http_request_create("GET", uri, belle_sip_header_create("User-Agent", ua), NULL);
	belle_sip_object_ref(msg->http_request); /* keep a reference on the request to be able to cancel the download */
	ms_free(ua);

	cbs.process_response_headers = linphone_chat_process_response_headers_from_get_file;
	cbs.process_response = linphone_chat_process_response_from_get_file;
	cbs.process_io_error = process_io_error_download;
	cbs.process_auth_requested = process_auth_requested_download;
	l = belle_http_request_listener_create_from_callbacks(&cbs, (void *)msg);
	belle_sip_object_data_set(BELLE_SIP_OBJECT(msg->http_request), "msg", (void *)msg, NULL);
	linphone_chat_message_set_state(msg, LinphoneChatMessageStateInProgress); /* start the download, status is In Progress */
	belle_http_provider_send_request(msg->chat_room->lc->http_provider, msg->http_request, l);
}

void linphone_chat_message_start_file_download(LinphoneChatMessage *msg,
											   LinphoneChatMessageStateChangedCb status_cb, void *ud) {
	msg->message_state_changed_cb = status_cb;
	msg->message_state_changed_user_data = ud;
	linphone_chat_message_download_file(msg);
}

void linphone_chat_message_cancel_file_transfer(LinphoneChatMessage *msg) {
	if (msg->http_request) {
		if (!belle_http_request_is_cancelled(msg->http_request)) {
			ms_message("Cancelling file transfer %s - msg [%p] chat room[%p]",
					   (msg->external_body_url == NULL) ? linphone_core_get_file_transfer_server(msg->chat_room->lc)
														: msg->external_body_url,
					   msg, msg->chat_room);

			belle_http_provider_cancel_request(msg->chat_room->lc->http_provider, msg->http_request);
			belle_sip_object_unref(msg->http_request);
			msg->http_request = NULL;
			linphone_chat_message_set_state(msg, LinphoneChatMessageStateNotDelivered);
		}
	} else {
		ms_message("No existing file transfer - nothing to cancel");
	}
}

void linphone_chat_message_set_from_address(LinphoneChatMessage *msg, const LinphoneAddress *from) {
	if (msg->from)
		linphone_address_destroy(msg->from);
	msg->from = linphone_address_clone(from);
}

const LinphoneAddress *linphone_chat_message_get_from_address(const LinphoneChatMessage *msg) {
	return msg->from;
}

void linphone_chat_message_set_to_address(LinphoneChatMessage *msg, const LinphoneAddress *to) {
	if (msg->to)
		linphone_address_destroy(msg->to);
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

bool_t linphone_chat_message_is_read(LinphoneChatMessage *msg) {
	return msg->is_read;
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
		linphone_address_destroy(msg->from);
	if (msg->to)
		linphone_address_destroy(msg->to);
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
	linphone_chat_message_cbs_unref(msg->callbacks);
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
	linphone_chat_message_unref(msg);
}

const LinphoneErrorInfo *linphone_chat_message_get_error_info(const LinphoneChatMessage *msg) {
	return linphone_error_info_from_sal_op(msg->op);
}

LinphoneReason linphone_chat_message_get_reason(LinphoneChatMessage *msg) {
	return linphone_error_info_get_reason(linphone_chat_message_get_error_info(msg));
}

void linphone_chat_message_set_file_transfer_filepath(LinphoneChatMessage *msg, const char *filepath) {
	if (msg->file_transfer_filepath != NULL) {
		ms_free(msg->file_transfer_filepath);
	}
	msg->file_transfer_filepath = ms_strdup(filepath);
}

const char *linphone_chat_message_get_file_transfer_filepath(LinphoneChatMessage *msg) {
	return msg->file_transfer_filepath;
}

LinphoneChatMessageCbs *linphone_chat_message_get_callbacks(const LinphoneChatMessage *msg) {
	return msg->callbacks;
}

LinphoneChatMessage *linphone_chat_room_create_file_transfer_message(LinphoneChatRoom *cr,
																	 const LinphoneContent *initial_content) {
	LinphoneChatMessage *msg = belle_sip_object_new(LinphoneChatMessage);
	msg->callbacks = linphone_chat_message_cbs_new();
	msg->chat_room = (LinphoneChatRoom *)cr;
	msg->message = NULL;
	msg->is_read = TRUE;
	msg->file_transfer_information = linphone_content_copy(initial_content);
	msg->dir = LinphoneChatMessageOutgoing;
	linphone_chat_message_set_to(msg, linphone_chat_room_get_peer_address(cr));
	linphone_chat_message_set_from(msg, linphone_address_new(linphone_core_get_identity(cr->lc)));
	msg->content_type =
		NULL; /* this will be set to application/vnd.gsma.rcs-ft-http+xml when we will transfer the xml reply from
				 server to the peers */
	msg->http_request = NULL; /* this will store the http request during file upload to the server */
	return msg;
}
