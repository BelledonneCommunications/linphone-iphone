/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef __linux

#include "mediastreamer-config.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

#include <linux/videodev.h>
#ifdef HAVE_LINUX_VIDEODEV2_H
#include <linux/videodev2.h>
#endif

#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/msv4l.h"
#include "mediastreamer2/mswebcam.h"
#include "nowebcam.h"

/* From: Logitech QuickCam USB driver */
#define QC_IOCTLBASE	    220
/* Get enable workaround for bugs, bitfield */
#define VIDIOCQCGCOMPATIBLE _IOR ('v',QC_IOCTLBASE+10,int)
/* Set enable workaround for bugs, bitfield */
#define VIDIOCQCSCOMPATIBLE _IOWR('v',QC_IOCTLBASE+10,int)

#ifndef VIDIOSFPS
#define VIDIOSFPS _IOW('v',BASE_VIDIOCPRIVATE+20, int)
#endif

typedef struct V4lState{
	int fd;
	ms_thread_t thread;
	char *dev;
	char *mmapdbuf;
	int msize;/*mmapped size*/
	MSVideoSize vsize;
	MSVideoSize got_vsize;
	int pix_fmt;
	int int_pix_fmt; /*internal pixel format */
	mblk_t *frames[VIDEO_MAX_FRAME];
	mblk_t *mire;
	queue_t rq;
	ms_mutex_t mutex;
	int frame_ind;
	int frame_max;
	float fps;
	float start_time;
	int frame_count;
	int queued;
	bool_t run;
	bool_t usemire;
	bool_t v4lv2; /*we interface with a V4Lv2 driver */
	bool_t force_v1;
	bool_t auto_started;
}V4lState;

static void *v4l_thread(void *s);
static int v4l_configure(V4lState *s);

#ifdef HAVE_LINUX_VIDEODEV2_H

static bool_t v4lv2_try_format(V4lState *s, int fmtid){
	struct v4l2_format fmt;

	memset(&fmt,0,sizeof(fmt));

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = s->vsize.width; 
	fmt.fmt.pix.height      = s->vsize.height;
	fmt.fmt.pix.pixelformat = fmtid;
	fmt.fmt.pix.field = V4L2_FIELD_ANY;

        if (ioctl (s->fd, VIDIOC_S_FMT, &fmt)<0){
		return FALSE;
	}
	s->got_vsize.width=s->vsize.width;
	s->got_vsize.height=s->vsize.height;
	return TRUE;
}

static int v4lv2_configure(V4lState *s)
{
	struct v4l2_capability cap;
	
        if (ioctl (s->fd, VIDIOC_QUERYCAP, &cap)<0) {
		ms_message("Not a v4lv2 driver.");
		return -1;
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		ms_error("%s is not a video capture device\n",s->dev);
		return -1;
        }

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		ms_error("%s does not support streaming i/o\n",s->dev);
		return -1;
	}

	if (v4lv2_try_format(s,V4L2_PIX_FMT_YUV420)){
		s->pix_fmt=MS_YUV420P;
		s->int_pix_fmt=V4L2_PIX_FMT_YUV420;
		ms_message("v4lv2: YUV420P choosen");
	}else if (v4lv2_try_format(s,V4L2_PIX_FMT_NV12)){
		s->pix_fmt=MS_YUV420P;
		s->int_pix_fmt=V4L2_PIX_FMT_NV12;
		ms_message("v4lv2: V4L2_PIX_FMT_NV12 choosen");
	}else if (v4lv2_try_format(s,V4L2_PIX_FMT_MJPEG)){
		s->pix_fmt=MS_MJPEG;
		s->int_pix_fmt=V4L2_PIX_FMT_MJPEG;
		ms_message("v4lv2: MJPEG choosen");
	}else if (v4lv2_try_format(s,V4L2_PIX_FMT_YUYV)){
		s->pix_fmt=MS_YUYV;
		s->int_pix_fmt=V4L2_PIX_FMT_YUYV;
		ms_message("v4lv2: V4L2_PIX_FMT_YUYV choosen");
	}else if (v4lv2_try_format(s,V4L2_PIX_FMT_RGB24)){
		s->pix_fmt=MS_RGB24;
		s->int_pix_fmt=V4L2_PIX_FMT_RGB24;
		ms_message("v4lv2: RGB24 choosen");
	}else{
		ms_error("Could not find supported pixel format.");
		return -1;
	}
	return 0;
}

