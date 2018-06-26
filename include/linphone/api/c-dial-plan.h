/*
 * c-dial-plan.h
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

#ifndef _L_C_DIAL_PLAN_H_
#define _L_C_DIAL_PLAN_H_

#include "linphone/api/c-types.h"

// =============================================================================

#ifdef __cplusplus
	extern "C" {
#endif // ifdef __cplusplus

/**
 * @addtogroup misc
 * @{
 */

 /**
  * Returns the country name of the dialplan
  * @return the country name
  */
 LINPHONE_PUBLIC const char * linphone_dial_plan_get_country(const LinphoneDialPlan *dp);

  /**
  * Returns the iso country code of the dialplan
  * @return the iso country code
  */
 LINPHONE_PUBLIC const char * linphone_dial_plan_get_iso_country_code(const LinphoneDialPlan *dp);

  /**
  * Returns the country calling code of the dialplan
  * @return the country calling code
  */
 LINPHONE_PUBLIC const char * linphone_dial_plan_get_country_calling_code(const LinphoneDialPlan *dp);

  /**
  * Returns the national number length of the dialplan
  * @return the national number length
  */
 LINPHONE_PUBLIC int linphone_dial_plan_get_national_number_length(const LinphoneDialPlan *dp);

  /**
  * Returns the international call prefix of the dialplan
  * @return the international call prefix
  */
 LINPHONE_PUBLIC const char * linphone_dial_plan_get_international_call_prefix(const LinphoneDialPlan *dp);

 /**
 *Function to get  call country code from  ISO 3166-1 alpha-2 code, ex: FR returns 33
 *@param iso country code alpha2
 *@return call country code or -1 if not found
 */
LINPHONE_PUBLIC	int linphone_dial_plan_lookup_ccc_from_iso(const char* iso);

/**
 *Function to get  call country code from  an e164 number, ex: +33952650121 will return 33
 *@param e164 phone number
 *@return call country code or -1 if not found
 */
LINPHONE_PUBLIC	int linphone_dial_plan_lookup_ccc_from_e164(const char* e164);

/**
 * Return NULL-terminated array of all known dial plans
 * @deprecated use linphone_dial_plan_get_all_list instead, this method will always return NULL
 * @donotwrap
**/
LINPHONE_PUBLIC const LinphoneDialPlan* linphone_dial_plan_get_all(void);

/**
 * @return \bctbx_list{LinphoneDialPlan} of all known dial plans
**/
LINPHONE_PUBLIC const bctbx_list_t * linphone_dial_plan_get_all_list(void);

/**
 * Find best match for given CCC
 * @return Return matching dial plan, or a generic one if none found
**/
LINPHONE_PUBLIC const LinphoneDialPlan* linphone_dial_plan_by_ccc(const char *ccc);
/**
 * Find best match for given CCC
 * @return Return matching dial plan, or a generic one if none found
 **/
LINPHONE_PUBLIC const LinphoneDialPlan* linphone_dial_plan_by_ccc_as_int(int ccc);

/**
 * Return if given plan is generic
**/
LINPHONE_PUBLIC bool_t linphone_dial_plan_is_generic(const LinphoneDialPlan *ccc);

/**
 * @}
 */

#ifdef __cplusplus
	}
#endif // ifdef __cplusplus

#endif // ifndef _L_C_DIAL_PLAN_H_
