/*
 * address.cpp
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

// From coreapi.
#include "private.h"

#include "logger/logger.h"
#include "object/clonable-object-p.h"

#include "address.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class AddressPrivate : public ClonableObjectPrivate {
public:
	SalAddress *internalAddress = nullptr;
};

// -----------------------------------------------------------------------------

Address::Address (const string &address) : ClonableObject(*new AddressPrivate) {
	L_D(Address);
	if (!address.empty() && !(d->internalAddress = sal_address_new(address.c_str())))
		lWarning() << "Cannot create address, bad uri [" << address << "].";
}

Address::Address (const Address &src) : ClonableObject(*new AddressPrivate) {
	L_D(Address);
	SalAddress *salAddress = src.getPrivate()->internalAddress;
	if (salAddress)
		d->internalAddress = sal_address_clone(salAddress);
}

Address::~Address () {
	L_D(Address);
	if (d->internalAddress)
		sal_address_destroy(d->internalAddress);
}

Address &Address::operator= (const Address &src) {
	L_D(Address);
	if (this != &src) {
		if (d->internalAddress)
			sal_address_destroy(d->internalAddress);
		SalAddress *salAddress = src.getPrivate()->internalAddress;
		d->internalAddress = salAddress ? sal_address_clone(salAddress) : nullptr;
	}

	return *this;
}

Address::operator bool () const {
	L_D(const Address);
	return static_cast<bool>(d->internalAddress);
}

bool Address::operator== (const Address &address) const {
	return equal(address);
}

string Address::getScheme () const {
	L_D(const Address);
	return d->internalAddress ? sal_address_get_scheme(d->internalAddress) : "";
}

string Address::getDisplayName () const {
	L_D(const Address);
	return d->internalAddress ? sal_address_get_display_name(d->internalAddress) : "";
}

bool Address::setDisplayName (const string &displayName) {
	L_D(const Address);

	if (!d->internalAddress)
		return false;

	sal_address_set_display_name(d->internalAddress, displayName.c_str());
	return true;
}

string Address::getUsername () const {
	L_D(const Address);
	return d->internalAddress ? sal_address_get_username(d->internalAddress) : "";
}

bool Address::setUsername (const string &username) {
	L_D(const Address);

	if (!d->internalAddress)
		return false;

	sal_address_set_username(d->internalAddress, username.c_str());
	return true;
}

string Address::getDomain () const {
	L_D(const Address);
	return d->internalAddress ? sal_address_get_domain(d->internalAddress) : "";
}

bool Address::setDomain (const string &domain) {
	L_D(const Address);

	if (!d->internalAddress)
		return false;

	sal_address_set_domain(d->internalAddress, domain.c_str());
	return true;
}

int Address::getPort () const {
	L_D(const Address);
	return d->internalAddress ? sal_address_get_port(d->internalAddress) : 0;
}

bool Address::setPort (int port) {
	L_D(const Address);

	if (!d->internalAddress)
		return false;

	sal_address_set_port(d->internalAddress, port);
	return true;
}

Transport Address::getTransport () const {
	L_D(const Address);
	return d->internalAddress ? static_cast<Transport>(sal_address_get_transport(d->internalAddress)) : Transport::Udp;
}

bool Address::setTransport (Transport transport) {
	L_D(const Address);

	if (!d->internalAddress)
		return false;

	sal_address_set_transport(d->internalAddress, static_cast<SalTransport>(transport));
	return true;
}

bool Address::getSecure () const {
	L_D(const Address);
	return d->internalAddress ? sal_address_is_secure(d->internalAddress) : false;
}

bool Address::setSecure (bool enabled) {
	L_D(const Address);

	if (!d->internalAddress)
		return false;

	sal_address_set_secure(d->internalAddress, enabled);
	return true;
}

bool Address::isSip () const {
	L_D(const Address);
	return d->internalAddress ? sal_address_is_sip(d->internalAddress) : false;
}

string Address::getMethodParam () const {
	L_D(const Address);
	return d->internalAddress ? sal_address_get_method_param(d->internalAddress) : "";
}

bool Address::setMethodParam (const string &methodParam) {
	L_D(const Address);

	if (!d->internalAddress)
		return false;

	sal_address_set_method_param(d->internalAddress, methodParam.c_str());
	return true;
}

string Address::getPassword () const {
	L_D(const Address);
	return sal_address_get_password(d->internalAddress);
}

bool Address::setPassword (const string &password) {
	L_D(const Address);

	if (!d->internalAddress)
		return false;

	sal_address_set_password(d->internalAddress, password.c_str());
	return true;
}

bool Address::clean () {
	L_D(const Address);

	if (!d->internalAddress)
		return false;

	sal_address_clean(d->internalAddress);
	return true;
}

string Address::asString () const {
	L_D(const Address);

	if (!d->internalAddress)
		return "";

	char *buf = sal_address_as_string(d->internalAddress);
	string out = buf;
	ms_free(buf);
	return out;
}

string Address::asStringUriOnly () const {
	L_D(const Address);

	if (!d->internalAddress)
		return "";

	char *buf = sal_address_as_string_uri_only(d->internalAddress);
	string out = buf;
	ms_free(buf);
	return out;
}

bool Address::equal (const Address &address) const {
	return asString() == address.asString();
}

bool Address::weakEqual (const Address &address) const {
	return getUsername() == address.getUsername() &&
				 getDomain() == address.getDomain() &&
				 getPort() == address.getPort();
}

string Address::getHeaderValue (const string &headerName) const {
	L_D(const Address);
	return d->internalAddress ? sal_address_get_header(d->internalAddress, headerName.c_str()) : "";
}

bool Address::setHeader (const string &headerName, const string &headerValue) {
	L_D(const Address);

	if (!d->internalAddress)
		return false;

	sal_address_set_header(d->internalAddress, headerName.c_str(), headerValue.c_str());
	return true;
}

LINPHONE_END_NAMESPACE
