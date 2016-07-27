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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "status_icon.h"
#include "linphone.h"

#ifdef HAVE_GTK_OSX
#include <gtkosxapplication.h>
#endif

#if !defined(_WIN32) && !defined(__APPLE__) && GLIB_CHECK_VERSION(2, 26, 0)
#define STATUS_NOTIFIER_IS_USABLE 1
#endif

#include "status_notifier.h"
#include <mediastreamer2/mscommon.h>

typedef struct __LinphoneStatusIconDesc _LinphoneStatusIconDesc;

static LinphoneStatusIcon *_linphone_status_icon_instance = NULL;
static const _LinphoneStatusIconDesc *_linphone_status_icon_selected_desc = NULL;
static GSList *_linphone_status_icon_impls = NULL;


struct _LinphoneStatusIconParams {
	char *title;
	char *desc;
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
		if(obj->desc) g_free(obj->desc);
		g_free(obj);
	}
}

void linphone_status_icon_params_set_title(LinphoneStatusIconParams *obj, const char *title) {
	if(obj->title) g_free(obj->title);
	if(title) obj->title = g_strdup(title);
	else obj->title = NULL;
}

void linphone_status_icon_params_set_description(LinphoneStatusIconParams *obj, const char *desc) {
	if(obj->desc) g_free(obj->desc);
	if(desc) obj->desc = g_strdup(desc);
	else obj->desc = NULL;
}

