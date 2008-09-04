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
import org.zoolu.sdp.ConnectionField;
import org.zoolu.sdp.MediaDescriptor;
import org.zoolu.sdp.MediaField;
import org.zoolu.sdp.SessionDescriptor;
import org.zoolu.sip.message.Message;

/**
 * rewrite SDP to insert an ICE aware rtp relay 
 *
 * 
 */
public class IceSdpProcessorImpl implements SdpProcessor {
   private final String CANDIDATE_RELAY_NAME="relay";

   class CandidateAttributeParser {
      final private  String mCandidateAttribute;

      final private String mfoundation;
      final private String mComponentId;
      final private String mTransport;
      final private String mPriority; 
      final private String mConnectionAddress;
      final private String mPort;    
      final private String mType;    

      public CandidateAttributeParser(String aCandidateAttribute){
         mCandidateAttribute = aCandidateAttribute;
         StringTokenizer st = new StringTokenizer(mCandidateAttribute);
         mfoundation = st.nextToken();
         mComponentId = st.nextToken();
         mTransport = st.nextToken();
         mPriority = st.nextToken(); 
         mConnectionAddress = st.nextToken();
         mPort = st.nextToken();
         st.nextToken(); //skip typ
         mType = st.nextToken();;
      }
      public CandidateAttributeParser(InetSocketAddress aRelayAddress){
         mfoundation = "3";
         mComponentId = "1";
         mTransport = "UDP";
         mPriority = "0"; 
         mConnectionAddress =  aRelayAddress.getAddress().getHostAddress();
         mPort =  String.valueOf(aRelayAddress.getPort());    
         mType = CANDIDATE_RELAY_NAME; 
         mCandidateAttribute = toString();
      }

      InetSocketAddress getAddress() {
         return new InetSocketAddress(mConnectionAddress,Integer.parseInt(mPort));
      }
      public String toString() {
         return mfoundation +" "+ mComponentId +" "+ mTransport +" "+ mPriority +" "+ mConnectionAddress +" "+ mPort +" typ " +mType;
      }
   }

   private final static Logger mLog = Logger.getLogger(IceSdpProcessorImpl.class);
   private final Map<String,Registration> mRegistrationTab;
   private final P2pProxyRtpRelayManagement mP2pProxyRtpRelayManagement;
   public IceSdpProcessorImpl(Map<String,Registration> aRegistrationTab,P2pProxyRtpRelayManagement aP2pProxyRtpRelayManagement) {
      mRegistrationTab = aRegistrationTab;
      mP2pProxyRtpRelayManagement = aP2pProxyRtpRelayManagement;

   }
   public void processSdpAfterSentToPipe(Message message, OutputPipe outputPipe)
   throws P2pProxyException {
      //nop

   }

   public void processSdpBeforeSendingToSipUA(Message aMessage) throws P2pProxyException {
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
      processSdp(aMessage,lUserName);
   }

   public void processSdpBeforeSendingToPipe(Message aMessage) throws P2pProxyException {
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
      processSdp(aMessage,lUserName);
   }

