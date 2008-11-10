/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

AutoConfigService.java - .

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

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.util.Enumeration;

import net.jxta.document.AdvertisementFactory;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.impl.protocol.HTTPAdv;
import net.jxta.impl.protocol.TCPAdv;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.TransportAdvertisement;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxyNetworkProbe;
import org.linphone.p2pproxy.core.Configurator;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.ServiceProvider;
import org.linphone.p2pproxy.core.media.MediaResourceService;


public class AutoConfigService implements ServiceProvider {
   private final static Logger mLog = Logger.getLogger(AutoConfigService.class);
   private final JxtaNetworkManager mJxtaNetworkManager;
   private final Configurator mProperties;
   private final int mRelayCapacity; 
   private final P2pProxyNetworkProbe mP2pProxyManagement;
   private InetAddress mPubliAddress;
   public AutoConfigService(Configurator lProperties,JxtaNetworkManager aJxtaNetworkManager,P2pProxyNetworkProbe aServiceProvider) {
      mJxtaNetworkManager = aJxtaNetworkManager; 
      mProperties = lProperties;
      mP2pProxyManagement = aServiceProvider;
      mRelayCapacity = Integer.parseInt(lProperties.getProperty(JxtaNetworkManager.RELAY_CAPACITY, "100"));
  }   
   public void start(long aTimeOut) throws P2pProxyException {
      try {
         if(!mJxtaNetworkManager.isConnectedToRendezVous(aTimeOut))
             throw new P2pProxyException("Cannot start peer info service because not connected to any rdv");
         //check if capable

         mLog.info("AutoConfigService started");
     }
     catch(Exception e) {
         throw new P2pProxyException(e);
     }

   }

   public void stop() {
      mLog.info("AutoConfigService stopped");
   }
   private HTTPAdv getHttpAdv() throws P2pProxyException {

      XMLDocument lHttpDoc = (XMLDocument) mJxtaNetworkManager.getPeerGroup().getConfigAdvertisement().getServiceParam(PeerGroup.httpProtoClassID);
      if (lHttpDoc == null)  {
         throw new P2pProxyException("cannot find HTTPAdv");
      }
      Enumeration lHttpChilds = lHttpDoc.getChildren( TransportAdvertisement.getAdvertisementType());
      // get the HTTPAdv from TransportAdv
      if( lHttpChilds.hasMoreElements() ) {
         return (HTTPAdv) AdvertisementFactory.newAdvertisement( (XMLElement) lHttpChilds.nextElement() );
      }  else {
         throw new P2pProxyException("cannot find HTTPAdv from transport");
      }
   }
   private TCPAdv getTcpAdv() throws P2pProxyException {
	      XMLDocument lTcpDoc = (XMLDocument) mJxtaNetworkManager.getPeerGroup().getConfigAdvertisement().getServiceParam(PeerGroup.tcpProtoClassID);
	      if (lTcpDoc == null)  {
	         throw new P2pProxyException("cannot find TCPAdv");
	      }
	      Enumeration lHttpChilds = lTcpDoc.getChildren( TransportAdvertisement.getAdvertisementType());
	      // get the HTTPAdv from TransportAdv
	      if( lHttpChilds.hasMoreElements() ) {
	         return (TCPAdv) AdvertisementFactory.newAdvertisement( (XMLElement) lHttpChilds.nextElement() );
	      }  else {
	         throw new P2pProxyException("cannot find TCPAdv from transport");
	      }
	   }
 
   /**
    * test if the peer is capable to behave as a superpeer
    * @return
    */
   public boolean canIBehaveAsASuperPeer() {
	   boolean lHttpServerOk=false;
	   boolean lTcpServerOk=false;

	   try {
//		   1 check local ip
	      mPubliAddress = mP2pProxyManagement.getPublicIpAddress();
		   //2 probe the http socket
		      InetSocketAddress  lSocketAddress = new InetSocketAddress(mPubliAddress,getHttpAdv().getPort());
		      lHttpServerOk = mP2pProxyManagement.probeSocket(lSocketAddress, P2pProxyNetworkProbe.Protocol.tcp);
			   
		   // 3 probe tcp socket
		      lSocketAddress = new InetSocketAddress(mPubliAddress,getTcpAdv().getPort());
		      lTcpServerOk = mP2pProxyManagement.probeSocket(lSocketAddress, P2pProxyNetworkProbe.Protocol.tcp);

		      if (lTcpServerOk || lHttpServerOk) {
			   mLog.info("can behave as a super peer :-)");
			   return true;
		   } else {
			   mLog.info("cannot behave as a super peer :-(");
			   return false;
		   }
	   } catch (P2pProxyException e) {
		   mLog.info("cannot behave as a super peer", e);
		   return false;
	   }
   }
   public boolean startUdpProxyIfPossible() {
      int lAudioVideoPort = Integer.parseInt(mProperties.getProperty(MediaResourceService.AUDIO_VIDEO_LOCAL_PORT,"16000"));
      InetSocketAddress  lSocketAddress=null;
      try {
          lSocketAddress = new InetSocketAddress(mPubliAddress,lAudioVideoPort);
          mP2pProxyManagement.probeSocket(lSocketAddress, P2pProxyNetworkProbe.Protocol.udp);
         return true;
      }catch (P2pProxyException e) {
          mLog.info("cannot open audio server on ["+lSocketAddress+"]", e);
      }      
   return false;
   }

}