static int v4lv2_do_mmap(V4lState *s){
	struct v4l2_requestbuffers req;
	int i;
	enum v4l2_buf_type type;
	
	memset(&req,0,sizeof(req));
	
	req.count               = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;
	
	if (ioctl (s->fd, VIDIOC_REQBUFS, &req)<0) {
		ms_error("Error requesting info on mmap'd buffers: %s",strerror(errno));
		return -1;
	}
	
	for (i=0; i<req.count; ++i) {
		struct v4l2_buffer buf;
		mblk_t *msg;
		void *start;
		memset(&buf,0,sizeof(buf));
	
		buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory=V4L2_MEMORY_MMAP;
		buf.index=i;
	
		if (ioctl (s->fd, VIDIOC_QUERYBUF, &buf)<0){
			ms_error("Could not VIDIOC_QUERYBUF : %s",strerror(errno));
			return -1;
		}
		
		start=mmap (NULL /* start anywhere */,
			buf.length,
			PROT_READ | PROT_WRITE /* required */,
			MAP_SHARED /* recommended */,
			s->fd, buf.m.offset);
	
		if (start==NULL){
			ms_error("Could not mmap: %s",strerror(errno));
		}
		msg=esballoc(start,buf.length,0,NULL);
		/* adjust to real size of picture*/
		if (s->pix_fmt==MS_RGB24)
			msg->b_wptr+=s->vsize.width*s->vsize.height*3;
		else 
			msg->b_wptr+=(s->vsize.width*s->vsize.height*3)/2;

		s->frames[i]=msg;
	}
	s->frame_max=req.count;
	/*
	for (i = 0; i < s->frame_max; ++i) {
		struct v4l2_buffer buf;

		memset(&buf,0,sizeof(buf));
		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = i;
		if (-1==ioctl (s->fd, VIDIOC_QBUF, &buf)){
			ms_error("VIDIOC_QBUF failed: %s",strerror(errno));
		}
	}
	*/
	/*start capture immediately*/
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 ==ioctl (s->fd, VIDIOC_STREAMON, &type)){
		ms_error("VIDIOC_STREAMON failed: %s",strerror(errno));
		return -1;
	}
	return 0;
}

static mblk_t * v4lv2_grab_image(V4lState *s){
	struct v4l2_buffer buf;
	unsigned int k;
	memset(&buf,0,sizeof(buf));
	mblk_t *ret=NULL;

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (s->queued){
		if (ioctl(s->fd, VIDIOC_DQBUF, &buf)<0) {
			switch (errno) {
			case EAGAIN:
			case EIO:
				/* Could ignore EIO, see spec. */
				break;
			default:
				ms_warning("VIDIOC_DQBUF failed: %s",strerror(errno));
			}
		}else{
			if (buf.index >= s->frame_max){
				ms_error("buf.index>=s->max_frames !");
				return NULL;
			}
			s->queued--;
			/*decrement ref count of dequeued buffer */
			ret=s->frames[buf.index];
			ret->b_datap->db_ref--;
			if (buf.bytesused<=30){
				ms_warning("Ignoring empty buffer...");
				return NULL;
			}
		}
	}

	/*queue buffers whose ref count has dropped to 1, because they are not
	still used anywhere in the filter chain */
	for(k=0;k<s->frame_max;++k){
		if (s->frames[k]->b_datap->db_ref==1){
			buf.index=k;
			if (-1==ioctl (s->fd, VIDIOC_QBUF, &buf))
				ms_warning("VIDIOC_QBUF %i failed: %s",k,  strerror(errno));
			else {
				/*increment ref count of queued buffer*/
				s->frames[k]->b_datap->db_ref++;
				s->queued++;
			}
		}
	}
	return ret;
}

#endif

static void v4l_init(MSFilter *f){
	V4lState *s=ms_new0(V4lState,1);
	s->fd=-1;
	s->run=FALSE;
	s->v4lv2=FALSE;
	s->mmapdbuf=NULL;
	s->vsize.width=MS_VIDEO_SIZE_CIF_W;
	s->vsize.height=MS_VIDEO_SIZE_CIF_H;
	s->pix_fmt=MS_RGB24;
	s->dev=ms_strdup("/dev/video0");
	qinit(&s->rq);
	s->mire=NULL;
	ms_mutex_init(&s->mutex,NULL);
	s->start_time=0;
	s->frame_count=-1;
	s->fps=15;
	s->usemire=(getenv("DEBUG")!=NULL);
	s->queued=0;
	s->force_v1=FALSE;
	s->auto_started=FALSE;
	f->data=s;
}