   //check if already have relay candidate
   //1 search for relay if not available
   //2 rewrite c line /port
   //3 add candidate
   //4 if relay was available, clean relay  
   private void processSdp(Message aMessage, String aUserName) throws P2pProxyException {
      //check if sdp present
      if (aMessage.hasBody() && "application/sdp".equals(aMessage.getBodyType())) {
         SessionDescriptor lOrigSessionDescriptor = new SessionDescriptor(aMessage.getBody());

         SessionDescriptor lNewSessionDescriptor = new SessionDescriptor(aMessage.getBody());
         lNewSessionDescriptor.removeMediaDescriptors();
         lNewSessionDescriptor.setConnection(null);

         // get Registration 
         Registration lRegistration = mRegistrationTab.get(aUserName);
         if (lRegistration == null) {
            throw new P2pProxyException("unknown user ["+aUserName+"]");
         }
         boolean lrelayCandidateDetected = false;
         //check if already have relay candidate
         for (Object lMediaDescriptorObject :lOrigSessionDescriptor.getMediaDescriptors()) {
            MediaDescriptor lMediaDescriptor = (MediaDescriptor)lMediaDescriptorObject;
            MediaType lMediaType = MediaType.parseString(lMediaDescriptor.getMedia().getMedia());
            String lRelayCandidateValue = getRelayCandidate(lMediaDescriptor); 

            if (lRelayCandidateValue != null) {
               if (lRegistration.RtpRelays.containsKey(lMediaType) == false) {// get relay address from from candidate
                  CandidateAttributeParser lCandidateAttributeParser = new CandidateAttributeParser(lRelayCandidateValue);
                  lRegistration.RtpRelays.put(lMediaType, lCandidateAttributeParser.getAddress());
                  mLog.info("relay candidate detected, adding relay ["+lCandidateAttributeParser.getAddress()+" for ["+lMediaDescriptor.getMedia().getMedia()+"]");
               } else {
                  lRegistration.RtpRelays.remove(lMediaType);
                  mLog.info("relay candidate  removing address ");
               }
               lrelayCandidateDetected = true;
            } else {
               InetSocketAddress lInetSocketAddress=null;
               if (lRegistration.RtpRelays.containsKey(lMediaType) == false) {
                  //put relay candidate from network
                  lRegistration.RtpRelays = mP2pProxyRtpRelayManagement.getAddresses();
                  lInetSocketAddress = lRegistration.RtpRelays.get(lMediaType);
               } else {
                  lInetSocketAddress = lRegistration.RtpRelays.get(lMediaType);
                  lRegistration.RtpRelays.remove(lMediaType);
                  mLog.info("no relay candidate relay removing address ");
               }

               // build candidate attribute
               CandidateAttributeParser lCandidateAttributeParser = new CandidateAttributeParser(lInetSocketAddress );
               // create new media desc
               ConnectionField lConnectionField = new ConnectionField("IP4",lInetSocketAddress.getAddress().getHostAddress());
               MediaField lOrigMediaField = lMediaDescriptor.getMedia();
               MediaField lNewMediaField = new MediaField(lOrigMediaField.getMedia()
                     , lInetSocketAddress.getPort() //new port
                     , 0
                     , lOrigMediaField.getTransport()
                     , lOrigMediaField.getFormatList());
               MediaDescriptor lNewMediaDescriptor = new MediaDescriptor(lNewMediaField,lConnectionField);
               for (Object lAttributeField:lMediaDescriptor.getAttributes()) {
                  lNewMediaDescriptor.addAttribute((AttributeField) lAttributeField);
               }
               // add relay candidate
               AttributeField lAttributeField = new AttributeField("candidate", lCandidateAttributeParser.toString());
               lNewMediaDescriptor.addAttribute(lAttributeField);
               lNewSessionDescriptor.addMediaDescriptor(lNewMediaDescriptor);
            } 

         }
         if (lrelayCandidateDetected == false) {
        	 //to work-around mjsip bug
        	 lNewSessionDescriptor.setConnection(((MediaDescriptor)lNewSessionDescriptor.getMediaDescriptors().elementAt(0)).getConnection());
        	 aMessage.setBody(lNewSessionDescriptor.toString());
            mLog.debug("new sdp:" +aMessage.getBody());
         }

      }
   }

   private String getRelayCandidate(MediaDescriptor aMediaDescriptor) {
      //1 get candidate
      for (Object lField:aMediaDescriptor.getAttributes()) {
         AttributeField lAttributeField = (AttributeField)lField;
         if (lAttributeField.getAttributeName().equalsIgnoreCase("candidate") && lAttributeField.getAttributeValue().contains(CANDIDATE_RELAY_NAME)) {
            return lAttributeField.getAttributeValue();
         }
      }
      return null;
   }

}
