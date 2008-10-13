/*
 * This file is part of JSTUN. 
 * 
 * Copyright (c) 2005 Thomas King <king@t-king.de> - All rights
 * reserved.
 * 
 * This software is licensed under either the GNU Public License (GPL),
 * or the Apache 2.0 license. Copies of both license agreements are
 * included in this distribution.
 */

package org.linphone.p2pproxy.core.stun;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Vector;
import java.util.logging.FileHandler;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

import de.javawi.jstun.attribute.ChangeRequest;
import de.javawi.jstun.attribute.ChangedAddress;
import de.javawi.jstun.attribute.MappedAddress;
import de.javawi.jstun.attribute.MessageAttributeException;
import de.javawi.jstun.attribute.MessageAttributeParsingException;
import de.javawi.jstun.attribute.ResponseAddress;
import de.javawi.jstun.attribute.SourceAddress;
import de.javawi.jstun.attribute.UnknownAttribute;
import de.javawi.jstun.attribute.UnknownMessageAttributeException;
import de.javawi.jstun.attribute.MessageAttributeInterface.MessageAttributeType;
import de.javawi.jstun.header.MessageHeader;
import de.javawi.jstun.header.MessageHeaderParsingException;
import de.javawi.jstun.header.MessageHeaderInterface.MessageHeaderType;
import de.javawi.jstun.util.Address;
import de.javawi.jstun.util.UtilityException;

/*
 * This class implements a STUN server as described in RFC 3489.
 * The server requires a machine that is dual-homed to be functional. 
 */
public class StunServer {
	private static Logger logger = Logger.getLogger("org.linphone.p2pproxy.core.stun.StunServer");
	Vector<DatagramSocket> sockets;
	
	public StunServer(int primaryPort, InetAddress primary, int secondaryPort) throws SocketException {
		sockets = new Vector<DatagramSocket>();
		sockets.add(new DatagramSocket(primaryPort, primary));
		sockets.add(new DatagramSocket(secondaryPort, primary));
	}
	
	public void start() throws SocketException {
		for (DatagramSocket socket : sockets) {
			socket.setReceiveBufferSize(2000);
			StunServerReceiverThread ssrt = new StunServerReceiverThread(socket);
			ssrt.start();
		}
	}
	
	/*
	 * Inner class to handle incoming packets and react accordingly.
	 * I decided not to start a thread for every received Binding Request, because the time
	 * required to receive a Binding Request, parse it, generate a Binding Response and send
	 * it varies only between 2 and 4 milliseconds. This amount of time is small enough so
	 * that no extra thread is needed for incoming Binding Request. 
	 */
	class StunServerReceiverThread extends Thread {
		private DatagramSocket receiverSocket;
		private DatagramSocket changedPort;
		
		StunServerReceiverThread(DatagramSocket datagramSocket) {
			this.receiverSocket = datagramSocket;
			for (DatagramSocket socket : sockets) {
				if ((socket.getLocalPort() != receiverSocket.getLocalPort()) &&
					(socket.getLocalAddress().equals(receiverSocket.getLocalAddress())))
					changedPort = socket;
			}
		}
		
