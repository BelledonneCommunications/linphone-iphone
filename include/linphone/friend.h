/*
linphonefriend.h
Copyright (C) 2010  Belledonne Communications SARL

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

#ifndef LINPHONE_FRIEND_H_
#define LINPHONE_FRIEND_H_

#include "linphone/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup buddy_list
 * @{
 */

/**
 * Contructor
 * @return a new empty #LinphoneFriend
 * @deprecated use #linphone_core_create_friend instead
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneFriend * linphone_friend_new(void);

/**
 * Contructor same as linphone_friend_new() + linphone_friend_set_address()
 * @param addr a buddy address, must be a sip uri like sip:joe@sip.linphone.org
 * @return a new #LinphoneFriend with \link linphone_friend_get_address() address initialized \endlink
 * @deprecated use #linphone_core_create_friend_with_address instead
 * @donotwrap
 */
LINPHONE_PUBLIC	LINPHONE_DEPRECATED LinphoneFriend *linphone_friend_new_with_address(const char *addr);

/**
 * Contructor same as linphone_friend_new() + linphone_friend_set_address()
 * @deprecated Use #linphone_friend_new_with_address instead
 */
#define linphone_friend_new_with_addr linphone_friend_new_with_address

/**
 * Destroy a LinphoneFriend.
 * @param lf LinphoneFriend object
 * @deprecated Use linphone_friend_unref() instead.
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED void linphone_friend_destroy(LinphoneFriend *lf);

/**
 * Set #LinphoneAddress for this friend
 * @param fr #LinphoneFriend object
 * @param address #LinphoneAddress
 */
LINPHONE_PUBLIC LinphoneStatus linphone_friend_set_address(LinphoneFriend *fr, const LinphoneAddress* address);

/**
 * Set #LinphoneAddress for this friend
 * @deprecated Use #linphone_friend_set_address instead
 */
#define linphone_friend_set_addr linphone_friend_set_address

/**
 * Get address of this friend.
 * @note the LinphoneAddress object returned is hold by the LinphoneFriend, however calling several time this function may return different objects.
 * @param lf #LinphoneFriend object
 * @return #LinphoneAddress
 */
LINPHONE_PUBLIC const LinphoneAddress * linphone_friend_get_address(const LinphoneFriend *lf);

/**
 * Adds an address in this friend
 * @param lf #LinphoneFriend object
 * @param addr #LinphoneAddress object
 */
LINPHONE_PUBLIC void linphone_friend_add_address(LinphoneFriend *lf, const LinphoneAddress *addr);

/**
 * Returns a list of #LinphoneAddress for this friend
 * @param lf #LinphoneFriend object
 * @return \bctbx_list{LinphoneAddress}
 */
LINPHONE_PUBLIC const bctbx_list_t* linphone_friend_get_addresses(const LinphoneFriend *lf);

/**
 * Removes an address in this friend
 * @param lf #LinphoneFriend object
 * @param addr #LinphoneAddress object
 */
LINPHONE_PUBLIC void linphone_friend_remove_address(LinphoneFriend *lf, const LinphoneAddress *addr);

/**
 * Adds a phone number in this friend
 * @param lf #LinphoneFriend object
 * @param phone number to add
 */
LINPHONE_PUBLIC void linphone_friend_add_phone_number(LinphoneFriend *lf, const char *phone);

/**
 * Returns a list of phone numbers for this friend
 * @param lf #LinphoneFriend object
 * @return \bctbx_list{const char *}
 */
LINPHONE_PUBLIC bctbx_list_t* linphone_friend_get_phone_numbers(LinphoneFriend *lf);

/**
 * Removes a phone number in this friend
 * @param lf #LinphoneFriend object
 * @param phone number to remove
 */
LINPHONE_PUBLIC void linphone_friend_remove_phone_number(LinphoneFriend *lf, const char *phone);

/**
 * Set the display name for this friend
 * @param lf #LinphoneFriend object
 * @param name
 */
LINPHONE_PUBLIC LinphoneStatus linphone_friend_set_name(LinphoneFriend *lf, const char *name);

/**
 * Get the display name for this friend
 * @param lf #LinphoneFriend object
 * @return The display name of this friend
 */
LINPHONE_PUBLIC const char * linphone_friend_get_name(const LinphoneFriend *lf);

