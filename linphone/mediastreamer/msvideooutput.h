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


#ifndef MSVIDEOOUTPUT_H
#define MSVIDEOOUTPUT_H


#include "ms.h"
#include "msfilter.h"
#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>
#include <gdk/gdk.h>

struct _MSVideoOutput
{
	MSFilter parent;
	MSQueue *input[2];
	GdkWindow *window;
	GdkGC *gc;
	GdkImage *image;
	gint width,height;
	gint bufsize;
	const gchar *palette;
	gdouble bpp;
	gint prev_w, prev_h;
	Display *xv_display;
	Window xv_window;
	GC xv_gc;
	XGCValues xv_values;
	XvImage *xv_image;
	unsigned int xv_port;
	XShmSegmentInfo xv_shminfo;
	gboolean active;
};


typedef struct _MSVideoOutput MSVideoOutput;
	
struct _MSVideoOutputClass
{
	MSFilterClass parent_class;
	GdkVisual *visual;
	GdkColormap *colormap;
};

typedef struct _MSVideoOutputClass MSVideoOutputClass;
	

#define MS_VIDEO_OUTPUT(obj)	((MSVideoOutput*)(obj))
#define MS_VIDEO_OUTPUT_CLASS(klass)	((MSVideoOutputClass*)(klass))
	
void ms_video_output_init(MSVideoOutput *obj);
void ms_video_output_class_init(MSVideoOutputClass *klass);
void ms_video_output_uninit(MSVideoOutput *obj);

MSFilter * ms_video_output_new();
void ms_video_output_set_format(MSVideoOutput *obj, const char *fmt);
void ms_video_output_set_size(MSVideoOutput *obj,gint width, gint height);
void ms_video_output_start(MSVideoOutput *obj);
void ms_video_output_stop(MSVideoOutput *obj);
void ms_video_output_set_title(MSVideoOutput *obj,gchar *title);

void ms_video_output_destroy(MSVideoOutput *obj);
void ms_video_output_process(MSVideoOutput *obj);
#endif
