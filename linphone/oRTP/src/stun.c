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

#ifdef HAVE_CONFIG_H
#include "ortp-config.h"
#endif

#ifndef _WIN32_WCE
#include <errno.h>
#endif

#include <assert.h>

#if defined(WIN32) || defined(_WIN32_WCE)
#include <winsock2.h>
#include <stdlib.h>
/* #include <io.h> */
#include <time.h>
#include <ctype.h> /*for isdigit() */
#else

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h> 
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <net/if.h>

#endif


#if !defined(HAVE_OPENSSL_HMAC_H) || !defined(HAVE_OPENSSL_MD5_H)
#define NOSSL 1
#endif

#include "ortp/stun_udp.h"
#include "ortp/stun.h"
#include "ortp/ortp.h"

static char *ipaddr(const StunAddress4 *addr)
{
   static char tmp[512];
   struct in_addr inaddr;
   char *atmp;
   inaddr.s_addr = htonl(addr->addr);
   atmp = (char *)inet_ntoa(inaddr);
   
   snprintf(tmp, 512, "%s:%i", atmp, addr->port);
   return tmp;
}

static bool_t 
stunParseAtrAddress( char* body, unsigned int hdrLen,  StunAtrAddress4 *result )
{
   if ( hdrLen != 8 )
   {
      ortp_error("stun: hdrLen wrong for Address\n");
      return FALSE;
   }
   result->pad = *body++;
   result->family = *body++;
   if (result->family == IPv4Family)
   {
      uint16_t nport;
      uint32_t naddr;
      memcpy(&nport, body, 2); body+=2;
      result->ipv4.port = ntohs(nport);

      memcpy(&naddr, body, 4); body+=4;
      result->ipv4.addr = ntohl(naddr);
      return TRUE;
   }
   else if (result->family == IPv6Family)
   {
      ortp_error("stun: ipv6 not supported\n");
   }
   else
   {
      ortp_error("stun: bad address family: %i\n", result->family);
   }
	
   return FALSE;
}

static bool_t 
stunParseAtrChangeRequest( char* body, unsigned int hdrLen,  StunAtrChangeRequest *result )
{
   if ( hdrLen != 4 )
   {
     /* ortp_error("stun: hdr length = %i expecting %i\n",hdrLen, sizeof(result)); */
		
      ortp_error("stun: Incorrect size for SA_CHANGEREQUEST");
      return FALSE;
   }
   else
   {
      memcpy(&result->value, body, 4);
      result->value = ntohl(result->value);
      return TRUE;
   }
}

static bool_t 
stunParseAtrError( char* body, unsigned int hdrLen,  StunAtrError *result )
{
   if ( hdrLen < 4 || hdrLen >= 128+4)
   {
      ortp_error("stun: Incorrect size for SA_ERRORCODE");
      return FALSE;
   }
   else
   {
      memcpy(&result->pad, body, 2); body+=2;
      result->pad = ntohs(result->pad);
      result->errorClass = *body++;
      result->number = *body++;
		
      result->sizeReason = hdrLen - 4;
      memcpy(&result->reason, body, result->sizeReason);
      result->reason[result->sizeReason] = 0;
      return TRUE;
   }
}

static bool_t 
stunParseAtrUnknown( char* body, unsigned int hdrLen,  StunAtrUnknown *result )
{
   if ( hdrLen >= sizeof(result) )
   {
      ortp_error("stun: Incorrect size for SA_UNKNOWNATTRIBUTE");
      return FALSE;
   }
   else
   {
      int i;
      if (hdrLen % 4 != 0) return FALSE;
      result->numAttributes = hdrLen / 4;
      for (i=0; i<result->numAttributes; i++)
      {
         memcpy(&result->attrType[i], body, 2); body+=2;
         result->attrType[i] = ntohs(result->attrType[i]);
      }
      return TRUE;
   }
}

static bool_t 
stunParseAtrString( char* body, unsigned int hdrLen,  StunAtrString *result )
{
   if ( hdrLen >= STUN_MAX_STRING )
   {
      ortp_error("stun: String is too large");
      return FALSE;
   }
   else
   {
      result->sizeValue = hdrLen;
      memcpy(&result->value, body, hdrLen);
      result->value[hdrLen] = 0;
      return TRUE;
   }
}


static bool_t 
stunParseAtrIntegrity( char* body, unsigned int hdrLen,  StunAtrIntegrity *result )
{
   if ( hdrLen != 20)
   {
      ortp_error("stun: SA_MESSAGEINTEGRITY must be 20 bytes");
      return FALSE;
   }
   else
   {
      memcpy(&result->hash, body, hdrLen);
      return TRUE;
   }
}

static bool_t 
turnParseAtrChannelNumber( char* body, unsigned int hdrLen,  TurnAtrChannelNumber *result )
{
   if ( hdrLen >= sizeof(result) )
   {
      ortp_error("stun: Incorrect size for TA_CHANNELNUMBER");
      return FALSE;
   }
   else
   {
      if (hdrLen % 4 != 0) return FALSE;
      memcpy(&result->channelNumber, body, 2);
      body+=2;
      result->channelNumber = ntohs(result->channelNumber);
      memcpy(&result->rffu, body, 2);
      body+=2;
      result->rffu = ntohs(result->rffu);
      return TRUE;
   }
}

static bool_t 
turnParseAtrLifetime( char* body, unsigned int hdrLen,  TurnAtrLifetime *result )
{
   if ( hdrLen != sizeof(result) )
   {
      ortp_error("stun: Incorrect size for TA_LIFETIME");
      return FALSE;
   }
   else
   {
      memcpy(&result->lifetime, body, 4);
      result->lifetime = ntohl(result->lifetime);
      return TRUE;
   }
}

static bool_t 
turnParseAtrData( char* body, unsigned int hdrLen,  TurnAtrData *result )
{
   if ( hdrLen >= 1500 )
   {
      ortp_error("stun: Incorrect size for TA_DATA");
      return FALSE;
   }
   else
   {
      result->sizeValue = hdrLen;
      memcpy(&result->value, body, hdrLen);
      result->value[hdrLen] = 0;
      return TRUE;
   }
}

static bool_t 
turnParseAtrRequestedTransport( char* body, unsigned int hdrLen,  TurnAtrRequestedTransport *result )
{
   if ( hdrLen != 4 )
   {
      ortp_error("stun: Incorrect size for TA_REQUESTEDTRANSPORT");
      return FALSE;
   }
   result->proto = *body++;
   result->pad1 = *body++;
   result->pad2 = *body++;
   result->pad3 = *body++;
   return TRUE;
}

#ifdef ORTP_BIGENDIAN
#define htonq(n) n
#define ntohq(n) n
#else /* little endian */
static inline uint64_t
htonq (uint64_t v)
{
  return htonl ((uint32_t) (v >> 32))
    | (uint64_t) htonl ((uint32_t) v) << 32;
}
static inline uint64_t
ntohq (uint64_t v)
{
  return ntohl ((uint32_t) (v >> 32))
    | (uint64_t) ntohl ((uint32_t) v) << 32;
}
#endif /* little endian */

static bool_t 
turnParseAtrReservationToken( char* body, unsigned int hdrLen,  TurnAtrReservationToken *result )
{
  if ( hdrLen != 8 )
  {
    ortp_error("stun: Incorrect size for TA_RESERVATIONTOKEN");
    return FALSE;
  }
  memcpy(&result->value, body, 8);
  result->value = ntohq(result->value);
  return TRUE;
}

static bool_t 
stunParseAtrFingerprint( char* body, unsigned int hdrLen, StunAtrFingerprint *result )
{
  if ( hdrLen != 4 )
  {
    ortp_error("stun: Incorrect size for SA_FINGERPRINT");
    return FALSE;
  }

  memcpy(&result->fingerprint, body, 4);
  result->fingerprint = ntohl(result->fingerprint);
  return TRUE;
}

static bool_t 
iceParseAtrPriority( char* body, unsigned int hdrLen, IceAtrPriority *result )
{
  if ( hdrLen != 4 )
  {
    ortp_error("stun: Incorrect size for ICEA_PRIORITY");
    return FALSE;
  }

  memcpy(&result->priority, body, 4);
  result->priority = ntohl(result->priority);
  return TRUE;
}

static bool_t 
iceParseAtrIceControll( char* body, unsigned int hdrLen, IceAtrIceControll *result )
{
  if ( hdrLen != 8 )
  {
    ortp_error("stun: Incorrect size for ICEA_ICECONTROLLED/ICEA_ICECONTROLLING");
    return FALSE;
  }
  memcpy(&result->value, body, 8);
  result->value = ntohq(result->value);
  return TRUE;
} 

