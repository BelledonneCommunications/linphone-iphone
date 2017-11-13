/*
 * file-transfer-chat-message-modifier.cpp
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

#include "c-wrapper/c-wrapper.h"
#include "address/address.h"
#include "chat/chat-message/chat-message-p.h"
#include "content/content-type.h"
#include "content/content.h"
#include "chat/chat-room/chat-room-p.h"
#include "core/core.h"
#include "logger/logger.h"

#include "file-transfer-chat-message-modifier.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

belle_http_request_t *FileTransferChatMessageModifier::getHttpRequest () const {
	return httpRequest;
}

void FileTransferChatMessageModifier::setHttpRequest (belle_http_request_t *request) {
	httpRequest = request;
}

ChatMessageModifier::Result FileTransferChatMessageModifier::encode (const shared_ptr<ChatMessage> &message, int &errorCode) {
	chatMessage = message;

	currentFileContentToTransfer = nullptr;
	// For each FileContent, upload it and create a FileTransferContent
	for (Content *content : chatMessage->getContents()) {
		ContentType contentType = content->getContentType();
		if (contentType.isFile()) {
				lInfo() << "Found content with type " << contentType.asString() << ", set it for file upload";
				FileContent *fileContent = (FileContent *)content;
				currentFileContentToTransfer = fileContent;
				break;
		}
	}
	if (currentFileContentToTransfer != nullptr) {
		/* Open a transaction with the server and send an empty request(RCS5.1 section 3.5.4.8.3.1) */
		if (uploadFile() == 0) {
			return ChatMessageModifier::Result::Suspended;
		}
		return ChatMessageModifier::Result::Error;
	}

	return ChatMessageModifier::Result::Skipped;
}

// ----------------------------------------------------------

static void _chat_message_file_transfer_on_progress (
	belle_sip_body_handler_t *bh,
	belle_sip_message_t *m,
	void *data,
	size_t offset,
	size_t total
) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->fileTransferOnProgress(bh, m, offset, total);
}

void FileTransferChatMessageModifier::fileTransferOnProgress (
	belle_sip_body_handler_t *bh,
	belle_sip_message_t *m,
	size_t offset,
	size_t total
) {
	if (!isFileTransferInProgressAndValid()) {
		/*lWarning() << "Cancelled request for " << (chatRoom->lock() ? "" : "ORPHAN") << " msg [" << this <<
			"], ignoring " << __FUNCTION__;*/
		releaseHttpRequest();
		return;
	}

	LinphoneChatMessage *msg = L_GET_C_BACK_PTR(chatMessage);
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
	LinphoneContent *content = currentFileContentToTransfer->toLinphoneContent();
	if (linphone_chat_message_cbs_get_file_transfer_progress_indication(cbs)) {
		linphone_chat_message_cbs_get_file_transfer_progress_indication(cbs)(msg, content, offset, total);
	} else {
		// Legacy: call back given by application level.
		shared_ptr<Core> core = chatMessage->getCore();
		if (core)
			linphone_core_notify_file_transfer_progress_indication(
				core->getCCore(),
				msg,
				content,
				offset,
				total
			);
	}
	linphone_content_unref(content);
}

static int _chat_message_on_send_body (
	belle_sip_user_body_handler_t *bh,
	belle_sip_message_t *m,
	void *data,
	size_t offset,
	uint8_t *buffer,
	size_t *size
) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	return d->onSendBody(bh, m, offset, buffer, size);
}

