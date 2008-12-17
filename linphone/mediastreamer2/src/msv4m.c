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

#ifdef __APPLE__

#include "mediastreamer-config.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/msv4l.h"
#include "nowebcam.h"

// build for carbon
#define TARGET_API_MAC_CARBON 1

#if __APPLE_CC__
  #include <Carbon/Carbon.h>
  #include <QuicKTime/QuickTime.h>
#else
  #include <ConditionalMacros.h>
  #include <QuickTimeComponents.h>
  #include <TextUtils.h>

  #include <stdio.h>
#endif

typedef struct V4lState{
  SeqGrabComponent seqgrab;
  SGChannel sgchanvideo;
  GWorldPtr pgworld;
  ImageSequence   decomseq;

  char *mmapdbuf;
  int msize;/*mmapped size*/
  MSVideoSize vsize;
  MSVideoSize got_vsize;
  int pix_fmt;
  int int_pix_fmt; /*internal pixel format */
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
}V4lState;

static void v4m_init(MSFilter *f){
	V4lState *s=ms_new0(V4lState,1);
	s->seqgrab=NULL;
	s->sgchanvideo=NULL;
	s->pgworld=NULL;
	s->decomseq=0;

	s->run=FALSE;
	s->mmapdbuf=NULL;
	s->vsize.width=MS_VIDEO_SIZE_CIF_W;
	s->vsize.height=MS_VIDEO_SIZE_CIF_H;
	s->pix_fmt=MS_RGB24;
	qinit(&s->rq);
	s->mire=NULL;
	ms_mutex_init(&s->mutex,NULL);
	s->start_time=0;
	s->frame_count=-1;
	s->fps=15;
	s->usemire=(getenv("DEBUG")!=NULL);
	s->queued=0;
	f->data=s;
}


#define BailErr(x) {err = x; if(err != noErr) goto bail;}

pascal OSErr sgdata_callback(SGChannel c, Ptr p, long len, long *offset, long chRefCon, TimeValue time, short writeType, long refCon);
pascal OSErr sgdata_callback(SGChannel c, Ptr p, long len, long *offset, long chRefCon, TimeValue time, short writeType, long refCon)
{
#pragma unused(offset,chRefCon,time,writeType)
    
    CodecFlags     ignore;
    V4lState *s=(V4lState *)refCon;
    ComponentResult err = noErr;
    
    if (!s) goto bail;
   
    Rect boundsRect = {0, 0, s->vsize.height, s->vsize.width}; /* 240 , 320*/
    if (s->pgworld) {

      if (s->decomseq == 0) {
	Rect sourceRect = { 0, 0 };
	MatrixRecord scaleMatrix;
	ImageDescriptionHandle imageDesc = (ImageDescriptionHandle)NewHandle(0);
	
	err = SGGetChannelSampleDescription(c,(Handle)imageDesc);
	BailErr(err);
	
	// make a scaling matrix for the sequence
	sourceRect.right = (**imageDesc).width;
	sourceRect.bottom = (**imageDesc).height;
	RectMatrix(&scaleMatrix, &sourceRect, &boundsRect);
            
	err = DecompressSequenceBegin(&s->decomseq,  // pointer to field to receive unique ID for sequence
				      imageDesc,        // handle to image description structure
				      s->pgworld,    // port for the DESTINATION image
				      NULL,            // graphics device handle, if port is set, set to NULL
				      NULL,            // source rectangle defining the portion of the image to decompress
				      &scaleMatrix,        // transformation matrix
				      srcCopy,          // transfer mode specifier
				      NULL,            // clipping region in dest. coordinate system to use as a mask
				      0,            // flags
				      codecNormalQuality,    // accuracy in decompression
				      bestSpeedCodec);      // compressor identifier or special identifiers ie. bestSpeedCodec
	BailErr(err);
	
	DisposeHandle((Handle)imageDesc);
	imageDesc = NULL;
      }
      
      // decompress a frame into the GWorld - can queue a frame for async decompression when passed in a completion proc
      // once the image is in the GWorld it can be manipulated at will
      err = DecompressSequenceFrameS(s->decomseq,  // sequence ID returned by DecompressSequenceBegin
				     p,            // pointer to compressed image data
				     len,          // size of the buffer
				     0,            // in flags
				     &ignore,        // out flags
				     NULL);          // async completion proc
        BailErr(err);
        
    {
      unsigned line;
      mblk_t *buf;
      int size = s->vsize.width * s->vsize.height * 3;
      buf=allocb(size,0);
      
      PixMap * pixmap = *GetGWorldPixMap(s->pgworld);
      uint8_t * data;
      unsigned rowBytes = pixmap->rowBytes & (((unsigned short) 0xFFFF) >> 2);
      unsigned pixelSize = pixmap->pixelSize / 8; // Pixel size in bytes
      unsigned lineOffset = rowBytes - s->vsize.width * pixelSize;
      
      data = (uint8_t *) GetPixBaseAddr(GetGWorldPixMap(s->pgworld));
      
      for (line = 0 ; line < s->vsize.height ; line++) {
	unsigned offset = line * (s->vsize.width * pixelSize + lineOffset);
	memcpy(buf->b_wptr + ((line * s->vsize.width) * pixelSize), data + offset, (rowBytes - lineOffset));
      }

      if (s->pix_fmt==MS_RGB24)
	{
	  /* Conversion from top down bottom up (BGR to RGB and flip) */
	  unsigned long Index,nPixels;
	  unsigned char *blue;
	  unsigned char tmp;
	  short iPixelSize;

	  blue=buf->b_wptr;

	  nPixels=s->vsize.width*s->vsize.height;
	  iPixelSize=24/8;

	  for(Index=0;Index!=nPixels;Index++)  // For each pixel
	    {
	      tmp=*blue;
	      *blue=*(blue+2);
	      *(blue+2)=tmp;
	      blue+=iPixelSize;
	    }
	}

      buf->b_wptr+=size;
      //ms_mutex_lock(&s->mutex); /* called during SGIdle? */
      putq(&s->rq, buf);
      //ms_mutex_unlock(&s->mutex);
    }
  }

bail:
  return err;
}