/**
 * get subscription flag value
 * @param lf #LinphoneFriend object
 * @return returns true is subscription is activated for this friend
 *
 */
LINPHONE_PUBLIC bool_t linphone_friend_subscribes_enabled(const LinphoneFriend *lf);
#define linphone_friend_get_send_subscribe linphone_friend_subscribes_enabled

/**
 * Configure #LinphoneFriend to subscribe to presence information
 * @param fr #LinphoneFriend object
 * @param val if TRUE this friend will receive subscription message
 */

LINPHONE_PUBLIC	LinphoneStatus linphone_friend_enable_subscribes(LinphoneFriend *fr, bool_t val);
#define linphone_friend_send_subscribe linphone_friend_enable_subscribes

/**
 * Configure incoming subscription policy for this friend.
 * @param fr #LinphoneFriend object
 * @param pol #LinphoneSubscribePolicy policy to apply.
 */
LINPHONE_PUBLIC LinphoneStatus linphone_friend_set_inc_subscribe_policy(LinphoneFriend *fr, LinphoneSubscribePolicy pol);

/**
 * get current subscription policy for this #LinphoneFriend
 * @param lf #LinphoneFriend object
 * @return #LinphoneSubscribePolicy
 *
 */
LINPHONE_PUBLIC LinphoneSubscribePolicy linphone_friend_get_inc_subscribe_policy(const LinphoneFriend *lf);

/**
 * Starts editing a friend configuration.
 *
 * Because friend configuration must be consistent, applications MUST
 * call linphone_friend_edit() before doing any attempts to modify
 * friend configuration (such as \link linphone_friend_set_address() address \endlink  or \link linphone_friend_set_inc_subscribe_policy() subscription policy\endlink  and so on).
 * Once the modifications are done, then the application must call
 * linphone_friend_done() to commit the changes.
**/
LINPHONE_PUBLIC	void linphone_friend_edit(LinphoneFriend *fr);

/**
 * Commits modification made to the friend configuration.
 * @param fr #LinphoneFriend object
**/
LINPHONE_PUBLIC	void linphone_friend_done(LinphoneFriend *fr);

/**
 * Get the status of a friend
 * @param[in] lf A #LinphoneFriend object
 * @return #LinphoneOnlineStatus
 * @deprecated Use linphone_friend_get_presence_model() instead
 * @donotwrap
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED LinphoneOnlineStatus linphone_friend_get_status(const LinphoneFriend *lf);

/**
 * Get subscription state of a friend
 * @param[in] lf A #LinphoneFriend object
 * @return #LinphoneSubscriptionState
 */

LINPHONE_PUBLIC LinphoneSubscriptionState linphone_friend_get_subscription_state(const LinphoneFriend *lf);

/**
 * Get the presence model of a friend
 * @param[in] lf A #LinphoneFriend object
 * @return A #LinphonePresenceModel object, or NULL if the friend do not have presence information (in which case he is considered offline)
 */
LINPHONE_PUBLIC const LinphonePresenceModel * linphone_friend_get_presence_model(const LinphoneFriend *lf);

/**
 * Get the consolidated presence of a friend.
 * @param[in] lf LinphoneFriend object
 * @return The consolidated presence of the friend
 */
LINPHONE_PUBLIC LinphoneConsolidatedPresence linphone_friend_get_consolidated_presence(const LinphoneFriend *lf);

/**
 * Get the presence model for a specific SIP URI or phone number of a friend
 * @param[in] lf A #LinphoneFriend object
 * @param[in] uri_or_tel The SIP URI or phone number for which to get the presence model
 * @return A #LinphonePresenceModel object, or NULL if the friend do not have presence information for this SIP URI or phone number
 */
LINPHONE_PUBLIC const LinphonePresenceModel * linphone_friend_get_presence_model_for_uri_or_tel(const LinphoneFriend *lf, const char *uri_or_tel);

/**
 * Set the presence model of a friend
 * @param[in] lf A #LinphoneFriend object
 * @param[in] presence The #LinphonePresenceModel object to set for the friend
 */
LINPHONE_PUBLIC void linphone_friend_set_presence_model(LinphoneFriend *lf, LinphonePresenceModel *presence);

/**
 * Set the presence model for a specific SIP URI or phone number of a friend
 * @param[in] lf A #LinphoneFriend object
 * @param[in] uri_or_tel The SIP URI or phone number for which to set the presence model
 * @param[in] presence The #LinphonePresenceModel object to set
 */