int FileTransferChatMessageModifier::onSendBody (
	belle_sip_user_body_handler_t *bh,
	belle_sip_message_t *m,
	size_t offset,
	uint8_t *buffer,
	size_t *size
) {
	int retval = -1;
	LinphoneChatMessage *msg = L_GET_C_BACK_PTR(chatMessage);

	if (!isFileTransferInProgressAndValid()) {
		if (httpRequest) {
			/*lWarning() << "Cancelled request for " << (chatRoom->lock() ? "" : "ORPHAN") <<
				" msg [" << this << "], ignoring " << __FUNCTION__;*/
			releaseHttpRequest();
		}
		return BELLE_SIP_STOP;
	}

	// if we've not reach the end of file yet, ask for more data
	// in case of file body handler, won't be called
	if (currentFileContentToTransfer->getFilePath().empty() && offset < currentFileContentToTransfer->getFileSize()) {
		// get data from call back
		LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
		LinphoneChatMessageCbsFileTransferSendCb file_transfer_send_cb =
			linphone_chat_message_cbs_get_file_transfer_send(cbs);
		LinphoneContent *content = currentFileContentToTransfer->toLinphoneContent();
		if (file_transfer_send_cb) {
			LinphoneBuffer *lb = file_transfer_send_cb(msg, content, offset, *size);
			if (lb == nullptr) {
				*size = 0;
			} else {
				*size = linphone_buffer_get_size(lb);
				memcpy(buffer, linphone_buffer_get_content(lb), *size);
				linphone_buffer_unref(lb);
			}
		} else {
			// Legacy
			shared_ptr<Core> core = chatMessage->getCore();
			if (core)
				linphone_core_notify_file_transfer_send(core->getCCore(), msg, content, (char *)buffer, size);
		}
		linphone_content_unref(content);
	}

	LinphoneImEncryptionEngine *imee = nullptr;
	shared_ptr<Core> core = chatMessage->getCore();
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
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->onSendEnd(bh);
}

void FileTransferChatMessageModifier::onSendEnd (belle_sip_user_body_handler_t *bh) {
	LinphoneImEncryptionEngine *imee = nullptr;
	shared_ptr<Core> core = chatMessage->getCore();
	if (core)
		imee = linphone_core_get_im_encryption_engine(core->getCCore());

	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsUploadingFileCb cb_process_uploading_file = linphone_im_encryption_engine_cbs_get_process_uploading_file(imee_cbs);
		if (cb_process_uploading_file) {
			cb_process_uploading_file(imee, L_GET_C_BACK_PTR(chatMessage), 0, nullptr, nullptr, nullptr);
		}
	}
}

static void _chat_message_process_response_from_post_file (void *data, const belle_http_response_event_t *event) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->processResponseFromPostFile(event);
}

