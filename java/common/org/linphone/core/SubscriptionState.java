package org.linphone.core;

public enum SubscriptionState {
	/**
	 * Initial state, should not be used.
	 */
	LinphoneSubscriptionNone, 
	/**
	 * An outgoing subcription was created.
	 */
	LinphoneSubscriptionOutoingInit,
	/**
	 * An incoming subcription is received.
	 */
	LinphoneSubscriptionIncomingReceived, 
	/**
	 * Subscription is pending, waiting for user approval
	 */
	LinphoneSubscriptionPending,
	/**
	 * Subscription is accepted and now active.
	 */
	LinphoneSubscriptionActive,
	/**
	 * Subscription is terminated normally
	 */
	LinphoneSubscriptionTerminated, 
	/**
	 * Subscription encountered an error, indicated by { @link LinphoneEvent.getReason() }
	 */
	LinphoneSubscriptionError
}
