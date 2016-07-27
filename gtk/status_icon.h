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

/*
 * LinphoneStatusIcon is an singleton interface which abstracts the
 * technology used to manage the status icon.
 */

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic push
#endif
#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#endif

#include <glib.h>
#include <gtk/gtk.h>

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif


struct _LinphoneStatusIcon;
typedef void (*LinphoneStatusIconOnClickCallback)(struct _LinphoneStatusIcon *si, void *user_data);


typedef struct _LinphoneStatusIconParams LinphoneStatusIconParams;

LinphoneStatusIconParams *linphone_status_icon_params_new(void);
LinphoneStatusIconParams *linphone_status_icon_params_ref(LinphoneStatusIconParams *obj);
void linphone_status_icon_params_unref(LinphoneStatusIconParams *obj);

void linphone_status_icon_params_set_title(LinphoneStatusIconParams *obj, const char *title);
void linphone_status_icon_params_set_description(LinphoneStatusIconParams *obj, const char *desc);
void linphone_status_icon_params_set_menu(LinphoneStatusIconParams *obj, GtkWidget *menu);
void linphone_status_icon_params_set_on_click_cb(LinphoneStatusIconParams* obj, LinphoneStatusIconOnClickCallback cb, void *user_data);


typedef void (*LinphoneStatusIconReadyCb)(void *user_data);

typedef struct _LinphoneStatusIcon LinphoneStatusIcon;

gboolean linphone_status_icon_init(LinphoneStatusIconReadyCb ready_cb, void* user_data);
void linphone_status_icon_uninit(void);
LinphoneStatusIcon *linphone_status_icon_get(void);
const char *linphone_status_icon_get_implementation_name(const LinphoneStatusIcon *obj);
void linphone_status_icon_start(LinphoneStatusIcon *obj, LinphoneStatusIconParams *params);
void linphone_status_icon_enable_blinking(LinphoneStatusIcon *obj, gboolean enable);
