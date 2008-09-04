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

#include "msv4l.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

char *v4l_palette_string[17]={
	"none",
	"GREY", /* Linear greyscale */
	"HI240",	/* High 240 cube (BT848) */
	"RGB565",	/* 565 16 bit RGB */
	"RGB24",	/* 24bit RGB */
	"RGB32",	/* 32bit RGB */	
	"RGB555",	/* 555 15bit RGB */
	"YUV422",	/* YUV422 capture */
	"YUYV",	
	"UYVY",		/* The great thing about standards is ... */
	"YUV420",
	"YUV411",	/* YUV411 capture */
	"RAW",		/* RAW capture (BT848) */
	"YUV422P",	/* YUV 4:2:2 Planar */
	"YUV411P",	/* YUV 4:1:1 Planar */
	"YUV420P",	/* YUV 4:2:0 Planar */
	"YUV410P",	/* YUV 4:1:0 Planar */
};

#define V4L_PALETTE_TO_STRING(pal) v4l_palette_string[(pal)]

static int v4l_string_to_enum(const char *format){
	if (strcmp(format,"RGB24")==0) return VIDEO_PALETTE_RGB24;
	if (strcmp(format,"YUV420P")==0) return VIDEO_PALETTE_YUV420P;
	g_error("unsupported format, fixme");
	return -1;
}

MSFilterInfo v4l_info=
{
	"Video4Linux",
	0,
	MS_FILTER_VIDEO_IO,
	ms_v4l_new,
	NULL
};


static MSV4lClass *ms_v4l_class=NULL;

MSFilter * ms_v4l_new()
{
	MSV4l *obj;
	obj=g_malloc0(sizeof(MSV4l));
	if (ms_v4l_class==NULL)
	{
		ms_v4l_class=g_malloc0(sizeof(MSV4lClass));
		ms_v4l_class_init(ms_v4l_class);
	}
	MS_FILTER(obj)->klass=MS_FILTER_CLASS(ms_v4l_class);
	ms_v4l_init(obj);
	return MS_FILTER(obj);
}	

void ms_v4l_init(MSV4l *obj)
{
	ms_video_source_init(MS_VIDEO_SOURCE(obj));
	/* initialize the static buffer */
	obj->use_mmap=0;
	obj->fd=-1;
	obj->device = g_strdup("/dev/video0");
	obj->count=0;
	obj->allocdbuf=NULL;
	obj->grab_image=FALSE;
	obj->image_grabbed=NULL;
	obj->cond=g_cond_new();
	obj->stopcond=g_cond_new();
	obj->v4lthread=NULL;
	obj->thread_exited=FALSE;
	obj->frame=0;
	MS_VIDEO_SOURCE(obj)->format="YUV420P";	/*default value */
	MS_VIDEO_SOURCE(obj)->width = VIDEO_SIZE_CIF_W; /*default value */
	MS_VIDEO_SOURCE(obj)->height = VIDEO_SIZE_CIF_H; /*default value */
}

void ms_v4l_class_init(MSV4lClass *klass)
{
	ms_video_source_class_init(MS_VIDEO_SOURCE_CLASS(klass));
	MS_VIDEO_SOURCE_CLASS(klass)->start=(void (*)(MSVideoSource *))ms_v4l_start;
	MS_VIDEO_SOURCE_CLASS(klass)->stop=(void (*)(MSVideoSource *))ms_v4l_stop;
	MS_VIDEO_SOURCE_CLASS(klass)->set_device=(int (*)(MSVideoSource*,const gchar*))ms_v4l_set_device;
	MS_FILTER_CLASS(klass)->process=(void (*)(MSFilter *))v4l_process;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_v4l_destroy;
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"msv4l");
	MS_FILTER_CLASS(klass)->info=(MSFilterInfo*)&v4l_info;
}

void *v4l_thread(MSV4l *obj);

void ms_v4l_start(MSV4l *obj)
{
	int err;
	ms_filter_lock(MS_FILTER(obj));
	obj->fd=open(obj->device,O_RDONLY);
	if (obj->fd<0)
	{
		g_warning("MSV4l: cannot open video device: %s.",strerror(errno));
		MS_VIDEO_SOURCE(obj)->format="RGB24";
	}else{
		err=v4l_configure(obj);
		if (err<0) 
		{
			g_warning("MSV4l: could not get configuration of video device");
		}
	}
	obj->thread_exited=FALSE;
	obj->v4lthread=g_thread_create((GThreadFunc)v4l_thread,(gpointer)obj,FALSE,NULL);
	while(!obj->thread_run) g_cond_wait(obj->cond,MS_FILTER(obj)->lock);
	ms_filter_unlock(MS_FILTER(obj));
}

