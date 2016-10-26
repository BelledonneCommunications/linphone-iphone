package org.linphone.core;

/**
 * LinphoneEvent
 */
public interface LinphoneEvent {
	/**
	 * Get the event name as standardized by the event package RFC.
	 * @return the event name.
	 */
	String getEventName();
	
	/**
	 * Return subscription direction (incoming or outgoing). For publish initiated LinphoneEvent it is set to Invalid.
	 * @return the subscription direction.
	 */
	SubscriptionDir getSubscriptionDir();
	
	/**
	 * Get subscription state.
	 * @return the current subscription state.
	 */
	SubscriptionState getSubscriptionState();
	/**
	 * Accept an incoming subscription. After it is accepted the application can immediately start to send notifications with
	 * {@link #notify(LinphoneContent) notify()}.
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
	 * Update a subscription initiated previously with {@link LinphoneCore#subscribe(LinphoneAddress, String, int, LinphoneContent) LinphoneCore.subscribe()}
	 * @param content the data to be put in the subscribe request.
	 */
	void updateSubscribe(LinphoneContent content);
	
	/**
	 * Update a Publish previously started with {@link LinphoneCore#publish(LinphoneAddress, String, int, LinphoneContent)}.
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
	 * In case of error notified, returns the full error details.
	 * @return an ErrorInfo.
	 */
	ErrorInfo getErrorInfo();
	
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
	
	/**
	 * Add a custom header to an outgoing susbscription or publish.
	 * @param name header's name
	 * @param value the header's value.
	 */
	void addCustomHeader(String name, String value);
	
	/**
	 * Obtain the value of a given header for an incoming subscription.
	 * @param name header's name
	 * @return the header's value or NULL if such header doesn't exist.
	 */
	String getCustomHeader(String name);
	
	/**
	 * Send a subscription previously created by linphone_core_create_subscribe().
	 * @param body optional content to attach with the subscription.
	 */
	void sendSubscribe(LinphoneContent body);
	
	/**
	 * Send a publish created by linphone_core_create_publish().
	 * @param body the new data to be published
	 */
	void sendPublish(LinphoneContent body);
	
	/**
	 * Get a back pointer to the LinphoneCore object managing this LinphoneEvent.
	 * @return
	 */
	LinphoneCore getCore();
}
