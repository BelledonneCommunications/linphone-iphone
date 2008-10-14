/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

UdpSession.java - .

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

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.GenericUdpSession;



public class RtpRelayServer implements GenericUdpSession.MessageHandler {
   private final static Logger mLog = Logger.getLogger(RtpRelayServer.class);
   private final DatagramSocket mSocket;
   
   
   class RoutingTable {
      class GarbageCollectorTask extends TimerTask {

         public void run() {
            synchronized (RtpRelayServer.RoutingTable.this) {
               for (RoutingElement lElement :new HashMap <String,RoutingElement>(mSessionIdTable).values()) {
                  if (System.currentTimeMillis() - lElement.getLastAccess() > mMaxSilenceDuration) {
                	  mLog.info("removing element ["+lElement+"]");
                	  mSessionIdTable.remove(lElement.mSessionId);
                	  mSsrcElementTable.remove(lElement.mSsrcA);
                	  mSsrcElementTable.remove(lElement.mSsrcB);
                  }
               }
            }
         }
      }
      class RoutingElement {
         //private final String mSessionId;
         private SocketAddress mPeerARtp;
         private SocketAddress mPeerARtcp;
         private long mSsrcA;
         private  SocketAddress mPeerBRtp;
         private  SocketAddress mPeerBRtcp;
         private long mSsrcB;
         private long mLastDestAddressAccess = System.currentTimeMillis();
         private final String mSessionId;
         RoutingElement(long aSsrc, String aSessionId) {
            mSsrcA = aSsrc;
            mSessionId = aSessionId;
         }
         void setPeerAddress(SocketAddress aReceivedAddress,long aSsrc,boolean isRtcp) {
            if (aSsrc == mSsrcA ) {
               if (isRtcp) {
                  mPeerARtcp = aReceivedAddress;
               } else {
                  mPeerARtp = aReceivedAddress;
               }
            } else if (aSsrc == mSsrcB) {
               if (isRtcp) {
                  mPeerBRtcp = aReceivedAddress;
               } else {
                  mPeerBRtp = aReceivedAddress;
               }
            } else {
               mLog.warn("ssrc ["+aSsrc+" not found for ["+aReceivedAddress+"]");
            }
         }
         void setPeerBSsrc(long aSsrc) {
            mSsrcB = aSsrc;
         }
         public SocketAddress getDestAddr(long aSourceSsrc,boolean isRtcp) throws P2pProxyException{
            mLastDestAddressAccess = System.currentTimeMillis();
            if (aSourceSsrc == mSsrcA && isRtcp) {
               if  ( mPeerBRtcp != null) {
                  return mPeerBRtcp ;
               } else {
                  throw new P2pProxyException("PeerBRtcp not found for ssrc ["+aSourceSsrc+"]");
               }
            } else if (aSourceSsrc == mSsrcA && !isRtcp) {

               if  ( mPeerBRtp != null) {
                  return mPeerBRtp ;
               } else {
                  throw new P2pProxyException("PeerBRtp not found for ssrc ["+aSourceSsrc+" ");
               }
            } else if (aSourceSsrc == mSsrcB && isRtcp) {

               if  ( mPeerARtcp != null) {
                  return mPeerARtcp ;
               } else {
                  throw new P2pProxyException("PeerARtcp not found for ssrc ["+aSourceSsrc+" ");
               }
            } else if (aSourceSsrc == mSsrcB && !isRtcp) {

               if  ( mPeerARtp != null) {
                  return mPeerARtp ;
               } else {
                  throw new P2pProxyException("PeerARtp not found for ssrc ["+aSourceSsrc+" ");
               }
            }else {
               throw new P2pProxyException("ssrc ["+aSourceSsrc+" not found");
            }

         }
         public long getLastAccess() {
            return mLastDestAddressAccess;
         }
         public String toString() {
        	 return "Session id ["+mSessionId+"] ssrc a ["+mSsrcA+"] rtp source ["+mPeerARtp+"] rtcp source ["+mPeerARtcp+"]"
        	 		+"ssrc b ["+mSsrcB+"] rtp source ["+mPeerBRtp+"] rtcp source ["+mPeerBRtcp+"]";
         }
      }
      private final Map <String,RoutingElement> mSessionIdTable =  new HashMap <String,RoutingElement>();
      private final Map <Long,RoutingElement> mSsrcElementTable =  new HashMap <Long,RoutingElement>();
      private final long mMaxSilenceDuration ;
      private final long mGCPeriod;
      Timer mTimer = new Timer("Routing Elements GC");

