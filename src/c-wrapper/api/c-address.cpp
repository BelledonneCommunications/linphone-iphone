/*
 * c-address.cpp
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

#include "address/address.h"
#include "c-wrapper/c-wrapper.h"

// =============================================================================

L_DECLARE_C_CLONABLE_OBJECT_IMPL(Address);

using namespace std;

// =============================================================================

LinphoneAddress *linphone_address_new (const char *address) {
	LinphonePrivate::Address *cppPtr = new LinphonePrivate::Address(L_C_TO_STRING(address));
	if (!cppPtr->isValid()) {
		delete cppPtr;
		return nullptr;
	}

	LinphoneAddress *object = L_INIT(Address);
	L_SET_CPP_PTR_FROM_C_OBJECT(object, cppPtr);

	return object;
}

LinphoneAddress *linphone_address_clone (const LinphoneAddress *address) {
	return reinterpret_cast<LinphoneAddress *>(belle_sip_object_clone(BELLE_SIP_OBJECT(address)));
}

LinphoneAddress *linphone_address_ref (LinphoneAddress *address) {
	belle_sip_object_ref(address);
	return address;
}

void linphone_address_unref (LinphoneAddress *address) {
	belle_sip_object_unref(address);
}

const char *linphone_address_get_scheme (const LinphoneAddress *address) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(address)->getScheme());
}

const char *linphone_address_get_display_name (const LinphoneAddress *address) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(address)->getDisplayName());
}

LinphoneStatus linphone_address_set_display_name (LinphoneAddress *address, const char *display_name) {
	return !L_GET_CPP_PTR_FROM_C_OBJECT(address)->setDisplayName(L_C_TO_STRING(display_name));
}

const char *linphone_address_get_username (const LinphoneAddress *address) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(address)->getUsername());
}

LinphoneStatus linphone_address_set_username (LinphoneAddress *address, const char *username) {
	return !L_GET_CPP_PTR_FROM_C_OBJECT(address)->setUsername(L_C_TO_STRING(username));
}

const char *linphone_address_get_domain (const LinphoneAddress *address) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(address)->getDomain());
}

LinphoneStatus linphone_address_set_domain (LinphoneAddress *address, const char *domain) {
	return !L_GET_CPP_PTR_FROM_C_OBJECT(address)->setDomain(L_C_TO_STRING(domain));
}

int linphone_address_get_port (const LinphoneAddress *address) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(address)->getPort();
}

LinphoneStatus linphone_address_set_port (LinphoneAddress *address, int port) {
	return !L_GET_CPP_PTR_FROM_C_OBJECT(address)->setPort(port);
}

LinphoneTransportType linphone_address_get_transport (const LinphoneAddress *address) {
	return static_cast<LinphoneTransportType>(L_GET_CPP_PTR_FROM_C_OBJECT(address)->getTransport());
}

LinphoneStatus linphone_address_set_transport (LinphoneAddress *address, LinphoneTransportType transport) {
	return !L_GET_CPP_PTR_FROM_C_OBJECT(address)->setTransport(static_cast<LinphonePrivate::Transport>(transport));
}

bool_t linphone_address_get_secure (const LinphoneAddress *address) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(address)->getSecure();
}

void linphone_address_set_secure (LinphoneAddress *address, bool_t enabled) {
	L_GET_CPP_PTR_FROM_C_OBJECT(address)->setSecure(!!enabled);
}

bool_t linphone_address_is_sip (const LinphoneAddress *address) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(address)->isSip();
}

const char *linphone_address_get_method_param (const LinphoneAddress *address) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(address)->getMethodParam());
}

void linphone_address_set_method_param (LinphoneAddress *address, const char *method_param) {
	L_GET_CPP_PTR_FROM_C_OBJECT(address)->setMethodParam(L_C_TO_STRING(method_param));
}

const char *linphone_address_get_password (const LinphoneAddress *address) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(address)->getPassword());
}

void linphone_address_set_password (LinphoneAddress *address, const char *password) {
	L_GET_CPP_PTR_FROM_C_OBJECT(address)->setPassword(L_C_TO_STRING(password));
}

void linphone_address_clean (LinphoneAddress *address) {
	L_GET_CPP_PTR_FROM_C_OBJECT(address)->clean();
}

char *linphone_address_as_string (const LinphoneAddress *address) {
	return bctbx_strdup(L_GET_CPP_PTR_FROM_C_OBJECT(address)->asString().c_str());
}

char *linphone_address_as_string_uri_only (const LinphoneAddress *address) {
	return bctbx_strdup(L_GET_CPP_PTR_FROM_C_OBJECT(address)->asStringUriOnly().c_str());
}

bool_t linphone_address_weak_equal (const LinphoneAddress *address1, const LinphoneAddress *address2) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(address1)->weakEqual(*L_GET_CPP_PTR_FROM_C_OBJECT(address2));
}

bool_t linphone_address_equal (const LinphoneAddress *address1, const LinphoneAddress *address2) {
	return *L_GET_CPP_PTR_FROM_C_OBJECT(address1) == *L_GET_CPP_PTR_FROM_C_OBJECT(address2);
}

const char *linphone_address_get_header (const LinphoneAddress *address, const char *header_name) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(address)->getHeaderValue(L_C_TO_STRING(header_name)));
}

void linphone_address_set_header (LinphoneAddress *address, const char *header_name, const char *header_value) {
	L_GET_CPP_PTR_FROM_C_OBJECT(address)->setHeader(L_C_TO_STRING(header_name), L_C_TO_STRING(header_value));
}

bool_t linphone_address_has_param (const LinphoneAddress *address, const char *param_name) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(address)->hasParam(L_C_TO_STRING(param_name));
}

const char *linphone_address_get_param (const LinphoneAddress *address, const char *param_name) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(address)->getParamValue(L_C_TO_STRING(param_name)));
}

void linphone_address_set_param (LinphoneAddress *address, const char *param_name, const char *param_value) {
	L_GET_CPP_PTR_FROM_C_OBJECT(address)->setParam(L_C_TO_STRING(param_name), L_C_TO_STRING(param_value));
}

void linphone_address_set_params (LinphoneAddress *address, const char *params) {
	L_GET_CPP_PTR_FROM_C_OBJECT(address)->setParams(L_C_TO_STRING(params));
}

bool_t linphone_address_has_uri_param (const LinphoneAddress *address, const char *uri_param_name) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(address)->hasUriParam(L_C_TO_STRING(uri_param_name));
}

const char *linphone_address_get_uri_param (const LinphoneAddress *address, const char *uri_param_name) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(address)->getUriParamValue(L_C_TO_STRING(uri_param_name)));
}

void linphone_address_set_uri_param (LinphoneAddress *address, const char *uri_param_name, const char *uri_param_value) {
	L_GET_CPP_PTR_FROM_C_OBJECT(address)->setUriParam(L_C_TO_STRING(uri_param_name), L_C_TO_STRING(uri_param_value));
}

void linphone_address_set_uri_params (LinphoneAddress *address, const char *params) {
	L_GET_CPP_PTR_FROM_C_OBJECT(address)->setUriParams(L_C_TO_STRING(params));
}

void linphone_address_remove_uri_param (LinphoneAddress *address, const char *uri_param_name) {
	L_GET_CPP_PTR_FROM_C_OBJECT(address)->removeUriParam(L_C_TO_STRING(uri_param_name));
}

void linphone_address_destroy (LinphoneAddress *address) {
	belle_sip_object_unref(address);
}

bool_t linphone_address_is_secure (const LinphoneAddress *address) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(address)->getSecure();
}