/* we try not to close the /dev/videoX device to workaround a bug of linux kernel.
The bug is this one:
One thread opens /dev/videoX, and mmap()s some pages to get data from the camera.
Then several threads are created (using clone) and automatically get a reference to the mmap'd page, thus a reference to the file_struct of the opened file descriptor.
Then when the first thread closes and unmap() the file descriptor, the .release method of the driver is not called because there are still some threads owning the reference to the mmap'd pages.
As far as I understand.
If all threads are correctly pthread_join()ed, then the file descriptor is closed correctly.
But unfortunately when using alsa dmix/asym plugins, some threads are started by those plugins and are kept alive (or zombie) for some time after the mmap()/close().
*/

static int v4l_fd=-1;
static bool_t reuse_fd=FALSE;
static char *v4l_devname=NULL;

static int v4l_start(MSFilter *f, void *arg)
{
	V4lState *s=(V4lState*)f->data;
	int err=0;
	if (v4l_fd>=0 && reuse_fd){
		if (strcmp(v4l_devname,s->dev)==0 ){
			/*use this one!*/
			ms_message("v4l_start: reusing previous file descriptor.");
			s->fd=v4l_fd;
		}else{
			ms_message("closing cached fd");
			close(v4l_fd);
			v4l_fd=-1;
			ms_free(v4l_devname);
			v4l_devname=NULL;
		}
	}
	if (s->fd==-1){
		s->fd=open(s->dev,O_RDWR);
		ms_message("v4l_start: open, fd=%i",s->fd);
		if (s->fd>=0){
			v4l_fd=s->fd;
			v4l_devname=ms_strdup(s->dev);
		}
	}
	if (s->fd<0){
		ms_error("MSV4l: cannot open video device (%s): %s.",s->dev,strerror(errno));
		if (!s->usemire){
			s->pix_fmt=MS_YUV420P;
			s->fps=1;
		}
		return -1;
	}else{
#ifdef HAVE_LINUX_VIDEODEV2_H
		if (s->force_v1 || v4lv2_configure(s)<0) {/*might not be V4LV2 */
#else
		if (1){
#endif
			struct video_capability vidcap;
			err=v4l_configure(s);
			if (err<0) 
			{
				ms_error("MSV4l: could not get configuration of video device");
				close(s->fd);
				s->fd=-1;
				return -1;
			}
			if (!s->force_v1) reuse_fd=TRUE;

			err=ioctl(s->fd, VIDIOCGCAP, &vidcap);
			if (err==0)
			  {
			    ms_message("MSV4l: Webcam is %s.", vidcap.name);
			    if (strcasecmp(vidcap.name, "Logitech QuickCam USB")==0)
			      {
				int comp_arg=0;
				err=ioctl(s->fd, VIDIOCQCSCOMPATIBLE, &comp_arg);
				if (err==0)
				  {
				    ms_message("MSV4l: compatibility mode disabled for %s.", vidcap.name);
				  }
			      }
			}
		}else{
			ms_message("Device is a video4linux V2 one.");
			s->v4lv2=TRUE;
			reuse_fd=FALSE;
		}
	}
	return 0;
}

static void v4l_start_capture(V4lState *s){
	if (s->fd>=0){
		s->run=TRUE;
		ms_thread_create(&s->thread,NULL,v4l_thread,s);
	}
}

static int v4l_stop(MSFilter *f, void *arg){
	V4lState *s=(V4lState*)f->data;
	if (s->fd>=0){
		if (!reuse_fd){
			if (close(s->fd)<0){
				ms_warning("MSV4l: Could not close(): %s",strerror(errno));
			}
			ms_message("v4l fd %i closed",s->fd);
		}
		s->fd=-1;
		s->frame_count=-1;
	}
	return 0;
}

static void v4l_stop_capture(V4lState *s){
	if (s->run){
		s->run=FALSE;
		ms_thread_join(s->thread,NULL);
		ms_message("v4l thread has joined.");
		flushq(&s->rq,0);
	}
}


static void v4l_uninit(MSFilter *f){
	V4lState *s=(V4lState*)f->data;
	if (s->fd>=0) v4l_stop(f,NULL);
	ms_free(s->dev);
	flushq(&s->rq,0);
	ms_mutex_destroy(&s->mutex);
	freemsg(s->mire);
	ms_free(s);
}

static bool_t try_format(int fd, struct video_picture *pict, int palette, int depth){
	int err;
	pict->palette=palette;
	pict->depth=depth;
	err=ioctl(fd,VIDIOCSPICT,pict);
	if (err<0){
		ms_warning("Could not set picture properties: %s",strerror(errno));
		return FALSE;
	}
	return TRUE;
}

static int v4l_do_mmap(V4lState *s){
	struct video_mbuf vmbuf;
	int err,i;
	memset(&vmbuf,0,sizeof(vmbuf));
	/* try to get mmap properties */
	err=ioctl(s->fd,VIDIOCGMBUF,&vmbuf);
	if (err<0){
		ms_error("Could not get mmap properties: %s",strerror(errno));
		return -1;
	}else {
		if (vmbuf.size>0){
			/* do the mmap */
			s->msize=vmbuf.size;
			s->frame_max=vmbuf.frames;
		} else {
			ms_error("This device cannot support mmap.");
			return -1;
		}
	}
	s->mmapdbuf=mmap(NULL,s->msize,PROT_READ,MAP_SHARED,s->fd,0);
	if (s->mmapdbuf==(void*)-1) {
		/* for non-mmu arch */
		s->mmapdbuf=mmap(NULL,s->msize,PROT_READ,MAP_PRIVATE,s->fd,0);
		if (s->mmapdbuf==(void*)-1) {
			ms_error("Could not mmap: %s",strerror(errno));
			s->mmapdbuf=NULL;
			return -1;
		}
	}
	/* initialize the mediastreamer buffers */
	ms_message("Using %i-frames mmap'd buffer at %p, len %i",
		s->frame_max, s->mmapdbuf,s->msize);
	for(i=0;i<s->frame_max;i++){
		mblk_t *buf=esballoc((uint8_t*)s->mmapdbuf+vmbuf.offsets[i],vmbuf.offsets[1],0,NULL);
		/* adjust to real size of picture*/
		if (s->pix_fmt==MS_RGB24)
			buf->b_wptr+=s->vsize.width*s->vsize.height*3;
		else 
			buf->b_wptr+=(s->vsize.width*s->vsize.height*3)/2;
		s->frames[i]=buf;
	}
	s->frame_ind=0;
	return 0;
}

static bool_t try_size(V4lState *s, MSVideoSize vsize){
	struct video_window win;
	int err;
	memset(&win,0,sizeof(win));
	/*set picture size */
	win.x=win.y=0;
	win.width=vsize.width;
	win.height=vsize.height;
	win.flags=0;
	win.clips=NULL;
	win.clipcount=0;

	ms_message("Trying to set capture size to %ix%i", vsize.width,vsize.height);
	err=ioctl(s->fd,VIDIOCSWIN,&win);
	if (err<0){
		ms_warning("Could not set window size: %s",strerror(errno));
		return FALSE;
	}

	err=ioctl(s->fd, VIDIOCGWIN, &win);
	if (err<0){
		ms_warning("Could not get window size: %s",strerror(errno));
		return FALSE;
	}
	s->vsize.width=vsize.width;
	s->vsize.height=vsize.height;

	if (s->vsize.width!=win.width || s->vsize.height!=win.height){
		ms_warning("Capture size is not what we expected: asked for %ix%i and get %ix%i",s->vsize.width,s->vsize.height, win.width, win.height);
	}
	s->got_vsize.width=win.width;
	s->got_vsize.height=win.height;
	ms_message("Capture size set to %ix%i", s->got_vsize.width,s->got_vsize.height);
	return TRUE;
}

static int v4l_configure(V4lState *s)
{
	struct video_channel chan;
	struct video_picture pict;
	struct video_capability cap;
	int err;
	int i;
	int fps = 0;
	int found=0;
	
	memset(&chan,0,sizeof(chan));
	memset(&pict,0,sizeof(pict));
	memset(&cap,0,sizeof(cap));

	err=ioctl(s->fd,VIDIOCGCAP,&cap);
	if (err!=0)
	{
		ms_warning("MSV4l: cannot get device capabilities: %s.",strerror(errno));
		return -1;
	}

	ms_message("Found %s device. (maxsize=%ix%i)",cap.name, cap.maxwidth, cap.maxheight);
	for (i=0;i<cap.channels;i++)
	{
		chan.channel=i;
		err=ioctl(s->fd,VIDIOCGCHAN,&chan);
		if (err==0)
		{
			ms_message("Getting video channel %s",chan.name);
			switch(chan.type){
				case VIDEO_TYPE_TV:
					ms_message("Channel is a TV.");
				break;
				case VIDEO_TYPE_CAMERA:
					ms_message("Channel is a camera");
				break;
				default:
					ms_warning("unknown video channel type.");
			}
			found=1;
			break;  /* find the first channel */
		}
	}
	if (found) ms_message("A valid video channel was found.");
	/* select this channel */
	ioctl(s->fd,VIDIOCSCHAN,&chan);
	
	/* get picture properties */
	err=ioctl(s->fd,VIDIOCGPICT,&pict);
	if (err<0){
		ms_warning("Could not get picture properties: %s",strerror(errno));
		return -1;
	}
	ms_message("Default picture properties: brightness=%i,hue=%i,colour=%i,contrast=%i,depth=%i, palette=%i.",
						pict.brightness,pict.hue,pict.colour, pict.contrast,pict.depth, pict.palette);
	
	/* trying color format */
	if (try_format(s->fd,&pict,VIDEO_PALETTE_YUV420P,16)){
		ms_message("Driver supports YUV420P, using that format.");
		s->pix_fmt=MS_YUV420P;
	}else if (try_format(s->fd, &pict,VIDEO_PALETTE_RGB24,24)){
		ms_message("Driver supports RGB24, using that format.");
		s->pix_fmt=MS_RGB24;
	}else if (try_format(s->fd, &pict,VIDEO_PALETTE_YUV422, 16)){
		ms_message("Driver supports YUV422, using that format.");
		s->pix_fmt=MS_YUYV;
	}else if (try_format(s->fd, &pict,VIDEO_PALETTE_UYVY, 16)){
		ms_message("Driver supports UYVY, using that format.");
		s->pix_fmt=MS_UYVY;
	}else{	
	  s->vsize.width=MS_VIDEO_SIZE_CIF_W;
	  s->vsize.height=MS_VIDEO_SIZE_CIF_H;
		s->pix_fmt=MS_YUV420P;
		ms_fatal("Unsupported video pixel format.");
		return -1;
	}

	if (!try_size(s,s->vsize)) {
		if (!try_size(s,MS_VIDEO_SIZE_NS1)){
			if (!try_size(s,MS_VIDEO_SIZE_VGA)){
				if (!try_size(s,MS_VIDEO_SIZE_CIF)) {
					if (!try_size(s,MS_VIDEO_SIZE_QCIF)) {
						if (!try_size(s,MS_VIDEO_SIZE_QVGA)) {
						  if (!try_size(s,MS_VIDEO_SIZE_4CIF)) {
						    if (!try_size(s,MS_VIDEO_SIZE_1024)) {
						      return -1;
						    }
						  }
						}
					}
				}
			}
		}
	}

	/* Try HW frame rate control */
	fps = s->fps;
	if (ioctl(s->fd, VIDIOSFPS, &fps) < 0 )
		ms_message("v4l_configure: cannot set HW frame rate control");
	else
		ms_message("v4l_configure: set HW fps to be : %d", fps);
	
	return 0;
}	


int ms_to_v4l_pix_fmt(MSPixFmt p){
	switch(p){
		case MS_YUV420P:
				return VIDEO_PALETTE_YUV420P;
		case MS_RGB24:
				return VIDEO_PALETTE_RGB24;
		case MS_YUYV:
				return VIDEO_PALETTE_YUV422;
		case MS_UYVY:
				return VIDEO_PALETTE_UYVY;
		default:
			ms_fatal("unsupported pix fmt");
			return -1;
	}
}

static void plane_copy(uint8_t *dest, int dw, int dh, uint8_t *src, int sw, int sh, int bpp){
	int diffw=dw-sw;
	int diffh=dh-sh;
	int dlsize=dw*bpp;
	int slsize=sw*bpp;
	int i;
	int dstartx=(diffw>0) ? diffw/2 : 0;
	int dstarty=(diffh>0) ? diffh/2 : 0;
	int sstartx=(diffw<0) ? diffw/2 : 0;
	int sstarty=(diffh<0) ? diffh/2 : 0;
	uint8_t *tmp1=dest;
	uint8_t *tmp2=src;	

	/* copy orig into dest */
	tmp2+=sstarty*slsize;
	tmp1+=dstarty*dlsize;
	for(i=dstarty;i<dh-dstarty;++i){
		memcpy(tmp1+(dstartx*bpp),tmp2+(sstartx*bpp),(sw-(2*sstartx))*bpp);
		tmp1+=dlsize;
		tmp2+=slsize;
	}
}

static void pic_copy(uint8_t *dest, int dw, int dh, uint8_t *src, int sw, int sh, MSPixFmt fmt){
	if (fmt==MS_YUV420P){
		plane_copy(dest,dw,dh,src,sw,sh,1);
		dest+=dw*dh;
		src+=sw*sh;
		plane_copy(dest,dw/2,dh/2,src,sw/2,sh/2,1);
		dest+=dw*dh/4;
		src+=sw*sh/4;
		plane_copy(dest,dw/2,dh/2,src,sw/2,sh/2,1);
	}else if (fmt==MS_RGB24){
		plane_copy(dest,dw,dh,src,sw,sh,3);
	}else if (fmt==MS_YUYV||fmt==MS_UYVY){
		plane_copy(dest,dw,dh,src,sw,sh,4);
	}
}

static mblk_t *crop_or_pad(V4lState *s, mblk_t *pic){
	int size=s->vsize.width*s->vsize.height;
	mblk_t *newpic;
	if (s->pix_fmt==MS_YUV420P) size=size*3/2;
	else if (s->pix_fmt==MS_YUYV) size=size*2;
	else if (s->pix_fmt==MS_UYVY) size=size*2;
	else if (s->pix_fmt==MS_RGB24) size=size*3;
	else ms_fatal("crop_or_pad: unsupported pixel format.");
	newpic=allocb(size,0);
	memset(newpic->b_wptr,0,size);
	pic_copy(newpic->b_wptr, s->vsize.width, s->vsize.height,
		pic->b_rptr,s->got_vsize.width,s->got_vsize.height,s->pix_fmt);
	newpic->b_wptr+=size;
	return newpic;
}

static mblk_t * v4l_grab_image_mmap(V4lState *s){
	struct video_mmap vmap;
	int err;
	int syncframe;
	int jitter=s->frame_max-1;
	int query_frame;
	mblk_t *ret;
	vmap.width=s->got_vsize.width;
	vmap.height=s->got_vsize.height;
	vmap.format=ms_to_v4l_pix_fmt(s->pix_fmt);
	
	query_frame=(s->frame_ind) % s->frame_max;
	/*ms_message("v4l_mmap_process: query_frame=%i",
			obj->query_frame);*/
	vmap.frame=query_frame;
	err=ioctl(s->fd,VIDIOCMCAPTURE,&vmap);
	if (err<0) {
		ms_warning("v4l_grab_image_mmap: error in VIDIOCMCAPTURE: %s.",strerror(errno));
		usleep(10000);
		return NULL;
	}
	/*g_message("v4l_mmap_process: query_frame=%i done",
			obj->query_frame);*/
	syncframe=(s->frame_ind-jitter);
	s->frame_ind++;
	if (syncframe>=0){
		syncframe=syncframe%s->frame_max;
		/*ms_message("Syncing on frame %i",syncframe);*/
		err=ioctl(s->fd,VIDIOCSYNC,&syncframe);
		if (err<0) {
			ms_warning("v4l_grab_image_mmap: error in VIDIOCSYNC: %s.",strerror(errno));	
			return NULL;
		}
		/*g_message("got frame %i",syncframe);*/
	}else {
		return NULL;
	}
	ret=s->frames[syncframe];
	/* crop or pad picture if obtained size is not what we want */
	if (s->vsize.width!=s->got_vsize.width){
		ret=crop_or_pad(s,ret);
	}
	return ret;
}

static mblk_t * v4l_make_mire(V4lState *s){
	unsigned char *data;
	int i,j,line,pos;
	int patternw=s->vsize.width/6; 
	int patternh=s->vsize.height/6;
	int red,green=0,blue=0;
	if (s->mire==NULL){
		s->mire=allocb(s->vsize.width*s->vsize.height*3,0);
		s->mire->b_wptr=s->mire->b_datap->db_lim;
	}
	data=s->mire->b_rptr;
	for (i=0;i<s->vsize.height;++i){
		line=i*s->vsize.width*3;
		if ( ((i+s->frame_ind)/patternh) & 0x1) red=255;
		else red= 0;
		for (j=0;j<s->vsize.width;++j){
			pos=line+(j*3);
			
			if ( ((j+s->frame_ind)/patternw) & 0x1) blue=255;
			else blue= 0;
			
			data[pos]=red;
			data[pos+1]=green;
			data[pos+2]=blue;
		}
	}
	s->frame_ind++;
	return s->mire;
}

static mblk_t * v4l_make_nowebcam(V4lState *s){
	if (s->mire==NULL && s->frame_ind==0){
		s->mire=ms_load_nowebcam(&s->vsize, -1);
	}
	s->frame_ind++;
	return s->mire;
}

static void v4l_purge(V4lState *s){
	int i;
	int err;
	int jitter=s->frame_max-1;
	for (i=s->frame_ind-jitter;i<s->frame_ind;++i){
		int syncframe=i%s->frame_max;
		ms_message("syncing last frame");
		err=ioctl(s->fd,VIDIOCSYNC,&syncframe);
		if (err<0) {
			ms_warning("v4l_mmap_process: error in VIDIOCSYNC: %s.",strerror(errno));	
		}
	}
}

#ifdef HAVE_LINUX_VIDEODEV2_H
static void v4lv2_purge(V4lState *s){
	struct v4l2_buffer buf;
	memset(&buf,0,sizeof(buf));

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	
	for(;s->queued>0;s->queued--){
		if (ioctl(s->fd, VIDIOC_DQBUF, &buf)==-1){
			ms_warning("v4lv2_purge: Could not DQ buffer: %s",strerror(errno));
		}
	}
}
#endif

static void v4l_do_munmap(V4lState *s){
	int i;
#ifdef HAVE_LINUX_VIDEODEV2_H
	enum v4l2_buf_type type;
	if (s->v4lv2){
		v4lv2_purge(s);
		/*stop capture immediately*/
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 ==ioctl (s->fd, VIDIOC_STREAMOFF, &type)){
			ms_error("VIDIOC_STREAMOFF failed: %s",strerror(errno));
		}
	}
#endif
	if (!s->v4lv2){
		v4l_purge(s);
	}
	if (s->mmapdbuf!=NULL){
		if (munmap(s->mmapdbuf,s->msize)<0){
			ms_warning("MSV4l: Fail to unmap: %s",strerror(errno));
		}
		ms_message("munmap() done (%p,%i)",s->mmapdbuf,s->msize);
		s->mmapdbuf=NULL;
	}
	s->msize=0;
	for(i=0;i<s->frame_max;++i){
		if (s->v4lv2){
			mblk_t *msg=s->frames[i];
			int len=msg->b_datap->db_lim-msg->b_datap->db_base;
			if (munmap(msg->b_datap->db_base,len)<0){
				ms_warning("MSV4l: Fail to unmap: %s",strerror(errno));
			}
		}
		freemsg(s->frames[i]);
		s->frames[i]=NULL;
	}
}


