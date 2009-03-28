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

#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/msv4l.h"
#include "Vfw.h"
#include <winuser.h>
#include <Windows.h>

#include "mediastreamer2/mswebcam.h"

#ifndef AVSTREAMMASTER_NONE
#define AVSTREAMMASTER_NONE 1
#endif

typedef void (*queue_msg_t)(void *, mblk_t *);

typedef struct VfwEngine{
	ms_thread_t thread;
	char dev[512];
	int devidx;
	HWND capvideo;
	MSVideoSize vsize;
	MSPixFmt pix_fmt;
	queue_msg_t cb;
	void *cb_data;
	bool_t started;
	bool_t configured;
	bool_t thread_running;
}VfwEngine;

#define VFW_ENGINE_MAX_INSTANCES 9

static VfwEngine *engines[VFW_ENGINE_MAX_INSTANCES]={0};

static bool_t try_format(VfwEngine *s, BITMAPINFO *videoformat, MSPixFmt pixfmt){
	MSVideoSize tried_size=s->vsize;
	bool_t ret;
	do{
		capGetVideoFormat(s->capvideo, videoformat, sizeof(BITMAPINFO));
		videoformat->bmiHeader.biSizeImage = 0;
		videoformat->bmiHeader.biWidth  = tried_size.width;
		videoformat->bmiHeader.biHeight = tried_size.height;
		switch(pixfmt){
			case MS_YUV420P:
				videoformat->bmiHeader.biBitCount = 12;
				videoformat->bmiHeader.biCompression=MAKEFOURCC('I','4','2','0');
			break;
			case MS_YUY2:
				videoformat->bmiHeader.biBitCount = 16;
				videoformat->bmiHeader.biCompression=MAKEFOURCC('Y','U','Y','2');
			break;
			case MS_RGB24:
				videoformat->bmiHeader.biBitCount = 24;
				videoformat->bmiHeader.biCompression=BI_RGB;
			break;
			default:
				return FALSE;
		}
		ms_message("Trying video size %ix%i",tried_size.width,tried_size.height);
		ret=capSetVideoFormat(s->capvideo, videoformat, sizeof(BITMAPINFO));
		tried_size=ms_video_size_get_just_lower_than(tried_size);
	}while(ret==FALSE && tried_size.width!=0);
	if (ret) {
		/*recheck video format */
		capGetVideoFormat(s->capvideo, videoformat, sizeof(BITMAPINFO));
		s->vsize.width=videoformat->bmiHeader.biWidth;
		s->vsize.height=videoformat->bmiHeader.biHeight;
	}
	return ret;
}

static int _vfw_engine_select_format(VfwEngine *obj){
	BITMAPINFO videoformat;
	MSPixFmt driver_last;
	char compname[5];
	
	capGetVideoFormat(obj->capvideo, &videoformat, sizeof(BITMAPINFO));
	memcpy(compname,&videoformat.bmiHeader.biCompression,4);
	compname[4]='\0';
	ms_message("vfw: camera's current format is '%s' at %ix%i", compname,
			videoformat.bmiHeader.biWidth,videoformat.bmiHeader.biHeight);
	driver_last=ms_fourcc_to_pix_fmt(videoformat.bmiHeader.biCompression);
	if (driver_last!=MS_PIX_FMT_UNKNOWN && try_format(obj,&videoformat,driver_last)){
		ms_message("Using driver last setting");
		obj->pix_fmt=driver_last;
	}else if (try_format(obj,&videoformat,MS_YUV420P)){
		obj->pix_fmt=MS_YUV420P;
		ms_message("Using YUV420P");
	}else if (try_format(obj,&videoformat,MS_RGB24)){
		obj->pix_fmt=MS_RGB24;
		ms_message("Using RGB24");
	}else if (try_format(obj,&videoformat,MS_YUY2)){
		obj->pix_fmt=MS_YUY2;
		ms_message("Using YUY2");
	}else{
		ms_error("v4w: Failed to set any video format.");
		return -1;
	}
	if (obj->pix_fmt==MS_RGB24){
		if (videoformat.bmiHeader.biHeight>0){
			obj->pix_fmt=MS_RGB24_REV;
		}
	}
	return 0;
}

