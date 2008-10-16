/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

P2pProxyInstanceImpl.java - .

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
import java.net.InetAddress;
import java.net.URI;
import java.net.UnknownHostException;
import java.util.Enumeration;
import java.util.Properties;

import net.jxta.id.IDFactory;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.platform.NetworkConfigurator;
import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxyInstance;
import org.linphone.p2pproxy.api.P2pProxyManagement;
import org.linphone.p2pproxy.api.P2pProxyNetworkProbe;
import org.linphone.p2pproxy.api.P2pProxyRtpRelayManagement;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrar;
import org.zoolu.net.SocketAddress;
import org.zoolu.sip.provider.SipProvider;


public class P2pProxyInstanceImpl implements P2pProxyInstance {
   private final static Logger mLog = Logger.getLogger(P2pProxyInstance.class);
   private static int BASE_PROXY_SIP_PORT = 6000;
   private static int BASE_CLIENT_SIP_PORT = 8000;
   int mIndex=0;
   private Mode mMode = Mode.edge ;
   boolean isStarted = false;
   boolean isRevoked = false;
   JxtaNetworkManager mJxtaNetworkManager;
   SipProxyRegistrar mSipProxy;
   SipProvider mProviderForSipClient;
   String mSipClientName;
   String mPrivateHostAddress;
   String mPublicHostAddress ;   
   private ServiceProvider mServiceProvider;
   private P2pProxyManagement mP2pProxyManagement;
   private P2pProxyAccountManagementMBean mP2pProxyAccountManagement;
   private int mRelayCapacity=4;
   private Configurator mConfigurator;
   private Properties startupProperties = new Properties();
   public P2pProxyInstanceImpl () {
             
   }
    /* (non-Javadoc)
    * @see org.linphone.p2pproxy.P2pProxyInstance#getIndex()
    */
   public int getIndex() {
      return mIndex;
   }
   /* (non-Javadoc)
    * @see org.linphone.p2pproxy.P2pProxyInstance#setIndex(int)
    */
   public void setIndex(int index) {
      mIndex = index;
   }
   /* (non-Javadoc)
    * @see org.linphone.p2pproxy.P2pProxyInstance#start()
    */
   public void start() throws Exception{
      Thread lThread = new Thread() {
         public void run() {
            try {
               File lJxtaDirectory = new File ("P2pNetwork-"+getMode()+"-"+mIndex);
               if (lJxtaDirectory.exists() == false) lJxtaDirectory.mkdir();
               mConfigurator = new Configurator(new File (lJxtaDirectory.getAbsolutePath()+"/prop.xml"));  
               mConfigurator.serProperties(P2pProxyInstanceImpl.this.startupProperties);
               mConfigurator.setProperty(JxtaNetworkManager.RELAY_CAPACITY, String.valueOf(mRelayCapacity));
               // setup jxta network
               NetworkConfigurator lNetworkConfigurator;
               // set mode
               lNetworkConfigurator = new NetworkConfigurator(JxtaNetworkManager.EDGE_MODE,lJxtaDirectory.toURI());
               lNetworkConfigurator.setHome(lJxtaDirectory);
               if (!lNetworkConfigurator.exists()) {
                  lNetworkConfigurator.setPeerID(IDFactory.newPeerID(PeerGroupID.defaultNetPeerGroupID));
                  lNetworkConfigurator.setDescription("p2p proxy instance");
                  lNetworkConfigurator.save();
               } else {
                  lNetworkConfigurator.load();
               }
              // set sedding host
                  lNetworkConfigurator.addSeedRendezvous(new URI("tcp://"+getPublicHostAddress()+":"+BASE_TCP));
                  lNetworkConfigurator.addSeedRelay(new URI("tcp://"+getPublicHostAddress()+":"+BASE_TCP));
               // set listening ports
//                  lNetworkConfigurator.setTcpInterfaceAddress(getPrivateHostAddress());
//                  lNetworkConfigurator.setHttpInterfaceAddress(getPrivateHostAddress());
//                  lNetworkConfigurator.setHttpPort(BASE_HTTP + mIndex);
                  lNetworkConfigurator.setTcpPort(BASE_TCP + mIndex);
               
                  switch (mMode) {
                  case edge:
                     startEdge(mConfigurator,lNetworkConfigurator);
                     break;
                  case relay:
                     startRelay(mConfigurator,lNetworkConfigurator,false);
                     break;
                  case seeding_server:
                     startRelay(mConfigurator,lNetworkConfigurator,true);
                     break;
                  case auto:
                     //1 start edge 
                     startEdge(mConfigurator,lNetworkConfigurator);
                     // check if peer mode required
                     if (mP2pProxyManagement.shouldIBehaveAsAnRdv() == true) {
                        String lPublicHttpAddress = mP2pProxyManagement.getPublicIpAddress().getHostAddress();
                        
                        lNetworkConfigurator.setHttpPublicAddress(lPublicHttpAddress+":"+(BASE_HTTP + mIndex), true);
                        mServiceProvider.stop();
                        mJxtaNetworkManager.stop();
                        
                        startRelay(mConfigurator,lNetworkConfigurator,false);
                        // become relay
                        mMode = Mode.relay;
                     } else {
                        mMode = Mode.edge;
                     }
                     break;
                  default:
                     throw new Exception("unsupported mode ["+mMode+"]");
                  }
               mConfigurator.setProperty(SipProxyRegistrar.REGISTRAR_PORT,Integer.toString(BASE_PROXY_SIP_PORT+mIndex));
               // setup sip proxy
               mP2pProxyAccountManagement = new P2pProxyAccountManagement(mJxtaNetworkManager);
               mSipProxy = new SipProxyRegistrar(mConfigurator,mJxtaNetworkManager,mP2pProxyAccountManagement);
               // setup sip client                
               mProviderForSipClient = new SipProvider(getPrivateHostAddress(),BASE_CLIENT_SIP_PORT+mIndex);
               mProviderForSipClient.setOutboundProxy(new SocketAddress(getPrivateHostAddress(),BASE_PROXY_SIP_PORT+mIndex));
               mSipClientName="sip:user-"+mIndex+"@p2pproxy.linphone.org";

               
               mLog.info(P2pProxyInstanceImpl.this+" started ");
               isStarted = true;
            } catch (Exception e) {
               mLog.error(P2pProxyInstanceImpl.this+" cannot be started",e);
               isRevoked = true;
            }
         }
      };
      lThread.start();
   }
   public boolean isStarted() throws P2pProxyException{
      if (isRevoked == true)  {
         throw new P2pProxyException("cannot start " + this.toString() );
      } else {
         return isStarted;
      }
   }
   public SipProvider getSipClientProvider() {
      return mProviderForSipClient;
   }
   
