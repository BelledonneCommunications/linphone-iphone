/*
TutorialChatRoom.java
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
 * It demonstrates how to send/receive SIP MESSAGE from a sip uri identity
 * passed from the command line.
 *
 * Argument must be like sip:jehan@sip.linphone.org .
 *
 * ex chatroom sip:jehan@sip.linphone.org
 * just takes a sip-uri as first argument and attempts to call it.
 * 
 * Ported from chatroom.c
 *
 * @author Guillaume Beraudo
 *
 */
public class TutorialChatRoom implements LinphoneCoreListener {
	private boolean running;
	private TutorialNotifier TutorialNotifier;


	public TutorialChatRoom(TutorialNotifier TutorialNotifier) {
		this.TutorialNotifier = TutorialNotifier;
	}

	public TutorialChatRoom() {
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
	public void callState(LinphoneCore lc, LinphoneCall call, State cstate, String msg){}

	
	
	public void textReceived(LinphoneCore lc, LinphoneChatRoom cr,LinphoneAddress from, String message) {
        write("Message ["+message+"] received from ["+from.asString()+"]");
	}

	
	
	public static void main(String[] args) {
		// Check tutorial was called with the right number of arguments
		// Takes the sip uri identity from the command line arguments
		if (args.length != 1) {
			throw new IllegalArgumentException("Bad number of arguments");
		}
		
		// Create tutorial object
		TutorialChatRoom tutorial = new TutorialChatRoom();
		try {
			String destinationSipAddress = args[1];
			tutorial.launchTutorial(destinationSipAddress);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	
	
	public void launchTutorial(String destinationSipAddress) throws LinphoneCoreException {
		
		// First instantiate the core Linphone object given only a listener.
		// The listener will react to events in Linphone core.
		LinphoneCore lc = LinphoneCoreFactory.instance().createLinphoneCore(this);

		try {
			// Next step is to create a chat room
			LinphoneChatRoom chatRoom = lc.createChatRoom(destinationSipAddress);
			
			// Send message
			chatRoom.sendMessage("Hello world");
			
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
