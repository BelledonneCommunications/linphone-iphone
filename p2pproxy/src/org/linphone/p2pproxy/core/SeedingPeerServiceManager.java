// Decompiled by Jad v1.5.8g. Copyright 2001 Pavel Kouznetsov.
// Jad home page: http://www.kpdus.com/jad.html
// Decompiler options: packimports(3) 
// Source File Name:   SuperPeerServiceManager.java

package org.linphone.p2pproxy.core;


import java.net.InetSocketAddress;
import java.net.SocketException;
import java.net.URI;
import java.net.UnknownHostException;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.media.rtprelay.RtpRelayService;
import org.linphone.p2pproxy.core.media.rtprelay.RtpRelayServerConfig;
import org.linphone.p2pproxy.core.rdvautoconfig.PeerInfoProviderService;

public class SeedingPeerServiceManager extends P2pProxyResourceManagementImpl implements ServiceProvider {
   protected final Configurator mConfigurator;
   private final PeerInfoProviderService mPeerInfoProviderService; 
   private RtpRelayService mUdpRelayService = null; 

   private final static Logger mLog = Logger.getLogger(SeedingPeerServiceManager.class);
   SeedingPeerServiceManager(Configurator aConfigurator, JxtaNetworkManager aJxtaNetworkManager,boolean enableUdpRelay) throws SocketException, UnknownHostException {
      super(aJxtaNetworkManager);
      mConfigurator = aConfigurator;
      mPeerInfoProviderService = new PeerInfoProviderService(aConfigurator, aJxtaNetworkManager);
      if (enableUdpRelay == true) {
       URI lAudioVideoPublicUri = URI.create(aConfigurator.getProperty(RtpRelayService.AUDIO_VIDEO_PUBLIC_URI,RtpRelayService.getDefaultAudioVideoPublicUri()));
       int lAudioVideoLocalPort = Integer.valueOf(aConfigurator.getProperty(RtpRelayService.AUDIO_VIDEO_LOCAL_PORT,String.valueOf(lAudioVideoPublicUri.getPort())));
       RtpRelayServerConfig lConfig = new RtpRelayServerConfig(new InetSocketAddress(lAudioVideoPublicUri.getHost(),lAudioVideoPublicUri.getPort())
                                     ,new InetSocketAddress(lAudioVideoLocalPort));
       mUdpRelayService = new RtpRelayService(lConfig,aConfigurator,aJxtaNetworkManager);
      }
   }

   public void start(long aTimeout) throws P2pProxyException     {
      mPeerInfoProviderService.start(aTimeout);
      if (mUdpRelayService != null) {
         mUdpRelayService.start(aTimeout);
      }
      mLog.info("SeedingPeerServiceManager started");
   }

   public void stop() {
      mPeerInfoProviderService.stop();
      if (mUdpRelayService != null) {
         mUdpRelayService.stop();
      }
      mLog.info("SeedingPeerServiceManager stopped");
   }




}
