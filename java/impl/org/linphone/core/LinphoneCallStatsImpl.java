/*
LinPhoneCallStatsImpl.java
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


class LinphoneCallStatsImpl implements LinphoneCallStats {
	private int mediaType;
	private int iceState;
	private float downloadBandwidth;
	private float uploadBandwidth;
	private float senderLossRate;
	private float receiverLossRate;
	private float senderInterarrivalJitter;
	private float receiverInterarrivalJitter;
	private float roundTripDelay;
	private long latePacketsCumulativeNumber;
	private float jitterBufferSize;
	private float localLossRate;
	private float localLateRate;
	private long nativePtr;
	private long nativeCPtr;

	private native int getMediaType(long nativeStatsPtr);
	private native int getIceState(long nativeStatsPtr);
	private native float getDownloadBandwidth(long nativeStatsPtr);
	private native float getUploadBandwidth(long nativeStatsPtr);
	private native float getSenderLossRate(long nativeStatsPtr);
	private native float getReceiverLossRate(long nativeStatsPtr);
	private native float getSenderInterarrivalJitter(long nativeStatsPtr, long nativeCallPtr);
	private native float getReceiverInterarrivalJitter(long nativeStatsPtr, long nativeCallPtr);
	private native float getRoundTripDelay(long nativeStatsPtr);
	private native long getLatePacketsCumulativeNumber(long nativeStatsPtr, long nativeCallPtr);
	private native float getJitterBufferSize(long nativeStatsPtr);
	private native float getLocalLossRate(long nativeStatsPtr);
	private native float getLocalLateRate(long nativeStatsPtr);
	private native String getEncoderName(long nativeStatsPtr, long nativeCallPtr, long payloadPtr);
	private native String getDecoderName(long nativeStatsPtr, long nativeCallPtr, long payloadPtr);
	private native void updateStats(long nativeCallPtr, int mediaType);
	private native int getIpFamilyOfRemote(long nativeStatsPtr);

	protected LinphoneCallStatsImpl(long nativeCallPtr, long nativeStatsPtr) {
		nativePtr = nativeStatsPtr;
		nativeCPtr = nativeCallPtr;
		mediaType = getMediaType(nativeStatsPtr);
		iceState = getIceState(nativeStatsPtr);
		downloadBandwidth = getDownloadBandwidth(nativeStatsPtr);
		uploadBandwidth = getUploadBandwidth(nativeStatsPtr);
		senderLossRate = getSenderLossRate(nativeStatsPtr);
		receiverLossRate = getReceiverLossRate(nativeStatsPtr);
		senderInterarrivalJitter = getSenderInterarrivalJitter(nativeStatsPtr, nativeCallPtr);
		receiverInterarrivalJitter = getReceiverInterarrivalJitter(nativeStatsPtr, nativeCallPtr);
		roundTripDelay = getRoundTripDelay(nativeStatsPtr);
		latePacketsCumulativeNumber = getLatePacketsCumulativeNumber(nativeStatsPtr, nativeCallPtr);
		jitterBufferSize = getJitterBufferSize(nativeStatsPtr);

	}

	protected void updateRealTimeStats(LinphoneCall call){
		updateStats( ((LinphoneCallImpl)call).nativePtr, mediaType);
		localLossRate=getLocalLossRate(nativePtr);
		localLateRate=getLocalLateRate(nativePtr);
	}

	public MediaType getMediaType() {
		return MediaType.fromInt(mediaType);
	}

	public IceState getIceState() {
		return IceState.fromInt(iceState);
	}

	public float getDownloadBandwidth() {
		return downloadBandwidth;
	}

	public float getUploadBandwidth() {
		return uploadBandwidth;
	}

	public float getSenderLossRate() {
		return senderLossRate;
	}

	public float getReceiverLossRate() {
		return receiverLossRate;
	}

	public float getSenderInterarrivalJitter() {
		return senderInterarrivalJitter;
	}

	public float getReceiverInterarrivalJitter() {
		return receiverInterarrivalJitter;
	}

	public float getRoundTripDelay() {
		return roundTripDelay;
	}

	public long getLatePacketsCumulativeNumber() {
		return latePacketsCumulativeNumber;
	}

	public float getJitterBufferSize() {
		return jitterBufferSize;
	}

	public float getLocalLossRate(){
		return localLossRate;
	}

	public float getLocalLateRate(){
		return localLateRate;
	}

	public String getEncoderName(PayloadType pl) {
		if (pl == null)
			return "";
		return getEncoderName(nativePtr, nativeCPtr, ((PayloadTypeImpl)pl).nativePtr);
	}

	public String getDecoderName(PayloadType pl) {
		if (pl == null)
			return "";
		return getDecoderName(nativePtr, nativeCPtr, ((PayloadTypeImpl)pl).nativePtr);
	}

	public int getIpFamilyOfRemote() {
		return getIpFamilyOfRemote(nativePtr);
	}
}
