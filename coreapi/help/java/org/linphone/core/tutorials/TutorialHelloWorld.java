/*
TutorialHelloWorld.java
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
import org.linphone.core.LinphoneCore.EcCalibratorStatus;
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
 * It just takes a sip-uri as first argument and attempts to call it.
 * 
 * Ported from helloworld.c
 *
 * @author Guillaume Beraudo
 *
 */
public class TutorialHelloWorld implements LinphoneCoreListener {
	private boolean running;
	private TutorialNotifier TutorialNotifier;


	public TutorialHelloWorld(TutorialNotifier TutorialNotifier) {
		this.TutorialNotifier = TutorialNotifier;
	}

	public TutorialHelloWorld() {
		this.TutorialNotifier = new TutorialNotifier();
	}

	
	
	public void show(LinphoneCore lc) {}
	public void byeReceived(LinphoneCore lc, String from) {}
	public void authInfoRequested(LinphoneCore lc, String realm, String username) {}
	public void displayStatus(LinphoneCore lc, String message) {}
	public void displayMessage(LinphoneCore lc, String message) {}
	public void displayWarning(LinphoneCore lc, String message) {}
	public void globalState(LinphoneCore lc, GlobalState state, String message) {}
	public void registrationState(LinphoneCore lc, LinphoneProxyConfig cfg,RegistrationState cstate, String smessage) {}
	public void newSubscriptionRequest(LinphoneCore lc, LinphoneFriend lf,String url) {}
	public void notifyPresenceReceived(LinphoneCore lc, LinphoneFriend lf) {}
	public void textReceived(LinphoneCore lc, LinphoneChatRoom cr,LinphoneAddress from, String message) {}
	public void ecCalibrationStatus(LinphoneCore lc, EcCalibratorStatus status,int delay_ms, Object data) {}
	public void callEncryptionChanged(LinphoneCore lc, LinphoneCall call,boolean encrypted, String authenticationToken) {}
	public void notifyReceived(LinphoneCore lc, LinphoneCall call, LinphoneAddress from, byte[] event){}
	/*
	 * Call state notification listener
	 */
	public void callState(LinphoneCore lc, LinphoneCall call, State cstate, String msg){
		write("State: " + msg);

		if (State.CallEnd.equals(cstate))
			running = false;
	}


	public static void main(String[] args) {
		// Check tutorial was called with the right number of arguments
		if (args.length != 1) {
			throw new IllegalArgumentException("Bad number of arguments");
		}
		
		// Create tutorial object
		TutorialHelloWorld helloWorld = new TutorialHelloWorld();
		try {
			String destinationSipAddress = args[1];
			helloWorld.launchTutorial(destinationSipAddress);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	
	
	public void launchTutorial(String destinationSipAddress) throws LinphoneCoreException {
		
		// First instantiate the core Linphone object given only a listener.
		// The listener will react to events in Linphone core.
		LinphoneCore lc = LinphoneCoreFactory.instance().createLinphoneCore(this);


		
		try {
			// Send the INVITE message to destination SIP address
			LinphoneCall call = lc.invite(destinationSipAddress);
			if (call == null) {
				write("Could not place call to " + destinationSipAddress);
				write("Aborting");
				return;
			}
			write("Call to " + destinationSipAddress + " is in progress...");


			
			// main loop for receiving notifications and doing background linphonecore work
			running = true;
			while (running) {
				lc.iterate();
				try{
					Thread.sleep(50);
				} catch(InterruptedException ie) {
					write("Interrupted!\nAborting");
					return;
				}
			}


			
			if (!State.CallEnd.equals(call.getState())) {
				write("Terminating the call");
				lc.terminateCall(call);
			}
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
