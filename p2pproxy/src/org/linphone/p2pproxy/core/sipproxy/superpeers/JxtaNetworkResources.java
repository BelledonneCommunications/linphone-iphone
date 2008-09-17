/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

JxtaNetworkResources.java - .

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
package org.linphone.p2pproxy.core.sipproxy.superpeers;

import java.io.IOException;

import net.jxta.discovery.DiscoveryService;
import net.jxta.document.AdvertisementFactory;
import net.jxta.id.IDFactory;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.P2pUserProfileAdvertisement;
import org.zoolu.sip.address.SipURL;

public class JxtaNetworkResources {
   private final JxtaNetworkManager mJxtaNetworkManager;
   private final static Logger mLog = Logger.getLogger(JxtaNetworkResources.class);
   private final P2pUserRegistrationAdvertisement mP2pUserRegistrationAdvertisement;
   private final static long EXPIRATION= 120000;
   
   JxtaNetworkResources (String aUserName, JxtaNetworkManager aJxtaNetworkManager,String aRegistrarAddress) {
      mJxtaNetworkManager = aJxtaNetworkManager;
      mP2pUserRegistrationAdvertisement = (P2pUserRegistrationAdvertisement) AdvertisementFactory.newAdvertisement(P2pUserProfileAdvertisement.getAdvertisementType());
      mP2pUserRegistrationAdvertisement.setID(IDFactory.newCodatID(mJxtaNetworkManager.getPeerGroup().getPeerGroupID(), aUserName.toString().getBytes()));
      mP2pUserRegistrationAdvertisement.setUserName(aUserName.toString());
      mP2pUserRegistrationAdvertisement.setRegistrarAddress(aRegistrarAddress.toString());
   }
   
   void publish(long aLiveTime) throws P2pProxyException {
      try {
         DiscoveryService lDiscoveryService = mJxtaNetworkManager.getPeerGroup().getDiscoveryService();
         if (aLiveTime > 0) {
            lDiscoveryService.publish(mP2pUserRegistrationAdvertisement,aLiveTime,EXPIRATION);
         } else {
            //first flush in any cases
            lDiscoveryService.flushAdvertisement(mP2pUserRegistrationAdvertisement);
         }
      } catch (IOException e1) {
         throw new P2pProxyException(e1);
      }
      mLog.debug("publishing P2pUserRegistration Advertisement published expire ["+aLiveTime+"]");
   }   
   
}
