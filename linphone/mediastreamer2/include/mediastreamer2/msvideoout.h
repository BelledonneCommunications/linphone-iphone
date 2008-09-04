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

#ifndef msvideoout_h
#define msvideoout_h

#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/msvideo.h>


struct _MSDisplay;

typedef enum _MSDisplayEventType{
	MS_DISPLAY_RESIZE_EVENT
}MSDisplayEventType;

typedef struct _MSDisplayEvent{
	MSDisplayEventType evtype;
	int w,h;
}MSDisplayEvent;

typedef struct _MSDisplayDesc{
	/*init requests setup of the display window at the proper size, given
	in frame_buffer argument. Memory buffer (data,strides) must be fulfilled
	at return. init() might be called several times upon screen resize*/
	bool_t (*init)(struct _MSDisplay *, MSPicture *frame_buffer);
	void (*lock)(struct _MSDisplay *);/*lock before writing to the framebuffer*/
	void (*unlock)(struct _MSDisplay *);/*unlock after writing to the framebuffer*/
	void (*update)(struct _MSDisplay *); /*display the picture to the screen*/
	void (*uninit)(struct _MSDisplay *);
	bool_t (*pollevent)(struct _MSDisplay *, MSDisplayEvent *ev);
}MSDisplayDesc;

typedef struct _MSDisplay{
	MSDisplayDesc *desc;
	long window_id; /*window id if the display should use an existing window*/
	void *data;
} MSDisplay;


#define ms_display_init(d,fbuf)	(d)->desc->init(d,fbuf)
#define ms_display_lock(d)	if ((d)->desc->lock) (d)->desc->lock(d)
#define ms_display_unlock(d)	if ((d)->desc->unlock) (d)->desc->unlock(d)
#define ms_display_update(d)	if ((d)->desc->update) (d)->desc->update(d)
bool_t ms_display_poll_event(MSDisplay *d, MSDisplayEvent *ev);

extern MSDisplayDesc ms_sdl_display_desc;

#if (defined(WIN32) || defined(_WIN32_WCE)) && !defined(MEDIASTREAMER_STATIC)
#ifdef MEDIASTREAMER2_EXPORTS
   #define MSVAR_DECLSPEC    __declspec(dllexport)
#else
   #define MSVAR_DECLSPEC    __declspec(dllimport)
#endif
#else
   #define MSVAR_DECLSPEC    extern
#endif

#ifdef __cplusplus
extern "C"{
#endif

MSVAR_DECLSPEC MSDisplayDesc ms_win_display_desc;

MSDisplay *ms_display_new(MSDisplayDesc *desc);
void ms_display_set_window_id(MSDisplay *d, long window_id);
void ms_display_destroy(MSDisplay *d);

#define MS_VIDEO_OUT_SET_DISPLAY MS_FILTER_METHOD(MS_VIDEO_OUT_ID,0,MSDisplay*)
#define MS_VIDEO_OUT_HANDLE_RESIZING MS_FILTER_METHOD_NO_ARG(MS_VIDEO_OUT_ID,1)
#define MS_VIDEO_OUT_SET_CORNER MS_FILTER_METHOD(MS_VIDEO_OUT_ID,2,int*)

#ifdef __cplusplus
}
#endif

#endif
