/***************************************************************************
 *            chat_file_transfer.c
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
#include "ortp/b64.h"

static bool_t file_transfer_in_progress_and_valid(LinphoneChatMessage* msg) {
	return (msg->chat_room && msg->chat_room->lc && msg->http_request && !belle_http_request_is_cancelled(msg->http_request));
}

static void _release_http_request(LinphoneChatMessage* msg) {
	if (msg->http_request) {
		belle_sip_object_unref(msg->http_request);
		msg->http_request = NULL;
		if (msg->http_listener){
			belle_sip_object_unref(msg->http_listener);
			msg->http_listener = NULL;
			// unhold the reference that the listener was holding on the message
			linphone_chat_message_unref(msg);
		}
	}
}

static void linphone_chat_message_process_io_error_upload(void *data, const belle_sip_io_error_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("I/O Error during file upload of msg [%p]", msg);
	linphone_chat_message_update_state(msg, LinphoneChatMessageStateNotDelivered);
	_release_http_request(msg);
	linphone_chat_message_unref(msg);
}

static void linphone_chat_message_process_auth_requested_upload(void *data, belle_sip_auth_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("Error during file upload: auth requested for msg [%p]", msg);
	linphone_chat_message_update_state(msg, LinphoneChatMessageStateNotDelivered);
	_release_http_request(msg);
	linphone_chat_message_unref(msg);
}

static void linphone_chat_message_process_io_error_download(void *data, const belle_sip_io_error_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("I/O Error during file download msg [%p]", msg);
	linphone_chat_message_update_state(msg, LinphoneChatMessageStateFileTransferError);
	_release_http_request(msg);
}

static void linphone_chat_message_process_auth_requested_download(void *data, belle_sip_auth_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("Error during file download : auth requested for msg [%p]", msg);
	linphone_chat_message_update_state(msg, LinphoneChatMessageStateFileTransferError);
	_release_http_request(msg);
}

static void linphone_chat_message_file_transfer_on_progress(belle_sip_body_handler_t *bh, belle_sip_message_t *m,
															void *data, size_t offset, size_t total) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	if (msg->http_request && !file_transfer_in_progress_and_valid(msg)) {
		ms_warning("Cancelled request for %s msg [%p], ignoring %s", msg->chat_room?"":"ORPHAN", msg, __FUNCTION__);
		_release_http_request(msg);
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

static int on_send_body(belle_sip_user_body_handler_t *bh, belle_sip_message_t *m,
															void *data, size_t offset, uint8_t *buffer, size_t *size) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	LinphoneCore *lc = NULL;
	LinphoneImEncryptionEngine *imee = NULL;
	int retval = -1;

	if (!file_transfer_in_progress_and_valid(msg)) {
		if (msg->http_request) {
			ms_warning("Cancelled request for %s msg [%p], ignoring %s", msg->chat_room?"":"ORPHAN", msg, __FUNCTION__);
			_release_http_request(msg);
		}
		return BELLE_SIP_STOP;
	}

	lc = msg->chat_room->lc;
	/* if we've not reach the end of file yet, ask for more data */
	/* in case of file body handler, won't be called */
	if (msg->file_transfer_filepath == NULL && offset < linphone_content_get_size(msg->file_transfer_information)) {
		/* get data from call back */
		LinphoneChatMessageCbsFileTransferSendCb file_transfer_send_cb = linphone_chat_message_cbs_get_file_transfer_send(msg->callbacks);
		if (file_transfer_send_cb) {
			LinphoneBuffer *lb = file_transfer_send_cb(msg, msg->file_transfer_information, offset, *size);
			if (lb == NULL) {
				*size = 0;
			} else {
				*size = linphone_buffer_get_size(lb);
				memcpy(buffer, linphone_buffer_get_content(lb), *size);
				linphone_buffer_unref(lb);
			}
		} else {
			/* Legacy */
			linphone_core_notify_file_transfer_send(lc, msg, msg->file_transfer_information, (char *)buffer, size);
		}
	}
		
	imee = linphone_core_get_im_encryption_engine(lc);
	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsUploadingFileCb cb_process_uploading_file = linphone_im_encryption_engine_cbs_get_process_uploading_file(imee_cbs);
		if (cb_process_uploading_file) {
			char *encrypted_buffer = (char *)ms_malloc0(*size);
			retval = cb_process_uploading_file(imee, msg, offset, (const char *)buffer, size, encrypted_buffer);
			if (retval == 0) {
				memcpy(buffer, encrypted_buffer, *size);
			}
			ms_free(encrypted_buffer);
		}
	}

	return retval <= 0 ? BELLE_SIP_CONTINUE : BELLE_SIP_STOP;
}

