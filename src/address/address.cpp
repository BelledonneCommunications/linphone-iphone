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

#include "object/clonable-object-p.h"

#include "address.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class AddressPrivate : public ClonableObjectPrivate {
	// TODO.
};

// -----------------------------------------------------------------------------

Address::Address (const string &address) : ClonableObject(*new AddressPrivate) {
	// TODO.
}

Address::Address (const Address &src) : ClonableObject(*new AddressPrivate) {
	// TODO.
}

Address &Address::operator= (const Address &src) {
	// TODO.
	return *this;
}

Address::operator bool () const {
	// TODO.
	return false;
}

bool Address::operator== (const Address &address) const {
	// TODO.
	return false;
}

string Address::getScheme () const {
	// TODO.
	return "";
}

string Address::getDisplayName () const {
	// TODO.
	return "";
}

bool setDisplayName (const string &displayName) {
	// TODO.
	return false;
}

string Address::getUsername () const {
	// TODO.
	return "";
}

bool setUsername (const string &username) {
	// TODO.
	return false;
}

string Address::getDomain () const {
	// TODO.
	return "";
}

bool setDomain (const string &host) {
	// TODO.
	return false;
}

int Address::getPort () const {
	// TODO.
	return 0;
}

bool Address::setPort (int port) {
	// TODO.
	return false;
}

Transport Address::getTransport () const {
	// TODO.
	return Transport::Dtls;
}

bool setTransport (Transport transport) {
	// TODO.
	return false;
}

bool Address::getSecure () const {
	// TODO.
	return false;
}

void Address::setSecure (bool enabled) {
	// TODO.
}

bool Address::isSip () const {
	// TODO.
	return false;
}

string Address::getMethodParam () const {
	// TODO.
	return "";
}

void Address::setMethodParam (const string &method) {
	// TODO.
}

string Address::getPassword () const {
	// TODO.
	return "";
}

void Address::setPassword (const string &passwd) {
	// TODO.
}

void clean () {
	// TODO.
}

string Address::asString () const {
	// TODO.
	return "";
}

string Address::asStringUriOnly () const {
	// TODO.
	return "";
}

bool Address::equal (const shared_ptr<const Address> &address) const {
	// TODO.
	return false;
}

bool Address::weakEqual (const shared_ptr<const Address> &a2) const {
	// TODO.
	return false;
}

string Address::getHeaderValue (const string &headerName) const {
	// TODO.
	return "";
}

void addHeader (const string &headerName, const string &headerValue) {
	// TODO.
}

void removeHeader (const string &headerName) {
	// TODO.
}

LINPHONE_END_NAMESPACE
