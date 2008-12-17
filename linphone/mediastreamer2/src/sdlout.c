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


#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msvideo.h"

#include <SDL/SDL.h>
#include <SDL/SDL_video.h>

typedef struct SdlOut
{
	MSVideoSize size;
	MSVideoSize local_size; /*size of local preview */
	MSPixFmt format;
	SDL_Surface *screen;
	SDL_Overlay *overlay;
	mblk_t *smallb;
	int scale_factor;
	bool_t lsize_init;
} SdlOut;


#define SCALE_FACTOR 6

static bool_t sdl_initialized=FALSE;

static void sdl_out_init(MSFilter  *f){
	SdlOut *obj=ms_new(SdlOut,1);
	obj->size.width = MS_VIDEO_SIZE_CIF_W;
	obj->size.height = MS_VIDEO_SIZE_CIF_H;
	obj->local_size.width = MS_VIDEO_SIZE_CIF_W;
	obj->local_size.height = MS_VIDEO_SIZE_CIF_H;
	obj->lsize_init=FALSE;
	obj->scale_factor=SCALE_FACTOR;
	obj->format=MS_RGB24;
	obj->screen=NULL;
	obj->overlay=NULL;
	obj->smallb=NULL;

#if !defined(WIN32) && !defined(__APPLE__)
	if (!sdl_initialized){

		/* Initialize the SDL library */
		if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
			ms_error("Couldn't initialize SDL: %s", SDL_GetError());
			return;
		}
		/* Clean up on exit */
		atexit(SDL_Quit);
		sdl_initialized=TRUE;
	}
#endif
	f->data=obj;
}

static void sdl_destroy_window(SdlOut *obj){
	if (obj->overlay!=NULL){
		SDL_FreeYUVOverlay(obj->overlay);
		obj->overlay=NULL;
	}
	if (obj->screen!=NULL){
		SDL_FreeSurface(obj->screen);
		obj->screen=NULL;
	}
}

static void sdl_out_uninit(MSFilter *f){
	SdlOut *s=(SdlOut*)f->data;
	sdl_destroy_window(s);
	if (s->smallb!=NULL) freemsg(s->smallb);
	ms_free(s);
}

static void sdl_create_window(SdlOut *obj){
	obj->screen = SDL_SetVideoMode(obj->size.width, obj->size.height, 0,SDL_SWSURFACE);
	if ( obj->screen == NULL ) {
		ms_warning("Couldn't set video mode: %s\n",
						SDL_GetError());
		return ;
	}
	if (obj->screen->flags & SDL_HWSURFACE) ms_message("SDL surface created in hardware");
	SDL_WM_SetCaption("Linphone Video", NULL);
	
	if (obj->format==MS_YUV420P){
		ms_message("Using yuv overlay.");
		obj->overlay=SDL_CreateYUVOverlay(obj->size.width,obj->size.height,SDL_YV12_OVERLAY,obj->screen);
		if (obj->overlay==NULL){
			ms_warning("Couldn't create yuv overlay: %s\n",
							SDL_GetError());
			return;
		}else{
			if (obj->overlay->hw_overlay) ms_message("YUV overlay using hardware acceleration.");
		}
	}
}

mblk_t * resize_yuv_small(unsigned char *pict, int w, int h, int scale){
	int i,j,id,jd;
	int nh,nw;
	unsigned char *smallpict;
	int ysize,usize,ydsize,udsize;
	int smallpict_sz;
	unsigned char *dptr,*sptr;
	mblk_t *smallb;
	nw=w/scale;
	nh=h/scale;
	ysize=w*h;
	usize=ysize/4;
	ydsize=nw*nh;
	udsize=ydsize/4;
	smallpict_sz=(ydsize*3)/2;
	smallb=allocb(smallpict_sz,0);
	smallpict=smallb->b_wptr;
	smallb->b_wptr+=smallpict_sz;
	
	dptr=smallpict;
	sptr=pict;
	for (j=0,jd=0;j<nh;j++,jd+=scale){
		for (i=0,id=0;i<nw;i++,id+=scale){
			dptr[(j*nw) + i]=sptr[(jd*w)+id];
		}
	}
	
	nh=nh/2;
	nw=nw/2;
	w=w/2;
	h=h/2;
	dptr+=ydsize;
	sptr+=ysize;
	for (j=0,jd=0;j<nh;j++,jd+=scale){
		for (i=0,id=0;i<nw;i++,id+=scale){
			dptr[(j*nw) + i]=sptr[(jd*w)+id];
		}
	}
	dptr+=udsize;
	sptr+=usize;
	for (j=0,jd=0;j<nh;j++,jd+=scale){
		for (i=0,id=0;i<nw;i++,id+=scale){
			dptr[(j*nw) + i]=sptr[(jd*w)+id];
		}
	}
	
	return smallb;
}

