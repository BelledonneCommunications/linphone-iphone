/*
 * variant.cpp
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

#include "linphone/utils/utils.h"

#include "variant.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

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

	~VariantPrivate () {
		deleteString();
	}

	inline int getType () const {
		return type;
	}

	inline void setType (int newType) {
		L_ASSERT(newType >= Variant::Invalid && newType != Variant::MaxDefaultTypes);

		if (newType != Variant::String)
			deleteString();
		else if (type != Variant::String)
			value.str = new string();

		type = newType;
	}

	Value value = {};

private:
	inline void deleteString () {
		if (type == Variant::String)
			delete value.str;
	}

	// Integer, because type can be a custom type.
	int type = Variant::Invalid;
};

// -----------------------------------------------------------------------------

Variant::Variant () {
	// Private can exist. (placement new)
	if (!mPrivate)
		mPrivate = new VariantPrivate();
}

Variant::Variant (Type type) : Variant() {
	L_D();
	d->setType(type);
}

Variant::Variant (const Variant &other) {
	// Don't call placement new.
	L_ASSERT(!mPrivate);
	mPrivate = new VariantPrivate();

	L_D();

	int type = other.getPrivate()->getType();
	d->setType(type);

	const VariantPrivate::Value &value = other.getPrivate()->value;
	if (type == String)
		*d->value.str = *value.str;
	else
		d->value = value;
}

Variant::Variant (Variant &&other) {
	// Don't call placement new.
	L_ASSERT(!mPrivate);
	::swap(mPrivate, other.mPrivate);
}

Variant::Variant (int value) : Variant(Int) {
	L_D();
	d->value.i = value;
}

Variant::Variant (unsigned int value) : Variant(UnsignedInt) {
	L_D();
	d->value.ui = value;
}

Variant::Variant (short value) : Variant(Short) {
	L_D();
	d->value.s = value;
}

Variant::Variant (unsigned short value) : Variant(UnsignedShort) {
	L_D();
	d->value.us = value;
}

Variant::Variant (long value) : Variant(Long) {
	L_D();
	d->value.l = value;
}

Variant::Variant (unsigned long value) : Variant(UnsignedLong) {
	L_D();
	d->value.ul = value;
}

Variant::Variant (long long value) : Variant(LongLong) {
	L_D();
	d->value.ll = value;
}

Variant::Variant (unsigned long long value) : Variant(UnsignedLongLong) {
	L_D();
	d->value.ull = value;
}

Variant::Variant (char value) : Variant(Char) {
	L_D();
	d->value.c = value;
}

Variant::Variant (bool value) : Variant(Bool) {
	L_D();
	d->value.b = value;
}

Variant::Variant (double value) : Variant(Double) {
	L_D();
	d->value.d = value;
}

Variant::Variant (float value) : Variant(Float) {
	L_D();
	d->value.f = value;
}

Variant::Variant (const string &value) : Variant(String) {
	L_D();
	*d->value.str = value;
}

Variant::~Variant () {
	L_D();

	// Note: Private data can be stolen by copy operator (r-value).
	// It can be null, but it useless to test it.
	delete d;
}

Variant &Variant::operator= (const Variant &other) {
	L_D();

	if (this != &other) {
		// Update type.
		int type = other.getPrivate()->getType();
		d->setType(type);

		// Update value.
		const VariantPrivate::Value &value = other.getPrivate()->value;
		if (type == String)
			*d->value.str = *value.str;
		else
			d->value = value;
	}

	return *this;
}

Variant &Variant::operator= (Variant &&other) {
	::swap(mPrivate, other.mPrivate);
	return *this;
}

bool Variant::isValid () const {
	L_D();
	return d->getType() != Invalid;
}

void Variant::clear () {
	L_D();
	d->setType(Invalid);
}

void Variant::swap (const Variant &other) {
	// TODO.
}

// -----------------------------------------------------------------------------
// Number helpers.
// -----------------------------------------------------------------------------

static inline long long getAssumedNumber (const VariantPrivate &p) {
	const int type = p.getType();
	L_ASSERT(type > Variant::Invalid && type < Variant::MaxDefaultTypes);

	switch (static_cast<Variant::Type>(type)) {
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
			return static_cast<long long>(p.value.d);
		case Variant::Float:
			return static_cast<long long>(p.value.f);

		default:
			L_ASSERT(false);
	}

	return 0;
}

static inline unsigned long long getAssumedUnsignedNumber (const VariantPrivate &p) {
	const int type = p.getType();
	L_ASSERT(type > Variant::Invalid && type < Variant::MaxDefaultTypes);

	switch (static_cast<Variant::Type>(type)) {
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
	const int type = p.getType();
	L_ASSERT(type > Variant::Invalid && type < Variant::MaxDefaultTypes);

	switch (static_cast<Variant::Type>(type)) {
		case Variant::Int:
		case Variant::Short:
		case Variant::Long:
		case Variant::LongLong:
		case Variant::Char:
		case Variant::Double:
		case Variant::Float:
			return getAssumedNumber(p);

		case Variant::UnsignedInt:
		case Variant::UnsignedShort:
		case Variant::UnsignedLong:
		case Variant::UnsignedLongLong:
			return static_cast<long long>(getAssumedUnsignedNumber(p));

		case Variant::Bool:
			return static_cast<long long>(p.value.b);

		case Variant::String:
			return Utils::stoll(*p.value.str);

		case Variant::Generic:
			return static_cast<long long>(!!p.value.g);

		default:
			*soFarSoGood = false;
	}

	return 0;
}

static inline unsigned long long getValueAsUnsignedNumber (const VariantPrivate &p, bool *soFarSoGood) {
	const int type = p.getType();
	L_ASSERT(type > Variant::Invalid && type < Variant::MaxDefaultTypes);

	switch (static_cast<Variant::Type>(type)) {
		case Variant::Int:
		case Variant::Short:
		case Variant::Long:
		case Variant::LongLong:
		case Variant::Char:
		case Variant::Double:
		case Variant::Float:
			return static_cast<unsigned long long>(getAssumedNumber(p));

		case Variant::UnsignedInt:
		case Variant::UnsignedShort:
		case Variant::UnsignedLong:
		case Variant::UnsignedLongLong:
			return getAssumedUnsignedNumber(p);

		case Variant::Bool:
			return static_cast<unsigned long long>(p.value.b);

		case Variant::String:
			return Utils::stoull(*p.value.str);

		case Variant::Generic:
			return static_cast<unsigned long long>(!!p.value.g);

		default:
			*soFarSoGood = false;
	}

	return 0;
}

// -----------------------------------------------------------------------------
// Specific conversions.
// -----------------------------------------------------------------------------

static inline bool getValueAsBool (const VariantPrivate &p, bool *soFarSoGood) {
	const int type = p.getType();
	L_ASSERT(type > Variant::Invalid && type < Variant::MaxDefaultTypes);

	switch (static_cast<Variant::Type>(type)) {
		case Variant::Int:
		case Variant::Short:
		case Variant::Long:
		case Variant::LongLong:
		case Variant::Char:
		case Variant::Double:
		case Variant::Float:
			return!!getAssumedNumber(p);

		case Variant::UnsignedInt:
		case Variant::UnsignedShort:
		case Variant::UnsignedLong:
		case Variant::UnsignedLongLong:
			return !!getAssumedUnsignedNumber(p);

		case Variant::Bool:
			return p.value.b;

		case Variant::String:
			return Utils::stob(*p.value.str);

		case Variant::Generic:
			return !!p.value.g;

		default:
			*soFarSoGood = false;
			break;
	}

	return false;
}

static inline double getValueAsDouble (const VariantPrivate &p, bool *soFarSoGood) {
	const int type = p.getType();
	L_ASSERT(type > Variant::Invalid && type < Variant::MaxDefaultTypes);

	switch (static_cast<Variant::Type>(type)) {
		case Variant::Int:
		case Variant::Short:
		case Variant::Long:
		case Variant::LongLong:
		case Variant::Char:
			return static_cast<double>(getAssumedNumber(p));

		case Variant::UnsignedInt:
		case Variant::UnsignedShort:
		case Variant::UnsignedLong:
		case Variant::UnsignedLongLong:
			return static_cast<double>(getAssumedUnsignedNumber(p));

		case Variant::Float:
			return static_cast<double>(p.value.f);

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
	const int type = p.getType();
	L_ASSERT(type > Variant::Invalid && type < Variant::MaxDefaultTypes);

	switch (static_cast<Variant::Type>(type)) {
		case Variant::Int:
		case Variant::Short:
		case Variant::Long:
		case Variant::LongLong:
		case Variant::Char:
			return static_cast<float>(getAssumedNumber(p));

		case Variant::UnsignedInt:
		case Variant::UnsignedShort:
		case Variant::UnsignedLong:
		case Variant::UnsignedLongLong:
			return static_cast<float>(getAssumedUnsignedNumber(p));

		case Variant::Float:
			return p.value.f;

		case Variant::Double:
			return static_cast<float>(p.value.d);

		case Variant::Bool:
			return static_cast<float>(p.value.b);

		case Variant::String:
			return Utils::stof(*p.value.str);

		case Variant::Generic:
			return static_cast<float>(!!p.value.g);

		default:
			*soFarSoGood = false;
			break;
	}

	return 0.f;
}

static inline string getValueAsString (const VariantPrivate &p, bool *soFarSoGood) {
	const int type = p.getType();
	L_ASSERT(type > Variant::Invalid && type < Variant::MaxDefaultTypes);

	switch (static_cast<Variant::Type>(type)) {
		case Variant::Int:
		case Variant::Short:
		case Variant::Long:
		case Variant::LongLong:
			return Utils::toString(getAssumedNumber(p));

		case Variant::UnsignedInt:
		case Variant::UnsignedShort:
		case Variant::UnsignedLong:
		case Variant::UnsignedLongLong:
			return Utils::toString(getAssumedUnsignedNumber(p));

		case Variant::Char:
			return string(1, p.value.c);

		case Variant::Bool:
			return string(p.value.b ? "true" : "false");

		case Variant::Double:
			return Utils::toString(p.value.d);

		case Variant::Float:
			return Utils::toString(p.value.f);

		case Variant::String:
			return *p.value.str;

		case Variant::Generic:
			return Utils::toString(p.value.g);

		default:
			*soFarSoGood = false;
	}

	return string();
}

static inline void *getValueAsGeneric (const VariantPrivate &p, bool *soFarSoGood) {
	if (p.getType() == Variant::Generic)
		return p.value.g;

	*soFarSoGood = false;
	return nullptr;
}

// -----------------------------------------------------------------------------

void Variant::getValue (int type, void *value, bool *soFarSoGood) const {
	L_ASSERT(type > Invalid && type != MaxDefaultTypes);

	L_D();

	*soFarSoGood = true;

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
			*static_cast<unsigned int *>(value) = static_cast<unsigned int>(getValueAsUnsignedNumber(*d, soFarSoGood));
			break;
		case UnsignedShort:
			*static_cast<unsigned short *>(value) = static_cast<unsigned short>(getValueAsUnsignedNumber(*d, soFarSoGood));
			break;
		case UnsignedLong:
			*static_cast<unsigned long *>(value) = static_cast<unsigned long>(getValueAsUnsignedNumber(*d, soFarSoGood));
			break;
		case UnsignedLongLong:
			*static_cast<unsigned long long *>(value) = getValueAsUnsignedNumber(*d, soFarSoGood);
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

Variant Variant::createGeneric (void *value) {
	Variant variant(Generic);
	variant.getPrivate()->value.g = value;
	return variant;
}

LINPHONE_END_NAMESPACE
