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

import org.linphone.core.LinphoneChatMessage.State;

public interface LinphoneFriendList {
	public void setRLSUri(String uri);
	public void addFriend(LinphoneFriend friend);
	public void addLocalFriend(LinphoneFriend friend);
	public LinphoneFriend[] getFriendList();
	public void updateSubscriptions(LinphoneProxyConfig proxyConfig,boolean onlyWhenRegistered);
	public LinphoneFriend findFriendByUri(String uri);
	public void setUri(String uri);
	public void synchronizeFriendsFromServer();

	/**
	 * Imports LinphoneFriends from a vCard 4 file
	 * @return the number of friend imported
	**/
	public int importFriendsFromVCardFile(String file);
	
	/**
	 * Imports LinphoneFriends from a vCard 4 buffer
	 * @return the number of friend imported
	**/
	public int importFriendsFromVCardBuffer(String buffer);
	
	/**
	 * Exports LinphoneFriends to a vCard 4 file
	**/
	public void exportFriendsToVCardFile(String file);
	
	long getNativePtr();
	
	/**
	 * Set the callbacks associated with the LinphoneFriendList.
	 */
	void setListener(LinphoneFriendList.LinphoneFriendListListener listener);
	
	interface LinphoneFriendListListener {
		void onLinphoneFriendCreated(LinphoneFriendList list, LinphoneFriend lf);
		
		void onLinphoneFriendUpdated(LinphoneFriendList list, LinphoneFriend newFriend, LinphoneFriend oldFriend);
		
		void onLinphoneFriendDeleted(LinphoneFriendList list, LinphoneFriend lf);
		
		void onLinphoneFriendSyncStatusChanged(LinphoneFriendList list, LinphoneFriendList.State status, String message);
	}
	public static class State {
		static private Vector<State> values = new Vector<State>();
		public final int value() { return mValue; }
		
		private final int mValue;
		private final String mStringValue;
		
		public final static State SyncStarted = new State(0, "SyncStarted");
		public final static State SyncSuccessful = new State(1, "SyncSuccessful");
		public final static State SyncFailure = new State(2, "SyncFailure");
		
		private State(int value,String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}
		
		public static State fromInt(int value) {

			for (int i = 0; i < values.size(); i++) {
				State state = (State) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("state not found [" + value + "]");
		}
		
		public String toString() {
			return mStringValue;
		}
		
		public int toInt() {
			return mValue;
		}
	}
}
