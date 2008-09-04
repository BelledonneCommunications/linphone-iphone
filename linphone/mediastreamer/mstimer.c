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


#include "mstimer.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

static MSTimerClass *ms_timer_class=NULL;


void ms_timer_init(MSTimer *sync)
{
	ms_sync_init(MS_SYNC(sync));
	MS_SYNC(sync)->attached_filters=sync->filters;
	memset(sync->filters,0,MSTIMER_MAX_FILTERS*sizeof(MSFilter*));
	MS_SYNC(sync)->samples_per_tick=80;
	ms_timer_set_interval(sync,10);
	sync->state=MS_TIMER_STOPPED;
}

void ms_timer_class_init(MSTimerClass *klass)
{
	ms_sync_class_init(MS_SYNC_CLASS(klass));
	MS_SYNC_CLASS(klass)->max_filters=MSTIMER_MAX_FILTERS;
	MS_SYNC_CLASS(klass)->synchronize=(MSSyncSyncFunc)ms_timer_synchronize;
	MS_SYNC_CLASS(klass)->destroy=(MSSyncDestroyFunc)ms_timer_destroy;
	/* no need to overload these function*/
	MS_SYNC_CLASS(klass)->attach=ms_sync_attach_generic;
	MS_SYNC_CLASS(klass)->detach=ms_sync_detach_generic;
}

void ms_timer_destroy(MSTimer *timer)
{
	g_free(timer);
}


void ms_timer_synchronize(MSTimer *timer)
{
	//printf("ticks=%i \n",MS_SYNC(timer)->ticks);
	if (timer->state==MS_TIMER_STOPPED){
		timer->state=MS_TIMER_RUNNING;
		gettimeofday(&timer->orig,NULL);
		timer->sync.time=0;
	}
	else {
		gint32 diff,time;
		struct timeval tv,cur;
			
		timer->sync.time+=timer->milisec;
	
		gettimeofday(&cur,NULL);
		time=((cur.tv_usec-timer->orig.tv_usec)/1000 ) + ((cur.tv_sec-timer->orig.tv_sec)*1000 );
		while((diff = timer->sync.time-time) > 0)
		{
			tv.tv_sec = timer->milisec/1000;
			tv.tv_usec = (timer->milisec%1000)*1000;
			select(0,NULL,NULL,NULL,&tv);
			gettimeofday(&cur,NULL);
			time=((cur.tv_usec-timer->orig.tv_usec)/1000 ) + ((cur.tv_sec-timer->orig.tv_sec)*1000 );
		}
		if (diff<-50) g_warning("Must catchup %i miliseconds.",-diff);
	}
	
	return;
}


MSSync *ms_timer_new()
{
	MSTimer *timer;
	
	timer=g_malloc(sizeof(MSTimer));
	ms_timer_init(timer);
	if (ms_timer_class==NULL)
	{
		ms_timer_class=g_new(MSTimerClass,1);
		ms_timer_class_init(ms_timer_class);
	}
	MS_SYNC(timer)->klass=MS_SYNC_CLASS(ms_timer_class);
	return(MS_SYNC(timer));
}

void ms_timer_set_interval(MSTimer *timer, int milisec)
{
	
	MS_SYNC(timer)->ticks=0;
	MS_SYNC(timer)->interval=milisec;
	timer->interval.tv_sec=milisec/1000;
	timer->interval.tv_usec=(milisec % 1000)*1000;
	timer->milisec=milisec;
	
	
}
