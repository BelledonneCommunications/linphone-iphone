package org.linphone.p2pproxy.core;

import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxySipProxyRegistrarManagement;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrarAdvertisement;

public class P2pProxySipProxyRegistrarManagementImpl implements P2pProxySipProxyRegistrarManagement {
	protected final JxtaNetworkManager mJxtaNetworkManager;
	P2pProxySipProxyRegistrarManagementImpl(JxtaNetworkManager aJxtaNetworkManager) {
		mJxtaNetworkManager = aJxtaNetworkManager;
	}
	public String getSipProxyRegistrarUri() throws P2pProxyException {
		try {
			SipProxyRegistrarAdvertisement lSipProxyRegistrarAdvertisement = (SipProxyRegistrarAdvertisement) (mJxtaNetworkManager.getAdvertisement(null, SipProxyRegistrarAdvertisement.NAME, true));
			return lSipProxyRegistrarAdvertisement.getAddress();
		}catch (Exception e) {
			throw new P2pProxyException(e);
		}

	}

}
