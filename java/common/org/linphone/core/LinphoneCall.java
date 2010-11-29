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
 * Object representing a Call. calls are created using {@link LinphoneCore#invite(LinphoneAddress)} or paased to the application by listener {@link LinphoneCoreListener#callState(LinphoneCore, LinphoneCall, State, String)}
 * 
 */
public interface LinphoneCall {
	/**
	 * Linphone call states
	 *
	 */
	static class State {
		static private Vector values = new Vector();
		private final int mValue;
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
		 * Paussing
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
		public static final State CallUpdatedByRemote = new State(15, "CallUpdatedByRemote");

		/**
		 * We are proposing early media to an incoming call
		 */
		public static final State CallIncomingEarlyMedia = new State(16,"CallIncomingEarlyMedia");

		/**
		 * The remote accepted the call update initiated by us
		 */
		public static final State CallUpdated = new State(17, "CallUpdated");


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
	public State getState();
	
	/**
	 * Returns the remote address associated to this call
	 *
	**/
	public LinphoneAddress  getRemoteAddress();
	/**
	 * get direction of the call (incoming or outgoing).
	 * @return CallDirection 
	 */
	public CallDirection getDirection();
	/**
	 * get the call log associated to this call.
	 * @Return LinphoneCallLog
	**/
	public LinphoneCallLog getCallLog();

	/**
	 * @return parameters for this call; read only, call copy() to get a read/write version.
	 */
	public LinphoneCallParams getCurrentParamsReadOnly();
	
}
