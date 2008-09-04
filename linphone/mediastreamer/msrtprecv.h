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


#ifndef MSRTPRECV_H
#define MSRTPRECV_H

#include "msfilter.h"
#include "mssync.h"

/* because of a conflict between config.h from oRTP and config.h from linphone:*/
#undef PACKAGE
#undef VERSION                                                
#include <ortp/ortp.h>

/*this is the class that implements a copy filter*/

#define MSRTPRECV_MAX_OUTPUTS  1 /* max output per filter*/

#define MSRTPRECV_DEF_GRAN 320 /* the default granularity*/

struct _MSRtpRecv
{
    /* the MSCopy derivates from MSFilter, so the MSFilter object MUST be the first of the MSCopy object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSFifo *f_outputs[MSRTPRECV_MAX_OUTPUTS];
	MSQueue *q_outputs[MSRTPRECV_MAX_OUTPUTS];
	MSSync *sync;
	RtpSession *rtpsession;
	guint32 prev_ts;
	gint stream_started;
	gint payload_expected;
	gboolean ignore;
};

typedef struct _MSRtpRecv MSRtpRecv;

struct _MSRtpRecvClass
{
	/* the MSCopy derivates from MSFilter, so the MSFilter class MUST be the first of the MSCopy class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
};

typedef struct _MSRtpRecvClass MSRtpRecvClass;

/* PUBLIC */
#define MS_RTP_RECV(filter) ((MSRtpRecv*)(filter))
#define MS_RTP_RECV_CLASS(klass) ((MSRtpRecvClass*)(klass))
MSFilter * ms_rtp_recv_new(void);
RtpSession * ms_rtp_recv_set_session(MSRtpRecv *obj,RtpSession *session);
#define ms_rtp_recv_unset_session(obj) (ms_rtp_recv_set_session((obj),NULL))
#define ms_rtp_recv_get_session(obj) ((obj)->rtpsession)



/* FOR INTERNAL USE*/
void ms_rtp_recv_init(MSRtpRecv *r);
void ms_rtp_recv_class_init(MSRtpRecvClass *klass);
void ms_rtp_recv_destroy( MSRtpRecv *obj);
void ms_rtp_recv_process(MSRtpRecv *r);
void ms_rtp_recv_setup(MSRtpRecv *r,MSSync *sync);

#endif
