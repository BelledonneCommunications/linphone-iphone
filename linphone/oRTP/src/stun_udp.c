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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if !defined(WIN32) && !defined(_WIN32_WCE)
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
#include <assert.h>

#include <time.h>

#if defined(WIN32) || defined(_WIN32_WCE)

#include <winsock2.h>
#include <stdlib.h>
/* #include <io.h> */

#else

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#endif

#include <string.h>

#include "ortp/stun_udp.h"
#include "ortp/ortp.h"

#if !defined(WIN32) && !defined(_WIN32_WCE)
int getErrno() { return errno; }
#else
int getErrno() { return WSAGetLastError(); }
#endif

Socket
openPort( unsigned short port, unsigned int interfaceIp )
{
   struct sockaddr_in addr;
   Socket fd;
    
   fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if ( fd == INVALID_SOCKET )
   {
	  ortp_error("stun_udp: Could not create a UDP socket");
      return INVALID_SOCKET;
   }
   
   memset((char*) &(addr),0, sizeof((addr)));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(port);
    
   if ( (interfaceIp != 0) && 
        ( interfaceIp != 0x100007f ) )
   {
      addr.sin_addr.s_addr = htonl(interfaceIp);
      //ortp_debug("Binding to interface 0x%lu\n",(unsigned long) htonl(interfaceIp));
   }
	
   if ( bind( fd,(struct sockaddr*)&addr, sizeof(addr)) != 0 )
   {
      int e = getErrno();
        
      switch (e)
      {
         case 0:
         {
            ortp_error("stun_udp: Could not bind socket");;
            return INVALID_SOCKET;
         }
         case EADDRINUSE:
         {
            ortp_error("stun_udp: Port %i for receiving UDP is in use", port);
            return INVALID_SOCKET;
         }
         break;
         case EADDRNOTAVAIL:
         {
            ortp_error("stun_udp: Cannot assign requested address");
            return INVALID_SOCKET;
         }
         break;
         default:
         {
#if !defined(_WIN32_WCE)
			 ortp_error("stun_udp: Could not bind UDP receive port Error=%i %s",
                   e, strerror(e));
#else
			 ortp_error("stun_udp: Could not bind UDP receive port Error=%i",
                   e);
#endif
            return INVALID_SOCKET;
         }
         break;
      }
   }

	 ortp_debug("stun: opened port %i with fd %i\n", port, fd);
   
   /* assert( fd != INVALID_SOCKET  ); */
	
   return fd;
}


bool_t 
getMessage( Socket fd, char* buf, int* len,
            unsigned int* srcIp, unsigned short* srcPort)
{
   /* assert( fd != INVALID_SOCKET ); */
	
   int originalSize = *len;
   struct sockaddr_in from;
   int fromLen = sizeof(from);
   	

   int err;
   struct timeval tv;
   fd_set fdSet; 
#if defined(WIN32) || defined(_WIN32_WCE)
   unsigned int fdSetSize;
#else
   int fdSetSize;
#endif

   if (originalSize <= 0)
   {
	   return FALSE;
   }

   tv.tv_sec=1;
   tv.tv_usec=0; /* 150 ms */
   FD_ZERO(&fdSet); fdSetSize=0;
   FD_SET(fd,&fdSet); fdSetSize = fd+1;

   err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
   if ( err == SOCKET_ERROR )
   {
      int e = getErrno();
      switch (e)
      {
         case ENOTSOCK:
            ortp_error("stun_udp: Error fd not a socket");
            break;
         case ECONNRESET:
            ortp_error("stun_udp: Error connection reset - host not reachable");
            break;
				
         default:
            ortp_error("stun_udp: Socket Error=%i", e);
      }
      return FALSE;
   }

    if (err==0)
    {
        ortp_error("stun_udp: Connection timeout with stun server!");
        *len = 0;
        return FALSE;
    }

    if (FD_ISSET (fd, &fdSet))
    {
        *len = recvfrom(fd,
                        buf,
                        originalSize,
                        0,
                        (struct sockaddr *)&from,
                        (socklen_t*)&fromLen);
        	
        if ( *len == SOCKET_ERROR )
        {
            int e = getErrno();
        		
            switch (e)
            {
                case ENOTSOCK:
                    ortp_error("stun_udp: Error fd not a socket");
                    break;
                case ECONNRESET:
                    ortp_error("stun_udp: Error connection reset - host not reachable");
                    break;
        				
                default:
                    ortp_error("stun_udp: Socket Error=%i", e);
            }
        		
            return FALSE;
        }
        	
        if ( *len < 0 )
        {
            ortp_error("stun_udp: socket closed? negative len");
            return FALSE;
        }
            
        if ( *len == 0 )
        {
            ortp_error("stun_udp: socket closed? zero len");
            return FALSE;
        }
            
        *srcPort = ntohs(from.sin_port);
        *srcIp = ntohl(from.sin_addr.s_addr);
        	
        if ( (*len)+1 >= originalSize )
        {
            ortp_error("stun_udp: Received a message that was too large");
            return FALSE;
        }
        buf[*len]=0;
            
        return TRUE;
    }
    return FALSE;
}


