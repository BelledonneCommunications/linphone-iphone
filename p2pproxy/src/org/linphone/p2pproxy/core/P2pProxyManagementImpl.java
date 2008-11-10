/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

P2pProxyManagementImpl.java - .

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

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.net.UnknownHostException;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxyManagement;

import org.linphone.p2pproxy.core.rdvautoconfig.PeerInfoServiceClient;

public abstract class P2pProxyManagementImpl extends P2pProxyResourceManagementImpl implements ServiceProvider,P2pProxyManagement {
   protected final Configurator mConfigurator;
   private final PeerInfoServiceClient mPeerInfoServiceClient;
   private final static Logger mLog = Logger.getLogger(P2pProxyManagementImpl.class);
   
   P2pProxyManagementImpl(Configurator aConfigurator, JxtaNetworkManager aJxtaNetworkManager) throws SocketException, UnknownHostException
    {
        super(aJxtaNetworkManager);
        mConfigurator = aConfigurator;
        mPeerInfoServiceClient = new PeerInfoServiceClient(aConfigurator, aJxtaNetworkManager);
    }

    public void start(long aTimeout) throws P2pProxyException
    {
        mPeerInfoServiceClient.start(aTimeout);
        
    }

    public void stop() {
        mPeerInfoServiceClient.stop();
        mLog.info("P2pProxyManagementImpl stopped");
    }

    public InetAddress getPublicIpAddress()
        throws P2pProxyException
    {
        return mPeerInfoServiceClient.getPublicIpAddress();
    }

   public boolean probeSocket(InetSocketAddress aSocketAddress, Protocol aProtocol) throws P2pProxyException {
      return mPeerInfoServiceClient.probeSocket(aSocketAddress, aProtocol);
   }

 
 

}
