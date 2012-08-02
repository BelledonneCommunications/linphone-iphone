/*
LinphoneCore.java
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

import java.util.Vector;

import org.linphone.core.LinphoneCallLog;
import org.linphone.core.LinphoneCallParams;

/**
 * Linphone core main object created by method {@link LinphoneCoreFactory#createLinphoneCore(LinphoneCoreListener, String, String, Object)}.	
 *
 */

public interface LinphoneCore {
	/**
	 * linphone core states
	 */
	static public class GlobalState {
		
		static private Vector values = new Vector();
		/**
		 * Off
		 */
		static public GlobalState GlobalOff = new GlobalState(0,"GlobalOff");       
		/**
		 * Startup
		 */
		static public GlobalState GlobalStartup = new GlobalState(1,"GlobalStartup");
		/**
		 * On
		 */
		static public GlobalState GlobalOn = new GlobalState(2,"GlobalOn");
		/**
		 * Shutdown
		 */
		static public GlobalState GlobalShutdown = new GlobalState(3,"GlobalShutdown");

		private final int mValue;
		private final String mStringValue;

		
		private GlobalState(int value,String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue=stringValue;
		}
		public static GlobalState fromInt(int value) {

			for (int i=0; i<values.size();i++) {
				GlobalState state = (GlobalState) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("state not found ["+value+"]");
		}
		public String toString() {
			return mStringValue;
		}
	}
	/**
	 * Describes proxy registration states.
	 *
	 */
	static public class RegistrationState {
		
		private static Vector values = new Vector();
		/**
		 * None
		 */
		public static RegistrationState RegistrationNone = new RegistrationState(0,"RegistrationNone");       
		/**
		 * In Progress
		 */
		public static RegistrationState RegistrationProgress  = new RegistrationState(1,"RegistrationProgress");
		/**
		 * Ok
		 */
		public static RegistrationState RegistrationOk = new RegistrationState(2,"RegistrationOk");
		/**
		 * Cleared
		 */
		public static RegistrationState RegistrationCleared = new RegistrationState(3,"RegistrationCleared");
		/**
		 * Failed
		 */
		public static RegistrationState RegistrationFailed = new RegistrationState(4,"RegistrationFailed");
		private final int mValue;
		private final String mStringValue;

		
		private RegistrationState(int value,String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue=stringValue;
		}
		public static RegistrationState fromInt(int value) {

			for (int i=0; i<values.size();i++) {
				RegistrationState state = (RegistrationState) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("state not found ["+value+"]");
		}
		public String toString() {
			return mStringValue;
		}
	}
	/**
	 * Describes firewall policy.
	 *
	 */
	static public class FirewallPolicy {
		
		static private Vector values = new Vector();
		/**
		 * No firewall is assumed.
		 */
		static public FirewallPolicy NoFirewall = new FirewallPolicy(0,"NoFirewall");       
		/**
		 * Use NAT address (discouraged)
		 */
		static public FirewallPolicy UseNatAddress  = new FirewallPolicy(1,"UseNatAddress");
		/**
		 * Use stun server to discover RTP addresses and ports.
		 */
		static public FirewallPolicy UseStun = new FirewallPolicy(2,"UseStun");
		
		private final int mValue;
		private final String mStringValue;

		
		private FirewallPolicy(int value,String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue=stringValue;
		}
		public static FirewallPolicy fromInt(int value) {

			for (int i=0; i<values.size();i++) {
				FirewallPolicy state = (FirewallPolicy) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("state not found ["+value+"]");
		}
		public String toString() {
			return mStringValue;
		}
		public int value(){
			return mValue;
		}
	}
	
	/**
	 * Signaling transports ports.
	 */
	static public class Transports {
		public int udp;
		public int tcp;
		public int tls;
		
		public Transports() {};
		public Transports(Transports t) {
			this.udp = t.udp;
			this.tcp = t.tcp;
			this.tls = t.tls;
		}
		public String toString() {
			return "udp["+udp+"] tcp["+tcp+"] tls["+tls+"]";
		}
	}
	/**
	 * Media (RTP) encryption enum-like.
	 *
	 */
	static public class MediaEncryption {
		
