/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

P2pProxyMain.java - main class.

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


import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.management.ManagementFactory;
import java.net.URI;
import java.net.URL;
import java.util.InvalidPropertiesFormatException;
import java.util.Properties;

import javax.management.ObjectName;

import net.jxta.exception.JxtaException;
import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxyManagement;
import org.linphone.p2pproxy.api.P2pProxyNotReadyException;
import org.linphone.p2pproxy.api.P2pProxyResourceManagement;
import org.linphone.p2pproxy.api.P2pProxyUserAlreadyExistException;
import org.linphone.p2pproxy.core.media.MediaResourceService;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrar;
import org.zoolu.sip.provider.SipStack;
import org.linphone.p2pproxy.launcher.P2pProxylauncherConstants;

public class P2pProxyMain  implements P2pProxyMainMBean {
   private final static Logger mLog = Logger.getLogger(P2pProxyMain.class);
   private  static JxtaNetworkManager mJxtaNetworkManager;
   private  static ServiceProvider mServiceProvider;
   private  static P2pProxyManagement mP2pProxyManagement;
   private  static SipProxyRegistrar mSipAndPipeListener;
   private static P2pProxyAccountManagementMBean mP2pProxyAccountManagement;
   private static P2pProxyResourceManagement mP2pProxySipProxyRegistrarManagement;
   public final static String ACCOUNT_MGR_MBEAN_NAME="org.linphone.p2proxy:type=account-manager";
   public final static String PROXY_REG_MBEAN_NAME="org.linphone.p2proxy:type=proxy-registrar";
   public final static String MAIN_MBEAN_NAME="org.linphone.p2proxy:type=main";
   private static P2pProxyMain mP2pProxyMain = new P2pProxyMain();
   private static Configurator mConfigurator;
   private static String mConfigHomeDir;
   static private boolean mExit = false;
   static private boolean isReady = false;
   
   
   static {
//      System.setProperty("com.sun.management.jmxremote", "true");
//      System.setProperty("com.sun.management.jmxremote.port", "6789");
//      System.setProperty("com.sun.management.jmxremote.authenticate", "false");
//      System.setProperty("com.sun.management.jmxremote.ssl", "false");
   }

