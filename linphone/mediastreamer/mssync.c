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

#include "mssync.h"
#include <errno.h>

/* TODO:
	-define an uninit function that free the mutex
*/

/**
 * function_name:ms_sync_get_bytes_per_tick
 * @sync:  A #MSSync object.
 *
 * Returns the number of bytes per tick. This is a usefull information for sources, so
 * that they can know how much data they must deliver each time they are called.
 *
 */

/* private */
void ms_sync_init(MSSync *sync)
{	
	sync->klass=NULL;  
	sync->lock=g_mutex_new();
	sync->thread_cond=g_cond_new();
	sync->stop_cond=g_cond_new();
	sync->attached_filters=NULL;
	sync->execution_list=NULL;
	sync->filters=0;
	sync->run=0;
	sync->flags=0;
	sync->samples_per_tick=0;
	sync->ticks=0;
	sync->time=0;
	sync->thread=NULL;
}

void ms_sync_class_init(MSSyncClass *klass)
{
	klass->max_filters=0;
	klass->synchronize=NULL;
	klass->attach=ms_sync_attach_generic;
	klass->detach=ms_sync_detach_generic;
	klass->destroy=NULL;
}

/* public*/


/**
 * ms_sync_attach:
 * @sync:  A #MSSync object.
 * @f:  A #MSFilter object.
 *
 * Attach a chain of filters to a synchronisation source @sync. Filter @f must be the first filter of the processing chain.
 *  In order to be run, each chain of filter must be attached to a synchronisation source, that will be responsible for scheduling
 *  the processing. Multiple chains can be attached to a single synchronisation.
 *
 * Returns: 0 if successfull, a negative value reprensenting the errno.h error.
 */
int ms_sync_attach(MSSync *sync,MSFilter *f)
{
	gint err;
	ms_sync_lock(sync);
	err=sync->klass->attach(sync,f);
	ms_sync_update(sync);
	ms_sync_unlock(sync);
	return(err);
}

int ms_sync_attach_generic(MSSync *sync,MSFilter *f)
{
	int i;
	//printf("attr: %i\n",f->klass->attributes);
	g_return_val_if_fail(f->klass->attributes & FILTER_IS_SOURCE,-EINVAL);
	g_return_val_if_fail(sync->attached_filters!=NULL,-EFAULT);
	

	/* find a free place to attach*/
	for (i=0;i<sync->klass->max_filters;i++)
	{
		if (sync->attached_filters[i]==NULL)
		{
			sync->attached_filters[i]=f;
			sync->filters++;
			ms_trace("Filter succesfully attached to sync.");
			return 0;
		}
	}
	g_warning("No more link on sync !");
	return(-EMLINK);
}

/**
 * ms_sync_detach:
 * @sync:  A #MSSync object.
 * @f:  A #MSFilter object.
 *
 * Dettach a chain of filters to a synchronisation source. Filter @f must be the first filter of the processing chain.
 * The processing chain will no more be executed.
 *
 * Returns: 0 if successfull, a negative value reprensenting the errno.h error.
 */
int ms_sync_detach(MSSync *sync,MSFilter *f)
{	
	gint err;
	ms_sync_lock(sync);
	err=sync->klass->detach(sync,f);
	ms_sync_update(sync);
	ms_sync_unlock(sync);
	return(err);
}

int ms_sync_detach_generic(MSSync *sync,MSFilter *f)
{
	int i;
	g_return_val_if_fail(f->klass->attributes & FILTER_IS_SOURCE,-EINVAL);
	g_return_val_if_fail(sync->attached_filters!=NULL,-EFAULT);
	for (i=0;i<sync->filters;i++)
	{
		if (sync->attached_filters[i]==f)
		{
			sync->attached_filters[i]=NULL;
			sync->filters--;
			return 0;
		}
	}
	return(-EMLINK);
}

void ms_sync_set_samples_per_tick(MSSync *sync,gint size)
{
	if (sync->samples_per_tick==0)
	{
		sync->samples_per_tick=size;
		g_cond_signal(sync->thread_cond);
	}
	else sync->samples_per_tick=size;
}

/* call the setup func of each filter attached to the graph */
void ms_sync_setup(MSSync *sync)
{
	GList *elem=sync->execution_list;
	MSFilter *f;
	while(elem!=NULL){
		f=(MSFilter*)elem->data;
		if (f->klass->setup!=NULL){
			f->klass->setup(f,sync);
		}
		elem=g_list_next(elem);
	}
}

/* call the unsetup func of each filter attached to the graph */
void ms_sync_unsetup(MSSync *sync)
{
	GList *elem=sync->execution_list;
	MSFilter *f;
	while(elem!=NULL){
		f=(MSFilter*)elem->data;
		if (f->klass->unsetup!=NULL){
			f->klass->unsetup(f,sync);
		}
		elem=g_list_next(elem);
	}
}


int ms_sync_uninit(MSSync *sync)
{
	g_mutex_free(sync->lock);
	g_cond_free(sync->thread_cond);
	g_cond_free(sync->stop_cond);
	return 0;
}

