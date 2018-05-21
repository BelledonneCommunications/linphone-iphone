/*
 * private-access.h
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

// =============================================================================
// Tools to get/set private data of Cpp objects.
//
// See: https://bloglitb.blogspot.fr/2011/12/access-to-private-members-safer.html
// See: http://en.cppreference.com/w/cpp/language/adl
// See: http://en.cppreference.com/w/cpp/language/friend
// =============================================================================

#ifndef _L_PRIVATE_ACCESS_H_
#define _L_PRIVATE_ACCESS_H_

#include <type_traits>

#include "linphone/utils/utils.h"

#define L_INTERNAL_STRUCT_L_ATTR_GET(CLASS, ATTR_NAME) AttrGet ## _ ## CLASS ## _ ## ATTR_NAME
#define L_INTERNAL_STRUCT_ATTR_SPY(ATTR_NAME) AttrSpy ## _ ## ATTR_NAME

#define L_ENABLE_ATTR_ACCESS(CLASS, ATTR_TYPE, ATTR_NAME) \
	template<typename AttrSpy, ATTR_TYPE CLASS::*Attr> \
	struct L_INTERNAL_STRUCT_L_ATTR_GET(CLASS, ATTR_NAME) { \
		friend constexpr ATTR_TYPE CLASS::*get(AttrSpy *) { \
			return Attr; \
		} \
	}; \
	template<typename T> \
	struct L_INTERNAL_STRUCT_ATTR_SPY(ATTR_NAME); \
	template<> \
	struct L_INTERNAL_STRUCT_ATTR_SPY(ATTR_NAME)<CLASS> { \
		friend constexpr ATTR_TYPE CLASS::*get(L_INTERNAL_STRUCT_ATTR_SPY(ATTR_NAME)<CLASS> *); \
	}; \
	template struct L_INTERNAL_STRUCT_L_ATTR_GET(CLASS, ATTR_NAME)< \
		L_INTERNAL_STRUCT_ATTR_SPY(ATTR_NAME)<CLASS>, \
		&CLASS::ATTR_NAME \
	>;

// Warning: Allow to modify const data.
// Returns a ref to `ATTR_NAME`.
#define L_ATTR_GET(OBJECT, ATTR_NAME) \
	(const_cast<std::remove_pointer<std::decay<decltype(OBJECT)>::type>::type *>(LinphonePrivate::Utils::getPtr(OBJECT)))->*get( \
		static_cast<L_INTERNAL_STRUCT_ATTR_SPY(ATTR_NAME)<std::remove_pointer<std::decay<decltype(OBJECT)>::type>::type> *>(nullptr) \
	)

#endif // ifndef _L_PRIVATE_ACCESS_H_
