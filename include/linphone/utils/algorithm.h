/*
 * algorithm.h
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

#ifndef _L_ALGORITHM_H_
#define _L_ALGORITHM_H_

#include <algorithm>

#include "general.h"

// =============================================================================

// NOTE: Maybe use https://github.com/ericniebler/range-v3 one day?

LINPHONE_BEGIN_NAMESPACE

template<typename T, typename Value>
typename T::const_iterator find (const T &container, const Value &value) {
	return std::find(container.cbegin(), container.cend(), value);
}

template<typename T, typename Value>
typename T::iterator find (T &container, const Value &value) {
	return std::find(container.begin(), container.end(), value);
}

template<typename T, typename Predicate>
typename T::const_iterator findIf (const T &container, Predicate predicate) {
	return std::find_if(container.cbegin(), container.cend(), predicate);
}

template<typename T, typename Predicate>
typename T::iterator findIf (T &container, Predicate predicate) {
	return std::find_if(container.begin(), container.end(), predicate);
}

template<typename T, typename Value>
bool removeFirst (T &container, const Value &value) {
	auto it = find(container, value);
	if (it != container.end()) {
		container.erase(it);
		return true;
	}
	return false;
}

template<typename T, typename Predicate>
void removeIf (T &container, Predicate predicate) {
	std::remove_if(container.begin(), container.end(), predicate);
}

LINPHONE_END_NAMESPACE

#endif // ifndef _L_ALGORITHM_H_
