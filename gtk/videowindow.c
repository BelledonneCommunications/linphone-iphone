/*
linphone, gtk interface.
Copyright (C) 2014 Belledonne Communications SARL

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "linphone.h"

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#elif defined(_WIN32)
#include <gdk/gdkwin32.h>
#elif defined(__APPLE__)
extern void *gdk_quartz_window_get_nswindow(GdkWindow      *window);
extern void *gdk_quartz_window_get_nsview(GdkWindow      *window);
#endif

#include <gdk/gdkkeysyms.h>

enum {
	TARGET_STRING,
	TARGET_TEXT,
	TARGET_URILIST
};

static GtkTargetEntry targets[] = {
	{ "text/uri-list", GTK_TARGET_OTHER_APP, TARGET_URILIST },
};

static void set_video_controls_position(GtkWidget *video_window);

static void on_end_of_play(LinphonePlayer *player, void *user_data){
	linphone_player_close(player);
}

static void drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
	GtkSelectionData *selection_data, guint target_type, guint time, gpointer user_data){
	int datalen=gtk_selection_data_get_length(selection_data);
	const void *data=gtk_selection_data_get_data(selection_data);
	LinphoneCall *call=g_object_get_data(G_OBJECT(widget),"call");

	ms_message("target_type=%i, datalen=%i, data=%p",target_type,datalen,data);
	if (target_type==TARGET_URILIST && data){
		LinphonePlayer *player=linphone_call_get_player(call);
		char *path=ms_strdup(data);
		while (datalen&&(path[datalen-1]=='\r'||path[datalen-1]=='\n')) {
			path[datalen-1]='\0';
			datalen--;
		}
		if (player){

			const char* filepath = (strstr(path,"file://")==path) ? path+strlen("file://") : path;
			if (linphone_player_open(player,filepath,on_end_of_play,NULL)==0){

				linphone_player_start(player);
			}else{
				GtkWidget *warn=gtk_message_dialog_new(GTK_WINDOW(widget),GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
								       GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE,
									_("Cannot play %s."),filepath);
				g_signal_connect(warn,"response",(GCallback)gtk_widget_destroy,NULL);
				gtk_widget_show(warn);
			}
		}
		ms_free(path);
	}
	gtk_drag_finish (context, TRUE, FALSE, time);
}

static gboolean drag_drop(GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data){
#if GTK_CHECK_VERSION(2,21,0)
	GList *l=gdk_drag_context_list_targets(drag_context);
	GList *elem;

	if (l){
		ms_message("drag_drop");
		/* Choose the best target type */
		for(elem=l;elem!=NULL;elem=g_list_next(elem)){
			char *name=gdk_atom_name(GDK_POINTER_TO_ATOM(elem->data));
			ms_message("target: %s",name);
			g_free(name);
		}
	}else{
		ms_warning("drag_drop no targets");
		return FALSE;
	}
#endif
	return TRUE;
}

static void *get_native_handle(GdkWindow *gdkw){
#ifdef GDK_WINDOWING_X11
	return (void *)GDK_WINDOW_XID(gdkw);
#elif defined(_WIN32)
	return (void *)GDK_WINDOW_HWND(gdkw);
#elif defined(__APPLE__)
	return (void *)gdk_quartz_window_get_nsview(gdkw);
#endif
	g_warning("No way to get the native handle from gdk window");
	return 0;
}

static void _resize_video_window(GtkWidget *video_window, MSVideoSize vsize){
	MSVideoSize cur;
	gtk_window_get_size(GTK_WINDOW(video_window),&cur.width,&cur.height);
	if (vsize.width*vsize.height > cur.width*cur.height ||
		ms_video_size_get_orientation(vsize)!=ms_video_size_get_orientation(cur) ){
		gtk_window_resize(GTK_WINDOW(video_window),vsize.width,vsize.height);
	}
}

static gboolean resize_video_window(LinphoneCall *call){
	const LinphoneCallParams *params=linphone_call_get_current_params(call);
	if (params){
		MSVideoSize vsize=linphone_call_params_get_received_video_size(params);
		if (vsize.width>0 && vsize.height>0){
			GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
			GtkWidget *video_window=(GtkWidget*)g_object_get_data(G_OBJECT(callview),"video_window");
			if (video_window){
				_resize_video_window(video_window,vsize);
			}
		}
	}
	return TRUE;
}

