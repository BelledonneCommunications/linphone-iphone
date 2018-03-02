/*
 * address.h
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

#ifndef _L_ADDRESS_H_
#define _L_ADDRESS_H_

#include <ostream>

#include "enums.h"
#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class AddressPrivate;
class IdentityAddress;

class LINPHONE_PUBLIC Address : public ClonableObject {
	// TODO: Remove me later.
	friend class CallSession;
	friend class ClientGroupChatRoom;
	friend class ClientGroupChatRoomPrivate;
	friend class ServerGroupChatRoom;
	friend class ServerGroupChatRoomPrivate;
	friend class IdentityAddress;

public:
	explicit Address (const std::string &address = "");
	Address (const IdentityAddress &identityAddress);
	Address (const Address &other);
	~Address ();

	Address &operator= (const Address &other);

	bool operator== (const Address &other) const;
	bool operator!= (const Address &other) const;

	bool operator< (const Address &other) const;

	bool isValid () const;

	const std::string &getScheme () const;

	const std::string &getDisplayName () const;
	bool setDisplayName (const std::string &displayName);

	const std::string &getUsername () const;
	bool setUsername (const std::string &username);

	const std::string &getDomain () const;
	bool setDomain (const std::string &domain);

	int getPort () const;
	bool setPort (int port);

	Transport getTransport () const;
	bool setTransport (Transport transport);

	bool getSecure () const;
	bool setSecure (bool enabled);

	bool isSip () const;

	const std::string &getMethodParam () const;
	bool setMethodParam (const std::string &methodParam);

	const std::string &getPassword () const;
	bool setPassword (const std::string &password);

	bool clean ();

	std::string asString () const;
	std::string asStringUriOnly () const;

	bool weakEqual (const Address &address) const;

	const std::string &getHeaderValue (const std::string &headerName) const;
	bool setHeader (const std::string &headerName, const std::string &headerValue);

	bool hasParam (const std::string &paramName) const;
	const std::string &getParamValue (const std::string &paramName) const;
	bool setParam (const std::string &paramName, const std::string &paramValue = "");
	bool setParams (const std::string &params);

	bool hasUriParam (const std::string &uriParamName) const;
	const std::string &getUriParamValue (const std::string &uriParamName) const;
	bool setUriParam (const std::string &uriParamName, const std::string &uriParamValue = "");
	bool setUriParams (const std::string &uriParams);
	bool removeUriParam (const std::string &uriParamName);

private:
	L_DECLARE_PRIVATE(Address);
};

inline std::ostream &operator<< (std::ostream &os, const Address &address) {
	os << "Address(" << address.asString() << ")";
	return os;
}

LINPHONE_END_NAMESPACE

#endif // ifndef _L_ADDRESS_H_
