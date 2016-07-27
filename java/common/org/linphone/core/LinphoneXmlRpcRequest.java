/*
LinphoneXmlRpcRequest.java
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

public interface LinphoneXmlRpcRequest {
	interface LinphoneXmlRpcRequestListener {
		void onXmlRpcRequestResponse(LinphoneXmlRpcRequest request);
	}
	
	public static class ArgType {
		static private Vector<ArgType> values = new Vector<ArgType>();
		private final int mValue;
		private final String mStringValue;
		
		public final int value() { return mValue; }

		public final static ArgType None = new ArgType(0, "None");
		public final static ArgType Int = new ArgType(1, "Int");
		public final static ArgType String = new ArgType(2, "String");
		
		private ArgType(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}
		
		public static ArgType fromInt(int value) {

			for (int i=0; i < values.size(); i++) {
				ArgType state = (ArgType) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("ArgType not found [" + value + "]");
		}
		
		public String toString() {
			return mStringValue;
		}
		
		public int toInt() {
			return mValue;
		}
	}
	
	public static class Status {
		static private Vector<Status> values = new Vector<Status>();
		private final int mValue;
		private final String mStringValue;
		
		public final int value() { return mValue; }

		public final static Status Pending = new Status(0, "Pending");
		public final static Status Ok = new Status(1, "Ok");
		public final static Status Failed = new Status(2, "Failed");
		
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
	
	void addIntArg(int arg);
	
	void addStringArg(String arg);
	
	String getContent();
	
	Status getStatus();
	
	int getIntResponse();
	
	String getStringResponse();
	
	void setListener(LinphoneXmlRpcRequestListener listener);
}
