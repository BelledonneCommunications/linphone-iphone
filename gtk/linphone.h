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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic push
#endif
#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#endif

#include <gtk/gtk.h>

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif

#ifdef _WIN32
// alloca is already defined by gtk
#undef alloca
#endif
#include "linphone/core.h"

#include "linphone/ldapprovider.h"

#ifdef ENABLE_NLS

#ifdef _MSC_VER
// prevent libintl.h from re-defining fprintf and vfprintf
#ifndef fprintf
#define fprintf fprintf
#endif
#ifndef vfprintf
#define vfprintf vfprintf
#endif
#define _GL_STDIO_H
#endif

# include <libintl.h>
# undef _
# define _(String) dgettext (GETTEXT_PACKAGE,String)
#else
# define _(String) (String)
# define ngettext(singular,plural,number) ((number>1) ? (plural) : (singular) )
#endif // ENABLE_NLS

#undef N_
#define N_(str) (str)

#ifdef USE_BUILDDATE_VERSION
#include "version_date.h"
#undef LINPHONE_VERSION
#define LINPHONE_VERSION LINPHONE_VERSION_DATE
#endif

#include "setupwizard.h"

#define LINPHONE_ICON "linphone.png"
#define LINPHONE_ICON_NAME "linphone"

enum {
	COMPLETION_HISTORY,
	COMPLETION_LDAP
};

typedef float (*get_volume_t)(void *data);

typedef struct _volume_ctx{
	GtkWidget *widget;
	get_volume_t get_volume;
	void *data;
	float last_value;
}volume_ctx_t;

typedef enum {
	CAP_IGNORE,
	CAP_PLAYBACK,
	CAP_CAPTURE
}DeviceCap;

enum {
	START_LINPHONE,
	START_AUDIO_ASSISTANT,
	START_LINPHONE_WITH_CALL
};

GdkPixbuf * create_pixbuf(const gchar *filename);
GdkPixbufAnimation *create_pixbuf_animation(const gchar *filename);
void add_pixmap_directory(const gchar *directory);
GtkWidget*create_pixmap(const gchar     *filename);
GtkWidget *_gtk_image_new_from_memory_at_scale(const void *data, gint len, gint w, gint h, gboolean preserve_ratio);
GdkPixbuf *_gdk_pixbuf_new_from_memory_at_scale(const void *data, gint len, gint w, gint h, gboolean preserve_ratio);

LINPHONE_PUBLIC void linphone_gtk_destroy_window(GtkWidget *window);
LINPHONE_PUBLIC GtkWidget *linphone_gtk_create_window(const char *window_name, GtkWidget *parent);
LINPHONE_PUBLIC GtkWidget *linphone_gtk_get_widget(GtkWidget *window, const char *name);
LINPHONE_PUBLIC GtkWidget *linphone_gtk_create_widget(const char* widget_name);
LINPHONE_PUBLIC GtkWidget *linphone_gtk_make_tab_header(const gchar *label, const gchar *icon_name, gboolean show_quit_button, GCallback cb, gpointer user_data);

char *linphone_gtk_message_storage_get_db_file(const char *filename);
char *linphone_gtk_call_logs_storage_get_db_file(const char *filename);
char *linphone_gtk_friends_storage_get_db_file(const char* filename);
LINPHONE_PUBLIC void linphone_gtk_close_assistant(void);

