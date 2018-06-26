/*
 * file-transfer-chat-message-modifier.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
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

#include "linphone/api/c-content.h"

#include "address/address.h"
#include "bctoolbox/crypto.h"
#include "c-wrapper/c-wrapper.h"
#include "chat/chat-message/chat-message-p.h"
#include "chat/chat-room/chat-room-p.h"
#include "content/content-type.h"
#include "content/content.h"
#include "core/core.h"
#include "logger/logger.h"

#include "file-transfer-chat-message-modifier.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

FileTransferChatMessageModifier::FileTransferChatMessageModifier (belle_http_provider_t *prov) : provider(prov) {
	bgTask.setName("File transfer upload");
}

belle_http_request_t *FileTransferChatMessageModifier::getHttpRequest () const {
	return httpRequest;
}

void FileTransferChatMessageModifier::setHttpRequest (belle_http_request_t *request) {
	httpRequest = request;
}

FileTransferChatMessageModifier::~FileTransferChatMessageModifier () {
	if (isFileTransferInProgressAndValid())
		cancelFileTransfer(); //to avoid body handler to still refference zombie FileTransferChatMessageModifier
	else
		releaseHttpRequest();
}

ChatMessageModifier::Result FileTransferChatMessageModifier::encode (const shared_ptr<ChatMessage> &message, int &errorCode) {
	chatMessage = message;

	currentFileContentToTransfer = nullptr;
	// For each FileContent, upload it and create a FileTransferContent
	for (Content *content : message->getContents()) {
		if (content->isFile()) {
				lInfo() << "Found file content, set it for file upload";
				FileContent *fileContent = (FileContent *)content;
				currentFileContentToTransfer = fileContent;
				break;
		}
	}
	if (!currentFileContentToTransfer)
		return ChatMessageModifier::Result::Skipped;

	/* Open a transaction with the server and send an empty request(RCS5.1 section 3.5.4.8.3.1) */
	if (uploadFile() == 0)
		return ChatMessageModifier::Result::Suspended;

	return ChatMessageModifier::Result::Error;
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
		releaseHttpRequest();
		return;
	}

	shared_ptr<ChatMessage> message = chatMessage.lock();
	if (!message)
		return;

	LinphoneChatMessage *msg = L_GET_C_BACK_PTR(message);
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
	LinphoneContent *content = L_GET_C_BACK_PTR((Content *)currentFileContentToTransfer);
	if (linphone_chat_message_cbs_get_file_transfer_progress_indication(cbs)) {
		linphone_chat_message_cbs_get_file_transfer_progress_indication(cbs)(msg, content, offset, total);
	} else {
		// Legacy: call back given by application level.
		linphone_core_notify_file_transfer_progress_indication(message->getCore()->getCCore(), msg, content, offset, total);
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
	shared_ptr<ChatMessage> message = chatMessage.lock();
	if (!message)
		return BELLE_SIP_STOP;

	LinphoneChatMessage *msg = L_GET_C_BACK_PTR(message);

	if (!isFileTransferInProgressAndValid()) {
		if (httpRequest) {
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
		LinphoneContent *content = L_GET_C_BACK_PTR((Content *)currentFileContentToTransfer);
		if (file_transfer_send_cb) {
			LinphoneBuffer *lb = file_transfer_send_cb(msg, content, offset, *size);
			if (lb) {
				*size = linphone_buffer_get_size(lb);
				memcpy(buffer, linphone_buffer_get_content(lb), *size);
				linphone_buffer_unref(lb);
			} else {
				*size = 0;
			}
		} else {
			// Legacy
			linphone_core_notify_file_transfer_send(message->getCore()->getCCore(), msg, content, (char *)buffer, size);
		}
	}

	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(message->getCore()->getCCore());
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
	shared_ptr<ChatMessage> message = chatMessage.lock();
	if (!message)
		return;

	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(message->getCore()->getCCore());
	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsUploadingFileCb cb_process_uploading_file = linphone_im_encryption_engine_cbs_get_process_uploading_file(imee_cbs);
		if (cb_process_uploading_file) {
			cb_process_uploading_file(imee, L_GET_C_BACK_PTR(message), 0, nullptr, nullptr, nullptr);
		}
	}
}

