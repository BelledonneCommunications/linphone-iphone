/*
 * property-container.cpp
 * Copyright (C) 2010-2017 Belledonne Communications SARL
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

#include <unordered_map>

#include "property-container.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

class PropertyContainerPrivate {
public:
	unordered_map<string, Variant> properties;
};

// -----------------------------------------------------------------------------

PropertyContainer::PropertyContainer () : mPrivate(new PropertyContainerPrivate) {}

/*
 * Empty copy constructor. Don't change this pattern.
 * PropertyContainer is an Entity component, not a simple structure.
 * An Entity is UNIQUE.
 */
PropertyContainer::PropertyContainer (const PropertyContainer &) : mPrivate(new PropertyContainerPrivate) {}

PropertyContainer::~PropertyContainer () {
	delete mPrivate;
}

PropertyContainer &PropertyContainer::operator= (const PropertyContainer &) {
	return *this;
}

Variant PropertyContainer::getProperty (const string &name) const {
	L_D();
	auto it = d->properties.find(name);
	return it == d->properties.cend() ? Variant() : it->second;
}

void PropertyContainer::setProperty (const string &name, const Variant &value) {
	L_D();
	d->properties[name] = value;
}

void PropertyContainer::setProperty (const string &name, Variant &&value) {
	L_D();
	d->properties[name] = move(value);
}

LINPHONE_END_NAMESPACE