void FileTransferChatMessageModifier::processResponseFromPostFile (const belle_http_response_event_t *event) {
	if (httpRequest && !isFileTransferInProgressAndValid()) {
		/*lWarning() << "Cancelled request for " << (chatRoom->lock() ? "" : "ORPHAN") <<
			" msg [" << this << "], ignoring " << __FUNCTION__;*/
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

			shared_ptr<Core> core = chatMessage->getCore();

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
					generate_file_transfer_key_cb(imee, L_GET_C_BACK_PTR(chatRoom), L_GET_C_BACK_PTR(chatMessage));
				}
				// temporary storage for the Content-disposition header value : use a generic filename to not leak it
				// Actual filename stored in msg->file_transfer_information->name will be set in encrypted msg
				// sended to the
				first_part_header = "form-data; name=\"File\"; filename=\"filename.txt\"";
			} else {
				// temporary storage for the Content-disposition header value
				first_part_header = "form-data; name=\"File\"; filename=\"" + currentFileContentToTransfer->getFileName() + "\"";
			}

			// create a user body handler to take care of the file and add the content disposition and content-type headers
			first_part_bh = (belle_sip_body_handler_t *)belle_sip_user_body_handler_new(currentFileContentToTransfer->getFileSize(),
					_chat_message_file_transfer_on_progress, nullptr, nullptr,
					_chat_message_on_send_body, _chat_message_on_send_end, this);
			if (!currentFileContentToTransfer->getFilePath().empty()) {
				belle_sip_user_body_handler_t *body_handler = (belle_sip_user_body_handler_t *)first_part_bh;
				// No need to add again the callback for progression, otherwise it will be called twice
				first_part_bh = (belle_sip_body_handler_t *)belle_sip_file_body_handler_new(currentFileContentToTransfer->getFilePath().c_str(), nullptr, this);
				//linphone_content_set_size(cFileTransferInformation, belle_sip_file_body_handler_get_file_size((belle_sip_file_body_handler_t *)first_part_bh));
				belle_sip_file_body_handler_set_user_body_handler((belle_sip_file_body_handler_t *)first_part_bh, body_handler);
			} else if (!currentFileContentToTransfer->isEmpty()) {
				first_part_bh = (belle_sip_body_handler_t *)belle_sip_memory_body_handler_new_from_buffer(
						ms_strdup(currentFileContentToTransfer->getBodyAsString().c_str()),
						currentFileContentToTransfer->getSize(), _chat_message_file_transfer_on_progress, this);
			}

			belle_sip_body_handler_add_header(first_part_bh,
				belle_sip_header_create("Content-disposition", first_part_header.c_str()));
			belle_sip_body_handler_add_header(first_part_bh,
				(belle_sip_header_t *)belle_sip_header_content_type_create(
					currentFileContentToTransfer->getContentType().getType().c_str(),
					currentFileContentToTransfer->getContentType().getSubType().c_str()));

			// insert it in a multipart body handler which will manage the boundaries of multipart msg
			bh = belle_sip_multipart_body_handler_new(_chat_message_file_transfer_on_progress, this, first_part_bh, nullptr);

			releaseHttpRequest();
			fileUploadBeginBackgroundTask();
			uploadFile();
			belle_sip_message_set_body_handler(BELLE_SIP_MESSAGE(httpRequest), BELLE_SIP_BODY_HANDLER(bh));
		} else if (code == 200) {     // file has been uploaded correctly, get server reply and send it
			const char *body = belle_sip_message_get_body((belle_sip_message_t *)event->response);
			if (body && strlen(body) > 0) {
				//TODO
				// if we have an encryption key for the file, we must insert it into the msg and restore the correct filename
				/*const char *content_key = linphone_content_get_key(cFileTransferInformation);
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
				} else {        // no encryption key, transfer in plain, just copy the msg sent by server
					setText(body);
				}
				setContentType(ContentType::FileTransfer);*/
				FileContent *fileContent = currentFileContentToTransfer;

				FileTransferContent *fileTransferContent = new FileTransferContent();
				fileTransferContent->setContentType(ContentType::FileTransfer);
				fileTransferContent->setFileContent(fileContent);
				fileTransferContent->setBody(body);

				chatMessage->removeContent(*fileContent);
				chatMessage->addContent(*fileTransferContent);

				chatMessage->updateState(ChatMessage::State::FileTransferDone);
				releaseHttpRequest();
				chatMessage->getPrivate()->send();
				fileUploadEndBackgroundTask();
			} else {
				lWarning() << "Received empty response from server, file transfer failed";
				chatMessage->updateState(ChatMessage::State::NotDelivered);
				releaseHttpRequest();
				fileUploadEndBackgroundTask();
			}
		} else {
			lWarning() << "Unhandled HTTP code response " << code << " for file transfer";
			chatMessage->updateState(ChatMessage::State::NotDelivered);
			releaseHttpRequest();
			fileUploadEndBackgroundTask();
		}
	}
}

static void _chat_message_process_io_error_upload (void *data, const belle_sip_io_error_event_t *event) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->processIoErrorUpload(event);
}

void FileTransferChatMessageModifier::processIoErrorUpload (const belle_sip_io_error_event_t *event) {
	lError() << "I/O Error during file upload of msg [" << this << "]";
	chatMessage->updateState(ChatMessage::State::NotDelivered);
	releaseHttpRequest();

	if (chatRoom)
		chatRoom->getPrivate()->removeTransientMessage(chatMessage);
}