static void _chat_message_process_response_from_post_file (void *data, const belle_http_response_event_t *event) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->processResponseFromPostFile(event);
}

void FileTransferChatMessageModifier::processResponseFromPostFile (const belle_http_response_event_t *event) {
	if (httpRequest && !isFileTransferInProgressAndValid()) {
		releaseHttpRequest();
		return;
	}

	shared_ptr<ChatMessage> message = chatMessage.lock();
	if (!message)
		return;

	// check the answer code
	if (event->response) {
		int code = belle_http_response_get_status_code(event->response);
		if (code == 204) { // this is the reply to the first post to the server - an empty msg
			// start uploading the file
			belle_sip_multipart_body_handler_t *bh;
			string first_part_header;
			belle_sip_body_handler_t *first_part_bh;

			bool is_file_encryption_enabled = false;
			LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(message->getCore()->getCCore());
			if (imee && message->getChatRoom()) {
				LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb is_encryption_enabled_for_file_transfer_cb =
					linphone_im_encryption_engine_cbs_get_is_encryption_enabled_for_file_transfer(imee_cbs);
				if (is_encryption_enabled_for_file_transfer_cb) {
					is_file_encryption_enabled = !!is_encryption_enabled_for_file_transfer_cb(imee, L_GET_C_BACK_PTR(message->getChatRoom()));
				}
			}

			FileTransferContent *fileTransferContent = new FileTransferContent();
			fileTransferContent->setContentType(ContentType::FileTransfer);
			fileTransferContent->setFileSize(currentFileContentToTransfer->getFileSize()); // Copy file size information
			message->getPrivate()->addContent(fileTransferContent);

			// shall we encrypt the file
			if (is_file_encryption_enabled && message->getChatRoom()) {
				// temporary storage for the Content-disposition header value : use a generic filename to not leak it
				// Actual filename stored in msg->file_transfer_information->name will be set in encrypted msg
				// sended to the
				first_part_header = "form-data; name=\"File\"; filename=\"filename.txt\"";

				LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
				LinphoneImEncryptionEngineCbsGenerateFileTransferKeyCb generate_file_transfer_key_cb =
					linphone_im_encryption_engine_cbs_get_generate_file_transfer_key(imee_cbs);
				if (generate_file_transfer_key_cb) {
					generate_file_transfer_key_cb(imee, L_GET_C_BACK_PTR(message->getChatRoom()), L_GET_C_BACK_PTR(message));
				}
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
				belle_sip_file_body_handler_set_user_body_handler((belle_sip_file_body_handler_t *)first_part_bh, body_handler);
				// Ensure the file size has been set to the correct value
				fileTransferContent->setFileSize(belle_sip_file_body_handler_get_file_size((belle_sip_file_body_handler_t *)first_part_bh));
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
			FileTransferContent *fileTransferContent = nullptr;
			for (Content *c : message->getPrivate()->getContents()) {
				if (c->isFileTransfer()) {
					FileTransferContent *tmpContent = static_cast<FileTransferContent *>(c);
					if (!tmpContent->getFileContent() && tmpContent->getSize() == 0) {
						// If FileTransferContent doesn't have a FileContent yet and is empty
						// It's the one we seek, otherwise it may be a previous uploaded FileTransferContent
						fileTransferContent = tmpContent;
						break;
					}
				}
			}

			const char *body = belle_sip_message_get_body((belle_sip_message_t *)event->response);
			if (body && strlen(body) > 0) {
				// if we have an encryption key for the file, we must insert it into the msg and restore the correct filename
				const unsigned char *contentKey = reinterpret_cast<const unsigned char *>(fileTransferContent->getFileKey().data());
				size_t contentKeySize = fileTransferContent->getFileKeySize();
				if (contentKeySize > 0) {
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
									size_t b64Size;
									bctbx_base64_encode(nullptr, &b64Size, contentKey, contentKeySize);
									unsigned char *keyb64 = (unsigned char *)ms_malloc0(b64Size + 1);
									int xmlStringLength;

									bctbx_base64_encode(keyb64, &b64Size, contentKey, contentKeySize);
									keyb64[b64Size] = '\0'; // libxml need a null terminated string

									// add the node containing the key to the file-info node
									xmlNewTextChild(cur, nullptr, (const xmlChar *)"file-key", (const xmlChar *)keyb64);
									xmlFree(typeAttribute);
									ms_free(keyb64);

									// look for the file-name node and update its content
									while (fileInfoNodeChildren != nullptr) {
										// we found a the file-name node, update its content with the real filename
										if (!xmlStrcmp(fileInfoNodeChildren->name, (const xmlChar *)"file-name")) {
											// update node content
											xmlNodeSetContent(fileInfoNodeChildren, (const xmlChar *)(currentFileContentToTransfer->getFileName().c_str()));
											break;
										}
										fileInfoNodeChildren = fileInfoNodeChildren->next;
									}

									// dump the xml into msg->message
									char *buffer;
									xmlDocDumpFormatMemoryEnc(xmlMessageBody, (xmlChar **)&buffer, &xmlStringLength, "UTF-8", 0);
									fileTransferContent->setBody(buffer);
									break;
								}
								xmlFree(typeAttribute);
							}
							cur = cur->next;
						}
					}
					xmlFreeDoc(xmlMessageBody);
				} else { // no encryption key, transfer in plain, just copy the msg sent by server
					fileTransferContent->setBody(body);
				}

				FileContent *fileContent = currentFileContentToTransfer;
				fileTransferContent->setFileContent(fileContent);

				message->getPrivate()->removeContent(fileContent);

				message->getPrivate()->setState(ChatMessage::State::FileTransferDone);
				releaseHttpRequest();
				message->getPrivate()->send();
				fileUploadEndBackgroundTask();
			} else {
				lWarning() << "Received empty response from server, file transfer failed";
				FileTransferContent *fileTransferContent = nullptr;
				for (Content *c : message->getPrivate()->getContents()) {
					if (c->isFileTransfer()) {
						fileTransferContent = static_cast<FileTransferContent *>(c);
						message->getPrivate()->removeContent(fileTransferContent);
						delete fileTransferContent;
						break;
					}
				}
				message->getPrivate()->setState(ChatMessage::State::NotDelivered);
				releaseHttpRequest();
				fileUploadEndBackgroundTask();
			}
		} else if (code == 400) {
			lWarning() << "Received HTTP code response " << code << " for file transfer, probably meaning file is too large";
			FileTransferContent *fileTransferContent = nullptr;
			for (Content *c : message->getPrivate()->getContents()) {
				if (c->isFileTransfer()) {
					fileTransferContent = static_cast<FileTransferContent *>(c);
					message->getPrivate()->removeContent(fileTransferContent);
					delete fileTransferContent;
					break;
				}
			}
			message->getPrivate()->setState(ChatMessage::State::FileTransferError);
			releaseHttpRequest();
			fileUploadEndBackgroundTask();
		} else {
			lWarning() << "Unhandled HTTP code response " << code << " for file transfer";
			FileTransferContent *fileTransferContent = nullptr;
			for (Content *c : message->getPrivate()->getContents()) {
				if (c->isFileTransfer()) {
					fileTransferContent = static_cast<FileTransferContent *>(c);
					message->getPrivate()->removeContent(fileTransferContent);
					delete fileTransferContent;
					break;
				}
			}
			message->getPrivate()->setState(ChatMessage::State::NotDelivered);
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
	shared_ptr<ChatMessage> message = chatMessage.lock();
	if (!message)
		return;
	message->getPrivate()->setState(ChatMessage::State::NotDelivered);
	releaseHttpRequest();
}

