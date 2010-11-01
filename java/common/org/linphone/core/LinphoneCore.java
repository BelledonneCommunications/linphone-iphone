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

	
public interface LinphoneCore {
	/**
	 * linphone core states
	 */
	static public class 	GlobalState {
		static private Vector values = new Vector();

		static public GlobalState GlobalOff = new GlobalState(0,"GlobalOff");       
		static public GlobalState GlobalStartup = new GlobalState(1,"GlobalStartup");
		static public GlobalState GlobalOn = new GlobalState(2,"GlobalOn");
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
	static public class 	RegistrationState {
		static private Vector values = new Vector();
		static public RegistrationState RegistrationNone = new RegistrationState(0,"RegistrationNone");       
		static public RegistrationState RegistrationProgress  = new RegistrationState(1,"RegistrationProgress");
		static public RegistrationState RegistrationOk = new RegistrationState(2,"RegistrationOk");
		static public RegistrationState RegistrationCleared = new RegistrationState(3,"RegistrationCleared");
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
	static public class Transport {
		public final static Transport udp =new Transport("udp");
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
	 * clear all added proxy config
	 */
	public void clearProxyConfigs();
	
	public void addProxyConfig(LinphoneProxyConfig proxyCfg) throws LinphoneCoreException;

	public void setDefaultProxyConfig(LinphoneProxyConfig proxyCfg);
	
	/**
	 * @return null if no default proxy config 
	 */
	public LinphoneProxyConfig getDefaultProxyConfig() ;
	
	/**
	 * clear all the added auth info
	 */
	void clearAuthInfos();
	
	void addAuthInfo(LinphoneAuthInfo info);
	
	/**
	 * Build an address according to the current proxy config. In case destination is not a sip address, the default proxy domain is automatically appended
	 * @param destination
	 * @return
	 * @throws If no LinphoneAddress can be built from destination
	 */
	public LinphoneAddress interpretUrl(String destination) throws LinphoneCoreException;
	
	/**
	 * Starts a call given a destination. Internally calls interpretUrl() then invite(LinphoneAddress).
	 * @param uri
	 */
	public LinphoneCall invite(String destination)throws LinphoneCoreException;
	
	public LinphoneCall invite(LinphoneAddress to)throws LinphoneCoreException;
	
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
	 * 
	 */
	public void clearCallLogs();
	
	
	/***
	 * get payload type  from mime type an clock rate
	 * 
	 * return null if not found
	 */
	public PayloadType findPayloadType(String mime,int clockRate); 
	
	public void enablePayloadType(PayloadType pt, boolean enable) throws LinphoneCoreException;
	
	public void enableEchoCancellation(boolean enable);
	
	public boolean isEchoCancellationEnabled();
	
	public void setSignalingTransport(Transport aTransport);
	
	public void enableSpeaker(boolean value);
	
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
	
	public void setVideoWindow(VideoWindow w);
	public void setPreviewWindow(VideoWindow w);
}
