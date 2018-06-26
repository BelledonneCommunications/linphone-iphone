/*
 * enum-generator.h
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

#ifndef _L_ENUM_GENERATOR_H_
#define _L_ENUM_GENERATOR_H_

#include "linphone/utils/magic-macros.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Low-level, do not call.
// -----------------------------------------------------------------------------

// Declare one enum value. `value` is optional, it can be generated.
// It's useful to force value in the case of mask.
#define L_DECLARE_ENUM_VALUE_1_ARG(NAME) NAME,
#define L_DECLARE_ENUM_VALUE_2_ARGS(NAME, VALUE) NAME = VALUE,

// Call the right macro. (With or without value.)
#define L_DECLARE_ENUM_MACRO_CHOOSER(...) \
	L_EXPAND(L_GET_ARG_3(__VA_ARGS__, L_DECLARE_ENUM_VALUE_2_ARGS, L_DECLARE_ENUM_VALUE_1_ARG))

// Enum value declaration.
#define L_DECLARE_ENUM_VALUE(...) \
	L_EXPAND(L_DECLARE_ENUM_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))

#ifdef __cplusplus

// `getEnumValue` helper.
#define L_DECLARE_ENUM_VALUE_STR_CASE(ENUM_NAME, VALUE_NAME, ...) \
	case ENUM_NAME::VALUE_NAME: return #ENUM_NAME "::" #VALUE_NAME;

// Helper to get enum name.
#define L_DECLARE_ENUM_NAME(NAME, ...) NAME,

// Get names as string from enum values.
#define L_GET_ENUM_VALUE_NAMES(VALUES) L_GET_HEAP(VALUES(L_DECLARE_ENUM_NAME))

#endif

// -----------------------------------------------------------------------------
// Public API.
// -----------------------------------------------------------------------------

#ifdef __cplusplus

#define L_DECLARE_ENUM(NAME, VALUES) \
	enum class NAME { \
		VALUES(L_DECLARE_ENUM_VALUE) \
	}; \
	friend constexpr const char *getEnumNameAsString (NAME) { \
		return #NAME; \
	} \
	friend const char *getEnumValueAsString (const NAME &value) { \
		switch (value) { \
			L_APPLY_WITHOUT_COMMA(L_DECLARE_ENUM_VALUE_STR_CASE, NAME, L_GET_ENUM_VALUE_NAMES(VALUES)) \
		} \
		return ""; \
	}

template<typename T>
inline char getEnumValueAsString (const T &) { return 0; }

template<typename T>
struct IsDefinedEnum {
	enum { value = sizeof(getEnumValueAsString(std::declval<T>())) == sizeof(const char *) };
};

#endif

#define L_C_ENUM_PREFIX Linphone

// TODO: This macro should be used but it is triggering a bug in doxygen that
// has been fixed in the 1.8.8 version. See https://bugzilla.gnome.org/show_bug.cgi?id=731985
// Meanwhile use 2 different macros.
#if 0
#define L_DECLARE_C_ENUM(NAME, VALUES) \
	typedef enum L_CONCAT(_, L_CONCAT(L_C_ENUM_PREFIX, NAME)) { \
		L_APPLY(L_CONCAT, L_CONCAT(L_C_ENUM_PREFIX, NAME), L_GET_HEAP(VALUES(L_DECLARE_ENUM_VALUE))) \
	} L_CONCAT(L_C_ENUM_PREFIX, NAME)
#else
#define L_DECLARE_C_ENUM(NAME, VALUES) \
	typedef enum L_CONCAT(_, L_CONCAT(L_C_ENUM_PREFIX, NAME)) { \
		L_APPLY(L_CONCAT, L_CONCAT(L_C_ENUM_PREFIX, NAME), L_GET_HEAP(VALUES(L_DECLARE_ENUM_VALUE_1_ARG))) \
	} L_CONCAT(L_C_ENUM_PREFIX, NAME)
#define L_DECLARE_C_ENUM_FIXED_VALUES(NAME, VALUES) \
	typedef enum L_CONCAT(_, L_CONCAT(L_C_ENUM_PREFIX, NAME)) { \
		L_APPLY(L_CONCAT, L_CONCAT(L_C_ENUM_PREFIX, NAME), L_GET_HEAP(VALUES(L_DECLARE_ENUM_VALUE_2_ARGS))) \
	} L_CONCAT(L_C_ENUM_PREFIX, NAME)
#endif

LINPHONE_END_NAMESPACE

#endif // ifndef _L_ENUM_GENERATOR_H_
