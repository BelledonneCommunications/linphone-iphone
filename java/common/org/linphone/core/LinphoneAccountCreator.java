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
	}

	public static class Status {
		static private Vector<Status> values = new Vector<Status>();
		private final int mValue;
		private final String mStringValue;
		private static int mStatusNum = 0;
		public final int value() { return mValue; }

		public final static Status Ok = new Status(mStatusNum++, "Ok");
		public final static Status Failed = new Status(mStatusNum++, "Failed");
		public final static Status AccountCreated = new Status(mStatusNum++, "AccountCreated");
		public final static Status AccountNotCreated = new Status(mStatusNum++, "AccountNotCreated");
		public final static Status AccountExist = new Status(mStatusNum++, "AccountExist");
		public final static Status AccountExistWithAlias = new Status(mStatusNum++, "AccountExistWithAlias");
		public final static Status AccountNotExist = new Status(mStatusNum++, "AccountNotExist");
		public final static Status AccountActivated = new Status(mStatusNum++, "AccountActivated");
		public final static Status AccountAlreadyActivated = new Status(mStatusNum++, "AccountAlreadyActivated");
		public final static Status AccountNotActivated = new Status(mStatusNum++, "AccountNotActivated");
		public final static Status AccountLinked = new Status(mStatusNum++, "AccountLinked");
		public final static Status AccountNotLinked = new Status(mStatusNum++, "AccountNotLinked");
		public final static Status EmailInvalid = new Status(mStatusNum++, "EmailInvalid");
		public final static Status UsernameInvalid = new Status(mStatusNum++, "UsernameInvalid");
		public final static Status UsernameTooShort = new Status(mStatusNum++, "UsernameTooShort");
		public final static Status UsernameTooLong = new Status(mStatusNum++, "UsernameTooLong");
		public final static Status UsernameInvalidSize = new Status(mStatusNum++, "UsernameInvalidSize");
		public final static Status PhoneNumberInvalid = new Status(mStatusNum++, "PhoneNumberInvalid");
		public final static Status PhoneNumberTooShort = new Status(mStatusNum++, "PhoneNumberTooShort");
		public final static Status PhoneNumberTooLong = new Status(mStatusNum++, "PhoneNumberTooLong");
		public final static Status PhoneNumberUsedAccount = new Status(mStatusNum++, "PhoneNumberUsed");
		public final static Status PhoneNumberUsedAlias = new Status(mStatusNum++, "PhoneNumberUsed");
		public final static Status PhoneNumberNotUsed = new Status(mStatusNum++, "PhoneNumberNotUsed");
		public final static Status PasswordTooShort = new Status(mStatusNum++, "PasswordTooShort");
		public final static Status PasswordTooLong = new Status(mStatusNum++, "PasswordTooLong");
		public final static Status DomainInvalid = new Status(mStatusNum++, "DomainInvalid");
		public final static Status RouteInvalid = new Status(mStatusNum++, "RouteInvalid");
		public final static Status DisplayNameInvalid = new Status(mStatusNum++, "DisplayNameInvalid");
		public final static Status TransportNotSupported = new Status(mStatusNum++, "TransportNotSupported");
		public final static Status CountryCodeInvalid = new Status(mStatusNum++, "CountryCodeInvalid");

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

	Status isAccountUsed();

	Status createAccount();

	Status activateAccount();

	Status isAccountActivated();

	Status linkPhoneNumberWithAccount();

	Status activatePhoneNumberLink();

	Status isAccountLinked();

	Status isPhoneNumberUsed();

	Status recoverPhoneAccount();

	LinphoneProxyConfig configure();
}
