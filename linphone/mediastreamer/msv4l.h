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

#ifndef MSV4L_H
#define MSV4L_H

#include "msvideosource.h"
#include <sys/types.h>
#include <linux/videodev.h>

struct _MSV4l
{
	MSVideoSource parent;
	int fd;
	char *device;
	struct video_capability cap;
	struct video_channel channel;
	struct video_window win;
	struct video_picture pict;
	struct video_mmap vmap;
	struct video_mbuf vmbuf;
	struct video_capture vcap;
	gint bsize;
	gint use_mmap;
	gint frame;
	guint query_frame;
	gchar *mmapdbuf; /* the mmap'd buffer */
	MSBuffer img[VIDEO_MAX_FRAME];	/* the buffer wrappers used for mmaps */
	MSBuffer *allocdbuf; /* the buffer allocated for read() and mire */
	gint count;
	MSBuffer *image_grabbed;
	GCond *cond;
	GCond *stopcond;
	GThread *v4lthread;
	gboolean grab_image;
	gboolean thread_run;
	gboolean thread_exited;
};

typedef struct _MSV4l MSV4l;


struct _MSV4lClass
{
	MSVideoSourceClass parent_class;
	
};

typedef struct _MSV4lClass MSV4lClass;


/* PUBLIC API */
#define MS_V4L(v)		((MSV4l*)(v))
#define MS_V4L_CLASS(k)		((MSV4lClass*)(k))
MSFilter * ms_v4l_new();

void ms_v4l_start(MSV4l *obj);
void ms_v4l_stop(MSV4l *obj);
int ms_v4l_set_device(MSV4l *f, const gchar *device);
void ms_v4l_set_size(MSV4l *v4l, gint w, gint h);

/* PRIVATE API */
void ms_v4l_init(MSV4l *obj);
void ms_v4l_class_init(MSV4lClass *klass);
int v4l_configure(MSV4l *f);

void v4l_process(MSV4l *obj);

void ms_v4l_uninit(MSV4l *obj);

void ms_v4l_destroy(MSV4l *obj);

extern MSFilterInfo v4l_info;

#endif