bool_t
stunParseMessage( char* buf, unsigned int bufLen, StunMessage *msg)
{
   char* body;
   unsigned int size;
	 ortp_debug("stun: Received stun message: %i bytes\n", bufLen);
   memset(msg, 0, sizeof(msg));
	
   if (sizeof(StunMsgHdr) > bufLen)
   {
      ortp_warning("stun: message too short\n");
      return FALSE;
   }

   memcpy(&msg->msgHdr, buf, sizeof(StunMsgHdr));
   msg->msgHdr.msgType = ntohs(msg->msgHdr.msgType);
   msg->msgHdr.msgLength = ntohs(msg->msgHdr.msgLength);

   if (msg->msgHdr.msgLength + sizeof(StunMsgHdr) != bufLen)
   {
      ortp_warning("stun: Message header length doesn't match message size: %i - %i\n", msg->msgHdr.msgLength, bufLen);
      return FALSE;
   }

   body = buf + sizeof(StunMsgHdr);
   size = msg->msgHdr.msgLength;
	
   /*ortp_message("stun: bytes after header = %i\n", size); */
	
   while ( size > 0 )
   {
      /* !jf! should check that there are enough bytes left in the buffer */
		
      StunAtrHdr* attr = (StunAtrHdr*)body; /*reinterpret_cast<StunAtrHdr*>(body);*/
		
      unsigned int attrLen = ntohs(attr->length);
      int atrType = ntohs(attr->type);
		
      if ( attrLen+4 > size ) 
      {
         ortp_error("stun: claims attribute is larger than size of message (attribute type=%i)\n", atrType);
         return FALSE;
      }
		
      body += 4; /* skip the length and type in attribute header */
      size -= 4;
		
      if (atrType == SA_MAPPEDADDRESS)
      {
            msg->hasMappedAddress = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->mappedAddress )== FALSE )
            {
               ortp_error("stun: problem parsing SA_MAPPEDADDRESS\n");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: SA_MAPPEDADDRESS = %s\n", ipaddr(&msg->mappedAddress.ipv4));
            }
					
      }
      else if (atrType == SA_RESPONSEADDRESS)
      {
            msg->hasResponseAddress = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->responseAddress )== FALSE )
            {
               ortp_error("stun: problem parsing SA_RESPONSEADDRESS");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: SA_RESPONSEADDRESS = %s\n", ipaddr(&msg->responseAddress.ipv4));
            }
      }
      else if (atrType == SA_CHANGEREQUEST)
      {
            msg->hasChangeRequest = TRUE;
            if (stunParseAtrChangeRequest( body, attrLen, &msg->changeRequest) == FALSE)
            {
               ortp_error("stun: problem parsing SA_CHANGEREQUEST\n");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: SA_CHANGEREQUEST = %i\n", msg->changeRequest.value);
            }
      }
      else if (atrType == SA_SOURCEADDRESS)
      {
            msg->hasSourceAddress = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->sourceAddress )== FALSE )
            {
               ortp_error("stun: problem parsing SA_SOURCEADDRESS\n");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: SA_SOURCEADDRESS = %s\n", ipaddr(&msg->sourceAddress.ipv4) );
            }
      }
      else if (atrType == SA_CHANGEDADDRESS)
      {
            msg->hasChangedAddress = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->changedAddress )== FALSE )
            {
               ortp_error("stun: problem parsing SA_CHANGEDADDRESS\n");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: SA_CHANGEDADDRESS = %s\n", ipaddr(&msg->changedAddress.ipv4));
            }
      }
      else if (atrType == SA_USERNAME)
      {
            msg->hasUsername = TRUE;
            if (stunParseAtrString( body, attrLen, &msg->username) == FALSE)
            {
               ortp_error("stun: problem parsing SA_USERNAME");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: SA_USERNAME = %s\n", msg->username.value );
            }					
      }
      else if (atrType == SA_PASSWORD)
      {
            msg->hasPassword = TRUE;
            if (stunParseAtrString( body, attrLen, &msg->password) == FALSE)
            {
               ortp_error("stun: problem parsing SA_PASSWORD");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: SA_PASSWORD = %s\n", msg->password.value );
            }
      }
      else if (atrType == SA_MESSAGEINTEGRITY)
      {
            msg->hasMessageIntegrity = TRUE;
            if (stunParseAtrIntegrity( body, attrLen, &msg->messageIntegrity) == FALSE)
            {
               ortp_error("stun: problem parsing SA_MESSAGEINTEGRITY");
               return FALSE;
            }					
      }
      else if (atrType == SA_ERRORCODE)
      {
            msg->hasErrorCode = TRUE;
            if (stunParseAtrError(body, attrLen, &msg->errorCode) == FALSE)
            {
               ortp_error("stun: problem parsing SA_ERRORCODE");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: SA_ERRORCODE = %i %i %s\n",
                                   msg->errorCode.errorClass ,
                                   msg->errorCode.number ,
                                   msg->errorCode.reason );
            }
					 
      }
      else if (atrType == SA_UNKNOWNATTRIBUTE)
      {
           msg->hasUnknownAttributes = TRUE;
            if (stunParseAtrUnknown(body, attrLen, &msg->unknownAttributes) == FALSE)
            {
               ortp_error("stun: problem parsing SA_UNKNOWNATTRIBUTE");
               return FALSE;
            }
      }
      else if (atrType == SA_REFLECTEDFROM)
      {
            msg->hasReflectedFrom = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->reflectedFrom ) == FALSE )
            {
               ortp_error("stun: problem parsing SA_REFLECTEDFROM");
               return FALSE;
            }
      }
      else if (atrType == SA_REALM)
      {
            msg->hasRealm = TRUE;
            if (stunParseAtrString( body, attrLen, &msg->realmName) == FALSE)
            {
               ortp_error("stun: problem parsing SA_REALM");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: SA_REALM = %s\n", msg->realmName.value );
            }
      }
      else if (atrType == SA_NONCE)
      {
            msg->hasNonce = TRUE;
            if (stunParseAtrString( body, attrLen, &msg->nonceName) == FALSE)
            {
               ortp_error("stun: problem parsing SA_NONCE");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: SA_NONCE = %s\n", msg->nonceName.value );
            }
      }
      else if (atrType == SA_XORMAPPEDADDRESS || atrType == SA_XORMAPPEDADDRESS2)
      { 
           msg->hasXorMappedAddress = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->xorMappedAddress ) == FALSE )
            {
               ortp_error("stun: problem parsing SA_XORMAPPEDADDRESS");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: SA_XORMAPPEDADDRESS = %s\n", ipaddr(&msg->xorMappedAddress.ipv4) );
            }
      }
      else if (atrType == SA_XORONLY)
      {
            ortp_warning("stun: SA_XORONLY - non standard extension ignored\n" );
      }
      else if (atrType == SA_SECONDARYADDRESS)
      {
            ortp_debug("stun: SA_SECONDARYADDRESS - non standard extension ignored\n" );
      }
      else if (atrType == SA_SOFTWARE)
      {
            msg->hasSoftware = TRUE;
            if (stunParseAtrString( body, attrLen, &msg->softwareName) == FALSE)
            {
               ortp_error("stun: problem parsing SA_SOFTWARE");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: SA_SOFTWARE = %s\n", msg->softwareName.value );
            }
      }
      else if (atrType == TA_CHANNELNUMBER)
      {
           msg->hasChannelNumberAttributes = TRUE;
            if (turnParseAtrChannelNumber(body, attrLen, &msg->channelNumberAttributes) == FALSE)
            {
               ortp_error("stun: problem parsing TA_CHANNELNUMBER");
               return FALSE;
            }					
      }
      else if (atrType == TA_LIFETIME)
      {
           msg->hasLifetimeAttributes = TRUE;
            if (turnParseAtrLifetime(body, attrLen, &msg->lifetimeAttributes) == FALSE)
            {
               ortp_error("stun: problem parsing TA_LIFETIME");
               return FALSE;
            }					
      }
      else if (atrType == TA_DEPRECATEDBANDWIDTH)
      {
           ortp_warning("stun: deprecated attribute TA_DEPRECATEDBANDWIDTH");
      }
      else if (atrType == TA_XORPEERADDRESS)
      {
            msg->hasXorPeerAddress = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->xorPeerAddress )== FALSE )
            {
               ortp_error("stun: problem parsing SA_XORPEERADDRESS\n");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: SA_XORPEERADDRESS = %s\n", ipaddr(&msg->xorPeerAddress.ipv4));
            }
      }
      else if (atrType == TA_DATA)
      {
            msg->hasData = TRUE;
            if (turnParseAtrData( body, attrLen, &msg->data) == FALSE)
            {
               ortp_error("stun: problem parsing TA_DATA");
               return FALSE;
            }
            else
            {
            }
      }
      else if (atrType == TA_XORRELAYEDADDRESS)
      {
            msg->hasXorRelayedAddress = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->xorRelayedAddress )== FALSE )
            {
               ortp_error("stun: problem parsing TA_XORRELAYEDADDRESS\n");
               return FALSE;
            }
            else
            {
               ortp_debug("stun: TA_XORRELAYEDADDRESS = %s\n", ipaddr(&msg->xorRelayedAddress.ipv4));
            }
      }
      else if (atrType == TA_EVENPORT)
      {
           ortp_warning("stun: do we need this... TA_EVENPORT");
      }
      else if (atrType == TA_REQUESTEDTRANSPORT)
      {
            msg->hasRequestedTransport = TRUE;
            if ( turnParseAtrRequestedTransport(  body,  attrLen,  &msg->requestedTransport )== FALSE )
            {
               ortp_error("stun: problem parsing TA_REQUESTEDTRANSPORT\n");
               return FALSE;
            }
      }
      else if (atrType == TA_DONTFRAGMENT)
      {
            msg->hasDontFragment = TRUE;
      }
      else if (atrType == TA_DEPRECATEDTIMERVAL)
      {
           ortp_warning("stun: deprecated attribute TA_DEPRECATEDTIMERVAL");
      }
      else if (atrType == TA_RESERVATIONTOKEN)
      {
            msg->hasReservationToken = TRUE;
            if ( turnParseAtrReservationToken(  body,  attrLen,  &msg->reservationToken)== FALSE )
            {
               ortp_error("stun: problem parsing TA_RESERVATIONTOKEN\n");
               return FALSE;
            }
      }
      else if (atrType == SA_FINGERPRINT)
      {
            msg->hasFingerprint = TRUE;
            if ( stunParseAtrFingerprint(  body,  attrLen,  &msg->fingerprint)== FALSE )
            {
               ortp_error("stun: problem parsing SA_FINGERPRINT\n");
               return FALSE;
            }
      }
      else if (atrType == ICEA_PRIORITY)
      {
            msg->hasPriority = TRUE;
            if (iceParseAtrPriority(body, attrLen, &msg->priority) == FALSE)
            {
               ortp_error("stun: problem parsing ICEA_PRIORITY");
               return FALSE;
            }					
      }
      else if (atrType == ICEA_USECANDIDATE)
      {
            msg->hasUseCandidate = TRUE;
      }
      else if (atrType == ICEA_ICECONTROLLED)
      {
           msg->hasIceControlled = TRUE;
           if (iceParseAtrIceControll(body, attrLen, &msg->iceControlled) == FALSE)
            {
               ortp_error("stun: problem parsing ICEA_ICECONTROLLED");
               return FALSE;
            }
      }
      else if (atrType == ICEA_ICECONTROLLING)
      {
           msg->hasIceControlling = TRUE;
           if (iceParseAtrIceControll(body, attrLen, &msg->iceControlling) == FALSE)
            {
               ortp_error("stun: problem parsing ICEA_ICECONTROLLING");
               return FALSE;
            }
      }
      else
      {
            if ( atrType <= 0x7FFF ) 
            {
              ortp_error("stun: Unknown Comprehension-Required attribute: %04x\n", atrType );
              return FALSE;
            }
            else
              ortp_warning("stun: Unknown attribute: %04x\n", atrType );
      }
		
      if (attrLen%4>0)
        attrLen += (4-(attrLen%4));
      
      body += attrLen;
      size -= attrLen;
   }
    
   return TRUE;
}


