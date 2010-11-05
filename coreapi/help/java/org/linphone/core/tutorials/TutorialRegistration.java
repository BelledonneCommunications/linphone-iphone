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

import org.linphone.core.LinphoneAddress;
import org.linphone.core.LinphoneCall;
import org.linphone.core.LinphoneChatRoom;
import org.linphone.core.LinphoneCore;
import org.linphone.core.LinphoneCoreException;
import org.linphone.core.LinphoneCoreFactory;
import org.linphone.core.LinphoneCoreListener;
import org.linphone.core.LinphoneFriend;
import org.linphone.core.LinphoneProxyConfig;
import org.linphone.core.LinphoneCall.State;
import org.linphone.core.LinphoneCore.GlobalState;
import org.linphone.core.LinphoneCore.RegistrationState;


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
		write(cfg.getIdentity() + " : "+smessage+"\n");

		if (RegistrationState.RegistrationOk.equals(cstate))
			running = false;
	}

	public void show(LinphoneCore lc) {}
	public void byeReceived(LinphoneCore lc, String from) {}
	public void authInfoRequested(LinphoneCore lc, String realm, String username) {}
	public void displayStatus(LinphoneCore lc, String message) {}
	public void displayMessage(LinphoneCore lc, String message) {}
	public void displayWarning(LinphoneCore lc, String message) {}
	public void globalState(LinphoneCore lc, GlobalState state, String message) {}
	public void newSubscriptionRequest(LinphoneCore lc, LinphoneFriend lf,String url) {}
	public void notifyPresenceReceived(LinphoneCore lc, LinphoneFriend lf) {}
	public void textReceived(LinphoneCore lc, LinphoneChatRoom cr,LinphoneAddress from, String message) {}
	public void callState(LinphoneCore lc, LinphoneCall call, State cstate, String msg) {}


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
		LinphoneCore lc = lcFactory.createLinphoneCore(this);

	
		try {
			
			// Parse identity
			LinphoneAddress address = lcFactory.createLinphoneAddress(sipAddress);
			String username = address.getUserName();
			String domain = address.getDomain();


			if (password != null) {
				// create authentication structure from identity and add to linphone
				lc.addAuthInfo(lcFactory.createAuthInfo(username, password, null));
			}

			// create proxy config
			LinphoneProxyConfig proxyCfg = lcFactory.createProxyConfig(sipAddress, domain, null, true);
			lc.addProxyConfig(proxyCfg); // add it to linphone
			lc.setDefaultProxyConfig(proxyCfg);

			
			
			
			// main loop for receiving notifications and doing background linphonecore work
			running = true;
			while (running) {
				lc.iterate(); // first iterate initiates registration 
				try{
					Thread.sleep(50);
				} catch(InterruptedException ie) {
					write("Interrupted!\nAborting");
					return;
				}
			}


			// Automatic unregistration on exit
			
			
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
