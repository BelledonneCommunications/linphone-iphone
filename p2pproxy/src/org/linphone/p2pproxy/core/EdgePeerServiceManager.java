/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

EdgePeerServiceManager.java - EdgePeer Service Manager.

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


import java.net.SocketException;
import java.net.UnknownHostException;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.rdvautoconfig.AutoConfigService;

// Referenced classes of package org.linphone.p2pproxy.core:
//            ServiceProvider, JxtaNetworkManager, Configurator

public class EdgePeerServiceManager extends P2pProxyManagementImpl
{
   private final AutoConfigService mAutoConfigService;
   private final static Logger mLog = Logger.getLogger(EdgePeerServiceManager.class);
   EdgePeerServiceManager(Configurator aConfigurator, JxtaNetworkManager aJxtaNetworkManager)throws SocketException, UnknownHostException
    {
      super(aConfigurator,aJxtaNetworkManager);  
      mAutoConfigService = new AutoConfigService(mConfigurator,mJxtaNetworkManager,this);
    }

    public void start(long aTimeout) throws P2pProxyException
    {
        super.start(aTimeout);
        mAutoConfigService.start(aTimeout);
    }

    public void stop() {
       super.stop();
       mAutoConfigService.stop();
       mLog.info("EdgePeerServiceManager stopped");
    }


   public boolean shouldIBehaveAsAnRdv() throws P2pProxyException{
      return mAutoConfigService.canIBehaveAsASuperPeer();
   }


}
