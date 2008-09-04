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

#ifndef MSWRITE_H
#define MSWRITE_H

#include "msfilter.h"


/*this is the class that implements writing reading sink filter*/

#define MSWRITE_MAX_INPUTS  1 /* max output per filter*/

#define MSWRITE_DEF_GRAN 512 /* the default granularity*/
#define MSWRITE_MIN_GRAN 64

typedef struct _MSWrite
{
    /* the MSWrite derivates from MSFilter, so the MSFilter object MUST be the first of the MSWrite object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSFifo *f_inputs[MSWRITE_MAX_INPUTS];
    MSQueue *q_inputs[MSWRITE_MAX_INPUTS];
    gint fd;  /* the file descriptor of the file being written*/
} MSWrite;

typedef struct _MSWriteClass
{
	/* the MSWrite derivates from MSFilter, so the MSFilter class MUST be the first of the MSWrite class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
} MSWriteClass;

/* PUBLIC */
#define MS_WRITE(filter) ((MSWrite*)(filter))
#define MS_WRITE_CLASS(klass) ((MSWriteClass*)(klass))
MSFilter * ms_write_new(char *name);

/* FOR INTERNAL USE*/
void ms_write_init(MSWrite *r);
void ms_write_class_init(MSWriteClass *klass);
void ms_write_destroy( MSWrite *obj);
void ms_write_process(MSWrite *r);

#endif

