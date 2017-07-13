/*
carddav.h
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

#ifndef LINPHONE_CARDDAV_H
#define LINPHONE_CARDDAV_H

#include "linphone/core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup carddav_vcard
 * @{
 */
	
typedef struct _LinphoneCardDavContext LinphoneCardDavContext;

typedef enum _LinphoneCardDavQueryType {
	LinphoneCardDavQueryTypePropfind,
	LinphoneCardDavQueryTypeAddressbookQuery,
	LinphoneCardDavQueryTypeAddressbookMultiget,
	LinphoneCardDavQueryTypePut,
	LinphoneCardDavQueryTypeDelete
} LinphoneCardDavQueryType;

typedef struct _LinphoneCardDavQuery LinphoneCardDavQuery;

typedef struct _LinphoneCardDavResponse LinphoneCardDavResponse;

/**
 * Callback used to notify a new contact has been created on the CardDAV server
**/
typedef void (*LinphoneCardDavContactCreatedCb)(LinphoneCardDavContext *cdc, LinphoneFriend *lf);

/**
 * Callback used to notify a contact has been updated on the CardDAV server
**/
typedef void (*LinphoneCardDavContactUpdatedCb)(LinphoneCardDavContext *cdc, LinphoneFriend *new_friend, LinphoneFriend *old_friend);

/**
 * Callback used to notify a contact has been removed on the CardDAV server
**/
typedef void (*LinphoneCardDavContactRemovedCb)(LinphoneCardDavContext *cdc, LinphoneFriend *lf);

/**
 * Callback used to notify a contact has been removed on the CardDAV server
**/
typedef void (*LinphoneCardDavSynchronizationDoneCb)(LinphoneCardDavContext *cdc, bool_t success, const char *message);

/**
 * Creates a CardDAV context for all related operations
 * @param lfl LinphoneFriendList object
 * @return LinphoneCardDavContext object if vCard support is enabled and server URL is available, NULL otherwise
 */
LINPHONE_PUBLIC LinphoneCardDavContext* linphone_carddav_context_new(LinphoneFriendList *lfl);

/**
 * Deletes a LinphoneCardDavContext object
 * @param cdc LinphoneCardDavContext object
 */
LINPHONE_PUBLIC void linphone_carddav_context_destroy(LinphoneCardDavContext *cdc);

/**
 * Sets a user pointer to the LinphoneCardDAVContext object
 * @param cdc LinphoneCardDavContext object
 * @param ud The user data pointer
 */
LINPHONE_PUBLIC void linphone_carddav_set_user_data(LinphoneCardDavContext *cdc, void *ud);

/**
 * Gets the user pointer set in the LinphoneCardDAVContext object
 * @param cdc LinphoneCardDavContext object
 * @return The user data pointer if set, NULL otherwise
 */
LINPHONE_PUBLIC void* linphone_carddav_get_user_data(LinphoneCardDavContext *cdc);

/**
 * Starts a synchronization with the remote server to update local friends with server changes
 * @param cdc LinphoneCardDavContext object
 */
LINPHONE_PUBLIC void linphone_carddav_synchronize(LinphoneCardDavContext *cdc);

/**
 * Sends a LinphoneFriend to the CardDAV server for update or creation
 * @param cdc LinphoneCardDavContext object
 * @param lf a LinphoneFriend object to update/create on the server
 */
LINPHONE_PUBLIC void linphone_carddav_put_vcard(LinphoneCardDavContext *cdc, LinphoneFriend *lf);

/**
 * Deletes a LinphoneFriend on the CardDAV server 
 * @param cdc LinphoneCardDavContext object
 * @param lf a LinphoneFriend object to delete on the server
 */
LINPHONE_PUBLIC void linphone_carddav_delete_vcard(LinphoneCardDavContext *cdc, LinphoneFriend *lf);

/**
 * Set the synchronization done callback.
 * @param cdc LinphoneCardDavContext object
 * @param cb The synchronization done callback to be used.
 */
LINPHONE_PUBLIC void linphone_carddav_set_synchronization_done_callback(LinphoneCardDavContext *cdc, LinphoneCardDavSynchronizationDoneCb cb);

/**
 * Set the new contact callback.
 * @param cdc LinphoneCardDavContext object
 * @param cb The new contact callback to be used.
 */
LINPHONE_PUBLIC void linphone_carddav_set_new_contact_callback(LinphoneCardDavContext *cdc, LinphoneCardDavContactCreatedCb cb);

/**
 * Set the updated contact callback.
 * @param cdc LinphoneCardDavContext object
 * @param cb The updated contact callback to be used.
 */
LINPHONE_PUBLIC void linphone_carddav_set_updated_contact_callback(LinphoneCardDavContext *cdc, LinphoneCardDavContactUpdatedCb cb);

/**
 * Set the removed contact callback.
 * @param cdc LinphoneCardDavContext object
 * @param cb The removed contact callback to be used.
 */
LINPHONE_PUBLIC void linphone_carddav_set_removed_contact_callback(LinphoneCardDavContext *cdc, LinphoneCardDavContactRemovedCb cb);

/**
 * Retrieves the current cTag value for the remote server
 * @param cdc LinphoneCardDavContext object
 */
void linphone_carddav_get_current_ctag(LinphoneCardDavContext *cdc);

/**
 * Retrieves a list of all the vCards on server side to be able to detect changes
 * @param cdc LinphoneCardDavContext object
 */
void linphone_carddav_fetch_vcards(LinphoneCardDavContext *cdc);

/**
 * Download asked vCards from the server
 * @param cdc LinphoneCardDavContext object
 * @param vcards_to_pull a MSList of LinphoneCardDavResponse objects with at least the url field filled
 */
void linphone_carddav_pull_vcards(LinphoneCardDavContext *cdc, MSList *vcards_to_pull);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
