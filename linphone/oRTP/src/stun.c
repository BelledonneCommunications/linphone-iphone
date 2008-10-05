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


#define NOSSL
/*
  #if defined(__sparc__) || defined(WIN32)
  #define NOSSL
  #endif
  #define NOSSL
*/

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

static void
computeHmac(char* hmac, const char* input, int length, const char* key, int keySize);

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
      UInt16 nport;
      UInt32 naddr;
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
		
      ortp_error("stun: Incorrect size for ChangeRequest");
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
   if ( hdrLen >= sizeof(result) )
   {
      ortp_error("stun: head on Error too large");
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
      if (hdrLen % 4 != 0)
      {
         ortp_error("stun: Bad length string %i\n", hdrLen);
         return FALSE;
      }
		
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
      ortp_error("stun: MessageIntegrity must be 20 bytes");
      return FALSE;
   }
   else
   {
      memcpy(&result->hash, body, hdrLen);
      return TRUE;
   }
}


bool_t
stunParseMessage( char* buf, unsigned int bufLen, StunMessage *msg, bool_t verbose)
{
   char* body;
   unsigned int size;
   if (verbose)
	   ortp_message("stun: Received stun message: %i bytes\n", bufLen);
   memset(msg, 0, sizeof(msg));
	
   if (sizeof(StunMsgHdr) > bufLen)
   {
      ortp_warning("stun: Bad message\n");
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
		
      /*if (verbose) ortp_message("stun: Found attribute type=" << AttrNames[atrType] << " length=" << attrLen << endl;*/
      if ( attrLen+4 > size ) 
      {
         ortp_error("stun: claims attribute is larger than size of message (attribute type=%i)\n", atrType);
         return FALSE;
      }
		
      body += 4; /* skip the length and type in attribute header */
      size -= 4;
		
      if (atrType == MappedAddress)
      {
            msg->hasMappedAddress = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->mappedAddress )== FALSE )
            {
               ortp_error("stun: problem parsing MappedAddress\n");
               return FALSE;
            }
            else
            {
               if (verbose)
				   ortp_message("stun: MappedAddress = %s\n", ipaddr(&msg->mappedAddress.ipv4));
            }
					
      }
      else if (atrType == ResponseAddress)
      {
            msg->hasResponseAddress = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->responseAddress )== FALSE )
            {
               ortp_error("stun: problem parsing ResponseAddress");
               return FALSE;
            }
            else
            {
               if (verbose)
				   ortp_message("stun: ResponseAddress = %s\n", ipaddr(&msg->responseAddress.ipv4));
            }
      }
      else if (atrType == ChangeRequest)
      {
            msg->hasChangeRequest = TRUE;
            if (stunParseAtrChangeRequest( body, attrLen, &msg->changeRequest) == FALSE)
            {
               ortp_error("stun: problem parsing ChangeRequest\n");
               return FALSE;
            }
            else
            {
               if (verbose)
				   ortp_message("stun: ChangeRequest = %i\n", msg->changeRequest.value);
            }
      }
      else if (atrType == SourceAddress)
      {
            msg->hasSourceAddress = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->sourceAddress )== FALSE )
            {
               ortp_error("stun: problem parsing SourceAddress\n");
               return FALSE;
            }
            else
            {
               if (verbose)
				   ortp_message("stun: SourceAddress = %s\n", ipaddr(&msg->sourceAddress.ipv4) );
            }
      }
      else if (atrType == ChangedAddress)
      {
            msg->hasChangedAddress = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->changedAddress )== FALSE )
            {
               ortp_error("stun: problem parsing ChangedAddress\n");
               return FALSE;
            }
            else
            {
               if (verbose) ortp_message("stun: ChangedAddress = %s\n", ipaddr(&msg->changedAddress.ipv4));
            }
      }
      else if (atrType == STUNUsername)
      {
            msg->hasUsername = TRUE;
            if (stunParseAtrString( body, attrLen, &msg->username) == FALSE)
            {
               ortp_error("stun: problem parsing Username");
               return FALSE;
            }
            else
            {
               if (verbose)
				   ortp_message("stun: Username = %s\n", msg->username.value );
            }					
      }
      else if (atrType == STUNPassword)
      {
            msg->hasPassword = TRUE;
            if (stunParseAtrString( body, attrLen, &msg->password) == FALSE)
            {
               ortp_error("stun: problem parsing Password");
               return FALSE;
            }
            else
            {
               if (verbose)
				   ortp_message("stun: Password = %s\n", msg->password.value );
            }
      }
      else if (atrType == MessageIntegrity)
      {
            msg->hasMessageIntegrity = TRUE;
            if (stunParseAtrIntegrity( body, attrLen, &msg->messageIntegrity) == FALSE)
            {
               ortp_error("stun: problem parsing MessageIntegrity");
               return FALSE;
            }
            else
            {
               /*if (verbose) ortp_message("stun: MessageIntegrity = " << msg->messageIntegrity.hash ); */
            }
					
            /* read the current HMAC
               look up the password given the user of given the transaction id 
               compute the HMAC on the buffer
               decide if they match or not */
      }
      else if (atrType == ErrorCode)
      {
            msg->hasErrorCode = TRUE;
            if (stunParseAtrError(body, attrLen, &msg->errorCode) == FALSE)
            {
               ortp_error("stun: problem parsing ErrorCode");
               return FALSE;
            }
            else
            {
               if (verbose)
				   ortp_message("stun: ErrorCode = %i %i %s\n",
                                   msg->errorCode.errorClass ,
                                   msg->errorCode.number ,
                                   msg->errorCode.reason );
            }
					 
      }
      else if (atrType == UnknownAttribute)
      {
           msg->hasUnknownAttributes = TRUE;
            if (stunParseAtrUnknown(body, attrLen, &msg->unknownAttributes) == FALSE)
            {
               ortp_error("stun: problem parsing UnknownAttribute");
               return FALSE;
            }
      }
      else if (atrType == ReflectedFrom)
      {
            msg->hasReflectedFrom = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->reflectedFrom ) == FALSE )
            {
               ortp_error("stun: problem parsing ReflectedFrom");
               return FALSE;
            }
      }
      else if (atrType == XorMappedAddress)
      { 
           msg->hasXorMappedAddress = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->xorMappedAddress ) == FALSE )
            {
               ortp_error("stun: problem parsing XorMappedAddress");
               return FALSE;
            }
            else
            {
               if (verbose)
				   ortp_message("stun: XorMappedAddress = %s\n", ipaddr(&msg->mappedAddress.ipv4) );
            }
      }
      else if (atrType == XorOnly)
      {
            msg->xorOnly = TRUE;
      }
      else if (atrType == ServerName)
      {
            msg->hasServerName = TRUE;
            if (stunParseAtrString( body, attrLen, &msg->serverName) == FALSE)
            {
               ortp_error("stun: problem parsing ServerName");
               return FALSE;
            }
            else
            {
               if (verbose)
				   ortp_message("stun: ServerName = %s\n", msg->serverName.value );
            }
      }
      else if (atrType == SecondaryAddress)
      {
            msg->hasSecondaryAddress = TRUE;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->secondaryAddress ) == FALSE )
            {
               ortp_error("stun: problem parsing secondaryAddress");
               return FALSE;
            }
            else
            {
               if (verbose)
				   ortp_message("stun: SecondaryAddress = %s\n", ipaddr(&msg->secondaryAddress.ipv4) );
            }
      }
      else
      {
            if (verbose)
				ortp_message("stun: Unknown attribute: %i\n", atrType );
            if ( atrType <= 0x7FFF ) 
            {
               return FALSE;
            }
      }
		
      body += attrLen;
      size -= attrLen;
   }
    
   return TRUE;
}