static void *v4l_thread(void *ptr){
	V4lState *s=(V4lState*)ptr;
	int err=-1;
	ms_message("v4l_thread starting");
	if (s->v4lv2){
#ifdef HAVE_LINUX_VIDEODEV2_H
		err=v4lv2_do_mmap(s);
#endif
	}else{
		err=v4l_do_mmap(s);
	}
	if (err<0){
		ms_thread_exit(NULL);
	}
	while(s->run){
		mblk_t *m;
#ifdef HAVE_LINUX_VIDEODEV2_H
		if (s->v4lv2)
			m=v4lv2_grab_image(s);
		else
#endif
			m=v4l_grab_image_mmap(s);

		if (s->vsize.width!=s->got_vsize.width){
			if (m){
				/* mblock was allocated by crop or pad! */ 
				ms_mutex_lock(&s->mutex);
				putq(&s->rq,m);
				ms_mutex_unlock(&s->mutex);
			}else{
				ms_error("grabbing failed !");
			}
		} else if (m!=NULL) {
			mblk_t *dm=dupmsg(m);
			ms_mutex_lock(&s->mutex);
			putq(&s->rq,dm);
			ms_mutex_unlock(&s->mutex);
		}
	}
	v4l_do_munmap(s);
	ms_message("v4l_thread exited.");
	ms_thread_exit(NULL);
}


