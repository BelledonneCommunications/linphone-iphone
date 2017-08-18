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

#include "enums.h"
#include "object/object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Address;
class ChatRoom;
class ErrorInfo;
class MessagePrivate;

class LINPHONE_PUBLIC Message : public Object {
	friend class ChatRoom;

public:
	std::string getText () const;

	/**
	 * @brief Set a chat message text to be sent by linphone_chat_room_send_message.
	 */
	int setText (const std::string &text);

	/**
	 * @brief Get the state of the message.
	 */
	// ChatMessageState getState () const;

	/**
	 * @brief Get if the message was encrypted when transfered.
	 */
	bool isSecured ();

	/**
	 * @brief Get the time the message was sent.
	 */
	time_t getTime () const;

	std::shared_ptr<ChatRoom> getChatRoom ();

	std::string getMessageId () const;

	std::string getAppdata () const;
	void setAppdata (const std::string &data);

	std::shared_ptr<const Address> getPeerAddress () const;

	std::shared_ptr<const Address> getFromAddress () const;
	void setFromAddress (const std::shared_ptr<const Address> &address);

	std::shared_ptr<const Address> getToAddress () const;
	void setToAddress (const std::shared_ptr<const Address> &addr);

	std::shared_ptr<const Address> getLocalAddress () const;

	std::string getCustomHeaderValue (const std::string &headerName);

	void addCustomHeader (const std::string &headerName, const std::string &headerValue);
	void removeCustomHeader (const std::string &headerName);

	std::shared_ptr<const ErrorInfo> getErrorInfo () const;

protected:
	Message ();

private:
	L_DECLARE_PRIVATE(Message);
	L_DISABLE_COPY(Message);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _MESSAGE_H_
