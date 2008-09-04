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

#include "msbuffer.h"
#include "msutils.h"
#include <string.h>



MSBuffer * ms_buffer_new(guint32 size)
{
	MSBuffer *buf;
	buf=(MSBuffer*)g_malloc(sizeof(MSBuffer)+size);
	buf->ref_count=0;
	buf->size=size;
	ms_trace("ms_buffer_new: Allocating buffer of %i bytes.",size);
	/* allocate the data buffer: there is a lot of optmisation that can be done by using a pool of cached buffers*/
	buf->buffer=((char*)(buf))+sizeof(MSBuffer); /* to avoid to do two allocations,
					buffer info and buffer are contigous.*/
	buf->freefn=NULL;
	buf->freearg=NULL;
	return(buf);
}

MSBuffer *ms_buffer_new_with_buf(char *extbuf, int size,void (*freefn)(void *), void *freearg)
{
	MSBuffer *buf;
	buf=(MSBuffer*)g_malloc(sizeof(MSBuffer));
	buf->ref_count=0;
	buf->size=size;
	buf->buffer=extbuf;
	buf->freefn=freefn;
	buf->freearg=freearg;
	return(buf);
}


void ms_buffer_destroy(MSBuffer *buf)
{
	if (buf->freefn!=NULL) buf->freefn(buf->freearg);
		g_free(buf);
}

MSMessage *ms_message_alloc()
{
   MSMessage *m=g_malloc(sizeof(MSMessage));
   memset(m,0,sizeof(MSMessage));
   return m;
}

MSMessage *ms_message_new(gint size)
{
   MSMessage *m=ms_message_alloc();
   MSBuffer *buf=ms_buffer_new(size);
   ms_message_set_buf(m,buf);
   return m;
}

void ms_message_destroy(MSMessage *m)
{
	/* the buffer  is freed if its ref_count goes to zero */
	if (m->buffer!=NULL){
		m->buffer->ref_count--;
		if (m->buffer->ref_count==0) ms_buffer_destroy(m->buffer);
	}
	g_free(m);
}

MSMessage * ms_message_dup(MSMessage *m)
{
   MSMessage *msg=ms_message_alloc();
   ms_message_set_buf(msg,m->buffer);
   return msg;
}