void ms_v4l_stop(MSV4l *obj)
{
	ms_filter_lock(MS_FILTER(obj));
	obj->thread_run=FALSE;
	obj->grab_image=FALSE;
	g_cond_signal(obj->cond);
	while(!obj->thread_exited) g_cond_wait(obj->stopcond,MS_FILTER(obj)->lock);
	if (obj->fd>0)
	{
		close(obj->fd);
		obj->fd=-1;
		if (!obj->use_mmap){
			if (obj->allocdbuf!=NULL) ms_buffer_destroy(obj->allocdbuf);
			obj->allocdbuf=NULL;
		}else
		{
			munmap(obj->mmapdbuf,obj->vmbuf.size);
			obj->mmapdbuf=NULL;
		}
		obj->image_grabbed=NULL;
	}
	
	obj->v4lthread=NULL;
	ms_filter_unlock(MS_FILTER(obj));
}

int ms_v4l_set_device(MSV4l *obj, const gchar *device)
{
	if (obj->device!=NULL) g_free(obj->device);
	obj->device=g_strdup(device);
	return 0;
}

void ms_v4l_set_size(MSV4l *obj, gint width, gint height)
{
	gint err;
	gboolean restart = FALSE;
	
	if (obj->fd == -1)
	{
		obj->fd = open(obj->device, O_RDONLY);
		if (obj->fd < 0)
		{
			g_warning("MSV4l: cannot open video device: %s.",strerror(errno));
			return;
		}
	} else
		restart = TRUE;
	
	ms_filter_lock(MS_FILTER(obj));
	err = ioctl(obj->fd, VIDIOCGCAP, &obj->cap);
	if (err != 0)
	{
		g_warning("MSV4l: cannot get device capabilities: %s.",strerror(errno));
		return;
	}
	if (width <= obj->cap.maxwidth && width >= obj->cap.minwidth &&
		height <= obj->cap.maxheight && height >= obj->cap.minheight)
	{
		MS_VIDEO_SOURCE(obj)->width = width;
		MS_VIDEO_SOURCE(obj)->height = height;
	}
	ms_filter_unlock(MS_FILTER(obj));
	
	if (restart)
	{
		ms_v4l_stop(obj);
		ms_v4l_start(obj);
	}
		
}

static gboolean try_format(MSV4l *obj, struct video_picture *pict, int palette, int depth){
	int err;
	pict->palette=palette;
	pict->depth=depth;
	pict->contrast=25000;
	err=ioctl(obj->fd,VIDIOCSPICT,pict);
	if (err<0){
		g_warning("Could not set picture properties: %s",strerror(errno));
		return FALSE;
	}
	return TRUE;
}

