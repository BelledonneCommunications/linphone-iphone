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


#ifdef __cplusplus
extern "C"
{
#endif

/* if you change this version, change in makefile too  */
#define STUN_VERSION "0.94"

#define STUN_MAX_STRING 256
#define STUN_MAX_UNKNOWN_ATTRIBUTES 8
#define STUN_MAX_MESSAGE_SIZE 2048

#define STUN_PORT 3478

/* define some basic types */
typedef unsigned char  UInt8;
typedef unsigned short UInt16;
typedef unsigned int   UInt32;
#if	defined(WIN32) || defined(_WIN32_WCE)
typedef unsigned __int64 UInt64;
#else
typedef unsigned long long UInt64;
#endif
typedef struct { unsigned char octet[16]; }  UInt128;

/* define a structure to hold a stun address  */
#define  IPv4Family  0x01
#define  IPv6Family  0x02

/* define  flags  */
#define ChangeIpFlag    0x04
#define ChangePortFlag  0x02

/* define  stun attribute */
#define MappedAddress     0x0001
#define ResponseAddress   0x0002
#define ChangeRequest     0x0003
#define SourceAddress     0x0004
#define ChangedAddress    0x0005
#define STUNUsername      0x0006 /* Username is too common: rename to avoid conflict */
#define STUNPassword      0x0007 /* Password is too common: rename to avoid conflict */
#define MessageIntegrity  0x0008
#define ErrorCode         0x0009
#define UnknownAttribute  0x000A
#define ReflectedFrom     0x000B
#define XorMappedAddress  0x0020
#define XorOnly           0x0021
#define ServerName        0x0022
#define SecondaryAddress  0x0050 /* Non standard extention */

/* define types for a stun message */
#define BindRequestMsg                0x0001
#define BindResponseMsg               0x0101
#define BindErrorResponseMsg          0x0111
#define SharedSecretRequestMsg        0x0002
#define SharedSecretResponseMsg       0x0102
#define SharedSecretErrorResponseMsg  0x0112

typedef struct 
{
      UInt16 msgType;
      UInt16 msgLength;
      UInt128 id;
} StunMsgHdr;


typedef struct
{
      UInt16 type;
      UInt16 length;
} StunAtrHdr;

typedef struct
{
      UInt16 port;
      UInt32 addr;
} StunAddress4;

typedef struct
{
      UInt8 pad;
      UInt8 family;
      StunAddress4 ipv4;
} StunAtrAddress4;

typedef struct
{
      UInt32 value;
} StunAtrChangeRequest;

typedef struct
{
      UInt16 pad; /* all 0 */
      UInt8 errorClass;
      UInt8 number;
      char reason[STUN_MAX_STRING];
      UInt16 sizeReason;
} StunAtrError;

typedef struct
{
      UInt16 attrType[STUN_MAX_UNKNOWN_ATTRIBUTES];
      UInt16 numAttributes;
} StunAtrUnknown;

typedef struct
{
      char value[STUN_MAX_STRING];      
      UInt16 sizeValue;
} StunAtrString;

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

      bool_t hasXorMappedAddress;
      StunAtrAddress4  xorMappedAddress;
	
      bool_t xorOnly;

      bool_t hasServerName;
      StunAtrString serverName;
      
      bool_t hasSecondaryAddress;
      StunAtrAddress4 secondaryAddress;
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

bool_t
stunParseMessage( char* buf, 
                  unsigned int bufLen, 
                  StunMessage *message, 
                  bool_t verbose );

void
stunBuildReqSimple( StunMessage* msg,
                    const StunAtrString *username,
                    bool_t changePort, bool_t changeIp, unsigned int id );

unsigned int
stunEncodeMessage( const StunMessage *message, 
                   char* buf, 
                   unsigned int bufLen, 
                   const StunAtrString *password,
                   bool_t verbose);

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

UInt64
stunGetSystemTimeSecs(void);

/* find the IP address of a the specified stun server - return false is fails parse  */
bool_t  
stunParseServerName( char* serverName, StunAddress4 *stunServerAddr);

bool_t 
stunParseHostName( char* peerName,
                   UInt32 *ip,
                   UInt16 *portVal,
                   UInt16 defaultPort );

/* return true if all is OK 
   Create a media relay and do the STERN thing if startMediaPort is non-zero */
bool_t
stunInitServer(StunServerInfo *info, 
               const StunAddress4 *myAddr, 
               const StunAddress4 *altAddr,
               int startMediaPort,
               bool_t verbose);

void
stunStopServer(StunServerInfo *info);

#if 0 /* no usefull here */
/* return true if all is OK */
bool_t
stunServerProcess(StunServerInfo *info, bool_t verbose);
#endif

/* returns number of address found - take array or addres */
int 
stunFindLocalInterfaces(UInt32* addresses, int maxSize );

int 
stunTest( StunAddress4 *dest, int testNum, bool_t verbose, StunAddress4* srcAddr, StunAddress4 *sMappedAddr, StunAddress4* sChangedAddr);

NatType
stunNatType( StunAddress4 *dest, bool_t verbose, 
             bool_t* preservePort, /* if set, is return for if NAT preservers ports or not */
             bool_t* hairpin ,  /* if set, is the return for if NAT will hairpin packets */
             int port, /* port to use for the test, 0 to choose random port */
             StunAddress4* sAddr /* NIC to use */
   );

bool_t
stunServerProcessMsg( char* buf,
                      unsigned int bufLen,
                      StunAddress4 *from, 
                      StunAddress4 *secondary,
                      StunAddress4 *myAddr,
                      StunAddress4 *altAddr, 
                      StunMessage *resp,
                      StunAddress4 *destination,
                      StunAtrString *hmacPassword,
                      bool_t* changePort,
                      bool_t* changeIp,
                      bool_t verbose);

int
stunOpenSocket( StunAddress4 *dest, 
                StunAddress4* mappedAddr, 
                int port, 
                StunAddress4* srcAddr, 
                bool_t verbose );

bool_t
stunOpenSocketPair(StunAddress4 *dest,
                   StunAddress4* mapAddr_rtp, 
                   StunAddress4* mapAddr_rtcp, 
                   int* fd1, int* fd2, 
                   int srcPort,  StunAddress4* srcAddr,
                   bool_t verbose);

#ifdef __cplusplus
}
#endif

#endif