		public void run() {
			while (true) {
				try {
					DatagramPacket receive = new DatagramPacket(new byte[200], 200);
					receiverSocket.receive(receive);
					logger.finest(receiverSocket.getLocalAddress().getHostAddress() + ":" + receiverSocket.getLocalPort() + " datagram received from " + receive.getAddress().getHostAddress() + ":" + receive.getPort());
					MessageHeader receiveMH = MessageHeader.parseHeader(receive.getData()); 
					try {
						receiveMH.parseAttributes(receive.getData());
						if (receiveMH.getType() == MessageHeaderType.BindingRequest) {
							logger.config(receiverSocket.getLocalAddress().getHostAddress() + ":" + receiverSocket.getLocalPort() + " Binding Request received from " + receive.getAddress().getHostAddress() + ":" + receive.getPort());
							ChangeRequest cr = (ChangeRequest) receiveMH.getMessageAttribute(MessageAttributeType.ChangeRequest);
							if (cr == null) throw new MessageAttributeException("Message attribute change request is not set.");
							ResponseAddress ra = (ResponseAddress) receiveMH.getMessageAttribute(MessageAttributeType.ResponseAddress);
						
							MessageHeader sendMH = new MessageHeader(MessageHeaderType.BindingResponse);
							sendMH.setTransactionID(receiveMH.getTransactionID());
						
							// Mapped address attribute
							MappedAddress ma = new MappedAddress();
							ma.setAddress(new Address(receive.getAddress().getAddress()));
							ma.setPort(receive.getPort());
							sendMH.addMessageAttribute(ma);
							if (cr.isChangePort()) {
								logger.finer("Change port received in Change Request attribute");
								// Source address attribute
								SourceAddress sa = new SourceAddress();
								sa.setAddress(new Address(changedPort.getLocalAddress().getAddress()));
								sa.setPort(changedPort.getLocalPort());
								sendMH.addMessageAttribute(sa);
								byte[] data = sendMH.getBytes();
								DatagramPacket send = new DatagramPacket(data, data.length);
								if (ra != null) {
									send.setPort(ra.getPort());
									send.setAddress(ra.getAddress().getInetAddress());
								} else {
									send.setPort(receive.getPort());
									send.setAddress(receive.getAddress());
								}
								changedPort.send(send);
								logger.config(changedPort.getLocalAddress().getHostAddress() + ":" + changedPort.getLocalPort() + " send Binding Response to " + send.getAddress().getHostAddress() + ":" + send.getPort());
							} else if ((!cr.isChangePort()) && (!cr.isChangeIP())) {
								logger.finer("Nothing received in Change Request attribute");
								// Source address attribute
								SourceAddress sa = new SourceAddress();
								sa.setAddress(new Address(receiverSocket.getLocalAddress().getAddress()));
								sa.setPort(receiverSocket.getLocalPort());
								sendMH.addMessageAttribute(sa);
								byte[] data = sendMH.getBytes();
								DatagramPacket send = new DatagramPacket(data, data.length);
								if (ra != null) {
									send.setPort(ra.getPort());
									send.setAddress(ra.getAddress().getInetAddress());
								} else {
									send.setPort(receive.getPort());
									send.setAddress(receive.getAddress());
								}
								receiverSocket.send(send);
								logger.config(receiverSocket.getLocalAddress().getHostAddress() + ":" + receiverSocket.getLocalPort() + " send Binding Response to " + send.getAddress().getHostAddress() + ":" + send.getPort());
							} else {
							   logger.warning("cannot handle cr ["+cr+"]");
							}
						}
					} catch (UnknownMessageAttributeException umae) {
						umae.printStackTrace();
						// Generate Binding error response
						MessageHeader sendMH = new MessageHeader(MessageHeaderType.BindingErrorResponse);
						sendMH.setTransactionID(receiveMH.getTransactionID());
						
						// Unknown attributes
						UnknownAttribute ua = new UnknownAttribute();
						ua.addAttribute(umae.getType());
						sendMH.addMessageAttribute(ua);
						
						byte[] data = sendMH.getBytes();
						DatagramPacket send = new DatagramPacket(data, data.length);
						send.setPort(receive.getPort());
						send.setAddress(receive.getAddress());
						receiverSocket.send(send);
						logger.config(" send Binding Error Response to " + send.getAddress().getHostAddress() + ":" + send.getPort());
					}	
				} catch (IOException ioe) {
					ioe.printStackTrace();
				} catch (MessageAttributeParsingException mape) {
					mape.printStackTrace();
				} catch (MessageAttributeException mae) {
					mae.printStackTrace();
				} catch (MessageHeaderParsingException mhpe) {
					mhpe.printStackTrace();
				} catch (UtilityException ue) {
					ue.printStackTrace();
				} catch (ArrayIndexOutOfBoundsException aioobe) {
					aioobe.printStackTrace();
				}
			}
		}
	}
	

}