static void dummy(void*p){
}

LRESULT CALLBACK vfw_engine_stream_callback(HWND hWnd, LPVIDEOHDR lpVHdr)
{
	VfwEngine *s;
	mblk_t *buf;
	int size;

	s = (VfwEngine *)capGetUserData(hWnd);
	if (s==NULL)
		return FALSE;

	size = lpVHdr->dwBufferLength;
	if (size>0 && s->cb!=NULL && s->started){
		buf = esballoc(lpVHdr->lpData,size,0,dummy);
		buf->b_wptr+=size;
		s->cb(s->cb_data,buf);
	}
	return TRUE ;
}

static void *
vfw_engine_thread(void *arg)
{
	VfwEngine *s=(VfwEngine*)arg;
    MSG msg;

	while(s->thread_running)
	{
		BOOL fGotMessage;
		while((fGotMessage = PeekMessage(&msg, (HWND) s->capvideo, 0, 0, PM_REMOVE)) != 0)
		{
		  TranslateMessage(&msg);
		  DispatchMessage(&msg);
		}
		Sleep(10);
	}
	ms_thread_exit(NULL);
	return NULL;
}

static void _vfw_engine_unconfigure(VfwEngine *obj){
	if (!capCaptureStop(obj->capvideo)){
		ms_error("vfw: fail to stop capture !");
	}
	obj->thread_running=FALSE;
	ms_thread_join(obj->thread,NULL);
	obj->configured=FALSE;
}

static void _vfw_engine_disconnect(VfwEngine *obj){
	capDriverDisconnect(obj->capvideo);
	DestroyWindow(obj->capvideo);
	obj->capvideo=NULL;
}

static void vfw_engine_destroy(VfwEngine *obj){
	if (obj->configured){
		_vfw_engine_unconfigure(obj);
	}
	_vfw_engine_disconnect(obj);
	ms_free(obj);
}

static int _vfw_engine_setup(VfwEngine *obj){
	CAPTUREPARMS capparam ;
	capCaptureGetSetup(obj->capvideo,&capparam,sizeof(capparam)) ;
	capparam.dwRequestMicroSecPerFrame = 33000 ; /*makes around 30fps*/
	// detach capture from application
	capparam.fYield                    = TRUE ;
	capparam.fMakeUserHitOKToCapture   = FALSE;
	capparam.fAbortLeftMouse           = FALSE;
	capparam.fAbortRightMouse          = FALSE;
	capparam.wPercentDropForError      = 90 ;
	capparam.fCaptureAudio             = FALSE ;
	capparam.fAbortRightMouse	= FALSE;
	capparam.fAbortLeftMouse	= FALSE;
	capparam.AVStreamMaster            = AVSTREAMMASTER_NONE ;
	if (!capCaptureSetSetup(obj->capvideo,&capparam,sizeof(capparam))){
		ms_error("capCaptureSetSetup failed.");
		return -1;
	}
	capSetUserData(obj->capvideo, obj);
	return 0;
}

static int _vfw_engine_connect(VfwEngine *obj){
	MSVideoSize sz;
	sz.width=MS_VIDEO_SIZE_CIF_W;
	sz.height=MS_VIDEO_SIZE_CIF_H;
	HWND hwnd=capCreateCaptureWindow("Capture Window",WS_CHILD /* WS_OVERLAPPED */
			,0,0,sz.width,sz.height,HWND_MESSAGE, 0) ;

	if (hwnd==NULL) return -1;
	if(!capDriverConnect(hwnd,obj->devidx)){
		ms_warning("vfw: could not connect to capture driver, no webcam connected.");
		DestroyWindow(hwnd);
		return -1;
	}
	obj->capvideo=hwnd;
	obj->vsize=sz;
	return 0;
}

static VfwEngine * vfw_engine_new(int i){
	char dev[512];
	char ver[512];
	VfwEngine *obj=(VfwEngine*)ms_new0(VfwEngine,1);
	if (capGetDriverDescription(i, dev, sizeof (dev),
		ver, sizeof (ver))){
		obj->devidx=i;
		if (_vfw_engine_connect(obj)==-1){
			ms_free(obj);
			return NULL;
		}
		strcpy(obj->dev,dev);
		engines[i]=obj;
		return obj;
	}
	return NULL;
}

