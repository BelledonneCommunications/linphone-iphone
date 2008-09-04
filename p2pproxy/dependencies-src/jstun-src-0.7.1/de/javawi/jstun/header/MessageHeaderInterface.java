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

package de.javawi.jstun.header;

public interface MessageHeaderInterface {
	public enum MessageHeaderType { BindingRequest
	                                 , BindingResponse
	                                 , BindingErrorResponse
	                                 , SharedSecretRequest
	                                 , SharedSecretResponse
	                                 , SharedSecretErrorResponse
	                                 , AllocateRequest
	                                 , AllocateResponse
	                                 , AllocateErrorResponse
	                                 , RefreshRequest
	                                 , RefreshResponse
	                                 , RefreshErrorResponse
	                                 , ChannelBindRequest
	                                 , ChannelBindResponse
	                                 , ChannelBindErrorResponse
	                                 ,SendIndication
	                                 ,DataIndication};
	final static int BINDINGREQUEST = 0x0001;
	final static int BINDINGRESPONSE = 0x0101;
	final static int BINDINGERRORRESPONSE = 0x0111;
	final static int SHAREDSECRETREQUEST = 0x0002;
	final static int SHAREDSECRETRESPONSE = 0x0102;
	final static int SHAREDSECRETERRORRESPONSE = 0x0112;

//	  TURN defines ten new Message Types:
//Request/Response Transactions
//0x003  :  Allocate
//0x004  :  Refresh
//0x009  :  ChannelBind
//0x006  :  Send
//0x007  :  Data


	final static int ALLOCATEREQUEST = 0x0003;
	final static int ALLOCATERESPONSE = 0x0103;
	final static int ALLOCATEERRORRESPONSE = 0x0113;
	final static int REFRESHREQUEST = 0x0004;
	final static int REFRESHRESPONSE = 0x0104;
	final static int REFRESHERRORRESPONSE = 0x0114;
    final static int CHANNELBINDREQUEST = 0x0009;
    final static int CHANNELBINDRESPONSE = 0x0109;
    final static int CHANNELBINDERRORRESPONSE = 0x0119;
    final static int SENDINDICATION = 0x0006;
    final static int DATAINDICATION = 0x0007;
	
	
}