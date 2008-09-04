/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

P2pProxyAccountManagement.java - .

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
import java.util.List;

import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxyUserNotFoundException;

import org.linphone.p2pproxy.api.P2pProxyUserAlreadyExistException;

public class P2pProxyAccountManagement implements ServiceProvider, P2pProxyAccountManagementMBean {
   private final static Logger mLog = Logger.getLogger(P2pProxyAccountManagement.class);
   protected final JxtaNetworkManager mJxtaNetworkManager;
   
   public P2pProxyAccountManagement() {
      mJxtaNetworkManager = null;
   }
   /**
    * @param jxtaNetworkManager
    */
   public P2pProxyAccountManagement(final JxtaNetworkManager jxtaNetworkManager) {
      super();
      mJxtaNetworkManager = jxtaNetworkManager;
   }

   public void start(long aTimeOut) throws P2pProxyException {
     mLog.info("P2pProxyAccountManagementMBean started");
   }

   public void stop() {
      mLog.info("P2pProxyAccountManagementMBean stopped");
   }

   public void createAccount(String aUserName) throws P2pProxyException, P2pProxyUserAlreadyExistException {
      // 1 check if already exist
      if (isValidAccount(aUserName) == false) {
         
         // 2 creates and remote publish
         P2pUserProfileAdvertisement lP2pUserProfileAdvertisement = (P2pUserProfileAdvertisement) AdvertisementFactory.newAdvertisement(P2pUserProfileAdvertisement.getAdvertisementType());
         
         lP2pUserProfileAdvertisement.setID(IDFactory.newCodatID(mJxtaNetworkManager.getPeerGroup().getPeerGroupID()));
         lP2pUserProfileAdvertisement.setUserName(aUserName);
         try {
            mJxtaNetworkManager.getPeerGroup().getDiscoveryService().publish(lP2pUserProfileAdvertisement,DiscoveryService.INFINITE_LIFETIME,DiscoveryService.DEFAULT_EXPIRATION);
         } catch (IOException e1) {
            throw new P2pProxyException(e1);
         }
         mJxtaNetworkManager.getPeerGroup().getDiscoveryService().remotePublish(lP2pUserProfileAdvertisement, DiscoveryService.NO_EXPIRATION);
         mLog.debug("publishing P2pUserProfileAdvertisement :"+lP2pUserProfileAdvertisement);
      } else {
         throw new P2pProxyUserAlreadyExistException(aUserName);
      }
      
   }

   public void deleteAccount(String aUserName) throws P2pProxyException, P2pProxyUserNotFoundException {
      // 1 check if already exist
      try {
         List<? extends Advertisement> lAdvertisements = mJxtaNetworkManager.getAdvertisementList(null, P2pUserProfileAdvertisement.USER_NAME_TAG, aUserName, true);
         if (lAdvertisements.isEmpty()) throw new P2pProxyUserNotFoundException (aUserName +" not found");
         
         // 2 local and remote publish to 0
         mJxtaNetworkManager.getPeerGroup().getDiscoveryService().flushAdvertisement(lAdvertisements.get(0));
         mJxtaNetworkManager.getPeerGroup().getDiscoveryService().remotePublish(lAdvertisements.get(0), 0);
 
      } catch (P2pProxyAdvertisementNotFoundException e) {
         throw e;
     } catch (InterruptedException e) {
         throw new P2pProxyException(e);
      } catch (IOException e) {
         throw new P2pProxyException(e);
      }
      
   }

   public boolean isValidAccount(String aUserName) throws P2pProxyException {
      boolean lStatus = false;
      try {
         if (mJxtaNetworkManager.getAdvertisementList(null, P2pUserProfileAdvertisement.USER_NAME_TAG, aUserName, true).size() >0 ) {
            lStatus = true;;
         }
      } catch (P2pProxyAdvertisementNotFoundException e) {
         lStatus = false;
      }catch (Exception e) {
         mLog.error("cannot check acount",e);
      }
      return lStatus;
   }

 

}
