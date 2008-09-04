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



#ifndef MS_H
#define MS_H
#include "msfilter.h"
#include "mssync.h"


void ms_init();

/* compile graphs attached to a sync source*/
int ms_compile(MSSync *source);


/* stop the processing chain attached to a sync source.*/
void ms_thread_stop(MSSync *sync);


/**
 * function_name:ms_thread_run
 * @sync:  The synchronization source for all the set of graphs to run.
 *
 * Execute the processing chain attached to a sync source. This function loops indefinitely.
 * The media streamer programmer can choose to execute this function directly, or to call ms_start(),
 * that will start a thread for the synchronisation source.
 *
 * Returns: no return value.
 */
void *ms_thread_run(void *sync);


/**
 * function_name:ms_start
 * @sync: A synchronisation source to be started.
 *
 * Starts a thread that will shedule all processing chains attached to the synchronisation source @sync.
 *
 * Returns: no return value.
 */
void ms_start(MSSync *sync);


/**
 * function_name:ms_stop
 * @sync: A synchronisation source to be stopped.
 *
 * Stop the thread that was sheduling the processing chains attached to the synchronisation source @sync.
 * The processing chains are kept unchanged, no object is freed. The synchronisation source can be restarted using ms_start().
 *
 * Returns: no return value.
 */
void ms_stop(MSSync *sync);


gchar * ms_proc_get_param(gchar *parameter);
gint ms_proc_get_type();
gint ms_proc_get_speed();

void ms_load_plugins(const char *dir);

#endif