   /**
    * @param args
    * @throws P2pProxyException 
    * @throws InterruptedException 
    * @throws JxtaException 
    * @throws IOException 
    * @throws FileNotFoundException 
    * @throws InvalidPropertiesFormatException 
    */
   public static void main(String[] args) {
	   try {
		   mConfigHomeDir=System.getProperty("user.home")+"/.p2pproxy";
		   int lsipPort=5040;
		   int lMediaPort=MediaResourceService.AUDIO_VIDEO_LOCAL_PORT_DEFAULT_VALUE;
		   int lP2pPort = 9701;
		   JxtaNetworkManager.Mode lMode = JxtaNetworkManager.Mode.auto;
		   // setup logging

		   // get config dire first
		   for (int i=0; i < args.length; i=i+2) {  
			   String argument = args[i];
			   if (argument.equals("-jxta")) {
				   mConfigHomeDir = args[i + 1];
				   File lFile = new File(mConfigHomeDir);
				   if (lFile.exists() == false) lFile.mkdir();
				 
				   System.out.println("mConfigHomeDir detected[" + mConfigHomeDir + "]");
			   } 
		   }
		   System.setProperty("org.linphone.p2pproxy.home", mConfigHomeDir);
		  
		   
		   System.setProperty("net.jxta.logging.Logging", "FINEST");
		   System.setProperty("net.jxta.level", "FINEST");
		   
		   mP2pProxyMain.loadTraceConfigFile();
		   
		   
		   
		   mLog.info("p2pproxy initilizing...");

		   File lPropertyFile =  new File(mConfigHomeDir+"/p2pproxy.properties.xml");
		   mConfigurator = new Configurator(lPropertyFile);
		   try {
			   ObjectName lObjectName = new ObjectName(MAIN_MBEAN_NAME);
			   ManagementFactory.getPlatformMBeanServer().registerMBean(mP2pProxyMain,lObjectName);


		   } catch (Exception e) {
			   mLog.warn("cannot register MBean",e);
		   }         
	
		   // get other params
		   for (int i=0; i < args.length; i=i+2) {  
			   String argument = args[i];
			   if (argument.equals("-jxta") || argument.equals("-home")) {
				   mConfigHomeDir = args[i + 1];
				   //nop
			   } else if (argument.equals("-sip")) {
				   lsipPort = Integer.parseInt(args[i + 1]);
				   System.out.println("sipPort detected[" + lsipPort + "]");
				   mConfigurator.setProperty(SipProxyRegistrar.REGISTRAR_PORT, Integer.toString(lsipPort));
			   } else if (argument.equals("-media")) {
                  lMediaPort = Integer.parseInt(args[i + 1]);
                  System.out.println("media detected[" + lMediaPort + "]");
                  mConfigurator.setProperty(MediaResourceService.AUDIO_VIDEO_LOCAL_PORT, Integer.toString(lMediaPort));
              } else if (argument.equals("-p2p")) {
                  lP2pPort = Integer.parseInt(args[i + 1]);
                  System.out.println("p2p port detected[" + lP2pPort + "]");
                  mConfigurator.setProperty(JxtaNetworkManager.TCP_LISTENING_PORT, Integer.toString(lP2pPort));
              } else if (argument.equals("-relay")) {
				   lMode = JxtaNetworkManager.Mode.relay;
				   mConfigurator.setProperty(JxtaNetworkManager.MODE, lMode.name());
				   System.out.println("relay mode detected");
				   i--;
			   } else if (argument.equals("-edge-only")) {
				   lMode = JxtaNetworkManager.Mode.edge;
				   mConfigurator.setProperty(JxtaNetworkManager.MODE, lMode.name());
				   System.out.println("edge only mode detected");
				   i--;
			   }else if (argument.equals("-seeding-server")) {
				   lMode = JxtaNetworkManager.Mode.seeding_server;
				   mConfigurator.setProperty(JxtaNetworkManager.MODE, lMode.name());
				   System.out.println("seeding-server  detected");
				   i--;
			   } else if (argument.equals("-auto-config")) {
				   lMode = JxtaNetworkManager.Mode.auto;
				   mConfigurator.setProperty(JxtaNetworkManager.MODE, lMode.name());
				   System.out.println("auto-mode mode detected");
				   i--;
			   }  else if (argument.equals("-seeding-rdv")) {
				   mConfigurator.setProperty(JxtaNetworkManager.SEEDING_RDV, args[i + 1]);
				   System.out.println("seeding rdv detected[" + args[i + 1] + "]");
			   }
			   else if (argument.equals("-seeding-relay")) {
				   mConfigurator.setProperty(JxtaNetworkManager.SEEDING_RELAY, args[i + 1]);
				   System.out.println("seeding relay detected[" + args[i + 1] + "]");
			   }  else if (argument.equals("-seeding")) {
				   mConfigurator.setProperty(JxtaNetworkManager.SEEDING_RDV, args[i + 1]);
				   mConfigurator.setProperty(JxtaNetworkManager.SEEDING_RELAY, args[i + 1]);
				   System.out.println("seeding  detected[" + args[i + 1] + "]");
			   }
			   else if (argument.equals("-public-address")) {
				   mConfigurator.setProperty(JxtaNetworkManager.HTTP_LISTENING_PUBLIC_ADDRESS,args[i + 1]+":9700");
				   mConfigurator.setProperty(JxtaNetworkManager.TCP_LISTENING_PUBLIC_ADDRESS,args[i + 1]+":"+lP2pPort);
                   mConfigurator.setProperty(MediaResourceService.AUDIO_VIDEO_PUBLIC_URI,"udp://"+args[i + 1]+":"+lMediaPort);
                   mConfigurator.setProperty(SipProxyRegistrar.REGISTRAR_PUBLIC_ADDRESS,args[i + 1]);
                   System.out.println("public address detected[" + args[i + 1] + "]");
			   }            
			   else
			   {
				   System.out.println("Invalid option: " + args[i]);
				   usage();
				   System.exit(1);
			   }
		   }

		   File lJxtaDirectory = new File (mConfigHomeDir);
		   if (lJxtaDirectory.exists() == false) lJxtaDirectory.mkdir();


		   switch (lMode) {
		   case edge:
			   startEdge(mConfigurator,lJxtaDirectory);
			   break;
		   case relay:
			   startRelay(mConfigurator,lJxtaDirectory);
			   break;
		   case seeding_server:
			   startSeeding(mConfigurator,lJxtaDirectory);
			   break; 
		   case auto:
			   //1 start edge 
			   startEdge(mConfigurator,lJxtaDirectory);
			   // check if peer mode required
			   if (mP2pProxyManagement.shouldIBehaveAsAnRdv() == true) {
				   String lPublicAddress = mP2pProxyManagement.getPublicIpAddress().getHostAddress();
				   mConfigurator.setProperty(JxtaNetworkManager.HTTP_LISTENING_PUBLIC_ADDRESS, lPublicAddress+":9700");
				   mConfigurator.setProperty(JxtaNetworkManager.TCP_LISTENING_PUBLIC_ADDRESS, lPublicAddress+":9701");
				   mServiceProvider.stop();
				   mJxtaNetworkManager.stop();
				   startRelay(mConfigurator,lJxtaDirectory);
				   mJxtaNetworkManager.getPeerGroup().getRendezVousService().setAutoStart(true);
			   }
			   break;
		   default:
			   mLog.fatal("unsupported mode ["+lMode+"]");
		   System.exit(1);

		   }


		   //set management
		   try {
			   ObjectName lObjectName = new ObjectName(ACCOUNT_MGR_MBEAN_NAME);
			   ManagementFactory.getPlatformMBeanServer().registerMBean(mP2pProxyAccountManagement,lObjectName);
		   } catch (Exception e) {
			   mLog.warn("cannot register MBean",e);
		   }


		   mLog.warn("p2pproxy initilized");
		   isReady = true;
		   while (mExit == false) {
			   Thread.sleep(1000);
		   }
		   if (mServiceProvider!= null) mServiceProvider.stop();
		   if (mServiceProvider!= null) mServiceProvider.stop();
		   if (mSipAndPipeListener!= null) mSipAndPipeListener.stop();
		   if (mJxtaNetworkManager != null) mJxtaNetworkManager.stop();
		   mLog.info("p2pproxy stopped");
		   return;
		   
	   } catch (Exception e) {
		   mLog.fatal("error",e);
		   System.exit(1);
	   }
   }
   private static void startEdge(Configurator aProperties,File aConfigDir) throws Exception{
      // setup jxta
      mJxtaNetworkManager = new JxtaNetworkManager(aProperties,aConfigDir);
      mServiceProvider = new EdgePeerServiceManager(aProperties, mJxtaNetworkManager);
      mP2pProxyManagement = (P2pProxyManagement) mServiceProvider;
      mP2pProxySipProxyRegistrarManagement = (P2pProxyResourceManagement) mServiceProvider;
	   //setup account manager
	   mP2pProxyAccountManagement = new P2pProxyAccountManagement(mJxtaNetworkManager);
      mServiceProvider.start(3000L);
   }