static void on_send_end(belle_sip_user_body_handler_t *bh, void *data) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	LinphoneCore *lc = msg->chat_room->lc;
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(lc);
	
	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsUploadingFileCb cb_process_uploading_file = linphone_im_encryption_engine_cbs_get_process_uploading_file(imee_cbs);
		if (cb_process_uploading_file) {
			cb_process_uploading_file(imee, msg, 0, NULL, NULL, NULL);
		}
	}
}

static void file_upload_end_background_task(LinphoneChatMessage *obj){
	if (obj->bg_task_id){
		ms_message("channel [%p]: ending file upload background task with id=[%lx].",obj,obj->bg_task_id);
		sal_end_background_task(obj->bg_task_id);
		obj->bg_task_id=0;
	}
}

static void file_upload_background_task_ended(LinphoneChatMessage *obj){
	ms_warning("channel [%p]: file upload background task has to be ended now, but work isn't finished.",obj);
	file_upload_end_background_task(obj);
}

static void file_upload_begin_background_task(LinphoneChatMessage *obj){
	if (obj->bg_task_id==0){
		obj->bg_task_id=sal_begin_background_task("file transfer upload",(void (*)(void*))file_upload_background_task_ended, obj);
		if (obj->bg_task_id) ms_message("channel [%p]: starting file upload background task with id=[%lx].",obj,obj->bg_task_id);
	}
}

static void linphone_chat_message_process_response_from_post_file(void *data,
																  const belle_http_response_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;

	if (msg->http_request && !file_transfer_in_progress_and_valid(msg)) {
		ms_warning("Cancelled request for %s msg [%p], ignoring %s", msg->chat_room?"":"ORPHAN", msg, __FUNCTION__);
		_release_http_request(msg);
		return;
	}

	/* check the answer code */
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code == 204) { /* this is the reply to the first post to the server - an empty msg */
			/* start uploading the file */
			belle_sip_multipart_body_handler_t *bh;
			char *first_part_header;
			belle_sip_body_handler_t *first_part_bh;

			bool_t is_file_encryption_enabled = FALSE;
			LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(msg->chat_room->lc);
			if (imee) {
				LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb is_encryption_enabled_for_file_transfer_cb = 
					linphone_im_encryption_engine_cbs_get_is_encryption_enabled_for_file_transfer(imee_cbs);
				if (is_encryption_enabled_for_file_transfer_cb) {
					is_file_encryption_enabled = is_encryption_enabled_for_file_transfer_cb(imee, msg->chat_room);
				}
			}
			/* shall we encrypt the file */
			if (is_file_encryption_enabled) {
				LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsGenerateFileTransferKeyCb generate_file_transfer_key_cb = 
					linphone_im_encryption_engine_cbs_get_generate_file_transfer_key(imee_cbs);
				if (generate_file_transfer_key_cb) {
					generate_file_transfer_key_cb(imee, msg->chat_room, msg);
				}
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
			first_part_bh = (belle_sip_body_handler_t *)belle_sip_user_body_handler_new(
					linphone_content_get_size(msg->file_transfer_information), 
					linphone_chat_message_file_transfer_on_progress, NULL, NULL,
					on_send_body, on_send_end, msg);
			if (msg->file_transfer_filepath != NULL) {
				belle_sip_user_body_handler_t *body_handler = (belle_sip_user_body_handler_t *)first_part_bh;
				first_part_bh = (belle_sip_body_handler_t *)belle_sip_file_body_handler_new(msg->file_transfer_filepath, 
					NULL, msg); // No need to add again the callback for progression, otherwise it will be called twice
				linphone_content_set_size(msg->file_transfer_information, belle_sip_file_body_handler_get_file_size((belle_sip_file_body_handler_t *)first_part_bh));
				belle_sip_file_body_handler_set_user_body_handler((belle_sip_file_body_handler_t *)first_part_bh, body_handler);
			} else if (linphone_content_get_buffer(msg->file_transfer_information) != NULL) {
				first_part_bh = (belle_sip_body_handler_t *)belle_sip_memory_body_handler_new_from_buffer(
					linphone_content_get_buffer(msg->file_transfer_information),
					linphone_content_get_size(msg->file_transfer_information), linphone_chat_message_file_transfer_on_progress, msg);
			}
			
			belle_sip_body_handler_add_header(first_part_bh,
											  belle_sip_header_create("Content-disposition", first_part_header));
			belle_sip_free(first_part_header);
			belle_sip_body_handler_add_header(first_part_bh,
											  (belle_sip_header_t *)belle_sip_header_content_type_create(
												  linphone_content_get_type(msg->file_transfer_information),
												  linphone_content_get_subtype(msg->file_transfer_information)));

			/* insert it in a multipart body handler which will manage the boundaries of multipart msg */
			bh = belle_sip_multipart_body_handler_new(linphone_chat_message_file_transfer_on_progress, msg, first_part_bh, NULL);

			linphone_chat_message_ref(msg);
			_release_http_request(msg);
			file_upload_begin_background_task(msg);
			linphone_chat_room_upload_file(msg);
			belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(msg->http_request), BELLE_SIP_BODY_HANDLER(bh));
			linphone_chat_message_unref(msg);
		} else if (code == 200) { /* file has been uplaoded correctly, get server reply and send it */
			const char *body = belle_sip_message_get_body((belle_sip_message_t *)event->response);
			if (body && strlen(body) > 0) {
				/* if we have an encryption key for the file, we must insert it into the msg and restore the correct
				 * filename */
				const char *content_key = linphone_content_get_key(msg->file_transfer_information);
				size_t content_key_size = linphone_content_get_key_size(msg->file_transfer_information);
				if (content_key != NULL) {
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
									size_t b64Size = b64_encode(NULL, content_key_size, NULL, 0);
									char *keyb64 = (char *)ms_malloc0(b64Size + 1);
									int xmlStringLength;

									b64Size = b64_encode(content_key, content_key_size, keyb64, b64Size);
									keyb64[b64Size] = '\0'; /* libxml need a null terminated string */

									/* add the node containing the key to the file-info node */
									xmlNewTextChild(cur, NULL, (const xmlChar *)"file-key", (const xmlChar *)keyb64);
									xmlFree(typeAttribute);
									ms_free(keyb64);

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
				linphone_chat_message_ref(msg);
				linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferDone);
				_release_http_request(msg);
				_linphone_chat_room_send_message(msg->chat_room, msg);
				file_upload_end_background_task(msg);
				linphone_chat_message_unref(msg);
			} else {
				ms_warning("Received empty response from server, file transfer failed");
				linphone_chat_message_update_state(msg, LinphoneChatMessageStateNotDelivered);
				_release_http_request(msg);
				file_upload_end_background_task(msg);
				linphone_chat_message_unref(msg);
			}
		} else {
			ms_warning("Unhandled HTTP code response %d for file transfer", code);
			linphone_chat_message_update_state(msg, LinphoneChatMessageStateNotDelivered);
			_release_http_request(msg);
			file_upload_end_background_task(msg);
			linphone_chat_message_unref(msg);
		}
	}
}

