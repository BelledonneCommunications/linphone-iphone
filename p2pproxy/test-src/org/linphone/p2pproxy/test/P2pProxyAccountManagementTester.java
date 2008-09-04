/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

P2pProxyAccountManagementTester.java - .

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

import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxyUserAlreadyExistException;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.P2pProxyAccountManagement;
import org.linphone.p2pproxy.core.P2pProxyAccountManagementMBean;
import org.linphone.p2pproxy.test.utils.P2pNetwork;

import junit.framework.Assert;
import junit.framework.TestCase;


public class P2pProxyAccountManagementTester extends TestCase {
   private final static Logger mLog = Logger.getLogger(P2pProxyAccountManagementTester.class);
   static private P2pNetwork mP2pNetwork=null;
   static private P2pProxyAccountManagementMBean mP2pProxyAccountManagementA;
   static private final String USER_A = "sip:usera@p2pproxytester.org";
   /* (non-Javadoc)
    * @see junit.framework.TestCase#setUp()
    */
   @Override
   protected void setUp() throws Exception {
      if (mP2pNetwork == null) {
          // setup logging
        PropertyConfigurator.configure("log4j.properties");

         mP2pNetwork = new P2pNetwork(0,1);
         mP2pProxyAccountManagementA = new P2pProxyAccountManagement((JxtaNetworkManager) mP2pNetwork.getSuperPeers().get(0).getOpaqueNetworkManager());
      }
   }

   /* (non-Javadoc)
    * @see junit.framework.TestCase#tearDown()
    */
   @Override
   protected void tearDown() throws Exception {
      // TODO Auto-generated method stub
      super.tearDown();
   }
   public void testCreateAccount() {
      try {
         //1 create
         mP2pProxyAccountManagementA.createAccount(USER_A);
         //2 delete
         mP2pProxyAccountManagementA.deleteAccount(USER_A);
      } catch (Exception e) {
         mLog.error("testCreateAccount ko",e);
         Assert.fail(e.getMessage());
     } 
   }

   public void testDeleteAccount() {
      
   }

   public void testCreateAlreadyExistingAccount() {
      
   }
   public void testDeleteNotExistingAccount() {
      
   }

}
