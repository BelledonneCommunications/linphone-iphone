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

#include "status_notifier.h"
#include <gio/gio.h>
#include <string.h>

#ifdef _MSC_VER
#include <process.h>
#define getpid() _getpid()
typedef int lppid_t;
#else
#include <unistd.h>
typedef pid_t lppid_t;
#endif

const gchar *bc_status_notifier_category_to_string(BcStatusNotifierCategory c) {
	switch(c){
		case BcStatusNotifierCategoryApplicationStatus:
			return "ApplicationStatus";
		case BcStatusNotifierCategoryCommunications:
			return "Communications";
		case BcStatusNotifierCategorySystemService:
			return "SystemServices";
		case BcStatusNotifierCategoryHardware:
			return "Hardware";
	}
	return "bad category";
}

const gchar *bc_status_notifier_status_to_string(BcStatusNotifierStatus s) {
	switch(s){
		case BcStatusNotifierStatusPassive:
			return "Passive";
		case BcStatusNotifierStatusActive:
			return "Active";
		case BcStatusNotifierStatusNeedsAttention:
			return "NeedsAttention";
	}
	return "badstatus";
};


struct _BcStatusNotifierToolTip {
	char *icon_name;
	char *title;
	char *text;
	int ref;
};

BcStatusNotifierToolTip *bc_status_notifier_tool_tip_new(const char *icon_name, const char *title, const char *text) {
	BcStatusNotifierToolTip *obj = (BcStatusNotifierToolTip *)g_new0(BcStatusNotifierToolTip, 1);
	if(icon_name) obj->icon_name = g_strdup(icon_name);
	if(title) obj->title = g_strdup(title);
	if(text) obj->text = g_strdup(text);
	return obj;
}

BcStatusNotifierToolTip *bc_status_notifier_tool_tip_ref(BcStatusNotifierToolTip *obj) {
	obj->ref++;
	return obj;
}

void bc_status_notifier_tool_tip_unref(BcStatusNotifierToolTip *obj) {
	obj->ref--;
	if(obj->ref < 0) {
		if(obj->icon_name) g_free(obj->icon_name);
		if(obj->title) g_free(obj->title);
		if(obj->text) g_free(obj->text);
		g_free(obj);
	}
}

static GVariant *_bc_status_notifier_tool_tip_to_variant(const BcStatusNotifierToolTip *obj) {
	GVariant *attr[] = {
		g_variant_new_string(obj->icon_name ? obj->icon_name : ""),
		g_variant_new_array(G_VARIANT_TYPE_VARIANT, NULL, 0),
		g_variant_new_string(obj->title ? obj->title : ""),
		g_variant_new_string(obj->text ? obj->text : ""),
	};
	return g_variant_new_tuple(attr, 4);
}


static const char *_bc_status_notifier_to_string[] = {
	"vertical",
	"horizontal",
	NULL
};

static BcStatusNotifierOrientation _bc_status_notifier_orientation_from_string(const char *s) {
	int i;
	for(i=0; _bc_status_notifier_to_string[i] && g_strcmp0(s, _bc_status_notifier_to_string[i]) == 0; i++);
	if(_bc_status_notifier_to_string[i]) return i;
	else return BcStatusNotifierOrientationVertical;
}


struct _BcStatusNotifierParams{
	char *prefix;
	int item_id;
	BcStatusNotifierCategory category;
	char *id;
	char *title;
	BcStatusNotifierStatus status;
	guint32 window_id;
	char *icon_name;
	char *overlay_icon_name;
	char *attention_icon_name;
	char *attention_movie_name;
	BcStatusNotifierToolTip *tool_tip;
	BcStatusNotifierSignalsVTable vtable;
	void *user_data;
	int ref;
};

#define DEFAULT_PREFIX "org.freedesktop"

BcStatusNotifierParams *bc_status_notifier_params_new(void) {
	BcStatusNotifierParams *obj = (BcStatusNotifierParams *)g_new0(BcStatusNotifierParams, 1);
	obj->prefix = g_strdup(DEFAULT_PREFIX);
	return obj;
}

BcStatusNotifierParams *bc_status_notifier_params_ref(BcStatusNotifierParams *obj) {
	obj->ref++;
	return obj;
}

