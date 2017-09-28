/*
 * chat-message.h
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

#ifndef _CHAT_MESSAGE_H_
#define _CHAT_MESSAGE_H_

#include <list>
#include <memory>

#include "imdn.h"
#include "linphone/api/c-types.h"
#include "linphone/api/c-chat-message.h"

#include "object/object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Address;
class ChatRoom;
class Content;
class ErrorInfo;
class ChatMessagePrivate;

class LINPHONE_PUBLIC ChatMessage : public Object {
	friend class ChatRoom;

public:
	enum Direction {
		Incoming,
		Outgoing
	};

	enum State {
		Idle,
		InProgress,
		Delivered,
		NotDelivered,
		FileTransferError,
		FileTransferDone,
		DeliveredToUser,
		Displayed
	};

	ChatMessage (const std::shared_ptr<ChatRoom> &room);
	virtual ~ChatMessage () = default;

	LinphoneChatMessage * getBackPtr();

	std::shared_ptr<ChatRoom> getChatRoom () const;

	// -----------------------------------------------------------------------------
	// Methods
	// -----------------------------------------------------------------------------

	void updateState(State state);
	void send();
	void reSend();
	void sendDeliveryNotification(LinphoneReason reason);
	void sendDisplayNotification();
	int uploadFile();
	int downloadFile();
	void cancelFileTransfer();
	int putCharacter(uint32_t character);

	// -----------------------------------------------------------------------------
	// Getters & setters
	// -----------------------------------------------------------------------------

	Direction getDirection () const;
	bool isOutgoing () const;
	bool isIncoming () const;

	std::string getExternalBodyUrl() const;
	void setExternalBodyUrl(const std::string &url);
	
	time_t getTime () const;

	bool isSecured () const;
	void setIsSecured(bool isSecured);

	State getState() const;
	
	std::string getId () const;
	void setId (std::string);

	bool isRead() const;
	
	std::string getAppdata () const;
	void setAppdata (const std::string &appData);
	
	std::shared_ptr<Address> getFromAddress () const;
	void setFromAddress(std::shared_ptr<Address> from);

	std::shared_ptr<Address> getToAddress () const;
	void setToAddress(std::shared_ptr<Address> to);

	std::string getFileTransferFilepath() const;
	void setFileTransferFilepath(const std::string &path);

	bool isToBeStored() const;
	void setIsToBeStored(bool store);

	const LinphoneErrorInfo * getErrorInfo () const;
	
	bool isReadOnly () const;
	
	std::list<std::shared_ptr<const Content> > getContents () const;
	void addContent (const std::shared_ptr<Content> &content);
	void removeContent (const std::shared_ptr<const Content> &content);

	std::string getCustomHeaderValue (const std::string &headerName) const;
	void addCustomHeader (const std::string &headerName, const std::string &headerValue);
	void removeCustomHeader (const std::string &headerName);

protected:
	explicit ChatMessage (ChatMessagePrivate &p);

private:
	L_DECLARE_PRIVATE(ChatMessage);
	L_DISABLE_COPY(ChatMessage);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CHAT_MESSAGE_H_
