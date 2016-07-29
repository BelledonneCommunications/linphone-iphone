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
		public final static Status AccountNotExist = new Status(5, "AccountNotExist");
		public final static Status AccountActivated = new Status(6, "AccountActivated");
		public final static Status AccountAlreadyActivated = new Status(7, "AccountAlreadyActivated");
		public final static Status AccountNotActivated = new Status(8, "AccountNotActivated");
		public final static Status EmailInvalid = new Status(9, "EmailInvalid");
		public final static Status UsernameInvalid = new Status(10, "UsernameInvalid");
		public final static Status UsernameTooShort = new Status(11, "UsernameTooShort");
		public final static Status UsernameTooLong = new Status(12, "UsernameTooLong");
		public final static Status UsernameInvalidSize = new Status(13, "UsernameInvalidSize");
		public final static Status PhoneNumberInvalid = new Status(14, "PhoneNumberInvalid");
		public final static Status PhoneNumberTooShort = new Status(15, "PhoneNumberTooShort");
		public final static Status PhoneNumberTooLong = new Status(16, "PhoneNumberTooLong");
		public final static Status PasswordTooShort = new Status(17, "PasswordTooShort");
		public final static Status PasswordTooLong = new Status(18, "PasswordTooLong");
		public final static Status DomainInvalid = new Status(19, "DomainInvalid");
		public final static Status RouteInvalid = new Status(20, "RouteInvalid");
		public final static Status DisplayNameInvalid = new Status(21, "DisplayNameInvalid");
		public final static Status TransportNotSupported = new Status(22, "TransportNotSupported");
		
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
	
	LinphoneProxyConfig configure();
}
