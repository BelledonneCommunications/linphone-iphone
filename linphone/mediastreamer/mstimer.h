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
#ifndef MSTIMER_H
#define MSTIMER_H

#include "mssync.h"
#include <sys/time.h>

#define MSTIMER_MAX_FILTERS 10

/* MSTimer derivates from MSSync base class*/

typedef struct _MSTimer
{
	/* the MSSync must be the first field of the object in order to the object mechanism to work*/
	MSSync  sync;
	MSFilter *filters[MSTIMER_MAX_FILTERS];
	gint milisec; /* the interval */
	struct timeval interval;
	struct timeval orig;
	gint state;
} MSTimer;


typedef struct _MSTimerClass
{
	/* the MSSyncClass must be the first field of the class in order to the class mechanism to work*/
	MSSyncClass parent_class;
} MSTimerClass;


/*private*/
#define MS_TIMER_RUNNING 1
#define MS_TIMER_STOPPED 0
void ms_timer_init(MSTimer *sync);
void ms_timer_class_init(MSTimerClass *sync);

void ms_timer_destroy(MSTimer *timer);
void ms_timer_synchronize(MSTimer *timer);

/*public*/
void ms_timer_set_interval(MSTimer *timer, gint milisec);

/* casts a MSSync object into a MSTimer */
#define MS_TIMER(sync) ((MSTimer*)(sync))
/* casts a MSSync class into a MSTimer class */
#define MS_TIMER_CLASS(klass) ((MSTimerClass*)(klass))

MSSync *ms_timer_new();

#endif
