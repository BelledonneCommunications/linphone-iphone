/*
LinphoneCallImpl.java
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

class LinphoneCallImpl implements LinphoneCall {
 
	protected final long nativePtr;
	boolean ownPtr = false;
	Object userData;
	private LinphoneCallStats audioStats;
	private LinphoneCallStats videoStats;

	native private void finalize(long nativePtr);
	native private long  getCallLog(long nativePtr);
	private native boolean isIncoming(long nativePtr);
	native private long getRemoteAddress(long nativePtr);
	native private int getState(long nativePtr);
	private native long getCurrentParamsCopy(long nativePtr);
	private native long getRemoteParams(long nativePtr);
	private native void enableCamera(long nativePtr, boolean enabled);
	private native boolean cameraEnabled(long nativePtr);
	private native void enableEchoCancellation(long nativePtr,boolean enable);
	private native boolean isEchoCancellationEnabled(long nativePtr) ;
	private native void enableEchoLimiter(long nativePtr,boolean enable);
	private native boolean isEchoLimiterEnabled(long nativePtr);
	private native Object getReplacedCall(long nativePtr);
	private native int getDuration(long nativePtr);
	private native float getCurrentQuality(long nativePtr);
	private native float getAverageQuality(long nativePtr);
	private native boolean mediaInProgress(long nativePtr);
	private native void setListener(long ptr, LinphoneCallListener listener);
	native private long getDiversionAddress(long nativePtr);
	
	/*
	 * This method must always be called from JNI, nothing else.
	 */
	private LinphoneCallImpl(long aNativePtr)  {
		nativePtr = aNativePtr;
	}
	protected void finalize() throws Throwable {
		finalize(nativePtr);
	}
	public LinphoneCallLog getCallLog() {
		long lNativePtr = getCallLog(nativePtr);
		if (lNativePtr!=0) {
			return new LinphoneCallLogImpl(lNativePtr); 
		} else {
			return null;
		}
	}
	public void setAudioStats(LinphoneCallStats stats) {
		audioStats = stats;
	}
	public void setVideoStats(LinphoneCallStats stats) {
		videoStats = stats;
	}
	public LinphoneCallStats getAudioStats() {
		if (audioStats!=null) ((LinphoneCallStatsImpl)audioStats).updateRealTimeStats(this);
		return audioStats;
	}
	public LinphoneCallStats getVideoStats() {
		if (videoStats!=null) ((LinphoneCallStatsImpl)videoStats).updateRealTimeStats(this);
		return videoStats;
	}
	public CallDirection getDirection() {
		return isIncoming(nativePtr)?CallDirection.Incoming:CallDirection.Outgoing;
	}
	public LinphoneAddress getRemoteAddress() {
		long lNativePtr = getRemoteAddress(nativePtr);
		if (lNativePtr!=0) {
			return new LinphoneAddressImpl(lNativePtr,LinphoneAddressImpl.WrapMode.FromConst); 
		} else {
			return null;
		}
	}
	public State getState() {
		return LinphoneCall.State.fromInt(getState(nativePtr));
	}
	public LinphoneCallParams getCurrentParamsCopy() {
		return new LinphoneCallParamsImpl(getCurrentParamsCopy(nativePtr));
	}
	public LinphoneCallParams getRemoteParams() {
		long remoteParamsPtr = getRemoteParams(nativePtr);
		if (remoteParamsPtr == 0) {
			return null;
		}
		return new LinphoneCallParamsImpl(remoteParamsPtr);
	}
	public void enableCamera(boolean enabled) {
		enableCamera(nativePtr, enabled);
	}
	public boolean cameraEnabled() {
		return cameraEnabled(nativePtr);
	}

	@Override
	public boolean equals(Object call) {
		if (this == call) return true;
		if (call == null) return false;
		if (!(call instanceof LinphoneCallImpl)) return false;
		return nativePtr == ((LinphoneCallImpl)call).nativePtr;
	}

	@Override
	public int hashCode() {
		int result = 17;
		result = 31 * result + (int) (nativePtr ^ (nativePtr >>> 32));
		return result;
	}
	public void enableEchoCancellation(boolean enable) {
		enableEchoCancellation(nativePtr,enable);
		
	}
	public boolean isEchoCancellationEnabled() {
		return isEchoCancellationEnabled(nativePtr);
	}
	public void enableEchoLimiter(boolean enable) {
		enableEchoLimiter(nativePtr,enable);
	}
	public boolean isEchoLimiterEnabled() {
		return isEchoLimiterEnabled(nativePtr);
	}
	public LinphoneCall getReplacedCall(){
		return (LinphoneCall)getReplacedCall(nativePtr);
	}

	public int getDuration() {
		return getDuration(nativePtr);
	}
	public float getAverageQuality() {
		return getAverageQuality(nativePtr);
	}
	public float getCurrentQuality() {
		return getCurrentQuality(nativePtr);
	}

	private native String getAuthenticationToken(long nativePtr);
	public String getAuthenticationToken(){
		return getAuthenticationToken(nativePtr);
	}

	private native boolean isAuthenticationTokenVerified(long nativePtr);
	public boolean isAuthenticationTokenVerified(){
		return isAuthenticationTokenVerified(nativePtr);
	}

	private native void setAuthenticationTokenVerified(long nativePtr, boolean verified);
	public void setAuthenticationTokenVerified(boolean verified){
		setAuthenticationTokenVerified(nativePtr, verified);
	}

	public boolean isInConference() {
		return getConference() != null;
	}
	public native LinphoneConference getConference(long nativePtr);
	public LinphoneConference getConference() {
		return getConference(nativePtr);
	}

	public boolean mediaInProgress() { return mediaInProgress(nativePtr);}

	@Override
	public String toString() {
		return "Call " + nativePtr;
	}

	private native float getPlayVolume(long nativePtr);
	public float getPlayVolume() {
		return getPlayVolume(nativePtr);
	}

	private native String getRemoteUserAgent(long nativePtr);
	public String getRemoteUserAgent() {
		return getRemoteUserAgent(nativePtr);
	}

	private native String getRemoteContact(long nativePtr);
	public String getRemoteContact() {
		return getRemoteContact(nativePtr);
	}
	
	private native void takeSnapshot(long nativePtr, String path);
	public void takeSnapshot(String path) {
		takeSnapshot(nativePtr, path);
	}

	private native void zoomVideo(long nativePtr, float factor, float cx, float cy);
	public void zoomVideo(float factor, float cx, float cy) {
		zoomVideo(nativePtr, factor, cx, cy);
	}
	
	private native void startRecording(long nativePtr);
	@Override
	public void startRecording() {
		startRecording(nativePtr);
	}
	private native void stopRecording(long nativePtr);
	@Override
	public void stopRecording() {
		stopRecording(nativePtr);
	}
	private native int getTransferState(long nativePtr);
	@Override
	public State getTransferState() {
		return State.fromInt(getTransferState(nativePtr));
	}
	private native int sendInfoMessage(long callPtr, long msgptr);
	@Override
	public void sendInfoMessage(LinphoneInfoMessage msg) {
		sendInfoMessage(nativePtr,((LinphoneInfoMessageImpl)msg).nativePtr);
	}
	private native Object getTransfererCall(long callPtr); 
	@Override
	public LinphoneCall getTransfererCall() {
		return (LinphoneCall)getTransfererCall(nativePtr);
	}
	private native Object getTransferTargetCall(long callPtr);
	@Override
	public LinphoneCall getTransferTargetCall() {
		return (LinphoneCall)getTransferTargetCall(nativePtr);
	}
	@Override
	public Reason getReason() {
		// TODO Auto-generated method stub
		return null;
	}
	private native long getErrorInfo(long nativePtr);
	@Override
	public ErrorInfo getErrorInfo() {
		return new ErrorInfoImpl(getErrorInfo(nativePtr));
	}
	@Override
	public void setUserData(Object obj) {
		userData = obj;
	}
	@Override
	public Object getUserData() {
		return userData;
	}
	
	private native long getPlayer(long callPtr);
	@Override
	public LinphonePlayer getPlayer() {
		return new LinphonePlayerImpl(getPlayer(nativePtr));
	}
	
	private native Object getChatRoom(long nativePtr);
	@Override
	public LinphoneChatRoom getChatRoom() {
		return (LinphoneChatRoom)(getChatRoom(nativePtr));
	}

	@Override
	public void setListener(LinphoneCallListener listener) {
		setListener(nativePtr, listener);
	}
    
	public LinphoneAddress getDiversionAddress() {
        long lNativePtr = getDiversionAddress(nativePtr);
        if (lNativePtr!=0) {
            return new LinphoneAddressImpl(lNativePtr,LinphoneAddressImpl.WrapMode.FromConst);
        } else {
            return null;
        }
    }

}
