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

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define PACKAGE_DATA_DIR "./"

#ifndef LINPHONE_VERSION
#define LINPHONE_VERSION "3.0.0-20090112"
#endif

#endif

#include <gtk/gtk.h>
#include "linphonecore.h"

#include <libintl.h>
#ifndef _
#define _(String) gettext (String)
#endif

GdkPixbuf * create_pixbuf(const gchar *filename);
void add_pixmap_directory(const gchar *directory);
GtkWidget *linphone_gtk_create_window(const char *window_name);
GtkWidget *linphone_gtk_get_widget(GtkWidget *window, const char *name);
LinphoneCore *linphone_gtk_get_core(void);
GtkWidget *linphone_gtk_get_main_window();
void linphone_gtk_display_something(GtkMessageType type,const gchar *message);
void linphone_gtk_start_call(GtkWidget *button);
void linphone_gtk_show_friends(void);
void linphone_gtk_show_contact(LinphoneFriend *lf);
void linphone_gtk_set_my_presence(LinphoneOnlineStatus ss);
void linphone_gtk_show_parameters(void);
void linphone_gtk_load_identities(void);
void linphone_gtk_create_chatroom(const char *with);
void linphone_gtk_text_received(LinphoneCore *lc, LinphoneChatRoom *room, const char *from, const char *message);
void linphone_gtk_call_log_update(GtkWidget *w);
void linphone_gtk_create_log_window(void);
void linphone_gtk_log_show(void);
void linphone_gtk_log_push(OrtpLogLevel lev, const char *fmt, va_list args);
void linphone_gtk_destroy_log_window(void);
gboolean linphone_gtk_check_logs();
const gchar *linphone_gtk_get_ui_config(const char *key, const char *def);
int linphone_gtk_get_ui_config_int(const char *key, int def);
void linphone_gtk_open_browser(const char *url);
void linphone_gtk_check_for_new_version(void);
const char *linphone_gtk_get_lang(const char *config_file);

