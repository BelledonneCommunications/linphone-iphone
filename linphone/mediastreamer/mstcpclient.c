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

#include "mstcpclient.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>


void ms_tcp_client_process(MSTcpClient *r);
void ms_tcp_client_init(MSTcpClient *r);
void ms_tcp_client_destroy(MSTcpClient *r);
void ms_tcp_client_class_init(MSTcpClientClass *klass);

static MSTcpClientClass *ms_tcp_client_class=NULL;

MSFilter * ms_tcp_client_new(void)
{
	MSTcpClient *r;
	
	r=g_new(MSTcpClient,1);
	
	if (ms_tcp_client_class==NULL)
	{
		ms_tcp_client_class=g_new(MSTcpClientClass,1);
		ms_tcp_client_class_init(ms_tcp_client_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_tcp_client_class);
	ms_tcp_client_init(r);
	return(MS_FILTER(r));
}
	

void ms_tcp_client_init(MSTcpClient *r)
{
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->outqueues=r->q_outputs;
	memset(r->q_outputs,0,sizeof(MSQueue*));
	r->sock=-1;
	r->msg=NULL;
}


int ms_tcp_client_connect(MSTcpClient *obj, const char *addr, int port){
	struct addrinfo hints;
	struct addrinfo *res;
	char service[40];
	int err;
	sprintf(service,"%i",port);
	memset(&hints,0,sizeof(hints));
	hints.ai_family=PF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;
	err=getaddrinfo(addr,service,&hints,&res);
	if (err!=0){
		g_warning("getaddrinfo error: %s",gai_strerror(err));
		return -1;
	}
	obj->sock=socket(res->ai_family,res->ai_socktype,0);
	if (obj->sock<0){
		g_warning("fail to create socket: %s",strerror(errno));
		return -1;
	}
	err=connect(obj->sock,res->ai_addr,res->ai_addrlen);
	if (err<0){
		g_warning("Could not connect to %s:%i : %s",addr,port,strerror(errno));
		close(obj->sock);
		obj->sock=-1;
		return -1;	
	}
	return 0;
}

void ms_tcp_client_class_init(MSTcpClientClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"TcpClient");
	MS_FILTER_CLASS(klass)->max_qoutputs=1;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_tcp_client_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_tcp_client_process;
	ms_filter_class_set_attr(MS_FILTER_CLASS(klass),FILTER_IS_SOURCE);
}



void ms_tcp_client_process(MSTcpClient *r)
{
	static const int rcvsz=5000;
	int err;
	if (r->sock>0){
		if (r->msg==NULL){
			r->msg=ms_message_new(rcvsz);
			memset(r->msg->data,0,rcvsz);
		}
		err=recv(r->sock,r->msg->data,rcvsz,MSG_DONTWAIT);
		if (err<0 && errno!=EWOULDBLOCK && errno!=EAGAIN){
			g_warning("recv error: %s",strerror(errno));
		}else if (err>0){
			r->msg->size=err;
			ms_queue_put(r->q_outputs[0],r->msg);
			printf("output new message %p,%i\n",r->msg->data,r->msg->size);
			r->msg=NULL;
			
		}
	}
}

void ms_tcp_client_uninit( MSTcpClient *obj){
	if (obj->sock>0) close(obj->sock);
	if (obj->msg!=NULL) ms_message_destroy(obj->msg);
}

void ms_tcp_client_destroy( MSTcpClient *obj)
{
	ms_tcp_client_uninit(obj);
	g_free(obj);
}



