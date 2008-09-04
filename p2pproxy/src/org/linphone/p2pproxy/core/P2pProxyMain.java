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
import org.linphone.p2pproxy.core.media.rtprelay.RtpRelayService;
import org.zoolu.sip.provider.SipStack;

public class P2pProxyMain  implements P2pProxyMainMBean {
   private final static Logger mLog = Logger.getLogger(P2pProxyMain.class);
   private  static JxtaNetworkManager mJxtaNetworkManager;
   private  static ServiceProvider mServiceProvider;
   private  static P2pProxyManagement mP2pProxyManagement;
   private  static SipProxyRegistrar mSipAndPipeListener;
   private static P2pProxyAccountManagementMBean mP2pProxyAccountManagement;
   public final static String ACCOUNT_MGR_MBEAN_NAME="org.linphone.p2proxy:type=account-manager";
   public final static String PROXY_REG_MBEAN_NAME="org.linphone.p2proxy:type=proxy-registrar";
   public final static String MAIN_MBEAN_NAME="org.linphone.p2proxy:type=main";
   private static P2pProxyMain mP2pProxyMain = new P2pProxyMain();
   private static Configurator mConfigurator;
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
		   String lconfigHomeDir=System.getProperty("user.home")+"/.p2pproxy";
		   int lsipPort=5040;
		   JxtaNetworkManager.Mode lMode = JxtaNetworkManager.Mode.auto;
		   boolean lEnableHttp = false;
		   // setup logging

		   // get config dire first
		   for (int i=0; i < args.length; i=i+2) {  
			   String argument = args[i];
			   if (argument.equals("-jxta")) {
				   lconfigHomeDir = args[i + 1];
				   File lFile = new File(lconfigHomeDir);
				   if (lFile.exists() == false) lFile.mkdir();
				 
				   System.out.println("lconfigHomeDir detected[" + lconfigHomeDir + "]");
			   } 
		   }
		   System.setProperty("org.linphone.p2pproxy.home", lconfigHomeDir);
		  
		   
		   System.setProperty("net.jxta.logging.Logging", "FINEST");
		   System.setProperty("net.jxta.level", "FINEST");
		   
		   mP2pProxyMain.loadTraceConfigFile();
		   
		   
		   
		   mLog.info("p2pproxy initilizing...");

		   File lPropertyFile =  new File(lconfigHomeDir+"/p2pproxy.properties.xml");
		   mConfigurator = new Configurator(lPropertyFile);
		   try {
			   ObjectName lObjectName = new ObjectName(MAIN_MBEAN_NAME);
			   ManagementFactory.getPlatformMBeanServer().registerMBean(mP2pProxyMain,lObjectName);


		   } catch (Exception e) {
			   mLog.warn("cannot register MBean",e);
		   }         
		   String lSocksHost = null;
		   String lSocksPort = null;
		   //         if (args.length <= 0) {
//		   usage();
//		   System.exit(1);
//		   }
		   // get other params
		   for (int i=0; i < args.length; i=i+2) {  
			   String argument = args[i];
			   if (argument.equals("-jxta")) {
				   lconfigHomeDir = args[i + 1];
				   //nop
			   } else if (argument.equals("-sip")) {
				   lsipPort = Integer.parseInt(args[i + 1]);
				   System.out.println("sipPort detected[" + lsipPort + "]");
				   mConfigurator.setProperty(SipProxyRegistrar.REGISTRAR_PORT, Integer.toString(lsipPort));
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
			   }
			   else if (argument.equals("-public-address")) {
				   mConfigurator.setProperty(JxtaNetworkManager.HTTP_LISTENING_PUBLIC_ADDRESS,args[i + 1]+":9700");
				   mConfigurator.setProperty(JxtaNetworkManager.TCP_LISTENING_PUBLIC_ADDRESS,args[i + 1]+":9701");
                   mConfigurator.setProperty(RtpRelayService.AUDIO_VIDEO_PUBLIC_URI,"udp://"+args[i + 1]+":16000");
                   System.out.println("public address detected[" + args[i + 1] + "]");
			   }            
			   else if (argument.equals("-socks-url")) {
				   try {
				      URI lSocksUrl = new URI(args[i + 1]);
					   lSocksHost = lSocksUrl.getHost();
					   lSocksPort = Integer.toString(lSocksUrl.getPort());
				   }catch (Exception e) {
					   mLog.warn("enable to get socks proxy from env",e);
				   }
				   System.out.println("socks serveur detected[" + args[i + 1] + "]");
			   } else if (argument.equals("-enable-http-client")) {
				   lEnableHttp = true;
				   mConfigurator.setProperty(JxtaNetworkManager.ENABLE_HTTP_CLIENT, "true");
				   System.out.println("enable-http mode detected");
				   i--;
			   }            
			   else
			   {
				   System.out.println("Invalid option: " + args[i]);
				   usage();
				   System.exit(1);
			   }
		   }
		   String lProxyUrlString = null;
		   String lProxyHost = null;
		   String lProxyPort = null;
		   //configure http proxy 
		   if ((lProxyUrlString = System.getenv("http_proxy")) != null) {
			   //get from env
			   try {
				   URL lProxyUrl = new URL(lProxyUrlString);
				   lProxyHost = lProxyUrl.getHost();
				   lProxyPort = Integer.toString(lProxyUrl.getPort());
			   }catch (Exception e) {
				   mLog.warn("enable do get http proxy from env",e);
			   }
		   }
		   //check from config 
		   if (lProxyHost != null || (lProxyHost = mConfigurator.getProperty("http.proxyHost")) != null)  {
			   System.setProperty("http.proxyHost", lProxyHost);
		   }
		   if (lProxyPort != null || (lProxyPort = mConfigurator.getProperty("http.proxyPort")) != null) {
			   System.setProperty("http.proxyPort", lProxyPort);
		   }
		   //configure socks proxy
		   if ((lProxyUrlString = System.getenv("socks_proxy")) != null) {
			   //get from env

		   }
		   //check from config 
		   if (lSocksHost != null || (lSocksHost = mConfigurator.getProperty("socksProxyHost")) != null)  {
			   System.setProperty("socksProxyHost", lSocksHost);
		   }
		   if (lSocksPort != null || (lSocksPort = mConfigurator.getProperty("socksProxyPort")) != null) {
			   System.setProperty("socksProxyPort", lSocksPort);
		   }

