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
package org.linphone.p2pproxy.core.media.rtprelay;

import java.net.InetSocketAddress;
import java.util.Map;
import java.util.StringTokenizer;

import net.jxta.pipe.OutputPipe;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.api.P2pProxyRtpRelayManagement;
import org.linphone.p2pproxy.core.SdpProcessor;
import org.linphone.p2pproxy.core.SipProxyRegistrar.Registration;
import org.zoolu.sdp.AttributeField;
import org.zoolu.sdp.MediaDescriptor;
import org.zoolu.sdp.SessionDescriptor;
import org.zoolu.sip.message.Message;

/**
 * rewrite SDP to insert a=relay-add
 *
 * 
 */
public class SdpProcessorImpl implements SdpProcessor {
   private final String RELAY_ADDR_NAME="relay-addr";

 
   private final static Logger mLog = Logger.getLogger(SdpProcessorImpl.class);
   private final Map<String,Registration> mRegistrationTab;
   private final P2pProxyRtpRelayManagement mP2pProxyRtpRelayManagement;
   public SdpProcessorImpl(Map<String,Registration> aRegistrationTab,P2pProxyRtpRelayManagement aP2pProxyRtpRelayManagement) {
      mRegistrationTab = aRegistrationTab;
      mP2pProxyRtpRelayManagement = aP2pProxyRtpRelayManagement;

   }
   public void processSdpAfterSentToPipe(Message message, OutputPipe outputPipe)
   throws P2pProxyException {
      //nop

   }

   public void processSdpBeforeSendingToSipUA(Message aMessage) throws P2pProxyException {
      //nop
   }

   public void processSdpBeforeSendingToPipe(Message aMessage) throws P2pProxyException {
      if (aMessage.isInvite() || aMessage.isAck()) {
         //check if sdp present
         if (aMessage.hasBody() && "application/sdp".equals(aMessage.getBodyType())) {
            SessionDescriptor lOrigSessionDescriptor = new SessionDescriptor(aMessage.getBody());
            //check if already have relay-session-id
            Map <MediaType,InetSocketAddress> lRelayAddresses = null;
            for (Object lMediaDescriptorObject :lOrigSessionDescriptor.getMediaDescriptors()) {
               MediaDescriptor lMediaDescriptor = (MediaDescriptor)lMediaDescriptorObject;
               MediaType lMediaType = MediaType.parseString(lMediaDescriptor.getMedia().getMedia());
               AttributeField lSessionID = lMediaDescriptor.getAttribute("relay-session-id");
               AttributeField lRelayAddr = lMediaDescriptor.getAttribute(RELAY_ADDR_NAME);
               if (lSessionID != null  && lRelayAddr == null) {
                  //need to find a relay
                  if (lRelayAddresses == null) lRelayAddresses = mP2pProxyRtpRelayManagement.getAddresses();
                  if (lRelayAddresses.get(lMediaType) !=  null) {
                     lRelayAddr = new AttributeField(RELAY_ADDR_NAME, lRelayAddresses.get(lMediaType).toString().substring(1));
                     lMediaDescriptor.addAttribute(lRelayAddr);
                     mLog.info("adding relay ["+lRelayAddr+ "to mline ["+lMediaType+"]");
                  } else {
                     mLog.warn("no relay for this media type ["+lMediaType+"]");

                  }
               }

            } 
         } else {
            mLog.info("strange, sdp in message ["+aMessage+"]");
         }
      }
   }
}
