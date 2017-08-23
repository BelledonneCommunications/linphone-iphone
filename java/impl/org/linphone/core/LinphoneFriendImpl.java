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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
package org.linphone.core;

import java.io.Serializable;

public class LinphoneFriendImpl implements LinphoneFriend, Serializable {
	protected final long nativePtr;
	private native void finalize(long nativePtr);
	private native long newLinphoneFriend(String friendUri);
	private native void setAddress(long nativePtr,long friend);
	private native long getAddress(long nativePtr);
	private native void setIncSubscribePolicy(long nativePtr,int enumValue);
	private native int  getIncSubscribePolicy(long nativePtr);
	private native void enableSubscribes(long nativePtr,boolean value);
	private native boolean isSubscribesEnabled(long nativePtr);
	private native boolean isPresenceReceived(long nativePtr);
	private native int getStatus(long nativePtr);
	private native Object getPresenceModel(long nativePtr);
	private native Object getPresenceModelForUri(long nativePtr, String uri);
	private native void setPresenceModel(long nativePtr, long presencePtr);
	private native void edit(long nativePtr);
	private native void done(long nativePtr);
	private native Object getCore(long ptr);
	private native void setRefKey(long nativePtr, String key);
	private native String getRefKey(long nativePtr);
	private native String getVcardToString(long nativePtr);
	
	private Object userdData;
	public Object getUserData() {
		return userdData;
	}
	public void setUserData(Object ud) {
		userdData = ud;
	}

	protected LinphoneFriendImpl()  {
		nativePtr = newLinphoneFriend(null);
	}
	protected LinphoneFriendImpl(String friendUri)  {
		nativePtr = newLinphoneFriend(friendUri);
	}

	/*reserved for JNI */
	protected LinphoneFriendImpl(long aNativePtr)  {
		nativePtr = aNativePtr;
	}
	protected void finalize() throws Throwable {
		if (nativePtr != 0) {
			finalize(nativePtr);
		}
		super.finalize();
	}
	public void setAddress(LinphoneAddress anAddress) {
		this.setAddress(nativePtr, ((LinphoneAddressImpl)anAddress).nativePtr);
	}
	public LinphoneAddress getAddress() {
		long ptr = getAddress(nativePtr);
		if (ptr != 0) {
			return new LinphoneAddressImpl(ptr, LinphoneAddressImpl.WrapMode.FromConst);
		}
		return null;
	}
	public void setIncSubscribePolicy(SubscribePolicy policy) {
		synchronized(getSyncObject()){
			setIncSubscribePolicy(nativePtr,policy.mValue);
		}
	}
	public SubscribePolicy getIncSubscribePolicy() {
		return SubscribePolicy.fromInt(getIncSubscribePolicy(nativePtr)) ;
	}
	public void enableSubscribes(boolean enable) {
		synchronized(getSyncObject()){
			enableSubscribes(nativePtr, enable);
		}
	}
	public boolean isSubscribesEnabled() {
		return isSubscribesEnabled(nativePtr);
	}
	public boolean isPresenceReceived() {
		return isPresenceReceived(nativePtr);
	}
	public OnlineStatus getStatus() {
		return OnlineStatus.fromInt(getStatus(nativePtr));
	}
	public PresenceModel getPresenceModel() {
		return (PresenceModel)getPresenceModel(nativePtr);
	}
	public PresenceModel getPresenceModelForUri(String uri) {
		return (PresenceModel)getPresenceModelForUri(nativePtr, uri);
	}
	public void setPresenceModel(PresenceModel presenceModel) {
		setPresenceModel(nativePtr, ((PresenceModelImpl)presenceModel).getNativePtr());
	}
	public void edit() {
		synchronized(getSyncObject()){
			edit(nativePtr);
		}
	}
	public void done() {
		synchronized(getSyncObject()){
			done(nativePtr);
		}
	}
	public long getNativePtr() {
		return nativePtr;
	}
	
	/*
	 * Returns a java object to synchronize this friend with.
	 * Indeed some operation must be synchronized with the LinphoneCore object.
	 * If the friend is not associated with a LinphoneCore object, it returns itself in order to avoid writing code for case where no synchronization is necessary.
	 */
	private Object getSyncObject(){
		Object core=getCore(nativePtr);
		if (core!=null) return core;
		else return this;
	}

	public void setRefKey(String key){
		synchronized(getSyncObject()){
			setRefKey(nativePtr,key);
		}
	}

	public String getRefKey(){
		return getRefKey(nativePtr);
	}


	public synchronized String getVcardToString() {
		return getVcardToString(nativePtr);
	}

	
	private native void setName(long nativePtr, String name);
	@Override
	public void setName(String name) {
		setName(nativePtr, name);
	}
	
	private native String getName(long nativePtr);
	@Override
	public String getName() {
		return getName(nativePtr);
	}
	
	private native void setFamilyName(long nativePtr, String name);
	@Override
	public void setFamilyName(String name) {
		setFamilyName(nativePtr, name);
	}

	private native String getFamilyName(long nativePtr);
	public String getFamilyName() {
		return getFamilyName(nativePtr);
	}

	private native void setGivenName(long nativePtr, String name);
	@Override
	public void setGivenName(String name) {
		setGivenName(nativePtr, name);
	}

	private native String getGivenName(long nativePtr);
	public String getGivenName() {
		return getGivenName(nativePtr);
	}
	
	private native void setOrganization(long nativePtr, String organization);
	@Override
	public void setOrganization(String organization) {
		setOrganization(nativePtr, organization);
	}
	
	private native String getOrganization(long nativePtr);
	@Override
	public String getOrganization() {
		return getOrganization(nativePtr);
	}
	
	private native long[] getAddresses(long nativePtr);
	@Override
	public LinphoneAddress[] getAddresses() {
		long[] ptrs = getAddresses(nativePtr);
		if (ptrs == null) return null;

		LinphoneAddress[] addresses = new LinphoneAddress[ptrs.length];
		for (int i = 0; i < addresses.length; i++) {
			addresses[i] = new LinphoneAddressImpl(ptrs[i], LinphoneAddressImpl.WrapMode.FromConst);
		}
		return addresses;
	}
	
	private native void addAddress(long nativePtr, long addr);
	@Override
	public void addAddress(LinphoneAddress addr) {
		addAddress(nativePtr, ((LinphoneAddressImpl)addr).nativePtr);
	}
	
	private native void removeAddress(long nativePtr, long addr);
	@Override
	public void removeAddress(LinphoneAddress addr) {
		removeAddress(nativePtr, ((LinphoneAddressImpl)addr).nativePtr);
	}
	
	private native Object[] getPhoneNumbers(long nativePtr);
	@Override
	public String[] getPhoneNumbers() {
		Object[] phones = getPhoneNumbers(nativePtr);
		if (phones == null) return null;
		
		String[] phoneNumbers = new String[phones.length];
		for (int i = 0; i < phones.length; i++) {
			phoneNumbers[i] = phones[i].toString();
		}
		return phoneNumbers;
	}
	
	private native void addPhoneNumber(long nativePtr, String phone);
	@Override
	public void addPhoneNumber(String phone) {
		addPhoneNumber(nativePtr, phone);
	}
	
	private native void removePhoneNumber(long nativePtr, String phone);
	@Override
	public void removePhoneNumber(String phone) {
		removePhoneNumber(nativePtr, phone);
	}
	
	private native boolean isAlreadyPresentInFriendList(long nativePtr);
	@Override
	public boolean isAlreadyPresentInFriendList() {
		return isAlreadyPresentInFriendList(nativePtr);
	}
}
