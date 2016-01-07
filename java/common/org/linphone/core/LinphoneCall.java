/*
LinphoneCall.java
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
package org.linphone.core;

import java.util.Vector;

/**
 * Object representing a call. Calls are created using {@link LinphoneCore#invite(LinphoneAddress)} or passed to the application by listener {@link LinphoneCoreListener#callState}
 * 
 */

public interface LinphoneCall {
	/**
	 * LinphoneCall listener
	 */
	interface LinphoneCallListener {
		void onNextVideoFrameDecoded(LinphoneCall call);
	}

	/**
	 * Linphone call states
	 *
	 */
	static class State {
		
		static private Vector<State> values = new Vector<State>();
		private final int mValue;
		public final int value() {return mValue;}
		
		private final String mStringValue;
		/**
		 * Idle
		 */
		public final static State Idle = new State(0,"Idle");
		/**
		 * Incoming call received.
		 */
		public final static State IncomingReceived = new State(1,"IncomingReceived");
		/**
		 * Outgoing call initialiazed.
		 */
		public final static State OutgoingInit = new State(2,"OutgoingInit");
		/**
		 * Outgoing call in progress. 
		 */
		public final static State OutgoingProgress = new State(3,"OutgoingProgress");
		/**
		 * Outgoing call ringing.
		 */
		public final static State OutgoingRinging = new State(4,"OutgoingRinging");
		/**
		 * Outgoing call early media
		 */
		public final static State OutgoingEarlyMedia = new State(5,"OutgoingEarlyMedia");
		/**
		 * Connected
		 */
		public final static State Connected = new State(6,"Connected");
		/**
		 * Streams running
		 */
		public final static State StreamsRunning = new State(7,"StreamsRunning");
		/**
		 * Pausing
		 */
		public final static State Pausing = new State(8,"Pausing");
		/**
		 * Paused
		 */
		public final static State Paused = new State(9,"Paused");
		/**
		 * Resuming
		 */
		public final static State Resuming = new State(10,"Resuming");
		/**
		 * Refered
		 */
		public final static State Refered = new State(11,"Refered");
		/**
		 * Error
		 */
		public final static State Error = new State(12,"Error");
		/**
		 * Call end
		 */
		public final static State CallEnd = new State(13,"CallEnd");
		
		/**
		 * Paused by remote
		 */
		public final static State PausedByRemote = new State(14,"PausedByRemote");
		
		/**
		 * The call's parameters are updated, used for example when video is asked by remote
		 */
		public static final State CallUpdatedByRemote = new State(15, "UpdatedByRemote");

		/**
		 * We are proposing early media to an incoming call
		 */
		public static final State CallIncomingEarlyMedia = new State(16,"IncomingEarlyMedia");

		/**
		 * We have initiated a call update. When the remote accepts the call update, state will move to StreamsRunning.
		 */
		public static final State CallUpdating = new State(17, "Updating");
		
		/**
		 * The call object is now released.
		 */
		public static final State CallReleased = new State(18,"Released");

		/**
		 * The call is updated by remote while not yet answered (SIP UPDATE in early dialog received)
		 */
		public static final State CallEarlyUpdatedByRemote = new State(19,"EarlyUpdatedByRemote");

		/**
		 * We are updating the call while not yet answered (SIP UPDATE in early dialog sent)
		**/
		public static final State CallEarlyUpdating = new State(20,"EarlyUpdating");
		
		private State(int value,String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue=stringValue;
		}
		
		public static State fromInt(int value) {

			for (int i=0; i<values.size();i++) {
				State state = (State) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("state not found ["+value+"]");
		}
		public String toString() {
			return mStringValue;
		}
	}
	
	/**
	 * Retrieves the call's current state.
	**/
	State getState();
	
	/**
	 * Returns the remote address associated to this call
	 *
	**/
	LinphoneAddress  getRemoteAddress();
	/**
	 * get direction of the call (incoming or outgoing).
	 * @return CallDirection 
	 */
	CallDirection getDirection();
	/**
	 * get the call log associated to this call.
	 * @Return LinphoneCallLog
	**/
	LinphoneCallLog getCallLog();


	/**
	 * Get the audio statistics associated with this call.
	 * @return LinphoneCallStats
	 */
	LinphoneCallStats getAudioStats();

	/**
	 * Get the video statistics associated with this call.
	 * @return LinphoneCallStats
	 */
	LinphoneCallStats getVideoStats();
	
	/**
	 * Get call's remote parameters, as proposed by far end.
	 * This is useful for example to know if far end supports video or encryption.
	**/
	LinphoneCallParams getRemoteParams();

	LinphoneCallParams getCurrentParamsCopy();
	
	void enableCamera(boolean enabled);
	boolean cameraEnabled();
	
	/**
	 * Enables or disable echo cancellation.
	 * @param enable
	 */
	void enableEchoCancellation(boolean enable);
	/**
	 * get EC status 
	 * @return true if echo cancellation is enabled.
	 */
	boolean isEchoCancellationEnabled();
	/**
	 * Enables or disable echo limiter cancellation.
	 * @param enable
	 */
	void enableEchoLimiter(boolean enable);
	/**
	 * get EL status 
	 * @return true if echo limiter is enabled.
	 */
	boolean isEchoLimiterEnabled();
	/**
	 * Returns the object associated to a call this one is replacing.
	 * Call replacement can occur during transfer scenarios.
	 */
	LinphoneCall getReplacedCall();