const LinphoneContent *linphone_chat_message_get_file_transfer_information(const LinphoneChatMessage *msg) {
	return msg->file_transfer_information;
}

static void on_recv_body(belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, void *data, size_t offset, uint8_t *buffer, size_t size) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	LinphoneCore *lc = NULL;
	LinphoneImEncryptionEngine *imee = NULL;
	int retval = -1;
	char *decrypted_buffer = NULL;

	if (!msg->chat_room) {
		linphone_chat_message_cancel_file_transfer(msg);
		return;
	}
	lc = msg->chat_room->lc;

	if (lc == NULL){
		return; /*might happen during linphone_core_destroy()*/
	}

	if (!msg->http_request || belle_http_request_is_cancelled(msg->http_request)) {
		ms_warning("Cancelled request for msg [%p], ignoring %s", msg, __FUNCTION__);
		return;
	}

	/* first call may be with a zero size, ignore it */
	if (size == 0) {
		return;
	}
	
	decrypted_buffer = (char *)ms_malloc0(size);
	imee = linphone_core_get_im_encryption_engine(lc);
	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsDownloadingFileCb cb_process_downloading_file = linphone_im_encryption_engine_cbs_get_process_downloading_file(imee_cbs);
		if (cb_process_downloading_file) {
			retval = cb_process_downloading_file(imee, msg, (const char *)buffer, size, decrypted_buffer);
			if (retval == 0) {
				memcpy(buffer, decrypted_buffer, size);
			}
		}
	}
	ms_free(decrypted_buffer);
	
	if (retval <= 0) {
		if (msg->file_transfer_filepath == NULL) {
			if (linphone_chat_message_cbs_get_file_transfer_recv(msg->callbacks)) {
				LinphoneBuffer *lb = linphone_buffer_new_from_data(buffer, size);
				linphone_chat_message_cbs_get_file_transfer_recv(msg->callbacks)(msg, msg->file_transfer_information, lb);
				linphone_buffer_unref(lb);
			} else {
				/* Legacy: call back given by application level */
				linphone_core_notify_file_transfer_recv(lc, msg, msg->file_transfer_information, (const char *)buffer, size);
			}
		}
	} else {
		ms_warning("File transfer decrypt failed with code %d", (int)retval);
		linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferError);
	}

	return;
}

