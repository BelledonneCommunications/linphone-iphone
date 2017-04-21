/*
friendlist.h
Copyright (C) 2010-2015 Belledonne Communications SARL

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


#ifndef LINPHONE_FRIENDLIST_H_
#define LINPHONE_FRIENDLIST_H_


#include "linphone/types.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup buddy_list
 * @{
 */

/**
 * Acquire a reference to the friend list.
 * @param[in] list LinphoneFriendList object.
 * @return The same LinphoneFriendList object.
**/
LINPHONE_PUBLIC LinphoneFriendList * linphone_friend_list_ref(LinphoneFriendList *list);

/**
 * Release reference to the friend list.
 * @param[in] list LinphoneFriendList object.
**/
LINPHONE_PUBLIC void linphone_friend_list_unref(LinphoneFriendList *list);

/**
 * Retrieve the user pointer associated with the friend list.
 * @param[in] list LinphoneFriendList object.
 * @return The user pointer associated with the friend list.
**/
LINPHONE_PUBLIC void * linphone_friend_list_get_user_data(const LinphoneFriendList *list);

/**
 * Assign a user pointer to the friend list.
 * @param[in] list LinphoneFriendList object.
 * @param[in] ud The user pointer to associate with the friend list.
**/
LINPHONE_PUBLIC void linphone_friend_list_set_user_data(LinphoneFriendList *list, void *ud);

/**
 * Get the display name of the friend list.
 * @param[in] list LinphoneFriendList object.
 * @return The display name of the friend list.
**/
LINPHONE_PUBLIC const char * linphone_friend_list_get_display_name(const LinphoneFriendList *list);

/**
 * Set the display name of the friend list.
 * @param[in] list LinphoneFriendList object.
 * @param[in] display_name The new display name of the friend list.
**/
LINPHONE_PUBLIC void linphone_friend_list_set_display_name(LinphoneFriendList *list, const char *display_name);

/**
 * Get the RLS (Resource List Server) URI associated with the friend list to subscribe to these friends presence.
 * @param[in] list LinphoneFriendList object.
 * @return The RLS URI associated with the friend list.
**/
LINPHONE_PUBLIC const char * linphone_friend_list_get_rls_uri(const LinphoneFriendList *list);

/**
 * Set the RLS (Resource List Server) URI associated with the friend list to subscribe to these friends presence.
 * @param[in] list LinphoneFriendList object.
 * @param[in] rls_uri The RLS URI to associate with the friend list.
**/
LINPHONE_PUBLIC void linphone_friend_list_set_rls_uri(LinphoneFriendList *list, const char *rls_uri);

/**
 * Get the RLS (Resource List Server) URI associated with the friend list to subscribe to these friends presence.
 * @param[in] list LinphoneFriendList object.
 * @return The RLS URI associated with the friend list.
**/
LINPHONE_PUBLIC const LinphoneAddress * linphone_friend_list_get_rls_address(const LinphoneFriendList *list);

/**
 * Set the RLS (Resource List Server) URI associated with the friend list to subscribe to these friends presence.
 * @param[in] list LinphoneFriendList object.
 * @param[in] rls_addr The RLS URI to associate with the friend list.
**/
LINPHONE_PUBLIC void linphone_friend_list_set_rls_address(LinphoneFriendList *list, const LinphoneAddress *rls_addr);

/**
 * Add a friend to a friend list. If or when a remote CardDAV server will be attached to the list, the friend will be sent to the server.
 * @param[in] list LinphoneFriendList object.
 * @param[in] lf LinphoneFriend object to add to the friend list.
 * @return LinphoneFriendListOK if successfully added, LinphoneFriendListInvalidFriend if the friend is not valid.
**/
LINPHONE_PUBLIC LinphoneFriendListStatus linphone_friend_list_add_friend(LinphoneFriendList *list, LinphoneFriend *lf);

/**
 * Add a friend to a friend list. The friend will never be sent to a remote CardDAV server.
 * Warning! LinphoneFriends added this way will be removed on the next synchronization, and the callback contact_deleted will be called.
 * @param[in] list LinphoneFriendList object.
 * @param[in] lf LinphoneFriend object to add to the friend list.
 * @return LinphoneFriendListOK if successfully added, LinphoneFriendListInvalidFriend if the friend is not valid.
**/
LINPHONE_PUBLIC LinphoneFriendListStatus linphone_friend_list_add_local_friend(LinphoneFriendList *list, LinphoneFriend *lf);

/**
 * Remove a friend from a friend list.
 * @param[in] list LinphoneFriendList object.
 * @param[in] lf LinphoneFriend object to remove from the friend list.
 * @return LinphoneFriendListOK if removed successfully, LinphoneFriendListNonExistentFriend if the friend is not in the list.
**/
LINPHONE_PUBLIC LinphoneFriendListStatus linphone_friend_list_remove_friend(LinphoneFriendList *list, LinphoneFriend *lf);

/**
 * Retrieves the list of LinphoneFriend from this LinphoneFriendList.
 * @param[in] list LinphoneFriendList object
 * @return \bctbx_list{LinphoneFriend} a list of LinphoneFriend
 */