static void on_video_window_destroy(GtkWidget *w, guint timeout){
	g_source_remove(timeout);
	linphone_core_set_native_video_window_id(linphone_gtk_get_core(),LINPHONE_VIDEO_DISPLAY_NONE);
}

static void video_window_set_fullscreen(GtkWidget *w, gboolean val){
	if (val){
		g_object_set_data(G_OBJECT(w),"fullscreen",GINT_TO_POINTER(1));
			gtk_window_fullscreen(GTK_WINDOW(w));
	}else{
		g_object_set_data(G_OBJECT(w),"fullscreen",GINT_TO_POINTER(0));
			gtk_window_unfullscreen(GTK_WINDOW(w));
	}
}
/*old names in old version of gdk*/
#ifndef GDK_KEY_Escape
#define GDK_KEY_Escape GDK_Escape
#define GDK_KEY_F GDK_F
#define GDK_KEY_f GDK_f
#endif

static void on_video_window_key_press(GtkWidget *w, GdkEvent *ev, gpointer up){
	g_message("Key press event");
	switch(ev->key.keyval){
		case GDK_KEY_f:
		case GDK_KEY_F:
			video_window_set_fullscreen(w,TRUE);
		break;
		case GDK_KEY_Escape:
			video_window_set_fullscreen(w,FALSE);
		break;
	}
}

static void on_controls_response(GtkWidget *dialog, int response_id, GtkWidget *video_window){

	gtk_widget_destroy(dialog);
	switch(response_id){
		case GTK_RESPONSE_YES:
			video_window_set_fullscreen(video_window,TRUE);
		break;
		case GTK_RESPONSE_NO:
			video_window_set_fullscreen(video_window,FALSE);
		break;
		case GTK_RESPONSE_REJECT:
		{
			LinphoneCall *call=(LinphoneCall*)g_object_get_data(G_OBJECT(video_window),"call");
			linphone_core_terminate_call(linphone_gtk_get_core(),call);
		}
		break;
		case GTK_RESPONSE_APPLY:
		{
			LinphoneCall *call=(LinphoneCall*)g_object_get_data(G_OBJECT(video_window),"call");
			char *path = (char *)linphone_gtk_get_snapshot_path();
			linphone_call_take_video_snapshot(call, path);
		}
	}

}

static gboolean on_controls_destroy(GtkWidget *w){
	GtkWidget *video_window=(GtkWidget*)g_object_get_data(G_OBJECT(w),"video_window");
	gint timeout=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"timeout"));
	if (timeout!=0){
		g_source_remove(timeout);
		g_object_set_data(G_OBJECT(w),"timeout",GINT_TO_POINTER(0));
	}
	if (video_window) {
		g_object_set_data(G_OBJECT(video_window),"controls",NULL);
	}
	return FALSE;
}

static gboolean _set_video_controls_position(GtkWidget *video_window){
	GtkWidget *w=(GtkWidget*)g_object_get_data(G_OBJECT(video_window),"controls");
	if (w){
		gint vw,vh;
		gint cw,ch;
		gint x,y;
		gtk_window_get_size(GTK_WINDOW(video_window),&vw,&vh);
		gtk_window_get_position(GTK_WINDOW(video_window),&x,&y);
		gtk_window_get_size(GTK_WINDOW(w),&cw,&ch);
		gtk_window_move(GTK_WINDOW(w),x+vw/2 - cw/2, y + vh - ch);
	}
	return FALSE;
}

static void set_video_controls_position(GtkWidget *video_window){
	/*do it a first time*/
	_set_video_controls_position(video_window);
	/*and schedule to do it a second time in order to workaround a bug in fullscreen mode, where poistion is not taken into account the first time*/
	g_timeout_add(0,(GSourceFunc)_set_video_controls_position,video_window);
}

static gboolean video_window_moved(GtkWidget *widget, GdkEvent  *event, gpointer   user_data){
	/*Workaround to Video window bug on Windows. */
	/* set_video_controls_position(widget); */
	return FALSE;
}

static gint do_gtk_widget_destroy(GtkWidget *w){
	gtk_widget_destroy(w);
	return FALSE;
}