static void _chat_message_process_auth_requested_upload (void *data, belle_sip_auth_event *event) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->processAuthRequestedUpload(event);
}

void FileTransferChatMessageModifier::processAuthRequestedUpload (const belle_sip_auth_event *event) {
	lError() << "Error during file upload: auth requested for msg [" << this << "]";
	chatMessage->updateState(ChatMessage::State::NotDelivered);
	releaseHttpRequest();

	if (chatRoom)
		chatRoom->getPrivate()->removeTransientMessage(chatMessage);
}

int FileTransferChatMessageModifier::uploadFile () {
	if (httpRequest) {
		lError() << "linphone_chat_room_upload_file(): there is already an upload in progress.";
		return -1;
	}

	// THIS IS ONLY FOR BACKWARD C API COMPAT
	if (currentFileContentToTransfer->getFilePath().empty() && !chatMessage->getPrivate()->getFileTransferFilepath().empty()) {
		currentFileContentToTransfer->setFilePath(chatMessage->getPrivate()->getFileTransferFilepath());
	}

	belle_http_request_listener_callbacks_t cbs = { 0 };
	cbs.process_response = _chat_message_process_response_from_post_file;
	cbs.process_io_error = _chat_message_process_io_error_upload;
	cbs.process_auth_requested = _chat_message_process_auth_requested_upload;

	int err = startHttpTransfer(linphone_core_get_file_transfer_server(chatMessage->getCore()->getCCore()), "POST", &cbs);
	return err;
}

int FileTransferChatMessageModifier::startHttpTransfer (const string &url, const string &action, belle_http_request_listener_callbacks_t *cbs) {
	belle_generic_uri_t *uri = nullptr;

	shared_ptr<Core> core = chatMessage->getCore();
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

static void _chat_message_file_upload_background_task_ended (void *data) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->fileUploadBackgroundTaskEnded();
}

void FileTransferChatMessageModifier::fileUploadBackgroundTaskEnded () {
	lWarning() << "channel [" << this << "]: file upload background task has to be ended now, but work isn't finished.";
	fileUploadEndBackgroundTask();
}

void FileTransferChatMessageModifier::fileUploadBeginBackgroundTask () {
	if (backgroundTaskId == 0) {
		backgroundTaskId = sal_begin_background_task("file transfer upload", _chat_message_file_upload_background_task_ended, this);
		if (backgroundTaskId) lInfo() << "channel [" << this << "]: starting file upload background task with id=[" << backgroundTaskId << "].";
	}
}

void FileTransferChatMessageModifier::fileUploadEndBackgroundTask () {
	if (backgroundTaskId) {
		lInfo() << "channel [" << this << "]: ending file upload background task with id=[" << backgroundTaskId << "].";
		sal_end_background_task(backgroundTaskId);
		backgroundTaskId = 0;
	}
}

// ----------------------------------------------------------

