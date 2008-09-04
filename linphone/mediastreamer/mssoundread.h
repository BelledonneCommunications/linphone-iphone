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
#ifndef MSSOUNDREAD_H
#define MSSOUNDREAD_H

#include "msfilter.h"
#include "mssync.h"



struct _MSSoundRead
{
	/* the MSOssRead derivates from MSFilter, so the MSFilter object MUST be the first of the MSOssRead object
       in order to the object mechanism to work*/
	MSFilter filter;
};

typedef struct _MSSoundRead MSSoundRead;

struct _MSSoundReadClass
{
	/* the MSOssRead derivates from MSFilter, so the MSFilter class MUST be the first of the MSOssRead class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
	gint (*set_device)(MSSoundRead *, gint devid);
	void (*start)(MSSoundRead *);
	void (*stop)(MSSoundRead*);
	void (*set_level)(MSSoundRead *, gint a);
};

typedef struct _MSSoundReadClass MSSoundReadClass;

/* PUBLIC */
#define MS_SOUND_READ(filter) ((MSSoundRead*)(filter))
#define MS_SOUND_READ_CLASS(klass) ((MSSoundReadClass*)(klass))

static inline int ms_sound_read_set_device(MSSoundRead *r,gint devid)
{
	return MS_SOUND_READ_CLASS( MS_FILTER(r)->klass )->set_device(r,devid);
}

static inline void ms_sound_read_start(MSSoundRead *r)
{
	MS_SOUND_READ_CLASS( MS_FILTER(r)->klass )->start(r);
}

static inline void ms_sound_read_stop(MSSoundRead *w)
{
	MS_SOUND_READ_CLASS( MS_FILTER(w)->klass )->stop(w);
}

static inline void ms_sound_read_set_level(MSSoundRead *w,gint a)
{
	MS_SOUND_READ_CLASS( MS_FILTER(w)->klass )->set_level(w,a);
}

/* FOR INTERNAL USE*/
void ms_sound_read_init(MSSoundRead *r);
void ms_sound_read_class_init(MSSoundReadClass *klass);


#endif

