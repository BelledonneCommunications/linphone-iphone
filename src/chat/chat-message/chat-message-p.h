/*
 * chat-message-p.h
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

#ifndef _CHAT_MESSAGE_P_H_
#define _CHAT_MESSAGE_P_H_

#include <belle-sip/types.h>

#include "chat/chat-message/chat-message.h"
#include "chat/notification/imdn.h"
#include "content/content.h"
#include "content/content-type.h"
#include "object/object-p.h"
#include "sal/sal.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatMessagePrivate : public ObjectPrivate {
	friend class CpimChatMessageModifier;
	friend class EncryptionChatMessageModifier;
	friend class MultipartChatMessageModifier;

public:
	enum Step {
		None = 1 << 0,
		FileUpload = 1 << 1,
		Multipart = 1 << 2,
		Encryption = 1 << 3,
		Cpim = 1 << 4
	};

	ChatMessagePrivate ();
	~ChatMessagePrivate ();

	void setChatRoom (std::shared_ptr<ChatRoom> chatRoom);

	// -----------------------------------------------------------------------------

	void setApplyModifiers (bool value) { applyModifiers = value; }

	void setDirection (ChatMessage::Direction dir);

	void setState(ChatMessage::State state);

	void setTime(time_t time);

	void setIsReadOnly(bool readOnly);

	unsigned int getStorageId() const;
	void setStorageId(unsigned int id);

	belle_http_request_t *getHttpRequest() const;
	void setHttpRequest(belle_http_request_t *request);

	SalOp *getSalOp() const;
	void setSalOp(SalOp *op);

	SalCustomHeader *getSalCustomHeaders() const;
	void setSalCustomHeaders(SalCustomHeader *headers);

	void addSalCustomHeader(const std::string& name, const std::string& value);
	void removeSalCustomHeader(const std::string& name);
	std::string getSalCustomHeaderValue(const std::string& name);

	// -----------------------------------------------------------------------------
	// Methods only used for C wrapper, to be removed some day...
	// -----------------------------------------------------------------------------

	const ContentType &getContentType();
	void setContentType(const ContentType &contentType);

	const std::string &getText();
	void setText(const std::string &text);

	LinphoneContent *getFileTransferInformation() const;
	void setFileTransferInformation(LinphoneContent *content);

	// -----------------------------------------------------------------------------
	// Need to be public to be called from static C callbacks
	// -----------------------------------------------------------------------------

	void fileTransferOnProgress(belle_sip_body_handler_t *bh, belle_sip_message_t *m, size_t offset, size_t total);
	int onSendBody(belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, size_t offset, uint8_t *buffer, size_t *size);
	void onSendEnd(belle_sip_user_body_handler_t *bh);
	void onRecvBody(belle_sip_user_body_handler_t *bh, belle_sip_message_t *m, size_t offset, uint8_t *buffer, size_t size);
	void onRecvEnd(belle_sip_user_body_handler_t *bh);
	void fileUploadBackgroundTaskEnded();
	void processResponseFromPostFile(const belle_http_response_event_t *event);
	void processResponseHeadersFromGetFile(const belle_http_response_event_t *event);
	void processAuthRequestedDownload(const belle_sip_auth_event *event);
	void processIoErrorUpload(const belle_sip_io_error_event_t *event);
	void processAuthRequestedUpload(const belle_sip_auth_event *event);
	void processIoErrorDownload(const belle_sip_io_error_event_t *event);
	void processResponseFromGetFile(const belle_http_response_event_t *event);

	// -----------------------------------------------------------------------------

	void sendImdn(Imdn::Type imdnType, LinphoneReason reason);

	LinphoneReason receive();
	void send();

private:
	std::weak_ptr<ChatRoom> chatRoom;
	Address peerAddress;

	// TODO: Clean attributes.
	ChatMessage::Direction direction = ChatMessage::Direction::Incoming;
	ChatMessage::State state = ChatMessage::State::Idle;
	unsigned int storageId = 0;
	Address from;
	Address to;
	time_t time = 0;
	std::string id;
	std::string appData;
	std::string fileTransferFilePath;
	std::string externalBodyUrl;
	std::string rttMessage;
	bool isSecured = false;
	bool isReadOnly = false;
	std::list<Content > contents;
	Content internalContent;
	std::unordered_map<std::string, std::string> customHeaders;
	mutable LinphoneErrorInfo * errorInfo = nullptr;
	belle_http_request_t *httpRequest = nullptr;
	belle_http_request_listener_t *httpListener = nullptr;
	SalOp *salOp = nullptr;
	SalCustomHeader *salCustomHeaders = nullptr;
	unsigned long backgroundTaskId = 0;
	unsigned char currentSendStep = Step::None;
	bool applyModifiers = true;
	// Cache for returned values, used for compatibility with previous C API
	ContentType cContentType;
	std::string cText;
	// Used for compatibility with previous C API
	LinphoneContent *cFileTransferInformation = nullptr;

	// -----------------------------------------------------------------------------

	std::string createImdnXml(Imdn::Type imdnType, LinphoneReason reason);

	void fileUploadEndBackgroundTask();
	void fileUploadBeginBackgroundTask();
	bool isFileTransferInProgressAndValid();
	int startHttpTransfer(
		const std::string &url,
		const std::string &action,
		belle_http_request_listener_callbacks_t *cbs
	);
	void releaseHttpRequest();
	void createFileTransferInformationsFromVndGsmaRcsFtHttpXml(const std::string &body);

	L_DECLARE_PUBLIC(ChatMessage);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CHAT_MESSAGE_P_H_
