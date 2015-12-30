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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#ifndef LINPHONE_FRIENDLIST_H_
#define LINPHONE_FRIENDLIST_H_


#ifdef IN_LINPHONE
#include "linphonefriend.h"
#include "linphonepresence.h"
#else
#include "linphone/linphonefriend.h"
#include "linphone/linphonepresence.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup buddy_list
 * @{
 */

/**
* Enum describing the status of a LinphoneFriendList operation.
**/
typedef enum _LinphoneFriendListStatus {
	LinphoneFriendListOK,
	LinphoneFriendListNonExistentFriend,
	LinphoneFriendListInvalidFriend
} LinphoneFriendListStatus;

/**
 * The LinphoneFriendList object representing a list of friends.
**/
typedef struct _LinphoneFriendList LinphoneFriendList;

/**
 * Create a new empty LinphoneFriendList object.
 * @param[in] lc LinphoneCore object.
 * @return A new LinphoneFriendList object.
**/
LINPHONE_PUBLIC LinphoneFriendList * linphone_core_create_friend_list(LinphoneCore *lc);

/**
 * Set the friend list.
 * @param[in] lc LinphoneCore object
 * @param[in] list LinphoneFriendList object
 */
LINPHONE_PUBLIC void linphone_core_set_friend_list(LinphoneCore *lc, LinphoneFriendList *list);

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
 * Add a friend to a friend list.
 * @param[in] list LinphoneFriendList object.
 * @param[in] friend LinphoneFriend object to add to the friend list.
 * @return LinphoneFriendListOK if successfully added, LinphoneFriendListInvalidFriend if the friend is not valid.
**/
LINPHONE_PUBLIC LinphoneFriendListStatus linphone_friend_list_add_friend(LinphoneFriendList *list, LinphoneFriend *afriend);

/**
 * Remove a friend from a friend list.
 * @param[in] list LinphoneFriendList object.
 * @param[in] friend LinphoneFriend object to remove from the friend list.
 * @return LinphoneFriendListOK if removed successfully, LinphoneFriendListNonExistentFriend if the friend is not in the list.
**/
LINPHONE_PUBLIC LinphoneFriendListStatus linphone_friend_list_remove_friend(LinphoneFriendList *list, LinphoneFriend *afriend);

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
 * Find a frient in the friend list using a ref key.
 * @param[in] list LinphoneFriendList object.
 * @param[in] ref_key The ref key string of the friend we want to search for.
 * @return A LinphoneFriend if found, NULL otherwise.
**/
LINPHONE_PUBLIC LinphoneFriend * linphone_friend_list_find_friend_by_ref_key(const LinphoneFriendList *list, const char *ref_key);

LINPHONE_PUBLIC void linphone_friend_list_close_subscriptions(LinphoneFriendList *list);

LINPHONE_PUBLIC void linphone_friend_list_update_subscriptions(LinphoneFriendList *list, LinphoneProxyConfig *cfg, bool_t only_when_registered);

/**
 * Notify our presence to all the friends in the friend list that have subscribed to our presence directly (not using a RLS).
 * @param[in] list LinphoneFriendList object.
 * @param[in] presence LinphonePresenceModel object.
**/
LINPHONE_PUBLIC void linphone_friend_list_notify_presence(LinphoneFriendList *list, LinphonePresenceModel *presence);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_FRIENDLIST_H_ */