static char* 
encode16(char* buf, UInt16 data)
{
   UInt16 ndata = htons(data);
   /*memcpy(buf, reinterpret_cast<void*>(&ndata), sizeof(UInt16)); */
   memcpy(buf, &ndata, sizeof(UInt16));
   return buf + sizeof(UInt16);
}

static char* 
encode32(char* buf, UInt32 data)
{
   UInt32 ndata = htonl(data);
   /*memcpy(buf, reinterpret_cast<void*>(&ndata), sizeof(UInt32));*/
   memcpy(buf, &ndata, sizeof(UInt32));
   return buf + sizeof(UInt32);
}


static char* 
encode(char* buf, const char* data, unsigned int length)
{
   memcpy(buf, data, length);
   return buf + length;
}


static char* 
encodeAtrAddress4(char* ptr, UInt16 type, const StunAtrAddress4 *atr)
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
   ptr = encode16(ptr, ChangeRequest);
   ptr = encode16(ptr, 4);
   ptr = encode32(ptr, atr->value);
   return ptr;
}

static char* 
encodeAtrError(char* ptr, const StunAtrError *atr)
{
   ptr = encode16(ptr, ErrorCode);
   ptr = encode16(ptr, 6 + atr->sizeReason);
   ptr = encode16(ptr, atr->pad);
   *ptr++ = atr->errorClass;
   *ptr++ = atr->number;
   ptr = encode(ptr, atr->reason, atr->sizeReason);
   return ptr;
}


static char* 
encodeAtrUnknown(char* ptr, const StunAtrUnknown *atr)
{
   int i;
   ptr = encode16(ptr, UnknownAttribute);
   ptr = encode16(ptr, 2+2*atr->numAttributes);
   for (i=0; i<atr->numAttributes; i++)
   {
      ptr = encode16(ptr, atr->attrType[i]);
   }
   return ptr;
}


static char* 
encodeXorOnly(char* ptr)
{
   ptr = encode16(ptr, XorOnly );
   return ptr;
}


static char* 
encodeAtrString(char* ptr, UInt16 type, const StunAtrString *atr)
{
   /*assert(atr->sizeValue % 4 == 0);*/
	
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, atr->sizeValue);
   ptr = encode(ptr, atr->value, atr->sizeValue);
   return ptr;
}


static char* 
encodeAtrIntegrity(char* ptr, const StunAtrIntegrity *atr)
{
   ptr = encode16(ptr, MessageIntegrity);
   ptr = encode16(ptr, 20);
   ptr = encode(ptr, atr->hash, sizeof(atr->hash));
   return ptr;
}


