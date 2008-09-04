package org.linphone.p2pproxy.core.media.rtprelay;


import java.net.InetSocketAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Map;


import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.Configurator;
import org.linphone.p2pproxy.core.GenericServiceClient;
import org.linphone.p2pproxy.core.JxtaNetworkManager;

public class RtpRelayServiceClient extends GenericServiceClient {
   private final static Logger mLog = Logger.getLogger(RtpRelayServiceClient.class);
   

   public RtpRelayServiceClient(Configurator aProperties,JxtaNetworkManager aJxtaNetworkManager) throws SocketException, UnknownHostException {
      super (aProperties,aJxtaNetworkManager,RtpRelayService.ADV_NAME);
   }

   public void stop() {
      super.stop();
   }



   public Map<MediaType, InetSocketAddress> getAddresses() throws P2pProxyException {
      try {
         checkObject();
         checkSocketConnection();
         AddressRequest lAddressRequest = new AddressRequest();
         mOut.writeObject(lAddressRequest);
         mOut.flush();
         mLog.info("request message ["+lAddressRequest+"] sent");
         Object lInputObj = mIn.readObject();
         mLog.info("response message ["+lInputObj+"] received");
         if(lInputObj instanceof AddressResponse) {
            AddressResponse lAddressResponse = (AddressResponse)lInputObj;
            if (mLog.isDebugEnabled()) mLog.debug("receiving relay address ["+lAddressResponse.toString()+"]");
            return lAddressResponse.getAddressTable();
         } else {
            throw new P2pProxyException("unknown response ["+lInputObj+"]");
         }
      } catch(Exception e) {
         throw new P2pProxyException(e);
      }      

   }

}
