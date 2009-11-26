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

#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

#ifdef HAVE_LINUX_VIDEODEV2_H


#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <poll.h>

#include <linux/videodev2.h>

#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/mswebcam.h"

#ifdef HAVE_LIBV4L2
#include <libv4l2.h>
#else

#define v4l2_open open
#define v4l2_close close
#define v4l2_mmap mmap
#define v4l2_munmap munmap
#define v4l2_ioctl ioctl

#endif

typedef struct V4l2State{
	int fd;
	char *dev;
	char *mmapdbuf;
	int msize;/*mmapped size*/
	MSVideoSize vsize;
	MSVideoSize got_vsize;
	int pix_fmt;
	int int_pix_fmt; /*internal pixel format */
	int picture_size;
	mblk_t *frames[VIDEO_MAX_FRAME];
	int frame_ind;
	int frame_max;
	float fps;
	float start_time;
	int frame_count;
	int queued;
	bool_t configured;
}V4l2State;

static int msv4l2_open(V4l2State *s){
	int fd=v4l2_open(s->dev,O_RDWR|O_NONBLOCK);
	if (fd==-1){
		ms_error("Could not open %s: %s",s->dev,strerror(errno));
		return -1;
	}
	s->fd=fd;
	return 0;
}

static int msv4l2_close(V4l2State *s){
	if (s->fd!=-1){
		v4l2_close(s->fd);
		s->fd=-1;
		s->configured=FALSE;
	}
	return 0;
}

static bool_t v4lv2_try_format( V4l2State *s, struct v4l2_format *fmt, int fmtid){
	
	fmt->type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt->fmt.pix.pixelformat = fmtid;
	fmt->fmt.pix.field = V4L2_FIELD_ANY;

        if (v4l2_ioctl (s->fd, VIDIOC_TRY_FMT, fmt)<0){
		ms_message("VIDIOC_TRY_FMT: %s",strerror(errno));
		return FALSE;
	}
	if (v4l2_ioctl (s->fd, VIDIOC_S_FMT, fmt)<0){
		ms_message("VIDIOC_S_FMT: %s",strerror(errno));
		return FALSE;
	}
	return TRUE;
}

static int get_picture_buffer_size(MSPixFmt pix_fmt, int w, int h){
	switch(pix_fmt){
		case MS_YUV420P:
			return (w*h*3)/2;
		break;
		case MS_RGB24:
			return w*h*3;
		break;
		case MS_YUYV:
			return w*h*2;
		break;
		default:
			return 0;
	}
	return 0;
}

