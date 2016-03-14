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

import org.linphone.core.LinphoneCoreListener;
import org.linphone.mediastream.video.AndroidVideoWindowImpl;
import org.linphone.mediastream.video.capture.hwconf.AndroidCameraConfiguration;

import android.view.SurfaceView;

/**
 * Linphone core main object created by method {@link LinphoneCoreFactory#createLinphoneCore(LinphoneCoreListener, String, String, Object)}.
 *
 */

public interface LinphoneCore {
	/**
	 * linphone core states
	 */
	static public class GlobalState {

		static private Vector<GlobalState> values = new Vector<GlobalState>();
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
		/**
		 * Configuring
		 */
		static public GlobalState GlobalConfiguring = new GlobalState(4,"GlobalConfiguring");

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
	 * linphone remote provisioning states
	 */
	static public class RemoteProvisioningState {

		static private Vector<RemoteProvisioningState> values = new Vector<RemoteProvisioningState>();
		/**
		 * Off
		 */
		static public RemoteProvisioningState ConfiguringSuccessful = new RemoteProvisioningState(0,"ConfiguringSuccessful");
		/**
		 * Startup
		 */
		static public RemoteProvisioningState ConfiguringFailed = new RemoteProvisioningState(1,"ConfiguringFailed");
		/**
		 * On
		 */
		static public RemoteProvisioningState ConfiguringSkipped = new RemoteProvisioningState(2,"ConfiguringSkipped");

		private final int mValue;
		private final String mStringValue;