static char* 
encode16(char* buf, uint16_t data)
{
   uint16_t ndata = htons(data);
   memcpy(buf, &ndata, sizeof(uint16_t));
   return buf + sizeof(uint16_t);
}

static char* 
encode32(char* buf, uint32_t data)
{
   uint32_t ndata = htonl(data);
   memcpy(buf, &ndata, sizeof(uint32_t));
   return buf + sizeof(uint32_t);
}

static char* 
encode64(char* buf, uint64_t data)
{
   uint64_t ndata = htonq(data);
   memcpy(buf, &ndata, sizeof(uint64_t));
   return buf + sizeof(uint64_t);
}

static char* 
encode(char* buf, const char* data, unsigned int length)
{
   memcpy(buf, data, length);
   return buf + length;
}


static char* 
encodeAtrAddress4(char* ptr, uint16_t type, const StunAtrAddress4 *atr)
{
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, 8);
   *ptr++ = atr->pad;
   *ptr++ = IPv4Family;
   ptr = encode16(ptr, atr->ipv4.port);
   ptr = encode32(ptr, atr->ipv4.addr);
	
   return ptr;
}

static char* 
encodeAtrChangeRequest(char* ptr, const StunAtrChangeRequest *atr)
{
   ptr = encode16(ptr, SA_CHANGEREQUEST);
   ptr = encode16(ptr, 4);
   ptr = encode32(ptr, atr->value);
   return ptr;
}

static char* 
encodeAtrError(char* ptr, const StunAtrError *atr)
{
   int padding;
   int i;

   ptr = encode16(ptr, SA_ERRORCODE);
   ptr = encode16(ptr, 4 + atr->sizeReason);
   ptr = encode16(ptr, atr->pad);
   *ptr++ = atr->errorClass;
   *ptr++ = atr->number;
   ptr = encode(ptr, atr->reason, atr->sizeReason);

   padding = (atr->sizeReason+4) % 4;
   if (padding>0)
   {
     for (i=0;i<4-padding;i++)
     {
       *ptr++ = 0;
     }
   }
   return ptr;
}


static char* 
encodeAtrUnknown(char* ptr, const StunAtrUnknown *atr)
{
   int i;
   ptr = encode16(ptr, SA_UNKNOWNATTRIBUTE);
   ptr = encode16(ptr, 2+2*atr->numAttributes);
   for (i=0; i<atr->numAttributes; i++)
   {
      ptr = encode16(ptr, atr->attrType[i]);
   }
   return ptr;
}

static char* 
encodeAtrString(char* ptr, uint16_t type, const StunAtrString *atr)
{
   int padding;
   int i;
	
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, atr->sizeValue);
   ptr = encode(ptr, atr->value, atr->sizeValue);

   padding = atr->sizeValue % 4;
   if (padding>0)
   {
     for (i=0;i<4-padding;i++)
     {
       *ptr++ = 0;
     }
   }
   return ptr;
}


static char* 
encodeAtrIntegrity(char* ptr, const StunAtrIntegrity *atr)
{
   ptr = encode16(ptr, SA_MESSAGEINTEGRITY);
   ptr = encode16(ptr, 20);
   ptr = encode(ptr, atr->hash, sizeof(atr->hash));
   return ptr;
}

static char*
encodeAtrFingerprint(char* ptr, const StunAtrFingerprint *atr)
{
	uint32_t val;
	ptr = encode16(ptr, SA_FINGERPRINT);
	ptr = encode16(ptr, 4);

	val = atr->fingerprint;
	val ^= 0x5354554E;
	ptr = encode32(ptr, val);
	return ptr;
}

static char* 
encodeAtrRequestedTransport(char* ptr, const TurnAtrRequestedTransport *atr)
{
   ptr = encode16(ptr, TA_REQUESTEDTRANSPORT);
   ptr = encode16(ptr, 4);
   *ptr++ = atr->proto;
   *ptr++ = atr->pad1;
   *ptr++ = atr->pad2;
   *ptr++ = atr->pad3;
   return ptr;
}

static char* 
encodeAtrLifeTime(char* ptr, const TurnAtrLifetime *atr)
{
   ptr = encode16(ptr, TA_LIFETIME);
   ptr = encode16(ptr, 4);
   ptr = encode32(ptr, atr->lifetime);
   return ptr;
}

static char* 
encodeAtrDontFragment(char* ptr)
{
   ptr = encode16(ptr, TA_DONTFRAGMENT);
   ptr = encode16(ptr, 0);
   return ptr;
}

static char* 
encodeAtrUseCandidate(char* ptr)
{
   ptr = encode16(ptr, ICEA_USECANDIDATE);
   ptr = encode16(ptr, 0);
   return ptr;
}

static char* 
encodeAtrPriority(char* ptr, const IceAtrPriority *atr)
{
   ptr = encode16(ptr, ICEA_PRIORITY);
   ptr = encode16(ptr, 4);
   ptr = encode32(ptr, atr->priority);
   return ptr;
}

static char* 
encodeAtrIceControll(char* ptr, uint16_t type, const IceAtrIceControll *atr)
{
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, 8);
   ptr = encode64(ptr, atr->value);
   return ptr;
}

unsigned int
stunEncodeMessage( const StunMessage *msg, 
                   char* buf, 
                   unsigned int bufLen, 
                   const StunAtrString *password)
{
   char* ptr = buf;
   char* lengthp;
   ptr = encode16(ptr, msg->msgHdr.msgType);
   lengthp = ptr;
   ptr = encode16(ptr, 0);
   ptr = encode32(ptr, msg->msgHdr.magic_cookie);
   ptr = encode(ptr, (const char*)msg->msgHdr.tr_id.octet, sizeof(msg->msgHdr.tr_id));
	
   ortp_debug("stun: Encoding stun message: ");

   if (msg->hasRequestedTransport)
   {
      ortp_debug("stun: Encoding TA_REQUESTEDTRANSPORT: %i\n", msg->requestedTransport.proto );
      ptr = encodeAtrRequestedTransport (ptr, &msg->requestedTransport);
   }
   if (msg->hasLifetimeAttributes)
   {
      ortp_debug("stun: Encoding TA_LIFETIME: %i\n", msg->lifetimeAttributes.lifetime );
      ptr = encodeAtrLifeTime (ptr, &msg->lifetimeAttributes);
   }
   if (msg->hasDontFragment)
   {
      ortp_debug("stun: Encoding TA_DONTFRAGMENT: DF\n");
      ptr = encodeAtrDontFragment (ptr);
   }		  
   if (msg->hasMappedAddress)
   {
      ortp_debug("stun: Encoding SA_MAPPEDADDRESS: %s\n", ipaddr(&msg->mappedAddress.ipv4) );
      ptr = encodeAtrAddress4 (ptr, SA_MAPPEDADDRESS, &msg->mappedAddress);
   }
   if (msg->hasResponseAddress)
   {
      ortp_debug("stun: Encoding SA_RESPONSEADDRESS: %s\n", ipaddr(&msg->responseAddress.ipv4) );
      ptr = encodeAtrAddress4(ptr, SA_RESPONSEADDRESS, &msg->responseAddress);
   }
   if (msg->hasChangeRequest)
   {
      ortp_debug("stun: Encoding SA_CHANGEREQUEST: %i\n", msg->changeRequest.value );
      ptr = encodeAtrChangeRequest(ptr, &msg->changeRequest);
   }
   if (msg->hasSourceAddress)
   {
      ortp_debug("stun: Encoding SA_SOURCEADDRESS: %s\n", ipaddr(&msg->sourceAddress.ipv4) );
      ptr = encodeAtrAddress4(ptr, SA_SOURCEADDRESS, &msg->sourceAddress);
   }
   if (msg->hasChangedAddress)
   {
      ortp_debug("stun: Encoding SA_CHANGEDADDRESS: %s\n", ipaddr(&msg->changedAddress.ipv4) );
      ptr = encodeAtrAddress4(ptr, SA_CHANGEDADDRESS, &msg->changedAddress);
   }
   if (msg->hasUsername)
   {
      ortp_debug("stun: Encoding SA_USERNAME: %s\n", msg->username.value );
      ptr = encodeAtrString(ptr, SA_USERNAME, &msg->username);
   }
   //if (msg->hasPassword)
   //{
   //   ortp_debug("stun: Encoding SA_PASSWORD: %s\n", msg->password.value );
   //   ptr = encodeAtrString(ptr, SA_PASSWORD, &msg->password);
   //}
   if (msg->hasErrorCode)
   {
      ortp_debug("stun: Encoding SA_ERRORCODE: class=%i number=%i reason=%s\n" 
                          , msg->errorCode.errorClass 
                          , msg->errorCode.number
                          , msg->errorCode.reason );
      
      ptr = encodeAtrError(ptr, &msg->errorCode);
   }
   if (msg->hasUnknownAttributes)
   {
      ortp_debug("stun: Encoding SA_UNKNOWNATTRIBUTE: ???");
      ptr = encodeAtrUnknown(ptr, &msg->unknownAttributes);
   }
   if (msg->hasReflectedFrom)
   {
      ortp_debug("stun: Encoding SA_REFLECTEDFROM: %s\n", ipaddr(&msg->reflectedFrom.ipv4) );
      ptr = encodeAtrAddress4(ptr, SA_REFLECTEDFROM, &msg->reflectedFrom);
   }
   if (msg->hasNonce)
   {
      ortp_debug("stun: Encoding SA_NONCE: %s\n", msg->nonceName.value );
      ptr = encodeAtrString(ptr, SA_NONCE, &msg->nonceName);
   }
   if (msg->hasRealm)
   {
      ortp_debug("stun: Encoding SA_REALM: %s\n", msg->realmName.value );
      ptr = encodeAtrString(ptr, SA_REALM, &msg->realmName);
   }
   
   if (msg->hasXorMappedAddress)
   {
      ortp_debug("stun: Encoding SA_XORMAPPEDADDRESS: %s\n", ipaddr(&msg->xorMappedAddress.ipv4) );
      ptr = encodeAtrAddress4 (ptr, SA_XORMAPPEDADDRESS, &msg->xorMappedAddress);
   }
   
   if (msg->hasPriority)
   {	   
      ortp_debug("stun: Encoding ICEA_PRIORITY\n");
      ptr = encodeAtrPriority (ptr, &msg->priority);
   }
   if (msg->hasUseCandidate)
   {	   
      ortp_debug("stun: Encoding ICEA_USECANDIDATE\n");
      ptr = encodeAtrUseCandidate (ptr);
   }
   if (msg->hasIceControlled)
   {	   
      ortp_debug("stun: Encoding ICEA_ICECONTROLLED\n");
      ptr = encodeAtrIceControll (ptr, ICEA_ICECONTROLLED, &msg->iceControlled);
   }
   if (msg->hasIceControlling)
   {	   
      ortp_debug("stun: Encoding ICEA_ICECONTROLLING\n");
      ptr = encodeAtrIceControll (ptr, ICEA_ICECONTROLLING, &msg->iceControlling);
   }

   if (msg->hasSoftware)
   {
      ortp_debug("stun: Encoding SA_SOFTWARE: %s\n", msg->softwareName.value );
      ptr = encodeAtrString(ptr, SA_SOFTWARE, &msg->softwareName);
   }

   if (msg->hasMessageIntegrity
     &&password!=NULL && password->sizeValue > 0
     &&msg->username.sizeValue>0
     &&msg->realmName.sizeValue>0)
   {
      StunAtrIntegrity integrity;
      //ortp_debug("stun: HMAC with password: %s\n", password->value );

      encode16(lengthp, (uint16_t)(ptr - buf - sizeof(StunMsgHdr)+24));
      stunCalculateIntegrity_longterm(integrity.hash, buf, (int)(ptr-buf) ,
        msg->username.value, msg->realmName.value, password->value);
      ptr = encodeAtrIntegrity(ptr, &integrity);
   }
   else if (msg->hasMessageIntegrity
     &&password!=NULL && password->sizeValue > 0
     &&msg->username.sizeValue>0)
   {
      StunAtrIntegrity integrity;
      //ortp_debug("stun: HMAC with password: %s\n", password->value );

      encode16(lengthp, (uint16_t)(ptr - buf - sizeof(StunMsgHdr)+24));
      stunCalculateIntegrity_shortterm(integrity.hash, buf, (int)(ptr-buf) ,
        password->value);
      ptr = encodeAtrIntegrity(ptr, &integrity);
   }

   if (msg->hasFingerprint)
   {
      StunAtrFingerprint fingerprint;
      //ortp_debug("stun: HMAC with password: %s\n", password->value );

      encode16(lengthp, (uint16_t)(ptr - buf - sizeof(StunMsgHdr)+8));
      fingerprint.fingerprint = stunCalculateFingerprint(buf, (int)(ptr-buf));
      ptr = encodeAtrFingerprint(ptr, &fingerprint);
   }
   encode16(lengthp, (uint16_t)(ptr - buf - sizeof(StunMsgHdr)));
   return (int)(ptr - buf);
}

