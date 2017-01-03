/*
LinphoneCoreImpl.java
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

import static android.media.AudioManager.MODE_IN_CALL;

import java.io.File;
import java.io.IOException;

import org.linphone.core.LinphoneCall.State;
import org.linphone.core.LinphoneCoreListener;
import org.linphone.core.LinphoneProxyConfigImpl;

import org.linphone.mediastream.Log;
import org.linphone.mediastream.Version;
import org.linphone.mediastream.video.AndroidVideoWindowImpl;
import org.linphone.mediastream.video.capture.hwconf.Hacks;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.media.AudioManager;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.net.wifi.WifiManager.WifiLock;

class LinphoneCoreImpl implements LinphoneCore {

	private final  LinphoneCoreListener mListener; //to make sure to keep a reference on this object
	protected long nativePtr = 0;
	private Context mContext = null;
	private AudioManager mAudioManager = null;
	private boolean openh264DownloadEnabled = false;
	private boolean mSpeakerEnabled = false;
	private native long newLinphoneCore(LinphoneCoreListener listener,String userConfig,String factoryConfig,Object  userdata);
	private native void iterate(long nativePtr);
	private native LinphoneProxyConfig getDefaultProxyConfig(long nativePtr);

	private native void setDefaultProxyConfig(long nativePtr,long proxyCfgNativePtr);
	private native int addProxyConfig(LinphoneProxyConfig jprtoxyCfg,long nativePtr,long proxyCfgNativePtr);
	private native void removeProxyConfig(long nativePtr, long proxyCfg);
	private native void clearAuthInfos(long nativePtr);

	private native void clearProxyConfigs(long nativePtr);
	private native void addAuthInfo(long nativePtr,long authInfoNativePtr);
	private native void removeAuthInfo(long nativePtr, long authInfoNativePtr);
	private native Object invite(long nativePtr,String uri);
	private native void terminateCall(long nativePtr, long call);
	private native long getRemoteAddress(long nativePtr);
	private native boolean  isInCall(long nativePtr);
	private native boolean isInComingInvitePending(long nativePtr);
	private native void acceptCall(long nativePtr, long call);
	private native long getCallLog(long nativePtr,int position);
	native private long[] getCallLogs(long nativePtr);
	private native int getNumberOfCallLogs(long nativePtr);
	private native long getLastOutgoingCallLog(long nativePtr);
	private native void delete(long nativePtr);
	private native void setNetworkStateReachable(long nativePtr,boolean isReachable);
	private native boolean isNetworkStateReachable(long nativePtr);
	private native void setPlaybackGain(long nativeptr, float gain);
	private native float getPlaybackGain(long nativeptr);
	private native void muteMic(long nativePtr,boolean isMuted);
	private native long interpretUrl(long nativePtr,String destination);
	private native Object inviteAddress(long nativePtr,long to);
	private native Object inviteAddressWithParams(long nativePtrLc,long to, long nativePtrParam);
	private native int sendDtmfs(long nativePtr,String dtmf);
	private native void sendDtmf(long nativePtr,char dtmf);
	private native void clearCallLogs(long nativePtr);
	private native boolean isMicMuted(long nativePtr);
	private native long findPayloadType(long nativePtr, String mime, int clockRate, int channels);
	private native int enablePayloadType(long nativePtr, long payloadType,	boolean enable);
	private native boolean isPayloadTypeEnabled(long nativePtr, long payloadType);
	private native boolean payloadTypeIsVbr(long nativePtr, long payloadType);
	private native void enableAdaptiveRateControl(long nativePtr,boolean enable);
	private native boolean isAdaptiveRateControlEnabled(long nativePtr);
	private native String getAdaptiveRateAlgorithm(long nativePtr);
	private native void setAdaptiveRateAlgorithm(long nativePtr, String alg);
	private native void enableEchoCancellation(long nativePtr,boolean enable);
	private native boolean isEchoCancellationEnabled(long nativePtr);
	private native Object getCurrentCall(long nativePtr) ;
	private native void playDtmf(long nativePtr,char dtmf,int duration);
	private native void stopDtmf(long nativePtr);
	private native void setVideoWindowId(long nativePtr, Object wid);
	private native void setPreviewWindowId(long nativePtr, Object wid);
	private native void setDeviceRotation(long nativePtr, int rotation);
	private native void addFriend(long nativePtr,long friend);
	private native void addFriendList(long nativePtr,long friendList);
	private native void removeFriendList(long nativePtr,long friendList);
	private native LinphoneFriend[] getFriendList(long nativePtr);
	private native void setPresenceInfo(long nativePtr, int minutes_away, String alternative_contact, int status);
	private native int getPresenceInfo(long nativePtr);
	private native void setPresenceModel(long nativePtr, long presencePtr);
	private native Object getPresenceModel(long nativePtr);
	private native Object getOrCreateChatRoom(long nativePtr,String to);
	private native Object getChatRoom(long nativePtr,long to);
	private native void enableVideo(long nativePtr,boolean vcap_enabled,boolean display_enabled);
	private native boolean isVideoEnabled(long nativePtr);
	private native boolean isVideoSupported(long nativePtr);
	private native boolean isVCardSupported(long nativePtr);
	private native void setFirewallPolicy(long nativePtr, int enum_value);
	private native int getFirewallPolicy(long nativePtr);
	private native Object createNatPolicy(long nativePtr);
	private native void setNatPolicy(long nativePtr, long policyPtr);
	private native Object getNatPolicy(long nativePtr);
	private native void setStunServer(long nativePtr, String stun_server);
	private native String getStunServer(long nativePtr);
	private native int updateCall(long ptrLc, long ptrCall, long ptrParams);
	private native int getUploadBandwidth(long nativePtr);
	private native void setUploadBandwidth(long nativePtr, int bw);
	private native int getDownloadBandwidth(long nativePtr);
	private native void setDownloadBandwidth(long nativePtr, int bw);
	private native void setPreferredVideoSize(long nativePtr, int width, int heigth);
	private native void setPreferredVideoSizeByName(long nativePtr, String name);
	private native int[] getPreferredVideoSize(long nativePtr);
	private native void setRing(long nativePtr, String path);
	private native String getRing(long nativePtr);
	private native void setRootCA(long nativePtr, String path);
	private native void setRootCAData(long nativePtr, String data);
	private native void setRingback(long nativePtr, String path);
	private native long[] listVideoPayloadTypes(long nativePtr);
	private native void setVideoCodecs(long nativePtr, long[] codecs);
	private native LinphoneProxyConfig[] getProxyConfigList(long nativePtr);
	private native long[] getAuthInfosList(long nativePtr);
	private native long findAuthInfos(long nativePtr, String username, String realm, String domain);
	private native long[] listAudioPayloadTypes(long nativePtr);
	private native void setAudioCodecs(long nativePtr, long[] codecs);
	private native void enableKeepAlive(long nativePtr,boolean enable);
	private native boolean isKeepAliveEnabled(long nativePtr);
	private native int startEchoCalibration(long nativePtr,Object data);
	private native int startEchoTester(long nativePtr, int rate);
	private native int stopEchoTester(long nativePtr);
	private native int getSignalingTransportPort(long nativePtr, int code);
	private native void setSignalingTransportPorts(long nativePtr, int udp, int tcp, int tls);
	private native void enableIpv6(long nativePtr,boolean enable);
	private native boolean isIpv6Enabled(long nativePtr);
	private native int pauseCall(long nativePtr, long callPtr);
	private native int pauseAllCalls(long nativePtr);
	private native int resumeCall(long nativePtr, long callPtr);
	private native void setUploadPtime(long nativePtr, int ptime);
	private native void setDownloadPtime(long nativePtr, int ptime);
	private native void setZrtpSecretsCache(long nativePtr, String file);
	private native void enableEchoLimiter(long nativePtr2, boolean val);
	private native int setVideoDevice(long nativePtr2, int id);
	private native int getVideoDevice(long nativePtr2);
	private native int getMediaEncryption(long nativePtr);
	private native void setMediaEncryption(long nativePtr, int menc);
	private native boolean isMediaEncryptionMandatory(long nativePtr);
	private native void setMediaEncryptionMandatory(long nativePtr, boolean yesno);
	private native void removeCallLog(long nativePtr, long callLogPtr);
	private native int getMissedCallsCount(long nativePtr);
	private native void resetMissedCallsCount(long nativePtr);
	private native String getVersion(long nativePtr);
	private native void setAudioPort(long nativePtr, int port);
	private native void setVideoPort(long nativePtr, int port);
	private native void setAudioPortRange(long nativePtr, int minPort, int maxPort);
	private native void setVideoPortRange(long nativePtr, int minPort, int maxPort);
	private native void setIncomingTimeout(long nativePtr, int timeout);
	private native void setInCallTimeout(long nativePtr, int timeout);
	private native void setPrimaryContact2(long nativePtr, String contact);
	private native String getPrimaryContact(long nativePtr);
	private native void setPrimaryContact(long nativePtr, String displayName, String username);
	private native String getPrimaryContactUsername(long nativePtr);
	private native String getPrimaryContactDisplayName(long nativePtr);
	private native void setChatDatabasePath(long nativePtr, String path);
	private native void setCallLogsDatabasePath(long nativePtr, String path);
	private native void setFriendsDatabasePath(long nativePtr, String path);
	private native Object[] getChatRooms(long nativePtr);
	private native int migrateToMultiTransport(long nativePtr);
	private native void migrateCallLogs(long nativePtr);
	private native void setCallErrorTone(long nativePtr, int reason, String path);
	private native void enableSdp200Ack(long nativePtr,boolean enable);
	private native boolean isSdp200AckEnabled(long nativePtr);
	private native void stopRinging(long nativePtr);
	private native static void setAndroidPowerManager(Object pm);
	private native void setAndroidWifiLock(long nativePtr,Object wifi_lock);
	private native void setAndroidMulticastLock(long nativePtr,Object multicast_lock);
	private native void reloadMsPlugins(long nativePtr, String path);
	private native void reloadSoundDevices(long nativePtr);
	private native void setDefaultSoundDevices(long nativePtr);
	private native Object createFriend(long nativePtr);
	private native Object createFriendWithAddress(long nativePtr, String address);

	LinphoneCoreImpl(LinphoneCoreListener listener, File userConfig, File factoryConfig, Object userdata) throws IOException {
		mListener = listener;
		String user = userConfig == null ? null : userConfig.getCanonicalPath();
		String factory = factoryConfig == null ? null : factoryConfig.getCanonicalPath();
		nativePtr = newLinphoneCore(listener, user, factory, userdata);
	}
	LinphoneCoreImpl(LinphoneCoreListener listener) throws IOException {
		mListener = listener;
		nativePtr = newLinphoneCore(listener,null,null,null);
	}

	protected void finalize() throws Throwable {
		if (nativePtr!=0) destroy();
	}

	private boolean contextInitialized() {
		if (mContext == null) {
			Log.e("Context of LinphoneCore has not been initialized, call setContext() after creating LinphoneCore.");
			return false;
		}
		return true;
	}
	public void setContext(Object context) {
		mContext = (Context)context;
		ApplicationInfo info = mContext.getApplicationInfo();
		reloadMsPlugins(info.nativeLibraryDir);
		mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
		setAndroidPowerManager(mContext.getSystemService(Context.POWER_SERVICE));
		if (Version.sdkAboveOrEqual(Version.API12_HONEYCOMB_MR1_31X)) {
			WifiManager wifiManager=(WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
			WifiLock lock = wifiManager.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "linphonecore ["+ nativePtr+"] wifi-lock");
			lock.setReferenceCounted(true);
			setAndroidWifiLock(nativePtr,lock);
		}
		if (Version.sdkAboveOrEqual(Version.API14_ICE_CREAM_SANDWICH_40)) {
			WifiManager wifiManager=(WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
			MulticastLock lock = wifiManager.createMulticastLock("linphonecore ["+ nativePtr+"] multicast-lock");
			lock.setReferenceCounted(true);
			setAndroidMulticastLock(nativePtr, lock);
		}
	}

	public Context getContext() {
		return mContext;
	}

	public synchronized void addAuthInfo(LinphoneAuthInfo info) {
		isValid();
		addAuthInfo(nativePtr,((LinphoneAuthInfoImpl)info).nativePtr);
	}

	public synchronized void removeAuthInfo(LinphoneAuthInfo info) {
		isValid();
		removeAuthInfo(nativePtr, ((LinphoneAuthInfoImpl) info).nativePtr);
	}

	public synchronized LinphoneProxyConfig getDefaultProxyConfig() {
		isValid();
		return getDefaultProxyConfig(nativePtr);
	}

	public synchronized LinphoneCall invite(String uri) {
		isValid();
		return (LinphoneCall)invite(nativePtr,uri);
	}

	public synchronized void iterate() {
		isValid();
		iterate(nativePtr);
	}

	public synchronized void setDefaultProxyConfig(LinphoneProxyConfig proxyCfg) {
		isValid();
		long proxyPtr=proxyCfg != null ? ((LinphoneProxyConfigImpl)proxyCfg).nativePtr : 0;
		setDefaultProxyConfig(nativePtr, proxyPtr);
	}
	public synchronized void addProxyConfig(LinphoneProxyConfig proxyCfg) throws LinphoneCoreException{
		isValid();
		if (addProxyConfig(proxyCfg,nativePtr,((LinphoneProxyConfigImpl)proxyCfg).nativePtr) !=0) {
			throw new LinphoneCoreException("bad proxy config");
		}
		((LinphoneProxyConfigImpl)proxyCfg).mCore=this;
	}
	public synchronized void removeProxyConfig(LinphoneProxyConfig proxyCfg) {
		isValid();
		if (proxyCfg != null) {
			removeProxyConfig(nativePtr, ((LinphoneProxyConfigImpl) proxyCfg).nativePtr);
		}
	}
	public synchronized void clearAuthInfos() {
		isValid();
		clearAuthInfos(nativePtr);
	}
	public synchronized void clearProxyConfigs() {
		isValid();
		clearProxyConfigs(nativePtr);
	}
	public synchronized void terminateCall(LinphoneCall aCall) {
		isValid();
		if (aCall!=null)terminateCall(nativePtr,((LinphoneCallImpl)aCall).nativePtr);
	}
	public synchronized LinphoneAddress getRemoteAddress() {
		isValid();
		long ptr = getRemoteAddress(nativePtr);
		if (ptr==0) {
			return null;
		} else {
			return new LinphoneAddressImpl(ptr,LinphoneAddressImpl.WrapMode.FromConst);
		}
	}
	public synchronized  boolean isIncall() {
		isValid();
		return isInCall(nativePtr);
	}
	public synchronized boolean isInComingInvitePending() {
		isValid();
		return isInComingInvitePending(nativePtr);
	}
	public synchronized void acceptCall(LinphoneCall aCall) {
		isValid();
		acceptCall(nativePtr, ((LinphoneCallImpl) aCall).nativePtr);
	}
	public synchronized LinphoneCallLog[] getCallLogs() {
		long[] typesPtr = getCallLogs(nativePtr);
		if (typesPtr == null) return null;
 		isValid();
		LinphoneCallLog[] logs = new LinphoneCallLog[typesPtr.length];
		for (int i=0;i < logs.length; i++) {
			logs[i] = new LinphoneCallLogImpl(typesPtr[i]);
		}
		return logs;
	}
	public synchronized LinphoneCallLog getLastOutgoingCallLog(){
		isValid();
		long callLog = getLastOutgoingCallLog(nativePtr);
		return new LinphoneCallLogImpl(callLog);
	}
	public synchronized void destroy() {
		setAndroidPowerManager(null);
		delete(nativePtr);
		nativePtr=0;
	}

	private void isValid() {
		if (nativePtr == 0) {
			throw new RuntimeException("object already destroyed");
		}
	}
	public synchronized void setNetworkReachable(boolean isReachable) {
		setNetworkStateReachable(nativePtr, isReachable);
	}
	public synchronized void setPlaybackGain(float gain) {
		setPlaybackGain(nativePtr, gain);

	}
	public synchronized float getPlaybackGain() {
		return getPlaybackGain(nativePtr);
	}
	public synchronized void muteMic(boolean isMuted) {
		muteMic(nativePtr, isMuted);
	}

	public synchronized LinphoneAddress interpretUrl(String destination) throws LinphoneCoreException {
		long lAddress = interpretUrl(nativePtr,destination);
		if (lAddress != 0) {
			return new LinphoneAddressImpl(lAddress,LinphoneAddressImpl.WrapMode.FromNew);
		} else {
			throw new LinphoneCoreException("Cannot interpret ["+destination+"]");
		}
	}
	public synchronized LinphoneCall invite(LinphoneAddress to) throws LinphoneCoreException {
		LinphoneCall call = (LinphoneCall)inviteAddress(nativePtr,((LinphoneAddressImpl)to).nativePtr);
		if (call!=null) {
			return call;
		} else {
			throw new LinphoneCoreException("Unable to invite address " + to.asString());
		}
	}

	public synchronized void sendDtmfs(String dtmf) {
		sendDtmfs(nativePtr, dtmf);
	}

	public synchronized void sendDtmf(char number) {
		sendDtmf(nativePtr, number);
	}
	public synchronized void clearCallLogs() {
		clearCallLogs(nativePtr);
	}
	public synchronized boolean isMicMuted() {
		return isMicMuted(nativePtr);
	}
	public synchronized PayloadType findPayloadType(String mime, int clockRate, int channels) {
		isValid();
		long playLoadType = findPayloadType(nativePtr, mime, clockRate, channels);
		if (playLoadType == 0) {
			return null;
		} else {
			return new PayloadTypeImpl(playLoadType);
		}
	}
	public synchronized void enablePayloadType(PayloadType pt, boolean enable)
			throws LinphoneCoreException {
		isValid();
		if (enablePayloadType(nativePtr,((PayloadTypeImpl)pt).nativePtr,enable) != 0) {
			throw new LinphoneCoreException("cannot enable payload type ["+pt+"]");
		}

	}
	public synchronized boolean isPayloadTypeEnabled(PayloadType pt) {
		isValid();
		return isPayloadTypeEnabled(nativePtr, ((PayloadTypeImpl)pt).nativePtr);
	}

	public synchronized boolean payloadTypeIsVbr(PayloadType pt) {
		isValid();
		return payloadTypeIsVbr(nativePtr, ((PayloadTypeImpl) pt).nativePtr);
	}

	public synchronized void enableEchoCancellation(boolean enable) {
		isValid();
		enableEchoCancellation(nativePtr, enable);
	}
	public synchronized boolean isEchoCancellationEnabled() {
		isValid();
		return isEchoCancellationEnabled(nativePtr);

	}

	public synchronized LinphoneCall getCurrentCall() {
		isValid();
		return (LinphoneCall)getCurrentCall(nativePtr);
	}

	public int getPlayLevel() {
		// TODO Auto-generated method stub
		return 0;
	}
	public void setPlayLevel(int level) {
		// TODO Auto-generated method stub

	}

	private void applyAudioHacks() {
		if (Hacks.needGalaxySAudioHack()) {
			/* The microphone gain is way too high on the Galaxy S so correct it here. */
			setMicrophoneGain(-9.0f);
		}
	}
	private void setAudioModeIncallForGalaxyS() {
		if (!contextInitialized()) return;
		mAudioManager.setMode(MODE_IN_CALL);
	}
	public void routeAudioToSpeakerHelper(boolean speakerOn) {
		if (!contextInitialized()) return;
		if (Hacks.needGalaxySAudioHack())
			setAudioModeIncallForGalaxyS();
		mAudioManager.setSpeakerphoneOn(speakerOn);
	}
	private native void forceSpeakerState(long nativePtr, boolean speakerOn);
	public void enableSpeaker(boolean value) {
		final LinphoneCall call = getCurrentCall();
		mSpeakerEnabled = value;
		applyAudioHacks();
		if (call != null && call.getState() == State.StreamsRunning && Hacks.needGalaxySAudioHack()) {
			Log.d("Hack to have speaker=", value, " while on call");
			forceSpeakerState(nativePtr, value);
		} else {
			routeAudioToSpeakerHelper(value);
		}
	}
	public boolean isSpeakerEnabled() {
		return mSpeakerEnabled;
	}
	public synchronized void playDtmf(char number, int duration) {
		playDtmf(nativePtr,number, duration);

	}
	public synchronized void stopDtmf() {
		stopDtmf(nativePtr);
	}

	public synchronized void addFriend(LinphoneFriend lf) throws LinphoneCoreException {
		addFriend(nativePtr, ((LinphoneFriendImpl) lf).nativePtr);
	}


	public synchronized LinphoneFriendList createLinphoneFriendList() {
		return new LinphoneFriendListImpl(this);
	}


	public synchronized void addFriendList(LinphoneFriendList friendList) throws LinphoneCoreException {
		addFriendList(nativePtr,((LinphoneFriendListImpl)friendList).nativePtr);
	}

	public synchronized void removeFriendList(LinphoneFriendList friendList) throws LinphoneCoreException {
		removeFriendList(nativePtr,((LinphoneFriendListImpl)friendList).nativePtr);
	}

	public synchronized LinphoneFriend[] getFriendList() {
		return getFriendList(nativePtr);
	}

	private native LinphoneFriendList[] getFriendLists(long nativePtr);
	public synchronized LinphoneFriendList[] getFriendLists() {
		return getFriendLists(nativePtr);
	}

	@SuppressWarnings("deprecation")
	public synchronized void setPresenceInfo(int minutes_away, String alternative_contact, OnlineStatus status) {
		setPresenceInfo(nativePtr, minutes_away, alternative_contact, status.mValue);

	}
	@SuppressWarnings("deprecation")
	public synchronized OnlineStatus getPresenceInfo() {
		return OnlineStatus.fromInt(getPresenceInfo(nativePtr));
	}
	public synchronized void setPresenceModel(PresenceModel presence) {
		setPresenceModel(nativePtr, ((PresenceModelImpl) presence).getNativePtr());
	}
	public synchronized PresenceModel getPresenceModel() {
		return (PresenceModel)getPresenceModel(nativePtr);
	}
	public synchronized LinphoneChatRoom getOrCreateChatRoom(String to) {
		return (LinphoneChatRoom)(getOrCreateChatRoom(nativePtr,to));
	}
	public synchronized LinphoneChatRoom getChatRoom(LinphoneAddress to) {
		return (LinphoneChatRoom)(getChatRoom(nativePtr, ((LinphoneAddressImpl) to).nativePtr));
	}
	public synchronized void setPreviewWindow(Object w) {
		setPreviewWindowId(nativePtr, w);
	}
	public synchronized void setVideoWindow(Object w) {
		setVideoWindowId(nativePtr, w);
	}
	public synchronized void setDeviceRotation(int rotation) {
		setDeviceRotation(nativePtr, rotation);
	}

	public synchronized void enableVideo(boolean vcap_enabled, boolean display_enabled) {
		enableVideo(nativePtr, vcap_enabled, display_enabled);
	}
	public synchronized boolean isVideoEnabled() {
		return isVideoEnabled(nativePtr);
	}
	public synchronized boolean isVideoSupported() {
		return isVideoSupported(nativePtr);
	}
	public synchronized boolean isVCardSupported() {
		return isVCardSupported(nativePtr);
	}
	public synchronized FirewallPolicy getFirewallPolicy() {
		return FirewallPolicy.fromInt(getFirewallPolicy(nativePtr));
	}
	public synchronized void setFirewallPolicy(FirewallPolicy pol) {
		setFirewallPolicy(nativePtr,pol.value());
	}
	public synchronized LinphoneNatPolicy createNatPolicy() {
		return (LinphoneNatPolicy)createNatPolicy(nativePtr);
	}
	public synchronized void setNatPolicy(LinphoneNatPolicy policy) {
		setNatPolicy(nativePtr, ((LinphoneNatPolicyImpl)policy).mNativePtr);
	}
	public synchronized LinphoneNatPolicy getNatPolicy() {
		return (LinphoneNatPolicy)getNatPolicy(nativePtr);
	}
	public synchronized String getStunServer() {
		return getStunServer(nativePtr);
	}
	public synchronized void setStunServer(String stunServer) {
		setStunServer(nativePtr, stunServer);
	}

	public synchronized LinphoneCall inviteAddressWithParams(LinphoneAddress to, LinphoneCallParams params) throws LinphoneCoreException {
		long ptrDestination = ((LinphoneAddressImpl)to).nativePtr;
		long ptrParams =((LinphoneCallParamsImpl)params).nativePtr;

		LinphoneCall call = (LinphoneCall)inviteAddressWithParams(nativePtr, ptrDestination, ptrParams);
		if (call!=null) {
			return call;
		} else {
			throw new LinphoneCoreException("Unable to invite with params " + to.asString());
		}
	}

	public synchronized int updateCall(LinphoneCall call, LinphoneCallParams params) {
		long ptrCall = ((LinphoneCallImpl) call).nativePtr;
		long ptrParams = params!=null ? ((LinphoneCallParamsImpl)params).nativePtr : 0;

		return updateCall(nativePtr, ptrCall, ptrParams);
	}

	public synchronized int getUploadBandwidth() {
		return getUploadBandwidth(nativePtr);
	}

	public synchronized void setUploadBandwidth(int bw) {
		setUploadBandwidth(nativePtr, bw);
	}

	public synchronized int getDownloadBandwidth() {
		return getDownloadBandwidth(nativePtr);
	}

	public synchronized void setDownloadBandwidth(int bw) {
		setDownloadBandwidth(nativePtr, bw);
	}

	public synchronized void setPreferredVideoSize(VideoSize vSize) {
		setPreferredVideoSize(nativePtr, vSize.width, vSize.height);
	}

	public synchronized void setPreferredVideoSizeByName(String name) {
		setPreferredVideoSizeByName(nativePtr, name);
	}

	public synchronized VideoSize getPreferredVideoSize() {
		int[] nativeSize = getPreferredVideoSize(nativePtr);

		VideoSize vSize = new VideoSize();
		vSize.width = nativeSize[0];
		vSize.height = nativeSize[1];
		return vSize;
	}
	public synchronized void setRing(String path) {
		setRing(nativePtr, path);
	}
	public synchronized String getRing() {
		return getRing(nativePtr);
	}

	public synchronized void setRootCA(String path) {
		setRootCA(nativePtr, path);
	}

	public synchronized void setRootCAData(String data) {
		setRootCAData(nativePtr, data);
	}

	public synchronized void setRingback(String path) {
		setRingback(nativePtr, path);
	}

	public synchronized LinphoneProxyConfig[] getProxyConfigList() {
		return getProxyConfigList(nativePtr);
	}

	public synchronized PayloadType[] getVideoCodecs() {
		long[] typesPtr = listVideoPayloadTypes(nativePtr);
		if (typesPtr == null) return null;

		PayloadType[] codecs = new PayloadType[typesPtr.length];

		for (int i=0; i < codecs.length; i++) {
			codecs[i] = new PayloadTypeImpl(typesPtr[i]);
		}

		return codecs;
	}
	public synchronized void setVideoCodecs(PayloadType[] codecs) {
		long[] typesPtr = new long[codecs.length];
		for (int i=0; i < codecs.length; i++) {
			typesPtr[i] = ((PayloadTypeImpl)codecs[i]).nativePtr;
		}
		setVideoCodecs(nativePtr, typesPtr);
	}
	public synchronized PayloadType[] getAudioCodecs() {
		long[] typesPtr = listAudioPayloadTypes(nativePtr);
		if (typesPtr == null) return null;

		PayloadType[] codecs = new PayloadType[typesPtr.length];

		for (int i=0; i < codecs.length; i++) {
			codecs[i] = new PayloadTypeImpl(typesPtr[i]);
		}

		return codecs;
	}
	public synchronized void setAudioCodecs(PayloadType[] codecs) {
		long[] typesPtr = new long[codecs.length];
		for (int i=0; i < codecs.length; i++) {
			typesPtr[i] = ((PayloadTypeImpl)codecs[i]).nativePtr;
		}
		setAudioCodecs(nativePtr, typesPtr);
	}
	public synchronized boolean isNetworkReachable() {
		return isNetworkStateReachable(nativePtr);
	}

	public synchronized void enableKeepAlive(boolean enable) {
		enableKeepAlive(nativePtr, enable);

	}
	public synchronized boolean isKeepAliveEnabled() {
		return isKeepAliveEnabled(nativePtr);
	}
	public synchronized void startEchoCalibration(LinphoneCoreListener listener) throws LinphoneCoreException {
		startEchoCalibration(nativePtr, listener);
	}

	public synchronized int startEchoTester(int rate) {
		return startEchoTester(nativePtr, rate);
	}

	public synchronized int stopEchoTester() {
		return stopEchoTester(nativePtr);
	}

	public synchronized Transports getSignalingTransportPorts() {
		Transports transports = new Transports();
		transports.udp = getSignalingTransportPort(nativePtr, 0);
		transports.tcp = getSignalingTransportPort(nativePtr, 1);
		transports.tls = getSignalingTransportPort(nativePtr, 3);
		// See C struct LCSipTransports in linphonecore.h
		// Code is the index in the structure
		return transports;
	}
	public synchronized void setSignalingTransportPorts(Transports transports) {
		setSignalingTransportPorts(nativePtr, transports.udp, transports.tcp, transports.tls);
	}

	public synchronized void enableIpv6(boolean enable) {
		enableIpv6(nativePtr, enable);
	}
	public synchronized boolean isIpv6Enabled() {
		return isIpv6Enabled(nativePtr);
	}
	public synchronized void adjustSoftwareVolume(int i) {
		//deprecated, does the same as setPlaybackGain().
	}

	public synchronized boolean pauseCall(LinphoneCall call) {
		return 0 == pauseCall(nativePtr, ((LinphoneCallImpl) call).nativePtr);
	}
	public synchronized boolean resumeCall(LinphoneCall call) {
		return 0 == resumeCall(nativePtr, ((LinphoneCallImpl) call).nativePtr);
	}
	public synchronized boolean pauseAllCalls() {
		return 0 == pauseAllCalls(nativePtr);
	}
	public synchronized void setDownloadPtime(int ptime) {
		setDownloadPtime(nativePtr,ptime);

	}
	public synchronized void setUploadPtime(int ptime) {
		setUploadPtime(nativePtr, ptime);
	}

	public synchronized void setZrtpSecretsCache(String file) {
		setZrtpSecretsCache(nativePtr, file);
	}
	public synchronized void enableEchoLimiter(boolean val) {
		enableEchoLimiter(nativePtr, val);
	}
	public synchronized void setVideoDevice(int id) {
		Log.i("Setting camera id :", id);
		if (setVideoDevice(nativePtr, id) != 0) {
			Log.e("Failed to set video device to id:", id);
		}
	}
	public synchronized int getVideoDevice() {
		return getVideoDevice(nativePtr);
	}


	private native void leaveConference(long nativePtr);
	public synchronized void leaveConference() {
		leaveConference(nativePtr);
	}

	private native boolean enterConference(long nativePtr);
	public synchronized boolean enterConference() {
		return enterConference(nativePtr);
	}

	private native boolean isInConference(long nativePtr);
	public synchronized boolean isInConference() {
		return isInConference(nativePtr);
	}

	private native void terminateConference(long nativePtr);
	public synchronized void terminateConference() {
		terminateConference(nativePtr);
	}
	private native int getConferenceSize(long nativePtr);
	public synchronized int getConferenceSize() {
		return getConferenceSize(nativePtr);
	}
	private native LinphoneConference createConference(long corePtr, LinphoneConferenceParams params);
	public synchronized LinphoneConference createConference(LinphoneConferenceParams params) {
		return createConference(this.nativePtr, params);
	}
	private native LinphoneConference getConference(long nativePtr);
	public synchronized LinphoneConference getConference() {
		return getConference(nativePtr);
	}
	private native int getCallsNb(long nativePtr);
	public synchronized int getCallsNb() {
		return getCallsNb(nativePtr);
	}
	private native void terminateAllCalls(long nativePtr);
	public synchronized void terminateAllCalls() {
		terminateAllCalls(nativePtr);
	}
	private native Object getCall(long nativePtr, int position);
	public synchronized LinphoneCall[] getCalls() {
		int size = getCallsNb(nativePtr);
		LinphoneCall[] calls = new LinphoneCall[size];
		for (int i=0; i < size; i++) {
			calls[i]=((LinphoneCall)getCall(nativePtr, i));
		}
		return calls;
	}
	private native void addAllToConference(long nativePtr);
	public synchronized void addAllToConference() {
		addAllToConference(nativePtr);

	}
	private native void addToConference(long nativePtr, long nativePtrLcall);
	public synchronized void addToConference(LinphoneCall call) {
		addToConference(nativePtr, getCallPtr(call));

	}
	private native void removeFromConference(long nativePtr, long nativeCallPtr);
	public synchronized void removeFromConference(LinphoneCall call) {
		removeFromConference(nativePtr,getCallPtr(call));
	}

	private long getCallPtr(LinphoneCall call) {
		return ((LinphoneCallImpl)call).nativePtr;
	}

	private long getCallParamsPtr(LinphoneCallParams callParams) {
		return ((LinphoneCallParamsImpl)callParams).nativePtr;
	}

	private native int transferCall(long nativePtr, long callPtr, String referTo);
	public synchronized void transferCall(LinphoneCall call, String referTo) {
		transferCall(nativePtr, getCallPtr(call), referTo);
	}

	private native int transferCallToAnother(long nativePtr, long callPtr, long destPtr);
	public synchronized void transferCallToAnother(LinphoneCall call, LinphoneCall dest) {
		transferCallToAnother(nativePtr, getCallPtr(call), getCallPtr(dest));
	}

	private native Object findCallFromUri(long nativePtr, String uri);
	@Override
	public synchronized LinphoneCall findCallFromUri(String uri) {
		return (LinphoneCall) findCallFromUri(nativePtr, uri);
	}

	public synchronized MediaEncryption getMediaEncryption() {
		return MediaEncryption.fromInt(getMediaEncryption(nativePtr));
	}
	public synchronized boolean isMediaEncryptionMandatory() {
		return isMediaEncryptionMandatory(nativePtr);
	}
	public synchronized void setMediaEncryption(MediaEncryption menc) {
		setMediaEncryption(nativePtr, menc.mValue);
	}
	public synchronized void setMediaEncryptionMandatory(boolean yesno) {
		setMediaEncryptionMandatory(nativePtr, yesno);
	}

	private native int getMaxCalls(long nativePtr);
	public synchronized int getMaxCalls() {
		return getMaxCalls(nativePtr);
	}
	@Override
	public boolean isMyself(String uri) {
		LinphoneProxyConfig lpc = getDefaultProxyConfig();
		if (lpc == null) return false;
		return uri.equals(lpc.getIdentity());
	}

	private native boolean soundResourcesLocked(long nativePtr);
	public synchronized boolean soundResourcesLocked() {
		return soundResourcesLocked(nativePtr);
	}

	private native void setMaxCalls(long nativePtr, int max);
	@Override
	public synchronized void setMaxCalls(int max) {
		setMaxCalls(nativePtr, max);
	}
	private native boolean isEchoLimiterEnabled(long nativePtr);
	@Override
	public synchronized boolean isEchoLimiterEnabled() {
		return isEchoLimiterEnabled(nativePtr);
	}
	private native boolean mediaEncryptionSupported(long nativePtr, int menc);
	@Override
	public synchronized boolean mediaEncryptionSupported(MediaEncryption menc) {
		return mediaEncryptionSupported(nativePtr,menc.mValue);
	}

	private native void setPlayFile(long nativePtr, String path);

	@Override
	public synchronized void setPlayFile(String path) {
		setPlayFile(nativePtr, path);
	}


	private native void tunnelAddServerAndMirror(long nativePtr, String host, int port, int mirror, int ms);
	@Override
	public synchronized void tunnelAddServerAndMirror(String host, int port, int mirror, int ms) {
		tunnelAddServerAndMirror(nativePtr, host, port, mirror, ms);
	}

	private native void tunnelAddServer(long nativePtr, long configPtr);
	@Override
	public synchronized void tunnelAddServer(TunnelConfig config) {
		tunnelAddServer(nativePtr, ((TunnelConfigImpl)config).mNativePtr);
	}

	private native final TunnelConfig[] tunnelGetServers(long nativePtr);
	@Override
	public synchronized final TunnelConfig[] tunnelGetServers() {
		return tunnelGetServers(nativePtr);
	}

	private native void tunnelAutoDetect(long nativePtr);
	@Override
	public synchronized void tunnelAutoDetect() {
		tunnelAutoDetect(nativePtr);
	}

	private native void tunnelCleanServers(long nativePtr);
	@Override
	public synchronized void tunnelCleanServers() {
		tunnelCleanServers(nativePtr);
	}

	private native void tunnelEnable(long nativePtr, boolean enable);
	@Override
	public synchronized void tunnelEnable(boolean enable) {
		tunnelEnable(nativePtr, enable);
	}

	private native void tunnelSetMode(long nativePtr, int mode);
	@Override
	public synchronized void tunnelSetMode(LinphoneCore.TunnelMode mode) {
		tunnelSetMode(nativePtr, TunnelMode.enumToInt(mode));
	}

	private native int tunnelGetMode(long nativePtr);
	@Override
	public synchronized LinphoneCore.TunnelMode tunnelGetMode() {
		return LinphoneCore.TunnelMode.intToEnum(tunnelGetMode(nativePtr));
	}

	private native void tunnelEnableSip(long nativePtr, boolean enable);
	@Override
	public void tunnelEnableSip(boolean enable) {
		tunnelEnableSip(nativePtr, enable);
	}

	private native boolean tunnelSipEnabled(long nativePtr);
	@Override
	public boolean tunnelSipEnabled() {
		return tunnelSipEnabled(nativePtr);
	}

	@Override
	public native boolean isTunnelAvailable();

	private native void acceptCallWithParams(long nativePtr, long aCall,
			long params);
	@Override
	public synchronized void acceptCallWithParams(LinphoneCall aCall,
			LinphoneCallParams params) throws LinphoneCoreException {
		acceptCallWithParams(nativePtr, getCallPtr(aCall), getCallParamsPtr(params));
	}

	private native void acceptCallUpdate(long nativePtr, long aCall, long params);
	@Override
	public synchronized void acceptCallUpdate(LinphoneCall aCall, LinphoneCallParams params)
			throws LinphoneCoreException {
		acceptCallUpdate(nativePtr, getCallPtr(aCall), getCallParamsPtr(params));
	}

	private native void deferCallUpdate(long nativePtr, long aCall);
	@Override
	public synchronized void deferCallUpdate(LinphoneCall aCall)
			throws LinphoneCoreException {
		deferCallUpdate(nativePtr, getCallPtr(aCall));
	}


	private native void setVideoPolicy(long nativePtr, boolean autoInitiate, boolean autoAccept);
	public synchronized void setVideoPolicy(boolean autoInitiate, boolean autoAccept) {
		setVideoPolicy(nativePtr, autoInitiate, autoAccept);
	}
	private native boolean getVideoAutoInitiatePolicy(long nativePtr);
	public synchronized boolean getVideoAutoInitiatePolicy() {
		return getVideoAutoInitiatePolicy(nativePtr);
	}
	private native boolean getVideoAutoAcceptPolicy(long nativePtr);
	public synchronized boolean getVideoAutoAcceptPolicy() {
		return getVideoAutoAcceptPolicy(nativePtr);
	}

	private native void setStaticPicture(long nativePtr, String path);
	public synchronized void setStaticPicture(String path) {
		setStaticPicture(nativePtr, path);
	}
	private native void setUserAgent(long nativePtr, String name, String version);
	@Override
	public synchronized void setUserAgent(String name, String version) {
		setUserAgent(nativePtr,name,version);
	}

	private native void setCpuCountNative(long nativePtr, int count);
	public synchronized void setCpuCount(int count)
	{
		setCpuCountNative(nativePtr, count);
	}

	public synchronized int getMissedCallsCount() {
		return getMissedCallsCount(nativePtr);
	}

	public synchronized void removeCallLog(LinphoneCallLog log) {
		removeCallLog(nativePtr, ((LinphoneCallLogImpl) log).getNativePtr());
	}

	public synchronized void resetMissedCallsCount() {
		resetMissedCallsCount(nativePtr);
	}

	private native void tunnelSetHttpProxy(long nativePtr, String proxy_host, int port,
			String username, String password);
	@Override
	public synchronized void tunnelSetHttpProxy(String proxy_host, int port,
			String username, String password) {
		tunnelSetHttpProxy(nativePtr, proxy_host, port, username, password);
	}

	private native void refreshRegisters(long nativePtr);
	public synchronized void refreshRegisters() {
		refreshRegisters(nativePtr);
	}

	@Override
	public String getVersion() {
		return getVersion(nativePtr);
	}
	/**
	 * Wildcard value used by #linphone_core_find_payload_type to ignore rate in search algorithm
	 */
	static int FIND_PAYLOAD_IGNORE_RATE = -1;
	/**
	 * Wildcard value used by #linphone_core_find_payload_type to ignore channel in search algorithm
	 */
	static int FIND_PAYLOAD_IGNORE_CHANNELS = -1;
	@Override
	public synchronized PayloadType findPayloadType(String mime, int clockRate) {
		return findPayloadType(mime, clockRate, FIND_PAYLOAD_IGNORE_CHANNELS);
	}

	private native void removeFriend(long ptr, long lf);
	@Override
	public synchronized void removeFriend(LinphoneFriend lf) {
		removeFriend(nativePtr, lf.getNativePtr());
	}

	private native LinphoneFriend getFriendByAddress(long ptr, String sipUri);
	@Override
	public synchronized LinphoneFriend findFriendByAddress(String sipUri) {
		return getFriendByAddress(nativePtr, sipUri);
	}

	public synchronized void setAudioPort(int port) {
		setAudioPort(nativePtr, port);
	}

	public synchronized void setVideoPort(int port) {
		setVideoPort(nativePtr, port);
	}

	public synchronized void setAudioPortRange(int minPort, int maxPort) {
		setAudioPortRange(nativePtr, minPort, maxPort);
	}

	public synchronized void setVideoPortRange(int minPort, int maxPort) {
		setVideoPortRange(nativePtr, minPort, maxPort);
	}

	public synchronized void setIncomingTimeout(int timeout) {
		setIncomingTimeout(nativePtr, timeout);
	}

	public synchronized void setInCallTimeout(int timeout)
	{
		setInCallTimeout(nativePtr, timeout);
	}

	private native void setMicrophoneGain(long ptr, float gain);
	public synchronized void setMicrophoneGain(float gain) {
		setMicrophoneGain(nativePtr, gain);
	}

	public synchronized void setPrimaryContact(String address) {
		setPrimaryContact2(nativePtr, address);
	}

	public synchronized String getPrimaryContact() {
		return getPrimaryContact(nativePtr);
	}

	public synchronized void setPrimaryContact(String displayName, String username) {
		setPrimaryContact(nativePtr, displayName, username);
	}

	public synchronized String getPrimaryContactUsername() {
		return getPrimaryContactUsername(nativePtr);
	}

	public synchronized String getPrimaryContactDisplayName() {
		return getPrimaryContactDisplayName(nativePtr);
	}

	private native void setUseSipInfoForDtmfs(long ptr, boolean use);
	public synchronized void setUseSipInfoForDtmfs(boolean use) {
		setUseSipInfoForDtmfs(nativePtr, use);
	}

	private native boolean getUseSipInfoForDtmfs(long ptr);
	public synchronized boolean getUseSipInfoForDtmfs() {
		return getUseSipInfoForDtmfs(nativePtr);
	}

	private native void setUseRfc2833ForDtmfs(long ptr, boolean use);
	public synchronized void setUseRfc2833ForDtmfs(boolean use) {
		setUseRfc2833ForDtmfs(nativePtr, use);
	}

	private native boolean getUseRfc2833ForDtmfs(long ptr);
	public synchronized boolean getUseRfc2833ForDtmfs() {
		return getUseRfc2833ForDtmfs(nativePtr);
	}

	private native long getConfig(long ptr);
	public synchronized LpConfig getConfig() {
		long configPtr=getConfig(nativePtr);
		return new LpConfigImpl(configPtr);
	}
	private native boolean needsEchoCalibration(long ptr);
	@Override
	public synchronized boolean needsEchoCalibration() {
		return needsEchoCalibration(nativePtr);
	}
	private native boolean hasBuiltInEchoCanceler(long ptr);
	@Override
	public synchronized boolean hasBuiltInEchoCanceler() {
		return hasBuiltInEchoCanceler(nativePtr);
	}
	private native boolean hasCrappyOpenGL(long ptr);
	@Override
	public synchronized boolean hasCrappyOpenGL() {
		return hasCrappyOpenGL(nativePtr);
	}
	private native void declineCall(long coreptr, long callptr, int reason);
	@Override
	public synchronized void declineCall(LinphoneCall aCall, Reason reason) {
		declineCall(nativePtr,((LinphoneCallImpl)aCall).nativePtr,reason.mValue);
	}

	private native boolean upnpAvailable(long ptr);
	public synchronized boolean upnpAvailable() {
		return upnpAvailable(nativePtr);
	}

	private native int getUpnpState(long ptr);
	public synchronized UpnpState getUpnpState() {
		return UpnpState.fromInt(getUpnpState(nativePtr));
	}

	private native String getUpnpExternalIpaddress(long ptr);
	public synchronized String getUpnpExternalIpaddress() {
		return getUpnpExternalIpaddress(nativePtr);
	}
	private native int startConferenceRecording(long nativePtr, String path);
	@Override
	public synchronized void startConferenceRecording(String path) {
		startConferenceRecording(nativePtr,path);
	}

	private native int stopConferenceRecording(long nativePtr);
	@Override
	public synchronized void stopConferenceRecording() {
		stopConferenceRecording(nativePtr);
	}
	@Override
	public synchronized PayloadType findPayloadType(String mime) {
		return findPayloadType(mime, FIND_PAYLOAD_IGNORE_RATE);
	}

	private native void setSipDscp(long nativePtr, int dscp);
	@Override
	public synchronized void setSipDscp(int dscp) {
		setSipDscp(nativePtr, dscp);
	}

	private native int getSipDscp(long nativePtr);
	@Override
	public synchronized int getSipDscp() {
		return getSipDscp(nativePtr);
	}
	private native void setAudioDscp(long nativePtr, int dscp);
	@Override
	public synchronized void setAudioDscp(int dscp) {
		setAudioDscp(nativePtr, dscp);
	}

	private native int getAudioDscp(long nativePtr);
	@Override
	public synchronized int getAudioDscp() {
		return getAudioDscp(nativePtr);
	}

	private native void setVideoDscp(long nativePtr, int dscp);
	@Override
	public synchronized void setVideoDscp(int dscp) {
		setVideoDscp(nativePtr,dscp);
	}

	private native int getVideoDscp(long nativePtr);
	@Override
	public synchronized int getVideoDscp() {
		return getVideoDscp(nativePtr);
	}

	private native long createInfoMessage(long nativeptr);
	@Override
	public synchronized LinphoneInfoMessage createInfoMessage() {
		return new LinphoneInfoMessageImpl(createInfoMessage(nativePtr));
	}

	private native Object subscribe(long coreptr, long addrptr, String eventname, int expires, String type, String subtype, byte data [], String encoding);
	@Override
	public synchronized LinphoneEvent subscribe(LinphoneAddress resource, String eventname,
			int expires, LinphoneContent content) {
		return (LinphoneEvent)subscribe(nativePtr, ((LinphoneAddressImpl)resource).nativePtr, eventname, expires,
				content!=null ? content.getType() : null, content!=null ? content.getSubtype() : null, content!=null ? content.getData() : null,
						content!=null ? content.getEncoding() : null);
	}
	private native Object publish(long coreptr, long addrptr, String eventname, int expires, String type, String subtype, byte data [], String encoding);
	@Override
	public synchronized LinphoneEvent publish(LinphoneAddress resource, String eventname,
			int expires, LinphoneContent content) {
		return (LinphoneEvent)publish(nativePtr, ((LinphoneAddressImpl) resource).nativePtr, eventname, expires,
				content != null ? content.getType() : null, content != null ? content.getSubtype() : null, content != null ? content.getData() : null,
				content != null ? content.getEncoding() : null);
	}

	private native Object createSubscribe(long core, long addr, String event, int expires);
	@Override
	public synchronized LinphoneEvent createSubscribe(LinphoneAddress resource,
			String event, int expires) {
		return (LinphoneEvent)createSubscribe(nativePtr, ((LinphoneAddressImpl) resource).nativePtr, event, expires);
	}
	private native Object createPublish(long core, long addr, String event, int expires);
	@Override
	public synchronized LinphoneEvent createPublish(LinphoneAddress resource,
			String event, int expires) {
		return (LinphoneEvent)createPublish(nativePtr, ((LinphoneAddressImpl) resource).nativePtr, event, expires);
	}

	public synchronized void setChatDatabasePath(String path) {
		setChatDatabasePath(nativePtr, path);
	}

	public synchronized void setCallLogsDatabasePath(String path) {
		setCallLogsDatabasePath(nativePtr, path);
	}

	public synchronized void setFriendsDatabasePath(String path) {
		setFriendsDatabasePath(nativePtr, path);
	}

	public synchronized LinphoneChatRoom[] getChatRooms() {
		Object[] typesPtr = getChatRooms(nativePtr);
		if (typesPtr == null) return null;

		LinphoneChatRoom[] proxies = new LinphoneChatRoom[typesPtr.length];

		for (int i=0; i < proxies.length; i++) {
			proxies[i] = (LinphoneChatRoom)(typesPtr[i]);
		}

		return proxies;
	}
	public synchronized LinphoneAuthInfo[] getAuthInfosList() {
		long[] typesPtr = getAuthInfosList(nativePtr);
		if (typesPtr == null) return null;

		LinphoneAuthInfo[] authInfos = new LinphoneAuthInfo[typesPtr.length];

		for (int i=0; i < authInfos.length; i++) {
			authInfos[i] = new LinphoneAuthInfoImpl(typesPtr[i]);
		}

		return authInfos;
	}

	public synchronized LinphoneAuthInfo findAuthInfo(String username, String realm, String domain) {
		long ptr = findAuthInfos(nativePtr, username, realm, domain);
		if (ptr == 0)
			return null;

		return new LinphoneAuthInfoImpl(ptr);
	}
	private native LinphoneCall startReferedCall(long corePtr, long callptr, long paramsPtr);
	@Override
	public synchronized LinphoneCall startReferedCall(LinphoneCall call,
			LinphoneCallParams params) {
		long ptrParams =((LinphoneCallParamsImpl)params).nativePtr;
		return startReferedCall(nativePtr, getCallPtr(call), ptrParams);
	}

	private native String[] listSupportedVideoResolutions(long ptr);
	@Override
	public synchronized String[] getSupportedVideoSizes() {
		return listSupportedVideoResolutions(nativePtr);
	}

	@Override
	public synchronized int migrateToMultiTransport() {
		return migrateToMultiTransport(nativePtr);
	}

	@Override
	public synchronized void migrateCallLogs() {
		migrateCallLogs(nativePtr);
	}

	private native boolean acceptEarlyMedia(long lc, long call);
	@Override
	public synchronized boolean acceptEarlyMedia(LinphoneCall call) {
		return acceptEarlyMedia(nativePtr, getCallPtr(call));
	}

	private native boolean acceptEarlyMediaWithParams(long lc, long call, long params);
	@Override
	public synchronized boolean acceptEarlyMediaWithParams(LinphoneCall call,
			LinphoneCallParams params) {
		long ptrParams = params != null ? ((LinphoneCallParamsImpl) params).nativePtr : 0;
		return acceptEarlyMediaWithParams(nativePtr, getCallPtr(call), ptrParams);
	}
	@Override
	public synchronized LinphoneProxyConfig createProxyConfig() {
		return new LinphoneProxyConfigImpl(this);
	}
	@Override
	public synchronized LinphoneProxyConfig createProxyConfig(String identity,String proxy,String route, boolean enableRegister) throws LinphoneCoreException {
		isValid();
		try {
			return new LinphoneProxyConfigImpl(this,identity,proxy,route,enableRegister);
		} catch(LinphoneCoreException e){
			return null;
		}
	}
	@Override
	public synchronized void setCallErrorTone(Reason reason, String path) {
		setCallErrorTone(nativePtr, reason.mValue, path);
	}
	private native void setMtu(long nativePtr, int mtu);
	@Override
	public synchronized void setMtu(int mtu) {
		setMtu(nativePtr, mtu);
	}
	private native int getMtu(long nativePtr);
	@Override
	public synchronized int getMtu() {
		return getMtu(nativePtr);
	}
	@Override
	public synchronized void enableSdp200Ack(boolean enable) {
		enableSdp200Ack(nativePtr, enable);
	}
	@Override
	public synchronized boolean isSdp200AckEnabled() {
		return isSdp200AckEnabled(nativePtr);
	}
	private native void setTone(long nativePtr, int id, String wavfile);
	@Override
	public synchronized void setTone(ToneID id, String wavfile) {
		setTone(nativePtr, id.mValue, wavfile);
	}
	private native void disableChat(long ptr, int denycode);
	@Override
	public synchronized void disableChat(Reason denycode) {
		disableChat(nativePtr, denycode.mValue);
	}
	private native void enableChat(long ptr);
	@Override
	public synchronized void enableChat() {
		enableChat(nativePtr);
	}
	private native boolean chatEnabled(long ptr);
	@Override
	public synchronized boolean chatEnabled() {
		return chatEnabled(nativePtr);
	}

	@Override
	public synchronized void stopRinging() {
		stopRinging(nativePtr);
	}
	private native void setPayloadTypeBitrate(long coreptr, long payload_ptr, int bitrate);
	@Override
	public synchronized void setPayloadTypeBitrate(PayloadType pt, int bitrate) {
		setPayloadTypeBitrate(nativePtr, ((PayloadTypeImpl) pt).nativePtr, bitrate);
	}
	private native int getPayloadTypeBitrate(long coreptr, long payload_ptr);
	@Override
	public synchronized int getPayloadTypeBitrate(PayloadType pt) {
		return getPayloadTypeBitrate(nativePtr, ((PayloadTypeImpl)pt).nativePtr);
	}

	private native void setPayloadTypeNumber(long coreptr, long payload_ptr, int number);
	@Override
	public synchronized void setPayloadTypeNumber(PayloadType pt, int number) {
		setPayloadTypeNumber(nativePtr, ((PayloadTypeImpl)pt).nativePtr, number);
	}
	private native int getPayloadTypeNumber(long coreptr, long payload_ptr);
	@Override
	public synchronized int getPayloadTypeNumber(PayloadType pt) {
		return getPayloadTypeNumber(nativePtr, ((PayloadTypeImpl)pt).nativePtr);
	}

	@Override
	public synchronized void enableAdaptiveRateControl(boolean enable) {
		enableAdaptiveRateControl(nativePtr, enable);

	}
	@Override
	public synchronized boolean isAdaptiveRateControlEnabled() {
		return isAdaptiveRateControlEnabled(nativePtr);
	}
	public synchronized AdaptiveRateAlgorithm getAdaptiveRateAlgorithm() {
		return AdaptiveRateAlgorithm.fromString(getAdaptiveRateAlgorithm(nativePtr));
	}
	public synchronized void setAdaptiveRateAlgorithm(AdaptiveRateAlgorithm alg) {
		setAdaptiveRateAlgorithm(nativePtr, alg.toString());
	}


	private native void setAudioJittcomp(long ptr, int value);
	@Override
	public synchronized void setAudioJittcomp(int value) {
		setAudioJittcomp(nativePtr, value);
	}
	private native void setVideoJittcomp(long ptr, int value);
	@Override
	public synchronized void setVideoJittcomp(int value) {
		setVideoJittcomp(nativePtr, value);
	}

	private native void setFileTransferServer(long ptr, String serverUrl);
	@Override
	public synchronized void setFileTransferServer(String serverUrl) {
		setFileTransferServer(nativePtr, serverUrl);
	}

	private native String getFileTransferServer(long ptr);
	@Override
	public synchronized String getFileTransferServer() {
		return getFileTransferServer(nativePtr);
	}

	private native long createLocalPlayer(long nativePtr, AndroidVideoWindowImpl window);
	@Override
	public synchronized LinphonePlayer createLocalPlayer(AndroidVideoWindowImpl window) {
		long playerPtr = createLocalPlayer(nativePtr, window);
		if(playerPtr != 0) {
			return new LinphonePlayerImpl(playerPtr);
		} else {
			return null;
		}
	}

	private native void addListener(long nativePtr, LinphoneCoreListener listener);
	@Override
	public synchronized void addListener(LinphoneCoreListener listener) {
		addListener(nativePtr, listener);
	}

	private native void removeListener(long nativePtr, LinphoneCoreListener listener);
	@Override
	public synchronized void removeListener(LinphoneCoreListener listener) {
		removeListener(nativePtr, listener);
	}
	private native void setRemoteRingbackTone(long nativePtr, String file);
	@Override
	public void setRemoteRingbackTone(String file) {
		setRemoteRingbackTone(nativePtr, file);
	}
	private native String getRemoteRingbackTone(long nativePtr);
	@Override
	public String getRemoteRingbackTone() {
		return getRemoteRingbackTone(nativePtr);
	}

	private native void uploadLogCollection(long nativePtr);
	@Override
	public void uploadLogCollection() {
		uploadLogCollection(nativePtr);
	}

	@Override
	public native void resetLogCollection();

	private native void setPreferredFramerate(long nativePtr, float fps);
	@Override
	public void setPreferredFramerate(float fps) {
		setPreferredFramerate(nativePtr, fps);
	}
	private native float getPreferredFramerate(long nativePtr);
	@Override
	public float getPreferredFramerate() {
		return getPreferredFramerate(nativePtr);
	}


	private native int setAudioMulticastAddr(long nativePtr, String ip);
	@Override
	public void setAudioMulticastAddr(String ip) throws LinphoneCoreException {
		if (setAudioMulticastAddr(nativePtr, ip)!=0)
			throw new LinphoneCoreException("bad ip address ["+ip+"]");
	}
	private native int setVideoMulticastAddr(long nativePtr, String ip);
	@Override
	public void setVideoMulticastAddr(String ip) throws LinphoneCoreException {
		if (setVideoMulticastAddr(nativePtr, ip)!=0)
			throw new LinphoneCoreException("bad ip address ["+ip+"]");
	}
	private native String getAudioMulticastAddr(long ptr);
	@Override
	public String getAudioMulticastAddr() {
		return getAudioMulticastAddr(nativePtr) ;
	}
	private native String getVideoMulticastAddr(long ptr);
	@Override
	public String getVideoMulticastAddr() {
		return getVideoMulticastAddr(nativePtr);
	}
	private native int setAudioMulticastTtl(long ptr,int ttl);
	@Override
	public void setAudioMulticastTtl(int ttl) throws LinphoneCoreException {
		if (setAudioMulticastTtl(nativePtr, ttl)!=0)
			throw new LinphoneCoreException("bad ttl value ["+ttl+"]");

	}
	private native int setVideoMulticastTtl(long ptr,int ttl);
	@Override
	public void setVideoMulticastTtl(int ttl) throws LinphoneCoreException {
		if (setVideoMulticastTtl(nativePtr, ttl)!=0)
			throw new LinphoneCoreException("bad ttl value ["+ttl+"]");
	}
	private native int getAudioMulticastTtl(long ptr);
	@Override
	public int getAudioMulticastTtl() {
		return getAudioMulticastTtl(nativePtr);
	}
	private native int getVideoMulticastTtl(long ptr);
	@Override
	public int getVideoMulticastTtl() {
		return getVideoMulticastTtl(nativePtr);
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

	private native void enableDnsSrv(long ptr, boolean yesno);
	@Override
	public void enableDnsSrv(boolean yesno) {
		enableDnsSrv(nativePtr, yesno);
	}
	private native boolean dnsSrvEnabled(long ptr);
	@Override
	public boolean dnsSrvEnabled() {
		return dnsSrvEnabled(nativePtr);
	}

	private native void setVideoPreset(long nativePtr, String preset);
	@Override
	public void setVideoPreset(String preset) {
		setVideoPreset(nativePtr, preset);
	}

	private native String getVideoPreset(long nativePtr);
	@Override
	public String getVideoPreset() {
		return getVideoPreset(nativePtr);
	}
	private native long createCallParams(long nativePtr, long callPtr);
	@Override
	public LinphoneCallParams createCallParams(LinphoneCall call) {
		long callptr = 0;
		if (call!=null) callptr = ((LinphoneCallImpl)call).nativePtr;
		long ptr = createCallParams(nativePtr, callptr);
		return new LinphoneCallParamsImpl(ptr);
	}
	private native void setProvisioningUri(long nativePtr, String uri);
	@Override
	public void setProvisioningUri(String uri){
		setProvisioningUri(nativePtr, uri);
	}

	private native String getProvisioningUri(long nativePtr);
	@Override
	public String getProvisioningUri(){
		return getProvisioningUri(nativePtr);
	}
	private native int getGlobalState(long nativePtr);
	public GlobalState getGlobalState(){
		return GlobalState.fromInt(getGlobalState(nativePtr));
	}
	private native void setHttpProxyHost(long nativePtr, String host);
	@Override
	public void setHttpProxyHost(String host){
		setHttpProxyHost(nativePtr, host);
	}

	private native void setHttpProxyPort(long nativePtr, int port);
	@Override
	public void setHttpProxyPort(int port){
		setHttpProxyPort(nativePtr, port);
	}

	private native String getHttpProxyHost(long nativePtr);
	@Override
	public String getHttpProxyHost(){
		return getHttpProxyHost(nativePtr);
	}

	private native int getHttpProxyPort(long nativePtr);
	@Override
	public int getHttpProxyPort(){
		return getHttpProxyPort(nativePtr);
	}
	private native void setSipTransportTimeout(long nativePtr, int timeout_ms);
	@Override
	public void setSipTransportTimeout(int timeout_ms){
		setSipTransportTimeout(nativePtr, timeout_ms);
	}

	private native int getSipTransportTimeout(long nativePtr);
	@Override
	public int getSipTransportTimeout(){
		return getSipTransportTimeout(nativePtr);
	}
	private native void setNortpTimeout(long nativePtr, int timeout);
	@Override
	public void setNortpTimeout(int timeout){
		setNortpTimeout(nativePtr, timeout);
	}

	private native int getNortpTimeout(long nativePtr);
	@Override
	public int getNortpTimeout(){
		return getNortpTimeout(nativePtr);
	}

	private native void setSipNetworkReachable(long nativePtr, boolean isReachable);
	@Override
	public void setSipNetworkReachable(boolean isReachable) {
		setSipNetworkReachable(nativePtr, isReachable);
	}

	private native void setMediaNetworkReachable(long nativePtr, boolean isReachable);
	@Override
	public void setMediaNetworkReachable(boolean isReachable) {
		setMediaNetworkReachable(nativePtr, isReachable);
	}
	private native Object getMSFactory(long nativePtr);
	@Override
	public org.linphone.mediastream.Factory getMSFactory(){
		return (org.linphone.mediastream.Factory) getMSFactory(nativePtr);
	}
	private native void setDnsServers(long nativePtr, String servers[]);
	@Override
	public void setDnsServers(String servers[]){
		setDnsServers(nativePtr, servers);
	}

	private native void setUserCertificatesPath(long nativePtr, String path);
	@Override
	public void setUserCertificatesPath(String path) {
		setUserCertificatesPath(nativePtr, path);
	}

	public void reloadMsPlugins(String path) {
		reloadMsPlugins(nativePtr, path);
	}

	public void reloadSoundDevices() {
		reloadSoundDevices(nativePtr);
	}

	public void setDefaultSoundDevices() {
		setDefaultSoundDevices(nativePtr);
	}

	private native boolean isLimeEncryptionAvailable(long nativePtr);
	public synchronized boolean isLimeEncryptionAvailable() {
		return isLimeEncryptionAvailable(nativePtr);
	}

	private native void setLimeEncryption(long nativePtr, int value);
	public synchronized void setLimeEncryption(LinphoneLimeState lime) {
		setLimeEncryption(nativePtr, lime.mValue);
	}

	private native int getLimeEncryption(long nativePtr);
	public synchronized LinphoneLimeState getLimeEncryption() {
		return LinphoneLimeState.fromInt(getLimeEncryption(nativePtr));
	}

	private native String getTlsCertificate(long ptr);
	@Override
	public String getTlsCertificate() {
		return getTlsCertificate(nativePtr);
	}

	private native String getTlsKey(long ptr);
	@Override
	public String getTlsKey() {
		return getTlsKey(nativePtr);
	}

	private native String getTlsCertificatePath(long ptr);
	@Override
	public String getTlsCertificatePath() {
		return getTlsCertificatePath(nativePtr);
	}

	private native String getTlsKeyPath(long ptr);
	@Override
	public String getTlsKeyPath() {
		return getTlsKeyPath(nativePtr);
	}

	private native void setTlsCertificate(long ptr, String cert);
	@Override
	public void setTlsCertificate(String cert) {
		setTlsCertificate(nativePtr, cert);
	}

	private native void setTlsKey(long ptr, String key);
	@Override
	public void setTlsKey(String key) {
		setTlsKey(nativePtr, key);
	}

	private native void setTlsCertificatePath(long ptr, String path);
	@Override
	public void setTlsCertificatePath(String path) {
		setTlsCertificatePath(nativePtr, path);
	}

	private native void setTlsKeyPath(long ptr, String path);
	@Override
	public void setTlsKeyPath(String path) {
		setTlsKeyPath(nativePtr, path);
	}

	private native void setVerifyServerCertificates(long ptr, boolean enable);
	@Override
	public void setVerifyServerCertificates(boolean enable) {
		setVerifyServerCertificates(nativePtr, enable);
	}

	private native void setVerifyServerCN(long ptr, boolean enable);
	@Override
	public void setVerifyServerCN(boolean enable) {
		setVerifyServerCN(nativePtr, enable);
	}

	public void enableDownloadOpenH264(boolean enable) {
		openh264DownloadEnabled = enable;
	}

	public boolean downloadOpenH264Enabled() {
		return openh264DownloadEnabled;
	}

	public LinphoneFriend createFriend() {
		return (LinphoneFriend) createFriend(nativePtr);
	}

	public LinphoneFriend createFriendWithAddress(String address) {
		return (LinphoneFriend) createFriendWithAddress(nativePtr, address);
	}
}
