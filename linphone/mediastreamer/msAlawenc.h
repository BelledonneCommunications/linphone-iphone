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

#ifndef MSALAWENCODER_H
#define MSALAWENCODER_H

#include "mscodec.h"


/*this is the class that implements a ALAWencoder filter*/

#define MSALAWENCODER_MAX_INPUTS  1 /* max output per filter*/


typedef struct _MSALAWEncoder
{
    /* the MSALAWEncoder derivates from MSFilter, so the MSFilter object MUST be the first of the MSALAWEncoder object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSFifo *f_inputs[MSALAWENCODER_MAX_INPUTS];
    MSFifo *f_outputs[MSALAWENCODER_MAX_INPUTS];
} MSALAWEncoder;

typedef struct _MSALAWEncoderClass
{
	/* the MSALAWEncoder derivates from MSFilter, so the MSFilter class MUST be the first of the MSALAWEncoder class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
} MSALAWEncoderClass;

/* PUBLIC */
#define MS_ALAWENCODER(filter) ((MSALAWEncoder*)(filter))
#define MS_ALAWENCODER_CLASS(klass) ((MSALAWEncoderClass*)(klass))
MSFilter * ms_ALAWencoder_new(void);

/* FOR INTERNAL USE*/
void ms_ALAWencoder_init(MSALAWEncoder *r);
void ms_ALAWencoder_class_init(MSALAWEncoderClass *klass);
void ms_ALAWencoder_destroy( MSALAWEncoder *obj);
void ms_ALAWencoder_process(MSALAWEncoder *r);

/* tuning parameters :*/
#define ALAW_ENCODER_WMAXGRAN 160
#define ALAW_ENCODER_RMAXGRAN 320


#endif
