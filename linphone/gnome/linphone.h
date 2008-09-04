

#ifndef LINPHONE_H
#define LINPHONE_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <linphonecore.h>
#include "lpconfig.h"

#include "support.h"
#include "propertybox.h"
#include "presence.h"
#include "addressbook.h"
#include "friends.h"

typedef struct _LinphoneMainWindow
{
	GtkWidget *window;
	GtkWidget *status_bar;
	GtkWidget *addressentry;
	GtkWidget *optioncontrols;
	GtkWidget *dtmfentry;
	GtkWidget *callbutton;
	PresenceBox presencebox;
	FriendList friendlist;
	gboolean shown_once;
}LinphoneMainWindow;

typedef struct _LinphoneGnomeUI
{
	LinphoneMainWindow main_window;
	LinphonePropertyBox propbox;
	GtkWidget *ab;	/*the address book */
	GtkWidget *logs;	/* the call logs window */
	LinphoneCore *core;
	guint timeout_id;
}LinphoneGnomeUI;


void linphone_gnome_ui_init(LinphoneGnomeUI *ui,LinphoneCore *core);
void linphone_gnome_ui_show(LinphoneGnomeUI *ui);
void linphone_gnome_ui_hide(LinphoneGnomeUI *ui);
void linphone_gnome_ui_uninit(LinphoneGnomeUI *ui);

void linphone_gnome_init(LinphoneGnomeUI *ui,LinphoneCore *lc);
void linphone_gnome_uninit(LinphoneGnomeUI *ui);

extern LinphoneGnomeUI *uiobj;

GtkWidget *proxy_combo_box_new(LinphoneProxyConfig *selected);
void linphone_refresh_proxy_combo_box(GtkWidget *window);
LinphoneProxyConfig *proxy_combo_box_get_selected(GtkWidget *combo);
void linphone_gnome_show_call_logs_window(LinphoneGnomeUI *ui);
void linphone_gnome_update_call_logs(LinphoneGnomeUI *ui);
void linphone_gnome_ui_display_something(LinphoneGnomeUI *ui,GtkMessageType type,const gchar *message);
void linphone_gnome_save_uri_history(LinphoneGnomeUI *ui);

GtkWidget *chatroom_new(const gchar *url, LinphoneChatRoom *cr);
void chatroom_append(GtkWidget *gcr, const gchar *from, const gchar *message);
void chatroom_close(GtkWidget *gcr);

#endif
