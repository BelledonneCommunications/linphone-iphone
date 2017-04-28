/*
vcard.h
Copyright (C) 2015  Belledonne Communications SARL

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef LINPHONE_VCARD_H
#define LINPHONE_VCARD_H

#include "linphone/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @addtogroup carddav_vcard
 * @{
 */

/**
 * Cast a belle_sip_object_t into LinphoneVcard.
 */
#define LINPHONE_VCARD BELLE_SIP_CAST(object, LinphoneVcard)

/**
 * Creates a LinphoneVcard object that has a pointer to an empty vCard
 * @return a new LinphoneVcard object
 * @deprecated Use linphone_factory_create_vcard() instead.
 * @donotwrap
 */
LINPHONE_DEPRECATED LINPHONE_PUBLIC LinphoneVcard* linphone_vcard_new(void);

/**
 * Deletes a LinphoneVcard object properly
 * @param[in] vCard the LinphoneVcard to destroy
 * @deprecated Use linphone_vcard_unref() or belle_sip_object_unref() instead.
 * @donotwrap
 */
LINPHONE_DEPRECATED LINPHONE_PUBLIC void linphone_vcard_free(LinphoneVcard *vCard);

/**
 * Take a ref on a #LinphoneVcard.
 * @param[in] vCard LinphoneVcard object
 */
LINPHONE_PUBLIC LinphoneVcard *linphone_vcard_ref(LinphoneVcard *vCard);

/**
 * Release a #LinphoneVcard.
 * @param[in] vCard LinphoneVcard object
 */
LINPHONE_PUBLIC void linphone_vcard_unref(LinphoneVcard *vCard);

/**
 * Clone a #LinphoneVcard.
 * @param[in] vCard LinphoneVcard object
 * @return a new LinphoneVcard object
 */
LINPHONE_PUBLIC LinphoneVcard *linphone_vcard_clone(const LinphoneVcard *vCard);

/**
 * Returns the vCard4 representation of the LinphoneVcard.
 * @param[in] vCard the LinphoneVcard
 * @return a const char * that represents the vCard
 */
LINPHONE_PUBLIC const char* linphone_vcard_as_vcard4_string(LinphoneVcard *vCard);

/**
 * Sets the FN attribute of the vCard (which is mandatory).
 * @param[in] vCard the LinphoneVcard
 * @param[in] name the display name to set for the vCard
 */
LINPHONE_PUBLIC void linphone_vcard_set_full_name(LinphoneVcard *vCard, const char *name);

/**
 * Returns the FN attribute of the vCard, or NULL if it isn't set yet.
 * @param[in] vCard the LinphoneVcard
 * @return the display name of the vCard, or NULL
 */
LINPHONE_PUBLIC const char* linphone_vcard_get_full_name(const LinphoneVcard *vCard);

/**
 * Sets the skipFieldValidation property of the vcard
 * @param[in] vCard the LinphoneVcard
 * @param[in] skip skipFieldValidation property of the vcard
 */
LINPHONE_PUBLIC void linphone_vcard_set_skip_validation(LinphoneVcard *vCard, bool_t skip);

/**
 * Returns the skipFieldValidation property of the vcard.
 * @param[in] vCard the LinphoneVcard
 * @return the skipFieldValidation property of the vcard
 */
LINPHONE_PUBLIC bool_t linphone_vcard_get_skip_validation(const LinphoneVcard *vCard);

/**
 * Sets the family name in the N attribute of the vCard.
 * @param[in] vCard the LinphoneVcard
 * @param[in] name the family name to set for the vCard
 */
LINPHONE_PUBLIC void linphone_vcard_set_family_name(LinphoneVcard *vCard, const char *name);

/**
 * Returns the family name in the N attribute of the vCard, or NULL if it isn't set yet.
 * @param[in] vCard the LinphoneVcard
 * @return the family name of the vCard, or NULL
 */
LINPHONE_PUBLIC const char* linphone_vcard_get_family_name(const LinphoneVcard *vCard);

/**
 * Sets the given name in the N attribute of the vCard.
 * @param[in] vCard the LinphoneVcard
 * @param[in] name the given name to set for the vCard
 */
LINPHONE_PUBLIC void linphone_vcard_set_given_name(LinphoneVcard *vCard, const char *name);

/**
 * Returns the given name in the N attribute of the vCard, or NULL if it isn't set yet.
 * @param[in] vCard the LinphoneVcard
 * @return the given name of the vCard, or NULL
 */
LINPHONE_PUBLIC const char* linphone_vcard_get_given_name(const LinphoneVcard *vCard);

/**
 * Adds a SIP address in the vCard, using the IMPP property
 * @param[in] vCard the LinphoneVcard
 * @param[in] sip_address the SIP address to add
 */
