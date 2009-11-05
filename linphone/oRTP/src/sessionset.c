/*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc1889) stack.
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

#include <ortp/ortp.h>
#include <ortp/sessionset.h>
#include "scheduler.h"


/**
 * Allocates and initialize a new empty session set.
 *
 * @return the empty session set.
**/
SessionSet * session_set_new()
{
	SessionSet *set=(SessionSet *) ortp_malloc(sizeof(SessionSet));
	session_set_init(set);
	return set;
}


/**
 * Destroys a session set.
 *
**/

void session_set_destroy(SessionSet *set)
{
	ortp_free(set);
}

int session_set_and(SessionSet *sched_set, int maxs, SessionSet *user_set, SessionSet *result_set)
{
	uint32_t *mask1,*mask2,*mask3;
	int i=0;
	int j,ret=0;
	mask1=(uint32_t*)(void*)&sched_set->rtpset;
	mask2=(uint32_t*)(void*)&user_set->rtpset;
	mask3=(uint32_t*)(void*)&result_set->rtpset;
	while(i<maxs+1){
		*mask3=(*mask1) & (*mask2);	/* computes the AND between the two masks*/
		/* and unset the sessions that have been found from the sched_set */
		*mask1=(*mask1) & (~(*mask3));
		if ((*mask3)!=0){
			for (j=0;j<32;j++){
				if ( ((*mask3)>>j) & 1){
					ret++;
				}
			}
		}
		i+=32;
		mask1++;
		mask2++;
		mask3++;
	}
	//printf("session_set_and: ret=%i\n",ret);
	return ret;
}

/**
 *	This function performs similarly as libc select() function, but performs on #RtpSession 
 *	instead of file descriptors.
 *	session_set_select() suspends the calling process until some events arrive on one of the
 *	three sets passed in argument. Two of the sets can be NULL.
 *	The first set @recvs is interpreted as a set of RtpSession waiting for receive events:
 *	a new buffer (perhaps empty) is availlable on one or more sessions of the set, or the last
 *	receive operation with rtp_session_recv_with_ts() would have finished if it were in 
 *	blocking mode.
 *	The second set is interpreted as a set of RtpSession waiting for send events, i.e. the last
 *	rtp_session_send_with_ts() call on a session would have finished if it were in blocking mode.
 *	
 *	When some events arrived on some of sets, then the function returns and sets are changed
 *	to indicate the sessions where events happened.
 *	Sessions can be added to sets using session_set_set(), a session has to be tested to be 
 *	part of a set using session_set_is_set().
 *
 * @param recvs a set of rtp sessions to be watched for read events
 * @param sends a set of rtp sessions to be watched for write events
 * @param errors a set of rtp sessions to be watched for errors
 * @return: the number of sessions on which the selected events happened.
**/
int session_set_select(SessionSet *recvs, SessionSet *sends, SessionSet *errors)
{
	int ret=0,bits;
	SessionSet temp;
	RtpScheduler *sched=ortp_get_scheduler();
	
	/*lock the scheduler to not read the masks while they are being modified by the scheduler*/
	rtp_scheduler_lock(sched);
	
	while(1){
		/* computes the SessionSet intersection (in the other words mask intersection) between
		the mask given by the user and scheduler masks */
		if (recvs!=NULL){
			session_set_init(&temp);
			bits=session_set_and(&sched->r_sessions,sched->all_max,recvs,&temp);
			ret+=bits;
			/* copy the result set in the given user set (might be empty) */
			if (ret>0) session_set_copy(recvs,&temp);
		}
		if (sends!=NULL){
			session_set_init(&temp);
			bits=session_set_and(&sched->w_sessions,sched->all_max,sends,&temp);
			ret+=bits;
			if (ret>0){
				/* copy the result set in the given user set (might be empty)*/
				session_set_copy(sends,&temp);
			}
		}
		if (errors!=NULL){
			session_set_init(&temp);
			bits=session_set_and(&sched->e_sessions,sched->all_max,errors,&temp);
			ret+=bits;
			if (ret>0){
				/* copy the result set in the given user set */
				session_set_copy(errors,&temp);
			}
		}
		if (ret>0){
			/* there are set file descriptors, return immediately */
			//printf("There are %i sessions set, returning.\n",ret);
			rtp_scheduler_unlock(sched);
			return ret;
		}
		//printf("There are %i sessions set.\n",ret);
		/* else we wait until the next loop of the scheduler*/
		ortp_cond_wait(&sched->unblock_select_cond,&sched->lock);
	}

	return -1;
}

int session_set_timedselect(SessionSet *recvs, SessionSet *sends, SessionSet *errors,  struct timeval *timeout)
{
	int ret=0,bits;
	int remainingTime; // duration in ms
	SessionSet temp;
	RtpScheduler *sched;
	if (timeout==NULL)
		return session_set_select(recvs, sends, errors);
	sched=ortp_get_scheduler();
	remainingTime = timeout->tv_usec/1000 + timeout->tv_sec*1000;

	/*lock the scheduler to not read the masks while they are being modified by the scheduler*/
	rtp_scheduler_lock(sched);

	do {
		/* computes the SessionSet intersection (in the other words mask intersection) between
		the mask given by the user and scheduler masks */
		if (recvs!=NULL){
			session_set_init(&temp);
			bits=session_set_and(&sched->r_sessions,sched->all_max,recvs,&temp);
			ret+=bits;
			/* copy the result set in the given user set (might be empty) */
			if (ret>0) session_set_copy(recvs,&temp);
		}
		if (sends!=NULL){
			session_set_init(&temp);
			bits=session_set_and(&sched->w_sessions,sched->all_max,sends,&temp);
			ret+=bits;
			if (ret>0){
				/* copy the result set in the given user set (might be empty)*/
				session_set_copy(sends,&temp);
			}
		}
		if (errors!=NULL){
			session_set_init(&temp);
			bits=session_set_and(&sched->e_sessions,sched->all_max,errors,&temp);
			ret+=bits;
			if (ret>0){
				/* copy the result set in the given user set */
				session_set_copy(errors,&temp);
			}
		}
		if (ret>0){
			/* there are set file descriptors, return immediately */
			//printf("There are %i sessions set, returning.\n",ret);
			rtp_scheduler_unlock(sched);
			return ret;
		}
		//printf("There are %i sessions set.\n",ret);
		/* else we wait until the next loop of the scheduler*/
		ortp_cond_wait(&sched->unblock_select_cond,&sched->lock);
		remainingTime -= sched->timer_inc;
	} while (remainingTime>0);

	return -1;
}
