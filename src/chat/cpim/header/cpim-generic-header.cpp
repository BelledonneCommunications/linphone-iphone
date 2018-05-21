/*
 * cpim-generic-header.cpp
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

#include <set>

#include "linphone/utils/utils.h"

#include "chat/cpim/parser/cpim-parser.h"
#include "cpim-header-p.h"

#include "cpim-generic-header.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class Cpim::GenericHeaderPrivate : public HeaderPrivate {
public:
	GenericHeaderPrivate () : parameters(make_shared<list<pair<string, string>>>()) {}

	string name;
	string value;
	shared_ptr<list<pair<string, string>>> parameters;
};

Cpim::GenericHeader::GenericHeader () : Header(*new GenericHeaderPrivate) {}

Cpim::GenericHeader::GenericHeader (string name, string value, string parameters) : GenericHeader() {
	setName(name);
	setValue(value);

	for (const auto &parameter : Utils::split(parameters, ';')) {
		size_t equalIndex = parameter.find('=');
		if (equalIndex != string::npos)
			addParameter(parameter.substr(0, equalIndex), parameter.substr(equalIndex + 1));
	}
}

string Cpim::GenericHeader::getName () const {
	L_D();
	return d->name;
}

void Cpim::GenericHeader::setName (const string &name) {
	L_D();

	static const set<string> reserved = {
		"From", "To", "cc", "DateTime", "Subject", "NS", "Require"
	};

	if (reserved.find(name) == reserved.end())
		d->name = name;
}

string Cpim::GenericHeader::getValue () const {
	L_D();
	return d->value;
}

void Cpim::GenericHeader::setValue (const string &value) {
	L_D();
	d->value = value;
}

Cpim::GenericHeader::ParameterList Cpim::GenericHeader::getParameters () const {
	L_D();
	return d->parameters;
}

void Cpim::GenericHeader::addParameter (const string &key, const string &value) {
	L_D();
	d->parameters->push_back(make_pair(key, value));
}

void Cpim::GenericHeader::removeParameter (const string &key, const string &value) {
	L_D();
	d->parameters->remove(make_pair(key, value));
}

string Cpim::GenericHeader::asString () const {
	L_D();

	string parameters;
	for (const auto &parameter : *d->parameters)
		parameters += ";" + parameter.first + "=" + parameter.second;

	return d->name + ":" + parameters + " " + getValue() + "\r\n";
}

LINPHONE_END_NAMESPACE