static int msv4l2_configure(V4l2State *s){
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	MSVideoSize vsize;

        if (v4l2_ioctl (s->fd, VIDIOC_QUERYCAP, &cap)<0) {
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
	
	ms_message("Driver is %s",cap.driver);
	memset(&fmt,0,sizeof(fmt));

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (v4l2_ioctl (s->fd, VIDIOC_G_FMT, &fmt)<0){
		ms_error("VIDIOC_G_FMT failed: %s",strerror(errno));
	}
	vsize=s->vsize;
	do{
		fmt.fmt.pix.width       = s->vsize.width; 
		fmt.fmt.pix.height      = s->vsize.height;
		if (v4lv2_try_format(s,&fmt,V4L2_PIX_FMT_YUV420)){
			s->pix_fmt=MS_YUV420P;
			s->int_pix_fmt=V4L2_PIX_FMT_YUV420;
			ms_message("v4lv2: YUV420P choosen");
			break;
		}else if (v4lv2_try_format(s,&fmt,V4L2_PIX_FMT_MJPEG)){
			s->pix_fmt=MS_MJPEG;
			s->int_pix_fmt=V4L2_PIX_FMT_MJPEG;
			ms_message("v4lv2: MJPEG choosen");
			break;
		}else if (v4lv2_try_format(s,&fmt,V4L2_PIX_FMT_YUYV)){
			s->pix_fmt=MS_YUYV;
			s->int_pix_fmt=V4L2_PIX_FMT_YUYV;
			ms_message("v4lv2: V4L2_PIX_FMT_YUYV choosen");
			break;
		}else if (v4lv2_try_format(s,&fmt,V4L2_PIX_FMT_RGB24)){
			s->pix_fmt=MS_RGB24;
			s->int_pix_fmt=V4L2_PIX_FMT_RGB24;
			ms_message("v4lv2: RGB24 choosen");
			break;
		}else{
			ms_error("Could not find supported pixel format for %ix%i", s->vsize.width, s->vsize.height);
		}
		s->vsize=ms_video_size_get_just_lower_than(s->vsize);
	}while(s->vsize.width!=0);
	if (s->vsize.width==0){
		ms_message("Could not find any combination of resolution/pixel-format that works !");
		s->vsize=vsize;
		return -1;
	}
	memset(&fmt,0,sizeof(fmt));

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (v4l2_ioctl (s->fd, VIDIOC_G_FMT, &fmt)<0){
		ms_error("VIDIOC_G_FMT failed: %s",strerror(errno));
	}else{
		ms_message("Size of webcam delivered pictures is %ix%i",fmt.fmt.pix.width,fmt.fmt.pix.height);
		s->vsize.width=fmt.fmt.pix.width;
		s->vsize.height=fmt.fmt.pix.height;
	}
	s->picture_size=get_picture_buffer_size(s->pix_fmt,s->vsize.width,s->vsize.height);
	s->configured=TRUE;
	return 0;
}

static int msv4l2_do_mmap(V4l2State *s){
	struct v4l2_requestbuffers req;
	int i;
	enum v4l2_buf_type type;
	
	memset(&req,0,sizeof(req));
	
	req.count               = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;
	
	if (v4l2_ioctl (s->fd, VIDIOC_REQBUFS, &req)<0) {
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
	
		if (v4l2_ioctl (s->fd, VIDIOC_QUERYBUF, &buf)<0){
			ms_error("Could not VIDIOC_QUERYBUF : %s",strerror(errno));
			return -1;
		}
		
		start=v4l2_mmap (NULL /* start anywhere */,
			buf.length,
			PROT_READ | PROT_WRITE /* required */,
			MAP_SHARED /* recommended */,
			s->fd, buf.m.offset);
	
		if (start==NULL){
			ms_error("Could not v4l2_mmap: %s",strerror(errno));
		}
		msg=esballoc(start,buf.length,0,NULL);
		msg->b_wptr+=buf.length;
		s->frames[i]=msg;
	}
	s->frame_max=req.count;
	for (i = 0; i < s->frame_max; ++i) {
		struct v4l2_buffer buf;

		memset(&buf,0,sizeof(buf));
		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = i;
		if (-1==v4l2_ioctl (s->fd, VIDIOC_QBUF, &buf)){
			ms_error("VIDIOC_QBUF failed: %s",strerror(errno));
		}else {
			s->frames[i]->b_datap->db_ref++;
			s->queued++;
		}
	}
	/*start capture immediately*/
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 ==v4l2_ioctl (s->fd, VIDIOC_STREAMON, &type)){
		ms_error("VIDIOC_STREAMON failed: %s",strerror(errno));
		return -1;
	}
	return 0;
}

