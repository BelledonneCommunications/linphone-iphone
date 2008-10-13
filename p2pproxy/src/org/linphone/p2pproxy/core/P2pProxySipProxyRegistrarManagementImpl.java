package org.linphone.p2pproxy.core;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxySipProxyRegistrarManagement;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrarAdvertisement;

public class P2pProxySipProxyRegistrarManagementImpl implements P2pProxySipProxyRegistrarManagement {
   protected final JxtaNetworkManager mJxtaNetworkManager;
   private final String DOMAINE="p2p.linphone.org";
   private final static Logger mLog = Logger.getLogger(P2pProxySipProxyRegistrarManagementImpl.class);
   P2pProxySipProxyRegistrarManagementImpl(JxtaNetworkManager aJxtaNetworkManager) {
      mJxtaNetworkManager = aJxtaNetworkManager;
   }
   public String lookupSipProxyUri(String aDomaine) throws P2pProxyException {
      try {
         if (!DOMAINE.equals(aDomaine)) {
            //unknown domaine
            return null;
         }
         SipProxyRegistrarAdvertisement lSipProxyRegistrarAdvertisement = (SipProxyRegistrarAdvertisement) (mJxtaNetworkManager.getAdvertisement(null, SipProxyRegistrarAdvertisement.NAME, true));
         return lSipProxyRegistrarAdvertisement.getAddress();
      }catch (Exception e) {
         throw new P2pProxyException(e);
      }

   }
   public void revokeSipProxy(String aProxy) throws P2pProxyException {
      try {
         SipProxyRegistrarAdvertisement lSipProxyRegistrarAdvertisement = (SipProxyRegistrarAdvertisement) (mJxtaNetworkManager.getAdvertisement(null, SipProxyRegistrarAdvertisement.ADDRESS_TAG, true));
         mJxtaNetworkManager.getPeerGroup().getDiscoveryService().flushAdvertisement(lSipProxyRegistrarAdvertisement);
         mLog.info(aProxy +"revoked");
      } catch (Exception e) {
         throw new P2pProxyException(e);
      }  
   }

}