static void v4l_process(MSFilter * obj){
	V4lState *s=(V4lState*)obj->data;
	uint32_t timestamp;
	int cur_frame;
	if (s->frame_count==-1){
		s->start_time=obj->ticker->time;
		s->frame_count=0;
	}
	cur_frame=((obj->ticker->time-s->start_time)*s->fps/1000.0);
	if (cur_frame>=s->frame_count){
		mblk_t *om=NULL;
		ms_mutex_lock(&s->mutex);
		/*keep the most recent frame if several frames have been captured */
		if (s->fd!=-1){
			om=getq(&s->rq);
		}else{
			if (s->usemire){
				om=dupmsg(v4l_make_mire(s));
			}else {
				mblk_t *tmpm=v4l_make_nowebcam(s);
				if (tmpm) om=dupmsg(tmpm);
			}
		}
		ms_mutex_unlock(&s->mutex);
		if (om!=NULL){
			timestamp=obj->ticker->time*90;/* rtp uses a 90000 Hz clockrate for video*/
			mblk_set_timestamp_info(om,timestamp);
			mblk_set_marker_info(om,TRUE);
			ms_queue_put(obj->outputs[0],om);
			/*ms_message("picture sent");*/
			s->frame_count++;
		}
	}else flushq(&s->rq,0);
}

