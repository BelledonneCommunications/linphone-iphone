/*
LinphoneCoreListener.java
Copyright (C) 2010  Belledonne Communications, Grenoble, France

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
package org.linphone.core;

import java.nio.ByteBuffer;


/**
 * 
 *This interface holds all callbacks that the application should implement. None is mandatory.
 */
public interface LinphoneCoreListener {

	/**
	 * @deprecated
	 * Ask the application some authentication information 
	 **/
	@Deprecated
	void authInfoRequested(LinphoneCore lc, String realm, String username, String domain);
	
	/**
	 * Ask the application some authentication information
	 * @param lc the LinphoneCore
	 * @param authInfo a LinphoneAuthInfo pre-filled with username, realm and domain values as much as possible
	 * @param method the type of authentication requested (HttpDigest, Tls, ...)
	 **/
	void authenticationRequested(LinphoneCore lc, LinphoneAuthInfo authInfo, LinphoneCore.AuthMethod method); 

	/**
	 * Call stats notification
	 */
	void callStatsUpdated(LinphoneCore lc, LinphoneCall call, LinphoneCallStats stats);

	/**
	 * Reports that a new subscription request has been received and wait for a decision. 
	 *Status on this subscription request is notified by changing policy for this friend
	 *@param lc LinphoneCore 	
	 *@param lf LinphoneFriend corresponding to the subscriber
	 *@param url of the subscriber
	 * 
	 */
	void newSubscriptionRequest(LinphoneCore lc, LinphoneFriend lf, String url);

	/**
	 * Report status change for a friend previously added to LinphoneCore.
	 * @param lc LinphoneCore
	 * @param lf updated LinphoneFriend
	 */
	void notifyPresenceReceived(LinphoneCore lc, LinphoneFriend lf);

	/**
	 * invoked when a new dtmf is received
	 * @param lc 	LinphoneCore
	 * @param call 	LinphoneCall involved in the dtmf sending
	 * @param dtmf 	value of the dtmf sent
	 */
	void dtmfReceived(LinphoneCore lc, LinphoneCall call, int dtmf);

	/**
	 *  Report Notified message received for this identity.
	 *  @param lc LinphoneCore
	 *  @param call LinphoneCall in case the notify is part of a dialog, may be null
	 *  @param from LinphoneAddress the message comes from
	 *  @param event String the raw body of the notify event.
	 *  
	 */
	void notifyReceived(LinphoneCore lc, LinphoneCall call, LinphoneAddress from, byte[] event);

	/** 
	 * Notifies progress of a call transfer. 
	 * @param lc the LinphoneCore
	 * @param call the call through which the transfer was sent.
	 * @param new_call_state the state of the call resulting of the transfer, at the other party. 
	 **/
	void transferState(LinphoneCore lc, LinphoneCall call, LinphoneCall.State new_call_state);

	/**
	 * Notifies an incoming INFO message.
	 * @param lc the LinphoneCore.
	 * @param info the info message
	 */
	void infoReceived(LinphoneCore lc, LinphoneCall call, LinphoneInfoMessage info);

	/**
	 * Notifies of subscription requests state changes, including new incoming subscriptions.
	 * @param lc the LinphoneCore
	 * @param ev LinphoneEvent object representing the subscription context.
	 * @param state actual state of the subscription.
	 */
	void subscriptionStateChanged(LinphoneCore lc, LinphoneEvent ev, SubscriptionState state);

	/**
	 * Notifies about outgoing generic publish states.
	 * @param lc the LinphoneCore
	 * @param ev a LinphoneEvent representing the publish, typically created by {@link LinphoneCore#publish}
	 * @param state the publish state
	 */
	void publishStateChanged(LinphoneCore lc, LinphoneEvent ev, PublishState state);

	/**
	 * Notifies the application that it should show up
	 * @deprecated
	 */
	@Deprecated
	void show(LinphoneCore lc);

	/**
	 * Callback that notifies various events with human readable text.
	 * @deprecated
	 */
	@Deprecated
	void displayStatus(LinphoneCore lc,String message);

	/**
	 * Callback to display a message to the user 
	 * @deprecated
	 */
	@Deprecated
	void displayMessage(LinphoneCore lc,String message);

	/**
	 * Callback to display a warning to the user 
	 * @deprecated
	 */
	@Deprecated
	void displayWarning(LinphoneCore lc,String message);

