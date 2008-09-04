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
package org.linphone.p2pproxy.core.rdvautoconfig;


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
import org.linphone.p2pproxy.core.GenericServiceClient;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.ServiceProvider;
import org.linphone.p2pproxy.core.JxtaNetworkManager.Mode;


public class PeerInfoServiceClient extends GenericServiceClient implements P2pProxyNetworkProbe, ServiceProvider{
   private final static Logger mLog = Logger.getLogger(PeerInfoServiceClient.class);
   
   public PeerInfoServiceClient(Configurator lProperties,JxtaNetworkManager aJxtaNetworkManager) {
   super(lProperties,aJxtaNetworkManager,PeerInfoProviderService.ADV_NAME);
   }
   
 
   public InetAddress getPublicIpAddress()  throws P2pProxyException {
      checkObject();
      try {
         checkSocketConnection();
         PublicIpAddressRequest lPublicIpAddressRequest = new PublicIpAddressRequest(mJxtaNetworkManager.getPeerGroup().getPeerID().toString());
         mOut.writeObject(lPublicIpAddressRequest);
         mOut.flush();
         mLog.info("request message ["+lPublicIpAddressRequest+"] sent");
         Object lInputObj = mIn.readObject();
         mLog.info("response message ["+lInputObj+"] received");
         if(lInputObj instanceof PublicIpAddressResponse) {
            PublicIpAddressResponse lPublicIpAddressResponse = (PublicIpAddressResponse)lInputObj;
            InetAddress lInetAddress = lPublicIpAddressResponse.getPublicAddress();
            mLog.info("public IP ["+lInetAddress+"] received");
            return lInetAddress;
         } else {
            throw new P2pProxyException("unknown response ["+lInputObj+"]");
         }
      }
      catch(Exception e) {
         throw new P2pProxyException(e);
      }

   }

 

 
   public boolean probeSocket(InetSocketAddress aSocketAddress, Protocol aProtocol) throws P2pProxyException {
      checkObject();
      boolean lResult = false;
      try {
         checkSocketConnection();
         switch(aProtocol) {
         case tcp: {
            // open the socket on 0.0.0.0:port for NAT
           
            ServerSocket lSocketServer = new ServerSocket(aSocketAddress.getPort());
            lSocketServer.setSoTimeout(mSoTimout);
            
            // send request to server
            sendRequest(aSocketAddress,aProtocol);
             try {
               lSocketServer.accept();
              
               lResult = true;
            }catch (SocketTimeoutException e) {
               //            
            }
            lSocketServer.close();
            break;
         }
         case udp: {
            // open the socket on 0.0.0.0:port for NAT
            DatagramSocket lLocalSocket = new DatagramSocket(aSocketAddress.getPort());
            byte[] lBuff = new byte[1500];
            DatagramPacket lDatagramPacket = new DatagramPacket(lBuff,lBuff.length);
            lLocalSocket.setSoTimeout(mSoTimout);
           
            // send request to server
            SocketProbeRequest lSocketProbeRequest = new SocketProbeRequest(aSocketAddress,aProtocol);
            mOut.writeObject(lSocketProbeRequest);
            mOut.flush();
            mLog.info("request message ["+lSocketProbeRequest+"] sent");
            try {
               lLocalSocket.receive(lDatagramPacket);
                lResult = true;
            }catch (SocketTimeoutException e) {
               // nop
            }
            lLocalSocket.close(); 
            break;
         }
         default: throw new P2pProxyException("unsupported protocol ["+aProtocol+"]");
         }
         if (lResult) {
            mLog.info("socket  ["+aSocketAddress+"] is reachable with protocol ["+aProtocol+"]");
         } else {
            mLog.info("cannot reach  ["+aSocketAddress+"] with protocol ["+aProtocol+"] for ["+mSoTimout+"] ms");
         }
         return lResult;
      }
      catch(Exception e) {
         throw new P2pProxyException(e);
      }
   }
   private void sendRequest(SocketAddress aSocketAddress, Protocol aProtocol) throws IOException {
      SocketProbeRequest lSocketProbeRequest = new SocketProbeRequest(aSocketAddress,aProtocol);
      mOut.writeObject(lSocketProbeRequest);
      mOut.flush();
      mLog.info("request message ["+lSocketProbeRequest+"] sent");      
   }
}