	/**
	 * @return call duration computed from media start
	 */
	int getDuration();
	/**
	 * Obtain real-time quality rating of the call
	 *
	 * Based on local RTP statistics and RTCP feedback, a quality rating is computed and updated
	 * during all the duration of the call. This function returns its value at the time of the function call.
	 * It is expected that the rating is updated at least every 5 seconds or so.
	 * The rating is a floating point number comprised between 0 and 5.
	 *
	 * 4-5 = good quality <br>
	 * 3-4 = average quality <br>
	 * 2-3 = poor quality <br>
	 * 1-2 = very poor quality <br>
	 * 0-1 = can't be worse, mostly unusable <br>
	 *
	 * @returns The function returns -1 if no quality measurement is available, for example if no 
	 * active audio stream exist. Otherwise it returns the quality rating.
	 */
	float getCurrentQuality();
	/**
	 * Returns call quality averaged over all the duration of the call.
	 *
	 * See getCurrentQuality() for more details about quality measurement.
	 */
	float getAverageQuality();
	
	/**
	 * Used by ZRTP encryption mechanism.
	 * @return SAS associated to the main stream [voice]
	 */
	String getAuthenticationToken();

	/**
	 * Used by ZRTP encryption mechanism.
	 * SAS can be verified manually by the user or automatically using a previously shared secret.
	 * @return true if the main stream [voice ]SAS was verified.
	 */
	boolean isAuthenticationTokenVerified();

	/**
	 * Used by ZRTP encryption mechanism.
	 * @param verified true when displayed SAS is correct
	 */
	void setAuthenticationTokenVerified(boolean verified);
	
	/**
	 * Checks wether the call is part of a conferece.
	 * @return A boolean
	 */
	boolean isInConference();
	/**
	 * Get the conference instance which the call is part of.
	 * @return An instance of #LinphoneConference if the call is part
	 * of a conference. null instead.
	 */
	LinphoneConference getConference();

	/**
	 * Indicates whether an operation is in progress at the media side.
	 * It can a bad idea to initiate signaling operations (adding video, pausing the call, removing video, changing video parameters) while
	 * the media is busy in establishing the connection (typically ICE connectivity checks). It can result in failures generating loss of time
	 * in future operations in the call.
	 * Applications are invited to check this function after each call state change to decide whether certain operations are permitted or not.
	 * @return TRUE if media is busy in establishing the connection, FALSE otherwise.
	 **/
	boolean mediaInProgress();
	
	float getPlayVolume();

	/**
	 * Obtain the remote user agent string. 
	 */
	String getRemoteUserAgent();

	/**
	 * Obtain the remote sip contact string.
	**/
	String getRemoteContact();
	
	/**
	 * Take a photo of currently received video and write it into a jpeg file.
	 */
	void takeSnapshot(String path);
	
	/**
	 * Scale the video by factor, and center it using cx,cy point
	 */
	void zoomVideo(float factor, float cx, float cy);
	
	/**
	 * Start call recording.
	 * A file path must be provided with LinphoneCallParams.setRecordFile() at call establishement for this method to work.
	 */
	void startRecording();
	
	/**
	 * Stop call recording.
	 */
	void stopRecording();
	
	/**
	 * If a call transfer has been initiated for this call, returns the call state of the new call performed at the remote end as a result of the transfer request.
	 * @return the call state of the new call performed by the referee to the refer target.
	 */
	State getTransferState();
	
	/**
	 * Send an info message to remote peer.
	 */
	void sendInfoMessage(LinphoneInfoMessage msg);
	
	/**
	 * Returns the transferer if this call was started automatically as a result of an incoming transfer request.
	 * The call in which the transfer request was received is returned in this case.
	 **/
	LinphoneCall getTransfererCall();
	
	/**
	 * When this call has received a transfer request, returns the new call that was automatically created as a result of the transfer.
	**/
	LinphoneCall getTransferTargetCall();
	
	Reason getReason();
	
	/**
	 * Returns last error reported for the call.
	 * @return an ErrorInfo.
	 */
	ErrorInfo getErrorInfo();
	
	/**
	 *  attached a user data to a call
	 **/
	void setUserData(Object obj);
	
	/**
	 * Returns user data from a call. return null if any
	 * @return an Object.
	 */
	Object getUserData();
	
	/**
	 * Get a call player
	 * Call player enable to stream a media file through a call
	 * @return A player
	 */
	public LinphonePlayer getPlayer();
	
	/**
	 * Create a new chat room for messaging from a call if not already existing, else return existing one
	 * @return LinphoneChatRoom where messaging can take place.
	 */
	public LinphoneChatRoom getChatRoom() ;

	/**
	 * Set the callbacks associated with the LinphoneCall.
	 */
	void setListener(LinphoneCall.LinphoneCallListener listener);
}