static void _chat_message_process_auth_requested_upload (void *data, belle_sip_auth_event *event) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->processAuthRequestedUpload(event);
}

void FileTransferChatMessageModifier::processAuthRequestedUpload (const belle_sip_auth_event *event) {
	lError() << "Error during file upload: auth requested for msg [" << this << "]";
	shared_ptr<ChatMessage> message = chatMessage.lock();
	if (!message)
		return;
	message->getPrivate()->setState(ChatMessage::State::NotDelivered);
	releaseHttpRequest();
}

int FileTransferChatMessageModifier::uploadFile () {
	if (httpRequest) {
		lError() << "Unable to upload file: there is already an upload in progress.";
		return -1;
	}

	shared_ptr<ChatMessage> message = chatMessage.lock();
	if (!message)
		return -1;

	// THIS IS ONLY FOR BACKWARD C API COMPAT
	if (currentFileContentToTransfer->getFilePath().empty() && !message->getPrivate()->getFileTransferFilepath().empty()) {
		currentFileContentToTransfer->setFilePath(message->getPrivate()->getFileTransferFilepath());
	}

	belle_http_request_listener_callbacks_t cbs = { 0 };
	cbs.process_response = _chat_message_process_response_from_post_file;
	cbs.process_io_error = _chat_message_process_io_error_upload;
	cbs.process_auth_requested = _chat_message_process_auth_requested_upload;

	const char *url = linphone_core_get_file_transfer_server(message->getCore()->getCCore());
	return startHttpTransfer(url ? url : "", "POST", &cbs);
}