void bc_status_notifier_params_unref(BcStatusNotifierParams *obj) {
	obj->ref--;
	if(obj->ref < 0) {
		if(obj->prefix) g_free(obj->prefix);
		if(obj->id) g_free(obj->id);
		if(obj->title) g_free(obj->title);
		if(obj->icon_name) g_free(obj->icon_name);
		if(obj->overlay_icon_name) g_free(obj->overlay_icon_name);
		if(obj->attention_icon_name) g_free(obj->attention_icon_name);
		if(obj->attention_movie_name) g_free(obj->attention_movie_name);
		if(obj->tool_tip) bc_status_notifier_tool_tip_unref(obj->tool_tip);
		g_free(obj);
	}
}

void bc_status_notifier_params_set_dbus_prefix(BcStatusNotifierParams *obj, const char *prefix) {
	if(obj->prefix) g_free(obj->prefix);
	if(prefix) obj->prefix = g_strdup(prefix);
	else obj->prefix = NULL;
}

const char *bc_satus_notifier_params_get_dbus_prefix(const BcStatusNotifierParams *obj) {
	return obj->prefix;
}

void bc_status_notifier_params_set_item_id(BcStatusNotifierParams *obj, int item_id) {
	obj->item_id = item_id;
}

int bc_status_notifier_params_get_item_id(const BcStatusNotifierParams *obj) {
	return obj->item_id;
}

void bc_status_notifier_params_set_category(BcStatusNotifierParams *obj, BcStatusNotifierCategory category) {
	obj->category = category;
}

BcStatusNotifierCategory bc_status_notifier_params_get_category(const BcStatusNotifierParams *obj) {
	return obj->category;
}

void bc_status_notifier_params_set_id(BcStatusNotifierParams *obj, const char *id) {
	if(obj->id) g_free(obj->id);
	if(id) obj->id = g_strdup(id);
	else obj->id = NULL;
}

const char *bc_status_notifier_params_get_id(const BcStatusNotifierParams *obj) {
	return obj->id;
}

void bc_status_notifier_params_set_title(BcStatusNotifierParams *obj, const char *title) {
	if(obj->title) g_free(obj->title);
	if(title) obj->title = g_strdup(title);
	else obj->title = NULL;
}

const char *bc_status_notifier_params_get_title(const BcStatusNotifierParams *obj) {
	return obj->title;
}

void bc_status_notifier_params_set_status(BcStatusNotifierParams *obj, BcStatusNotifierStatus status) {
	obj->status = status;
}

BcStatusNotifierStatus bc_status_notifier_params_get_status(const BcStatusNotifierParams *obj) {
	return obj->status;
}

void bc_status_notifier_params_set_window_id(BcStatusNotifierParams *obj, guint32 window_id) {
	obj->window_id = window_id;
}

guint32 bc_status_notifier_params_get_window_id(const BcStatusNotifierParams *obj) {
	return obj->window_id;
}

void bc_status_notifier_params_set_icon_name(BcStatusNotifierParams *obj, const char *name) {
	if(obj->icon_name) g_free(obj->icon_name);
	if(name) obj->icon_name = g_strdup(name);
	else obj->icon_name = NULL;
}

const char *bc_status_notifier_params_get_icon_name(const BcStatusNotifierParams *obj) {
	return obj->icon_name;
}

void bc_status_notifier_params_set_overlay_icon_name(BcStatusNotifierParams *obj, const char *name) {
	if(obj->overlay_icon_name) g_free(obj->overlay_icon_name);
	if(name) obj->overlay_icon_name = g_strdup(name);
	else obj->overlay_icon_name = NULL;
}

const char *bc_status_notifier_params_get_overlay_icon_name(const BcStatusNotifierParams *obj) {
	return obj->overlay_icon_name;
}

void bc_status_notifier_params_set_attention_icon_name(BcStatusNotifierParams *obj, const char *name) {
	if(obj->attention_icon_name) g_free(obj->attention_icon_name);
	if(name) obj->attention_icon_name = g_strdup(name);
	else obj->attention_icon_name = NULL;
}

const char *bc_status_notifier_params_get_attention_icon_name(const BcStatusNotifierParams *obj) {
	return obj->attention_icon_name;
}

void bc_status_notifier_params_set_attention_movie_name(BcStatusNotifierParams *obj, const char *name) {
	if(obj->attention_movie_name) g_free(obj->attention_movie_name);
	if(name) obj->attention_movie_name = g_strdup(name);
	else obj->attention_movie_name = NULL;
}

const char *bc_status_notifier_params_get_attention_movie_name(const BcStatusNotifierParams *obj) {
	return obj->attention_movie_name;
}