unsigned int
stunEncodeMessage( const StunMessage *msg, 
                   char* buf, 
                   unsigned int bufLen, 
                   const StunAtrString *password, 
                   bool_t verbose)
{
   /*assert(bufLen >= sizeof(StunMsgHdr));*/
   char* ptr = buf;
   char* lengthp;
   ptr = encode16(ptr, msg->msgHdr.msgType);
   lengthp = ptr;
   ptr = encode16(ptr, 0);
   /*ptr = encode(ptr, reinterpret_cast<const char*>(msg->msgHdr.id.octet), sizeof(msg->msgHdr.id));*/
   ptr = encode(ptr, (const char*)msg->msgHdr.id.octet, sizeof(msg->msgHdr.id));
	
   if (verbose) ortp_message("stun: Encoding stun message: ");
   if (msg->hasMappedAddress)
   {
      if (verbose) ortp_message("stun: Encoding MappedAddress: %s\n", ipaddr(&msg->mappedAddress.ipv4) );
      ptr = encodeAtrAddress4 (ptr, MappedAddress, &msg->mappedAddress);
   }
   if (msg->hasResponseAddress)
   {
      if (verbose) ortp_message("stun: Encoding ResponseAddress: %s\n", ipaddr(&msg->responseAddress.ipv4) );
      ptr = encodeAtrAddress4(ptr, ResponseAddress, &msg->responseAddress);
   }
   if (msg->hasChangeRequest)
   {
      if (verbose) ortp_message("stun: Encoding ChangeRequest: %i\n", msg->changeRequest.value );
      ptr = encodeAtrChangeRequest(ptr, &msg->changeRequest);
   }
   if (msg->hasSourceAddress)
   {
      if (verbose) ortp_message("stun: Encoding SourceAddress: %s\n", ipaddr(&msg->sourceAddress.ipv4) );
      ptr = encodeAtrAddress4(ptr, SourceAddress, &msg->sourceAddress);
   }
   if (msg->hasChangedAddress)
   {
      if (verbose) ortp_message("stun: Encoding ChangedAddress: %s\n", ipaddr(&msg->changedAddress.ipv4) );
      ptr = encodeAtrAddress4(ptr, ChangedAddress, &msg->changedAddress);
   }
   if (msg->hasUsername)
   {
      if (verbose) ortp_message("stun: Encoding Username: %s\n", msg->username.value );
      ptr = encodeAtrString(ptr, STUNUsername, &msg->username);
   }
   if (msg->hasPassword)
   {
      if (verbose) ortp_message("stun: Encoding Password: %s\n", msg->password.value );
      ptr = encodeAtrString(ptr, STUNPassword, &msg->password);
   }
   if (msg->hasErrorCode)
   {
      if (verbose) ortp_message("stun: Encoding ErrorCode: class=%i number=%i reason=%s\n" 
                          , msg->errorCode.errorClass 
                          , msg->errorCode.number
                          , msg->errorCode.reason );
      
      ptr = encodeAtrError(ptr, &msg->errorCode);
   }
   if (msg->hasUnknownAttributes)
   {
      if (verbose) ortp_message("stun: Encoding UnknownAttribute: ???");
      ptr = encodeAtrUnknown(ptr, &msg->unknownAttributes);
   }
   if (msg->hasReflectedFrom)
   {
      if (verbose) ortp_message("stun: Encoding ReflectedFrom: %s\n", ipaddr(&msg->reflectedFrom.ipv4) );
      ptr = encodeAtrAddress4(ptr, ReflectedFrom, &msg->reflectedFrom);
   }
   if (msg->hasXorMappedAddress)
   {
      if (verbose) ortp_message("stun: Encoding XorMappedAddress: %s\n", ipaddr(&msg->xorMappedAddress.ipv4) );
      ptr = encodeAtrAddress4 (ptr, XorMappedAddress, &msg->xorMappedAddress);
   }
   if (msg->xorOnly)
   {
      if (verbose) ortp_message("stun: Encoding xorOnly: ");
      ptr = encodeXorOnly( ptr );
   }
   if (msg->hasServerName)
   {
      if (verbose) ortp_message("stun: Encoding ServerName: %s\n", msg->serverName.value );
      ptr = encodeAtrString(ptr, ServerName, &msg->serverName);
   }
   if (msg->hasSecondaryAddress)
   {
      if (verbose) ortp_message("stun: Encoding SecondaryAddress: %s\n", ipaddr(&msg->secondaryAddress.ipv4) );
      ptr = encodeAtrAddress4 (ptr, SecondaryAddress, &msg->secondaryAddress);
   }

   if (password->sizeValue > 0)
   {
      StunAtrIntegrity integrity;
      if (verbose) ortp_message("stun: HMAC with password: %s\n", password->value );

      computeHmac(integrity.hash, buf, (int)(ptr-buf) , password->value, password->sizeValue);
      ptr = encodeAtrIntegrity(ptr, &integrity);
   }
	
   encode16(lengthp, (UInt16)(ptr - buf - sizeof(StunMsgHdr)));
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
      UInt64 tick;
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
#elif defined(__GNUC__) && ( defined(__i686__) || defined(__i386__) )
      asm("rdtsc" : "=A" (tick));
#elif defined(__GNUC__) && defined(__amd64__)
      asm("rdtsc" : "=A" (tick));
#elif defined (__SUNPRO_CC) && defined( __sparc__ )	
      tick = gethrtime();
#elif defined(__MACH__) 
      {
	int fd=open("/dev/random",O_RDONLY);
	read(fd,&tick,sizeof(tick));
	closesocket(fd);
      }
#elif defined(__linux) 
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
static void
computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey)
{
   strncpy(hmac,"hmac-not-implemented",20);
}
#else
#include <openssl/hmac.h>

static void
computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey)
{
   unsigned int resultSize=0;
   HMAC(EVP_sha1(), 
        key, sizeKey, 
        (const unsigned char*) input, length, 
        (unsigned char*)hmac, &resultSize);
   /*
     HMAC(EVP_sha1(), 
        key, sizeKey, 
        reinterpret_cast<const unsigned char*>(input), length, 
        reinterpret_cast<unsigned char*>(hmac), &resultSize);
	//assert(resultSize == 20);
   */
}
#endif


static void
toHex(const char* buffer, int bufferSize, char* output) 
{
   int i;
   static char hexmap[] = "0123456789abcdef";
	
   const char* p = buffer;
   char* r = output;
   for (i=0; i < bufferSize; i++)
   {
      unsigned char temp = *p++;
		
      int hi = (temp & 0xf0)>>4;
      int low = (temp & 0xf);
		
      *r++ = hexmap[hi];
      *r++ = hexmap[low];
   }
   *r = 0;
}

