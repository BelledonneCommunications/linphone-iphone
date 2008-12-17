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

#include "nowebcam.h"
#include "mediastreamer2/mswebcam.h"

#ifndef AVSTREAMMASTER_NONE
#define AVSTREAMMASTER_NONE 1
#endif

#define AMD_HACK2

typedef struct V4wState{
#ifdef AMD_HACK2
	ms_thread_t thread;
	ms_mutex_t thread_lock;
	ms_cond_t thread_cond;
	bool_t thread_running;
#endif
	char dev[512];
	int devidx;
	HWND capvideo;
	MSVideoSize vsize;
	int pix_fmt;
	mblk_t *mire[10];
	mblk_t *reverted;
	queue_t rq;
	ms_mutex_t mutex;
	int frame_ind;
	int frame_max;
	float fps;
	float start_time;
	int frame_count;
	bool_t running;
	bool_t startwith_yuv_bug; /* avoid bug with USB vimicro cards. */
	bool_t started;
	bool_t autostarted;
	bool_t invert_rgb;
}V4wState;

static void dummy(void*p){
}

LRESULT CALLBACK VideoStreamCallback(HWND hWnd, LPVIDEOHDR lpVHdr)
{
	V4wState *s;
	mblk_t *buf;
	int size;
	
	s = (V4wState *)capGetUserData(hWnd);
	if (s==NULL)
		return FALSE;

	size = lpVHdr->dwBufferLength;
	if (size>0 && s->running){
		buf = esballoc(lpVHdr->lpData,size,0,dummy);
		buf->b_wptr+=size;  
		
		ms_mutex_lock(&s->mutex);
		putq(&s->rq, buf);
		ms_mutex_unlock(&s->mutex);
	}
	return TRUE ;
}

static bool_t try_format(V4wState *s, BITMAPINFO *videoformat, MSPixFmt pixfmt){
	bool_t ret;
	capGetVideoFormat(s->capvideo, videoformat, sizeof(BITMAPINFO));
	videoformat->bmiHeader.biSizeImage = 0;
	videoformat->bmiHeader.biWidth  = s->vsize.width;
	videoformat->bmiHeader.biHeight = s->vsize.height;
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
	ret=capSetVideoFormat(s->capvideo, videoformat, sizeof(BITMAPINFO));
	if (ret) {
		/*recheck video format */
		capGetVideoFormat(s->capvideo, videoformat, sizeof(BITMAPINFO));
	}
	return ret;
}