void bc_status_notifier_params_set_tool_tip(BcStatusNotifierParams *obj, BcStatusNotifierToolTip *tool_tip) {
	if(obj->tool_tip) bc_status_notifier_tool_tip_unref(obj->tool_tip);
	if(tool_tip) obj->tool_tip = bc_status_notifier_tool_tip_ref(tool_tip);
	else obj->tool_tip = NULL;
}

const BcStatusNotifierToolTip *bc_status_notifier_params_get_tool_tip(const BcStatusNotifierParams *obj) {
	return obj->tool_tip;
}

void bc_status_notifier_params_set_vtable(BcStatusNotifierParams *obj, const BcStatusNotifierSignalsVTable *vtable, void *user_data) {
	obj->vtable = *vtable;
	obj->user_data = user_data;
}


struct _BcStatusNotifier {
	BcStatusNotifierParams *params;
	guint bus_owner_id;
	GDBusConnection *conn;
	BcStatusNotifierState state;
	BcStatusNotifierStateVTable vtable;
	void *user_data;
	int ref;
};

#define ITEM_NAME "StatusNotifierItem"
#define WATCHER_NAME "StatusNotifierWatcher"
#define CALL_TIMEOUT 1000

#define STATUS_NOTIFIER_INTROSPECTION_DATA \
	"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" \
	\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\"> \
		<node name=\"/StatusNotifierItem\"> \
			<interface name=\"org.kde.StatusNotifierItem\"> \
				<property name=\"Category\" type=\"s\" access=\"read\"/> \
				<property name=\"Id\" type=\"s\" access=\"read\"/> \
				<property name=\"Title\" type=\"s\" access=\"read\"/> \
				<property name=\"Status\" type=\"s\" access=\"read\"/> \
				<property name=\"WindowId\" type=\"u\" access=\"read\"/> \
				<property name=\"IconName\" type=\"s\" access=\"read\"/> \
				<property name=\"IconPixmap\" type=\"a(iiay)\" access=\"read\"/> \
				<property name=\"OverlayIconName\" type=\"s\" access=\"read\"/> \
				<property name=\"OverlayIconPixmap\" type=\"a(iiay)\" access=\"read\"/> \
				<property name=\"AttentionIconName\" type=\"s\" access=\"read\"/> \
				<property name=\"AttentionIconPixmap\" type=\"a(iiay)\" access=\"read\"/> \
				<property name=\"AttentionMovieName\" type=\"s\" access=\"read\"/> \
				<property name=\"ToolTip\" type=\"(sa(iiay)ss)\" access=\"read\"/> \
				<method name=\"ContextMenu\"> \
					<arg name=\"x\" type=\"i\" direction=\"in\" /> \
					<arg name=\"y\" type=\"i\" direction=\"in\" /> \
				</method> \
				<method name=\"Activate\"> \
					<arg name=\"x\" type=\"i\" direction=\"in\" /> \
					<arg name=\"y\" type=\"i\" direction=\"in\" /> \
				</method> \
				<method name=\"SecondaryActivate\"> \
					<arg name=\"x\" type=\"i\" direction=\"in\" /> \
					<arg name=\"y\" type=\"i\" direction=\"in\" /> \
				</method> \
				<method name=\"Scroll\"> \
					<arg name=\"delta\" type=\"i\" direction=\"in\" /> \
					<arg name=\"orientation\" type=\"s\" direction=\"in\" /> \
				</method> \
				<signal name=\"NewTitle\" /> \
				<signal name=\"NewIcon\" /> \
				<signal name=\"NewAttentionIcon\" /> \
				<signal name=\"NewOverlayIcon\" /> \
				<signal name=\"NewToolTip\" /> \
				<signal name=\"NewStatus\"> \
					<arg name=\"status\" type=\"s\" /> \
				</signal> \
			</interface> \
		</node>"

BcStatusNotifier *bc_status_notifier_new(void) {
	return (BcStatusNotifier *)g_new0(BcStatusNotifier, 1);
}

BcStatusNotifier *bc_status_notifier_ref(BcStatusNotifier *obj) {
	obj->ref++;
	return obj;
}

void bc_status_notifier_unref(BcStatusNotifier *obj) {
	obj->ref--;
	if(obj->ref < 0) {
		bc_status_notifier_stop(obj);
		if(obj->params) bc_status_notifier_params_unref(obj->params);
		g_free(obj);
	}
}

