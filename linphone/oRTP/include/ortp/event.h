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

#ifndef ortp_events_h
#define ortp_events_h

#include <ortp/str_utils.h>

typedef mblk_t OrtpEvent;

typedef unsigned long OrtpEventType;

typedef struct RtpEndpoint{
#ifdef ORTP_INET6
	struct sockaddr_storage addr;
#else
	struct sockaddr addr;
#endif
	socklen_t addrlen;
}RtpEndpoint;


struct _OrtpEventData{
	mblk_t *packet;	/* most events are associated to a received packet */
	RtpEndpoint *ep;
	union {
		int telephone_event;
		int payload_type;
	} info;
};

typedef struct _OrtpEventData OrtpEventData;



#ifdef __cplusplus
extern "C"{
#endif

RtpEndpoint *rtp_endpoint_new(struct sockaddr *addr, socklen_t addrlen);
RtpEndpoint *rtp_endpoint_dup(const RtpEndpoint *ep);

OrtpEvent * ortp_event_new(OrtpEventType tp);
OrtpEventType ortp_event_get_type(const OrtpEvent *ev);
/* type is one of the following*/
#define ORTP_EVENT_STUN_PACKET_RECEIVED		1
#define ORTP_EVENT_PAYLOAD_TYPE_CHANGED 	2
#define ORTP_EVENT_TELEPHONE_EVENT		3
#define ORTP_EVENT_RTCP_PACKET_RECEIVED		4
OrtpEventData * ortp_event_get_data(OrtpEvent *ev);
void ortp_event_destroy(OrtpEvent *ev);
OrtpEvent *ortp_event_dup(OrtpEvent *ev);

typedef struct OrtpEvQueue{
	queue_t q;
	ortp_mutex_t mutex;
} OrtpEvQueue;

OrtpEvQueue * ortp_ev_queue_new(void);
void ortp_ev_queue_destroy(OrtpEvQueue *q);
OrtpEvent * ortp_ev_queue_get(OrtpEvQueue *q);
void ortp_ev_queue_flush(OrtpEvQueue * qp);

#ifdef __cplusplus
}
#endif

#endif

