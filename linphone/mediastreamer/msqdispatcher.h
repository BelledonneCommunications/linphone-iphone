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


#ifndef MSQDISPATCHER_H
#define MSQDISPATCHER_H

#include "msfilter.h"


/*this is the class that implements a qdispatcher filter*/

#define MS_QDISPATCHER_MAX_INPUTS  1
#define MS_QDISPATCHER_MAX_OUTPUTS 5 

typedef struct _MSQdispatcher
{
    /* the MSQdispatcher derivates from MSFilter, so the MSFilter object MUST be the first of the MSQdispatcher object
       in order to the object mechanism to work*/
    MSFilter filter;
	MSQueue *q_inputs[MS_QDISPATCHER_MAX_INPUTS];
	MSQueue *q_outputs[MS_QDISPATCHER_MAX_OUTPUTS];
} MSQdispatcher;

typedef struct _MSQdispatcherClass
{
	/* the MSQdispatcher derivates from MSFilter, so the MSFilter class MUST be the first of the MSQdispatcher class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
} MSQdispatcherClass;

/* PUBLIC */
#define MS_QDISPATCHER(filter) ((MSQdispatcher*)(filter))
#define MS_QDISPATCHER_CLASS(klass) ((MSQdispatcherClass*)(klass))
MSFilter * ms_qdispatcher_new(void);

/* FOR INTERNAL USE*/
void ms_qdispatcher_init(MSQdispatcher *r);
void ms_qdispatcher_class_init(MSQdispatcherClass *klass);
void ms_qdispatcher_destroy( MSQdispatcher *obj);
void ms_qdispatcher_process(MSQdispatcher *r);

#endif
