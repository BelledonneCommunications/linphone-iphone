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

#ifndef MSMULAWENCODER_H
#define MSMULAWENCODER_H

#include "mscodec.h"


/*this is the class that implements a MULAWencoder filter*/

#define MSMULAWENCODER_MAX_INPUTS  1 /* max output per filter*/


typedef struct _MSMULAWEncoder
{
    /* the MSMULAWEncoder derivates from MSFilter, so the MSFilter object MUST be the first of the MSMULAWEncoder object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSFifo *f_inputs[MSMULAWENCODER_MAX_INPUTS];
    MSFifo *f_outputs[MSMULAWENCODER_MAX_INPUTS];
} MSMULAWEncoder;

typedef struct _MSMULAWEncoderClass
{
	/* the MSMULAWEncoder derivates from MSFilter, so the MSFilter class MUST be the first of the MSMULAWEncoder class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
} MSMULAWEncoderClass;

/* PUBLIC */
#define MS_MULAWENCODER(filter) ((MSMULAWEncoder*)(filter))
#define MS_MULAWENCODER_CLASS(klass) ((MSMULAWEncoderClass*)(klass))
MSFilter * ms_MULAWencoder_new(void);

/* FOR INTERNAL USE*/
void ms_MULAWencoder_init(MSMULAWEncoder *r);
void ms_MULAWencoder_class_init(MSMULAWEncoderClass *klass);
void ms_MULAWencoder_destroy( MSMULAWEncoder *obj);
void ms_MULAWencoder_process(MSMULAWEncoder *r);

/* tuning parameters :*/
#define MULAW_ENCODER_WMAXGRAN 160
#define MULAW_ENCODER_RMAXGRAN 320

#endif
