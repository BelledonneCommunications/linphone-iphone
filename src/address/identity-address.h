/*
 * identity-address.h
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

#ifndef _L_IDENTITY_ADDRESS_H_
#define _L_IDENTITY_ADDRESS_H_

#include <ostream>

#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Address;
class IdentityAddressPrivate;

class LINPHONE_PUBLIC IdentityAddress : public ClonableObject {
public:
	explicit IdentityAddress (const std::string &address = "");
	IdentityAddress (const Address &address);
	IdentityAddress (const IdentityAddress &other);
	~IdentityAddress () = default;

	IdentityAddress &operator= (const IdentityAddress &other);

	bool operator== (const IdentityAddress &other) const;
	bool operator!= (const IdentityAddress &other) const;

	bool operator< (const IdentityAddress &other) const;

	bool isValid () const;

	const std::string &getScheme () const;

	const std::string &getUsername () const;
	bool setUsername (const std::string &username);

	const std::string &getDomain () const;
	bool setDomain (const std::string &domain);

	bool hasGruu () const;
	const std::string &getGruu () const;
	bool setGruu (const std::string &gruu);

	IdentityAddress getAddressWithoutGruu () const;

	virtual std::string asString () const;

private:
	L_DECLARE_PRIVATE(IdentityAddress);
};

inline std::ostream &operator<< (std::ostream &os, const IdentityAddress &identityAddress) {
	os << "IdentityAddress(" << identityAddress.asString() << ")";
	return os;
}

LINPHONE_END_NAMESPACE

#endif // ifndef _L_IDENTITY_ADDRESS_H_
