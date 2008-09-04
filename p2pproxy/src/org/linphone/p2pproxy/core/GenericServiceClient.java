/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

PeerInfoServiceClient.java - .

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
package org.linphone.p2pproxy.core;


import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.SocketAddress;
import java.net.SocketTimeoutException;
import java.util.List;

import net.jxta.document.Advertisement;
import net.jxta.id.ID;
import net.jxta.protocol.ModuleSpecAdvertisement;
import net.jxta.socket.JxtaSocket;
import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxyNetworkProbe;
import org.linphone.p2pproxy.core.Configurator;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.ServiceProvider;
import org.linphone.p2pproxy.core.JxtaNetworkManager.Mode;


public abstract class GenericServiceClient implements ServiceProvider{
   protected final JxtaNetworkManager mJxtaNetworkManager;
   private final Configurator mProperties;
   private final static Logger mLog = Logger.getLogger(GenericServiceClient.class);
   protected JxtaSocket mJxtaSocket;
   protected ObjectOutputStream mOut;
   protected ObjectInputStream mIn;
   boolean mStarted = false;
   protected final int mSoTimout;
   private final String mAdvertisementName;
   
   public GenericServiceClient(Configurator lProperties,JxtaNetworkManager aJxtaNetworkManager,String aAdvertisementName) {
      mJxtaNetworkManager = aJxtaNetworkManager; 
      mProperties = lProperties;
      mSoTimout = Integer.parseInt(lProperties.getProperty(JxtaNetworkManager.SO_TIMEOUT, "10000"));
      mAdvertisementName = aAdvertisementName;
  }
   
   public void start(long aTimeOut) throws P2pProxyException {
      // 1 check if connected to a rdv
      try {

          mStarted = true;
          mLog.info(mAdvertisementName+" client started");
      }
      catch(Exception e) {
          throw new P2pProxyException(e);
      }
  
   }


   public void stop() {
      try {
         checkObject();
         mIn.close();
         mOut.close();
         mJxtaSocket.close();
      }catch (Exception e ) {
         mLog.error("cannot "+mAdvertisementName+" client" , e);
      }
      mLog.info(mAdvertisementName+" client stopped");
    }

   protected void checkSocketConnection() throws  IOException, P2pProxyException, InterruptedException {
      //wo because close not sent
      if (mJxtaSocket != null) mJxtaSocket.close();
      
      if (mJxtaSocket == null ||mJxtaSocket.isClosed() || mJxtaSocket.isBound() == false) {
         try {
            mLog.info("Opening socket for ["+mAdvertisementName+"]");
            // try from local
            mJxtaSocket = mJxtaNetworkManager.openSocket(null, mAdvertisementName, mSoTimout,true);
         } catch (P2pProxyException e) {
            //last chance
            mLog.warn("cannot open socket ["+mAdvertisementName+"], trying from remote",e);
            mJxtaSocket = mJxtaNetworkManager.openSocket(null, mAdvertisementName, mSoTimout,false);
         }
         if (mJxtaSocket == null) throw new P2pProxyException("Cannot start"+mAdvertisementName+" client  because cannot bind jxta socket"); 
         mOut = new ObjectOutputStream(mJxtaSocket.getOutputStream());
         mIn = new ObjectInputStream(mJxtaSocket.getInputStream());
         //just to work-around socket establishment
         mIn.readBoolean();
      }

   }

   protected void checkObject() throws P2pProxyException{
      if(!mStarted) throw new P2pProxyException(mAdvertisementName+" client not started");
   }
 

}