int FileTransferChatMessageModifier::startHttpTransfer (const string &url, const string &action, belle_http_request_listener_callbacks_t *cbs) {
	belle_generic_uri_t *uri = nullptr;

	shared_ptr<ChatMessage> message = chatMessage.lock();
	if (!message)
		return -1;

	if (url.empty()) {
		lWarning() << "Cannot process file transfer msg [" << this << "]: no file remote URI configured.";
		goto error;
	}
	uri = belle_generic_uri_parse(url.c_str());
	if (!uri || !belle_generic_uri_get_host(uri)) {
		lWarning() << "Cannot process file transfer msg [" << this << "]: incorrect file remote URI configured '" <<
			url << "'.";
		goto error;
	}

	httpRequest = belle_http_request_create(
		action.c_str(),
		uri,
		belle_sip_header_create("User-Agent", linphone_core_get_user_agent(message->getCore()->getCCore())),
		nullptr
	);

	if (!httpRequest) {
		lWarning() << "Could not create http request for uri " << url;
		goto error;
	}
	// keep a reference to the http request to be able to cancel it during upload
	belle_sip_object_ref(httpRequest);

	// give msg to listener to be able to start the actual file upload when server answer a 204 No content
	httpListener = belle_http_request_listener_create_from_callbacks(cbs, this);
	belle_http_provider_send_request(provider, httpRequest, httpListener);
	return 0;

error:
	if (uri) {
		belle_sip_object_unref(uri);
	}
	return -1;
}

void FileTransferChatMessageModifier::fileUploadBeginBackgroundTask () {
	shared_ptr<ChatMessage> message = chatMessage.lock();
	if (!message)
		return;
	bgTask.start(message->getCore());
}

void FileTransferChatMessageModifier::fileUploadEndBackgroundTask () {
	bgTask.stop();
}

// ----------------------------------------------------------

