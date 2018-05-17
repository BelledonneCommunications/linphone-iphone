/*
 * file-transfer-chat-message-modifier.h
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

#ifndef _L_FILE_TRANSFER_CHAT_MESSAGE_MODIFIER_H_
#define _L_FILE_TRANSFER_CHAT_MESSAGE_MODIFIER_H_

#include <belle-sip/belle-sip.h>

#include "chat-message-modifier.h"
#include "utils/background-task.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatRoom;
class Core;
class FileContent;
class FileTransferContent;

class FileTransferChatMessageModifier : public ChatMessageModifier {
public:
	FileTransferChatMessageModifier (belle_http_provider_t *prov);
	~FileTransferChatMessageModifier ();

	Result encode (const std::shared_ptr<ChatMessage> &message, int &errorCode) override;
	Result decode (const std::shared_ptr<ChatMessage> &message, int &errorCode) override;

	belle_http_request_t *getHttpRequest() const;
	void setHttpRequest(belle_http_request_t *request);

	int onSendBody (belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, size_t offset, uint8_t *buffer, size_t *size);
	void onSendEnd (belle_sip_user_body_handler_t *bh);
	void fileUploadBackgroundTaskEnded();
	void fileTransferOnProgress (belle_sip_body_handler_t *bh, belle_sip_message_t *m, size_t offset, size_t total);
	void processResponseFromPostFile (const belle_http_response_event_t *event);
	void processIoErrorUpload (const belle_sip_io_error_event_t *event);
	void processAuthRequestedUpload (const belle_sip_auth_event *event);

	void onRecvBody (belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, size_t offset, uint8_t *buffer, size_t size);
	void onRecvEnd (belle_sip_user_body_handler_t *bh);
	void processResponseHeadersFromGetFile (const belle_http_response_event_t *event);
	void processAuthRequestedDownload (const belle_sip_auth_event *event);
	void processIoErrorDownload (const belle_sip_io_error_event_t *event);
	void processResponseFromGetFile (const belle_http_response_event_t *event);

	bool downloadFile (const std::shared_ptr<ChatMessage> &message, FileTransferContent *fileTransferContent);
	void cancelFileTransfer ();
	bool isFileTransferInProgressAndValid ();
	std::string createFakeFileTransferFromUrl (const std::string &url);

private:
	int uploadFile ();
	int startHttpTransfer (const std::string &url, const std::string &action, belle_http_request_listener_callbacks_t *cbs);
	void fileUploadBeginBackgroundTask ();
	void fileUploadEndBackgroundTask ();

	void releaseHttpRequest ();

	std::weak_ptr<ChatMessage> chatMessage;
	FileContent* currentFileContentToTransfer = nullptr;

	belle_http_request_t *httpRequest = nullptr;
	belle_http_request_listener_t *httpListener = nullptr;
	belle_http_provider_t *provider  = nullptr;

	BackgroundTask bgTask;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_FILE_TRANSFER_CHAT_MESSAGE_MODIFIER_H_
