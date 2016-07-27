/*
PresenceActivityImpl.java
Copyright (C) 2010-2013  Belledonne Communications, Grenoble, France

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

public class PresenceActivityImpl implements PresenceActivity {
	private long mNativePtr;

	protected PresenceActivityImpl(long nativePtr) {
		mNativePtr = nativePtr;
	}

	private native long newPresenceActivityImpl(int type, String description);
	protected PresenceActivityImpl(PresenceActivityType type, String description) {
		mNativePtr = newPresenceActivityImpl(type.toInt(), description);
	}

	private native void unref(long nativePtr);
	protected void finalize() {
		unref(mNativePtr);
	}

	private native String toString(long nativePtr);
	@Override
	public String toString() {
		return toString(mNativePtr);
	}

	private native int getType(long nativePtr);
	@Override
	public PresenceActivityType getType() {
		return PresenceActivityType.fromInt(getType(mNativePtr));
	}

	private native int setType(long nativePtr, int type);
	@Override
	public int setType(PresenceActivityType type) {
		return setType(mNativePtr, type.toInt());
	}

	private native String getDescription(long nativePtr);
	@Override
	public String getDescription() {
		return getDescription(mNativePtr);
	}

	private native int setDescription(long nativePtr, String description);
	@Override
	public int setDescription(String description) {
		return setDescription(mNativePtr, description);
	}

	public long getNativePtr() {
		return mNativePtr;
	}
}
