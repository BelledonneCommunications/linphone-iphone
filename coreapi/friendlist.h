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


#include "linphonefriend.h"


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
 * @return A new LinphoneFriendList object.
 */
LINPHONE_PUBLIC LinphoneFriendList * linphone_friend_list_new(void);

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
LINPHONE_PUBLIC LinphoneFriendListStatus linphone_friend_list_add_friend(LinphoneFriendList *list, LinphoneFriend *friend);

/**
 * Remove a friend from a friend list.
 * @param[in] list LinphoneFriendList object.
 * @param[in] friend LinphoneFriend object to remove from the friend list.
 * @return LinphoneFriendListOK if removed successfully, LinphoneFriendListNonExistentFriend if the friend is not in the list.
**/
LINPHONE_PUBLIC LinphoneFriendListStatus linphone_friend_list_remove_friend(LinphoneFriendList *list, LinphoneFriend *friend);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_FRIENDLIST_H_ */
