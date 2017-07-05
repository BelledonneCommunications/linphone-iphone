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
		void onAccountCreatorIsAccountUsed(LinphoneAccountCreator accountCreator, RequestStatus RequestStatus);
		void onAccountCreatorAccountCreated(LinphoneAccountCreator accountCreator, RequestStatus RequestStatus);
		void onAccountCreatorAccountActivated(LinphoneAccountCreator accountCreator, RequestStatus RequestStatus);
		void onAccountCreatorAccountLinkedWithPhoneNumber(LinphoneAccountCreator accountCreator, RequestStatus RequestStatus);
		void onAccountCreatorPhoneNumberLinkActivated(LinphoneAccountCreator accountCreator, RequestStatus RequestStatus);
		void onAccountCreatorIsAccountActivated(LinphoneAccountCreator accountCreator, RequestStatus RequestStatus);
		void onAccountCreatorPhoneAccountRecovered(LinphoneAccountCreator accountCreator, RequestStatus RequestStatus);
		void onAccountCreatorIsAccountLinked(LinphoneAccountCreator accountCreator, RequestStatus RequestStatus);
		void onAccountCreatorIsPhoneNumberUsed(LinphoneAccountCreator accountCreator, RequestStatus RequestStatus);
		void onAccountCreatorPasswordUpdated(LinphoneAccountCreator accountCreator, RequestStatus RequestStatus);
	}

	public static class UsernameCheck {
		static private Vector<UsernameCheck> values = new Vector<UsernameCheck>();
		private final int mValue;
		private final String mStringValue;
		public final int value() { return mValue; }

		public final static UsernameCheck Ok = new UsernameCheck(0, "Ok");
		public final static UsernameCheck TooShort = new UsernameCheck(1, "TooShort");
		public final static UsernameCheck TooLong = new UsernameCheck(2, "TooLong");
		public final static UsernameCheck InvalidCharacters = new UsernameCheck(3, "InvalidCharacters");
		public final static UsernameCheck Invalid = new UsernameCheck(4, "Invalid");

		private UsernameCheck(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}

		public static UsernameCheck fromInt(int value) {
			for (int i=0; i < values.size(); i++) {
				UsernameCheck state = (UsernameCheck) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("UsernameCheck not found [" + value + "]");
		}

		public String toString() {
			return mStringValue;
		}

		public int toInt() {
			return mValue;
		}
	}

	public static class PhoneNumberCheck {
		static private Vector<PhoneNumberCheck> values = new Vector<PhoneNumberCheck>();
		private final int mValue;
		private final String mStringValue;
		public final int value() { return mValue; }

		public final static PhoneNumberCheck Ok = new PhoneNumberCheck(0x1, "Ok");
		public final static PhoneNumberCheck TooShort = new PhoneNumberCheck(0x2, "TooShort");
		public final static PhoneNumberCheck TooLong = new PhoneNumberCheck(0x4, "TooLong");
		public final static PhoneNumberCheck CountryCodeInvalid = new PhoneNumberCheck(0x8, "CountryCodeInvalid");
		public final static PhoneNumberCheck Invalid = new PhoneNumberCheck(0x10, "Invalid");

		private PhoneNumberCheck(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}

		public static PhoneNumberCheck fromInt(int value) {
			for (int i=0; i < values.size(); i++) {
				PhoneNumberCheck state = (PhoneNumberCheck) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("UsernameCheck not found [" + value + "]");
		}

		public String toString() {
			return mStringValue;
		}

		public int toInt() {
			return mValue;
		}
	}

	public static class EmailCheck {
		static private Vector<EmailCheck> values = new Vector<EmailCheck>();
		private final int mValue;
		private final String mStringValue;
		public final int value() { return mValue; }

		public final static EmailCheck Ok = new EmailCheck(0, "Ok");
		public final static EmailCheck Malformed = new EmailCheck(1, "Malformed");
		public final static EmailCheck InvalidCharacters = new EmailCheck(2, "InvalidCharacters");

		private EmailCheck(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}

		public static EmailCheck fromInt(int value) {
			for (int i=0; i < values.size(); i++) {
				EmailCheck state = (EmailCheck) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("EmailCheck not found [" + value + "]");
		}

		public String toString() {
			return mStringValue;
		}

		public int toInt() {
			return mValue;
		}
	}

	public static class DomainCheck {
		static private Vector<DomainCheck> values = new Vector<DomainCheck>();
		private final int mValue;
		private final String mStringValue;
		public final int value() { return mValue; }

		public final static DomainCheck Ok = new DomainCheck(0, "Ok");
		public final static DomainCheck Invalid = new DomainCheck(1, "Invalid");

		private DomainCheck(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}

		public static DomainCheck fromInt(int value) {
			for (int i=0; i < values.size(); i++) {
				DomainCheck state = (DomainCheck) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("DomainCheck not found [" + value + "]");
		}

		public String toString() {
			return mStringValue;
		}

		public int toInt() {
			return mValue;
		}
	}

	public static class PasswordCheck {
		static private Vector<PasswordCheck> values = new Vector<PasswordCheck>();
		private final int mValue;
		private final String mStringValue;
		public final int value() { return mValue; }

		public final static PasswordCheck Ok = new PasswordCheck(0, "Ok");
		public final static PasswordCheck TooShort = new PasswordCheck(1, "TooShort");
		public final static PasswordCheck TooLong = new PasswordCheck(2, "TooLong");
		public final static PasswordCheck InvalidCharacters = new PasswordCheck(3, "InvalidCharacters");
		public final static PasswordCheck MissingCharacters = new PasswordCheck(4, "MissingCharacters");

		private PasswordCheck(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}

		public static PasswordCheck fromInt(int value) {
			for (int i=0; i < values.size(); i++) {
				PasswordCheck state = (PasswordCheck) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("PasswordCheck not found [" + value + "]");
		}

		public String toString() {
			return mStringValue;
		}

		public int toInt() {
			return mValue;
		}
	}

	public static class LanguageCheck {
		static private Vector<LanguageCheck> values = new Vector<LanguageCheck>();
		private final int mValue;
		private final String mStringValue;
		public final int value() { return mValue; }

		public final static LanguageCheck Ok = new LanguageCheck(0, "Ok");

		private LanguageCheck(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}

		public static LanguageCheck fromInt(int value) {
			for (int i=0; i < values.size(); i++) {
				LanguageCheck state = (LanguageCheck) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("LanguageCheck not found [" + value + "]");
		}

		public String toString() {
			return mStringValue;
		}

		public int toInt() {
			return mValue;
		}
	}

	public static class ActivationCodeCheck {
		static private Vector<ActivationCodeCheck> values = new Vector<ActivationCodeCheck>();
		private final int mValue;
		private final String mStringValue;
		public final int value() { return mValue; }

		public final static ActivationCodeCheck Ok = new ActivationCodeCheck(0, "Ok");
		public final static ActivationCodeCheck TooShort = new ActivationCodeCheck(1, "TooShort");
		public final static ActivationCodeCheck TooLong = new ActivationCodeCheck(2, "TooLong");
		public final static ActivationCodeCheck InvalidCharacters = new ActivationCodeCheck(3, "InvalidCharacters");

		private ActivationCodeCheck(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}

		public static ActivationCodeCheck fromInt(int value) {
			for (int i=0; i < values.size(); i++) {
				ActivationCodeCheck state = (ActivationCodeCheck) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("ActivationCodeCheck not found [" + value + "]");
		}

		public String toString() {
			return mStringValue;
		}

		public int toInt() {
			return mValue;
		}
	}

	public static class RequestStatus {
		static private Vector<RequestStatus> values = new Vector<RequestStatus>();
		private final int mValue;
		private final String mStringValue;
		public final int value() { return mValue; }

		public final static RequestStatus Ok = new RequestStatus(0, "Ok");
		public final static RequestStatus Failed = new RequestStatus(1, "Failed");
		public final static RequestStatus MissingArguments = new RequestStatus(2, "MissingArguments");
		public final static RequestStatus MissingCallbacks = new RequestStatus(3, "MissingCallbacks");

		public final static RequestStatus AccountCreated = new RequestStatus(4, "AccountCreated");
		public final static RequestStatus AccountNotCreated = new RequestStatus(5, "AccountNotCreated");

		public final static RequestStatus AccountExist = new RequestStatus(6, "AccountExist");
		public final static RequestStatus AccountExistWithAlias = new RequestStatus(7, "AccountExistWithAlias");
		public final static RequestStatus AccountNotExist = new RequestStatus(8, "AccountNotExist");
		public final static RequestStatus AliasIsAccount = new RequestStatus(9, "AliasIsAccount");
		public final static RequestStatus AliasExist = new RequestStatus(10, "AliasExist");
		public final static RequestStatus AliasNotExist = new RequestStatus(11, "AliasNotExist");

		public final static RequestStatus AccountActivated = new RequestStatus(12, "AccountActivated");
		public final static RequestStatus AccountAlreadyActivated = new RequestStatus(13, "AccountAlreadyActivated");
		public final static RequestStatus AccountNotActivated = new RequestStatus(14, "AccountNotActivated");

		public final static RequestStatus AccountLinked = new RequestStatus(15, "AccountLinked");
		public final static RequestStatus AccountNotLinked = new RequestStatus(16, "AccountNotLinked");

		public final static RequestStatus ErrorServer = new RequestStatus(17, "ErrorServer");

		private RequestStatus(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}

		public static RequestStatus fromInt(int value) {
			for (int i=0; i < values.size(); i++) {
				RequestStatus state = (RequestStatus) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("RequestStatus not found [" + value + "]");
		}

		public String toString() {
			return mStringValue;
		}

		public int toInt() {
			return mValue;
		}
	}

	void setListener(LinphoneAccountCreatorListener listener);

	UsernameCheck setUsername(String username);

	String getUsername();

	int setPhoneNumber(String phoneNumber, String countryCode);

	String getPhoneNumber();

	PasswordCheck setPassword(String password);

	String getPassword();

	PasswordCheck setHa1(String ha1);

	String getHa1();

	ActivationCodeCheck setActivationCode(String activationCode);

	LanguageCheck setLanguage(String lang);

	UsernameCheck setDisplayName(String displayName);

	String getDisplayName();

	EmailCheck setEmail(String email);

	String getEmail();

	String getPrefix(String phone);

	DomainCheck setDomain(String domain);

	String getDomain();

	LinphoneProxyConfig configure();

	RequestStatus isAccountUsed();

	RequestStatus createAccount();

	RequestStatus activateAccount();

	RequestStatus isAccountActivated();

	RequestStatus linkPhoneNumberWithAccount();

	RequestStatus activatePhoneNumberLink();

	RequestStatus isAccountLinked();

	RequestStatus isPhoneNumberUsed();

	RequestStatus recoverPhoneAccount();

	RequestStatus updatePassword(String newPassword);
}
