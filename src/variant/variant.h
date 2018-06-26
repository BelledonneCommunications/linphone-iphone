/*
 * variant.h
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

#ifndef _L_VARIANT_H_
#define _L_VARIANT_H_

#include <string>

#include "linphone/utils/general.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

#define L_DECLARE_VARIANT_TYPES(FUNC) \
	FUNC(int, Int, 1) \
	FUNC(unsigned int, UnsignedInt, 2) \
	FUNC(short, Short, 3) \
	FUNC(unsigned short, UnsignedShort, 4) \
	FUNC(long, Long, 5) \
	FUNC(unsigned long, UnsignedLong, 6) \
	FUNC(long long, LongLong, 7) \
	FUNC(unsigned long long, UnsignedLongLong, 8) \
	FUNC(char, Char, 9) \
	FUNC(bool, Bool, 10) \
	FUNC(double, Double, 11) \
	FUNC(float, Float, 12) \
	FUNC(std::string, String, 13) \
	FUNC(void *, Generic, 14)

#define L_DECLARE_VARIANT_ENUM_TYPE(TYPE, NAME, ID) NAME = ID,
#define L_DECLARE_VARIANT_TRAIT_TYPE(TYPE, NAME, ID) \
	template<> \
	struct Variant::IdOfType<TYPE> { \
		static const int id = ID; \
	};

class VariantPrivate;

class LINPHONE_PUBLIC Variant {
public:
	enum Type {
		Invalid = 0,
		L_DECLARE_VARIANT_TYPES(L_DECLARE_VARIANT_ENUM_TYPE)
		MaxDefaultTypes
	};

	Variant ();
	Variant (Type type);

	Variant (const Variant &other);
	Variant (Variant &&other);

	Variant (int value);
	Variant (unsigned int value);
	Variant (short value);
	Variant (unsigned short value);
	Variant (long value);
	Variant (unsigned long value);
	Variant (long long value);
	Variant (unsigned long long value);
	Variant (char value);
	Variant (bool value);
	Variant (double value);
	Variant (float value);
	Variant (const std::string &value);

	// void* constructor. Must be explicitly called.
	template<typename T, typename = typename std::enable_if<std::is_same<T, void *>::value>>
	Variant (T value) : Variant (Variant::createGeneric(value)) {}

	~Variant ();

	Variant &operator= (const Variant &other);
	Variant &operator= (Variant &&other);

	template<typename T>
	void setValue (const T &value) {
		// Yeah, I'm crazy but it's useful to avoid code duplication.
		new(this) Variant(value);
	}

	template<typename T>
	T getValue (bool *soFarSoGood = nullptr) const {
		constexpr int id = IdOfType<T>::id;
		static_assert(id != Invalid, "Unable to get value of unsupported type.");

		T value;

		bool ok;
		getValue(id, static_cast<void *>(&value), &ok);
		if (soFarSoGood)
			*soFarSoGood = ok;

		return value;
	}

	bool isValid () const;

	void clear ();
	void swap (const Variant &variant);

private:
	template<typename T>
	struct IdOfType {
		static const int id = Invalid;
	};

	void getValue (int type, void *value, bool *soFarSoGood) const;

	static Variant createGeneric (void *value);

	VariantPrivate *mPrivate = nullptr;

	L_DECLARE_PRIVATE(Variant);
};

L_DECLARE_VARIANT_TYPES(L_DECLARE_VARIANT_TRAIT_TYPE);

#undef L_DECLARE_VARIANT_TYPES
#undef L_DECLARE_VARIANT_ENUM_TYPE
#undef L_DECLARE_VARIANT_TRAIT_TYPE

LINPHONE_END_NAMESPACE

#endif // ifndef _L_VARIANT_H_
