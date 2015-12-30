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
import org.linphone.core.LinphoneProxyConfigImpl;

class LinphoneFriendListImpl implements LinphoneFriendList, Serializable {
	
	protected final long nativePtr;
	private native void finalize(long nativePtr);
	private native long newLinphoneFriendList(long corePtr);
	private native void setRLSUri(long nativePtr,String uri);
	private native void addFriend(long nativePtr,long friendPtr);
	private native void updateSubscriptions(long nativePtr,long proxyConfigPtr,boolean onlyWhenRegistered);
	private native Object getCore(long ptr);
	private native LinphoneFriend findFriendByUri(long nativePtr,String uri);

	

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

