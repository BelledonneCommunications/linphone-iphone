/*
 * cpim-header.cpp
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

#include "cpim-header-p.h"

#include "cpim-header.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

Cpim::Header::Header (HeaderPrivate &p) : Object(p) {}

string Cpim::Header::getValue () const {
	L_D();
	return d->value;
}

bool Cpim::Header::setValue (const string &value) {
	L_D();
	d->value = value;
	return true;
}

string Cpim::Header::asString () const {
	L_D();
	return getName() + ": " + d->value + "\r\n";
}

LINPHONE_END_NAMESPACE
