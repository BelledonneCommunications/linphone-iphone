/*
LinphoneCallParamsImpl.java
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
package org.linphone.core;

import org.linphone.core.LinphoneCore.MediaEncryption;

public class LinphoneCallParamsImpl implements LinphoneCallParams {
	protected final long nativePtr;
	
	public LinphoneCallParamsImpl(long nativePtr) {
		this.nativePtr = nativePtr;
	}

	private native void enableVideo(long nativePtr, boolean b);
	private native boolean getVideoEnabled(long nativePtr);
	private native void audioBandwidth(long nativePtr, int bw);
	private native void setMediaEncryption(long nativePtr, int menc);
	private native int getMediaEncryption(long nativePtr);
	private native long getUsedAudioCodec(long nativePtr);
	private native long getUsedVideoCodec(long nativePtr);
	private native void destroy(long nativePtr);
	private native void enableLowBandwidth(long nativePtr, boolean enable);
	private native boolean isLowBandwidthEnabled(long nativePtr);
	
	public boolean getVideoEnabled() {
		return getVideoEnabled(nativePtr);
	}

	public void setVideoEnabled(boolean b) {
		enableVideo(nativePtr, b);
	}
	
	@Override
	protected void finalize() throws Throwable {
		destroy(nativePtr);
		super.finalize();
	}

	public void setAudioBandwidth(int value) {
		audioBandwidth(nativePtr, value);
	}
	
	public MediaEncryption getMediaEncryption() {
		return MediaEncryption.fromInt(getMediaEncryption(nativePtr));
	}
	
	public void setMediaEnctyption(MediaEncryption menc) {
		setMediaEncryption(nativePtr, menc.mValue);
	}

	public PayloadType getUsedAudioCodec() {
		long ptr = getUsedAudioCodec(nativePtr);
		if (ptr == 0) return null;
		return new PayloadTypeImpl(ptr);
	}

	public PayloadType getUsedVideoCodec() {
		long ptr = getUsedVideoCodec(nativePtr);
		if (ptr == 0) return null;
		return new PayloadTypeImpl(ptr);
	}

	private native boolean localConferenceMode(long nativePtr);
	public boolean localConferenceMode() {
		return localConferenceMode(nativePtr);
	}
	
	public void enableLowBandwidth(boolean enable) {
		enableLowBandwidth(nativePtr, enable);
	}
	
	public boolean isLowBandwidthEnabled() {
		return isLowBandwidthEnabled(nativePtr);
	}

	private native void setRecordFile(long nativePtr, String path);
	@Override
	public void setRecordFile(String path) {
		setRecordFile(nativePtr,path);
	}

	private native void addCustomHeader(long nativePtr, String name, String value); 
	@Override
	public void addCustomHeader(String name, String value) {
		addCustomHeader(nativePtr,name,value);
	}

	private native String getCustomHeader(long nativePtr, String name);
	@Override
	public String getCustomHeader(String name) {
		return getCustomHeader(nativePtr,name);
	}
	
}