static void fillFileTransferContentInformationsFromVndGsmaRcsFtHttpXml(FileTransferContent *fileTransferContent) {
	xmlChar *file_url = nullptr;
	xmlDocPtr xmlMessageBody;
	xmlNodePtr cur;
	/* parse the msg body to get all informations from it */
	xmlMessageBody = xmlParseDoc((const xmlChar *)fileTransferContent->getBodyAsString().c_str());

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
						if (!xmlStrcmp(cur->name, (const xmlChar *)"file-name")) {
							xmlChar *filename = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
							fileTransferContent->setFileName((char *)filename);

							xmlFree(filename);
						}
						if (!xmlStrcmp(cur->name, (const xmlChar *)"data")) {
							file_url = xmlGetProp(cur, (const xmlChar *)"url");
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

	fileTransferContent->setFileUrl((const char *)file_url); // Set file url in the file transfer content for the download

	xmlFree(file_url);
}

ChatMessageModifier::Result FileTransferChatMessageModifier::decode (const shared_ptr<ChatMessage> &message, int &errorCode) {
	chatMessage = message;

	Content internalContent = chatMessage->getInternalContent();
	if (internalContent.getContentType() == ContentType::FileTransfer) {
		FileTransferContent *fileTransferContent = new FileTransferContent();
		fileTransferContent->setContentType(internalContent.getContentType());
		fileTransferContent->setBody(internalContent.getBody());
		fillFileTransferContentInformationsFromVndGsmaRcsFtHttpXml(fileTransferContent);
		chatMessage->addContent(*fileTransferContent);
		return ChatMessageModifier::Result::Done;
	} else {
		for (Content *content : chatMessage->getContents()) {
			if (content->getContentType() == ContentType::FileTransfer) {
				FileTransferContent *fileTransferContent = (FileTransferContent *)content;
				fillFileTransferContentInformationsFromVndGsmaRcsFtHttpXml(fileTransferContent);
			}
		}
		return ChatMessageModifier::Result::Done;
	}

	return ChatMessageModifier::Result::Skipped;
}

// ----------------------------------------------------------

static void createFileTransferInformationsFromVndGsmaRcsFtHttpXml (FileTransferContent *fileTransferContent) {
	xmlChar *file_url = nullptr;
	xmlDocPtr xmlMessageBody;
	xmlNodePtr cur;
	/* parse the msg body to get all informations from it */
	xmlMessageBody = xmlParseDoc((const xmlChar *)fileTransferContent->getBodyAsString().c_str());
	FileContent *fileContent = new FileContent();

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
							size_t size = (size_t)strtol((const char *)fileSizeString, nullptr, 10);
							fileContent->setFileSize(size);
							xmlFree(fileSizeString);
						}

						if (!xmlStrcmp(cur->name, (const xmlChar *)"file-name")) {
							xmlChar *filename = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
							fileContent->setFileName((char *)filename);

							xmlFree(filename);
						}
						if (!xmlStrcmp(cur->name, (const xmlChar *)"content-type")) {
							xmlChar *content_type = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
							int contentTypeIndex = 0;
							char *type;
							char *subtype;
							while (content_type[contentTypeIndex] != '/' && content_type[contentTypeIndex] != '\0') {
								contentTypeIndex++;
							}
							type = ms_strndup((char *)content_type, contentTypeIndex);
							subtype = ms_strdup(((char *)content_type + contentTypeIndex + 1));
							ContentType contentType(type, subtype);
							fileContent->setContentType(contentType);
							ms_free(subtype);
							ms_free(type);
							ms_free(content_type);
						}
						if (!xmlStrcmp(cur->name, (const xmlChar *)"data")) {
							file_url = xmlGetProp(cur, (const xmlChar *)"url");
						}

						//TODO
						/*if (!xmlStrcmp(cur->name, (const xmlChar *)"file-key")) {
							// there is a key in the msg: file has been encrypted
							// convert the key from base 64
							xmlChar *keyb64 = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
							size_t keyLength = b64::b64_decode((char *)keyb64, strlen((char *)keyb64), nullptr, 0);
							uint8_t *keyBuffer = (uint8_t *)malloc(keyLength);
							// decode the key into local key buffer
							b64::b64_decode((char *)keyb64, strlen((char *)keyb64), keyBuffer, keyLength);
							linphone_content_set_key(content, (char *)keyBuffer, keyLength);
							// duplicate key value into the linphone content private structure
							xmlFree(keyb64);
							free(keyBuffer);
						}*/

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

	fileContent->setFilePath(fileTransferContent->getFilePath()); // Copy file path from file transfer content to file content for file body handler
	// Link the FileContent to the FileTransferContent
	fileTransferContent->setFileContent(fileContent);

	xmlFree(file_url);
}

static void _chat_message_on_recv_body (belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, void *data, size_t offset, uint8_t *buffer, size_t size) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->onRecvBody(bh, m, offset, buffer, size);
}

