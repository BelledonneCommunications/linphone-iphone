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

/**
 * BcStatusNotifier is an implementation of the StatusNotiferItem standard defined by freedesktop.org.
 * It is a new way to manage status icons on GNU/Linux systems by using D-Bus. It is implemented by
 * Unity desktop environmemt and it is the only way to create status icons on KDE 5.
 * Visit http://freedesktop.org/wiki/Specifications/StatusNotifierItem/ for more information.
 */

#ifndef STATUS_NOTIFIER_H
#define STATUS_NOTIFIER_H

#include <glib.h>

struct _BcStatusNotifier;


typedef enum {
	BcStatusNotifierCategoryApplicationStatus,
	BcStatusNotifierCategoryCommunications,
	BcStatusNotifierCategorySystemService,
	BcStatusNotifierCategoryHardware
} BcStatusNotifierCategory;

const gchar *bc_status_notifier_category_to_string(BcStatusNotifierCategory c);


typedef enum {
	BcStatusNotifierStatusPassive,
	BcStatusNotifierStatusActive,
	BcStatusNotifierStatusNeedsAttention
} BcStatusNotifierStatus;

const gchar *bc_status_notifier_status_to_string(BcStatusNotifierStatus s);


typedef struct _BcStatusNotifierToolTip BcStatusNotifierToolTip;

BcStatusNotifierToolTip *bc_status_notifier_tool_tip_new(const char *icon_name, const char *title, const char *text);
BcStatusNotifierToolTip *bc_status_notifier_tool_tip_ref(BcStatusNotifierToolTip *obj);
void bc_status_notifier_tool_tip_unref(BcStatusNotifierToolTip *obj);


typedef enum _BcStatusNotifierOrientation {
	BcStatusNotifierOrientationVertical,
	BcStatusNotifierOrientationHorizontal
} BcStatusNotifierOrientation;


typedef void (*BcStatusNotifierContextMenuCalledCb)(struct _BcStatusNotifier *sn, int x, int y, void *user_data);
typedef void (*BcStatusNotifierActivateCalledCb)(struct _BcStatusNotifier *sn, int x, int y, void *user_data);
typedef void (*BcStatusNotifierSecondaryActivateCb)(struct _BcStatusNotifier *sn, int x, int y, void *user_data);
typedef void (*BcStatusNotifierScrollCalledCb)(struct _BcStatusNotifier *sn, int delta, BcStatusNotifierOrientation o, void *user_data);

typedef struct _BcStatusNotifierSignalsVTable {
	BcStatusNotifierContextMenuCalledCb context_menu_called_cb;
	BcStatusNotifierActivateCalledCb activate_called_cb;
	BcStatusNotifierSecondaryActivateCb secondary_activate_called_cb;
	BcStatusNotifierScrollCalledCb scroll_called_cb;
} BcStatusNotifierSignalsVTable;


typedef struct _BcStatusNotifierParams BcStatusNotifierParams;

BcStatusNotifierParams *bc_status_notifier_params_new(void);
BcStatusNotifierParams *bc_status_notifier_params_ref(BcStatusNotifierParams *obj);
void bc_status_notifier_params_unref(BcStatusNotifierParams *obj);

void bc_status_notifier_params_set_dbus_prefix(BcStatusNotifierParams *obj, const char *prefix);
const char *bc_satus_notifier_params_get_dbus_prefix(const BcStatusNotifierParams *obj);

void bc_status_notifier_params_set_item_id(BcStatusNotifierParams *obj, int item_id);
int bc_status_notifier_params_get_item_id(const BcStatusNotifierParams *obj);

void bc_status_notifier_params_set_category(BcStatusNotifierParams *obj, BcStatusNotifierCategory category);
BcStatusNotifierCategory bc_status_notifier_params_get_category(const BcStatusNotifierParams *obj);

void bc_status_notifier_params_set_id(BcStatusNotifierParams *obj, const char *id);
const char *bc_status_notifier_params_get_id(const BcStatusNotifierParams *obj);

