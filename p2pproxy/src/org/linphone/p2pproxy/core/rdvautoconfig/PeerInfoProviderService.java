/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

PeerInfoProviderService.java - .

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
package org.linphone.p2pproxy.core.rdvautoconfig;



import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.net.URI;

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
import org.linphone.p2pproxy.core.Configurator;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.ServiceProvider;
import org.linphone.p2pproxy.core.jxtaext.EndpointRegistry;



public class PeerInfoProviderService implements Runnable,PeerInfoProvider,ServiceProvider{
   private final JxtaNetworkManager mJxtaNetworkManager;
   private final Configurator mProperties;
   public final static String PEER_INFO_SERVICE_PIPE_ID="org.linphone.p2pproxy.PeerInfoProviderService.bidi-pipe.id";
   private final static Logger mLog = Logger.getLogger(PeerInfoProviderService.class);
   private JxtaServerSocket mJxtaServerSocket;
   private Thread mSocketServerThread = new Thread(this,"PeerInfoProviderService server thread");
   public final static String ADV_NAME = "JXTASPEC:LINPHONE-PEERINFO";  
   private final static int SO_TIMOUT=3000;
   public final static String PEERINFO_MODULE_CLASS_ID="org.linphone.p2pproxy.PeerInfoProviderService.module-class.id";
   public final static String PEERINFO_MODULE_SPEC_ID="org.linphone.p2pproxy.PeerInfoProviderService.module-spec.id";
   private boolean mExist = false; 
   
   public PeerInfoProviderService(Configurator lProperties,JxtaNetworkManager aJxtaNetworkManager) {
       mJxtaNetworkManager = aJxtaNetworkManager; 
       mProperties = lProperties;
   }

