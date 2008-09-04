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


#include "msvideooutput.h"
#include "msvideosource.h"
#include "affine.h"
#include "msavdecoder.h"
#include "msutils.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifndef XV_YV12
#define XV_YV12 0x32315659
#endif

#ifndef XV_YUY2
#define XV_YUY2 0x32595559
#endif

#ifndef XV_UYVY
#define XV_UYVY 0x59565955
#endif

#ifndef XV_I420
#define XV_I420 0x30323449
#endif

static MSVideoOutputClass *ms_video_output_class=NULL;


gboolean xv_init (MSVideoOutput *obj)
{
	gboolean got_port = FALSE;
	unsigned int	count;
	XvAdaptorInfo	*adaptor;
	
	obj->xv_shminfo.shmaddr = NULL;
    obj->xv_window = gdk_x11_drawable_get_xid(obj->window);
    obj->xv_display = (Display*) gdk_x11_drawable_get_xdisplay(obj->window);

   	if ( XvQueryAdaptors(obj->xv_display, obj->xv_window, &count, &adaptor) ==  Success )
	{
		unsigned int n, i;
		
        for (n = 0; !got_port && n < count; ++n)
		{
            for ( obj->xv_port = adaptor[n].base_id; 
			  	  obj->xv_port < adaptor[n].base_id + adaptor[n].num_ports; 
			  	  obj->xv_port++ )
			{
                if (XvGrabPort(obj->xv_display, obj->xv_port, CurrentTime) == 0)
				{
					int formats; 
					XvImageFormatValues  *list;

					list = XvListImageFormats( obj->xv_display, obj->xv_port, &formats);
					for ( i = 0; i < formats; i ++ ) {
						if ( list[i].id == XV_I420 && !got_port )
							got_port = TRUE;
					}
					if ( got_port )
						break;
					else
						XvUngrabPort( obj->xv_display, obj->xv_port, CurrentTime );
                }
        	}
		}
    	if ( got_port )
		{
        	obj->xv_gc = XCreateGC(obj->xv_display, obj->xv_window, 0, &obj->xv_values);

    		obj->xv_image = (XvImage *) XvShmCreateImage( obj->xv_display, obj->xv_port, XV_I420, 0, obj->width, obj->height, &obj->xv_shminfo);
			if (obj->xv_image == NULL) {
				g_message("Unable to allocate XvImage, falling back to GDK output");
				XvUngrabPort( obj->xv_display, obj->xv_port, CurrentTime );
				obj->xv_port = 0;
				return FALSE;
			}
			else {
				g_message("allocated XvImage with size %i", obj->xv_image->data_size);
			}
	
    		obj->xv_shminfo.shmid = shmget( IPC_PRIVATE, obj->xv_image->data_size, IPC_CREAT | 0777);
    		obj->xv_shminfo.shmaddr = (char *) shmat( obj->xv_shminfo.shmid, 0, 0);
    		obj->xv_image->data = obj->xv_shminfo.shmaddr;
    		obj->xv_shminfo.readOnly = 0;
    		if (!XShmAttach( (Display*) gdk_x11_get_default_xdisplay(), &obj->xv_shminfo)) {
				got_port = FALSE;
    		}
        	shmctl(obj->xv_shminfo.shmid, IPC_RMID, 0);
		}
		else obj->xv_port = 0;
    }
	else {
		got_port = FALSE;
		obj->xv_port = 0;
	}
	return got_port;
}


void xv_uninit(MSVideoOutput *obj)
{
	if ( obj->xv_port ) {
		XvUngrabPort( obj->xv_display, obj->xv_port, CurrentTime );
	}

    if (obj->xv_image != NULL)
        XvStopVideo(obj->xv_display, obj->xv_port, obj->xv_window);

    if (obj->xv_shminfo.shmaddr != NULL) {
        XShmDetach(obj->xv_display, &obj->xv_shminfo);
        shmctl(obj->xv_shminfo.shmid, IPC_RMID, 0);
        shmdt(obj->xv_shminfo.shmaddr);
    }
    if (obj->xv_image != NULL)
        XFree(obj->xv_image);
}

