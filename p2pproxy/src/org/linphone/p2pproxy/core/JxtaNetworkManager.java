/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

JxtaNetworkManager.java -- connection to a jxta network.

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
import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;

import java.util.Enumeration;
import java.util.List;
import java.util.Properties;
import java.util.StringTokenizer;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import javax.security.cert.CertificateException;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.sipproxy.NetworkResourceAdvertisement;
import org.linphone.p2pproxy.core.sipproxy.superpeers.P2pUserRegistrationAdvertisement;



import net.jxta.discovery.DiscoveryEvent;
import net.jxta.discovery.DiscoveryListener;
import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.exception.JxtaException;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.peergroup.NetPeerGroupFactory;
import net.jxta.peergroup.PeerGroup;
import net.jxta.peergroup.PeerGroupID;
import net.jxta.pipe.InputPipe;
import net.jxta.pipe.PipeID;
import net.jxta.pipe.PipeMsgListener;
import net.jxta.pipe.PipeService;
import net.jxta.platform.NetworkConfigurator;
import net.jxta.protocol.ConfigParams;
import net.jxta.protocol.DiscoveryResponseMsg;
import net.jxta.protocol.ModuleSpecAdvertisement;
import net.jxta.protocol.PipeAdvertisement;
import net.jxta.rendezvous.RendezVousService;
import net.jxta.socket.JxtaSocket;


