/*
 * c-event-log.cpp
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "linphone/api/c-address.h"

#include "c-wrapper/c-tools.h"

#include "address/address.h"

// =============================================================================

using namespace std;

L_DECLARE_C_CLONABLE_STRUCT_IMPL(Address, address);

LinphoneAddress *linphone_address_new (const char *address) {
	LINPHONE_NAMESPACE::Address *cppPtr = new LINPHONE_NAMESPACE::Address(L_C_TO_STRING(address));
	if (!cppPtr->isValid()) {
		delete cppPtr;
		return nullptr;
	}

	LinphoneAddress *object = _linphone_address_init();
	object->cppPtr = cppPtr;
	return object;
}

LinphoneAddress *linphone_address_clone (const LinphoneAddress *address) {
	return (LinphoneAddress *)belle_sip_object_clone(BELLE_SIP_OBJECT(address));
}

LinphoneAddress *linphone_address_ref (LinphoneAddress *address) {
	belle_sip_object_ref(address);
	return address;
}

void linphone_address_unref (LinphoneAddress *address) {
	belle_sip_object_unref(address);
}

const char *linphone_address_get_scheme (const LinphoneAddress *address) {
	return L_STRING_TO_C(address->cppPtr->getScheme());
}

const char *linphone_address_get_display_name (const LinphoneAddress *address) {
	return L_STRING_TO_C(address->cppPtr->getDisplayName());
}

LinphoneStatus linphone_address_set_display_name (LinphoneAddress *address, const char *display_name) {
	return !address->cppPtr->setDisplayName(L_C_TO_STRING(display_name));
}

const char *linphone_address_get_username (const LinphoneAddress *address) {
	return L_STRING_TO_C(address->cppPtr->getUsername());
}

LinphoneStatus linphone_address_set_username (LinphoneAddress *address, const char *username) {
	return !address->cppPtr->setUsername(L_C_TO_STRING(username));
}

const char *linphone_address_get_domain (const LinphoneAddress *address) {
	return L_STRING_TO_C(address->cppPtr->getDomain());
}

LinphoneStatus linphone_address_set_domain (LinphoneAddress *address, const char *domain) {
	return !address->cppPtr->setDomain(L_C_TO_STRING(domain));
}

int linphone_address_get_port (const LinphoneAddress *address) {
	return address->cppPtr->getPort();
}

LinphoneStatus linphone_address_set_port (LinphoneAddress *address, int port) {
	return !address->cppPtr->setPort(port);
}

LinphoneTransportType linphone_address_get_transport (const LinphoneAddress *address) {
	return static_cast<LinphoneTransportType>(address->cppPtr->getTransport());
}

LinphoneStatus linphone_address_set_transport (LinphoneAddress *address, LinphoneTransportType transport) {
	return !address->cppPtr->setTransport(static_cast<LINPHONE_NAMESPACE::Transport>(transport));
}

bool_t linphone_address_get_secure (const LinphoneAddress *address) {
	return address->cppPtr->getSecure();
}

void linphone_address_set_secure (LinphoneAddress *address, bool_t enabled) {
	address->cppPtr->setSecure(enabled);
}

bool_t linphone_address_is_sip (const LinphoneAddress *address) {
	return address->cppPtr->isSip();
}

const char *linphone_address_get_method_param (const LinphoneAddress *address) {
	return L_STRING_TO_C(address->cppPtr->getMethodParam());
}

void linphone_address_set_method_param (LinphoneAddress *address, const char *method_param) {
	address->cppPtr->setMethodParam(L_C_TO_STRING(method_param));
}

const char *linphone_address_get_password (const LinphoneAddress *address) {
	return L_STRING_TO_C(address->cppPtr->getPassword());
}

void linphone_address_set_password (LinphoneAddress *address, const char *password) {
	address->cppPtr->setPassword(L_C_TO_STRING(password));
}

void linphone_address_clean (LinphoneAddress *address) {
	address->cppPtr->clean();
}

char *linphone_address_as_string (const LinphoneAddress *address) {
	return ms_strdup(address->cppPtr->asString().c_str());
}

char *linphone_address_as_string_uri_only (const LinphoneAddress *address) {
	return ms_strdup(address->cppPtr->asStringUriOnly().c_str());
}

bool_t linphone_address_weak_equal (const LinphoneAddress *address1, const LinphoneAddress *address2) {
	return address1->cppPtr->weakEqual(*address2->cppPtr);
}

bool_t linphone_address_equal (const LinphoneAddress *address1, const LinphoneAddress *address2) {
	return *address1->cppPtr == *address2->cppPtr;
}

const char *linphone_address_get_header (const LinphoneAddress *address, const char *header_name) {
	return L_STRING_TO_C(address->cppPtr->getHeaderValue(L_C_TO_STRING(header_name)));
}

void linphone_address_set_header (LinphoneAddress *address, const char *header_name, const char *header_value) {
	address->cppPtr->setHeader(L_C_TO_STRING(header_name), L_C_TO_STRING(header_value));
}

bool_t linphone_address_has_param (const LinphoneAddress *address, const char *param_name) {
	return address->cppPtr->hasParam(L_C_TO_STRING(param_name));
}

const char *linphone_address_get_param (const LinphoneAddress *address, const char *param_name) {
	return L_STRING_TO_C(address->cppPtr->getParamValue(L_C_TO_STRING(param_name)));
}

void linphone_address_set_param (LinphoneAddress *address, const char *param_name, const char *param_value) {
	address->cppPtr->setParam(L_C_TO_STRING(param_name), L_C_TO_STRING(param_value));
}

void linphone_address_set_params (LinphoneAddress *address, const char *params) {
	address->cppPtr->setParams(L_C_TO_STRING(params));
}

bool_t linphone_address_has_uri_param (const LinphoneAddress *address, const char *uri_param_name) {
	return address->cppPtr->hasUriParam(L_C_TO_STRING(uri_param_name));
}

const char *linphone_address_get_uri_param (const LinphoneAddress *address, const char *uri_param_name) {
	return L_STRING_TO_C(address->cppPtr->getUriParamValue(L_C_TO_STRING(uri_param_name)));
}

void linphone_address_set_uri_param (LinphoneAddress *address, const char *uri_param_name, const char *uri_param_value) {
	address->cppPtr->setUriParam(L_C_TO_STRING(uri_param_name), L_C_TO_STRING(uri_param_value));
}

void linphone_address_set_uri_params (LinphoneAddress *address, const char *params) {
	address->cppPtr->setUriParams(L_C_TO_STRING(params));
}

void linphone_address_destroy (LinphoneAddress *address) {
	belle_sip_object_unref(address);
}

bool_t linphone_address_is_secure (const LinphoneAddress *address) {
	return address->cppPtr->getSecure();
}