void ms_video_output_init(MSVideoOutput *obj)
{
	gint error;
	GdkWindowAttr attr;
	MSVideoOutputClass *klass=MS_VIDEO_OUTPUT_CLASS(MS_FILTER(obj)->klass);
	memset(&attr,0,sizeof(attr));
	attr.title="linphone video";
	attr.window_type=GDK_WINDOW_CHILD;
	attr.wclass=GDK_INPUT_OUTPUT;
	attr.x=0;
	attr.y=0;
	attr.width = VIDEO_SIZE_CIF_W;
	attr.height = VIDEO_SIZE_CIF_H;
	attr.visual=klass->visual;
	attr.colormap=klass->colormap;
	attr.override_redirect = TRUE;
	ms_filter_init(MS_FILTER(obj));
	MS_FILTER(obj)->inqueues=obj->input;
	obj->window=gdk_window_new(NULL,&attr,GDK_WA_TITLE|GDK_WA_X|GDK_WA_Y|GDK_WA_COLORMAP|GDK_WA_VISUAL);
	if (obj->window==NULL)
	{
		g_error("Could not create gdk video window");
	}
	obj->gc=gdk_gc_new(obj->window);
	gdk_window_show(obj->window);
	gdk_flush();
	//gdk_window_withdraw(obj->window);
	obj->width = VIDEO_SIZE_CIF_W;
	obj->height = VIDEO_SIZE_CIF_H;
	obj->prev_h=0;
	obj->prev_w=0;

	obj->bpp = 3/2;
	obj->bufsize=obj->width*obj->height*obj->bpp;
	obj->palette = "YUV420P";
	obj->active=TRUE;
}

void ms_video_output_setup(MSVideoOutput *vo, MSSync *sync)
{
#if 0
	/* tries to find the video source of the stream */
	MSFilter *vs;
	vs=ms_filter_search_upstream_by_type(MS_FILTER(vo),MS_FILTER_VIDEO_IO);
	if (vs != NULL) {
		/* get video source properties */
		vo->width=MS_VIDEO_SOURCE(vs)->width;
		vo->height=MS_VIDEO_SOURCE(vs)->height;
		vo->palette=MS_VIDEO_SOURCE(vs)->format;
		ms_video_output_set_size(vo,vo->width,vo->height);
	} else
		g_warning("ms_video_output_setup: could not find the video source.");

	if (xv_init(vo) == FALSE && vo->palette!=NULL && strcmp(vo->palette, "RGB24") != 0) {
		/* tell our upstream codec to use RGB too! */
		vs=ms_filter_search_upstream_by_type(MS_FILTER(vo),MS_FILTER_VIDEO_CODEC);
		if (vs != NULL) {
			ms_trace("found upstream codec");
			vo->palette = "RGB24";
			vo->bpp = 3;
			vo->bufsize = vo->width*vo->height*vo->bpp;
			ms_AVdecoder_set_format(MS_AVDECODER(vs), "RGB24");
		}else{
			g_warning("ms_video_output_setup: could not find the video codec.");
		}
	}
#endif
}


void ms_video_output_class_init(MSVideoOutputClass *klass)
{
	gint status;
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	MS_FILTER_CLASS(klass)->max_qinputs=2;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_video_output_destroy;
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_video_output_process;
	MS_FILTER_CLASS(klass)->setup=(MSFilterSetupFunc)ms_video_output_setup;
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"MSVideoOutput");
	status=gdk_init_check(0,NULL);
	if (status==0){
		g_error("Failed to initialize gdk.");
	}
	gdk_rgb_init();
	gdk_rgb_set_verbose(1);
	klass->visual=gdk_rgb_get_visual();
	klass->colormap=gdk_rgb_get_cmap();
}

void ms_video_output_uninit(MSVideoOutput *obj)
{
	xv_uninit(obj);
	gdk_gc_destroy(obj->gc);
	gdk_window_destroy(obj->window);
	gdk_flush();
}

MSFilter * ms_video_output_new()
{
	MSVideoOutput *obj=g_malloc0(sizeof(MSVideoOutput));
	
	if (ms_video_output_class==NULL)
	{
		ms_video_output_class=g_malloc0(sizeof(MSVideoOutputClass));
		ms_video_output_class_init(ms_video_output_class);
	}
	MS_FILTER(obj)->klass=MS_FILTER_CLASS(ms_video_output_class);
	ms_video_output_init(obj);
	return MS_FILTER(obj);
}

void ms_video_output_start(MSVideoOutput *obj)
{
	ms_filter_lock(MS_FILTER(obj));
	obj->active=TRUE;
	ms_filter_unlock(MS_FILTER(obj));
}

void ms_video_output_stop(MSVideoOutput *obj)
{
	ms_filter_lock(MS_FILTER(obj));
	obj->active=FALSE;
	ms_filter_unlock(MS_FILTER(obj));
}

void ms_video_output_set_title(MSVideoOutput *obj,gchar *title)
{
	g_return_if_fail(obj->window);
	gdk_threads_enter();
	gdk_window_set_title(obj->window,title);
	gdk_flush();
	gdk_threads_leave();
}

void ms_video_output_set_size(MSVideoOutput *obj,gint width, gint height)
{
	gdk_threads_enter();
	obj->width=width;
	obj->height=height;
	obj->bufsize=width*height*obj->bpp;
	gdk_window_resize(obj->window,width,height);
	gdk_flush();
	gdk_threads_leave();
}

