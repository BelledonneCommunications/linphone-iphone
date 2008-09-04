/***************************************************************************
 *            mssdlout.h
 *
 *  Mon Jul 11 16:18:55 2005
 *  Copyright  2005  Simon Morlat
 *  Email simon dot morlat at linphone dot org
 ****************************************************************************/

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

#ifndef mssdlout_h
#define mssdlout_h

#include "msfilter.h"

#include <SDL/SDL.h>
#include <SDL/SDL_video.h>

struct _MSSdlOut
{
	MSFilter parent;
	MSQueue *input[2];
	gint width,height;
	const gchar *format;
	SDL_Surface *screen;
	SDL_Overlay *overlay;
	MSMessage *oldinm1;
	gboolean use_yuv;
};


typedef struct _MSSdlOut MSSdlOut;
	
struct _MSSdlOutClass
{
	MSFilterClass parent_class;
};

typedef struct _MSSdlOutClass MSSdlOutClass;
	
MSFilter * ms_sdl_out_new(void);
void ms_sdl_out_set_format(MSSdlOut *obj, const char *fmt);

#define MS_SDL_OUT(obj) ((MSSdlOut*)obj)

#endif