      public RoutingTable (long aMaxSilenceDuration, long aGCPeriod) {
         mMaxSilenceDuration = aMaxSilenceDuration;
         mGCPeriod = aGCPeriod;
         mTimer.scheduleAtFixedRate(new GarbageCollectorTask(), 0, mGCPeriod);
      }
      public synchronized boolean containsSessionId(String aSessionId) {
         return mSessionIdTable.containsKey(aSessionId);
      }

      public synchronized void addRoutingElement(String aSessionId, long aSsrc) {
         if (mLog.isInfoEnabled()) mLog.info("add routing element for session id ["+aSessionId+"] with ssrc ["+aSsrc+"]");
         RoutingElement lRoutingElement = new RoutingElement(aSsrc,aSessionId); 
         mSessionIdTable.put(aSessionId, lRoutingElement);
         mSsrcElementTable.put(aSsrc, lRoutingElement);
      }

      public synchronized void UpdateRoutingElement(String aSessionId, long aSsrc) {
         if (mLog.isInfoEnabled()) mLog.info("update routing element session id ["+aSessionId+"] with ssrc ["+aSsrc+"]");
         RoutingElement lRoutingElement = mSessionIdTable.get(aSessionId);
         lRoutingElement.setPeerBSsrc(aSsrc);
         mSsrcElementTable.put(aSsrc, lRoutingElement);
         
      }
      
      public synchronized void updateSourceAddress(long aSsrc, SocketAddress aSocketAddress,boolean isRtcp) throws P2pProxyException{
         if (mSsrcElementTable.containsKey(aSsrc) == false) {
            throw new P2pProxyException("No routing element present for ssrc["+aSsrc+"]");
         }
         if (mLog.isInfoEnabled()) mLog.info("update routing element for ssrc ["+aSsrc+"] with address ["+aSocketAddress+"]" );
         RoutingElement lRoutineElement = mSsrcElementTable.get(aSsrc);
         lRoutineElement.setPeerAddress(aSocketAddress, aSsrc, isRtcp);
      }
      public synchronized SocketAddress getDestAddress(long aSsrc,boolean isRtcp) throws P2pProxyException{
         //1 check if element exist for this ssrc
         if (mSsrcElementTable.containsKey(aSsrc) == false) {
            throw new P2pProxyException("No routing element present for ssrc["+aSsrc+"]");
         }
         RoutingElement lRoutingElement = mSsrcElementTable.get(aSsrc);
         return lRoutingElement.getDestAddr(aSsrc, isRtcp);
      }
      public synchronized int getSize() {
         return mSessionIdTable.size();
      }
   }
   

   
   private static final String SESSIONID_NAME="RSID"; //Relay session Id 
   private  final RoutingTable mRoutingTable;
   public RtpRelayServer(DatagramSocket aListeningSocket) throws SocketException, UnknownHostException {
      this(aListeningSocket,3600000,60000);
   }
   public RtpRelayServer(DatagramSocket aListeningSocket,long aMaxSilenceDuration, long aGCPeriod) throws SocketException, UnknownHostException {
      mRoutingTable = new RoutingTable(aMaxSilenceDuration,aGCPeriod);
      mSocket = aListeningSocket;
     
   }
   public void onMessage(DatagramPacket aMessage) {
      try {
         if (mLog.isInfoEnabled()) mLog.info("new incoming message from ["+aMessage.getSocketAddress()+"]");
         long lSsrc = getSsrc(aMessage);
         if (isSessionIdPresent(aMessage)) {
            String lSessionId = getSessionId(aMessage);
            //1 check if already exist
            if (mRoutingTable.containsSessionId(lSessionId)) {
               //second call, update ssrc
               mRoutingTable.UpdateRoutingElement(lSessionId,lSsrc);

            } else {
               //first call 
               mRoutingTable.addRoutingElement(lSessionId,lSsrc); 
            }
         }
         mRoutingTable.updateSourceAddress(lSsrc, aMessage.getSocketAddress(),isRtcp(aMessage));

         SocketAddress lDestAddress = mRoutingTable.getDestAddress(lSsrc,isRtcp(aMessage));
         //does not forward session id msg
         if (!isSessionIdPresent(aMessage)) {
            // ok forwarding
            if (mLog.isInfoEnabled()) mLog.info("forwarding ["+aMessage.getLength()+"] bytes  from ["+aMessage.getSocketAddress()+"] to ["+lDestAddress+"]");
            aMessage.setSocketAddress(lDestAddress);
            mSocket.send(aMessage);
         }
      } catch (IOException e) {
         mLog.error("cannot forward ["+aMessage+"]", e);
      } catch (Exception e) {
         mLog.warn("unknown destination for message from ["+aMessage.getSocketAddress()+"] discarding",e);

      }      
   }
   public InetSocketAddress getInetSocketAddress() {
      return (InetSocketAddress) mSocket.getLocalSocketAddress();
   }
 
