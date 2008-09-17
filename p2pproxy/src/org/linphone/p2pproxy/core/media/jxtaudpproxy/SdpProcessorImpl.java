/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

SdpProcessorImpl.java - .

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
package org.linphone.p2pproxy.core.media.jxtaudpproxy;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.util.Map;

import net.jxta.pipe.OutputPipe;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.Configurator;

import org.linphone.p2pproxy.core.sipproxy.SdpProcessor;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrar;
import org.linphone.p2pproxy.core.sipproxy.SipProxyRegistrar.Registration;
import org.linphone.p2pproxy.core.sipproxy.peers.NetworkResources;
import org.zoolu.sdp.AttributeField;
import org.zoolu.sdp.ConnectionField;
import org.zoolu.sdp.MediaDescriptor;
import org.zoolu.sdp.MediaField;
import org.zoolu.sdp.SessionDescriptor;
import org.zoolu.sip.message.Message;

public class SdpProcessorImpl implements SdpProcessor {
   private final static Logger mLog = Logger.getLogger(SdpProcessorImpl.class);
   private final Map<String,Registration> mRegistrationTab;
   private final Configurator mProperties;
   public SdpProcessorImpl(Map<String,Registration> aRegistrationTab,Configurator aProperties) {
      mRegistrationTab = aRegistrationTab;
      mProperties = aProperties;;
   }
   /*
    * process SDP in mode relay
    * rewrite sdp.
    */
   public void processSdpBeforeSendingToSipUA( Message aMessage) throws P2pProxyException {
      try {
         //check if sdp present
         if (aMessage.hasBody() && "application/sdp".equals(aMessage.getBodyType())) {
            SessionDescriptor lOrigSessionDescriptor = new SessionDescriptor(aMessage.getBody());
            SessionDescriptor lNewSessionDescriptor = new SessionDescriptor(aMessage.getBody());
            lNewSessionDescriptor.removeMediaDescriptors();
            lNewSessionDescriptor.setConnection(null);

            String lUserName="";
            if (aMessage.isInvite()) {
               lUserName = aMessage.getToHeader().getNameAddress().getAddress().toString();
            } else if (aMessage.isResponse()) {
               lUserName = aMessage.getFromHeader().getNameAddress().getAddress().toString();
            } else if (aMessage.isAck()) {
               lUserName = aMessage.getToHeader().getNameAddress().getAddress().toString();
            } else {
               mLog.warn("strange, sdp in message ["+aMessage+"]");
            }
            // build rtp session if required
            NetworkResources lNetworkResources = buildRtpSessions(lOrigSessionDescriptor, lUserName);

            for (Object lMediaDescriptorObject :lOrigSessionDescriptor.getMediaDescriptors()) {
               MediaDescriptor lMediaDescriptor = (MediaDescriptor)lMediaDescriptorObject;
               RtpSessionImpl lRtpSession = lNetworkResources.getRtpSession(lMediaDescriptor.getMedia().getMedia()); 
               // create new media desc
               ConnectionField lConnectionField = new ConnectionField("IP4",InetAddress.getLocalHost().getHostAddress());
               MediaField lOrigMediaField = lMediaDescriptor.getMedia();
               MediaField lNewMediaField = new MediaField(lOrigMediaField.getMedia()
                     , lRtpSession.getPort() //new port
                     , 0
                     , lOrigMediaField.getTransport()
                     , lOrigMediaField.getFormatList());
               MediaDescriptor lNewMediaDescriptor = new MediaDescriptor(lNewMediaField,lConnectionField);
               for (Object lAttributeField:lMediaDescriptor.getAttributes()) {
                  lNewMediaDescriptor.addAttribute((AttributeField) lAttributeField);
               }
               lNewSessionDescriptor.addMediaDescriptor(lNewMediaDescriptor);

            }
            aMessage.setBody(lNewSessionDescriptor.toString());
            mLog.debug("new sdp:" +aMessage.getBody());

         }
      }catch (Exception e) {
         throw new P2pProxyException(e);
      }

   }
   public void processSdpAfterSentToPipe( Message aMessage,OutputPipe lOutputPipe) throws P2pProxyException {
      try {
         if (aMessage.hasBody() && "application/sdp".equals(aMessage.getBodyType())) {
            SessionDescriptor lOrigSessionDescriptor = new SessionDescriptor(aMessage.getBody());
            String lUserName="";
            if (aMessage.isInvite()) {
               lUserName = aMessage.getFromHeader().getNameAddress().getAddress().toString();
            } else if (aMessage.isResponse()) {
               lUserName = aMessage.getToHeader().getNameAddress().getAddress().toString();
            } else if (aMessage.isAck()) {
               lUserName = aMessage.getFromHeader().getNameAddress().getAddress().toString();
            } else {
               mLog.warn("strange, sdp in message ["+aMessage+"]");
            }
            // build rtp session if required
            NetworkResources lNetworkResources = buildRtpSessions(lOrigSessionDescriptor, lUserName,lOutputPipe);
            
            // check remote address
            ConnectionField lConnectionField = lOrigSessionDescriptor.getConnection();
            //search from m blocs
            for (Object lMediaDescriptorObject :lOrigSessionDescriptor.getMediaDescriptors()) {
               MediaDescriptor lMediaDescriptor = (MediaDescriptor)lMediaDescriptorObject;
               if (lConnectionField == null ) {
                  lConnectionField = lMediaDescriptor.getConnection();
               }
               RtpSession lRtpSession = lNetworkResources.getRtpSession(lMediaDescriptor.getMedia().getMedia());
               InetAddress lInetAddress =  InetAddress.getByName(lConnectionField.getAddress());
               InetSocketAddress lInetSocketAddress = new InetSocketAddress(lInetAddress,lMediaDescriptor.getMedia().getPort());
               lRtpSession.setRemoteAddress(lInetSocketAddress);
               mLog.info("rtp session updated ["+lRtpSession+"]");
            }
            // 

         }       
      }catch (Exception e) {
         throw new P2pProxyException(e);
      }

   }
   private NetworkResources buildRtpSessions(SessionDescriptor lSessionDescriptor , String aFrom) throws P2pProxyException {
	   return buildRtpSessions(lSessionDescriptor , aFrom,null);
   }
   private NetworkResources buildRtpSessions(SessionDescriptor lSessionDescriptor , String aFrom,OutputPipe aRemotePipe) throws P2pProxyException {
      try { 
         // get Registration 
         Registration lRegistration = mRegistrationTab.get(aFrom);

         for (Object lMediaDescriptorObject :lSessionDescriptor.getMediaDescriptors()) {
            MediaDescriptor lMediaDescriptor = (MediaDescriptor)lMediaDescriptorObject;
            //1 check if port != 0
            if (lMediaDescriptor.getMedia().getPort() == 0) {
               // nothing to change, offers rejected
               break;
            }
            //3 open datagram socket
            if (lMediaDescriptor.getMedia().getTransport().equalsIgnoreCase("RTP/AVP") == false) { 
               mLog.warn("enable to process non udp mlines ["+lMediaDescriptor.getMedia().getTransport()+"]");
               break;
            }

            String lMLineName = lMediaDescriptor.getMedia().getMedia();
            RtpSessionImpl lRtpSession = null;
            if (((NetworkResources) lRegistration.NetResources).hasRtpSession(lMLineName) == false) {
               // first time, just create
               int lPortStart = Integer.parseInt(mProperties.getProperty(SipProxyRegistrar.UDP_MEDIA_RELAY_PORT_START, "15000"));
               lRtpSession = new RtpSessionImpl (lPortStart,lMLineName);
               ((NetworkResources) lRegistration.NetResources).putRtpSession(lMLineName, lRtpSession);
            } else {
               lRtpSession = ((NetworkResources) lRegistration.NetResources).getRtpSession(lMLineName);
            }
            if (aRemotePipe != null) {
           	 lRtpSession.setRemotePipe(aRemotePipe);
            }

         }
         return (NetworkResources) lRegistration.NetResources;
      }catch (Exception e) {
         throw new P2pProxyException(e);
      }

   }
   public void processSdpBeforeSendingToPipe(Message message) throws P2pProxyException {
      // TODO Auto-generated method stub

   }
}