LINPHONE_PUBLIC const bctbx_list_t * linphone_friend_list_get_friends(const LinphoneFriendList *list);

/**
 * Find a friend in the friend list using a LinphoneAddress.
 * @param[in] list LinphoneFriendList object.
 * @param[in] address LinphoneAddress object of the friend we want to search for.
 * @return A LinphoneFriend if found, NULL otherwise.
**/
LINPHONE_PUBLIC LinphoneFriend * linphone_friend_list_find_friend_by_address(const LinphoneFriendList *list, const LinphoneAddress *address);

/**
 * Find a friend in the friend list using an URI string.
 * @param[in] list LinphoneFriendList object.
 * @param[in] uri A string containing the URI of the friend we want to search for.
 * @return A LinphoneFriend if found, NULL otherwise.
**/
LINPHONE_PUBLIC LinphoneFriend * linphone_friend_list_find_friend_by_uri(const LinphoneFriendList *list, const char *uri);

/**
 * Find a friend in the friend list using a ref key.
 * @param[in] list LinphoneFriendList object.
 * @param[in] ref_key The ref key string of the friend we want to search for.
 * @return A LinphoneFriend if found, NULL otherwise.
**/
LINPHONE_PUBLIC LinphoneFriend * linphone_friend_list_find_friend_by_ref_key(const LinphoneFriendList *list, const char *ref_key);

/**
 * Update presence subscriptions for the entire list. Calling this function is necessary when list subscriptions are enabled,
 * ie when a RLS presence server is used.
 * @param[in] list the friend list
**/
LINPHONE_PUBLIC void linphone_friend_list_update_subscriptions(LinphoneFriendList *list);

/**
 * Notify our presence to all the friends in the friend list that have subscribed to our presence directly (not using a RLS).
 * @param[in] list LinphoneFriendList object.
 * @param[in] presence LinphonePresenceModel object.
**/
LINPHONE_PUBLIC void linphone_friend_list_notify_presence(LinphoneFriendList *list, LinphonePresenceModel *presence);

/**
 * Get the URI associated with the friend list.
 * @param[in] list LinphoneFriendList object.
 * @return The URI associated with the friend list.
**/
LINPHONE_PUBLIC const char * linphone_friend_list_get_uri(const LinphoneFriendList *list);

/**
 * Set the URI associated with the friend list.
 * @param[in] list LinphoneFriendList object.
 * @param[in] uri The URI to associate with the friend list.
**/
LINPHONE_PUBLIC void linphone_friend_list_set_uri(LinphoneFriendList *list, const char *uri);

/**
 * Sets the revision from the last synchronization.
 * @param[in] list LinphoneFriendList object.
 * @param[in] rev The revision
 */
LINPHONE_PUBLIC void linphone_friend_list_update_revision(LinphoneFriendList *list, int rev);

/**
 * Get the LinphoneFriendListCbs object associated with a LinphoneFriendList.
 * @param[in] list LinphoneFriendList object
 * @return The LinphoneFriendListCbs object associated with the LinphoneFriendList.
**/
LINPHONE_PUBLIC LinphoneFriendListCbs * linphone_friend_list_get_callbacks(const LinphoneFriendList *list);

/**
 * Acquire a reference to a LinphoneFriendListCbs object.
 * @param[in] cbs LinphoneFriendListCbs object.
 * @return The same LinphoneFriendListCbs object.
**/
LINPHONE_PUBLIC LinphoneFriendListCbs * linphone_friend_list_cbs_ref(LinphoneFriendListCbs *cbs);

/**
 * Release a reference to a LinphoneFriendListCbs object.
 * @param[in] cbs LinphoneFriendListCbs object.
**/
LINPHONE_PUBLIC void linphone_friend_list_cbs_unref(LinphoneFriendListCbs *cbs);

/**
 * Retrieve the user pointer associated with a LinphoneFriendListCbs object.
 * @param[in] cbs LinphoneFriendListCbs object.
 * @return The user pointer associated with the LinphoneFriendListCbs object.
**/
LINPHONE_PUBLIC void *linphone_friend_list_cbs_get_user_data(const LinphoneFriendListCbs *cbs);

/**
 * Assign a user pointer to a LinphoneFriendListCbs object.
 * @param[in] cbs LinphoneFriendListCbs object.
 * @param[in] ud The user pointer to associate with the LinphoneFriendListCbs object.
**/
LINPHONE_PUBLIC void linphone_friend_list_cbs_set_user_data(LinphoneFriendListCbs *cbs, void *ud);

/**
 * Get the contact created callback.
 * @param[in] cbs LinphoneFriendListCbs object.
 * @return The current contact created callback.
**/
LINPHONE_PUBLIC LinphoneFriendListCbsContactCreatedCb linphone_friend_list_cbs_get_contact_created(const LinphoneFriendListCbs *cbs);

