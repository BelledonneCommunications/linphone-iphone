/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

P2pProxyNatedNetworkTester.java - .

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
package org.linphone.p2pproxy.test;

import java.net.InetAddress;
import java.util.ArrayList;

import org.linphone.p2pproxy.api.P2pProxyInstance;
import org.linphone.p2pproxy.test.utils.P2pNetwork;
import org.linphone.p2pproxy.test.utils.SipClient;


public class P2pProxyNatedNetworkTester extends P2pProxyNetworkingTester {

   /* (non-Javadoc)
    * @see org.linphone.p2pproxy.test.P2pProxyNetworkingTester#setUp()
    */
   @Override
   protected void setUp() throws Exception {
      if (mP2pNetwork == null) {
         mP2pNetwork = new P2pNetwork(1,1,0,0,10,true,System.getProperty("p2pproxy.publicAddressUser",System.getProperty("user.name")),System.getProperty("p2pproxy.publicAddress",InetAddress.getLocalHost().getHostName()));
         mProxyInstanceList = new ArrayList<P2pProxyInstance>(mP2pNetwork.getAllPeers());
          for (P2pProxyInstance lP2pProxyInstance:mProxyInstanceList) {
             SipClient lSipClient = new SipClient(lP2pProxyInstance.getSipClientProvider(),lP2pProxyInstance.getSipClientName(),lP2pProxyInstance.getAdvertisementDiscoveryTimeout()); 
             lSipClient.register();
             mClientList.add(lSipClient);
          }
      }
   }
}
