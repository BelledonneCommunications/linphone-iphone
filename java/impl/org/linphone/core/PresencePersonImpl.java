/*
PresencePersonImpl.java
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

public class PresencePersonImpl implements PresencePerson {
	private long mNativePtr;

	protected PresencePersonImpl(long nativePtr) {
		mNativePtr = nativePtr;
	}

	private native long newPresencePersonImpl(String id);
	protected PresencePersonImpl(String id) {
		mNativePtr = newPresencePersonImpl(id);
	}

	private native void unref(long nativePtr);
	protected void finalize() {
		unref(mNativePtr);
	}

	private native String getId(long nativePtr);
	@Override
	public String getId() {
		return getId(mNativePtr);
	}

	private native int setId(long nativePtr, String id);
	@Override
	public int setId(String id) {
		return setId(mNativePtr, id);
	}

	private native long getNbActivities(long nativePtr);
	@Override
	public long getNbActivities() {
		return getNbActivities(mNativePtr);
	}

	private native Object getNthActivity(long nativePtr, long idx);
	@Override
	public PresenceActivity getNthActivity(long idx) {
		return (PresenceActivity)getNthActivity(mNativePtr, idx);
	}

	private native int addActivity(long nativePtr, long activityPtr);
	@Override
	public int addActivity(PresenceActivity activity) {
		return addActivity(mNativePtr, activity.getNativePtr());
	}

	private native int clearActivities(long nativePtr);
	@Override
	public int clearActivities() {
		return clearActivities(mNativePtr);
	}

	private native long getNbNotes(long nativePtr);
	@Override
	public long getNbNotes() {
		return getNbNotes(mNativePtr);
	}

	private native Object getNthNote(long nativePtr, long idx);
	@Override
	public PresenceNote getNthNote(long idx) {
		return (PresenceNote)getNthNote(mNativePtr, idx);
	}

	private native int addNote(long nativePtr, long notePtr);
	@Override
	public int addNote(PresenceNote note) {
		return addNote(mNativePtr, note.getNativePtr());
	}

	private native int clearNotes(long nativePtr);
	@Override
	public int clearNotes() {
		return clearNotes(mNativePtr);
	}

	private native long getNbActivitiesNotes(long nativePtr);
	@Override
	public long getNbActivitiesNotes() {
		return getNbActivitiesNotes(mNativePtr);
	}

	private native Object getNthActivitiesNote(long nativePtr, long idx);
	@Override
	public PresenceNote getNthActivitiesNote(long idx) {
		return (PresenceNote)getNthActivitiesNote(mNativePtr, idx);
	}

	private native int addActivitiesNote(long nativePtr, long notePtr);
	@Override
	public int addActivitiesNote(PresenceNote note) {
		return addActivitiesNote(mNativePtr, note.getNativePtr());
	}

	private native int clearActivitesNotes(long nativePtr);
	@Override
	public int clearActivitesNotes() {
		return clearActivitesNotes(mNativePtr);
	}

	public long getNativePtr() {
		return mNativePtr;
	}
}
