/*
 * enum-mask.h
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

#ifndef _L_ENUM_MASK_H_
#define _L_ENUM_MASK_H_

#include <initializer_list>

#include "linphone/utils/general.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

template<typename T>
class EnumMask {
	static_assert(std::is_enum<T>::value, "EnumMask must be used with enum type. Logic no?");
	static_assert(sizeof(T) <= sizeof(int), "EnumMask supports only int, short or char values.");

public:
	// EnumMask's type. Can be int or unsigned int.
	// See: http://en.cppreference.com/w/cpp/types/underlying_type
	typedef typename std::conditional<
		std::is_signed<typename std::underlying_type<T>::type>::value, int, unsigned int
	>::type StorageType;

	constexpr EnumMask (int mask = 0) : mMask(mask) {}
	constexpr EnumMask (T value) : mMask(StorageType(value)) {}
	constexpr EnumMask (std::initializer_list<T> mask) : mMask(init(mask.begin(), mask.end())) {}

	constexpr operator StorageType () const {
		return mMask;
	}

	constexpr bool isSet (T value) const {
		return isSet(StorageType(value));
	}

	inline EnumMask &set (T value) {
		*this |= value;
		return *this;
	}

	inline EnumMask &unset (T value) {
		*this &= ~value;
		return *this;
	}

	constexpr bool operator! () const {
		return !mMask;
	}

	inline EnumMask &operator&= (int mask) {
		mMask &= mask;
		return *this;
	}

	inline EnumMask &operator&= (unsigned int mask) {
		mMask &= mask;
		return *this;
	}

	inline EnumMask &operator&= (T mask) {
		mMask &= StorageType(mask);
		return *this;
	}

	inline EnumMask &operator|= (EnumMask mask) {
		mMask |= mask.mMask;
		return *this;
	}

	inline EnumMask &operator|= (T mask) {
		mMask |= StorageType(mask);
		return *this;
	}

	inline EnumMask &operator^= (EnumMask mask) {
		mMask ^= mask.mMask;
		return *this;
	}

	inline EnumMask &operator^= (T mask) {
		mMask ^= StorageType(mask);
		return *this;
	}

	constexpr EnumMask operator& (int mask) const {
		return mMask & mask;
	}

	constexpr EnumMask operator& (unsigned int mask) const {
		return mMask & mask;
	}

	constexpr EnumMask operator& (T mask) const {
		return mMask & StorageType(mask);
	}

	constexpr EnumMask operator| (EnumMask mask) const {
		return mMask | mask.mMask;
	}

	constexpr EnumMask operator| (T mask) const {
		return mMask | StorageType(mask);
	}

	constexpr EnumMask operator^ (EnumMask mask) const {
		return mMask ^ mask.mMask;
	}

	constexpr EnumMask operator^ (T mask) const {
		return mMask ^ StorageType(mask);
	}

	constexpr EnumMask operator~ () const {
		return ~mMask;
	}

private:
	constexpr bool isSet (StorageType value) const {
		return (mMask & value) == value && (value || mMask == 0);
	}

// On CentOs 7 GCC 4.8.5 have issue with array-bounds.
#if __GNUC__ == 4 && __GNUC_MINOR__ == 8 && __GNUC_PATCHLEVEL__ == 5
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Warray-bounds"
#endif

	static constexpr StorageType init (
		typename std::initializer_list<T>::const_iterator begin,
		typename std::initializer_list<T>::const_iterator end
	) {
		return begin != end ? (StorageType(*begin) | init(begin + 1, end)) : StorageType(0);
	}

#if __GNUC__ == 4 && __GNUC_MINOR__ == 8 && __GNUC_PATCHLEVEL__ == 5
	#pragma GCC diagnostic pop
#endif

	StorageType mMask;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_ENUM_MASK_H_