static int v4m_close(V4lState *s)
{
  if(s->seqgrab)
    CloseComponent(s->seqgrab);
  s->seqgrab=NULL;
  if (s->decomseq)
    CDSequenceEnd(s->decomseq);
  s->decomseq=NULL;
  if (s->pgworld!=NULL)
    DisposeGWorld(s->pgworld);
  s->pgworld=NULL;
  return 0;
}

static int sequence_grabber_start(V4lState *s)
{
  int err;
  Rect        theRect = {0, 0, s->vsize.height, s->vsize.width};

  err = QTNewGWorld(&(s->pgworld),  // returned GWorld
		    k24BGRPixelFormat,
		    &theRect,      // bounding rectangle
		    0,             // color table
		    NULL,          // graphic device handle
		    0);            // flags
  if (err!=noErr)
    {
      return -1;
    }

  if(!LockPixels(GetPortPixMap(s->pgworld)))
    {
      v4m_close(s);
      return -1;
    }

  s->seqgrab = OpenDefaultComponent(SeqGrabComponentType, 0);
  err = SGInitialize(s->seqgrab);
  if (err!=noErr)
    {
      v4m_close(s);
      return -1;
    }
  err = SGSetDataRef(s->seqgrab, 0, 0, seqGrabDontMakeMovie);
  if (err!=noErr)
    {
      v4m_close(s);
      return -1;
    }

  err = SGSetGWorld(s->seqgrab, s->pgworld, GetMainDevice());
  if (err!=noErr)
    {
      v4m_close(s);
      return -1;
    }

  err = SGNewChannel(s->seqgrab, VideoMediaType, &s->sgchanvideo);
  if (err!=noErr)
    {
      v4m_close(s);
      return -1;
    }

  err = SGSetChannelBounds(s->sgchanvideo, &theRect);
  if (err!=noErr)
    {
      v4m_close(s);
      return -1;
    }

  err = SGSetChannelUsage(s->sgchanvideo, seqGrabRecord);
  if (err!=noErr)
    {
      v4m_close(s);
      return -1;
    }

  err = SGSetDataProc(s->seqgrab,NewSGDataUPP(sgdata_callback),(long)s);
  if (err!=noErr)
    {
      v4m_close(s);
      return -1;
    }

  err = SGStartRecord(s->seqgrab);
  if (err!=noErr)
    {
      v4m_close(s);
      return -1;
    }

  return 0;
}

static int v4m_start(MSFilter *f, void *arg)
{
	V4lState *s=(V4lState*)f->data;
	int err=0;

	err = sequence_grabber_start(s);

	if (err!=0)
	  {
	    s->pix_fmt=MS_YUV420P;
	    s->vsize.width=MS_VIDEO_SIZE_CIF_W;
	    s->vsize.height=MS_VIDEO_SIZE_CIF_H;
	    return 0;
	  }

	ms_message("v4m video device opened.");
	s->pix_fmt=MS_RGB24;

	return 0;
}

static void v4m_start_capture(V4lState *s){
	if (s->seqgrab!=NULL){
		s->run=TRUE;
	}
}

static int v4m_stop(MSFilter *f, void *arg){
	V4lState *s=(V4lState*)f->data;
	if (s->seqgrab!=NULL){
	  ms_mutex_lock(&s->mutex);
	  SGStop(s->seqgrab);
	  v4m_close(s);
	  flushq(&s->rq,0);
	  ms_mutex_unlock(&s->mutex);
	}
	return 0;
}