void
stunCreateUserName(const StunAddress4* source, StunAtrString* username)
{
   UInt64 time = stunGetSystemTimeSecs();
   UInt64 lotime;
   char buffer[1024];
   char hmac[20];
   char key[] = "Jason";
   char hmacHex[41];
   int l;

   time -= (time % 20*60);
   /* UInt64 hitime = time >> 32; */
   lotime = time & 0xFFFFFFFF;

   sprintf(buffer,
           "%08x:%08x:%08x:", 
           (UInt32)(source->addr),
           (UInt32)(stunRand()),
           (UInt32)(lotime));
   /*assert( strlen(buffer) < 1024 ); */
	
   /*assert(strlen(buffer) + 41 < STUN_MAX_STRING); */
   
   computeHmac(hmac, buffer, strlen(buffer), key, strlen(key) );
   toHex(hmac, 20, hmacHex );
   hmacHex[40] =0;
	
   strcat(buffer,hmacHex);
	
   l = strlen(buffer);
   /* assert( l+1 < STUN_MAX_STRING );*/
   /* assert( l%4 == 0 ); */
   
   username->sizeValue = l;
   memcpy(username->value,buffer,l);
   username->value[l]=0;
	
   /* if (verbose) ortp_message("stun: computed username=%s\n", username.value ); */
}

void
stunCreatePassword(const StunAtrString *username, StunAtrString* password)
{
   char hmac[20];
   char key[] = "Fluffy";
   /* char buffer[STUN_MAX_STRING]; */
   computeHmac(hmac, username->value, strlen(username->value), key, strlen(key));
   toHex(hmac, 20, password->value);
   password->sizeValue = 40;
   password->value[40]=0;
	
   /* ortp_message("stun: password=%s\n", password->value ); */
}


