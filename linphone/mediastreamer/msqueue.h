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

#ifndef MSQUEUE_H
#define MSQUEUE_H

#include "msbuffer.h"

/* for the moment these are stupid queues limited to one element*/

typedef struct _MSQueue
{
	MSMessage *first;
	MSMessage *last;
	gint size;
	void *prev_data; /*user data, usually the writting filter*/
	void *next_data; /* user data, usually the reading filter*/
}MSQueue;


MSQueue * ms_queue_new();

MSMessage *ms_queue_get(MSQueue *q);

void ms_queue_put(MSQueue *q, MSMessage *m);

MSMessage *ms_queue_peek_last(MSQueue *q);

#define ms_queue_can_get(q) ( (q)->size!=0 )

#define ms_queue_destroy(q) g_free(q)


#endif
