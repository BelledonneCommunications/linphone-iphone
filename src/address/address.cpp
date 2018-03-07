/*
 * address.cpp
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

#include "address-p.h"
#include "address/identity-address.h"
#include "c-wrapper/c-wrapper.h"
#include "containers/lru-cache.h"
#include "logger/logger.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

namespace {
	class SalAddressWrap {
	public:
		explicit SalAddressWrap (SalAddress *salAddress = nullptr) : mSalAddress(salAddress) {}

		SalAddressWrap (const SalAddressWrap &other) : mSalAddress(other.mSalAddress) {
			if (mSalAddress)
				sal_address_ref(mSalAddress);
		}

		SalAddressWrap (SalAddressWrap &&other) : mSalAddress(other.mSalAddress) {
			other.mSalAddress = nullptr;
		}

		~SalAddressWrap () {
			if (mSalAddress)
				sal_address_unref(mSalAddress);
		}

		const SalAddress *get () {
			return mSalAddress;
		}

	private:
		SalAddress *mSalAddress;
	};
	LruCache<string, SalAddressWrap> addressesCache;
}

static SalAddress *getSalAddressFromCache (const string &uri) {
	SalAddressWrap *wrap = addressesCache[uri];
	if (wrap)
		return sal_address_clone(wrap->get());

	SalAddress *address = sal_address_new(L_STRING_TO_C(uri));
	if (address) {
		addressesCache.insert(uri, SalAddressWrap(address));
		return sal_address_clone(address);
	}

	return nullptr;
}

// -----------------------------------------------------------------------------

void AddressPrivate::setInternalAddress (const SalAddress *addr) {
	if (internalAddress)
		sal_address_unref(internalAddress);
	internalAddress = sal_address_clone(addr);
}

void AddressPrivate::clearSipAddressesCache () {
	addressesCache.clear();
}

// -----------------------------------------------------------------------------

Address::Address (const string &address) : ClonableObject(*new AddressPrivate) {
	L_D();

	if (!(d->internalAddress = getSalAddressFromCache(address))) {
		lWarning() << "Cannot create Address, bad uri [" << address << "]";
	}
}

Address::Address (const IdentityAddress &identityAddress) : ClonableObject(*new AddressPrivate) {
	L_D();

	const string &username = identityAddress.getUsername();
	if (username.empty())
		return;
	const string &domain = identityAddress.getDomain();
	if (domain.empty())
		return;

	string uri = identityAddress.getScheme() + ":" + username + "@" + (
		domain.find(':') != string::npos ? "[" + domain + "]" : domain
	);

	if (identityAddress.hasGruu())
		uri += ";gr=" + identityAddress.getGruu();

	d->internalAddress = getSalAddressFromCache(uri);
}

Address::Address (const Address &other) : ClonableObject(*new AddressPrivate) {
	L_D();
	SalAddress *salAddress = other.getPrivate()->internalAddress;
	if (salAddress)
		d->internalAddress = sal_address_clone(salAddress);
}

Address::~Address () {
	L_D();
	if (d->internalAddress)
		sal_address_destroy(d->internalAddress);
}

Address &Address::operator= (const Address &other) {
	L_D();
	if (this != &other) {
		if (d->internalAddress)
			sal_address_destroy(d->internalAddress);
		SalAddress *salAddress = other.getPrivate()->internalAddress;
		d->internalAddress = salAddress ? sal_address_clone(salAddress) : nullptr;
	}

	return *this;
}

bool Address::operator== (const Address &other) const {
	return asString() == other.asString();
}

bool Address::operator!= (const Address &other) const {
	return !(*this == other);
}

bool Address::operator< (const Address &other) const {
	return asString() < other.asString();
}

bool Address::isValid () const {
	L_D();
	return !!d->internalAddress;
}

const string &Address::getScheme () const {
	L_D();

	if (!d->internalAddress)
		return Utils::getEmptyConstRefObject<string>();

	string scheme(L_C_TO_STRING(sal_address_get_scheme(d->internalAddress)));
	if (scheme != d->cache.scheme)
		d->cache.scheme = scheme;
	return d->cache.scheme;
}

const string &Address::getDisplayName () const {
	L_D();

	if (!d->internalAddress)
		return Utils::getEmptyConstRefObject<string>();

	string displayName(L_C_TO_STRING(sal_address_get_display_name(d->internalAddress)));
	if (displayName != d->cache.displayName)
		d->cache.displayName = displayName;
	return d->cache.displayName;
}

bool Address::setDisplayName (const string &displayName) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_set_display_name(d->internalAddress, L_STRING_TO_C(displayName));
	return true;
}

const string &Address::getUsername () const {
	L_D();

	if (!d->internalAddress)
		return Utils::getEmptyConstRefObject<string>();

	string username(L_C_TO_STRING(sal_address_get_username(d->internalAddress)));
	if (username != d->cache.username)
		d->cache.username = username;
	return d->cache.username;
}

bool Address::setUsername (const string &username) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_set_username(d->internalAddress, L_STRING_TO_C(username));
	return true;
}

const string &Address::getDomain () const {
	L_D();

	if (!d->internalAddress)
		return Utils::getEmptyConstRefObject<string>();

	string domain(L_C_TO_STRING(sal_address_get_domain(d->internalAddress)));
	if (domain != d->cache.domain)
		d->cache.domain = domain;
	return d->cache.domain;
}

bool Address::setDomain (const string &domain) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_set_domain(d->internalAddress, L_STRING_TO_C(domain));
	return true;
}

int Address::getPort () const {
	L_D();
	return d->internalAddress ? sal_address_get_port(d->internalAddress) : 0;
}

bool Address::setPort (int port) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_set_port(d->internalAddress, port);
	return true;
}

Transport Address::getTransport () const {
	L_D();
	return d->internalAddress ? static_cast<Transport>(sal_address_get_transport(d->internalAddress)) : Transport::Udp;
}

bool Address::setTransport (Transport transport) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_set_transport(d->internalAddress, static_cast<SalTransport>(transport));
	return true;
}

bool Address::getSecure () const {
	L_D();
	return d->internalAddress && sal_address_is_secure(d->internalAddress);
}

bool Address::setSecure (bool enabled) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_set_secure(d->internalAddress, enabled);
	return true;
}

bool Address::isSip () const {
	L_D();
	return d->internalAddress && sal_address_is_sip(d->internalAddress);
}

const string &Address::getMethodParam () const {
	L_D();

	if (!d->internalAddress)
		return Utils::getEmptyConstRefObject<string>();

	string methodParam(L_C_TO_STRING(sal_address_get_method_param(d->internalAddress)));
	if (methodParam != d->cache.methodParam)
		d->cache.methodParam = methodParam;
	return d->cache.methodParam;
}

bool Address::setMethodParam (const string &methodParam) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_set_method_param(d->internalAddress, L_STRING_TO_C(methodParam));
	return true;
}

const string &Address::getPassword () const {
	L_D();

	if (!d->internalAddress)
		return Utils::getEmptyConstRefObject<string>();

	string password(L_C_TO_STRING(sal_address_get_password(d->internalAddress)));
	if (password != d->cache.password)
		d->cache.password = password;
	return d->cache.password;
}

bool Address::setPassword (const string &password) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_set_password(d->internalAddress, L_STRING_TO_C(password));
	return true;
}

bool Address::clean () {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_clean(d->internalAddress);
	return true;
}

string Address::asString () const {
	L_D();

	if (!d->internalAddress)
		return "";

	char *buf = sal_address_as_string(d->internalAddress);
	string out = buf;
	ms_free(buf);
	return out;
}

string Address::asStringUriOnly () const {
	L_D();

	if (!d->internalAddress)
		return "";

	char *buf = sal_address_as_string_uri_only(d->internalAddress);
	string out = buf;
	ms_free(buf);
	return out;
}

bool Address::weakEqual (const Address &address) const {
	return getUsername() == address.getUsername() &&
				 getDomain() == address.getDomain() &&
				 getPort() == address.getPort();
}

const string &Address::getHeaderValue (const string &headerName) const {
	L_D();

	if (d->internalAddress) {
		const char *value = sal_address_get_header(d->internalAddress, L_STRING_TO_C(headerName));
		if (value) {
			d->cache.headers[headerName] = value;
			return d->cache.headers[headerName];
		}
	}

	return Utils::getEmptyConstRefObject<string>();
}

bool Address::setHeader (const string &headerName, const string &headerValue) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_set_header(d->internalAddress, L_STRING_TO_C(headerName), L_STRING_TO_C(headerValue));
	return true;
}

bool Address::hasParam (const string &paramName) const {
	L_D();
	return d->internalAddress && !!sal_address_has_param(d->internalAddress, L_STRING_TO_C(paramName));
}

const string &Address::getParamValue (const string &paramName) const {
	L_D();

	if (d->internalAddress) {
		const char *value = sal_address_get_param(d->internalAddress, L_STRING_TO_C(paramName));
		if (value) {
			d->cache.params[paramName] = value;
			return d->cache.params[paramName];
		}
	}

	return Utils::getEmptyConstRefObject<string>();
}

bool Address::setParam (const string &paramName, const string &paramValue) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_set_param(d->internalAddress, L_STRING_TO_C(paramName), L_STRING_TO_C(paramValue));
	return true;
}

bool Address::setParams (const string &params) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_set_params(d->internalAddress, L_STRING_TO_C(params));
	return true;
}

bool Address::hasUriParam (const string &uriParamName) const {
	L_D();
	return d->internalAddress && !!sal_address_has_uri_param(d->internalAddress, L_STRING_TO_C(uriParamName));
}

const string &Address::getUriParamValue (const string &uriParamName) const {
	L_D();

	if (d->internalAddress) {
		const char *value = sal_address_get_uri_param(d->internalAddress, L_STRING_TO_C(uriParamName));
		if (value) {
			d->cache.uriParams[uriParamName] = value;
			return d->cache.uriParams[uriParamName];
		}
	}

	return Utils::getEmptyConstRefObject<string>();
}

bool Address::setUriParam (const string &uriParamName, const string &uriParamValue) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_set_uri_param(d->internalAddress, L_STRING_TO_C(uriParamName), L_STRING_TO_C(uriParamValue));
	return true;
}

bool Address::setUriParams (const string &uriParams) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_set_uri_params(d->internalAddress, L_STRING_TO_C(uriParams));
	return true;
}

bool Address::removeUriParam (const string &uriParamName) {
	L_D();

	if (!d->internalAddress)
		return false;

	sal_address_remove_uri_param(d->internalAddress, L_STRING_TO_C(uriParamName));
	return true;
}

LINPHONE_END_NAMESPACE
