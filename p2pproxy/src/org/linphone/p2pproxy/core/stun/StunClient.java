/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

StunClient.java - .

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
package org.linphone.p2pproxy.core.stun;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.URI;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.JxtaNetworkManager;
import org.linphone.p2pproxy.core.P2pProxyAdvertisementNotFoundException;
import org.linphone.p2pproxy.core.sipproxy.NetworkResourceAdvertisement;
import org.linphone.p2pproxy.test.StunServerTester;

import de.javawi.jstun.attribute.ChangeRequest;
import de.javawi.jstun.attribute.ChangedAddress;
import de.javawi.jstun.attribute.ErrorCode;
import de.javawi.jstun.attribute.MappedAddress;
import de.javawi.jstun.attribute.MessageAttribute;
import de.javawi.jstun.attribute.MessageAttributeParsingException;
import de.javawi.jstun.header.MessageHeader;
import de.javawi.jstun.header.MessageHeaderParsingException;
import de.javawi.jstun.util.UtilityException;

public class StunClient {
	private static Logger mLog = Logger.getLogger(StunClient.class);
	private List<InetSocketAddress> mStunServerList;
   JxtaNetworkManager mJxtaNetworkManager;
   private DiscoveryInfo mDiscoveryInfo;
   private int SO_TIME_OUT = 300;
   
   StunClient(List<InetSocketAddress> aStunServerList) {
      mStunServerList = aStunServerList;
   }
   StunClient(JxtaNetworkManager aJxtaNetworkManager) throws P2pProxyException {
      //need to acquire stun server address()
      mJxtaNetworkManager = aJxtaNetworkManager;
      try {
         mStunServerList = acquireStunServerAddress();
      } catch (Exception e) {
         throw new P2pProxyException(e);
      }
   }   
   private List<InetSocketAddress> acquireStunServerAddress() throws P2pProxyAdvertisementNotFoundException, InterruptedException, IOException {
      List<NetworkResourceAdvertisement> lStunServerAdv = (List<NetworkResourceAdvertisement>) mJxtaNetworkManager.getAdvertisementList(null, StunServer.ADV_NAME, true);
      List<InetSocketAddress> lSocketAddressList = new ArrayList<InetSocketAddress>(lStunServerAdv.size());
      for (NetworkResourceAdvertisement lNetworkResourceAdvertisement: lStunServerAdv) {
         URI lServerUri = URI.create(lNetworkResourceAdvertisement.getAddress());
         lSocketAddressList.add(new InetSocketAddress(lServerUri.getHost(),lServerUri.getPort()));
      }
      return lSocketAddressList;
   }
   
   public AddressInfo computeAddressInfo(DatagramSocket lLocalSocket) throws P2pProxyException {
	   //1 bind request
	   try {
		   //1 bind request 
		   bindRequest(lLocalSocket,lLocalSocket, mStunServerList.get(0));
		   //open new socket
	} catch (Exception e) {
		throw new P2pProxyException(e);
	}
	   return null;
   }
	private void bindRequest(DatagramSocket aLocalSocket, DatagramSocket aResponseSocket,InetSocketAddress aStunAddress) throws UtilityException, SocketException, UnknownHostException, IOException, MessageAttributeParsingException, MessageHeaderParsingException, P2pProxyException {
		int timeSinceFirstTransmission = 0;
		int lSoTimeOut = SO_TIME_OUT;
		while (true) {
			try {
				aLocalSocket.setReuseAddress(true);
				aLocalSocket.connect(aStunAddress);
				aLocalSocket.setSoTimeout(lSoTimeOut);
				
				MessageHeader sendMH = new MessageHeader(MessageHeader.MessageHeaderType.BindingRequest);
				sendMH.generateTransactionID();
				
				ChangeRequest changeRequest = new ChangeRequest();
				sendMH.addMessageAttribute(changeRequest);
				
				byte[] data = sendMH.getBytes();
				DatagramPacket send = new DatagramPacket(data, data.length);
				aLocalSocket.send(send);
							
				MessageHeader receiveMH = new MessageHeader();
				while (!(receiveMH.equalTransactionID(sendMH))) {
					DatagramPacket receive = new DatagramPacket(new byte[200], 200);
					aResponseSocket.receive(receive);
					receiveMH = MessageHeader.parseHeader(receive.getData());
					receiveMH.parseAttributes(receive.getData());
				}
				
				MappedAddress lMappedAddress = (MappedAddress) receiveMH.getMessageAttribute(MessageAttribute.MessageAttributeType.MappedAddress);
				ErrorCode ec = (ErrorCode) receiveMH.getMessageAttribute(MessageAttribute.MessageAttributeType.ErrorCode);
				if (ec != null) {
					mDiscoveryInfo.setError(ec.getResponseCode(), ec.getReason());
					throw new P2pProxyException("Message header contains an Errorcode message attribute. ["+ec+"]");
				}
				if ((lMappedAddress == null)) {
					throw new P2pProxyException("Response does not contain a Mapped Address message attribute.");
					
				} else {
					if (aLocalSocket.getLocalSocketAddress().equals(aResponseSocket.getLocalSocketAddress())) {
						mDiscoveryInfo.setPublicSocketAddress(new InetSocketAddress(lMappedAddress.getAddress().getInetAddress(),lMappedAddress.getPort()));
					} else {
						mDiscoveryInfo.setFullCone();
					}
					}
					return;
				
			} catch (SocketTimeoutException ste) {
				if (timeSinceFirstTransmission < 7900) {
					if (mLog.isInfoEnabled()) mLog.info("Socket timeout while receiving the response.");
					timeSinceFirstTransmission += lSoTimeOut;
					int timeoutAddValue = (timeSinceFirstTransmission * 2);
					if (timeoutAddValue > 1600) timeoutAddValue = 1600;
					lSoTimeOut = timeoutAddValue;
				} else {
					// node is not capable of udp communication
					if (mLog.isInfoEnabled()) mLog.info("Socket timeout while receiving the response. Maximum retry limit exceed. Give up.");
					if (aLocalSocket.getLocalSocketAddress().equals(aResponseSocket.getLocalSocketAddress())) {
						mDiscoveryInfo.setBlockedUDP();
					} else {
						mDiscoveryInfo.setSymmetric();
					}
					if (mLog.isInfoEnabled()) mLog.info("Node is not capable of UDP communication.");
					return ;
				}
			} 
		}
	}
}