LINPHONE_PUBLIC void linphone_friend_set_presence_model_for_uri_or_tel(LinphoneFriend *lf, const char *uri_or_tel, LinphonePresenceModel *presence);

/**
 * Tells whether we already received presence information for a friend.
 * @param[in] lf A #LinphoneFriend object
 * @return TRUE if presence information has been received for the friend, FALSE otherwise.
 */
LINPHONE_PUBLIC bool_t linphone_friend_is_presence_received(const LinphoneFriend *lf);

/**
 * Store user pointer to friend object.
**/
LINPHONE_PUBLIC void linphone_friend_set_user_data(LinphoneFriend *lf, void *data);

/**
 * Retrieve user data associated with friend.
**/
LINPHONE_PUBLIC void* linphone_friend_get_user_data(const LinphoneFriend *lf);

LINPHONE_PUBLIC BuddyInfo * linphone_friend_get_info(const LinphoneFriend *lf);

/**
 * Set the reference key of a friend.
 * @param[in] lf #LinphoneFriend object.
 * @param[in] key The reference key to use for the friend.
**/
LINPHONE_PUBLIC void linphone_friend_set_ref_key(LinphoneFriend *lf, const char *key);

/**
 * Get the reference key of a friend.
 * @param[in] lf #LinphoneFriend object.
 * @return The reference key of the friend.
**/
LINPHONE_PUBLIC const char *linphone_friend_get_ref_key(const LinphoneFriend *lf);

/**
 * Check that the given friend is in a friend list.
 * @param[in] lf #LinphoneFriend object.
 * @return TRUE if the friend is in a friend list, FALSE otherwise.
**/
LINPHONE_PUBLIC bool_t linphone_friend_in_list(const LinphoneFriend *lf);

/**
 * Acquire a reference to the linphone friend.
 * @param[in] lf LinphoneFriend object
 * @return The same LinphoneFriend object
**/
LINPHONE_PUBLIC LinphoneFriend * linphone_friend_ref(LinphoneFriend *lf);

/**
 * Release a reference to the linphone friend.
 * @param[in] lf LinphoneFriend object
**/
LINPHONE_PUBLIC void linphone_friend_unref(LinphoneFriend *lf);

/**
 * Returns the LinphoneCore object managing this friend, if any.
 * @param[in] fr LinphoneFriend object
 */
LINPHONE_PUBLIC LinphoneCore *linphone_friend_get_core(const LinphoneFriend *fr);

/**
 * Returns the vCard object associated to this friend, if any
 * @param[in] fr LinphoneFriend object
 */
LINPHONE_PUBLIC LinphoneVcard* linphone_friend_get_vcard(LinphoneFriend *fr);

/**
 * Binds a vCard object to a friend
 * @param[in] fr LinphoneFriend object
 * @param[in] vcard The vCard object to bind
 */
LINPHONE_PUBLIC void linphone_friend_set_vcard(LinphoneFriend *fr, LinphoneVcard *vcard);

/**
 * Creates a vCard object associated to this friend if there isn't one yet and if the full name is available, either by the parameter or the one in the friend's SIP URI
 * @param[in] fr LinphoneFriend object
 * @param[in] name The full name of the friend or NULL to use the one from the friend's SIP URI
 * @return true if the vCard has been created, false if it wasn't possible (for exemple if name and the friend's SIP URI are null or if the friend's SIP URI doesn't have a display name), or if there is already one vcard
 */
LINPHONE_PUBLIC bool_t linphone_friend_create_vcard(LinphoneFriend *fr, const char *name);

/**
 * Contructor same as linphone_friend_new() + linphone_friend_set_address()
 * @param vcard a vCard object
 * @return a new #LinphoneFriend with \link linphone_friend_get_vcard() vCard initialized \endlink
 */
LINPHONE_PUBLIC	LinphoneFriend *linphone_friend_new_from_vcard(LinphoneVcard *vcard);

/**
 * Saves a friend either in database if configured, otherwise in linphonerc
 * @param fr the linphone friend to save
 * @param lc the linphone core
 */
LINPHONE_PUBLIC void linphone_friend_save(LinphoneFriend *fr, LinphoneCore *lc);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_FRIEND_H_ */
