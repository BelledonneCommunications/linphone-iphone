/*
 * chat-message.cpp
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

#include "db/events-db.h"
#include "object/object-p.h"

#include "linphone/core.h"
#include "linphone/lpconfig.h"
#include "c-wrapper/c-wrapper.h"

#include "chat-message-p.h"
#include "chat-message.h"
#include "content/content.h"

#include "modifier/multipart-chat-message-modifier.h"
#include "modifier/cpim-chat-message-modifier.h"
#include "chat-room-p.h"
#include "real-time-text-chat-room.h"

#include "ortp/b64.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

using namespace B64_NAMESPACE;
using namespace std;

// =============================================================================
// ChatMessagePrivate
// =============================================================================

ChatMessagePrivate::ChatMessagePrivate (const shared_ptr<ChatRoom> &room)
: chatRoom(room) {
}

ChatMessagePrivate::~ChatMessagePrivate () {}

// -----------------------------------------------------------------------------

void ChatMessagePrivate::setChatRoom (shared_ptr<ChatRoom> cr) {
	chatRoom = cr;
}

void ChatMessagePrivate::setDirection (ChatMessage::Direction dir) {
	direction = dir;
}

void ChatMessagePrivate::setTime(time_t t) {
	time = t;
}

void ChatMessagePrivate::setState(ChatMessage::State s) {
	L_Q();

	if (s != state && chatRoom) {
		if (((state == ChatMessage::State::Displayed) || (state == ChatMessage::State::DeliveredToUser))
			&& ((s == ChatMessage::State::DeliveredToUser) || (s == ChatMessage::State::Delivered) || (s == ChatMessage::State::NotDelivered))) {
			return;
		}
		ms_message("Chat message %p: moving from state %s to %s", this, linphone_chat_message_state_to_string((LinphoneChatMessageState)state), linphone_chat_message_state_to_string((LinphoneChatMessageState)s));
		state = s;

		LinphoneChatMessage *msg = L_GET_C_BACK_PTR(q);
		if (linphone_chat_message_get_message_state_changed_cb(msg)) {
			linphone_chat_message_get_message_state_changed_cb(msg)(msg, (LinphoneChatMessageState)state, linphone_chat_message_get_message_state_changed_cb_user_data(msg));
		}
		LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
		if (linphone_chat_message_cbs_get_msg_state_changed(cbs)) {
			linphone_chat_message_cbs_get_msg_state_changed(cbs)(msg, linphone_chat_message_get_state(msg));
		}
	}
}

unsigned int ChatMessagePrivate::getStorageId() const {
	return storageId;
}

void ChatMessagePrivate::setStorageId(unsigned int id) {
	storageId = id;
}

belle_http_request_t *ChatMessagePrivate::getHttpRequest() const {
	return httpRequest;
}

void ChatMessagePrivate::setHttpRequest(belle_http_request_t *request) {
	httpRequest = request;
}

SalOp *ChatMessagePrivate::getSalOp() const {
	return salOp;
}

void ChatMessagePrivate::setSalOp(SalOp *op) {
	salOp = op;
}

SalCustomHeader *ChatMessagePrivate::getSalCustomHeaders() const {
	return salCustomHeaders;
}

void ChatMessagePrivate::setSalCustomHeaders(SalCustomHeader *headers) {
	salCustomHeaders = headers;
}

void ChatMessagePrivate::addSalCustomHeader(const string& name, const string& value) {
	salCustomHeaders = sal_custom_header_append(salCustomHeaders, name.c_str(), value.c_str());
}

void ChatMessagePrivate::removeSalCustomHeader(const string& name) {
	salCustomHeaders = sal_custom_header_remove(salCustomHeaders, name.c_str());
}

string ChatMessagePrivate::getSalCustomHeaderValue(const string& name) {
	return L_C_TO_STRING(sal_custom_header_find(salCustomHeaders, name.c_str()));
}

// -----------------------------------------------------------------------------

const string& ChatMessagePrivate::getContentType() const {
	return cContentType;
}

void ChatMessagePrivate::setContentType(const string& contentType) {
	cContentType = contentType;
}

const string& ChatMessagePrivate::getText() const {
	return cText;
}

void ChatMessagePrivate::setText(const string& text) {
	cText = text;
}

LinphoneContent * ChatMessagePrivate::getFileTransferInformation() const {
	return cFileTransferInformation;
}

void ChatMessagePrivate::setFileTransferInformation(LinphoneContent *content) {
	cFileTransferInformation = content;
}

// -----------------------------------------------------------------------------

string ChatMessagePrivate::createImdnXml(ImdnType imdnType, LinphoneReason reason) {
	xmlBufferPtr buf;
	xmlTextWriterPtr writer;
	int err;
	string content;
	char *datetime = NULL;

	// Check that the chat message has a message id
	if (id.empty()) return NULL;

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

	datetime = linphone_timestamp_to_rfc3339_string(time);
	err = xmlTextWriterStartDocument(writer, "1.0", "UTF-8", NULL);
	if (err >= 0) {
		err = xmlTextWriterStartElementNS(writer, NULL, (const xmlChar *)"imdn",
										  (const xmlChar *)"urn:ietf:params:xml:ns:imdn");
	}
	if ((err >= 0) && (reason != LinphoneReasonNone)) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xmlns", (const xmlChar *)"linphoneimdn", NULL, (const xmlChar *)"http://www.linphone.org/xsds/imdn.xsd");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"message-id", (const xmlChar *)id.c_str());
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"datetime", (const xmlChar *)datetime);
	}
	if (err >= 0) {
		if (imdnType == ImdnTypeDelivery) {
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
			if (imdnType == ImdnTypeDelivery) {
				err = xmlTextWriterStartElement(writer, (const xmlChar *)"delivered");
			} else {
				err = xmlTextWriterStartElement(writer, (const xmlChar *)"displayed");
			}
		} else {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"error");
		}
	}
	if (err >= 0) {
		// Close the "delivered", "displayed" or "error" element.
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
		// Close the "status" element.
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		// Close the "delivery-notification" or "display-notification" element.
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		// Close the "imdn" element.
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		err = xmlTextWriterEndDocument(writer);
	}
	if (err > 0) {
		// xmlTextWriterEndDocument returns the size of the content.
		content = string((char *)buf->content);
	}
	xmlFreeTextWriter(writer);
	xmlBufferFree(buf);
	ms_free(datetime);
	return content;
}

void ChatMessagePrivate::sendImdn(ImdnType imdnType, LinphoneReason reason) {
	string content = createImdnXml(imdnType, reason);
	chatRoom->getPrivate()->sendImdn(content, reason);
}

static void _chat_message_file_transfer_on_progress(belle_sip_body_handler_t *bh, belle_sip_message_t *m,
															void *data, size_t offset, size_t total) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->fileTransferOnProgress(bh, m, offset, total);
}

void ChatMessagePrivate::fileTransferOnProgress(belle_sip_body_handler_t *bh, belle_sip_message_t *m,
												size_t offset, size_t total) {
	L_Q();

	if (!isFileTransferInProgressAndValid()) {
		ms_warning("Cancelled request for %s msg [%p], ignoring %s", chatRoom ? "" : "ORPHAN", this, __FUNCTION__);
		releaseHttpRequest();
		return;
	}

	LinphoneChatMessage *msg = L_GET_C_BACK_PTR(q);
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
	if (linphone_chat_message_cbs_get_file_transfer_progress_indication(cbs)) {
		linphone_chat_message_cbs_get_file_transfer_progress_indication(cbs)(msg, cFileTransferInformation, offset, total);
	} else {
		// Legacy: call back given by application level
		linphone_core_notify_file_transfer_progress_indication(chatRoom->getCore(), msg, cFileTransferInformation, offset, total);
	}
}

static int _chat_message_on_send_body(belle_sip_user_body_handler_t *bh, belle_sip_message_t *m,
										void *data, size_t offset, uint8_t *buffer, size_t *size) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	return d->onSendBody(bh, m, offset, buffer, size);
}

int ChatMessagePrivate::onSendBody(belle_sip_user_body_handler_t *bh, belle_sip_message_t *m,
									size_t offset, uint8_t *buffer, size_t *size) {
	L_Q();

	LinphoneCore *lc = NULL;
	LinphoneImEncryptionEngine *imee = NULL;
	int retval = -1;
	LinphoneChatMessage *msg = L_GET_C_BACK_PTR(q);

	if (!isFileTransferInProgressAndValid()) {
		if (httpRequest) {
			ms_warning("Cancelled request for %s msg [%p], ignoring %s", chatRoom ? "" : "ORPHAN", this, __FUNCTION__);
			releaseHttpRequest();
		}
		return BELLE_SIP_STOP;
	}

	lc = chatRoom->getCore();
	// if we've not reach the end of file yet, ask for more data
	// in case of file body handler, won't be called
	if (fileTransferFilePath.empty() && offset < linphone_content_get_size(cFileTransferInformation)) {
		// get data from call back
		LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
		LinphoneChatMessageCbsFileTransferSendCb file_transfer_send_cb = linphone_chat_message_cbs_get_file_transfer_send(cbs);
		if (file_transfer_send_cb) {
			LinphoneBuffer *lb = file_transfer_send_cb(msg, cFileTransferInformation, offset, *size);
			if (lb == NULL) {
				*size = 0;
			} else {
				*size = linphone_buffer_get_size(lb);
				memcpy(buffer, linphone_buffer_get_content(lb), *size);
				linphone_buffer_unref(lb);
			}
		} else {
			// Legacy
			linphone_core_notify_file_transfer_send(lc, msg, cFileTransferInformation, (char *)buffer, size);
		}
	}

	imee = linphone_core_get_im_encryption_engine(lc);
	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsUploadingFileCb cb_process_uploading_file = linphone_im_encryption_engine_cbs_get_process_uploading_file(imee_cbs);
		if (cb_process_uploading_file) {
			size_t max_size = *size;
			uint8_t *encrypted_buffer = (uint8_t *)ms_malloc0(max_size);
			retval = cb_process_uploading_file(imee, msg, offset, (const uint8_t *)buffer, size, encrypted_buffer);
			if (retval == 0) {
				if (*size > max_size) {
					ms_error("IM encryption engine process upload file callback returned a size bigger than the size of the buffer, so it will be truncated !");
					*size = max_size;
				}
				memcpy(buffer, encrypted_buffer, *size);
			}
			ms_free(encrypted_buffer);
		}
	}

	return retval <= 0 ? BELLE_SIP_CONTINUE : BELLE_SIP_STOP;
}

static void _chat_message_on_send_end(belle_sip_user_body_handler_t *bh, void *data) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->onSendEnd(bh);
}

void ChatMessagePrivate::onSendEnd(belle_sip_user_body_handler_t *bh) {
	L_Q();

	LinphoneCore *lc = chatRoom->getCore();
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(lc);

	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsUploadingFileCb cb_process_uploading_file = linphone_im_encryption_engine_cbs_get_process_uploading_file(imee_cbs);
		if (cb_process_uploading_file) {
			cb_process_uploading_file(imee, L_GET_C_BACK_PTR(q), 0, NULL, NULL, NULL);
		}
	}
}

void ChatMessagePrivate::fileUploadEndBackgroundTask() {
	if (backgroundTaskId) {
		ms_message("channel [%p]: ending file upload background task with id=[%lx].", this, backgroundTaskId);
		sal_end_background_task(backgroundTaskId);
		backgroundTaskId = 0;
	}
}

static void _chat_message_file_upload_background_task_ended(void *data) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->fileUploadBackgroundTaskEnded();
}

void ChatMessagePrivate::fileUploadBackgroundTaskEnded() {
	ms_warning("channel [%p]: file upload background task has to be ended now, but work isn't finished.", this);
	fileUploadEndBackgroundTask();
}

void ChatMessagePrivate::fileUploadBeginBackgroundTask() {
	if (backgroundTaskId == 0) {
		backgroundTaskId = sal_begin_background_task("file transfer upload", _chat_message_file_upload_background_task_ended, this);
		if (backgroundTaskId) ms_message("channel [%p]: starting file upload background task with id=[%lx].", this, backgroundTaskId);
	}
}

static void _chat_message_on_recv_body(belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, void *data, size_t offset, uint8_t *buffer, size_t size) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->onRecvBody(bh, m, offset, buffer, size);
}

void ChatMessagePrivate::onRecvBody(belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, size_t offset, uint8_t *buffer, size_t size) {
	L_Q();

	LinphoneCore *lc = NULL;
	LinphoneImEncryptionEngine *imee = NULL;
	int retval = -1;
	uint8_t *decrypted_buffer = NULL;

	if (!chatRoom) {
		q->cancelFileTransfer();
		return;
	}
	lc = chatRoom->getCore();

	if (lc == NULL) {
		return; // might happen during linphone_core_destroy()
	}

	if (!httpRequest || belle_http_request_is_cancelled(httpRequest)) {
		ms_warning("Cancelled request for msg [%p], ignoring %s", this, __FUNCTION__);
		return;
	}

	// first call may be with a zero size, ignore it
	if (size == 0) {
		return;
	}

	decrypted_buffer = (uint8_t *)ms_malloc0(size);
	imee = linphone_core_get_im_encryption_engine(lc);
	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsDownloadingFileCb cb_process_downloading_file = linphone_im_encryption_engine_cbs_get_process_downloading_file(imee_cbs);
		if (cb_process_downloading_file) {
			retval = cb_process_downloading_file(imee, L_GET_C_BACK_PTR(q), offset, (const uint8_t *)buffer, size, decrypted_buffer);
			if (retval == 0) {
				memcpy(buffer, decrypted_buffer, size);
			}
		}
	}
	ms_free(decrypted_buffer);

	if (retval <= 0) {
		if (fileTransferFilePath.empty()) {
			LinphoneChatMessage *msg = L_GET_C_BACK_PTR(q);
			LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
			if (linphone_chat_message_cbs_get_file_transfer_recv(cbs)) {
				LinphoneBuffer *lb = linphone_buffer_new_from_data(buffer, size);
				linphone_chat_message_cbs_get_file_transfer_recv(cbs)(msg, cFileTransferInformation, lb);
				linphone_buffer_unref(lb);
			} else {
				// Legacy: call back given by application level
				linphone_core_notify_file_transfer_recv(lc, msg, cFileTransferInformation, (const char *)buffer, size);
			}
		}
	} else {
		ms_warning("File transfer decrypt failed with code %d", (int)retval);
		setState(ChatMessage::FileTransferError);
	}

	return;
}

static void _chat_message_on_recv_end(belle_sip_user_body_handler_t *bh, void *data) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->onRecvEnd(bh);
}

void ChatMessagePrivate::onRecvEnd(belle_sip_user_body_handler_t *bh) {
	L_Q();

	LinphoneCore *lc = chatRoom->getCore();
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(lc);
	int retval = -1;

	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsDownloadingFileCb cb_process_downloading_file = linphone_im_encryption_engine_cbs_get_process_downloading_file(imee_cbs);
		if (cb_process_downloading_file) {
			retval = cb_process_downloading_file(imee, L_GET_C_BACK_PTR(q), 0, NULL, 0, NULL);
		}
	}

	if (retval <= 0) {
		if (fileTransferFilePath.empty()) {
			LinphoneChatMessage *msg = L_GET_C_BACK_PTR(q);
			LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
			if (linphone_chat_message_cbs_get_file_transfer_recv(cbs)) {
				LinphoneBuffer *lb = linphone_buffer_new();
				linphone_chat_message_cbs_get_file_transfer_recv(cbs)(msg, cFileTransferInformation, lb);
				linphone_buffer_unref(lb);
			} else {
				// Legacy: call back given by application level
				linphone_core_notify_file_transfer_recv(lc, msg, cFileTransferInformation, NULL, 0);
			}
		}
	}

	if (retval <= 0 && state != ChatMessage::State::FileTransferError) {
		setState(ChatMessage::State::FileTransferDone);
	}
}

bool ChatMessagePrivate::isFileTransferInProgressAndValid() {
	return (chatRoom && chatRoom->getCore() && httpRequest && !belle_http_request_is_cancelled(httpRequest));
}

static void _chat_message_process_response_from_post_file(void *data, const belle_http_response_event_t *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processResponseFromPostFile(event);
}

void ChatMessagePrivate::processResponseFromPostFile(const belle_http_response_event_t *event) {
	L_Q();

	if (httpRequest && !isFileTransferInProgressAndValid()) {
		ms_warning("Cancelled request for %s msg [%p], ignoring %s", chatRoom ? "" : "ORPHAN", this, __FUNCTION__);
		releaseHttpRequest();
		return;
	}

	// check the answer code
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code == 204) { // this is the reply to the first post to the server - an empty msg
			// start uploading the file
			belle_sip_multipart_body_handler_t *bh;
			string first_part_header;
			belle_sip_body_handler_t *first_part_bh;

			bool_t is_file_encryption_enabled = FALSE;
			LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(chatRoom->getCore());
			if (imee) {
				LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb is_encryption_enabled_for_file_transfer_cb =
					linphone_im_encryption_engine_cbs_get_is_encryption_enabled_for_file_transfer(imee_cbs);
				if (is_encryption_enabled_for_file_transfer_cb) {
					is_file_encryption_enabled = is_encryption_enabled_for_file_transfer_cb(imee, L_GET_C_BACK_PTR(chatRoom));
				}
			}
			// shall we encrypt the file
			if (is_file_encryption_enabled) {
				LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsGenerateFileTransferKeyCb generate_file_transfer_key_cb =
					linphone_im_encryption_engine_cbs_get_generate_file_transfer_key(imee_cbs);
				if (generate_file_transfer_key_cb) {
					generate_file_transfer_key_cb(imee, L_GET_C_BACK_PTR(chatRoom), L_GET_C_BACK_PTR(q));
				}
				// temporary storage for the Content-disposition header value : use a generic filename to not leak it
				// Actual filename stored in msg->file_transfer_information->name will be set in encrypted msg
				// sended to the
				first_part_header = "form-data; name=\"File\"; filename=\"filename.txt\"";
			} else {
				// temporary storage for the Content-disposition header value
				first_part_header = "form-data; name=\"File\"; filename=\"%s\"";
				first_part_header = first_part_header + linphone_content_get_name(cFileTransferInformation);
			}

			// create a user body handler to take care of the file and add the content disposition and content-type headers
			first_part_bh = (belle_sip_body_handler_t *)belle_sip_user_body_handler_new(
					linphone_content_get_size(cFileTransferInformation),
					_chat_message_file_transfer_on_progress, NULL, NULL,
					_chat_message_on_send_body, _chat_message_on_send_end, this);
			if (!fileTransferFilePath.empty()) {
				belle_sip_user_body_handler_t *body_handler = (belle_sip_user_body_handler_t *)first_part_bh; 
				// No need to add again the callback for progression, otherwise it will be called twice
				first_part_bh = (belle_sip_body_handler_t *)belle_sip_file_body_handler_new(fileTransferFilePath.c_str(), NULL, this);
				linphone_content_set_size(cFileTransferInformation, belle_sip_file_body_handler_get_file_size((belle_sip_file_body_handler_t *)first_part_bh));
				belle_sip_file_body_handler_set_user_body_handler((belle_sip_file_body_handler_t *)first_part_bh, body_handler);
			} else if (linphone_content_get_buffer(cFileTransferInformation) != NULL) {
				first_part_bh = (belle_sip_body_handler_t *)belle_sip_memory_body_handler_new_from_buffer(
					linphone_content_get_buffer(cFileTransferInformation),
					linphone_content_get_size(cFileTransferInformation), _chat_message_file_transfer_on_progress, this);
			}

			belle_sip_body_handler_add_header(first_part_bh,
											  belle_sip_header_create("Content-disposition", first_part_header.c_str()));
			belle_sip_body_handler_add_header(first_part_bh,
											  (belle_sip_header_t *)belle_sip_header_content_type_create(
												  linphone_content_get_type(cFileTransferInformation),
												  linphone_content_get_subtype(cFileTransferInformation)));

			// insert it in a multipart body handler which will manage the boundaries of multipart msg
			bh = belle_sip_multipart_body_handler_new(_chat_message_file_transfer_on_progress, this, first_part_bh, NULL);

			releaseHttpRequest();
			fileUploadBeginBackgroundTask();
			q->uploadFile();
			belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(httpRequest), BELLE_SIP_BODY_HANDLER(bh));
		} else if (code == 200) { // file has been uploaded correctly, get server reply and send it
			const char *body = belle_sip_message_get_body((belle_sip_message_t *)event->response);
			if (body && strlen(body) > 0) {
				// if we have an encryption key for the file, we must insert it into the msg and restore the correct filename
				const char *content_key = linphone_content_get_key(cFileTransferInformation);
				size_t content_key_size = linphone_content_get_key_size(cFileTransferInformation);
				if (content_key != NULL) {
					// parse the msg body
					xmlDocPtr xmlMessageBody = xmlParseDoc((const xmlChar *)body);

					xmlNodePtr cur = xmlDocGetRootElement(xmlMessageBody);
					if (cur != NULL) {
						cur = cur->xmlChildrenNode;
						while (cur != NULL) {
							// we found a file info node, check it has a type="file" attribute
							if (!xmlStrcmp(cur->name, (const xmlChar *)"file-info")) {
								xmlChar *typeAttribute = xmlGetProp(cur, (const xmlChar *)"type");
								// this is the node we are looking for : add a file-key children node
								if (!xmlStrcmp(typeAttribute, (const xmlChar *)"file")) {
									// need to parse the children node to update the file-name one
									xmlNodePtr fileInfoNodeChildren = cur->xmlChildrenNode;
									// convert key to base64
									size_t b64Size = b64_encode(NULL, content_key_size, NULL, 0);
									char *keyb64 = (char *)ms_malloc0(b64Size + 1);
									int xmlStringLength;

									b64Size = b64_encode(content_key, content_key_size, keyb64, b64Size);
									keyb64[b64Size] = '\0'; // libxml need a null terminated string

									// add the node containing the key to the file-info node
									xmlNewTextChild(cur, NULL, (const xmlChar *)"file-key", (const xmlChar *)keyb64);
									xmlFree(typeAttribute);
									ms_free(keyb64);

									// look for the file-name node and update its content
									while (fileInfoNodeChildren != NULL) {
										// we found a the file-name node, update its content with the real filename
										if (!xmlStrcmp(fileInfoNodeChildren->name, (const xmlChar *)"file-name")) {
											// update node content
											xmlNodeSetContent(fileInfoNodeChildren, (const xmlChar *)(linphone_content_get_name(cFileTransferInformation)));
											break;
										}
										fileInfoNodeChildren = fileInfoNodeChildren->next;
									}

									// dump the xml into msg->message
									char *buffer;
									xmlDocDumpFormatMemoryEnc(xmlMessageBody, (xmlChar **)&buffer, &xmlStringLength, "UTF-8", 0);
									setText(buffer);
									break;
								}
								xmlFree(typeAttribute);
							}
							cur = cur->next;
						}
					}
					xmlFreeDoc(xmlMessageBody);
				} else { // no encryption key, transfer in plain, just copy the msg sent by server
					cText = ms_strdup(body);
				}
				cContentType = "application/vnd.gsma.rcs-ft-http+xml";
				q->updateState(ChatMessage::FileTransferDone);
				releaseHttpRequest();
				chatRoom->sendMessage(L_GET_C_BACK_PTR(q));
				fileUploadEndBackgroundTask();
			} else {
				ms_warning("Received empty response from server, file transfer failed");
				q->updateState(ChatMessage::NotDelivered);
				releaseHttpRequest();
				fileUploadEndBackgroundTask();
			}
		} else {
			ms_warning("Unhandled HTTP code response %d for file transfer", code);
			q->updateState(ChatMessage::NotDelivered);
			releaseHttpRequest();
			fileUploadEndBackgroundTask();
		}
	}
}

static void _chat_process_response_headers_from_get_file(void *data, const belle_http_response_event_t *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processResponseHeadersFromGetFile(event);
}

static LinphoneContent *createFileTransferInformationFromHeaders(const belle_sip_message_t *m) {
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
		if (type) {
			linphone_content_set_type(content, type);
		}
		if (subtype) {
			linphone_content_set_subtype(content, subtype);
		}
	}

	if (content_length_hdr) {
		linphone_content_set_size(content, belle_sip_header_content_length_get_content_length(content_length_hdr));
		ms_message("Extracted content length %i from header", (int)linphone_content_get_size(content));
	}

	return content;
}

void ChatMessagePrivate::processResponseHeadersFromGetFile(const belle_http_response_event_t *event) {
	if (event->response) {
		//we are receiving a response, set a specific body handler to acquire the response.
		// if not done, belle-sip will create a memory body handler, the default
		belle_sip_message_t *response = BELLE_SIP_MESSAGE(event->response);
		belle_sip_body_handler_t *body_handler = NULL;
		size_t body_size = 0;

		if (cFileTransferInformation == NULL) {
			ms_warning("No file transfer information for msg [%p]: creating...", this);
			cFileTransferInformation = createFileTransferInformationFromHeaders(response);
		}

		if (cFileTransferInformation) {
			body_size = linphone_content_get_size(cFileTransferInformation);
		}

		body_handler = (belle_sip_body_handler_t *)belle_sip_user_body_handler_new(body_size, _chat_message_file_transfer_on_progress, 
																					NULL, _chat_message_on_recv_body, 
																					NULL, _chat_message_on_recv_end, this);
		if (!fileTransferFilePath.empty()) {
			belle_sip_user_body_handler_t *bh = (belle_sip_user_body_handler_t *)body_handler;
			body_handler = (belle_sip_body_handler_t *)belle_sip_file_body_handler_new(fileTransferFilePath.c_str(), _chat_message_file_transfer_on_progress, this);
			if (belle_sip_body_handler_get_size((belle_sip_body_handler_t *)body_handler) == 0) {
				// If the size of the body has not been initialized from the file stat, use the one from the
				// file_transfer_information.
				belle_sip_body_handler_set_size((belle_sip_body_handler_t *)body_handler, body_size);
			}
			belle_sip_file_body_handler_set_user_body_handler((belle_sip_file_body_handler_t *)body_handler, bh);
		}
		belle_sip_message_set_body_handler((belle_sip_message_t *)event->response, body_handler);
	}
}

static void _chat_message_process_auth_requested_download(void *data, belle_sip_auth_event *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processAuthRequestedDownload(event);
}

void ChatMessagePrivate::processAuthRequestedDownload(const belle_sip_auth_event *event) {
	L_Q();

	ms_error("Error during file download : auth requested for msg [%p]", this);
	q->updateState(ChatMessage::FileTransferError);
	releaseHttpRequest();
}

static void _chat_message_process_io_error_upload(void *data, const belle_sip_io_error_event_t *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processIoErrorUpload(event);
}

void ChatMessagePrivate::processIoErrorUpload(const belle_sip_io_error_event_t *event) {
	L_Q();
	ms_error("I/O Error during file upload of msg [%p]", this);
	q->updateState(ChatMessage::NotDelivered);
	releaseHttpRequest();
	chatRoom->getPrivate()->removeTransientMessage(L_GET_C_BACK_PTR(q));
}

static void _chat_message_process_auth_requested_upload(void *data, belle_sip_auth_event *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processAuthRequestedUpload(event);
}

void ChatMessagePrivate::processAuthRequestedUpload(const belle_sip_auth_event *event) {
	L_Q();
	ms_error("Error during file upload: auth requested for msg [%p]", this);
	q->updateState(ChatMessage::NotDelivered);
	releaseHttpRequest();
	chatRoom->getPrivate()->removeTransientMessage(L_GET_C_BACK_PTR(q));
}

static void _chat_message_process_io_error_download(void *data, const belle_sip_io_error_event_t *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processIoErrorDownload(event);
}

void ChatMessagePrivate::processIoErrorDownload(const belle_sip_io_error_event_t *event) {
	L_Q();

	ms_error("I/O Error during file download msg [%p]", this);
	q->updateState(ChatMessage::FileTransferError);
	releaseHttpRequest();
}

static void _chat_message_process_response_from_get_file(void *data, const belle_http_response_event_t *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processResponseFromGetFile(event);
}

void ChatMessagePrivate::processResponseFromGetFile(const belle_http_response_event_t *event) {
	// check the answer code
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code >= 400 && code < 500) {
			ms_warning("File transfer failed with code %d", code);
			setState(ChatMessage::FileTransferError);
		} else if (code != 200) {
			ms_warning("Unhandled HTTP code response %d for file transfer", code);
		}
		releaseHttpRequest();
	}
}

int ChatMessagePrivate::startHttpTransfer(std::string url, std::string action, belle_http_request_listener_callbacks_t *cbs) {
	belle_generic_uri_t *uri = NULL;
	const char* ua = linphone_core_get_user_agent(chatRoom->getCore());

	if (url.empty()) {
		ms_warning("Cannot process file transfer msg [%p]: no file remote URI configured.", this);
		goto error;
	}
	uri = belle_generic_uri_parse(url.c_str());
	if (uri == NULL || belle_generic_uri_get_host(uri) == NULL) {
		ms_warning("Cannot process file transfer msg [%p]: incorrect file remote URI configured '%s'.", this, url.c_str());
		goto error;
	}

	httpRequest = belle_http_request_create(action.c_str(), uri, belle_sip_header_create("User-Agent", ua), NULL);

	if (httpRequest == NULL) {
		ms_warning("Could not create http request for uri %s", url.c_str());
		goto error;
	}
	// keep a reference to the http request to be able to cancel it during upload
	belle_sip_object_ref(httpRequest);

	// give msg to listener to be able to start the actual file upload when server answer a 204 No content
	httpListener = belle_http_request_listener_create_from_callbacks(cbs, this);
	belle_http_provider_send_request(chatRoom->getCore()->http_provider, httpRequest, httpListener);
	return 0;
error:
	if (uri) {
		belle_sip_object_unref(uri);
	}
	return -1;
}

void ChatMessagePrivate::releaseHttpRequest() {
	if (httpRequest) {
		belle_sip_object_unref(httpRequest);
		httpRequest = NULL;
		if (httpListener){
			belle_sip_object_unref(httpListener);
			httpListener = NULL;
		}
	}
}

// -----------------------------------------------------------------------------

// =============================================================================
// ChatMessage
// =============================================================================

ChatMessage::ChatMessage (const shared_ptr<ChatRoom> &room) : Object(*new ChatMessagePrivate(room)) {}

ChatMessage::ChatMessage (ChatMessagePrivate &p) : Object(p) {}

LinphoneChatMessage * ChatMessage::getBackPtr() {
	return L_GET_C_BACK_PTR(this);
}

shared_ptr<ChatRoom> ChatMessage::getChatRoom () const {
	L_D();
	return d->chatRoom;
}

// -----------------------------------------------------------------------------

const string& ChatMessage::getExternalBodyUrl() const {
	L_D();
	return d->externalBodyUrl;
}

void ChatMessage::setExternalBodyUrl(const string &url) {
	L_D();
	d->externalBodyUrl = url;
}

time_t ChatMessage::getTime () const {
	L_D();
	return d->time;
}

bool ChatMessage::isSecured () const {
	L_D();
	return d->isSecured;
}

void ChatMessage::setIsSecured(bool isSecured) {
	L_D();
	d->isSecured = isSecured;
}

ChatMessage::Direction ChatMessage::getDirection () const {
	L_D();
	return d->direction;
}

bool ChatMessage::isOutgoing () const {
	L_D();
	return d->direction == Outgoing;
}

bool ChatMessage::isIncoming () const {
	L_D();
	return d->direction == Incoming;
}

ChatMessage::State ChatMessage::getState() const {
	L_D();
	return d->state;
}

const string& ChatMessage::getId () const {
	L_D();
	return d->id;
}

void ChatMessage::setId (const string& id) {
	L_D();
	d->id = id;
}

bool ChatMessage::isRead() const {
	return false;
	L_D();
	LinphoneCore *lc = d->chatRoom->getCore();
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(lc);
	if (linphone_im_notif_policy_get_recv_imdn_displayed(policy) && d->state == Displayed) return true;
	if (linphone_im_notif_policy_get_recv_imdn_delivered(policy) && (d->state == DeliveredToUser || d->state == Displayed)) return true;
	return d->state == Delivered || d->state == Displayed || d->state == DeliveredToUser;
}

const string& ChatMessage::getAppdata () const {
	L_D();
	return d->appData;
}

void ChatMessage::setAppdata (const string &appData) {
	L_D();
	d->appData = appData;
	linphone_chat_message_store_appdata(L_GET_C_BACK_PTR(this));
}

shared_ptr<Address> ChatMessage::getFromAddress () const {
	L_D();
	return d->from;
}

void ChatMessage::setFromAddress(shared_ptr<Address> from) {
	L_D();
	d->from = from;
}

shared_ptr<Address> ChatMessage::getToAddress () const {
	L_D();
	return d->to;
}

void ChatMessage::setToAddress(shared_ptr<Address> to) {
	L_D();
	d->to = to;
}

const string& ChatMessage::getFileTransferFilepath() const {
	L_D();
	return d->fileTransferFilePath;
}

void ChatMessage::setFileTransferFilepath(const string &path) {
	L_D();
	d->fileTransferFilePath = path;
}

bool ChatMessage::isToBeStored() const {
	L_D();
	return d->isToBeStored;
}

void ChatMessage::setIsToBeStored(bool store) {
	L_D();
	d->isToBeStored = store;
}

// -----------------------------------------------------------------------------

const LinphoneErrorInfo * ChatMessage::getErrorInfo() const {
	L_D();
	if (!d->errorInfo) d->errorInfo = linphone_error_info_new(); //let's do it mutable
	linphone_error_info_from_sal_op(d->errorInfo, d->salOp);
	return d->errorInfo;
}

bool ChatMessage::isReadOnly () const {
	L_D();
	return d->isReadOnly;
}

list<shared_ptr<const Content> > ChatMessage::getContents () const {
	L_D();
	list<shared_ptr<const Content> > contents;
	for (const auto &content : d->contents)
		contents.push_back(content);
	return contents;
}

void ChatMessage::addContent (const shared_ptr<Content> &content) {
	L_D();
	if (d->isReadOnly) return;

	d->contents.push_back(content);
}

void ChatMessage::removeContent (const shared_ptr<const Content> &content) {
	L_D();
	if (d->isReadOnly) return;

	d->contents.remove(const_pointer_cast<Content>(content));
}

string ChatMessage::getCustomHeaderValue (const string &headerName) const {
	L_D();
	try {
		return d->customHeaders.at(headerName);
	} catch (const exception &) {
		// Key doesn't exist.
	}
	return nullptr;
}

void ChatMessage::addCustomHeader (const string &headerName, const string &headerValue) {
	L_D();
	if (d->isReadOnly) return;

	d->customHeaders[headerName] = headerValue;
}

void ChatMessage::removeCustomHeader (const string &headerName) {
	L_D();
	if (d->isReadOnly) return;

	d->customHeaders.erase(headerName);
}

// -----------------------------------------------------------------------------

void ChatMessage::updateState(State state) {
	L_D();

	d->setState(state);
	linphone_chat_message_store_state(L_GET_C_BACK_PTR(this));

	if (state == Delivered || state == NotDelivered) {
		d->chatRoom->getPrivate()->moveTransientMessageToWeakMessages(L_GET_C_BACK_PTR(this));
	}
}

void ChatMessage::send () {
	L_D();

	if (d->contents.size() > 1) {
		MultipartChatMessageModifier mcmm;
		mcmm.encode(d);
	}

	LinphoneCore *lc = getChatRoom()->getCore();
	LpConfig *lpc = linphone_core_get_config(lc);
	if (lp_config_get_int(lpc, "sip", "use_cpim", 0) == 1) {
		CpimChatMessageModifier ccmm;
		ccmm.encode(d);
	}

	d->isReadOnly = true;
}

void ChatMessage::reSend() {
	L_D();

	if (d->state != NotDelivered) {
		ms_warning("Cannot resend chat message in state %s", linphone_chat_message_state_to_string((LinphoneChatMessageState)d->state));
		return;
	}

	d->chatRoom->sendMessage(L_GET_C_BACK_PTR(this));
}

void ChatMessage::sendDeliveryNotification(LinphoneReason reason) {
	L_D();
	LinphoneCore *lc = d->chatRoom->getCore();
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(lc);
	if (linphone_im_notif_policy_get_send_imdn_delivered(policy)) {
		d->sendImdn(ImdnTypeDelivery, reason);
	}
}

void ChatMessage::sendDisplayNotification() {
	L_D();
	LinphoneCore *lc = d->chatRoom->getCore();
	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(lc);
	if (linphone_im_notif_policy_get_send_imdn_displayed(policy)) {
		d->sendImdn(ImdnTypeDisplay, LinphoneReasonNone);
	}
}

int ChatMessage::uploadFile() {
	L_D();

	if (d->httpRequest){
		ms_error("linphone_chat_room_upload_file(): there is already an upload in progress.");
		return -1;
	}

	belle_http_request_listener_callbacks_t cbs = {0};
	cbs.process_response = _chat_message_process_response_from_post_file;
	cbs.process_io_error = _chat_message_process_io_error_upload;
	cbs.process_auth_requested = _chat_message_process_auth_requested_upload;
	int err = d->startHttpTransfer(linphone_core_get_file_transfer_server(d->chatRoom->getCore()), "POST", &cbs);

	if (err == -1) {
		d->setState(NotDelivered);
	}
	return err;
}

int ChatMessage::downloadFile() {
	L_D();

	if (d->httpRequest) {
		ms_error("linphone_chat_message_download_file(): there is already a download in progress");
		return -1;
	}

	belle_http_request_listener_callbacks_t cbs = {0};
	cbs.process_response_headers = _chat_process_response_headers_from_get_file;
	cbs.process_response = _chat_message_process_response_from_get_file;
	cbs.process_io_error = _chat_message_process_io_error_download;
	cbs.process_auth_requested = _chat_message_process_auth_requested_download;
	int err = d->startHttpTransfer(d->externalBodyUrl, "GET", &cbs);

	if (err == -1) return -1;
	// start the download, status is In Progress
	d->setState(InProgress);
	return 0;
}

void ChatMessage::cancelFileTransfer() {
	L_D();
	if (d->httpRequest) {
		if (d->state == InProgress) {
			d->setState(NotDelivered);
		}
		if (!belle_http_request_is_cancelled(d->httpRequest)) {
			if (d->chatRoom) {
				ms_message("Canceling file transfer %s", (d->externalBodyUrl.empty()) ? linphone_core_get_file_transfer_server(d->chatRoom->getCore()) : d->externalBodyUrl.c_str());
				belle_http_provider_cancel_request(d->chatRoom->getCore()->http_provider, d->httpRequest);
			} else {
				ms_message("Warning: http request still running for ORPHAN msg: this is a memory leak");
			}
		}
		d->releaseHttpRequest();
	} else {
		ms_message("No existing file transfer - nothing to cancel");
	}
}

int ChatMessage::putCharacter(uint32_t character) {
	L_D();
	LinphoneCore *lc = d->chatRoom->getCore();
	if (linphone_core_realtime_text_enabled(lc)) {
		shared_ptr<LinphonePrivate::RealTimeTextChatRoom> rttcr =
			static_pointer_cast<LinphonePrivate::RealTimeTextChatRoom>(d->chatRoom);
		LinphoneCall *call = rttcr->getCall();
		const uint32_t new_line = 0x2028;
		const uint32_t crlf = 0x0D0A;
		const uint32_t lf = 0x0A;

		if (!call || !linphone_call_get_stream(call, LinphoneStreamTypeText)) {
			return -1;
		}

		if (character == new_line || character == crlf || character == lf) {
			if (lc && lp_config_get_int(lc->config, "misc", "store_rtt_messages", 1) == 1) {
				ms_debug("New line sent, forge a message with content %s", d->rttMessage.c_str());
				d->setTime(ms_time(0));
				d->state = Displayed;
				d->direction = Outgoing;
				setFromAddress(make_shared<LinphonePrivate::Address>(linphone_address_as_string(linphone_address_new(linphone_core_get_identity(lc)))));
				linphone_chat_message_store(L_GET_C_BACK_PTR(this));
				d->rttMessage = "";
			}
		} else {
			char *value = LinphonePrivate::Utils::utf8ToChar(character);
			d->rttMessage = d->rttMessage + string(value);
			ms_debug("Sent RTT character: %s (%lu), pending text is %s", value, (unsigned long)character, d->rttMessage.c_str());
			delete value;
		}

		text_stream_putchar32(reinterpret_cast<TextStream *>(linphone_call_get_stream(call, LinphoneStreamTypeText)), character);
		return 0;
	}
	return -1;
}

LINPHONE_END_NAMESPACE