static void v4l_preprocess(MSFilter *f){
	V4lState *s=(V4lState*)f->data;
	if (s->fd==-1){
		s->auto_started=TRUE;
		v4l_start(f,NULL);
		v4l_start_capture(s);
	}
	else
	{
		v4l_start_capture(s);
	}
}

static void v4l_postprocess(MSFilter *f){
	V4lState *s=(V4lState*)f->data;
	if (s->auto_started){
		v4l_stop_capture(s);
		v4l_stop(f,NULL);
	}
	else
	{
		v4l_stop_capture(s);
	}
}

static int v4l_set_fps(MSFilter *f, void *arg){
	V4lState *s=(V4lState*)f->data;
	s->fps=*((float*)arg);
	s->frame_count=-1;
	return 0;
}

static int v4l_get_pix_fmt(MSFilter *f,void *arg){
	V4lState *s=(V4lState*)f->data;
	MSPixFmt res;
	if (s->fd==-1){
		v4l_start(f,NULL);
		res=s->pix_fmt;
		v4l_stop(f,NULL);
	}else res=s->pix_fmt;
	*((MSPixFmt*)arg)=res;
	return 0;
}

static int v4l_set_vsize(MSFilter *f, void *arg){
	V4lState *s=(V4lState*)f->data;
	s->vsize=*((MSVideoSize*)arg);
	return 0;
}

