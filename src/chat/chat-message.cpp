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

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

using namespace std;

// =============================================================================
// ChatMessagePrivate
// =============================================================================

ChatMessagePrivate::ChatMessagePrivate (const shared_ptr<ChatRoom> &room)
: chatRoom(room) {}

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
		/* TODO
		ms_message("Chat message %p: moving from state %s to %s", msg, linphone_chat_message_state_to_string(msg->state), linphone_chat_message_state_to_string(state));
		*/
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

void ChatMessagePrivate::addSalCustomHeader(string name, string value) {
	salCustomHeaders = sal_custom_header_append(salCustomHeaders, name.c_str(), value.c_str());
}

void ChatMessagePrivate::removeSalCustomHeader(string name) {
	salCustomHeaders = sal_custom_header_remove(salCustomHeaders, name.c_str());
}

string ChatMessagePrivate::getSalCustomHeaderValue(string name) {
	return sal_custom_header_find(salCustomHeaders, name.c_str());
}

// -----------------------------------------------------------------------------

string ChatMessagePrivate::getContentType() const {
	return cContentType;
}

void ChatMessagePrivate::setContentType(string contentType) {
	cContentType = contentType;
}

string ChatMessagePrivate::getText() const {
	return cText;
}

void ChatMessagePrivate::setText(string text) {
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
	/*xmlBufferPtr buf;
	xmlTextWriterPtr writer;
	int err;
	char *content = NULL;
	char *datetime = NULL;
	const char *message_id;

	// Check that the chat message has a message id
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
		content = ms_strdup((char *)buf->content);
	}
	xmlFreeTextWriter(writer);
	xmlBufferFree(buf);
	ms_free(datetime);
	return content;*/
	return "";
}

