/*
 * gruu-address.cpp
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

#include "gruu-address-p.h"
#include "c-wrapper/c-wrapper.h"
#include "logger/logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

GruuAddress::GruuAddress (const string &address) : SimpleAddress(*new GruuAddressPrivate) {
	L_D();
	Address tmpAddress(address);
	if (tmpAddress.isValid()) {
		if (!tmpAddress.hasUriParam("gr"))
			return;
		SimpleAddress base(address);
		SimpleAddress::clone(base);
		d->urn = tmpAddress.getUriParamValue("gr");
		d->valid = true;
	}
}

GruuAddress::GruuAddress (const GruuAddress &src) : SimpleAddress(*new GruuAddressPrivate) {
	L_D();
	SimpleAddress::clone(src);
	d->urn = src.getPrivate()->urn;
	d->valid = src.getPrivate()->valid;
}

GruuAddress::GruuAddress (const Address &src) : SimpleAddress(*new GruuAddressPrivate) {
	L_D();
	if (src.isValid()) {
		if (!src.hasUriParam("gr"))
			return;
		SimpleAddress::clone(SimpleAddress(src));
		d->urn = src.getUriParamValue("gr");
		d->valid = true;
	}
}

GruuAddress &GruuAddress::operator= (const GruuAddress &src) {
	L_D();
	if (this != &src) {
		SimpleAddress::operator=(src);
		d->urn = src.getPrivate()->urn;
		d->valid = src.getPrivate()->valid;
	}
	return *this;
}

bool GruuAddress::operator== (const GruuAddress &address) const {
	return asString() == address.asString();
}

bool GruuAddress::operator!= (const GruuAddress &address) const {
	return !(*this == address);
}

bool GruuAddress::operator< (const GruuAddress &address) const {
	return asString() < address.asString();
}

bool GruuAddress::isValid () const {
	L_D();
	return d->valid;
}

string GruuAddress::getUrn () const {
	L_D();
	return d->urn;
}

void GruuAddress::setUrn (const string &urn) {
	L_D();
	d->urn = urn;
}

string GruuAddress::asString () const {
	Address tmpAddress(*this);
	return tmpAddress.asStringUriOnly();
}

LINPHONE_END_NAMESPACE
