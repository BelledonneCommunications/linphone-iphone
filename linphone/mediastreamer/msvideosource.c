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


#include "msvideosource.h"
#include "mediastream.h"

#ifdef __linux
#include "msv4l.h"
#endif
#ifdef HAVE_LIBDC1394
#include "msdc1394.h"
#endif

/* register all statically linked codecs */
void ms_video_source_register_all()
{
#ifdef __linux
	ms_filter_register(&v4l_info);
#endif
#ifdef HAVE_LIBDC1394
	ms_filter_register(MS_FILTER_INFO(&dc1394_info));
#endif
}

void ms_video_source_class_init(MSVideoSourceClass *klass)
{
	/* init base class first*/
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	/* then init videosource specific things*/
	MS_FILTER_CLASS(klass)->max_qoutputs=MSVIDEOSOURCE_MAX_OUTPUTS;
	ms_filter_class_set_attr(MS_FILTER_CLASS(klass),FILTER_IS_SOURCE|FILTER_HAS_QUEUES);
}

void ms_video_source_init(MSVideoSource *obj)
{
	ms_filter_init(MS_FILTER(obj));
	MS_FILTER(obj)->outqueues=obj->outputs;
	obj->width = VIDEO_SIZE_CIF_W;
	obj->height = VIDEO_SIZE_CIF_H;
}

void ms_video_source_start(MSVideoSource *f)
{
	MS_VIDEO_SOURCE_CLASS(MS_FILTER(f)->klass)->start(f);
}

void ms_video_source_stop(MSVideoSource *f)
{
	MS_VIDEO_SOURCE_CLASS(MS_FILTER(f)->klass)->stop(f);
}

int ms_video_source_set_device(MSVideoSource *f, const gchar *device)
{
	return MS_VIDEO_SOURCE_CLASS(MS_FILTER(f)->klass)->set_device(f,device);
}

gchar* ms_video_source_get_device_name(MSVideoSource *f)
{
	return f->dev_name;
}

void ms_video_source_set_size(MSVideoSource *f, gint width, gint height)
{
	if (MS_VIDEO_SOURCE_CLASS(MS_FILTER(f)->klass)->set_size)
		MS_VIDEO_SOURCE_CLASS(MS_FILTER(f)->klass)->set_size(f, width, height);
}

void ms_video_source_set_frame_rate(MSVideoSource *f, gint frame_rate, gint frame_rate_base)
{
	if (MS_VIDEO_SOURCE_CLASS(MS_FILTER(f)->klass)->set_frame_rate)
		MS_VIDEO_SOURCE_CLASS(MS_FILTER(f)->klass)->set_frame_rate(f, frame_rate, frame_rate_base);
	else{
		f->frame_rate=frame_rate;
		f->frame_rate_base=frame_rate_base;
	}
}

gchar* ms_video_source_get_format(MSVideoSource *f)
{
	return f->format;
}
