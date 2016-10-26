/*
LinphoneNatPolicyImpl.java
Copyright (C) 2015  Belledonne Communications, Grenoble, France

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

public class LinphoneNatPolicyImpl implements LinphoneNatPolicy {
	protected final long mNativePtr;

	private native Object getCore(long nativePtr);
	private native void clear(long nativePtr);
	private native boolean stunEnabled(long nativePtr);
	private native void enableStun(long nativePtr, boolean enable);
	private native boolean turnEnabled(long nativePtr);
	private native void enableTurn(long nativePtr, boolean enable);
	private native boolean iceEnabled(long nativePtr);
	private native void enableIce(long nativePtr, boolean enable);
	private native boolean upnpEnabled(long nativePtr);
	private native void enableUpnp(long nativePtr, boolean enable);
	private native String getStunServer(long nativePtr);
	private native void setStunServer(long nativePtr, String stun_server);
	private native String getStunServerUsername(long nativePtr);
	private native void setStunServerUsername(long nativePtr, String username);

	protected LinphoneNatPolicyImpl(long nativePtr)  {
		mNativePtr = nativePtr;
	}

	private synchronized LinphoneCore getCore() {
		return (LinphoneCore)getCore(mNativePtr);
	}

	public void clear() {
		synchronized(getCore()) {
			clear(mNativePtr);
		}
	}

	public boolean stunEnabled() {
		synchronized(getCore()) {
			return stunEnabled(mNativePtr);
		}
	}

	public void enableStun(boolean enable) {
		synchronized(getCore()) {
			enableStun(mNativePtr, enable);
		}
	}

	public boolean turnEnabled() {
		synchronized(getCore()) {
			return turnEnabled(mNativePtr);
		}
	}

	public void enableTurn(boolean enable) {
		synchronized(getCore()) {
			enableTurn(mNativePtr, enable);
		}
	}

	public boolean iceEnabled() {
		synchronized(getCore()) {
			return iceEnabled(mNativePtr);
		}
	}

	public void enableIce(boolean enable) {
		synchronized(getCore()) {
			enableIce(mNativePtr, enable);
		}
	}

	public boolean upnpEnabled() {
		synchronized(getCore()) {
			return upnpEnabled(mNativePtr);
		}
	}

	public void enableUpnp(boolean enable) {
		synchronized(getCore()) {
			enableUpnp(mNativePtr, enable);
		}
	}

	public String getStunServer() {
		synchronized(getCore()) {
			return getStunServer(mNativePtr);
		}
	}

	public void setStunServer(String stun_server) {
		synchronized(getCore()) {
			setStunServer(mNativePtr, stun_server);
		}
	}

	public String getStunServerUsername() {
		synchronized(getCore()) {
			return getStunServerUsername(mNativePtr);
		}
	}

	public void setStunServerUsername(String username) {
		synchronized(getCore()) {
			setStunServerUsername(mNativePtr, username);
		}
	}
}
