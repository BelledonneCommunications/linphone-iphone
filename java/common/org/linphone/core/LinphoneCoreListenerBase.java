package org.linphone.core;

import java.nio.ByteBuffer;

import org.linphone.core.LinphoneCall.State;
import org.linphone.core.LinphoneCore.EcCalibratorStatus;
import org.linphone.core.LinphoneCore.GlobalState;
import org.linphone.core.LinphoneCore.LogCollectionUploadState;
import org.linphone.core.LinphoneCore.RegistrationState;
import org.linphone.core.LinphoneCore.RemoteProvisioningState;

public class LinphoneCoreListenerBase implements LinphoneCoreListener {

	@Override
	public void authInfoRequested(LinphoneCore lc, String realm,
			String username, String Domain) {
		// TODO Auto-generated method stub

	}

	@Override
	public void callStatsUpdated(LinphoneCore lc, LinphoneCall call,
			LinphoneCallStats stats) {
		// TODO Auto-generated method stub

	}

	@Override
	public void newSubscriptionRequest(LinphoneCore lc, LinphoneFriend lf,
			String url) {
		// TODO Auto-generated method stub

	}

	@Override
	public void notifyPresenceReceived(LinphoneCore lc, LinphoneFriend lf) {
		// TODO Auto-generated method stub

	}

	@Override
	public void dtmfReceived(LinphoneCore lc, LinphoneCall call, int dtmf) {
		// TODO Auto-generated method stub

	}

	@Override
	public void notifyReceived(LinphoneCore lc, LinphoneCall call,
			LinphoneAddress from, byte[] event) {
		// TODO Auto-generated method stub

	}

	@Override
	public void transferState(LinphoneCore lc, LinphoneCall call,
			State new_call_state) {
		// TODO Auto-generated method stub

	}

	@Override
	public void infoReceived(LinphoneCore lc, LinphoneCall call,
			LinphoneInfoMessage info) {
		// TODO Auto-generated method stub

	}

	@Override
	public void subscriptionStateChanged(LinphoneCore lc, LinphoneEvent ev,
			SubscriptionState state) {
		// TODO Auto-generated method stub

	}

	@Override
	public void publishStateChanged(LinphoneCore lc, LinphoneEvent ev,
			PublishState state) {
		// TODO Auto-generated method stub

	}

	@Override
	public void show(LinphoneCore lc) {
		// TODO Auto-generated method stub

	}

	@Override
	public void displayStatus(LinphoneCore lc, String message) {
		// TODO Auto-generated method stub

	}

	@Override
	public void displayMessage(LinphoneCore lc, String message) {
		// TODO Auto-generated method stub

	}

	@Override
	public void displayWarning(LinphoneCore lc, String message) {
		// TODO Auto-generated method stub

	}

	@Override
	public void fileTransferProgressIndication(LinphoneCore lc,
			LinphoneChatMessage message, LinphoneContent content, int progress) {
		// TODO Auto-generated method stub

	}

	@Override
	public void fileTransferRecv(LinphoneCore lc, LinphoneChatMessage message,
			LinphoneContent content, byte[] buffer, int size) {
		// TODO Auto-generated method stub

	}

	@Override
	public int fileTransferSend(LinphoneCore lc, LinphoneChatMessage message,
			LinphoneContent content, ByteBuffer buffer, int size) {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public void globalState(LinphoneCore lc, GlobalState state, String message) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void registrationState(LinphoneCore lc, LinphoneProxyConfig cfg,
			RegistrationState state, String smessage) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void configuringStatus(LinphoneCore lc,
			RemoteProvisioningState state, String message) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void messageReceived(LinphoneCore lc, LinphoneChatRoom cr,
			LinphoneChatMessage message) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void callState(LinphoneCore lc, LinphoneCall call, State state,
			String message) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void callEncryptionChanged(LinphoneCore lc, LinphoneCall call,
			boolean encrypted, String authenticationToken) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void notifyReceived(LinphoneCore lc, LinphoneEvent ev,
			String eventName, LinphoneContent content) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void isComposingReceived(LinphoneCore lc, LinphoneChatRoom cr) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void ecCalibrationStatus(LinphoneCore lc, EcCalibratorStatus status,
			int delay_ms, Object data) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void uploadProgressIndication(LinphoneCore lc, int offset, int total) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void uploadStateChanged(LinphoneCore lc,
			LogCollectionUploadState state, String info) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void friendListCreated(LinphoneCore lc, LinphoneFriendList list) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void friendListRemoved(LinphoneCore lc, LinphoneFriendList list) {
		// TODO Auto-generated method stub
		
	}
}
