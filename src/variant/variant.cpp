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

#include "variant.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class VariantPrivate {
public:
	union Value {
		int i;
		unsigned int ui;
		long l;
		unsigned long ul;
		long long ll;
		unsigned long long ull;
		bool b;
		double d;
		float f;
	};

	Variant::Type type = Variant::Invalid;
	Value value = {};
};

Variant::Variant () {
	// Nothing. Construct an invalid invariant.
}

Variant::Variant (Type type) {
	// TODO.
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

Variant::Variant (unsigned int value) : Variant(Int) {
	L_D(Variant);
	d->value.ui = value;
}

Variant::Variant (long value) : Variant(Int) {
	L_D(Variant);
	d->value.l = value;
}

Variant::Variant (unsigned long value) : Variant(Int) {
	L_D(Variant);
	d->value.ul = value;
}

Variant::Variant (long long value) : Variant(Int) {
	L_D(Variant);
	d->value.ll = value;
}

Variant::Variant (unsigned long long value) : Variant(Int) {
	L_D(Variant);
	d->value.ull = value;
}

Variant::Variant (bool value) : Variant(Int) {
	L_D(Variant);
	d->value.b = value;
}

Variant::Variant (double value) : Variant(Int) {
	L_D(Variant);
	d->value.d = value;
}

Variant::Variant (float value) : Variant(Int) {
	L_D(Variant);
	d->value.f = value;
}

Variant::Variant (const std::string &value) {
	// TODO.
}

Variant::~Variant () {
	// TODO.
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
	// TODO.
	return false;
}

void Variant::clear () {
	// TODO.
}

void Variant::swap (const Variant &variant) {
	// TODO.
}

// -----------------------------------------------------------------------------

void Variant::getValue (int type, void *value, bool *soFarSoGood) {
	if (type <= 0 || type >= MaxDefaultTypes)
		return; // Unable to get value.

	// TODO.
}

LINPHONE_END_NAMESPACE
