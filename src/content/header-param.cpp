/*
 * header-param.cpp
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

#include "header-param.h"
#include "object/clonable-object-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

class HeaderParamPrivate : public ClonableObjectPrivate {
public:
	string name;
	string value;
};

// -----------------------------------------------------------------------------

HeaderParam::HeaderParam (const string &param) : ClonableObject(*new HeaderParamPrivate) {
	size_t pos = param.find("=");
	size_t end = param.length();

	if (pos == string::npos) {
		setName(param);
	} else {
		setName(param.substr(0, pos));
		setValue(param.substr(pos + 1, end - (pos + 1)));
	}
}

HeaderParam::HeaderParam (const string &name, const string &value) : ClonableObject(*new HeaderParamPrivate) {
	setName(name);
	setValue(value);
}

HeaderParam::HeaderParam (const HeaderParam &other) : HeaderParam(other.getName(), other.getValue()) {}

HeaderParam &HeaderParam::operator= (const HeaderParam &other) {
	if (this != &other) {
		setName(other.getName());
		setValue(other.getValue());
	}

	return *this;
}

bool HeaderParam::operator== (const HeaderParam &other) const {
	return getName() == other.getName() &&
		getValue() == other.getValue();
}

bool HeaderParam::operator!= (const HeaderParam &other) const {
	return !(*this == other);
}

const string &HeaderParam::getName () const {
	L_D();
	return d->name;
}

bool HeaderParam::setName (const string &name) {
	L_D();
	d->name = name;
	return true;
}

const string &HeaderParam::getValue () const {
	L_D();
	return d->value;
}

bool HeaderParam::setValue (const string &value) {
	L_D();
	d->value = value;
	return true;
}

string HeaderParam::asString () const {
	L_D();
	string asString = ";" + d->name;
	if (!d->value.empty())
		asString += "=" + d->value;
	return asString;
}

LINPHONE_END_NAMESPACE
