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
	public UsernameCheck setUsername(String username) {
		return UsernameCheck.fromInt(setUsername(nativePtr, username));
	}

	private native String getUsername(long ptr);
	@Override
	public String getUsername() {
		return getUsername(nativePtr);
	}

	private native int setPhoneNumber(long ptr, String phoneNumber, String countryCode);
	@Override
	public int setPhoneNumber(String phoneNumber, String countryCode) {
		return setPhoneNumber(nativePtr, phoneNumber, countryCode);
	}

	private native String getPhoneNumber(long ptr);
	@Override
	public String getPhoneNumber() {
		return getPhoneNumber(nativePtr);
	}

	private native int setPassword(long ptr, String password);
	@Override
	public PasswordCheck setPassword(String password) {
		return PasswordCheck.fromInt(setPassword(nativePtr, password));
	}

	private native String getPassword(long ptr);
	@Override
	public String getPassword() {
		return getPassword(nativePtr);
	}

	private native int setHa1(long ptr, String ha1);
	@Override
	public PasswordCheck setHa1(String ha1) {
		return PasswordCheck.fromInt(setHa1(nativePtr, ha1));
	}

	private native String getHa1(long ptr);
	@Override
	public String getHa1() {
		return getHa1(nativePtr);
	}

	private native int setActivationCode(long ptr, String activationCode);
	@Override
	public ActivationCodeCheck setActivationCode(String activationCode) {
		return ActivationCodeCheck.fromInt(setActivationCode(nativePtr, activationCode));
	}

	private native int setLanguage(long ptr, String lang);
	@Override
	public LanguageCheck setLanguage(String lang) {
		return LanguageCheck.fromInt(setLanguage(nativePtr, lang));
	}

	private native int setDisplayName(long ptr, String displayName);
	@Override
	public UsernameCheck setDisplayName(String displayName) {
		return UsernameCheck.fromInt(setDisplayName(nativePtr, displayName));
	}

	private native String getDisplayName(long ptr);
	@Override
	public String getDisplayName() {
		return getDisplayName(nativePtr);
	}

	private native int setEmail(long ptr, String email);
	@Override
	public EmailCheck setEmail(String email) {
		return EmailCheck.fromInt(setEmail(nativePtr, email));
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

	private native int setDomain(long ptr, String domain);
	@Override
	public DomainCheck setDomain(String domain) {
		return DomainCheck.fromInt(setDomain(nativePtr, domain));
	}

	private native String getDomain(long ptr);
	@Override
	public String getDomain() {
		return getDomain(nativePtr);
	}

	private native LinphoneProxyConfig configure(long ptr);
	@Override
	public LinphoneProxyConfig configure() {
		return configure(nativePtr);
	}

	private native int isAccountUsed(long ptr);
	@Override
	public RequestStatus isAccountUsed() {
		return RequestStatus.fromInt(isAccountUsed(nativePtr));
	}

	private native int createAccount(long ptr);
	@Override
	public RequestStatus createAccount() {
		return RequestStatus.fromInt(createAccount(nativePtr));
	}

	private native int activateAccount(long ptr);
	@Override
	public RequestStatus activateAccount() {
		return RequestStatus.fromInt(activateAccount(nativePtr));
	}

	private native int isAccountLinked(long ptr);
	@Override
	public RequestStatus isAccountLinked() {
		return RequestStatus.fromInt(isAccountLinked(nativePtr));
	}

	private native int isPhoneNumberUsed(long ptr);
	@Override
	public RequestStatus isPhoneNumberUsed() {
		return RequestStatus.fromInt(isPhoneNumberUsed(nativePtr));
	}

	private native int isAccountActivated(long ptr);
	@Override
	public RequestStatus isAccountActivated() {
		return RequestStatus.fromInt(isAccountActivated(nativePtr));
	}

	private native int linkPhoneNumberWithAccount(long ptr);
	@Override
	public RequestStatus linkPhoneNumberWithAccount() {
		return RequestStatus.fromInt(linkPhoneNumberWithAccount(nativePtr));
	}

	private native int activatePhoneNumberLink(long ptr);
	@Override
	public RequestStatus activatePhoneNumberLink() {
		return RequestStatus.fromInt(activatePhoneNumberLink(nativePtr));
	}

	private native int recoverPhoneAccount(long ptr);
	@Override
	public RequestStatus recoverPhoneAccount() {
		return RequestStatus.fromInt(recoverPhoneAccount(nativePtr));
	}

	private native int updatePassword(long ptr, String newPassword);
	@Override
	public RequestStatus updatePassword(String newPassword) {
		return RequestStatus.fromInt(updatePassword(nativePtr, newPassword));
	}
}
