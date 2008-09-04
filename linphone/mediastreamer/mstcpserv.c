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

#include "mstcpserv.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <netinet/tcp.h>


void ms_tcp_serv_process(MSTcpServ *r);
void ms_tcp_serv_init(MSTcpServ *r, int port);
void ms_tcp_serv_destroy(MSTcpServ *r);
void ms_tcp_serv_class_init(MSTcpServClass *klass);

static MSTcpServClass *ms_tcp_serv_class=NULL;

MSFilter * ms_tcp_serv_new(void)
{
	MSTcpServ *r;
	
	r=g_new(MSTcpServ,1);
	
	if (ms_tcp_serv_class==NULL)
	{
		ms_tcp_serv_class=g_new(MSTcpServClass,1);
		ms_tcp_serv_class_init(ms_tcp_serv_class);
	}
	MS_FILTER(r)->klass=MS_FILTER_CLASS(ms_tcp_serv_class);
	ms_tcp_serv_init(r,5800);
	return(MS_FILTER(r));
}
	

void ms_tcp_serv_init(MSTcpServ *r, int port)
{
	struct addrinfo *res;
	struct addrinfo hints;
	char service[20];
	int err;
	int val=1;
	
	ms_filter_init(MS_FILTER(r));
	MS_FILTER(r)->inqueues=r->q_inputs;
	memset(r->q_inputs,0,sizeof(MSQueue*));
	memset(&r->set,0,sizeof(fd_set));
	r->maxfd=0;
	r->asock=socket(PF_INET,SOCK_STREAM,0);
	if (r->asock<0){
		g_warning("Could not create socket: %s",strerror(errno));
		return;
	}
	err=fcntl(r->asock,F_SETFL,O_NONBLOCK);
	if (err<0){
		g_warning("Could not non blocking flag on socket: %s",strerror(errno));
		return;
	}
	err=setsockopt(r->asock,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(int));
	if (err<0){
		g_warning("Could not set socket reusable: %s",strerror(errno));
		return;
	}
	sprintf(service,"%i",port);
	memset(&hints,0,sizeof(hints));
	hints.ai_family=PF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;
	if (err=getaddrinfo("0.0.0.0",service,&hints,&res)!=0){
		g_warning("Could not getaddrinfo: %s",gai_strerror(err));
		return;	
	}
	err=bind(r->asock,(struct sockaddr *)res->ai_addr,res->ai_addrlen);
	freeaddrinfo(res);
	if (err<0){
		g_warning("Could not bind socket: %s",strerror(errno));
		return;
	}
	err=listen(r->asock,10);
	if (err<0){
		g_warning("Could not listen: %s",strerror(errno));
		return;
	}
}

void ms_tcp_serv_class_init(MSTcpServClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"TcpServ");
	MS_FILTER_CLASS(klass)->max_qinputs=1;
	MS_FILTER_CLASS(klass)->max_finputs=0;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_tcp_serv_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_tcp_serv_process;
}

static void accept_new_clients(MSTcpServ *r){
	int sock;
	struct sockaddr_storage ss;
	socklen_t len=sizeof(ss);
	int val=1;
	int err;
	
	sock=accept(r->asock,(struct sockaddr*)&ss,&len);
	if (sock<0){
		if (errno!=EWOULDBLOCK && errno!=EAGAIN) g_warning("Could not accept connection: %s",strerror(errno));
		return;
	}
	FD_SET(sock,&r->set);
	err=setsockopt(sock,SOL_TCP,TCP_NODELAY,&val,sizeof(int));
	if (err<0){
		g_warning("Could not set tcp nodelay option: %s",strerror(errno));
	}
	if (r->maxfd<sock) r->maxfd=sock;
	printf("New client accepted.\n");
}


void ms_tcp_serv_process(MSTcpServ *r)
{
	MSMessage *msg;
	int err;
	g_return_if_fail(r->asock>0);
	/*printf("ms_tcp_serv_process\n");*/
	/* first accept incoming connections */
	accept_new_clients(r);
	/* send data to all clients */
	msg=ms_queue_get(r->q_inputs[0]);
	if (msg!=NULL){
		int i;
		for (i=0;i<r->maxfd+1;i++){
			if (FD_ISSET(i,&r->set)){
				err=send(i,msg->data,msg->size,0);
				if (err<0){
					FD_CLR(i,&r->set);
					close(i);
					g_message("Client disconnected.");
				}
			}
		}
		ms_message_destroy(msg);
	}
	
}

void ms_tcp_serv_uninit( MSTcpServ *obj){
	if (obj->asock>0) close(obj->asock);
}

void ms_tcp_serv_destroy( MSTcpServ *obj)
{
	ms_tcp_serv_uninit(obj);
	g_free(obj);
}