		   //check from env

		   File lJxtaDirectory = new File (lconfigHomeDir);
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

		   //setup account manager
		   mP2pProxyAccountManagement = new P2pProxyAccountManagement(mJxtaNetworkManager);
		   // setup sip provider
		   SipStack.log_path = lconfigHomeDir+"/logs";
		   mSipAndPipeListener = new SipProxyRegistrar(mConfigurator,mJxtaNetworkManager,mP2pProxyAccountManagement,mP2pProxyManagement);
		   //set management
		   try {
			   ObjectName lObjectName = new ObjectName(ACCOUNT_MGR_MBEAN_NAME);
			   ManagementFactory.getPlatformMBeanServer().registerMBean(mP2pProxyAccountManagement,lObjectName);

			   lObjectName = new ObjectName(PROXY_REG_MBEAN_NAME);
			   ManagementFactory.getPlatformMBeanServer().registerMBean(mSipAndPipeListener,lObjectName);

		   } catch (Exception e) {
			   mLog.warn("cannot register MBean",e);
		   }


		   mLog.warn("p2pproxy initilized");

		   while (true) {
			   Thread.sleep(1000);
		   }
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
      mServiceProvider.start(3000L);
   }

   private static void startRelay(Configurator aProperties,File aConfigDir) throws Exception{
      // setup jxta
      mJxtaNetworkManager = new JxtaNetworkManager(aProperties,aConfigDir);
      mServiceProvider = new SuperPeerServiceManager(aProperties, mJxtaNetworkManager);
      mP2pProxyManagement = (P2pProxyManagement) mServiceProvider;
      mServiceProvider.start(3000L);
   }
   private static void startSeeding(Configurator aProperties,File aConfigDir) throws Exception{
      // setup jxta
      mJxtaNetworkManager = new JxtaNetworkManager(aProperties,aConfigDir);
      mServiceProvider = new SeedingPeerServiceManager(aProperties, mJxtaNetworkManager,true);
      mP2pProxyManagement = null;
      mServiceProvider.start(3000L);
   }   
   private static void usage() {
      System.out.println("p2pproxy");
      System.out.println("-jxta : directory where configuration/cache is located  (including jxta cache.default is $HOME/.p2pproxy");
      System.out.println("-sip : udp proxy port, default 5060");
      System.out.println("-relay : super peer mode");
      System.out.println("-edge-only : edge mode");
      System.out.println("-seeding-server : seeding server mode");
      System.out.println("-auto-config : automatically choose edge or relay (default mode)");
      System.out.println("-seeding-rdv : list of boostrap rdv separated by | (ex tcp://127.0.0.1:9701|http://127.0.0.2:9700)");
      System.out.println("-seeding-relay : list of boostrap relay separated by |(ex tcp://127.0.0.1:9701|http://127.0.0.2:9700)");
      System.out.println("-public-address : ip as exported to peers (ex myPublicAddress.no-ip.org)");
      System.out.println("-socks-url : tcp://ip:port for socks server (ex tcp://socks.com:1080)");
      System.out.println("-enable-http-client : enable http transport for client (default = false)");
   }
   public String getHttpProxy() {
      return System.getProperty("http.proxyHost", "not-set")+":"+System.getProperty("http.proxyPort", "not-set");
   }

   public void setHttpProxy(String aProxyHost, String aProxyPort, String aUserName, String aPassword) {
      System.setProperty("http.proxyHost",aProxyHost);
      System.setProperty("http.proxyPort",aProxyPort);

   }
public String getSocksServer() {
    return System.getProperty("socksProxyHost", "not-set")+":"+System.getProperty("socksProxyPort", "not-set");
}
public void setSocksServer(String aSocksHost, String aSocksPort, String aUserName, String aPassword) {
    System.setProperty("socksProxyHost",aSocksHost);
    System.setProperty("socksProxyPort",aSocksPort);
}
  
public void loadTraceConfigFile() throws P2pProxyException {
   staticLoadTraceConfigFile();
   }
public  static void staticLoadTraceConfigFile()  throws P2pProxyException {
   try {
      PropertyConfigurator.configureAndWatch("log4j.properties");
      // read java.util.logging properties 
      Properties lLogginProperties = new Properties();
      lLogginProperties.load(new FileInputStream("log4j.properties"));
      lLogginProperties.setProperty("java.util.logging.FileHandler.pattern",System.getProperty("org.linphone.p2pproxy.home")+"/logs/p2pproxy.log");
      lLogginProperties.store(new FileOutputStream("log4j.properties.tmp"), "tmp");
      System.setProperty("java.util.logging.config.file","log4j.properties.tmp");
      java.util.logging.LogManager.getLogManager().readConfiguration();
   } catch (Exception e) {
      throw new P2pProxyException("enable to load traces",e);
   }
}
}