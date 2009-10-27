 /*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc3550) stack.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */


#ifndef __STUN_H__
#define __STUN_H__

#include <stdio.h>
#include <time.h>
#include <ortp/port.h>
#include <ortp/stun_udp.h>

#ifdef __APPLE__
   #include "TargetConditionals.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* if you change this version, change in makefile too  */
#define STUN_VERSION "0.99"

#define STUN_MAX_STRING 256
#define STUN_MAX_UNKNOWN_ATTRIBUTES 8
#define STUN_MAX_MESSAGE_SIZE 2048

#define STUN_PORT 3478

/* define some basic types */
#if 0
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#if	defined(WIN32) || defined(_WIN32_WCE)
typedef unsigned __int64 uint64_t;
#else
typedef unsigned long long uint64_t;
#endif
#endif
typedef struct { unsigned char octet[12]; }  UInt96;

/* define a structure to hold a stun address  */
#define  IPv4Family  0x01
#define  IPv6Family  0x02

/* define  flags  */
#define ChangeIpFlag    0x04
#define ChangePortFlag  0x02

/* define  stun attribute */
#define SA_MAPPEDADDRESS     0x0001
#define SA_RESPONSEADDRESS   0x0002 /** deprecated **/
#define SA_CHANGEREQUEST     0x0003 /** deprecated **/
#define SA_SOURCEADDRESS     0x0004 /** deprecated **/
#define SA_CHANGEDADDRESS    0x0005 /** deprecated **/
#define SA_USERNAME          0x0006
#define SA_PASSWORD          0x0007  /** deprecated **/
#define SA_MESSAGEINTEGRITY  0x0008
#define SA_ERRORCODE         0x0009
#define SA_UNKNOWNATTRIBUTE  0x000A
#define SA_REFLECTEDFROM     0x000B /** deprecated **/
#define SA_REALM             0x0014
#define SA_NONCE             0x0015
#define SA_XORMAPPEDADDRESS  0x0020

#define SA_XORMAPPEDADDRESS2 0x8020 /* Non standard extention */
#define SA_XORONLY           0x0021 /* deprecated */
#define SA_SECONDARYADDRESS  0x0050 /* Non standard extention */

#define SA_SOFTWARE          0x8022
#define SA_ALTERNATESERVER   0x8023
#define SA_FINGERPRINT       0x8028

/* define turn attribute */
#define TA_CHANNELNUMBER       0x000C
#define TA_LIFETIME            0x000D
#define TA_DEPRECATEDBANDWIDTH 0x0010
#define TA_XORPEERADDRESS      0x0012
#define TA_DATA                0x0013
#define TA_XORRELAYEDADDRESS   0x0016
#define TA_EVENPORT            0x0018
#define TA_REQUESTEDTRANSPORT  0x0019
#define TA_DONTFRAGMENT        0x001A
#define TA_DEPRECATEDTIMERVAL  0x0021
#define TA_RESERVATIONTOKEN    0x0022

#define ICEA_PRIORITY          0x0024
#define ICEA_USECANDIDATE      0x0025
#define ICEA_ICECONTROLLED     0x8029
#define ICEA_ICECONTROLLING    0x802a

#define STUN_REQUEST           0x0000
#define STUN_INDICATION        0x0010
#define STUN_SUCCESS_RESP      0x0100
#define STUN_ERR_RESP          0x0110

#define STUN_IS_REQUEST(msg_type)       (((msg_type) & 0x0110) == 0x0000)
#define STUN_IS_INDICATION(msg_type)    (((msg_type) & 0x0110) == 0x0010)
#define STUN_IS_SUCCESS_RESP(msg_type)  (((msg_type) & 0x0110) == 0x0100)
#define STUN_IS_ERR_RESP(msg_type)      (((msg_type) & 0x0110) == 0x0110)

/* define types for a stun message */
#define STUN_METHOD_BINDING           0x0001
#define TURN_MEDHOD_ALLOCATE          0x0003 //(only request/response semantics defined)
#define TURN_METHOD_REFRESH           0x0004 //(only request/response semantics defined)
#define TURN_METHOD_CREATEPERMISSION  0x0008 //(only request/response semantics defined
#define TURN_METHOD_CHANNELBIND       0x0009 //(only request/response semantics defined)

//#define BindResponseMsg               0x0101
//#define BindErrorResponseMsg          0x0111
#define SharedSecretRequestMsg        0x0002
#define SharedSecretResponseMsg       0x0102
#define SharedSecretErrorResponseMsg  0x0112

#define TURN_INDICATION_SEND          0x0006 //(only indication semantics defined)
#define TURN_INDICATION_DATA          0x0007 //(only indication semantics defined)