static void v4m_stop_capture(V4lState *s){
	if (s->run){
		s->run=FALSE;
		ms_message("v4m capture stopped.");
	}
}


static void v4m_uninit(MSFilter *f){
	V4lState *s=(V4lState*)f->data;
	if (s->seqgrab!=NULL) v4m_stop(f,NULL);
	//ms_free(s->dev);
	flushq(&s->rq,0);
	ms_mutex_destroy(&s->mutex);
	freemsg(s->mire);
	ms_free(s);
}

static mblk_t * v4m_make_mire(V4lState *s){
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

static mblk_t * v4m_make_nowebcam(V4lState *s){
	if (s->mire==NULL && s->frame_ind==0){
		s->mire=ms_load_nowebcam(&s->vsize, -1);
	}
	s->frame_ind++;
	return s->mire;
}

static void v4m_process(MSFilter * obj){
	V4lState *s=(V4lState*)obj->data;
	uint32_t timestamp;
	int cur_frame;
	if (s->frame_count==-1){
		s->start_time=obj->ticker->time;
		s->frame_count=0;
	}

	ms_mutex_lock(&s->mutex);

	if (s->seqgrab!=NULL)
	{
	  SGIdle(s->seqgrab);
	}

	cur_frame=((obj->ticker->time-s->start_time)*s->fps/1000.0);
	if (cur_frame>=s->frame_count){
		mblk_t *om=NULL;
		/*keep the most recent frame if several frames have been captured */
		if (s->seqgrab!=NULL){
			om=getq(&s->rq);
		}else{
		  if (s->pix_fmt==MS_YUV420P
		      && s->vsize.width==MS_VIDEO_SIZE_CIF_W
		      && s->vsize.height==MS_VIDEO_SIZE_CIF_H)
		    {
			if (s->usemire){
				om=dupmsg(v4m_make_mire(s));
			}else {
				mblk_t *tmpm=v4m_make_nowebcam(s);
				if (tmpm) om=dupmsg(tmpm);
			}
		    }
		}
		if (om!=NULL){
			timestamp=obj->ticker->time*90;/* rtp uses a 90000 Hz clockrate for video*/
			mblk_set_timestamp_info(om,timestamp);
			mblk_set_marker_info(om,TRUE);
			ms_queue_put(obj->outputs[0],om);
			/*ms_message("picture sent");*/
			s->frame_count++;
		}
	}else flushq(&s->rq,0);

	ms_mutex_unlock(&s->mutex);
}

static void v4m_preprocess(MSFilter *f){
	V4lState *s=(V4lState*)f->data;
	v4m_start_capture(s);
}

static void v4m_postprocess(MSFilter *f){
	V4lState *s=(V4lState*)f->data;
	v4m_stop_capture(s);
}

static int v4m_set_fps(MSFilter *f, void *arg){
	V4lState *s=(V4lState*)f->data;
	s->fps=*((float*)arg);
	s->frame_count=-1;
	return 0;
}

static int v4m_get_pix_fmt(MSFilter *f,void *arg){
	V4lState *s=(V4lState*)f->data;
	*((MSPixFmt*)arg) = s->pix_fmt;
	return 0;
}

static int v4m_set_vsize(MSFilter *f, void *arg){
	V4lState *s=(V4lState*)f->data;
	s->vsize=*((MSVideoSize*)arg);
	return 0;
}

static int v4m_get_vsize(MSFilter *f, void *arg){
	V4lState *s=(V4lState*)f->data;
	*(MSVideoSize*)arg=s->vsize;
	return 0;
}

static MSFilterMethod methods[]={
	{	MS_FILTER_SET_FPS	,	v4m_set_fps	},
	{	MS_FILTER_GET_PIX_FMT	,	v4m_get_pix_fmt	},
	{	MS_FILTER_SET_VIDEO_SIZE, 	v4m_set_vsize	},
	{	MS_V4L_START			,	v4m_start	},
	{	MS_V4L_STOP			,	v4m_stop	},
	{	MS_FILTER_GET_VIDEO_SIZE,	v4m_get_vsize },
	{	0	,	NULL			}
};

MSFilterDesc ms_v4l_desc={
	.id=MS_V4L_ID,
	.name="MSV4m",
	.text=N_("A video for macosx compatible source filter to stream pictures."),
	.ninputs=0,
	.noutputs=1,
	.category=MS_FILTER_OTHER,
	.init=v4m_init,
	.preprocess=v4m_preprocess,
	.process=v4m_process,
	.postprocess=v4m_postprocess,
	.uninit=v4m_uninit,
	.methods=methods
};

MS_FILTER_DESC_EXPORT(ms_v4l_desc)

#endif