int v4l_configure(MSV4l *f)
{
	gint err;
	gint i;
	struct video_channel *chan=&f->channel;
	struct video_picture *pict=&f->pict;
	struct video_mmap *vmap=&f->vmap;
	struct video_mbuf *vmbuf=&f->vmbuf;
//	struct video_capture *vcap=&f->vcap;
	int found=0;
	
	err=ioctl(f->fd,VIDIOCGCAP,&f->cap);
	if (err!=0)
	{
		g_warning("MSV4l: cannot get device capabilities: %s.",strerror(errno));
		return -1;
	}
	MS_VIDEO_SOURCE(f)->dev_name=f->cap.name;
	
	for (i=0;i<f->cap.channels;i++)
	{
		chan->channel=i;
		err=ioctl(f->fd,VIDIOCGCHAN,chan);
		if (err==0)
		{
			g_message("Getting video channel %s",chan->name);
			switch(chan->type){
				case VIDEO_TYPE_TV:
					g_message("Channel is a TV.");
				break;
				case VIDEO_TYPE_CAMERA:
					g_message("Channel is a camera");
				break;
				default:
					g_warning("unknown video channel type.");
			}
			found=1;
			break;  /* find the first channel */
		}
	}
	if (found) g_message("A valid video channel was found.");
	/* select this channel */
	ioctl(f->fd,VIDIOCSCHAN,chan);
	
	/* set/get the resolution */
	err = -1;

	/*
	if (f->cap.type & VID_TYPE_SUBCAPTURE) {
		struct video_capture vcap;
		vcap.x = vcap.y = 0;
		vcap.width = MS_VIDEO_SOURCE(f)->width;
		vcap.height = MS_VIDEO_SOURCE(f)->height;
		err = ioctl(f->fd, VIDIOCSCAPTURE, &vcap);
		
	}
	*/
	
	/* get picture properties */
	err=ioctl(f->fd,VIDIOCGPICT,pict);
	if (err<0){
		g_warning("Could not get picture properties: %s",strerror(errno));
		return -1;
	}
	g_message("Default picture properties: brightness=%i,hue=%i,colour=%i,contrast=%i,depth=%i, palette=%i.",
						pict->brightness,pict->hue,pict->colour, pict->contrast,pict->depth, pict->palette);
	/* trying YUV420P format:*/
	if (try_format(f,pict,VIDEO_PALETTE_YUV420P,16)){
		g_message("Driver supports YUV420P, using that format.");
		MS_VIDEO_SOURCE(f)->format="YUV420P";
	}else{
		g_message("Driver does not support YUV420P, trying RGB24...");
		if (try_format(f, pict,VIDEO_PALETTE_RGB24,24)){
			g_message("Driver supports RGB24, using that format.");
			MS_VIDEO_SOURCE(f)->format="RGB24";
		}else{
			g_error("Unsupported video formats.");
		}
	}
	
	f->bsize=(pict->depth/8) * MS_VIDEO_SOURCE(f)->width* MS_VIDEO_SOURCE(f)->height;
	
	/* try to get mmap properties */
	err=ioctl(f->fd,VIDIOCGMBUF,vmbuf);
	if (err<0){
		g_warning("Could not get mmap properties: %s",strerror(errno));
		f->use_mmap=0;
	}else 
	{
		if (vmbuf->size>0){
			f->use_mmap=1;
			/* do the mmap */
			f->mmapdbuf=mmap((void*)f,vmbuf->size,PROT_READ,MAP_SHARED,f->fd,0);
			if (f->mmapdbuf==(void*)-1) {
				g_warning("Could not mmap. Using read instead: %s",strerror(errno));
				f->use_mmap=0;
				f->mmapdbuf=NULL;
			}else {
				/* initialize the mediastreamer buffers */
				gint i;
				g_message("Using %i-frames mmap'd buffer.",vmbuf->frames);
				for(i=0;i<vmbuf->frames;i++){
					f->img[i].buffer=f->mmapdbuf+vmbuf->offsets[i];
					f->img[i].size=vmbuf->offsets[1];
					f->img[i].ref_count=1;
				}
				f->frame=0;
			}
		} else g_warning("This device cannot support mmap.");
	}
	
	/* initialize the video map structure */
	vmap->width=MS_VIDEO_SOURCE(f)->width;
	vmap->height=MS_VIDEO_SOURCE(f)->height;
	vmap->format=v4l_string_to_enum(MS_VIDEO_SOURCE(f)->format);
	vmap->frame=0;
	return 0;
}	

#define BPP 3
static inline
void crop( guchar *src, gint s_width, gint s_height, guchar *dest, gint d_width, gint d_height)
{
	register int i;
	register int stride = d_width*BPP;
	register guchar *s = src, *d = dest;
	s += ((s_height - d_height)/2 * s_width * BPP) + ((s_width - d_width)/2 * BPP);
	for (i = 0; i < d_height; i++, d += stride, s += s_width * BPP)
		memcpy( d, s, stride);
}

MSBuffer * v4l_grab_image_mmap(MSV4l *obj){
	struct video_mmap *vmap=&obj->vmap;
	struct video_mbuf *vmbuf=&obj->vmbuf;
	int err;
	int syncframe;
	int jitter=vmbuf->frames-1;
	obj->query_frame=(obj->frame) % vmbuf->frames;
	/*g_message("v4l_mmap_process: query_frame=%i",
			obj->query_frame);*/
	vmap->frame=obj->query_frame;
	err=ioctl(obj->fd,VIDIOCMCAPTURE,vmap);
	if (err<0) {
		g_warning("v4l_mmap_process: error in VIDIOCMCAPTURE: %s.",strerror(errno));
		return NULL;
	}
	/*g_message("v4l_mmap_process: query_frame=%i done",
			obj->query_frame);*/
	syncframe=(obj->frame-jitter);
	obj->frame++;
	if (syncframe>=0){
		syncframe=syncframe%vmbuf->frames;
		g_message("Syncing on frame %i",syncframe);
		err=ioctl(obj->fd,VIDIOCSYNC,&syncframe);
		if (err<0) {
			g_warning("v4l_mmap_process: error in VIDIOCSYNC: %s.",strerror(errno));	
			return NULL;
		}
		/*g_message("got frame %i",syncframe);*/
	}else {
		return NULL;
	}
	/* not particularly efficient - hope for a capture source that 
	   provides subcapture or setting window */
	/*
	if (obj->width != MS_VIDEO_SOURCE(obj)->width || obj->height != MS_VIDEO_SOURCE(obj)->height){
		guchar tmp[obj->bsize];
		crop((guchar*) obj->img[syncframe].buffer, obj->width, obj->height, tmp,
			MS_VIDEO_SOURCE(obj)->width, MS_VIDEO_SOURCE(obj)->height);
		memcpy(obj->img[syncframe].buffer, tmp, MS_VIDEO_SOURCE(obj)->width *
			MS_VIDEO_SOURCE(obj)->height * obj->pict.depth/8);
	}
	*/
	return &obj->img[syncframe];
}

