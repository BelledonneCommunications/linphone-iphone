/*
TutorialBuddyStatus
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
import org.linphone.core.LinphoneAuthInfo;
import org.linphone.core.LinphoneCoreException;
import org.linphone.core.LinphoneCoreFactory;
import org.linphone.core.LinphoneCoreListener;
import org.linphone.core.LinphoneEvent;
import org.linphone.core.LinphoneFriend;
import org.linphone.core.LinphoneFriendList;
import org.linphone.core.LinphoneFriend.SubscribePolicy;
import org.linphone.core.LinphoneInfoMessage;
import org.linphone.core.LinphoneProxyConfig;
import org.linphone.core.OnlineStatus;
import org.linphone.core.PublishState;
import org.linphone.core.SubscriptionState;

/**
 * 
 * This program is a _very_ simple usage example of liblinphone,
 * demonstrating how to initiate  SIP subscriptions and receive notifications
 * from a sip uri identity passed from the command line.
 * <br>Argument must be like sip:jehan@sip.linphone.org .
 * ex budy_list sip:jehan@sip.linphone.org
 * <br>
 * Optionnally argument 2 can be registration sip identy.Argument 3 can be passord.
 * ex: budy_list sip:jehan@sip.linphone.org sip:myidentity@sip.linphone.org mypassword
 *
 * Ported from buddy_status.c
 *
 * @author Guillaume Beraudo
 *
 */
public class TutorialBuddyStatus implements LinphoneCoreListener {

	private boolean running;
	private TutorialNotifier TutorialNotifier;


	public TutorialBuddyStatus(TutorialNotifier TutorialNotifier) {
		this.TutorialNotifier = TutorialNotifier;
	}

	public TutorialBuddyStatus() {
		this.TutorialNotifier = new TutorialNotifier();
	}



	public void newSubscriptionRequest(LinphoneCore lc, LinphoneFriend lf,String url) {
		write("["+lf.getAddress().getUserName()+"] wants to see your status, accepting");
		lf.edit(); // start editing friend
		lf.setIncSubscribePolicy(SubscribePolicy.SPAccept); // accept incoming subscription request for this friend
		lf.done(); // commit change
		try {
			// add this new friend to the buddy list
			lc.addFriend(lf);
		} catch (LinphoneCoreException e) {
			write("Error while adding friend [" + lf.getAddress().getUserName() + "] to linphone in the callback");
		}
	}

	public void notifyPresenceReceived(LinphoneCore lc, LinphoneFriend lf) {
		write("New state [" + lf.getStatus() +"] for user id ["+lf.getAddress().getUserName()+"]");
	}


	public void registrationState(LinphoneCore lc, LinphoneProxyConfig cfg,RegistrationState cstate, String smessage) {
		write(cfg.getIdentity() + " : "+smessage+"\n");
	}
	public void show(LinphoneCore lc) {}
	public void byeReceived(LinphoneCore lc, String from) {}
	public void displayStatus(LinphoneCore lc, String message) {}
	public void displayMessage(LinphoneCore lc, String message) {}
	public void displayWarning(LinphoneCore lc, String message) {}
	public void globalState(LinphoneCore lc, GlobalState state, String message) {}
	public void callState(LinphoneCore lc, LinphoneCall call, State cstate, String msg) {}
	public void callStatsUpdated(LinphoneCore lc, LinphoneCall call, LinphoneCallStats stats) {}
	public void ecCalibrationStatus(LinphoneCore lc, EcCalibratorStatus status,int delay_ms, Object data) {}
	public void callEncryptionChanged(LinphoneCore lc, LinphoneCall call,boolean encrypted, String authenticationToken) {}
	public void notifyReceived(LinphoneCore lc, LinphoneCall call, LinphoneAddress from, byte[] event){}
	public void dtmfReceived(LinphoneCore lc, LinphoneCall call, int dtmf) {}


