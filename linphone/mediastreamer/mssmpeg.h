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

#ifndef MSSMPEG_H
#define MSSMPEG_H


#include "ms.h"
#include "msfilter.h"

#include <smpeg/smpeg.h>
#include <SDL/SDL.h>

struct _MSSmpeg
{
	MSFilter parent;
	MSQueue *input[1];
	SMPEG *handle;
	SDL_Surface *surface;
	SDL_RWops *rwops;
	int run_cond;
	int first_time;
	MSMessage *current;
	int pos;
	int end_pos;
};


typedef struct _MSSmpeg MSSmpeg;
	
struct _MSSmpegClass
{
	MSFilterClass parent_class;
};

typedef struct _MSSmpegClass MSSmpegClass;
	

#define MS_SMPEG(obj)	((MSSmpeg*)(obj))
#define MS_SMPEG_CLASS(klass)	((MSSmpegClass*)(klass))
	
void ms_smpeg_init(MSSmpeg *obj);
void ms_smpeg_class_init(MSSmpegClass *klass);
void ms_smpeg_uninit(MSSmpeg *obj);

MSFilter * ms_smpeg_new();
void ms_smpeg_start(MSSmpeg *obj);
void ms_smpeg_stop(MSSmpeg *obj);


void ms_smpeg_destroy(MSSmpeg *obj);
void ms_smpeg_process(MSSmpeg *obj);
#endif