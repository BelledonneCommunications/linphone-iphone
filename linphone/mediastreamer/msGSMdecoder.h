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


#ifndef MSGSMDECODER_H
#define MSGSMDECODER_H

#include "msfilter.h"
#include "mscodec.h"
#include <gsm.h>

/*this is the class that implements a GSMdecoder filter*/

#define MSGSMDECODER_MAX_INPUTS  1 /* max output per filter*/


typedef struct _MSGSMDecoder
{
    /* the MSGSMDecoder derivates from MSFilter, so the MSFilter object MUST be the first of the MSGSMDecoder object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSQueue *q_inputs[MSGSMDECODER_MAX_INPUTS];
    MSFifo *f_outputs[MSGSMDECODER_MAX_INPUTS];
    gsm gsm_handle;
} MSGSMDecoder;

typedef struct _MSGSMDecoderClass
{
	/* the MSGSMDecoder derivates from MSFilter, so the MSFilter class MUST be the first of the MSGSMDecoder class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
} MSGSMDecoderClass;

/* PUBLIC */
#define MS_GSMDECODER(filter) ((MSGSMDecoder*)(filter))
#define MS_GSMDECODER_CLASS(klass) ((MSGSMDecoderClass*)(klass))
MSFilter * ms_GSMdecoder_new(void);

/* FOR INTERNAL USE*/
void ms_GSMdecoder_init(MSGSMDecoder *r);
void ms_GSMdecoder_class_init(MSGSMDecoderClass *klass);
void ms_GSMdecoder_destroy( MSGSMDecoder *obj);
void ms_GSMdecoder_process(MSGSMDecoder *r);

extern MSCodecInfo GSMinfo;

#endif
