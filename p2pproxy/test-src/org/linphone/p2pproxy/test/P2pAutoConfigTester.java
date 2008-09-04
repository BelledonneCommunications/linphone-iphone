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
import org.linphone.p2pproxy.core.P2pProxyInstanceImpl;

public class P2pAutoConfigTester extends TestCase {
   static private P2pProxyInstance mP2pProxyInstance;
   private final static Logger mLog = Logger.getLogger(P2pAutoConfigTester.class);
   //@BeforeClass
   public static void setUpBeforeClass() throws Exception {
      // setup logging
      PropertyConfigurator.configure("log4j.properties");
 
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

     
      // setup edge
      mP2pProxyInstance = new P2pProxyInstanceImpl();
      mP2pProxyInstance.setMode(Mode.edge);
      mP2pProxyInstance.setIndex(1);
      mP2pProxyInstance.start();
      while (mP2pProxyInstance.isStarted() == false) Thread.sleep(500);
      
   }

   //@Before
   public void setUp() throws Exception {
      if (mP2pProxyInstance == null) {
         setUpBeforeClass();
      }
   }
   /**
    * 
    */
   public void testGetPublicAddress() {
      try {
         Assert.assertEquals("wrong public ip" ,mP2pProxyInstance.getManager().getPublicIpAddress().getHostAddress(), InetAddress.getLocalHost().getHostAddress());
         mLog.info("testGetPublicAddress ok");
      } catch (Exception e) {
          mLog.error("testGetPublicAddress ko",e);
          Assert.fail(e.getMessage());
      }  
   }
   /**
    * 
    */
   public void testProbeTcpSocket() {
      try {
         InetSocketAddress  lSocketAddress = new InetSocketAddress(InetAddress.getLocalHost(),9500);
         Assert.assertTrue("cannot prob" ,mP2pProxyInstance.getManager().probeSocket(lSocketAddress, P2pProxyNetworkProbe.Protocol.tcp));
         mLog.info("testProbeSocket ok");
      } catch (Exception e) {
          mLog.error("testProbeSocket ko",e);
          Assert.fail(e.getMessage());
      }  
   }
   /**
    * 
    */
   public void testProbeUdpSocket() {
      try {
         InetSocketAddress  lSocketAddress = new InetSocketAddress(InetAddress.getLocalHost(),9500);
         Assert.assertTrue("cannot prob" ,mP2pProxyInstance.getManager().probeSocket(lSocketAddress, P2pProxyNetworkProbe.Protocol.udp));
         mLog.info("testProbeSocket ok");
      } catch (Exception e) {
          mLog.error("testProbeSocket ko",e);
          Assert.fail(e.getMessage());
      }  
   }
}
