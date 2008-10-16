/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

P2pProxyInstance.java -- 

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
package org.linphone.p2pproxy.api;

import org.zoolu.sip.provider.SipProvider;

public interface P2pProxyInstance {
   public static int BASE_HTTP = 30700;
   public static int BASE_TCP = 9701;
    
   public enum Mode {relay, edge, auto,seeding_server};
   /**
    * @return Returns the Mode.
    */
   public abstract Mode getMode();

   /**
    * @param aModet.
    */
   public abstract void setMode(Mode aMode);

   /**
    * @return Returns the mIndex.
    */
   public abstract int getIndex();

   /**
    * @param index The mIndex to set.
    */
   public abstract void setIndex(int index);

   public abstract void start() throws Exception;
   public abstract void stop() throws Exception;
   
   public abstract boolean isStarted() throws P2pProxyException; 
   
   public abstract SipProvider getSipClientProvider();
   
   public abstract String getSipClientName();
   
   public int getNumberOfconnectedPeers();
   
   public Object getOpaqueNetworkManager();
   
   public void setPrivateHostAdress(String anAddress);
   
   public void setPublicHostAdress(String anAddress);

   public abstract P2pProxyNetworkProbe getManager();
   
   public abstract P2pProxyRtpRelayManagement getRtpRelayManager();
   
   public void setRelayCapacity(int aCapacity);
   
   public void setProperty(String key,String value) throws P2pProxyException;
   
   public int getAdvertisementDiscoveryTimeout();
   
}