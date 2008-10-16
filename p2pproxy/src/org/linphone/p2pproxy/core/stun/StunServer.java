/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

StunServer.java - .

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


import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;
import org.apache.log4j.Logger;
import org.linphone.p2pproxy.api.P2pProxyException;
import org.linphone.p2pproxy.core.GenericUdpSession;
import org.linphone.p2pproxy.core.ServiceProvider;

import de.javawi.jstun.attribute.ChangeRequest;
import de.javawi.jstun.attribute.ErrorCode;
import de.javawi.jstun.attribute.MappedAddress;
import de.javawi.jstun.attribute.MessageAttributeException;
import de.javawi.jstun.attribute.ResponseAddress;
import de.javawi.jstun.attribute.SourceAddress;
import de.javawi.jstun.attribute.UnknownAttribute;
import de.javawi.jstun.attribute.UnknownMessageAttributeException;
import de.javawi.jstun.attribute.MessageAttributeInterface.MessageAttributeType;
import de.javawi.jstun.header.MessageHeader;
import de.javawi.jstun.header.MessageHeaderParsingException;
import de.javawi.jstun.header.MessageHeaderInterface.MessageHeaderType;
import de.javawi.jstun.util.Address;


/*
 * This class implements a STUN server as described in RFC 3489.
 * neither change port nor change address are implemented 
 */
public class StunServer implements GenericUdpSession.MessageHandler {
	private static Logger mLog = Logger.getLogger(StunServer.class);
	private final DatagramSocket mSocket;
	
	public StunServer(DatagramSocket mListeningSocket) throws SocketException {
		mSocket = mListeningSocket;
	}

	public void onMessage(DatagramPacket lMessage) {
		// derivated from JSTUN (Thomas King) 
		MessageHeader receiveMH = null;
		try {
			receiveMH = MessageHeader.parseHeader(lMessage.getData());
		} catch (MessageHeaderParsingException e1) {
			if (mLog.isInfoEnabled()) mLog.info("not a stun message");
			return;
		}

		try {
			receiveMH.parseAttributes(lMessage.getData());
			if (receiveMH.getType() == MessageHeaderType.BindingRequest) {
				ChangeRequest cr = (ChangeRequest) receiveMH.getMessageAttribute(MessageAttributeType.ChangeRequest);
				if (cr == null) throw new MessageAttributeException("Message attribute change request is not set.");
				ResponseAddress ra = (ResponseAddress) receiveMH.getMessageAttribute(MessageAttributeType.ResponseAddress);

				MessageHeader sendMH = new MessageHeader(MessageHeaderType.BindingResponse);
				sendMH.setTransactionID(receiveMH.getTransactionID());

				// Mapped address attribute
				MappedAddress ma = new MappedAddress();
				ma.setAddress(new Address(lMessage.getAddress().getAddress()));
				ma.setPort(lMessage.getPort());
				sendMH.addMessageAttribute(ma);
				if ((!cr.isChangePort()) && (!cr.isChangeIP())) {
					if (mLog.isInfoEnabled()) mLog.info("Nothing received in Change Request attribute");
					// Source address attribute
					SourceAddress sa = new SourceAddress();
					sa.setAddress(new Address(mSocket.getLocalAddress().getAddress()));
					sa.setPort(mSocket.getLocalPort());
					sendMH.addMessageAttribute(sa);
					byte[] data = sendMH.getBytes();
					DatagramPacket send = new DatagramPacket(data, data.length);
					if (ra != null) {
						send.setPort(ra.getPort());
						send.setAddress(ra.getAddress().getInetAddress());
					} else {
						send.setPort(lMessage.getPort());
						send.setAddress(lMessage.getAddress());
					}
					mSocket.send(send);
					if (mLog.isInfoEnabled()) mLog.info(mSocket.getLocalAddress().getHostAddress() + ":" + mSocket.getLocalPort() + " send Binding Response to " + send.getAddress().getHostAddress() + ":" + send.getPort());
				} else {
//					Generate Binding error response
					sendMH = new MessageHeader(MessageHeaderType.BindingErrorResponse);
					sendMH.setTransactionID(receiveMH.getTransactionID());
					ErrorCode lErrorCode = new ErrorCode();
					lErrorCode.setResponseCode(400); //bad request
					sendMH.addMessageAttribute(lErrorCode);
					byte[] data = sendMH.getBytes();
					DatagramPacket send = new DatagramPacket(data, data.length);
					send.setPort(lMessage.getPort());
					send.setAddress(lMessage.getAddress());
					mSocket.send(send);
					if (mLog.isInfoEnabled()) mLog.info("cannot handle cr ["+cr+"] attibute");
				}
			}
		} catch ( Exception e) {

			try {
				// Generate Binding error response

				MessageHeader sendMH = new MessageHeader(MessageHeaderType.BindingErrorResponse);
				sendMH.setTransactionID(receiveMH.getTransactionID());

				if (e instanceof UnknownMessageAttributeException) {
					// Unknown attributes
					UnknownAttribute ua = new UnknownAttribute();
					ua.addAttribute(((UnknownMessageAttributeException) e).getType());
					sendMH.addMessageAttribute(ua);
				} else {
					ErrorCode lErrorCode = new ErrorCode();
					lErrorCode.setResponseCode(500);
					sendMH.addMessageAttribute(lErrorCode);	
				}
				byte[] data = sendMH.getBytes();
				DatagramPacket send = new DatagramPacket(data, data.length);
				send.setPort(lMessage.getPort());
				send.setAddress(lMessage.getAddress());
				mSocket.send(send);
				mLog.error(" send Binding Error Response to " + send.getAddress().getHostAddress() + ":" + send.getPort(),e);
			} catch(Exception e1) {
				mLog.error("cannot handle error", e1);
			}
		}	

	}


}