void FileTransferChatMessageModifier::onRecvBody (belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, size_t offset, uint8_t *buffer, size_t size) {
	int retval = -1;
	uint8_t *decrypted_buffer = nullptr;

	/*if (!chatRoom.lock()) {
		q->cancelFileTransfer();
		return;
	}*/

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
	shared_ptr<Core> core = chatMessage->getCore();
	if (core)
		imee = linphone_core_get_im_encryption_engine(core->getCCore());

	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsDownloadingFileCb cb_process_downloading_file = linphone_im_encryption_engine_cbs_get_process_downloading_file(imee_cbs);
		if (cb_process_downloading_file) {
			retval = cb_process_downloading_file(imee, L_GET_C_BACK_PTR(chatMessage), offset, (const uint8_t *)buffer, size, decrypted_buffer);
			if (retval == 0) {
				memcpy(buffer, decrypted_buffer, size);
			}
		}
	}
	ms_free(decrypted_buffer);

	if (retval <= 0) {
		if (currentFileContentToTransfer->getFilePath().empty()) {
			LinphoneChatMessage *msg = L_GET_C_BACK_PTR(chatMessage);
			LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
			LinphoneContent *content = currentFileContentToTransfer->toLinphoneContent();
			if (linphone_chat_message_cbs_get_file_transfer_recv(cbs)) {
				LinphoneBuffer *lb = linphone_buffer_new_from_data(buffer, size);
				linphone_chat_message_cbs_get_file_transfer_recv(cbs)(msg, content, lb);
				linphone_buffer_unref(lb);
			} else {
				// Legacy: call back given by application level
				linphone_core_notify_file_transfer_recv(core->getCCore(), msg, content, (const char *)buffer, size);
			}
			linphone_content_unref(content);
		}
	} else {
		lWarning() << "File transfer decrypt failed with code " << (int)retval;
		chatMessage->getPrivate()->setState(ChatMessage::State::FileTransferError);
	}
}

static void _chat_message_on_recv_end (belle_sip_user_body_handler_t *bh, void *data) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->onRecvEnd(bh);
}

void FileTransferChatMessageModifier::onRecvEnd (belle_sip_user_body_handler_t *bh) {
	shared_ptr<Core> core = chatMessage->getCore();
	if (!core)
		return;

	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(core->getCCore());
	int retval = -1;

	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsDownloadingFileCb cb_process_downloading_file = linphone_im_encryption_engine_cbs_get_process_downloading_file(imee_cbs);
		if (cb_process_downloading_file) {
			retval = cb_process_downloading_file(imee, L_GET_C_BACK_PTR(chatMessage), 0, nullptr, 0, nullptr);
		}
	}

	if (retval <= 0) {
		if (currentFileContentToTransfer->getFilePath().empty()) {
			LinphoneChatMessage *msg = L_GET_C_BACK_PTR(chatMessage);
			LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
			LinphoneContent *content = currentFileContentToTransfer->toLinphoneContent();
			if (linphone_chat_message_cbs_get_file_transfer_recv(cbs)) {
				LinphoneBuffer *lb = linphone_buffer_new();
				linphone_chat_message_cbs_get_file_transfer_recv(cbs)(msg, content, lb);
				linphone_buffer_unref(lb);
			} else {
				// Legacy: call back given by application level
				linphone_core_notify_file_transfer_recv(core->getCCore(), msg, content, nullptr, 0);
			}
			linphone_content_unref(content);
		}
	}

	if (retval <= 0 && chatMessage->getState() != ChatMessage::State::FileTransferError) {
		// Remove the FileTransferContent from the message and store the FileContent
		FileContent *fileContent = currentFileContentToTransfer;
		chatMessage->addContent(*fileContent);
		for (Content *content : chatMessage->getContents()) {
			if (content->getContentType() == ContentType::FileTransfer) {
				FileTransferContent *fileTransferContent = (FileTransferContent*)content;
				if (fileTransferContent->getFileContent() == fileContent) {
					chatMessage->removeContent(*content);
					free(fileTransferContent);
					break;
				}
			}
		}
		chatMessage->getPrivate()->setState(ChatMessage::State::FileTransferDone);
	}
}

