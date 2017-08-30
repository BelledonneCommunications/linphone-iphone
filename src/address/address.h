/*
 * address.h
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

#ifndef _ADDRESS_H_
#define _ADDRESS_H_

#include <string>

#include "enums.h"
#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class AddressPrivate;

class LINPHONE_PUBLIC Address : public ClonableObject {
public:
	Address (const std::string &address = "");
	Address (const Address &src);
	~Address ();

	Address &operator= (const Address &src);

	operator bool () const;

	bool operator== (const Address &address) const;

	std::string getScheme () const;

	std::string getDisplayName () const;
	bool setDisplayName (const std::string &displayName);

	std::string getUsername () const;
	bool setUsername (const std::string &username);

	std::string getDomain () const;
	bool setDomain (const std::string &domain);

	int getPort () const;
	bool setPort (int port);

	Transport getTransport () const;
	bool setTransport (Transport transport);

	bool getSecure () const;
	bool setSecure (bool enabled);

	bool isSip () const;

	std::string getMethodParam () const;
	bool setMethodParam (const std::string &methodParam);

	std::string getPassword () const;
	bool setPassword (const std::string &password);

	bool clean ();

	std::string asString () const;
	std::string asStringUriOnly () const;

	bool equal (const Address &address) const;
	bool weakEqual (const Address &address) const;

	std::string getHeaderValue (const std::string &headerName) const;
	bool setHeader (const std::string &headerName, const std::string &headerValue);

private:
	L_DECLARE_PRIVATE(Address);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _ADDRESS_H_