typedef struct 
{
      uint16_t msgType;
      uint16_t msgLength;
      uint32_t magic_cookie;
      UInt96 tr_id;
} StunMsgHdr;


typedef struct
{
      uint16_t type;
      uint16_t length;
} StunAtrHdr;

typedef struct
{
      uint16_t port;
      uint32_t addr;
} StunAddress4;

typedef struct
{
      uint8_t pad;
      uint8_t family;
      StunAddress4 ipv4;
} StunAtrAddress4;

typedef struct
{
      uint32_t value;
} StunAtrChangeRequest;

typedef struct
{
      uint16_t pad; /* all 0 */
      uint8_t errorClass;
      uint8_t number;
      char reason[STUN_MAX_STRING];
      uint16_t sizeReason;
} StunAtrError;

typedef struct
{
      uint16_t attrType[STUN_MAX_UNKNOWN_ATTRIBUTES];
      uint16_t numAttributes;
} StunAtrUnknown;

typedef struct
{
      uint16_t channelNumber;
      uint16_t rffu; /* Reserved For Future Use */
} TurnAtrChannelNumber;

typedef struct
{
      uint32_t lifetime;
} TurnAtrLifetime;

typedef struct
{
      char value[1500];      
      uint16_t sizeValue;
} TurnAtrData;

typedef struct
{
      uint8_t proto;
      uint8_t pad1;
      uint8_t pad2;
      uint8_t pad3;
} TurnAtrRequestedTransport;

typedef struct
{
      uint64_t value;
} TurnAtrReservationToken;

typedef struct
{
      uint32_t fingerprint;
} StunAtrFingerprint;


typedef struct
{
      char value[STUN_MAX_STRING];      
      uint16_t sizeValue;
} StunAtrString;

typedef struct
{
      uint32_t priority;
} IceAtrPriority;

typedef struct
{
      uint64_t value;
} IceAtrIceControll;

typedef struct
{
      char hash[20];
} StunAtrIntegrity;

typedef enum 
{
   HmacUnkown=0,
   HmacOK,
   HmacBadUserName,
   HmacUnkownUserName,
   HmacFailed
} StunHmacStatus;


typedef struct
{
      uint16_t attrType[STUN_MAX_UNKNOWN_ATTRIBUTES];
      uint16_t numAttributes;
} TurnAtrUnknown;

typedef struct
{
      StunMsgHdr msgHdr;
	
      bool_t hasMappedAddress;
      StunAtrAddress4  mappedAddress;
	
      bool_t hasResponseAddress;
      StunAtrAddress4  responseAddress;
	
      bool_t hasChangeRequest;
      StunAtrChangeRequest changeRequest;
	
      bool_t hasSourceAddress;
      StunAtrAddress4 sourceAddress;
	
      bool_t hasChangedAddress;
      StunAtrAddress4 changedAddress;
	
      bool_t hasUsername;
      StunAtrString username;
	
      bool_t hasPassword;
      StunAtrString password;
	
      bool_t hasMessageIntegrity;
      StunAtrIntegrity messageIntegrity;
	
      bool_t hasErrorCode;
      StunAtrError errorCode;
	
      bool_t hasUnknownAttributes;
      StunAtrUnknown unknownAttributes;
	
      bool_t hasReflectedFrom;
      StunAtrAddress4 reflectedFrom;

      bool_t hasRealm;
      StunAtrString realmName;

      bool_t hasNonce;
      StunAtrString nonceName;

      bool_t hasXorMappedAddress;
      StunAtrAddress4  xorMappedAddress;
	
      bool_t hasSoftware;
      StunAtrString softwareName;

      bool_t hasXorPeerAddress;
      StunAtrAddress4 xorPeerAddress;

      bool_t hasXorRelayedAddress;
      StunAtrAddress4 xorRelayedAddress;

      bool_t hasFingerprint;
      StunAtrFingerprint fingerprint;

      /* Turn elements */
      bool_t hasChannelNumberAttributes;
      TurnAtrChannelNumber channelNumberAttributes;

      bool_t hasLifetimeAttributes;
      TurnAtrLifetime lifetimeAttributes;

      bool_t hasData;
      TurnAtrData data;

      bool_t hasRequestedTransport;
      TurnAtrRequestedTransport requestedTransport;

      bool_t hasDontFragment;

      bool_t hasReservationToken;
      TurnAtrReservationToken reservationToken;

      bool_t hasPriority;
      IceAtrPriority priority;

      bool_t hasUseCandidate;

      bool_t hasIceControlled;
      IceAtrIceControll iceControlled;

      bool_t hasIceControlling;
      IceAtrIceControll iceControlling;
} StunMessage; 