   private long getSsrc(DatagramPacket aMessage) {
//      The RTP header has the following format:
//
//         0                   1                   2                   3
//         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |V=2|P|X|  CC   |M|     PT      |       sequence number         |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |                           timestamp                           |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//        |           synchronization source (SSRC) identifier            |
//        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//        |            contributing source (CSRC) identifiers             |
//        |                             ....                              |
//        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      if (isRtcp(aMessage)) {
         //rtcp packet
         return  b2UB(aMessage.getData()[7]) 
               + (b2UB(aMessage.getData()[6]) << 8) 
               + (b2UB(aMessage.getData()[5]) << 16) 
               + (b2UB(aMessage.getData()[4]) << 24);
      } else {
         //rtp packet 
         return  b2UB(aMessage.getData()[11])
               + (b2UB(aMessage.getData()[10]) << 8) 
               + (b2UB(aMessage.getData()[9]) << 16) 
               + (b2UB(aMessage.getData()[8]) << 24);
      }
      
   }
   private boolean isSessionIdPresent(DatagramPacket aMessage) {
//    APP: Application-Defined RTCP Packet
//
//            0                   1                   2                   3
//            0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//           |V=2|P| subtype |   PT=APP=204  |             length            |
//           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//           |                           SSRC/CSRC                           |
//           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//           |                          name (ASCII)                         |
//           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//           |                   application-dependent data                ...
//           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      if (b2UB(aMessage.getData()[1]) != 204) return false;
      String aName =   String.valueOf((char)b2UB(aMessage.getData()[8])) 
                     + String.valueOf((char)b2UB(aMessage.getData()[9]))
                     + String.valueOf((char)b2UB(aMessage.getData()[10]))
                     + String.valueOf((char)b2UB(aMessage.getData()[11]));
      return SESSIONID_NAME.equalsIgnoreCase(aName);
   }

   private String getSessionId(DatagramPacket aMessage) throws P2pProxyException{
//      APP: Application-Defined RTCP Packet
//
//      0                   1                   2                   3
//      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//     |V=2|P| subtype |   PT=APP=204  |             length            |
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//     |                           SSRC/CSRC                           |
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//     |                          name (ASCII)                         |
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//     |                   application-dependent data                ...
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      
      if (isSessionIdPresent(aMessage) == false) {
         throw new  P2pProxyException("Cannot find SessionId");
      }
      long lPacketLenght = 1 + b2UB(aMessage.getData()[3]) + (b2UB(aMessage.getData()[2]) << 8) ;
      StringBuffer lSessionId = new StringBuffer();
      for (int i = 12 ; i< lPacketLenght; i++) {
         lSessionId.append(b2UB(aMessage.getData()[i]));
      }
      return lSessionId.toString();
      
   }
   private boolean isRtcp(DatagramPacket aMessage) {
      return b2UB(aMessage.getData()[1]) >= 200 ;
   }
   public static int b2UB(byte b) {
      return b >= 0 ? b : 256 + b;
   }
   
   //stats
   public int getRoutingtableSize() {
      return mRoutingTable.getSize();
   }
   
}