static int v4w_open_videodevice(V4wState *s)
{
	CAPTUREPARMS capparam ;
	BITMAPINFO videoformat;
	char compname[5];
	int i;
	MSPixFmt driver_last;
	char dev[80];
	char ver[80];
	compname[4]='\0';

	for (i = 0; i < 9; i++){
		if (capGetDriverDescription(i, dev, sizeof (dev),
			ver, sizeof (ver)))
		{
			snprintf(s->dev, sizeof(s->dev), "%s/%s",dev,ver);
			ms_message("v4w: detected %s",s->dev);
			s->devidx=i;
			break;
		}
	}
	if (s->capvideo==NULL)
	{
		s->capvideo = capCreateCaptureWindow("Capture Window",WS_CHILD /* WS_OVERLAPPED */
			,0,0,s->vsize.width,s->vsize.height,HWND_MESSAGE, 0) ;
		if (s->capvideo==NULL)
		{
			ms_warning("v4w: could not create capture windows");
			return -1;
		}
	}

	if(!capDriverConnect(s->capvideo,s->devidx ))
	{
		ms_warning("v4w: could not connect to capture driver");
		DestroyWindow(s->capvideo);
		s->capvideo=NULL;
		s->pix_fmt=MS_YUV420P; /* no webcam stuff */
		return -1;
	}
	/*
	capPreviewRate(s->capvideo,s->fps) ;
	if(!capPreview (s->capvideo, 1))
	{
		ms_warning("v4w: cannot start video preview");
		capDriverDisconnect(s->capvideo);
		DestroyWindow(s->capvideo);
		s->capvideo=NULL;
		return -1;
	}
	*/
	capCaptureGetSetup(s->capvideo,&capparam,sizeof(capparam)) ;
	capparam.dwRequestMicroSecPerFrame = 100000 ;
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

	if (!capCaptureSetSetup(s->capvideo,&capparam,sizeof(capparam))){
		ms_error("capCaptureSetSetup failed.");
	}
	capSetUserData(s->capvideo, s);
	capGetVideoFormat(s->capvideo, &videoformat, sizeof(BITMAPINFO));
	/* "orig planes = " disp->videoformat.bmiHeader.biPlanes */
	/* "orig bitcount = " disp->videoformat.bmiHeader.biBitCount */
	/* "orig compression = " disp->videoformat.bmiHeader.biCompression */
	memcpy(compname,&videoformat.bmiHeader.biCompression,4);
	ms_message("v4w: camera's current format is %s", compname);

	driver_last=ms_fourcc_to_pix_fmt(videoformat.bmiHeader.biCompression);
	if (s->startwith_yuv_bug==TRUE && try_format(s,&videoformat,MS_RGB24)){
		s->pix_fmt=MS_RGB24;
		ms_message("Using RGB24");
	}else if (driver_last!=MS_PIX_FMT_UNKNOWN && try_format(s,&videoformat,driver_last)){
		ms_message("Using driver last setting");
		s->pix_fmt=driver_last;
	}else if (try_format(s,&videoformat,MS_YUV420P)){
		s->pix_fmt=MS_YUV420P;
		ms_message("Using YUV420P");
	}else if (try_format(s,&videoformat,MS_RGB24)){
		s->pix_fmt=MS_RGB24;
		ms_message("Using RGB24");
		s->startwith_yuv_bug=TRUE;
	}else if (try_format(s,&videoformat,MS_YUY2)){
		s->pix_fmt=MS_YUY2;
		ms_message("Using YUY2");
	}else{
		ms_error("v4w: Failed to set any video format.");
		capDriverDisconnect (s->capvideo);
		DestroyWindow(s->capvideo);
		s->capvideo=NULL;
		return -1;
	}
	if (s->pix_fmt==MS_RGB24){
		s->invert_rgb=(videoformat.bmiHeader.biHeight>0);
	}else s->invert_rgb=FALSE;

	if (!capSetCallbackOnVideoStream(s->capvideo, VideoStreamCallback))
	{
		ms_error("v4w: fail to set capture callback");
		capDriverDisconnect (s->capvideo);
		DestroyWindow(s->capvideo);
		s->capvideo=NULL;
		return -1;
	}
	if (!capCaptureSequenceNoFile(s->capvideo)){
		ms_error("v4w: fail to start capture");
		capDriverDisconnect (s->capvideo);
		capSetCallbackOnVideoStream(s->capvideo, NULL);
		DestroyWindow(s->capvideo);
		s->capvideo=NULL;
	}
	return 0;
}

static void v4w_init(MSFilter *f){
	V4wState *s=(V4wState *)ms_new0(V4wState,1);
	int idx;
	s->vsize.width=MS_VIDEO_SIZE_CIF_W;
	s->vsize.height=MS_VIDEO_SIZE_CIF_H;
	s->pix_fmt=MS_RGB24;

	s->capvideo=NULL;
	qinit(&s->rq);
	for (idx=0;idx<10;idx++)
	{
		s->mire[idx]=NULL;
	}
	ms_mutex_init(&s->mutex,NULL);
	s->start_time=0;
	s->frame_count=-1;
	s->fps=15;
	s->started=FALSE;
	s->autostarted=FALSE;
	s->invert_rgb=FALSE;
	s->reverted=NULL;
#ifdef AMD_HACK2
	/* avoid bug with USB vimicro cards:
		How can I detect that this problem exist?
	*/
	s->startwith_yuv_bug=FALSE;
#endif

#ifdef AMD_HACK2
	s->thread = NULL;
	ms_mutex_init(&s->thread_lock,NULL);
	ms_cond_init(&s->thread_cond,NULL);
	s->thread_running = FALSE;
#endif

	f->data=s;
}

