/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

RtpRelayService.java - .

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

import java.net.Socket;
import java.net.URI;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import net.jxta.document.AdvertisementFactory;
import net.jxta.id.IDFactory;

import net.jxta.platform.ModuleClassID;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ModuleClassAdvertisement;
import net.jxta.protocol.ModuleSpecAdvertisement;
import net.jxta.protocol.PipeAdvertisement;
import net.jxta.socket.JxtaServerSocket;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;


public class GenericService implements Runnable,ServiceProvider {
   public interface ServiceSocketHandlerFactory {

      public Runnable create(Socket aSocket) ; 

   }
   private final JxtaNetworkManager mJxtaNetworkManager;
   private final Configurator mProperties;
   private final  String SERVICE_PIPE_ID;
   private final static Logger mLog = Logger.getLogger(GenericService.class);
   private final  String ADV_NAME ;  
   private final  String MODULE_CLASS_ID;
   private final  String MODULE_SPEC_ID;
   private JxtaServerSocket mJxtaServerSocket;
   private final String mServiceName; 
   private Thread mSocketServerThread ;
   private final ExecutorService mPool;
   private final ServiceSocketHandlerFactory mServiceSocketHandlerFactory;
  
   public GenericService(Configurator lProperties,JxtaNetworkManager aJxtaNetworkManager,String aServiceName,ServiceSocketHandlerFactory aServiceSocketHandlerFactory) {
       mJxtaNetworkManager = aJxtaNetworkManager; 
       mProperties = lProperties;
       mServiceName = aServiceName.trim();
       SERVICE_PIPE_ID="org.linphone.p2pproxy."+mServiceName+".bidi-pipe.id";
       ADV_NAME = "JXTASPEC:LINPHONE-"+mServiceName;
       MODULE_CLASS_ID="org.linphone.p2pproxy."+mServiceName+"Service.module-class.id";
       MODULE_SPEC_ID="org.linphone.p2pproxy."+mServiceName+"Service.module-spec.id";
       mSocketServerThread = new Thread(this,mServiceName+"Service server thread");
       mPool = Executors.newCachedThreadPool();
       mServiceSocketHandlerFactory = aServiceSocketHandlerFactory;
   }

   public void start(long l)  throws P2pProxyException {
      try {         
         mLog.info("Start the RtpRelayService daemon");
         ModuleClassAdvertisement lModuleAdvertisement = (ModuleClassAdvertisement) AdvertisementFactory.newAdvertisement(ModuleClassAdvertisement.getAdvertisementType());

         lModuleAdvertisement.setName("JXTAMOD:LINPHONE-"+mServiceName);
         lModuleAdvertisement.setDescription("Service to provide " +mServiceName);

         ModuleClassID lModuleClassID;
         // to avoid ID creation at each start
         if (mProperties.getProperty(MODULE_CLASS_ID) == null) {
            lModuleClassID = IDFactory.newModuleClassID();
            mProperties.setProperty(MODULE_CLASS_ID, lModuleClassID.toURI().toString());
         } else {
            lModuleClassID = (ModuleClassID) IDFactory.fromURI(URI.create(mProperties.getProperty(MODULE_CLASS_ID)));
         }
         lModuleAdvertisement.setModuleClassID(lModuleClassID);

         // publish local only
         mJxtaNetworkManager.getPeerGroup().getDiscoveryService().publish(lModuleAdvertisement);

         ModuleSpecAdvertisement lModuleSpecAdvertisement = (ModuleSpecAdvertisement)AdvertisementFactory.newAdvertisement(ModuleSpecAdvertisement.getAdvertisementType());
         lModuleSpecAdvertisement.setName(ADV_NAME);
         lModuleSpecAdvertisement.setVersion("Version 1.0");
         lModuleSpecAdvertisement.setCreator("linphone.org");
         // to avoid ID creation at each start
         ModuleSpecID  lModuleSpecId;
         if (mProperties.getProperty(MODULE_SPEC_ID) == null) {
            lModuleSpecId = IDFactory.newModuleSpecID(lModuleClassID);
            mProperties.setProperty(MODULE_SPEC_ID, lModuleSpecId.toURI().toString());
         } else {
            lModuleSpecId = (ModuleSpecID) IDFactory.fromURI(URI.create(mProperties.getProperty(MODULE_SPEC_ID)));
         }
         lModuleSpecAdvertisement.setModuleSpecID(lModuleSpecId);
         lModuleSpecAdvertisement.setSpecURI("http://www.linphone.org/"+mServiceName.toLowerCase());

         PipeAdvertisement lSocketAdvertisement = mJxtaNetworkManager.createPipeAdvertisement(SERVICE_PIPE_ID, mServiceName.toLowerCase());

         lModuleSpecAdvertisement.setPipeAdvertisement(lSocketAdvertisement);
         mJxtaServerSocket = new JxtaServerSocket(mJxtaNetworkManager.getPeerGroup(), lSocketAdvertisement, 10);
         mJxtaServerSocket.setSoTimeout(0);
         mSocketServerThread.start();
         //publish local only
         mJxtaNetworkManager.getPeerGroup().getDiscoveryService().publish(lModuleSpecAdvertisement);
         mLog.info("Adv ["+lModuleSpecAdvertisement+"] published");
      }
      catch(Exception e)
      {
         mLog.error("socket instance error", e);
      }        
   }
   public void stop(){
      throw new RuntimeException("Not implemented");
   }
   public void run() {
      while (true) {
         try {
            mLog.info("Waiting for connection on service ["+ADV_NAME+"]");
             Socket lSocket = mJxtaServerSocket.accept();
             // set reliable
             if (lSocket != null) {
                mLog.info("socket created");
                mPool.execute(mServiceSocketHandlerFactory.create(lSocket));
             }
         } catch (Exception e) {
            mLog.error("Server socket  error",e);
         }
     }
      
   }

   /**
    * @return Returns the aDV_NAME.
    */
   public String getAdvName() {
      return ADV_NAME;
   }
}
