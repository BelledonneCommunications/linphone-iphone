/*
linphone, gtk-glade interface.
Copyright (C) 2015 Belledonne Communications <info@belledonne-communications.com>

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

#include <glib.h>
#include <gtk/gtk.h>
#include "status_icon.h"
#include "linphone.h"

struct _LinphoneStatusIconParams {
	char *title;
	GtkWidget *menu;
	LinphoneStatusIconOnClickCallback on_click_cb;
	void *user_data;
	int ref;
};

LinphoneStatusIconParams *linphone_status_icon_params_new(void) {
	return g_new0(LinphoneStatusIconParams, 1);
}

LinphoneStatusIconParams *linphone_status_icon_params_ref(LinphoneStatusIconParams *obj) {
	obj->ref++;
	return obj;
}

void linphone_status_icon_params_unref(LinphoneStatusIconParams *obj) {
	obj->ref--;
	if(obj->ref < 0) {
		if(obj->title) g_free(obj->title);
		if(obj->menu) g_object_unref(obj->menu);
		g_free(obj);
	}
}

void linphone_status_icon_params_set_title(LinphoneStatusIconParams *obj, const char *title) {
	if(obj->title) g_free(obj->title);
	if(title) obj->title = g_strdup(title);
	else obj->title = NULL;
}

void linphone_status_icon_params_set_menu(LinphoneStatusIconParams *obj, GtkWidget *menu) {
	if(obj->menu) g_object_unref(obj->menu);
	if(menu) obj->menu = g_object_ref(menu);
	else obj->menu = NULL;
}

void linphone_status_icon_params_set_on_click_cb(LinphoneStatusIconParams *obj, LinphoneStatusIconOnClickCallback cb, void *user_data) {
	obj->on_click_cb = cb;
	obj->user_data = user_data;
}


typedef void (*LinphoneStatusIconDescInitFunc)(LinphoneStatusIcon *obj);
typedef void (*LinphoneStatusIconDescUninitFunc)(LinphoneStatusIcon *obj);
typedef void (*LinphoneStatusIconDescStartFunc)(LinphoneStatusIcon *obj);
typedef void (*LinphoneStatusIconDescEnableBlinkingFunc)(LinphoneStatusIcon *obj, gboolean enable);
typedef gboolean (*LinphoneStatusIconDescIsSupported)(void);

typedef struct {
	const char *impl_name;
	LinphoneStatusIconDescInitFunc init;
	LinphoneStatusIconDescUninitFunc uninit;
	LinphoneStatusIconDescStartFunc start;
	LinphoneStatusIconDescEnableBlinkingFunc enable_blinking;
	LinphoneStatusIconDescIsSupported is_supported;
} _LinphoneStatusIconDesc;

static const _LinphoneStatusIconDesc *_linphone_status_icon_impls[];

static const _LinphoneStatusIconDesc *_status_icon_find_instance(void) {
	int i;
	for(i=0; _linphone_status_icon_impls[i] && !_linphone_status_icon_impls[i]->is_supported(); i++);
	return _linphone_status_icon_impls[i];
}


struct _LinphoneStatusIcon {
	const _LinphoneStatusIconDesc *desc;
	LinphoneStatusIconParams *params;
	void *data;
};

static LinphoneStatusIcon *_linphone_status_icon_new(const _LinphoneStatusIconDesc *desc) {
	LinphoneStatusIcon *si = (LinphoneStatusIcon *)g_new0(LinphoneStatusIcon, 1);
	si->desc = desc;
	if(desc->init) desc->init(si);
	return si;
}

static void _linphone_status_icon_free(LinphoneStatusIcon *obj) {
	if(obj->desc->uninit) obj->desc->uninit(obj->data);
	g_free(obj);
}

const char *linphone_status_icon_get_implementation_name(const LinphoneStatusIcon *obj) {
	return obj->desc->impl_name;
}

void linphone_status_icon_start(LinphoneStatusIcon *obj, LinphoneStatusIconParams *params) {
	obj->params = linphone_status_icon_params_ref(params);
	obj->desc->start(obj);
}

void linphone_status_icon_enable_blinking(LinphoneStatusIcon *obj, gboolean enable) {
	obj->desc->enable_blinking(obj, enable);
}

static void _linphone_status_icon_notify_click(LinphoneStatusIcon *obj) {
	if(obj->params->on_click_cb) {
		obj->params->on_click_cb(obj, obj->params->user_data);
	}
}


static LinphoneStatusIcon *_linphone_status_icon_instance = NULL;

static void _linphone_status_icon_free_singleton(void) {
	_linphone_status_icon_free(_linphone_status_icon_instance);
}

LinphoneStatusIcon *linphone_status_icon_get(void) {
	if(_linphone_status_icon_instance == NULL) {
		const _LinphoneStatusIconDesc *desc = _status_icon_find_instance();
		if(desc) {
			_linphone_status_icon_instance = _linphone_status_icon_new(desc);
			atexit(_linphone_status_icon_free_singleton);
		}
	}
	return _linphone_status_icon_instance;
}


/* GtkStatusIcon implementation */
static void _linphone_status_icon_impl_gtk_on_click_cb(LinphoneStatusIcon *si) {
	_linphone_status_icon_notify_click(si);
}

