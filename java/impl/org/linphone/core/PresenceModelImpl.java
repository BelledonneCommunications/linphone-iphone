/*
PresenceModelImpl.java
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

public class PresenceModelImpl implements PresenceModel {
	private long mNativePtr;

	protected PresenceModelImpl(long nativePtr) {
		mNativePtr = nativePtr;
	}

	private native void unref(long nativePtr);
	protected void finalize() {
		unref(mNativePtr);
	}

	public long getNativePtr() {
		return mNativePtr;
	}

	private native int getBasicStatus(long nativePtr);
	@Override
	public PresenceBasicStatus getBasicStatus() {
		return PresenceBasicStatus.fromInt(getBasicStatus(mNativePtr));
	}

	private native int setBasicStatus(long nativePtr, int basic_status);
	@Override
	public int setBasicStatus(PresenceBasicStatus basic_status) {
		return setBasicStatus(mNativePtr, basic_status.toInt());
	}

	private native long getTimestamp(long nativePtr);
	@Override
	public long getTimestamp() {
		return getTimestamp(mNativePtr);
	}

	private native String getContact(long nativePtr);
	@Override
	public String getContact() {
		return getContact(mNativePtr);
	}

	private native void setContact(long nativePtr, String contact);
	@Override
	public void setContact(String contact) {
		setContact(mNativePtr, contact);
	}

	private native long nbActivities(long nativePtr);
	@Override
	public long nbActivities() {
		return nbActivities(mNativePtr);
	}

	private native Object getNthActivity(long nativePtr, long idx);
	@Override
	public PresenceActivity getNthActivity(long idx) {
		return (PresenceActivity)getNthActivity(mNativePtr, idx);
	}

	private native Object getActivity(long nativePtr);
	@Override
	public PresenceActivity getActivity() {
		return (PresenceActivity)getActivity(mNativePtr);
	}

	private native int setActivity(long nativePtr, int activity, String description);
	@Override
	public int setActivity(PresenceActivityType activity, String description) {
		return setActivity(mNativePtr, activity.toInt(), description);
	}

	private native int addActivity(long nativePtr, int activity, String description);
	@Override
	public int addActivity(PresenceActivityType activity, String description) {
		return addActivity(mNativePtr, activity.toInt(), description);
	}

	private native int clearActivities(long nativePtr);
	@Override
	public int clearActivities() {
		return clearActivities(mNativePtr);
	}

	private native Object getNote(long nativePtr, String lang);
	@Override
	public PresenceNote getNote(String lang) {
		return (PresenceNote)getNote(mNativePtr, lang);
	}

	private native int addNote(long nativePtr, String note_content, String lang);
	@Override
	public int addNote(String note_content, String lang) {
		return addNote(mNativePtr, note_content, lang);
	}

	private native int clearNotes(long nativePtr);
	@Override
	public int clearNotes() {
		return clearNotes(mNativePtr);
	}
}
