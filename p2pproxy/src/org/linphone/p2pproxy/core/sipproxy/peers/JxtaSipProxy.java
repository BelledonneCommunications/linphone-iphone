/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

JxtaSipProxy.java - .

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
package org.linphone.p2pproxy.core.sipproxy.peers;

import java.io.IOException;
import java.util.Map;

import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.StringMessageElement;
import net.jxta.pipe.OutputPipe;
import net.jxta.pipe.PipeMsgEvent;
import net.jxta.pipe.PipeMsgListener;
import net.jxta.protocol.PipeAdvertisement;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxyRtpRelayManagement;
import org.linphone.p2pproxy.api.P2pProxyUserNotFoundException;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.P2pProxyAdvertisementNotFoundException;
import org.linphone.p2pproxy.core.media.rtprelay.SdpProcessorImpl;
import org.linphone.p2pproxy.core.sipproxy.RegistrationHandler;
import org.linphone.p2pproxy.core.sipproxy.SdpProcessor;
import org.linphone.p2pproxy.core.sipproxy.SipProxy;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrar;
import org.linphone.p2pproxy.core.sipproxy.SipUtils;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrar.Registration;
import org.zoolu.sip.address.NameAddress;
import org.zoolu.sip.address.SipURL;
import org.zoolu.sip.header.MultipleHeader;
import org.zoolu.sip.header.RouteHeader;
import org.zoolu.sip.message.Message;
import org.zoolu.sip.message.MessageFactory;
import org.zoolu.sip.provider.SipProvider;
import org.zoolu.sip.transaction.TransactionServer;

public class JxtaSipProxy implements SipProxy, PipeMsgListener,RegistrationHandler {
   private final JxtaNetworkManager mJxtaNetworkManager;
   private final Map<String,SipProxyRegistrar.Registration> mRegistrationTab;
   private final SdpProcessor mSdpProcessor;
   private final SipProvider mProvider;
   
   private final static Logger mLog = Logger.getLogger(JxtaSipProxy.class);   
   
   public JxtaSipProxy(SipProvider aProvider,JxtaNetworkManager aJxtaNetworkManager, Map<String,SipProxyRegistrar.Registration> aRegistrationTab,P2pProxyRtpRelayManagement aP2pProxyRtpRelayManagement) {
      mJxtaNetworkManager =  aJxtaNetworkManager;
      mSdpProcessor = new SdpProcessorImpl(aP2pProxyRtpRelayManagement);
      mRegistrationTab = aRegistrationTab;
      mProvider = aProvider;
   }
   public void proxyRequest(SipProvider aProvider, Message aMessage) throws P2pProxyException {
      String lTo =  aMessage.getToHeader().getNameAddress().getAddress().toString();
      try {
         mSdpProcessor.processSdpBeforeSendingToPipe(aMessage);
         // proxy message to pipe
         OutputPipe lOutputPipe = sendMessageToPipe(lTo,aMessage.toString());
         mSdpProcessor.processSdpAfterSentToPipe( aMessage,lOutputPipe);
      }  catch (Exception e2) {
         //remove via 
         SipUtils.removeVia(aProvider, aMessage);
         throw new P2pProxyException(e2);

      }


   }

