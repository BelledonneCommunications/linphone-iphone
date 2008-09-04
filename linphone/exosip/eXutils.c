/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2002, 2003  Aymeric MOIZARD  - jack@atosc.org
  
  eXosip is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  eXosip is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif

#include <osipparser2/osip_port.h>
#include "eXosip2.h"

extern eXosip_t eXosip;

#ifdef WIN32
/* You need the Platform SDK to compile this. */
#include <Windows.h>

#else /* sun, *BSD, linux, and other? */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/param.h>

#include <stdio.h>

#endif

int
eXosip_guess_ip_for_via (int family, char *address, int size){
	char *res=NULL;
	if (family==AF_INET6)
		eXosip_get_localip_for("2001:638:500:101:2e0:81ff:fe24:37c6",&res);
	else
		eXosip_get_localip_for("15.128.128.93",&res);
	strncpy(address,res,size);
	osip_free(res);
	return 0;
}



#ifdef SM

void eXosip_get_localip_from_via(osip_message_t *mesg,char **locip){
	osip_via_t *via=NULL;
	char *host;
	via=(osip_via_t*)osip_list_get(mesg->vias,0);
	if (via==NULL) {
		host="15.128.128.93";
		eXosip_trace(OSIP_ERROR,("Could not get via:%s"));
	}else host=via->host;
	eXosip_get_localip_for(host,locip);
	
}
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 1024
#endif

void eXosip_get_localip_for(char *address_to_reach,char **loc){
	int err,tmp;
	struct addrinfo hints;
	struct addrinfo *res=NULL,*res0=NULL;
	struct sockaddr_storage addr;
	int sock;
#ifdef __APPLE_CC__
	int s;
#else
	socklen_t s;
#endif
	
	if (eXosip.forced_localip){
		*loc=osip_strdup(eXosip.localip);
		return;
	}
	
	*loc=osip_malloc(MAXHOSTNAMELEN);
	if (eXosip.ip_family==AF_INET)
		strcpy(*loc,"127.0.0.1");  /* always fallback to local loopback */
	else strcpy(*loc,"::1"); 

	memset(&hints,0,sizeof(hints));
	hints.ai_family=(eXosip.ip_family==AF_INET) ? PF_INET:PF_INET6;
	hints.ai_socktype=SOCK_DGRAM;
	/*hints.ai_flags=AI_NUMERICHOST|AI_CANONNAME;*/
	err=getaddrinfo(address_to_reach,"5060",&hints,&res0);
	if (err!=0){
		eXosip_trace(OSIP_ERROR,("Error in getaddrinfo for %s: %s\n",address_to_reach,gai_strerror(err)));
		return ;
	}
	if (res0==NULL){
		eXosip_trace(OSIP_ERROR,("getaddrinfo reported nothing !"));
		abort();
		return ;
	}
	for (res=res0;res!=NULL;res=res->ai_next){
		sock=socket(res->ai_family,SOCK_DGRAM,0);
		tmp=1;
		err=setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&tmp,sizeof(int));
		if (err<0){
			eXosip_trace(OSIP_ERROR,("Error in setsockopt: %s\n",strerror(errno)));
			abort();
			return ;
		}
		err=connect(sock,res->ai_addr,res->ai_addrlen);
		if (err<0) {
			eXosip_trace(OSIP_ERROR,("Error in connect: %s\n",strerror(errno)));
 			close(sock);
			sock=-1;
			continue;
		}else break;
	}
	freeaddrinfo(res0);
	if (sock==-1){
		eXosip_trace(OSIP_WARNING,("Could not find interface to reach %s\n",address_to_reach));
		return;
	}
	res0=NULL;
	res=NULL;
	s=sizeof(addr);
	err=getsockname(sock,(struct sockaddr*)&addr,&s);
	if (err!=0) {
		eXosip_trace(OSIP_ERROR,("Error in getsockname: %s\n",strerror(errno)));
		close(sock);
		return ;
	}
	
	err=getnameinfo((struct sockaddr *)&addr,s,*loc,MAXHOSTNAMELEN,NULL,0,NI_NUMERICHOST);
	if (err!=0){
		eXosip_trace(OSIP_ERROR,("getnameinfo error:%s ; while finding local address for %s",strerror(errno), address_to_reach));
		abort();
		return ;
	}
	close(sock);
	eXosip_trace(OSIP_INFO1,("Outgoing interface to reach %s is %s.\n",address_to_reach,*loc));
	return ;
}

