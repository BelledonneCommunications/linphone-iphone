/*
PresenceNoteImpl.java
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

package org.linphone.core;

public class PresenceNoteImpl implements PresenceNote {
	private long mNativePtr;

	protected PresenceNoteImpl(long nativePtr) {
		mNativePtr = nativePtr;
	}

	private native long newPresenceNoteImpl(String content, String lang);
	protected PresenceNoteImpl(String content, String lang) {
		mNativePtr = newPresenceNoteImpl(content, lang);
	}

	private native void unref(long nativePtr);
	protected void finalize() {
		unref(mNativePtr);
	}

	private native String getContent(long nativePtr);
	@Override
	public String getContent() {
		return getContent(mNativePtr);
	}

	private native int setContent(long nativePtr, String content);
	@Override
	public int setContent(String content) {
		return setContent(mNativePtr, content);
	}

	private native String getLang(long nativePtr);
	@Override
	public String getLang() {
		return getLang(mNativePtr);
	}

	private native int setLang(long nativePtr, String lang);
	@Override
	public int setLang(String lang) {
		return setLang(mNativePtr, lang);
	}

	public long getNativePtr() {
		return mNativePtr;
	}
}