void linphone_status_icon_params_set_menu(LinphoneStatusIconParams *obj, GtkWidget *menu) {
	if(obj->menu) g_object_unref(obj->menu);
	if(menu) obj->menu = g_object_ref_sink(menu);
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
typedef void (*LinphoneStatusIconDescIsSupportedResultCb)(const _LinphoneStatusIconDesc *obj, gboolean result, void *user_data);
typedef gboolean (*LinphoneStatusIconDescIsSupportedFunc)(
	const _LinphoneStatusIconDesc *desc,
	gboolean *result,
	LinphoneStatusIconDescIsSupportedResultCb cb,
	void *user_data
);
typedef void (*LinphoneStatusIconDescFindResultCb)(const _LinphoneStatusIconDesc *desc, void *user_data);

struct __LinphoneStatusIconDesc {
	const char *impl_name;
	LinphoneStatusIconDescInitFunc init;
	LinphoneStatusIconDescUninitFunc uninit;
	LinphoneStatusIconDescStartFunc start;
	LinphoneStatusIconDescEnableBlinkingFunc enable_blinking;
	LinphoneStatusIconDescIsSupportedFunc is_supported;
};

static gboolean _linphone_status_icon_desc_is_supported(
	const _LinphoneStatusIconDesc *desc,
	gboolean *result,
	LinphoneStatusIconDescIsSupportedResultCb cb,
	void *user_data) {

	return desc->is_supported(desc, result, cb, user_data);
}

typedef struct {
	GSList *i;
	LinphoneStatusIconDescFindResultCb cb;
	void *user_data;
} _LinphoneStatusIconDescSearchCtx;

static void _linphone_status_icon_desc_is_supported_result_cb(
	const _LinphoneStatusIconDesc *desc,
	gboolean result,
	_LinphoneStatusIconDescSearchCtx *ctx) {

	if(!result) {
		ctx->i = g_slist_next(ctx->i);
		for(; ctx->i; ctx->i = g_slist_next(ctx->i)) {
			if(_linphone_status_icon_desc_is_supported(
				(const _LinphoneStatusIconDesc *)g_slist_nth_data(ctx->i, 0),
				&result,
				(LinphoneStatusIconDescIsSupportedResultCb)_linphone_status_icon_desc_is_supported_result_cb,
				ctx)) {

				if(result) break;
			} else return;
		}
	}

	if(ctx->i) {
		const _LinphoneStatusIconDesc *desc = (const _LinphoneStatusIconDesc *)g_slist_nth_data(ctx->i, 0);
		ms_message("StatusIcon: found implementation: %s", desc->impl_name);
		if(ctx->cb) ctx->cb(desc, ctx->user_data);
	} else {
		g_warning("StatusIcon: no implementation found");
	}

	g_free(ctx);
}

static gboolean _linphone_status_icon_find_first_available_impl(
	const _LinphoneStatusIconDesc **desc,
	LinphoneStatusIconDescFindResultCb cb,
	void *user_data) {

	gboolean result;
	_LinphoneStatusIconDescSearchCtx *ctx = g_new0(_LinphoneStatusIconDescSearchCtx, 1);
	ctx->cb = cb;
	ctx->user_data = user_data;

	ms_message("StatusIcon: looking for implementation...");

	for(ctx->i=_linphone_status_icon_impls; ctx->i; ctx->i = g_slist_next(ctx->i)) {
		if(_linphone_status_icon_desc_is_supported(
			(const _LinphoneStatusIconDesc *)g_slist_nth_data(ctx->i, 0),
			&result,
			(LinphoneStatusIconDescIsSupportedResultCb)_linphone_status_icon_desc_is_supported_result_cb,
			ctx)) {

			if(result) {
				*desc = (const _LinphoneStatusIconDesc *)g_slist_nth_data(ctx->i, 0);
				ms_message("StatusIcon: found implementation: %s", (*desc)->impl_name);
				goto sync_return;
			}
		} else {
			return 0;
		}
	}
	g_warning("StatusIcon: no implementation found");
	*desc = NULL;

sync_return:
	g_free(ctx);
	return 1;
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
	if(obj->desc->uninit) obj->desc->uninit(obj);
	if(obj->params) linphone_status_icon_params_unref(obj->params);
	g_free(obj);
}

const char *linphone_status_icon_get_implementation_name(const LinphoneStatusIcon *obj) {
	return obj->desc->impl_name;
}

void linphone_status_icon_start(LinphoneStatusIcon *obj, LinphoneStatusIconParams *params) {
	ms_message("StatusIcon: starting status icon");
	obj->params = linphone_status_icon_params_ref(params);
	if(obj->desc->start) obj->desc->start(obj);
}

void linphone_status_icon_enable_blinking(LinphoneStatusIcon *obj, gboolean enable) {
	ms_message("StatusIcon: blinking set to %s", enable ? "TRUE" : "FALSE");
	if(obj->desc->enable_blinking) obj->desc->enable_blinking(obj, enable);
}

static void _linphone_status_icon_notify_click(LinphoneStatusIcon *obj) {
	if(obj->params->on_click_cb) {
		obj->params->on_click_cb(obj, obj->params->user_data);
	}
}


void _linphone_status_icon_init_cb(const _LinphoneStatusIconDesc *desc, void *user_data) {
	void **ctx = (void **)user_data;
	LinphoneStatusIconReadyCb cb = (LinphoneStatusIconReadyCb)ctx[0];
	_linphone_status_icon_selected_desc = desc;
	if(cb) cb(ctx[1]);
	g_free(ctx);
}

#ifdef STATUS_NOTIFIER_IS_USABLE
static const _LinphoneStatusIconDesc _linphone_status_icon_impl_status_notifier;
#endif
#ifdef HAVE_GTK_OSX
static const _LinphoneStatusIconDesc _linphone_status_icon_impl_gtkosx_app_desc;
#else
static const _LinphoneStatusIconDesc _linphone_status_icon_impl_gtk_desc;
#endif

void _linphone_status_icon_create_implementations_list(void) {
#ifdef STATUS_NOTIFIER_IS_USABLE
	_linphone_status_icon_impls = g_slist_append(_linphone_status_icon_impls, (void *)&_linphone_status_icon_impl_status_notifier);
#endif
#if HAVE_GTK_OSX
	_linphone_status_icon_impls = g_slist_append(_linphone_status_icon_impls, (void *)&_linphone_status_icon_impl_gtkosx_app_desc);
#else
	_linphone_status_icon_impls = g_slist_append(_linphone_status_icon_impls, (void *)&_linphone_status_icon_impl_gtk_desc);
#endif
}

gboolean linphone_status_icon_init(LinphoneStatusIconReadyCb ready_cb, void *user_data) {
	const _LinphoneStatusIconDesc *desc;
	void **ctx;

	ms_message("StatusIcon: Initialising");

	_linphone_status_icon_create_implementations_list();

	ctx = g_new(void *, 2);
	ctx[0] = ready_cb;
	ctx[1] = user_data;

	if(_linphone_status_icon_find_first_available_impl(&desc, _linphone_status_icon_init_cb, ctx)) {
		_linphone_status_icon_selected_desc = desc;
		g_free(ctx);
		return 1;
	} else return 0;
}

void linphone_status_icon_uninit(void) {
	if(_linphone_status_icon_instance) {
		_linphone_status_icon_free(_linphone_status_icon_instance);
		_linphone_status_icon_instance = NULL;
	}
	if(_linphone_status_icon_impls) {
		g_slist_free(_linphone_status_icon_impls);
		_linphone_status_icon_impls = NULL;
	}
	_linphone_status_icon_selected_desc = NULL;
}

LinphoneStatusIcon *linphone_status_icon_get(void) {
	if(_linphone_status_icon_instance == NULL) {
		if(_linphone_status_icon_selected_desc)
			ms_message("StatusIcon: instanciating singleton");
			_linphone_status_icon_instance = _linphone_status_icon_new(_linphone_status_icon_selected_desc);
	}
	return _linphone_status_icon_instance;
}


/* GtkStatusIcon implementation */
#ifndef HAVE_GTK_OSX
static void _linphone_status_icon_impl_gtk_on_click_cb(LinphoneStatusIcon *si) {
	_linphone_status_icon_notify_click(si);
}

static void _linphone_status_icon_impl_gtk_popup_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, LinphoneStatusIcon *si){
	GtkWidget *menu = si->params->menu;
	gtk_menu_popup(GTK_MENU(menu),NULL,NULL,gtk_status_icon_position_menu,status_icon,button,activate_time);
}