static int v4l_get_vsize(MSFilter *f, void *arg){
	V4lState *s=(V4lState*)f->data;
	*(MSVideoSize*)arg=s->vsize;
	return 0;
}

static MSFilterMethod methods[]={
	{	MS_FILTER_SET_FPS	,	v4l_set_fps	},
	{	MS_FILTER_GET_PIX_FMT	,	v4l_get_pix_fmt	},
	{	MS_FILTER_SET_VIDEO_SIZE, 	v4l_set_vsize	},
	{	MS_V4L_START			,	v4l_start	},
	{	MS_V4L_STOP			,	v4l_stop	},
	{	MS_FILTER_GET_VIDEO_SIZE,	v4l_get_vsize },
	{	0	,	NULL			}
};

static int v4l_set_devfile(MSFilter *f, void *arg){
	V4lState *s=(V4lState*)f->data;
	if (s->dev) ms_free(s->dev);
	s->dev=ms_strdup((char*)arg);
	return 0;
}

MSFilterDesc ms_v4l_desc={
	.id=MS_V4L_ID,
	.name="MSV4l",
	.text="A video4linux compatible source filter to stream pictures.",
	.ninputs=0,
	.noutputs=1,
	.category=MS_FILTER_OTHER,
	.init=v4l_init,
	.preprocess=v4l_preprocess,
	.process=v4l_process,
	.postprocess=v4l_postprocess,
	.uninit=v4l_uninit,
	.methods=methods
};

