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
#ifndef MSSOUNDWRITE_H
#define MSSOUNDWRITE_H

#include "msfilter.h"
#include "mssync.h"



struct _MSSoundWrite
{
	/* the MSOssWrite derivates from MSFilter, so the MSFilter object MUST be the first of the MSOssWrite object
       in order to the object mechanism to work*/
	MSFilter filter;
};

typedef struct _MSSoundWrite MSSoundWrite;

struct _MSSoundWriteClass
{
	/* the MSOssWrite derivates from MSFilter, so the MSFilter class MUST be the first of the MSOssWrite class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
	gint (*set_device)(MSSoundWrite *, gint devid);
	void (*start)(MSSoundWrite *);
	void (*stop)(MSSoundWrite*);
	void (*set_level)(MSSoundWrite *, gint a);
};

typedef struct _MSSoundWriteClass MSSoundWriteClass;

/* PUBLIC */
#define MS_SOUND_WRITE(filter) ((MSSoundWrite*)(filter))
#define MS_SOUND_WRITE_CLASS(klass) ((MSSoundWriteClass*)(klass))

static inline int ms_sound_write_set_device(MSSoundWrite *r,gint devid)
{
	return MS_SOUND_WRITE_CLASS( MS_FILTER(r)->klass )->set_device(r,devid);
}

static inline void ms_sound_write_start(MSSoundWrite *r)
{
	MS_SOUND_WRITE_CLASS( MS_FILTER(r)->klass )->start(r);
}

static inline void ms_sound_write_stop(MSSoundWrite *w)
{
	MS_SOUND_WRITE_CLASS( MS_FILTER(w)->klass )->stop(w);
}

static inline void ms_sound_write_set_level(MSSoundWrite *w,gint a)
{
	MS_SOUND_WRITE_CLASS( MS_FILTER(w)->klass )->set_level(w,a);
}

/* FOR INTERNAL USE*/
void ms_sound_write_init(MSSoundWrite *r);
void ms_sound_write_class_init(MSSoundWriteClass *klass);


#endif

