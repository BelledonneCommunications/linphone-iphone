/*
vcard_private.h
Copyright (C) 2010-2016  Belledonne Communications SARL

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


#ifndef LINPHONE_VCARD_PRIVATE_H
#define LINPHONE_VCARD_PRIVATE_H

#include "linphone/vcard.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * The LinphoneVcardContext object.
 */
typedef struct _LinphoneVcardContext LinphoneVcardContext;

/**
 * Creates a vCard context to reuse the same BelCardParser object
 * @return a new LinphoneVcardContext object
 */
LINPHONE_PUBLIC LinphoneVcardContext* linphone_vcard_context_new(void);

/**
 * Destroys the vCard context
 * @param[in] context a LinphoneVcardContext object
 */
LINPHONE_PUBLIC void linphone_vcard_context_destroy(LinphoneVcardContext *context);

/**
 * Gets the user data set in the LinphoneVcardContext
 * @param[in] context a LinphoneVcardContext object
 * @return the user data pointer
 */
LINPHONE_PUBLIC void* linphone_vcard_context_get_user_data(const LinphoneVcardContext *context);

/**
 * Sets the user data in the LinphoneVcardContext
 * @param[in] context a LinphoneVcardContext object
 * @param[in] data the user data pointer
 */
LINPHONE_PUBLIC void linphone_vcard_context_set_user_data(LinphoneVcardContext *context, void *data);

/**
 * Uses belcard to parse the content of a file and returns all the vcards it contains as LinphoneVcards, or NULL if it contains none.
 * @param[in] context the vCard context to use (speed up the process by not creating a Belcard parser each time)
 * @param[in] file the path to the file to parse
 * @return \bctbx_list{LinphoneVcard}
 */
LINPHONE_PUBLIC bctbx_list_t* linphone_vcard_context_get_vcard_list_from_file(LinphoneVcardContext *context, const char *file);

/**
 * Uses belcard to parse the content of a buffer and returns all the vcards it contains as LinphoneVcards, or NULL if it contains none.
 * @param[in] context the vCard context to use (speed up the process by not creating a Belcard parser each time)
 * @param[in] buffer the buffer to parse
 * @return \bctbx_list{LinphoneVcard}
 */
LINPHONE_PUBLIC bctbx_list_t* linphone_vcard_context_get_vcard_list_from_buffer(LinphoneVcardContext *context, const char *buffer);

/**
 * Uses belcard to parse the content of a buffer and returns one vCard if possible, or NULL otherwise.
 * @param[in] context the vCard context to use (speed up the process by not creating a Belcard parser each time)
 * @param[in] buffer the buffer to parse
 * @return a LinphoneVcard if one could be parsed, or NULL otherwise
 */
LINPHONE_PUBLIC LinphoneVcard* linphone_vcard_context_get_vcard_from_buffer(LinphoneVcardContext *context, const char *buffer);


/**
 * Computes the md5 hash for the vCard
 * @param[in] vCard the LinphoneVcard
 */
void linphone_vcard_compute_md5_hash(LinphoneVcard *vCard);

/**
 * Compares the previously computed md5 hash (using linphone_vcard_compute_md5_hash) with the current one
 * @param[in] vCard the LinphoneVcard
 * @return 0 if the md5 hasn't changed, 1 otherwise
 */
bool_t linphone_vcard_compare_md5_hash(LinphoneVcard *vCard);

void linphone_vcard_clean_cache(LinphoneVcard *vCard);

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_VCARD_PRIVATE_H */
