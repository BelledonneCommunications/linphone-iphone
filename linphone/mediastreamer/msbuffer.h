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


#ifndef MSBUFFER_H
#define MSBUFFER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GLIB
#include <glib.h>
#else
#include <uglib.h>
#endif


#define MS_BUFFER_LARGE 4092


typedef struct _MSBuffer
{
    gchar *buffer;
    guint32 size;
    gint ref_count;
	void (*freefn)(void *);
	void *freearg;
}MSBuffer;

MSBuffer * ms_buffer_new(guint32 size);
MSBuffer *ms_buffer_new_with_buf(char *extbuf, int size,void (*freefn)(void *), void *freearg);
void ms_buffer_destroy(MSBuffer *buf);

struct _MSMessage
{
	MSBuffer *buffer; /* points to a MSBuffer */
	char *data;          /*points to buffer->buffer  */
	guint32 size;   /* the size of the buffer to read in data. It may not be the
   								physical size (I mean buffer->buffer->size */
	struct _MSMessage *next;
	struct _MSMessage *prev;  /* MSMessage are queued into MSQueues */
	gboolean markbit;
};

typedef struct _MSMessage MSMessage;

MSMessage *ms_message_new(gint size);

#define ms_message_set_buf(m,b) do { (b)->ref_count++; (m)->buffer=(b); (m)->data=(b)->buffer; (m)->size=(b)->size; }while(0)
#define ms_message_unset_buf(m) do { (m)->buffer->ref_count--; (m)->buffer=NULL; (m)->size=0; (m)->data=NULL; } while(0)

#define ms_message_size(m)		(m)->size
void ms_message_destroy(MSMessage *m);

MSMessage * ms_message_dup(MSMessage *m);

/* allocate a single message without buffer */
MSMessage *ms_message_alloc();

#endif
