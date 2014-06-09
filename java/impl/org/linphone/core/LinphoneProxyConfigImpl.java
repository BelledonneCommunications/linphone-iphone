/*
LinphoneProxyConfigImpl.java
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

import org.linphone.core.LinphoneCore.RegistrationState;





class LinphoneProxyConfigImpl implements LinphoneProxyConfig {

	protected final long nativePtr;
	protected LinphoneCoreImpl mCore;
	
	private native int getState(long nativePtr);
	private native void setExpires(long nativePtr, int delay);
	private native int getExpires(long nativePtr);

	boolean ownPtr = false;
	protected LinphoneProxyConfigImpl(String identity,String proxy,String route, boolean enableRegister) throws LinphoneCoreException {
		nativePtr = newLinphoneProxyConfig();
		setIdentity(identity);
		setProxy(proxy);
		setRoute(route);
		enableRegister(enableRegister);
		ownPtr=true;
	}
	protected LinphoneProxyConfigImpl(LinphoneCoreImpl core, long aNativePtr)  {
		nativePtr = aNativePtr;
		ownPtr=false;
		mCore=core;
	}
	protected void finalize() throws Throwable {
		//Log.e(LinphoneService.TAG,"fixme, should release underlying proxy config");
		if (ownPtr) delete(nativePtr);
	}
	private native long newLinphoneProxyConfig();
	private native void  delete(long ptr);

	private native void edit(long ptr);
	private native void done(long ptr);
	
	private native void setIdentity(long ptr,String identity);
	private native String getIdentity(long ptr);
	private native int setProxy(long ptr,String proxy);
	private native String getProxy(long ptr);
	

	private native void enableRegister(long ptr,boolean value);
	private native boolean isRegisterEnabled(long ptr);
	
	private native boolean isRegistered(long ptr);
	private native void setDialPrefix(long ptr, String prefix);
	private native String getDialPrefix(long ptr);
	
	private native String normalizePhoneNumber(long ptr,String number);
	
	private native String getDomain(long ptr);
	
	private native void setDialEscapePlus(long ptr, boolean value);
	private native boolean getDialEscapePlus(long ptr);
	
	private native String getRoute(long ptr);
	private native int setRoute(long ptr,String uri);
	private native void enablePublish(long ptr,boolean enable);
	private native boolean publishEnabled(long ptr);
	private native void setContactParameters(long ptr, String params);
	
	private native int lookupCCCFromIso(long nativePtr, String iso);
	private native int lookupCCCFromE164(long nativePtr, String e164);
	
	public LinphoneProxyConfig enableRegister(boolean value) {
		enableRegister(nativePtr,value);
		return this;
	}

	public void done() {
		Object mutex=mCore!=null ? mCore : this;
		synchronized(mutex){
			done(nativePtr);
		}
	}

	public LinphoneProxyConfig edit() {
		Object mutex=mCore!=null ? mCore : this;
		synchronized(mutex){
			edit(nativePtr);
		}
		return this;
	}

	public void setIdentity(String identity) throws LinphoneCoreException {
		setIdentity(nativePtr,identity);
	}

	public void setProxy(String proxyUri) throws LinphoneCoreException {
		if (setProxy(nativePtr,proxyUri)!=0) {
			throw new LinphoneCoreException("Bad proxy address ["+proxyUri+"]");
		}
	}
	public String normalizePhoneNumber(String number) {
		return normalizePhoneNumber(nativePtr,number);
	}
	public void setDialPrefix(String prefix) {
		setDialPrefix(nativePtr, prefix);
	}
	public String getDialPrefix() {
		return getDialPrefix(nativePtr);
	}
	public String getDomain() {
		return getDomain(nativePtr);
	}
	public void setDialEscapePlus(boolean value) {
		 setDialEscapePlus(nativePtr,value);
	}
	public boolean getDialEscapePlus() {
		return getDialEscapePlus(nativePtr);
	}
	public String getIdentity() {
		return getIdentity(nativePtr);
	}
	public String getProxy() {
		return getProxy(nativePtr);
	}
	public boolean isRegistered() {
		return isRegistered(nativePtr);
	}
	public boolean registerEnabled() {
		return isRegisterEnabled(nativePtr);
	}
	public String getRoute() {
		return getRoute(nativePtr);
	}
	public void setRoute(String routeUri) throws LinphoneCoreException {
		if (setRoute(nativePtr, routeUri) != 0) {
			throw new LinphoneCoreException("cannot set route ["+routeUri+"]");
		}
	}
	public void enablePublish(boolean enable) {
		enablePublish(nativePtr,enable);
	}
	public RegistrationState getState() {
		return RegistrationState.fromInt(getState(nativePtr));
	}

	public void setExpires(int delay) {
		setExpires(nativePtr, delay);
	}
	public int getExpires() {
		return getExpires(nativePtr);
	}
	public boolean publishEnabled() {
		return publishEnabled(nativePtr); 
	}
	@Override
	public void setContactParameters(String params) {
		setContactParameters(nativePtr, params);
	}
	@Override
	public int lookupCCCFromIso(String iso) {
		return lookupCCCFromIso(nativePtr, iso);
	}
	@Override
	public int lookupCCCFromE164(String e164) {
		return lookupCCCFromE164(nativePtr, e164);
	}
	private native int getError(long nativeptr);
	@Override
	public Reason getError() {
		return Reason.fromInt(getError(nativePtr));
	}
	private native void setPrivacy(long nativePtr, int mask);
	@Override
	public void setPrivacy(int privacy_mask) {
		setPrivacy(nativePtr,privacy_mask);
	}

	private native int getPrivacy(long nativePtr);
	@Override
	public int getPrivacy() {
		return getPrivacy(nativePtr);
	}
	
	private native void enableAvpf(long nativePtr, boolean enable);
	@Override
	public void enableAvpf(boolean enable) {
		enableAvpf(nativePtr, enable);
	}

	private native boolean avpfEnabled(long nativePtr);
	@Override
	public boolean avpfEnabled() {
		return avpfEnabled(nativePtr);
	}
	
	private native void setAvpfRRInterval(long nativePtr, int interval);
	@Override
	public void setAvpfRRInterval(int interval) {
		setAvpfRRInterval(nativePtr, interval);
	}
	
	private native int getAvpfRRInterval(long nativePtr);
	@Override
	public int getAvpfRRInterval() {
		return getAvpfRRInterval(nativePtr);
	}
	
	private native String getContactParameters(long ptr);
	@Override
	public String getContactParameters() {
		return getContactParameters(nativePtr);
	}
	
	private native void setContactUriParameters(long ptr, String params);
	@Override
	public void setContactUriParameters(String params) {
		setContactUriParameters(nativePtr,params);
	}
	
	private native String getContactUriParameters(long ptr);
	@Override
	public String getContactUriParameters() {
		return getContactUriParameters(nativePtr);
	}
	private native long getErrorInfo(long nativePtr);
	@Override
	public ErrorInfo getErrorInfo() {
		return new ErrorInfoImpl(getErrorInfo(nativePtr));
	}
}