static GVariant *_bc_status_notifier_get_property_cb(
		GDBusConnection *connection,
		const gchar *sender,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *property_name,
		GError **error,
		BcStatusNotifier *sn) {
	
	
	GVariant *value = NULL;
	if(g_strcmp0(property_name, "Category") == 0) {
		value = g_variant_new_string(bc_status_notifier_category_to_string(sn->params->category));
	} else if(g_strcmp0(property_name, "Id") == 0) {
		value = g_variant_new_string(sn->params->id ? sn->params->id : "");
	} else if(g_strcmp0(property_name, "Title") == 0) {
		value = g_variant_new_string(sn->params->title ? sn->params->title : "");
	} else if(g_strcmp0(property_name, "Status") == 0) {
		value = g_variant_new_string(bc_status_notifier_status_to_string(sn->params->status));
	} else if(g_strcmp0(property_name, "WindowId") == 0) {
		value = g_variant_new_uint32(sn->params->window_id);
	} else if(g_strcmp0(property_name, "IconName") == 0) {
		value = g_variant_new_string(sn->params->icon_name ? sn->params->icon_name : "");
	} else if(g_strcmp0(property_name, "IconPixmap") == 0) {
		value = g_variant_new_array(G_VARIANT_TYPE_VARIANT, NULL, 0);
	} else if(g_strcmp0(property_name, "OverlayIconName") == 0) {
		value = g_variant_new_string(sn->params->overlay_icon_name ? sn->params->overlay_icon_name : "");
	} else if(g_strcmp0(property_name, "OverlayIconPixmap") == 0) {
		value = g_variant_new_array(G_VARIANT_TYPE_VARIANT, NULL, 0);
	} else if(g_strcmp0(property_name, "AttentionIconName") == 0) {
		value = g_variant_new_string(sn->params->attention_icon_name ? sn->params->attention_icon_name : "");
	} else if(g_strcmp0(property_name, "AttentionIconPixmap") == 0) {
		value = g_variant_new_array(G_VARIANT_TYPE_VARIANT, NULL, 0);
	} else if(g_strcmp0(property_name, "AttentionMovieName") == 0) {
		value = g_variant_new_string(sn->params->attention_movie_name ? sn->params->attention_movie_name : "");
	} else if(g_strcmp0(property_name, "ToolTip") == 0) {
		if(sn->params->tool_tip) {
			value = _bc_status_notifier_tool_tip_to_variant(sn->params->tool_tip);
		} else {
			BcStatusNotifierToolTip *tool_tip = bc_status_notifier_tool_tip_new("", "", "");
			value = _bc_status_notifier_tool_tip_to_variant(tool_tip);
			bc_status_notifier_tool_tip_unref(tool_tip);
		}
	}
	return value;
}

static void _bc_status_notifier_method_call_cb(
		GDBusConnection *connection,
		const gchar *sender,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *method_name,
		GVariant *parameters,
		GDBusMethodInvocation *invocation,
		BcStatusNotifier *sn) {
	
	if(g_strcmp0(method_name, "ContextMenu") == 0) {
		if(sn->params->vtable.context_menu_called_cb) {
			int x, y;
			g_variant_get_child(parameters, 0, "i", &x);
			g_variant_get_child(parameters, 1, "i", &y);
			sn->params->vtable.context_menu_called_cb(sn, x, y, sn->params->user_data);
		}
	} else if(g_strcmp0(method_name, "Activate") == 0) {
		if(sn->params->vtable.activate_called_cb) {
			int x, y;
			g_variant_get_child(parameters, 0, "i", &x);
			g_variant_get_child(parameters, 1, "i", &y);
			sn->params->vtable.activate_called_cb(sn, x, y, sn->params->user_data);
		}
	} else if(g_strcmp0(method_name, "SecondaryActivate") == 0) {
		if(sn->params->vtable.secondary_activate_called_cb) {
			int x, y;
			g_variant_get_child(parameters, 0, "i", &x);
			g_variant_get_child(parameters, 1, "i", &y);
			sn->params->vtable.secondary_activate_called_cb(sn, x, y, sn->params->user_data);
		}
	} else if(g_strcmp0(method_name, "Scroll") == 0) {
		if(sn->params->vtable.scroll_called_cb) {
			int delta;
			BcStatusNotifierOrientation orient;
			char *orient_str;
			g_variant_get_child(parameters, 0, "i", &delta);
			g_variant_get_child(parameters, 1, "&s", &orient_str);
			orient = _bc_status_notifier_orientation_from_string(orient_str);
			sn->params->vtable.scroll_called_cb(sn, delta, orient, sn->params->user_data);
		}
	}
	g_dbus_method_invocation_return_value(invocation, NULL);
}