int 
stunRand(void)
{
   /* return 32 bits of random stuff */
   /* assert( sizeof(int) == 4 ); */
   static bool_t init=FALSE;
   if ( !init )
   { 
      uint64_t tick;
      int seed;
      init = TRUE;

#if defined(_WIN32_WCE)
      tick = GetTickCount ();
#elif defined(_MSC_VER)
      {
      volatile unsigned int lowtick=0,hightick=0;
      __asm
         {
            rdtsc 
               mov lowtick, eax
               mov hightick, edx
               }
      tick = hightick;
      tick <<= 32;
      tick |= lowtick;
      }
#elif defined(__MACH__) 
	   {
		   int fd=open("/dev/random",O_RDONLY);
		   read(fd,&tick,sizeof(tick));
		   closesocket(fd);
	   }
#elif defined(__GNUC__) && ( defined(__i686__) || defined(__i386__) )
      asm("rdtsc" : "=A" (tick));
#elif defined(__GNUC__) && defined(__amd64__)
      asm("rdtsc" : "=A" (tick));
#elif defined (__SUNPRO_CC) && defined( __sparc__ )	
      tick = gethrtime();
#elif defined(__linux) || defined(HAVE_DEV_RANDOM) 
      {
 	fd_set fdSet;
	int maxFd=0;
	struct timeval tv;
	int e;

        int fd=open("/dev/random",O_RDONLY);

	if (fd<0)
	{
	    ortp_message("stun: Failed to open random device\n");
	    return random();
	}
        FD_ZERO(&fdSet);
        FD_SET(fd,&fdSet);
        maxFd=fd+1;

	tv.tv_sec = 0;
	tv.tv_usec = 500;

	e = select( maxFd, &fdSet, NULL,NULL, &tv );
	if (e <= 0)
	{
           ortp_error("stun: Failed to get data from random device\n");
           closesocket(fd);
	   return random();
	}
	read(fd,&tick,sizeof(tick));
	closesocket(fd);
      }
#else
#     error Need some way to seed the random number generator 
#endif 
      seed = (int)(tick);
#if	defined(_WIN32) || defined(_WIN32_WCE)
      srand(seed);
#else
      srandom(seed);
#endif
   }
	
#if	defined(_WIN32) || defined(_WIN32_WCE)
   /* assert( RAND_MAX == 0x7fff ); */
   {
       int r1 = rand();
       int r2 = rand();
       int ret = (r1<<16) + r2;
	
       return ret;
   }
#else
   return random(); 
#endif
}


/* return a random number to use as a port */
static int
randomPort()
{
   int min=0x4000;
   int max=0x7FFF;
	
   int ret = stunRand();
   ret = ret|min;
   ret = ret&max;
	
   return ret;
}


#ifdef NOSSL
void
stunCalculateIntegrity_longterm(char* hmac, const char* input, int length,
                     const char *username, const char *realm, const char *password)
{
   strncpy(hmac,"hmac-not-implemented",20);
}
void
stunCalculateIntegrity_shortterm(char* hmac, const char* input, int length, const char* key)
{
   strncpy(hmac,"hmac-not-implemented",20);
}

#else
#include <openssl/hmac.h>
#include <openssl/md5.h>

void
stunCalculateIntegrity_longterm(char* hmac, const char* input, int length,
                     const char *username, const char *realm, const char *password)
{
   unsigned int resultSize=0;
   unsigned char HA1[16];
   char HA1_text[1024];

   snprintf(HA1_text, sizeof(HA1_text), "%s:%s:%s", username, realm, password);
   MD5((unsigned char *)HA1_text, strlen(HA1_text), HA1);

   HMAC(EVP_sha1(), 
        HA1, 16, 
        (const unsigned char*) input, length, 
        (unsigned char*)hmac, &resultSize);
}

void
stunCalculateIntegrity_shortterm(char* hmac, const char* input, int length, const char* key)
{
   unsigned int resultSize=0;
   HMAC(EVP_sha1(), 
        key, strlen(key), 
        (const unsigned char*) input, length, 
        (unsigned char*)hmac, &resultSize);
}

#endif

