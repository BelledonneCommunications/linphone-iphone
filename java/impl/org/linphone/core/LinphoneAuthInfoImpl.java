/*
LinphoneAuthInfoImpl.java
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

class LinphoneAuthInfoImpl implements LinphoneAuthInfo {
	protected final long nativePtr;
	private native long newLinphoneAuthInfo();
	private native void  delete(long ptr);
	private native String getPassword(long ptr);
	private native String getRealm(long ptr);
	private native String getUsername(long ptr);
	private native void setPassword(long ptr, String password);
	private native void setRealm(long ptr, String realm);
	private native void setUsername(long ptr, String username);
	private native void setUserId(long ptr, String username);
	private native void setHa1(long ptr, String ha1);
	private native String getUserId(long ptr);
	private native String getHa1(long ptr);
	private native String getDomain(long ptr);
	private native void setDomain(long ptr, String domain);
	
	boolean ownPtr = false;
	protected LinphoneAuthInfoImpl(String username,String password, String realm, String domain)  {
		this(username, null, password, null, realm, domain);
	}
	protected LinphoneAuthInfoImpl(String username, String userid, String passwd, String ha1, String realm, String domain)  {
		nativePtr = newLinphoneAuthInfo();
		this.setUsername(username);
		this.setUserId(userid);
		this.setPassword(passwd);
		this.setHa1(ha1);
		this.setDomain(domain);
		this.setRealm(realm);
		ownPtr = true;
	}
	protected LinphoneAuthInfoImpl(long aNativePtr)  {
		nativePtr = aNativePtr;
		ownPtr = false;
	}
	protected void finalize() throws Throwable {
		if (ownPtr) delete(nativePtr);
	}
	public String getPassword() {
		return getPassword (nativePtr);
	}
	public String getRealm() {
		return getRealm (nativePtr);
	}
	public String getUsername() {
		return getUsername (nativePtr);
	}
	public void setPassword(String password) {
		setPassword(nativePtr,password);
	}
	public void setRealm(String realm) {
		setRealm(nativePtr,realm);
	}
	public void setUsername(String username) {
		setUsername(nativePtr,username);
	}
	@Override
	public String getUserId() {
		return getUserId(nativePtr);
	}
	@Override
	public void setUserId(String userid) {
		setUserId(nativePtr,userid);
		
	}
	@Override
	public String getHa1() {
		return getHa1(nativePtr);
	}
	@Override
	public void setHa1(String ha1) {
		setHa1(nativePtr,ha1);
		
	}
	@Override
	public void setDomain(String domain) {
		setDomain(nativePtr, domain);
	}
	@Override
	public String getDomain() {
		return getDomain(nativePtr);
	}
	
	public LinphoneAuthInfo clone() {
		LinphoneAuthInfo clone = LinphoneCoreFactory.instance().createAuthInfo(
				getUsername(), 
				getUserId(), 
				getPassword(), 
				getHa1(), 
				getRealm(), 
				getDomain());
		return clone;
	}
}
