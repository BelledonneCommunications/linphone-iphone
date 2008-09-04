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
#ifndef MSOSSWRITE_H
#define MSOSSWRITE_H

#include "mssoundwrite.h"
#include "sndcard.h"

/*this is the class that implements oss writing sink filter*/

#define MS_OSS_WRITE_MAX_INPUTS  1 /* max output per filter*/

#define MS_OSS_WRITE_DEF_GRAN (512*2) /* the default granularity*/

struct _MSOssWrite
{
	/* the MSOssWrite derivates from MSSoundWrite, so the MSSoundWrite object MUST be the first of the MSOssWrite object
       in order to the object mechanism to work*/
	MSSoundWrite filter;
	MSFifo *f_inputs[MS_OSS_WRITE_MAX_INPUTS];
	gint devid;  /* the sound device id it depends on*/
	SndCard *sndcard;
	gint bsize;
	gint freq;
	gint channels;
	gdouble lowfreq;
	gdouble highfreq;
	gint dtmf_time;
	gint dtmf_duration;
};

typedef struct _MSOssWrite MSOssWrite;

struct _MSOssWriteClass
{
	/* the MSOssWrite derivates from MSSoundWrite, so the MSSoundWrite class MUST be the first of the MSOssWrite class
       in order to the class mechanism to work*/
	MSSoundWriteClass parent_class;
};

typedef struct _MSOssWriteClass MSOssWriteClass;

/* PUBLIC */
#define MS_OSS_WRITE(filter) ((MSOssWrite*)(filter))
#define MS_OSS_WRITE_CLASS(klass) ((MSOssWriteClass*)(klass))
MSFilter * ms_oss_write_new(void);
gint ms_oss_write_set_device(MSOssWrite *w,gint devid);
void ms_oss_write_start(MSOssWrite *w);
void ms_oss_write_stop(MSOssWrite *w);
void ms_oss_write_set_level(MSOssWrite *w, gint level);
void ms_oss_write_play_dtmf(MSOssWrite *w, char dtmf);

/* FOR INTERNAL USE*/
void ms_oss_write_init(MSOssWrite *r);
void ms_oss_write_setup(MSOssWrite *r);
void ms_oss_write_class_init(MSOssWriteClass *klass);
void ms_oss_write_destroy( MSOssWrite *obj);
void ms_oss_write_process(MSOssWrite *f);


#endif
