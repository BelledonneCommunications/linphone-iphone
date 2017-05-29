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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
package org.linphone.core;

import org.linphone.core.LinphoneCore.MediaDirection;
import org.linphone.core.LinphoneCore.MediaEncryption;
import org.linphone.core.LinphoneCore.StreamType;

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

	private native void addCustomSdpAttribute(long nativePtr, String name, String value); 
	@Override
	public void addCustomSdpAttribute(String name, String value) {
		addCustomSdpAttribute(nativePtr, name, value);
	}

	private native void addCustomSdpMediaAttribute(long nativePtr, int type, String name, String value); 
	@Override
	public void addCustomSdpMediaAttribute(StreamType type, String name, String value) {
		addCustomSdpMediaAttribute(nativePtr, type.mValue, name, value);
	}

	private native String getCustomSdpAttribute(long nativePtr, String name);
	@Override
	public String getCustomSdpAttribute(String name) {
		return getCustomSdpAttribute(nativePtr, name);
	}

	private native String getCustomSdpMediaAttribute(long nativePtr, int type, String name);
	@Override
	public String getCustomSdpMediaAttribute(StreamType type, String name) {
		return getCustomSdpMediaAttribute(nativePtr, type.mValue, name);
	}

	private native void clearCustomSdpAttributes(long nativePtr);
	@Override
	public void clearCustomSdpAttributes() {
		clearCustomSdpAttributes(nativePtr);
	}

	private native void clearCustomSdpMediaAttributes(long nativePtr, int type);
	@Override
	public void clearCustomSdpMediaAttributes(StreamType type) {
		clearCustomSdpMediaAttributes(nativePtr, type.mValue);
	}

	private native void setPrivacy(long nativePtr, int mask);
	@Override
	public void setPrivacy(int privacy_mask) {
		setPrivacy(nativePtr,privacy_mask);
	}

	private native int getPrivacy(long nativePtr);
	@Override
	public int getPrivacy() {
		return getPrivacy(nativePtr);
	}

	private native void setSessionName(long nativePtr, String name);
	@Override
	public void setSessionName(String name) {
		setSessionName(nativePtr,name);
	}

	private native String getSessionName(long nativePtr);
	@Override
	public String getSessionName() {
		return getSessionName(nativePtr);
	}

	private native int[] getSentVideoSize(long nativePtr);
	@Override
	public VideoSize getSentVideoSize() {
		int[] nativeSize = getSentVideoSize(nativePtr);
		VideoSize vSize = new VideoSize();
		vSize.width = nativeSize[0];
		vSize.height = nativeSize[1];
		return vSize;
	}

	private native int[] getReceivedVideoSize(long nativePtr);
	@Override
	public VideoSize getReceivedVideoSize() {
		int[] nativeSize = getReceivedVideoSize(nativePtr);
		VideoSize vSize = new VideoSize();
		vSize.width = nativeSize[0];
		vSize.height = nativeSize[1];
		return vSize;
	}
	private native void enableAudioMulticast(long ptr,boolean yesno);
	@Override
	public void enableAudioMulticast(boolean yesno) {
		enableAudioMulticast(nativePtr,yesno);
	}
	private native boolean audioMulticastEnabled(long ptr);
	@Override
	public boolean audioMulticastEnabled() {
		return audioMulticastEnabled(nativePtr);
	}
	private native void enableVideoMulticast(long ptr,boolean yesno);
	
	@Override
	public void enableVideoMulticast(boolean yesno) {
		enableVideoMulticast(nativePtr,yesno);
	}
	private native boolean videoMulticastEnabled(long ptr);
	@Override
	public boolean videoMulticastEnabled() {
		return videoMulticastEnabled(nativePtr);
	}

	private native void enableRealTimeText(long nativePtr, boolean yesno);
	@Override
	public void enableRealTimeText(boolean yesno) {
		enableRealTimeText(nativePtr, yesno);
	}

	private native boolean realTimeTextEnabled(long nativePtr);
	@Override
	public boolean realTimeTextEnabled() {
		return realTimeTextEnabled(nativePtr);
	}

	private native int getAudioDirection(long nativePtr);
	@Override
	public MediaDirection getAudioDirection() {
		return MediaDirection.fromInt(getAudioDirection(nativePtr));
	}

	private native int getVideoDirection(long nativePtr);
	@Override
	public MediaDirection getVideoDirection() {
		return MediaDirection.fromInt(getVideoDirection(nativePtr));
	}

	private native void setAudioDirection(long nativePtr, int direction);
	@Override
	public void setAudioDirection(MediaDirection direction) {
		setAudioDirection(nativePtr, direction.mValue);
	}

	private native void setVideoDirection(long nativePtr, int direction);
	@Override
	public void setVideoDirection(MediaDirection direction) {
		setVideoDirection(nativePtr, direction.mValue);
	}
	
	private native float getSentFramerate(long nativePtr);
	@Override
	public float getSentFramerate() {
		return getSentFramerate(nativePtr);
	}
	
	private native float getReceivedFramerate(long nativePtr);
	@Override
	public float getReceivedFramerate() {
		return getReceivedFramerate(nativePtr);
	}
}
