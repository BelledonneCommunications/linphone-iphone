/*
TutorialRegistration.java
Copyright (C) 2010  Belledonne Communications SARL 

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
package org.linphone.core.tutorials;

import java.nio.ByteBuffer;

import org.linphone.core.LinphoneAddress;
import org.linphone.core.LinphoneCall;
import org.linphone.core.LinphoneCall.State;
import org.linphone.core.LinphoneCallStats;
import org.linphone.core.LinphoneChatMessage;
import org.linphone.core.LinphoneChatRoom;
import org.linphone.core.LinphoneContent;
import org.linphone.core.LinphoneCore;
import org.linphone.core.LinphoneCore.EcCalibratorStatus;
import org.linphone.core.LinphoneCore.GlobalState;
import org.linphone.core.LinphoneCore.LogCollectionUploadState;
import org.linphone.core.LinphoneCore.RegistrationState;
import org.linphone.core.LinphoneCore.RemoteProvisioningState;
import org.linphone.core.LinphoneCoreException;
import org.linphone.core.LinphoneCoreFactory;
import org.linphone.core.LinphoneCoreListener;
import org.linphone.core.LinphoneEvent;
import org.linphone.core.LinphoneFriend;
import org.linphone.core.LinphoneFriendList;
import org.linphone.core.LinphoneInfoMessage;
import org.linphone.core.LinphoneProxyConfig;
import org.linphone.core.PublishState;
import org.linphone.core.SubscriptionState;


/**
 * This program is a _very_ simple usage example of liblinphone.
 * Demonstrating how to initiate a SIP registration from a sip uri identity
 * passed from the command line.
 *
 * First argument must be like sip:jehan@sip.linphone.org, second must be password.
 * <br>
 * ex registration sip:jehan@sip.linphone.org secret
 *
 * Ported from registration.c
 *
 * @author Guillaume Beraudo
 *
 */
public class TutorialRegistration implements LinphoneCoreListener {
	private boolean running;
	private TutorialNotifier TutorialNotifier;


	public TutorialRegistration(TutorialNotifier TutorialNotifier) {
		this.TutorialNotifier = TutorialNotifier;
	}

	public TutorialRegistration() {
		this.TutorialNotifier = new TutorialNotifier();
	}

	
	/*
	 * Registration state notification listener
	 */
	public void registrationState(LinphoneCore lc, LinphoneProxyConfig cfg,RegistrationState cstate, String smessage) {
		write(cfg.getIdentity() + " : "+smessage);
	}

	public void show(LinphoneCore lc) {}
	public void byeReceived(LinphoneCore lc, String from) {}
	public void authInfoRequested(LinphoneCore lc, String realm, String username, String domain) {}
	public void displayStatus(LinphoneCore lc, String message) {}
	public void displayMessage(LinphoneCore lc, String message) {}
	public void displayWarning(LinphoneCore lc, String message) {}
	public void globalState(LinphoneCore lc, GlobalState state, String message) {}
	public void newSubscriptionRequest(LinphoneCore lc, LinphoneFriend lf,String url) {}
	public void notifyPresenceReceived(LinphoneCore lc, LinphoneFriend lf) {}
	public void callState(LinphoneCore lc, LinphoneCall call, State cstate, String msg) {}
	public void callStatsUpdated(LinphoneCore lc, LinphoneCall call, LinphoneCallStats stats) {}
	public void ecCalibrationStatus(LinphoneCore lc, EcCalibratorStatus status,int delay_ms, Object data) {}
	public void callEncryptionChanged(LinphoneCore lc, LinphoneCall call,boolean encrypted, String authenticationToken) {}
	public void notifyReceived(LinphoneCore lc, LinphoneCall call, LinphoneAddress from, byte[] event){}
	public void dtmfReceived(LinphoneCore lc, LinphoneCall call, int dtmf) {}

	public static void main(String[] args) {
		// Check tutorial was called with the right number of arguments
		if (args.length != 2) {
			throw new IllegalArgumentException("Bad number of arguments");
		}
		
		// Create tutorial object
		TutorialRegistration tutorial = new TutorialRegistration();
		try {
			// takes sip uri identity from the command line arguments
			String userSipAddress = args[1];
			// takes password from the command line arguments
			String userSipPassword = args[2];
			tutorial.launchTutorial(userSipAddress, userSipPassword);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	
	
	public void launchTutorial(String sipAddress, String password) throws LinphoneCoreException {
		final LinphoneCoreFactory lcFactory = LinphoneCoreFactory.instance();

		// First instantiate the core Linphone object given only a listener.
		// The listener will react to events in Linphone core.
		LinphoneCore lc = lcFactory.createLinphoneCore(this, null);

	
		try {
			
			// Parse identity
			LinphoneAddress address = lcFactory.createLinphoneAddress(sipAddress);
			String username = address.getUserName();
			String domain = address.getDomain();


			if (password != null) {
				// create authentication structure from identity and add to linphone
				lc.addAuthInfo(lcFactory.createAuthInfo(username, password, null, domain));
			}

			// create proxy config
			LinphoneProxyConfig proxyCfg = lc.createProxyConfig(sipAddress, domain, null, true);
			proxyCfg.setExpires(2000);
			lc.addProxyConfig(proxyCfg); // add it to linphone
			lc.setDefaultProxyConfig(proxyCfg);


			
			// main loop for receiving notifications and doing background linphonecore work
			running = true;
			while (running) {
				lc.iterate(); // first iterate initiates registration 
				sleep(50);
			}


			// Unregister
			lc.getDefaultProxyConfig().edit();
			lc.getDefaultProxyConfig().enableRegister(false);
			lc.getDefaultProxyConfig().done();
			while(lc.getDefaultProxyConfig().getState() != RegistrationState.RegistrationCleared) {
				lc.iterate();
				sleep(50);
			}

			// Then register again
			lc.getDefaultProxyConfig().edit();
			lc.getDefaultProxyConfig().enableRegister(true);
			lc.getDefaultProxyConfig().done();

			while(lc.getDefaultProxyConfig().getState() != RegistrationState.RegistrationOk
					&& lc.getDefaultProxyConfig().getState() != RegistrationState.RegistrationFailed) {
				lc.iterate();
				sleep(50);
			}

			// Automatic unregistration on exit
		} finally {
			write("Shutting down linphone...");
			// You need to destroy the LinphoneCore object when no longer used
			lc.destroy();
		}
	}

	private void sleep(int ms) {
		try {
			Thread.sleep(ms);
		} catch(InterruptedException ie) {
			write("Interrupted!\nAborting");
			return;
		}
	}

	public void stopMainLoop() {
		running=false;
	}

	
	private void write(String s) {
		TutorialNotifier.notify(s);
	}

	@Override
	public void messageReceived(LinphoneCore lc, LinphoneChatRoom cr,
			LinphoneChatMessage message) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void transferState(LinphoneCore lc, LinphoneCall call,
			State new_call_state) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void infoReceived(LinphoneCore lc, LinphoneCall call, LinphoneInfoMessage info) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void subscriptionStateChanged(LinphoneCore lc, LinphoneEvent ev,
			SubscriptionState state) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void notifyReceived(LinphoneCore lc, LinphoneEvent ev,
			String eventName, LinphoneContent content) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void publishStateChanged(LinphoneCore lc, LinphoneEvent ev,
			PublishState state) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void isComposingReceived(LinphoneCore lc, LinphoneChatRoom cr) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void configuringStatus(LinphoneCore lc,
			RemoteProvisioningState state, String message) {
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
