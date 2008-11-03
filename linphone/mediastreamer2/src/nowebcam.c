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

#include "mediastreamer2/mscommon.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/mswebcam.h"

#include "ffmpeg-priv.h"

#include <sys/stat.h>

#ifdef WIN32
#include <fcntl.h>
#include <sys/types.h>
#include <io.h>
#include <stdio.h>
#include <malloc.h>
#endif

static mblk_t *jpeg2yuv(uint8_t *jpgbuf, int bufsize, MSVideoSize *reqsize){
	AVCodecContext av_context;
	int got_picture=0;
	AVFrame orig;
	AVPicture dest;
	mblk_t *ret;
	struct SwsContext *sws_ctx;

	avcodec_get_context_defaults(&av_context);
	if (avcodec_open(&av_context,avcodec_find_decoder(CODEC_ID_MJPEG))<0){
		ms_error("jpeg2yuv: avcodec_open failed");
		return NULL;
	}
	if (avcodec_decode_video(&av_context,&orig,&got_picture,jpgbuf,bufsize)<0){
		ms_error("jpeg2yuv: avcodec_decode_video failed");
		avcodec_close(&av_context);
		return NULL;
	}
	ret=allocb(avpicture_get_size(PIX_FMT_YUV420P,reqsize->width,reqsize->height),0);
	ret->b_wptr=ret->b_datap->db_lim;
	avpicture_fill(&dest,ret->b_rptr,PIX_FMT_YUV420P,reqsize->width,reqsize->height);
	
	sws_ctx=sws_getContext(av_context.width,av_context.height,PIX_FMT_YUV420P,
		reqsize->width,reqsize->height,PIX_FMT_YUV420P,SWS_FAST_BILINEAR,
                NULL, NULL, NULL);
	if (sws_scale(sws_ctx,orig.data,orig.linesize,0,av_context.height,dest.data,dest.linesize)<0){
		ms_error("jpeg2yuv: sws_scale() failed.");
	}
	sws_freeContext(sws_ctx);
	avcodec_close(&av_context);
	return ret;
}

mblk_t *ms_load_jpeg_as_yuv(const char *jpgpath, MSVideoSize *reqsize){
	mblk_t *m=NULL;
	struct stat statbuf;
	uint8_t *jpgbuf;
#if !defined(_MSC_VER)
	int fd=open(jpgpath,O_RDONLY);
#else
	int fd=_open(jpgpath,O_RDONLY);
#endif
	if (fd!=-1){
		fstat(fd,&statbuf);
		jpgbuf=(uint8_t*)alloca(statbuf.st_size);
#if !defined(_MSC_VER)
		read(fd,jpgbuf,statbuf.st_size);
#else
		_read(fd,jpgbuf,statbuf.st_size);
#endif
		m=jpeg2yuv(jpgbuf,statbuf.st_size,reqsize);
	}else{
		ms_error("Cannot load %s",jpgpath);
	}
	return m;
}

#ifndef PACKAGE_DATA_DIR
#define PACKAGE_DATA_DIR "."
#endif

#ifndef NOWEBCAM_JPG
#define NOWEBCAM_JPG "nowebcamCIF"
#endif

mblk_t *ms_load_nowebcam(MSVideoSize *reqsize, int idx){
	char tmp[256];
	if (idx<0)
		snprintf(tmp, sizeof(tmp), "%s/images/%s.jpg", PACKAGE_DATA_DIR, NOWEBCAM_JPG);
	else
		snprintf(tmp, sizeof(tmp), "%s/images/%s%i.jpg", PACKAGE_DATA_DIR, NOWEBCAM_JPG, idx);
	return ms_load_jpeg_as_yuv(tmp,reqsize);
}

typedef struct _SIData{
	MSVideoSize vsize;
	char nowebcamimage[256];
	int index;
	uint64_t lasttime;
	mblk_t *pic;
}SIData;

void static_image_init(MSFilter *f){
	SIData *d=(SIData*)ms_new(SIData,1);
	d->vsize.width=MS_VIDEO_SIZE_CIF_W;
	d->vsize.height=MS_VIDEO_SIZE_CIF_H;
	memset(d->nowebcamimage, 0, sizeof(d->nowebcamimage));
	d->index=-1;
	d->lasttime=0;
	d->pic=NULL;
	f->data=d;
}

