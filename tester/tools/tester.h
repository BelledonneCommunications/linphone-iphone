/*
 * tester.h
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

#ifndef _L_TESTER_H_
#define _L_TESTER_H_

#include <utility>

#include "linphone/utils/utils.h"

// =============================================================================

// -----------------------------------------------------------------------------
// Internal.
// -----------------------------------------------------------------------------

LINPHONE_BEGIN_NAMESPACE

class Tester {
public:
	Tester () = delete;

	template<typename T>
	static constexpr decltype(std::declval<T>().getPrivate()) getPrivate (T *object) {
		return object->getPrivate();
	}

private:
	L_DISABLE_COPY(Tester);
};

LINPHONE_END_NAMESPACE

// -----------------------------------------------------------------------------
// Public.
// -----------------------------------------------------------------------------

#define L_GET_PRIVATE(OBJECT) \
	LinphonePrivate::Tester::getPrivate(LinphonePrivate::Utils::getPtr(OBJECT))

#endif // ifndef _L_TESTER_H_
