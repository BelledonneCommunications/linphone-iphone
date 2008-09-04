/*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc3550) stack.
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

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "ortp/rtpsession.h"
#include "ortp/sessionset.h"
#include "rtptimer.h"


struct _RtpScheduler {
 
	RtpSession *list;	/* list of scheduled sessions*/
	SessionSet	all_sessions;  /* mask of scheduled sessions */
	int		all_max;		/* the highest pos in the all mask */
	SessionSet  r_sessions;		/* mask of sessions that have a recv event */
	int		r_max;
	SessionSet	w_sessions;		/* mask of sessions that have a send event */
	int 		w_max;
	SessionSet	e_sessions;	/* mask of session that have error event */
	int		e_max;
	int max_sessions;		/* the number of position in the masks */
  /* GMutex  *unblock_select_mutex; */
	ortp_cond_t   unblock_select_cond;
	ortp_mutex_t	lock;
	ortp_thread_t thread;
	int thread_running;
	struct _RtpTimer *timer;
	uint32_t time_;       /*number of miliseconds elapsed since the start of the thread */
	uint32_t timer_inc;	/* the timer increment in milisec */
};

typedef struct _RtpScheduler RtpScheduler;
	
RtpScheduler * rtp_scheduler_new(void);
void rtp_scheduler_set_timer(RtpScheduler *sched,RtpTimer *timer);
void rtp_scheduler_start(RtpScheduler *sched);
void rtp_scheduler_stop(RtpScheduler *sched);
void rtp_scheduler_destroy(RtpScheduler *sched);

void rtp_scheduler_add_session(RtpScheduler *sched, RtpSession *session);
void rtp_scheduler_remove_session(RtpScheduler *sched, RtpSession *session);

void * rtp_scheduler_schedule(void * sched);

#define rtp_scheduler_lock(sched)	ortp_mutex_lock(&(sched)->lock)
#define rtp_scheduler_unlock(sched)	ortp_mutex_unlock(&(sched)->lock)

/* void rtp_scheduler_add_set(RtpScheduler *sched, SessionSet *set); */

RtpScheduler * ortp_get_scheduler(void);
#endif
