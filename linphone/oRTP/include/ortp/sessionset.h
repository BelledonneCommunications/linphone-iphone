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
/** 
 * \file sessionset.h
 * \brief Sending and receiving multiple streams together with only one thread.
 *
**/
#ifndef SESSIONSET_H
#define SESSIONSET_H


#include <ortp/rtpsession.h>

#ifdef __cplusplus
extern "C"{
#endif


#if	!defined(_WIN32) && !defined(_WIN32_WCE)
/* UNIX */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define ORTP_FD_SET(d, s)     FD_SET(d, s)
#define ORTP_FD_CLR(d, s)     FD_CLR(d, s)
#define ORTP_FD_ISSET(d, s)   FD_ISSET(d, s)
#define ORTP_FD_ZERO(s)		  FD_ZERO(s)

typedef fd_set ortp_fd_set;


#else
/* WIN32 */

#define ORTP_FD_ZERO(s) \
  do {									      \
    unsigned int __i;							      \
    ortp_fd_set *__arr = (s);						      \
    for (__i = 0; __i < sizeof (ortp_fd_set) / sizeof (ortp__fd_mask); ++__i)	      \
      ORTP__FDS_BITS (__arr)[__i] = 0;					      \
  } while (0)
#define ORTP_FD_SET(d, s)     (ORTP__FDS_BITS (s)[ORTP__FDELT(d)] |= ORTP__FDMASK(d))
#define ORTP_FD_CLR(d, s)     (ORTP__FDS_BITS (s)[ORTP__FDELT(d)] &= ~ORTP__FDMASK(d))
#define ORTP_FD_ISSET(d, s)   ((ORTP__FDS_BITS (s)[ORTP__FDELT(d)] & ORTP__FDMASK(d)) != 0)



/* The fd_set member is required to be an array of longs.  */
typedef long int ortp__fd_mask;


/* Number of bits per word of `fd_set' (some code assumes this is 32).  */
#define ORTP__FD_SETSIZE 1024

/* It's easier to assume 8-bit bytes than to get CHAR_BIT.  */
#define ORTP__NFDBITS	(8 * sizeof (ortp__fd_mask))
#define	ORTP__FDELT(d)	((d) / ORTP__NFDBITS)
#define	ORTP__FDMASK(d)	((ortp__fd_mask) 1 << ((d) % ORTP__NFDBITS))


/* fd_set for select and pselect.  */
typedef struct
  {
    ortp__fd_mask fds_bits[ORTP__FD_SETSIZE / ORTP__NFDBITS];
# define ORTP__FDS_BITS(set) ((set)->fds_bits)
  } ortp_fd_set;


#endif /*end WIN32*/

struct _SessionSet
{
	ortp_fd_set rtpset;
};


typedef struct _SessionSet SessionSet;

#define session_set_init(ss)		ORTP_FD_ZERO(&(ss)->rtpset)

SessionSet * session_set_new(void);
/**
 * This macro adds the rtp session to the set.
 * @param ss a set (SessionSet object)
 * @param rtpsession a RtpSession
**/
#define session_set_set(ss,rtpsession)		ORTP_FD_SET((rtpsession)->mask_pos,&(ss)->rtpset)

/**
 * This macro tests if the session is part of the set. 1 is returned if true, 0 else.
 *@param ss a set (#SessionSet object)
 *@param rtpsession a rtp session
 *
**/
#define session_set_is_set(ss,rtpsession)	ORTP_FD_ISSET((rtpsession)->mask_pos,&(ss)->rtpset)

/**
 * Removes the session from the set.
 *@param ss a set of sessions.
 *@param rtpsession a rtp session.
 *
 *
**/
#define session_set_clr(ss,rtpsession)		ORTP_FD_CLR((rtpsession)->mask_pos,&(ss)->rtpset)

#define session_set_copy(dest,src)		memcpy(&(dest)->rtpset,&(src)->rtpset,sizeof(ortp_fd_set))


/**
 * Frees a SessionSet.
**/
void session_set_destroy(SessionSet *set);

	
int session_set_select(SessionSet *recvs, SessionSet *sends, SessionSet *errors);
int session_set_timedselect(SessionSet *recvs, SessionSet *sends, SessionSet *errors,  struct timeval *timeout);

#ifdef __cplusplus
}
#endif
	
#endif