static void _linphone_status_icon_impl_gtk_init(LinphoneStatusIcon *si) {
	const char *icon_name=linphone_gtk_get_ui_config("icon_name",LINPHONE_ICON_NAME);
	const char *blinking_icon_name=linphone_gtk_get_ui_config("binking_status_icon_name","linphone-start-call");
	GtkStatusIcon *icon=gtk_status_icon_new_from_icon_name(icon_name);
	g_signal_connect_swapped(G_OBJECT(icon),"activate", G_CALLBACK(_linphone_status_icon_impl_gtk_on_click_cb), si);
	g_signal_connect(G_OBJECT(icon), "popup-menu", G_CALLBACK(_linphone_status_icon_impl_gtk_popup_menu), si);
	g_object_set_data_full(G_OBJECT(icon), "icon", g_strdup(icon_name), g_free);
	g_object_set_data_full(G_OBJECT(icon), "call_icon", g_strdup(blinking_icon_name), g_free);
	si->data = icon;
}

static void _linphone_status_icon_impl_gtk_uninit(LinphoneStatusIcon *si) {
	GtkStatusIcon *icon = GTK_STATUS_ICON(si->data);
	gtk_status_icon_set_visible(icon, FALSE);
}

static void _linphone_status_icon_impl_gtk_start(LinphoneStatusIcon *si) {
	GtkStatusIcon *icon = GTK_STATUS_ICON(si->data);
#if GTK_CHECK_VERSION(2,20,2)
	char *name = g_strdup_printf("%s - %s", si->params->title, si->params->desc);
	gtk_status_icon_set_name(icon, name);
	g_free(name);
#endif
	gtk_status_icon_set_visible(icon,TRUE);
}

static gboolean _linphone_status_icon_impl_gtk_do_icon_blink_cb(GtkStatusIcon *gi){
	const gchar *call_icon = (const gchar *)g_object_get_data(G_OBJECT(gi),"call_icon");
	const gchar *normal_icon = (const gchar *)g_object_get_data(G_OBJECT(gi),"icon");
	const gchar *cur_icon = (const gchar *)gtk_status_icon_get_icon_name(gi);
	if (cur_icon == call_icon){
		gtk_status_icon_set_from_icon_name(gi,normal_icon);
	}else{
		gtk_status_icon_set_from_icon_name(gi,call_icon);
	}
	return TRUE;
}

