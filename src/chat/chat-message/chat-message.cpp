/*
 * chat-message.cpp
 * Copyright (C) 2010-2017 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "object/object-p.h"

#include "linphone/core.h"
#include "linphone/lpconfig.h"
#include "c-wrapper/c-wrapper.h"
#include "address/address.h"

#include "chat/chat-message/chat-message-p.h"

#include "chat/chat-room/chat-room-p.h"
#include "chat/chat-room/real-time-text-chat-room.h"
#include "chat/modifier/cpim-chat-message-modifier.h"
#include "chat/modifier/encryption-chat-message-modifier.h"
#include "chat/modifier/multipart-chat-message-modifier.h"
#include "content/content.h"
#include "core/core.h"

#include "logger/logger.h"
#include "ortp/b64.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

using namespace B64_NAMESPACE;
using namespace std;

// =============================================================================
// ChatMessagePrivate
// =============================================================================

ChatMessagePrivate::ChatMessagePrivate () {}

ChatMessagePrivate::~ChatMessagePrivate () {
	if (salOp)
		salOp->release();
}

// -----------------------------------------------------------------------------

void ChatMessagePrivate::setChatRoom (shared_ptr<ChatRoom> cr) {
	chatRoom = cr;
}

void ChatMessagePrivate::setDirection (ChatMessage::Direction dir) {
	direction = dir;
}

void ChatMessagePrivate::setTime (time_t t) {
	time = t;
}

void ChatMessagePrivate::setIsReadOnly (bool readOnly) {
	isReadOnly = readOnly;
}

void ChatMessagePrivate::setState (ChatMessage::State s) {
	L_Q();

	if (s == state || !q->getChatRoom())
		return;

	if (
		(state == ChatMessage::State::Displayed || state == ChatMessage::State::DeliveredToUser) &&
		(
			s == ChatMessage::State::DeliveredToUser ||
			s == ChatMessage::State::Delivered ||
			s == ChatMessage::State::NotDelivered
		)
	)
		return;

	lInfo() << "Chat message " << this << ": moving from state " <<
		linphone_chat_message_state_to_string((LinphoneChatMessageState)state) << " to " <<
		linphone_chat_message_state_to_string((LinphoneChatMessageState)s);
	state = s;

	LinphoneChatMessage *msg = L_GET_C_BACK_PTR(q);
	if (linphone_chat_message_get_message_state_changed_cb(msg))
		linphone_chat_message_get_message_state_changed_cb(msg)(
			msg,
			(LinphoneChatMessageState)state,
			linphone_chat_message_get_message_state_changed_cb_user_data(msg)
		);

	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
	if (cbs && linphone_chat_message_cbs_get_msg_state_changed(cbs))
		linphone_chat_message_cbs_get_msg_state_changed(cbs)(msg, linphone_chat_message_get_state(msg));
}

unsigned int ChatMessagePrivate::getStorageId () const {
	return storageId;
}

void ChatMessagePrivate::setStorageId (unsigned int id) {
	storageId = id;
}

belle_http_request_t *ChatMessagePrivate::getHttpRequest () const {
	return httpRequest;
}

void ChatMessagePrivate::setHttpRequest (belle_http_request_t *request) {
	httpRequest = request;
}

SalOp *ChatMessagePrivate::getSalOp () const {
	return salOp;
}

void ChatMessagePrivate::setSalOp (SalOp *op) {
	salOp = op;
}

SalCustomHeader *ChatMessagePrivate::getSalCustomHeaders () const {
	return salCustomHeaders;
}

void ChatMessagePrivate::setSalCustomHeaders (SalCustomHeader *headers) {
	salCustomHeaders = headers;
}

void ChatMessagePrivate::addSalCustomHeader (const string &name, const string &value) {
	salCustomHeaders = sal_custom_header_append(salCustomHeaders, name.c_str(), value.c_str());
}

void ChatMessagePrivate::removeSalCustomHeader (const string &name) {
	salCustomHeaders = sal_custom_header_remove(salCustomHeaders, name.c_str());
}

string ChatMessagePrivate::getSalCustomHeaderValue (const string &name) {
	return L_C_TO_STRING(sal_custom_header_find(salCustomHeaders, name.c_str()));
}

// -----------------------------------------------------------------------------

const ContentType &ChatMessagePrivate::getContentType () {
	if (direction == ChatMessage::Direction::Incoming) {
		if (contents.size() > 0) {
			Content content = contents.front();
			cContentType = content.getContentType();
		} else {
			cContentType = internalContent.getContentType();
		}
	} else {
		if (internalContent.getContentType().isValid()) {
			cContentType = internalContent.getContentType();
		} else {
			if (contents.size() > 0) {
				Content content = contents.front();
				cContentType = content.getContentType();
			}
		}
	}
	return cContentType;
}

void ChatMessagePrivate::setContentType (const ContentType &contentType) {
	internalContent.setContentType(contentType);
}

const string &ChatMessagePrivate::getText () {
	L_Q();
	if (direction == ChatMessage::Direction::Incoming) {
		if (contents.size() > 0) {
			Content content = contents.front();
			cText = content.getBodyAsString();
		} else {
			cText = internalContent.getBodyAsString();
		}
	} else {
		if (q->hasTextContent()) {
			cText = q->getTextContent().getBodyAsString();
		} else if (!internalContent.isEmpty()) {
			cText = internalContent.getBodyAsString();
		} else {
			if (contents.size() > 0) {
				Content content = contents.front();
				cText = content.getBodyAsString();
			}
		}
	}
	return cText;
}

void ChatMessagePrivate::setText (const string &text) {
	internalContent.setBody(text);
}

LinphoneContent *ChatMessagePrivate::getFileTransferInformation () const {
	return cFileTransferInformation;
}

void ChatMessagePrivate::setFileTransferInformation (LinphoneContent *content) {
	if (cFileTransferInformation) {
		linphone_content_unref(cFileTransferInformation);
		cFileTransferInformation = nullptr;
	}
	cFileTransferInformation = content;
}

// -----------------------------------------------------------------------------

string ChatMessagePrivate::createImdnXml (Imdn::Type imdnType, LinphoneReason reason) {
	xmlBufferPtr buf;
	xmlTextWriterPtr writer;
	int err;
	string content;
	char *datetime = nullptr;

	// Check that the chat message has a message id.
	if (id.empty()) return nullptr;

	buf = xmlBufferCreate();
	if (buf == nullptr) {
		lError() << "Error creating the XML buffer";
		return content;
	}
	writer = xmlNewTextWriterMemory(buf, 0);
	if (writer == nullptr) {
		lError() << "Error creating the XML writer";
		return content;
	}

	datetime = linphone_timestamp_to_rfc3339_string(time);
	err = xmlTextWriterStartDocument(writer, "1.0", "UTF-8", nullptr);
	if (err >= 0) {
		err = xmlTextWriterStartElementNS(writer, nullptr, (const xmlChar *)"imdn",
				(const xmlChar *)"urn:ietf:params:xml:ns:imdn");
	}
	if ((err >= 0) && (reason != LinphoneReasonNone)) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xmlns", (const xmlChar *)"linphoneimdn", nullptr, (const xmlChar *)"http://www.linphone.org/xsds/imdn.xsd");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"message-id", (const xmlChar *)id.c_str());
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"datetime", (const xmlChar *)datetime);
	}
	if (err >= 0) {
		if (imdnType == Imdn::Type::Delivery) {
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
			if (imdnType == Imdn::Type::Delivery) {
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
		err = xmlTextWriterStartElementNS(writer, (const xmlChar *)"linphoneimdn", (const xmlChar *)"reason", nullptr);
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

void ChatMessagePrivate::sendImdn (Imdn::Type imdnType, LinphoneReason reason) {
	L_Q();
	shared_ptr<ChatRoom> chatRoom = q->getChatRoom();
	if (chatRoom)
		chatRoom->getPrivate()->sendImdn(createImdnXml(imdnType, reason), reason);
}

static void _chat_message_file_transfer_on_progress (
	belle_sip_body_handler_t *bh,
	belle_sip_message_t *m,
	void *data,
	size_t offset,
	size_t total
) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->fileTransferOnProgress(bh, m, offset, total);
}

void ChatMessagePrivate::fileTransferOnProgress (
	belle_sip_body_handler_t *bh,
	belle_sip_message_t *m,
	size_t offset,
	size_t total
) {
	L_Q();

	if (!isFileTransferInProgressAndValid()) {
		lWarning() << "Cancelled request for " << (chatRoom.lock() ? "" : "ORPHAN") << " msg [" << this <<
			"], ignoring " << __FUNCTION__;
		releaseHttpRequest();
		return;
	}

	LinphoneChatMessage *msg = L_GET_C_BACK_PTR(q);
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
	if (linphone_chat_message_cbs_get_file_transfer_progress_indication(cbs)) {
		linphone_chat_message_cbs_get_file_transfer_progress_indication(cbs)(msg, cFileTransferInformation, offset, total);
	} else {
		// Legacy: call back given by application level.
		shared_ptr<Core> core = q->getCore();
		if (core)
			linphone_core_notify_file_transfer_progress_indication(
				core->getCCore(),
				msg,
				cFileTransferInformation,
				offset,
				total
			);
	}
}

static int _chat_message_on_send_body (
	belle_sip_user_body_handler_t *bh,
	belle_sip_message_t *m,
	void *data,
	size_t offset,
	uint8_t *buffer,
	size_t *size
) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	return d->onSendBody(bh, m, offset, buffer, size);
}

int ChatMessagePrivate::onSendBody (
	belle_sip_user_body_handler_t *bh,
	belle_sip_message_t *m,
	size_t offset,
	uint8_t *buffer,
	size_t *size
) {
	L_Q();

	int retval = -1;
	LinphoneChatMessage *msg = L_GET_C_BACK_PTR(q);

	if (!isFileTransferInProgressAndValid()) {
		if (httpRequest) {
			lWarning() << "Cancelled request for " << (chatRoom.lock() ? "" : "ORPHAN") <<
				" msg [" << this << "], ignoring " << __FUNCTION__;
			releaseHttpRequest();
		}
		return BELLE_SIP_STOP;
	}

	// if we've not reach the end of file yet, ask for more data
	// in case of file body handler, won't be called
	if (fileTransferFilePath.empty() && offset < linphone_content_get_size(cFileTransferInformation)) {
		// get data from call back
		LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
		LinphoneChatMessageCbsFileTransferSendCb file_transfer_send_cb =
			linphone_chat_message_cbs_get_file_transfer_send(cbs);
		if (file_transfer_send_cb) {
			LinphoneBuffer *lb = file_transfer_send_cb(msg, cFileTransferInformation, offset, *size);
			if (lb == nullptr) {
				*size = 0;
			} else {
				*size = linphone_buffer_get_size(lb);
				memcpy(buffer, linphone_buffer_get_content(lb), *size);
				linphone_buffer_unref(lb);
			}
		} else {
			// Legacy
			shared_ptr<Core> core = q->getCore();
			if (core)
				linphone_core_notify_file_transfer_send(core->getCCore(), msg, cFileTransferInformation, (char *)buffer, size);
		}
	}

	LinphoneImEncryptionEngine *imee = nullptr;
	shared_ptr<Core> core = q->getCore();
	if (core)
		imee = linphone_core_get_im_encryption_engine(core->getCCore());

	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsUploadingFileCb cb_process_uploading_file =
			linphone_im_encryption_engine_cbs_get_process_uploading_file(imee_cbs);
		if (cb_process_uploading_file) {
			size_t max_size = *size;
			uint8_t *encrypted_buffer = (uint8_t *)ms_malloc0(max_size);
			retval = cb_process_uploading_file(imee, msg, offset, (const uint8_t *)buffer, size, encrypted_buffer);
			if (retval == 0) {
				if (*size > max_size) {
					lError() << "IM encryption engine process upload file callback returned a size bigger than the size of the buffer, so it will be truncated !";
					*size = max_size;
				}
				memcpy(buffer, encrypted_buffer, *size);
			}
			ms_free(encrypted_buffer);
		}
	}

	return retval <= 0 ? BELLE_SIP_CONTINUE : BELLE_SIP_STOP;
}

static void _chat_message_on_send_end (belle_sip_user_body_handler_t *bh, void *data) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->onSendEnd(bh);
}

void ChatMessagePrivate::onSendEnd (belle_sip_user_body_handler_t *bh) {
	L_Q();

	LinphoneImEncryptionEngine *imee = nullptr;
	shared_ptr<Core> core = q->getCore();
	if (core)
		imee = linphone_core_get_im_encryption_engine(core->getCCore());

	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsUploadingFileCb cb_process_uploading_file = linphone_im_encryption_engine_cbs_get_process_uploading_file(imee_cbs);
		if (cb_process_uploading_file) {
			cb_process_uploading_file(imee, L_GET_C_BACK_PTR(q), 0, nullptr, nullptr, nullptr);
		}
	}
}

void ChatMessagePrivate::fileUploadEndBackgroundTask () {
	if (backgroundTaskId) {
		lInfo() << "channel [" << this << "]: ending file upload background task with id=[" << backgroundTaskId << "].";
		sal_end_background_task(backgroundTaskId);
		backgroundTaskId = 0;
	}
}

static void _chat_message_file_upload_background_task_ended (void *data) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->fileUploadBackgroundTaskEnded();
}

void ChatMessagePrivate::fileUploadBackgroundTaskEnded () {
	lWarning() << "channel [" << this << "]: file upload background task has to be ended now, but work isn't finished.";
	fileUploadEndBackgroundTask();
}

void ChatMessagePrivate::fileUploadBeginBackgroundTask () {
	if (backgroundTaskId == 0) {
		backgroundTaskId = sal_begin_background_task("file transfer upload", _chat_message_file_upload_background_task_ended, this);
		if (backgroundTaskId) lInfo() << "channel [" << this << "]: starting file upload background task with id=[" << backgroundTaskId << "].";
	}
}

static void _chat_message_on_recv_body (belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, void *data, size_t offset, uint8_t *buffer, size_t size) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->onRecvBody(bh, m, offset, buffer, size);
}

void ChatMessagePrivate::onRecvBody (belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, size_t offset, uint8_t *buffer, size_t size) {
	L_Q();

	int retval = -1;
	uint8_t *decrypted_buffer = nullptr;

	if (!chatRoom.lock()) {
		q->cancelFileTransfer();
		return;
	}

	if (!httpRequest || belle_http_request_is_cancelled(httpRequest)) {
		lWarning() << "Cancelled request for msg [" << this << "], ignoring " << __FUNCTION__;
		return;
	}

	// first call may be with a zero size, ignore it
	if (size == 0) {
		return;
	}

	decrypted_buffer = (uint8_t *)ms_malloc0(size);

	LinphoneImEncryptionEngine *imee = nullptr;
	shared_ptr<Core> core = q->getCore();
	if (core)
		imee = linphone_core_get_im_encryption_engine(core->getCCore());

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
				linphone_core_notify_file_transfer_recv(core->getCCore(), msg, cFileTransferInformation, (const char *)buffer, size);
			}
		}
	} else {
		lWarning() << "File transfer decrypt failed with code " << (int)retval;
		setState(ChatMessage::State::FileTransferError);
	}
}

static void _chat_message_on_recv_end (belle_sip_user_body_handler_t *bh, void *data) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->onRecvEnd(bh);
}

void ChatMessagePrivate::onRecvEnd (belle_sip_user_body_handler_t *bh) {
	L_Q();

	shared_ptr<Core> core = q->getCore();
	if (!core)
		return;

	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(core->getCCore());
	int retval = -1;

	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsDownloadingFileCb cb_process_downloading_file = linphone_im_encryption_engine_cbs_get_process_downloading_file(imee_cbs);
		if (cb_process_downloading_file) {
			retval = cb_process_downloading_file(imee, L_GET_C_BACK_PTR(q), 0, nullptr, 0, nullptr);
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
				linphone_core_notify_file_transfer_recv(core->getCCore(), msg, cFileTransferInformation, nullptr, 0);
			}
		}
	}

	if (retval <= 0 && state != ChatMessage::State::FileTransferError) {
		setState(ChatMessage::State::FileTransferDone);
	}
}

bool ChatMessagePrivate::isFileTransferInProgressAndValid () {
	L_Q();
	shared_ptr<ChatRoom> chatRoom = q->getChatRoom();
	return chatRoom && q->getCore() && httpRequest && !belle_http_request_is_cancelled(httpRequest);
}

static void _chat_message_process_response_from_post_file (void *data, const belle_http_response_event_t *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processResponseFromPostFile(event);
}

void ChatMessagePrivate::processResponseFromPostFile (const belle_http_response_event_t *event) {
	L_Q();

	if (httpRequest && !isFileTransferInProgressAndValid()) {
		lWarning() << "Cancelled request for " << (chatRoom.lock() ? "" : "ORPHAN") <<
			" msg [" << this << "], ignoring " << __FUNCTION__;
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
			LinphoneImEncryptionEngine *imee = nullptr;

			shared_ptr<Core> core = q->getCore();
			shared_ptr<ChatRoom> chatRoom = q->getChatRoom();

			imee = linphone_core_get_im_encryption_engine(core->getCCore());

			if (imee && chatRoom) {
				LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb is_encryption_enabled_for_file_transfer_cb =
					linphone_im_encryption_engine_cbs_get_is_encryption_enabled_for_file_transfer(imee_cbs);
				if (is_encryption_enabled_for_file_transfer_cb) {
					is_file_encryption_enabled = is_encryption_enabled_for_file_transfer_cb(imee, L_GET_C_BACK_PTR(chatRoom));
				}
			}
			// shall we encrypt the file
			if (is_file_encryption_enabled && chatRoom) {
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
				first_part_header = "form-data; name=\"File\"; filename=\"" + string(linphone_content_get_name(cFileTransferInformation)) + "\"";
			}

			// create a user body handler to take care of the file and add the content disposition and content-type headers
			first_part_bh = (belle_sip_body_handler_t *)belle_sip_user_body_handler_new(
					linphone_content_get_size(cFileTransferInformation),
					_chat_message_file_transfer_on_progress, nullptr, nullptr,
					_chat_message_on_send_body, _chat_message_on_send_end, this);
			if (!fileTransferFilePath.empty()) {
				belle_sip_user_body_handler_t *body_handler = (belle_sip_user_body_handler_t *)first_part_bh;
				// No need to add again the callback for progression, otherwise it will be called twice
				first_part_bh = (belle_sip_body_handler_t *)belle_sip_file_body_handler_new(fileTransferFilePath.c_str(), nullptr, this);
				linphone_content_set_size(cFileTransferInformation, belle_sip_file_body_handler_get_file_size((belle_sip_file_body_handler_t *)first_part_bh));
				belle_sip_file_body_handler_set_user_body_handler((belle_sip_file_body_handler_t *)first_part_bh, body_handler);
			} else if (linphone_content_get_buffer(cFileTransferInformation) != nullptr) {
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
			bh = belle_sip_multipart_body_handler_new(_chat_message_file_transfer_on_progress, this, first_part_bh, nullptr);

			releaseHttpRequest();
			fileUploadBeginBackgroundTask();
			q->uploadFile();
			belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(httpRequest), BELLE_SIP_BODY_HANDLER(bh));
		} else if (code == 200) {     // file has been uploaded correctly, get server reply and send it
			const char *body = belle_sip_message_get_body((belle_sip_message_t *)event->response);
			if (body && strlen(body) > 0) {
				// if we have an encryption key for the file, we must insert it into the msg and restore the correct filename
				const char *content_key = linphone_content_get_key(cFileTransferInformation);
				size_t content_key_size = linphone_content_get_key_size(cFileTransferInformation);
				if (content_key != nullptr) {
					// parse the msg body
					xmlDocPtr xmlMessageBody = xmlParseDoc((const xmlChar *)body);

					xmlNodePtr cur = xmlDocGetRootElement(xmlMessageBody);
					if (cur != nullptr) {
						cur = cur->xmlChildrenNode;
						while (cur != nullptr) {
							// we found a file info node, check it has a type="file" attribute
							if (!xmlStrcmp(cur->name, (const xmlChar *)"file-info")) {
								xmlChar *typeAttribute = xmlGetProp(cur, (const xmlChar *)"type");
								// this is the node we are looking for : add a file-key children node
								if (!xmlStrcmp(typeAttribute, (const xmlChar *)"file")) {
									// need to parse the children node to update the file-name one
									xmlNodePtr fileInfoNodeChildren = cur->xmlChildrenNode;
									// convert key to base64
									size_t b64Size = b64_encode(nullptr, content_key_size, nullptr, 0);
									char *keyb64 = (char *)ms_malloc0(b64Size + 1);
									int xmlStringLength;

									b64Size = b64_encode(content_key, content_key_size, keyb64, b64Size);
									keyb64[b64Size] = '\0';                   // libxml need a null terminated string

									// add the node containing the key to the file-info node
									xmlNewTextChild(cur, nullptr, (const xmlChar *)"file-key", (const xmlChar *)keyb64);
									xmlFree(typeAttribute);
									ms_free(keyb64);

									// look for the file-name node and update its content
									while (fileInfoNodeChildren != nullptr) {
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
				} else {         // no encryption key, transfer in plain, just copy the msg sent by server
					setText(body);
				}
				setContentType(ContentType::FileTransfer);
				
				Content fileTransferContent;
				fileTransferContent.setContentType(ContentType::FileTransfer);
				fileTransferContent.setBody(internalContent.getBodyAsString());
				q->addContent(fileTransferContent);

				q->updateState(ChatMessage::State::FileTransferDone);
				releaseHttpRequest();
				send();
				fileUploadEndBackgroundTask();
			} else {
				lWarning() << "Received empty response from server, file transfer failed";
				q->updateState(ChatMessage::State::NotDelivered);
				releaseHttpRequest();
				fileUploadEndBackgroundTask();
			}
		} else {
			lWarning() << "Unhandled HTTP code response " << code << " for file transfer";
			q->updateState(ChatMessage::State::NotDelivered);
			releaseHttpRequest();
			fileUploadEndBackgroundTask();
		}
	}
}

static void _chat_process_response_headers_from_get_file (void *data, const belle_http_response_event_t *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processResponseHeadersFromGetFile(event);
}

static LinphoneContent *createFileTransferInformationFromHeaders (const belle_sip_message_t *m) {
	LinphoneContent *content = linphone_content_new();

	belle_sip_header_content_length_t *content_length_hdr = BELLE_SIP_HEADER_CONTENT_LENGTH(belle_sip_message_get_header(m, "Content-Length"));
	belle_sip_header_content_type_t *content_type_hdr = BELLE_SIP_HEADER_CONTENT_TYPE(belle_sip_message_get_header(m, "Content-Type"));
	const char *type = nullptr, *subtype = nullptr;

	linphone_content_set_name(content, "");

	if (content_type_hdr) {
		type = belle_sip_header_content_type_get_type(content_type_hdr);
		subtype = belle_sip_header_content_type_get_subtype(content_type_hdr);
		lInfo() << "Extracted content type " << type << " / " << subtype << " from header";
		if (type) {
			linphone_content_set_type(content, type);
		}
		if (subtype) {
			linphone_content_set_subtype(content, subtype);
		}
	}

	if (content_length_hdr) {
		linphone_content_set_size(content, belle_sip_header_content_length_get_content_length(content_length_hdr));
		lInfo() << "Extracted content length " << linphone_content_get_size(content) << " from header";
	}

	return content;
}

void ChatMessagePrivate::processResponseHeadersFromGetFile (const belle_http_response_event_t *event) {
	if (event->response) {
		// we are receiving a response, set a specific body handler to acquire the response.
		// if not done, belle-sip will create a memory body handler, the default
		belle_sip_message_t *response = BELLE_SIP_MESSAGE(event->response);
		belle_sip_body_handler_t *body_handler = nullptr;
		size_t body_size = 0;

		if (cFileTransferInformation == nullptr) {
			lWarning() << "No file transfer information for msg [" << this << "]: creating...";
			cFileTransferInformation = createFileTransferInformationFromHeaders(response);
		}

		if (cFileTransferInformation) {
			body_size = linphone_content_get_size(cFileTransferInformation);
		}

		body_handler = (belle_sip_body_handler_t *)belle_sip_user_body_handler_new(body_size, _chat_message_file_transfer_on_progress,
				nullptr, _chat_message_on_recv_body,
				nullptr, _chat_message_on_recv_end, this);
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

static void _chat_message_process_auth_requested_download (void *data, belle_sip_auth_event *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processAuthRequestedDownload(event);
}

void ChatMessagePrivate::processAuthRequestedDownload (const belle_sip_auth_event *event) {
	L_Q();

	lError() << "Error during file download : auth requested for msg [" << this << "]";
	q->updateState(ChatMessage::State::FileTransferError);
	releaseHttpRequest();
}

static void _chat_message_process_io_error_upload (void *data, const belle_sip_io_error_event_t *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processIoErrorUpload(event);
}

void ChatMessagePrivate::processIoErrorUpload (const belle_sip_io_error_event_t *event) {
	L_Q();

	lError() << "I/O Error during file upload of msg [" << this << "]";
	q->updateState(ChatMessage::State::NotDelivered);
	releaseHttpRequest();

	shared_ptr<ChatRoom> chatRoom = q->getChatRoom();
	if (chatRoom)
		chatRoom->getPrivate()->removeTransientMessage(q->getSharedFromThis());
}

static void _chat_message_process_auth_requested_upload (void *data, belle_sip_auth_event *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processAuthRequestedUpload(event);
}

void ChatMessagePrivate::processAuthRequestedUpload (const belle_sip_auth_event *event) {
	L_Q();

	lError() << "Error during file upload: auth requested for msg [" << this << "]";
	q->updateState(ChatMessage::State::NotDelivered);
	releaseHttpRequest();

	shared_ptr<ChatRoom> chatRoom = q->getChatRoom();
	if (chatRoom)
		chatRoom->getPrivate()->removeTransientMessage(q->getSharedFromThis());
}

static void _chat_message_process_io_error_download (void *data, const belle_sip_io_error_event_t *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processIoErrorDownload(event);
}

void ChatMessagePrivate::processIoErrorDownload (const belle_sip_io_error_event_t *event) {
	L_Q();

	lError() << "I/O Error during file download msg [" << this << "]";
	q->updateState(ChatMessage::State::FileTransferError);
	releaseHttpRequest();
}

static void _chat_message_process_response_from_get_file (void *data, const belle_http_response_event_t *event) {
	ChatMessagePrivate *d = (ChatMessagePrivate *)data;
	d->processResponseFromGetFile(event);
}

void ChatMessagePrivate::processResponseFromGetFile (const belle_http_response_event_t *event) {
	// check the answer code
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code >= 400 && code < 500) {
			lWarning() << "File transfer failed with code " << code;
			setState(ChatMessage::State::FileTransferError);
		} else if (code != 200) {
			lWarning() << "Unhandled HTTP code response " << code << " for file transfer";
		}
		releaseHttpRequest();
	}
}

int ChatMessagePrivate::startHttpTransfer (
	const string &url,
	const string &action,
	belle_http_request_listener_callbacks_t *cbs
) {
	L_Q();

	belle_generic_uri_t *uri = nullptr;

	shared_ptr<Core> core = q->getCore();
	const char *ua = linphone_core_get_user_agent(core->getCCore());

	if (url.empty()) {
		lWarning() << "Cannot process file transfer msg [" << this << "]: no file remote URI configured.";
		goto error;
	}
	uri = belle_generic_uri_parse(url.c_str());
	if (uri == nullptr || belle_generic_uri_get_host(uri) == nullptr) {
		lWarning() << "Cannot process file transfer msg [" << this << "]: incorrect file remote URI configured '" <<
			url << "'.";
		goto error;
	}

	httpRequest = belle_http_request_create(action.c_str(), uri, belle_sip_header_create("User-Agent", ua), nullptr);

	if (httpRequest == nullptr) {
		lWarning() << "Could not create http request for uri " << url;
		goto error;
	}
	// keep a reference to the http request to be able to cancel it during upload
	belle_sip_object_ref(httpRequest);

	// give msg to listener to be able to start the actual file upload when server answer a 204 No content
	httpListener = belle_http_request_listener_create_from_callbacks(cbs, this);
	belle_http_provider_send_request(core->getCCore()->http_provider, httpRequest, httpListener);
	return 0;

error:
	if (uri) {
		belle_sip_object_unref(uri);
	}
	return -1;
}

void ChatMessagePrivate::releaseHttpRequest () {
	if (httpRequest) {
		belle_sip_object_unref(httpRequest);
		httpRequest = nullptr;
		if (httpListener) {
			belle_sip_object_unref(httpListener);
			httpListener = nullptr;
		}
	}
}

void ChatMessagePrivate::createFileTransferInformationsFromVndGsmaRcsFtHttpXml (const string &body) {
	xmlChar *file_url = nullptr;
	xmlDocPtr xmlMessageBody;
	xmlNodePtr cur;
	/* parse the msg body to get all informations from it */
	xmlMessageBody = xmlParseDoc((const xmlChar *)body.c_str());
	LinphoneContent *content = linphone_content_new();
	setFileTransferInformation(content);

	cur = xmlDocGetRootElement(xmlMessageBody);
	if (cur != nullptr) {
		cur = cur->xmlChildrenNode;
		while (cur != nullptr) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"file-info")) {
				/* we found a file info node, check if it has a type="file" attribute */
				xmlChar *typeAttribute = xmlGetProp(cur, (const xmlChar *)"type");
				if (!xmlStrcmp(typeAttribute, (const xmlChar *)"file")) {         /* this is the node we are looking for */
					cur = cur->xmlChildrenNode;           /* now loop on the content of the file-info node */
					while (cur != nullptr) {
						if (!xmlStrcmp(cur->name, (const xmlChar *)"file-size")) {
							xmlChar *fileSizeString = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
							linphone_content_set_size(content, (size_t)strtol((const char *)fileSizeString, nullptr, 10));
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
							size_t keyLength = b64::b64_decode((char *)keyb64, strlen((char *)keyb64), nullptr, 0);
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

	externalBodyUrl = string((const char *)file_url);
	xmlFree(file_url);
}

LinphoneReason ChatMessagePrivate::receive () {
	L_Q();
	int errorCode = 0;
	LinphoneReason reason = LinphoneReasonNone;

	shared_ptr<Core> core = q->getCore();
	shared_ptr<ChatRoom> chatRoom = q->getChatRoom();

	// ---------------------------------------
	// Start of message modification
	// ---------------------------------------

	if (internalContent.getContentType() == ContentType::Cpim) {
		CpimChatMessageModifier ccmm;
		ccmm.decode(q->getSharedFromThis(), errorCode);
	}

	EncryptionChatMessageModifier ecmm;
	ChatMessageModifier::Result result = ecmm.decode(q->getSharedFromThis(), errorCode);
	if (result == ChatMessageModifier::Result::Error) {
		/* Unable to decrypt message */
		if (chatRoom)
			chatRoom->getPrivate()->notifyUndecryptableMessageReceived(q->getSharedFromThis());
		reason = linphone_error_code_to_reason(errorCode);
		q->sendDeliveryNotification(reason);
		return reason;
	}

	MultipartChatMessageModifier mcmm;
	mcmm.decode(q->getSharedFromThis(), errorCode);

	if (contents.size() == 0) {
		// All previous modifiers only altered the internal content, let's fill the content list
		contents.push_back(internalContent);
	}

	// ---------------------------------------
	// End of message modification
	// ---------------------------------------

	if (errorCode <= 0) {
		bool foundSupportContentType = false;
		for (const auto &c : contents) {
			if (linphone_core_is_content_type_supported(core->getCCore(), c.getContentType().asString().c_str())) {
				foundSupportContentType = true;
				break;
			} else
			lError() << "Unsupported content-type: " << c.getContentType().asString();
		}

		if (!foundSupportContentType) {
			errorCode = 415;
			lError() << "No content-type in the contents list is supported...";
		}
	}

	// Check if this is in fact an outgoing message (case where this is a message sent by us from an other device).
	Address me(linphone_core_get_identity(core->getCCore()));
	if (me.weakEqual(from))
		setDirection(ChatMessage::Direction::Outgoing);

	// Check if this is a duplicate message.
	if (chatRoom && chatRoom->findMessageWithDirection(q->getImdnMessageId(), q->getDirection()))
		return core->getCCore()->chat_deny_code;

	if (errorCode > 0) {
		reason = linphone_error_code_to_reason(errorCode);
		q->sendDeliveryNotification(reason);
		return reason;
	}

	bool messageToBeStored = false;
	for (const auto &c : contents) {
		if (c.getContentType() == ContentType::FileTransfer) {
			messageToBeStored = true;
			createFileTransferInformationsFromVndGsmaRcsFtHttpXml(c.getBodyAsString());
		} else if (c.getContentType() == ContentType::PlainText)
			messageToBeStored = true;
	}
	if (messageToBeStored)
		q->store();

	return reason;
}

void ChatMessagePrivate::send () {
	L_Q();
	SalOp *op = salOp;
	LinphoneCall *call = nullptr;
	int errorCode = 0;

	if ((currentSendStep & ChatMessagePrivate::Step::FileUpload) == ChatMessagePrivate::Step::FileUpload) {
		lInfo() << "File upload step already done, skipping";
	} else {
		if (getFileTransferInformation()) {
			/* Open a transaction with the server and send an empty request(RCS5.1 section 3.5.4.8.3.1) */
			if (q->uploadFile() == 0) {
				setState(ChatMessage::State::InProgress);
				currentSendStep |= ChatMessagePrivate::Step::FileUpload;
			}
			return;
		}
	}

	shared_ptr<Core> core = q->getCore();
	if (lp_config_get_int(core->getCCore()->config, "sip", "chat_use_call_dialogs", 0) != 0) {
		call = linphone_core_get_call_by_remote_address(core->getCCore(), to.asString().c_str());
		if (call) {
			if (linphone_call_get_state(call) == LinphoneCallConnected || linphone_call_get_state(call) == LinphoneCallStreamsRunning ||
					linphone_call_get_state(call) == LinphoneCallPaused || linphone_call_get_state(call) == LinphoneCallPausing ||
					linphone_call_get_state(call) == LinphoneCallPausedByRemote) {
				lInfo() << "send SIP msg through the existing call.";
				op = linphone_call_get_op(call);
				string identity = linphone_core_find_best_identity(core->getCCore(), linphone_call_get_remote_address(call));
				if (identity.empty()) {
					LinphoneAddress *addr = linphone_address_new(to.asString().c_str());
					LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(core->getCCore(), addr);
					if (proxy) {
						identity = L_GET_CPP_PTR_FROM_C_OBJECT(linphone_proxy_config_get_identity_address(proxy))->asString();
					} else {
						identity = linphone_core_get_primary_contact(core->getCCore());
					}
					linphone_address_unref(addr);
				}
				q->setFromAddress(Address(identity));
			}
		}
	}

	if (!op) {
		LinphoneAddress *peer = linphone_address_new(to.asString().c_str());
		/* Sending out of call */
		salOp = op = new SalMessageOp(core->getCCore()->sal);
		linphone_configure_op(
			core->getCCore(), op, peer, getSalCustomHeaders(),
			!!lp_config_get_int(core->getCCore()->config, "sip", "chat_msg_with_contact", 0)
		);
		op->set_user_pointer(L_GET_C_BACK_PTR(q));     /* If out of call, directly store msg */
		linphone_address_unref(peer);
	}

	// ---------------------------------------
	// Start of message modification
	// ---------------------------------------

	// TODO Remove : This won't be necessary once we store the contentsList
	string clearTextMessage;
	ContentType clearTextContentType;

	if (!getText().empty()) {
		clearTextMessage = getText().c_str();
	}
	if (getContentType().isValid()) {
		clearTextContentType = getContentType();
	}
	// End of TODO Remove

	if (applyModifiers) {
		if ((currentSendStep &ChatMessagePrivate::Step::Multipart) == ChatMessagePrivate::Step::Multipart) {
			lInfo() << "Multipart step already done, skipping";
		} else {
			if (contents.size() > 1) {
				MultipartChatMessageModifier mcmm;
				mcmm.encode(q->getSharedFromThis(), errorCode);
			}
			currentSendStep |= ChatMessagePrivate::Step::Multipart;
		}

		if ((currentSendStep &ChatMessagePrivate::Step::Encryption) == ChatMessagePrivate::Step::Encryption) {
			lInfo() << "Encryption step already done, skipping";
		} else {
			EncryptionChatMessageModifier ecmm;
			ChatMessageModifier::Result result = ecmm.encode(q->getSharedFromThis(), errorCode);
			if (result == ChatMessageModifier::Result::Error) {
				sal_error_info_set((SalErrorInfo *)op->get_error_info(), SalReasonNotAcceptable, "SIP", errorCode, "Unable to encrypt IM", nullptr);
				q->updateState(ChatMessage::State::NotDelivered);
				q->store();
				return;
			} else if (result == ChatMessageModifier::Result::Suspended) {
				currentSendStep |= ChatMessagePrivate::Step::Encryption;
				return;
			}
			currentSendStep |= ChatMessagePrivate::Step::Encryption;
		}

		if ((currentSendStep &ChatMessagePrivate::Step::Cpim) == ChatMessagePrivate::Step::Cpim) {
			lInfo() << "Cpim step already done, skipping";
		} else {
			if (lp_config_get_int(core->getCCore()->config, "sip", "use_cpim", 0) == 1) {
				CpimChatMessageModifier ccmm;
				ccmm.encode(q->getSharedFromThis(), errorCode);
			}
			currentSendStep |= ChatMessagePrivate::Step::Cpim;
		}
	}

	// ---------------------------------------
	// End of message modification
	// ---------------------------------------

	if (internalContent.isEmpty()) {
		internalContent = contents.front();
	}

	if (!externalBodyUrl.empty()) {
		char *content_type = ms_strdup_printf("message/external-body; access-type=URL; URL=\"%s\"", externalBodyUrl.c_str());
		auto msgOp = dynamic_cast<SalMessageOpInterface *>(op);
		msgOp->send_message(from.asString().c_str(), to.asString().c_str(), content_type, nullptr, nullptr);
		ms_free(content_type);
	} else {
		auto msgOp = dynamic_cast<SalMessageOpInterface *>(op);
		if (internalContent.getContentType().isValid()) {
			msgOp->send_message(from.asString().c_str(), to.asString().c_str(), internalContent.getContentType().asString().c_str(), internalContent.getBodyAsString().c_str(), to.asStringUriOnly().c_str());
		} else {
			msgOp->send_message(from.asString().c_str(), to.asString().c_str(), internalContent.getBodyAsString().c_str());
		}
	}

	// TODO Remove : This won't be necessary once we store the contentsList
	if (!getText().empty() && getText() == clearTextMessage) {
		/* We replace the encrypted message by the original one so it can be correctly stored and displayed by the application */
		setText(clearTextMessage);
	}
	if (getContentType().isValid() && (getContentType() != clearTextContentType)) {
		/* We replace the encrypted content type by the original one */
		setContentType(clearTextContentType);
	}
	// End of TODO Remove

	q->setImdnMessageId(op->get_call_id());   /* must be known at that time */

	if (call && linphone_call_get_op(call) == op) {
		/* In this case, chat delivery status is not notified, so unrefing chat message right now */
		/* Might be better fixed by delivering status, but too costly for now */
		return;
	}

	/* If operation failed, we should not change message state */
	if (q->getDirection() == ChatMessage::Direction::Outgoing) {
		setIsReadOnly(true);
		setState(ChatMessage::State::InProgress);
	}
}

// -----------------------------------------------------------------------------

ChatMessage::ChatMessage (const shared_ptr<ChatRoom> &chatRoom) :
	Object(*new ChatMessagePrivate),
	CoreAccessor(chatRoom->getCore()) {
	L_ASSERT(chatRoom);
	L_D();

	d->chatRoom = chatRoom;
	d->peerAddress = chatRoom->getPeerAddress();
}

shared_ptr<ChatRoom> ChatMessage::getChatRoom () const {
	L_D();

	shared_ptr<ChatRoom> chatRoom = d->chatRoom.lock();
	if (!chatRoom)
		chatRoom = getCore()->getOrCreateBasicChatRoom(d->peerAddress);

	return chatRoom;
}

// -----------------------------------------------------------------------------

const string &ChatMessage::getExternalBodyUrl () const {
	L_D();
	return d->externalBodyUrl;
}

void ChatMessage::setExternalBodyUrl (const string &url) {
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

void ChatMessage::setIsSecured (bool isSecured) {
	L_D();
	d->isSecured = isSecured;
}

ChatMessage::Direction ChatMessage::getDirection () const {
	L_D();
	return d->direction;
}

ChatMessage::State ChatMessage::getState () const {
	L_D();
	return d->state;
}

const string &ChatMessage::getImdnMessageId () const {
	L_D();
	return d->id;
}

void ChatMessage::setImdnMessageId (const string &id) {
	L_D();
	d->id = id;
}

bool ChatMessage::isRead () const {
	L_D();

	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(getCore()->getCCore());
	if (linphone_im_notif_policy_get_recv_imdn_displayed(policy) && d->state == State::Displayed)
		return true;

	if (
		linphone_im_notif_policy_get_recv_imdn_delivered(policy) &&
		(d->state == State::DeliveredToUser || d->state == State::Displayed)
	)
		return true;

	return d->state == State::Delivered || d->state == State::Displayed || d->state == State::DeliveredToUser;
}

const string &ChatMessage::getAppdata () const {
	L_D();
	return d->appData;
}

void ChatMessage::setAppdata (const string &appData) {
	L_D();
	d->appData = appData;

	// TODO: history.
	// linphone_chat_message_store_appdata(L_GET_C_BACK_PTR(this));
}

const Address &ChatMessage::getFromAddress () const {
	L_D();
	return d->from;
}

void ChatMessage::setFromAddress (Address from) {
	L_D();
	d->from = from;
}

const Address &ChatMessage::getToAddress () const {
	L_D();
	return d->to;
}

void ChatMessage::setToAddress (Address to) {
	L_D();
	d->to = to;
}

const Address &ChatMessage::getLocalAddress () const {
	return getDirection() == Direction::Incoming ? getToAddress() : getFromAddress();
}

const Address &ChatMessage::getRemoteAddress () const {
	return getDirection() != Direction::Incoming ? getToAddress() : getFromAddress();
}

const string &ChatMessage::getFileTransferFilepath () const {
	L_D();
	return d->fileTransferFilePath;
}

void ChatMessage::setFileTransferFilepath (const string &path) {
	L_D();
	d->fileTransferFilePath = path;
}

// -----------------------------------------------------------------------------

const LinphoneErrorInfo *ChatMessage::getErrorInfo () const {
	L_D();
	if (!d->errorInfo) d->errorInfo = linphone_error_info_new();   // let's do it mutable
	linphone_error_info_from_sal_op(d->errorInfo, d->salOp);
	return d->errorInfo;
}

bool ChatMessage::isReadOnly () const {
	L_D();
	return d->isReadOnly;
}

const list<Content> &ChatMessage::getContents () const {
	L_D();
	return d->contents;
}

void ChatMessage::addContent (Content &&content) {
	L_D();
	if (d->isReadOnly) return;

	d->contents.push_back(move(content));
}

void ChatMessage::addContent (const Content &content) {
	L_D();
	if (d->isReadOnly) return;

	d->contents.push_back(content);
}

void ChatMessage::removeContent (const Content &content) {
	L_D();
	if (d->isReadOnly) return;

	d->contents.remove(content);
}

const Content &ChatMessage::getInternalContent () const {
	L_D();
	return d->internalContent;
}

void ChatMessage::setInternalContent (const Content &content) {
	L_D();
	d->internalContent = content;
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

bool ChatMessage::hasTextContent() const {
	L_D();
	for (const auto &c : d->contents) {
		if (c.getContentType() == ContentType::PlainText) {
			return true;
		}
	}
	return false;
}

const Content &ChatMessage::getTextContent() const {
	L_D();
	for (const auto &c : d->contents) {
		if (c.getContentType() == ContentType::PlainText) {
			return c;
		}
	}
	return Content::Empty;
}

bool ChatMessage::hasFileTransferContent() const {
	L_D();
	for (const auto &c : d->contents) {
		if (c.getContentType() == ContentType::FileTransfer) {
			return true;
		}
	}
	return false;
}

const Content &ChatMessage::getFileTransferContent() const {
	L_D();
	for (const auto &c : d->contents) {
		if (c.getContentType() == ContentType::FileTransfer) {
			return c;
		}
	}
	return Content::Empty;
}

// -----------------------------------------------------------------------------

void ChatMessage::store () {
	L_D();

	if (d->storageId != 0) {
		/* The message has already been stored (probably because of file transfer), update it */
		// TODO: history.
		// linphone_chat_message_store_update(L_GET_C_BACK_PTR(this));
	} else {
		/* Store the new message */
		// TODO: history.
		// linphone_chat_message_store(L_GET_C_BACK_PTR(this));
	}
}

void ChatMessage::updateState (State state) {
	L_D();

	d->setState(state);
	// TODO: history.
	// linphone_chat_message_store_state(L_GET_C_BACK_PTR(this));

	if (state == State::Delivered || state == State::NotDelivered) {
		shared_ptr<ChatRoom> chatRoom = getChatRoom();
		if (chatRoom)
			chatRoom->getPrivate()->moveTransientMessageToWeakMessages(getSharedFromThis());
	}
}

void ChatMessage::send () {
	L_D();

	// Do not allow sending a message that is already being sent or that has been correctly delivered/displayed
	if ((d->state == State::InProgress) || (d->state == State::Delivered) || (d->state == State::FileTransferDone) ||
			(d->state == State::DeliveredToUser) || (d->state == State::Displayed)) {
		lWarning() << "Cannot send chat message in state " << linphone_chat_message_state_to_string((LinphoneChatMessageState)d->state);
		return;
	}

	shared_ptr<ChatRoom> chatRoom = getChatRoom();
	if (chatRoom)
		chatRoom->getPrivate()->sendMessage(getSharedFromThis());
}

void ChatMessage::sendDeliveryNotification (LinphoneReason reason) {
	L_D();

	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(getCore()->getCCore());
	if (linphone_im_notif_policy_get_send_imdn_delivered(policy))
		d->sendImdn(Imdn::Type::Delivery, reason);
}

void ChatMessage::sendDisplayNotification () {
	L_D();

	LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(getCore()->getCCore());
	if (linphone_im_notif_policy_get_send_imdn_displayed(policy))
		d->sendImdn(Imdn::Type::Display, LinphoneReasonNone);
}

int ChatMessage::uploadFile () {
	L_D();

	if (d->httpRequest) {
		lError() << "linphone_chat_room_upload_file(): there is already an upload in progress.";
		return -1;
	}

	belle_http_request_listener_callbacks_t cbs = { 0 };
	cbs.process_response = _chat_message_process_response_from_post_file;
	cbs.process_io_error = _chat_message_process_io_error_upload;
	cbs.process_auth_requested = _chat_message_process_auth_requested_upload;

	int err = d->startHttpTransfer(linphone_core_get_file_transfer_server(getCore()->getCCore()), "POST", &cbs);
	if (err == -1)
		d->setState(State::NotDelivered);

	return err;
}

int ChatMessage::downloadFile () {
	L_D();

	if (d->httpRequest) {
		lError() << "linphone_chat_message_download_file(): there is already a download in progress";
		return -1;
	}

	belle_http_request_listener_callbacks_t cbs = { 0 };
	cbs.process_response_headers = _chat_process_response_headers_from_get_file;
	cbs.process_response = _chat_message_process_response_from_get_file;
	cbs.process_io_error = _chat_message_process_io_error_download;
	cbs.process_auth_requested = _chat_message_process_auth_requested_download;
	int err = d->startHttpTransfer(d->externalBodyUrl, "GET", &cbs);

	if (err == -1) return -1;
	// start the download, status is In Progress
	d->setState(State::InProgress);
	return 0;
}

void ChatMessage::cancelFileTransfer () {
	L_D();
	if (d->httpRequest) {
		if (d->state == State::InProgress) {
			d->setState(State::NotDelivered);
		}
		if (!belle_http_request_is_cancelled(d->httpRequest)) {
			shared_ptr<ChatRoom> chatRoom = getChatRoom();
			if (chatRoom) {
				shared_ptr<Core> core = getCore();
				lInfo() << "Canceling file transfer " << (
					d->externalBodyUrl.empty()
						? linphone_core_get_file_transfer_server(core->getCCore())
						: d->externalBodyUrl.c_str()
					);
				belle_http_provider_cancel_request(core->getCCore()->http_provider, d->httpRequest);
			} else {
				lInfo() << "Warning: http request still running for ORPHAN msg: this is a memory leak";
			}
		}
		d->releaseHttpRequest();
	} else {
		lInfo() << "No existing file transfer - nothing to cancel";
	}
}

int ChatMessage::putCharacter (uint32_t character) {
	L_D();

	shared_ptr<Core> core = getCore();
	if (linphone_core_realtime_text_enabled(core->getCCore())) {
		static const uint32_t new_line = 0x2028;
		static const uint32_t crlf = 0x0D0A;
		static const uint32_t lf = 0x0A;

		shared_ptr<ChatRoom> chatRoom = getChatRoom();
		if (!chatRoom)
			return -1;

		shared_ptr<LinphonePrivate::RealTimeTextChatRoom> rttcr =
			static_pointer_cast<LinphonePrivate::RealTimeTextChatRoom>(chatRoom);
		LinphoneCall *call = rttcr->getCall();

		if (!call || !linphone_call_get_stream(call, LinphoneStreamTypeText))
			return -1;

		if (character == new_line || character == crlf || character == lf) {
			if (lp_config_get_int(core->getCCore()->config, "misc", "store_rtt_messages", 1) == 1) {
				lDebug() << "New line sent, forge a message with content " << d->rttMessage.c_str();
				d->setTime(ms_time(0));
				d->state = State::Displayed;
				d->direction = Direction::Outgoing;
				setFromAddress(LinphonePrivate::Address(
					linphone_address_as_string(linphone_address_new(linphone_core_get_identity(core->getCCore())))
				));
				// TODO: History.
				// linphone_chat_message_store(L_GET_C_BACK_PTR(this));
				d->rttMessage = "";
			}
		} else {
			char *value = LinphonePrivate::Utils::utf8ToChar(character);
			d->rttMessage = d->rttMessage + string(value);
			lDebug() << "Sent RTT character: " << value << "(" << (unsigned long)character <<
				"), pending text is " << d->rttMessage.c_str();
			delete value;
		}

		text_stream_putchar32(reinterpret_cast<TextStream *>(
			linphone_call_get_stream(call, LinphoneStreamTypeText)), character
		);
		return 0;
	}
	return -1;
}

LINPHONE_END_NAMESPACE
