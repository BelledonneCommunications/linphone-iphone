/*
 * gruu-address.h
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

#ifndef _GRUU_ADDRESS_H_
#define _GRUU_ADDRESS_H_

#include "address/simple-address.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Address;
class GruuAddressPrivate;

class LINPHONE_PUBLIC GruuAddress : public SimpleAddress {
public:
	explicit GruuAddress (const std::string &address = "");
	GruuAddress (const GruuAddress &src);
	GruuAddress (const Address &src);
	~GruuAddress () = default;

	GruuAddress &operator= (const GruuAddress &src);

	bool operator== (const GruuAddress &address) const;
	bool operator!= (const GruuAddress &address) const;

	bool operator< (const GruuAddress &address) const;

	bool isValid () const;

	std::string getUrn () const;
	void setUrn (const std::string &urn);

	std::string asString () const override;

private:
	L_DECLARE_PRIVATE(GruuAddress);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _GRUU_ADDRESS_H_
