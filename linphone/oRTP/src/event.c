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

#include "ortp/event.h"
#include "ortp/ortp.h"

RtpEndpoint *rtp_endpoint_new(struct sockaddr *addr, socklen_t addrlen){
	RtpEndpoint *ep=ortp_new(RtpEndpoint,1);
	if (sizeof(ep->addr)<addrlen){
		ortp_free(ep);
		ortp_fatal("Bad socklen_t size !");
		return NULL;
	}
	memcpy(&ep->addr,addr,addrlen);
	ep->addrlen=addrlen;
	return ep;
}

void rtp_endpoint_destroy(RtpEndpoint *ep){
	ortp_free(ep);
}

RtpEndpoint *rtp_endpoint_dup(const RtpEndpoint *ep){
	return rtp_endpoint_new((struct sockaddr*)&ep->addr,ep->addrlen);
}

OrtpEvent * ortp_event_new(unsigned long type){
	const int size=sizeof(OrtpEventType)+sizeof(OrtpEventData);
	mblk_t *m=allocb(size,0);
	memset(m->b_wptr,0,size);
	*((OrtpEventType*)m->b_wptr)=type;
	return m;
}

OrtpEvent *ortp_event_dup(OrtpEvent *ev){
#if 0
	OrtpEvent *nev=dupb(ev);
#else
	OrtpEvent *nev = ortp_event_new(ortp_event_get_type(ev));
	OrtpEventData * ed = ortp_event_get_data(ev);
	OrtpEventData * edv = ortp_event_get_data(nev);

	if (ed->ep) edv->ep = rtp_endpoint_dup(ed->ep);
	if (ed->packet) edv->packet = copymsg(ed->packet);
	edv->info.telephone_event = ed->info.telephone_event;
#endif
	return nev;
}

OrtpEventType ortp_event_get_type(const OrtpEvent *ev){
	return ((OrtpEventType*)ev->b_rptr)[0];
}

OrtpEventData * ortp_event_get_data(OrtpEvent *ev){
	return (OrtpEventData*)(ev->b_rptr+sizeof(OrtpEventType));
}

void ortp_event_destroy(OrtpEvent *ev){
	OrtpEventData *d=ortp_event_get_data(ev);
	if (ev->b_datap->db_ref==1){
		if (d->packet) 	freemsg(d->packet);
		if (d->ep) rtp_endpoint_destroy(d->ep);
	}
	freemsg(ev);
}

OrtpEvQueue * ortp_ev_queue_new(){
	OrtpEvQueue *q=ortp_new(OrtpEvQueue,1);
	qinit(&q->q);
	ortp_mutex_init(&q->mutex,NULL);
	return q;
}

void ortp_ev_queue_flush(OrtpEvQueue * qp){
	OrtpEvent *ev;
	while((ev=ortp_ev_queue_get(qp))!=NULL){
		ortp_event_destroy(ev);
	}
}

OrtpEvent * ortp_ev_queue_get(OrtpEvQueue *q){
	OrtpEvent *ev;
	ortp_mutex_lock(&q->mutex);
	ev=getq(&q->q);
	ortp_mutex_unlock(&q->mutex);
	return ev;
}

void ortp_ev_queue_destroy(OrtpEvQueue * qp){
	ortp_ev_queue_flush(qp);
	ortp_mutex_destroy(&qp->mutex);
	ortp_free(qp);
}

void ortp_ev_queue_put(OrtpEvQueue *q, OrtpEvent *ev){
	ortp_mutex_lock(&q->mutex);
	putq(&q->q,ev);
	ortp_mutex_unlock(&q->mutex);
}