   public void proxyResponse(SipProvider aProvider, Message aMessage) throws P2pProxyException {
      try {
         String lFrom =  aMessage.getFromHeader().getNameAddress().getAddress().toString();
         mSdpProcessor.processSdpBeforeSendingToPipe(aMessage);
         OutputPipe lOutputPipe = sendMessageToPipe(lFrom,aMessage.toString());
         mSdpProcessor.processSdpAfterSentToPipe( aMessage,lOutputPipe);
      } catch(Exception e) {
         throw new P2pProxyException(e);
      }

   }
   private Advertisement getPipeAdv(String aUser,long aDiscoveryTimout,boolean isTryFromLocal) throws InterruptedException, P2pProxyUserNotFoundException, IOException {
      // search on all peers
      try {
         return mJxtaNetworkManager.getAdvertisement(null,aUser, isTryFromLocal);
      } catch (P2pProxyAdvertisementNotFoundException e) {
         throw new P2pProxyUserNotFoundException(e);
      } 
   }
   private OutputPipe sendMessageToPipe(String aDestination,String lContent) throws NumberFormatException, InterruptedException, P2pProxyException, IOException {
      
      //1 search for pipe
      long lTimeout = JxtaNetworkManager.ADV_DISCOVERY_TIMEOUT_INT;
      PipeAdvertisement lPipeAdvertisement = (PipeAdvertisement)getPipeAdv(aDestination,lTimeout,true);
      OutputPipe lOutputPipe=null;
      try {
         // create output pipe
         lOutputPipe = mJxtaNetworkManager.getPeerGroup().getPipeService().createOutputPipe(lPipeAdvertisement, lTimeout);
         //create the message
      } catch (IOException e) {
         //second try from remote only to avoid wrong cached value
          mJxtaNetworkManager.getPeerGroup().getDiscoveryService().flushAdvertisement(lPipeAdvertisement);
          mLog.warn("cannot create output pipe, trying to ask from rdv ",e);
         lPipeAdvertisement = (PipeAdvertisement)getPipeAdv(aDestination,lTimeout,false);
         lOutputPipe = mJxtaNetworkManager.getPeerGroup().getPipeService().createOutputPipe(lPipeAdvertisement, lTimeout);
      }
      net.jxta.endpoint.Message lMessage = new net.jxta.endpoint.Message();
      StringMessageElement lStringMessageElement = new StringMessageElement("SIP", lContent, null);
      lMessage.addMessageElement("SIP", lStringMessageElement);
      //send the message
      lOutputPipe.send(lMessage);
      mLog.debug("message sent to ["+aDestination+"]");
      return lOutputPipe;
      
   }   
//////////////////////////////////////////////////////////////////////
////jxta service methods
/////////////////////////////////////////////////////////////////////   
  

  public void pipeMsgEvent(PipeMsgEvent anEvent) {
     MessageElement lMessageElement = anEvent.getMessage().getMessageElement("SIP");
     if (lMessageElement == null) {
         //nop, this is not for me
         return;
     }
     String lMesssage = lMessageElement.toString();
     mLog.info("pipe event sip message["+lMesssage+"]");
     Message lSipMessage = new Message(lMesssage);
     // process request
     if (lSipMessage.isRequest()) {
        SipURL  lNextHope ;
        // get next hope from registrar
        String lToName = lSipMessage.getToHeader().getNameAddress().getAddress().toString();
        if (mRegistrationTab.containsKey(lToName)) {
           lNextHope = new SipURL(mRegistrationTab.get(lToName).Contact); 
        } else {
           mLog.error("user ["+lToName+"] not found");
           return;
        }
        //RequestLine lRequestLine = new RequestLine(lSipMessage.getRequestLine().getMethod(),lNextHope);
        //lSipMessage.setRequestLine(lRequestLine);
        MultipleHeader lMultipleRoute = lSipMessage.getRoutes();
        RouteHeader lRouteHeader = new RouteHeader(new NameAddress(lNextHope+";lr"));
        //lRouteHeader.setParameter("lr", null);
        if (lMultipleRoute != null) {
           lMultipleRoute.addTop(lRouteHeader);
           lSipMessage.setRoutes(lMultipleRoute);
        } else {
           lSipMessage.addRouteHeader(lRouteHeader);
        }
        // add Via only udp
        SipUtils.addVia(mProvider,lSipMessage);
        // add recordRoute
        SipUtils.addRecordRoute(mProvider,lSipMessage);
        
     } else {
        //response
        //1 remove via header   
        SipUtils.removeVia(mProvider,lSipMessage);
     }
     try {
        mSdpProcessor.processSdpBeforeSendingToSipUA( lSipMessage);
   } catch (P2pProxyException e) {
       mLog.error("enable to re-write sdp",e);
   }
     
     mProvider.sendMessage(lSipMessage);
     //
  }
public void updateRegistration(Registration aRegistration, Message aRegistrationMessage) throws P2pProxyException {
   try {
   if (aRegistration.NetResources == null) {
      // new registration, create pipe
      NetworkResources lRegistration = new NetworkResources(aRegistration.From,mJxtaNetworkManager);  
      aRegistration.NetResources = lRegistration;
      lRegistration.addPipeMsgListener(this);
   }
   
   ((NetworkResources) aRegistration.NetResources).publish(aRegistration.Expiration);
   } catch (Exception e) {
      throw new P2pProxyException(e);
   }
   
}

}
