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

#include "msqueue.h"
#include <string.h>

MSQueue * ms_queue_new()
{
   MSQueue *q=g_malloc(sizeof(MSQueue));
   memset(q,0,sizeof(MSQueue));
   return q;
}

MSMessage *ms_queue_get(MSQueue *q)
{
	MSMessage *b=q->last;
	if (b==NULL) return NULL;
	q->last=b->prev;
	if (b->prev==NULL) q->first=NULL; /* it was the only element of the queue*/
	q->size--;
	b->next=b->prev=NULL;
	return(b);
}

void ms_queue_put(MSQueue *q, MSMessage *m)
{
   MSMessage *mtmp=q->first;
   g_return_if_fail(m!=NULL);
   q->first=m;
   m->next=mtmp;
   if (mtmp!=NULL)
   {
      mtmp->prev=m;
   }
   else q->last=m; /* it was the first element of the q */
   q->size++;
}

MSMessage *ms_queue_peek_last(MSQueue *q){
	return q->last;
}