uint32_t
stunCalculateFingerprint(const char* input, int length)
{
	/*-
	2  *  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or
	3  *  code or tables extracted from it, as desired without restriction.
	4  */
	static uint32_t crc32_tab[] = {
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
		0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
		0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
		0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
		0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
		0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
		0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
		0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
		0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
		0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
		0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
		0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
		0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
		0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
		0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
		0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
		0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
		0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
		0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
		0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
		0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
		0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
		0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
		0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
		0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
		0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
		0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
		0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
		0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
		0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
		0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
		0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
		0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
		0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
		0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
		0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
	};
	const uint8_t *p = (uint8_t*)input;
	uint32_t crc;

	crc = ~0U;
	while (length--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	return crc ^ ~0U;
}

uint64_t
stunGetSystemTimeSecs(void)
{
   uint64_t time=0;
#if	defined(_WIN32) || defined(_WIN32_WCE)
   SYSTEMTIME t;
   /*  CJ TODO - this probably has bug on wrap around every 24 hours */
   GetSystemTime( &t );
   time = (t.wHour*60+t.wMinute)*60+t.wSecond; 
#else
   struct timeval now;
   gettimeofday( &now , NULL );
   /* assert( now ); */
   time = now.tv_sec;
#endif
   return time;
}


/* returns TRUE if it scucceeded */
bool_t 
stunParseHostName( const char* peerName,
                   uint32_t* ip,
                   uint16_t* portVal,
                   uint16_t defaultPort )
{
   struct in_addr sin_addr;
    
   char host[512];
   char* port = NULL;
   int portNum = defaultPort;
   char* sep;
   struct hostent* h;

   strncpy(host,peerName,512);
   host[512-1]='\0';
	
   /* pull out the port part if present. */
   sep = strchr(host,':');
	
   if ( sep == NULL )
   {
      portNum = defaultPort;
   }
   else
   {
      char* endPtr=NULL;
      *sep = '\0';
      port = sep + 1;
      /* set port part */
      
		
      portNum = strtol(port,&endPtr,10);
		
      if ( endPtr != NULL )
      {
         if ( *endPtr != '\0' )
         {
            portNum = defaultPort;
         }
      }
   }
    
   if ( portNum < 1024 ) return FALSE;
   if ( portNum >= 0xFFFF ) return FALSE;
	
   /* figure out the host part */
	
#if	defined(_WIN32) || defined(_WIN32_WCE)
   /* assert( strlen(host) >= 1 ); */
   if ( isdigit( host[0] ) )
   {
      /* assume it is a ip address */
      unsigned long a = inet_addr(host);
      /* cerr << "a=0x" << hex << a << dec ); */
		
      *ip = ntohl( a );
   }
   else
   {
      /* assume it is a host name */
      h = gethostbyname( host );
		
      if ( h == NULL )
      {
         /*int err = getErrno();*/

         /* ortp_message("stun: error was %i\n", err); */
         /* std::cerr << "error was " << err << std::endl; */
         /* assert( err != WSANOTINITIALISED ); */
			
         *ip = ntohl( 0x7F000001L );
			
         return FALSE;
      }
      else
      {
         sin_addr = *(struct in_addr*)h->h_addr;
         *ip = ntohl( sin_addr.s_addr );
      }
   }
	
#else
   h = gethostbyname( host );
   if ( h == NULL )
   {
      /* 
	 int err = getErrno();
	 ortp_message("stun: error was %i\n", err);
      */
      *ip = ntohl( 0x7F000001L );
      return FALSE;
   }
   else
   {
      sin_addr = *(struct in_addr*)h->h_addr;
      *ip = ntohl( sin_addr.s_addr );
   }
#endif
	
   *portVal = portNum;
	
   return TRUE;
}


bool_t
stunParseServerName( const char* name, StunAddress4 *addr)
{
   /* assert(name); */
	
   /* TODO - put in DNS SRV stuff. */
	
   bool_t ret = stunParseHostName( name, &addr->addr, &addr->port, 3478); 
   if ( ret != TRUE ) 
   {
       addr->port=0xFFFF;
   }	
   return ret;
}


static void
stunCreateErrorResponse(StunMessage *response, int cl, int number, const char* msg)
{
   response->msgHdr.msgType = (STUN_METHOD_BINDING | STUN_ERR_RESP);
   response->hasErrorCode = TRUE;
   response->errorCode.errorClass = cl;
   response->errorCode.number = number;
   strcpy(response->errorCode.reason, msg);
}

#if 0
static void
stunCreateSharedSecretErrorResponse(StunMessage& response, int cl, int number, const char* msg)
{
   response.msgHdr.msgType = SharedSecretErrorResponseMsg;
   response.hasErrorCode = TRUE;
   response.errorCode.errorClass = cl;
   response.errorCode.number = number;
   strcpy(response.errorCode.reason, msg);
}
#endif

#if 0
static void
stunCreateSharedSecretResponse(const StunMessage *request, const StunAddress4 *source, StunMessage *response)
{
   response->msgHdr.msgType = SharedSecretResponseMsg;
   response->msgHdr.tr_id = request->msgHdr.tr_id;
	
   response->hasUsername = TRUE;
   stunCreateUserName( source, &response->username);
	
   response->hasPassword = TRUE;
   stunCreatePassword( &response->username, &response->password);
}
#endif

/* This funtion takes a single message sent to a stun server, parses
   and constructs an apropriate repsonse - returns TRUE if message is
   valid */
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
                      bool_t* changeIp)
{
   int i;
   StunMessage req;
   StunAddress4 mapped;
   StunAddress4 respondTo;
   uint32_t flags;
   bool_t ok;
   /* set up information for default response */
	
   memset( &req, 0 , sizeof(req) );
   memset( resp, 0 , sizeof(*resp) );
	
   *changeIp = FALSE;
   *changePort = FALSE;
	
   ok = stunParseMessage( buf,bufLen, &req);
   
   if (!ok)      /* Complete garbage, drop it on the floor */
   {
      ortp_error("stun: Request did not parse");
      return FALSE;
   }
   //ortp_debug("stun: Request parsed ok");
	
   mapped = req.mappedAddress.ipv4;
   respondTo = req.responseAddress.ipv4;
   flags = req.changeRequest.value;
	
   if (req.msgHdr.msgType==(STUN_METHOD_BINDING|STUN_REQUEST))
   {
         if (!req.hasMessageIntegrity)
         {
            //ortp_debug("stun: BindRequest does not contain SA_MESSAGEINTEGRITY");
				
            if (0) /* !jf! mustAuthenticate */
            {
               ortp_error("stun: Received BindRequest with no SA_MESSAGEINTEGRITY. Sending 401.");
               stunCreateErrorResponse(resp, 4, 1, "Missing SA_MESSAGEINTEGRITY");
               return TRUE;
            }
         }
         else
         {
            if (!req.hasUsername)
            {
               ortp_error("stun: No UserName. Send 432.");
               stunCreateErrorResponse(resp, 4, 32, "No UserName and contains SA_MESSAGEINTEGRITY");
               return TRUE;
            }
            else
            {
               //ortp_debug("stun: Validating username: %s", req.username.value );
               /* !jf! could retrieve associated password from provisioning here */
               if (strcmp(req.username.value, "test") == 0)
               {
                  if (0)
                  {
                     /* !jf! if the credentials are stale */
                     stunCreateErrorResponse(resp, 4, 30, "Stale credentials on BindRequest");
                     return TRUE;
                  }
                  else
                  {
                     unsigned char hmac[20];
                     //ortp_debug("stun: Validating SA_MESSAGEINTEGRITY");
                     /* need access to shared secret */

#ifndef NOSSL
                     {
                        unsigned int hmacSize=20;

                        HMAC(EVP_sha1(), 
                             "1234", 4, 
                             (const unsigned char*) buf, bufLen-20-4, 
                             hmac, &hmacSize);
                     }
#endif
							
                     if (memcmp(buf, hmac, 20) != 0)
                     {
                        ortp_error("stun: SA_MESSAGEINTEGRITY is bad. Sending ");
                        stunCreateErrorResponse(resp, 4, 3, "Unknown username. Try test with password 1234");
                        return TRUE;
                     }
							
                     /* need to compute this later after message is filled in */
                     resp->hasMessageIntegrity = TRUE;
                     /* assert(req.hasUsername); */
                     resp->hasUsername = TRUE;
                     resp->username = req.username; /* copy username in */
                  }
               }
               else
               {
                  ortp_error("stun: Invalid username: %s Send 430", req.username.value); 
               }
            }
         }
			
         /* TODO !jf! should check for unknown attributes here and send 420 listing the
            unknown attributes. */
			
         if ( respondTo.port == 0 )
         {
            /* respondTo = from; */
            memcpy(&respondTo, from, sizeof(StunAddress4));
         }
         if ( mapped.port == 0 ) 
         {
            /* mapped = from; */
            memcpy(&mapped, from, sizeof(StunAddress4));
         }

         *changeIp   = ( flags & ChangeIpFlag )?TRUE:FALSE;
         *changePort = ( flags & ChangePortFlag )?TRUE:FALSE;
			
         //ortp_debug("stun: Request is valid:\n");
         //ortp_debug("stun: \t flags= %i\n", flags );
         //ortp_debug("stun: \t changeIp= %i\n", *changeIp );
         //ortp_debug("stun: \t changePort=%i\n", *changePort );
         //ortp_debug("stun: \t from= %i\n", from->addr );
         //ortp_debug("stun: \t respond to= %i\n", respondTo.addr );
         //ortp_debug("stun: \t mapped= %i\n", mapped.addr );
				
         /* form the outgoing message */
         resp->msgHdr.msgType = (STUN_METHOD_BINDING | STUN_SUCCESS_RESP);
         resp->msgHdr.magic_cookie = ntohl(req.msgHdr.magic_cookie);
         for (i=0; i<12; i++ )
         {
            resp->msgHdr.tr_id.octet[i] = req.msgHdr.tr_id.octet[i];
         }
		
         if (1) /* do xorMapped address or not */
         {
            uint32_t cookie = 0x2112A442;
            resp->hasXorMappedAddress = TRUE;
            resp->xorMappedAddress.ipv4.port = mapped.port^(cookie>>16);
            resp->xorMappedAddress.ipv4.addr = mapped.addr^cookie;
         }
         
         resp->hasSourceAddress = TRUE;
         resp->sourceAddress.ipv4.port = (*changePort) ? altAddr->port : myAddr->port;
         resp->sourceAddress.ipv4.addr = (*changeIp)   ? altAddr->addr : myAddr->addr;
			
         resp->hasChangedAddress = TRUE;
         resp->changedAddress.ipv4.port = altAddr->port;
         resp->changedAddress.ipv4.addr = altAddr->addr;
	
         if ( req.hasUsername && req.username.sizeValue > 0 ) 
         {
            /* copy username in */
            resp->hasUsername = TRUE;
            /* assert( req.username.sizeValue % 4 == 0 ); */
            /* assert( req.username.sizeValue < STUN_MAX_STRING ); */
            memcpy( resp->username.value, req.username.value, req.username.sizeValue );
            resp->username.sizeValue = req.username.sizeValue;
         }
		
         if (1) /* add ServerName */
         {
            const char serverName[] = "oRTP   " STUN_VERSION; /* must pad to mult of 4 */
            resp->hasSoftware = TRUE;
            
            /* assert( sizeof(serverName) < STUN_MAX_STRING ); */
            /* cerr << "sizeof serverName is "  << sizeof(serverName) ); */
            /* assert( sizeof(serverName)%4 == 0 ); */
            memcpy( resp->softwareName.value, serverName, sizeof(serverName));
            resp->softwareName.sizeValue = sizeof(serverName);
         }
         
#if 0
         if ( req.hasMessageIntegrity & req.hasUsername )  
         {
            /* this creates the password that will be used in the HMAC when then */
            /* messages is sent */
            stunCreatePassword( &req.username, hmacPassword );
         }
#endif

         if (req.hasUsername && (req.username.sizeValue > 64 ) )
         {
            uint32_t source;
            /* assert( sizeof(int) == sizeof(uint32_t) ); */
					
            sscanf(req.username.value, "%x", &source);
            resp->hasReflectedFrom = TRUE;
            resp->reflectedFrom.ipv4.port = 0;
            resp->reflectedFrom.ipv4.addr = source;
         }
				
         destination->port = respondTo.port;
         destination->addr = respondTo.addr;
			
         return TRUE;		
   }
   else
   {
         ortp_error("stun: Unknown or unsupported request ");
         return FALSE;
   }
	
   /* assert(0); */
   return FALSE;
}