		static private Vector values = new Vector();
		/**
		 * None
		 */
		static public MediaEncryption None = new MediaEncryption(0,"None");       
		/**
		 * SRTP
		 */
		static public MediaEncryption SRTP = new MediaEncryption(1,"SRTP");
		/**
		 * ZRTP
		 */
		static public MediaEncryption ZRTP = new MediaEncryption(2,"ZRTP");
		protected final int mValue;
		private final String mStringValue;

		
		private MediaEncryption(int value,String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue=stringValue;
		}
		public static MediaEncryption fromInt(int value) {

			for (int i=0; i<values.size();i++) {
				MediaEncryption menc = (MediaEncryption) values.elementAt(i);
				if (menc.mValue == value) return menc;
			}
			throw new RuntimeException("MediaEncryption not found ["+value+"]");
		}
		public String toString() {
			return mStringValue;
		}
	}
	/**
	 * 	EC Calibrator Status
	 */
	static public class EcCalibratorStatus {
		
		static private Vector values = new Vector();
		public static final int IN_PROGRESS_STATUS=0;
		public static final int DONE_STATUS=1;
		public static final int FAILED_STATUS=2;
		/**
		 * Calibration in progress
		 */
		static public EcCalibratorStatus InProgress = new EcCalibratorStatus(IN_PROGRESS_STATUS,"InProgress");       
		/**
		 * Calibration done
		 */
		static public EcCalibratorStatus Done  = new EcCalibratorStatus(DONE_STATUS,"Done");
		/**
		 * Calibration in progress
		 */
		static public EcCalibratorStatus Failed = new EcCalibratorStatus(FAILED_STATUS,"Failed");

		private final int mValue;
		private final String mStringValue;

		
		private EcCalibratorStatus(int value,String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue=stringValue;
		}
		public static EcCalibratorStatus fromInt(int value) {

			for (int i=0; i<values.size();i++) {
				EcCalibratorStatus status = (EcCalibratorStatus) values.elementAt(i);
				if (status.mValue == value) return status;
			}
			throw new RuntimeException("status not found ["+value+"]");
		}
		public String toString() {
			return mStringValue;
		}
		public int value(){
			return mValue;
		}
	}
	/**
	 * clear all added proxy configs
	 */
	public void clearProxyConfigs();
	/**
	 * Add a proxy configuration. This will start registration on the proxy, if registration is enabled.
	 * @param proxyCfg
	 * @throws LinphoneCoreException
	 */
	public void addProxyConfig(LinphoneProxyConfig proxyCfg) throws LinphoneCoreException;
	/**
	 * Sets the default proxy.
	 *<br>
	 * This default proxy must be part of the list of already entered {@link LinphoneProxyConfig}. 
	 * Toggling it as default will make LinphoneCore use the identity associated with the proxy configuration in all incoming and outgoing calls.
	 * @param proxyCfg 
	 */
	public void setDefaultProxyConfig(LinphoneProxyConfig proxyCfg);
	
	/**
	 * get he default proxy configuration, that is the one used to determine the current identity.
	 * @return null if no default proxy config 
	 */
	public LinphoneProxyConfig getDefaultProxyConfig() ;
	
	/**
	 * clear all the added auth info
	 */
	void clearAuthInfos();
	/**
	 * Adds authentication information to the LinphoneCore.
	 * <br>This information will be used during all SIP transacations that require authentication.
	 * @param info
	 */
	void addAuthInfo(LinphoneAuthInfo info);
	
	/**
	 * Build an address according to the current proxy config. In case destination is not a sip address, the default proxy domain is automatically appended
	 * @param destination
	 * @return
	 * @throws If no LinphoneAddress can be built from destination
	 */
	public LinphoneAddress interpretUrl(String destination) throws LinphoneCoreException;
	
	/**
	 * Starts a call given a destination. Internally calls {@link #interpretUrl(String)} then {@link #invite(LinphoneAddress)}.
	 * @param uri
	 */
	public LinphoneCall invite(String destination)throws LinphoneCoreException;
	/**
	 * Initiates an outgoing call given a destination LinphoneAddress
	 *<br>The LinphoneAddress can be constructed directly using linphone_address_new(), or created by linphone_core_interpret_url(). The application doesn't own a reference to the returned LinphoneCall object. Use linphone_call_ref() to safely keep the LinphoneCall pointer valid within your application.
	 * @param to the destination of the call (sip address).
	 * @return LinphoneCall
	 * @throws LinphoneCoreException
	 */
	public LinphoneCall invite(LinphoneAddress to)throws LinphoneCoreException;
	/**
	 * Terminates a call.
	 * @param aCall to be terminated
	 */
	public void terminateCall(LinphoneCall aCall);
	/**
	 * Returns The LinphoneCall the current call if one is in call
	 *
	**/
	public LinphoneCall getCurrentCall(); 
	