static int _v4w_start(V4wState *s, void *arg)
{
	int i;
	s->frame_count=-1;
	i = v4w_open_videodevice(s);
	if (i==0 && s->startwith_yuv_bug==TRUE)
	{
		/* reopen device directly with MS_RGB24 */
		if (s->capvideo){
			capSetUserData(s->capvideo, (long) 0);
			capCaptureStop(s->capvideo);
			capCaptureAbort(s->capvideo);
			capDriverDisconnect(s->capvideo);
			capSetCallbackOnVideoStream(s->capvideo, NULL);
			flushq(&s->rq,0);
			ms_message("v4w: destroying capture window");
			DestroyWindow(s->capvideo);
			ms_message("v4w: capture window destroyed");
			s->capvideo=NULL;
		}
		i = v4w_open_videodevice(s);
	}
	return i;
}

static int _v4w_stop(V4wState *s, void *arg){
	s->frame_count=-1;
	if (s->capvideo){
		capCaptureStop(s->capvideo);
		Sleep(1000);
		//capCaptureAbort(s->capvideo);
		capSetCallbackOnVideoStream(s->capvideo, NULL);
		//SendMessage(s->capvideo, WM_CLOSE, 0, 0);
		capDriverDisconnect(s->capvideo);
		capSetUserData(s->capvideo, (long) 0);
		flushq(&s->rq,0);
		ms_message("v4w: destroying capture window");
		DestroyWindow(s->capvideo);
		ms_message("v4w: capture window destroyed");
		s->capvideo=NULL;
	}
#if 0
	if (s->capvideo){
		CAPSTATUS status;
		capCaptureStop(s->capvideo);
		capDriverDisconnect(s->capvideo);
		capCaptureAbort(s->capvideo);

		capSetCallbackOnVideoStream(s->capvideo, NULL);
		while (1)
		{
			if (capGetStatus(s->capvideo, &status, sizeof(status)))
			{
				if (status.fCapturingNow==FALSE)
					break;
				Sleep(10);
				ms_message("still capturing");
			}
		}
		DestroyWindow(s->capvideo);
		s->capvideo=NULL;
	}
#endif
	return 0;
}

#ifdef AMD_HACK2

void *  
v4w_thread(void *arg)
{
	V4wState *s=(V4wState*)arg;
    MSG msg;
	
	ms_mutex_lock(&s->thread_lock);
	_v4w_start(s, NULL);
	ms_cond_signal(&s->thread_cond);
	ms_mutex_unlock(&s->thread_lock);

	while(s->thread_running)
	{
		BOOL fGotMessage;
		if((fGotMessage = PeekMessage(&msg, (HWND) s->capvideo, 0, 0, PM_REMOVE)) != 0)
		{
		  TranslateMessage(&msg); 
		  DispatchMessage(&msg);
		}
		else
			Sleep(10);
	}

	ms_mutex_lock(&s->thread_lock);
	_v4w_stop(s, NULL);
	ms_cond_signal(&s->thread_cond);
	ms_mutex_unlock(&s->thread_lock);
	ms_thread_exit(NULL);
	return NULL;
}


static int v4w_start(MSFilter *f, void *arg){
	V4wState *s=(V4wState*)f->data;
	s->thread_running=TRUE;
	ms_thread_create(&s->thread,NULL,v4w_thread,s);
	ms_mutex_lock(&s->thread_lock);
	ms_cond_wait(&s->thread_cond,&s->thread_lock);
	ms_mutex_unlock(&s->thread_lock);
	s->started=TRUE;
	return 0;
}

static int v4w_stop(MSFilter *f, void *arg){
	V4wState *s=(V4wState*)f->data;
	ms_mutex_lock(&s->thread_lock);
	s->thread_running=FALSE;
	//SendMessage(s->capvideo, WM_CLOSE, 0, 0);
	ms_cond_wait(&s->thread_cond,&s->thread_lock);
	ms_mutex_unlock(&s->thread_lock);
	ms_thread_join(s->thread,NULL);
	s->started=FALSE;
	return 0;
}

#else

static int v4w_start(MSFilter *f, void *arg){
	V4wState *s=(V4wState*)f->data;
	_v4w_start(s, NULL);
	s->started=TRUE;
	return 0;
}