void ms_video_output_set_format(MSVideoOutput *obj, const char *fmt){
	obj->palette=fmt;
}

static inline
void composite( guchar *dest, gint d_width, gint d_height, gint d_x, gint d_y, guchar *src, gint s_width, gint s_height, gint bpp)
{
	register gint i;
	register gint s_stride = s_width*bpp;
	register gint d_stride = d_width*bpp;
	register guchar *s = src, *d = dest;
	d += (d_y * d_stride) + (d_x * bpp);
	for (i = 0; i < s_height; i++, s += s_stride, d += d_stride)
		memcpy( d, s, s_stride);
}

static inline void ntohl_block(guint32  *buf, int len)
{
    register int i;
    for (i=0; i<len; i++) {
        buf[i] = ntohl(buf[i]);
    }
}



#define YUV2RGB(y, u, v, r, g, b)\
  r = y + ((v*1436) >>10);\
  g = y - ((u*352 + v*731) >> 10);\
  b = y + ((u*1814) >> 10);\
  r = r < 0 ? 0 : r;\
  g = g < 0 ? 0 : g;\
  b = b < 0 ? 0 : b;\
  r = r > 255 ? 255 : r;\
  g = g > 255 ? 255 : g;\
  b = b > 255 ? 255 : b


static inline
void yv12_to_rgb24 (unsigned char *src, unsigned char *dest, int width, int height)
{
  register int i,j;
  register int y0, y1, u, v;
  register int r, g, b;
  register unsigned char *s[3];
  s[0] = src;
  s[1] = s[0] + width*height;
  s[2] = s[1] + width*height/4;

  for (i = 0; i < height; i++) {
	for (j = 0; j < width/2; j++) {
      y0 = *(s[0])++;
      y1 = *(s[0])++;
      if (i % 2 == 0 ) {
        u  = *(s[1])++ - 128;
        v  = *(s[2])++ - 128;
      }
      YUV2RGB (y0, u, v, r, g, b);
      *dest++ = r;
      *dest++ = g;
      *dest++ = b;
      YUV2RGB (y1, u, v, r, g, b);
      *dest++ = r;
      *dest++ = g;
      *dest++ = b;
    }
  }
}


#define PIP_FACTOR 5.0

