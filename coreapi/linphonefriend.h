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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef LINPHONEFRIEND_H_
#define LINPHONEFRIEND_H_

#include "linphonepresence.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup buddy_list
 * @{
 */
/**
 * Enum controlling behavior for incoming subscription request.
 * <br> Use by linphone_friend_set_inc_subscribe_policy()
 */
typedef enum _LinphoneSubscribePolicy {
	/**
	 * Does not automatically accept an incoming subscription request.
	 * This policy implies that a decision has to be taken for each incoming subscription request notified by callback LinphoneCoreVTable.new_subscription_requested
	 *
	 */
	LinphoneSPWait,
	/**
	 * Rejects incoming subscription request.
	 */
	LinphoneSPDeny,
	/**
	 * Automatically accepts a subscription request.
	 */
	LinphoneSPAccept
} LinphoneSubscribePolicy;

/**
 * Enum describing remote friend status
 * @deprecated Use #LinphonePresenceModel and #LinphonePresenceActivity instead
 */
typedef enum _LinphoneOnlineStatus{
	/**
	 * Offline
	 */
	LinphoneStatusOffline,
	/**
	 * Online
	 */
	LinphoneStatusOnline,
	/**
	 * Busy
	 */
	LinphoneStatusBusy,
	/**
	 * Be right back
	 */
	LinphoneStatusBeRightBack,
	/**
	 * Away
	 */
	LinphoneStatusAway,
	/**
	 * On the phone
	 */
	LinphoneStatusOnThePhone,
	/**
	 * Out to lunch
	 */
	LinphoneStatusOutToLunch,
	/**
	 * Do not disturb
	 */
	LinphoneStatusDoNotDisturb,
	/**
	 * Moved in this sate, call can be redirected if an alternate contact address has been set using function linphone_core_set_presence_info()
	 */
	LinphoneStatusMoved,
	/**
	 * Using another messaging service
	 */
	LinphoneStatusAltService,
	/**
	 * Pending
	 */
	LinphoneStatusPending,
	/**
	 * Vacation
	 */
	LinphoneStatusVacation,

	LinphoneStatusEnd
}LinphoneOnlineStatus;


struct _LinphoneFriend;
/**
 * Represents a buddy, all presence actions like subscription and status change notification are performed on this object
 */
typedef struct _LinphoneFriend LinphoneFriend;

/**
 * Contructor
 * @return a new empty #LinphoneFriend
 * @deprecated use #linphone_core_create_friend instead
 */
LINPHONE_PUBLIC MS2_DEPRECATED LinphoneFriend * linphone_friend_new(void); /*fix me me replace MS2_DEPRECATED by LINPHONE_DEPRECATED*/

/**
 * Contructor same as linphone_friend_new() + linphone_friend_set_address()
 * @param addr a buddy address, must be a sip uri like sip:joe@sip.linphone.org
 * @return a new #LinphoneFriend with \link linphone_friend_get_address() address initialized \endlink
 * @deprecated use #linphone_core_create_friend_with_address instead
 */
LINPHONE_PUBLIC	MS2_DEPRECATED LinphoneFriend *linphone_friend_new_with_address(const char *addr); /*fix me me replace MS2_DEPRECATED by LINPHONE_DEPRECATED*/

/**
 * Contructor same as linphone_friend_new() + linphone_friend_set_address()
 * @deprecated Use #linphone_friend_new_with_address instead
 */
#define linphone_friend_new_with_addr linphone_friend_new_with_address

/**
 * Destroy a LinphoneFriend.
 * @param lf LinphoneFriend object
 * @deprecated Use linphone_friend_unref() instead.
 */
LINPHONE_PUBLIC void linphone_friend_destroy(LinphoneFriend *lf);

/**
 * Set #LinphoneAddress for this friend
 * @param fr #LinphoneFriend object
 * @param address #LinphoneAddress
 */
LINPHONE_PUBLIC int linphone_friend_set_address(LinphoneFriend *fr, const LinphoneAddress* address);

/**
 * Set #LinphoneAddress for this friend
 * @deprecated Use #linphone_friend_set_address instead
 */
#define linphone_friend_set_addr linphone_friend_set_address

/**
 * Get address of this friend
 * @param lf #LinphoneFriend object
 * @return #LinphoneAddress
 */
LINPHONE_PUBLIC	const LinphoneAddress *linphone_friend_get_address(const LinphoneFriend *lf);

/**
 * Set the display name for this friend
 * @param lf #LinphoneFriend object
 * @param name 
 */
LINPHONE_PUBLIC int linphone_friend_set_name(LinphoneFriend *lf, const char *name);

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

LINPHONE_PUBLIC	int linphone_friend_enable_subscribes(LinphoneFriend *fr, bool_t val);
#define linphone_friend_send_subscribe linphone_friend_enable_subscribes

/**
 * Configure incoming subscription policy for this friend.
 * @param fr #LinphoneFriend object
 * @param pol #LinphoneSubscribePolicy policy to apply.
 */
LINPHONE_PUBLIC int linphone_friend_set_inc_subscribe_policy(LinphoneFriend *fr, LinphoneSubscribePolicy pol);

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
 */
LINPHONE_PUBLIC LinphoneOnlineStatus linphone_friend_get_status(const LinphoneFriend *lf);

/**
 * Get the presence model of a friend
 * @param[in] lf A #LinphoneFriend object
 * @return A #LinphonePresenceModel object, or NULL if the friend do not have presence information (in which case he is considered offline)
 */
LINPHONE_PUBLIC const LinphonePresenceModel * linphone_friend_get_presence_model(LinphoneFriend *lf);

/**
 * Set the presence model of a friend
 * @param[in] lf A #LinphoneFriend object
 * @param[in] presence The #LinphonePresenceModel object to set for the friend
 */
LINPHONE_PUBLIC void linphone_friend_set_presence_model(LinphoneFriend *lf, LinphonePresenceModel *presence);

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
 * Return humain readable presence status
 * @param ss
 * @deprecated Use #LinphonePresenceModel, #LinphonePresenceActivity and linphone_presence_activity_to_string() instead.
 */
LINPHONE_PUBLIC const char *linphone_online_status_to_string(LinphoneOnlineStatus ss);


/**
 * Create a default LinphoneFriend.
 * @param[in] lc #LinphoneCore object
 * @return The created #LinphoneFriend object
 */
LINPHONE_PUBLIC LinphoneFriend * linphone_core_create_friend(LinphoneCore *lc);

/**
 * Create a LinphoneFriend from the given address.
 * @param[in] lc #LinphoneCore object
 * @param[in] address A string containing the address to create the LinphoneFriend from
 * @return The created #LinphoneFriend object
 */
LINPHONE_PUBLIC LinphoneFriend * linphone_core_create_friend_with_address(LinphoneCore *lc, const char *address);

/**
 * Set my presence status
 * @param[in] lc #LinphoneCore object
 * @param[in] minutes_away how long in away
 * @param[in] alternative_contact sip uri used to redirect call in state #LinphoneStatusMoved
 * @param[in] os #LinphoneOnlineStatus
 * @deprecated Use linphone_core_set_presence_model() instead
 */
LINPHONE_PUBLIC void linphone_core_set_presence_info(LinphoneCore *lc,int minutes_away,const char *alternative_contact,LinphoneOnlineStatus os);

/**
 * Set my presence model
 * @param[in] lc #LinphoneCore object
 * @param[in] presence #LinphonePresenceModel
 */
LINPHONE_PUBLIC void linphone_core_set_presence_model(LinphoneCore *lc, LinphonePresenceModel *presence);

/**
 * Get my presence status
 * @param[in] lc #LinphoneCore object
 * @return #LinphoneOnlineStatus
 * @deprecated Use linphone_core_get_presence_model() instead
 */
LINPHONE_PUBLIC LinphoneOnlineStatus linphone_core_get_presence_info(const LinphoneCore *lc);

/**
 * Get my presence model
 * @param[in] lc #LinphoneCore object
 * @return A #LinphonePresenceModel object, or NULL if no presence model has been set.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_core_get_presence_model(const LinphoneCore *lc);

/**
 * @deprecated Use linphone_core_interpret_url() instead
 */
LINPHONE_PUBLIC void linphone_core_interpret_friend_uri(LinphoneCore *lc, const char *uri, char **result);

/**
 * Add a friend to the current buddy list, if \link linphone_friend_enable_subscribes() subscription attribute \endlink is set, a SIP SUBSCRIBE message is sent.
 * @param lc #LinphoneCore object
 * @param fr #LinphoneFriend to add
 */
LINPHONE_PUBLIC	void linphone_core_add_friend(LinphoneCore *lc, LinphoneFriend *fr);

/**
 * remove a friend from the buddy list
 * @param lc #LinphoneCore object
 * @param fr #LinphoneFriend to add
 */
LINPHONE_PUBLIC void linphone_core_remove_friend(LinphoneCore *lc, LinphoneFriend *fr);

/**
 * Black list a friend. same as linphone_friend_set_inc_subscribe_policy() with #LinphoneSPDeny policy;
 * @param lc #LinphoneCore object
 * @param lf #LinphoneFriend to add
 */
LINPHONE_PUBLIC void linphone_core_reject_subscriber(LinphoneCore *lc, LinphoneFriend *lf);

/**
 * Get Buddy list of LinphoneFriend
 * @param[in] lc #LinphoneCore object
 * @return \mslist{LinphoneFriend}
 */
LINPHONE_PUBLIC	const MSList * linphone_core_get_friend_list(const LinphoneCore *lc);

/**
 * Notify all friends that have subscribed
 * @param lc #LinphoneCore object
 * @param presence #LinphonePresenceModel to notify
 */
LINPHONE_PUBLIC void linphone_core_notify_all_friends(LinphoneCore *lc, LinphonePresenceModel *presence);

/**
 * Search a LinphoneFriend by its address.
 * @param[in] lc #LinphoneCore object.
 * @param[in] addr The address to use to search the friend.
 * @return The #LinphoneFriend object corresponding to the given address.
 * @deprecated use linphone_core_find_friend() instead.
 */
LINPHONE_PUBLIC LinphoneFriend *linphone_core_get_friend_by_address(const LinphoneCore *lc, const char *addr);

/**
 * Search a LinphoneFriend by its address.
 * @param[in] lc #LinphoneCore object.
 * @param[in] addr The address to use to search the friend.
 * @return The #LinphoneFriend object corresponding to the given address.
 */
LINPHONE_PUBLIC LinphoneFriend *linphone_core_find_friend(const LinphoneCore *lc, const LinphoneAddress *addr);

/**
 * Search a LinphoneFriend by its reference key.
 * @param[in] lc #LinphoneCore object.
 * @param[in] key The reference key to use to search the friend.
 * @return The #LinphoneFriend object corresponding to the given reference key.
 */
LINPHONE_PUBLIC LinphoneFriend *linphone_core_get_friend_by_ref_key(const LinphoneCore *lc, const char *key);

/**
 * Acquire a reference to the linphone friend.
 * @param[in] lf LinphoneFriend object
 * @return The same LinphoneFriend object
**/
LINPHONE_PUBLIC LinphoneFriend * linphone_friend_ref(LinphoneFriend *lf);

/**
 * Release a reference to the linphone friend.
 * @param[in] lf LinohoneFriend object
**/
LINPHONE_PUBLIC void linphone_friend_unref(LinphoneFriend *lf);

/**
 * Returns the LinphoneCore object managing this friend, if any.
 */
LINPHONE_PUBLIC LinphoneCore *linphone_friend_get_core(const LinphoneFriend *fr);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINPHONEFRIEND_H_ */
