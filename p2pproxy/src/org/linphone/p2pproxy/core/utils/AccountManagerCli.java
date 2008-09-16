/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

AccountManagerCli.java - .

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
package org.linphone.p2pproxy.core.utils;


import javax.management.MBeanServerConnection;
import javax.management.MBeanServerInvocationHandler;
import javax.management.ObjectName;
import javax.management.remote.JMXConnector;
import javax.management.remote.JMXConnectorFactory;
import javax.management.remote.JMXServiceURL;

import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxyUserAlreadyExistException;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.P2pProxyAccountManagementMBean;
import org.linphone.p2pproxy.core.P2pProxyMain;
import org.linphone.p2pproxy.core.P2pProxyMainMBean;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrar;

public class AccountManagerCli {
   public final static String USER_ADD = "user-add";
   public final static String RELOAD_TRACE = "reload-trace";
   /**
    * @param args
    * @throws Exception 
    */
   public static void main(String[] args)  {
      try {
         // get other params
         String lCommand = "null";
         int lCommandArgsIndex=0;
         for (int i=0; i < args.length; i=i+2) {  
            String argument = args[i];
            if (argument.equals("-c")) {
               lCommand = args[i + 1];
               //nop
            } else {
               // no more option, takes args
               lCommandArgsIndex = i;

            }
         }
         // Create an RMI connector client and
         // connect it to the RMI connector server
         //
         JMXServiceURL lJMXServiceURL = new JMXServiceURL("service:jmx:rmi:///jndi/rmi://:6789/jmxrmi");
         JMXConnector lJMXConnector = JMXConnectorFactory.connect(lJMXServiceURL, null);

         // Get an MBeanServerConnection
         //
         MBeanServerConnection lMBeanServerConnection = lJMXConnector.getMBeanServerConnection();

         
         ObjectName lP2pProxyAccountManagementMBeanName = new ObjectName(P2pProxyMain.ACCOUNT_MGR_MBEAN_NAME);
         ObjectName lP2pProxyMainMBeanName = new ObjectName(P2pProxyMain.MAIN_MBEAN_NAME);
         
         if (USER_ADD.equals(lCommand)) {
            P2pProxyAccountManagementMBean lP2pProxyAccountManagementMBean = (P2pProxyAccountManagementMBean)
            MBeanServerInvocationHandler.newProxyInstance(  lMBeanServerConnection
                  ,lP2pProxyAccountManagementMBeanName
                  ,P2pProxyAccountManagementMBean.class
                  ,true);

            System.out.println("creating user account for "+args[lCommandArgsIndex]);
            System.out.println("please wait");
            try {
               lP2pProxyAccountManagementMBean.createAccount(args[lCommandArgsIndex]);
            } catch (P2pProxyUserAlreadyExistException e) {
               System.out.println("cannot create user account for "+args[lCommandArgsIndex]);
               System.out.println("already exist");

            } 
            System.out.println("done");
         } else if (RELOAD_TRACE.equals(lCommand)) {
            P2pProxyMainMBean lP2pProxyMainMBean = (P2pProxyMainMBean)
            MBeanServerInvocationHandler.newProxyInstance(  lMBeanServerConnection
                  ,lP2pProxyMainMBeanName
                  ,P2pProxyMainMBean.class
                  ,true);

            System.out.println("loading trace config file...");
            try {
               lP2pProxyMainMBean.loadTraceConfigFile();
            } catch (P2pProxyException e) {
               System.out.println("cannot load trace config file");
            } 
            System.out.println("done");         
         }   
            else {
         
            System.out.println("unkwon command ["+lCommand+"]");
            usage();
            System.exit(1);
         }
         lJMXConnector.close();
      }catch (Exception e) {
         e.printStackTrace();
         System.exit(1);
      }



   }

   private static void usage() {
      System.out.println("p2pproxy-cli");
      System.out.println("-c : command to execute {"+USER_ADD+" | "+RELOAD_TRACE+"}");
      System.out.println("-p : client port");
   }
}
