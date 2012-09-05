package org.linphone.core;

import java.util.Vector;


public interface LinphoneChatMessage {
	interface StateListener{
		void onLinphoneChatMessageStateChanged(LinphoneChatMessage msg, State state);
	}
	static class State {
		static private Vector values = new Vector();
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
		public final static State InProgress = new State(1,"InProgress");
		/**
		 * Outgoing call initialiazed.
		 */
		public final static State Delivered = new State(2,"Delivered");
		/**
		 * Outgoing call in progress. 
		 */
		public final static State NotDelivered = new State(3,"NotDelivered");
		
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
	Object getUserData();
	void setUserData();
}
