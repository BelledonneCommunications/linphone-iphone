/*
LinPhoneCallLog.java
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
 * Call data records object 
 *
 */
import java.util.Vector;



public interface LinphoneCallLog {
	/**
	 * Represents call status
	 *
	 */
	static class CallStatus {
		
		static private Vector values = new Vector();
		private final int mValue;
		private final String mStringValue;
		/**
		 * Call success.
		 */
		public final static CallStatus Sucess = new CallStatus(0,"Sucess");
		/**
		 * Call aborted.
		 */
		public final static CallStatus Aborted = new CallStatus(1,"Aborted");
		/**
		 * missed incoming call.
		 */
		public final static CallStatus Missed = new CallStatus(2,"Missed");
		/**
		 * remote call declined.
		 */
		public final static CallStatus Declined = new CallStatus(3,"Declined");
		
		
		private CallStatus(int value,String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue=stringValue;
		}
		public static CallStatus fromInt(int value) {

			for (int i=0; i<values.size();i++) {
				CallStatus state = (CallStatus) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("CallStatus not found ["+value+"]");
		}
		public String toString() {
			return mStringValue;
		}
		public int toInt() {
			return mValue;
		}
	}
	
	/**
	 * Originator of the call as a LinphoneAddress object.
	 * @return LinphoneAddress
	 */
	public LinphoneAddress getFrom();
	/**
	 * Destination of the call as a LinphoneAddress object.
	 * @return
	 */
	public LinphoneAddress getTo ();
	/**
	 * The direction of the call
	 * @return CallDirection
	 */
	public CallDirection getDirection();
	/**
	 * get status of this call
	 * @return
	 */
	public CallStatus getStatus();
	
	/**
	 * @return a human readble String with the start date/time of the call
	 */
	public String getStartDate();
	
	/**
	 * @return the call duration, in seconds
	 */
	public int getCallDuration();
}
