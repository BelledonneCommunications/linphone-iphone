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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef LINPHONEEVENT_H
#define LINPHONEEVENT_H

/**
 * @addtogroup subscriptions
 * @{
**/

struct _LinphoneEvent;

typedef struct _LinphoneEvent LinphoneEvent;

/**
 * Enum for subscription direction (incoming or outgoing).
**/
enum _LinphoneSubscriptionDir{
	LinphoneSubscriptionIncoming,
	LinphoneSubscriptionOutgoing,
	LinphoneSubscriptionInvalidDir
};

/**
 * Typedef alias for _LinphoneSubscriptionDir
**/
typedef enum _LinphoneSubscriptionDir LinphoneSubscriptionDir;

/**
 * Enum for subscription states.
**/
enum _LinphoneSubscriptionState{
	LinphoneSubscriptionNone, /**< Initial state, should not be used.**/
	LinphoneSubscriptionOutoingInit, /**<An outgoing subcription was created*/
	LinphoneSubscriptionIncomingReceived, /**<An incoming subcription is received*/
	LinphoneSubscriptionPending, /**<Subscription is pending, waiting for user approval*/
	LinphoneSubscriptionActive, /**<Subscription is accepted.*/
	LinphoneSubscriptionTerminated, /**<Subscription is terminated normally*/
	LinphoneSubscriptionError /**<Subscription encountered an error, indicated by linphone_event_get_reason()*/
};

typedef enum _LinphoneSubscriptionState LinphoneSubscriptionState;

/**
 * Callback prototype for notifying the application about notification received from the network.
**/
typedef void (*LinphoneEventIncomingNotifyCb)(LinphoneCore *lc, LinphoneEvent *lev, const char *notified_event, const LinphoneContent *body);

/**
 * Callback prototype for notifying the application about changes of subscription states, including arrival of new subscriptions.
**/ 
typedef void (*LinphoneSubscriptionStateChangedCb)(LinphoneCore *lc, LinphoneEvent *lev, LinphoneSubscriptionState state);

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
LinphoneEvent *linphone_core_subscribe(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires, const LinphoneContent *body);

/**
 * Update an outgoing subscription.
 * @param lev a LinphoneEvent
 * @param body an optional body to include in the subscription update, may be NULL.
**/
int linphone_event_update_subscribe(LinphoneEvent *lev, const LinphoneContent *body);


/**
 * Accept an incoming subcription.
**/
int linphone_event_accept_subscription(LinphoneEvent *lev);
/**
 * Deny an incoming subscription with given reason.
**/
int linphone_event_deny_subscription(LinphoneEvent *lev, LinphoneReason reason);
/**
 * Send a notification.
 * @param lev a #LinphoneEvent corresponding to an incoming subscription previously received and accepted.
 * @param body an optional body containing the actual notification data.
 * @return 0 if successful, -1 otherwise.
 **/
int linphone_event_notify(LinphoneEvent *lev, const LinphoneContent *body);


/**
 * Publish an event.
 * After expiry, the publication is refreshed unless it is terminated before.
 * @param lc the #LinphoneCore
 * @param resource the resource uri for the event
 * @param event the event name
 * @param expires the lifetime of the publication
 * @param body the actual published data
 * @return the LinphoneEvent holding the context of the publish.
**/
LinphoneEvent *linphone_core_publish(LinphoneCore *lc, const LinphoneAddress *resource, const char *event, int expires, const LinphoneContent *body);

/**
 * Update a publication.
 * @param lev the #LinphoneEvent
 * @param body the new data to be published
**/
int linphone_event_update_publish(LinphoneEvent *lev, const LinphoneContent *body);


/**
 * Return reason code (in case of error state reached).
**/
LinphoneReason linphone_event_get_reason(const LinphoneEvent *lev);

/**
 * Get subscription state. If the event object was not created by a subscription mechanism, #LinphoneSubscriptionNone is returned.
**/
LinphoneSubscriptionState linphone_event_get_subscription_state(const LinphoneEvent *lev);

/**
 * Get subscription direction.
 * If the object wasn't created by a subscription mechanism, #LinphoneSubscriptionInvalidDir is returned.
**/
LinphoneSubscriptionDir linphone_event_get_subscription_dir(LinphoneEvent *lev);

/**
 * Set a user (application) pointer.
**/
void linphone_event_set_user_data(LinphoneEvent *ev, void *up);

/**
 * Retrieve user pointer.
**/
void *linphone_event_get_user_data(const LinphoneEvent *ev);

/**
 * Terminate an incoming or outgoing subscription that was previously acccepted, or a previous publication.
**/
void linphone_event_terminate(LinphoneEvent *lev);


/**
 * Increase reference count.
**/
LinphoneEvent *linphone_event_ref(LinphoneEvent *lev);

/**
 * Decrease reference count.
**/
void linphone_event_unref(LinphoneEvent *lev);

/**
 * Get the name of the event as specified in the event package RFC.
**/
const char *linphone_event_get_name(const LinphoneEvent *lev);

/**
 * Get the "from" address of the subscription.
**/
const LinphoneAddress *linphone_event_get_from(const LinphoneEvent *lev);

/**
 * Get the resource address of the subscription or publish.
**/
const LinphoneAddress *linphone_event_get_resource(const LinphoneEvent *lev);

/**
 * @}
**/


#endif
