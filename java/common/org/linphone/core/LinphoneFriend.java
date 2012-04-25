/*
LinphoneFriend.java
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
 * Represents a buddy, all presence actions like subscription and status change notification are performed on this object
 * 
 *
 */

public interface LinphoneFriend {
	/**
	 * Enum controlling behavior for incoming subscription request. 
	 *	Use by {@link LinphoneFriend#setIncSubscribePolicy()}
	 *
	 */
	static class SubscribePolicy {

		
		static private Vector values = new Vector();
		protected final int mValue;
		private final String mStringValue;
		/**
		 * Does not automatically accept an incoming subscription request. 
		 * This policy implies that a decision has to be taken for each incoming subscription request notified by {@link LinphoneCoreListener#newSubscriptionRequest(LinphoneCore, LinphoneFriend, String)}
		 */
		public final static SubscribePolicy SPWait = new SubscribePolicy(0,"SPWait");
		/**
		 * Rejects incoming subscription request.
		 */
		public final static SubscribePolicy SPDeny = new SubscribePolicy(1,"SPDeny");
		/**
		 * Automatically accepts a subscription request.
		 */
		public final static SubscribePolicy SPAccept = new SubscribePolicy(2,"SPAccept");
		
		
		private SubscribePolicy(int value,String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue=stringValue;
		}
		public static SubscribePolicy fromInt(int value) {

			for (int i=0; i<values.size();i++) {
				SubscribePolicy policy = (SubscribePolicy) values.elementAt(i);
				if (policy.mValue == value) return policy;
			}
			throw new RuntimeException("Policy not found ["+value+"]");
		}
		public String toString() {
			return mStringValue;
		}
	}
	/**
	 * Set a {@link LinphoneAddress } for this friend
	 * @param anAddress
	 */
	void setAddress(LinphoneAddress anAddress);
	/**
	 * get address of this friend
	 * @return
	 */
	LinphoneAddress getAddress();
	/**
	 * Configure incoming subscription policy for this friend.
	 * @param policy to apply
	 */
	void setIncSubscribePolicy(SubscribePolicy policy);
	/**
	 * get current subscription policy for this LinphoneFriend
	 * @return
	 */
	SubscribePolicy getIncSubscribePolicy();
	/**
	 * Configure LinphoneFriend to subscribe to presence information
	 * @param enable if true this friend will receive subscription message
	 */
	void enableSubscribes(boolean enable);
	/**
	 * get subscription flag value
	 * @return true is subscription is activated for this friend.
	 */
	boolean isSubscribesEnabled();
	/**
	 * get friend status
	 * @return
	 */
	OnlineStatus getStatus();
	/**
	 * Starts editing a friend configuration.
	 *<br> Because friend configuration must be consistent, applications MUST call {@link #edit()} before doing any attempts to modify friend configuration (such as address or subscription policy and so on). 
	 *Once the modifications are done, then the application must call {@link #done()} to commit the changes.
	 */
	void edit();
	/**
	 * Commits modification made to the friend configuration.
	 */
	void done();
	/**
	 * Human readable representation of this friend
	 * @return
	 */
	String toString();
	

}
