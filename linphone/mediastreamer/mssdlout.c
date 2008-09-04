/***************************************************************************
 *            mssdlout.c
 *
 *  Mon Jul 11 16:17:59 2005
 *  Copyright  2005  Simon Morlat
 *  Email simon dot morlat at linphone dot org
 ****************************************************************************/

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

#include "mssdlout.h"

MSSdlOutClass *ms_sdl_out_class=NULL;

void ms_sdl_out_init(MSSdlOut *obj){
	ms_filter_init(MS_FILTER(obj));
	obj->width=VIDEO_SIZE_CIF_W;
	obj->height=VIDEO_SIZE_CIF_H;
	obj->format="RGB24";
	obj->use_yuv=FALSE;
	obj->oldinm1=NULL;
	MS_FILTER(obj)->inqueues=obj->input;
}

void ms_sdl_out_set_format(MSSdlOut *obj, const char *fmt){
	obj->format=fmt;
	if (strcmp(fmt,"YUV420P")==0) obj->use_yuv=TRUE;
	else obj->use_yuv=FALSE;
}

void ms_sdl_uninit_sdl(MSSdlOut *obj){
	if (obj->overlay!=NULL){
		SDL_FreeYUVOverlay(obj->overlay);
		obj->overlay=NULL;
	}
	if (obj->screen!=NULL){
		SDL_FreeSurface(obj->screen);
		obj->screen=NULL;
	}
	
}

void ms_sdl_out_uninit(MSSdlOut *obj){
	ms_sdl_uninit_sdl(obj);
}

void ms_sdl_out_destroy(MSSdlOut *obj){
	ms_sdl_out_uninit(obj);
	if (obj->oldinm1!=NULL) ms_message_destroy(obj->oldinm1);
	g_free(obj);
}

void ms_sdl_init_sdl(MSSdlOut *obj){
	if (strcmp(obj->format,"RGB24")==0){
	}else{
		obj->use_yuv=TRUE;
	}
	obj->screen = SDL_SetVideoMode(obj->width, obj->height, 0,SDL_HWSURFACE|SDL_ANYFORMAT);
	if ( obj->screen == NULL ) {
		g_warning("Couldn't set video mode: %s\n",
						SDL_GetError());
		return ;
	}
	if (obj->screen->flags & SDL_HWSURFACE) g_message("SDL surface created in hardware");
	SDL_WM_SetCaption("Linphone Video", NULL);
	
	if (obj->use_yuv){
		g_message("Using yuv overlay.");
		obj->overlay=SDL_CreateYUVOverlay(obj->width,obj->height,SDL_IYUV_OVERLAY,obj->screen);
		if (obj->overlay==NULL){
			g_warning("Couldn't create yuv overlay: %s\n",
							SDL_GetError());
		}else{
			if (obj->overlay->hw_overlay) g_message("YUV overlay using hardware acceleration.");
		}
	}
	
}

static void resize_yuv_small(char *pict, int w, int h, int scale){
	int i,j,id,jd;
	int nh,nw;
	char *smallpict;
	int ysize,usize,ydsize,udsize;
	int smallpict_sz;
	char *dptr,*sptr;
	nw=w/scale;
	nh=h/scale;
	ysize=w*h;
	usize=ysize/4;
	ydsize=nw*nh;
	udsize=ydsize/4;
	smallpict_sz=(ydsize*3)/2;
	smallpict=(char*)alloca(smallpict_sz);
	memset(smallpict,0,smallpict_sz);
	
	
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
	
	memcpy(pict,smallpict,smallpict_sz);
}

static void fill_overlay_at_pos(SDL_Overlay *lay, MSMessage *m, int x, int y, int w, int h){
	char *data=(char*)m->data;
	int i,j;
	int jlim,ilim;
	int off;
	char *dptr;
	
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
	dptr=lay->pixels[1];
	for (j=y/2;j<jlim;j++){
		off=j*(lay->w/2);
		for (i=x/2;i<ilim;i++){
			dptr[off + i]=*data;
			data++;
		}
	}
	dptr=lay->pixels[2];
	for (j=y/2;j<jlim;j++){
		off=j*(lay->w/2);
		for (i=x/2;i<ilim;i++){
			dptr[off + i]=*data;
			data++;
		}
	}
	SDL_UnlockYUVOverlay(lay);
}