static mblk_t * v4lv2_grab_image(V4l2State *s){
	struct v4l2_buffer buf;
	unsigned int k;
	memset(&buf,0,sizeof(buf));
	mblk_t *ret=NULL;

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	
	/*queue buffers whose ref count is 1, because they are not
	still used anywhere in the filter chain */
	for(k=0;k<s->frame_max;++k){
		if (s->frames[k]->b_datap->db_ref==1){
			buf.index=k;
			if (-1==v4l2_ioctl (s->fd, VIDIOC_QBUF, &buf))
				ms_warning("VIDIOC_QBUF %i failed: %s",k,  strerror(errno));
			else {
				ms_debug("v4l2: queue buf %i",k);
				/*increment ref count of queued buffer*/
				s->frames[k]->b_datap->db_ref++;
				s->queued++;
			}
		}
	}

	if (s->queued){
		struct pollfd fds;
		memset(&fds,0,sizeof(fds));
		fds.events=POLLIN;
		fds.fd=s->fd;
		/*check with poll if there is something to read */
		if (poll(&fds,1,0)==1 && fds.revents==POLLIN){
			if (v4l2_ioctl(s->fd, VIDIOC_DQBUF, &buf)<0) {
				switch (errno) {
				case EAGAIN:
				case EIO:
					/* Could ignore EIO, see spec. */
					break;
				default:
					ms_warning("VIDIOC_DQBUF failed: %s",strerror(errno));
				}
			}else{
				s->queued--;
				ms_debug("v4l2: de-queue buf %i",buf.index);
				/*decrement ref count of dequeued buffer */
				ret=s->frames[buf.index];
				ret->b_datap->db_ref--;
				if (buf.index >= s->frame_max){
					ms_error("buf.index>=s->max_frames !");
					return NULL;
				}
				if (buf.bytesused<=30){
					ms_warning("Ignoring empty buffer...");
					return NULL;
				}
				/*normally buf.bytesused should contain the right buffer size; however we have found a buggy
				driver that puts a random value inside */
				if (s->picture_size!=0)
					ret->b_wptr=ret->b_rptr+s->picture_size;
				else ret->b_wptr=ret->b_rptr+buf.bytesused;
			}
		}
	}
	return ret;
}

static void msv4l2_do_munmap(V4l2State *s){
	int i;
	enum v4l2_buf_type type;
	/*stop capture immediately*/
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 ==v4l2_ioctl (s->fd, VIDIOC_STREAMOFF, &type)){
		ms_error("VIDIOC_STREAMOFF failed: %s",strerror(errno));
	}

	for(i=0;i<s->frame_max;++i){
		mblk_t *msg=s->frames[i];
		int len=msg->b_datap->db_lim-msg->b_datap->db_base;
		if (v4l2_munmap(msg->b_datap->db_base,len)<0){
			ms_warning("MSV4l2: Fail to unmap: %s",strerror(errno));
		}
		freemsg(s->frames[i]);
		s->frames[i]=NULL;
	}
}



static void msv4l2_init(MSFilter *f){
	V4l2State *s=ms_new0(V4l2State,1);
	s->dev=ms_strdup("/dev/video0");
	s->fd=-1;
	s->vsize=MS_VIDEO_SIZE_CIF;
	s->fps=15;
	s->configured=FALSE;
	f->data=s;
}

static void msv4l2_uninit(MSFilter *f){
	V4l2State *s=(V4l2State*)f->data;
	ms_free(s->dev);
	ms_free(s);
}

static void msv4l2_preprocess(MSFilter *f){
	V4l2State *s=(V4l2State*)f->data;
	if (s->fd==-1 && msv4l2_open(s)!=0) {
		return;
	}
	if (!s->configured && msv4l2_configure(s)!=0){
		return;
	}
	if (msv4l2_do_mmap(s)==0){
		ms_message("V4L2 video capture started.");
	}else{
		msv4l2_close(s);
	}
	s->start_time=f->ticker->time;
}

static void msv4l2_process(MSFilter *f){
	V4l2State *s=(V4l2State*)f->data;
	uint32_t elapsed;
	
	if (s->fd!=-1){
		/*see it is necessary to output a frame:*/
		elapsed=f->ticker->time-s->start_time;
		if (((float)elapsed*s->fps/1000.0)>s->frame_count){
			mblk_t *m;
			m=v4lv2_grab_image(s);
			if (m){
				mblk_t *om=dupb(m);
				mblk_set_marker_info(om,(s->pix_fmt==MS_MJPEG));
				ms_queue_put(f->outputs[0],om);
				s->frame_count++;
			}
		}
	}
}

