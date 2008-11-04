package org.linphone.p2pproxy.core;

import java.util.List;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxyResourceManagement;
import org.linphone.p2pproxy.core.media.MediaResourceService;
import org.linphone.p2pproxy.core.sipproxy.NetworkResourceAdvertisement;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrar;

public class P2pProxyResourceManagementImpl implements P2pProxyResourceManagement {
   protected final JxtaNetworkManager mJxtaNetworkManager;
   
   private final static Logger mLog = Logger.getLogger(P2pProxyResourceManagementImpl.class);
   P2pProxyResourceManagementImpl(JxtaNetworkManager aJxtaNetworkManager) {
      mJxtaNetworkManager = aJxtaNetworkManager;
   }
   public String[] lookupSipProxiesUri(String aDomaine) throws P2pProxyException {
      try {
         if (!DOMAINE.equals(aDomaine)) {
            //unknown domaine
            return new String[0];
         }
         List<NetworkResourceAdvertisement> lSipProxyRegistrarAdvertisements =  (List<NetworkResourceAdvertisement>) (mJxtaNetworkManager.getAdvertisementList(null, "Name",SipProxyRegistrar.ADV_NAME, true,2));
         String[] lAddresses = new String[lSipProxyRegistrarAdvertisements.size()];
         for (int i=0;i<lSipProxyRegistrarAdvertisements.size();i++) {
        	 lAddresses[i] = lSipProxyRegistrarAdvertisements.get(i).getAddress();
         }
         return lAddresses;
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
    try {

        List<NetworkResourceAdvertisement> lMediaResoureAdvertisements =  (List<NetworkResourceAdvertisement>) (mJxtaNetworkManager.getAdvertisementList(null, "Name",MediaResourceService.ADV_NAME, true,2));
        String[] lAddresses = new String[lMediaResoureAdvertisements.size()];
        for (int i=0;i<lMediaResoureAdvertisements.size();i++) {
       	 lAddresses[i] = lMediaResoureAdvertisements.get(i).getAddress();
        }
        return lAddresses;
     }catch (Exception e) {
        throw new P2pProxyException(e);
     }
}

}