public class JxtaNetworkManager {
   public enum Mode  {relay, edge, auto,seeding_server};
   private  PeerGroup mNetworkPeerGroup;
   private  RendezVousService mRendezVousService;
   private final static Logger mLog = Logger.getLogger(JxtaNetworkManager.class);
   public final static String RDV_CONNECT_TIMEOUT="org.linphone.p2pproxy.JxtaNetworkManager.rdv-connect.timout";
   public final static String ADV_DISCOVERY_TIMEOUT="org.linphone.p2pproxy.JxtaNetworkManager.adv-discovery.timout";   
   public final static String MODE="org.linphone.p2pproxy.JxtaNetworkManager.mode";
   public final static String RELAY_CAPACITY="org.linphone.p2pproxy.JxtaNetworkManager.relay-capacity";
   public final static String SEEDING_RDV="org.linphone.p2pproxy.JxtaNetworkManager.seeding-rdv.url";
   public final static String SEEDING_RELAY="org.linphone.p2pproxy.JxtaNetworkManager.seeding-relay.url";
   public final static String TCP_LISTENING_PORT="org.linphone.p2pproxy.JxtaNetworkManager.tcp.port";
   public final static String HTTP_LISTENING_PORT="org.linphone.p2pproxy.JxtaNetworkManager.http.port";
   public final static String HTTP_LISTENING_PUBLIC_ADDRESS="org.linphone.p2pproxy.JxtaNetworkManager.http.listening.public.address";
   public final static String TCP_LISTENING_PUBLIC_ADDRESS="org.linphone.p2pproxy.JxtaNetworkManager.tcp.listening.public.address";
   public final static String SO_TIMEOUT="org.linphone.p2pproxy.so-timout";
   public final static String ENABLE_HTTP_CLIENT="org.linphone.p2pproxy.JxtaNetworkManager.http.client.enable";
   public static int EDGE_MODE =  NetworkConfigurator.TCP_CLIENT| NetworkConfigurator.RDV_CLIENT | NetworkConfigurator.RELAY_CLIENT;
   public static int SUPER_PEER_MODE = NetworkConfigurator.RDV_RELAY_PROXY_NODE;
   final private Properties mProperties;
   private Mode mMode;
   public static int ADV_DISCOVERY_TIMEOUT_INT = 15000; 
   static {
      System.setProperty("net.jxta.impl.cm.index.rebuild", "true");
   }
   /**
    * @return Returns the mMode.
    */
   public Mode getMode() {
      return mMode;
   }
   /**
    * Create a jxta connection from a given config directory
    * and connect to a rendez vous perr 
    * @param aConfigDir a jxta config dir
    * @param aRdvConnectionTimout time in ms to wait until a succesfull connection to a rdv peer
    * @throws JxtaException
    * @throws InterruptedException 
    * @throws P2pProxyException 
    * @throws IOException 
    * @throws URISyntaxException 
    * @throws CertificateException 
    */
   public JxtaNetworkManager(Configurator aProperties,File aConfigDir) throws JxtaException, InterruptedException, P2pProxyException, IOException, URISyntaxException, CertificateException {
      super();
      // get configuration
      //System.setProperty("JXTA_HOME", aConfigDir.getAbsolutePath());
      
      NetworkConfigurator lNetworkConfigurator;
      mProperties = aProperties;
      // set mode
      mMode = Mode.valueOf(aProperties.getProperty(MODE, Mode.edge.name()));
      int lMode;
      if (mMode == Mode.relay || mMode == Mode.seeding_server) {
    	  lMode = SUPER_PEER_MODE;
      } else {
    	  lMode = EDGE_MODE;
      }
      if (aProperties.getProperty(ENABLE_HTTP_CLIENT) != null && Boolean.parseBoolean(aProperties.getProperty(ENABLE_HTTP_CLIENT)) == true) {
    	  lMode = lMode | NetworkConfigurator.HTTP_CLIENT;
       } 
      
      lNetworkConfigurator = new NetworkConfigurator(lMode,aConfigDir.toURI());
     
      if (!lNetworkConfigurator.exists()) {
         lNetworkConfigurator.setPeerID(IDFactory.newPeerID(PeerGroupID.defaultNetPeerGroupID));
         lNetworkConfigurator.setDescription("p2p proxy instance");
         lNetworkConfigurator.save();
      } else {
         lNetworkConfigurator.load();
      }
      //mode is alway taken from start line
      lNetworkConfigurator.setMode(lMode);
      // set sedding host
      if (aProperties.getProperty(SEEDING_RDV) != null) {
         StringTokenizer lSeedingRdvList =  new StringTokenizer(aProperties.getProperty(SEEDING_RDV),"|" );
         while (lSeedingRdvList.hasMoreTokens()) {
            lNetworkConfigurator.addSeedRendezvous(new URI(lSeedingRdvList.nextToken()));
         }
     }
      if (aProperties.getProperty(SEEDING_RELAY) != null) {
         StringTokenizer lSeedingRelayList =  new StringTokenizer(aProperties.getProperty(SEEDING_RELAY),"|" );
         while (lSeedingRelayList.hasMoreTokens()) {
            lNetworkConfigurator.addSeedRelay(new URI(lSeedingRelayList.nextToken()));
         }
      }
      
      if (aProperties.getProperty(HTTP_LISTENING_PUBLIC_ADDRESS) != null) {
         lNetworkConfigurator.setHttpPublicAddress(aProperties.getProperty(HTTP_LISTENING_PUBLIC_ADDRESS), true);
      }
      
      // set listening ports
      if (aProperties.getProperty(HTTP_LISTENING_PORT) != null) {
         lNetworkConfigurator.setHttpPort(Integer.parseInt(aProperties.getProperty(HTTP_LISTENING_PORT)));
      }
      
      if (aProperties.getProperty(TCP_LISTENING_PUBLIC_ADDRESS) != null) {
          lNetworkConfigurator.setTcpPublicAddress(aProperties.getProperty(TCP_LISTENING_PUBLIC_ADDRESS), true);
          lNetworkConfigurator.setTcpStartPort(-1);
          lNetworkConfigurator.setTcpEndPort(-1);
       }

      if (aProperties.getProperty(TCP_LISTENING_PORT) != null) {
         lNetworkConfigurator.setTcpPort(Integer.parseInt(aProperties.getProperty(TCP_LISTENING_PORT)));
      }
      
      // connect to rdv
      int lRdvConnectionTimout = Integer.parseInt(aProperties.getProperty(RDV_CONNECT_TIMEOUT,"60000"));
      init(lNetworkConfigurator,lRdvConnectionTimout,mMode);
      
      
   }
   /**
    * @param aProperties use to store pipe ID
    * @param aNetworkConfigurator jxya native config
    * @param aConnectTimout rdv connection timeout
    * @throws JxtaException
    * @throws InterruptedException
    * @throws P2pProxyException
    * @throws IOException
    * @throws URISyntaxException
    * @throws CertificateException
    */
   public JxtaNetworkManager(Configurator aProperties, NetworkConfigurator aNetworkConfigurator,int aConnectTimout,Mode aMode) throws JxtaException, InterruptedException, P2pProxyException, IOException, URISyntaxException, CertificateException {
      mProperties = aProperties;
      mMode  = aMode;
      init(aNetworkConfigurator,aConnectTimout,aMode);
   }
   private void init(NetworkConfigurator aNetworkConfigurator,int aConnectTimout, Mode aMode) throws JxtaException, InterruptedException, P2pProxyException, IOException, URISyntaxException, CertificateException {
      // connect to rdv
      if (mProperties.getProperty(ADV_DISCOVERY_TIMEOUT) != null) {
         ADV_DISCOVERY_TIMEOUT_INT = Integer.parseInt(mProperties.getProperty(ADV_DISCOVERY_TIMEOUT));
      }
      NetPeerGroupFactory lFactory  = new NetPeerGroupFactory((ConfigParams) aNetworkConfigurator.getPlatformConfig(),aNetworkConfigurator.getHome().toURI());
      mNetworkPeerGroup = lFactory.getInterface();
      
      // The following step is required and only need to be done once,
      // without this step the AdvertisementFactory has no means of
      // associating an advertisement name space with the proper object
      // in this cast the AdvertisementTutorial
      AdvertisementFactory.registerAdvertisementInstance(P2pUserProfileAdvertisement.getAdvertisementType(),new P2pUserProfileAdvertisement.Instantiator());
      AdvertisementFactory.registerAdvertisementInstance(P2pUserRegistrationAdvertisement.getAdvertisementType(),new P2pUserRegistrationAdvertisement.Instantiator());
      AdvertisementFactory.registerAdvertisementInstance(NetworkResourceAdvertisement.getAdvertisementType(),new NetworkResourceAdvertisement.Instantiator());
      
      mRendezVousService = mNetworkPeerGroup.getRendezVousService();
      mLog.info("Node PeerID ["+mNetworkPeerGroup.getPeerID()+"]");
      if ( aMode == Mode.edge && isConnectedToRendezVous(aConnectTimout) == false) {
         throw new P2pProxyException("Cannot connect to rdv in the last "+aConnectTimout+" ms");
      }
      mLog.info("jxta info name ["+mNetworkPeerGroup+"] mode ["+aMode+"] ");

   }
   public PeerGroup getPeerGroup() {
      return mNetworkPeerGroup;
   }
   
