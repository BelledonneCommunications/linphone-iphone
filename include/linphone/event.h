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
#ifndef LINPHONEEVENT_H
#define LINPHONEEVENT_H

/**
 * @addtogroup event_api
 * @{
**/

struct _LinphoneEvent;

/**
 * Object representing an event state, which is subcribed or published.
 * @see linphone_core_publish()
 * @see linphone_core_subscribe()
**/
typedef struct _LinphoneEvent LinphoneEvent;

/**
 * Enum for subscription direction (incoming or outgoing).
**/
enum _LinphoneSubscriptionDir{
	LinphoneSubscriptionIncoming, /**< Incoming subscription. */
	LinphoneSubscriptionOutgoing, /**< Outgoing subscription. */
	LinphoneSubscriptionInvalidDir /**< Invalid subscription direction. */
};

/**
 * Typedef alias for _LinphoneSubscriptionDir
**/
typedef enum _LinphoneSubscriptionDir LinphoneSubscriptionDir;

/**
 * Enum for subscription states.
 * LinphoneSubscriptionTerminated and LinphoneSubscriptionError are final states.
**/
enum _LinphoneSubscriptionState{
	LinphoneSubscriptionNone, /**< Initial state, should not be used.**/
	LinphoneSubscriptionOutgoingProgress, /**<An outgoing subcription was sent*/
	LinphoneSubscriptionIncomingReceived, /**<An incoming subcription is received*/
	LinphoneSubscriptionPending, /**<Subscription is pending, waiting for user approval*/
	LinphoneSubscriptionActive, /**<Subscription is accepted.*/
	LinphoneSubscriptionTerminated, /**<Subscription is terminated normally*/
	LinphoneSubscriptionError, /**<Subscription was terminated by an error, indicated by linphone_event_get_reason().*/
	LinphoneSubscriptionExpiring, /**<Subscription is about to expire, only sent if [sip]->refresh_generic_subscribe property is set to 0.*/
};

/**
 * Typedef for subscription state enum.
**/
typedef enum _LinphoneSubscriptionState LinphoneSubscriptionState;

LINPHONE_PUBLIC const char *linphone_subscription_state_to_string(LinphoneSubscriptionState state);

/**
 * Enum for publish states.
**/
enum _LinphonePublishState{
	LinphonePublishNone, /**< Initial state, do not use**/
	LinphonePublishProgress, /**<An outgoing publish was created and submitted*/
	LinphonePublishOk, /**<Publish is accepted.*/
	LinphonePublishError, /**<Publish encoutered an error, linphone_event_get_reason() gives reason code*/
	LinphonePublishExpiring, /**<Publish is about to expire, only sent if [sip]->refresh_generic_publish property is set to 0.*/
	LinphonePublishCleared /**<Event has been un published*/
};

/**
 * Typedef for publish state enum
**/
typedef enum _LinphonePublishState LinphonePublishState;

LINPHONE_PUBLIC const char *linphone_publish_state_to_string(LinphonePublishState state);

/**
 * Callback prototype for notifying the application about notification received from the network.
**/
typedef void (*LinphoneCoreCbsNotifyReceivedCb)(LinphoneCore *lc, LinphoneEvent *lev, const char *notified_event, const LinphoneContent *body);
/**
 * Old name of #LinphoneCoreCbsNotifyReceivedCb.
 */
typedef LinphoneCoreCbsNotifyReceivedCb LinphoneCoreNotifyReceivedCb;

/**
 * Callback prototype for notifying the application about changes of subscription states, including arrival of new subscriptions.
**/
typedef void (*LinphoneCoreCbsSubscriptionStateChangedCb)(LinphoneCore *lc, LinphoneEvent *lev, LinphoneSubscriptionState state);
/**
 * Old name of #LinphoneCoreCbsSubscriptionStateChangedCb.
 */
typedef LinphoneCoreCbsSubscriptionStateChangedCb LinphoneCoreSubscriptionStateChangedCb;

/**
 * Callback prototype for notifying the application about changes of publish states.
**/
typedef void (*LinphoneCoreCbsPublishStateChangedCb)(LinphoneCore *lc, LinphoneEvent *lev, LinphonePublishState state);
/**
 * Old name of LinphoneCoreCbsPublishStateChangedCb.
 */
typedef LinphoneCoreCbsPublishStateChangedCb LinphoneCorePublishStateChangedCb;

/**
 * Create an outgoing subscription, specifying the destination resource, the event name, and an optional content body.
 * If accepted, the subscription runs for a finite period, but is automatically renewed if not terminated before.
 * @param lc the #LinphoneCore
 * @param resource the destination resource
 * @param event the event name
 * @param expires the whished duration of the subscription
 * @param body an optional body, may be NULL.
 * @return a LinphoneEvent holding the context of the created subcription.
**/
LINPHONE_PUBLIC LinphoneEvent *linphone_core_subscribe(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires, const LinphoneContent *body);

