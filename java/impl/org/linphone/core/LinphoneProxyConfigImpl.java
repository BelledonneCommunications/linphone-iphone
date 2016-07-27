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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
package org.linphone.core;

import org.linphone.core.LinphoneCore.RegistrationState;

class LinphoneProxyConfigImpl implements LinphoneProxyConfig {

	protected final long nativePtr;
	protected LinphoneCoreImpl mCore;
	Object userData;
	
	private native void finalize(long ptr);
	private native int getState(long nativePtr);
	private native void setExpires(long nativePtr, int delay);
	private native int getExpires(long nativePtr);
	private native long createProxyConfig( long nativePtr);

	protected LinphoneProxyConfigImpl(LinphoneCoreImpl core,String identity,String proxy,String route, boolean enableRegister) throws LinphoneCoreException {
		mCore=core;
		nativePtr = createProxyConfig(core.nativePtr);
		setIdentity(identity);
		setProxy(proxy);
		setRoute(route);
		enableRegister(enableRegister);
	}

	protected LinphoneProxyConfigImpl(LinphoneCoreImpl core)  {
		mCore=core;
		nativePtr = createProxyConfig(core.nativePtr);
	}
	/*reserved for JNI */
	protected LinphoneProxyConfigImpl(LinphoneCoreImpl core, long aNativePtr)  {
		mCore=core;
		nativePtr = aNativePtr;
	}

	private void isValid() {
		if (nativePtr == 0) {
			throw new RuntimeException("proxy config removed");
		}
	}

	protected void finalize() throws Throwable {
		if (nativePtr != 0) {
			finalize(nativePtr);
		}
		super.finalize();
	}
	private native long newLinphoneProxyConfig();

	private native void edit(long ptr);
	private native void done(long ptr);

	private native void setIdentity(long ptr,String identity);
	private native String getIdentity(long ptr);
	private native void setAddress(long ptr, long address);
	private native long getAddress(long ptr);
	private native int setProxy(long ptr,String proxy);
	private native String getProxy(long ptr);


	private native void enableRegister(long ptr,boolean value);
	private native boolean isRegisterEnabled(long ptr);

	private native boolean isRegistered(long ptr);
	private native void setDialPrefix(long ptr, String prefix);
	private native String getDialPrefix(long ptr);