	public static void main(String[] args) {
		// Check tutorial was called with the right number of arguments
		if (args.length < 1 || args.length > 3 ) {
			throw new IllegalArgumentException("Bad number of arguments ["+args.length+"] should be 1, 2 or 3");
		}

		// Create tutorial object
		TutorialBuddyStatus tutorial = new TutorialBuddyStatus();
		try {
			// takes sip uri identity from the command line arguments 
			String userSipAddress = args[1];
			
			// takes sip uri identity from the command line arguments
			String mySipAddress = args.length>1?args[1]:null;
			// takes password from the command line arguments
			String mySipPassword =args.length>2?args[2]:null;

			tutorial.launchTutorial(userSipAddress,mySipAddress,mySipPassword);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}



	public void launchTutorial(String sipAddress,String mySipAddress, String mySipPassword) throws LinphoneCoreException {
		final LinphoneCoreFactory lcFactory = LinphoneCoreFactory.instance();

		// First instantiate the core Linphone object given only a listener.
		// The listener will react to events in Linphone core.
		LinphoneCore lc = lcFactory.createLinphoneCore(this, null);


		try {

			// Create friend object from string address
			LinphoneFriend lf = lc.createFriendWithAddress(sipAddress);
			if (lf == null) {
				write("Could not create friend; weird SIP address?");
				return;
			}

			if (mySipAddress != null) {
				// Parse identity
				LinphoneAddress address = lcFactory.createLinphoneAddress(mySipAddress);
				String username = address.getUserName();
				String domain = address.getDomain();


				if (mySipPassword != null) {
					// create authentication structure from identity and add to linphone
					lc.addAuthInfo(lcFactory.createAuthInfo(username, mySipPassword, null, domain));
				}

				// create proxy config
				LinphoneProxyConfig proxyCfg = lc.createProxyConfig(mySipAddress, domain, null, true);
				proxyCfg.enablePublish(true);
				lc.addProxyConfig(proxyCfg); // add it to linphone
				lc.setDefaultProxyConfig(proxyCfg);
				while (!proxyCfg.isRegistered()) {
					lc.iterate(); //iterate until registration
					try{
						Thread.sleep(1000);
					} catch(InterruptedException ie) {
						write("Interrupted!\nAborting");
						return;
					}
				}
			}
			
			// configure this friend to emit SUBSCRIBE message after being added to LinphoneCore
			lf.enableSubscribes(true);
			
			// accept incoming subscription request for this friend
			lf.setIncSubscribePolicy(SubscribePolicy.SPAccept);
			try {
				// add my friend to the buddy list, initiate SUBSCRIBE message
				lc.addFriend(lf);
			} catch (LinphoneCoreException e) {
				write("Error while adding friend " + lf.getAddress().getUserName() + " to linphone");
				return;
			}
			
			// set my status to online 
			lc.setPresenceInfo(0, null, OnlineStatus.Online);
			
			
			// main loop for receiving notifications and doing background linphonecore work
			running = true;
			while (running) {
				lc.iterate(); // first iterate initiates subscription
				try{
					Thread.sleep(50);
				} catch(InterruptedException ie) {
					write("Interrupted!\nAborting");
					return;
				}
			}


			// change my presence status to offline
			lc.setPresenceInfo(0, null, OnlineStatus.Offline);
			// just to make sure new status is initiate message is issued
			lc.iterate();

			
			lf.edit(); // start editing friend 
			lf.enableSubscribes(false); // disable subscription for this friend
			lf.done(); // commit changes triggering an UNSUBSCRIBE message
			lc.iterate(); // just to make sure unsubscribe message is issued


		} finally {
			write("Shutting down...");
			// You need to destroy the LinphoneCore object when no longer used
			lc.destroy();
			write("Exited");
		}
	}


	public void stopMainLoop() {
		running=false;
	}


	private void write(String s) {
		TutorialNotifier.notify(s);
	}

	@Override
	public void messageReceived(LinphoneCore lc, LinphoneChatRoom cr, LinphoneChatMessage message) {
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
	public void authInfoRequested(LinphoneCore lc, String realm,
			String username, String domain) {
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void authenticationRequested(LinphoneCore lc, 
			LinphoneAuthInfo authInfo, LinphoneCore.AuthMethod method) {
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
