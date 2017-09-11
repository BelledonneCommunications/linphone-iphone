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
	private float estimatedDownloadBandwidth;
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

	private native int getMediaType(long nativeStatsPtr);
	private native int getIceState(long nativeStatsPtr);
	private native float getDownloadBandwidth(long nativeStatsPtr);
	private native float getUploadBandwidth(long nativeStatsPtr);
	private native float getEstimatedDownloadBandwidth(long nativeStatsptr);
	private native float getSenderLossRate(long nativeStatsPtr);
	private native float getReceiverLossRate(long nativeStatsPtr);
	private native float getSenderInterarrivalJitter(long nativeStatsPtr);
	private native float getReceiverInterarrivalJitter(long nativeStatsPtr);
	private native float getRoundTripDelay(long nativeStatsPtr);
	private native long getLatePacketsCumulativeNumber(long nativeStatsPtr);
	private native float getJitterBufferSize(long nativeStatsPtr);
	private native float getLocalLossRate(long nativeStatsPtr);
	private native float getLocalLateRate(long nativeStatsPtr);
	private native void updateStats(long nativeCallPtr, int mediaType);
	private native int getIpFamilyOfRemote(long nativeStatsPtr);

	protected LinphoneCallStatsImpl(long nativeStatsPtr) {
		nativePtr = nativeStatsPtr;
		mediaType = getMediaType(nativeStatsPtr);
		iceState = getIceState(nativeStatsPtr);
		downloadBandwidth = getDownloadBandwidth(nativeStatsPtr);
		uploadBandwidth = getUploadBandwidth(nativeStatsPtr);
		estimatedDownloadBandwidth = getEstimatedDownloadBandwidth(nativeStatsPtr);
		senderLossRate = getSenderLossRate(nativeStatsPtr);
		receiverLossRate = getReceiverLossRate(nativeStatsPtr);
		senderInterarrivalJitter = getSenderInterarrivalJitter(nativeStatsPtr);
		receiverInterarrivalJitter = getReceiverInterarrivalJitter(nativeStatsPtr);
		roundTripDelay = getRoundTripDelay(nativeStatsPtr);
		latePacketsCumulativeNumber = getLatePacketsCumulativeNumber(nativeStatsPtr);
		jitterBufferSize = getJitterBufferSize(nativeStatsPtr);
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
	
	public float getEstimatedDownloadBandwidth() {
		return estimatedDownloadBandwidth;
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

	public int getIpFamilyOfRemote() {
		return getIpFamilyOfRemote(nativePtr);
	}
}
