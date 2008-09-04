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

#include "msxine.h"
#include "stdlib.h"


static MSXineClass *ms_xine_class=NULL;

static void change_window_size(void *user_data,
					 int video_width, int video_height,
					 int *dest_x, int *dest_y,
					 int *dest_height, int *dest_width)
{
	MSXine *obj=(MSXine*)user_data;
	XLockDisplay(obj->vis.display);
	g_message("entering change window size!!!!");
	g_message("video_width=%i, video_height=%i",video_width,video_height);
	sleep(1);
	fflush(NULL);
	XUnlockDisplay(obj->vis.display);
	
}

void calc_dest_size (void *this,
				  int video_width, int video_height,
				  int *dest_width, int *dest_height)  {
	g_message("entering video_window_calc_dest_size() !");
	*dest_width=video_width;
	*dest_height=video_height;
}
void ms_xine_init(MSXine *obj)
{
	MSXineClass *klass=MS_XINE_CLASS(MS_FILTER(obj)->klass);
	double res_h,res_v;
	XSetWindowAttributes att;
	ao_driver_t *ao_driver;
	XGCValues             xgcv;

	
	ms_filter_init(MS_FILTER(obj));
	MS_FILTER(obj)->inqueues=obj->input;
	memset(&obj->vis,0,sizeof(x11_visual_t));
	/* create an X11 window */
	obj->vis.display=XOpenDisplay(NULL);
	obj->vis.screen=DefaultScreen(obj->vis.display);
	
	res_h                 = (DisplayWidth  (obj->vis.display, obj->vis.screen)*1000 
			   / DisplayWidthMM (obj->vis.display, obj->vis.screen));
	res_v                 = (DisplayHeight (obj->vis.display, obj->vis.screen)*1000
			   / DisplayHeightMM (obj->vis.display, obj->vis.screen));
	obj->vis.display_ratio     = res_h / res_v;
	if (fabs(obj->vis.display_ratio - 1.0) < 0.01) {
    /*
     * we have a display with *almost* square pixels (<1% error),
     * to avoid time consuming software scaling in video_out_xshm,
     * correct this to the exact value of 1.0 and pretend we have
     * perfect square pixels.
     */
		obj->vis.display_ratio   = 1.0;
	}
	
	att.colormap=DefaultColormap(obj->vis.display,obj->vis.screen);
	att.background_pixel  = BlackPixel(obj->vis.display,obj->vis.screen);
    att.border_pixel      = BlackPixel(obj->vis.display,obj->vis.screen);
	obj->vis.d=XCreateWindow(obj->vis.display,RootWindow(obj->vis.display,obj->vis.screen),0,0,1000,800,0,
						DefaultDepth(obj->vis.display,obj->vis.screen),CopyFromParent,
						DefaultVisual(obj->vis.display,obj->vis.screen),
						CWColormap | CWBackPixel  | CWBorderPixel,
						&att);
	XMapWindow(obj->vis.display,obj->vis.d);
	XSync(obj->vis.display,False);
	XSetStandardProperties(obj->vis.display, obj->vis.d, 
 			   "Linphone video display", "Linphone video display", None, NULL, 0, 0);
	XCreateGC(obj->vis.display, obj->vis.d, 0L, &xgcv);
	
	//XMapRaised(obj->vis.display,obj->vis.d);
	//sleep(1);
	//XFlush(obj->vis.display);
	
	/* select the first available output plugin type*/
	obj->vo_driver=xine_load_video_output_plugin(klass->config,
							klass->video_plugins[0],
							VISUAL_TYPE_X11,
							&obj->vis);
	if (obj->vo_driver==NULL){
		g_error("Could not load a xine output plugin.");
	}
	else g_message("New vo driver %x created.",obj->vo_driver);
	ao_driver=xine_load_audio_output_plugin(klass->config,"esd");
	if (ao_driver==NULL) g_message("Could not load audio output plugin");
		
	/* set some callbacks */
	obj->vis.user_data=(void*)obj;
	obj->vis.request_dest_size=change_window_size;
	obj->vis.calc_dest_size=calc_dest_size;
	
	/* initializing a xine engine*/
	obj->engine=xine_init(obj->vo_driver,ao_driver,klass->config);
	if (obj->engine==NULL){
		g_error("Could not create a new xine engine");
	}
	
	obj->vo_driver->gui_data_exchange (obj->vo_driver,
					  GUI_DATA_EX_DRAWABLE_CHANGED, 
					  (void*)obj->vis.d);
}

void ms_xine_class_init(MSXineClass *klass)
{
	int i;
	
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	MS_FILTER_CLASS(klass)->max_qinputs=1;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_xine_destroy;
	/* read xine config file */
	klass->config=config_file_init(NULL);
	if (klass->config==NULL){
		g_error("Could not read xine config file");
	}
	/* list available video output plugins */
	klass->video_plugins=xine_list_video_output_plugins(VISUAL_TYPE_X11);
	g_message("Xine video plugins for X11 are:");
	i=0;
	while (klass->video_plugins[i]!=NULL)
	{
		g_message("\t- %s",klass->video_plugins[i]);
		i++;
	}
	
}

MSFilter * ms_xine_new()
{
	MSXine *obj=g_malloc(sizeof(MSXine));
	
	if (ms_xine_class==NULL)
	{
		ms_xine_class=g_malloc(sizeof(MSXineClass));
		ms_xine_class_init(ms_xine_class);
	}
	MS_FILTER(obj)->klass=MS_FILTER_CLASS(ms_xine_class);
	ms_xine_init(obj);
	return MS_FILTER(obj);
}

void ms_xine_start(MSXine *obj)
{
	xine_play(obj->engine,
	"file://cdrom/Videos/(smr)shrek-ts(1of2).avi",
	0,0);
}

void ms_xine_uninit(MSXine *obj)
{
	xine_exit(obj->engine);
	free(obj->vo_driver);
	XDestroyWindow(obj->vis.display,obj->vis.d);
}

void ms_xine_stop(MSXine *obj)
{
	xine_stop(obj->engine);
}


void ms_xine_destroy(MSXine *obj)
{
	ms_xine_uninit(obj);
	g_free(obj);
}

