/*
 * magic-macros.h
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

#ifndef _MAGIC_MACROS_H_
#define _MAGIC_MACROS_H_

#include "general.h"

// =============================================================================
// Original header.
// Source: https://github.com/facebookresearch/ELF/blob/master/elf/pybind_helper.h
// =============================================================================

/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

// File: macros.h
// Author: Yuxin Wu <ppwwyyxxc@gmail.com>

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

#define MM_CONCAT__(A, B) A ## B
#define MM_CONCAT_(A, B) MM_CONCAT__(A, B)
#define MM_CONCAT(A, B) MM_CONCAT_(A, B)

#define MM_INVOKE(MACRO, ARGS) MACRO ARGS
#define MM_INVOKE_B(MACRO, ARGS) MACRO ARGS

#define MM_APPLY_1(MACRONAME, C, A1) \
	MM_INVOKE_B(MACRONAME, (C, A1))

#define MM_APPLY_2(MACRONAME, C, A1, A2) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_1(MACRONAME, C, A2)

#define MM_APPLY_3(MACRONAME, C, A1, A2, A3) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_2(MACRONAME, C, A2, A3)

#define MM_APPLY_4(MACRONAME, C, A1, A2, A3, A4) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_3(MACRONAME, C, A2, A3, A4)

#define MM_APPLY_5(MACRONAME, C, A1, A2, A3, A4, A5) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_4(MACRONAME, C, A2, A3, A4, A5)

#define MM_APPLY_6(MACRONAME, C, A1, A2, A3, A4, A5, A6) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_5(MACRONAME, C, A2, A3, A4, A5, A6)

#define MM_APPLY_7(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_6(MACRONAME, C, A2, A3, A4, A5, A6, A7)

#define MM_APPLY_8(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_7(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8)

#define MM_APPLY_9(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_8(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9)

#define MM_APPLY_10(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_9(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10)

#define MM_APPLY_11(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_10(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11)

#define MM_APPLY_12(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_11(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12)

#define MM_APPLY_13(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_12(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13)

#define MM_APPLY_14(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_13(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14)

#define MM_APPLY_15(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_14(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15)

#define MM_APPLY_16(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_15(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16)

#define MM_APPLY_17(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_16(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17)

#define MM_APPLY_18(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_17(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18)

#define MM_APPLY_19(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_18(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19)

#define MM_APPLY_20(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_19(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20)

#define MM_APPLY_21(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_20(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21)

#define MM_APPLY_22(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21, A22) \
	MM_INVOKE_B(MACRONAME, (C, A1)) \
	MM_APPLY_21(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21, A22)

#define MM_NARG(...) \
	MM_NARG_(__VA_ARGS__, MM_RSEQ_N())

#define MM_NARG_(...) \
	MM_ARG_N(__VA_ARGS__)

#define MM_ARG_N( \
	_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
	_11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
	_21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
	_31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
	_41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
	_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
	_61, _62, _63, N, ...) N

#define MM_RSEQ_N() \
	63, 62, 61, 60, \
	59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
	49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
	39, 38, 37, 36, 35, 34, 33, 32, 31, 30, \
	29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
	19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
	9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define MM_APPLY(MACRONAME, C, ...) \
	MM_INVOKE( \
	MM_CONCAT(MM_APPLY_, MM_NARG(__VA_ARGS__)), \
	(MACRONAME, C, __VA_ARGS__) \
	)

#define MM_APPLY_COMMA(MACRONAME, C, ...) \
	MM_INVOKE( \
	MM_CONCAT(MM_APPLY_COMMA_, MM_NARG(__VA_ARGS__)), \
	(MACRONAME, C, __VA_ARGS__) \
	)

#define MM_APPLY_COMMA_1(MACRONAME, C, A1) \
	MM_INVOKE_B(MACRONAME, (C, A1))

#define MM_APPLY_COMMA_2(MACRONAME, C, A1, A2) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_1(MACRONAME, C, A2)

#define MM_APPLY_COMMA_3(MACRONAME, C, A1, A2, A3) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_2(MACRONAME, C, A2, A3)

#define MM_APPLY_COMMA_4(MACRONAME, C, A1, A2, A3, A4) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_3(MACRONAME, C, A2, A3, A4)

#define MM_APPLY_COMMA_5(MACRONAME, C, A1, A2, A3, A4, A5) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_4(MACRONAME, C, A2, A3, A4, A5)

#define MM_APPLY_COMMA_6(MACRONAME, C, A1, A2, A3, A4, A5, A6) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_5(MACRONAME, C, A2, A3, A4, A5, A6)

#define MM_APPLY_COMMA_7(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_6(MACRONAME, C, A2, A3, A4, A5, A6, A7)

#define MM_APPLY_COMMA_8(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_7(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8)

#define MM_APPLY_COMMA_9(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_8(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9)

#define MM_APPLY_COMMA_10(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_9(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10)

#define MM_APPLY_COMMA_11(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_10(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11)

#define MM_APPLY_COMMA_12(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_11(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12)

#define MM_APPLY_COMMA_13(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_12(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13)

#define MM_APPLY_COMMA_14(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_13(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14)

#define MM_APPLY_COMMA_15(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_14(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15)

#define MM_APPLY_COMMA_16(MACRONAME, C, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16) \
	MM_INVOKE_B(MACRONAME, (C, A1)), \
	MM_APPLY_COMMA_15(MACRONAME, C, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16)

LINPHONE_END_NAMESPACE

#endif // ifndef _MAGIC_MACROS_H_