void ms_video_output_process(MSVideoOutput *obj)
{
	MSQueue *q=obj->input[0];
	MSMessage *m;
	GdkPixbuf *pb_pip = NULL;
	guchar buf[VIDEO_SIZE_MAX_W*VIDEO_SIZE_MAX_H*3], buf2[VIDEO_SIZE_MAX_W*VIDEO_SIZE_MAX_H*3];

	ms_filter_lock(MS_FILTER(obj));
	if (obj->active==FALSE){
		ms_filter_unlock(MS_FILTER(obj));
		while((m=ms_queue_get(q))!=NULL) ms_message_destroy(m);
		return;
	}
	
	while((m=ms_queue_get(q))!=NULL)
	{
		ms_trace("Getting new buffer");
		if (m->size >= obj->bufsize)
		{
			gint w=VIDEO_SIZE_CIF_W, h=VIDEO_SIZE_CIF_H;
			
			gdk_threads_enter();
			gdk_window_get_geometry(obj->window, NULL, NULL, &w, &h, NULL);
			if (w != obj->prev_w || h != obj->prev_h) {
				gdk_window_resize(obj->window,w,h);
				gdk_window_clear(obj->window);
				obj->prev_w = w;
				obj->prev_h = h;
			}
			gdk_flush();
			if (obj->xv_port != 0 && strcmp(obj->palette, "YUV420P") == 0) {
				int imageWidth = obj->width * obj->width / VIDEO_SIZE_MAX_W;
				int imageHeight = obj->height * obj->height / VIDEO_SIZE_MAX_H;
				double ratioWidth = (double)w / (double)imageWidth;
				double ratioHeight = (double)h / (double)imageHeight;
				int width, height, x, y;
				if (ratioHeight < ratioWidth) {
					width = (int)( imageWidth * ratioHeight );
					height = (int)( imageHeight * ratioHeight );
				} else {
					width = (int)( imageWidth * ratioWidth );
					height = (int)( imageHeight * ratioWidth );
				}
				x = ( w - width ) / 2;
				y = ( h - height ) / 2;
	
				memcpy( obj->xv_image->data, m->data, obj->xv_image->data_size );
				if (obj->input[1] != NULL) {
					MSMessage *m = ms_queue_get(obj->input[1]);
					
					if (m != NULL) {
						/* CAUTION: this is very tricky planar scaling and compositing! */
						affine_scale((const unsigned char *)m->data, buf, 
							obj->width, obj->height, obj->width/PIP_FACTOR, obj->height/PIP_FACTOR, 1);
						
						affine_scale( (const unsigned char *) m->data + obj->width * obj->height, 
							buf + (int)(obj->width/PIP_FACTOR * obj->height/PIP_FACTOR), 
							obj->width/2, obj->height/2, obj->width/PIP_FACTOR/2, obj->height/PIP_FACTOR/2, 1);
						
						affine_scale( (const unsigned char *) m->data + (int)(obj->width * obj->height * 5/4),
							buf + (int)(obj->width/PIP_FACTOR * obj->height/PIP_FACTOR * 5/4), 
							obj->width/2, obj->height/2, obj->width/PIP_FACTOR/2, obj->height/PIP_FACTOR/2, 1);
						
						ms_message_destroy(m);
						
						composite( obj->xv_image->data, obj->width, obj->height, obj->width-obj->width/PIP_FACTOR-6, 6, 
							buf, obj->width/PIP_FACTOR, obj->height/PIP_FACTOR, 1);
						
						composite( obj->xv_image->data + (int)(obj->width * obj->height),
							obj->width/2, obj->height/2, obj->width/2 - obj->width/PIP_FACTOR/2 - 3, 3, 
							buf + (int)(obj->width/PIP_FACTOR * obj->height/PIP_FACTOR),
							obj->width/PIP_FACTOR/2, obj->height/PIP_FACTOR/2, 1);
						
						composite( obj->xv_image->data + (int)(obj->width * obj->height * 5/4),
							obj->width/2, obj->height/2, obj->width/2 - obj->width/PIP_FACTOR/2 - 3, 3, 
							buf + (int)(obj->width/PIP_FACTOR * obj->height/PIP_FACTOR * 5/4),
							obj->width/PIP_FACTOR/2, obj->height/PIP_FACTOR/2, 1);
					}
				}
				XvShmPutImage(obj->xv_display, obj->xv_port, obj->xv_window, obj->xv_gc, obj->xv_image,
					0, 0, obj->width, obj->height,
					x, y, width, height, FALSE );
				XFlush(obj->xv_display);
			}
			else
			{
				GdkPixbuf *pb, *pb_scaled;
				
				if (strcmp(obj->palette, "RGB24") == 0) {
					pb = gdk_pixbuf_new_from_data( m->data, GDK_COLORSPACE_RGB,
						FALSE, 8, obj->width, obj->height, obj->width*3, NULL, NULL);
				} else {
					/* convert the YUV420P image to RGB24 ourselves */
					yv12_to_rgb24(m->data, buf2, obj->width, obj->height);
					pb = gdk_pixbuf_new_from_data(buf2, GDK_COLORSPACE_RGB,
						FALSE, 8, obj->width, obj->height, obj->width*3, NULL, NULL);
				}
				
				pb_scaled = gdk_pixbuf_scale_simple(pb, w, h, GDK_INTERP_BILINEAR);
				if (obj->input[1] != NULL) {
					MSMessage *m = ms_queue_get(obj->input[1]);
					
					if (m != NULL) {
						GdkPixbuf *pb;
						/* XXX: assumes the pip video source is always YV12 */
						yv12_to_rgb24(m->data, buf, obj->width, obj->height);
						
						 pb = gdk_pixbuf_new_from_data( buf, GDK_COLORSPACE_RGB,
							FALSE, 8, obj->width, obj->height, obj->width*3, NULL, NULL);
						if (pb_pip != NULL)
							gdk_pixbuf_unref(pb_pip);
						pb_pip = gdk_pixbuf_scale_simple(pb, w/PIP_FACTOR, h/PIP_FACTOR, GDK_INTERP_BILINEAR);
						ms_message_destroy(m);
						gdk_pixbuf_unref(pb);
					}
					if (pb_pip != NULL) {
						gdk_pixbuf_composite(pb_pip, pb_scaled,
							w-w/PIP_FACTOR-5, 5, w/PIP_FACTOR, h/PIP_FACTOR,
							w-w/PIP_FACTOR-5, 5, 1.0, 1.0,
							GDK_INTERP_BILINEAR, 255);
					}
				}
				gdk_pixbuf_render_to_drawable(pb_scaled, obj->window, obj->gc, 0, 0, 0, 0,
					w, h, GDK_RGB_DITHER_NONE, 0, 0);
				
				gdk_flush();
				gdk_pixbuf_unref(pb);
				gdk_pixbuf_unref(pb_scaled);
			}
			gdk_threads_leave();
		}
		else g_warning("Image is too small for current window");
		ms_message_destroy(m);
	}
	ms_filter_unlock(MS_FILTER(obj));
}

void ms_video_output_destroy(MSVideoOutput *obj)
{
	ms_video_output_uninit(obj);
	g_free(obj);
}