   public void start(long l)  throws P2pProxyException {
      try {         
         mLog.info("Start the PeerInfoProviderService daemon");
         ModuleClassAdvertisement lModuleAdvertisement = (ModuleClassAdvertisement) AdvertisementFactory.newAdvertisement(ModuleClassAdvertisement.getAdvertisementType());

         lModuleAdvertisement.setName("JXTAMOD:LINPHONE-PEERINFO");
         lModuleAdvertisement.setDescription("Service to provide peer with data like its public ip,etc");

         ModuleClassID lModuleClassID;
         // to avoid ID creation at each start
         if (mProperties.getProperty(PEERINFO_MODULE_CLASS_ID) == null) {
            lModuleClassID = IDFactory.newModuleClassID();
            mProperties.setProperty(PEERINFO_MODULE_CLASS_ID, lModuleClassID.toURI().toString());
         } else {
            lModuleClassID = (ModuleClassID) IDFactory.fromURI(URI.create(mProperties.getProperty(PEERINFO_MODULE_CLASS_ID)));
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
         if (mProperties.getProperty(PEERINFO_MODULE_SPEC_ID) == null) {
            lModuleSpecId = IDFactory.newModuleSpecID(lModuleClassID);
            mProperties.setProperty(PEERINFO_MODULE_SPEC_ID, lModuleSpecId.toURI().toString());
         } else {
            lModuleSpecId = (ModuleSpecID) IDFactory.fromURI(URI.create(mProperties.getProperty(PEERINFO_MODULE_SPEC_ID)));
         }
         lModuleSpecAdvertisement.setModuleSpecID(lModuleSpecId);
         lModuleSpecAdvertisement.setSpecURI("http://www.linphone.org/peerinfo");

         PipeAdvertisement lSocketAdvertisement = mJxtaNetworkManager.createPipeAdvertisement(PEER_INFO_SERVICE_PIPE_ID, "peer-info-service");

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
         PeerInfoProviderService.mLog.error("socket instance error", e);
      }        
   }
   public void stop() {
	   try {
		   mJxtaServerSocket.close();
	   } catch (IOException e) {
		   //nop
	   }
	   mExist = true;
	   mLog.info("PeerInfoProviderService stopped");
   }

   public void run() {
      while (mExist) {
         try {
            mLog.info("Waiting for connection on ["+ADV_NAME+"]");
             Socket lSocket = mJxtaServerSocket.accept();
             // set reliable
             if (lSocket != null) {
                mLog.info("socket created");
                Thread thread = new Thread(new PeerInfoProviderServiceHandler(lSocket), "Connection Handler Thread");
                thread.start();
             }
         } catch (Exception e) {
            mLog.error("Server socket  error",e);
         }
     }
      
   }
   class PeerInfoProviderServiceHandler implements Runnable {
      Socket mSocket = null;

      PeerInfoProviderServiceHandler(Socket aSocket) {
          mSocket = aSocket;
      }

      public void run() {
         try {
         ObjectOutputStream lOut = new ObjectOutputStream(mSocket.getOutputStream());
         //work around to unlock the socket
         lOut.writeBoolean(true);
         lOut.flush();
         ObjectInputStream lIn = new ObjectInputStream(mSocket.getInputStream());
         Object lInputObj;
         Object lOutputObj;
         while (mExist == false) {
            lInputObj = lIn.readObject();
            mLog.info("request message ["+lInputObj+"] received");
            
            if (lInputObj instanceof PublicIpAddressRequest) {
//             PublicIpAddressRequest
               PublicIpAddressRequest lPublicIpAddressRequest = (PublicIpAddressRequest) lInputObj;
               InetAddress lInetAddress = getPeerInfoProvider().getPublicAddress(lPublicIpAddressRequest.getPeerId());   
               PublicIpAddressResponse lPublicIpAddressResponse;
               if (lInetAddress == null) {
                  lPublicIpAddressResponse = new PublicIpAddressResponse("address not found for["+lPublicIpAddressRequest.getPeerId()+"]");
               } else {
                  lPublicIpAddressResponse = new PublicIpAddressResponse(lInetAddress);
               }
               lOutputObj = lPublicIpAddressResponse;
               lOut.writeObject(lOutputObj);
               lOut.flush();
               mLog.info("request reponse ["+lOutputObj+"] sent");  
            } else if (lInputObj instanceof SocketProbeRequest) {
//             SocketProbeRequest
               SocketProbeRequest lSocketProbeRequest = (SocketProbeRequest)lInputObj;
               switch (lSocketProbeRequest.getProtocol()) {
               case tcp: {
                  Socket lSocket = new Socket();
                  lSocket.setSoTimeout(SO_TIMOUT);
                  
                  try {
                     lSocket.connect(lSocketProbeRequest.getSocketAddress());
                     mLog.info("socket  ["+lSocketProbeRequest+"] is reachable");
                  }catch (Exception e) {
                     mLog.info("cannot reach  ["+lSocketProbeRequest+"] in mode ["+lSocketProbeRequest.getProtocol()+"] for"+SO_TIMOUT+"");
                  }                  
                  lSocket.close(); 
                  break;
               }
               case udp: {
                  DatagramSocket lSocket = new DatagramSocket();
                  String lPing = "ping";
                  DatagramPacket lDatagramPacket = new DatagramPacket(lPing.getBytes(),lPing.getBytes().length);
                  lDatagramPacket.setSocketAddress(lSocketProbeRequest.getSocketAddress());
                  lSocket.send(lDatagramPacket);
                   mLog.info("ping sent to socket  ["+lSocketProbeRequest+"] in mode ["+lSocketProbeRequest.getProtocol()+"]");
                  lSocket.close(); 
                  break;
               }
               default: throw new P2pProxyException("unsupported protocol ["+lSocketProbeRequest.getProtocol()+"]");
               }

            }else {
               mLog.error("unknown request ["+lInputObj+"]");
               lOutputObj = null;
            }


         }
         } catch (Exception e) {
            mLog.error("socket instance error",e);
         }
         finally {
            try {
               mSocket.close();
            } catch (IOException e) {
               mLog.error("cannot close socket ",e);
            }
         }
      }
   }
   public PeerInfoProvider getPeerInfoProvider() {
      return this;
   }
   public InetAddress getPublicAddress(String aPeerAddress) {
      return EndpointRegistry.getInstance().get(aPeerAddress);
   }

}