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

#ifndef MSALAWDECODER_H
#define MSALAWDECODER_H

#include "msfilter.h"
#include "mscodec.h"

/*this is the class that implements a ALAWdecoder filter*/

#define MSALAWDECODER_MAX_INPUTS  1 /* max output per filter*/


typedef struct _MSALAWDecoder
{
    /* the MSALAWDecoder derivates from MSFilter, so the MSFilter object MUST be the first of the MSALAWDecoder object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSFifo *f_inputs[MSALAWDECODER_MAX_INPUTS];
    MSFifo *f_outputs[MSALAWDECODER_MAX_INPUTS];
} MSALAWDecoder;

typedef struct _MSALAWDecoderClass
{
	/* the MSALAWDecoder derivates from MSFilter, so the MSFilter class MUST be the first of the MSALAWDecoder class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
} MSALAWDecoderClass;

/* PUBLIC */
#define MS_ALAWDECODER(filter) ((MSALAWDecoder*)(filter))
#define MS_ALAWDECODER_CLASS(klass) ((MSALAWDecoderClass*)(klass))
MSFilter * ms_ALAWdecoder_new(void);

/* FOR INTERNAL USE*/
void ms_ALAWdecoder_init(MSALAWDecoder *r);
void ms_ALAWdecoder_class_init(MSALAWDecoderClass *klass);
void ms_ALAWdecoder_destroy( MSALAWDecoder *obj);
void ms_ALAWdecoder_process(MSALAWDecoder *r);

/* tuning parameters :*/
#define ALAW_DECODER_WMAXGRAN 320
#define ALAW_DECODER_RMAXGRAN 160

extern MSCodecInfo ALAWinfo;

#endif