	/**
	 * Callback to be notified about the transfer progress.
	 * @param lc the LinphoneCore
	 * @param message the LinphoneChatMessage
	 * @param content the LinphoneContent
	 * @param progress percentage of the transfer done
	 */
	void fileTransferProgressIndication(LinphoneCore lc, LinphoneChatMessage message, LinphoneContent content, int progress);

	/**
	 * Callback to be notified when new data has been received
	 * @param lc the LinphoneCore
	 * @param message the LinphoneChatMessage
	 * @param content the LinphoneContent
	 * @param buffer
	 * @param size
	 */
	void fileTransferRecv(LinphoneCore lc, LinphoneChatMessage message, LinphoneContent content, byte[] buffer, int size);

	/**
	 * Callback to be notified when new data needs to be sent
	 * @param lc the LinphoneCore
	 * @param message the LinphoneChatMessage
	 * @param content the LinphoneContent
	 * @param buffer
	 * @param size
	 * @return the number of bytes written into buffer
	 */
	int fileTransferSend(LinphoneCore lc, LinphoneChatMessage message, LinphoneContent content, ByteBuffer buffer, int size);

	/**
	 * General State notification  
	 * @param state LinphoneCore.State
	 */		
	void globalState(LinphoneCore lc,LinphoneCore.GlobalState state, String message);

	/**
	 * Registration state notification
	 * */
	void registrationState(LinphoneCore lc, LinphoneProxyConfig cfg, LinphoneCore.RegistrationState state, String smessage);

	/**
	 * Notifies the changes about the remote provisioning step
	 * @param lc the LinphoneCore
	 * @param state the RemoteProvisioningState
	 * @param message the error message if state == Failed
	 */
	void configuringStatus(LinphoneCore lc, LinphoneCore.RemoteProvisioningState state, String message);

	/**
	 * invoked when a new linphone chat message is received
	 * @param lc LinphoneCore
	 * @param cr LinphoneChatRoom involved in this conversation. Can be be created by the framework in case the from is not present in any chat room.
	 * @param message incoming linphone chat message message
	 */
	void messageReceived(LinphoneCore lc, LinphoneChatRoom cr, LinphoneChatMessage message);


	/** Call  State notification  
	 * @param state LinphoneCall.State
	 */		
	void callState(LinphoneCore lc, LinphoneCall call, LinphoneCall.State state, String message);

	/**
	 * Callback to display change in encryption state.
	 * @param encrypted true if all streams of the call are encrypted
	 * @param authenticationToken token like ZRTP SAS that may be displayed to user
	 */
	void callEncryptionChanged(LinphoneCore lc, LinphoneCall call, boolean encrypted, String authenticationToken);

	/**
	 * Notifies of an incoming NOTIFY received.
	 * @param lc the linphoneCore
	 * @param ev a LinphoneEvent representing the subscription context for which this notify belongs, or null if it is a NOTIFY out of of any subscription. 
	 * @param eventName the event name
	 * @param content content of the NOTIFY request.
	 */
	void notifyReceived(LinphoneCore lc, LinphoneEvent ev, String eventName, LinphoneContent content);

	/**
	 * invoked when a composing notification is received
	 * @param lc LinphoneCore
	 * @param cr LinphoneChatRoom involved in the conversation.
	 */
	void isComposingReceived(LinphoneCore lc, LinphoneChatRoom cr);

	/**
	 * Invoked when echo cancalation calibration is completed
	 * @param lc LinphoneCore
	 * @param status 
	 * @param delay_ms echo delay
	 * @param data
	 */
	void ecCalibrationStatus(LinphoneCore lc, LinphoneCore.EcCalibratorStatus status, int delay_ms, Object data);

	/**
	 * Callback prototype for reporting log collection upload progress indication.
	 */
	void uploadProgressIndication(LinphoneCore lc, int offset, int total);

	/**
	 * Callback prototype for reporting log collection upload state change.
	 * @param lc LinphoneCore object
	 * @param state The state of the log collection upload
	 * @param info Additional information: error message in case of error state, URL of uploaded file in case of success.
	 */
	void uploadStateChanged(LinphoneCore lc, LinphoneCore.LogCollectionUploadState state, String info);
	
	/**
	 * Callback prototype for reporting LinphoneFriendList creation.
	 * @param lc LinphoneCore object
	 * @param list LinphoneFriendList object
	 */
	void friendListCreated(LinphoneCore lc, LinphoneFriendList list);
	
	/**
	 * Callback prototype for reporting LinphoneFriendList removal.
	 * @param lc LinphoneCore object
	 * @param list LinphoneFriendList object
	 */
	void friendListRemoved(LinphoneCore lc, LinphoneFriendList list);
}

