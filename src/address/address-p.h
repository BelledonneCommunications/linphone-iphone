/*
 * address-p.h
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

#ifndef _ADDRESS_P_H_
#define _ADDRESS_P_H_

#include "address.h"
#include "object/clonable-object-p.h"

// =============================================================================

struct SalAddress;

LINPHONE_BEGIN_NAMESPACE

class AddressPrivate : public ClonableObjectPrivate {
public:
	inline const SalAddress *getInternalAddress () const {
		return internalAddress;
	}

private:
	struct AddressCache {
		mutable std::string scheme;
		mutable std::string displayName;
		mutable std::string username;
		mutable std::string domain;
		mutable std::string methodParam;
		mutable std::string password;

		mutable std::unordered_map<std::string, std::string> headers;
		mutable std::unordered_map<std::string, std::string> params;
		mutable std::unordered_map<std::string, std::string> uriParams;
	};

	SalAddress *internalAddress = nullptr;
	AddressCache cache;

	L_DECLARE_PUBLIC(Address);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _ADDRESS_P_H_
