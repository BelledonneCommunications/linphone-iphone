/*
 * header-param.h
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

#ifndef _L_HEADER_PARAM_H_
#define _L_HEADER_PARAM_H_

#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class HeaderParamPrivate;

class LINPHONE_PUBLIC HeaderParam : public ClonableObject {
public:
	explicit HeaderParam (const std::string &header = "");
	HeaderParam (const std::string &name, const std::string &value);
	HeaderParam (const HeaderParam &other);

	HeaderParam &operator= (const HeaderParam &other);

	bool operator== (const HeaderParam &other) const;
	bool operator!= (const HeaderParam &other) const;

	// Delete these operators to prevent putting complicated content-type strings
	// in the code. Instead define static const HeaderParam objects below.
	bool operator== (const std::string &other) const = delete;
	bool operator!= (const std::string &other) const = delete;

	const std::string &getName () const;
	bool setName (const std::string &name);

	const std::string &getValue () const;
	bool setValue (const std::string &value);

	std::string asString () const;

private:
	L_DECLARE_PRIVATE(HeaderParam);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_HEADER_PARAM_H_
