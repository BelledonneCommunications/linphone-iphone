/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

P2pProxyNetworkTester.java - junit test for a whole p2pproxy network

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
import java.util.List;

import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;
import org.linphone.p2pproxy.api.P2pProxyInstance;
import org.linphone.p2pproxy.test.utils.P2pNetwork;
import org.linphone.p2pproxy.test.utils.SipClient;

import junit.framework.Assert;
import junit.framework.TestCase;

public class P2pProxyNetworkingTester extends TestCase {
   static {
	   PropertyConfigurator.configure("log4j.properties");
   }
	static P2pNetwork mP2pNetwork =null;
   static List<SipClient> mClientList = new ArrayList<SipClient>();
   static List<P2pProxyInstance> mProxyInstanceList;
   private final static Logger mLog = Logger.getLogger(P2pProxyNetworkingTester.class);
	public P2pProxyNetworkingTester() {
		// TODO Auto-generated constructor stub
	}

	public P2pProxyNetworkingTester(String arg0) {
		super(arg0);
		// TODO Auto-generated constructor stub
	}
	public void testConnectedRdv() {
	   int lConnectedPeers=0;
       for (P2pProxyInstance lSuperPeer:mP2pNetwork.getSuperPeers()) {
          lConnectedPeers += lSuperPeer.getNumberOfconnectedPeers();
       }
       Assert.assertEquals("all peer are not connected",mP2pNetwork.getEdgesPeers().size(),lConnectedPeers) ;
    mLog.info("testConnectedPeers ok");
    }
    /**
	 *  test call from all peers
	 *
	 */
	public void testBasicCallFromAllPeers() {
	   try {

	      for (int i=0;i<mClientList.size();i++) {
	         SipClient lCaller = mClientList.get(i);
	         for (int j=0;j<mClientList.size();j++) {
	            if (i!=j) {
	               P2pProxyInstance lCallee = mProxyInstanceList.get(j);
	               lCaller.call(lCallee.getSipClientName(),lCallee.getSipClientProvider());
	            }
	         }
	      }
	      mLog.info("testBasicCallFrom ok");
	   } catch (Exception e) {
	      mLog.error("testBasicCallFrom ko",e);
	      Assert.fail(e.getMessage());
	   }
	   
	}
    /**
     *  same as testBasicCallFromAllPeers but wihout seeding relay
     *
     */
    public void testBasicCallWithoutSeedingPeer() {
       try {
          
          
          //1 shutdown seeding peer
          mProxyInstanceList.get(0).stop();
          Thread.sleep(60000);
          int lConnectedPeers=0;
          for (int j=1;j<mP2pNetwork.getSuperPeers().size();j++) {
             lConnectedPeers += mP2pNetwork.getSuperPeers().get(j).getNumberOfconnectedPeers();
          }
          //Assert.assertEquals("all peer are not connected to rdv",mP2pNetwork.getEdgesPeers().size(),lConnectedPeers) ;
          mLog.warn("all peer are not connected to rdv edges["+mP2pNetwork.getEdgesPeers().size()+"connected ["+lConnectedPeers+"]") ;
          //Thread.sleep(5000);
          for (int i=1;i<mClientList.size();i++) {
             SipClient lCaller = mClientList.get(i);
             for (int j=1;j<mClientList.size();j++) {
                if (i!=j) {
                   P2pProxyInstance lCallee = mProxyInstanceList.get(j);
                   lCaller.call(lCallee.getSipClientName(),lCallee.getSipClientProvider());
                }
             }
          }
          mLog.info("testBasicCallWithoutSeedingPeer ok");
       } catch (Exception e) {
          mLog.error("testBasicCallWithoutSeedingPeer ko",e);
          Assert.fail(e.getMessage());
       }
       
    }

   /* (non-Javadoc)
    * @see junit.framework.TestCase#setUp()
    */
   @Override
   protected void setUp() throws Exception {
      super.setUp();
      if (mP2pNetwork == null) {
         mP2pNetwork = new P2pNetwork(4,2,4,0,14,false,System.getProperty("p2pproxy.publicAddressUser",System.getProperty("user.name")),System.getProperty("p2pproxy.publicAddress",InetAddress.getLocalHost().getHostName()));
         Thread.sleep(10000);
         mProxyInstanceList = new ArrayList<P2pProxyInstance>(mP2pNetwork.getAllPeers());
          for (P2pProxyInstance lP2pProxyInstance:mProxyInstanceList) {
             SipClient lSipClient = new SipClient(lP2pProxyInstance.getSipClientProvider(),lP2pProxyInstance.getSipClientName(),lP2pProxyInstance.getAdvertisementDiscoveryTimeout()); 
             lSipClient.register();
             mClientList.add(lSipClient);
          }
          Thread.sleep(10000);
      }
   }

   /* (non-Javadoc)
    * @see junit.framework.TestCase#tearDown()
    */
   @Override
   protected void tearDown() throws Exception {
      // TODO Auto-generated method stub
      super.tearDown();
//      for (P2pProxyInstance lP2pProxyInstance:mProxyInstanceList) {
//         SipClient lSipClient = new SipClient(lP2pProxyInstance.getSipClientProvider(),lP2pProxyInstance.getSipClientName()); 
//         lSipClient.unRegister();
//      }
   }
}