static void fillFileTransferContentInformationsFromVndGsmaRcsFtHttpXml (FileTransferContent *fileTransferContent) {
	xmlChar *fileUrl = nullptr;
	xmlDocPtr xmlMessageBody;
	xmlNodePtr cur;
	/* parse the msg body to get all informations from it */
	xmlMessageBody = xmlParseDoc((const xmlChar *)fileTransferContent->getBodyAsString().c_str());

	cur = xmlDocGetRootElement(xmlMessageBody);
	if (cur) {
		cur = cur->xmlChildrenNode;
		while (cur) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"file-info")) {
				/* we found a file info node, check if it has a type="file" attribute */
				xmlChar *typeAttribute = xmlGetProp(cur, (const xmlChar *)"type");
				if (!xmlStrcmp(typeAttribute, (const xmlChar *)"file")) {         /* this is the node we are looking for */
					cur = cur->xmlChildrenNode;           /* now loop on the content of the file-info node */
					while (cur) {
						if (!xmlStrcmp(cur->name, (const xmlChar *)"file-size")) {
							xmlChar *fileSizeString = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
							size_t size = (size_t)strtol((const char *)fileSizeString, nullptr, 10);
							fileTransferContent->setFileSize(size);
							xmlFree(fileSizeString);
						}
						if (!xmlStrcmp(cur->name, (const xmlChar *)"file-name")) {
							xmlChar *filename = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
							fileTransferContent->setFileName((char *)filename);
							xmlFree(filename);
						}
						if (!xmlStrcmp(cur->name, (const xmlChar *)"data")) {
							fileUrl = xmlGetProp(cur, (const xmlChar *)"url");
						}
						if (!xmlStrcmp(cur->name, (const xmlChar *)"file-key")) {
							// there is a key in the msg: file has been encrypted
							// convert the key from base 64
							xmlChar *keyb64 = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
							size_t keyLength;
							bctbx_base64_decode(NULL, &keyLength, (unsigned char *)keyb64, strlen((const char *)keyb64));
							uint8_t *keyBuffer = (uint8_t *)malloc(keyLength);
							// decode the key into local key buffer
							bctbx_base64_decode(keyBuffer, &keyLength, (unsigned char *)keyb64, strlen((const char *)keyb64));
							fileTransferContent->setFileKey((const char *)keyBuffer, keyLength);
							// duplicate key value into the linphone content private structure
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

	fileTransferContent->setFileUrl(fileUrl ? (const char *)fileUrl : "");

	xmlFree(fileUrl);
}

ChatMessageModifier::Result FileTransferChatMessageModifier::decode (const shared_ptr<ChatMessage> &message, int &errorCode) {
	chatMessage = message;

	Content internalContent = message->getInternalContent();
	if (internalContent.getContentType() == ContentType::FileTransfer) {
		FileTransferContent *fileTransferContent = new FileTransferContent();
		fileTransferContent->setContentType(internalContent.getContentType());
		fileTransferContent->setBody(internalContent.getBody());
		fillFileTransferContentInformationsFromVndGsmaRcsFtHttpXml(fileTransferContent);
		message->addContent(fileTransferContent);
		return ChatMessageModifier::Result::Done;
	}

	for (Content *content : message->getContents()) {
		if (content->isFileTransfer()) {
			FileTransferContent *fileTransferContent = static_cast<FileTransferContent *>(content);
			fillFileTransferContentInformationsFromVndGsmaRcsFtHttpXml(fileTransferContent);
		}
	}
	return ChatMessageModifier::Result::Done;
}

// ----------------------------------------------------------

static void createFileTransferInformationsFromVndGsmaRcsFtHttpXml (FileTransferContent *fileTransferContent) {
	xmlChar *fileUrl = nullptr;
	xmlDocPtr xmlMessageBody;
	xmlNodePtr cur;
	/* parse the msg body to get all informations from it */
	xmlMessageBody = xmlParseDoc((const xmlChar *)fileTransferContent->getBodyAsString().c_str());
	FileContent *fileContent = new FileContent();

	cur = xmlDocGetRootElement(xmlMessageBody);
	if (cur) {
		cur = cur->xmlChildrenNode;
		while (cur) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"file-info")) {
				/* we found a file info node, check if it has a type="file" attribute */
				xmlChar *typeAttribute = xmlGetProp(cur, (const xmlChar *)"type");
				if (!xmlStrcmp(typeAttribute, (const xmlChar *)"file")) {         /* this is the node we are looking for */
					cur = cur->xmlChildrenNode;           /* now loop on the content of the file-info node */
					while (cur) {
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
							fileUrl = xmlGetProp(cur, (const xmlChar *)"url");
						}

						if (!xmlStrcmp(cur->name, (const xmlChar *)"file-key")) {
							// there is a key in the msg: file has been encrypted
							// convert the key from base 64
							xmlChar *keyb64 = xmlNodeListGetString(xmlMessageBody, cur->xmlChildrenNode, 1);
							size_t keyLength;
							bctbx_base64_decode(NULL, &keyLength, (unsigned char *)keyb64, strlen((const char *)keyb64));
							uint8_t *keyBuffer = (uint8_t *)malloc(keyLength);
							// decode the key into local key buffer
							bctbx_base64_decode(keyBuffer, &keyLength, (unsigned char *)keyb64, strlen((const char *)keyb64));
							fileTransferContent->setFileKey((const char *)keyBuffer, keyLength);
							// duplicate key value into the linphone content private structure
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

	fileContent->setFilePath(fileTransferContent->getFilePath()); // Copy file path from file transfer content to file content for file body handler
	// Link the FileContent to the FileTransferContent
	fileTransferContent->setFileContent(fileContent);

	xmlFree(fileUrl);
}

static void _chat_message_on_recv_body (belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, void *data, size_t offset, uint8_t *buffer, size_t size) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->onRecvBody(bh, m, offset, buffer, size);
}

void FileTransferChatMessageModifier::onRecvBody (belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, size_t offset, uint8_t *buffer, size_t size) {
	if (!httpRequest || belle_http_request_is_cancelled(httpRequest)) {
		lWarning() << "Cancelled request for msg [" << this << "], ignoring " << __FUNCTION__;
		return;
	}

	// first call may be with a zero size, ignore it
	if (size == 0) {
		return;
	}

	shared_ptr<ChatMessage> message = chatMessage.lock();
	if (!message)
		return;

	int retval = -1;
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(message->getCore()->getCCore());
	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsDownloadingFileCb cb_process_downloading_file = linphone_im_encryption_engine_cbs_get_process_downloading_file(imee_cbs);
		if (cb_process_downloading_file) {
			uint8_t *decrypted_buffer = (uint8_t *)ms_malloc0(size);
			retval = cb_process_downloading_file(imee, L_GET_C_BACK_PTR(message), offset, (const uint8_t *)buffer, size, decrypted_buffer);
			if (retval == 0) {
				memcpy(buffer, decrypted_buffer, size);
			}
			ms_free(decrypted_buffer);
		}
	}

	if (retval <= 0) {
		if (currentFileContentToTransfer->getFilePath().empty()) {
			LinphoneChatMessage *msg = L_GET_C_BACK_PTR(message);
			LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
			LinphoneContent *content = L_GET_C_BACK_PTR((Content *)currentFileContentToTransfer);
			if (linphone_chat_message_cbs_get_file_transfer_recv(cbs)) {
				LinphoneBuffer *lb = linphone_buffer_new_from_data(buffer, size);
				linphone_chat_message_cbs_get_file_transfer_recv(cbs)(msg, content, lb);
				linphone_buffer_unref(lb);
			} else {
				// Legacy: call back given by application level
				linphone_core_notify_file_transfer_recv(message->getCore()->getCCore(), msg, content, (const char *)buffer, size);
			}
		}
	} else {
		lWarning() << "File transfer decrypt failed with code " << (int)retval;
		message->getPrivate()->setState(ChatMessage::State::FileTransferError);
	}
}

static void _chat_message_on_recv_end (belle_sip_user_body_handler_t *bh, void *data) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->onRecvEnd(bh);
}

void FileTransferChatMessageModifier::onRecvEnd (belle_sip_user_body_handler_t *bh) {
	shared_ptr<ChatMessage> message = chatMessage.lock();
	if (!message)
		return;

	int retval = -1;
	LinphoneImEncryptionEngine *imee = linphone_core_get_im_encryption_engine(message->getCore()->getCCore());
	if (imee) {
		LinphoneImEncryptionEngineCbs *imee_cbs = linphone_im_encryption_engine_get_callbacks(imee);
		LinphoneImEncryptionEngineCbsDownloadingFileCb cb_process_downloading_file = linphone_im_encryption_engine_cbs_get_process_downloading_file(imee_cbs);
		if (cb_process_downloading_file) {
			retval = cb_process_downloading_file(imee, L_GET_C_BACK_PTR(message), 0, nullptr, 0, nullptr);
		}
	}

	if (retval <= 0) {
		if (currentFileContentToTransfer->getFilePath().empty()) {
			LinphoneChatMessage *msg = L_GET_C_BACK_PTR(message);
			LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
			LinphoneContent *content = L_GET_C_BACK_PTR((Content *)currentFileContentToTransfer);
			if (linphone_chat_message_cbs_get_file_transfer_recv(cbs)) {
				LinphoneBuffer *lb = linphone_buffer_new();
				linphone_chat_message_cbs_get_file_transfer_recv(cbs)(msg, content, lb);
				linphone_buffer_unref(lb);
			} else {
				// Legacy: call back given by application level
				linphone_core_notify_file_transfer_recv(message->getCore()->getCCore(), msg, content, nullptr, 0);
			}
		}
	}

	if (retval <= 0 && message->getState() != ChatMessage::State::FileTransferError) {
		// Remove the FileTransferContent from the message and store the FileContent
		FileContent *fileContent = currentFileContentToTransfer;
		message->getPrivate()->addContent(fileContent);
		for (Content *content : message->getContents()) {
			if (content->isFileTransfer()) {
				FileTransferContent *fileTransferContent = static_cast<FileTransferContent *>(content);
				if (fileTransferContent->getFileContent() == fileContent) {
					message->getPrivate()->removeContent(content);
					delete fileTransferContent;
					break;
				}
			}
		}
		message->getPrivate()->setState(ChatMessage::State::FileTransferDone);
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

	if (content_type_hdr) {
		const char *type = belle_sip_header_content_type_get_type(content_type_hdr);
		const char *subtype = belle_sip_header_content_type_get_subtype(content_type_hdr);
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

		shared_ptr<ChatMessage> message = chatMessage.lock();
		if (!message)
			return;

		if (currentFileContentToTransfer) {
			belle_sip_header_content_length_t *content_length_hdr = BELLE_SIP_HEADER_CONTENT_LENGTH(belle_sip_message_get_header(response, "Content-Length"));
			currentFileContentToTransfer->setFileSize(belle_sip_header_content_length_get_content_length(content_length_hdr));
			lInfo() << "Extracted content length " << currentFileContentToTransfer->getFileSize() << " from header";
		} else {
			lWarning() << "No file transfer information for msg [" << this << "]: creating...";
			FileContent *content = createFileTransferInformationFromHeaders(response);
			message->addContent(content);
		}

		size_t body_size = 0;
		if (currentFileContentToTransfer)
			body_size = currentFileContentToTransfer->getFileSize();

		belle_sip_body_handler_t *body_handler = (belle_sip_body_handler_t *)belle_sip_user_body_handler_new(
			body_size, _chat_message_file_transfer_on_progress,
			nullptr, _chat_message_on_recv_body,
			nullptr, _chat_message_on_recv_end, this
		);
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
	shared_ptr<ChatMessage> message = chatMessage.lock();
	if (!message)
		return;
	message->getPrivate()->setState(ChatMessage::State::FileTransferError);
	releaseHttpRequest();
}

static void _chat_message_process_io_error_download (void *data, const belle_sip_io_error_event_t *event) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->processIoErrorDownload(event);
}

void FileTransferChatMessageModifier::processIoErrorDownload (const belle_sip_io_error_event_t *event) {
	lError() << "I/O Error during file download msg [" << this << "]";
	shared_ptr<ChatMessage> message = chatMessage.lock();
	if (!message)
		return;
	message->getPrivate()->setState(ChatMessage::State::FileTransferError);
	releaseHttpRequest();
}

static void _chat_message_process_response_from_get_file (void *data, const belle_http_response_event_t *event) {
	FileTransferChatMessageModifier *d = (FileTransferChatMessageModifier *)data;
	d->processResponseFromGetFile(event);
}

void FileTransferChatMessageModifier::processResponseFromGetFile (const belle_http_response_event_t *event) {
	// check the answer code
	if (event->response) {
		shared_ptr<ChatMessage> message = chatMessage.lock();
		if (!message)
			return;

		int code = belle_http_response_get_status_code(event->response);
		if (code >= 400 && code < 500) {
			lWarning() << "File transfer failed with code " << code;
			message->getPrivate()->setState(ChatMessage::State::FileTransferError);
		} else if (code != 200) {
			lWarning() << "Unhandled HTTP code response " << code << " for file transfer";
		}
		releaseHttpRequest();
	}
}

bool FileTransferChatMessageModifier::downloadFile (
	const shared_ptr<ChatMessage> &message,
	FileTransferContent *fileTransferContent
) {
	chatMessage = message;

	if (httpRequest) {
		lError() << "There is already a download in progress.";
		return false;
	}

	if (fileTransferContent->getContentType() != ContentType::FileTransfer) {
		lError() << "Content type is not a FileTransfer.";
		return false;
	}

	createFileTransferInformationsFromVndGsmaRcsFtHttpXml(fileTransferContent);
	FileContent *fileContent = fileTransferContent->getFileContent();
	currentFileContentToTransfer = fileContent;
	if (!currentFileContentToTransfer)
		return false;

	// THIS IS ONLY FOR BACKWARD C API COMPAT
	if (currentFileContentToTransfer->getFilePath().empty() && !message->getPrivate()->getFileTransferFilepath().empty()) {
		currentFileContentToTransfer->setFilePath(message->getPrivate()->getFileTransferFilepath());
	}

	belle_http_request_listener_callbacks_t cbs = { 0 };
	cbs.process_response_headers = _chat_process_response_headers_from_get_file;
	cbs.process_response = _chat_message_process_response_from_get_file;
	cbs.process_io_error = _chat_message_process_io_error_download;
	cbs.process_auth_requested = _chat_message_process_auth_requested_download;
	int err = startHttpTransfer(fileTransferContent->getFileUrl(), "GET", &cbs); // File URL has been set by createFileTransferInformationsFromVndGsmaRcsFtHttpXml
	if (err == -1)
		return false;
	// start the download, status is In Progress
	message->getPrivate()->setState(ChatMessage::State::InProgress);
	return true;
}

// ----------------------------------------------------------

void FileTransferChatMessageModifier::cancelFileTransfer () {
	if (!httpRequest) {
		lInfo() << "No existing file transfer - nothing to cancel";
		return;
	}

	if (!belle_http_request_is_cancelled(httpRequest)) {
		shared_ptr<ChatMessage> message = chatMessage.lock();
		if (message) {
			lInfo() << "Canceling file transfer " << (
				currentFileContentToTransfer->getFilePath().empty()
					? L_C_TO_STRING(linphone_core_get_file_transfer_server(message->getCore()->getCCore()))
					: currentFileContentToTransfer->getFilePath().c_str()
				);

		} else {
			lInfo() << "Warning: http request still running for ORPHAN msg: this is a memory leak";
		}
		belle_http_provider_cancel_request(provider, httpRequest);
	}
	releaseHttpRequest();
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

string FileTransferChatMessageModifier::createFakeFileTransferFromUrl (const string &url) {
	string fileName = url.substr(url.find_last_of("/") + 1);
	stringstream fakeXml;
	fakeXml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
	fakeXml << "<file xmlns=\"urn:gsma:params:xml:ns:rcs:rcs:fthttp\">\r\n";
	fakeXml << "<file-info type=\"file\">\r\n";
	fakeXml << "<file-name>" << fileName << "</file-name>\r\n";
	fakeXml << "<content-type>application/binary</content-type>\r\n";
	fakeXml << "<data url = \"" << url << "\"/>\r\n";
	fakeXml << "</file-info>\r\n";
	fakeXml << "</file>";
	return fakeXml.str();
}

LINPHONE_END_NAMESPACE
