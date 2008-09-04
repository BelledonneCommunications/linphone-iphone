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

#ifndef MSREAD_H
#define MSREAD_H

#include "msfilter.h"
#include "mssync.h"

/*this is the class that implements file reading source filter*/

#define MSREAD_MAX_OUTPUTS  1 /* max output per filter*/

#define MSREAD_DEF_GRAN 640 /* the default granularity*/

typedef enum{
	MS_READ_STATE_STARTED,
	MS_READ_STATE_STOPPED,
	MS_READ_STATE_EOF
}MSReadState;

typedef struct _MSRead
{
    /* the MSRead derivates from MSFilter, so the MSFilter object MUST be the first of the MSRead object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSFifo *foutputs[MSREAD_MAX_OUTPUTS];
    MSQueue *qoutputs[MSREAD_MAX_OUTPUTS];
	MSSync *sync;
	gint rate;
    gint fd;  /* the file descriptor of the file being read*/
    gint gran;  /*granularity*/  /* for use with queues */
	gint need_swap;
	gint state;
} MSRead;

typedef struct _MSReadClass
{
	/* the MSRead derivates from MSFilter, so the MSFilter class MUST be the first of the MSRead class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
} MSReadClass;

/* PUBLIC */
#define MS_READ(filter) ((MSRead*)(filter))
#define MS_READ_CLASS(klass) ((MSReadClass*)(klass))
MSFilter * ms_read_new(char *name);
/* set the granularity for reading file on disk */
#define ms_read_set_bufsize(filter,sz) (filter)->gran=(sz)

/* FOR INTERNAL USE*/
void ms_read_init(MSRead *r);
void ms_read_class_init(MSReadClass *klass);
void ms_read_destroy( MSRead *obj);
void ms_read_process(MSRead *r);
void ms_read_setup(MSRead *r, MSSync *sync);

typedef enum{
	MS_READ_EVENT_EOF	/* end of file */
} MSReadEvent;


#endif
