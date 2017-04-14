/*
linphone
Copyright (C) 2000 - 2010 Simon MORLAT (simon.morlat@linphone.org)

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

#ifndef LINPHONE_EVENT_H_
#define LINPHONE_EVENT_H_

#include "linphone/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup event_api
 * @{
**/

/**
 * Send a subscription previously created by linphone_core_create_subscribe().
 * @param ev the LinphoneEvent
 * @param body optional content to attach with the subscription.
 * @return 0 if successful, -1 otherwise.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_event_send_subscribe(LinphoneEvent *ev, const LinphoneContent *body);

/**
 * Update (refresh) an outgoing subscription, changing the body.
 * @param lev a LinphoneEvent
 * @param body an optional body to include in the subscription update, may be NULL.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_event_update_subscribe(LinphoneEvent *lev, const LinphoneContent *body);

/**
 * Refresh an outgoing subscription keeping the same body.
 * @param lev LinphoneEvent object.
 * @return 0 if successful, -1 otherwise.
 */
LINPHONE_PUBLIC LinphoneStatus linphone_event_refresh_subscribe(LinphoneEvent *lev);


/**
 * Accept an incoming subcription.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_event_accept_subscription(LinphoneEvent *lev);

/**
 * Deny an incoming subscription with given reason.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_event_deny_subscription(LinphoneEvent *lev, LinphoneReason reason);

/**
 * Send a notification.
 * @param lev a #LinphoneEvent corresponding to an incoming subscription previously received and accepted.
 * @param body an optional body containing the actual notification data.
 * @return 0 if successful, -1 otherwise.
 **/
LINPHONE_PUBLIC LinphoneStatus linphone_event_notify(LinphoneEvent *lev, const LinphoneContent *body);

/**
 * Send a publish created by linphone_core_create_publish().
 * @param lev the #LinphoneEvent
 * @param body the new data to be published
**/
LINPHONE_PUBLIC LinphoneStatus linphone_event_send_publish(LinphoneEvent *lev, const LinphoneContent *body);

/**
 * Update (refresh) a publish.
 * @param lev the #LinphoneEvent
 * @param body the new data to be published
**/
LINPHONE_PUBLIC LinphoneStatus linphone_event_update_publish(LinphoneEvent *lev, const LinphoneContent *body);

/**
 * Refresh an outgoing publish keeping the same body.
 * @param lev LinphoneEvent object.
 * @return 0 if successful, -1 otherwise.
 */
LINPHONE_PUBLIC LinphoneStatus linphone_event_refresh_publish(LinphoneEvent *lev);

/**
 * Prevent an event from refreshing its publish.
 * This is useful to let registrations to expire naturally (or) when the application wants to keep control on when
 * refreshes are sent.
 * The refreshing operations can be resumed with linphone_proxy_config_refresh_register().
 * @param[in] lev #LinphoneEvent object.
 **/
LINPHONE_PUBLIC void linphone_event_pause_publish(LinphoneEvent *lev);

/**
 * Return reason code (in case of error state reached).
**/
LINPHONE_PUBLIC LinphoneReason linphone_event_get_reason(const LinphoneEvent *lev);

/**
 * Get full details about an error occured.
**/
LINPHONE_PUBLIC const LinphoneErrorInfo *linphone_event_get_error_info(const LinphoneEvent *lev);

/**
 * Get subscription state. If the event object was not created by a subscription mechanism, #LinphoneSubscriptionNone is returned.
**/
LINPHONE_PUBLIC LinphoneSubscriptionState linphone_event_get_subscription_state(const LinphoneEvent *lev);

/**
 * Get publish state. If the event object was not created by a publish mechanism, #LinphonePublishNone is returned.
**/
LINPHONE_PUBLIC LinphonePublishState linphone_event_get_publish_state(const LinphoneEvent *lev);

/**
 * Get subscription direction.
 * If the object wasn't created by a subscription mechanism, #LinphoneSubscriptionInvalidDir is returned.
**/
LINPHONE_PUBLIC LinphoneSubscriptionDir linphone_event_get_subscription_dir(LinphoneEvent *lev);

/**
 * Set a user (application) pointer.
**/
LINPHONE_PUBLIC void linphone_event_set_user_data(LinphoneEvent *ev, void *up);

/**
 * Retrieve user pointer.
**/
LINPHONE_PUBLIC void *linphone_event_get_user_data(const LinphoneEvent *ev);

/**
 * Add a custom header to an outgoing susbscription or publish.
 * @param ev the LinphoneEvent
 * @param name header's name
 * @param value the header's value.
**/
LINPHONE_PUBLIC void linphone_event_add_custom_header(LinphoneEvent *ev, const char *name, const char *value);

/**
 * Obtain the value of a given header for an incoming subscription.
 * @param ev the LinphoneEvent
 * @param name header's name
 * @return the header's value or NULL if such header doesn't exist.
**/
LINPHONE_PUBLIC const char *linphone_event_get_custom_header(LinphoneEvent *ev, const char *name);

/**
 * Terminate an incoming or outgoing subscription that was previously acccepted, or a previous publication.
 * The LinphoneEvent shall not be used anymore after this operation, unless the application explicitely took a reference on the object with
 * linphone_event_ref().
**/
LINPHONE_PUBLIC void linphone_event_terminate(LinphoneEvent *lev);

/**
 * Increase reference count of LinphoneEvent.
 * By default LinphoneEvents created by the core are owned by the core only.
 * An application that wishes to retain a reference to it must call linphone_event_ref().
 * When this reference is no longer needed, linphone_event_unref() must be called.
 *
**/
LINPHONE_PUBLIC LinphoneEvent *linphone_event_ref(LinphoneEvent *lev);

/**
 * Decrease reference count.
 * @see linphone_event_ref()
**/
LINPHONE_PUBLIC void linphone_event_unref(LinphoneEvent *lev);

/**
 * Get the name of the event as specified in the event package RFC.
**/
LINPHONE_PUBLIC const char *linphone_event_get_name(const LinphoneEvent *lev);

/**
 * Get the "from" address of the subscription.
**/
LINPHONE_PUBLIC const LinphoneAddress *linphone_event_get_from(const LinphoneEvent *lev);

/**
 * Get the resource address of the subscription or publish.
**/
LINPHONE_PUBLIC const LinphoneAddress *linphone_event_get_resource(const LinphoneEvent *lev);

/**
 * Returns back pointer to the LinphoneCore that created this LinphoneEvent
**/
LINPHONE_PUBLIC LinphoneCore *linphone_event_get_core(const LinphoneEvent *lev);

/**
 * @}
**/

#ifdef __cplusplus
}
#endif


#endif /* LINPHONE_EVENT_H_ */