		private RemoteProvisioningState(int value,String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue=stringValue;
		}
		public static RemoteProvisioningState fromInt(int value) {

			for (int i=0; i<values.size();i++) {
				RemoteProvisioningState state = (RemoteProvisioningState) values.elementAt(i);
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

		private static Vector<RegistrationState> values = new Vector<RegistrationState>();
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

		static private Vector<FirewallPolicy> values = new Vector<FirewallPolicy>();
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
		/**
		 * Use ICE.
		 */
		static public FirewallPolicy UseIce = new FirewallPolicy(3,"UseIce");
		/**
		 * Use uPnP.
		 */
		static public FirewallPolicy UseUpnp = new FirewallPolicy(4,"UseUpnp");

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
	 * Linphone core SIP transport ports.
	 * Use with {@link LinphoneCore#setSignalingTransportPorts(Transports)}
	 * @ingroup initializing
	 */
	static public class Transports {
		/**
		 * udp port to listening on, negative value if not set
		 * */
		public int udp;
		/**
		 * tcp port to listening on, negative value if not set
		 * */
		public int tcp;
		/**
		 * tls port to listening on, negative value if not set
		 * */
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
	 * Stream type enum-like.
	 *
	 */
	static public final class StreamType {

		static private Vector<StreamType> values = new Vector<StreamType>();
		/**
		 * Audio
		 */
		static public final StreamType Audio = new StreamType(0, "Audio");
		/**
		 * Video
		 */
		static public final StreamType Video = new StreamType(1, "Video");
		/**
		 * Text
		 */
		static public final StreamType Text = new StreamType(2, "Text");
		/**
		 * Unknown
		 */
		static public final StreamType Unknown = new StreamType(3, "Unknown");
		protected final int mValue;
		private final String mStringValue;

		private StreamType(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}
		public static StreamType fromInt(int value) {
			for (int i = 0; i < values.size(); i++) {
				StreamType stype = (StreamType) values.elementAt(i);
				if (stype.mValue == value) return stype;
			}
			throw new RuntimeException("StreamType not found [" + value + "]");
		}
		public String toString() {
			return mStringValue;
		}
	}
	/**
	 * Stream type enum-like.
	 *
	 */
	static public final class MediaDirection {

		static private Vector<MediaDirection> values = new Vector<MediaDirection>();
		/**
		 * Invalid
		 */
		static public final MediaDirection Invalid = new MediaDirection(-1, "Invalid");
		/**
		 * Inactive
		 */
		static public final MediaDirection Inactive = new MediaDirection(0, "Inactive");
		/**
		 * SendOnly
		 */
		static public final MediaDirection SendOnly = new MediaDirection(1, "SendOnly");
		/**
		 * RecvOnly
		 */
		static public final MediaDirection RecvOnly = new MediaDirection(2, "RecvOnly");
		/**
		 * SendRecv
		 */
		static public final MediaDirection SendRecv = new MediaDirection(3, "SendRecv");
		protected final int mValue;
		private final String mStringValue;

		private MediaDirection(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}
		public static MediaDirection fromInt(int value) {
			for (int i = 0; i < values.size(); i++) {
				MediaDirection dir = (MediaDirection) values.elementAt(i);
				if (dir.mValue == value) return dir;
			}
			throw new RuntimeException("MediaDirection not found [" + value + "]");
		}
		public String toString() {
			return mStringValue;
		}
	}
	/**
	 * Media (RTP) encryption enum-like.
	 *
	 */
	static public final class MediaEncryption {

		static private Vector<MediaEncryption> values = new Vector<MediaEncryption>();
		/**
		 * None
		 */
		static public final MediaEncryption None = new MediaEncryption(0,"None");
		/**
		 * SRTP
		 */
		static public final MediaEncryption SRTP = new MediaEncryption(1,"SRTP");
		/**
		 * ZRTP
		 */
		static public final MediaEncryption ZRTP = new MediaEncryption(2,"ZRTP");
		/**
		 * DTLS
		 */
		static public final MediaEncryption DTLS = new MediaEncryption(3,"DTLS");
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
	static public final class AdaptiveRateAlgorithm {

		static private Vector<AdaptiveRateAlgorithm> values = new Vector<AdaptiveRateAlgorithm>();
		/**
		 * Simple
		 */
		static public final AdaptiveRateAlgorithm Simple = new AdaptiveRateAlgorithm(0,"Simple");
		/**
		 * Stateful
		 */
		static public final AdaptiveRateAlgorithm Stateful = new AdaptiveRateAlgorithm(1,"Stateful");
		protected final int mValue;
		private final String mStringValue;


		private AdaptiveRateAlgorithm(int value,String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue=stringValue;
		}
		public static AdaptiveRateAlgorithm fromString(String value) {

			for (int i=0; i<values.size();i++) {
				AdaptiveRateAlgorithm alg = (AdaptiveRateAlgorithm) values.elementAt(i);
				if (alg.mStringValue.equalsIgnoreCase(value)) return alg;
			}
			throw new RuntimeException("AdaptiveRateAlgorithm not found ["+value+"]");
		}
		public String toString() {
			return mStringValue;
		}
	}
	/**
	 * 	EC Calibrator Status
	 */
	static public class EcCalibratorStatus {

		static private Vector<EcCalibratorStatus> values = new Vector<EcCalibratorStatus>();
		/* Do not change the values of these constants or the strings associated with them to prevent breaking
		   the collection of echo canceller calibration results during the wizard! */
		public static final int IN_PROGRESS_STATUS=0;
		public static final int DONE_STATUS=1;
		public static final int FAILED_STATUS=2;
		public static final int DONE_NO_ECHO_STATUS=3;
		/**
		 * Calibration in progress
		 */
		static public EcCalibratorStatus InProgress = new EcCalibratorStatus(IN_PROGRESS_STATUS,"InProgress");
		/**
		 * Calibration done that produced an echo delay measure
		 */
		static public EcCalibratorStatus Done = new EcCalibratorStatus(DONE_STATUS,"Done");
		/**
		 * Calibration failed
		 */
		static public EcCalibratorStatus Failed = new EcCalibratorStatus(FAILED_STATUS,"Failed");
		/**
		 * Calibration done with no echo detected
		 */
		static public EcCalibratorStatus DoneNoEcho = new EcCalibratorStatus(DONE_NO_ECHO_STATUS, "DoneNoEcho");

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

	static public class UpnpState {
		static private Vector<UpnpState> values = new Vector<UpnpState>();
		/**
		 * Idle
		 */
		static public UpnpState Idle = new UpnpState(0, "Idle");
		/**
		 * Pending
		 */
		static public UpnpState Pending = new UpnpState(1, "Pending");
		/**
		 * Adding
		 */
		static public UpnpState Adding = new UpnpState(2, "Adding");
		/**
		 * Removing
		 */
		static public UpnpState Removing = new UpnpState(3, "Removing");
		/**
		 * Not Available
		 */
		static public UpnpState NotAvailable = new UpnpState(4, "Not available");
		/**
		 * Ok
		 */
		static public UpnpState Ok = new UpnpState(5, "Ok");
		/**
		 * Ko
		 */
		static public UpnpState Ko = new UpnpState(6, "Ko");
		/**
		 * Blacklisted
		 */
		static public UpnpState Blacklisted = new UpnpState(7, "Blacklisted");

		protected final int mValue;
		private final String mStringValue;

		private UpnpState(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue = stringValue;
		}
		public static UpnpState fromInt(int value) {
			for (int i = 0; i < values.size(); i++) {
				UpnpState mstate = (UpnpState) values.elementAt(i);
				if (mstate.mValue == value) return mstate;
			}
			throw new RuntimeException("UpnpState not found [" + value + "]");
		}
		public String toString() {
			return mStringValue;
		}
	}
	/**
	 * linphone log collection upload states
	 */
	static public class LogCollectionUploadState {

		static private Vector<LogCollectionUploadState> values = new Vector<LogCollectionUploadState>();
		/**
		 * Delivery in progress
		 */
		static public LogCollectionUploadState LogCollectionUploadStateInProgress = new LogCollectionUploadState(0,"LinphoneCoreLogCollectionUploadStateInProgress");
		/**
		 * Log collection upload successfully delivered and acknowledged by remote end point
		 */
		static public LogCollectionUploadState LogCollectionUploadStateDelivered = new LogCollectionUploadState(1,"LinphoneCoreLogCollectionUploadStateDelivered");
		/**
		 * Log collection upload was not delivered
		 */
		static public LogCollectionUploadState LogCollectionUploadStateNotDelivered = new LogCollectionUploadState(2,"LinphoneCoreLogCollectionUploadStateNotDelivered");

		private final int mValue;
		private final String mStringValue;

		private LogCollectionUploadState(int value, String stringValue) {
			mValue = value;
			values.addElement(this);
			mStringValue=stringValue;
		}
		public static LogCollectionUploadState fromInt(int value) {

			for (int i=0; i<values.size();i++) {
				LogCollectionUploadState state = (LogCollectionUploadState) values.elementAt(i);
				if (state.mValue == value) return state;
			}
			throw new RuntimeException("state not found ["+value+"]");
		}
		public String toString() {
			return mStringValue;
		}
	}

	/**
	 * Set the context of creation of the LinphoneCore.
	 */
	public void setContext(Object context);

	/**
	 * Get the LinphoneCore's global state.
	**/
	public GlobalState getGlobalState();

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
	 * Removes a proxy configuration.
	 * @param proxyCfg
	 */
	public void removeProxyConfig(LinphoneProxyConfig proxyCfg);

	/**
	 * Sets the default proxy.
	 *<br>
	 * This default proxy must be part of the list of already entered {@link LinphoneProxyConfig}.
	 * Toggling it as default will make LinphoneCore favor the identity associated with the proxy configuration in all incoming and outgoing calls.
	 * Better proxy configuration match may override this choice. Pass null to unset the default proxy.
	 * @param proxyCfg
	 */
	public void setDefaultProxyConfig(LinphoneProxyConfig proxyCfg);

	/**
	 * get he default proxy configuration, that is the one used to determine the current identity.
	 * @return null if no default proxy config
	 */
	public LinphoneProxyConfig getDefaultProxyConfig() ;

	/**
	 * Returns an array with all the auth infos stored in LinphoneCore
	 */
	LinphoneAuthInfo[] getAuthInfosList();

	/**
	 * Returns a matching auth info or null if no match found
	 */
	LinphoneAuthInfo findAuthInfo(String username, String realm, String domain);
	/**
	 * Removes a auth info.
	 * @param authInfo
	 */
	public void removeAuthInfo(LinphoneAuthInfo authInfo);

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
	 *<br>The LinphoneAddress can be constructed directly using {@link LinphoneCoreFactory#createLinphoneAddress} , or created {@link LinphoneCore#interpretUrl(String)}. .
	 * @param to the destination of the call (sip address).
	 * @return linphone call
	 * @throws LinphoneCoreException if linphone call cannot be created
	 */
	public LinphoneCall invite(LinphoneAddress to)throws LinphoneCoreException;
	/**
	 * Terminates a call.
	 * @param aCall to be terminated
	 */
	public void terminateCall(LinphoneCall aCall);
	/**
	 * Declines an incoming call, providing a reason for declining it.
	 * @param call the LinphoneCall, must be in the {@link LinphoneCall.State#IncomingReceived} state.
	 * @param reason the reason for rejecting the call: {@link Reason#Declined}  or {@link Reason#Busy}
	 */
	public void declineCall(LinphoneCall call, Reason reason);
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
	 * @return  true if there is a call running or pending.
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
	 * {@link LinphoneCoreListener#callState} listener method.
	 * The application can later respond positively to the call using
	 * this method.
	 * @throws LinphoneCoreException
	 */
	public void acceptCall(LinphoneCall aCall) throws LinphoneCoreException;

	/**
	 * Accept an incoming call.
	 *
	 * Basically the application is notified of incoming calls within the
	 * {@link LinphoneCoreListener#callState} listener method.
	 * The application can later respond positively to the call using
	 * this method.
	 * @throws LinphoneCoreException
	 */
	public void acceptCallWithParams(LinphoneCall aCall, LinphoneCallParams params) throws LinphoneCoreException;

	/**
	 * Accept call modifications initiated by other end.
	 *
	 * Basically the application is notified of incoming calls within the
	 * {@link LinphoneCoreListener#callState} listener method.
	 * The application can later respond positively to the call using
	 * this method.
	 * @throws LinphoneCoreException
	 */
	public void acceptCallUpdate(LinphoneCall aCall, LinphoneCallParams params) throws LinphoneCoreException;


	/**
	 * Prevent LinphoneCore from performing an automatic answer
	 *
	 * Basically the application is notified of incoming calls within the
	 * {@link LinphoneCoreListener#callState} listener method.
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
	 * @return the latest outgoing call log.
	 */
	public LinphoneCallLog getLastOutgoingCallLog();

	/**
	 * This method is called by the application to notify the Linphone core library when network is reachable.
	 * Calling this method with true trigger Linphone to initiate a registration process for all proxy
	 * configuration with parameter register set to enable.
	 * This method disable the automatic registration mode. It means you must call this method after each network state changes
	 * @param isReachable network state
	 *
	 */
	public void setNetworkReachable(boolean isReachable);
	/**
	 * Get network state has known by {@link LinphoneCore}
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
	 * @param number send dtmf ['0'..'9'] | '#', '*'
	 */
	void sendDtmf(char number);
	/**
	 * Initiate a dtmf signal to the speaker if not in call.
	 * Sending of the DTMF is done in another function.
	 * @param dtmf ['0'..'9'] | '#', '*'
	 * @param duration in ms , -1 for unlimited
	 */
	void playDtmf(char dtmf,int duration);
	/**
	 * stop current dtmf
	 */
	void stopDtmf();

	/**
	 * remove all call logs
	 */
	void clearCallLogs();




	/**
	 * Get payload type from mime type and clock rate
	 *
	 * This function searches in audio and video codecs for the given payload type name and clockrate.
	 * @param mime payload mime type (I.E SPEEX, PCMU, VP8)
	 * @param clockRate (I.E 8000, 16000, 90000, ...)
	 * @param channels  number of channels
	 * @return Returns null if not found.
	 */
	PayloadType findPayloadType(String mime, int clockRate, int channels);
	/***
	 * get payload type  from mime type and clock rate..
	 * Same as @{link {@link #findPayloadType(String, int, int)} but ignoring channels params
	 * @param mime payload mime type (I.E SPEEX, PCMU, VP8)
	 * @param clockRate (I.E 8000, 16000, 90000, ...)
	 * @return null if not found
	 */
	PayloadType findPayloadType(String mime, int clockRate);

	/***
	 * get payload type  from mime type
	 * Same as @{link {@link #findPayloadType(String, int, int)} but ignoring channels and clock rate params
	 * @param mime payload mime type (I.E SPEEX, PCMU, VP8)
	 * @return null if not found
	 */
	PayloadType findPayloadType(String mime);

	/**
	 * Enable payload type
	 * @param pt payload type to enable, can be retrieve from {@link #findPayloadType}
	 * @param enable for enable or disable the payload type
	 * @exception LinphoneCoreException
	 *
	 */
	void enablePayloadType(PayloadType pt, boolean enable) throws LinphoneCoreException;

	/**
	 * @param pt the payload type
	 * @return whether or not the payload is enabled in linphonecore.
	 */
	boolean isPayloadTypeEnabled(PayloadType pt);

	/**
	 * @param pt the payload type
	 * @return whether or not the payload epresents a VBR codec
	 */
	boolean payloadTypeIsVbr(PayloadType pt);

	/**
	 * Set an explicit bitrate (IP bitrate, not codec bitrate) for a given codec, in kbit/s.
	 * @param pt the payload type
	 * @param bitrate target IP bitrate in kbit/s
	 */
	void setPayloadTypeBitrate(PayloadType pt, int bitrate);

	/**
	 * Get target bitrate previously set by setPayloadTypeBitrate().
	 * @param pt
	 * @return IP bitrate in kbit/s
	 */
	int getPayloadTypeBitrate(PayloadType pt);

	/**
	 * Set an explicit bitrate (IP bitrate, not codec bitrate) for a given codec, in kbit/s.
	 * @param pt the payload type
	 * @param number target IP bitrate in kbit/s
	 */

	/**
	 * Force a number for a payload type. The LinphoneCore does payload type number assignment automatically. THis function is to be used mainly for tests, in order
	 * to override the automatic assignment mechanism.
	 * @param pt the payload type
	 * @param number
	 **/
	void setPayloadTypeNumber(PayloadType pt, int number);

	/**
	 * @param pt the payload type
	 * @return the payload type number assigned for this codec.
	 */
	int getPayloadTypeNumber(PayloadType pt);

	/**
	 * Enable adaptive rate control.
	 * @param enable
	 */
	void enableAdaptiveRateControl(boolean enable);

	/**
	 * Enables or disable adaptive rate control.
	 * @return true if adaptive rate control is enabled.
	*/
	boolean isAdaptiveRateControlEnabled();

	/**
	 *	Sets adaptive rate algorithm. It will be used for each new calls
	 *  starting from now. Calls already started will not be updated.
	 */
	void setAdaptiveRateAlgorithm(AdaptiveRateAlgorithm alg);

	/**
	 * Returns which adaptive rate algorithm is currently configured for
	 * future calls.
	*/
	AdaptiveRateAlgorithm getAdaptiveRateAlgorithm();

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
	 * Get echo limiter status (another method of doing echo suppression, more brute force)
	 * @return true if echo limiter is enabled
	 */
	boolean isEchoLimiterEnabled();
	/**
	 * Set transport ports linphone core will listen on
	 * @param local transports ports used for signaling (TCP, UDP and TLS)
	 */
	void setSignalingTransportPorts(Transports transports);
	/**Get
	 * @return transports used for signaling (TCP, UDP, TLS)
	 */
	Transports getSignalingTransportPorts();

	/**
	 * Assign a dscp value for the SIP socket.
	 * DSCP is an IP packet field used to indicate the type of routing service to routers.
	 * @param dscp
	 */
	void setSipDscp(int dscp);

	/**
	 * Set the timeout in milliseconds for SIP transport (TCP or TLS connection establishment maximum time).
	 * @param timeout_ms
	**/
	void setSipTransportTimeout(int timeout_ms);

	/**
	 * Get the current SIP transport timeout.
	 * @param timeout_ms
	**/
	int getSipTransportTimeout();

	/**
	 * Get DSCP used for SIP socket.
	 * @return the DSCP value used for the SIP socket.
	 */
	int getSipDscp();

	/**
	 * Activates or deactivates the speaker.
	 * @param value
	 */
	void enableSpeaker(boolean value);
	/**
	 * Tells whether the speaker is activated.
	 * @return true if speaker enabled, false otherwise
	 */
	boolean isSpeakerEnabled();
	/**
	 * add a friend to the current buddy list, if subscription attribute is set, a SIP SUBSCRIBE message is sent.
	 * @param lf LinphoenFriend to add
	 * @throws LinphoneCoreException
	 */
	void addFriend(LinphoneFriend lf) throws LinphoneCoreException;
	
	/**
	 * Adds the friend list to the linphone core.
	 */
	void addFriendList(LinphoneFriendList friendList) throws LinphoneCoreException;
	
	/**
	 * Removes the friend list from the linphone core.
	 */
	void removeFriendList(LinphoneFriendList friendList) throws LinphoneCoreException;
	
	/**
	 * Creates a friend list.
	 */
	LinphoneFriendList createLinphoneFriendList() throws LinphoneCoreException;

	/**
	 * Get list of LinphoneFriend
	 * @return LinphoneFriend list
	 */
	LinphoneFriend[] getFriendList();

	/**
	 * Get list of LinphoneFriendList
	 * @return LinphoneFriendList list
	 */
	LinphoneFriendList[] getFriendLists();

	/**
	 * @brief Set my presence status
	 * @param minutes_away how long in away
	 * @param alternative_contact sip uri used to redirect call in state LinphoneStatusMoved
	 * @param status OnlineStatus
	 * @deprecated Use setPresenceModel() instead
	 */
	void setPresenceInfo(int minutes_away, String alternative_contact, OnlineStatus status);
	/**
	 * @brief Get my presence status
	 * @return OnlineStatus
	 * @deprecated Use getPresenceModel() instead
	 */
	OnlineStatus getPresenceInfo();
	/**
	 * @brief Set my presence status
	 * @param presence #LinphonePresenceModel
	 */
	void setPresenceModel(PresenceModel presence);
	/**
	 * @brief Get my presence status
	 * @return A #PresenceModel object, or null if no presence model has been set.
	 */
	PresenceModel getPresenceModel();
	/**
	 * Create a new chat room for messaging from a sip uri like sip:joe@sip.linphone.org
	 * @param to 	destination address for messages
	 *
	 * @return {@link LinphoneChatRoom} where messaging can take place.
	 */
	LinphoneChatRoom getOrCreateChatRoom(String to);
	/**
	 * Create a new chat room for messaging from a linphone address
	 * @param to 	destination address for messages
	 *
	 * @return {@link LinphoneChatRoom} where messaging can take place.
	 */
	LinphoneChatRoom getChatRoom(LinphoneAddress to);
	/**
	 * Set the native video window id where the video is to be displayed.
	 * On Android, it must be of type {@link AndroidVideoWindowImpl}
	 * @param video window of type {@link AndroidVideoWindowImpl}
	**/
	void setVideoWindow(Object w);
	/**
	 * Set the native video window id where the video preview is to be displayed.
	 * On Android, it must of type {@link SurfaceView}
	 * @param video window of type {@link SurfaceView}
	**/
	void setPreviewWindow(Object w);
	/**
	 * Tells the core the device current orientation. This can be used by capture filters
	 * on mobile devices to select between portrait/landscape mode and to produce properly
	 * oriented images. The exact meaning of the value in rotation if left to each device
	 * specific implementations.
	 *@param rotation . Android supported values are 0, 90, 180 and 270.
	 *
	**/
	void setDeviceRotation(int rotation);
	/**
	 * Sets the active video device.
	 *
	 * @param id  of the video device as returned by {@link AndroidCameraConfiguration#retrieveCameras}
	**/
	void setVideoDevice(int id);
	/**
	 * Returns the id of the currently active video device as found in {@link AndroidCameraConfiguration#retrieveCameras}.
	**/
	int getVideoDevice();


	/**
	 * Teturns true if the underlying sdk support video
	 *
	 * */
	boolean isVideoSupported();

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
	 * Get STUN server
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
	/**
	 * Initiates an outgoing call given a destination LinphoneAddress
	 *
	 * @param addr the destination of the call {@link #LinphoneAddress }.
	 * @param params call parameters {@link #LinphoneCallParams }
	 *
	 *<br>The LinphoneAddress can be constructed directly using {@link LinphoneCoreFactory#createLinphoneAddress} , or created {@link LinphoneCore#interpretUrl(String)}. .
	 *
	 * @return a {@link #LinphoneCall LinphoneCall} object
	 * @throws LinphoneCoreException  in case of failure
	**/
	LinphoneCall inviteAddressWithParams(LinphoneAddress destination, LinphoneCallParams params) throws LinphoneCoreException ;
	/**
	 * Updates a running call according to supplied call parameters or parameters changed in the LinphoneCore.
	 *
	 * In this version this is limited to the following use cases:
	 * - setting up/down the video stream according to the video parameter of the {@link LinphoneCallParams} (see {@link LinphoneCallParams#enableVideo} ).
	 * - changing the size of the transmitted video after calling {@link LinphoneCore#setPreferredVideoSize(VideoSize)}
	 *
	 * In case no changes are requested through the {@link LinphoneCallParams} argument, then this argument can be omitted and set to null.
	 * @param call the {@link LinphoneCall} to be updated
	 * @param params the new  {@link  LinphoneCallParams call parameters} to use. (may be NULL)
	 * @return 0 if successful, -1 otherwise.
	**/
	int updateCall(LinphoneCall call, LinphoneCallParams params);

	/**
	 * Create a LinphoneCallParams suitable to be used for a new incoming call or an established call, in
	 * methods LinphoneCore.inviteAddressWithParams(), LinphoneCore.updateCall(), LinphoneCore.acceptCallWithParams(), LinphoneCore.acceptCallUpdate().
	 * The call parameter is optional: when creating a LinphoneCallParams for an outgoing call that is about to be created, 
	 * it shall be set to null.
	 * @param call (optional)
	 * @return a LinphoneCallParams object, representing the call settings guessed from the current LinphoneCore and compatible with the call object if any.
	 */
	LinphoneCallParams createCallParams(LinphoneCall call);
	
	/**
	 * Sets the path to a wav file used for ringing.
	 *
	 * @param path The file must be a wav 16bit linear. Local ring is disabled if null
	 */
	void setRing(String path);
	/**
	 * gets the path to a wav file used for ringing.
	 *
	 * @return null if not set
	 */
	String getRing();

	/**
	 * Sets file or folder containing trusted root CAs
	 *
	 * @param path path to file with multiple PEM certif or to folder with multiple PEM files
	 */
	void setRootCA(String path);

	/**
	 * Sets the path to a wav file used for for ringing back.
	 *
	 * Ringback means the ring that is heard when it's ringing at the remote party.
	 *
	 * @param path The file must be a wav 16bit linear.
	 */
	void setRingback(String path);

	/**
	 * Retrieve the maximum available upload bandwidth.
	**/
	int getUploadBandwidth();

	void setUploadBandwidth(int bw);

	/**
	 * Retrieve the maximum available download bandwidth.
	**/
	int getDownloadBandwidth();

	/**
	 * Sets maximum available download bandwidth
	 *
	 *
	 * This is IP bandwidth, in kbit/s.
	 * This information is used signaled to other parties during
	 * calls (within SDP messages) so that the remote end can have
	 * sufficient knowledge to properly configure its audio & video
	 * codec output bitrate to not overflow available bandwidth.
	 *
	 * @param bw the bandwidth in kbits/s, 0 for infinite
	 */
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
	/**
	 * Sets the preferred video size.
	 *
	 * This applies only to the stream that is captured and sent to the remote party,
	 * since we accept all standard video size on the receive path.
	 * @param vSize
	 *
	**/
	void setPreferredVideoSize(VideoSize vSize);
	/**
	 * Sets the preferred video size giving a known size name.
	 *
	 * This applies only to the stream that is captured and sent to the remote party,
	 * since we accept all standard video size on the receive path.
	 * @param name A known video name (eg. vga or 720p)
	**/
	void setPreferredVideoSizeByName(String name);
	/**
	 * get current preferred video size for sending.
	 * @return  video size
	 *
	**/
	VideoSize getPreferredVideoSize();

	/**
	 * Set the preferred frame rate for video.
	 * Based on the available bandwidth constraints and network conditions, the video encoder
	 * remains free to lower the framerate. There is no warranty that the preferred frame rate be the actual framerate.
	 * used during a call. Default value is 0, which means "use encoder's default fps value".
	 * @param fps the target frame rate in number of frames per seconds.
	**/
	void setPreferredFramerate(float fps);
	
	/**
	 * Returns the preferred video framerate, previously set by setPreferredFramerate().
	 * @return frame rate in number of frames per seconds.
	**/
	float getPreferredFramerate();
	/**
	 * Returns the currently supported audio codecs, as PayloadType elements
	 * @return
	 */
	PayloadType[] getAudioCodecs();
	/**
	 * Set the list of audio codecs.
	 * @param codecs List of PayloadType objects
	 */
	void setAudioCodecs(PayloadType[] codecs);
	/**
	 * Returns the currently supported video codecs, as PayloadType elements
	 * @return
	 */
	PayloadType[] getVideoCodecs();
	/**
	 * Set the list of video codecs.
	 * @param codecs List of PayloadType objects
	 */
	void setVideoCodecs(PayloadType[] codecs);
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
	 * @param listener the LinphoneEchoCalibrationListener to call when the calibration is done
	 * @throws LinphoneCoreException if operation is still in progress;
	**/
	void startEchoCalibration(LinphoneCoreListener listener) throws LinphoneCoreException;

	/**
	 * Returns true if echo calibration is recommended.
	 * If the device has a builtin echo canceller or calibration value is already known, it will return false.
	 */
	boolean needsEchoCalibration();
	
	/**
	 * Returns true if the software echo canceler needs to be turned on.
	 * If the device has a builtin echo canceller, it will return false.
	 */
	boolean hasBuiltInEchoCanceler();

	void enableIpv6(boolean enable);

	boolean isIpv6Enabled();

	/**
	 * @deprecated
	 * @param i
	 */
	void adjustSoftwareVolume(int i);

	/**
	 * Pauses a call. If a music file has been setup using {@link LinphoneCore#setPlayFile(String)},
	 * this file will be played to the remote user.
	 *
	**/
	boolean pauseCall(LinphoneCall call);
	/**
	 * Resume a call.
	**/
	boolean resumeCall(LinphoneCall call);
	/**
	 * Pause all currently running calls.
	**/
	boolean pauseAllCalls();

	void setZrtpSecretsCache(String file);
	void enableEchoLimiter(boolean val);

	/**
	 * Create a conference
	 * @param params Parameters of the conference. Can be null
	 * @return The new conference or null if the creation has failed
	 */
	LinphoneConference createConference(LinphoneConferenceParams params);
	/**
	 * Return the value of the C pointer on the conference instance.
	 *
	 * That function can be used to test whether a conference is running.
	 * @return A positive value if a conference is running, 0 if not.
	 **/
	LinphoneConference getConference();
	/**
	 * Indicates whether the local user is part of the conference.
	**/
	boolean isInConference();
	/**
	 * Moves the local participant inside the conference.
	 *
	 * Makes the local participant to join the conference.
	 * Typically, the local participant is by default always part of the conference when joining an active call into a conference.
	 * However, by calling {@link #leaveConference()} and {@link #enterConference()} the application can decide to temporarily
	 * move out and in the local participant from the conference.
	 *
	 * @returns true if successful
	**/
	boolean enterConference();
	/**
	 * Moves the local participant out of the conference.
	 * When the local participant is out of the conference, the remote participants can continue to talk normally.
	**/
	void leaveConference();
	/**
	 * Merge a call into a conference.
	 *
	 * If this is the first call that enters the conference, the virtual conference will be created automatically.
	 * If the local user was actively part of the call (ie not in paused state), then the local user is automatically entered into the conference.
	 * If the call was in paused state, then it is automatically resumed when entering into the conference.
	 * @param call an established call, either in {@link LinphoneCall.State#StreamsRunning} or {@link LinphoneCall.State#Paused} state.
	 *
	**/
	void addToConference(LinphoneCall call);
	/**
	 * Remove a call from the conference.
	 * @param call a call that has been previously merged into the conference.
	 *
	 * After removing the remote participant belonging to the supplied call, the call becomes a normal call in paused state.
	 * If one single remote participant is left alone together with the local user in the conference after the removal, then the conference is
	 * automatically transformed into a simple call in StreamsRunning state.
	 * The conference's resources are then automatically destroyed.
	 *
	 * In other words, unless {@link #leaveConference()} is explicitely called, the last remote participant of a conference is automatically
	 * put in a simple call in running state.
	 *
	 **/
	void removeFromConference(LinphoneCall call);
	/**
	 * Add all calls into a conference.
	 *
	 * Merge all established calls (either in {@link LinphoneCall.State#StreamsRunning} or {@link LinphoneCall.State#Paused}) into a conference.
	 *
	**/
	void addAllToConference();

	/**
	 * Terminates the conference and the calls associated with it.
	 *
	 * All the calls that were merged to the conference are terminated, and the conference resources are destroyed.
	 *
	**/
	void terminateConference();
	/**
	 * Returns the number of participants to the conference, including the local participant.
	 *
	 * Typically, after merging two calls into the conference, there is total of 3 participants:
	 * the local participant (or local user), and two remote participants that were the destinations of the two previously establised calls.
	 *
	 * @returns the number of participants to the conference
	**/
	int getConferenceSize();

	/**
	 * Request recording of the conference into a supplied file path.
	 * The format is wav.
	 * @param path where to write recording file
	**/
	void startConferenceRecording(String path);

	/**
	 * Stop recording of the conference.
	**/
	void stopConferenceRecording();
	/**
	 * Terminates all the calls.
	 */
	void terminateAllCalls();
	/**
	 * Returns all calls.
	 * @return an array with all call currently handle by Linphone core
	**/
	LinphoneCall[] getCalls();
	/**
	 * Get number of calls currently handled by Linphone core
	 * @returns number of calls
	 * */
	int getCallsNb();

	/**
	 * Performs a simple call transfer to the specified destination.
	 *
	 * @param call The current local call remains active and thus can be later paused or terminated.
	 * @param  referTo The remote call party endpoint is expected to issue a new call to this specified destination.
	**/
	void transferCall(LinphoneCall call, String referTo);
	/**
	 * Transfer a call to destination of another running call. This is used for "attended transfer" scenarios.
	 * The transfered call is supposed to be in paused state, so that it is able to accept the transfer immediately.
	 * The destination call is a call previously established to introduce the transfered person.
	 * This method will send a transfer request to the transfered person. The phone of the transfered is then
	 * expected to automatically call to the destination of the transfer. The receiver of the transfer will then automatically
	 * close the call with us (the 'dest' call).
	 * @param call a running call you want to transfer
	 * @param dest a running call whose remote person will receive the transfer
	**/
	void transferCallToAnother(LinphoneCall callToTransfer, LinphoneCall destination);

	/**
	 * Start a new call as a consequence of a transfer request received from a call.
	 * This function is for advanced usage: the execution of transfers is automatically managed by the LinphoneCore. However if an application
	 * wants to have control over the call parameters for the new call, it should call this function immediately during the LinphoneCallRefered notification.
	 * @param call a call that has just been notified about LinphoneCallRefered state event.
	 * @param params the call parameters to be applied to the new call.
	 * @return a LinphoneCall corresponding to the new call that is attempted to the transfer destination.
	**/
	LinphoneCall startReferedCall(LinphoneCall call, LinphoneCallParams params);
	/**
	 * Search from the list of current calls if a remote address match uri
	 * @param uri which should match call remote uri
	 * @return LinphoneCall or NULL is no match is found
	 */
	LinphoneCall findCallFromUri(String uri);
	/**
	 * Get the maximum number of simultaneous calls Linphone core can manage at a time. All new call above this limit are declined with a busy answer
	 * @return max number of simultaneous calls
	 */
	int getMaxCalls();
	/**
	 * Set the maximum number of simultaneous calls Linphone core can manage at a time. All new call above this limit are declined with a busy answer
	 * @param max number of simultaneous calls
	 */
	void setMaxCalls(int max);
	/**
	 * @deprecated
	 * @param uri
	 * @return
	 */
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
	
	
	enum TunnelMode {
		disable(0),
		enable(1),
		auto(2);
		private final int value;
		
		private TunnelMode(int value){
			this.value = value;
		}
		public static int enumToInt(TunnelMode enum_mode) {
			return enum_mode.value;
		}
		public static TunnelMode intToEnum(int value) {
			switch(value) {
			case 0: return disable;
			case 1: return enable;
			case 2: return auto;
			default: return disable;
			}
		}
	}
	
	/**
	 * @deprecated Use tunnelSetMode() instead
	 * Enable or disable tunnel
	 * @param enable True to enable and false to disable
	 */
	void tunnelEnable(boolean enable);
	
	/**
	 * Set the tunnel mode.
	 * The tunnel can be enable or disable by passing 'enable' or 'disable'.
	 * If the mode is set to 'auto', Linphone will try to establish an RTP session
	 * on the mirror port of the tunnel server. If the connection fails, the tunnel
	 * will be activated. 
	 * @param mode enable, disable or auto
	 */
	void tunnelSetMode(TunnelMode mode);
	
	/**
	 * Get the set mode
	 * @return 
	 */
	TunnelMode tunnelGetMode();
	
	/**
	 * Set whether sip packets must pass through the tunnel
	 * @param enable If true, tunnel will transport SIP packets in addition
	 * of RTP packets.
	 */
	void tunnelEnableSip(boolean enable);
	
	/**
	 * Check whether SIP tuneling is enabled
	 * @return true means the tunnel is set to transport SIP packets
	 */
	boolean tunnelSipEnabled();
	
	/**
	 * @deprecated Use tunnelSetMode instaead
	 * Enable tunnel if the mirror RTP session cannot be established
	 */
	void tunnelAutoDetect();
	
	/**
	 * Clean the list of server
	 */
	void tunnelCleanServers();
	
	/**
	 * Set an optional HTTP proxy
	 * @param proxy_host
	 * @param port 
	 * @param username 
	 * @param password
	 */
	void tunnelSetHttpProxy(String proxy_host, int port, String username, String password);
	/**
	 * @param host tunnel server ip address
	 * @param port tunnel server tls port, recommended value is 443
	 * @param udpMirrorPort remote port on the tunnel server side  used to test udp reachability
	 * @param roundTripDelay udp packet round trip delay in ms considered as acceptable. recommended value is 1000 ms
	 */
	void tunnelAddServerAndMirror(String host, int port, int udpMirrorPort, int roundTripDelay);
	/**
	 * Add a server to the list of tunnel servers.
	 * @param config Parameters of the server to add.
	 */
	void tunnelAddServer(TunnelConfig config);
	/**
	 * Returns a list of configured servers.
	 * @return Array of server configs.
	 */
	TunnelConfig[] tunnelGetServers();

	boolean isTunnelAvailable();
	/**
	 * Returns an unmodifiable list of entered proxy configurations.
	 * @return list of proxy config
	**/
	LinphoneProxyConfig[] getProxyConfigList();
	/**
	 * Sets the default policy for video.
	 * This policy defines whether:
	 * @param autoInitiate video shall be initiated by default for outgoing calls
	 * @param autoAccept video shall be accepter by default for incoming calls
	**/
	void setVideoPolicy(boolean autoInitiate, boolean autoAccept);

	/**
	 * Gets the policy for the autoInitiate video
	 */
	boolean getVideoAutoInitiatePolicy();

	/**
	 * Gets the policy for the autoAccept video
	 */
	boolean getVideoAutoAcceptPolicy();

	/** Set static picture to be used when "Static picture" is the video device
	 * @param path to the static picture file
	 * */
	void setStaticPicture(String path);
	/**
	 * Sets the user agent string used in SIP messages.
	 * @param user agent name
	 * @param user agent version
	**/
	void setUserAgent(String name, String version);
	/**
	 * Set the number of cores used for media processing
	 * */
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

	/**
	 * remove a linphone friend from linphone core and linphonerc
	 */
	void removeFriend(LinphoneFriend lf);

	/**
	 * return a linphone friend (if exists) that matches the sip address
	 */
	LinphoneFriend findFriendByAddress(String sipUri);

	/**
	 * Sets the UDP port used for audio streaming.
	**/
	void setAudioPort(int port);

	/**
	 * Sets the UDP port range from which to randomly select the port used for audio streaming.
	 */
	void setAudioPortRange(int minPort, int maxPort);

	/**
	 * Assign a DSCP value to the audio RTP sockets.
	 * @param dscp the DSCP value.
	 * DSCP is an IP header field used to indicate a type of service to routers.
	 */
	void setAudioDscp(int dscp);

	/**
	 * Return DSCP value used for the audio RTP sockets.
	 * @return the DSCP value used for the audio RTP sockets.
	 */
	int getAudioDscp();

	/**
	 * Sets the UDP port used for video streaming.
	**/
	void setVideoPort(int port);

	/**
	 * Sets the UDP port range from which to randomly select the port used for video streaming.
	 */
	void setVideoPortRange(int minPort, int maxPort);

	/**
	 * Assign a DSCP value to the video RTP sockets.
	 * @param dscp the DSCP value.
	 * DSCP is an IP header field used to indicate a type of service to routers.
	 */
	void setVideoDscp(int dscp);

	/**
	 * Return DSCP value used for the video RTP sockets.
	 * @return the DSCP value used for the video RTP sockets.
	 */
	int getVideoDscp();

	/**
	 * Set the incoming call timeout in seconds.
	 * If an incoming call isn't answered for this timeout period, it is
	 * automatically declined.
	**/
	void setIncomingTimeout(int timeout);

	/**
	 * Set the call timeout in seconds.
	 * Once this time is elapsed (ringing included), the call is automatically hung up.
	**/
	void setInCallTimeout(int timeout);
	/**
	 * Allow to control microphone level:
	 * @param gain in db
	**/
	void setMicrophoneGain(float gain);

	/**
	 * Set address to use if no LinphoneProxyConfig configured
	 */
	void setPrimaryContact(String address);

	/**
	 * Returns the address used if no LinphoneProxyConfig configured
	 */
	String getPrimaryContact();

	/**
	 * Set username and display name to use if no LinphoneProxyConfig configured
	 */
	void setPrimaryContact(String displayName, String username);

	/**
	 * Returns the username used if no LinphoneProxyConfig configured
	 */
	String getPrimaryContactUsername();

	/**
	 * Returns the display name used if no LinphoneProxyConfig configured
	 */
	String getPrimaryContactDisplayName();

	/**
	 * Enable/Disable the use of SIP INFO for DTMFs
	 */
	void setUseSipInfoForDtmfs(boolean use);

	/**
	 * Returns the state of use of SIP INFO for DTMFs
	 */
	boolean getUseSipInfoForDtmfs();

	/**
	 * Enable/Disable the use of inband DTMFs
	 */
	void setUseRfc2833ForDtmfs(boolean use);

	/**
	 * Returns the state of use of inband DTMFs
	 */
	boolean getUseRfc2833ForDtmfs();

	/**
	 * @return returns LpConfig object to read/write to the config file: usefull if you wish to extend
	 * the config file with your own sections
	 */
	LpConfig getConfig();


	/**
	 * Return the availability of uPnP.
	 *
	 * @return true if uPnP is available otherwise return false.
	 */
	public boolean upnpAvailable();

	/**
	 * Return the internal state of uPnP.
	 *
	 * @return an UpnpState.
	 */
	public UpnpState getUpnpState();

	/**
	 * Return the external ip address of router.
	 * In some cases the uPnP can have an external ip address but not a usable uPnP
	 * (state different of Ok).
	 *
	 * @return a null terminated string containing the external ip address. If the
	 * the external ip address is not available return null.
	 */
	public String getUpnpExternalIpaddress();

	/**
	 * Create an empty INFO message.
	 * It can later be sent using {@link LinphoneCall.sendInfoMessage() }.
	 * @return the new info message.
	 */
	public LinphoneInfoMessage createInfoMessage();

	/**
	 * Sends an outgoing subscription for a resource with given event, expiration period, and content.
	 * The state changes of the new subscriptions can be followed thanks to { @link LinphoneCoreListener.subscriptionStateChanged() } and
	 * { @link LinphoneCoreListener.notifyReceived }.
	 * @param resource the address of the resource for which the event needs to be monitored.
	 * @param event the event name, as specified in the event package RFC.
	 * @param expires the expiration period in seconds.
	 * @param content optional content of the subscription.
	 * @return a LinphoneEvent representing the subscription context.
	 */
	public LinphoneEvent subscribe(LinphoneAddress resource, String event, int expires, LinphoneContent content);

	/**
	 * Create an outgoing subscription, specifying the destination resource, the event name, and an optional content body.
	 * If accepted, the subscription runs for a finite period, but is automatically renewed if not terminated before.
	 * Unlike linphone_core_subscribe() the subscription isn't sent immediately. It will be send when calling linphone_event_send_subscribe().
	 * @param resource the destination resource
	 * @param event the event name
	 * @param expires the whished duration of the subscription
	 * @param body an optional body, may be NULL.
	 * @return a LinphoneEvent holding the context of the created subcription.
	 */
	public LinphoneEvent createSubscribe(LinphoneAddress resource, String event, int expires);

	/**
	 * Create a publish context for an event state.
	 * After being created, the publish must be sent using linphone_event_send_publish().
	 * After expiry, the publication is refreshed unless it is terminated before.
	 * @param resource the resource uri for the event
	 * @param event the event name
	 * @param expires the lifetime of the publication
	 * @return the LinphoneEvent holding the context of the publish.
	 */
	public LinphoneEvent createPublish(LinphoneAddress resource, String event, int expires);

	/**
	 * Publish an event.
	 * After the initial publication, updates can be done with { @link LinphoneEvent.updatePublish() }
	 * @param resource the resource to which the event belongs.
	 * @param event the event name as specified in the event package RFC.
	 * @param expires valid time for the event.
	 * @param content content of the publish.
	 * @return a LinphoneEvent representing the publish context.
	 */
	public LinphoneEvent publish(LinphoneAddress resource, String event, int expires, LinphoneContent content);

	/**
	 * Sets the path to the database where the chat messages will be stored (if enabled)
	 * @param path the database where the chat messages will be stored.
	 */
	public void setChatDatabasePath(String path);

	/**
	 * Sets the path to the database where the logs will be stored (if enabled)
	 * @param path the database where the logs will be stored.
	 */
	public void setCallLogsDatabasePath(String path);

	/**
	 * Sets the path to the database where the friends will be stored (if enabled)
	 * @param path the database where the friends will be stored.
	 */
	public void setFriendsDatabasePath(String path);

	/**
	 * Gets the chat rooms
	 * @return an array of LinphoneChatRoom
	 */
	public LinphoneChatRoom[] getChatRooms();

	/**
	 * Gets the linphonecore supported resolutions for video
	 * @return an array of String
	 */
	public String[] getSupportedVideoSizes();

	/**
	 * Migrate configuration so that all SIP transports are enabled.
	 * Versions of linphone < 3.7 did not support using multiple SIP transport simultaneously.
	 * This function helps application to migrate the configuration so that all transports are enabled.
	 * Existing proxy configuration are added a transport parameter so that they continue using the unique transport that was set previously.
	 * This function must be used just after creating the core, before any call to linphone_core_iterate()
	 * @returns 1 if migration was done, 0 if not done because unnecessary or already done, -1 in case of error.
	 */
	public int migrateToMultiTransport();

	/**
	 * Migrates the call logs from the linphonerc to the database if not done yet
	 **/
	public void migrateCallLogs();

	/**
	 * When receiving an incoming, accept to start a media session as early-media.
	 * This means the call is not accepted but audio & video streams can be established if the remote party supports early media.
	 * However, unlike after call acceptance, mic and camera input are not sent during early-media, though received audio & video are played normally.
	 * The call can then later be fully accepted using linphone_core_accept_call() or linphone_core_accept_call_with_params().
	 * @param lc the linphonecore
	 * @param call the call
	 * @param params the call params, can be NULL.
	 * @return true if successful, false otherwise.
	 */
	public boolean acceptEarlyMedia(LinphoneCall call);

	/**
	 * Accept an early media session for an incoming call.
	 * This is identical as calling linphone_core_accept_early_media_with_params() with NULL call parameters.
	 * @see linphone_core_accept_early_media_with_params()
	 * @param call the incoming call
	 * @param params
	 * @return true if successful, false otherwise.
	 */
	public boolean acceptEarlyMediaWithParams(LinphoneCall call, LinphoneCallParams params);

	/**
	 * Creates a proxy config using the default values if they exists
	 * @return a default proxy config
	 */
	public LinphoneProxyConfig createProxyConfig();
	public LinphoneProxyConfig createProxyConfig(String identity,String proxy,String route, boolean enableRegister) throws LinphoneCoreException;

	/**
	 * Assign an audio file to played locally upon call failure, for a given reason.
	 * @param reason the #LinphoneReason representing the failure error code.
	 * @param path a wav file to be played when such call failure happens.
	 */
	public void setCallErrorTone(Reason reason, String path);

	/**
	 * Assign an audio file to be played locally in replacement of common telephony tone.
	 * This is typically used to internationalize tones.
	 * @param id a tone id
	 * @param wav a path to a 16 bit PCM linear wav file.
	 */
	public void setTone(ToneID id, String wavfile);

	/**
	 * Inform the core about the maximum transmission unit of the network.
	 * This is used for fragmenting video RTP packets to a size compatible with the network.
	 * @param mtu the MTU in bytes.
	 */
	public void setMtu(int mtu);
	/**
	 * Returns the mtu value previously set by setMtu().
	 *
	 * @return the MTU in bytes.
	 */
	public int getMtu();
	/**
	Control when media offer is sent in SIP INVITE.
	 * @param enable true if INVITE has to be sent whitout SDP.
	 * */
	public void enableSdp200Ack(boolean enable);
	/**
	 * Media offer control param for SIP INVITE.
	 * @return true if INVITE has to be sent whitout SDP.
	 */
	public boolean isSdp200AckEnabled();

	/**
	 * Inconditionnaly disable incoming chat messages.
	 * @param lc the core
	 * @param deny_reason the deny reason (using ReasonNone has no effect).
	**/
	public void disableChat(Reason denycode);

	/**
	 * Enable reception of incoming chat messages.
	 * By default it is enabled but it can be disabled with linphone_core_disable_chat().
	 * @param lc the core
	**/
	public void enableChat();


	/**
	 * Returns whether chat is enabled.
	 * @return true if chat is enabled, false otherwise.
	**/
	public boolean chatEnabled();

	/**
	 * Whenever the liblinphone is playing a ring to advertise an incoming call or ringback of an outgoing call, this function stops the ringing.
	 * Typical use is to stop ringing when the user requests to ignore the call.
	**/
	public void stopRinging();

	/**
	 * Set audio jitter buffer size in milliseconds.
	 * A value of zero disables the jitter buffer.
	 * The new value is taken into account immediately for all running or pending calls.
	 * @param value the jitter buffer size in milliseconds.
	 */
	public void setAudioJittcomp(int value);

	/**
	 * Set video jitter buffer size in milliseconds.
	 * A value of zero disables the jitter buffer.
	 * The new value is taken into account immediately for all running or pending calls.
	 * @param value the jitter buffer size in milliseconds.
	 */
	public void setVideoJittcomp(int value);
	
	/**
	 * Globaly set an http file transfer server to be used for content type application/vnd.gsma.rcs-ft-http+xml.
	 * @param serverUrl URL of the file server like https://file.linphone.org/upload.php
	 */
	public void setFileTransferServer(String serverUrl);
	
	/**
	 * Get the globaly set http file transfer server to be used for content type application/vnd.gsma.rcs-ft-http+xml.
	 * @return the serverUrl
	 */
	public String getFileTransferServer();
	
	/**
	 * Create a media player
	 * @return An object that implement LinphonePlayer
	 */
	public LinphonePlayer createLocalPlayer(AndroidVideoWindowImpl window);
	
	/**
	 * Adds a new listener to be called by the core
	 * @param listener to add
	 */
	public void addListener(LinphoneCoreListener listener);
	
	/**
	 * Removes a listener previously added with addListener
	 * @param listener to remove
	 */
	public void removeListener(LinphoneCoreListener listener);
	
	/**
	 * Specifies a ring back tone to be played to far end during incoming calls, when early media is requested.
	 * @param file
	 */
	public void setRemoteRingbackTone(String file);

	/**
	 * Return the ringback tone file used when doing early media. It may be null.
	 * @return the ringback tone file path.
	 */
	String getRemoteRingbackTone();
	
	/**
	 * Upload the log collection to the configured server url.
	 */
	public void uploadLogCollection();

	/**
	 * Reset the log collection by removing the log files.
	 */
	public void resetLogCollection();
	
	
	/**
	 * Use to set multicast address to be used for audio stream.
	 * @param ip an ipv4/6 multicast address
	 * @return 
	 * @thow LinphoneCoreException
	**/
	public void setAudioMulticastAddr(String ip) throws LinphoneCoreException;
	/**
	 * Use to set multicast address to be used for video stream.
	 * @param ip an ipv4/6 multicast address
	 * @thow LinphoneCoreException
	**/
	public void setVideoMulticastAddr(String ip) throws LinphoneCoreException;

	/**
	 * Use to get multicast address to be used for audio stream.
	 * @return an ipv4/6 multicast address or default value
	**/
	public String getAudioMulticastAddr();

	/**
	 * Use to get multicast address to be used for video stream.
	 * @return an ipv4/6 multicast address, or default value
	**/
	public String getVideoMulticastAddr();

	/**
	 * Use to set multicast ttl to be used for audio stream.
	 * @param ttl value or -1 if not used. [0..255] default value is 1
	 * @thow LinphoneCoreException
	**/
	public void setAudioMulticastTtl(int ttl)  throws LinphoneCoreException;
	/**
	 * Use to set multicast ttl to be used for video stream.
	 * @param  ttl value or -1 if not used. [0..255] default value is 1
	 * @thow LinphoneCoreException
	**/
	public void setVideoMulticastTtl(int ttl) throws LinphoneCoreException;

	/**
	 * Use to get multicast ttl to be used for audio stream.
	 * @return a time to leave value
	 * @thow LinphoneCoreException
	 * 	**/
	public int getAudioMulticastTtl();

	/**
	 * Use to get multicast ttl to be used for video stream.
	 * @return a time to leave value
	 * @thow LinphoneCoreException
	**/
	public int getVideoMulticastTtl();


	/**
	 * Use to enable multicast rtp for audio stream.
	 * * If enabled, outgoing calls put a multicast address from {@link linphone_core_get_video_multicast_addr} into audio cline. In case of outgoing call audio stream is sent to this multicast address.
	 * <br> For incoming calls behavior is unchanged.
	 * @param yesno if yes, subsequent calls will propose multicast ip set by {@link linphone_core_set_audio_multicast_addr}
	**/
	public void enableAudioMulticast(boolean yesno);

	/**
	 * Use to get multicast state of audio stream.
	 * @return true if  subsequent calls will propose multicast ip set by {@link linphone_core_set_audio_multicast_addr}
	**/
	public  boolean audioMulticastEnabled();

	/**
	 * Use to enable multicast rtp for video stream.
	 * If enabled, outgoing calls put a multicast address from {@link linphone_core_get_video_multicast_addr} into video cline. In case of outgoing call video stream is sent to this  multicast address.
	 * <br> For incoming calls behavior is unchanged.
	 * @param yesno if yes, subsequent outgoing calls will propose multicast ip set by {@link linphone_core_set_video_multicast_addr}
	**/
	public void enableVideoMulticast(boolean yesno);
	/**
	 * Use to get multicast state of video stream.
	 * @return true if  subsequent calls will propose multicast ip set by {@link linphone_core_set_video_multicast_addr}
	**/
	public  boolean videoMulticastEnabled();

	/**
	 * Enable or disable DNS SRV resolution.
	 * @param yesno true to enable DNS SRV resolution, false to disable it.
	 */
	public void enableDnsSrv(boolean yesno);

	/**
	 * Tells whether DNS SRV resolution is enabled.
	 * @return true if DNS SRV resolution is enabled, false if disabled.
	 */
	public boolean dnsSrvEnabled();

	/**
	 * Set the video preset to be used for video calls.
	 * @param lc LinphoneCore object
	 * @param preset The name of the video preset to be used (can be null to use the default video preset).
	 */
	public void setVideoPreset(String preset);

	/**
	 * Get the video preset used for video calls.
	 * @param lc LinphoneCore object
	 * @return The name of the video preset used for video calls (can be null if the default video preset is used).
	 */
	public String getVideoPreset();

	/**
	 * Set a provisioning URI to fetch an xml linphonerc config file from, at next LinphoneCore instantiation.
	**/
	public void setProvisioningUri(String uri);

	/**
	 * Get the provisioning URI previously set.
    **/
	public String getProvisioningUri();

	/**
	 * Set an http proxy hostname or IP address to use for SIP connection.
	 */
	public void setHttpProxyHost(String host);
	/**
	 * Set an http proxy port to use for SIP connection.
	 */
	public void setHttpProxyPort(int port);
	/**
	 * Get the http proxy host previously set.
	**/
	public String getHttpProxyHost();
	/**
	 * Get the http proxy port previously set.
	**/
	public int getHttpProxyPort();

	/*
	 * Set the nortp timeout (timeout after which call is closed if no RTP or RTCP packets are received).
	 */
	public void setNortpTimeout(int seconds);

	/**
	 * Get the nortp timeout.
	 * @return
	 */
	public int getNortpTimeout();

	/**
	 * This method is called by the application to notify the linphone core library when the SIP network is reachable.
	 * This is for advanced usage, when SIP and RTP layers are required to use different interfaces.
	 * Most applications just need LinphoneCore.setNetworkReachable().
	 * @param isReachable whether SIP network is reachable.
	 */
	public void setSipNetworkReachable(boolean isReachable);
	
	/**
	 * This method is called by the application to notify the linphone core library when the media (RTP) network is reachable.
	 * This is for advanced usage, when SIP and RTP layers are required to use different interfaces.
	 * Most applications just need LinphoneCore.setNetworkReachable().
	 * @param isReachable whether media network is reachable.
	 */
	public void setMediaNetworkReachable(boolean isReachable);

	/**
	 * Returns the mediastreamer2 (media stack) factory object used by the LinphoneCore. This may be useful to perform
	 * some advanced media-related configuration options.
	**/
	org.linphone.mediastream.Factory getMSFactory();

	/**
	 * Overrides the list of DNS servers to use.
	 * Passing null restores the default behaviour, which is to use the dns server list provided by the system.
	**/
	public void setDnsServers(String servers[]);

	/**
	 * Set user certificates directory path (used by SRTP-DTLS).
	 */
	public void setUserCertificatesPath(String path);

	/**
	 * Reload mediastreamer2 plugins from specified directory.
	 * @param path The path from where plugins are to be loaded.
	**/
	public void reloadMsPlugins(String path);
}
