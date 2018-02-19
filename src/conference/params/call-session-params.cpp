/*
 * call-session-params.cpp
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

#include "call-session-params-p.h"

#include "call-session-params.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

void CallSessionParamsPrivate::clone (const CallSessionParamsPrivate *src) {
	sessionName = src->sessionName;
	privacy = src->privacy;
	inConference = src->inConference;
	internalCallUpdate = src->internalCallUpdate;
	noUserConsent = src->noUserConsent;
	/* The management of the custom headers is not optimal. We copy everything while ref counting would be more efficient. */
	if (customHeaders) {
		sal_custom_header_free(customHeaders);
		customHeaders = nullptr;
	}
	if (src->customHeaders)
		customHeaders = sal_custom_header_clone(src->customHeaders);
	customContactParameters = src->customContactParameters;
	referer = src->referer;
}

// -----------------------------------------------------------------------------

SalCustomHeader * CallSessionParamsPrivate::getCustomHeaders () const {
	return customHeaders;
}

void CallSessionParamsPrivate::setCustomHeaders (const SalCustomHeader *ch) {
	if (customHeaders) {
		sal_custom_header_free(customHeaders);
		customHeaders = nullptr;
	}
	if (ch)
		customHeaders = sal_custom_header_clone(ch);
}

// =============================================================================

CallSessionParams::CallSessionParams () : ClonableObject(*new CallSessionParamsPrivate) {}

CallSessionParams::CallSessionParams (CallSessionParamsPrivate &p) : ClonableObject(p) {}

CallSessionParams::CallSessionParams (const CallSessionParams &other)
	: ClonableObject(*new CallSessionParamsPrivate) {
	L_D();
	d->clone(other.getPrivate());
}

CallSessionParams::~CallSessionParams () {
	L_D();
	if (d->customHeaders)
		sal_custom_header_free(d->customHeaders);
}

CallSessionParams &CallSessionParams::operator= (const CallSessionParams &other) {
	L_D();
	if (this != &other)
		d->clone(other.getPrivate());
	return *this;
}

// -----------------------------------------------------------------------------

void CallSessionParams::initDefault (const std::shared_ptr<Core> &core) {
	L_D();
	d->inConference = false;
	d->privacy = LinphonePrivacyDefault;
}

// -----------------------------------------------------------------------------

const string& CallSessionParams::getSessionName () const {
	L_D();
	return d->sessionName;
}

void CallSessionParams::setSessionName (const string &sessionName) {
	L_D();
	d->sessionName = sessionName;
}

// -----------------------------------------------------------------------------

LinphonePrivacyMask CallSessionParams::getPrivacy () const {
	L_D();
	return d->privacy;
}

void CallSessionParams::setPrivacy (LinphonePrivacyMask privacy) {
	L_D();
	d->privacy = privacy;
}

// -----------------------------------------------------------------------------

void CallSessionParams::addCustomHeader (const string &headerName, const string &headerValue) {
	L_D();
	d->customHeaders = sal_custom_header_append(d->customHeaders, headerName.c_str(), headerValue.c_str());
}

void CallSessionParams::clearCustomHeaders () {
	L_D();
	d->setCustomHeaders(nullptr);
}

const char * CallSessionParams::getCustomHeader (const string &headerName) const {
	L_D();
	return sal_custom_header_find(d->customHeaders, headerName.c_str());
}

// -----------------------------------------------------------------------------

void CallSessionParams::addCustomContactParameter (const std::string &paramName, const std::string &paramValue) {
	L_D();
	auto it = d->customContactParameters.find(paramName);
	if (it != d->customContactParameters.end())
		d->customContactParameters.erase(it);
	pair<string, string> param(paramName, paramValue);
	d->customContactParameters.insert(param);
}

void CallSessionParams::clearCustomContactParameters () {
	L_D();
	d->customContactParameters.clear();
}

std::string CallSessionParams::getCustomContactParameter (const std::string &paramName) const {
	L_D();
	auto it = d->customContactParameters.find(paramName);
	if (it == d->customContactParameters.end())
		return "";
	return it->second;
}

LINPHONE_END_NAMESPACE
