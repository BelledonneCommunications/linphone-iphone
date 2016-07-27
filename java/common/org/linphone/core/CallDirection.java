/*
CallDirection.java
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
/**
 * Enum representing the direction of a call.
**/
 public class CallDirection {
	 /**
	  * outgoing calls*
	  * */
	public static CallDirection Outgoing = new CallDirection("CallOutgoing");
	/**
	 * incoming calls
	 */
	public static CallDirection Incoming = new CallDirection("Callincoming");
	private String mStringValue;
	private CallDirection(String aStringValue) {
		mStringValue = aStringValue;
	}
	public String toString() {
		return mStringValue;
	}
}