	/**
	 * get current call remote address in case of in/out call
	 * @return null if no call engaged yet
	 */
	public LinphoneAddress getRemoteAddress();
	/**
	 *  
	 * @return  TRUE if there is a call running or pending.
	 */
	public boolean isIncall();
	/**
	 * 
	 * @return Returns true if in incoming call is pending, ie waiting for being answered or declined.
	 */
	public boolean isInComingInvitePending();
	/**
	 * Main loop function. It is crucial that your application call it periodically.
	 *
	 *	#iterate() performs various backgrounds tasks:
	 * <li>receiving of SIP messages
	 * <li> handles timers and timeout
	 * <li> performs registration to proxies
	 * <li> authentication retries The application MUST call this function from periodically, in its main loop. 
	 * <br> Be careful that this function must be call from the same thread as other liblinphone methods. In not the case make sure all liblinphone calls are serialized with a mutex.

	 */
	public void iterate();
	/**
	 * Accept an incoming call.
	 *
	 * Basically the application is notified of incoming calls within the
	 * {@link LinphoneCoreListener#inviteReceived(LinphoneCore, String)} listener.
	 * The application can later respond positively to the call using
	 * this method.
	 * @throws LinphoneCoreException 
	 */
	public void acceptCall(LinphoneCall aCall) throws LinphoneCoreException;
	
	/**
	 * Accept an incoming call.
	 *
	 * Basically the application is notified of incoming calls within the
	 * {@link LinphoneCoreListener#inviteReceived(LinphoneCore, String)} listener.
	 * The application can later respond positively to the call using
	 * this method.
	 * @throws LinphoneCoreException 
	 */
	public void acceptCallWithParams(LinphoneCall aCall, LinphoneCallParams params) throws LinphoneCoreException;
	
	/**
	 * Accept call modifications initiated by other end.
	 *
	 * Basically the application is notified of incoming calls within the
	 * {@link LinphoneCoreListener#inviteReceived(LinphoneCore, String)} listener.
	 * The application can later respond positively to the call using
	 * this method.
	 * @throws LinphoneCoreException 
	 */
	public void acceptCallUpdate(LinphoneCall aCall, LinphoneCallParams params) throws LinphoneCoreException;
	
	
	/**
	 * Prevent LinphoneCore from performing an automatic answer
	 *
	 * Basically the application is notified of incoming calls within the
	 * {@link LinphoneCoreListener#inviteReceived(LinphoneCore, String)} listener.
	 * The application can later respond positively to the call using
	 * this method.
	 * @throws LinphoneCoreException 
	 */
	public void deferCallUpdate(LinphoneCall aCall) throws LinphoneCoreException;
	
	/**
	 * @return a list of LinphoneCallLog 
	 */
	public LinphoneCallLog[] getCallLogs();
	
	/**
	 * This method is called by the application to notify the Linphone core library when network is reachable.
	 * Calling this method with true trigger Linphone to initiate a registration process for all proxy
	 * configuration with parameter register set to enable.
	 * This method disable the automatic registration mode. It means you must call this method after each network state changes
	 * @param network state  
	 *
	 */
	public void setNetworkReachable(boolean isReachable);
	/**
	 * 
	 * @return if false, there is no network connection.
	 */
	public boolean isNetworkReachable();
	/**
	 * destroy linphone core and free all underlying resources
	 */
	public void destroy();
	/**
	 * Allow to control play level before entering  sound card:  
	 * @param level in db
	 */
	public void setPlaybackGain(float gain);
	/**
	 * get play level before entering  sound card:  
	 * @return level in db
	 */
	public float getPlaybackGain();
	/**
	 * set play level
	 * @param level [0..100]
	 */
	public void setPlayLevel(int level);
	/**
	 * get playback level [0..100];
	 * -1 if not cannot be determined
	 * @return
	 */
	public int getPlayLevel();
	/**
	 *  Mutes or unmutes the local microphone.
	 * @param isMuted
	 */
	void muteMic(boolean isMuted);
	/**
	 * 
	 * @return true is mic is muted
	 */
	boolean isMicMuted();
	
	/**
	 * Initiate a dtmf signal if in call
	 * @param number
	 */
	void sendDtmf(char number);
	/**
	 * Initiate a dtmf signal to the speaker if not in call.
	 * Sending of the DTMF is done in another function.
	 * @param number
	 * @param duration in ms , -1 for unlimited
	 */
	void playDtmf(char number,int duration);
	/**
	 * stop current dtmf
	 */
	void stopDtmf();
	