static void _linphone_status_icon_impl_gtk_popup_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, LinphoneStatusIcon *si){
	GtkWidget *menu = si->params->menu;
	gtk_menu_popup(GTK_MENU(menu),NULL,NULL,gtk_status_icon_position_menu,status_icon,button,activate_time);
}

static void _linphone_status_icon_impl_gtk_init(LinphoneStatusIcon *si) {
	const char *icon_path=linphone_gtk_get_ui_config("icon",LINPHONE_ICON);
	const char *call_icon_path=linphone_gtk_get_ui_config("start_call_icon","startcall-green.png");
	GdkPixbuf *pbuf=create_pixbuf(icon_path);
	GtkStatusIcon *icon=gtk_status_icon_new_from_pixbuf(pbuf);
	g_signal_connect_swapped(G_OBJECT(icon),"activate", G_CALLBACK(_linphone_status_icon_impl_gtk_on_click_cb), si);
	g_signal_connect(G_OBJECT(icon), "popup-menu", G_CALLBACK(_linphone_status_icon_impl_gtk_popup_menu), si);
	g_object_set_data(G_OBJECT(icon),"icon",pbuf);
	g_object_unref(pbuf);
	pbuf=create_pixbuf(call_icon_path);
	g_object_set_data(G_OBJECT(icon),"call_icon",pbuf);
	g_object_unref(pbuf);
	si->data = icon;
}

// static void _linphone_status_icon_impl_gtk_uninit(LinphoneStatusIcon *si) {
// 	g_object_unref((GtkStatusIcon *)si->data);
// }

static void _linphone_status_icon_impl_gtk_start(LinphoneStatusIcon *si) {
	GtkStatusIcon *icon = GTK_STATUS_ICON(si->data);
#if GTK_CHECK_VERSION(2,20,2)
	gtk_status_icon_set_name(icon, si->params->title);
#endif
	gtk_status_icon_set_visible(icon,TRUE);
}

static gboolean _linphone_status_icon_impl_gtk_do_icon_blink_cb(GtkStatusIcon *gi){
	GdkPixbuf *call_icon=g_object_get_data(G_OBJECT(gi),"call_icon");
	GdkPixbuf *normal_icon=g_object_get_data(G_OBJECT(gi),"icon");
	GdkPixbuf *cur_icon=gtk_status_icon_get_pixbuf(gi);
	if (cur_icon==call_icon){
		gtk_status_icon_set_from_pixbuf(gi,normal_icon);
	}else{
		gtk_status_icon_set_from_pixbuf(gi,call_icon);
	}
	return TRUE;
}

static void _linphone_status_icon_impl_enable_blinking(LinphoneStatusIcon *si, gboolean val) {
	GtkStatusIcon *icon = GTK_STATUS_ICON(si->data);
#ifdef HAVE_GTK_OSX
	static gint attention_id;
	GtkosxApplication *theMacApp=gtkosx_application_get();
	if (val)
		attention_id=gtkosx_application_attention_request(theMacApp,CRITICAL_REQUEST);
	else gtkosx_application_cancel_attention_request(theMacApp,attention_id);
#else
	guint tout;
	tout=(unsigned)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(icon),"timeout"));
	if (val && tout==0){
		tout=g_timeout_add(500,(GSourceFunc)_linphone_status_icon_impl_gtk_do_icon_blink_cb,icon);
		g_object_set_data(G_OBJECT(icon),"timeout",GINT_TO_POINTER(tout));
	}else if (!val && tout!=0){
		GdkPixbuf *normal_icon=g_object_get_data(G_OBJECT(icon),"icon");
		g_source_remove(tout);
		g_object_set_data(G_OBJECT(icon),"timeout",NULL);
		gtk_status_icon_set_from_pixbuf(icon,normal_icon);
	}
#endif
}

static gboolean _linphone_status_icon_impl_is_supported(void) {
#ifndef HAVE_GTK_OSX
	return 1;
#else
	return 0;
#endif
}

static const _LinphoneStatusIconDesc _linphone_status_icon_impl_gtk_desc = {
	.impl_name = "gtk_status_icon",
	.init = _linphone_status_icon_impl_gtk_init,
	.uninit = NULL,
	.start = _linphone_status_icon_impl_gtk_start,
	.enable_blinking = _linphone_status_icon_impl_enable_blinking,
	.is_supported = _linphone_status_icon_impl_is_supported
};

static const _LinphoneStatusIconDesc *_linphone_status_icon_impls[] = {
	&_linphone_status_icon_impl_gtk_desc,
	NULL
};