   public PipeAdvertisement createPipeAdvertisement(String aPipePropertyName,String aPipeName) throws IOException {
      PipeID lpipeID = null;
      if (mProperties.getProperty(aPipePropertyName) == null) {
         lpipeID = IDFactory.newPipeID(PeerGroupID.defaultNetPeerGroupID);
         mProperties.setProperty(aPipePropertyName, lpipeID.toURI().toString());

      } else {
         lpipeID = (PipeID) ID.create(URI.create(mProperties.getProperty(aPipePropertyName)));
      }
      //create advertisement
      PipeAdvertisement lAdvertisement = (PipeAdvertisement)AdvertisementFactory.newAdvertisement(PipeAdvertisement.getAdvertisementType());
      lAdvertisement.setPipeID(lpipeID);
      lAdvertisement.setType(PipeService.UnicastType);
      lAdvertisement.setName(aPipeName);
      mLog.debug("aPipePropertyName pipe:"+lAdvertisement);
      return lAdvertisement;
   }
   
   public InputPipe createPipe(PipeAdvertisement aPipeAdvertisement,PipeMsgListener aListener) throws IOException {
      //create pipe
      return mNetworkPeerGroup.getPipeService().createInputPipe(aPipeAdvertisement, aListener);   
   }
   /**
    * @param aPipePropertyName name use to save/load this pipe ID in the configuration file
    * @param aPipeName pipe name
    * @return
    * @throws IOException
    */
   public InputPipe createPipe(String aPipePropertyName,String aPipeName,PipeMsgListener aListener) throws IOException {
      return createPipe(createPipeAdvertisement(aPipePropertyName,aPipeName),aListener);
   }   
   public Advertisement getAdvertisement(String aPeerId, String anAdvertisementName,boolean isTryFromLocal) throws InterruptedException, IOException, P2pProxyAdvertisementNotFoundException {
      return getAdvertisementList(aPeerId, anAdvertisementName,isTryFromLocal).get(0);
   }      
   
   /**
    * seach advervitisement indexed by attribute "Name"
    * @param aPeerId
    * @param anAdvertisementName
    * @param isTryFromLocal
    * @return
    * @throws InterruptedException
    * @throws IOException
    * @throws P2pProxyAdvertisementNotFoundException
    */
   public  List<? extends Advertisement> getAdvertisementList(String aPeerId, String anAdvertisementName,boolean isTryFromLocal) throws InterruptedException, IOException, P2pProxyAdvertisementNotFoundException {
      return getAdvertisementList(aPeerId, "Name",anAdvertisementName, isTryFromLocal);
   }
   
