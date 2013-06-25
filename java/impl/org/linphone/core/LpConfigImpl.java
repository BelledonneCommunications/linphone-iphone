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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
package org.linphone.core;



class LpConfigImpl implements LpConfig {

	private final long nativePtr;
	boolean ownPtr = false;
	
	public LpConfigImpl(long ptr) {
		nativePtr=ptr;
	}
	
	private native long newLpConfigImpl(String file);
	private native void delete(long ptr);
	public LpConfigImpl(String file) {
		nativePtr = newLpConfigImpl(file);
		ownPtr = true;
	}
	protected void finalize() throws Throwable {
		if(ownPtr) {
			delete(nativePtr);
		}
	}

	private native void sync(long ptr);
	public void sync() {
		sync(nativePtr);
	}

	private native void setInt(long ptr, String section, String key, int value);
	public void setInt(String section, String key, int value) {
		setInt(nativePtr, section, key, value);
	}

}
