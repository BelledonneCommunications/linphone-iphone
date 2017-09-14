/*
 * variant.cpp
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY {} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "linphone/utils/utils.h"

#include "variant.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class VariantPrivate {
public:
	union Value {
		int i;
		unsigned int ui;
		short s;
		unsigned short us;
		long l;
		unsigned long ul;
		long long ll;
		unsigned long long ull;
		char c;
		bool b;
		double d;
		float f;
		string *str;
		void *g;
	};

	// Integer, because type can be a custom type.
	int type = Variant::Invalid;
	Value value = {};
};

Variant::Variant () {
	if (!mPrivate)
		mPrivate = new VariantPrivate();
}

Variant::Variant (Type type) : Variant() {
	if (type != String)
		return;

	L_D(Variant);

	if (!d->value.str)
		d->value.str = new string();
}

Variant::Variant (const Variant &src) {
	// TODO.
}

Variant::Variant (Variant &&src) {
	// TODO.
}

Variant::Variant (int value) : Variant(Int) {
	L_D(Variant);
	d->value.i = value;
}

Variant::Variant (unsigned int value) : Variant(UnsignedInt) {
	L_D(Variant);
	d->value.ui = value;
}

Variant::Variant (short value) : Variant(Short) {
	L_D(Variant);
	d->value.s = value;
}

Variant::Variant (unsigned short value) : Variant(UnsignedShort) {
	L_D(Variant);
	d->value.us = value;
}

Variant::Variant (long value) : Variant(Long) {
	L_D(Variant);
	d->value.l = value;
}

Variant::Variant (unsigned long value) : Variant(UnsignedLong) {
	L_D(Variant);
	d->value.ul = value;
}

Variant::Variant (long long value) : Variant(LongLong) {
	L_D(Variant);
	d->value.ll = value;
}

Variant::Variant (unsigned long long value) : Variant(UnsignedLongLong) {
	L_D(Variant);
	d->value.ull = value;
}

Variant::Variant (char value) : Variant(Char) {
	L_D(Variant);
	d->value.c = value;
}

Variant::Variant (bool value) : Variant(Bool) {
	L_D(Variant);
	d->value.b = value;
}

Variant::Variant (double value) : Variant(Double) {
	L_D(Variant);
	d->value.d = value;
}

Variant::Variant (float value) : Variant(Float) {
	L_D(Variant);
	d->value.f = value;
}

Variant::Variant (const std::string &value) : Variant(String) {
	L_D(Variant);
	*d->value.str = value;
}

Variant::~Variant () {
	L_D(Variant);
	if (d->type == String)
		delete d->value.str;

	delete mPrivate;
}

bool Variant::operator!= (const Variant &variant) const {
	// TODO.
	return false;
}

bool Variant::operator< (const Variant &variant) const {
	// TODO.
	return false;
}

bool Variant::operator<= (const Variant &variant) const {
	// TODO.
	return false;
}

Variant &Variant::operator= (const Variant &variant) {
	// TODO.
	return *this;
}

Variant &Variant::operator= (Variant &&variant) {
	// TODO.
	return *this;
}

bool Variant::operator== (const Variant &variant) const {
	// TODO.
	return false;
}

bool Variant::operator> (const Variant &variant) const {
	// TODO.
	return false;
}

bool Variant::operator>= (const Variant &variant) const {
	// TODO.
	return false;
}

bool Variant::isValid () const {
	L_D(const Variant);
	return d->type != Invalid;
}

void Variant::clear () {
	// TODO.
}

void Variant::swap (const Variant &variant) {
	// TODO.
}

// -----------------------------------------------------------------------------
// Number helpers.
// -----------------------------------------------------------------------------

static inline long long getAssumedNumber (const VariantPrivate &p) {
	L_ASSERT(p.type > Variant::Invalid && p.type < Variant::MaxDefaultTypes);

	switch (static_cast<Variant::Type>(p.type)) {
		case Variant::Int:
			return p.value.i;
		case Variant::Short:
			return p.value.s;
		case Variant::Long:
			return p.value.l;
		case Variant::LongLong:
			return p.value.ll;
		case Variant::Char:
			return p.value.c;
		case Variant::Double:
			return p.value.d;
		case Variant::Float:
			return p.value.f;

		default:
			L_ASSERT(false);
	}

	return 0;
}

static inline unsigned long long getAssumedUnsignedNumber (const VariantPrivate &p) {
	L_ASSERT(p.type > Variant::Invalid && p.type < Variant::MaxDefaultTypes);

	switch (static_cast<Variant::Type>(p.type)) {
		case Variant::UnsignedInt:
			return p.value.ui;
		case Variant::UnsignedShort:
			return p.value.us;
		case Variant::UnsignedLong:
			return p.value.ul;
		case Variant::UnsignedLongLong:
			return p.value.ull;

		default:
			L_ASSERT(false);
	}

	return 0;
}

// -----------------------------------------------------------------------------
// Number conversions.
// -----------------------------------------------------------------------------

static inline long long getValueAsNumber (const VariantPrivate &p, bool *soFarSoGood) {
	// TODO.
	return 0;
}

static inline unsigned long long getValueAsUnsignedNumber (const VariantPrivate &p, bool *soFarSoGood) {
	// TODO.
	return 0;
}

// -----------------------------------------------------------------------------
// Specific conversions.
// -----------------------------------------------------------------------------

static inline bool getValueAsBool (const VariantPrivate &p, bool *soFarSoGood) {
	L_ASSERT(p.type > Variant::Invalid && p.type < Variant::MaxDefaultTypes);

	*soFarSoGood = true;
	switch (static_cast<Variant::Type>(p.type)) {
		case Variant::Int:
		case Variant::Short:
		case Variant::Long:
		case Variant::LongLong:
		case Variant::Char:
		case Variant::Double:
		case Variant::Float:
			return static_cast<bool>(getAssumedNumber(p));

		case Variant::UnsignedInt:
		case Variant::UnsignedShort:
		case Variant::UnsignedLong:
		case Variant::UnsignedLongLong:
			return static_cast<bool>(getAssumedUnsignedNumber(p));

		case Variant::Bool:
			return p.value.b;

		case Variant::String:
			return Utils::stob(*p.value.str);

		case Variant::Generic:
			return static_cast<bool>(p.value.g);

		default:
			*soFarSoGood = false;
			break;
	}

	return false;
}

static inline double getValueAsDouble (const VariantPrivate &p, bool *soFarSoGood) {
	L_ASSERT(p.type > Variant::Invalid && p.type < Variant::MaxDefaultTypes);

	*soFarSoGood = true;
	switch (static_cast<Variant::Type>(p.type)) {
		case Variant::Int:
		case Variant::Short:
		case Variant::Long:
		case Variant::LongLong:
		case Variant::Char:
		case Variant::Float:
			return static_cast<double>(getAssumedNumber(p));

		case Variant::UnsignedInt:
		case Variant::UnsignedShort:
		case Variant::UnsignedLong:
		case Variant::UnsignedLongLong:
			return static_cast<double>(getAssumedUnsignedNumber(p));

		case Variant::Double:
			return p.value.d;

		case Variant::Bool:
			return static_cast<double>(p.value.b);

		case Variant::String:
			return Utils::stod(*p.value.str);

		case Variant::Generic:
			return static_cast<double>(!!p.value.g);

		default:
			*soFarSoGood = false;
			break;
	}

	return 0.0;
}

static inline float getValueAsFloat (const VariantPrivate &p, bool *soFarSoGood) {
	// TODO.
	return 0.f;
}

static inline float getValueAsString (const VariantPrivate &p, bool *soFarSoGood) {
	// TODO.
	return 0.f;
}

static inline void *getValueAsGeneric (const VariantPrivate &p, bool *soFarSoGood) {
	// TODO.
	return nullptr;
}

// -----------------------------------------------------------------------------

void Variant::getValue (int type, void *value, bool *soFarSoGood) const {
	L_ASSERT(type > Invalid && type != MaxDefaultTypes);

	L_D(const Variant);

	if (type > MaxDefaultTypes) {
		*soFarSoGood = false;
		// TODO: Unable to get value. It will be great to support custom types.
		return;
	}

	switch (static_cast<Type>(type)) {
		// Cast as Number.
		case Int:
			*static_cast<int *>(value) = static_cast<int>(getValueAsNumber(*d, soFarSoGood));
			break;
		case Short:
			*static_cast<short *>(value) = static_cast<short>(getValueAsNumber(*d, soFarSoGood));
			break;
		case Long:
			*static_cast<long *>(value) = static_cast<long>(getValueAsNumber(*d, soFarSoGood));
			break;
		case LongLong:
			*static_cast<long long *>(value) = getValueAsNumber(*d, soFarSoGood);
			break;
		case Char:
			*static_cast<char *>(value) = static_cast<char>(getValueAsNumber(*d, soFarSoGood));
			break;

		// Cast as Unsigned number.
		case UnsignedInt:
			*static_cast<unsigned int *>(value) = static_cast<unsigned int>(getValueAsNumber(*d, soFarSoGood));
			break;
		case UnsignedShort:
			*static_cast<unsigned short *>(value) = static_cast<unsigned short>(getValueAsNumber(*d, soFarSoGood));
			break;
		case UnsignedLong:
			*static_cast<unsigned long *>(value) = static_cast<unsigned long>(getValueAsNumber(*d, soFarSoGood));
			break;
		case UnsignedLongLong:
			*static_cast<unsigned long long *>(value) = getValueAsNumber(*d, soFarSoGood);
			break;

		// Cast as specific value.
		case Bool:
			*static_cast<bool *>(value) = getValueAsBool(*d, soFarSoGood);
			break;
		case Double:
			*static_cast<double *>(value) = getValueAsDouble(*d, soFarSoGood);
			break;
		case Float:
			*static_cast<float *>(value) = getValueAsFloat(*d, soFarSoGood);
			break;
		case String:
			*static_cast<string *>(value) = getValueAsString(*d, soFarSoGood);
			break;
		case Generic:
			*static_cast<void **>(value) = getValueAsGeneric(*d, soFarSoGood);
			break;

		case Invalid:
		case MaxDefaultTypes:
			*soFarSoGood = false;
			break;
	}
}

LINPHONE_END_NAMESPACE