static void _linphone_status_icon_impl_enable_blinking(LinphoneStatusIcon *si, gboolean val) {
	GtkStatusIcon *icon = GTK_STATUS_ICON(si->data);
	guint tout;
	tout=(unsigned)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(icon),"timeout"));
	if (val && tout==0){
		tout=g_timeout_add(500,(GSourceFunc)_linphone_status_icon_impl_gtk_do_icon_blink_cb,icon);
		g_object_set_data(G_OBJECT(icon),"timeout",GINT_TO_POINTER(tout));
	}else if (!val && tout!=0){
		const gchar *normal_icon = (const gchar *)g_object_get_data(G_OBJECT(icon),"icon");
		g_source_remove(tout);
		g_object_set_data(G_OBJECT(icon),"timeout",NULL);
		gtk_status_icon_set_from_icon_name(icon,normal_icon);
	}
}

static gboolean _linphone_status_icon_impl_is_supported(
	const _LinphoneStatusIconDesc *desc,
	gboolean *result,
	LinphoneStatusIconDescIsSupportedResultCb cb,
	void *user_data) {

	*result = 1;
	return 1;
}

static const _LinphoneStatusIconDesc _linphone_status_icon_impl_gtk_desc = {
	"gtk_status_icon",
	_linphone_status_icon_impl_gtk_init,
	_linphone_status_icon_impl_gtk_uninit,
	_linphone_status_icon_impl_gtk_start,
	_linphone_status_icon_impl_enable_blinking,
	_linphone_status_icon_impl_is_supported
};
#endif


/* GtkosxApplication implementation */
#ifdef HAVE_GTK_OSX
static void _linphone_status_icon_impl_gtkosx_app_enable_blinking(LinphoneStatusIcon *si, gboolean val) {
	GtkosxApplication *theMacApp=gtkosx_application_get();
	gint *attention_id = (gint *)&si->data;
	if (val) {
		*attention_id=gtkosx_application_attention_request(theMacApp, CRITICAL_REQUEST);
	} else if (*attention_id != 0) {
		gtkosx_application_cancel_attention_request(theMacApp, *attention_id);
		*attention_id = 0;
	}
}

static gboolean _linphone_status_icon_impl_gtkosx_app_is_supported(
	const _LinphoneStatusIconDesc *desc,
	gboolean *result,
	LinphoneStatusIconDescIsSupportedResultCb cb,
	void *user_data) {

	*result = 1;
	return 1;
}

static const _LinphoneStatusIconDesc _linphone_status_icon_impl_gtkosx_app_desc = {
	.impl_name = "gtkosx_application",
	.init = NULL,
	.uninit = NULL,
	.start = NULL,
	.enable_blinking = _linphone_status_icon_impl_gtkosx_app_enable_blinking,
	.is_supported = _linphone_status_icon_impl_gtkosx_app_is_supported
};
#endif


/* Implementation based on the StatusNotifier Freedesktop standard */
#ifdef STATUS_NOTIFIER_IS_USABLE
typedef struct {
	int x;
	int y;
} _LinphoneStatusIconPosition;

static void _linphone_status_icon_impl_sn_init(LinphoneStatusIcon *si) {
	si->data = bc_status_notifier_new();
}

// static void _linphone_status_icon_impl_sn_uninit(LinphoneStatusIcon *si) {
// 	bc_status_notifier_unref((BcStatusNotifier *)si->data);
// }

static void _linphone_status_icon_impl_sn_activated_cb(BcStatusNotifier *sn, int x, int y, void *user_data) {
	LinphoneStatusIcon *si = (LinphoneStatusIcon *)user_data;
	_linphone_status_icon_notify_click(si);
}

static void _linphone_status_icon_impr_sn_get_position(GtkMenu *menu, int *x, int *y, gboolean *push_in, gpointer data) {
	_LinphoneStatusIconPosition *pos = (_LinphoneStatusIconPosition *)data;
	*x = pos->x;
	*y = pos->y;
	*push_in = TRUE;
}

