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

#ifndef MSMULAWDECODER_H
#define MSMULAWDECODER_H

#include "msfilter.h"
#include "mscodec.h"

/*this is the class that implements a MULAWdecoder filter*/

#define MSMULAWDECODER_MAX_INPUTS  1 /* max output per filter*/


typedef struct _MSMULAWDecoder
{
    /* the MSMULAWDecoder derivates from MSFilter, so the MSFilter object MUST be the first of the MSMULAWDecoder object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSFifo *f_inputs[MSMULAWDECODER_MAX_INPUTS];
    MSFifo *f_outputs[MSMULAWDECODER_MAX_INPUTS];
} MSMULAWDecoder;

typedef struct _MSMULAWDecoderClass
{
	/* the MSMULAWDecoder derivates from MSFilter, so the MSFilter class MUST be the first of the MSMULAWDecoder class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
} MSMULAWDecoderClass;

/* PUBLIC */
#define MS_MULAWDECODER(filter) ((MSMULAWDecoder*)(filter))
#define MS_MULAWDECODER_CLASS(klass) ((MSMULAWDecoderClass*)(klass))
MSFilter * ms_MULAWdecoder_new(void);

/* FOR INTERNAL USE*/
void ms_MULAWdecoder_init(MSMULAWDecoder *r);
void ms_MULAWdecoder_class_init(MSMULAWDecoderClass *klass);
void ms_MULAWdecoder_destroy( MSMULAWDecoder *obj);
void ms_MULAWdecoder_process(MSMULAWDecoder *r);

/* tuning parameters :*/
#define MULAW_DECODER_WMAXGRAN 320
#define MULAW_DECODER_RMAXGRAN 160

extern MSCodecInfo MULAWinfo;


#endif
