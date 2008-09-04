/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

P2pAutoConfigTester.java - .

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
import java.net.InetSocketAddress;
import java.net.SocketAddress;

import junit.framework.Assert;
import junit.framework.TestCase;
import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;
import org.linphone.p2pproxy.api.P2pProxyInstance;
import org.linphone.p2pproxy.api.P2pProxyNetworkProbe;
import org.linphone.p2pproxy.api.P2pProxyInstance.Mode;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.P2pProxyAccountManagement;
import org.linphone.p2pproxy.core.P2pProxyInstanceImpl;
import org.linphone.p2pproxy.core.P2pProxyMain;
import org.linphone.p2pproxy.test.utils.SipClient;

public class UdpRelayTester extends TestCase {
   static private P2pProxyInstance mP2pProxyInstance;
   private static P2pProxyAccountManagement mP2pProxyAccountManagement;
   static final String mDefaultSipIdentity = "sip:p2pTester@p2p.linphone.org";
   static final private String mCallerUri = "sip:caller@p2p.linphone.org";
   static final private String mCalleeUri = "sip:callee@p2p.linphone.org";
   
   private final static Logger mLog = Logger.getLogger(UdpRelayTester.class);
   
   //@BeforeClass
   public static void setUpBeforeClass() throws Exception {
      // setup logging
      //PropertyConfigurator.configure("log4j.properties");
 
//      // 1 setup relays
//      String lRunString = "java -cp eclipsebuild:dependencies/*.jar ";
//      //lRunString +=" -Xdebug -Xrunjdwp:transport=dt_socket,address=8000,server=y,suspend=n";
//      lRunString +=" org.linphone.p2pproxy.core.P2pProxyMain";
//      lRunString +=" -jxta " +System.getProperty("user.home")+"/P2pAutoConfigTester-seeding";
//      lRunString +=" -sip 5040 -seeding-server ";
//      lRunString +=" -seeding-relay http://"+InetAddress.getLocalHost().getHostAddress()+":"+P2pProxyInstance.BASE_HTTP;
//      lRunString +=" -seeding-rdv http://"+InetAddress.getLocalHost().getHostAddress()+":"+P2pProxyInstance.BASE_HTTP;
//      mLog.info("starting ["+lRunString+"]");
//      Process lProcess = Runtime.getRuntime().exec(lRunString);

      System.setProperty("org.linphone.p2pproxy.home", ".");
      P2pProxyMain.staticLoadTraceConfigFile();
      // setup edge
      mP2pProxyInstance = new P2pProxyInstanceImpl();
      mP2pProxyInstance.setMode(Mode.edge);
      mP2pProxyInstance.setIndex(1);
      mP2pProxyInstance.start();
      while (mP2pProxyInstance.isStarted() == false) Thread.sleep(500);
      
      mP2pProxyAccountManagement = new P2pProxyAccountManagement((JxtaNetworkManager)mP2pProxyInstance.getOpaqueNetworkManager());

      try {
         mP2pProxyAccountManagement.createAccount(mDefaultSipIdentity);
       } catch (Exception e) {
         mLog.warn(e);
      }           
       try {
          mP2pProxyAccountManagement.createAccount(mCallerUri);
        } catch (Exception e) {
          mLog.warn(e);
       }

        try {
           mP2pProxyAccountManagement.createAccount(mCalleeUri);
         } catch (Exception e) {
           mLog.warn(e);
        }
   }

   //@Before
   public void setUp() throws Exception {
      if (mP2pProxyInstance == null) {
         setUpBeforeClass();
      }
   }
   public void testAddressRequest() throws Exception {
      try {
         
         Assert.assertTrue("cannot get relay addresses" ,mP2pProxyInstance.getRtpRelayManager().getAddresses().size()!=0);
         mLog.info("testAddressRequest ok");
      } catch (Exception e) {
          mLog.error("testAddressRequest ko",e);
          Assert.fail(e.getMessage());
      }  
   }
   public void testCallWithRelay() throws Exception {
      try {
         SipClient lCaller = new SipClient(mP2pProxyInstance.getSipClientProvider(),mCallerUri,mP2pProxyInstance.getAdvertisementDiscoveryTimeout());
         lCaller.register();
         SipClient lCallee = new SipClient(mP2pProxyInstance.getSipClientProvider(),mCalleeUri,mP2pProxyInstance.getAdvertisementDiscoveryTimeout());
         lCallee.register();
         
         mLog.info("testCallWithRelay ok");
      } catch (Exception e) {
          mLog.error("testCallWithRelay ko",e);
          Assert.fail(e.getMessage());
      }  
      
   }
}