void bc_status_notifier_params_set_title(BcStatusNotifierParams *obj, const char *title);
const char *bc_status_notifier_params_get_title(const BcStatusNotifierParams *obj);

void bc_status_notifier_params_set_status(BcStatusNotifierParams *obj, BcStatusNotifierStatus status);
BcStatusNotifierStatus bc_status_notifier_params_get_status(const BcStatusNotifierParams *obj);

void bc_status_notifier_params_set_window_id(BcStatusNotifierParams *obj, guint32 window_id);
guint32 bc_status_notifier_params_get_window_id(const BcStatusNotifierParams *obj);

void bc_status_notifier_params_set_icon_name(BcStatusNotifierParams *obj, const char *name);
const char *bc_status_notifier_params_get_icon_name(const BcStatusNotifierParams *obj);

void bc_status_notifier_params_set_overlay_icon_name(BcStatusNotifierParams *obj, const char *name);
const char *bc_status_notifier_params_get_overlay_icon_name(const BcStatusNotifierParams *obj);

void bc_status_notifier_params_set_attention_icon_name(BcStatusNotifierParams *obj, const char *icon_name);
const char *bc_status_notifier_params_get_attention_icon_name(const BcStatusNotifierParams *obj);

void bc_status_notifier_params_set_attention_movie_name(BcStatusNotifierParams *obj, const char *name);
const char *bc_status_notifier_params_get_attention_movie_name(const BcStatusNotifierParams *obj);

void bc_status_notifier_params_set_tool_tip(BcStatusNotifierParams *obj, BcStatusNotifierToolTip *tool_tip);
const BcStatusNotifierToolTip *bc_status_notifier_params_get_tool_tip(const BcStatusNotifierParams *obj);

void bc_status_notifier_params_set_vtable(BcStatusNotifierParams *obj, const BcStatusNotifierSignalsVTable *vtable, void *user_data);


typedef enum _BcStatusNotifierState {
	BcStatusNotifierStateStopped,
	BcStatusNotifierStateStarting,
	BcStatusNotifierStateRunning
} BcStatusNotifierState;


typedef void (*BcStatusNotifierStartedCb)(struct _BcStatusNotifier *sn, void *user_data);
typedef void (*BcStatusNotifierStartingFailedCb)(struct _BcStatusNotifier *sn, void *user_data);

typedef struct _BcStatusNotifierStateVTable {
	BcStatusNotifierStartedCb success;
	BcStatusNotifierStartingFailedCb fail;
} BcStatusNotifierStateVTable;


typedef struct _BcStatusNotifier BcStatusNotifier;

BcStatusNotifier *bc_status_notifier_new(void);
BcStatusNotifier *bc_status_notifier_ref(BcStatusNotifier *obj);
void bc_status_notifier_unref(BcStatusNotifier *obj);

void bc_status_notifier_start(BcStatusNotifier* obj, BcStatusNotifierParams* params, const BcStatusNotifierStateVTable* vtable, void* user_data);
void bc_status_notifier_stop(BcStatusNotifier* obj);

const BcStatusNotifierParams *bc_status_notifier_get_params(const BcStatusNotifier *obj);
void bc_status_notifier_update_title(BcStatusNotifier* obj, const char* title);
void bc_status_notifier_update_icon(BcStatusNotifier* obj, const char* icon_name);
void bc_status_notifier_update_attention_icon(BcStatusNotifier* obj, const char* icon_name);
void bc_status_notifier_update_overlay_icon(BcStatusNotifier* obj, const char* icon_name);
void bc_status_notifier_update_tool_tip(BcStatusNotifier* obj, BcStatusNotifierToolTip* tool_tip);
void bc_status_notifier_update_status(BcStatusNotifier* obj, BcStatusNotifierStatus status);


typedef void (*BcStatusNotifierSupportDetectionCb)(const char *prefix, gboolean is_supported, void *user_data);

void bc_status_notifier_is_supported(const char* prefix, BcStatusNotifierSupportDetectionCb cb, void *user_data);

#endif