MSBuffer *v4l_grab_image_read(MSV4l *obj){
	int err;
	if (obj->allocdbuf==NULL){
		obj->allocdbuf=ms_buffer_new(obj->bsize);
		obj->allocdbuf->ref_count++;
	}
	{
		err=read(obj->fd,obj->allocdbuf->buffer,obj->bsize);
		if (err<0){
			g_warning("MSV4l: Fail to read(): %s",strerror(errno));
			return NULL;
		}
	}
	return obj->allocdbuf;
}


MSBuffer * v4l_make_mire(MSV4l *obj){
	gchar *data;
	int i,j,line,pos;
	int patternw=obj->parent.width/6; 
	int patternh=obj->parent.height/6;
	int red,green=0,blue=0;
	if (obj->allocdbuf==NULL){
		obj->allocdbuf=ms_buffer_new(obj->parent.width*obj->parent.height*3);
		obj->allocdbuf->ref_count++;
	}
	data=obj->allocdbuf->buffer;
	for (i=0;i<obj->parent.height;++i){
		line=i*obj->parent.width*3;
		if ( ((i+obj->count)/patternh) & 0x1) red=255;
		else red= 0;
		for (j=0;j<obj->parent.width;++j){
			pos=line+(j*3);
			
			if ( ((j+obj->count)/patternw) & 0x1) blue=255;
			else blue= 0;
			
			data[pos]=red;
			data[pos+1]=green;
			data[pos+2]=blue;
		}
	}
	obj->count++;
	usleep(60000);
	return obj->allocdbuf;
}


void *v4l_thread(MSV4l *obj){
	GMutex *mutex=MS_FILTER(obj)->lock;
	g_mutex_lock(mutex);
	obj->thread_run=TRUE;
	g_cond_signal(obj->cond);
	while(obj->thread_run){
		g_cond_wait(obj->cond,mutex);
		if (obj->grab_image){
			MSBuffer *grabbed;
			g_mutex_unlock(mutex);
			if (obj->fd>0){
				if (obj->use_mmap){
					grabbed=v4l_grab_image_mmap(obj);
				}else{
					grabbed=v4l_grab_image_read(obj);
				}
			}else grabbed=v4l_make_mire(obj);
			g_mutex_lock(mutex);
			if (grabbed){
				obj->image_grabbed=grabbed;
				obj->grab_image=FALSE;
			}
		}
	}
	g_cond_signal(obj->stopcond);
	obj->thread_exited=TRUE;
	g_mutex_unlock(mutex);
	return NULL;
}




void v4l_process(MSV4l * obj)
{
	GMutex *mutex=MS_FILTER(obj)->lock;
	g_mutex_lock(mutex);
	if (obj->image_grabbed!=NULL){
		MSMessage *m=ms_message_alloc();
		ms_message_set_buf(m,obj->image_grabbed);
		ms_queue_put(MS_FILTER(obj)->outqueues[0],m);
		obj->image_grabbed=NULL;
	}else{
		obj->grab_image=TRUE;
		g_cond_signal(obj->cond);
	}
	g_mutex_unlock(mutex);
}

void ms_v4l_uninit(MSV4l *obj)
{
	if (obj->device!=NULL) {
		g_free(obj->device);
		obj->device=NULL;
	}
	if (obj->v4lthread!=NULL) ms_v4l_stop(obj);
	if (obj->allocdbuf!=NULL) {
		ms_buffer_destroy(obj->allocdbuf);
		obj->allocdbuf=NULL;
	}
	g_cond_free(obj->cond);
	g_cond_free(obj->stopcond);
	ms_filter_uninit(MS_FILTER(obj));
}

void ms_v4l_destroy(MSV4l *obj)
{
	ms_v4l_uninit(obj);
	g_free(obj);
}
