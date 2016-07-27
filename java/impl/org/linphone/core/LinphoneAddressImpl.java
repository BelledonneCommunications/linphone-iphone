/*
LinphoneAddressImpl.java
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



public class LinphoneAddressImpl implements LinphoneAddress {
	public enum WrapMode{
		FromNew,
		FromConst,
		FromExisting
	};
	protected final long nativePtr;
	private native long newLinphoneAddressImpl(String uri,String displayName);
	private native long ref(long ptr);
	private native void unref(long ptr);
	private native long clone(long ptr);
	private native String getDisplayName(long ptr);
	private native String getUserName(long ptr);
	private native String getDomain(long ptr);
	private native int getTransport(long ptr);
    private native int getPort(long ptr);
	private native String toUri(long ptr);
	private native void setDisplayName(long ptr,String name);
	private native void setDomain(long ptr,String domain);
	private native void setUserName(long ptr,String username);
	private native void setTransport(long ptr, int transport);
    private native void setPort(long ptr, int port);
	private native String toString(long ptr);
	
	protected LinphoneAddressImpl(String identity)  throws LinphoneCoreException{
		nativePtr = newLinphoneAddressImpl(identity, null);
		if(nativePtr==0) {
			throw new LinphoneCoreException("Cannot create LinphoneAdress from ["+identity+"]");
		}
	}
	
	protected LinphoneAddressImpl(String username,String domain,String displayName)  {
		nativePtr = newLinphoneAddressImpl(null, displayName);
		this.setUserName(username);
		this.setDomain(domain);
	}
	//this method is there because JNI is calling it.
	private LinphoneAddressImpl(long aNativeptr){
		this(aNativeptr,WrapMode.FromConst);
	}
	protected LinphoneAddressImpl(long aNativePtr, WrapMode mode)  {
		switch(mode){
		case FromNew:
			nativePtr=aNativePtr;
			break;
		case FromConst:
			nativePtr=clone(aNativePtr);
			break;
		case FromExisting:
			nativePtr=ref(aNativePtr);
			break;
		default:
			nativePtr=0;
		}
	}
	
	protected void finalize() throws Throwable {
		if (nativePtr!=0) unref(nativePtr);
	}
	public String getDisplayName() {
		return getDisplayName(nativePtr);
	}
	public String getDomain() {
		return getDomain(nativePtr);
	}
	public String getUserName() {
		return getUserName(nativePtr);
	}
	public TransportType getTransport() {
		return TransportType.fromInt(getTransport(nativePtr));
	}
    public int getPort() { return getPort(nativePtr); }

    public String toString() {
		return toString(nativePtr);
	}
	public String toUri() {
		return toUri(nativePtr);	
	}

    public String asString() {
        return toString();
    }
    public String asStringUriOnly() {
        return toUri(nativePtr);
    }

    public void clean() {
        throw new RuntimeException("Not implemented");
    }

	public void setDisplayName(String name) {
		setDisplayName(nativePtr,name);
	}
	public void setDomain(String domain) {
		setDomain(nativePtr, domain);
	}
	public void setPort(int port) { setPort(nativePtr, port); }
	public void setUserName(String username) {
		setUserName(nativePtr,username);
	}
	public void setTransport(TransportType transport) {
		setTransport(nativePtr, transport.toInt());
	}
}
