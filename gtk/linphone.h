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
#endif

#include <gtk/gtk.h>
#ifdef WIN32
// alloca is already defined by gtk
#undef alloca
#endif
#include "linphonecore.h"

#include <libintl.h>
#ifndef _
#define _(String) gettext (String)
#endif

#ifdef USE_BUILDDATE_VERSION
#include "version_date.h"
#undef LINPHONE_VERSION
#define LINPHONE_VERSION LINPHONE_VERSION_DATE
#endif

GdkPixbuf * create_pixbuf(const gchar *filename);
GdkPixbufAnimation *create_pixbuf_animation(const gchar *filename);
void add_pixmap_directory(const gchar *directory);
GtkWidget*create_pixmap(const gchar     *filename);
GtkWidget *_gtk_image_new_from_memory_at_scale(const void *data, gint len, gint w, gint h, gboolean preserve_ratio);
GdkPixbuf *_gdk_pixbuf_new_from_memory_at_scale(const void *data, gint len, gint w, gint h, gboolean preserve_ratio);

GtkWidget *linphone_gtk_create_window(const char *window_name);
GtkWidget *linphone_gtk_get_widget(GtkWidget *window, const char *name);
GtkWidget *linphone_gtk_create_widget(const char *filename, const char *widget_name);

LinphoneCore *linphone_gtk_get_core(void);
GtkWidget *linphone_gtk_get_main_window();
void linphone_gtk_display_something(GtkMessageType type,const gchar *message);
void linphone_gtk_start_call(GtkWidget *button);
void linphone_gtk_call_terminated();
void linphone_gtk_show_friends(void);
void linphone_gtk_show_contact(LinphoneFriend *lf);
void linphone_gtk_set_my_presence(LinphoneOnlineStatus ss);
void linphone_gtk_show_parameters(void);
void linphone_gtk_load_identities(void);
void linphone_gtk_create_chatroom(const char *with);
void linphone_gtk_text_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from, const char *message);
void linphone_gtk_call_log_update(GtkWidget *w);
void linphone_gtk_create_log_window(void);
void linphone_gtk_log_show(void);
void linphone_gtk_log_push(OrtpLogLevel lev, const char *fmt, va_list args);
void linphone_gtk_destroy_log_window(void);
gboolean linphone_gtk_check_logs();
void linphone_gtk_buddy_info_updated(LinphoneCore *lc, LinphoneFriend *lf);
const gchar *linphone_gtk_get_ui_config(const char *key, const char *def);
int linphone_gtk_get_ui_config_int(const char *key, int def);
void linphone_gtk_set_ui_config_int(const char *key , int val);
void linphone_gtk_visibility_set(const char *hiddens, const char *window_name, GtkWidget *w, gboolean show);

void linphone_gtk_open_browser(const char *url);
void linphone_gtk_check_for_new_version(void);
const char *linphone_gtk_get_lang(const char *config_file);
void linphone_gtk_set_lang(const char *code);
SipSetupContext* linphone_gtk_get_default_sip_setup_context(void);
GtkWidget * linphone_gtk_show_buddy_lookup_window(SipSetupContext *ctx);
void linphone_gtk_buddy_lookup_set_keyword(GtkWidget *w, const char *kw);
void * linphone_gtk_wait(LinphoneCore *lc, void *ctx, LinphoneWaitingState ws, const char *purpose, float progress);

void linphone_gtk_show_directory_search(void);

/*functions controlling the different views*/
gboolean linphone_gtk_use_in_call_view();
LinphoneCall *linphone_gtk_get_currently_displayed_call();
void linphone_gtk_create_in_call_view(LinphoneCall *call);
void linphone_gtk_in_call_view_set_calling(LinphoneCall *call);
void linphone_gtk_in_call_view_set_in_call(LinphoneCall *call);
void linphone_gtk_in_call_view_update_duration(LinphoneCall *call);
void linphone_gtk_in_call_view_terminate(LinphoneCall *call, const char *error_msg);
void linphone_gtk_in_call_view_set_incoming(LinphoneCall *call, bool_t with_pause);
void linphone_gtk_in_call_view_set_paused(LinphoneCall *call);
void linphone_gtk_enable_mute_button(GtkButton *button, gboolean sensitive);
void linphone_gtk_enable_hold_button(LinphoneCall *call, gboolean sensitive, gboolean holdon);

void linphone_gtk_show_login_frame(LinphoneProxyConfig *cfg);
void linphone_gtk_exit_login_frame(void);
void linphone_gtk_set_ui_config(const char *key, const char *value);