static void schedule_video_controls_disapearance(GtkWidget *w){
	gint timeout=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"timeout"));
	if (timeout != 0) g_source_remove(timeout);
	timeout=g_timeout_add(3000,(GSourceFunc)do_gtk_widget_destroy,w);
	g_object_set_data(G_OBJECT(w),"timeout",GINT_TO_POINTER(timeout));
}

static GtkWidget *show_video_controls(GtkWidget *video_window){
	GtkWidget *w;
	w=(GtkWidget*)g_object_get_data(G_OBJECT(video_window),"controls");
	if (!w){
		gboolean isfullscreen=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(video_window),"fullscreen"));
		const char *stock_button=isfullscreen ? GTK_STOCK_LEAVE_FULLSCREEN : GTK_STOCK_FULLSCREEN;
		gint response_id=isfullscreen ? GTK_RESPONSE_NO : GTK_RESPONSE_YES ;
		GtkWidget *image = gtk_image_new_from_icon_name(linphone_gtk_get_ui_config("stop_call_icon_name","linphone-stop-call"), GTK_ICON_SIZE_BUTTON);
		GtkWidget *button;
		w=gtk_dialog_new_with_buttons("",GTK_WINDOW(video_window),GTK_DIALOG_DESTROY_WITH_PARENT,stock_button,response_id,NULL);
		gtk_window_set_opacity(GTK_WINDOW(w),0.5);
		gtk_window_set_decorated(GTK_WINDOW(w),FALSE);
		button=gtk_button_new_with_label(_("Hang up"));
		gtk_button_set_image(GTK_BUTTON(button), image);
		gtk_widget_show(button);
		gtk_dialog_add_action_widget(GTK_DIALOG(w),button,GTK_RESPONSE_REJECT);
		button=gtk_button_new_with_label(_("Take screenshot"));
		image = gtk_image_new_from_icon_name("linphone-take-screenshot", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image(GTK_BUTTON(button), image);
		gtk_widget_show(button);
		gtk_dialog_add_action_widget(GTK_DIALOG(w),button,GTK_RESPONSE_APPLY);
		g_signal_connect(w,"response",(GCallback)on_controls_response,video_window);
		schedule_video_controls_disapearance(w);
		g_signal_connect(w,"destroy",(GCallback)on_controls_destroy,NULL);
		g_object_set_data(G_OBJECT(w),"video_window",video_window);
		g_object_set_data(G_OBJECT(video_window),"controls",w);
		set_video_controls_position(video_window);
		gtk_widget_show(w);
	}else{
		schedule_video_controls_disapearance(w);
	}
	return w;
}

static GtkWidget *create_video_window(LinphoneCall *call){
	char *remote,*title;
	GtkWidget *video_window;
	const LinphoneAddress *addr;
	guint timeout;
	MSVideoSize vsize={MS_VIDEO_SIZE_CIF_W,MS_VIDEO_SIZE_CIF_H};
	GdkColor color;

	addr=linphone_call_get_remote_address(call);
	remote=linphone_gtk_address(addr);
	video_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	/*gtk_window_set_transient_for(GTK_WINDOW(video_window), GTK_WINDOW(linphone_gtk_get_main_window()));*/
	title=g_strdup_printf("%s - Video call with %s",linphone_gtk_get_ui_config("title","Linphone"),remote);
	ms_free(remote);
	gtk_window_set_title(GTK_WINDOW(video_window),title);
	g_free(title);
	gtk_window_resize(GTK_WINDOW(video_window),vsize.width,vsize.height);
	gdk_color_parse("black",&color);
	gtk_widget_modify_bg(video_window,GTK_STATE_NORMAL,&color);

	gtk_drag_dest_set(video_window, GTK_DEST_DEFAULT_ALL, targets, sizeof(targets)/sizeof(GtkTargetEntry), GDK_ACTION_COPY);
	gtk_widget_show(video_window);
	gdk_window_set_events(gtk_widget_get_window(video_window),
			      gdk_window_get_events(gtk_widget_get_window(video_window)) | GDK_POINTER_MOTION_MASK);
	timeout=g_timeout_add(500,(GSourceFunc)resize_video_window,call);
	g_signal_connect(video_window,"destroy",(GCallback)on_video_window_destroy,GINT_TO_POINTER(timeout));
	g_signal_connect(video_window,"key-press-event",(GCallback)on_video_window_key_press,NULL);
	g_signal_connect_swapped(video_window,"motion-notify-event",(GCallback)show_video_controls,video_window);
	g_signal_connect(video_window,"configure-event",(GCallback)video_window_moved,NULL);
	g_signal_connect(video_window, "drag-data-received",(GCallback)drag_data_received, NULL);
	g_signal_connect(video_window, "drag-drop",(GCallback)drag_drop, NULL);
	g_object_set_data(G_OBJECT(video_window),"call",call);
	return video_window;
}