	/**
	 * remove all call logs
	 */
	void clearCallLogs();
	/***
	 * get payload type  from mime type an clock rate
	 * 
	 * return null if not found
	 */
	PayloadType findPayloadType(String mime,int clockRate); 
	/**
	 * not implemented yet
	 * @param pt
	 * @param enable
	 * @throws LinphoneCoreException
	 */
	void enablePayloadType(PayloadType pt, boolean enable) throws LinphoneCoreException;
	/**
	 * Enables or disable echo cancellation.
	 * @param enable
	 */
	void enableEchoCancellation(boolean enable);
	/**
	 * get EC status 
	 * @return true if echo cancellation is enabled.
	 */
	boolean isEchoCancellationEnabled();
	/**
	 * Get echo limiter status (another method of doing echo suppressionn, more brute force)
	 * @return true if echo limiter is enabled
	 */
	boolean isEchoLimiterEnabled();
	/**
	 * @param transports used for signaling (TCP, UDP and TLS)
	 */
	void setSignalingTransportPorts(Transports transports);
	/**
	 * @return transports used for signaling (TCP, UDP, TLS)
	 */
	Transports getSignalingTransportPorts();
	/**
	 * not implemented
	 * @param value
	 */
	void enableSpeaker(boolean value);
	/**
	 * not implemented
	 * @return
	 */
	boolean isSpeakerEnabled();
	/**
	 * add a friend to the current buddy list, if subscription attribute is set, a SIP SUBSCRIBE message is sent.
	 * @param lf LinphoenFriend to add
	 * @throws LinphoneCoreException
	 */
	void addFriend(LinphoneFriend lf) throws LinphoneCoreException;

	/**
	 * Set my presence status
	 * @param minute_away how long in away
	 * @param status sip uri used to redirect call in state LinphoneStatusMoved
	 */
	void setPresenceInfo(int minute_away,String alternative_contact, OnlineStatus status);
	/**
	 * Create a new chat room for messaging from a sip uri like sip:joe@sip.linphone.org
	 * @param to 	destination address for messages 
	 *
	 * @return {@link LinphoneChatRoom} where messaging can take place.
	 */
	LinphoneChatRoom createChatRoom(String to);
	
	void setVideoWindow(Object w);
	void setPreviewWindow(Object w);
	void setDeviceRotation(int rotation);
	
	void setVideoDevice(int id);
	int getVideoDevice();
	
	/**
	 * Enables video globally.
	 *
	 * 
	 * This function does not have any effect during calls. It just indicates #LinphoneCore to
	 * initiate future calls with video or not. The two boolean parameters indicate in which
	 * direction video is enabled. Setting both to false disables video entirely.
	 *
	 * @param vcap_enabled indicates whether video capture is enabled
	 * @param display_enabled indicates whether video display should be shown
	 *
	**/
	void enableVideo(boolean vcap_enabled, boolean display_enabled);
	/**
	 * Returns TRUE if video is enabled, FALSE otherwise.
	 *	
	 ***/
	boolean isVideoEnabled();
	
	/**
	 * Specify a STUN server to help firewall traversal.
	 * @param stun_server Stun server address and port, such as stun.linphone.org or stun.linphone.org:3478
	 */
	void setStunServer(String stun_server);
	/**
	 * @return stun server address if previously set.
	 */
	String getStunServer();
	
	/**
	 * Sets policy regarding workarounding NATs
	 * @param pol one of the FirewallPolicy members.
	**/
	void setFirewallPolicy(FirewallPolicy pol);
	/**
	 * @return previously set firewall policy.
	 */
	FirewallPolicy getFirewallPolicy();

	LinphoneCall inviteAddressWithParams(LinphoneAddress destination, LinphoneCallParams params) throws LinphoneCoreException ;
	
	int updateCall(LinphoneCall call, LinphoneCallParams params);

	LinphoneCallParams createDefaultCallParameters();

	/**
	 * Sets the path to a wav file used for ringing.
	 *
	 * @param path The file must be a wav 16bit linear. Local ring is disabled if null
	 */
	void setRing(String path);
	/**
	 * gets the path to a wav file used for ringing.
	 *
	 * @param null if not set
	 */
	String getRing();
	
	/**
	 * Sets file or folder containing trusted root CAs
	 *
	 * @param path path to file with multiple PEM certif or to folder with multiple PEM files
	 */	
	void setRootCA(String path);
	
	void setUploadBandwidth(int bw);

	void setDownloadBandwidth(int bw);
	
	/**
	 * Sets audio packetization interval suggested for remote end.
	 * @param ptime packetization interval in milliseconds
	 */
	void setDownloadPtime(int ptime);
	
	/**
	 * Sets audio packetization interval sent to remote end.
	 * @param ptime packetization interval in milliseconds
	 */
	void setUploadPtime(int ptime);

