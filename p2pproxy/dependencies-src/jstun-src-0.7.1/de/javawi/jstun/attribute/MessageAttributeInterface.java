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

package de.javawi.jstun.attribute;

public interface MessageAttributeInterface {
	public enum MessageAttributeType { MappedAddress
										, ResponseAddress
										, ChangeRequest
										, SourceAddress
										, ChangedAddress
										, Username
										, Password
										, MessageIntegrity
										, ErrorCode
										, UnknownAttribute
										, ReflectedFrom
										, Dummy
										, ChannelNumber
										, LifeTime
										, Bandwidth
										, PeerAddress
										, Data
										, RelayAddress
										, RequestedProps
										, RequestedTransport
										, ReservationToken};
	final static int MAPPEDADDRESS = 0x0001;
	final static int RESPONSEADDRESS = 0x0002;
	final static int CHANGEREQUEST = 0x0003;
	final static int SOURCEADDRESS = 0x0004;
	final static int CHANGEDADDRESS = 0x0005;
	final static int USERNAME = 0x0006;
	final static int PASSWORD = 0x0007;
	final static int MESSAGEINTEGRITY = 0x0008;
	final static int ERRORCODE = 0x0009;
	final static int UNKNOWNATTRIBUTE = 0x000a;
	final static int REFLECTEDFROM = 0x000b;
	final static int DUMMY = 0x0000;
// turn attributes
//    0x000C: CHANNEL-NUMBER
//    0x000D: LIFETIME
//    0x0010: BANDWIDTH
//    0x0012: PEER-ADDRESS
//    0x0013: DATA
//    0x0016: RELAY-ADDRESS
//    0x0018: REQUESTED-PROPS
//    0x0019: REQUESTED-TRANSPORT
//    0x0022: RESERVATION-TOKEN
	final static int CHANNELNUMBER = 0x000C;
	final static int LIFETIME = 0x000D;
	final static int BANDWIDTH = 0x0010;
	final static int PEERADDRESS = 0x0012;
	final static int DATA = 0x0013;
	final static int RELAYADDRESS = 0x0016;
	final static int REQUESTEDPROPS = 0x0018;
	final static int REQUESTEDTRANSPORT = 0x0019;
	final static int RESERVATIONTOKEN = 0x0022;
	
}