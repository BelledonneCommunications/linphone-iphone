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
	GenericHeaderPrivate () : parameters(make_shared<list<pair<string, string> > >()) {}

	string name;
	shared_ptr<list<pair<string, string> > > parameters;
};

Cpim::GenericHeader::GenericHeader () : Header(*new GenericHeaderPrivate) {}

string Cpim::GenericHeader::getName () const {
	L_D();
	return d->name;
}

bool Cpim::GenericHeader::setName (const string &name) {
	L_D();

	static const set<string> reserved = {
		"From", "To", "cc", "DateTime", "Subject", "NS", "Require"
	};

	if (
		reserved.find(name) != reserved.end() ||
		!Parser::getInstance()->headerNameIsValid(name)
	)
		return false;

	d->name = name;
	return true;
}

bool Cpim::GenericHeader::setValue (const string &value) {
	return Parser::getInstance()->headerValueIsValid(value) && Header::setValue(value);
}

Cpim::GenericHeader::ParameterList Cpim::GenericHeader::getParameters () const {
	L_D();
	return d->parameters;
}

bool Cpim::GenericHeader::addParameter (const string &key, const string &value) {
	L_D();

	if (!Parser::getInstance()->headerParameterIsValid(key + "=" + value))
		return false;

	d->parameters->push_back(make_pair(key, value));
	return true;
}

void Cpim::GenericHeader::removeParameter (const string &key, const string &value) {
	L_D();
	d->parameters->remove(make_pair(key, value));
}

bool Cpim::GenericHeader::isValid () const {
	L_D();
	return !d->name.empty() && !getValue().empty();
}

string Cpim::GenericHeader::asString () const {
	L_D();

	string parameters;
	for (const auto &parameter : *d->parameters)
		parameters += ";" + parameter.first + "=" + parameter.second;

	return d->name + ":" + parameters + " " + getValue() + "\r\n";
}

// -----------------------------------------------------------------------------

void Cpim::GenericHeader::force (const string &name, const string &value, const string &parameters) {
	L_D();

	// Set name/value.
	d->name = name;
	Header::setValue(value);

	// Parse and build parameters list.
	for (const auto &parameter : Utils::split(parameters, ';')) {
		size_t equalIndex = parameter.find('=');
		if (equalIndex != string::npos)
			d->parameters->push_back(make_pair(parameter.substr(0, equalIndex), parameter.substr(equalIndex + 1)));
	}
}

LINPHONE_END_NAMESPACE
