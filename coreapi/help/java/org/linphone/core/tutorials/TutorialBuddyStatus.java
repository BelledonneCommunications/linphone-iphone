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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
package org.linphone.core.tutorials;

import org.linphone.core.LinphoneAddress;
import org.linphone.core.LinphoneCall;
import org.linphone.core.LinphoneChatRoom;
import org.linphone.core.LinphoneCore;
import org.linphone.core.LinphoneCoreException;
import org.linphone.core.LinphoneCoreFactory;
import org.linphone.core.LinphoneCoreListener;
import org.linphone.core.LinphoneFriend;
import org.linphone.core.LinphoneProxyConfig;
import org.linphone.core.OnlineStatus;
import org.linphone.core.LinphoneCall.State;
import org.linphone.core.LinphoneCore.GlobalState;
import org.linphone.core.LinphoneCore.RegistrationState;
import org.linphone.core.LinphoneFriend.SubscribePolicy;

/**
 * 
 * This program is a _very_ simple usage example of liblinphone,
 * demonstrating how to initiate  SIP subscriptions and receive notifications
 * from a sip uri identity passed from the command line.
 * <br>Argument must be like sip:jehan@sip.linphone.org .
 * ex budy_list sip:jehan@sip.linphone.org
 * <br>Subscription is cleared on SIGINT
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


	public void registrationState(LinphoneCore lc, LinphoneProxyConfig cfg,RegistrationState cstate, String smessage) {}
	public void show(LinphoneCore lc) {}
	public void byeReceived(LinphoneCore lc, String from) {}
	public void authInfoRequested(LinphoneCore lc, String realm, String username) {}
	public void displayStatus(LinphoneCore lc, String message) {}
	public void displayMessage(LinphoneCore lc, String message) {}
	public void displayWarning(LinphoneCore lc, String message) {}
	public void globalState(LinphoneCore lc, GlobalState state, String message) {}
	public void textReceived(LinphoneCore lc, LinphoneChatRoom cr,LinphoneAddress from, String message) {}
	public void callState(LinphoneCore lc, LinphoneCall call, State cstate, String msg) {}




	public static void main(String[] args) {
		// Check tutorial was called with the right number of arguments
		if (args.length != 1) {
			throw new IllegalArgumentException("Bad number of arguments");
		}

		// Create tutorial object
		TutorialBuddyStatus tutorial = new TutorialBuddyStatus();
		try {
			// takes sip uri identity from the command line arguments 
			String userSipAddress = args[1];
			tutorial.launchTutorial(userSipAddress);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}



	public void launchTutorial(String sipAddress) throws LinphoneCoreException {
		final LinphoneCoreFactory lcFactory = LinphoneCoreFactory.instance();

		// First instantiate the core Linphone object given only a listener.
		// The listener will react to events in Linphone core.
		LinphoneCore lc = lcFactory.createLinphoneCore(this);


		try {

			// Create friend object from string address
			LinphoneFriend lf = lcFactory.createLinphoneFriend(sipAddress);
			if (lf == null) {
				write("Could not create friend; weird SIP address?");
				return;
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

}
