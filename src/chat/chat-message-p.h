/*
 * chat-message-p.h
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

#ifndef _CHAT_MESSAGE_P_H_
#define _CHAT_MESSAGE_P_H_

#include "chat-message.h"
#include "db/events-db.h"
#include "object/object-p.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatMessagePrivate : public ObjectPrivate {
	friend class CpimChatMessageModifier;
	friend class MultipartChatMessageModifier;

public:
	ChatMessagePrivate (const std::shared_ptr<ChatRoom> &room);
	virtual ~ChatMessagePrivate ();

	void setChatRoom (std::shared_ptr<ChatRoom> chatRoom);
	
	// -----------------------------------------------------------------------------

	void setDirection (ChatMessage::Direction dir);

	void setState(ChatMessage::State state);
	
	void setTime(time_t time);
	
	unsigned int getStorageId() const;
	void setStorageId(unsigned int id);
	
	belle_http_request_t *getHttpRequest() const;
	void setHttpRequest(belle_http_request_t *request);

	SalOp *getSalOp() const;
	void setSalOp(SalOp *op);

	SalCustomHeader *getSalCustomHeaders() const;
	void setSalCustomHeaders(SalCustomHeader *headers);

	void addSalCustomHeader(std::string name, std::string value);
	void removeSalCustomHeader(std::string name);
	std::string getSalCustomHeaderValue(std::string name);

	// -----------------------------------------------------------------------------
	// Methods only used for C wrapper
	// -----------------------------------------------------------------------------
	
	const std::string& getContentType() const;
	void setContentType(const std::string& contentType);

	const std::string& getText() const;
	void setText(const std::string& text);
	
	LinphoneContent * getFileTransferInformation() const;
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
	
	void sendImdn(ImdnType imdnType, LinphoneReason reason);

private:
	std::shared_ptr<ChatRoom> chatRoom;
	ChatMessage::Direction direction = ChatMessage::Incoming;
	ChatMessage::State state = ChatMessage::Idle;
	unsigned int storageId;
	std::shared_ptr<Address> from;
	std::shared_ptr<Address> to;
	time_t time = 0;
	std::string id = "";
	std::string appData = "";
	std::string fileTransferFilePath = "";
	std::string externalBodyUrl = "";
	std::string rttMessage = "";
	bool isSecured = false;
	bool isReadOnly = false;
	bool isToBeStored = false;
	std::list<std::shared_ptr<Content> > contents;
	std::shared_ptr<Content> internalContent;
	std::unordered_map<std::string, std::string> customHeaders;
	std::shared_ptr<EventsDb> eventsDb;
	mutable LinphoneErrorInfo * errorInfo = NULL;
	belle_http_request_t *httpRequest = NULL;
	belle_http_request_listener_t *httpListener = NULL;
	SalOp *salOp = NULL;
	SalCustomHeader *salCustomHeaders = NULL;
	unsigned long backgroundTaskId;
	// Used for compatibility with previous C API
	std::string cContentType = "";
	std::string cText = "";
	LinphoneContent *cFileTransferInformation = NULL;
	
	// -----------------------------------------------------------------------------

	std::string createImdnXml(ImdnType imdnType, LinphoneReason reason);

	void fileUploadEndBackgroundTask();
	void fileUploadBeginBackgroundTask();
	bool isFileTransferInProgressAndValid();

	int startHttpTransfer(std::string url, std::string action, belle_http_request_listener_callbacks_t *cbs);

	void releaseHttpRequest();
	
	// -----------------------------------------------------------------------------

	L_DECLARE_PUBLIC(ChatMessage);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CHAT_MESSAGE_P_H_