bool_t
stunInitServer(StunServerInfo *info, const StunAddress4 *myAddr, const StunAddress4 *altAddr, int startMediaPort)
{
   /* assert( myAddr.port != 0 ); */
   /* assert( altAddr.port!= 0 ); */
   /* assert( myAddr.addr  != 0 ); */
   /* assert( altAddr.addr != 0 ); */
	
   /* info->myAddr = myAddr; */
   info->myAddr.port = myAddr->port;
   info->myAddr.addr = myAddr->addr;

   /* info->altAddr = altAddr; */
   info->altAddr.port = altAddr->port;
   info->altAddr.addr = altAddr->addr;
	
   info->myFd = INVALID_SOCKET;
   info->altPortFd = INVALID_SOCKET;
   info->altIpFd = INVALID_SOCKET;
   info->altIpPortFd = INVALID_SOCKET;

   memset(info->relays, 0, sizeof(info->relays));
   if (startMediaPort > 0)
   {
      int i;
      info->relay = TRUE;

      for (i=0; i<MAX_MEDIA_RELAYS; ++i)
      {
         StunMediaRelay* relay = &info->relays[i];
         relay->relayPort = startMediaPort+i;
         relay->fd = 0;
         relay->expireTime = 0;
      }
   }
   else
   {
      info->relay = FALSE;
   }
   
   if ((info->myFd = openPort(myAddr->port, myAddr->addr)) == INVALID_SOCKET)
   {
      ortp_error("stun: Can't open %i\n", myAddr->addr );
      stunStopServer(info);

      return FALSE;
   }

   if ((info->altPortFd = openPort(altAddr->port,myAddr->addr)) == INVALID_SOCKET)
   {
      ortp_error("stun: Can't open %i\n", myAddr->addr );
      stunStopServer(info);
      return FALSE;
   }
   
   
   info->altIpFd = INVALID_SOCKET;
   if (  altAddr->addr != 0 )
   {
      if ((info->altIpFd = openPort( myAddr->port, altAddr->addr)) == INVALID_SOCKET)
      {
         ortp_error("stun: Can't open %i\n", altAddr->addr );
         stunStopServer(info);
         return FALSE;
      }
   }
   
   info->altIpPortFd = INVALID_SOCKET;
   if (  altAddr->addr != 0 )
   {  if ((info->altIpPortFd = openPort(altAddr->port, altAddr->addr)) == INVALID_SOCKET)
      {
         ortp_error("stun: Can't open %i\n", altAddr->addr );
         stunStopServer(info);
         return FALSE;
      }
   }
   
   return TRUE;
}

void
stunStopServer(StunServerInfo *info)
{
   if (info->myFd > 0) closesocket(info->myFd);
   if (info->altPortFd > 0) closesocket(info->altPortFd);
   if (info->altIpFd > 0) closesocket(info->altIpFd);
   if (info->altIpPortFd > 0) closesocket(info->altIpPortFd);
   
   if (info->relay)
   {
      int i;
      for (i=0; i<MAX_MEDIA_RELAYS; ++i)
      {
         StunMediaRelay* relay = &info->relays[i];
         if (relay->fd)
         {
            closesocket(relay->fd);
            relay->fd = 0;
         }
      }
   }
}

int 
stunFindLocalInterfaces(uint32_t* addresses,int maxRet)
{
#if defined(WIN32) || defined(_WIN32_WCE) || defined(__sparc__)
   return 0;
#else
   struct ifconf ifc;
   int e;

   int s = socket( AF_INET, SOCK_DGRAM, 0 );
   int len = 100 * sizeof(struct ifreq);
   
   char buf[ 100 * sizeof(struct ifreq) ];
   char *ptr;
   int tl;
   int count=0;

   ifc.ifc_len = len;
   ifc.ifc_buf = buf;
	
   e = ioctl(s,SIOCGIFCONF,&ifc);
   ptr = buf;
   tl = ifc.ifc_len;
   
   while ( (tl > 0) && ( count < maxRet) )
   {
      struct ifreq* ifr = (struct ifreq *)ptr;
      struct ifreq ifr2;
      struct sockaddr a;
      struct sockaddr_in* addr;
   
      uint32_t ai;
      int si = sizeof(ifr->ifr_name) + sizeof(struct sockaddr);
      tl -= si;
      ptr += si;
      /* char* name = ifr->ifr_ifrn.ifrn_name; */
      /* cerr << "name = " << name ); */
      
      ifr2 = *ifr;
      
      e = ioctl(s,SIOCGIFADDR,&ifr2);
      if ( e == -1 )
      {
         break;
      }
      
      /* cerr << "ioctl addr e = " << e ; */
      
      a = ifr2.ifr_addr;
      addr = (struct sockaddr_in*) &a;
      
      ai = ntohl( addr->sin_addr.s_addr );
      if ((int)((ai>>24)&0xFF) != 127)
      {
         addresses[count++] = ai;
      }
		
   }
   
   closesocket(s);
   
   return count;
#endif
}


void
stunBuildReqSimple( StunMessage* msg,
                    const StunAtrString *username,
                    bool_t changePort, bool_t changeIp, unsigned int id )
{
   int i;
   /* assert( msg ); */
   memset( msg , 0 , sizeof(*msg) );
	
   msg->msgHdr.msgType = (STUN_METHOD_BINDING|STUN_REQUEST);
	
   msg->msgHdr.magic_cookie = 0x2112A442;
   for ( i=0; i<12; i=i+4 )
   {
      /* assert(i+3<16); */
      int r = stunRand();
      msg->msgHdr.tr_id.octet[i+0]= r>>0;
      msg->msgHdr.tr_id.octet[i+1]= r>>8;
      msg->msgHdr.tr_id.octet[i+2]= r>>16;
      msg->msgHdr.tr_id.octet[i+3]= r>>24;
   }
	
   if ( id != 0 )
   {
      msg->msgHdr.tr_id.octet[0] = id; 
   }
	
   if (changePort==TRUE || changeIp==TRUE)
   {
     msg->hasChangeRequest = TRUE;
     msg->changeRequest.value =(changeIp?ChangeIpFlag:0) | 
        (changePort?ChangePortFlag:0);
   }

   if ( username!=NULL && username->sizeValue > 0 )
   {
      msg->hasUsername = TRUE;
      /* msg->username = username; */
      memcpy(&msg->username, username, sizeof(StunAtrString));
   }
}


static void 
stunSendTest( Socket myFd, StunAddress4 *dest, 
              const StunAtrString *username, const StunAtrString *password, 
              int testNum )
{ 
   /* assert( dest.addr != 0 ); */
   /* assert( dest.port != 0 ); */
	
   bool_t changePort=FALSE;
   bool_t changeIP=FALSE;
   bool_t discard=FALSE;

   StunMessage req;
   char buf[STUN_MAX_MESSAGE_SIZE];
   int len = STUN_MAX_MESSAGE_SIZE;
   
   switch (testNum)
   {
      case 1:
      case 10:
      case 11:
         break;
      case 2:
         /* changePort=TRUE; */
         changeIP=TRUE;
         break;
      case 3:
         changePort=TRUE;
         break;
      case 4:
         changeIP=TRUE;
         break;
      case 5:
         discard=TRUE;
         break;
      default:
         ortp_error("stun: Test %i is unkown\n", testNum);
         return ; /* error */
   }
   
   memset(&req, 0, sizeof(StunMessage));
	
   stunBuildReqSimple( &req, username, 
                       changePort , changeIP , 
                       testNum );
	
   len = stunEncodeMessage( &req, buf, len, password );
	
   //ortp_debug("stun: About to send msg of len %i to %s\n", len, ipaddr(dest) );
	
   sendMessage( myFd, buf, len, dest->addr, dest->port );
	
   /* add some delay so the packets don't get sent too quickly */
#if defined(_WIN32_WCE) 
   Sleep (10);
#elif defined(WIN32)/* !cj! TODO - should fix this up in windows */
   {
       clock_t now = clock();
       /* assert( CLOCKS_PER_SEC == 1000 ); */
       while ( clock() <= now+10 ) { };
   }
#else
		 usleep(10*1000);
#endif

}


#if 0
void 
stunGetUserNameAndPassword(  const StunAddress4 *dest, 
                             StunAtrString* username,
                             StunAtrString* password)
{ 
   /* !cj! This is totally bogus - need to make TLS connection to dest and get a */
   /* username and password to use */
   stunCreateUserName(dest, username);
   stunCreatePassword(username, password);
}
#endif

int 
stunTest( StunAddress4 *dest, int testNum, StunAddress4* sAddr , StunAddress4 *sMappedAddr, StunAddress4* sChangedAddr)
{
   /* assert( dest.addr != 0 ); */
   /* assert( dest.port != 0 ); */
	
   int port = randomPort();
   uint32_t interfaceIp=0;
   Socket myFd;
   StunAtrString username;
   StunAtrString password;
   char msg[STUN_MAX_MESSAGE_SIZE];
   int msgLen = STUN_MAX_MESSAGE_SIZE;
   StunAddress4 from;
   StunMessage resp;
   bool_t ok;

   if (sAddr)
   {
      interfaceIp = sAddr->addr;
      if ( sAddr->port != 0 )
      {
        port = sAddr->port;
      }
   }
   myFd = openPort(port,interfaceIp);
   if ( myFd == INVALID_SOCKET)
       return -1;
   
   username.sizeValue = 0;
   password.sizeValue = 0;
	
#if 0
   stunGetUserNameAndPassword( dest, &username, &password );
#endif
	
   stunSendTest( myFd, dest, &username, &password, testNum );
   
   ok = getMessage( myFd,
               msg,
               &msgLen,
               &from.addr,
               &from.port );
   closesocket(myFd);
   if (!ok)
       return -1;

   memset(&resp, 0, sizeof(StunMessage));
	
   //ortp_debug("stun: Got a response");
   ok = stunParseMessage( msg,msgLen, &resp );
	
   //ortp_debug("stun: \t ok=%i\n", ok );
   //ortp_debug("stun: \t SA_MAPPEDADDRESS=%i\n", resp.mappedAddress.ipv4.addr );
   //ortp_debug("stun: \t SA_CHANGEDADDRESS=%i\n", resp.changedAddress.ipv4.addr );
	
   if (sAddr)
   {
       sAddr->port = port;
   }

   if (sMappedAddr)
   {
      sMappedAddr->port = resp.mappedAddress.ipv4.port;
      sMappedAddr->addr = resp.mappedAddress.ipv4.addr;
   }

   if (sChangedAddr)
   {
      sChangedAddr->port = resp.changedAddress.ipv4.port;
      sChangedAddr->addr = resp.changedAddress.ipv4.addr;
   }

   if (ok)
       return 0;
   else
       return -1;
}