void static_image_uninit(MSFilter *f){
	ms_free(f->data);
}

void static_image_preprocess(MSFilter *f){
	SIData *d=(SIData*)f->data;
	if (d->pic==NULL){
		if (d->nowebcamimage[0] != '\0')
			d->pic=ms_load_jpeg_as_yuv(d->nowebcamimage,&d->vsize);
		else
			d->pic=ms_load_nowebcam(&d->vsize,d->index);
	}
}

void static_image_process(MSFilter *f){
	SIData *d=(SIData*)f->data;
	/*output a frame every second*/
	if ((f->ticker->time - d->lasttime>1000) || d->lasttime==0){
		ms_mutex_lock(&f->lock);
		if (d->pic) {
			mblk_t *o=dupb(d->pic);
			/*prevent mirroring at the output*/
			mblk_set_precious_flag(o,1);
			ms_queue_put(f->outputs[0],o);
		}
		ms_mutex_unlock(&f->lock);
		d->lasttime=f->ticker->time;
	}
}

void static_image_postprocess(MSFilter *f){
	SIData *d=(SIData*)f->data;
	if (d->pic) {
		freemsg(d->pic);
		d->pic=NULL;
	}
}

int static_image_set_vsize(MSFilter *f, void* data){
	SIData *d=(SIData*)f->data;
	d->vsize=*(MSVideoSize*)data;
	return 0;
}

int static_image_get_vsize(MSFilter *f, void* data){
	SIData *d=(SIData*)f->data;
	*(MSVideoSize*)data=d->vsize;
	return 0;
}

int static_image_get_pix_fmt(MSFilter *f, void *data){
	*(MSPixFmt*)data=MS_YUV420P;
	return 0;
}

static int static_image_set_image(MSFilter *f, void *arg){
	SIData *d=(SIData*)f->data;
	char *image = (char *)arg;
	ms_mutex_lock(&f->lock);
	if (image!=NULL && image[0]!='\0')
		snprintf(d->nowebcamimage, sizeof(d->nowebcamimage), "%s", image);
	else
		d->nowebcamimage[0] = '\0';

	if (d->pic!=NULL)
		freemsg(d->pic);

  //if (d->nowebcamimage[0] != '\0')
	 // d->pic=ms_load_jpeg_as_yuv(d->nowebcamimage,&d->vsize);
  //else
	 // d->pic=ms_load_nowebcam(&d->vsize,d->index);
	ms_mutex_unlock(&f->lock);
	return 0;
}

MSFilterMethod static_image_methods[]={
	{	MS_FILTER_SET_VIDEO_SIZE, static_image_set_vsize },
	{	MS_FILTER_GET_VIDEO_SIZE, static_image_get_vsize },
	{	MS_FILTER_GET_PIX_FMT, static_image_get_pix_fmt },
	{	MS_FILTER_SET_IMAGE, static_image_set_image },
	{	0,0 }
};

MSFilterDesc ms_static_image_desc={
	MS_STATIC_IMAGE_ID,
	"MSStaticImage",
	"A filter that outputs a static image.",
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	static_image_init,
	static_image_preprocess,
	static_image_process,
	static_image_postprocess,
	static_image_uninit,
	static_image_methods
};

MS_FILTER_DESC_EXPORT(ms_static_image_desc)

static void static_image_detect(MSWebCamManager *obj);

static void static_image_cam_init(MSWebCam *cam){
	cam->name=ms_strdup("Static picture");
}


static MSFilter *static_image_create_reader(MSWebCam *obj){
	return ms_filter_new_from_desc(&ms_static_image_desc);
}

MSWebCamDesc static_image_desc={
	"StaticImage",
	&static_image_detect,
	&static_image_cam_init,
	&static_image_create_reader,
	NULL
};

static void static_image_detect(MSWebCamManager *obj){
	MSWebCam *cam=ms_web_cam_new(&static_image_desc);
	ms_web_cam_manager_add_cam(obj,cam);
}

