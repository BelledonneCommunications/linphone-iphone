/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

SuperPeerProxy.java - .

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
package org.linphone.p2pproxy.core.sipproxy.superpeers;

import java.util.Map;

import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.sipproxy.RegistrationHandler;
import org.linphone.p2pproxy.core.sipproxy.SipProxy;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrar.Registration;

import org.zoolu.sip.message.Message;
import org.zoolu.sip.provider.SipProvider;

public class SuperPeerProxy implements SipProxy, RegistrationHandler {
   private final JxtaNetworkManager mJxtaNetworkManager;
   private final String mRegistrarAddress;
   private final Map<String,Registration> mRegistrationTab;
   
   public SuperPeerProxy(JxtaNetworkManager aJxtaNetworkManager, String aRegistrarAddress, Map<String,Registration> aRegistrationTab ) {
      mJxtaNetworkManager = aJxtaNetworkManager;
      mRegistrarAddress = aRegistrarAddress;
      mRegistrationTab = aRegistrationTab;
   }
   public void proxyRequest(SipProvider provider, Message message) throws P2pProxyException {
      // 1 check if user is a local user
      

   }

   public void proxyResponse(SipProvider provider, Message message) throws P2pProxyException {
      // TODO Auto-generated method stub

   }

   public void updateRegistration(Registration aRegistration, Message aRegistrationMessage) throws P2pProxyException {
      if (aRegistration.NetResources == null) {
         // new registration, create adv
         JxtaNetworkResources lRegistration = new JxtaNetworkResources(aRegistration.From, mJxtaNetworkManager,mRegistrarAddress);  
         aRegistration.NetResources = lRegistration;
      }

      ((JxtaNetworkResources) aRegistration.NetResources).publish(aRegistration.Expiration);
   }
}