UInt64
stunGetSystemTimeSecs(void)
{
   UInt64 time=0;
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
stunParseHostName( char* peerName,
                   UInt32* ip,
                   UInt16* portVal,
                   UInt16 defaultPort )
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
stunParseServerName( char* name, StunAddress4 *addr)
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
   response->msgHdr.msgType = BindErrorResponseMsg;
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

static void
stunCreateSharedSecretResponse(const StunMessage *request, const StunAddress4 *source, StunMessage *response)
{
   response->msgHdr.msgType = SharedSecretResponseMsg;
   response->msgHdr.id = request->msgHdr.id;
	
   response->hasUsername = TRUE;
   stunCreateUserName( source, &response->username);
	
   response->hasPassword = TRUE;
   stunCreatePassword( &response->username, &response->password);
}


/* This funtion takes a single message sent to a stun server, parses
   and constructs an apropriate repsonse - returns TRUE if message is
   valid */
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
                      bool_t verbose)
{
   int i;
   StunMessage req;
   StunAddress4 mapped;
   StunAddress4 respondTo;
   UInt32 flags;
   bool_t ok;
   /* set up information for default response */
	
   memset( &req, 0 , sizeof(req) );
   memset( resp, 0 , sizeof(*resp) );
	
   *changeIp = FALSE;
   *changePort = FALSE;
	
   ok = stunParseMessage( buf,bufLen, &req, verbose);
   
   if (!ok)      /* Complete garbage, drop it on the floor */
   {
      if (verbose) ortp_error("stun: Request did not parse");
      return FALSE;
   }
   if (verbose) ortp_message("stun: Request parsed ok");
	
   mapped = req.mappedAddress.ipv4;
   respondTo = req.responseAddress.ipv4;
   flags = req.changeRequest.value;
	
   if (req.msgHdr.msgType==SharedSecretRequestMsg)
   {
         if(verbose) ortp_message("stun: Received SharedSecretRequestMsg on udp. send error 433.");
         /* !cj! - should fix so you know if this came over TLS or UDP */
         stunCreateSharedSecretResponse(&req, from, resp);
         /* stunCreateSharedSecretErrorResponse(*resp, 4, 33, "this request must be over TLS"); */
         return TRUE;
			
   }
   else if (req.msgHdr.msgType==BindRequestMsg)
   {
         if (!req.hasMessageIntegrity)
         {
            if (verbose) ortp_message("stun: BindRequest does not contain MessageIntegrity");
				
            if (0) /* !jf! mustAuthenticate */
            {
               if(verbose) ortp_message("stun: Received BindRequest with no MessageIntegrity. Sending 401.");
               stunCreateErrorResponse(resp, 4, 1, "Missing MessageIntegrity");
               return TRUE;
            }
         }
         else
         {
            if (!req.hasUsername)
            {
               if (verbose) ortp_message("stun: No UserName. Send 432.");
               stunCreateErrorResponse(resp, 4, 32, "No UserName and contains MessageIntegrity");
               return TRUE;
            }
            else
            {
               if (verbose) ortp_message("stun: Validating username: %s", req.username.value );
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
                     if (verbose) ortp_message("stun: Validating MessageIntegrity");
                     /* need access to shared secret */

#ifndef NOSSL
                     {
                        unsigned int hmacSize=20;

                        HMAC(EVP_sha1(), 
                             "1234", 4, 
                             (const unsigned char*) buf, bufLen-20-4, 
                             hmac, &hmacSize);
                        /*HMAC(EVP_sha1(), 
                             "1234", 4, 
                             reinterpret_cast<const unsigned char*>(buf), bufLen-20-4, 
                             hmac, &hmacSize);
                        //assert(hmacSize == 20);
			*/
                     }
#endif
							
                     if (memcmp(buf, hmac, 20) != 0)
                     {
                        if (verbose) ortp_warning("stun: MessageIntegrity is bad. Sending ");
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
                  if (verbose) ortp_message("stun: Invalid username: %s Send 430", req.username.value); 
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
			
         if (verbose)
         {
            ortp_message("stun: Request is valid:\n");
            ortp_message("stun: \t flags= %i\n", flags );
            ortp_message("stun: \t changeIp= %i\n", *changeIp );
            ortp_message("stun: \t changePort=%i\n", *changePort );
            ortp_message("stun: \t from= %i\n", from->addr );
            ortp_message("stun: \t respond to= %i\n", respondTo.addr );
            ortp_message("stun: \t mapped= %i\n", mapped.addr );
         }
				
         /* form the outgoing message */
         resp->msgHdr.msgType = BindResponseMsg;
         for (i=0; i<16; i++ )
         {
            resp->msgHdr.id.octet[i] = req.msgHdr.id.octet[i];
         }
		
         if ( req.xorOnly == FALSE )
         {
            resp->hasMappedAddress = TRUE;
            resp->mappedAddress.ipv4.port = mapped.port;
            resp->mappedAddress.ipv4.addr = mapped.addr;
         }

         if (1) /* do xorMapped address or not */
         {
            UInt16 id16;
            UInt32 id32;
            resp->hasXorMappedAddress = TRUE;
            id16 = req.msgHdr.id.octet[7]<<8 
               | req.msgHdr.id.octet[6];
            id32 = req.msgHdr.id.octet[7]<<24 
               |  req.msgHdr.id.octet[6]<<16 
               |  req.msgHdr.id.octet[5]<<8 
               | req.msgHdr.id.octet[4];
            resp->xorMappedAddress.ipv4.port = mapped.port^id16;
            resp->xorMappedAddress.ipv4.addr = mapped.addr^id32;
         }
         
         resp->hasSourceAddress = TRUE;
         resp->sourceAddress.ipv4.port = (*changePort) ? altAddr->port : myAddr->port;
         resp->sourceAddress.ipv4.addr = (*changeIp)   ? altAddr->addr : myAddr->addr;
			
         resp->hasChangedAddress = TRUE;
         resp->changedAddress.ipv4.port = altAddr->port;
         resp->changedAddress.ipv4.addr = altAddr->addr;
	
         if ( secondary->port != 0 )
         {
            resp->hasSecondaryAddress = TRUE;
            resp->secondaryAddress.ipv4.port = secondary->port;
            resp->secondaryAddress.ipv4.addr = secondary->addr;
         }
         
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
            const char serverName[] = "Vovida.org " STUN_VERSION; /* must pad to mult of 4 */
            resp->hasServerName = TRUE;
            
            /* assert( sizeof(serverName) < STUN_MAX_STRING ); */
            /* cerr << "sizeof serverName is "  << sizeof(serverName) ); */
            /* assert( sizeof(serverName)%4 == 0 ); */
            memcpy( resp->serverName.value, serverName, sizeof(serverName));
            resp->serverName.sizeValue = sizeof(serverName);
         }
         
         if ( req.hasMessageIntegrity & req.hasUsername )  
         {
            /* this creates the password that will be used in the HMAC when then */
            /* messages is sent */
            stunCreatePassword( &req.username, hmacPassword );
         }
				
         if (req.hasUsername && (req.username.sizeValue > 64 ) )
         {
            UInt32 source;
            /* assert( sizeof(int) == sizeof(UInt32) ); */
					
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
         if (verbose) ortp_error("stun: Unknown or unsupported request ");
         return FALSE;
   }
	
   /* assert(0); */
   return FALSE;
}

bool_t
stunInitServer(StunServerInfo *info, const StunAddress4 *myAddr, const StunAddress4 *altAddr, int startMediaPort, bool_t verbose )
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
   
   if ((info->myFd = openPort(myAddr->port, myAddr->addr,verbose)) == INVALID_SOCKET)
   {
      ortp_error("stun: Can't open %i\n", myAddr->addr );
      stunStopServer(info);

      return FALSE;
   }
   /*if (verbose) ortp_message("stun: Opened " << myAddr->addr << ":" << myAddr->port << " --> " << info->myFd ); */

   if ((info->altPortFd = openPort(altAddr->port,myAddr->addr,verbose)) == INVALID_SOCKET)
   {
      ortp_error("stun: Can't open %i\n", myAddr->addr );
      stunStopServer(info);
      return FALSE;
   }
   /* if (verbose) ortp_message("stun: Opened " << myAddr->addr << ":" << altAddr->port << " --> " << info->altPortFd ); */
   
   
   info->altIpFd = INVALID_SOCKET;
   if (  altAddr->addr != 0 )
   {
      if ((info->altIpFd = openPort( myAddr->port, altAddr->addr,verbose)) == INVALID_SOCKET)
      {
         ortp_error("stun: Can't open %i\n", altAddr->addr );
         stunStopServer(info);
         return FALSE;
      }
      /* if (verbose) ortp_message("stun: Opened " << altAddr->addr << ":" << myAddr->port << " --> " << info->altIpFd ); */
   }
   
   info->altIpPortFd = INVALID_SOCKET;
   if (  altAddr->addr != 0 )
   {  if ((info->altIpPortFd = openPort(altAddr->port, altAddr->addr,verbose)) == INVALID_SOCKET)
      {
         ortp_error("stun: Can't open %i\n", altAddr->addr );
         stunStopServer(info);
         return FALSE;
      }
   /* if (verbose) ortp_message("stun: Opened " << altAddr->addr << ":" << altAddr->port << " --> " << info->altIpPortFd ); */
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

#if 0 /* no usefull here */

bool_t
stunServerProcess(StunServerInfo *info, bool_t verbose)
{
   char msg[STUN_MAX_MESSAGE_SIZE];
   int msgLen = sizeof(msg);
   	
   bool_t ok = FALSE;
   bool_t recvAltIp =FALSE;
   bool_t recvAltPort = FALSE;
	
   fd_set fdSet; 
#if	defined(_WIN32) || defined(_WIN32_WCE)
   unsigned int maxFd=0;
#else
   int maxFd=0;
#endif
   struct timeval tv;
   int e;

   FD_ZERO(&fdSet); 
   FD_SET(info->myFd,&fdSet); 
   if ( info->myFd >= maxFd ) maxFd=info->myFd+1;
   FD_SET(info->altPortFd,&fdSet); 
   if ( info->altPortFd >= maxFd ) maxFd=info->altPortFd+1;

   if ( info->altIpFd != INVALID_SOCKET )
   {
      FD_SET(info->altIpFd,&fdSet);
      if (info->altIpFd>=maxFd) maxFd=info->altIpFd+1;
   }
   if ( info->altIpPortFd != INVALID_SOCKET )
   {
      FD_SET(info->altIpPortFd,&fdSet);
      if (info->altIpPortFd>=maxFd) maxFd=info->altIpPortFd+1;
   }

   if (info->relay)
   {
      int i;
      for (i=0; i<MAX_MEDIA_RELAYS; ++i)
      {
         StunMediaRelay* relay = &info->relays[i];
         if (relay->fd)
         {
            FD_SET(relay->fd, &fdSet);
            if (relay->fd >= maxFd) maxFd=relay->fd+1;
         }
      }
   }
   
   if ( info->altIpFd != INVALID_SOCKET )
   {
      FD_SET(info->altIpFd,&fdSet);
      if (info->altIpFd>=maxFd) maxFd=info->altIpFd+1;
   }
   if ( info->altIpPortFd != INVALID_SOCKET )
   {
      FD_SET(info->altIpPortFd,&fdSet);
      if (info->altIpPortFd>=maxFd) maxFd=info->altIpPortFd+1;
   }
   
   tv.tv_sec = 0;
   tv.tv_usec = 1000;
	
   e = select( maxFd, &fdSet, NULL,NULL, &tv );
   if (e < 0)
   {
      int err = getErrno();
#if !defined(_WIN32_WCE)
      ortp_error("stun: Error on select: %s\n",  strerror(err) );
#else
      ortp_error("stun: Error on select: %i\n",  err );
#endif
   }
   else if (e >= 0)
   {
      StunAddress4 from;
      int relayPort = 0;

      bool_t changePort = FALSE;
      bool_t changeIp = FALSE;
		
      StunMessage resp;
      StunAddress4 dest;
      StunAtrString hmacPassword;  

      StunAddress4 secondary;
               
      char buf[STUN_MAX_MESSAGE_SIZE];
      int len = sizeof(buf);

      hmacPassword.sizeValue = 0;
      secondary.port = 0;
      secondary.addr = 0;

      /* do the media relaying */
      if (info->relay)
      {
         time_t now;
         int i;
#if !defined(_WIN32_WCE)
         now = time(0);
#else
         DWORD timemillis = GetTickCount();
         now = timemillis/1000;
#endif
         for (i=0; i<MAX_MEDIA_RELAYS; ++i)
         {
            StunMediaRelay* relay = &info->relays[i];
            if (relay->fd)
            {
               if (FD_ISSET(relay->fd, &fdSet))
               {
                  char msg[MAX_RTP_MSG_SIZE];
                  int msgLen = sizeof(msg);
                  
                  StunAddress4 rtpFrom;
                  ok = getMessage( relay->fd, msg, &msgLen, &rtpFrom.addr, &rtpFrom.port ,verbose);
                  if (ok)
                  {
                     sendMessage(info->myFd, msg, msgLen, relay->destination.addr, relay->destination.port, verbose);
                     relay->expireTime = now + MEDIA_RELAY_TIMEOUT;
                     if ( verbose ) ortp_message("stun: Relay packet on %i from %i -> %i",
                                           relay->fd,
                                           rtpFrom.addr,
                                           relay->destination.addr 
                                         );
                  }
               }
               else if (now > relay->expireTime)
               {
                  closesocket(relay->fd);
                  relay->fd = 0;
               }
            }
         }
      }
      
     
      if (FD_ISSET(info->myFd,&fdSet))
      {
         if (verbose) ortp_message("stun: received on A1:P1");
         recvAltIp = FALSE;
         recvAltPort = FALSE;
         ok = getMessage( info->myFd, msg, &msgLen, &from.addr, &from.port,verbose );
      }
      else if (FD_ISSET(info->altPortFd, &fdSet))
      {
         if (verbose) ortp_message("stun: received on A1:P2");
         recvAltIp = FALSE;
         recvAltPort = TRUE;
         ok = getMessage( info->altPortFd, msg, &msgLen, &from.addr, &from.port,verbose );
      }
      else if ( (info->altIpFd!=INVALID_SOCKET) && FD_ISSET(info->altIpFd,&fdSet))
      {
         if (verbose) ortp_message("stun: received on A2:P1");
         recvAltIp = TRUE;
         recvAltPort = FALSE;
         ok = getMessage( info->altIpFd, msg, &msgLen, &from.addr, &from.port ,verbose);
      }
      else if ( (info->altIpPortFd!=INVALID_SOCKET) && FD_ISSET(info->altIpPortFd, &fdSet))
      {
         if (verbose) ortp_message("stun: received on A2:P2");
         recvAltIp = TRUE;
         recvAltPort = TRUE;
         ok = getMessage( info->altIpPortFd, msg, &msgLen, &from.addr, &from.port,verbose );
      }
      else
      {
         return TRUE;
      }

      if (info->relay)
      {
         int i;
         for (i=0; i<MAX_MEDIA_RELAYS; ++i)
         {
            StunMediaRelay* relay = &info->relays[i];
            if (relay->destination.addr == from.addr && 
                relay->destination.port == from.port)
            {
               relayPort = relay->relayPort;
               relay->expireTime = time(0) + MEDIA_RELAY_TIMEOUT;
               break;
            }
         }

         if (relayPort == 0)
         {
            int i;
            for (i=0; i<MAX_MEDIA_RELAYS; ++i)
            {
               StunMediaRelay* relay = &info->relays[i];
               if (relay->fd == 0)
               {
                  if ( verbose ) ortp_message("stun: Open relay port %i\n", relay->relayPort );
                  relay->fd = openPort(relay->relayPort, info->myAddr.addr, verbose);
                  relay->destination.addr = from.addr;
                  relay->destination.port = from.port;
                  relay->expireTime = time(0) + MEDIA_RELAY_TIMEOUT;
                  relayPort = relay->relayPort;
                  break;
               }
            }
         }
      }
         
      if ( !ok ) 
      {
         if ( verbose ) ortp_message("stun: Get message did not return a valid message\n");
         return TRUE;
      }
		
      if ( verbose ) ortp_message("stun: Got a request (len=%i) from %i", msgLen, from.addr);
		
      if ( msgLen <= 0 )
      {
         return TRUE;
      }
		
      if (info->relay && relayPort)
      {
         secondary = from;
         
         from.addr = info->myAddr.addr;
         from.port = relayPort;
      }
      
      ok = stunServerProcessMsg( msg, msgLen, &from, &secondary,
                                 recvAltIp ? &info->altAddr : &info->myAddr,
                                 recvAltIp ? &info->myAddr : &info->altAddr, 
                                 &resp,
                                 &dest,
                                 &hmacPassword,
                                 &changePort,
                                 &changeIp,
                                 verbose );
		
      if ( !ok )
      {
         if ( verbose ) ortp_error("stun: Failed to parse message");
         return TRUE;
      }
		
      len = stunEncodeMessage( &resp, buf, len, &hmacPassword,verbose );
		
      if ( dest.addr == 0 )  ok=FALSE;
      if ( dest.port == 0 ) ok=FALSE;
		
      if ( ok )
      {
         /* assert( dest.addr != 0 ); */
         /* assert( dest.port != 0 ); */
			
         Socket sendFd;
			
         bool_t sendAltIp   = recvAltIp;   /* send on the received IP address */
         bool_t sendAltPort = recvAltPort; /* send on the received port */
			
         if ( changeIp )   sendAltIp   = !sendAltIp; /* if need to change IP, then flip logic */
         if ( changePort ) sendAltPort = !sendAltPort; /* if need to change port, then flip logic */
			
         if ( !sendAltPort )
         {
            if ( !sendAltIp )
            {
               sendFd = info->myFd;
            }
            else
            {
               sendFd = info->altIpFd;
            }
         }
         else
         {
            if ( !sendAltIp )
            {
               sendFd = info->altPortFd;
            }
            else
            {
               sendFd = info->altIpPortFd;
            }
         }
	
         if ( sendFd != INVALID_SOCKET )
         {
            sendMessage( sendFd, buf, len, dest.addr, dest.port, verbose );
         }
      }
   }
   
   return TRUE;
}
#endif

int 
stunFindLocalInterfaces(UInt32* addresses,int maxRet)
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
   
      UInt32 ai;
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
	
   msg->msgHdr.msgType = BindRequestMsg;
	
   for ( i=0; i<16; i=i+4 )
   {
      /* assert(i+3<16); */
      int r = stunRand();
      msg->msgHdr.id.octet[i+0]= r>>0;
      msg->msgHdr.id.octet[i+1]= r>>8;
      msg->msgHdr.id.octet[i+2]= r>>16;
      msg->msgHdr.id.octet[i+3]= r>>24;
   }
	
   if ( id != 0 )
   {
      msg->msgHdr.id.octet[0] = id; 
   }
	
   msg->hasChangeRequest = TRUE;
   msg->changeRequest.value =(changeIp?ChangeIpFlag:0) | 
      (changePort?ChangePortFlag:0);
	
   if ( username->sizeValue > 0 )
   {
      msg->hasUsername = TRUE;
      /* msg->username = username; */
      memcpy(&msg->username, username, sizeof(StunAtrString));
   }
}


static void 
stunSendTest( Socket myFd, StunAddress4 *dest, 
              const StunAtrString *username, const StunAtrString *password, 
              int testNum, bool_t verbose )
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
	
   len = stunEncodeMessage( &req, buf, len, password,verbose );
	
   if ( verbose )
   {
      ortp_message("stun: About to send msg of len %i to %s\n", len, ipaddr(dest) );
   }
	
   sendMessage( myFd, buf, len, dest->addr, dest->port, verbose );
	
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


int 
stunTest( StunAddress4 *dest, int testNum, bool_t verbose, StunAddress4* sAddr , StunAddress4 *sMappedAddr, StunAddress4* sChangedAddr)
{
   /* assert( dest.addr != 0 ); */
   /* assert( dest.port != 0 ); */
	
   int port = randomPort();
   UInt32 interfaceIp=0;
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
   myFd = openPort(port,interfaceIp,verbose);
   if ( myFd == INVALID_SOCKET)
       return -1;
   
   username.sizeValue = 0;
   password.sizeValue = 0;
	
#ifdef USE_TLS
   stunGetUserNameAndPassword( dest, &username, &password );
#endif
	
   stunSendTest( myFd, dest, &username, &password, testNum, verbose );
   
   ok = getMessage( myFd,
               msg,
               &msgLen,
               &from.addr,
               &from.port,verbose );
   closesocket(myFd);
   if (!ok)
       return -1;

   memset(&resp, 0, sizeof(StunMessage));
	
   if ( verbose ) ortp_message("stun: Got a response");
   ok = stunParseMessage( msg,msgLen, &resp,verbose );
	
   if ( verbose )
   {
      ortp_message("stun: \t ok=%i\n", ok );
#if defined(WIN32) || defined(_WIN32_WCE)
      ortp_message("stun: \t id=%u\n", *(unsigned int*)&resp.msgHdr.id );
#endif
      ortp_message("stun: \t mappedAddr=%i\n", resp.mappedAddress.ipv4.addr );
      ortp_message("stun: \t changedAddr=%i\n", resp.changedAddress.ipv4.addr );
   }
	
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
             bool_t verbose,
             bool_t* preservePort, /* if set, is return for if NAT preservers ports or not */
             bool_t* hairpin,  /* if set, is the return for if NAT will hairpin packets */
             int port, /* port to use for the test, 0 to choose random port */
             StunAddress4* sAddr /* NIC to use */
   )
{ 
   /* assert( dest.addr != 0 ); */
   /* assert( dest.port != 0 ); */
   UInt32 interfaceIp=0;
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
   UInt64 second_started;
   UInt64 second_elapsed;
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
   myFd1 = openPort(port,interfaceIp,verbose);
   myFd2 = openPort(port+1,interfaceIp,verbose);

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
	
#ifdef USE_TLS 
   stunGetUserNameAndPassword( dest, username, password );
#endif
	
   /* stunSendTest( myFd1, dest, username, password, 1, verbose ); */

   
   second_started = stunGetSystemTimeSecs();
   second_elapsed = 1;

   while ( count < 7 && second_elapsed < 5)
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
            stunSendTest( myFd1, dest, &username, &password, 1 ,verbose );
         }         
			
         if ( (!respTestI2) && respTestI ) 
         {
            /* check the address to send to if valid */
            if (  ( testI2dest.addr != 0 ) &&
                  ( testI2dest.port != 0 ) )
            {
               stunSendTest( myFd1, &testI2dest, &username, &password, 10  ,verbose);
            }
         }
			
         if ( !respTestII )
         {
            stunSendTest( myFd2, dest, &username, &password, 2 ,verbose );
         }
			
         if ( !respTestIII )
         {
            stunSendTest( myFd2, dest, &username, &password, 3 ,verbose );
         }
			
         if ( respTestI && (!respTestHairpin) )
         {
            if (  ( testImappedAddr.addr != 0 ) &&
                  ( testImappedAddr.port != 0 ) )
            {
               stunSendTest( myFd1, &testImappedAddr, &username, &password, 11 ,verbose );
            }
         }
      }
      else
      {
         int i;
         /* if (verbose) ortp_message("stun: -----------------------------------------"); */
         /* assert( err>0 ); */
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
                              &from.port,verbose );
                  
                  memset(&resp, 0, sizeof(StunMessage));
						
                  stunParseMessage( msg,msgLen, &resp,verbose );
						
                  if ( verbose )
                  {
                     ortp_message("stun: Received message of type %i id=%i\n",
                            resp.msgHdr.msgType,
                            (int)(resp.msgHdr.id.octet[0]) );
                  }
						
                  switch( resp.msgHdr.id.octet[0] )
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
   s = openPort( 0/*use ephemeral*/, testImappedAddr.addr, FALSE );
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
	
   if (verbose)
   {
      ortp_message("stun: test I = %i\n", respTestI );
      ortp_message("stun: test II = %i\n", respTestII );
      ortp_message("stun: test III = %i\n", respTestIII );
      ortp_message("stun: test I(2) = %i\n", respTestI2 );
      ortp_message("stun: is nat  = %i\n", isNat);
      ortp_message("stun: mapped IP same = %i\n", mappedIpSame );
   }
	
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
                int port, StunAddress4* srcAddr, 
                bool_t verbose )
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
   
   myFd = openPort(port,interfaceIp,verbose);
   if (myFd == INVALID_SOCKET)
   {
      return myFd;
   }
   
   username.sizeValue = 0;
   password.sizeValue = 0;
	
#ifdef USE_TLS
   stunGetUserNameAndPassword( dest, username, password );
#endif
	
   stunSendTest(myFd, dest, &username, &password, 1, 0/*FALSE*/ );
	
   getMessage( myFd, msg, &msgLen, &from.addr, &from.port,verbose );

   memset(&resp, 0, sizeof(StunMessage));
	
   ok = stunParseMessage( msg, msgLen, &resp,verbose );
   if (!ok)
   {
       closesocket(myFd);
       return -1;
   }
	
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
                   int port, StunAddress4* srcAddr, 
                   bool_t verbose )
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
      fd[i] = openPort( (port == 0) ? 0 : (port + i), 
                        interfaceIp, verbose);
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
	
