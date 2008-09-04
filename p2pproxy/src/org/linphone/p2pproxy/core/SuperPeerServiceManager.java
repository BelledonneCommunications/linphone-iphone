// Decompiled by Jad v1.5.8g. Copyright 2001 Pavel Kouznetsov.
// Jad home page: http://www.kpdus.com/jad.html
// Decompiler options: packimports(3) 
// Source File Name:   SuperPeerServiceManager.java

package org.linphone.p2pproxy.core;



import java.net.SocketAddress;
import java.net.SocketException;
import java.net.UnknownHostException;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;

import org.linphone.p2pproxy.core.rdvautoconfig.PeerInfoProviderService;

// Referenced classes of package org.linphone.p2pproxy.core:
//            EdgePeerServiceManager, Configurator, JxtaNetworkManager

public class SuperPeerServiceManager extends P2pProxyManagementImpl {
   private final static Logger mLog = Logger.getLogger(SuperPeerServiceManager.class);
   private final PeerInfoProviderService mPeerInfoProviderService;
   SuperPeerServiceManager(Configurator aConfigurator, JxtaNetworkManager aJxtaNetworkManager) throws P2pProxyException, SocketException, UnknownHostException
   {
     super(aConfigurator,aJxtaNetworkManager); 
     mPeerInfoProviderService = new PeerInfoProviderService(aConfigurator, aJxtaNetworkManager);
   }

   public void start(long aTimeOut) throws P2pProxyException  {
      mPeerInfoProviderService.start(aTimeOut);
      mLog.info("SuperPeerServiceManager started");
   }

   public void stop() {
      super.stop();
      mPeerInfoProviderService.stop();
      mLog.info("SuperPeerServiceManager stopped");
   }


   public boolean shouldIBehaveAsAnRdv() throws P2pProxyException{
      return false;
   }

}