LINPHONE_PUBLIC LinphoneCore *linphone_gtk_get_core(void);
LINPHONE_PUBLIC GtkWidget *linphone_gtk_get_main_window(void);
LINPHONE_PUBLIC void linphone_gtk_display_something(GtkMessageType type, const gchar *message);
LINPHONE_PUBLIC void linphone_gtk_call_terminated(LinphoneCall *call, const char *error);
LINPHONE_PUBLIC void linphone_gtk_set_my_presence(LinphoneOnlineStatus ss);
LINPHONE_PUBLIC void linphone_gtk_show_parameters(void);
LINPHONE_PUBLIC void linphone_gtk_fill_soundcards(GtkWidget *pb);
LINPHONE_PUBLIC void linphone_gtk_fill_webcams(GtkWidget *pb);
LINPHONE_PUBLIC void linphone_gtk_load_identities(void);
LINPHONE_PUBLIC void linphone_gtk_call_log_update(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_create_log_window(void);
LINPHONE_PUBLIC void linphone_gtk_log_show(void);
LINPHONE_PUBLIC void linphone_gtk_show_main_window(void);
LINPHONE_PUBLIC void linphone_gtk_log_push(OrtpLogLevel lev, const char *fmt, va_list args);
LINPHONE_PUBLIC void linphone_gtk_destroy_log_window(void);
LINPHONE_PUBLIC void linphone_gtk_refer_received(LinphoneCore *lc, const char *refer_to);
LINPHONE_PUBLIC gboolean linphone_gtk_check_logs(void);
LINPHONE_PUBLIC const gchar *linphone_gtk_get_ui_config(const char *key, const char *def);
LINPHONE_PUBLIC int linphone_gtk_get_ui_config_int(const char *key, int def);
LINPHONE_PUBLIC void linphone_gtk_set_ui_config_int(const char *key, int val);
LINPHONE_PUBLIC void linphone_gtk_visibility_set(const char *hiddens, const char *window_name, GtkWidget *w, gboolean show);

LINPHONE_PUBLIC LinphoneLDAPContactProvider* linphone_gtk_get_ldap(void);
LINPHONE_PUBLIC void linphone_gtk_set_ldap(LinphoneLDAPContactProvider* ldap);
LINPHONE_PUBLIC int linphone_gtk_is_ldap_supported(void);

LINPHONE_PUBLIC void linphone_gtk_open_browser(const char *url);
LINPHONE_PUBLIC void linphone_gtk_check_for_new_version(void);
LINPHONE_PUBLIC const char *linphone_gtk_get_lang(const char *config_file);
LINPHONE_PUBLIC void linphone_gtk_set_lang(const char *code);
LINPHONE_PUBLIC SipSetupContext* linphone_gtk_get_default_sip_setup_context(void);
LINPHONE_PUBLIC GtkWidget * linphone_gtk_show_buddy_lookup_window(SipSetupContext *ctx);
LINPHONE_PUBLIC void linphone_gtk_buddy_lookup_set_keyword(GtkWidget *w, const char *kw);
LINPHONE_PUBLIC void * linphone_gtk_wait(LinphoneCore *lc, void *ctx, LinphoneWaitingState ws, const char *purpose, float progress);
LINPHONE_PUBLIC void linphone_gtk_terminate_call(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_call_update_tab_header(LinphoneCall *call, gboolean pause);
LINPHONE_PUBLIC void linphone_gtk_show_directory_search(void);
LINPHONE_PUBLIC void linphone_gtk_status_icon_set_blinking(gboolean val);
LINPHONE_PUBLIC void linphone_gtk_notify(LinphoneCall *call, LinphoneChatMessage *chat_message, const char *msg);

LINPHONE_PUBLIC void linphone_gtk_load_chatroom(LinphoneChatRoom *cr, const LinphoneAddress *uri, GtkWidget *chat_view);
LINPHONE_PUBLIC void linphone_gtk_send_text(void);
LINPHONE_PUBLIC GtkWidget * linphone_gtk_init_chatroom(LinphoneChatRoom *cr, const LinphoneAddress *with);
LINPHONE_PUBLIC LinphoneChatRoom * linphone_gtk_create_chatroom(const LinphoneAddress *with);
LINPHONE_PUBLIC void linphone_gtk_text_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage *msg);
LINPHONE_PUBLIC void linphone_gtk_is_composing_received(LinphoneCore *lc, LinphoneChatRoom *room);

LINPHONE_PUBLIC void linphone_gtk_friend_list_update_button_display(GtkTreeView *friendlist);
LINPHONE_PUBLIC void linphone_gtk_friend_list_set_chat_conversation(const LinphoneAddress *la);
LINPHONE_PUBLIC gboolean linphone_gtk_friend_list_is_contact(const LinphoneAddress *addr);
LINPHONE_PUBLIC void linphone_gtk_friend_list_set_active_address(const LinphoneAddress *addr);
LINPHONE_PUBLIC const LinphoneAddress *linphone_gtk_friend_list_get_active_address(void);
LINPHONE_PUBLIC gboolean linphone_gtk_friend_list_enter_event_handler(GtkTreeView *friendlist, GdkEventCrossing *event);
LINPHONE_PUBLIC gboolean linphone_gtk_friend_list_leave_event_handler(GtkTreeView *friendlist, GdkEventCrossing *event);
LINPHONE_PUBLIC gboolean linphone_gtk_friend_list_motion_event_handler(GtkTreeView *friendlist, GdkEventMotion *event);
LINPHONE_PUBLIC void linphone_gtk_friend_list_on_name_column_clicked(GtkTreeModel *model);
LINPHONE_PUBLIC void linphone_gtk_notebook_tab_select(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer data);
LINPHONE_PUBLIC void linphone_gtk_show_friends(void);
LINPHONE_PUBLIC void linphone_gtk_show_contact(LinphoneFriend *lf, GtkWidget *parent);
LINPHONE_PUBLIC void linphone_gtk_buddy_info_updated(LinphoneCore *lc, LinphoneFriend *lf);

/*functions controlling the different views*/
LINPHONE_PUBLIC gboolean linphone_gtk_use_in_call_view(void);
LINPHONE_PUBLIC LinphoneCall *linphone_gtk_get_currently_displayed_call(gboolean *is_conf);
LINPHONE_PUBLIC void linphone_gtk_create_in_call_view(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_gtk_in_call_view_set_calling(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_gtk_in_call_view_set_in_call(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_gtk_in_call_view_update_duration(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_gtk_in_call_view_terminate(LinphoneCall *call, const char *error_msg);
LINPHONE_PUBLIC void linphone_gtk_in_call_view_set_incoming(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_gtk_in_call_view_set_paused(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_gtk_in_call_view_set_transfer_status(LinphoneCall *call, LinphoneCallState cstate);
LINPHONE_PUBLIC void linphone_gtk_mute_clicked(GtkButton *button);
LINPHONE_PUBLIC void transfer_button_clicked(GtkWidget *button, gpointer call_ref);
LINPHONE_PUBLIC void linphone_gtk_enable_mute_button(GtkButton *button, gboolean sensitive);
LINPHONE_PUBLIC void linphone_gtk_enable_hold_button(LinphoneCall *call, gboolean sensitive, gboolean holdon);
LINPHONE_PUBLIC void linphone_gtk_enable_transfer_button(LinphoneCore *lc, gboolean value);
LINPHONE_PUBLIC void linphone_gtk_enable_conference_button(LinphoneCore *lc, gboolean value);
LINPHONE_PUBLIC void linphone_gtk_set_in_conference(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_gtk_unset_from_conference(LinphoneCall *call);
LINPHONE_PUBLIC bool_t linphone_gtk_call_is_in_conference_view(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_gtk_terminate_conference_participant(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_gtk_in_call_view_show_encryption(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_gtk_in_call_view_hide_encryption(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_gtk_update_video_button(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_gtk_init_audio_meter(GtkWidget *w, get_volume_t get_volume, void *data);
LINPHONE_PUBLIC void linphone_gtk_uninit_audio_meter(GtkWidget *w);

LINPHONE_PUBLIC void linphone_gtk_show_login_frame(LinphoneProxyConfig *cfg, gboolean disable_auto_login);
LINPHONE_PUBLIC void linphone_gtk_exit_login_frame(void);
LINPHONE_PUBLIC void linphone_gtk_set_ui_config(const char *key, const char *value);

LINPHONE_PUBLIC void linphone_gtk_log_uninit(void);

LINPHONE_PUBLIC bool_t linphone_gtk_init_instance(const char *app_name, int option, const char *addr_to_call);
LINPHONE_PUBLIC void linphone_gtk_uninit_instance(void);
LINPHONE_PUBLIC void linphone_gtk_monitor_usb(void);
LINPHONE_PUBLIC void linphone_gtk_unmonitor_usb(void);

LINPHONE_PUBLIC void linphone_gtk_fill_combo_box(GtkWidget *combo, const char **devices, const char *selected, DeviceCap cap);
LINPHONE_PUBLIC gchar *linphone_gtk_get_record_path(const LinphoneAddress *address, gboolean is_conference);
LINPHONE_PUBLIC gchar *linphone_gtk_get_snapshot_path(void);
LINPHONE_PUBLIC void linphone_gtk_schedule_restart(void);

LINPHONE_PUBLIC void linphone_gtk_show_audio_assistant(void);
LINPHONE_PUBLIC gboolean linphone_gtk_get_audio_assistant_option(void);

LINPHONE_PUBLIC void linphone_gtk_set_configuration_uri(void);
LINPHONE_PUBLIC GtkWidget * linphone_gtk_show_config_fetching(void);
LINPHONE_PUBLIC void linphone_gtk_close_config_fetching(GtkWidget *w, LinphoneConfiguringState state);
LINPHONE_PUBLIC const char *linphone_gtk_get_sound_path(const char *file);
LINPHONE_PUBLIC void linphone_gtk_in_call_show_video(LinphoneCall *call);
LINPHONE_PUBLIC char *linphone_gtk_address(const LinphoneAddress *addr);/*return human readable identifier for a LinphoneAddress */
LINPHONE_PUBLIC GtkWidget *linphone_gtk_get_camera_preview_window(void);

LINPHONE_PUBLIC void linphone_gtk_login_frame_connect_clicked(GtkWidget *button, GtkWidget *login_frame);

LINPHONE_PUBLIC gboolean linphone_gtk_call_log_reset_missed_call(GtkWidget *w, GdkEvent *event, gpointer user_data);
LINPHONE_PUBLIC void linphone_gtk_history_row_activated(GtkWidget *treeview);
LINPHONE_PUBLIC void linphone_gtk_history_row_selected(GtkWidget *treeview);
LINPHONE_PUBLIC void linphone_gtk_clear_call_logs(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_add_contact(void);
LINPHONE_PUBLIC void linphone_gtk_contact_clicked(GtkTreeSelection *selection);
LINPHONE_PUBLIC void linphone_gtk_add_button_clicked(void);
LINPHONE_PUBLIC void linphone_gtk_edit_button_clicked(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_remove_button_clicked(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_my_presence_clicked(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_directory_search_button_clicked(GtkWidget *button);
LINPHONE_PUBLIC gboolean linphone_gtk_popup_contact_menu(GtkWidget *list, GdkEventButton *event);
LINPHONE_PUBLIC gboolean linphone_gtk_contact_list_button_pressed(GtkTreeView* firendlist, GdkEventButton* event);
LINPHONE_PUBLIC void linphone_gtk_auth_token_verified_clicked(GtkButton *button);
LINPHONE_PUBLIC void linphone_gtk_hold_clicked(GtkButton *button);
LINPHONE_PUBLIC void linphone_gtk_record_call_toggled(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_log_hide(void);
LINPHONE_PUBLIC void linphone_gtk_log_scroll_to_end(GtkToggleButton *button);
LINPHONE_PUBLIC void linphone_gtk_log_clear(void);
LINPHONE_PUBLIC void linphone_gtk_logout_clicked(void);

LINPHONE_PUBLIC void linphone_gtk_about_response(GtkDialog *dialog, gint id);
LINPHONE_PUBLIC void linphone_gtk_show_about(void);
LINPHONE_PUBLIC void linphone_gtk_start_call(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_start_chat(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_uri_bar_activate(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_terminate_call(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_decline_clicked(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_answer_clicked(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_enable_video(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_enable_self_view(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_used_identity_changed(GtkWidget *w);
LINPHONE_PUBLIC void on_proxy_refresh_button_clicked(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_link_to_website(GtkWidget *item);
LINPHONE_PUBLIC void linphone_gtk_options_activate(GtkWidget *item);
LINPHONE_PUBLIC void linphone_gtk_show_keypad_checked(GtkCheckMenuItem *check_menu_item);
LINPHONE_PUBLIC gboolean linphone_gtk_keypad_destroyed_handler(void);

LINPHONE_PUBLIC void linphone_gtk_keyword_changed(GtkEditable *e);
LINPHONE_PUBLIC void linphone_gtk_buddy_lookup_contact_activated(GtkWidget *treeview);
LINPHONE_PUBLIC void linphone_gtk_add_buddy_from_database(GtkWidget *button);
LINPHONE_PUBLIC gboolean linphone_gtk_call_log_button_pressed(GtkWidget *widget, GdkEventButton *event);
LINPHONE_PUBLIC void linphone_gtk_call_statistics_closed(GtkWidget *call_stats);
LINPHONE_PUBLIC void linphone_gtk_config_uri_cancel(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_config_uri_changed(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_contact_cancel(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_contact_ok(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_dscp_edit(void);
LINPHONE_PUBLIC void linphone_gtk_dscp_edit_response(GtkWidget *dialog, guint response_id);
LINPHONE_PUBLIC void linphone_gtk_keypad_key_released(GtkWidget *w, GdkEvent *event, gpointer userdata);
LINPHONE_PUBLIC void linphone_gtk_keypad_key_pressed(GtkWidget *w, GdkEvent *event, gpointer userdata);
LINPHONE_PUBLIC void linphone_gtk_ldap_save(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_ldap_reset(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_parameters_destroyed(GtkWidget *pb);
LINPHONE_PUBLIC void linphone_gtk_mtu_set(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_mtu_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_use_sip_info_dtmf_toggled(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_ipv6_toggled(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_lime_changed(GtkComboBoxText *comboext);
LINPHONE_PUBLIC void linphone_gtk_disabled_udp_port_toggle(GtkCheckButton *button);
LINPHONE_PUBLIC void linphone_gtk_random_udp_port_toggle(GtkCheckButton *button);
LINPHONE_PUBLIC void linphone_gtk_udp_port_value_changed(GtkSpinButton *button);
LINPHONE_PUBLIC void linphone_gtk_disabled_tcp_port_toggle(GtkCheckButton *button);
LINPHONE_PUBLIC void linphone_gtk_random_tcp_port_toggle(GtkCheckButton *button);
LINPHONE_PUBLIC void linphone_gtk_tcp_port_value_changed(GtkSpinButton *button);
LINPHONE_PUBLIC void linphone_gtk_min_audio_port_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_max_audio_port_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_fixed_audio_port_toggle(void);
LINPHONE_PUBLIC void linphone_gtk_min_video_port_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_max_video_port_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_fixed_video_port_toggle(void);
LINPHONE_PUBLIC void linphone_gtk_set_media_encryption_mandatory(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_edit_tunnel(GtkButton *button);
LINPHONE_PUBLIC void linphone_gtk_no_firewall_toggled(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_use_nat_address_toggled(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_use_stun_toggled(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_use_ice_toggled(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_use_upnp_toggled(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_nat_address_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_stun_server_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_ring_file_set(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_play_ring_file(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_alsa_special_device_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_capture_device_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_ring_device_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_playback_device_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_echo_cancelation_toggled(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_cam_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_video_size_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_video_renderer_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_video_preset_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_show_camera_preview_clicked(GtkButton *button);
LINPHONE_PUBLIC void linphone_gtk_update_my_contact(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_add_proxy(GtkButton *button);
LINPHONE_PUBLIC void linphone_gtk_show_sip_accounts(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_edit_proxy(GtkButton *button);
LINPHONE_PUBLIC void linphone_gtk_remove_proxy(GtkButton *button);
LINPHONE_PUBLIC void linphone_gtk_clear_passwords(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_audio_codec_up(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_audio_codec_down(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_audio_codec_enable(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_audio_codec_disable(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_video_codec_up(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_video_codec_down(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_video_codec_enable(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_video_codec_disable(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_video_framerate_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_upload_bw_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_download_bw_changed(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_adaptive_rate_control_toggled(GtkToggleButton *button);
LINPHONE_PUBLIC void linphone_gtk_lang_changed(GtkComboBox *combo);
LINPHONE_PUBLIC void linphone_gtk_ui_level_toggled(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_show_ldap_config(GtkWidget* button);
LINPHONE_PUBLIC void linphone_gtk_parameters_closed(GtkWidget *button);
LINPHONE_PUBLIC void linphone_gtk_password_ok(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_password_cancel(GtkWidget *w);
LINPHONE_PUBLIC void linphone_gtk_proxy_ok(GtkButton *button);
LINPHONE_PUBLIC void linphone_gtk_proxy_cancel(GtkButton *button);
LINPHONE_PUBLIC void linphone_gtk_proxy_address_changed(GtkEditable *editable);
LINPHONE_PUBLIC void linphone_gtk_proxy_transport_changed(GtkWidget *combo);
LINPHONE_PUBLIC void linphone_gtk_tunnel_ok(GtkButton *button);
LINPHONE_PUBLIC void linphone_gtk_notebook_current_page_changed(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data);
LINPHONE_PUBLIC void linphone_gtk_reload_sound_devices(void);
LINPHONE_PUBLIC void linphone_gtk_reload_video_devices(void);
LINPHONE_PUBLIC bool_t linphone_gtk_is_friend(LinphoneCore *lc, const char *contact);
LINPHONE_PUBLIC gboolean linphone_gtk_auto_answer_enabled(void);
LINPHONE_PUBLIC void linphone_gtk_auto_answer_delay_changed(GtkSpinButton *spinbutton, gpointer user_data);
LINPHONE_PUBLIC void linphone_gtk_update_status_bar_icons(void);
LINPHONE_PUBLIC void linphone_gtk_enable_auto_answer(GtkToggleButton *checkbox, gpointer user_data);

LINPHONE_PUBLIC void linphone_gtk_import_contacts(void);
LINPHONE_PUBLIC void linphone_gtk_export_contacts(void);

LINPHONE_PUBLIC void linphone_gtk_mark_chat_read(LinphoneChatRoom *cr);
#ifdef __APPLE__
LINPHONE_PUBLIC void linphone_gtk_update_badge_count();
#endif

LINPHONE_PUBLIC gboolean linphone_gtk_on_key_press(GtkWidget *widget, GdkEvent *event, gpointer user_data);