MS_FILTER_DESC_EXPORT(ms_v4l_desc)

static MSFilter *v4l_create_reader(MSWebCam *obj){
	MSFilter *f=ms_filter_new_from_desc(&ms_v4l_desc);
	V4lState *s=(V4lState*)f->data;
	v4l_set_devfile(f,obj->name);
	s->force_v1=TRUE;
	return f;
}

static void v4l_detect(MSWebCamManager *obj);

static void v4l_cam_init(MSWebCam *cam){
	
}

MSWebCamDesc v4l_desc={
	"V4L",
	&v4l_detect,
	&v4l_cam_init,
	&v4l_create_reader,
	NULL
};

static void v4l_detect(MSWebCamManager *obj){
	struct video_capability cap;
	const char *devname="/dev/video0";
	int fd=open(devname,O_RDWR);
	if (fd!=-1){
		if (ioctl (fd, VIDIOCGCAP, &cap)==0) {
			/* is a V4Lv1 */
			MSWebCam *cam=ms_web_cam_new(&v4l_desc);
			cam->name=ms_strdup(devname);
			ms_web_cam_manager_add_cam(obj,cam);
		}
		close(fd);
	}
	devname="/dev/video1";
	fd=open(devname,O_RDWR);
	if (fd!=-1){
		if (ioctl (fd, VIDIOCGCAP, &cap)==0) {
			/* is a V4Lv1 */
			MSWebCam *cam=ms_web_cam_new(&v4l_desc);
			cam->name=ms_strdup(devname);
			ms_web_cam_manager_add_cam(obj,cam);
		}
		close(fd);
	}
}


#endif
