/*
  The mediastreamer library aims at providing modular media processing and I/O
	for linphone, but also for any telephony application.
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

#ifndef mstcpclient_h
#define mstcpclient_h

#include "msfilter.h"

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


struct _MSTcpClient{
	MSFilter parent;
	MSQueue *q_outputs[1];
	int sock;
	MSMessage *msg;
};

typedef struct _MSTcpClient MSTcpClient;

struct _MSTcpClientClass{
	MSFilterClass parent;
};

typedef struct _MSTcpClientClass MSTcpClientClass;

MSFilter *ms_tcp_client_new();
int ms_tcp_client_connect(MSTcpClient *obj, const char *addr, int port);
#define MS_TCP_CLIENT(o)	((MSTcpClient*)(o))
#endif
