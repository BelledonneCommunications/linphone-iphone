/*
 * wrapper.h
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

#ifndef _WRAPPER_H_
#define _WRAPPER_H_

#include <utility>

#include "object/object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class Wrapper {
public:
	template<typename T>
	static decltype(std::declval<T>().getPrivate()) getPrivate (T *object) {
		return object->getPrivate();
	}

private:
	Wrapper ();

	L_DISABLE_COPY(Wrapper);
};

LINPHONE_END_NAMESPACE

#define L_GET_PRIVATE(OBJECT) \
	LINPHONE_NAMESPACE::Wrapper::getPrivate(OBJECT)

#endif // ifndef _WRAPPER_H_
