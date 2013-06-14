package org.linphone.core;

public interface LinphoneEvent {
	/**
	 * Get the event name as standardized by the event package RFC.
	 * @return the event name.
	 */
	String getEventName();
	/**
	 * Accept an incoming subscription. After it is accepted the application can immediately start to send notifications with
	 * {@link LinphoneEvent.notify() }.
	 */
	void acceptSubscription();
	
	/**
	 * Reject an incoming subscription.
	 * @param reason reason code for rejection.
	 */
	void denySubscription(Reason reason);
	
	/**
	 * Sends a NOTIFY request in the context of a LinphoneEvent created by an incoming subscription.
	 * @param content the data to be put in the notification.
	 */
	void notify(LinphoneContent content);
	
	/**
	 * Update a subscription initiated previously with {@link LinphoneCore.subscribe() }
	 * @param content the data to be put in the subscribe request.
	 */
	void updateSubscribe(LinphoneContent content);
	
	/**
	 * Update a Publish previously started with {@link LinphoneCore.publish() }.
	 * @param content the data to be put in the publish request.
	 */
	void updatePublish(LinphoneContent content);
	
	/**
	 * Terminate an outgoing or incoming subscription, depending on the way the LinphoneEvent was created.
	 */
	void terminate();
	/**
	 * In the event where an error would be returned or notified relatively to this LinphoneEvent, returns a reason error code.
	 * @return
	 */
	Reason getReason();
	
	/**
	 * Assign an application context to the LinphoneEvent, for later use.
	 * @param obj
	 */
	void setUserContext(Object obj);
	/**
	 * Retrieve application context previously set by setUserContext().
	 * @return
	 */
	Object getUserContext();
}
