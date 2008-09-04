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

  You should have received a dispatcher of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef MSFDISPATCHER_H
#define MSFDISPATCHER_H

#include "msfilter.h"


/*this is the class that implements a fdispatcher filter*/

#define MS_FDISPATCHER_MAX_INPUTS  1
#define MS_FDISPATCHER_MAX_OUTPUTS 5 
#define MS_FDISPATCHER_DEF_GRAN 64 /* the default granularity*/

typedef struct _MSFdispatcher
{
    /* the MSFdispatcher derivates from MSFilter, so the MSFilter object MUST be the first of the MSFdispatcher object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSFifo *f_inputs[MS_FDISPATCHER_MAX_INPUTS];
    MSFifo *f_outputs[MS_FDISPATCHER_MAX_OUTPUTS];
} MSFdispatcher;

typedef struct _MSFdispatcherClass
{
	/* the MSFdispatcher derivates from MSFilter, so the MSFilter class MUST be the first of the MSFdispatcher class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
} MSFdispatcherClass;

/* PUBLIC */
#define MS_FDISPATCHER(filter) ((MSFdispatcher*)(filter))
#define MS_FDISPATCHER_CLASS(klass) ((MSFdispatcherClass*)(klass))
MSFilter * ms_fdispatcher_new(void);

/* FOR INTERNAL USE*/
void ms_fdispatcher_init(MSFdispatcher *r);
void ms_fdispatcher_class_init(MSFdispatcherClass *klass);
void ms_fdispatcher_destroy( MSFdispatcher *obj);
void ms_fdispatcher_process(MSFdispatcher *r);

#endif
