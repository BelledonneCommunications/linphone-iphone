/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

StunClient.java - .

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
package org.linphone.p2pproxy.core.stun;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.URI;
import java.util.ArrayList;
import java.util.List;

import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.P2pProxyAdvertisementNotFoundException;
import org.linphone.p2pproxy.core.sipproxy.NetworkResourceAdvertisement;

public class StunClient {
   private List<InetSocketAddress> mStunServerList;
   JxtaNetworkManager mJxtaNetworkManager;
   
   StunClient(List<InetSocketAddress> aStunServerList) {
      mStunServerList = aStunServerList;
   }
   StunClient(JxtaNetworkManager aJxtaNetworkManager) throws P2pProxyException {
      //need to acquire stun server address()
      mJxtaNetworkManager = aJxtaNetworkManager;
      try {
         mStunServerList = acquireStunServerAddress();
      } catch (Exception e) {
         throw new P2pProxyException(e);
      }
   }   
   private List<InetSocketAddress> acquireStunServerAddress() throws P2pProxyAdvertisementNotFoundException, InterruptedException, IOException {
      List<NetworkResourceAdvertisement> lStunServerAdv = (List<NetworkResourceAdvertisement>) mJxtaNetworkManager.getAdvertisementList(null, StunServer.ADV_NAME, true);
      List<InetSocketAddress> lSocketAddressList = new ArrayList<InetSocketAddress>(lStunServerAdv.size());
      for (NetworkResourceAdvertisement lNetworkResourceAdvertisement: lStunServerAdv) {
         URI lServerUri = URI.create(lNetworkResourceAdvertisement.getAddress());
         lSocketAddressList.add(new InetSocketAddress(lServerUri.getHost(),lServerUri.getPort()));
      }
      return lSocketAddressList;
   }
   
   
}
