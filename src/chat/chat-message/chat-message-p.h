/*
 * chat-message-p.h
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

#ifndef _L_CHAT_MESSAGE_P_H_
#define _L_CHAT_MESSAGE_P_H_

#include <belle-sip/types.h>

#include "chat/chat-message/chat-message.h"
#include "chat/chat-room/chat-room-id.h"
#include "chat/modifier/file-transfer-chat-message-modifier.h"
#include "chat/notification/imdn.h"
#include "content/content.h"
#include "content/content-type.h"
#include "content/file-content.h"
#include "content/file-transfer-content.h"
#include "db/main-db.h"
#include "db/main-db-chat-message-key.h"
#include "event-log/conference/conference-chat-message-event.h"
#include "object/object-p.h"
#include "sal/sal.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class ChatMessagePrivate : public ObjectPrivate {
	friend class CpimChatMessageModifier;
	friend class EncryptionChatMessageModifier;
	friend class MultipartChatMessageModifier;
	friend class NotificationMessagePrivate;

public:
	enum Step {
		None = 1 << 0,
		FileUpload = 1 << 1,
		Multipart = 1 << 2,
		Encryption = 1 << 3,
		Cpim = 1 << 4,
		Started = 1 << 5,
	};

	void setApplyModifiers (bool value) { applyModifiers = value; }

	void setDirection (ChatMessage::Direction dir);

	void setParticipantState (const IdentityAddress &participantAddress, ChatMessage::State newState, time_t stateChangeTime);
	virtual void setState (ChatMessage::State newState, bool force = false);

	void setTime (time_t time);

	void setIsReadOnly (bool readOnly);

	void setImdnMessageId (const std::string &imdnMessageId);

	void forceFromAddress (const IdentityAddress &fromAddress) {
		this->fromAddress = fromAddress;
	}

	void forceToAddress (const IdentityAddress &toAddress) {
		this->toAddress = toAddress;
	}

	void markContentsAsNotLoaded () {
		contentsNotLoadedFromDatabase = true;
	}

	void loadContentsFromDatabase () const;

	std::list<Content* > &getContents () {
		loadContentsFromDatabase();
		return contents;
	}

	const std::list<Content* > &getContents () const {
		loadContentsFromDatabase();
		return contents;
	}

	belle_http_request_t *getHttpRequest () const;
	void setHttpRequest (belle_http_request_t *request);

	SalOp *getSalOp () const;
	void setSalOp (SalOp *op);

	bool getDisplayNotificationRequired () const { return displayNotificationRequired; }
	bool getNegativeDeliveryNotificationRequired () const { return negativeDeliveryNotificationRequired; }
	bool getPositiveDeliveryNotificationRequired () const { return positiveDeliveryNotificationRequired; }
	virtual void setDisplayNotificationRequired (bool value) { displayNotificationRequired = value; }
	virtual void setNegativeDeliveryNotificationRequired (bool value) { negativeDeliveryNotificationRequired = value; }
	virtual void setPositiveDeliveryNotificationRequired (bool value) { positiveDeliveryNotificationRequired = value; }

	SalCustomHeader *getSalCustomHeaders () const;
	void setSalCustomHeaders (SalCustomHeader *headers);

	void addSalCustomHeader (const std::string &name, const std::string &value);
	void removeSalCustomHeader (const std::string &name);
	std::string getSalCustomHeaderValue (const std::string &name);

	void loadFileTransferUrlFromBodyToContent ();
	std::string createFakeFileTransferFromUrl(const std::string &url);

	void setChatRoom (const std::shared_ptr<AbstractChatRoom> &chatRoom);

	void setEncryptionPrevented (bool value) { encryptionPrevented = value; }

	// -----------------------------------------------------------------------------
	// Deprecated methods only used for C wrapper, to be removed some day...
	// -----------------------------------------------------------------------------

	const ContentType &getContentType ();
	void setContentType (const ContentType &contentType);

	const std::string &getText ();
	void setText (const std::string &text);

	const std::string &getFileTransferFilepath () const;
	void setFileTransferFilepath (const std::string &path);

	const std::string &getAppdata () const;
	void setAppdata (const std::string &appData);

	const std::string &getExternalBodyUrl () const;
	void setExternalBodyUrl (const std::string &url);

	bool hasTextContent () const;
	const Content* getTextContent () const;

	bool hasFileTransferContent () const;
	const Content* getFileTransferContent () const;

	const Content* getFileTransferInformation () const;
	void setFileTransferInformation (Content *content);

	void addContent (Content *content);
	void removeContent (Content *content);

	bool downloadFile ();

	void notifyReceiving ();
	LinphoneReason receive ();
	void send ();

	void storeInDb ();
	void updateInDb ();

private:
	ChatMessagePrivate(const std::shared_ptr<AbstractChatRoom> &cr, ChatMessage::Direction dir);

	static bool validStateTransition (ChatMessage::State currentState, ChatMessage::State newState);

public:
	mutable MainDbChatMessageKey dbKey;

protected:
	bool displayNotificationRequired = true;
	bool negativeDeliveryNotificationRequired = true;
	bool positiveDeliveryNotificationRequired = true;
	bool toBeStored = true;
	std::string contentEncoding;

private:
	// TODO: Clean attributes.
	time_t time = ::ms_time(0); // TODO: Change me in all files.
	std::string imdnId;
	std::string rttMessage;
	std::string externalBodyUrl;
	bool isSecured = false;
	mutable bool isReadOnly = false;
	Content internalContent;

	// TODO: to replace salCustomheaders
	std::unordered_map<std::string, std::string> customHeaders;

	mutable LinphoneErrorInfo * errorInfo = nullptr;
	SalOp *salOp = nullptr;
	SalCustomHeader *salCustomHeaders = nullptr;
	unsigned char currentSendStep = Step::None;
	unsigned char currentRecvStep = Step::None;
	bool applyModifiers = true;
	FileTransferChatMessageModifier fileTransferChatMessageModifier;

	// Cache for returned values, used for compatibility with previous C API
	std::string fileTransferFilePath;
	ContentType cContentType;
	std::string cText;

	// TODO: Remove my comment. VARIABLES OK.
	// Do not expose.

	std::weak_ptr<AbstractChatRoom> chatRoom;
	ChatRoomId chatRoomId;
	IdentityAddress fromAddress;
	IdentityAddress toAddress;

	ChatMessage::State state = ChatMessage::State::Idle;
	ChatMessage::Direction direction = ChatMessage::Direction::Incoming;

	std::list<Content* > contents;

	bool encryptionPrevented = false;
	mutable bool contentsNotLoadedFromDatabase = false;
	L_DECLARE_PUBLIC(ChatMessage);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CHAT_MESSAGE_P_H_
