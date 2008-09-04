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


#ifndef MSRTPSEND_H
#define MSRTPSEND_H

#include "msfilter.h"
#include "mssync.h"

#undef PACKAGE
#undef VERSION
#include <ortp/ortp.h>


/*this is the class that implements a sending through rtp filter*/

#define MSRTPSEND_MAX_INPUTS  1 /* max input per filter*/

#define MSRTPSEND_DEF_GRAN  160/* the default granularity*/

struct _MSRtpSend
{
    /* the MSCopy derivates from MSFilter, so the MSFilter object MUST be the first of the MSCopy object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSFifo *f_inputs[MSRTPSEND_MAX_INPUTS];
	MSQueue *q_inputs[MSRTPSEND_MAX_INPUTS];
	MSSync *sync;
	RtpSession *rtpsession;
	guint32 ts;
	guint32 ts_inc;	/* the timestamp increment */
	gint packet_size;
	guint flags;
        guint delay; /* number of _proccess call which must be skipped */
#define RTPSEND_CONFIGURED (1)
};

typedef struct _MSRtpSend MSRtpSend;

struct _MSRtpSendClass
{
	/* the MSRtpSend derivates from MSFilter, so the MSFilter class MUST be the first of the MSCopy class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
};

typedef struct _MSRtpSendClass MSRtpSendClass;

/* PUBLIC */
#define MS_RTP_SEND(filter) ((MSRtpSend*)(filter))
#define MS_RTP_SEND_CLASS(klass) ((MSRtpSendClass*)(klass))
MSFilter * ms_rtp_send_new(void);
RtpSession * ms_rtp_send_set_session(MSRtpSend *obj,RtpSession *session);
#define ms_rtp_send_unset_session(obj) (ms_rtp_send_set_session((obj),NULL))
#define ms_rtp_send_get_session(obj) ((obj)->rtpsession)
void ms_rtp_send_set_timing(MSRtpSend *r, guint32 ts_inc, gint payload_size);
gint ms_rtp_send_dtmf(MSRtpSend *r, gchar dtmf);


/* FOR INTERNAL USE*/
void ms_rtp_send_init(MSRtpSend *r);
void ms_rtp_send_class_init(MSRtpSendClass *klass);
void ms_rtp_send_destroy( MSRtpSend *obj);
void ms_rtp_send_process(MSRtpSend *r);
void ms_rtp_send_setup(MSRtpSend *r, MSSync *sync);

#endif