	void setPreferredVideoSize(VideoSize vSize);
	
	VideoSize getPreferredVideoSize();
	
	/**
	 * Returns the currently supported audio codecs, as PayloadType elements
	 * @return
	 */
	PayloadType[] getAudioCodecs();
	/**
	 * Returns the currently supported video codecs, as PayloadType elements
	 * @return
	 */
	PayloadType[] getVideoCodecs();
	/**
	 * enable signaling keep alive. small udp packet sent periodically to keep udp NAT association
	 */
	void enableKeepAlive(boolean enable);
	/**
	 * get keep elive mode
	 * @return true if enable
 	 */
	boolean isKeepAliveEnabled();
	/**
	 * Start an echo calibration of the sound devices, in order to find adequate settings for the echo canceler automatically.
	 * status is notified to {@link LinphoneCoreListener#ecCalibrationStatus(EcCalibratorStatus, int, Object)}
	 * @param User object
	 * @throws LinphoneCoreException if operation is still in progress;
	**/
	void startEchoCalibration(Object data) throws LinphoneCoreException;

	void enableIpv6(boolean enable);
	void adjustSoftwareVolume(int i);
	
	boolean pauseCall(LinphoneCall call);
	boolean resumeCall(LinphoneCall call);
	boolean pauseAllCalls();
	
	void setZrtpSecretsCache(String file);
	void enableEchoLimiter(boolean val);

	boolean isInConference();
	boolean enterConference();
	void leaveConference();

	void addToConference(LinphoneCall call);
	void addAllToConference();
	void removeFromConference(LinphoneCall call);

	void terminateConference();
	int getConferenceSize();
	
	void terminateAllCalls();
	LinphoneCall[] getCalls();
	int getCallsNb();


	void transferCall(LinphoneCall call, String referTo);
	void transferCallToAnother(LinphoneCall callToTransfer, LinphoneCall destination);

	LinphoneCall findCallFromUri(String uri);

	int getMaxCalls();
	void setMaxCalls(int max);
	boolean isMyself(String uri);

	/**
	 * Use this method to check the calls state and forbid proposing actions
	 * which could result in an active call.
	 * Eg: don't start a new call if one is in outgoing ringing.
	 * Eg: don't merge to conference either as it could result
	 *     in two active calls (conference and accepted call). 
	 * @return
	 */
	boolean soundResourcesLocked();
	/**
	 * Returns whether given media encryption is supported by liblinphone.
	 */
	boolean mediaEncryptionSupported(MediaEncryption menc);
	/**
	 * set media encryption (rtp) to use
	 * @params menc: MediaEncryption.None, MediaEncryption.SRTP or MediaEncryption.ZRTP
	 */
	void setMediaEncryption(MediaEncryption menc);
	/**
	 * return selected media encryption
	 * @return MediaEncryption.None, MediaEncryption.SRTP or MediaEncryption.ZRTP
	 */
	MediaEncryption getMediaEncryption();
/**
	 * Set media encryption required for outgoing calls
	 */
	void setMediaEncryptionMandatory(boolean yesno);
	/**
	 * @return if media encryption is required for outgoing calls
	 */
	boolean isMediaEncryptionMandatory();

	/**
	 * @param path path to music file played to remote side when on hold.
	 */
	void setPlayFile(String path);
	void tunnelEnable(boolean enable);
	void tunnelAutoDetect();
	void tunnelCleanServers();
	void tunnelSetHttpProxy(String proxy_host, int port, String username, String password);
	/**
	 * @param host tunnel server ip address
	 * @param port tunnel server tls port, recommended value is 443
	 * @param udpMirrorPort remote port on the tunnel server side  used to test udp reachability
	 * @param roundTripDelay udp packet round trip delay in ms considered as acceptable. recommended value is 1000 ms
	 */
	void tunnelAddServerAndMirror(String host, int port, int udpMirrorPort, int roundTripDelay);

	boolean isTunnelAvailable();
	
	LinphoneProxyConfig[] getProxyConfigList();
	
	void setVideoPolicy(boolean autoInitiate, boolean autoAccept);
	
	void setUserAgent(String name, String version);
	
	void setCpuCount(int count);
	
	/**
	 * remove a call log
	 */
	public void removeCallLog(LinphoneCallLog log);
	
	/**
	 * @return count of missed calls
	 */
	public int getMissedCallsCount();
	
	/**
	 * Set missed calls count to zero
	 */
	public void resetMissedCallsCount();
	/**
	 * re-initiates registration if network is up.
	 */
	public void refreshRegisters();

	/**
	 * return the version code of linphone core
	 */
	public String getVersion();
}
