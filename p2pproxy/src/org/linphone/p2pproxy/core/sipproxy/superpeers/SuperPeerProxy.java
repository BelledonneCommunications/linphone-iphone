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


import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxyUserNotFoundException;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.sipproxy.RegistrationHandler;
import org.linphone.p2pproxy.core.sipproxy.SipProxy;
import org.linphone.p2pproxy.core.sipproxy.SipUtils;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrar.Registration;

import org.zoolu.sip.address.NameAddress;
import org.zoolu.sip.address.SipURL;
import org.zoolu.sip.header.Header;
import org.zoolu.sip.header.MultipleHeader;
import org.zoolu.sip.header.RecordRouteHeader;
import org.zoolu.sip.header.RequestLine;
import org.zoolu.sip.header.RouteHeader;
import org.zoolu.sip.message.Message;
import org.zoolu.sip.provider.SipProvider;

public class SuperPeerProxy implements SipProxy, RegistrationHandler {
   private final JxtaNetworkManager mJxtaNetworkManager;
   private final String mRegistrarAddress;
   private final Map<String,Registration> mRegistrationTab;
   private static Logger mLog = Logger.getLogger(SuperPeerProxy.class);
   
   public SuperPeerProxy(JxtaNetworkManager aJxtaNetworkManager, String aRegistrarAddress, Map<String,Registration> aRegistrationTab ) {
      mJxtaNetworkManager = aJxtaNetworkManager;
      mRegistrarAddress = aRegistrarAddress;
      mRegistrationTab = aRegistrationTab;
   }
   public void proxyRequest(SipProvider aProvider, Message aMessage) throws P2pProxyException {
      // 1 check if user is a local user
      
	   if (mLog.isDebugEnabled()) mLog.debug("processing request " +aMessage);
	   String lTo = aMessage.getToHeader().getNameAddress().getAddress().toString();
	   SipURL lNextHope = null;
	
		   //ok need to find the root
		   //is local ?
		   synchronized (mRegistrationTab) {
			   if (mRegistrationTab.containsKey(lTo)) {
				   //great, just need to get it
				   lNextHope = new SipURL(mRegistrationTab.get(lTo).Contact);
			   } else {
				   throw new P2pProxyUserNotFoundException("user ["+lTo+"] not found");
			   }
		   }
		   if (aMessage.isInvite() || aMessage.isCancel()) {
			   //check if invite
			   // add recordRoute
			   SipUtils.addRecordRoute(aProvider,aMessage);    
		   }
		   
	   if (lNextHope == null) {
		   // not for us
		   //just look at route set
		   MultipleHeader lMultipleRoute = aMessage.getRoutes();
		   
		   if (lMultipleRoute != null && lMultipleRoute.isEmpty()== false) {
			   lNextHope = ((RecordRouteHeader)lMultipleRoute.getTop()).getNameAddress().getAddress();
		   } else {
			   // last proxy, get route from request uri
			   //check if we know the user
			   lNextHope = aMessage.getRequestLine().getAddress();
		   }
		   
	   }
       
       aMessage.setRequestLine(new RequestLine(aMessage.getRequestLine().getMethod(), lNextHope));
       aProvider.sendMessage(aMessage);

   }

   public void proxyResponse(SipProvider aProvider, Message aMessage) throws P2pProxyException {
      if (mLog.isInfoEnabled()) mLog.info("processing response " +aMessage);
      //1 remove via header   
      SipUtils.removeVia(aProvider,aMessage);
      aProvider.sendMessage(aMessage);

   }

   public void updateRegistration(Registration aRegistration, Message aRegistrationMessage) throws P2pProxyException {
      if (aRegistration.NetResources == null) {
         // new registration, create adv
         JxtaNetworkResources lRegistration = new JxtaNetworkResources(aRegistration.From, mJxtaNetworkManager,mRegistrarAddress);  
         aRegistration.NetResources = lRegistration;
      }

      ((JxtaNetworkResources) aRegistration.NetResources).publish(aRegistration.Expiration);
   }
   public String getSipProxyRegistrarAddress() {
      return mRegistrarAddress;
   }
}
