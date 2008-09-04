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
#ifndef MSOSSREAD_H
#define MSOSSREAD_H

#include "mssoundread.h"
#include "sndcard.h"
#include "mssync.h"


/*this is the class that implements oss writing sink filter*/

#define MS_OSS_READ_MAX_INPUTS  1 /* max output per filter*/

#define MS_OSS_READ_MAX_GRAN (512*2) /* the maximum granularity*/

struct _MSOssRead
{
	/* the MSOssRead derivates from MSSoundRead so the MSSoundRead object MUST be the first of the MSOssRead object
       in order to the object mechanism to work*/
	MSSoundRead filter;
	MSFifo *f_outputs[MS_OSS_READ_MAX_INPUTS];
	MSSync *sync;
	SndCard *sndcard;
	gint freq;
	gint devid;  /* the sound device id it depends on*/
	gint gran;
	gint flags;
#define START_REQUESTED 1
#define STOP_REQUESTED  2
};

typedef struct _MSOssRead MSOssRead;

struct _MSOssReadClass
{
	/* the MSOssRead derivates from MSSoundRead, so the MSSoundRead class MUST be the first of the MSOssRead class
       in order to the class mechanism to work*/
	MSSoundReadClass parent_class;
};

typedef struct _MSOssReadClass MSOssReadClass;

/* PUBLIC */
#define MS_OSS_READ(filter) ((MSOssRead*)(filter))
#define MS_OSS_READ_CLASS(klass) ((MSOssReadClass*)(klass))
MSFilter * ms_oss_read_new(void);
gint ms_oss_read_set_device(MSOssRead *w,gint devid);
void ms_oss_read_start(MSOssRead *w);
void ms_oss_read_stop(MSOssRead *w);

/* FOR INTERNAL USE*/
void ms_oss_read_init(MSOssRead *r);
void ms_oss_read_class_init(MSOssReadClass *klass);
void ms_oss_read_destroy( MSOssRead *obj);
void ms_oss_read_process(MSOssRead *f);
void ms_oss_read_setup(MSOssRead *f, MSSync *sync);


#endif
