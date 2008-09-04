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


#include "msnosync.h"

static MSNoSyncClass *ms_nosync_class=NULL;

void ms_nosync_init(MSNoSync *sync)
{
	ms_sync_init(MS_SYNC(sync));
	MS_SYNC(sync)->attached_filters=sync->filters;
	memset(sync->filters,0,MSNOSYNC_MAX_FILTERS*sizeof(MSFilter*));
	MS_SYNC(sync)->samples_per_tick=160;
	sync->started=0;
}

void ms_nosync_class_init(MSNoSyncClass *klass)
{
	ms_sync_class_init(MS_SYNC_CLASS(klass));
	MS_SYNC_CLASS(klass)->max_filters=MSNOSYNC_MAX_FILTERS;
	MS_SYNC_CLASS(klass)->synchronize=(MSSyncSyncFunc)ms_nosync_synchronize;
	MS_SYNC_CLASS(klass)->destroy=(MSSyncDestroyFunc)ms_nosync_destroy;
	/* no need to overload these function*/
	MS_SYNC_CLASS(klass)->attach=ms_sync_attach_generic;
	MS_SYNC_CLASS(klass)->detach=ms_sync_detach_generic;
}

void ms_nosync_destroy(MSNoSync *nosync)
{
	g_free(nosync);
}

/* the synchronization function that does nothing*/
void ms_nosync_synchronize(MSNoSync *nosync)
{
	gint32 time;
	if (nosync->started==0){
		gettimeofday(&nosync->start,NULL);
		nosync->started=1;
	}
	gettimeofday(&nosync->current,NULL);
	MS_SYNC(nosync)->ticks++;
	/* update the time, we are supposed to work at 8000 Hz */
	time=((nosync->current.tv_sec-nosync->start.tv_sec)*1000) + 
		((nosync->current.tv_usec-nosync->start.tv_usec)/1000);
	MS_SYNC(nosync)->time=time;
	return;
}


MSSync *ms_nosync_new()
{
	MSNoSync *nosync;
	
	nosync=g_malloc(sizeof(MSNoSync));
	ms_nosync_init(nosync);
	if (ms_nosync_class==NULL)
	{
		ms_nosync_class=g_new(MSNoSyncClass,1);
		ms_nosync_class_init(ms_nosync_class);
	}
	MS_SYNC(nosync)->klass=MS_SYNC_CLASS(ms_nosync_class);
	return(MS_SYNC(nosync));
}
