/*
 * identity-address.cpp
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

#include "identity-address-p.h"
#include "c-wrapper/c-wrapper.h"
#include "logger/logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

IdentityAddress::IdentityAddress (const string &address) : ClonableObject(*new IdentityAddressPrivate) {
	L_D();
	Address tmpAddress(address);
	if (tmpAddress.isValid() && ((tmpAddress.getScheme() == "sip") || (tmpAddress.getScheme() == "sips"))) {
		d->scheme = tmpAddress.getScheme();
		d->username = tmpAddress.getUsername();
		d->domain = tmpAddress.getDomain();
		d->gruu = tmpAddress.getUriParamValue("gr");
	}
}

IdentityAddress::IdentityAddress (const Address &address) : ClonableObject(*new IdentityAddressPrivate) {
	L_D();
	d->scheme = address.getScheme();
	d->username = address.getUsername();
	d->domain = address.getDomain();
	if (address.hasUriParam("gr"))
		d->gruu = address.getUriParamValue("gr");
}

IdentityAddress::IdentityAddress (const IdentityAddress &src) : ClonableObject(*new IdentityAddressPrivate) {
	L_D();
	d->scheme = src.getScheme();
	d->username = src.getUsername();
	d->domain = src.getDomain();
	d->gruu = src.getGruu();
}

IdentityAddress &IdentityAddress::operator= (const IdentityAddress &src) {
	L_D();
	if (this != &src) {
		d->scheme = src.getScheme();
		d->username = src.getUsername();
		d->domain = src.getDomain();
		d->gruu = src.getGruu();
	}
	return *this;
}

bool IdentityAddress::operator== (const IdentityAddress &address) const {
	return asString() == address.asString();
}

bool IdentityAddress::operator!= (const IdentityAddress &address) const {
	return !(*this == address);
}

bool IdentityAddress::operator< (const IdentityAddress &address) const {
	return asString() < address.asString();
}

bool IdentityAddress::isValid () const {
	Address tmpAddress(*this);
	return tmpAddress.isValid();
}

const string &IdentityAddress::getScheme () const {
	L_D();
	return d->scheme;
}

const string &IdentityAddress::getUsername () const {
	L_D();
	return d->username;
}

bool IdentityAddress::setUsername (const string &username) {
	L_D();
	d->username = username;
	return true;
}

const string &IdentityAddress::getDomain () const {
	L_D();
	return d->domain;
}

bool IdentityAddress::setDomain (const string &domain) {
	L_D();
	d->domain = domain;
	return true;
}

bool IdentityAddress::hasGruu () const {
	L_D();
	return !d->gruu.empty();
}

const string &IdentityAddress::getGruu () const {
	L_D();
	return d->gruu;
}

bool IdentityAddress::setGruu (const string &gruu) {
	L_D();
	d->gruu = gruu;
	return true;
}

string IdentityAddress::asString () const {
	Address tmpAddress(*this);
	return tmpAddress.asStringUriOnly();
}

LINPHONE_END_NAMESPACE
