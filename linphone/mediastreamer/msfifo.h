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

#ifdef HAVE_GLIB
#include <glib.h>
#else
#include "glist.h"
#endif
#include "msbuffer.h"

typedef struct _MSFifo
{
	gint r_gran;  					/*maximum granularity for reading*/
	gint w_gran;						/*maximum granularity for writing*/
	gchar * rd_ptr;        /* read pointer on the position where there is something to read on the MSBuffer */
	guint32 readsize;
	gchar * wr_ptr;
	gchar * prev_wr_ptr;
	guint32 writesize;      /* write pointer on the position where it is possible to write on the MSBuffer */
	gchar * begin;  /* rd_ptr et wr_ptr must all be >=begin*/
	guint32 size;        /* the length of the fifo, but this may not be equal to buffer->size*/
	guint32 saved_offset;
	gchar * pre_end;  /* the end of the buffer that is copied at the begginning when we wrap around*/
	gchar * w_end;    /* when a wr ptr is expected to exceed end_offset,
											it must be wrapped around to go at the beginning of the buffer. This is the end of the buffer*/
	gchar * r_end;    /* this is the last position written at the end of the fifo. If a read ptr is expected to
											exceed this pointer, it must be put at the begginning of the buffer */
	void *prev_data;   /*user data, usually the writing MSFilter*/
	void *next_data;   /* user data, usually the reading MSFilter */
	MSBuffer *buffer;
} MSFifo;

/* constructor*/
/* r_gran: max granularity for reading (in number of bytes)*/
/* w_gran: max granularity for writing (in number of bytes)*/
/* r_offset: number of bytes that are kept available behind read pointer (for recursive filters)*/
/* w_offset: number of bytes that are kept available behind write pointer (for recursive filters)*/
/* buf is a MSBuffer that should be compatible with the above parameter*/
MSFifo * ms_fifo_new(MSBuffer *buf, gint r_gran, gint w_gran, gint r_offset, gint w_offset);

/*does the same that ms_fifo_new(), but also allocate a compatible buffer automatically*/
MSFifo * ms_fifo_new_with_buffer(gint r_gran, gint w_gran, gint r_offset, gint w_offset, gint min_buffer_size);

void ms_fifo_destroy( MSFifo *fifo);

void ms_fifo_destroy_with_buffer(MSFifo *fifo);

/* get data to read */
gint ms_fifo_get_read_ptr(MSFifo *fifo, gint bsize, void **ret_ptr);

/* get a buffer to write*/
gint ms_fifo_get_write_ptr(MSFifo *fifo, gint bsize, void **ret_ptr);

/* in case the buffer got by ms_fifo_get_write_ptr() could not be filled completely, you must
tell it by using this function */
void ms_fifo_update_write_ptr(MSFifo *fifo, gint written);
