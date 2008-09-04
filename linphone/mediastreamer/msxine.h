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

#ifndef MSXINE_H
#define MSXINE_H


#include "ms.h"
#include "msfilter.h"

#include <Xlib.h>
#include <xine.h>
#include <xine/video_out_x11.h>

struct _MSXine
{
	MSFilter parent;
	MSQueue *input[1];
	x11_visual_t vis;
	vo_driver_t *vo_driver;
	xine_t *engine;
};


typedef struct _MSXine MSXine;
	
struct _MSXineClass
{
	MSFilterClass parent_class;
	config_values_t *config;
	char **video_plugins;
	
};

typedef struct _MSXineClass MSXineClass;
	

#define MS_XINE(obj)	((MSXine*)(obj))
#define MS_XINE_CLASS(klass)	((MSXineClass*)(klass))
	
void ms_xine_init(MSXine *obj);
void ms_xine_class_init(MSXineClass *klass);
void ms_xine_uninit(MSXine *obj);

MSFilter * ms_xine_new();
void ms_xine_start(MSXine *obj);
void ms_xine_stop(MSXine *obj);


void ms_xine_destroy(MSXine *obj);

#endif