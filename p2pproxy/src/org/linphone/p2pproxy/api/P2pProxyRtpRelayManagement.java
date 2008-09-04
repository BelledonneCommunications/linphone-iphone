package org.linphone.p2pproxy.api;

import java.net.InetSocketAddress;
import java.util.Map;

import org.linphone.p2pproxy.core.media.rtprelay.MediaType;

public interface P2pProxyRtpRelayManagement {
	
    /**
     * 
     * @param aSource get list Socket address available for relay
     */
    public Map<MediaType,InetSocketAddress> getAddresses() throws P2pProxyException ;
}
