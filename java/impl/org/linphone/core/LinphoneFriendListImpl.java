/*
LinphoneFriendImpl.java
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

import java.io.Serializable;

class LinphoneFriendListImpl implements LinphoneFriendList, Serializable {
	
	protected final long nativePtr;
	private native void finalize(long nativePtr);
	private native long newLinphoneFriendList(long corePtr);
	private native void setRLSUri(long nativePtr, String uri);
	private native void addFriend(long nativePtr, long friendPtr);
	private native void addLocalFriend(long nativePtr, long friendPtr);
	private native LinphoneFriend[] getFriendList(long nativePtr);
	private native void updateSubscriptions(long nativePtr, long proxyConfigPtr, boolean onlyWhenRegistered);
	private native Object getCore(long ptr);
	private native LinphoneFriend findFriendByUri(long nativePtr, String uri);
	private native void setListener(long ptr, LinphoneFriendListListener listener);

	protected LinphoneFriendListImpl(LinphoneCoreImpl core)  {
		nativePtr = newLinphoneFriendList(core.nativePtr);
	}

	@Override
	public void  setRLSUri(String uri) {
		synchronized(getSyncObject()){
			setRLSUri(nativePtr, uri);
		}
	}
	
	@Override
	public void addFriend(LinphoneFriend friend) {	
		synchronized(getSyncObject()){
			addFriend(nativePtr, friend.getNativePtr());
		}
	}
	
	@Override
	public void addLocalFriend(LinphoneFriend friend) {	
		synchronized(getSyncObject()){
			addLocalFriend(nativePtr, friend.getNativePtr());
		}
	}
	
	@Override
	public LinphoneFriend[] getFriendList() {
		synchronized(getSyncObject()){
			return getFriendList(nativePtr);
		}
	}
	
	@Override
	public void updateSubscriptions(LinphoneProxyConfig proxyConfig,boolean onlyWhenRegistered) {
		synchronized(getSyncObject()){
			updateSubscriptions(nativePtr,  ((LinphoneProxyConfigImpl)proxyConfig).nativePtr,onlyWhenRegistered);
		}
	}
	
	@Override
	public LinphoneFriend findFriendByUri(String uri) {
		synchronized(getSyncObject()){
			return findFriendByUri(nativePtr,uri);
		}
	}
	
	private native void setUri(long nativePtr, String uri);
	@Override
	public void setUri(String uri) {
		synchronized(getSyncObject()) {
			setUri(nativePtr, uri);
		}
	}
	
	private native void synchronizeFriendsFromServer(long nativePtr);
	@Override
	public void synchronizeFriendsFromServer() {
		synchronized(getSyncObject()) {
			synchronizeFriendsFromServer(nativePtr);
		}
	}
	
	private native int importFriendsFromVCardFile(long nativePtr, String file);
	@Override
	public int importFriendsFromVCardFile(String file) {
		return importFriendsFromVCardFile(nativePtr, file);
	}
	
	private native int importFriendsFromVCardBuffer(long nativePtr, String buffer);
	@Override
	public int importFriendsFromVCardBuffer(String buffer) {
		return importFriendsFromVCardBuffer(nativePtr, buffer);
	}
	
	private native void exportFriendsToVCardFile(long nativePtr, String file);
	@Override
	public void exportFriendsToVCardFile(String file) {
		exportFriendsToVCardFile(nativePtr, file);
	}
	
	@Override
	public void setListener(LinphoneFriendListListener listener) {
		setListener(nativePtr, listener);
	}
	

	/*reserved for JNI */
	protected LinphoneFriendListImpl(long aNativePtr)  {
		nativePtr = aNativePtr;
	}
	@Override
	protected void finalize() throws Throwable {
		if (nativePtr != 0) {
			finalize(nativePtr);
		}
		super.finalize();
	}

	@Override
	public long getNativePtr() {
		return nativePtr;
	}
	private Object getSyncObject(){
		 return this;
	}	
}

