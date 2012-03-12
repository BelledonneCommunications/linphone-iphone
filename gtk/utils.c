/*
linphone, gtk-glade interface.
Copyright (C) 2008  Simon MORLAT (simon.morlat@linphone.org)

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

#include "linphone.h"

static void run_gtk(){
	while (gtk_events_pending ())
		gtk_main_iteration ();

}

void *linphone_gtk_wait(LinphoneCore *lc, void *ctx, LinphoneWaitingState ws, const char *purpose, float progress){
	GtkWidget *w;
	switch(ws){
		case LinphoneWaitingStart:
			gdk_threads_enter();
			w=linphone_gtk_create_window("waiting");
			gtk_window_set_transient_for(GTK_WINDOW(w),GTK_WINDOW(linphone_gtk_get_main_window()));
			gtk_window_set_position(GTK_WINDOW(w),GTK_WIN_POS_CENTER_ON_PARENT);
			if (purpose) {
				gtk_progress_bar_set_text(
					GTK_PROGRESS_BAR(linphone_gtk_get_widget(w,"progressbar")),
					purpose);
			}
			gtk_widget_show(w);
			/*g_message("Creating waiting window");*/
			run_gtk();
			gdk_threads_leave();
			return w;
		break;
		case LinphoneWaitingProgress:
			w=(GtkWidget*)ctx;
			gdk_threads_enter();
			if (progress>=0){
				gtk_progress_bar_set_fraction(
					GTK_PROGRESS_BAR(linphone_gtk_get_widget(w,"progressbar")),
					progress);
				
				
			}else {
				gtk_progress_bar_pulse(
					GTK_PROGRESS_BAR(linphone_gtk_get_widget(w,"progressbar"))
				);
			}
			/*g_message("Updating progress");*/
			run_gtk();
			gdk_threads_leave();
			g_usleep(50000);
			return w;
		break;
		case LinphoneWaitingFinished:
			w=(GtkWidget*)ctx;
			gdk_threads_enter();
			gtk_widget_destroy(w);
			run_gtk();
			gdk_threads_leave();
			return NULL;
		break;
	}
	return NULL;
}

GdkPixbuf *_gdk_pixbuf_new_from_memory_at_scale(const void *data, gint len, gint w, gint h, gboolean preserve_ratio){
	GInputStream *stream=g_memory_input_stream_new_from_data (data,len,NULL);
	GError *error=NULL;
	
	GdkPixbuf *pbuf=gdk_pixbuf_new_from_stream_at_scale (stream,w,h,preserve_ratio,NULL,&error);
	g_input_stream_close(stream,NULL,NULL);
	g_object_unref(G_OBJECT(stream));
	if (pbuf==NULL){
		g_warning("Could not open image from memory");
	}
	return pbuf;
}

GtkWidget * _gtk_image_new_from_memory_at_scale(const void *data, gint len, gint w, gint h, gboolean preserve_ratio){
	GtkWidget *image;
	GdkPixbuf *pbuf=_gdk_pixbuf_new_from_memory_at_scale(data,len,w,h,preserve_ratio);
	if (pbuf==NULL) return NULL;
	image=gtk_image_new_from_pixbuf(pbuf);
	g_object_unref(G_OBJECT(pbuf));
	return image;
}

void linphone_gtk_reload_sound_devices(void){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *pb=(GtkWidget*)g_object_get_data(G_OBJECT(mw),"parameters");
	linphone_core_reload_sound_devices(linphone_gtk_get_core());
	linphone_gtk_fill_soundcards(pb);
}

void linphone_gtk_reload_video_devices(void){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *pb=(GtkWidget*)g_object_get_data(G_OBJECT(mw),"parameters");
	linphone_core_reload_video_devices(linphone_gtk_get_core());
	linphone_gtk_fill_webcams(pb);
}

#ifdef HAVE_LIBUDEV_H

static struct udev *udevroot=NULL;
static struct udev_monitor *monitor=NULL;
static GIOChannel *monitor_channel=NULL;
static guint monitor_src_id;

#include <libudev.h>

static gboolean on_monitor_data(GIOChannel *chan, GIOCondition cond, void *userdata){
	struct udev_device *dev=udev_monitor_receive_device(monitor);
	const char *subsys=udev_device_get_subsystem(dev);
	const char *type=udev_device_get_action(dev);
	g_message("USB event arrived for class %s of action type %s",subsys,type);
	if (strcmp(subsys,"sound")==0) linphone_gtk_reload_sound_devices();
	if (strcmp(subsys,"video4linux")==0) linphone_gtk_reload_video_devices();
	udev_device_unref(dev);
	return TRUE;
}

void linphone_gtk_monitor_usb(void){
	int fd;
	udevroot=udev_new();
	if (!udevroot) return;
	monitor=udev_monitor_new_from_netlink(udevroot,"udev");
	udev_monitor_filter_add_match_subsystem_devtype(monitor,"sound",NULL);
	udev_monitor_filter_add_match_subsystem_devtype(monitor,"video4linux",NULL);
	fd=udev_monitor_get_fd(monitor);
	monitor_channel=g_io_channel_unix_new(fd);
	monitor_src_id=g_io_add_watch(monitor_channel,G_IO_IN,on_monitor_data,NULL);
	udev_monitor_enable_receiving(monitor);
}

void linphone_gtk_unmonitor_usb(void){
	if (monitor) udev_monitor_unref(monitor);
	if (udevroot) udev_unref(udevroot);
	if (monitor_channel) {
		g_source_remove(monitor_src_id);
		g_io_channel_unref(monitor_channel);
	}
}

#else

void linphone_gtk_monitor_usb(void){
}
void linphone_gtk_unmonitor_usb(void){
}

#endif


