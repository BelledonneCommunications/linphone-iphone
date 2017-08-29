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

#include "magic-macros.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

#define L_ENUM_VALUE(C, VALUE) C ## VALUE

#ifndef L_USE_C_ENUM
	#define L_DECLARE_ENUM_VALUES(CLASS_NAME, ENUM_NAME, ...) \
		MM_APPLY_COMMA(L_ENUM_VALUE, ENUM_NAME, __VA_ARGS__)
#else
	#define L_DECLARE_ENUM_VALUES(CLASS_NAME, ENUM_NAME, ...) \
		MM_APPLY_COMMA(L_ENUM_VALUE, Linphone ## CLASS_NAME ## ENUM_NAME, __VA_ARGS__)
#endif // ifndef L_USE_C_ENUM

LINPHONE_END_NAMESPACE

#endif // ifndef _ENUM_GENERATOR_H_