NatType
stunNatType( StunAddress4 *dest, 
             bool_t* preservePort, /* if set, is return for if NAT preservers ports or not */
             bool_t* hairpin,  /* if set, is the return for if NAT will hairpin packets */
             int port, /* port to use for the test, 0 to choose random port */
             StunAddress4* sAddr /* NIC to use */
   )
{ 
   /* assert( dest.addr != 0 ); */
   /* assert( dest.port != 0 ); */
   uint32_t interfaceIp=0;
   Socket myFd1;
   Socket myFd2;

   bool_t respTestI=FALSE;
   bool_t isNat=TRUE;
   StunAddress4 testIchangedAddr;
   StunAddress4 testImappedAddr;
   bool_t respTestI2=FALSE; 
   bool_t mappedIpSame = TRUE;
   StunAddress4 testI2mappedAddr;
   /* StunAddress4 testI2dest=dest; */
   StunAddress4 testI2dest;
   bool_t respTestII=FALSE;
   bool_t respTestIII=FALSE;
   bool_t respTestHairpin=FALSE;
   StunAtrString username;
   StunAtrString password;
   int count=0;
   uint64_t second_started;
   uint64_t second_elapsed;
   Socket s;

   if ( hairpin ) 
   {
      *hairpin = FALSE;
   }
	
   if ( port == 0 )
   {
      port = randomPort();
   }

   if (sAddr)
   {
      interfaceIp = sAddr->addr;
   }
   myFd1 = openPort(port,interfaceIp);
   myFd2 = openPort(port+1,interfaceIp);

   if ( ( myFd1 == INVALID_SOCKET) || ( myFd2 == INVALID_SOCKET) )
   {
      ortp_error("stun: Some problem opening port/interface to send on\n");
      return StunTypeFailure; 
   }

   /* assert( myFd1 != INVALID_SOCKET ); */
   /* assert( myFd2 != INVALID_SOCKET ); */
   
   memcpy(&testI2dest, dest, sizeof(StunAddress4));

   memset(&testImappedAddr,0,sizeof(testImappedAddr));
   
   username.sizeValue = 0;
   password.sizeValue = 0;
	
#if 0 
   stunGetUserNameAndPassword( dest, username, password );
#endif
	
   /* stunSendTest( myFd1, dest, username, password, 1 ); */

   
   second_started = stunGetSystemTimeSecs();
   second_elapsed = 1;

   while ( count < 3 && second_elapsed < 5)
   {
      struct timeval tv;
      fd_set fdSet; 
      int err;
      int e;

#if defined(WIN32) || defined(_WIN32_WCE)
      unsigned int fdSetSize;
#else
      int fdSetSize;
#endif

      second_elapsed = stunGetSystemTimeSecs() - second_started ;

      FD_ZERO(&fdSet); fdSetSize=0;
      FD_SET(myFd1,&fdSet); fdSetSize = (myFd1+1>fdSetSize) ? myFd1+1 : fdSetSize;
      FD_SET(myFd2,&fdSet); fdSetSize = (myFd2+1>fdSetSize) ? myFd2+1 : fdSetSize;
      tv.tv_sec=0;
      tv.tv_usec=500*1000; /* 150 ms */
      if ( count == 0 ) tv.tv_usec=0;
		
      err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
      e = getErrno();
      if ( err == SOCKET_ERROR )
      {
         /* error occured */
#if !defined(_WIN32_WCE)
         ortp_error("stun: Error %i %s in select\n", e, strerror(e));
#else
         ortp_error("stun: Error %i in select\n", e);
#endif
		 closesocket(myFd1); /* AMD */
         closesocket(myFd2); /* AMD */
         return StunTypeFailure;
     }
      else if ( err == 0 )
      {
         /* timeout occured */
         count++;
         if ( !respTestI ) 
         {
            stunSendTest( myFd1, dest, &username, &password, 1 );
         }         
			
         if ( (!respTestI2) && respTestI ) 
         {
            /* check the address to send to if valid */
            if (  ( testI2dest.addr != 0 ) &&
                  ( testI2dest.port != 0 ) )
            {
               stunSendTest( myFd1, &testI2dest, &username, &password, 10 );
            }
         }
			
         if ( !respTestII )
         {
            stunSendTest( myFd2, dest, &username, &password, 2 );
         }
			
         if ( !respTestIII )
         {
            stunSendTest( myFd2, dest, &username, &password, 3 );
         }
			
         if ( respTestI && (!respTestHairpin) )
         {
            if (  ( testImappedAddr.addr != 0 ) &&
                  ( testImappedAddr.port != 0 ) )
            {
               stunSendTest( myFd1, &testImappedAddr, &username, &password, 11 );
            }
         }

      }
      else
      {
         int i;
         /* data is avialbe on some fd */
			
         for ( i=0; i<2; i++)
         {
            Socket myFd;
            if ( i==0 ) 
            {
               myFd=myFd1;
            }
            else
            {
               myFd=myFd2;
            }
				
            if ( myFd!=INVALID_SOCKET ) 
            {					
               if ( FD_ISSET(myFd,&fdSet) )
               {
                  char msg[STUN_MAX_MESSAGE_SIZE];
                  int msgLen = sizeof(msg);
                  						
                  StunAddress4 from;
                  StunMessage resp;

                  getMessage( myFd,
                              msg,
                              &msgLen,
                              &from.addr,
                              &from.port );
                  
                  memset(&resp, 0, sizeof(StunMessage));
						
                  stunParseMessage( msg,msgLen, &resp );
						
                  //ortp_debug("stun: Received message of type %i id=%i\n",
                          //resp.msgHdr.msgType,
                          //(int)(resp.msgHdr.tr_id.octet[0]) );
						
                  switch( resp.msgHdr.tr_id.octet[0] )
                  {
                     case 1:
                     {
                        if ( !respTestI )
                        {
									
                           testIchangedAddr.addr = resp.changedAddress.ipv4.addr;
                           testIchangedAddr.port = resp.changedAddress.ipv4.port;
                           testImappedAddr.addr = resp.mappedAddress.ipv4.addr;
                           testImappedAddr.port = resp.mappedAddress.ipv4.port;
									
                           if ( preservePort )
                           {
                              *preservePort = ( testImappedAddr.port == port );
                           }								
									
                           testI2dest.addr = resp.changedAddress.ipv4.addr;
									
                           if (sAddr)
                           {
                              sAddr->port = testImappedAddr.port;
                              sAddr->addr = testImappedAddr.addr;
                           }
									
                           count = 0;
                        }		
                        respTestI=TRUE;
                     }
                     break;
                     case 2:
                     {  
                        respTestII=TRUE;
                     }
                     break;
                     case 3:
                     {
                        respTestIII=TRUE;
                     }
                     break;
                     case 10:
                     {
                        if ( !respTestI2 )
                        {
                           testI2mappedAddr.addr = resp.mappedAddress.ipv4.addr;
                           testI2mappedAddr.port = resp.mappedAddress.ipv4.port;
								
                           mappedIpSame = FALSE;
                           if ( (testI2mappedAddr.addr  == testImappedAddr.addr ) &&
                                (testI2mappedAddr.port == testImappedAddr.port ))
                           { 
                              mappedIpSame = TRUE;
                           }
								
							
                        }
                        respTestI2=TRUE;
                     }
                     break;
                     case 11:
                     {
							
                        if ( hairpin ) 
                        {
                           *hairpin = TRUE;
                        }
                        respTestHairpin = TRUE;
                     }
                     break;
                  }
               }
            }
         }
      }
   }
	
   closesocket(myFd1); /* AMD */
   closesocket(myFd2); /* AMD */

   /* see if we can bind to this address */
   /* cerr << "try binding to " << testImappedAddr ); */
   s = openPort( 0/*use ephemeral*/, testImappedAddr.addr );
   if ( s != INVALID_SOCKET )
   {
      isNat = FALSE;
      /* cerr << "binding worked"); */
   }
   else
   {
      isNat = TRUE;
      /* cerr << "binding failed"); */
   }

   closesocket(s); /* AMD */
	
   //ortp_debug("stun: test I = %i\n", respTestI );
   //ortp_debug("stun: test II = %i\n", respTestII );
   //ortp_debug("stun: test III = %i\n", respTestIII );
   //ortp_debug("stun: test I(2) = %i\n", respTestI2 );
   ortp_debug("stun: is nat  = %i\n", isNat);
   ortp_debug("stun: mapped IP same = %i\n", mappedIpSame );
	
   /* implement logic flow chart from draft RFC */
   if ( respTestI )
   {
      if ( isNat )
      {
         if (respTestII)
         {
            return StunTypeConeNat;
         }
         else
         {
            if ( mappedIpSame )
            {
               if ( respTestIII )
               {
                  return StunTypeRestrictedNat;
               }
               else
               {
                  return StunTypePortRestrictedNat;
               }
            }
            else
            {
               return StunTypeSymNat;
            }
         }
      }
      else
      {
         if (respTestII)
         {
            return StunTypeOpen;
         }
         else
         {
            return StunTypeSymFirewall;
         }
      }
   }
   else
   {
      return StunTypeBlocked;
   }
	
   return StunTypeUnknown;
}

int
stunOpenSocket( StunAddress4 *dest, StunAddress4* mapAddr, 
                int port, StunAddress4* srcAddr )
{
   /* assert( dest.addr != 0 ); */
   /* assert( dest.port != 0 ); */
   /* assert( mapAddr );*/
   unsigned int interfaceIp = 0;
   Socket myFd;
   char msg[STUN_MAX_MESSAGE_SIZE];
   int msgLen = sizeof(msg);
	
   StunAtrString username;
   StunAtrString password;

   StunAddress4 from;
   StunMessage resp;
   bool_t ok;
   StunAddress4 mappedAddr;

   if ( port == 0 )
   {
      port = randomPort();
   }

   if ( srcAddr )
   {
      interfaceIp = srcAddr->addr;
   }
   
   myFd = openPort(port,interfaceIp);
   if (myFd == INVALID_SOCKET)
   {
      return myFd;
   }
   
   username.sizeValue = 0;
   password.sizeValue = 0;
	
#if 0
   stunGetUserNameAndPassword( dest, username, password );
#endif
	
   stunSendTest(myFd, dest, &username, &password, 1 );
	
   getMessage( myFd, msg, &msgLen, &from.addr, &from.port );

   memset(&resp, 0, sizeof(StunMessage));
	
   ok = stunParseMessage( msg, msgLen, &resp );
   if (!ok)
   {
       closesocket(myFd);
       return -1;
   }

   if (resp.hasXorMappedAddress==TRUE)
   {
      uint32_t cookie = 0x2112A442;
      uint16_t cookie16 = 0x2112A442 >> 16;
      mappedAddr.port = resp.xorMappedAddress.ipv4.port^cookie16;
      mappedAddr.addr = resp.xorMappedAddress.ipv4.addr^cookie;
   }
   else
     mappedAddr = resp.mappedAddress.ipv4;
	
   /*
     ortp_message("stun: --- stunOpenSocket --- ");
     ortp_message("stun: \treq  id=" << req.id );
     ortp_message("stun: \tresp id=" << id );
     ortp_message("stun: \tmappedAddr=" << mappedAddr );
   */

   *mapAddr = mappedAddr;
	
   return myFd;
}


