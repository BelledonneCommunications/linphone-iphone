package org.linphone.core;

import java.util.Vector;


public interface LinphoneChatMessage {
	interface StateListener{
		void onLinphoneChatMessageStateChanged(LinphoneChatMessage msg, State state);
	}
	public static class State {
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
		public int toInt() {
			return mValue;
		}
	}
	
	long getNativePtr();
	
	Object getUserData();
	
	void setUserData();
	
	/**
	 * get text associated to this LinphoneChatMessage
	 * 
	 * @return text sent along with the message
	 */
	String getText();
	
	/**
	 * get peer address associated to this LinphoneChatMessage
	 *
	 * @return LinphoneAddress peer address
	 */
	LinphoneAddress getPeerAddress();
	
	/**
	 * get from address associated to this LinphoneChatMessage
	 *
	 * @return LinphoneAddress from address
	 */
	LinphoneAddress getFrom();
	
	/**
	 * Linphone message can carry external body as defined by rfc2017
	 * @param message #LinphoneChatMessage
	 * @return return external body url null if not present.
	 */
	String getExternalBodyUrl();
	
	/**
	 * Linphone message can carry external body as defined by rfc2017
	 * @param  #LinphoneChatMessage  
	 * @param url ex: access-type=URL; URL="http://www.foo.com/file"
	 */
	void setExternalBodyUrl(String url);
	
	/**
	 * Add a custom header into the message.
	 * @param name
	 * @param value
	 */
	void addCustomHeader(String name, String value);
	
	/**
	 * Obtain a header value.
	 * @param name
	 * @return the value of the header, or null if not found.
	 */
	String  getCustomHeader(String name);
	
	/**
	 * Gets the time at which the message was sent
	 * @return the time in milliseconds
	 */
	long getTime();
}
