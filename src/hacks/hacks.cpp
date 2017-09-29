/*
 * hacks.cpp
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

#include <sstream>

#include <belle-sip/headers.h>

#include "hacks.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

bool Hacks::contactHasParam(const string &contact, const string &paramName) {
	// This is very ugly!!! The handling of Contact headers and addresses is a real
	// crap that really needs to be reworked. Meanwhile, we cannot get the params on the
	// remote contact address and need to forge and parse a contact header.
	ostringstream os;
	os << "Contact: " << contact;
	belle_sip_header_contact_t *contactHeader = belle_sip_header_contact_parse(os.str().c_str());
	bool result = belle_sip_parameters_has_parameter(BELLE_SIP_PARAMETERS(contactHeader), paramName.c_str());
	belle_sip_object_unref(contactHeader);
	return result;
}

LINPHONE_END_NAMESPACE
