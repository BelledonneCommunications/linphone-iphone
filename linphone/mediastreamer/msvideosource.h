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

#ifndef MSVIDEOSOURCE_H
#define MSVIDEOSOURCE_H


#include "msfilter.h"

/* this is the video input abstract class */

#define MSVIDEOSOURCE_MAX_OUTPUTS  1 /* max output per filter*/

typedef struct _MSVideoSource
{
    /* the MSVideoSource derivates from MSFilter, so the MSFilter object MUST be the first of the MSVideoSource object
       in order to the object mechanism to work*/
    MSFilter filter;
    MSQueue *outputs[MSVIDEOSOURCE_MAX_OUTPUTS];
    gchar *dev_name;
	gint width, height;
	gchar *format;
	gint frame_rate;
	gint frame_rate_base;
} MSVideoSource;

typedef struct _MSVideoSourceClass
{
	/* the MSVideoSource derivates from MSFilter, so the MSFilter class MUST be the first of the MSVideoSource class
       in order to the class mechanism to work*/
	MSFilterClass parent_class;
	gint (*set_device)(MSVideoSource *s, const gchar *name);
	void (*start)(MSVideoSource *s);
	void (*stop)(MSVideoSource *s);
	void (*set_size)(MSVideoSource *s, gint width, gint height);
	void (*set_frame_rate)(MSVideoSource *s, gint frame_rate, gint frame_rate_base);
} MSVideoSourceClass;

/* PUBLIC */
void ms_video_source_register_all();
int ms_video_source_set_device(MSVideoSource *f, const gchar *device);
gchar* ms_video_source_get_device_name(MSVideoSource *f);
void ms_video_source_start(MSVideoSource *f);
void ms_video_source_stop(MSVideoSource *f);
void ms_video_source_set_size(MSVideoSource *f, gint width, gint height);
void ms_video_source_set_frame_rate(MSVideoSource *f, gint frame_rate, gint frame_rate_base);
gchar* ms_video_source_get_format(MSVideoSource *f);

#define MS_VIDEO_SOURCE(obj)		((MSVideoSource*)(obj))
#define MS_VIDEO_SOURCE_CLASS(klass)		((MSVideoSourceClass*)(klass))


/* FOR INTERNAL USE*/
void ms_video_source_init(MSVideoSource *f);
void ms_video_source_class_init(MSVideoSourceClass *klass);

#endif
