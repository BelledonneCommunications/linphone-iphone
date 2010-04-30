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
	/*
	 * linphone core states
	 */
	static public class 	GeneralState {
		  /* states for GSTATE_GROUP_POWER */
	static public GeneralState GSTATE_POWER_OFF = new GeneralState(0);        /* initial state */
	static public GeneralState GSTATE_POWER_STARTUP = new GeneralState(1);
	static public GeneralState GSTATE_POWER_ON = new GeneralState(2);
	static public GeneralState GSTATE_POWER_SHUTDOWN = new GeneralState(3);
		  /* states for GSTATE_GROUP_REG */
	static public GeneralState GSTATE_REG_NONE = new GeneralState(10);       /* initial state */
	static public GeneralState GSTATE_REG_OK  = new GeneralState(11);
	static public GeneralState GSTATE_REG_FAILED = new GeneralState(12);
		  /* states for GSTATE_GROUP_CALL */
	static public GeneralState GSTATE_CALL_IDLE = new GeneralState(20);      /* initial state */
	static public GeneralState GSTATE_CALL_OUT_INVITE = new GeneralState(21);
	static public GeneralState GSTATE_CALL_OUT_CONNECTED = new GeneralState(22);
	static public GeneralState GSTATE_CALL_IN_INVITE = new GeneralState(23);
	static public GeneralState GSTATE_CALL_IN_CONNECTED = new GeneralState(24);
	static public GeneralState GSTATE_CALL_END = new GeneralState(25);
	static public GeneralState GSTATE_CALL_ERROR = new GeneralState(26);
	static public GeneralState GSTATE_INVALID = new GeneralState(27);
	private final int mValue;
	static private Vector values = new Vector();
	
	private GeneralState(int value) {
		mValue = value;
		values.addElement(this);
	}
	public static GeneralState fromInt(int value) {
		
		for (int i=0; i<values.size();i++) {
			GeneralState state = (GeneralState) values.elementAt(i);
			if (state.mValue == value) return state;
		}
		throw new RuntimeException("sate not found ["+value+"]");
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
	public void invite(String destination)throws LinphoneCoreException;
	
	public void invite(LinphoneAddress to)throws LinphoneCoreException;
	
	public void terminateCall();
	/**
	 * get the remote address in case of in/out call
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
	 */
	public void acceptCall();
	
	
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
	public void setSoftPlayLevel(float gain);
	/**
	 * get play level before entering  sound card:  
	 * @return level in db
	 */
	public float getSoftPlayLevel();
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
	 * 
	 */
	public void clearCallLogs();
}