/**
 * Create an outgoing subscription, specifying the destination resource, the event name, and an optional content body.
 * If accepted, the subscription runs for a finite period, but is automatically renewed if not terminated before.
 * Unlike linphone_core_subscribe() the subscription isn't sent immediately. It will be send when calling linphone_event_send_subscribe().
 * @param lc the #LinphoneCore
 * @param resource the destination resource
 * @param event the event name
 * @param expires the whished duration of the subscription
 * @return a LinphoneEvent holding the context of the created subcription.
**/
LINPHONE_PUBLIC LinphoneEvent *linphone_core_create_subscribe(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires);


/**
 * Create an out-of-dialog notification, specifying the destination resource, the event name.
 * The notification can be send with linphone_event_notify().
 * @param lc the #LinphoneCore
 * @param resource the destination resource
 * @param event the event name
 * @return a LinphoneEvent holding the context of the notification.
**/
LINPHONE_PUBLIC LinphoneEvent *linphone_core_create_notify(LinphoneCore *lc, const LinphoneAddress *resource, const char *event);


/**
 * Send a subscription previously created by linphone_core_create_subscribe().
 * @param ev the LinphoneEvent
 * @param body optional content to attach with the subscription.
 * @return 0 if successful, -1 otherwise.
**/
LINPHONE_PUBLIC int linphone_event_send_subscribe(LinphoneEvent *ev, const LinphoneContent *body);

/**
 * Update (refresh) an outgoing subscription, changing the body.
 * @param lev a LinphoneEvent
 * @param body an optional body to include in the subscription update, may be NULL.
**/
LINPHONE_PUBLIC int linphone_event_update_subscribe(LinphoneEvent *lev, const LinphoneContent *body);

/**
 * Refresh an outgoing subscription keeping the same body.
 * @param lev LinphoneEvent object.
 * @return 0 if successful, -1 otherwise.
 */
LINPHONE_PUBLIC int linphone_event_refresh_subscribe(LinphoneEvent *lev);


/**
 * Accept an incoming subcription.
**/
LINPHONE_PUBLIC int linphone_event_accept_subscription(LinphoneEvent *lev);
/**
 * Deny an incoming subscription with given reason.
**/
LINPHONE_PUBLIC int linphone_event_deny_subscription(LinphoneEvent *lev, LinphoneReason reason);
/**
 * Send a notification.
 * @param lev a #LinphoneEvent corresponding to an incoming subscription previously received and accepted.
 * @param body an optional body containing the actual notification data.
 * @return 0 if successful, -1 otherwise.
 **/
LINPHONE_PUBLIC int linphone_event_notify(LinphoneEvent *lev, const LinphoneContent *body);


/**
 * Publish an event state.
 * This first create a LinphoneEvent with linphone_core_create_publish() and calls linphone_event_send_publish() to actually send it.
 * After expiry, the publication is refreshed unless it is terminated before.
 * @param lc the #LinphoneCore
 * @param resource the resource uri for the event
 * @param event the event name
 * @param expires the lifetime of event being published, -1 if no associated duration, in which case it will not be refreshed.
 * @param body the actual published data
 * @return the LinphoneEvent holding the context of the publish.
**/
LINPHONE_PUBLIC LinphoneEvent *linphone_core_publish(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires, const LinphoneContent *body);

/**
 * Create a publish context for an event state.
 * After being created, the publish must be sent using linphone_event_send_publish().
 * After expiry, the publication is refreshed unless it is terminated before.
 * @param lc the #LinphoneCore
 * @param resource the resource uri for the event
 * @param event the event name
 * @param expires the lifetime of event being published, -1 if no associated duration, in which case it will not be refreshed.
 * @return the LinphoneEvent holding the context of the publish.
**/
LINPHONE_PUBLIC LinphoneEvent *linphone_core_create_publish(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires);


/**
 * Create a publish context for a one-shot publish.
 * After being created, the publish must be sent using linphone_event_send_publish().
 * The LinphoneEvent is automatically terminated when the publish transaction is finished, either with success or failure.
 * The application must not call linphone_event_terminate() for such one-shot publish.
 * @param lc the #LinphoneCore
 * @param resource the resource uri for the event
 * @param event the event name
 * @return the LinphoneEvent holding the context of the publish.
**/
LINPHONE_PUBLIC LinphoneEvent *linphone_core_create_one_shot_publish(LinphoneCore *lc, const LinphoneAddress *resource, const char *event);

/**
 * Send a publish created by linphone_core_create_publish().
 * @param lev the #LinphoneEvent
 * @param body the new data to be published
**/
LINPHONE_PUBLIC int linphone_event_send_publish(LinphoneEvent *lev, const LinphoneContent *body);

/**
 * Update (refresh) a publish.
 * @param lev the #LinphoneEvent
 * @param body the new data to be published
**/
LINPHONE_PUBLIC int linphone_event_update_publish(LinphoneEvent *lev, const LinphoneContent *body);

/**
 * Refresh an outgoing publish keeping the same body.
 * @param lev LinphoneEvent object.
 * @return 0 if successful, -1 otherwise.
 */
LINPHONE_PUBLIC int linphone_event_refresh_publish(LinphoneEvent *lev);

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


#endif
