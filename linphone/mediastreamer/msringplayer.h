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

#ifndef MSRINGPLAYER_H
#define MSRINGPLAYER_H

#include "msfilter.h"
#include "mssync.h"


/*this is the class that implements file reading source filter*/

#define MS_RING_PLAYER_MAX_OUTPUTS  1 /* max output per filter*/

#define MS_RING_PLAYER_DEF_GRAN 8192 /* the default granularity*/

#define MS_RING_PLAYER_END_OF_RING_EVENT 1

struct _MSRingPlayer
{
	/* the MSRingPlayer derivates from MSFilter, so the MSFilter object MUST be the first of the MSRingPlayer object
	in order to the object mechanism to work*/
	MSFilter filter;
	MSFifo *foutputs[MS_RING_PLAYER_MAX_OUTPUTS];
	MSQueue *qoutputs[MS_RING_PLAYER_MAX_OUTPUTS];\
	MSSync *sync;
	gint gran;
	gint freq;
	gint rate;
	gint channel;	/* number of interleaved channels */
	gint silence;	/* silence time between each ring, in seconds */
	gint state;
	gint fd;  /* the file descriptor of the file being read*/
	gint silence_bytes; /*silence in number of bytes between each ring */
	gint current_pos;
	gint need_swap;
};

typedef struct _MSRingPlayer MSRingPlayer;

struct _MSRingPlayerClass
{
	/* the MSRingPlayer derivates from MSFilter, so the MSFilter class MUST be the first of the MSRingPlayer class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
};

typedef struct _MSRingPlayerClass MSRingPlayerClass;

/* PUBLIC */
#define MS_RING_PLAYER(filter) ((MSRingPlayer*)(filter))
#define MS_RING_PLAYER_CLASS(klass) ((MSRingPlayerClass*)(klass))
MSFilter * ms_ring_player_new(char *name, gint seconds);
gint ms_ring_player_get_sample_freq(MSRingPlayer *obj);


/* FOR INTERNAL USE*/
void ms_ring_player_init(MSRingPlayer *r);
void ms_ring_player_class_init(MSRingPlayerClass *klass);
void ms_ring_player_destroy( MSRingPlayer *obj);
void ms_ring_player_process(MSRingPlayer *r);
#define ms_ring_player_set_bufsize(filter,sz) (filter)->gran=(sz)
void ms_ring_player_setup(MSRingPlayer *r,MSSync *sync);
#endif