static void on_recv_end(belle_sip_user_body_handler_t *bh, void *data) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	LinphoneCore *lc = msg->chat_room->lc;
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(lc);
	int retval = -1;
	
	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsDownloadingFileCb cb_process_downloading_file = linphone_im_encryption_engine_cbs_get_process_downloading_file(imee_cbs);
		if (cb_process_downloading_file) {
			retval = cb_process_downloading_file(imee, msg, NULL, 0, NULL);
		}
	}
	
	if (retval <= 0) {
		if (msg->file_transfer_filepath == NULL) {
			if (linphone_chat_message_cbs_get_file_transfer_recv(msg->callbacks)) {
				LinphoneBuffer *lb = linphone_buffer_new();
				linphone_chat_message_cbs_get_file_transfer_recv(msg->callbacks)(msg, msg->file_transfer_information, lb);
				linphone_buffer_unref(lb);
			} else {
				/* Legacy: call back given by application level */
				linphone_core_notify_file_transfer_recv(lc, msg, msg->file_transfer_information, NULL, 0);
			}
		}
	}

	if (retval <= 0 && linphone_chat_message_get_state(msg) != LinphoneChatMessageStateFileTransferError) {
		linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferDone);
	}
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
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	if (event->response) {
		/*we are receiving a response, set a specific body handler to acquire the response.
		 * if not done, belle-sip will create a memory body handler, the default*/
		belle_sip_message_t *response = BELLE_SIP_MESSAGE(event->response);
		belle_sip_body_handler_t *body_handler = NULL;
		size_t body_size = 0;

		if (msg->file_transfer_information == NULL) {
			ms_warning("No file transfer information for msg %p: creating...", msg);
			msg->file_transfer_information = linphone_chat_create_file_transfer_information_from_headers(response);
		}

		if (msg->file_transfer_information) {
			body_size = linphone_content_get_size(msg->file_transfer_information);
		}

		
		body_handler = (belle_sip_body_handler_t *)belle_sip_user_body_handler_new(body_size, linphone_chat_message_file_transfer_on_progress, NULL, on_recv_body, NULL, on_recv_end, msg);
		if (msg->file_transfer_filepath != NULL) {
			belle_sip_user_body_handler_t *bh = (belle_sip_user_body_handler_t *)body_handler;
			body_handler = (belle_sip_body_handler_t *)belle_sip_file_body_handler_new(
				msg->file_transfer_filepath, linphone_chat_message_file_transfer_on_progress, msg);
			if (belle_sip_body_handler_get_size((belle_sip_body_handler_t *)body_handler) == 0) {
				/* If the size of the body has not been initialized from the file stat, use the one from the
				 * file_transfer_information. */
				belle_sip_body_handler_set_size((belle_sip_body_handler_t *)body_handler, body_size);
			}
			belle_sip_file_body_handler_set_user_body_handler((belle_sip_file_body_handler_t *)body_handler, bh);
		}
		belle_sip_message_set_body_handler((belle_sip_message_t *)event->response, body_handler);
	}
}

static void linphone_chat_process_response_from_get_file(void *data, const belle_http_response_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	/* check the answer code */
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code >= 400 && code < 500) {
			ms_warning("File transfer failed with code %d", code);
			linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferError);
		} else if (code != 200) {
			ms_warning("Unhandled HTTP code response %d for file transfer", code);
		}
		_release_http_request(msg);
	}
}

int _linphone_chat_room_start_http_transfer(LinphoneChatMessage *msg, const char* url, const char* action, const belle_http_request_listener_callbacks_t *cbs) {
	belle_generic_uri_t *uri = NULL;
	const char* ua = linphone_core_get_user_agent(msg->chat_room->lc);

	if (url == NULL) {
		ms_warning("Cannot process file transfer msg: no file remote URI configured.");
		goto error;
	}
	uri = belle_generic_uri_parse(url);
	if (uri == NULL || belle_generic_uri_get_host(uri)==NULL) {
		ms_warning("Cannot process file transfer msg: incorrect file remote URI configured '%s'.", url);
		goto error;
	}

	msg->http_request = belle_http_request_create(action, uri, belle_sip_header_create("User-Agent", ua), NULL);

	if (msg->http_request == NULL) {
		ms_warning("Could not create http request for uri %s", url);
		goto error;
	}
	/* keep a reference to the http request to be able to cancel it during upload */
	belle_sip_object_ref(msg->http_request);

	/* give msg to listener to be able to start the actual file upload when server answer a 204 No content */
	msg->http_listener = belle_http_request_listener_create_from_callbacks(cbs, linphone_chat_message_ref(msg));
	belle_http_provider_send_request(msg->chat_room->lc->http_provider, msg->http_request, msg->http_listener);
	return 0;
error:
	if (uri) {
		belle_sip_object_unref(uri);
	}
	return -1;
}