bool_t 
sendMessage( Socket fd, char* buf, int l, 
             unsigned int dstIp, unsigned short dstPort)
{
   int s;

   if (fd == INVALID_SOCKET)
	   return FALSE;

   if ( dstPort == 0 )
   {
      /* sending on a connected port */
      s = send(fd,buf,l,0);
   }
   else
   {
      struct sockaddr_in to;
      int toLen = sizeof(to);
      if (dstIp == 0)
	  {
		  ortp_error("stun_udp: invalid IP provided (dstIP==0)");
		  return FALSE;
	  }

      memset(&to,0,toLen);
        
      to.sin_family = AF_INET;
      to.sin_port = htons(dstPort);
      to.sin_addr.s_addr = htonl(dstIp);
        
      s = sendto(fd, buf, l, 0,(struct sockaddr*)&to, toLen);
   }
    
   if ( s == SOCKET_ERROR )
   {
      int e = getErrno();
      switch (e)
      {
         case ECONNREFUSED:
         case EHOSTDOWN:
         case EHOSTUNREACH:
         {
            /* quietly ignore this */
         }
         break;
         case EAFNOSUPPORT:
         {
            ortp_error("stun_udp: err EAFNOSUPPORT in send");
         }
         break;
         default:
         {
#if !defined(_WIN32_WCE)
            ortp_error("stun_udp: err %i %s in send", e, strerror(e));
#else
            ortp_error("stun_udp: err %i in send", e);
#endif
         }
      }
      return FALSE;
   }
    
   if ( s == 0 )
   {
      ortp_error("stun_udp: no data sent in send");
      return FALSE;
   }
    
   if ( s != l )
   {
      ortp_error("stun_udp: only %i out of %i bytes sent", s, l);
      return FALSE;
   }
    
   return TRUE;
}


void
initNetwork()
{
#if defined(WIN32) || defined(_WIN32_WCE)
   WORD wVersionRequested = MAKEWORD( 2, 2 );
   WSADATA wsaData;
   int err;
	
   err = WSAStartup( wVersionRequested, &wsaData );
   if ( err != 0 ) 
   {
      /* could not find a usable WinSock DLL */
      ortp_error("stun_udp: Could not load winsock");
   }
    
   /* Confirm that the WinSock DLL supports 2.2.*/
   /* Note that if the DLL supports versions greater    */
   /* than 2.2 in addition to 2.2, it will still return */
   /* 2.2 in wVersion since that is the version we      */
   /* requested.                                        */
    
   if ( LOBYTE( wsaData.wVersion ) != 2 ||
        HIBYTE( wsaData.wVersion ) != 2 ) 
   {
      /* Tell the user that we could not find a usable */
      /* WinSock DLL.                                  */
      WSACleanup( );
      ortp_error("stun_udp: Wrong winsock (!= 2.2) version");
   }    
#endif
}


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

/* Local Variables:
   mode:c
   c-file-style:"ellemtel"
   c-file-offsets:((case-label . +))
   indent-tabs-mode:nil
   End:
*/
