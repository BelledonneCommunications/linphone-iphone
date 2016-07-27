/*
LinphoneConferenceParamsImpl.java
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

public class LinphoneConferenceParamsImpl implements LinphoneConferenceParams {
	private long nativePtr = 0;
	
	private native long createInstance(LinphoneCoreImpl core);
	public LinphoneConferenceParamsImpl(LinphoneCore core) {
		this.nativePtr = createInstance((LinphoneCoreImpl)core);
	}

	private native long copyInstance(long paramsPtr);
	public LinphoneConferenceParamsImpl(LinphoneConferenceParamsImpl params) {
		nativePtr = copyInstance(params.nativePtr);
	}

	private native void destroyInstance(long nativePtr);
	public void finalize() {
		destroyInstance(this.nativePtr);
	}

	private native void enableVideo(long paramsPtr, boolean enable);
	@Override
	public void enableVideo(boolean enable) {
		enableVideo(this.nativePtr, enable);
	}

	private native boolean isVideoRequested(long paramsPtr);
	@Override
	public boolean isVideoRequested() {
		return isVideoRequested(this.nativePtr);
	}
}