static void _bc_status_notifier_bus_acquired_cb(GDBusConnection *conn, const gchar *name, BcStatusNotifier *sn) {
	char *interface_name = g_strdup_printf("%s.%s", sn->params->prefix, ITEM_NAME);
	char *item_path = g_strdup_printf("/%s", ITEM_NAME);
	GDBusInterfaceVTable vtable;
	GDBusNodeInfo *node_info = g_dbus_node_info_new_for_xml(STATUS_NOTIFIER_INTROSPECTION_DATA, NULL);
	GDBusInterfaceInfo *interface = g_dbus_node_info_lookup_interface(
		node_info,
		interface_name
	);

	memset(&vtable, 0, sizeof(vtable));
	vtable.method_call = (GDBusInterfaceMethodCallFunc)_bc_status_notifier_method_call_cb;
	vtable.get_property = (GDBusInterfaceGetPropertyFunc)_bc_status_notifier_get_property_cb;
	g_free(interface_name);
	
	sn->conn = conn;
	
	g_dbus_connection_register_object(
		conn,
		item_path,
		interface,
		&vtable,
		bc_status_notifier_ref(sn),
		(GDestroyNotify)bc_status_notifier_unref,
		NULL
	);
	g_free(item_path);
	
	g_dbus_node_info_unref(node_info);
}

static void _bc_status_notifier_name_acquired_cb(GDBusConnection *conn, const gchar *name, BcStatusNotifier *sn) {
	GVariant *item_name = g_variant_new_string(name); 
	GVariant *parameters = g_variant_new_tuple(&item_name, 1);
	char *watcher_bus_name = g_strdup_printf("%s.%s", sn->params->prefix, WATCHER_NAME);
	char *watcher_interface_name = watcher_bus_name;
	char *watcher_path = g_strdup_printf("/%s", WATCHER_NAME);
	
	g_dbus_connection_call(
		conn,
		watcher_bus_name,
		watcher_path,
		watcher_interface_name,
		"RegisterStatusNotifierItem",
		parameters,
		NULL,
		G_DBUS_CALL_FLAGS_NONE,
		CALL_TIMEOUT,
		NULL,
		NULL,
		NULL
	);
	g_free(watcher_bus_name);
	g_free(watcher_path);
	
	sn->state = BcStatusNotifierStateRunning;
	if(sn->vtable.success) sn->vtable.success(sn, sn->user_data);
}

static void _bc_status_notifier_name_lost(GDBusConnection *conn, const gchar *name, BcStatusNotifier *sn) {
	if(conn == NULL) {
		sn->state = BcStatusNotifierStateStopped;
		if(sn->vtable.fail) sn->vtable.fail(sn, sn->user_data);
	}
}

void bc_status_notifier_start(BcStatusNotifier* obj, BcStatusNotifierParams* params, const BcStatusNotifierStateVTable *vtable, void *user_data) {
	if(obj->state == BcStatusNotifierStateStopped) {
		lppid_t pid = getpid();
		char *dbus_name = g_strdup_printf("%s.%s-%d-%d", params->prefix, ITEM_NAME, pid, params->item_id);
		
		if(obj->params) bc_status_notifier_params_unref(obj->params);
		obj->params = bc_status_notifier_params_ref(params);
		if(vtable) obj->vtable = *vtable;
		else {
			obj->vtable.success = NULL;
			obj->vtable.fail = NULL;
		}
		obj->user_data = user_data;
		obj->state = BcStatusNotifierStateStarting;
		obj->bus_owner_id = g_bus_own_name(
			G_BUS_TYPE_SESSION,
			dbus_name,
			G_BUS_NAME_OWNER_FLAGS_NONE,
			(GBusAcquiredCallback)_bc_status_notifier_bus_acquired_cb,
			(GBusNameAcquiredCallback)_bc_status_notifier_name_acquired_cb,
			(GBusNameLostCallback)_bc_status_notifier_name_lost,
			bc_status_notifier_ref(obj),
			(GDestroyNotify)bc_status_notifier_unref
		);
		g_free(dbus_name);
	}
}

void bc_status_notifier_stop(BcStatusNotifier *obj) {
	if(obj->state == BcStatusNotifierStateRunning) {
		g_bus_unown_name(obj->bus_owner_id);
		obj->bus_owner_id = 0;
		obj->conn = NULL;
		obj->state = BcStatusNotifierStateStopped;
	}
}