   public  List<? extends Advertisement> getAdvertisementList(String aPeerId, String anAttributeName,String anAttributeValue, boolean isTryFromLocal) throws InterruptedException, IOException, P2pProxyAdvertisementNotFoundException {
	   return getAdvertisementList(aPeerId, anAttributeName, anAttributeValue, isTryFromLocal, 1);
   }
   public  List<? extends Advertisement> getAdvertisementList(String aPeerId, String anAttributeName,String anAttributeValue, boolean isTryFromLocal, int numberOfexpectedAdv) throws InterruptedException, IOException, P2pProxyAdvertisementNotFoundException {
      DiscoveryService lDiscoveryService = getPeerGroup().getDiscoveryService();
      final Semaphore lSemaphore = new Semaphore(1-numberOfexpectedAdv);
      final List<Advertisement> lReturnList = new ArrayList<Advertisement>();
      DiscoveryListener lDiscoveryListener = new DiscoveryListener() {
         
         public void discoveryEvent(DiscoveryEvent event) {
            DiscoveryResponseMsg lRes = event.getResponse();
            int lOrigListSize = lReturnList.size();
            enumeration2List(lRes.getAdvertisements(), lReturnList);
            lSemaphore.release(lReturnList.size()-lOrigListSize);
         }
         
      };
      if (isTryFromLocal == true) {
         mLog.info("looking for advertisement indexing with  ["+ anAttributeName+"="+anAttributeValue+"]");
         Enumeration lEnumeration = lDiscoveryService.getLocalAdvertisements(DiscoveryService.ADV, anAttributeName,anAttributeValue);
         enumeration2List(lEnumeration, lReturnList);
      }
      if (lReturnList.size() < numberOfexpectedAdv) {
         mLog.info(lReturnList.size() +" of ["+numberOfexpectedAdv+"] advertisements found in local, trying remote...");
         lDiscoveryService.getRemoteAdvertisements(aPeerId, DiscoveryService.ADV, anAttributeName,anAttributeValue, 10,lDiscoveryListener);
         if (lSemaphore.tryAcquire(ADV_DISCOVERY_TIMEOUT_INT,TimeUnit.MILLISECONDS) == false && lReturnList.isEmpty()) {
            throw new P2pProxyAdvertisementNotFoundException( anAttributeName+"="+anAttributeValue+ " not found");
         }
         lSemaphore.release();
      }
     if (mLog.isInfoEnabled() && mLog.isDebugEnabled() == false) mLog.info(lReturnList.get(0).toString());
     for (Advertisement lAdvertisement :lReturnList) {
        mLog.debug(lAdvertisement.toString());
     }
      return lReturnList;  
   }
   public JxtaSocket openSocket(String aPeerId, String anAdvertisementName,int aSocketTimout,boolean isTryFromLocal) throws InterruptedException, P2pProxyException, IOException {
      List<ModuleSpecAdvertisement> lModuleSpecAdvertisementList;
      lModuleSpecAdvertisementList = (List<ModuleSpecAdvertisement>) getAdvertisementList(aPeerId, anAdvertisementName, isTryFromLocal);
      // reset just in case
      JxtaSocket lJxtaSocket = null;
      for (int i=0; i < lModuleSpecAdvertisementList.size(); i++) {
         try {
            lJxtaSocket = new JxtaSocket(getPeerGroup(), null, lModuleSpecAdvertisementList.get(i).getPipeAdvertisement(), aSocketTimout, true);
            // ok, socket connected :-)
            mLog.info("socket ["+lJxtaSocket+"] connected");
            break;
         }catch (IOException e) {
            mLog.warn("cannot open socket, for index ["+i+"]  try next from ["+lModuleSpecAdvertisementList.size()+"]", e);
            mLog.debug("bad adv "+lModuleSpecAdvertisementList.get(i).getPipeAdvertisement().toString());
         }
      }
      if (lJxtaSocket == null) {
         throw new P2pProxyException("Cannot start peer info service because cannot bind jxta socket"); 
      } else {
         return lJxtaSocket;
      }

}
   /**
    * check if connected to an rdv for aTimeout
    * @param aTimeout
    * @return true if connected else false
    * @throws InterruptedException
    */
   public boolean isConnectedToRendezVous(long aTimeout) throws InterruptedException{
      long lStartTime = System.currentTimeMillis();
      boolean lExit = false;
      while(lExit == false) {
         if (mRendezVousService.isConnectedToRendezVous() ) {
            ID lRdvPeerId = (ID)getPeerGroup().getRendezVousService().getConnectedRendezVous().nextElement();
            mLog.info("Connected to rdv   ["+lRdvPeerId+"]");
            lExit=true;
         } else {
            if (System.currentTimeMillis() - lStartTime > aTimeout) {
               return  false;
            }
            mLog.info("waiting to rdv connection");
            Thread.sleep(500);
         }

      }
      return true;      
   }
   public void stop() {
      mNetworkPeerGroup.stopApp();
      //mNetworkPeerGroup.unref();
   }
   private List<Advertisement> enumeration2List(Enumeration<Advertisement> lEnumeration,List<Advertisement> aList) {
      if (aList == null) {
         aList = new ArrayList<Advertisement>();
      }
      while (lEnumeration.hasMoreElements()) {
    	  Advertisement lNewAdv =  lEnumeration.nextElement();
    	  //1 check if already exist
    	  for (Advertisement lAdv: aList) {
    		  if (lAdv.equals(lNewAdv)) {
    			  if (mLog.isDebugEnabled()) mLog.debug("adv ["+lNewAdv.getID()+"]already gathered");
    			  lNewAdv = null;
    			  break;
    		  }
    	  }
    	  
    	  if (lNewAdv != null) {
    		  aList.add((lNewAdv)) ;
    	  }
       }
      return aList;
   }
   
}