   public String toString() {
      return "p2p instance ["+mIndex+"] mode ["+mMode+"] sip port ["+BASE_PROXY_SIP_PORT+mIndex+"]";
   }
   public String getSipClientName() {
      return mSipClientName;
   }
   public void stop() throws Exception {
      mProviderForSipClient.halt();
      //mSipProxy.halt();
      mJxtaNetworkManager.stop();
   }
   public int getNumberOfconnectedPeers() {
     Enumeration lConnectedPeers = mJxtaNetworkManager.getPeerGroup().getRendezVousService().getConnectedPeers();
     int lResult = 0;
     while (lConnectedPeers.hasMoreElements()) {
        lResult++;
        lConnectedPeers.nextElement();
        }
     return lResult;
   }
   public Object getOpaqueNetworkManager() {
      return mJxtaNetworkManager;
   }
 
   /**
    * @return Returns the mHostAddress.
    * @throws UnknownHostException 
    */
   public String getPrivateHostAddress() throws UnknownHostException {
      if (mPrivateHostAddress == null) {
         mPrivateHostAddress = InetAddress.getLocalHost().getHostAddress();
      }
      return mPrivateHostAddress;
   }
   public String getPublicHostAddress() throws UnknownHostException {
      if (mPublicHostAddress == null) {
         mPublicHostAddress = InetAddress.getLocalHost().getHostAddress();
      }
      return mPublicHostAddress;
   }

   public void setPrivateHostAdress(String anAddress) {
      mPrivateHostAddress = anAddress; 
   }
   public void setPublicHostAdress(String anAddress) {
      mPublicHostAddress = anAddress;
   }
   public P2pProxyNetworkProbe getManager() {
      return (P2pProxyNetworkProbe)mServiceProvider;
   }
   public Mode getMode() {
      return mMode;
   }
   public void setMode(Mode aMode) {
      mMode = aMode;
   }
   private  void startEdge(Configurator aProperties,NetworkConfigurator aNetworkConfigurator) throws Exception{
      // setup jxta
      aNetworkConfigurator.setMode(JxtaNetworkManager.EDGE_MODE);
      aNetworkConfigurator.setHttpEnabled(true);
      aNetworkConfigurator.setHttpOutgoing(true);

      mJxtaNetworkManager = new JxtaNetworkManager(aProperties,aNetworkConfigurator,60000,JxtaNetworkManager.Mode.edge);
      mServiceProvider = new EdgePeerServiceManager(aProperties, mJxtaNetworkManager);
      mP2pProxyManagement = (P2pProxyManagement) mServiceProvider;
      mServiceProvider.start(3000L);
   }

   private  void startRelay(Configurator aProperties,NetworkConfigurator aNetworkConfigurator,boolean isSeeding) throws Exception{
      // setup jxta
      aNetworkConfigurator.setMode(JxtaNetworkManager.SUPER_PEER_MODE);
      if (isSeeding == true) {
         mJxtaNetworkManager = new JxtaNetworkManager(aProperties,aNetworkConfigurator,60000,JxtaNetworkManager.Mode.seeding_server);
         mServiceProvider = new SeedingPeerServiceManager(aProperties, mJxtaNetworkManager,true);
         mP2pProxyManagement = null;
      } else {
         mJxtaNetworkManager = new JxtaNetworkManager(aProperties,aNetworkConfigurator,60000,JxtaNetworkManager.Mode.relay);
         mServiceProvider = new SuperPeerServiceManager(aProperties, mJxtaNetworkManager);
         mP2pProxyManagement = (P2pProxyManagement) mServiceProvider;
      }
      mServiceProvider.start(3000L);
   }
   public void setRelayCapacity(int aCapacity) {
      mRelayCapacity = aCapacity;
      
   }
   public void setProperty(String key, String value) throws P2pProxyException {
      if (mConfigurator == null) {
         startupProperties.setProperty(key, value);
      } else {
         throw new P2pProxyException(" started");
      }
   }
   public int getAdvertisementDiscoveryTimeout() {
      return JxtaNetworkManager.ADV_DISCOVERY_TIMEOUT_INT;
   }
   public P2pProxyRtpRelayManagement getRtpRelayManager() {
      return (P2pProxyRtpRelayManagement)mServiceProvider;
   }
}