static void msv4l2_postprocess(MSFilter *f){
	V4l2State *s=(V4l2State*)f->data;
	if (s->fd!=-1){
		msv4l2_do_munmap(s);
		msv4l2_close(s);
	}
}

static int msv4l2_set_fps(MSFilter *f, void *arg){
	V4l2State *s=(V4l2State*)f->data;
	s->fps=*(float*)arg;
	return 0;
}

static int msv4l2_set_vsize(MSFilter *f, void *arg){
	V4l2State *s=(V4l2State*)f->data;
	s->vsize=*(MSVideoSize*)arg;
	return 0;
}

static int msv4l2_get_vsize(MSFilter *f, void *arg){
	V4l2State *s=(V4l2State*)f->data;
	*(MSVideoSize*)arg=s->vsize;
	return 0;
}

static int msv4l2_get_pixfmt(MSFilter *f, void *arg){
	V4l2State *s=(V4l2State*)f->data;
	if (s->fd==-1){
		if (msv4l2_open(s)==0){
			msv4l2_configure(s);
			*(MSPixFmt*)arg=s->pix_fmt;
			return 0;
		}else return -1;
	}
	*(MSPixFmt*)arg=s->pix_fmt;
	return 0;
}

static int msv4l2_set_devfile(MSFilter *f, void *arg){
	V4l2State *s=(V4l2State*)f->data;
	if (s->dev) ms_free(s->dev);
	s->dev=ms_strdup((char*)arg);
	return 0;
}

static MSFilterMethod msv4l2_methods[]={
	{	MS_FILTER_SET_FPS	,	msv4l2_set_fps	},
	{	MS_FILTER_SET_VIDEO_SIZE,	msv4l2_set_vsize	},
	{	MS_FILTER_GET_VIDEO_SIZE,	msv4l2_get_vsize	},
	{	MS_FILTER_GET_PIX_FMT	,	msv4l2_get_pixfmt	},
	{	0			,	NULL		}
};

MSFilterDesc ms_v4l2_desc={
	.id=MS_V4L2_CAPTURE_ID,
	.name="MSV4L2Capture",
	.text=N_("A filter to grab pictures from Video4Linux2-powered cameras"),
	.category=MS_FILTER_OTHER,
	.ninputs=0,
	.noutputs=1,
	.init=msv4l2_init,
	.preprocess=msv4l2_preprocess,
	.process=msv4l2_process,
	.postprocess=msv4l2_postprocess,
	.uninit=msv4l2_uninit,
	.methods=msv4l2_methods
};

MS_FILTER_DESC_EXPORT(ms_v4l2_desc)

static MSFilter *msv4l2_create_reader(MSWebCam *obj){
	MSFilter *f=ms_filter_new(MS_V4L2_CAPTURE_ID);
	msv4l2_set_devfile(f,obj->name);
	return f;
}

static void msv4l2_detect(MSWebCamManager *obj);

static void msv4l2_cam_init(MSWebCam *cam){
}

MSWebCamDesc v4l2_card_desc={
	"V4L2",
	&msv4l2_detect,
	&msv4l2_cam_init,
	&msv4l2_create_reader,
	NULL
};

static void msv4l2_detect(MSWebCamManager *obj){
	struct v4l2_capability cap;
	char devname[32];
	int i;
	for(i=0;i<10;++i){
		int fd;
		snprintf(devname,sizeof(devname),"/dev/video%i",i);
		fd=open(devname,O_RDWR);
		if (fd!=-1){
			if (v4l2_ioctl (fd, VIDIOC_QUERYCAP, &cap)==0) {
				/* is a V4LV2 */
				MSWebCam *cam=ms_web_cam_new(&v4l2_card_desc);
				cam->name=ms_strdup(devname);
				ms_web_cam_manager_add_cam(obj,cam);
			}
			close(fd);
		}
	}
}


#endif