static int v4w_stop(MSFilter *f, void *arg){
	V4wState *s=(V4wState*)f->data;
	_v4w_stop(s, NULL);
	s->started=FALSE;
	return 0;
}

#endif

static void v4w_uninit(MSFilter *f){
	V4wState *s=(V4wState*)f->data;
	int idx;
	flushq(&s->rq,0);
	ms_mutex_destroy(&s->mutex);
	for (idx=0;idx<10;idx++)
	{
		if (s->mire[idx]==NULL)
			break;
		freemsg(s->mire[idx]);
	}
	if (s->capvideo!=NULL)
	{
		ms_message("v4w: destroying capture window");
		DestroyWindow(s->capvideo);
		ms_message("v4w: capture window destroyed");
		s->capvideo=NULL;
	}
	if (s->reverted){
		freemsg(s->reverted);
	}
#ifdef AMD_HACK2
	ms_cond_destroy(&s->thread_cond);
	ms_mutex_destroy(&s->thread_lock);
#endif
	ms_free(s);
}

static mblk_t * v4w_make_nowebcam(V4wState *s){
	int idx;
	int count;
	if (s->mire[0]==NULL && s->frame_ind==0){
		/* load several images to fake a movie */
		for (idx=0;idx<10;idx++)
		{
			s->mire[idx]=ms_load_nowebcam(&s->vsize, idx);
			if (s->mire[idx]==NULL)
				break;
		}
		if (idx==0)
			s->mire[0]=ms_load_nowebcam(&s->vsize, -1);
	}
	for (count=0;count<10;count++)
	{
		if (s->mire[count]==NULL)
			break;
	}

	s->frame_ind++;
	if (count==0)
		return NULL;

	idx = s->frame_ind%count;
	if (s->mire[idx]!=NULL)
		return s->mire[idx];
	return s->mire[0];
}

static void v4w_preprocess(MSFilter * obj){
	V4wState *s=(V4wState*)obj->data;
	if (!s->started) {
		ms_message("V4W auto-started.");
		v4w_start(obj,NULL);
		s->autostarted=TRUE;
	}
	s->running=TRUE;
	if (s->capvideo==NULL)
		s->fps=1;
}

static void v4w_postprocess(MSFilter * obj){
	V4wState *s=(V4wState*)obj->data;
	s->running=FALSE;
	if (s->autostarted){
		v4w_stop(obj,NULL);
	}
}

static void v4w_process(MSFilter * obj){
	V4wState *s=(V4wState*)obj->data;
	mblk_t *m;
	uint32_t timestamp;
	int cur_frame;

	if (s->frame_count==-1){
		s->start_time=obj->ticker->time;
		s->frame_count=0;
	}


	cur_frame=((obj->ticker->time-s->start_time)*s->fps/1000.0);
	if (cur_frame>s->frame_count){
		mblk_t *om=NULL;
		ms_mutex_lock(&s->mutex);
		/*keep the most recent frame if several frames have been captured */
		if (s->capvideo!=NULL){
			while((m=getq(&s->rq))!=NULL){
				if (om!=NULL) freemsg(om);
				om=m;
			}
			if (om!=NULL){
				if (s->invert_rgb){
					MSVideoSize roi;
					if (s->reverted==NULL){
						s->reverted=allocb(om->b_wptr-om->b_rptr,0);
						s->reverted->b_wptr=s->reverted->b_datap->db_lim;
					}
					roi=s->vsize;
					rgb24_copy_revert(s->reverted->b_rptr,roi.width*3,
									om->b_rptr,roi.width*3,roi);
					freemsg(om);
					om=dupb(s->reverted);
				}
			}
		}else {
			mblk_t *nowebcam = v4w_make_nowebcam(s);
			if (nowebcam!=NULL){
				om=dupmsg(nowebcam);
				mblk_set_precious_flag(om,1);
			}
		}
		ms_mutex_unlock(&s->mutex);
		if (om!=NULL){
			timestamp=obj->ticker->time*90;/* rtp uses a 90000 Hz clockrate for video*/
			mblk_set_timestamp_info(om,timestamp);
			ms_queue_put(obj->outputs[0],om);
		}
		s->frame_count++;
	}
}