bool_t
stunOpenSocketPair(StunAddress4 *dest,
                   StunAddress4* mapAddr_rtp, 
                   StunAddress4* mapAddr_rtcp, 
                   int* fd1, int* fd2, 
                   int port, StunAddress4* srcAddr )
{
   /* assert( dest.addr!= 0 ); */
   /* assert( dest.port != 0 ); */
   /* assert( mapAddr ); */
   
   const int NUM=2;
   char msg[STUN_MAX_MESSAGE_SIZE];
   int msgLen =sizeof(msg);
	
   StunAddress4 from;
   int fd[2/*NUM*/];
   int i;
	
   unsigned int interfaceIp = 0;
	
   StunAtrString username;
   StunAtrString password;
	
   StunAddress4 mappedAddr[2/*NUM*/];

   if ( port == 0 )
   {
      port = randomPort();
   }
	
   *fd1=-1;
   *fd2=-1;
	
   if ( srcAddr )
   {
      interfaceIp = srcAddr->addr;
   }

   for( i=0; i<NUM; i++)
   {
      fd[i] = openPort( (port == 0) ? 0 : (port + i), interfaceIp);
      if (fd[i] < 0) 
      {
         while (i > 0)
         {
            closesocket(fd[--i]);
         }
         return FALSE;
      }
   }
	
   username.sizeValue = 0;
   password.sizeValue = 0;
	
#if 0
   stunGetUserNameAndPassword( dest, username, password );
#endif
	
   for( i=0; i<NUM; i++)
   {
      stunSendTest(fd[i], dest, &username, &password, 1/*testNum*/ );
   }
   
   for( i=0; i<NUM; i++)
   {
      StunMessage resp;
      bool_t ok;
      msgLen = sizeof(msg)/sizeof(*msg);
      getMessage( fd[i],
                  msg,
                  &msgLen,
                  &from.addr,
                  &from.port);
      
      memset(&resp, 0, sizeof(StunMessage));
		
      ok = stunParseMessage( msg, msgLen, &resp );
      if (!ok) 
      {  
          for( i=0; i<NUM; i++)
          {
              closesocket(fd[i]);
          }
         return FALSE;
      }
	  
      if (resp.hasXorMappedAddress==TRUE)
      {
        uint32_t cookie = 0x2112A442;
        uint16_t cookie16 = 0x2112A442 >> 16;
        mappedAddr[i].port = resp.xorMappedAddress.ipv4.port^cookie16;
        mappedAddr[i].addr = resp.xorMappedAddress.ipv4.addr^cookie;
      }
      else
        mappedAddr[i] = resp.mappedAddress.ipv4;
   }
	
   //ortp_debug("stun: --- stunOpenSocketPair --- \n");
   for( i=0; i<NUM; i++)
   {
      //ortp_debug("stun: \t mappedAddr=%s\n", ipaddr(&mappedAddr[i]) );
   }
	
    *mapAddr_rtp = mappedAddr[0];
    *mapAddr_rtcp = mappedAddr[1];
    *fd1 = fd[0];
    *fd2 = fd[1];

   for( i=0; i<NUM; i++)
   {
      closesocket( fd[i] );
   }
	
   return TRUE;
}

static void 
turnSendAllocate( Socket myFd, StunAddress4 *dest, 
              const StunAtrString *username, const StunAtrString *password,
              StunMessage *resp)
{ 
  StunMessage req;
  char buf[STUN_MAX_MESSAGE_SIZE];
  int len = STUN_MAX_MESSAGE_SIZE;
  const char serverName[] = "oRTP   " STUN_VERSION; /* must pad to mult of 4 */

  memset(&req, 0, sizeof(StunMessage));

  stunBuildReqSimple( &req, username, 
                     FALSE , FALSE , 
                     0 );
  req.msgHdr.msgType = (TURN_MEDHOD_ALLOCATE|STUN_REQUEST);

  req.hasSoftware = TRUE;
  memcpy( req.softwareName.value, serverName, sizeof(serverName));
  req.softwareName.sizeValue = sizeof(serverName);

  req.hasRequestedTransport = TRUE;
  memset(&req.requestedTransport, 0, sizeof(req.requestedTransport));
  req.requestedTransport.proto = IPPROTO_UDP;

  req.hasDontFragment = TRUE;

  if (resp!=NULL
    && username!=NULL && username->sizeValue>0
    && password!=NULL && password->sizeValue>0
    && resp->hasRealm==TRUE
    && resp->hasNonce==TRUE)
  {
    req.hasUsername = TRUE;
    memcpy( req.username.value, username->value, username->sizeValue );
    req.username.sizeValue = username->sizeValue;

    req.hasNonce = TRUE;
    memcpy( &req.nonceName, &resp->nonceName, sizeof(resp->nonceName));

    req.hasRealm = TRUE;
    memcpy( &req.realmName, &resp->realmName, sizeof(resp->realmName));

    req.hasMessageIntegrity = TRUE;
  }

  len = stunEncodeMessage( &req, buf, len, password );

  ortp_debug("stun: About to send msg of len %i to %s\n", len, ipaddr(dest) );

  sendMessage( myFd, buf, len, dest->addr, dest->port);

  /* add some delay so the packets don't get sent too quickly */
#if defined(_WIN32_WCE) 
  Sleep (10);
#elif defined(WIN32)/* !cj! TODO - should fix this up in windows */
  {
    clock_t now = clock();
    /* assert( CLOCKS_PER_SEC == 1000 ); */
    while ( clock() <= now+10 ) { };
  }
#else
  usleep(10*1000);
#endif
}

bool_t
turnAllocateSocketPair(StunAddress4 *dest,
                   StunAddress4* mapAddr_rtp, 
                   StunAddress4* mapAddr_rtcp, 
                   int* fd1, int* fd2, 
                   int port, StunAddress4* srcAddr)
{
   const int NUM=2;
   char msg[STUN_MAX_MESSAGE_SIZE];
   int msgLen =sizeof(msg);
	
   StunAddress4 from;
   int fd[2/*NUM*/];
   int i;
	
   unsigned int interfaceIp = 0;
	
   StunAtrString username;
   StunAtrString password;
	
   StunAddress4 mappedAddr[2/*NUM*/];

   if ( port == 0 )
   {
      port = randomPort();
   }
	
   *fd1=-1;
   *fd2=-1;
	
   if ( srcAddr )
   {
      interfaceIp = srcAddr->addr;
   }

   for( i=0; i<NUM; i++)
   {
      fd[i] = openPort( (port == 0) ? 0 : (port + i), 
                        interfaceIp);
      if (fd[i] < 0) 
      {
         while (i > 0)
         {
            closesocket(fd[--i]);
         }
         return FALSE;
      }
   }
	
   snprintf(username.value, sizeof(username.value), "antisip");
   username.sizeValue = strlen(username.value);
   snprintf(password.value, sizeof(password.value), "exosip");
   password.sizeValue = strlen(password.value);
	
   for( i=0; i<NUM; i++)
   {
      turnSendAllocate(fd[i], dest, NULL, NULL, NULL );
   }
   
   for( i=0; i<NUM; i++)
   {
      StunMessage resp;
      bool_t ok;
      msgLen = sizeof(msg)/sizeof(*msg);
      getMessage( fd[i],
                  msg,
                  &msgLen,
                  &from.addr,
                  &from.port);
      
      memset(&resp, 0, sizeof(StunMessage));
		
      ok = stunParseMessage( msg, msgLen, &resp );
      if (!ok) 
      {  
          for( i=0; i<NUM; i++)
          {
              closesocket(fd[i]);
          }
         return FALSE;
      }

      if (STUN_IS_ERR_RESP(resp.msgHdr.msgType))
      {
        /* check if we need to authenticate */
        if (resp.hasErrorCode==TRUE
          && resp.errorCode.errorClass==4 && resp.errorCode.number==1
          && resp.hasNonce == TRUE
          && resp.hasRealm == TRUE)
        {
          turnSendAllocate(fd[i], dest, &username, &password, &resp);
          i--;
        }
      }
      else if (STUN_IS_SUCCESS_RESP(resp.msgHdr.msgType))
      {
        if (resp.hasXorRelayedAddress==TRUE)
        {
          uint32_t cookie = 0x2112A442;
          uint16_t cookie16 = 0x2112A442 >> 16;
          mappedAddr[i].port = resp.xorRelayedAddress.ipv4.port^cookie16;
          mappedAddr[i].addr = resp.xorRelayedAddress.ipv4.addr^cookie;
        }
        else
        {
          for( i=0; i<NUM; i++)
          {
            closesocket(fd[i]);
          }
          return FALSE;
        }
      }
   }
	
  for( i=0; i<NUM; i++)
  {
     ortp_message("stun: stunOpenSocketPair mappedAddr=%s\n", ipaddr(&mappedAddr[i]) );
  }
	
  *mapAddr_rtp = mappedAddr[0];
  *mapAddr_rtcp = mappedAddr[1];
  *fd1 = fd[0];
  *fd2 = fd[1];

  for( i=0; i<NUM; i++)
  {
    closesocket( fd[i] );
  }
	
  return TRUE;
}

/* Local Variables:
   mode:c
   c-file-style:"ellemtel"
   c-file-offsets:((case-label . +))
   indent-tabs-mode:nil
   End:
*/

