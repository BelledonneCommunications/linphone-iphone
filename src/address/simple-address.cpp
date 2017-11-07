/*
 * simple-address.cpp
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

#include "linphone/utils/utils.h"

#include "simple-address-p.h"
#include "c-wrapper/c-wrapper.h"
#include "logger/logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

SimpleAddress::SimpleAddress (const string &address) : ClonableObject(*new SimpleAddressPrivate) {
	L_D();
	Address tmpAddress(address);
	if (tmpAddress.isValid()) {
		d->scheme = tmpAddress.getScheme();
		d->username = tmpAddress.getUsername();
		d->domain = tmpAddress.getDomain();
	}
}

SimpleAddress::SimpleAddress (const SimpleAddress &src) : ClonableObject(*new SimpleAddressPrivate) {
	L_D();
	d->scheme = src.getScheme();
	d->username = src.getUsername();
	d->domain = src.getDomain();
}

SimpleAddress::SimpleAddress (const Address &src) : ClonableObject(*new SimpleAddressPrivate) {
	L_D();
	d->scheme = src.getScheme();
	d->username = src.getUsername();
	d->domain = src.getDomain();
}

SimpleAddress &SimpleAddress::operator= (const SimpleAddress &src) {
	L_D();
	if (this != &src) {
		d->scheme = src.getScheme();
		d->username = src.getUsername();
		d->domain = src.getDomain();
	}
	return *this;
}

bool SimpleAddress::operator== (const SimpleAddress &address) const {
	return asString() == address.asString();
}

bool SimpleAddress::operator!= (const SimpleAddress &address) const {
	return !(*this == address);
}

bool SimpleAddress::operator< (const SimpleAddress &address) const {
	return asString() < address.asString();
}

const string &SimpleAddress::getScheme () const {
	L_D();
	return d->scheme;
}

const string &SimpleAddress::getUsername () const {
	L_D();
	return d->username;
}

bool SimpleAddress::setUsername (const string &username) {
	L_D();
	d->username = username;
	return true;
}

const string &SimpleAddress::getDomain () const {
	L_D();
	return d->domain;
}

bool SimpleAddress::setDomain (const string &domain) {
	L_D();
	d->domain = domain;
	return true;
}

string SimpleAddress::asString () const {
	Address tmpAddress(*this);
	return tmpAddress.asString();
}

string SimpleAddress::asStringUriOnly () const {
	Address tmpAddress(*this);
	return tmpAddress.asStringUriOnly();
}

LINPHONE_END_NAMESPACE