	private native String normalizePhoneNumber(long ptr,String number);
	private native long normalizeSipUri(long ptr,String username);

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
		isValid();
		enableRegister(nativePtr,value);
		return this;
	}

	public void done() {
		isValid();
		Object mutex=mCore!=null ? mCore : this;
		synchronized(mutex){
			done(nativePtr);
		}
	}

	public LinphoneProxyConfig edit() {
		isValid();
		Object mutex=mCore!=null ? mCore : this;
		synchronized(mutex){
			edit(nativePtr);
		}
		return this;
	}

	public void setIdentity(String identity) throws LinphoneCoreException {
		isValid();
		setIdentity(nativePtr,identity);
	}

	public void setAddress(LinphoneAddress address) throws LinphoneCoreException {
		isValid();
		setAddress(nativePtr,((LinphoneAddressImpl)address).nativePtr);
	}

	public void setProxy(String proxyUri) throws LinphoneCoreException {
		isValid();
		if (setProxy(nativePtr,proxyUri)!=0) {
			throw new LinphoneCoreException("Bad proxy address ["+proxyUri+"]");
		}
	}
	public String normalizePhoneNumber(String number) {
		isValid();
		return normalizePhoneNumber(nativePtr,number);
	}
	public LinphoneAddress normalizeSipUri(String username) {
		isValid();
		long ptr = normalizeSipUri(nativePtr,username);
		if (ptr==0) {
			return null;
		} else {
			return new LinphoneAddressImpl(ptr,LinphoneAddressImpl.WrapMode.FromConst);
		}
	}
	public void setDialPrefix(String prefix) {
		isValid();
		setDialPrefix(nativePtr, prefix);
	}
	public String getDialPrefix() {
		isValid();
		return getDialPrefix(nativePtr);
	}
	public String getDomain() {
		isValid();
		return getDomain(nativePtr);
	}
	public void setDialEscapePlus(boolean value) {
		isValid();
		setDialEscapePlus(nativePtr,value);
	}
	public boolean getDialEscapePlus() {
		isValid();
		return getDialEscapePlus(nativePtr);
	}
	public String getIdentity() {
		isValid();
		return getIdentity(nativePtr);
	}
	public LinphoneAddress getAddress() {
		isValid();
		long ptr = getAddress(nativePtr);
		if (ptr==0) {
			return null;
		} else {
			return new LinphoneAddressImpl(ptr,LinphoneAddressImpl.WrapMode.FromConst);
		}
	}
	public String getProxy() {
		isValid();
		return getProxy(nativePtr);
	}
	public boolean isRegistered() {
		isValid();
		return isRegistered(nativePtr);
	}
	public boolean registerEnabled() {
		isValid();
		return isRegisterEnabled(nativePtr);
	}
	public String getRoute() {
		isValid();
		return getRoute(nativePtr);
	}
	public void setRoute(String routeUri) throws LinphoneCoreException {
		isValid();
		if (setRoute(nativePtr, routeUri) != 0) {
			throw new LinphoneCoreException("cannot set route ["+routeUri+"]");
		}
	}
	public void enablePublish(boolean enable) {
		isValid();
		enablePublish(nativePtr,enable);
	}
	public RegistrationState getState() {
		isValid();
		return RegistrationState.fromInt(getState(nativePtr));
	}

	public void setExpires(int delay) {
		isValid();
		setExpires(nativePtr, delay);
	}
	public int getExpires() {
		isValid();
		return getExpires(nativePtr);
	}
	public boolean publishEnabled() {
		isValid();
		return publishEnabled(nativePtr);
	}
	@Override
	public void setContactParameters(String params) {
		isValid();
		setContactParameters(nativePtr, params);
	}
	@Override
	public int lookupCCCFromIso(String iso) {
		isValid();
		return lookupCCCFromIso(nativePtr, iso);
	}
	@Override
	public int lookupCCCFromE164(String e164) {
		isValid();
		return lookupCCCFromE164(nativePtr, e164);
	}
	private native int getError(long nativeptr);
	@Override
	public Reason getError() {
		isValid();
		return Reason.fromInt(getError(nativePtr));
	}
	private native void setPrivacy(long nativePtr, int mask);
	@Override
	public void setPrivacy(int privacy_mask) {
		isValid();
		setPrivacy(nativePtr,privacy_mask);
	}

	private native int getPrivacy(long nativePtr);
	@Override
	public int getPrivacy() {
		isValid();
		return getPrivacy(nativePtr);
	}

	private native void enableAvpf(long nativePtr, boolean enable);
	@Override
	public void enableAvpf(boolean enable) {
		isValid();
		enableAvpf(nativePtr, enable);
	}

	private native boolean avpfEnabled(long nativePtr);
	@Override
	public boolean avpfEnabled() {
		isValid();
		return avpfEnabled(nativePtr);
	}

	private native void setAvpfRRInterval(long nativePtr, int interval);
	@Override
	public void setAvpfRRInterval(int interval) {
		isValid();
		setAvpfRRInterval(nativePtr, interval);
	}

	private native int getAvpfRRInterval(long nativePtr);
	@Override
	public int getAvpfRRInterval() {
		isValid();
		return getAvpfRRInterval(nativePtr);
	}

	private native String getContactParameters(long ptr);
	@Override
	public String getContactParameters() {
		isValid();
		return getContactParameters(nativePtr);
	}

	private native void setContactUriParameters(long ptr, String params);
	@Override
	public void setContactUriParameters(String params) {
		isValid();
		setContactUriParameters(nativePtr,params);
	}

	private native String getContactUriParameters(long ptr);
	@Override
	public String getContactUriParameters() {
		isValid();
		return getContactUriParameters(nativePtr);
	}

	private native long getErrorInfo(long nativePtr);

	@Override
	public ErrorInfo getErrorInfo() {
		return new ErrorInfoImpl(getErrorInfo(nativePtr));
	}

	private native void enableQualityReporting(long nativePtr, boolean enable);
	@Override
	public void enableQualityReporting(boolean enable) {
		isValid();
		enableQualityReporting(nativePtr, enable);
	}

	private native boolean qualityReportingEnabled(long nativePtr);
	@Override
	public boolean qualityReportingEnabled() {
		isValid();
		return avpfEnabled(nativePtr);
	}

	private native void setQualityReportingInterval(long nativePtr, int interval);
	@Override
	public void setQualityReportingInterval(int interval) {
		isValid();
		setQualityReportingInterval(nativePtr, interval);
	}
	private native int getQualityReportingInterval(long nativePtr);
	@Override
	public int getQualityReportingInterval() {
		isValid();
		return getQualityReportingInterval(nativePtr);
	}

	private native void setQualityReportingCollector(long nativePtr, String collector);
	@Override
	public void setQualityReportingCollector(String collector) {
		isValid();
		setQualityReportingCollector(nativePtr, collector);
	}
	private native String getQualityReportingCollector(long nativePtr);
	@Override
	public String getQualityReportingCollector() {

		isValid();
		return getQualityReportingCollector(nativePtr);
	}

	private native void setRealm(long nativePtr, String realm);
	@Override
	public void setRealm(String realm) {
		isValid();
		setRealm(nativePtr, realm);
	}
	private native String getRealm(long nativePtr);
	@Override
	public String getRealm() {

		isValid();
		return getRealm(nativePtr);
	}

	private native void setPublishExpires(long nativePtr, int expires);
	@Override
	public void setPublishExpires(int expires) {
		isValid();
		setPublishExpires(nativePtr, expires);
	}
	private native int getPublishExpires(long nativePtr);
	@Override
	public int getPublishExpires() {

		isValid();
		return getPublishExpires(nativePtr);
	}

	private native boolean isPhoneNumber(long nativePtr,String username);
	@Override
	public boolean isPhoneNumber(String username){
		return isPhoneNumber(nativePtr,username);
	}

	@Override
	public void setUserData(Object obj) {
		userData = obj;
	}
	@Override
	public Object getUserData() {
		return userData;
	}
	
	
	private native void setCustomHeader(long ptr, String name, String value);
	@Override
	public void setCustomHeader(String name, String value){
		setCustomHeader(nativePtr, name, value);
	}
	
	
	private native String getCustomHeader(long ptr, String name);
	@Override
	public String getCustomHeader(String name){
		return getCustomHeader(nativePtr, name);
	}
	
	
	
	
}
