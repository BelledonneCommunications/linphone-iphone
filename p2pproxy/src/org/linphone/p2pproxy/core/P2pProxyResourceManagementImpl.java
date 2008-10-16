package org.linphone.p2pproxy.core;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxyResourceManagement;
import org.linphone.p2pproxy.core.sipproxy.NetworkResourceAdvertisement;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrar;

public class P2pProxyResourceManagementImpl implements P2pProxyResourceManagement {
   protected final JxtaNetworkManager mJxtaNetworkManager;
   
   private final static Logger mLog = Logger.getLogger(P2pProxyResourceManagementImpl.class);
   P2pProxyResourceManagementImpl(JxtaNetworkManager aJxtaNetworkManager) {
      mJxtaNetworkManager = aJxtaNetworkManager;
   }
   public String lookupSipProxyUri(String aDomaine) throws P2pProxyException {
      try {
         if (!DOMAINE.equals(aDomaine)) {
            //unknown domaine
            return null;
         }
         NetworkResourceAdvertisement lSipProxyRegistrarAdvertisement = (NetworkResourceAdvertisement) (mJxtaNetworkManager.getAdvertisement(null, SipProxyRegistrar.ADV_NAME, true));
         return lSipProxyRegistrarAdvertisement.getAddress();
      }catch (Exception e) {
         throw new P2pProxyException(e);
      }

   }
   public void revokeSipProxy(String aProxy) throws P2pProxyException {
      try {
         NetworkResourceAdvertisement lSipProxyRegistrarAdvertisement = (NetworkResourceAdvertisement) (mJxtaNetworkManager.getAdvertisement(null, NetworkResourceAdvertisement.ADDRESS_TAG, true));
         mJxtaNetworkManager.getPeerGroup().getDiscoveryService().flushAdvertisement(lSipProxyRegistrarAdvertisement);
         mLog.info(aProxy +"revoked");
      } catch (Exception e) {
         throw new P2pProxyException(e);
      }  
   }
public String[] getMediaServerList() throws P2pProxyException {
	throw new RuntimeException("not implmented yet");
}

}
