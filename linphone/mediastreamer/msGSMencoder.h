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


#ifndef MSGSMENCODER_H
#define MSGSMENCODER_H

#include "msfilter.h"
#include <gsm.h>

/*this is the class that implements a GSMencoder filter*/

#define MSGSMENCODER_MAX_INPUTS  1 /* max output per filter*/


typedef struct _MSGSMEncoder
{
    /* the MSGSMEncoder derivates from MSFilter, so the MSFilter object MUST be the first of the MSGSMEncoder object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSFifo *f_inputs[MSGSMENCODER_MAX_INPUTS];
    MSQueue *q_outputs[MSGSMENCODER_MAX_INPUTS];
    gsm gsm_handle;
} MSGSMEncoder;

typedef struct _MSGSMEncoderClass
{
	/* the MSGSMEncoder derivates from MSFilter, so the MSFilter class MUST be the first of the MSGSMEncoder class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
} MSGSMEncoderClass;

/* PUBLIC */
#define MS_GSMENCODER(filter) ((MSGSMEncoder*)(filter))
#define MS_GSMENCODER_CLASS(klass) ((MSGSMEncoderClass*)(klass))
MSFilter * ms_GSMencoder_new(void);

/* FOR INTERNAL USE*/
void ms_GSMencoder_init(MSGSMEncoder *r);
void ms_GSMencoder_class_init(MSGSMEncoderClass *klass);
void ms_GSMencoder_destroy( MSGSMEncoder *obj);
void ms_GSMencoder_process(MSGSMEncoder *r);

#endif