LINPHONE_PUBLIC void linphone_vcard_add_sip_address(LinphoneVcard *vCard, const char *sip_address);

/**
 * Removes a SIP address in the vCard (if it exists), using the IMPP property
 * @param[in] vCard the LinphoneVcard
 * @param[in] sip_address the SIP address to remove
 */
LINPHONE_PUBLIC void linphone_vcard_remove_sip_address(LinphoneVcard *vCard, const char *sip_address);

/**
 * Edits the preferred SIP address in the vCard (or the first one), using the IMPP property
 * @param[in] vCard the LinphoneVcard
 * @param[in] sip_address the new SIP address
 */
LINPHONE_PUBLIC void linphone_vcard_edit_main_sip_address(LinphoneVcard *vCard, const char *sip_address);

/**
 * Returns the list of SIP addresses (as LinphoneAddress) in the vCard (all the IMPP attributes that has an URI value starting by "sip:") or NULL
 * @param[in] vCard the LinphoneVcard
 * @return \bctbx_list{LinphoneAddress}
 */
LINPHONE_PUBLIC const bctbx_list_t* linphone_vcard_get_sip_addresses(LinphoneVcard *vCard);

/**
 * Adds a phone number in the vCard, using the TEL property
 * @param[in] vCard the LinphoneVcard
 * @param[in] phone the phone number to add
 */
LINPHONE_PUBLIC void linphone_vcard_add_phone_number(LinphoneVcard *vCard, const char *phone);

/**
 * Removes a phone number in the vCard (if it exists), using the TEL property
 * @param[in] vCard the LinphoneVcard
 * @param[in] phone the phone number to remove
 */
LINPHONE_PUBLIC void linphone_vcard_remove_phone_number(LinphoneVcard *vCard, const char *phone);

/**
 * Returns the list of phone numbers (as string) in the vCard (all the TEL attributes) or NULL
 * @param[in] vCard the LinphoneVcard
 * @return \bctbx_list{const char *}
 */
LINPHONE_PUBLIC bctbx_list_t* linphone_vcard_get_phone_numbers(const LinphoneVcard *vCard);

/**
 * Fills the Organization field of the vCard
 * @param[in] vCard the LinphoneVcard
 * @param[in] organization the Organization
 */
LINPHONE_PUBLIC void linphone_vcard_set_organization(LinphoneVcard *vCard, const char *organization);

/**
 * Gets the Organization of the vCard
 * @param[in] vCard the LinphoneVcard
 * @return the Organization of the vCard or NULL
 */
LINPHONE_PUBLIC const char* linphone_vcard_get_organization(const LinphoneVcard *vCard);

/**
 * Generates a random unique id for the vCard.
 * If is required to be able to synchronize the vCard with a CardDAV server
 * @param[in] vCard the LinphoneVcard
 * @return TRUE if operation is successful, otherwise FALSE (for example if it already has an unique ID)
 */
LINPHONE_PUBLIC bool_t linphone_vcard_generate_unique_id(LinphoneVcard *vCard);

/**
 * Sets the unique ID of the vCard
 * @param[in] vCard the LinphoneVcard
 * @param[in] uid the unique id
 */
LINPHONE_PUBLIC void linphone_vcard_set_uid(LinphoneVcard *vCard, const char *uid);

/**
 * Gets the UID of the vCard
 * @param[in] vCard the LinphoneVcard
 * @return the UID of the vCard, otherwise NULL
 */
LINPHONE_PUBLIC const char* linphone_vcard_get_uid(const LinphoneVcard *vCard);

/**
 * Sets the eTAG of the vCard
 * @param[in] vCard the LinphoneVcard
 * @param[in] etag the eTAG
 */
LINPHONE_PUBLIC void linphone_vcard_set_etag(LinphoneVcard *vCard, const char * etag);

/**
 * Gets the eTag of the vCard
 * @param[in] vCard the LinphoneVcard
 * @return the eTag of the vCard in the CardDAV server, otherwise NULL
 */
LINPHONE_PUBLIC const char* linphone_vcard_get_etag(const LinphoneVcard *vCard);

/**
 * Sets the URL of the vCard
 * @param[in] vCard the LinphoneVcard
 * @param[in] url the URL
 */
LINPHONE_PUBLIC void linphone_vcard_set_url(LinphoneVcard *vCard, const char *url);

/**
 * Gets the URL of the vCard
 * @param[in] vCard the LinphoneVcard
 * @return the URL of the vCard in the CardDAV server, otherwise NULL
 */
LINPHONE_PUBLIC const char* linphone_vcard_get_url(const LinphoneVcard *vCard);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