#ifdef USE_TLS
   stunGetUserNameAndPassword( dest, username, password );
#endif
	
   for( i=0; i<NUM; i++)
   {
      stunSendTest(fd[i], dest, &username, &password, 1/*testNum*/, verbose );
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
                  &from.port ,verbose);
      
      memset(&resp, 0, sizeof(StunMessage));
		
      ok = stunParseMessage( msg, msgLen, &resp, verbose );
      if (!ok) 
      {  
          for( i=0; i<NUM; i++)
          {
              closesocket(fd[i]);
          }
         return FALSE;
      }
	  
      mappedAddr[i] = resp.mappedAddress.ipv4;
   }
	
   if (verbose)
   {
      ortp_message("stun: --- stunOpenSocketPair --- \n");
      for( i=0; i<NUM; i++)
      {
         ortp_message("stun: \t mappedAddr=%s\n", ipaddr(&mappedAddr[i]) );
      }
   }
	
#if 0
   if ( mappedAddr[0].port %2 == 0 )
   {
      if (  mappedAddr[0].port+1 ==  mappedAddr[1].port )
      {
         *mapAddr = mappedAddr[0];
         *fd1 = fd[0];
         *fd2 = fd[1];
         closesocket( fd[2] );
         return TRUE;
      }
   }
   else
   {
      if (( mappedAddr[1].port %2 == 0 )
          && (  mappedAddr[1].port+1 ==  mappedAddr[2].port ))
      {
         *mapAddr = mappedAddr[1];
         *fd1 = fd[1];
         *fd2 = fd[2];
         closesocket( fd[0] );
         return TRUE;
      }
   }
#else
    *mapAddr_rtp = mappedAddr[0];
    *mapAddr_rtcp = mappedAddr[1];
    *fd1 = fd[0];
    *fd2 = fd[1];
#endif

   /* something failed, close all and return error */
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

