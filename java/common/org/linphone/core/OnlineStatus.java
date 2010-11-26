/*
OnlineStatus.java
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
 * Enum describing remote friend status
 *
 */
public class OnlineStatus {

		static private Vector values = new Vector();
		/**
		 * Offline
		 */
		static public OnlineStatus Offline = new OnlineStatus(0,"Offline");
		/**
		 * Online 
		 */
		static public OnlineStatus Online = new OnlineStatus(1,"Online");
		/**
		 * Busy
		 */
		static public OnlineStatus Busy = new OnlineStatus(2,"Busy");
		/**
		 * Be Right Back
		 */
		static public OnlineStatus BeRightBack = new OnlineStatus(3,"BeRightBack");
		/**
		 * Away
		 */
		static public OnlineStatus Away = new OnlineStatus(4,"Away");
		/**
		 * On The Phone
		 */
		static public OnlineStatus OnThePhone = new OnlineStatus(5,"OnThePhone");
		/**
		 * Out To Lunch 
		 */
		static public OnlineStatus OutToLunch  = new OnlineStatus(6,"OutToLunch ");		
		/**
		 * Do Not Disturb
		 */
		static public OnlineStatus DoNotDisturb = new OnlineStatus(7,"DoNotDisturb");		
		/**
		 * Moved in this sate, call can be redirected if an alternate contact address has been set using function {@link LinphoneCore#setPresenceInfo(int, String, OnlineStatus)}
		 */
		static public OnlineStatus StatusMoved = new OnlineStatus(8,"StatusMoved");		
		/**
		 * Using another messaging service
		 */
		static public OnlineStatus StatusAltService = new OnlineStatus(9,"StatusAltService");		
		/**
		 * Pending
		 */
		static public OnlineStatus Pending = new OnlineStatus(10,"Pending");
		
		protected final int mValue;
		private final String mStringValue;

		private OnlineStatus(int value,String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue=stringValue;
		}
		public static OnlineStatus fromInt(int value) {

			for (int i=0; i<values.size();i++) {
				OnlineStatus state = (OnlineStatus) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("state not found ["+value+"]");
		}
		public String toString() {
			return mStringValue;
		}

}