void ChatMessagePrivate::sendImdn(ImdnType imdnType, LinphoneReason reason) {
	string content = createImdnXml(imdnType, reason);
	chatRoom->getPrivate()->sendImdn(content, reason);
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

string ChatMessage::getExternalBodyUrl() const {
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

string ChatMessage::getId () const {
	L_D();
	return d->id;
}

void ChatMessage::setId (string id) {
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

string ChatMessage::getAppdata () const {
	L_D();
	return d->appData;
}

void ChatMessage::setAppdata (const string &appData) {
	L_D();
	d->appData = appData;
	// TODO: store app data in db !
	// linphone_chat_message_store_appdata(msg);
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

string ChatMessage::getFileTransferFilepath() const {
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
	return "";
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
	//TODO
	/*linphone_chat_message_set_state(msg, new_state);
	linphone_chat_message_store_state(msg);

	if (msg->state == LinphoneChatMessageStateDelivered || msg->state == LinphoneChatMessageStateNotDelivered) {
		L_GET_PRIVATE_FROM_C_OBJECT(msg->chat_room)->moveTransientMessageToWeakMessages(msg);
	}*/
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

	// TODO.

	d->isReadOnly = true;
}

void ChatMessage::reSend() {
	L_D();

	if (d->state != NotDelivered) {
		// ms_warning("Cannot resend chat message in state %s", linphone_chat_message_state_to_string(state));
		return;
	}

	d->chatRoom->sendMessage(L_GET_C_BACK_PTR(this));
}

/*static void linphone_chat_message_process_io_error_upload(void *data, const belle_sip_io_error_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("I/O Error during file upload of msg [%p]", msg);
	linphone_chat_message_update_state(msg, LinphoneChatMessageStateNotDelivered);
	_release_http_request(msg);
	linphone_chat_room_remove_transient_message(msg->chat_room, msg);
	linphone_chat_message_unref(msg);
}

static void linphone_chat_message_process_auth_requested_upload(void *data, belle_sip_auth_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("Error during file upload: auth requested for msg [%p]", msg);
	linphone_chat_message_update_state(msg, LinphoneChatMessageStateNotDelivered);
	_release_http_request(msg);
	linphone_chat_room_remove_transient_message(msg->chat_room, msg);
	linphone_chat_message_unref(msg);
}

static void linphone_chat_message_process_io_error_download(void *data, const belle_sip_io_error_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("I/O Error during file download msg [%p]", msg);
	linphone_chat_message_update_state(msg, LinphoneChatMessageStateFileTransferError);
	_release_http_request(msg);
}

static void linphone_chat_process_response_from_get_file(void *data, const belle_http_response_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	// check the answer code
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
}*/

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

/*
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
		 //we are receiving a response, set a specific body handler to acquire the response.
		 // if not done, belle-sip will create a memory body handler, the default
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
				 // If the size of the body has not been initialized from the file stat, use the one from the
				 // file_transfer_information.
				 belle_sip_body_handler_set_size((belle_sip_body_handler_t *)body_handler, body_size);
			 }
			 belle_sip_file_body_handler_set_user_body_handler((belle_sip_file_body_handler_t *)body_handler, bh);
		 }
		 belle_sip_message_set_body_handler((belle_sip_message_t *)event->response, body_handler);
	 }
 }*/

/*static bool_t file_transfer_in_progress_and_valid(LinphoneChatMessage* msg) {
	return (linphone_chat_message_get_chat_room(msg) &&
		linphone_chat_room_get_core(linphone_chat_message_get_chat_room(msg)) &&
		linphone_chat_message_get_http_request(msg) &&
		!belle_http_request_is_cancelled(linphone_chat_message_get_http_request(msg)));
}

static void _release_http_request(LinphoneChatMessage* msg) {
	if (linphone_chat_message_get_http_request(msg)) {
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

static void linphone_chat_message_process_auth_requested_download(void *data, belle_sip_auth_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	ms_error("Error during file download : auth requested for msg [%p]", msg);
	linphone_chat_message_update_state(msg, LinphoneChatMessageStateFileTransferError);
	_release_http_request(msg);
}

static void linphone_chat_message_file_transfer_on_progress(belle_sip_body_handler_t *bh, belle_sip_message_t *m,
															void *data, size_t offset, size_t total) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	if (!file_transfer_in_progress_and_valid(msg)) {
		ms_warning("Cancelled request for %s msg [%p], ignoring %s", msg->chat_room?"":"ORPHAN", msg, __FUNCTION__);
		_release_http_request(msg);
		return;
	}
	if (linphone_chat_message_cbs_get_file_transfer_progress_indication(msg->cbs)) {
		linphone_chat_message_cbs_get_file_transfer_progress_indication(msg->cbs)(
			msg, msg->file_transfer_information, offset, total);
	} else {
		// Legacy: call back given by application level
		linphone_core_notify_file_transfer_progress_indication(linphone_chat_room_get_core(msg->chat_room), msg, msg->file_transfer_information,
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

	lc = linphone_chat_room_get_core(msg->chat_room);
	// if we've not reach the end of file yet, ask for more data
	// in case of file body handler, won't be called
	if (msg->file_transfer_filepath == NULL && offset < linphone_content_get_size(msg->file_transfer_information)) {
		// get data from call back
		LinphoneChatMessageCbsFileTransferSendCb file_transfer_send_cb = linphone_chat_message_cbs_get_file_transfer_send(msg->cbs);
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
			// Legacy
			linphone_core_notify_file_transfer_send(lc, msg, msg->file_transfer_information, (char *)buffer, size);
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

static void on_send_end(belle_sip_user_body_handler_t *bh, void *data) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	LinphoneCore *lc = linphone_chat_room_get_core(msg->chat_room);
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

static void linphone_chat_message_process_response_from_post_file(void *data, const belle_http_response_event_t *event) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;

	if (msg->http_request && !file_transfer_in_progress_and_valid(msg)) {
		ms_warning("Cancelled request for %s msg [%p], ignoring %s", msg->chat_room?"":"ORPHAN", msg, __FUNCTION__);
		_release_http_request(msg);
		return;
	}

	// check the answer code
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code == 204) { // this is the reply to the first post to the server - an empty msg
			// start uploading the file
			belle_sip_multipart_body_handler_t *bh;
			char *first_part_header;
			belle_sip_body_handler_t *first_part_bh;

			bool_t is_file_encryption_enabled = FALSE;
			LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(linphone_chat_room_get_core(msg->chat_room));
			if (imee) {
				LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb is_encryption_enabled_for_file_transfer_cb =
					linphone_im_encryption_engine_cbs_get_is_encryption_enabled_for_file_transfer(imee_cbs);
				if (is_encryption_enabled_for_file_transfer_cb) {
					is_file_encryption_enabled = is_encryption_enabled_for_file_transfer_cb(imee, msg->chat_room);
				}
			}
			// shall we encrypt the file
			if (is_file_encryption_enabled) {
				LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsGenerateFileTransferKeyCb generate_file_transfer_key_cb =
					linphone_im_encryption_engine_cbs_get_generate_file_transfer_key(imee_cbs);
				if (generate_file_transfer_key_cb) {
					generate_file_transfer_key_cb(imee, msg->chat_room, msg);
				}
				// temporary storage for the Content-disposition header value : use a generic filename to not leak it
				// Actual filename stored in msg->file_transfer_information->name will be set in encrypted msg
				// sended to the
				first_part_header = belle_sip_strdup_printf("form-data; name=\"File\"; filename=\"filename.txt\"");
			} else {
				// temporary storage for the Content-disposition header value
				first_part_header = belle_sip_strdup_printf("form-data; name=\"File\"; filename=\"%s\"",
															linphone_content_get_name(msg->file_transfer_information));
			}

			// create a user body handler to take care of the file and add the content disposition and content-type headers
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

			// insert it in a multipart body handler which will manage the boundaries of multipart msg
			bh = belle_sip_multipart_body_handler_new(linphone_chat_message_file_transfer_on_progress, msg, first_part_bh, NULL);

			linphone_chat_message_ref(msg);
			_release_http_request(msg);
			file_upload_begin_background_task(msg);
			linphone_chat_room_upload_file(msg);
			belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(msg->http_request), BELLE_SIP_BODY_HANDLER(bh));
			linphone_chat_message_unref(msg);
		} else if (code == 200) { // file has been uploaded correctly, get server reply and send it
			const char *body = belle_sip_message_get_body((belle_sip_message_t *)event->response);
			if (body && strlen(body) > 0) {
				// if we have an encryption key for the file, we must insert it into the msg and restore the correct filename
				const char *content_key = linphone_content_get_key(msg->file_transfer_information);
				size_t content_key_size = linphone_content_get_key_size(msg->file_transfer_information);
				if (content_key != NULL) {
					// parse the msg body
					xmlDocPtr xmlMessageBody = xmlParseDoc((const xmlChar *)body);

					xmlNodePtr cur = xmlDocGetRootElement(xmlMessageBody);
					if (cur != NULL) {
						cur = cur->xmlChildrenNode;
						while (cur != NULL) {
							if (!xmlStrcmp(cur->name, (const xmlChar *)"file-info")) { // we found a file info node, check
																					   //  it has a type="file" attribute
								xmlChar *typeAttribute = xmlGetProp(cur, (const xmlChar *)"type");
								if (!xmlStrcmp(typeAttribute,
											   (const xmlChar *)"file")) { // this is the node we are looking for : add a
																		   // file-key children node
									xmlNodePtr fileInfoNodeChildren =
										cur
											->xmlChildrenNode; // need to parse the children node to update the file-name one
									// convert key to base64
									size_t b64Size = b64::b64_encode(NULL, content_key_size, NULL, 0);
									char *keyb64 = (char *)ms_malloc0(b64Size + 1);
									int xmlStringLength;

									b64Size = b64::b64_encode(content_key, content_key_size, keyb64, b64Size);
									keyb64[b64Size] = '\0'; // libxml need a null terminated string

									// add the node containing the key to the file-info node
									xmlNewTextChild(cur, NULL, (const xmlChar *)"file-key", (const xmlChar *)keyb64);
									xmlFree(typeAttribute);
									ms_free(keyb64);

									// look for the file-name node and update its content
									while (fileInfoNodeChildren != NULL) {
										if (!xmlStrcmp(
												fileInfoNodeChildren->name,
												(const xmlChar *)"file-name")) { // we found a the file-name node, update
																				 //	its content with the real filename
											// update node content
											xmlNodeSetContent(fileInfoNodeChildren,
															  (const xmlChar *)(linphone_content_get_name(
																  msg->file_transfer_information)));
											break;
										}
										fileInfoNodeChildren = fileInfoNodeChildren->next;
									}

									// dump the xml into msg->message
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
				} else { // no encryption key, transfer in plain, just copy the msg sent by server
					msg->message = ms_strdup(body);
				}
				linphone_chat_message_set_content_type(msg, "application/vnd.gsma.rcs-ft-http+xml");
				linphone_chat_message_ref(msg);
				linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferDone);
				_release_http_request(msg);
				L_GET_CPP_PTR_FROM_C_OBJECT(msg->chat_room)->sendMessage(msg);
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

static void on_recv_body(belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, void *data, size_t offset, uint8_t *buffer, size_t size) {
	LinphoneChatMessage *msg = (LinphoneChatMessage *)data;
	LinphoneCore *lc = NULL;
	LinphoneImEncryptionEngine *imee = NULL;
	int retval = -1;
	uint8_t *decrypted_buffer = NULL;

	if (!msg->chat_room) {
		linphone_chat_message_cancel_file_transfer(msg);
		return;
	}
	lc = linphone_chat_room_get_core(msg->chat_room);

	if (lc == NULL){
		return; // might happen during linphone_core_destroy()
	}

	if (!msg->http_request || belle_http_request_is_cancelled(msg->http_request)) {
		ms_warning("Cancelled request for msg [%p], ignoring %s", msg, __FUNCTION__);
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
			retval = cb_process_downloading_file(imee, msg, offset, (const uint8_t *)buffer, size, decrypted_buffer);
			if (retval == 0) {
				memcpy(buffer, decrypted_buffer, size);
			}
		}
	}
	ms_free(decrypted_buffer);

	if (retval <= 0) {
		if (msg->file_transfer_filepath == NULL) {
			if (linphone_chat_message_cbs_get_file_transfer_recv(msg->cbs)) {
				LinphoneBuffer *lb = linphone_buffer_new_from_data(buffer, size);
				linphone_chat_message_cbs_get_file_transfer_recv(msg->cbs)(msg, msg->file_transfer_information, lb);
				linphone_buffer_unref(lb);
			} else {
				// Legacy: call back given by application level
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
	LinphoneCore *lc = linphone_chat_room_get_core(msg->chat_room);
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(lc);
	int retval = -1;

	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsDownloadingFileCb cb_process_downloading_file = linphone_im_encryption_engine_cbs_get_process_downloading_file(imee_cbs);
		if (cb_process_downloading_file) {
			retval = cb_process_downloading_file(imee, msg, 0, NULL, 0, NULL);
		}
	}

	if (retval <= 0) {
		if (msg->file_transfer_filepath == NULL) {
			if (linphone_chat_message_cbs_get_file_transfer_recv(msg->cbs)) {
				LinphoneBuffer *lb = linphone_buffer_new();
				linphone_chat_message_cbs_get_file_transfer_recv(msg->cbs)(msg, msg->file_transfer_information, lb);
				linphone_buffer_unref(lb);
			} else {
				// Legacy: call back given by application level
				linphone_core_notify_file_transfer_recv(lc, msg, msg->file_transfer_information, NULL, 0);
			}
		}
	}

	if (retval <= 0 && linphone_chat_message_get_state(msg) != LinphoneChatMessageStateFileTransferError) {
		linphone_chat_message_set_state(msg, LinphoneChatMessageStateFileTransferDone);
	}
}*/

/*

static int _linphone_chat_room_start_http_transfer(LinphoneChatMessage *msg, const char* url, const char* action, const belle_http_request_listener_callbacks_t *cbs) {
	belle_generic_uri_t *uri = NULL;
	const char* ua = linphone_core_get_user_agent(linphone_chat_room_get_core(msg->chat_room));

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
	// keep a reference to the http request to be able to cancel it during upload
	belle_sip_object_ref(msg->http_request);

		// give msg to listener to be able to start the actual file upload when server answer a 204 No content
		msg->http_listener = belle_http_request_listener_create_from_callbacks(cbs, linphone_chat_message_ref(msg));
		belle_http_provider_send_request(linphone_chat_room_get_core(msg->chat_room)->http_provider, msg->http_request, msg->http_listener);
		return 0;
	error:
		if (uri) {
			belle_sip_object_unref(uri);
		}
		return -1;
	}
*/

int ChatMessage::uploadFile() {
	return -1;
	//TODO
	/*belle_http_request_listener_callbacks_t cbs = {0};
	int err;

	if (msg->http_request){
		ms_error("linphone_chat_room_upload_file(): there is already an upload in progress.");
		return -1;
	}

	cbs.process_response = linphone_chat_message_process_response_from_post_file;
	cbs.process_io_error = linphone_chat_message_process_io_error_upload;
	cbs.process_auth_requested = linphone_chat_message_process_auth_requested_upload;
	err = _linphone_chat_room_start_http_transfer(msg, linphone_core_get_file_transfer_server(linphone_chat_room_get_core(msg->chat_room)), "POST", &cbs);
	if (err == -1){
		linphone_chat_message_set_state(msg, LinphoneChatMessageStateNotDelivered);
	}
	return err;*/
}

int ChatMessage::downloadFile() {
	return -1;
	//TODO
	/*belle_http_request_listener_callbacks_t cbs = {0};
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
	// start the download, status is In Progress
	linphone_chat_message_set_state(msg, LinphoneChatMessageStateInProgress);
	return 0;*/
}

void ChatMessage::cancelFileTransfer() {
	//TODO
	/*if (msg->http_request) {
		if (msg->state == LinphoneChatMessageStateInProgress) {
			linphone_chat_message_set_state(msg, LinphoneChatMessageStateNotDelivered);
		}
		if (!belle_http_request_is_cancelled(msg->http_request)) {
			if (msg->chat_room) {
				ms_message("Canceling file transfer %s - msg [%p] chat room[%p]"
								, (msg->external_body_url == NULL) ? linphone_core_get_file_transfer_server(linphone_chat_room_get_core(msg->chat_room)) : msg->external_body_url
								, msg
								, msg->chat_room);
				belle_http_provider_cancel_request(linphone_chat_room_get_core(msg->chat_room)->http_provider, msg->http_request);
				if ((msg->dir == LinphoneChatMessageOutgoing) && unref) {
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
	}*/
}

int ChatMessage::putCharacter(uint32_t character) {
	//TODO
	/*LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(msg);
	if (linphone_core_realtime_text_enabled(linphone_chat_room_get_core(cr))) {
		shared_ptr<LinphonePrivate::RealTimeTextChatRoom> rttcr =
			static_pointer_cast<LinphonePrivate::RealTimeTextChatRoom>(L_GET_CPP_PTR_FROM_C_OBJECT(cr));
		LinphoneCall *call = rttcr->getCall();
		LinphoneCore *lc = rttcr->getCore();
		const uint32_t new_line = 0x2028;
		const uint32_t crlf = 0x0D0A;
		const uint32_t lf = 0x0A;

		if (!call || !linphone_call_get_stream(call, LinphoneStreamTypeText)) {
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

		text_stream_putchar32(reinterpret_cast<TextStream *>(linphone_call_get_stream(call, LinphoneStreamTypeText)), character);
		return 0;
	}*/
	return -1;
}

LINPHONE_END_NAMESPACE