static void _chat_process_response_headers_from_get_file (void *data, const belle_http_response_event_t *event) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->processResponseHeadersFromGetFile(event);
}

static FileContent* createFileTransferInformationFromHeaders (const belle_sip_message_t *m) {
	FileContent *fileContent = new FileContent();

	belle_sip_header_content_length_t *content_length_hdr = BELLE_SIP_HEADER_CONTENT_LENGTH(belle_sip_message_get_header(m, "Content-Length"));
	belle_sip_header_content_type_t *content_type_hdr = BELLE_SIP_HEADER_CONTENT_TYPE(belle_sip_message_get_header(m, "Content-Type"));
	const char *type = nullptr, *subtype = nullptr;

	if (content_type_hdr) {
		type = belle_sip_header_content_type_get_type(content_type_hdr);
		subtype = belle_sip_header_content_type_get_subtype(content_type_hdr);
		lInfo() << "Extracted content type " << type << " / " << subtype << " from header";
		ContentType contentType(type, subtype);
		fileContent->setContentType(contentType);
	}
	if (content_length_hdr) {
		fileContent->setFileSize(belle_sip_header_content_length_get_content_length(content_length_hdr));
		lInfo() << "Extracted content length " << fileContent->getFileSize() << " from header";
	}

	return fileContent;
}

void FileTransferChatMessageModifier::processResponseHeadersFromGetFile (const belle_http_response_event_t *event) {
	if (event->response) {
		// we are receiving a response, set a specific body handler to acquire the response.
		// if not done, belle-sip will create a memory body handler, the default
		belle_sip_message_t *response = BELLE_SIP_MESSAGE(event->response);
		belle_sip_body_handler_t *body_handler = nullptr;
		size_t body_size = 0;

		if (currentFileContentToTransfer == nullptr) {
			lWarning() << "No file transfer information for msg [" << this << "]: creating...";
			FileContent *content = createFileTransferInformationFromHeaders(response);
			chatMessage->addContent(*content);
		} else {
			belle_sip_header_content_length_t *content_length_hdr = BELLE_SIP_HEADER_CONTENT_LENGTH(belle_sip_message_get_header(response, "Content-Length"));
			currentFileContentToTransfer->setFileSize(belle_sip_header_content_length_get_content_length(content_length_hdr));
			lInfo() << "Extracted content length " << currentFileContentToTransfer->getFileSize() << " from header";
		}

		if (currentFileContentToTransfer) {
			body_size = currentFileContentToTransfer->getFileSize();
		}

		body_handler = (belle_sip_body_handler_t *)belle_sip_user_body_handler_new(body_size, _chat_message_file_transfer_on_progress,
				nullptr, _chat_message_on_recv_body,
				nullptr, _chat_message_on_recv_end, this);
		if (!currentFileContentToTransfer->getFilePath().empty()) {
			belle_sip_user_body_handler_t *bh = (belle_sip_user_body_handler_t *)body_handler;
			body_handler = (belle_sip_body_handler_t *)belle_sip_file_body_handler_new(currentFileContentToTransfer->getFilePath().c_str(), _chat_message_file_transfer_on_progress, this);
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
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->processAuthRequestedDownload(event);
}

void FileTransferChatMessageModifier::processAuthRequestedDownload (const belle_sip_auth_event *event) {
	lError() << "Error during file download : auth requested for msg [" << this << "]";
	chatMessage->updateState(ChatMessage::State::FileTransferError);
	releaseHttpRequest();
}

static void _chat_message_process_io_error_download (void *data, const belle_sip_io_error_event_t *event) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->processIoErrorDownload(event);
}

void FileTransferChatMessageModifier::processIoErrorDownload (const belle_sip_io_error_event_t *event) {
	lError() << "I/O Error during file download msg [" << this << "]";
	chatMessage->updateState(ChatMessage::State::FileTransferError);
	releaseHttpRequest();
}

static void _chat_message_process_response_from_get_file (void *data, const belle_http_response_event_t *event) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->processResponseFromGetFile(event);
}

