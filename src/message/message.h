/*
 * message.h
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

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <list>
#include <memory>
#include <string>

#include "object/object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Address;
class ChatRoom;
class Content;
class ErrorInfo;
class MessagePrivate;

class LINPHONE_PUBLIC Message : public Object {
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

	std::shared_ptr<ChatRoom> getChatRoom () const;

	Direction getDirection () const;

	std::shared_ptr<const Address> getFromAddress () const;
	std::shared_ptr<const Address> getToAddress () const;
	std::shared_ptr<const Address> getLocalAddress () const;
	std::shared_ptr<const Address> getRemoteAddress () const;

	State getState () const;

	std::shared_ptr<const ErrorInfo> getErrorInfo () const;

	std::string getContentType () const;

	std::string getText () const;
	void setText (const std::string &text);

	void send () const;

	bool containsReadableText () const;

	bool isSecured () const;

	time_t getTime () const;

	std::string getId () const;

	std::string getAppdata () const;
	void setAppdata (const std::string &appData);

	std::list<std::shared_ptr<const Content> > getContents () const;
	void addContent (const std::shared_ptr<Content> &content);
	void removeContent (const std::shared_ptr<const Content> &content);

	std::string getCustomHeaderValue (const std::string &headerName) const;
	void addCustomHeader (const std::string &headerName, const std::string &headerValue);
	void removeCustomHeader (const std::string &headerName);

private:
	Message (MessagePrivate &p);

	L_DECLARE_PRIVATE(Message);
	L_DISABLE_COPY(Message);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _MESSAGE_H_
