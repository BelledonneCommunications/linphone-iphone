/*
 * c-dial-plan.cpp
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

#include "linphone/api/c-dial-plan.h"
#include "linphone/wrapper_utils.h"

#include "c-wrapper/c-wrapper.h"
#include "dial-plan/dial-plan.h"

// =============================================================================

using namespace std;

L_DECLARE_C_CLONABLE_OBJECT_IMPL(DialPlan);

LinphoneDialPlan *linphone_dial_plan_ref (LinphoneDialPlan *dp) {
	return reinterpret_cast<LinphoneDialPlan *>(belle_sip_object_ref(dp));
}

void linphone_dial_plan_unref (LinphoneDialPlan *dp) {
	belle_sip_object_unref(dp);
}

const char *linphone_dial_plan_get_country (const LinphoneDialPlan *dp) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(dp)->getCountry());
}

const char *linphone_dial_plan_get_iso_country_code (const LinphoneDialPlan *dp) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(dp)->getIsoCountryCode());
}

const char *linphone_dial_plan_get_country_calling_code (const LinphoneDialPlan *dp) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(dp)->getCountryCallingCode());
}

int linphone_dial_plan_get_national_number_length (const LinphoneDialPlan *dp) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(dp)->getNationalNumberLength();
}

const char *linphone_dial_plan_get_international_call_prefix (const LinphoneDialPlan *dp) {
	return L_STRING_TO_C(L_GET_CPP_PTR_FROM_C_OBJECT(dp)->getInternationalCallPrefix());
}

int linphone_dial_plan_lookup_ccc_from_e164 (const char *e164) {
	return LinphonePrivate::DialPlan::lookupCccFromE164(L_C_TO_STRING(e164));
}

int linphone_dial_plan_lookup_ccc_from_iso (const char *iso) {
	return LinphonePrivate::DialPlan::lookupCccFromIso(L_C_TO_STRING(iso));
}

const LinphoneDialPlan *linphone_dial_plan_by_ccc_as_int (int ccc) {
	const LinphonePrivate::DialPlan &dp = LinphonePrivate::DialPlan::findByCccAsInt(ccc);
	return L_GET_C_BACK_PTR(&dp);
}

const LinphoneDialPlan *linphone_dial_plan_by_ccc (const char *ccc) {
	const LinphonePrivate::DialPlan &dp = LinphonePrivate::DialPlan::findByCcc(L_C_TO_STRING(ccc));
	return L_GET_C_BACK_PTR(&dp);
}

const LinphoneDialPlan *linphone_dial_plan_get_all () {
	return nullptr;
}

const bctbx_list_t *linphone_dial_plan_get_all_list () {
	const list<LinphonePrivate::DialPlan> &dps = LinphonePrivate::DialPlan::getAllDialPlans();
	return L_GET_RESOLVED_C_LIST_FROM_CPP_LIST(dps);
}

bool_t linphone_dial_plan_is_generic (const LinphoneDialPlan *ccc) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(ccc)->isGeneric();
}