static void _linphone_status_icon_impl_sn_menu_called_cb(BcStatusNotifier *sn, int x, int y, void *user_data) {
	LinphoneStatusIcon *si = (LinphoneStatusIcon *)user_data;
	GtkWidget *menu = si->params->menu;
	_LinphoneStatusIconPosition pos = {x, y};
	gtk_menu_popup(
		GTK_MENU(menu),
		NULL,
		NULL,
		_linphone_status_icon_impr_sn_get_position,
		&pos,
		0,
		gtk_get_current_event_time()
	);
}

static void _linphone_status_icon_impl_sn_start(LinphoneStatusIcon *si) {
	BcStatusNotifier *sn = (BcStatusNotifier *)si->data;
	BcStatusNotifierParams *params;
	BcStatusNotifierToolTip *tooltip = bc_status_notifier_tool_tip_new("linphone", si->params->title, si->params->desc);
	BcStatusNotifierSignalsVTable vtable = {NULL};

	vtable.activate_called_cb = _linphone_status_icon_impl_sn_activated_cb;
	vtable.context_menu_called_cb = _linphone_status_icon_impl_sn_menu_called_cb;

	params = bc_status_notifier_params_new();
	bc_status_notifier_params_set_dbus_prefix(params, "org.kde");
	bc_status_notifier_params_set_category(params, BcStatusNotifierCategoryCommunications);
	bc_status_notifier_params_set_id(params, "linphone");
	bc_status_notifier_params_set_title(params, si->params->title);
	bc_status_notifier_params_set_icon_name(params, "linphone");
	bc_status_notifier_params_set_tool_tip(params, tooltip);
	bc_status_notifier_params_set_vtable(params, &vtable, si);

	bc_status_notifier_start(sn, params, NULL, NULL);

	bc_status_notifier_tool_tip_unref(tooltip);
	bc_status_notifier_params_unref(params);
}

static void _linphone_status_icon_impl_sn_enable_blinking(LinphoneStatusIcon *si, gboolean val) {
	BcStatusNotifier *sn = (BcStatusNotifier *)si->data;
	if(val) {
		bc_status_notifier_update_status(sn, BcStatusNotifierStatusNeedsAttention);
	} else {
		bc_status_notifier_update_status(sn, BcStatusNotifierStatusPassive);
	}
}

static void _linphone_status_icon_impl_is_supported_cb(const char *prefix, gboolean result, void **data) {
	_LinphoneStatusIconDesc *desc = (_LinphoneStatusIconDesc *)data[0];
	LinphoneStatusIconDescIsSupportedResultCb cb = (LinphoneStatusIconDescIsSupportedResultCb)data[1];
	if(cb) cb(desc, result, data[2]);
	g_free(data);
	g_free(desc);
}

static gboolean _linphone_status_icon_impl_sn_is_supported(
	const _LinphoneStatusIconDesc *desc,
	gboolean *result,
	LinphoneStatusIconDescIsSupportedResultCb cb,
	void *user_data) {

	_LinphoneStatusIconDesc *desc2;
	void **data;
	const char *desktop = g_getenv("XDG_CURRENT_DESKTOP");

	if(desktop == NULL || g_strcmp0(desktop, "KDE") != 0) {
		*result = FALSE;
		return TRUE;
	}

	desc2 = g_new(_LinphoneStatusIconDesc, 1);
	*desc2 = *desc;
	data = g_new(void *, 3);
	data[0] = desc2;
	data[1] = cb;
	data[2] = user_data;
	bc_status_notifier_is_supported(
		"org.kde",
		(BcStatusNotifierSupportDetectionCb)_linphone_status_icon_impl_is_supported_cb,
		data
	);
	return FALSE;
}

static const _LinphoneStatusIconDesc _linphone_status_icon_impl_status_notifier = {
	.impl_name = "status_notifier",
	.init = _linphone_status_icon_impl_sn_init,
	.uninit = NULL,
	.start = _linphone_status_icon_impl_sn_start,
	.enable_blinking = _linphone_status_icon_impl_sn_enable_blinking,
	.is_supported = _linphone_status_icon_impl_sn_is_supported
};
#endif
