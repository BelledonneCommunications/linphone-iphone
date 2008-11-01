/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

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


/* mtu.c : discover the mtu automatically */

#include "mediastreamer2/mscommon.h"


#if defined(WIN32) && !defined(_WIN32_WCE)

HINSTANCE m_IcmpInst = NULL;

typedef struct ip_option_information {
    UCHAR   Ttl;
    UCHAR   Tos;
    UCHAR   Flags;
    UCHAR   OptionsSize;
    PUCHAR  OptionsData;
} IP_OPTION_INFORMATION, * PIP_OPTION_INFORMATION;

typedef BOOL (WINAPI *ICMPCLOSEHANDLE)(HANDLE IcmpHandle);
typedef HANDLE (WINAPI *ICMPCREATEFILE)(VOID);
typedef DWORD (WINAPI *ICMPSENDECHO)(HANDLE IcmpHandle,ULONG DestinationAddress, LPVOID RequestData, WORD RequestSize, PIP_OPTION_INFORMATION RequestOptions, LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout);

ICMPCLOSEHANDLE pIcmpCloseHandle = NULL;
ICMPCREATEFILE pIcmpCreateFile = NULL;
ICMPSENDECHO pIcmpSendEcho = NULL;

#define IP_FLAG_DF      0x2         // Don't fragment this packet.
#define IP_OPT_ROUTER_ALERT 0x94  // Router Alert Option

#define IP_STATUS_BASE              11000
#define IP_PACKET_TOO_BIG           (IP_STATUS_BASE + 9)
#define IP_REQ_TIMED_OUT            (IP_STATUS_BASE + 10)

static int mtus[] = {
  1500,   // Ethernet, Point-to-Point (default)
  1492,   // IEEE 802.3
  1006,   // SLIP, ARPANET
  576,    // X.25 Networks
  544,    // DEC IP Portal
  512,    // NETBIOS
  508,    // IEEE 802/Source-Rt Bridge, ARCNET
  296,    // Point-to-Point (low delay)
  68,     // Official minimum
  0
};

int ms_discover_mtu(const char *host)
{
  int i;

	struct addrinfo hints,*ai=NULL;
	char port[10];
  char ipaddr[INET6_ADDRSTRLEN];
  int err;

  HANDLE hIcmp;
  unsigned long target_addr;

  struct ip_option_information ip_opts;
  unsigned char reply_buffer[10000];

  if (!m_IcmpInst)
	{
		m_IcmpInst = LoadLibrary("icmp.dll");
		if (m_IcmpInst)
		{
			pIcmpCloseHandle = (ICMPCLOSEHANDLE)GetProcAddress(m_IcmpInst, "IcmpCloseHandle");
			pIcmpCreateFile  = (ICMPCREATEFILE) GetProcAddress(m_IcmpInst, "IcmpCreateFile");
			pIcmpSendEcho =	   (ICMPSENDECHO)   GetProcAddress(m_IcmpInst, "IcmpSendEcho");
		}
	}

  hIcmp = pIcmpCreateFile();

	memset(&hints,0,sizeof(hints));
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	
	snprintf(port,sizeof(port),"0");
	err=getaddrinfo(host,port,&hints,&ai);
	if (err!=0){
    pIcmpCloseHandle( hIcmp );
		ms_error("getaddrinfo(): error\n");
		return -1;
	}
  getnameinfo (ai->ai_addr, ai->ai_addrlen, ipaddr, sizeof (ipaddr), port,
               sizeof (port), NI_NUMERICHOST | NI_NUMERICSERV);
	freeaddrinfo(ai);

  target_addr=inet_addr(ipaddr);


  /* Prepare the IP options */
  memset(&ip_opts,0,sizeof(ip_opts));
  ip_opts.Ttl=30;
  ip_opts.Flags = IP_FLAG_DF | IP_OPT_ROUTER_ALERT;


  // ignore icmpbuff data contents 
  for (i=0;mtus[i]!=0;i++)
  {
    char icmpbuff[2048];
    char *icmp_data = icmpbuff;

    int status = -1;
    if (pIcmpSendEcho)
      status=pIcmpSendEcho(hIcmp,
                          target_addr,
                          (LPVOID)icmp_data,
                          mtus[i]-60, /* icmp_data_size */
                          &ip_opts,
                          reply_buffer,
                          sizeof(reply_buffer),
                          3000L); // 3 seconds
    if (status || GetLastError() == IP_REQ_TIMED_OUT)
    {
      pIcmpCloseHandle( hIcmp );
      return mtus[i];
    }
  }

  pIcmpCloseHandle( hIcmp );

  return -1;
}

#elif defined(__linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>

#ifndef IP_MTU
#define IP_MTU 14
#endif

int ms_discover_mtu(const char *host){
	int sock;
	int err,mtu=0,new_mtu;
	socklen_t optlen;
	char buf[1500-28]={0};
	char port[10];
	struct addrinfo hints,*ai=NULL;
	int rand_port;
	int retry=0;
	struct timeval tv;

	memset(&hints,0,sizeof(hints));
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	
	gettimeofday(&tv,NULL);	
	srandom(tv.tv_usec);
	rand_port=random() & 0xFFFF;
	if (rand_port<1000) rand_port+=1000;
	snprintf(port,sizeof(port),"%i",rand_port);
	err=getaddrinfo(host,port,&hints,&ai);
	if (err!=0){
		ms_error("getaddrinfo(): %s\n",gai_strerror(err));
		return -1;
	}
	sock=socket(PF_INET,SOCK_DGRAM,0);

	mtu=IP_PMTUDISC_DO;
	optlen=sizeof(mtu);
	err=setsockopt(sock,IPPROTO_IP,IP_MTU_DISCOVER,&mtu,optlen);
	if (err!=0){
		ms_error("setsockopt(): %s",strerror(errno));
		return -1;
	}
	err=connect(sock,ai->ai_addr,ai->ai_addrlen);
	freeaddrinfo(ai);
	if (err!=0){
		ms_error("connect(): %s",strerror(errno));
		return -1;
	}
	mtu=sizeof(buf);
	do{
		send(sock,buf,mtu,0);
    usleep(500000);/*wait for an icmp message come back */
    err=getsockopt(sock,IPPROTO_IP,IP_MTU,&new_mtu,&optlen);
		if (err!=0){
			ms_error("getsockopt(): %s",strerror(errno));
			return -1;
		}else{
			ms_message("Partial MTU discovered : %i",new_mtu);
			if (new_mtu==mtu) break;
			else mtu=new_mtu;
		}
		retry++;
	}while(retry<10);
	
	ms_message("mtu to %s is %i",host,mtu);
	return mtu;
}

#else

int ms_discover_mtu(const char*host){
	ms_warning("mtu discovery not implemented.");
	return -1;
}

#endif


void ms_set_mtu(int mtu){
	/*60= IPv6+UDP+RTP overhead */
	if (mtu>60){
		if (mtu>1500) mtu=1500;/*limit to 1500, the mediastreamer2 buffer are not large enough anyway*/
		ms_set_payload_max_size(mtu-60);
	}else ms_set_payload_max_size(0);
}