/**
 * Set the contact created callback.
 * @param[in] cbs LinphoneFriendListCbs object.
 * @param[in] cb The contact created to be used.
**/
LINPHONE_PUBLIC void linphone_friend_list_cbs_set_contact_created(LinphoneFriendListCbs *cbs, LinphoneFriendListCbsContactCreatedCb cb);

/**
 * Get the contact deleted callback.
 * @param[in] cbs LinphoneFriendListCbs object.
 * @return The current contact deleted callback.
**/
LINPHONE_PUBLIC LinphoneFriendListCbsContactDeletedCb linphone_friend_list_cbs_get_contact_deleted(const LinphoneFriendListCbs *cbs);

/**
 * Set the contact deleted callback.
 * @param[in] cbs LinphoneFriendListCbs object.
 * @param[in] cb The contact deleted to be used.
**/
LINPHONE_PUBLIC void linphone_friend_list_cbs_set_contact_deleted(LinphoneFriendListCbs *cbs, LinphoneFriendListCbsContactDeletedCb cb);

/**
 * Get the contact updated callback.
 * @param[in] cbs LinphoneFriendListCbs object.
 * @return The current contact updated callback.
**/
LINPHONE_PUBLIC LinphoneFriendListCbsContactUpdatedCb linphone_friend_list_cbs_get_contact_updated(const LinphoneFriendListCbs *cbs);

/**
 * Set the contact updated callback.
 * @param[in] cbs LinphoneFriendListCbs object.
 * @param[in] cb The contact updated to be used.
**/
LINPHONE_PUBLIC void linphone_friend_list_cbs_set_contact_updated(LinphoneFriendListCbs *cbs, LinphoneFriendListCbsContactUpdatedCb cb);

/**
 * Get the sync status changed callback.
 * @param[in] cbs LinphoneFriendListCbs object.
 * @return The current sync status changedcallback.
**/
LINPHONE_PUBLIC LinphoneFriendListCbsSyncStateChangedCb linphone_friend_list_cbs_get_sync_status_changed(const LinphoneFriendListCbs *cbs);

/**
 * Set the contact updated callback.
 * @param[in] cbs LinphoneFriendListCbs object.
 * @param[in] cb The sync status changed to be used.
**/
LINPHONE_PUBLIC void linphone_friend_list_cbs_set_sync_status_changed(LinphoneFriendListCbs *cbs, LinphoneFriendListCbsSyncStateChangedCb cb);

/**
 * Starts a CardDAV synchronization using value set using linphone_friend_list_set_uri.
 * @param[in] list LinphoneFriendList object.
 */
LINPHONE_PUBLIC void linphone_friend_list_synchronize_friends_from_server(LinphoneFriendList *list);

/**
 * Goes through all the LinphoneFriend that are dirty and does a CardDAV PUT to update the server.
 * @param[in] list LinphoneFriendList object.
 */
LINPHONE_PUBLIC void linphone_friend_list_update_dirty_friends(LinphoneFriendList *list);

/**
 * Returns the LinphoneCore object attached to this LinphoneFriendList.
 * @param[in] list LinphoneFriendList object.
 * @return a LinphoneCore object
 */
LINPHONE_PUBLIC LinphoneCore* linphone_friend_list_get_core(const LinphoneFriendList *list);

/**
 * Creates and adds LinphoneFriend objects to LinphoneFriendList from a file that contains the vCard(s) to parse
 * @param[in] list the LinphoneFriendList object
 * @param[in] vcard_file the path to a file that contains the vCard(s) to parse
 * @return the amount of linphone friends created
 */
LINPHONE_PUBLIC LinphoneStatus linphone_friend_list_import_friends_from_vcard4_file(LinphoneFriendList *list, const char *vcard_file);

/**
 * Creates and adds LinphoneFriend objects to LinphoneFriendList from a buffer that contains the vCard(s) to parse
 * @param[in] list the LinphoneFriendList object
 * @param[in] vcard_buffer the buffer that contains the vCard(s) to parse
 * @return the amount of linphone friends created
 */
LINPHONE_PUBLIC LinphoneStatus linphone_friend_list_import_friends_from_vcard4_buffer(LinphoneFriendList *list, const char *vcard_buffer);

/**
 * Creates and export LinphoneFriend objects from LinphoneFriendList to a file using vCard 4 format
 * @param[in] list the LinphoneFriendList object
 * @param[in] vcard_file the path to a file that will contain the vCards
 */
LINPHONE_PUBLIC void linphone_friend_list_export_friends_as_vcard4_file(LinphoneFriendList *list, const char *vcard_file);

/**
 * Enable subscription to NOTIFYes of all friends list
 * @param[in] list the LinphoneFriendList object
 * @param[in] enabled should subscription be enabled or not
 */
LINPHONE_PUBLIC void linphone_friend_list_enable_subscriptions(LinphoneFriendList *list, bool_t enabled);

/**
 * Gets whether subscription to NOTIFYes of all friends list are enabled or not
 * @param[in] list the LinphoneFriendList object
 * @return Whether subscriptions are enabled or not
 */
LINPHONE_PUBLIC bool_t linphone_friend_list_subscriptions_enabled(LinphoneFriendList *list);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_FRIENDLIST_H_ */
