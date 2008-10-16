// Decompiled by Jad v1.5.8g. Copyright 2001 Pavel Kouznetsov.
// Jad home page: http://www.kpdus.com/jad.html
// Decompiler options: packimports(3) 
// Source File Name:   SuperPeerServiceManager.java

package org.linphone.p2pproxy.core;



import java.net.SocketException;
import java.net.UnknownHostException;
import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.media.MediaResourceService;
import org.linphone.p2pproxy.core.rdvautoconfig.PeerInfoProviderService;

public class SeedingPeerServiceManager extends P2pProxyResourceManagementImpl implements ServiceProvider {
   protected final Configurator mConfigurator;
   private final PeerInfoProviderService mPeerInfoProviderService; 
   private MediaResourceService mStunUdpRelayService = null; 

   private final static Logger mLog = Logger.getLogger(SeedingPeerServiceManager.class);
   SeedingPeerServiceManager(Configurator aConfigurator, JxtaNetworkManager aJxtaNetworkManager,boolean enableUdpRelay) throws SocketException, UnknownHostException {
      super(aJxtaNetworkManager);
      mConfigurator = aConfigurator;
      mPeerInfoProviderService = new PeerInfoProviderService(aConfigurator, aJxtaNetworkManager);
      if (enableUdpRelay == true) {
      mStunUdpRelayService = new MediaResourceService(aConfigurator,aJxtaNetworkManager);
      }
   }

   public void start(long aTimeout) throws P2pProxyException     {
      mPeerInfoProviderService.start(aTimeout);
      if (mStunUdpRelayService != null) {
         mStunUdpRelayService.start(aTimeout);
      }
      mLog.info("SeedingPeerServiceManager started");
   }

   public void stop() {
      mPeerInfoProviderService.stop();
      if (mStunUdpRelayService != null) {
         mStunUdpRelayService.stop();
      }
      mLog.info("SeedingPeerServiceManager stopped");
   }




}