static void _vfw_engine_configure(VfwEngine *obj){
	if (_vfw_engine_setup(obj)==-1){
		return;
	}
	if (_vfw_engine_select_format(obj)==-1){
		return ;
	}
	capSetCallbackOnVideoStream(obj->capvideo, vfw_engine_stream_callback);
	if (!capCaptureSequenceNoFile(obj->capvideo)){
		ms_error("vfw: fail to start capture !");
	}
	ms_thread_create(&obj->thread,NULL,vfw_engine_thread,obj);
	obj->configured=TRUE;
}

static MSPixFmt vfw_engine_get_pix_fmt(VfwEngine *obj){
	if (!obj->configured)
		_vfw_engine_configure(obj); 
	return obj->pix_fmt;
}

static MSVideoSize vfw_engine_get_video_size(VfwEngine *obj){
	return obj->vsize;
}

static void vfw_engine_set_video_size(VfwEngine *obj, MSVideoSize vsize){
	if (!obj->configured)
		obj->vsize=vsize;
	else if (ms_video_size_greater_than(vsize,obj->vsize) && !ms_video_size_equal(vsize,obj->vsize) ){
		_vfw_engine_unconfigure(obj);
		_vfw_engine_disconnect(obj);
		_vfw_engine_connect(obj);
		obj->vsize=vsize;
		_vfw_engine_configure(obj);
	}
}

static void vfw_engine_set_callback(VfwEngine* obj, queue_msg_t cb, void *cb_data){
	obj->cb=cb;
	obj->cb_data=cb_data;
}

static void vfw_engine_start_capture(VfwEngine *obj){
	if (!obj->configured) _vfw_engine_configure(obj);
	obj->started=TRUE;
}

static void vfw_engine_stop_capture(VfwEngine *obj){
	obj->started=FALSE;
}

static void vfw_engines_free(void){
	int i;
	for(i=0;i<VFW_ENGINE_MAX_INSTANCES;++i){
		if (engines[i])
			vfw_engine_destroy(engines[i]);
	}
}


typedef struct VfwState{
	MSVideoSize vsize;
	queue_t rq;
	ms_mutex_t mutex;
	int frame_ind;
	int frame_max;
	float fps;
	float start_time;
	int frame_count;
	VfwEngine *eng;
} VfwState;


static void vfw_init(MSFilter *f){
	VfwState *s=(VfwState *)ms_new0(VfwState,1);
	s->vsize.width=MS_VIDEO_SIZE_CIF_W;
	s->vsize.height=MS_VIDEO_SIZE_CIF_H;
	qinit(&s->rq);
	ms_mutex_init(&s->mutex,NULL);
	s->start_time=0;
	s->frame_count=-1;
	s->fps=15;
	f->data=s;
}



static void vfw_uninit(MSFilter *f){
	VfwState *s=(VfwState*)f->data;
	flushq(&s->rq,0);
	ms_mutex_destroy(&s->mutex);
	ms_free(s);
}

static void vfw_callback(void *data, mblk_t *m){
	VfwState *s=(VfwState*)data;
	ms_mutex_lock(&s->mutex);
	putq(&s->rq,m);
	ms_mutex_unlock(&s->mutex);
}

static void vfw_preprocess(MSFilter * obj){
	VfwState *s=(VfwState*)obj->data;
	if (s->eng==NULL) s->eng=engines[0];
	vfw_engine_set_callback(s->eng,vfw_callback,s);
	vfw_engine_start_capture(s->eng);
}

static void vfw_postprocess(MSFilter * obj){
	VfwState *s=(VfwState*)obj->data;
	vfw_engine_stop_capture(s->eng);
	flushq(&s->rq,0);
}