int linphone_chat_room_upload_file(LinphoneChatMessage *msg) {
	belle_http_request_listener_callbacks_t cbs = {0};
	int err;

	if (msg->http_request){
		ms_error("linphone_chat_room_upload_file(): there is already an upload in progress.");
		return -1;
	}

	cbs.process_response = linphone_chat_message_process_response_from_post_file;
	cbs.process_io_error = linphone_chat_message_process_io_error_upload;
	cbs.process_auth_requested = linphone_chat_message_process_auth_requested_upload;
	err = _linphone_chat_room_start_http_transfer(msg, linphone_core_get_file_transfer_server(msg->chat_room->lc), "POST", &cbs);
	if (err == -1){
		linphone_chat_message_set_state(msg, LinphoneChatMessageStateNotDelivered);
	}
	return err;
}

int linphone_chat_message_download_file(LinphoneChatMessage *msg) {
	belle_http_request_listener_callbacks_t cbs = {0};
	int err;

	if (msg->http_request){
		ms_error("linphone_chat_message_download_file(): there is already a download in progress");
		return -1;
	}
	cbs.process_response_headers = linphone_chat_process_response_headers_from_get_file;
	cbs.process_response = linphone_chat_process_response_from_get_file;
	cbs.process_io_error = linphone_chat_message_process_io_error_download;
	cbs.process_auth_requested = linphone_chat_message_process_auth_requested_download;
	err = _linphone_chat_room_start_http_transfer(msg, msg->external_body_url, "GET", &cbs);
	if (err == -1) return -1;
	/* start the download, status is In Progress */
	linphone_chat_message_set_state(msg, LinphoneChatMessageStateInProgress);
	return 0;
}

void linphone_chat_message_start_file_download(LinphoneChatMessage *msg,
											   LinphoneChatMessageStateChangedCb status_cb, void *ud) {
	msg->message_state_changed_cb = status_cb;
	msg->message_state_changed_user_data = ud;
	linphone_chat_message_download_file(msg);
}

void linphone_chat_message_cancel_file_transfer(LinphoneChatMessage *msg) {
	if (msg->http_request) {
		if (msg->state == LinphoneChatMessageStateInProgress) {
			linphone_chat_message_set_state(msg, LinphoneChatMessageStateNotDelivered);
		}
		if (!belle_http_request_is_cancelled(msg->http_request)) {
			if (msg->chat_room) {
				ms_message("Canceling file transfer %s - msg [%p] chat room[%p]"
								, (msg->external_body_url == NULL) ? linphone_core_get_file_transfer_server(msg->chat_room->lc) : msg->external_body_url
								, msg
								, msg->chat_room);
				belle_http_provider_cancel_request(msg->chat_room->lc->http_provider, msg->http_request);
				if (msg->dir == LinphoneChatMessageOutgoing) {
					// must release it
					linphone_chat_message_unref(msg);
				}
			} else {
				ms_message("Warning: http request still running for ORPHAN msg [%p]: this is a memory leak", msg);
			}
		}
		_release_http_request(msg);
	} else {
		ms_message("No existing file transfer - nothing to cancel");
	}
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

LinphoneChatMessage *linphone_chat_room_create_file_transfer_message(LinphoneChatRoom *cr,
																	 const LinphoneContent *initial_content) {
	LinphoneChatMessage *msg = belle_sip_object_new(LinphoneChatMessage);
	msg->callbacks = linphone_chat_message_cbs_new();
	msg->chat_room = (LinphoneChatRoom *)cr;
	msg->message = NULL;
	msg->file_transfer_information = linphone_content_copy(initial_content);
	msg->dir = LinphoneChatMessageOutgoing;
	linphone_chat_message_set_to(msg, linphone_chat_room_get_peer_address(cr));
	msg->from = linphone_address_new(linphone_core_get_identity(cr->lc)); /*direct assignment*/
	/* this will be set to application/vnd.gsma.rcs-ft-http+xml when we will transfer the xml reply from server to the peers */
	msg->content_type = NULL;
	/* this will store the http request during file upload to the server */
	msg->http_request = NULL;
	msg->time = ms_time(0);
	return msg;
}
