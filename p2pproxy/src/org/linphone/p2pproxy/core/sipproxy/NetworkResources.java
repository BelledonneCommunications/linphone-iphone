/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

NetworkResources.java -- connection to a jxta network.

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
package org.linphone.p2pproxy.core.sipproxy;

import java.io.IOException;

import java.util.HashMap;
import java.util.Map;

import org.apache.log4j.Logger;

import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.MessageDispatcher;
import org.linphone.p2pproxy.core.media.jxtaudpproxy.RtpSessionImpl;

import net.jxta.discovery.DiscoveryService;
import net.jxta.pipe.InputPipe;

import net.jxta.pipe.PipeMsgListener;

/**
 * @author jehan
 *
 */
public class NetworkResources {
   public enum State {idle,offering,answering,ready} 
   private static String PIPE_ID="org.linphone.p2pproxy.bidi-pipe.id";
   private final InputPipe mInputPipe;
   private MessageDispatcher mMessageDispatcher = new MessageDispatcher();
   private Map<String,RtpSessionImpl> mRtpSessionTab = new HashMap<String,RtpSessionImpl>();
   private final JxtaNetworkManager mJxtaNetworkManager;
   private final static Logger mLog = Logger.getLogger(NetworkResources.class);
   private State mState = State.idle;
   public NetworkResources(String aUserName, JxtaNetworkManager aJxtaNetworkManager) throws IOException {
      mJxtaNetworkManager = aJxtaNetworkManager;
      mInputPipe = mJxtaNetworkManager.createPipe(PIPE_ID+"-"+aUserName, aUserName, mMessageDispatcher);
   }
   public void release(){
      ;
   }
   public void putRtpSession(String key,RtpSessionImpl value) {
	   mRtpSessionTab.put(key, value);
	   addPipeMsgListener(value);
   }
   public RtpSessionImpl getRtpSession(String key) {
	   return mRtpSessionTab.get(key);
   }
   public boolean hasRtpSession(String key) {
	   return mRtpSessionTab.containsKey(key);
   }
   public void addPipeMsgListener(PipeMsgListener aPipeMsgListener) {
      mMessageDispatcher.addPipeMsgListener( aPipeMsgListener);
   }
   public void publish(long anExpiration) throws IOException {
      DiscoveryService lDiscoveryService = mJxtaNetworkManager.getPeerGroup().getDiscoveryService();
      
      if (anExpiration > 0) {
         //publish sip pipe
         lDiscoveryService.publish(mInputPipe.getAdvertisement(), anExpiration, anExpiration);
      } else {
          //first flush in any cases
         lDiscoveryService.flushAdvertisement(mInputPipe.getAdvertisement());
         mInputPipe.close();
      }

      mLog.info("adv ["+mInputPipe.getAdvertisement().getName()+"] published expire ["+anExpiration+"]");
   }

	public State getState() {
		return mState;
	}
	public void setState(State state) {
		mState = state;
	}

}
