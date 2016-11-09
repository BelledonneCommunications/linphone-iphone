/*LinphoneAccountCreatorImpl.java
Copyright (C) 2016  Belledonne Communications, Grenoble, France

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

import org.linphone.core.LinphoneAddress.TransportType;

public class LinphoneAccountCreatorImpl implements LinphoneAccountCreator {
	protected long nativePtr;

	protected LinphoneAccountCreatorImpl(long aNativePtr)  {
		nativePtr = aNativePtr;
	}

	private native long newLinphoneAccountCreator(long lc, String url);
	public LinphoneAccountCreatorImpl(LinphoneCore lc, String url) {
		nativePtr = newLinphoneAccountCreator(((LinphoneCoreImpl)lc).nativePtr, url);
	}

	public long getNativePtr() {
		return nativePtr;
	}

	private native void unref(long ptr);
	protected void finalize() {
		unref(nativePtr);
	}

	private native void setListener(long ptr, LinphoneAccountCreatorListener listener);
	@Override
	public void setListener(LinphoneAccountCreatorListener listener) {
		setListener(nativePtr, listener);
	}

	private native int setUsername(long ptr, String username);
	@Override
	public Status setUsername(String username) {
		return Status.fromInt(setUsername(nativePtr, username));
	}

	private native String getUsername(long ptr);
	@Override
	public String getUsername() {
		return getUsername(nativePtr);
	}

	private native int setPhoneNumber(long ptr, String phoneNumber, String countryCode);
	@Override
	public Status setPhoneNumber(String phoneNumber, String countryCode) {
		return Status.fromInt(setPhoneNumber(nativePtr, phoneNumber, countryCode));
	}

	private native String getPhoneNumber(long ptr);
	@Override
	public String getPhoneNumber() {
		return getPhoneNumber(nativePtr);
	}

	private native int setPassword(long ptr, String password);
	@Override
	public Status setPassword(String password) {
		return Status.fromInt(setPassword(nativePtr, password));
	}

	private native String getPassword(long ptr);
	@Override
	public String getPassword() {
		return getPassword(nativePtr);
	}

	private native int setHa1(long ptr, String ha1);
	@Override
	public Status setHa1(String ha1) {
		return Status.fromInt(setHa1(nativePtr, ha1));
	}

	private native String getHa1(long ptr);
	@Override
	public String getHa1() {
		return getHa1(nativePtr);
	}

	private native int setActivationCode(long ptr, String activationCode);
	@Override
	public Status setActivationCode(String activationCode) {
		return Status.fromInt(setActivationCode(nativePtr, activationCode));
	}

	private native int setLanguage(long ptr, String lang);
	@Override
	public Status setLanguage(String lang) {
		return Status.fromInt(setLanguage(nativePtr, lang));
	}

	private native int setTransport(long ptr, int transport);
	@Override
	public Status setTransport(TransportType transport) {
		return Status.fromInt(setTransport(nativePtr, transport.toInt()));
	}

	private native int getTransport(long ptr);
	@Override
	public TransportType getTransport() {
		return TransportType.fromInt(getTransport(nativePtr));
	}

	private native int setDomain(long ptr, String domain);
	@Override
	public Status setDomain(String domain) {
		return Status.fromInt(setDomain(nativePtr, domain));
	}

	private native String getDomain(long ptr);
	@Override
	public String getDomain() {
		return getDomain(nativePtr);
	}

	private native int setRoute(long ptr, String route);
	@Override
	public Status setRoute(String route) {
		return Status.fromInt(setRoute(nativePtr, route));
	}

	private native String getRoute(long ptr);
	@Override
	public String getRoute() {
		return getRoute(nativePtr);
	}

	private native int setDisplayName(long ptr, String displayName);
	@Override
	public Status setDisplayName(String displayName) {
		return Status.fromInt(setDisplayName(nativePtr, displayName));
	}

	private native String getDisplayName(long ptr);
	@Override
	public String getDisplayName() {
		return getDisplayName(nativePtr);
	}

	private native int setEmail(long ptr, String email);
	@Override
	public Status setEmail(String email) {
		return Status.fromInt(setEmail(nativePtr, email));
	}

	private native String getEmail(long ptr);
	@Override
	public String getEmail() {
		return getEmail(nativePtr);
	}

	private native String getPrefix(long ptr, String s);
	@Override
	public String getPrefix(String phone) {
		return getPrefix(nativePtr, phone);
	}

	private native int isAccountUsed(long ptr);
	@Override
	public Status isAccountUsed() {
		return Status.fromInt(isAccountUsed(nativePtr));
	}

	private native int createAccount(long ptr);
	@Override
	public Status createAccount() {
		return Status.fromInt(createAccount(nativePtr));
	}

	private native int activateAccount(long ptr);
	@Override
	public Status activateAccount() {
		return Status.fromInt(activateAccount(nativePtr));
	}

	private native int isAccountLinked(long ptr);
	@Override
	public Status isAccountLinked() {
		return Status.fromInt(isAccountLinked(nativePtr));
	}

	private native int isPhoneNumberUsed(long ptr);
	@Override
	public Status isPhoneNumberUsed() {
		return Status.fromInt(isPhoneNumberUsed(nativePtr));
	}

	private native int isAccountActivated(long ptr);
	@Override
	public Status isAccountActivated() {
		return Status.fromInt(isAccountActivated(nativePtr));
	}

	private native int linkPhoneNumberWithAccount(long ptr);
	@Override
	public Status linkPhoneNumberWithAccount() {
		return Status.fromInt(linkPhoneNumberWithAccount(nativePtr));
	}

	private native int activatePhoneNumberLink(long ptr);
	@Override
	public Status activatePhoneNumberLink() {
		return Status.fromInt(activatePhoneNumberLink(nativePtr));
	}

	private native int recoverPhoneAccount(long ptr);
	@Override
	public Status recoverPhoneAccount() {
		return Status.fromInt(recoverPhoneAccount(nativePtr));
	}

	private native int updatePassword(long ptr, String newPassword);
	@Override
	public Status updatePassword(String newPassword) {
		return Status.fromInt(updatePassword(nativePtr, newPassword));
	}

	private native LinphoneProxyConfig configure(long ptr);
	@Override
	public LinphoneProxyConfig configure() {
		return configure(nativePtr);
	}
}