static void fill_overlay_at_pos(SDL_Overlay *lay, mblk_t *m, int x, int y, int w, int h){
	unsigned char *data=m->b_rptr;
	int i,j;
	int jlim,ilim;
	int off;
	unsigned char *dptr;
	
	ilim=MIN(x+w,lay->w);
	jlim=MIN(y+h,lay->h);
	SDL_LockYUVOverlay(lay);
	/* set Y */
	dptr=lay->pixels[0];
	for (j=y;j<jlim;j++){
		off=j*lay->w;
		for (i=x;i<ilim;i++){
			dptr[off + i]=*data;
			data++;
		}
	}
	/*set U and V*/
	ilim=ilim/2;
	jlim=jlim/2;
	dptr=lay->pixels[2];
	for (j=y/2;j<jlim;j++){
		off=j*(lay->w/2);
		for (i=x/2;i<ilim;i++){
			dptr[off + i]=*data;
			data++;
		}
	}
	dptr=lay->pixels[1];
	for (j=y/2;j<jlim;j++){
		off=j*(lay->w/2);
		for (i=x/2;i<ilim;i++){
			dptr[off + i]=*data;
			data++;
		}
	}
	SDL_UnlockYUVOverlay(lay);
}

static void fill_overlay(SDL_Overlay *lay,mblk_t *m){
	
	int w2,h2;
	char *data=(char*)m->b_rptr;
	int ysize=lay->pitches[0]*lay->h;
	int usize;
	w2=lay->w/2;
	h2=lay->h/2;
	usize=w2*h2;
	SDL_LockYUVOverlay(lay);
	memcpy(lay->pixels[0],data,ysize);
	memcpy(lay->pixels[2],data+ysize,usize);
	memcpy(lay->pixels[1],data+ysize+usize,usize);
	SDL_UnlockYUVOverlay(lay);
}

static void sdl_out_process(MSFilter *f){
	SdlOut *obj=(SdlOut*)f->data;
	mblk_t *inm0=NULL;
	mblk_t *inm1=NULL;
	int err;
	SDL_Rect smallrect;
	SDL_Rect rect;
	bool_t got_preview=FALSE;
	
#if defined(WIN32) || defined(__APPLE__)
	if (!sdl_initialized){

		/* Initialize the SDL library */
		if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
			ms_error("Couldn't initialize SDL: %s", SDL_GetError());
			return;
		}
		/* Clean up on exit */
		atexit(SDL_Quit);
		sdl_initialized=TRUE;
	}
