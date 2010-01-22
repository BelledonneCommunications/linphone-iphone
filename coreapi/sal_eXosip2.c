/*
linphone
Copyright (C) 2010  Simon MORLAT (simon.morlat@free.fr)

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

#include "sal.h"
#include <eXosip2/eXosip.h>

extern char *media_description_to_sdp(const SalMediaDescription *sal);

struct Sal{
	SalCallbacks callbacks;
	int running;
	int session_expires;
};

struct SalOp{
	SalOpBase base;
	int cid;
	int did;
	int tid;
	bool_t supports_session_timers;
};

SalOp * sal_op_new(Sal *sal){
	SalOp *op=ms_new(SalOp,1);
	__sal_op_init(op,sal);
	op->cid=op->did=op->tid=-1;
	op->supports_session_timers=FALSE;
	return op;
}

void sal_op_release(SalOp *op){
	__sal_op_free(op);
}

Sal * sal_init(){
	eXosip_init();
	return ms_new0(Sal,1);
}

void sal_uninit(Sal* sal){
	eXosip_quit();
	ms_free(sal);
}


int sal_listen_port(Sal *ctx, const char *addr, int port, SalTransport tr, int is_secure){
	int err;
	bool_t ipv6;
	int proto=IPPROTO_UDP;
	
	if (ctx->running) eXosip_quit();
	eXosip_init();
	err=0;
	eXosip_set_option(13,&err); /*13=EXOSIP_OPT_SRV_WITH_NAPTR, as it is an enum value, we can't use it unless we are sure of the
					version of eXosip, which is not the case*/
	/*see if it looks like an IPv6 address*/
	ipv6=strchr(addr,':')!=NULL;
	eXosip_enable_ipv6(ipv6);

	if (tr!=SAL_TRANSPORT_DATAGRAM || is_secure){
		ms_fatal("SIP over TCP or TLS or DTLS is not supported yet.");
		return -1;
	}
	
	err=eXosip_listen_addr(proto, addr, port, ipv6 ?  PF_INET6 : PF_INET, 0);
	return err;
}

void sal_set_user_agent(Sal *ctx, const char *user_agent){
	eXosip_set_user_agent(user_agent);
}

void sal_use_session_timers(Sal *ctx, int expires){
	ctx->session_expires=expires;
}

static void set_sdp(osip_message_t *sip, const SalMediaDescription *desc){
	int sdplen;
	char clen[10];
	char *sdp=media_description_to_sdp(desc);

	if (sdp==NULL) {
		ms_error("Fail to print sdp message !");
		return;
	}
	sdplen=strlen(sdp);
	snprintf(clen,sizeof(clen),"%i",sdplen);
	osip_message_set_body(sip,sdp,sdplen);
	osip_message_set_content_type(sip,"application/sdp");
	osip_message_set_content_length(sip,clen);
	osip_free(sdp);
}

int sal_call_set_local_media_description(SalOp *h, SalMediaDescription *desc){
	h->base.local_media=desc;
	return 0;
}

int sal_call(SalOp *h, const char *from, const char *to){
	int err;
	osip_message_t *invite=NULL;
	sal_op_set_from(h,from);
	sal_op_set_to(h,to);
	err=eXosip_call_build_initial_invite(&invite,to,from,h->base.route,"Phone call");
	if (err!=0){
		ms_error("Could not create call.");
		return -1;
	}
	if (h->base.contact)
		osip_message_set_contact(invite,h->base.contact);
	if (h->base.root->session_expires!=0){
		osip_message_set_header(invite, "Session-expires", "200");
		osip_message_set_supported(invite, "timer");
	}
	if (h->base.local_media) set_sdp(invite,h->base.local_media);
	eXosip_lock();
	err=eXosip_call_send_initial_invite(invite);
	eXosip_unlock();
	h->cid=err;
	if (err<0){
		ms_error("Fail to send invite !");
		return -1;
	}
	return 0;
}

int sal_call_accept(SalOp * h){
	osip_message_t *msg;
	/* sends a 200 OK */
	int err=eXosip_call_build_answer(h->tid,200,&msg);
	if (err<0 || msg==NULL){
		ms_error("Fail to build answer for call: err=%i",err);
		return -1;
	}
	if (h->base.root->session_expires!=0){
		if (h->supports_session_timers) osip_message_set_supported(msg, "timer");
	}
	return 0;
}

const SalMediaDescription * sal_call_get_final_media_description(SalOp *h){
	return NULL;
}

int sal_call_terminate(SalOp *h){
	eXosip_lock();
	eXosip_call_terminate(h->cid,h->did);
	eXosip_unlock();
	return 0;
}

int sal_iterate(Sal *sal);

int sal_register(SalOp *h, const char *from, const char *contact, int expires);
