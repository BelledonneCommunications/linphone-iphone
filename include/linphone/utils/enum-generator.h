/*
 * enum-generator.h
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

#ifndef _ENUM_GENERATOR_H_
#define _ENUM_GENERATOR_H_

#include "linphone/utils/magic-macros.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Low-level, do not call.
// -----------------------------------------------------------------------------

// Declare one enum value. `value` is optional, it can be generated.
// It's useful to force value in the case of mask.
#define L_DECLARE_ENUM_VALUE_1_ARGS(NAME) NAME,
#define L_DECLARE_ENUM_VALUE_2_ARGS(NAME, VALUE) NAME = VALUE,

// Call the right macro. (With or without value.)
#define L_DECLARE_ENUM_MACRO_CHOOSER(...) \
	L_GET_ARG_3(__VA_ARGS__, L_DECLARE_ENUM_VALUE_2_ARGS, L_DECLARE_ENUM_VALUE_1_ARGS)

// Enum value declaration.
#define L_DECLARE_ENUM_VALUE(...) L_DECLARE_ENUM_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

// -----------------------------------------------------------------------------
// Public API.
// -----------------------------------------------------------------------------

#define L_DECLARE_ENUM(NAME, VALUES) \
	enum class NAME { \
		VALUES(L_DECLARE_ENUM_VALUE) \
	};

#define L_C_ENUM_PREFIX Linphone

#define L_DECLARE_C_ENUM(NAME, VALUES) \
	enum L_CONCAT(L_C_ENUM_PREFIX, NAME) { \
		L_APPLY(L_CONCAT, L_CONCAT(L_C_ENUM_PREFIX, NAME), L_GET_HEAP(VALUES(L_DECLARE_ENUM_VALUE))) \
	}; \
	typedef enum L_CONCAT(L_C_ENUM_PREFIX, NAME) L_CONCAT(L_C_ENUM_PREFIX, NAME);

LINPHONE_END_NAMESPACE

#endif // ifndef _ENUM_GENERATOR_H_
