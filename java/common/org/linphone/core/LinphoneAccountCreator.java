/*LinphoneAccountCreator.java
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

import java.util.Vector;

import org.linphone.core.LinphoneAddress.TransportType;

public interface LinphoneAccountCreator {
	interface LinphoneAccountCreatorListener {
		void onAccountCreatorIsAccountUsed(LinphoneAccountCreator accountCreator, Status status);
		void onAccountCreatorAccountCreated(LinphoneAccountCreator accountCreator, Status status);
		void onAccountCreatorAccountActivated(LinphoneAccountCreator accountCreator, Status status);
		void onAccountCreatorAccountLinkedWithPhoneNumber(LinphoneAccountCreator accountCreator, Status status);
		void onAccountCreatorPhoneNumberLinkActivated(LinphoneAccountCreator accountCreator, Status status);
		void onAccountCreatorIsAccountActivated(LinphoneAccountCreator accountCreator, Status status);
		void onAccountCreatorPhoneAccountRecovered(LinphoneAccountCreator accountCreator, Status status);
		void onAccountCreatorIsAccountLinked(LinphoneAccountCreator accountCreator, Status status);
		void onAccountCreatorIsPhoneNumberUsed(LinphoneAccountCreator accountCreator, Status status);
		void onAccountCreatorPasswordUpdated(LinphoneAccountCreator accountCreator, Status status);
	}

	public static class Status {
		static private Vector<Status> values = new Vector<Status>();
		private final int mValue;
		private final String mStringValue;
		public final int value() { return mValue; }

		public final static Status Ok = new Status(0, "Ok");
		public final static Status Failed = new Status(1, "Failed");
		public final static Status AccountCreated = new Status(2, "AccountCreated");
		public final static Status AccountNotCreated = new Status(3, "AccountNotCreated");
		public final static Status AccountExist = new Status(4, "AccountExist");
		public final static Status AccountExistWithAlias = new Status(5, "AccountExistWithAlias");
		public final static Status AccountNotExist = new Status(6, "AccountNotExist");
		public final static Status AccountActivated = new Status(7, "AccountActivated");
		public final static Status AccountAlreadyActivated = new Status(8, "AccountAlreadyActivated");
		public final static Status AccountNotActivated = new Status(9, "AccountNotActivated");
		public final static Status AccountLinked = new Status(10, "AccountLinked");
		public final static Status AccountNotLinked = new Status(11, "AccountNotLinked");
		public final static Status EmailInvalid = new Status(12, "EmailInvalid");
		public final static Status UsernameInvalid = new Status(13, "UsernameInvalid");
		public final static Status UsernameTooShort = new Status(14, "UsernameTooShort");
		public final static Status UsernameTooLong = new Status(15, "UsernameTooLong");
		public final static Status UsernameInvalidSize = new Status(16, "UsernameInvalidSize");
		public final static Status PhoneNumberInvalid = new Status(17, "PhoneNumberInvalid");
		public final static Status PhoneNumberTooShort = new Status(18, "PhoneNumberTooShort");
		public final static Status PhoneNumberTooLong = new Status(19, "PhoneNumberTooLong");
		public final static Status PhoneNumberUsedAccount = new Status(20, "PhoneNumberUsed");
		public final static Status PhoneNumberUsedAlias = new Status(21, "PhoneNumberUsed");
		public final static Status PhoneNumberNotUsed = new Status(22, "PhoneNumberNotUsed");
		public final static Status PasswordTooShort = new Status(23, "PasswordTooShort");
		public final static Status PasswordTooLong = new Status(24, "PasswordTooLong");
		public final static Status DomainInvalid = new Status(25, "DomainInvalid");
		public final static Status RouteInvalid = new Status(26, "RouteInvalid");
		public final static Status DisplayNameInvalid = new Status(27, "DisplayNameInvalid");
		public final static Status TransportNotSupported = new Status(28, "TransportNotSupported");
		public final static Status CountryCodeInvalid = new Status(29, "CountryCodeInvalid");
		public final static Status ErrorServer = new Status(30, "ErrorServer");

		private Status(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}

		public static Status fromInt(int value) {
			for (int i=0; i < values.size(); i++) {
				Status state = (Status) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("Status not found [" + value + "]");
		}

		public String toString() {
			return mStringValue;
		}

		public int toInt() {
			return mValue;
		}
	}

	void setListener(LinphoneAccountCreatorListener listener);

	Status setUsername(String username);

	String getUsername();

	Status setPhoneNumber(String phoneNumber, String countryCode);

	String getPhoneNumber();

	Status setPassword(String password);

	String getPassword();

	Status setHa1(String ha1);

	String getHa1();

	Status setActivationCode(String activationCode);

	Status setLanguage(String lang);

	Status setTransport(TransportType transport);

	TransportType getTransport();

	Status setDomain(String domain);

	String getDomain();

	Status setRoute(String route);

	String getRoute();

	Status setDisplayName(String displayName);

	String getDisplayName();

	Status setEmail(String email);

	String getEmail();

	String getPrefix(String phone);

	Status isAccountUsed();

	Status createAccount();

	Status activateAccount();

	Status isAccountActivated();

	Status linkPhoneNumberWithAccount();

	Status activatePhoneNumberLink();

	Status isAccountLinked();

	Status isPhoneNumberUsed();

	Status recoverPhoneAccount();

	Status updatePassword(String newPassword);

	LinphoneProxyConfig configure();
}