const BcStatusNotifierParams *bc_status_notifier_get_params(const BcStatusNotifier *obj) {
	return obj->params;
}

static void _bc_status_notifier_emit_signal(const BcStatusNotifier *obj, const char *sig_name, GVariant *parameters) {
	char *item_interface_name = g_strdup_printf("%s.%s", obj->params->prefix, ITEM_NAME);
	char *item_path = g_strdup_printf("/%s", ITEM_NAME);
	g_dbus_connection_emit_signal(
		obj->conn,
		NULL,
		item_path,
		item_interface_name,
		sig_name,
		parameters,
		NULL
	);
	g_free(item_interface_name);
	g_free(item_path);
}

void bc_status_notifier_update_title(BcStatusNotifier *obj, const char *title) {
	bc_status_notifier_params_set_title(obj->params, title);
	_bc_status_notifier_emit_signal(obj, "NewTitle", NULL);
}

void bc_status_notifier_update_icon(BcStatusNotifier *obj, const char *icon_name) {
	bc_status_notifier_params_set_icon_name(obj->params, icon_name);
	_bc_status_notifier_emit_signal(obj, "NewIcon", NULL);
}

void bc_status_notifier_update_attention_icon(BcStatusNotifier *obj, const char *icon_name) {
	bc_status_notifier_params_set_attention_icon_name(obj->params, icon_name);
	_bc_status_notifier_emit_signal(obj, "NewAttentionIcon", NULL);
}

void bc_status_notifier_update_overlay_icon(BcStatusNotifier *obj, const char *icon_name) {
	bc_status_notifier_params_set_overlay_icon_name(obj->params, icon_name);
	_bc_status_notifier_emit_signal(obj, "NewOverlayIcon", NULL);
}

void bc_status_notifier_update_tool_tip(BcStatusNotifier *obj, BcStatusNotifierToolTip *tool_tip) {
	bc_status_notifier_params_set_tool_tip(obj->params, tool_tip);
	_bc_status_notifier_emit_signal(obj, "NewToolTip", NULL);
}

void bc_status_notifier_update_status(BcStatusNotifier *obj, BcStatusNotifierStatus status) {
	GVariant *status_var = g_variant_new_string(bc_status_notifier_status_to_string(status));
	GVariant *parameter = g_variant_new_tuple(&status_var, 1);
	bc_status_notifier_params_set_status(obj->params, status);
	_bc_status_notifier_emit_signal(obj, "NewStatus", parameter);
}


typedef struct _BcWatcherDetectionCtx {
	char *prefix;
	guint watcher_id;
	BcStatusNotifierSupportDetectionCb cb;
	void *user_data;
} BcWatcherDetectionCtx;

static void _bc_watcher_detection_ctx_free(BcWatcherDetectionCtx *obj) {
	g_free(obj->prefix);
	g_free(obj);
}

static void _bc_watcher_name_appeared_cb(GDBusConnection *conn, const char *name, const char *name_owner, BcWatcherDetectionCtx *ctx) {
	g_bus_unwatch_name(ctx->watcher_id);
	if(ctx->cb) ctx->cb(ctx->prefix, 1, ctx->user_data);
}

static void _bc_watcher_name_vannished_cb(GDBusConnection *conn, const char *name, BcWatcherDetectionCtx *ctx) {
	g_bus_unwatch_name(ctx->watcher_id);
	if(ctx->cb) ctx->cb(ctx->prefix, 0, ctx->user_data);
}

void bc_status_notifier_is_supported(const char* prefix, BcStatusNotifierSupportDetectionCb cb, void *user_data) {
	char *bus_name = g_strdup_printf("%s.%s", prefix, WATCHER_NAME);
	BcWatcherDetectionCtx *ctx = (BcWatcherDetectionCtx *)g_new0(BcWatcherDetectionCtx, 1);
	ctx->prefix = g_strdup(prefix);
	ctx->cb = cb;
	ctx->user_data = user_data;
	
	ctx->watcher_id = g_bus_watch_name(
		G_BUS_TYPE_SESSION,
		bus_name,
		G_BUS_NAME_WATCHER_FLAGS_NONE,
		(GBusNameAppearedCallback)_bc_watcher_name_appeared_cb,
		(GBusNameVanishedCallback)_bc_watcher_name_vannished_cb,
		ctx,
		(GDestroyNotify)_bc_watcher_detection_ctx_free
	);
	g_free(bus_name);
}
