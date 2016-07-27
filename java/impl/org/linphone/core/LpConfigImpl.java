/*
LPConfigImpl.java
Copyright (C) 2013  Belledonne Communications, Grenoble, France

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

class LpConfigImpl implements LpConfig {

	private long nativePtr;
	boolean ownPtr = false;
	
	public LpConfigImpl(long ptr) {
		nativePtr = ptr;
	}
	
	private native long newLpConfigImpl(String file);
	private native long newLpConfigImplFromBuffer(String buffer);
	private native void delete(long ptr);
	
	@Deprecated
	public LpConfigImpl(String file) {
		nativePtr = newLpConfigImpl(file);
		ownPtr = true;
	}
	
	private LpConfigImpl() {
		nativePtr = -1;
		ownPtr = false;
	}
	
	public static LpConfigImpl fromFile(String file) {
		LpConfigImpl impl = new LpConfigImpl();
		impl.nativePtr = impl.newLpConfigImpl(file);
		impl.ownPtr = true;
		return impl;
	}
	
	public static LpConfigImpl fromBuffer(String buffer) {
		LpConfigImpl impl = new LpConfigImpl();
		impl.nativePtr = impl.newLpConfigImplFromBuffer(buffer);
		impl.ownPtr = true;
		return impl;
	}
	
	protected void finalize() throws Throwable {
		if(ownPtr) {
			delete(nativePtr);
		}
	}

	private native void sync(long ptr);
	@Override
	public void sync() {
		sync(nativePtr);
	}

	private native void setInt(long ptr, String section, String key, int value);
	@Override
	public void setInt(String section, String key, int value) {
		setInt(nativePtr, section, key, value);
	}

	private native void setFloat(long ptr, String section, String key, float value);
	@Override
	public void setFloat(String section, String key, float value) {
		setFloat(nativePtr, section, key, value);
	}

	private native void setBool(long ptr, String section, String key, boolean value);
	@Override
	public void setBool(String section, String key, boolean value) {
		setBool(nativePtr, section, key, value);
	}

	private native void setString(long ptr, String section, String key, String value);
	@Override
	public void setString(String section, String key, String value) {
		setString(nativePtr, section, key, value);
	}

	private native void setIntRange(long ptr, String section, String key, int min, int max);
	@Override
	public void setIntRange(String section, String key, int min, int max) {
		setIntRange(nativePtr, section, key, min, max);
	}

	private native int getInt(long ptr, String section, String key, int defaultValue);
	@Override
	public int getInt(String section, String key, int defaultValue) {
		return getInt(nativePtr, section, key, defaultValue);
	}

	private native float getFloat(long ptr, String section, String key, float defaultValue);
	@Override
	public float getFloat(String section, String key, float defaultValue) {
		return getFloat(nativePtr, section, key, defaultValue);
	}

	private native boolean getBool(long ptr, String section, String key, boolean defaultValue);
	@Override
	public boolean getBool(String section, String key, boolean defaultValue) {
		return getBool(nativePtr, section, key, defaultValue);
	}

	private native String getString(long ptr, String section, String key, String defaultValue);
	@Override
	public String getString(String section, String key, String defaultValue) {
		return getString(nativePtr, section, key, defaultValue);
	}

	private native int[] getIntRange(long ptr, String section, String key, int defaultMin, int defaultMax);
	@Override
	public int[] getIntRange(String section, String key, int defaultMin, int defaultMax) {
		return getIntRange(nativePtr, section, key, defaultMin, defaultMax);
	}

}