static int v4w_set_fps(MSFilter *f, void *arg){
	V4wState *s=(V4wState*)f->data;
	s->fps=*((float*)arg);
	return 0;
}

static int v4w_get_pix_fmt(MSFilter *f,void *arg){
	V4wState *s=(V4wState*)f->data;
	if (!s->started) {
		ms_message("V4W auto-started in v4w_get_pix_fmt()");
		v4w_start(f,NULL);
		s->autostarted=TRUE;
	}
	*((MSPixFmt*)arg) = (MSPixFmt)s->pix_fmt;
	return 0;
}

static int v4w_set_vsize(MSFilter *f, void *arg){
	V4wState *s=(V4wState*)f->data;
	s->vsize=*((MSVideoSize*)arg);
	return 0;
}

static int v4w_get_vsize(MSFilter *f, void *arg){
	V4wState *s=(V4wState*)f->data;
	MSVideoSize *vs=(MSVideoSize*)arg;
	vs->width=s->vsize.width;
	vs->height=s->vsize.height;
	return 0;
}

static MSFilterMethod methods[]={
	{	MS_FILTER_SET_FPS	,	v4w_set_fps	},
	{	MS_FILTER_GET_PIX_FMT	,	v4w_get_pix_fmt	},
	{	MS_FILTER_SET_VIDEO_SIZE, v4w_set_vsize	},
	{	MS_FILTER_GET_VIDEO_SIZE, v4w_get_vsize	},
	{	MS_V4L_START			,	v4w_start		},
	{	MS_V4L_STOP			,	v4w_stop		},
	{	0								,	NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_v4w_desc={
	MS_V4L_ID,
	"MSV4w",
	N_("A video4windows compatible source filter to stream pictures."),
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	v4w_init,
	v4w_preprocess,
	v4w_process,
	v4w_postprocess,
	v4w_uninit,
	methods
};

#else

MSFilterDesc ms_v4w_desc={
	.id=MS_V4L_ID,
	.name="MSV4w",
	.text=N_("A video4windows compatible source filter to stream pictures."),
	.ninputs=0,
	.noutputs=1,
	.category=MS_FILTER_OTHER,
	.init=v4w_init,
	.preprocess=v4w_preprocess,
	.process=v4w_process,
	.postprocess=v4w_postprocess,
	.uninit=v4w_uninit,
	.methods=methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_v4w_desc)

#if 0
static void ms_v4w_detect(MSWebCamManager *obj);

static void ms_v4w_cam_init(MSWebCam *cam){
}


static MSFilter *ms_v4w_create_reader(MSWebCam *obj){
	MSFilter *f= ms_filter_new_from_desc(&ms_v4w_desc);
	V4wState *s=(V4wState*)f->data;
	s->devidx=(int)obj->data;
	return f;
}

MSWebCamDesc ms_v4w_cam_desc={
	"VideoForWindows grabber",
	&ms_v4w_detect,
	&ms_v4w_cam_init,
	&ms_v4w_create_reader,
	NULL
};

static void ms_v4w_detect(MSWebCamManager *obj){
	int i;
	char dev[80];
	char ver[80];
	char name[160];
	MSWebCam *cam;
	for (i = 0; i < 9; i++){
		if (capGetDriverDescription(i, dev, sizeof (dev),
			ver, sizeof (ver))){
			HWND hwnd=capCreateCaptureWindow("Capture Window",WS_CHILD /* WS_OVERLAPPED */
				,0,0,352,288,HWND_MESSAGE, 0) ;
			if (hwnd==NULL) break;
			if(!capDriverConnect(hwnd,i )){
				ms_warning("v4w: could not connect to capture driver, no webcam connected.");
				DestroyWindow(hwnd);
				break;
			}else{
				capGetDriverDescription(i, dev, sizeof (dev),ver, sizeof (ver));
				capDriverDisconnect(hwnd);
				DestroyWindow(hwnd);
			}
			snprintf(name, sizeof(name), "%s/%s",dev,ver);
			cam=ms_web_cam_new(&ms_v4w_cam_desc);
			cam->data=(void*)i;/*store the device index */
			cam->name=ms_strdup(name);
			ms_web_cam_manager_add_cam(obj,cam);
		}
	}
}

#endif