   private static void startRelay(Configurator aProperties,File aConfigDir) throws Exception{
      // setup jxta
      mJxtaNetworkManager = new JxtaNetworkManager(aProperties,aConfigDir);
      mServiceProvider = new SuperPeerServiceManager(aProperties, mJxtaNetworkManager);
      mP2pProxyManagement = (P2pProxyManagement) mServiceProvider;
      mP2pProxySipProxyRegistrarManagement = (P2pProxyResourceManagement) mServiceProvider;
      mServiceProvider.start(3000L);
	   //setup account manager
	   mP2pProxyAccountManagement = new P2pProxyAccountManagement(mJxtaNetworkManager);
//    setup sip provider
	   SipStack.log_path = mConfigHomeDir+"/logs";
	   mSipAndPipeListener = new SipProxyRegistrar(mConfigurator,mJxtaNetworkManager,mP2pProxyAccountManagement);
	   //set management
	   try {
		   ObjectName lObjectName  = new ObjectName(PROXY_REG_MBEAN_NAME);
		   ManagementFactory.getPlatformMBeanServer().registerMBean(mSipAndPipeListener,lObjectName);

	   } catch (Exception e) {
		   mLog.warn("cannot register MBean",e);
	   }
   }
   private static void startSeeding(Configurator aProperties,File aConfigDir) throws Exception{
      // setup jxta
      mJxtaNetworkManager = new JxtaNetworkManager(aProperties,aConfigDir);
      mServiceProvider = new SeedingPeerServiceManager(aProperties, mJxtaNetworkManager,true);
      mP2pProxyManagement = null;
      mP2pProxySipProxyRegistrarManagement = (P2pProxyResourceManagement) mServiceProvider;
      mServiceProvider.start(3000L);
	   //setup account manager
	   mP2pProxyAccountManagement = new P2pProxyAccountManagement(mJxtaNetworkManager);
	   //    setup sip provider
	   SipStack.log_path = mConfigHomeDir+"/logs";
	   mSipAndPipeListener = new SipProxyRegistrar(mConfigurator,mJxtaNetworkManager,mP2pProxyAccountManagement);
	   //set management
	   try {
		   ObjectName lObjectName  = new ObjectName(PROXY_REG_MBEAN_NAME);
		   ManagementFactory.getPlatformMBeanServer().registerMBean(mSipAndPipeListener,lObjectName);

	   } catch (Exception e) {
		   mLog.warn("cannot register MBean",e);
	   }
   }   
   private static void usage() {
      System.out.println("p2pproxy");
      System.out.println("-home : directory where configuration/cache is located  (including jxta cache.default is $HOME/.p2pproxy");
      System.out.println("-sip : udp proxy port, default 5060");
      System.out.println("-media : udp relay/stun port, default 16000");
      System.out.println("-p2p : p2p tcp port, default 9701");
      System.out.println("-relay : super peer mode");
      System.out.println("-edge-only : edge mode");
      System.out.println("-seeding-server : seeding server mode");
      System.out.println("-auto-config : automatically choose edge or relay (default mode)");
      System.out.println("-seeding : list of boostrap rdv separated by | (ex tcp://127.0.0.1:9701|http://127.0.0.2:9700)");
      System.out.println("-public-address : ip as exported to peers (ex myPublicAddress.no-ip.org)");
   }
  
public void loadTraceConfigFile() throws P2pProxyException {
   staticLoadTraceConfigFile();
   }
public  static void staticLoadTraceConfigFile()  throws P2pProxyException {
   try {
      String lSearchDir;
      //search build dir
      lSearchDir = System.getProperty("org.linphone.p2pproxy.build.dir");
      File lFile = new File(lSearchDir+"/log4j.properties");
      if (lFile.exists() == false) {
         lSearchDir = mConfigHomeDir;
         lFile = new File(lSearchDir+"/log4j.properties");
         if (lFile.exists() == false) {
            lSearchDir=".";
         }
      }
      String lLog4jFile= lSearchDir+"/log4j.properties";
      PropertyConfigurator.configureAndWatch(lLog4jFile);
      // read java.util.logging properties 
      Properties lLogginProperties = new Properties();
      lLogginProperties.load(new FileInputStream(lLog4jFile));
      lLogginProperties.setProperty("java.util.logging.FileHandler.pattern",System.getProperty("org.linphone.p2pproxy.home")+"/logs/p2pproxy.log");
      lLogginProperties.store(new FileOutputStream(lLog4jFile+".tmp"), "tmp");
      System.setProperty("java.util.logging.config.file",lLog4jFile+".tmp");
      java.util.logging.LogManager.getLogManager().readConfiguration();
   } catch (Exception e) {
      throw new P2pProxyException("enable to load traces",e);
   }
}

private static void isReady() throws P2pProxyNotReadyException {
    try {
      if ((isReady == true && mJxtaNetworkManager.isConnectedToRendezVous(0) == true) 
    	  || 
    	  (isReady == true && mJxtaNetworkManager.getPeerGroup().getRendezVousService().isRendezVous())) {
         //nop connected
      } else {
    	  if (mJxtaNetworkManager != null ) {
    		  throw new P2pProxyNotReadyException("not connected to any rdv: status ["+mJxtaNetworkManager.getPeerGroup().getRendezVousService().getRendezVousStatus()+"]");
    	  } else {
    		  throw new P2pProxyNotReadyException("initializing");
    	  }
      }
   } catch (InterruptedException e) {
      throw new P2pProxyNotReadyException(e);
   }
}
/* p2pproxy.h implementation*/

public static int createAccount(String aUserName) {
   try {
      isReady();
      mP2pProxyAccountManagement.createAccount(aUserName);
   } catch (P2pProxyUserAlreadyExistException e) {
      return P2pProxylauncherConstants.P2PPROXY_ACCOUNTMGT_USER_EXIST;
   } catch (P2pProxyException e) {
      return P2pProxylauncherConstants.P2PPROXY_ERROR;
   }
   return P2pProxylauncherConstants.P2PPROXY_NO_ERROR;
}
public static int deleteAccount(String aUserName)  {
   try {
      isReady();
      mP2pProxyAccountManagement.deleteAccount(aUserName);
   } catch (P2pProxyException e) {
      return P2pProxylauncherConstants.P2PPROXY_ERROR;
   }
   return P2pProxylauncherConstants.P2PPROXY_NO_ERROR;

}
public static int isValidAccount(String aUserName){
   try {
      isReady();
      if (mP2pProxyAccountManagement.isValidAccount(aUserName)) {
         return P2pProxylauncherConstants.P2PPROXY_ACCOUNTMGT_USER_EXIST;
      } else {
         return P2pProxylauncherConstants.P2PPROXY_ACCOUNTMGT_USER_NOT_EXIST;
      }
   } catch (P2pProxyException e) {
      return P2pProxylauncherConstants.P2PPROXY_ERROR;
   }
}
public static String lookupSipProxyUri(String aDomaine) {
   try {
      isReady();
      String[] lProxies = mP2pProxySipProxyRegistrarManagement.lookupSipProxiesUri(aDomaine);
      if (lProxies.length != 0) {
    	  return lProxies[0];
      } else {
    	  return null;
      }
   } catch (Exception e) {
      return null;
   } 
}

public static String[] lookupSipProxiesUri(String aDomaine) {
	   try {
	      isReady();
	      return mP2pProxySipProxyRegistrarManagement.lookupSipProxiesUri(aDomaine);
	   } catch (Exception e) {
	      return null;
	   } 
	}

public static String[] lookupMediaServerAddress(String aDomaine) {
   try {
      isReady();
      return mP2pProxySipProxyRegistrarManagement.getMediaServerList();
   } catch (Exception e) {
	   mLog.error("cannot find media resource",e);
	   return null;
   } 
}

public static int getState() {
   try {
      isReady();
      return P2pProxylauncherConstants.P2PPROXY_CONNECTED;
   } catch (P2pProxyException e) {
      mLog.error("cannot get state",e);
      return P2pProxylauncherConstants.P2PPROXY_NOT_CONNECTED;
   }   
}
public static int revokeSipProxy(String aProxy) {
   try {
      isReady();
      mP2pProxySipProxyRegistrarManagement.revokeSipProxy(aProxy);
      return P2pProxylauncherConstants.P2PPROXY_NO_ERROR;
   } catch (P2pProxyException e) {
      return P2pProxylauncherConstants.P2PPROXY_NOT_CONNECTED;
   }   
}
public static int revokeMediaServer(String aServer) {
	   try {
	      isReady();
	      mP2pProxySipProxyRegistrarManagement.revokeMediaServer(aServer);
	      return P2pProxylauncherConstants.P2PPROXY_NO_ERROR;
	   } catch (P2pProxyException e) {
	      return P2pProxylauncherConstants.P2PPROXY_NOT_CONNECTED;
	   }   
	}
public static void stop() {
   mExit = true;
  
}
}