static void vfw_process(MSFilter * obj){
	VfwState *s=(VfwState*)obj->data;
	mblk_t *m;
	uint32_t timestamp;
	int cur_frame;

	if (s->frame_count==-1){
		s->start_time=(float)obj->ticker->time;
		s->frame_count=0;
	}

	cur_frame=(int)((obj->ticker->time-s->start_time)*s->fps/1000.0);
	if (cur_frame>s->frame_count){
		mblk_t *om=NULL;
		/*keep the most recent frame if several frames have been captured */
		if (s->eng!=NULL){
			ms_mutex_lock(&s->mutex);
			while((m=getq(&s->rq))!=NULL){
				ms_mutex_unlock(&s->mutex);
				if (om!=NULL) freemsg(om);
				om=m;
				ms_mutex_lock(&s->mutex);
			}
			ms_mutex_unlock(&s->mutex);
		}
		if (om!=NULL){
			timestamp=(uint32_t)(obj->ticker->time*90);/* rtp uses a 90000 Hz clockrate for video*/
			mblk_set_timestamp_info(om,timestamp);
			ms_queue_put(obj->outputs[0],om);
		}
		s->frame_count++;
	}
}

static int vfw_set_fps(MSFilter *f, void *arg){
	VfwState *s=(VfwState*)f->data;
	s->fps=*((float*)arg);
	return 0;
}

static int vfw_get_pix_fmt(MSFilter *f,void *arg){
	VfwState *s=(VfwState*)f->data;
	MSPixFmt fmt=vfw_engine_get_pix_fmt(s->eng);
	*((MSPixFmt*)arg)=fmt;
	return 0;
}

static int vfw_set_vsize(MSFilter *f, void *arg){
	VfwState *s=(VfwState*)f->data;
	s->vsize=*((MSVideoSize*)arg);
	vfw_engine_set_video_size(s->eng,s->vsize);
	return 0;
}

static int vfw_get_vsize(MSFilter *f, void *arg){
	VfwState *s=(VfwState*)f->data;
	MSVideoSize *vs=(MSVideoSize*)arg;
	*vs=vfw_engine_get_video_size(s->eng);
	return 0;
}

static MSFilterMethod methods[]={
	{	MS_FILTER_SET_FPS	,	vfw_set_fps	},
	{	MS_FILTER_GET_PIX_FMT	,	vfw_get_pix_fmt	},
	{	MS_FILTER_SET_VIDEO_SIZE, vfw_set_vsize	},
	{	MS_FILTER_GET_VIDEO_SIZE, vfw_get_vsize	},
	{	0								,	NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_vfw_desc={
	MS_VFW_ID,
	"MSVfw",
	N_("A video for windows (vfw.h) based source filter to grab pictures."),
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	vfw_init,
	vfw_preprocess,
	vfw_process,
	vfw_postprocess,
	vfw_uninit,
	methods
};

#else

MSFilterDesc ms_vfw_desc={
	.id=MS_VFW_ID,
	.name="MSVfw",
	.text=N_("A video for windows (vfw.h) based source filter to grab pictures."),
	.ninputs=0,
	.noutputs=1,
	.category=MS_FILTER_OTHER,
	.init=vfw_init,
	.preprocess=vfw_preprocess,
	.process=vfw_process,
	.postprocess=vfw_postprocess,
	.uninit=vfw_uninit,
	.methods=methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_vfw_desc)

static void ms_vfw_detect(MSWebCamManager *obj);

static void ms_vfw_cam_init(MSWebCam *cam){
}


static MSFilter *ms_vfw_create_reader(MSWebCam *obj){
	MSFilter *f= ms_filter_new_from_desc(&ms_vfw_desc);
	VfwState *s=(VfwState*)f->data;
	s->eng=(VfwEngine*)obj->data;
	return f;
}

MSWebCamDesc ms_vfw_cam_desc={
	"VideoForWindows grabber",
	&ms_vfw_detect,
	&ms_vfw_cam_init,
	&ms_vfw_create_reader,
	NULL
};

static void ms_vfw_detect(MSWebCamManager *obj){
	int i;
	MSWebCam *cam;
	for (i = 0; i < VFW_ENGINE_MAX_INSTANCES; i++){
		VfwEngine *eng;
		if ((eng=vfw_engine_new(i))!=NULL){
			cam=ms_web_cam_new(&ms_vfw_cam_desc);
			cam->data=(void*)eng;/*store the engine */
			cam->name=ms_strdup(eng->dev);
			ms_web_cam_manager_add_cam(obj,cam);
		}
	}
	atexit(vfw_engines_free);
}

