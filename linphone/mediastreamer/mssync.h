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

#ifndef MS_SYNC_H
#define MS_SYNC_H


#include "msfilter.h"

struct _MSSync
{
	struct _MSSyncClass *klass;
	GMutex *lock;
	MSFilter **attached_filters; /* pointer to a table of pointer of filters*/
	GList *execution_list;     /* the list of filters to be executed. This is filled with compilation */
	gint filters;   /*number of filters attached to the sync */
	gint run;       /* flag to indicate whether the sync must be run or not */
	GThread * thread;   /* the thread ressource if this sync is run by a thread*/
	GCond *thread_cond;
	GCond *stop_cond;
	guint32 flags;
	gint interval; /* in miliseconds*/
#define MS_SYNC_NEED_UPDATE (0x0001)  /* a modification has occured in the processing chains
							attached to this sync; so the execution list has to be updated */
	guint samples_per_tick; /* number of bytes produced by sources of the processing chains*/
	guint32 ticks;
	guint32 time;	/* a time since the start of the sync expressed in milisec*/
};

typedef struct _MSSync MSSync;

typedef void (*MSSyncDestroyFunc)(MSSync*);
typedef void (*MSSyncSyncFunc)(MSSync*);
typedef int (*MSSyncAttachFunc)(MSSync*,MSFilter*);
typedef int (*MSSyncDetachFunc)(MSSync*,MSFilter*);

typedef struct _MSSyncClass
{
	gint max_filters;  /* the maximum number of filters that can be attached to this sync*/
	MSSyncSyncFunc synchronize;
	MSSyncDestroyFunc destroy;
	MSSyncAttachFunc attach;
	MSSyncDetachFunc detach;
} MSSyncClass;

/* private */
void ms_sync_init(MSSync *sync);
void ms_sync_class_init(MSSyncClass *klass);

int ms_sync_attach_generic(MSSync *sync,MSFilter *f);
int ms_sync_detach_generic(MSSync *sync,MSFilter *f);

/* public*/

#define MS_SYNC(sync) ((MSSync*)(sync))
#define MS_SYNC_CLASS(klass) ((MSSyncClass*)(klass))

#define ms_sync_synchronize(_sync) \
do       \
{         \
	MSSync *__sync=_sync; \
	__sync->ticks++;       \
	((__sync)->klass->synchronize((__sync))); \
}while(0)

void ms_sync_setup(MSSync *sync);

void ms_sync_unsetup(MSSync *sync);

#define ms_sync_update(sync) (sync)->flags|=MS_SYNC_NEED_UPDATE

#define ms_sync_get_samples_per_tick(sync) ((sync)->samples_per_tick)

void ms_sync_set_samples_per_tick(MSSync *sync,gint size);

#define ms_sync_get_tick_count(sync)  ((sync)->ticks)

#define ms_sync_suspend(sync) g_cond_wait((sync)->thread_cond,(sync)->lock)

#define ms_sync_lock(sync) g_mutex_lock((sync)->lock)

#define ms_sync_unlock(sync) g_mutex_unlock((sync)->lock)

#define ms_sync_trylock(sync) g_mutex_trylock((sync)->lock)

/**
 * function_name:ms_sync_attach
 * @sync:  A #MSSync object.
 * @f:  A #MSFilter object.
 *
 * Attach a chain of filters to a synchronisation source. Filter @f must be the first filter of the processing chain.
 *
 * Returns: 0 if successfull, a negative value reprensenting the errno.h error.
 */
int ms_sync_attach(MSSync *sync,MSFilter *f);

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
int ms_sync_detach(MSSync *sync,MSFilter *f);

int ms_sync_uninit(MSSync *sync);

#define ms_sync_start(sync)	ms_start((sync))
#define ms_sync_stop(sync)	ms_stop((sync))


/*destroy*/
#define ms_sync_destroy(sync)     (sync)->klass->destroy((sync))


#endif