void FileTransferChatMessageModifier::processResponseFromGetFile (const belle_http_response_event_t *event) {
	// check the answer code
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code >= 400 && code < 500) {
			lWarning() << "File transfer failed with code " << code;
			chatMessage->getPrivate()->setState(ChatMessage::State::FileTransferError);
		} else if (code != 200) {
			lWarning() << "Unhandled HTTP code response " << code << " for file transfer";
		}
		releaseHttpRequest();
	}
}

int FileTransferChatMessageModifier::downloadFile(const shared_ptr<ChatMessage> &message, FileTransferContent *fileTransferContent) {
	chatMessage = message;

	if (httpRequest) {
		lError() << "linphone_chat_message_download_file(): there is already a download in progress";
		return -1;
	}

	if (fileTransferContent->getContentType() != ContentType::FileTransfer) {
		lError() << "linphone_chat_message_download_file(): content type is not FileTransfer";
		return -1;
	}

	createFileTransferInformationsFromVndGsmaRcsFtHttpXml(fileTransferContent);
	FileContent *fileContent = fileTransferContent->getFileContent();
	currentFileContentToTransfer = fileContent;
	if (currentFileContentToTransfer == nullptr) {
		return -1;
	}

	// THIS IS ONLY FOR BACKWARD C API COMPAT
	if (currentFileContentToTransfer->getFilePath().empty() && !chatMessage->getPrivate()->getFileTransferFilepath().empty()) {
		currentFileContentToTransfer->setFilePath(chatMessage->getPrivate()->getFileTransferFilepath());
	}

	belle_http_request_listener_callbacks_t cbs = { 0 };
	cbs.process_response_headers = _chat_process_response_headers_from_get_file;
	cbs.process_response = _chat_message_process_response_from_get_file;
	cbs.process_io_error = _chat_message_process_io_error_download;
	cbs.process_auth_requested = _chat_message_process_auth_requested_download;
	int err = startHttpTransfer(fileTransferContent->getFileUrl(), "GET", &cbs); // File URL has been set by createFileTransferInformationsFromVndGsmaRcsFtHttpXml

	if (err == -1) return -1;
	// start the download, status is In Progress
	chatMessage->getPrivate()->setState(ChatMessage::State::InProgress);
	return 0;
}

// ----------------------------------------------------------

void FileTransferChatMessageModifier::cancelFileTransfer () {
	if (httpRequest) {
		if (!belle_http_request_is_cancelled(httpRequest)) {
			if (chatRoom) {
				shared_ptr<Core> core = chatRoom->getCore();
				lInfo() << "Canceling file transfer " << (
					currentFileContentToTransfer->getFilePath().empty()
						? linphone_core_get_file_transfer_server(core->getCCore())
						: currentFileContentToTransfer->getFilePath().c_str()
					);
				belle_http_provider_cancel_request(core->getCCore()->http_provider, httpRequest);
			} else {
				lInfo() << "Warning: http request still running for ORPHAN msg: this is a memory leak";
			}
		}
		releaseHttpRequest();
	} else {
		lInfo() << "No existing file transfer - nothing to cancel";
	}
}

bool FileTransferChatMessageModifier::isFileTransferInProgressAndValid () {
	return httpRequest && !belle_http_request_is_cancelled(httpRequest);
}

void FileTransferChatMessageModifier::releaseHttpRequest () {
	if (httpRequest) {
		belle_sip_object_unref(httpRequest);
		httpRequest = nullptr;
		if (httpListener) {
			belle_sip_object_unref(httpListener);
			httpListener = nullptr;
		}
	}
}

LINPHONE_END_NAMESPACE
