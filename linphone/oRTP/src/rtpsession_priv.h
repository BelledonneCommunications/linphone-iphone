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

#ifndef rtpsession_priv_h
#define rtpsession_priv_h

#include "ortp/rtpsession.h"

typedef enum {
	RTP_SESSION_RECV_SYNC=1,	/* the rtp session is synchronising in the incoming stream */
	RTP_SESSION_FIRST_PACKET_DELIVERED=1<<1,
	RTP_SESSION_SCHEDULED=1<<2,/* the scheduler controls this session*/
	RTP_SESSION_BLOCKING_MODE=1<<3, /* in blocking mode */
	RTP_SESSION_RECV_NOT_STARTED=1<<4,	/* the application has not started to try to recv */
	RTP_SESSION_SEND_NOT_STARTED=1<<5,  /* the application has not started to send something */
	RTP_SESSION_IN_SCHEDULER=1<<6,	/* the rtp session is in the scheduler list */
	RTP_SESSION_USING_EXT_SOCKETS=1<<7, /* the session is using externaly supplied sockets */
	RTP_SOCKET_CONNECTED=1<<8,
	RTCP_SOCKET_CONNECTED=1<<9,
	RTP_SESSION_USING_TRANSPORT=1<<10
}RtpSessionFlags;

#define rtp_session_using_transport(s, stream) (((s)->flags & RTP_SESSION_USING_TRANSPORT) && (s->stream.tr != 0))

void rtp_session_update_payload_type(RtpSession * session, int pt);
void rtp_putq(queue_t *q, mblk_t *mp);
mblk_t * rtp_getq(queue_t *q, uint32_t ts, int *rejected);
int rtp_session_rtp_recv(RtpSession * session, uint32_t ts);
int rtp_session_rtcp_recv(RtpSession * session);
int rtp_session_rtp_send (RtpSession * session, mblk_t * m);
int rtp_session_rtcp_send (RtpSession * session, mblk_t * m);

void rtp_session_rtp_parse(RtpSession *session, mblk_t *mp, uint32_t local_str_ts, struct sockaddr *addr, socklen_t addrlen);
void rtp_session_rtcp_parse(RtpSession *session, mblk_t *mp);

void rtp_session_dispatch_event(RtpSession *session, OrtpEvent *ev);

#endif