/* Define enum with different types of NAT */
typedef enum 
{
   StunTypeUnknown=0,
   StunTypeOpen,
   StunTypeConeNat,
   StunTypeRestrictedNat,
   StunTypePortRestrictedNat,
   StunTypeSymNat,
   StunTypeSymFirewall,
   StunTypeBlocked,
   StunTypeFailure
} NatType;


#define MAX_MEDIA_RELAYS 500
#define MAX_RTP_MSG_SIZE 1500
#define MEDIA_RELAY_TIMEOUT 3*60

typedef struct 
{
      int relayPort;       /* media relay port */
      int fd;              /* media relay file descriptor */
      StunAddress4 destination; /* NAT IP:port */
      time_t expireTime;      /* if no activity after time, close the socket */
} StunMediaRelay;

typedef struct
{
      StunAddress4 myAddr;
      StunAddress4 altAddr;
      Socket myFd;
      Socket altPortFd;
      Socket altIpFd;
      Socket altIpPortFd;
      bool_t relay; /* true if media relaying is to be done */
      StunMediaRelay relays[MAX_MEDIA_RELAYS];
} StunServerInfo;

void
stunCalculateIntegrity_longterm(char* hmac, const char* input, int length,
                     const char *username, const char *realm, const char *password);
void
stunCalculateIntegrity_shortterm(char* hmac, const char* input, int length, const char* key);
uint32_t
stunCalculateFingerprint(const char* input, int length);

bool_t
stunParseMessage( char* buf, 
                  unsigned int bufLen, 
                  StunMessage *message);

void
stunBuildReqSimple( StunMessage* msg,
                    const StunAtrString *username,
                    bool_t changePort, bool_t changeIp, unsigned int id );

unsigned int
stunEncodeMessage( const StunMessage *message, 
                   char* buf, 
                   unsigned int bufLen, 
                   const StunAtrString *password);

void
stunCreateUserName(const StunAddress4 *addr, StunAtrString* username);

void 
stunGetUserNameAndPassword(  const StunAddress4 *dest, 
                             StunAtrString* username,
                             StunAtrString* password);

void
stunCreatePassword(const StunAtrString *username, StunAtrString* password);

int 
stunRand(void);

uint64_t
stunGetSystemTimeSecs(void);

/* find the IP address of a the specified stun server - return false is fails parse  */
bool_t  
stunParseServerName( const char* serverName, StunAddress4 *stunServerAddr);

bool_t 
stunParseHostName( const char* peerName,
                   uint32_t *ip,
                   uint16_t *portVal,
                   uint16_t defaultPort );

/* return true if all is OK 
   Create a media relay and do the STERN thing if startMediaPort is non-zero */
bool_t
stunInitServer(StunServerInfo *info, 
               const StunAddress4 *myAddr, 
               const StunAddress4 *altAddr,
               int startMediaPort);

void
stunStopServer(StunServerInfo *info);

/* returns number of address found - take array or addres */
int 
stunFindLocalInterfaces(uint32_t* addresses, int maxSize );

int 
stunTest( StunAddress4 *dest, int testNum, StunAddress4* srcAddr, StunAddress4 *sMappedAddr, StunAddress4* sChangedAddr);

NatType
stunNatType( StunAddress4 *dest, 
             bool_t* preservePort, /* if set, is return for if NAT preservers ports or not */
             bool_t* hairpin ,  /* if set, is the return for if NAT will hairpin packets */
             int port, /* port to use for the test, 0 to choose random port */
             StunAddress4* sAddr /* NIC to use */
   );

bool_t
stunServerProcessMsg( char* buf,
                      unsigned int bufLen,
                      StunAddress4 *from, 
                      StunAddress4 *myAddr,
                      StunAddress4 *altAddr, 
                      StunMessage *resp,
                      StunAddress4 *destination,
                      StunAtrString *hmacPassword,
                      bool_t* changePort,
                      bool_t* changeIp);

int
stunOpenSocket( StunAddress4 *dest, 
                StunAddress4* mappedAddr, 
                int port, 
                StunAddress4* srcAddr);

bool_t
stunOpenSocketPair(StunAddress4 *dest,
                   StunAddress4* mapAddr_rtp, 
                   StunAddress4* mapAddr_rtcp, 
                   int* fd1, int* fd2, 
                   int srcPort,  StunAddress4* srcAddr);

bool_t
turnAllocateSocketPair(StunAddress4 *dest,
                   StunAddress4* mapAddr_rtp, 
                   StunAddress4* mapAddr_rtcp, 
                   int* fd1, int* fd2, 
                   int srcPort,  StunAddress4* srcAddr);

#ifdef __cplusplus
}
#endif

#endif

