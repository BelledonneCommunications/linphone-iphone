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


/**
 * Object representing a Call. calls are created using {@link LinphoneCore#invite(LinphoneAddress)} or passed to the application by listener {@link LinphoneCoreListener#callState(LinphoneCore, LinphoneCall, State, String)}
 * 
 */

public interface LinphoneCall {
	/**
	 * Linphone call states
	 *
	 */
	enum State {
		/**
		 * Idle
		 */
		Idle(0, "Idle"),
		/**
		 * Incoming call received.
		 */
		IncomingReceived(1, "IncomingReceived"),
		/**
		 * Outgoing call initialiazed.
		 */
		OutgoingInit(2, "OutgoingInit"),
		/**
		 * Outgoing call in progress. 
		 */
		OutgoingProgress(3, "OutgoingProgress"),
		/**
		 * Outgoing call ringing.
		 */
		OutgoingRinging(4, "OutgoingRinging"),
		/**
		 * Outgoing call early media
		 */
		OutgoingEarlyMedia(5, "OutgoingEarlyMedia"),
		/**
		 * Connected
		 */
		Connected(6, "Connected"),
		/**
		 * Streams running
		 */
		StreamsRunning(7, "StreamsRunning"),
		/**
		 * Pausing
		 */
		Pausing(8, "Pausing"),
		/**
		 * Paused
		 */
		Paused(9, "Paused"),
		/**
		 * Resuming
		 */
		Resuming(10, "Resuming"),
		/**
		 * Refered
		 */
		Refered(11,"Refered"),
		/**
		 * Error
		 */
		Error(12,"Error"),
		/**
		 * Call end
		 */
		CallEnd(13,"CallEnd"),
		/**
		 * Paused by remote
		 */
		PausedByRemote(14,"PausedByRemote"),
		/**
		 * The call's parameters are updated, used for example when video is asked by remote
		 */
		CallUpdatedByRemote(15, "CallUpdatedByRemote"),
		/**
		 * We are proposing early media to an incoming call
		 */
		CallIncomingEarlyMedia(16,"CallIncomingEarlyMedia"),
		/**
		 * The remote accepted the call update initiated by us
		 */
		CallUpdated(17, "CallUpdated"),
		/**
		 * The call object is now released.
		 */
		CallReleased(18,"CallReleased");

		
		private final int mValue;
		private final String mStringValue;
		private State(int v, String desc) {
			this.mValue = v;
			this.mStringValue = desc;
		}
		
		public static State fromInt(int value) {
			State[] allStates = State.values();
			for (int i=0; i<allStates.length;i++) {
				if (allStates[i].mValue == value)
					return allStates[i];
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

	LinphoneCallParams getCurrentParamsCopy();
	
	void enableCamera(boolean enabled);
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
	
	
	String getAuthenticationToken();
	boolean isAuthenticationTokenVerified();
	boolean areStreamsEncrypted();
}