#endif

	if (obj->screen==NULL){
		sdl_create_window(obj);
	}

	rect.w=obj->size.width;
	rect.h=obj->size.height;
	rect.x=0;
	rect.y=0;
	smallrect.w=obj->size.width/SCALE_FACTOR;
	smallrect.h=obj->size.height/SCALE_FACTOR;
	smallrect.x=obj->size.width - smallrect.w ;
	smallrect.y=obj->size.height -smallrect.h;
	
	
	while (f->inputs[0]!=NULL && (inm0=ms_queue_get(f->inputs[0]))!=NULL){
		SDL_Surface *surf;
		if  (obj->format==MS_YUV420P){
			fill_overlay(obj->overlay,inm0);
		}else {
			surf=SDL_CreateRGBSurfaceFrom(inm0->b_rptr,obj->size.width,obj->size.height,24,obj->size.width*3,0,0,0,0);

			err=SDL_BlitSurface(surf,NULL,obj->screen,NULL);
			if (err<0) ms_warning("Fail to blit surface: %s",SDL_GetError());
			SDL_FreeSurface(surf);
		}
		freemsg(inm0);
	}
	while (f->inputs[1]!=NULL && (inm1=ms_queue_get(f->inputs[1]))!=NULL){
		/* this message is blitted on the right,bottom corner of the screen */
		SDL_Surface *surf;
		got_preview=TRUE;
		if (!obj->lsize_init){
			/*attempt to guess the video size of the local preview buffer*/
			int bsize=msgdsize(inm1);
			if (bsize<(MS_VIDEO_SIZE_CIF_W*MS_VIDEO_SIZE_CIF_H*3/2)){
				/*surely qcif ?*/
				obj->local_size.width=MS_VIDEO_SIZE_QCIF_W;
				obj->local_size.height=MS_VIDEO_SIZE_QCIF_H;
				ms_message("preview is in QCIF.");
				obj->scale_factor=SCALE_FACTOR/2;
			}
			obj->lsize_init=TRUE;
		}
		if  (obj->format==MS_YUV420P){
			if (obj->smallb!=NULL) {
				freemsg(obj->smallb);
			}
			obj->smallb=resize_yuv_small(inm1->b_rptr,obj->local_size.width,obj->local_size.height,obj->scale_factor);
			fill_overlay_at_pos(obj->overlay,obj->smallb,smallrect.x, smallrect.y, smallrect.w, smallrect.h);
			freemsg(inm1);
		}else {
			surf=SDL_CreateRGBSurfaceFrom(inm1->b_rptr,obj->size.width,obj->size.height,24,obj->size.width*3,0,0,0,0);

			err=SDL_BlitSurface(surf,NULL,obj->screen,&smallrect);
			if (err<0) ms_warning("Fail to blit surface: %s",SDL_GetError());
			SDL_FreeSurface(surf);
		}
	}
	if (!got_preview){
		/* this is the case were we have only inm0, we have to redisplay inm1 */
		if  (obj->format==MS_YUV420P){
			if (obj->smallb!=NULL){
				fill_overlay_at_pos(obj->overlay,obj->smallb,smallrect.x, smallrect.y, smallrect.w, smallrect.h);
			}
		}
	}
	
	if (obj->format==MS_YUV420P) SDL_DisplayYUVOverlay(obj->overlay,&rect);
	else SDL_UpdateRect(obj->screen,0,0,obj->size.width,obj->size.height);
	
#if defined(WIN32) || defined(__APPLE__)
	{
		SDL_Event event;
		SDL_PollEvent(&event);
	}
#endif
}

static int sdl_out_set_pix_fmt(MSFilter *f,void *arg){
	SdlOut *s=(SdlOut*)f->data;
	s->format=*(MSPixFmt*)arg;
	return 0;
}

static int sdl_out_set_vsize(MSFilter *f,void *arg){
	SdlOut *s=(SdlOut*)f->data;
	s->size=*(MSVideoSize*)arg;
	s->local_size=*(MSVideoSize*)arg;
	return 0;
}

static MSFilterMethod methods[]={
	{	MS_FILTER_SET_PIX_FMT	,	sdl_out_set_pix_fmt},
	{	MS_FILTER_SET_VIDEO_SIZE	,	sdl_out_set_vsize },
	{	0	,NULL}
};

#ifdef _MSC_VER

MSFilterDesc ms_sdl_out_desc={
	MS_SDL_OUT_ID,
	"MSSdlOut",
	N_("A video display window using SDL"),
	MS_FILTER_OTHER,
	NULL,
	2,
	0,
	sdl_out_init,
	NULL,
	sdl_out_process,
	NULL,
	sdl_out_uninit,
	methods
};

#else

MSFilterDesc ms_sdl_out_desc={
	.id=MS_SDL_OUT_ID,
	.name="MSSdlOut",
	.text=N_("A video display window using SDL"),
	.category=MS_FILTER_OTHER,
	.ninputs=2,
	.noutputs=0,
	.init=sdl_out_init,
	.process=sdl_out_process,
	.uninit=sdl_out_uninit,
	.methods=methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_sdl_out_desc)