char *strdup_printf(const char *fmt, ...)
{
	/* Guess we need no more than 100 bytes. */
	int n, size = 100;
	char *p;
	va_list ap;
	if ((p = osip_malloc (size)) == NULL)
		return NULL;
	while (1)
	{
		/* Try to print in the allocated space. */
		va_start (ap, fmt);
#ifdef WIN32
		n = _vsnprintf (p, size, fmt, ap);
#else
		n = vsnprintf (p, size, fmt, ap);
#endif
		va_end (ap);
		/* If that worked, return the string. */
		if (n > -1 && n < size)
			return p;
		/* Else try again with more space. */
		if (n > -1)	/* glibc 2.1 */
			size = n + 1;	/* precisely what is needed */
		else		/* glibc 2.0 */
			size *= 2;	/* twice the old size */
		if ((p = realloc (p, size)) == NULL)
			return NULL;
	}
}

int
eXosip_get_addrinfo (struct addrinfo **addrinfo, char *hostname, int service)
{
#ifndef WIN32
  struct in_addr addr;
  struct in6_addr addrv6;
#else
  unsigned long int one_inet_addr;
#endif
  struct addrinfo hints;
  int error;
  char portbuf[10];
  if (service!=0)
    snprintf(portbuf, sizeof(portbuf), "%i", service);

  memset (&hints, 0, sizeof (hints));
#ifndef WIN32
 if (inet_pton(AF_INET, hostname, &addr)>0)
 {
   /* ipv4 address detected */
   hints.ai_flags = AI_NUMERICHOST;
   hints.ai_family = PF_INET;
   OSIP_TRACE (osip_trace
	       (__FILE__, __LINE__, OSIP_INFO2, NULL,
		"IPv4 address detected: %s\n", hostname));
 }
 else if (inet_pton(AF_INET6, hostname, &addrv6)>0)
 {
   /* ipv6 address detected */
   /* Do the resolution anyway */
   hints.ai_flags = AI_CANONNAME;
   hints.ai_family = PF_INET6;
   OSIP_TRACE (osip_trace
	       (__FILE__, __LINE__, OSIP_INFO2, NULL,
		"IPv6 address detected: %s\n", hostname));
 }
 else
 {
   /* hostname must be resolved */
   hints.ai_flags = 0;
   hints.ai_family = (eXosip.ip_family==AF_INET) ? PF_INET:PF_INET6;
   OSIP_TRACE (osip_trace
	       (__FILE__, __LINE__, OSIP_INFO2, NULL,
		"Not an IPv4 or IPv6 address: %s\n", hostname));
 }
#else
  if ((int)(one_inet_addr = inet_addr(hostname)) == -1)
    hints.ai_flags = AI_CANONNAME;
  else
    hints.ai_flags = AI_NUMERICHOST;

#ifdef IPV6_SUPPORT
  hints.ai_family = PF_UNSPEC; /* ipv6 support */
#else
  hints.ai_family = PF_INET;   /* ipv4 only support */
#endif

#endif

  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  if (service==0)
    {
      error = getaddrinfo (hostname, "sip", &hints, addrinfo);
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "SRV resolution with udp-sip-%s\n", hostname));
    }
  else
    {
      error = getaddrinfo (hostname, portbuf, &hints, addrinfo);
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "DNS resolution with %s:%i\n", hostname, service));
    }
  if (error || *addrinfo == NULL)
    { 
      OSIP_TRACE (osip_trace
		  (__FILE__, __LINE__, OSIP_INFO2, NULL,
		   "getaddrinfo failure. %s:%s (%s)\n", hostname, portbuf, gai_strerror(error)));
     return -1;
    }

  return 0;
}
