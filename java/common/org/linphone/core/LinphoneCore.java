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

/**
 * Linphone core main object created by method {@link LinphoneCoreFactory#createLinphoneCore(LinphoneCoreListener, String, String, Object)}.	
 *
 */
public interface LinphoneCore {
	/**
	 * linphone core states
	 */
	static public class 	GlobalState {
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
	static public class 	RegistrationState {
		static private Vector values = new Vector();
		/**
		 * None
		 */
		static public RegistrationState RegistrationNone = new RegistrationState(0,"RegistrationNone");       
		/**
		 * In Progress
		 */
		static public RegistrationState RegistrationProgress  = new RegistrationState(1,"RegistrationProgress");
		/**
		 * Ok
		 */
		static public RegistrationState RegistrationOk = new RegistrationState(2,"RegistrationOk");
		/**
		 * Cleared
		 */
		static public RegistrationState RegistrationCleared = new RegistrationState(3,"RegistrationCleared");
		/**
		 * Failed
		 */
		static public RegistrationState RegistrationFailed = new RegistrationState(4,"RegistrationFailed");
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
	static public class 	FirewallPolicy {
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
	 * Signaling transports 
	 *
	 */
	static public class Transport {
		/**
		 * UDP transport
		 */
		public final static Transport udp =new Transport("udp");
		/**
		 * TCP transport
		 */
		public final static Transport tcp =new Transport("tcp");
		private final String mStringValue;

		private Transport(String stringValue) {
			mStringValue=stringValue;
		}
		public String toString() {
			return mStringValue;
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
	 * @return a list of LinphoneCallLog 
	 */
	public Vector getCallLogs();
	
	/**
	 * This method is called by the application to notify the Linphone core library when network is reachable.
	 * Calling this method with true trigger Linphone to initiate a registration process for all proxy
	 * configuration with parameter register set to enable.
	 * This method disable the automatic registration mode. It means you must call this method after each network state changes
	 * @param network state  
	 *
	 */
	public void setNetworkStateReachable(boolean isReachable);
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
	public void muteMic(boolean isMuted);
	/**
	 * 
	 * @return true is mic is muted
	 */
	public boolean isMicMuted();
	
	/**
	 * Initiate a dtmf signal if in call
	 * @param number
	 */
	public void sendDtmf(char number);
	/**
	 * Initiate a dtmf signal to the speqker if not in call
	 * @param number
	 * @param duration in ms , -1 for unlimited
	 */
	public void playDtmf(char number,int duration);
	/**
	 * stop current dtmf
	 */
	public void stopDtmf();
	
	/**
	 * remove all call logs
	 */
	public void clearCallLogs();
	/***
	 * get payload type  from mime type an clock rate
	 * 
	 * return null if not found
	 */
	public PayloadType findPayloadType(String mime,int clockRate); 
	/**
	 * not implemented yet
	 * @param pt
	 * @param enable
	 * @throws LinphoneCoreException
	 */
	public void enablePayloadType(PayloadType pt, boolean enable) throws LinphoneCoreException;
	/**
	 * Enables or disable echo cancellation.
	 * @param enable
	 */
	public void enableEchoCancellation(boolean enable);
	/**
	 * get EC status 
	 * @return true if echo cancellation is enabled.
	 */
	public boolean isEchoCancellationEnabled();
	/**
	 * not implemented yet
	 * @param aTransport
	 */
	public void setSignalingTransport(Transport aTransport);
	/**
	 * not implemented
	 * @param value
	 */
	public void enableSpeaker(boolean value);
	/**
	 * not implemented
	 * @return
	 */
	public boolean isSpeakerEnabled();
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
	
	public void setVideoWindow(Object w);
	public void setPreviewWindow(Object w);
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
	public void setStunServer(String stun_server);
	/**
	 * @return stun server address if previously set.
	 */
	public String getStunServer();
	
	/**
	 * Sets policy regarding workarounding NATs
	 * @param pol one of the FirewallPolicy members.
	**/
	public void setFirewallPolicy(FirewallPolicy pol);
	/**
	 * @return previously set firewall policy.
	 */
	public FirewallPolicy getFirewallPolicy();

	public LinphoneCall inviteAddressWithParams(LinphoneAddress destination, LinphoneCallParams params) throws LinphoneCoreException ;
	
	public int updateCall(LinphoneCall call, LinphoneCallParams params);

	public LinphoneCallParams createDefaultCallParameters();

	/**
	 * Sets the path to a wav file used for ringing.
	 *
	 * @param path The file must be a wav 16bit linear. Local ring is disabled if null
	 */
	public void setRing(String path);
	/**
	 * gets the path to a wav file used for ringing.
	 *
	 * @param null if not set
	 */
	public String getRing();
	public void setUploadBandwidth(int bw);

	public void setDownloadBandwidth(int bw);

	public void setPreferredVideoSize(VideoSize vSize);
	
	public VideoSize getPreferredVideoSize();
}