static void fill_overlay(SDL_Overlay *lay, MSMessage *m){
	
	int w2,h2;
	char *data=(char*)m->data;
	int ysize=lay->w*lay->h;
	int usize;
	w2=lay->w/2;
	h2=lay->h/2;
	usize=w2*h2;
	SDL_LockYUVOverlay(lay);
	memcpy(lay->pixels[0],data,ysize);
	memcpy(lay->pixels[1],data+ysize,usize);
	memcpy(lay->pixels[2],data+ysize+usize,usize);
	SDL_UnlockYUVOverlay(lay);
}

#define SCALE_FACTOR 6

void ms_sdl_out_process(MSSdlOut *obj){
	MSQueue *q0=obj->input[0];
	MSQueue *q1=obj->input[1];
	MSMessage *inm0=NULL;
	MSMessage *inm1=NULL;
	int err;
	SDL_Rect smallrect;
	SDL_Rect rect;
	rect.w=obj->width;
	rect.h=obj->height;
	rect.x=0;
	rect.y=0;
	smallrect.w=obj->width/SCALE_FACTOR;
	smallrect.h=obj->height/SCALE_FACTOR;
	smallrect.x=obj->width - smallrect.w ;
	smallrect.y=obj->height -smallrect.h;
	
	if (obj->screen==NULL){
		ms_sdl_init_sdl(obj);
	}
	
	if (q0!=NULL)
		inm0=ms_queue_get(q0);
	if (q1!=NULL)
		inm1=ms_queue_get(q1);
	
	if (inm0!=NULL){
		SDL_Surface *surf;
		if (obj->use_yuv){
			
			fill_overlay(obj->overlay,inm0);
			
		}else {
			surf=SDL_CreateRGBSurfaceFrom(inm0->data,obj->width,obj->height,24,obj->width*3,0,0,0,0);

			err=SDL_BlitSurface(surf,NULL,obj->screen,NULL);
			if (err<0) g_warning("Fail to blit surface: %s",SDL_GetError());
			SDL_FreeSurface(surf);
		}
		ms_message_destroy(inm0);
	}
	if (inm1!=NULL){
		/* this message is blitted on the right,bottom corner of the screen */
		SDL_Surface *surf;
		
		if (obj->use_yuv){
			resize_yuv_small(inm1->data,rect.w,rect.h,SCALE_FACTOR);
			fill_overlay_at_pos(obj->overlay,inm1,smallrect.x, smallrect.y, smallrect.w, smallrect.h);
		}else {
			surf=SDL_CreateRGBSurfaceFrom(inm1->data,obj->width,obj->height,24,obj->width*3,0,0,0,0);

			err=SDL_BlitSurface(surf,NULL,obj->screen,&smallrect);
			if (err<0) g_warning("Fail to blit surface: %s",SDL_GetError());
			SDL_FreeSurface(surf);
		}
		if (obj->oldinm1!=NULL) {
			ms_message_destroy(obj->oldinm1);
		}
		obj->oldinm1=inm1;
		
	}else{
		/* this is the case were we have only inm0, we have to redisplay inm1 */
		if (obj->use_yuv){
			if (obj->oldinm1!=NULL){
				fill_overlay_at_pos(obj->overlay,obj->oldinm1,smallrect.x, smallrect.y, smallrect.w, smallrect.h);
			}
		}
	}
	
	if (obj->use_yuv) SDL_DisplayYUVOverlay(obj->overlay,&rect);
	SDL_UpdateRect(obj->screen,0,0,obj->width,obj->height);
	
}

void ms_sdl_out_class_init(MSSdlOutClass *klass){
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_sdl_out_process;
	MS_FILTER_CLASS(klass)->max_qinputs=2;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_sdl_out_destroy;
	MS_FILTER_CLASS(klass)->name="MSSdlOut";
	/* Initialize the SDL library */
    if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
        fprintf(stderr,
                "Couldn't initialize SDL: %s\n", SDL_GetError());
		return;
    }
	/* Clean up on exit */
    atexit(SDL_Quit);
}

MSFilter * ms_sdl_out_new(void){
	MSSdlOut *obj=g_new0(MSSdlOut,1);
	if (ms_sdl_out_class==NULL){
		ms_sdl_out_class=g_new0(MSSdlOutClass,1);
		ms_sdl_out_class_init(ms_sdl_out_class);
	}
	MS_FILTER(obj)->klass=MS_FILTER_CLASS(ms_sdl_out_class);
	ms_sdl_out_init(obj);
	return MS_FILTER(obj);
}