void linphone_gtk_in_call_show_video(LinphoneCall *call){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *video_window=(GtkWidget*)g_object_get_data(G_OBJECT(callview),"video_window");
	const LinphoneCallParams *params=linphone_call_get_current_params(call);
	LinphoneCore *lc=linphone_gtk_get_core();

	if (((bool_t)lp_config_get_int(linphone_core_get_config(lc), "video", "rtp_io", FALSE)) == FALSE) {
		if (linphone_call_get_state(call)!=LinphoneCallPaused && params && linphone_call_params_video_enabled(params)){
			if (video_window==NULL){
				video_window=create_video_window(call);
				g_object_set_data(G_OBJECT(callview),"video_window",video_window);
			}
			linphone_core_set_native_video_window_id(lc,get_native_handle(gtk_widget_get_window(video_window)));
			gtk_window_present(GTK_WINDOW(video_window));
		}else{
			if (video_window){
				gtk_widget_destroy(video_window);
				g_object_set_data(G_OBJECT(callview),"video_window",NULL);
			}
		}
	}
}

static void on_video_preview_destroyed(GtkWidget *video_preview, GtkWidget *mw){
	LinphoneCore *lc=linphone_gtk_get_core();
	guint timeout_id=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(video_preview),"timeout-id"));
	g_object_set_data(G_OBJECT(mw),"video_preview",NULL);
	linphone_core_enable_video_preview(lc,FALSE);
	linphone_core_set_native_preview_window_id(lc,(void *)(unsigned long)-1);
	g_source_remove(timeout_id);
}

GtkWidget *linphone_gtk_get_camera_preview_window(void){
	return (GtkWidget *)g_object_get_data(G_OBJECT(linphone_gtk_get_main_window()),"video_preview");
}

static gboolean check_preview_size(GtkWidget *video_preview){
	MSVideoSize vsize=linphone_core_get_current_preview_video_size(linphone_gtk_get_core());
	if (vsize.width && vsize.height){
		MSVideoSize cur;
		gtk_window_get_size(GTK_WINDOW(video_preview),&cur.width,&cur.height);
		if (cur.width!=vsize.width || cur.height!=vsize.height){
			gtk_window_resize(GTK_WINDOW(video_preview),vsize.width,vsize.height);
		}
	}
	return TRUE;
}

void linphone_gtk_show_camera_preview_clicked(GtkButton *button){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *video_preview=(GtkWidget *)g_object_get_data(G_OBJECT(mw),"video_preview");

	if (!video_preview){
		gchar *title;
		LinphoneCore *lc=linphone_gtk_get_core();
		GdkColor color;
		guint tid;

		video_preview=gtk_window_new(GTK_WINDOW_TOPLEVEL);
		title=g_strdup_printf("%s - Video preview",linphone_gtk_get_ui_config("title","Linphone"));
		gtk_window_set_title(GTK_WINDOW(video_preview),title);
		gdk_color_parse("black",&color);
		gtk_widget_modify_bg(video_preview,GTK_STATE_NORMAL,&color);
		g_free(title);
		g_object_set_data(G_OBJECT(mw),"video_preview",video_preview);
		g_signal_connect(video_preview,"destroy",(GCallback)on_video_preview_destroyed,mw);
		gtk_widget_show(video_preview);
		linphone_core_set_native_preview_window_id(lc,get_native_handle(gtk_widget_get_window(video_preview)));
		linphone_core_enable_video_preview(lc,TRUE);
		tid=g_timeout_add(100,(GSourceFunc)check_preview_size,video_preview);
		g_object_set_data(G_OBJECT(video_preview),"timeout-id",GINT_TO_POINTER(tid));
	}
}

