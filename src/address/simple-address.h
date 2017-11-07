/*
 * simple-address.h
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

#ifndef _SIMPLE_ADDRESS_H_
#define _SIMPLE_ADDRESS_H_

#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Address;
class SimpleAddressPrivate;

class LINPHONE_PUBLIC SimpleAddress : public ClonableObject {
public:
	explicit SimpleAddress (const std::string &address = "");
	SimpleAddress (const SimpleAddress &src);
	SimpleAddress (const Address &src);
	~SimpleAddress () = default;

	SimpleAddress &operator= (const SimpleAddress &src);

	bool operator== (const SimpleAddress &address) const;
	bool operator!= (const SimpleAddress &address) const;

	bool operator< (const SimpleAddress &address) const;

	const std::string &getScheme () const;

	const std::string &getUsername () const;
	bool setUsername (const std::string &username);

	const std::string &getDomain () const;
	bool setDomain (const std::string &domain);

	bool isSip () const;

	std::string asString () const;
	std::string asStringUriOnly () const;

private:
	L_DECLARE_PRIVATE(SimpleAddress);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _SIMPLE_ADDRESS_H_
