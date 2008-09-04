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


#ifndef MSCOPY_H
#define MSCOPY_H

#include "msfilter.h"


/*this is the class that implements a copy filter*/

#define MSCOPY_MAX_INPUTS  1 /* max output per filter*/

#define MSCOPY_DEF_GRAN 64 /* the default granularity*/

typedef struct _MSCopy
{
    /* the MSCopy derivates from MSFilter, so the MSFilter object MUST be the first of the MSCopy object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSFifo *f_inputs[MSCOPY_MAX_INPUTS];
    MSFifo *f_outputs[MSCOPY_MAX_INPUTS];
} MSCopy;

typedef struct _MSCopyClass
{
	/* the MSCopy derivates from MSFilter, so the MSFilter class MUST be the first of the MSCopy class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
} MSCopyClass;

/* PUBLIC */
#define MS_COPY(filter) ((MSCopy*)(filter))
#define MS_COPY_CLASS(klass) ((MSCopyClass*)(klass))
MSFilter * ms_copy_new(void);

/* FOR INTERNAL USE*/
void ms_copy_init(MSCopy *r);
void ms_copy_class_init(MSCopyClass *klass);
void ms_copy_destroy( MSCopy *obj);
void ms_copy_process(MSCopy *r);

#endif
