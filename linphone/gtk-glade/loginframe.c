/*
linphone, gtk-glade interface.
Copyright (C) 2009  Simon MORLAT (simon.morlat@linphone.org)

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

#include "linphone.h"

enum {
	NetworkKindAdsl,
	NetworkKindOpticalFiber
};

void linphone_gtk_show_login_frame(LinphoneProxyConfig *cfg){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *label=linphone_gtk_get_widget(mw,"login_label");
	LinphoneAuthInfo *ai;
	gchar *str;
	osip_from_t *from;
	LinphoneCore *lc=linphone_gtk_get_core();
	int nettype;

	gtk_widget_hide(linphone_gtk_get_widget(mw,"idle_frame"));
	gtk_widget_show(linphone_gtk_get_widget(mw,"login_frame"));
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"main_menu"),FALSE);
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"modes"),FALSE);
	str=g_strdup_printf(_("Please enter login information for %s"),linphone_proxy_config_get_domain(cfg));
	gtk_label_set_text(GTK_LABEL(label),str);
	g_object_set_data(G_OBJECT(mw),"login_proxy_config",cfg);
	g_free(str);

	osip_from_init(&from);
	osip_from_parse(from,linphone_proxy_config_get_identity(cfg));
	
	ai=linphone_core_find_auth_info(lc,linphone_proxy_config_get_domain(cfg),from->url->username);
	/*display the last entered username*/
	gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(mw,"login_username")),
		from->url->username);
	gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(mw,"login_password")),
		ai!=NULL ? ai->passwd : "");
	if (linphone_core_get_download_bandwidth(lc)==0 &&
		linphone_core_get_upload_bandwidth(lc)==0)
		nettype=NetworkKindOpticalFiber;
	else nettype=NetworkKindAdsl;
	gtk_combo_box_set_active(GTK_COMBO_BOX(linphone_gtk_get_widget(mw,"login_internet_kind")),nettype);
	osip_from_free(from);
}

void linphone_gtk_exit_login_frame(void){
	GtkWidget *mw=linphone_gtk_get_main_window();
	gtk_widget_show(linphone_gtk_get_widget(mw,"idle_frame"));
	gtk_widget_hide(linphone_gtk_get_widget(mw,"login_frame"));
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"main_menu"),TRUE);
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"modes"),TRUE);
}

gboolean check_login_ok(LinphoneProxyConfig *cfg){
	if (linphone_proxy_config_is_registered(cfg)){
		linphone_gtk_exit_login_frame();
		return FALSE;	
	}
	return TRUE;
}

void linphone_gtk_login_frame_connect_clicked(GtkWidget *button){
	GtkWidget *mw=gtk_widget_get_toplevel(button);
	const char *username;
	const char *password;
	char *identity;
	int netkind_id;
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)g_object_get_data(G_OBJECT(mw),"login_proxy_config");
	SipSetupContext *ssctx=linphone_proxy_config_get_sip_setup_context(cfg);
	osip_from_t *from;

	username=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(mw,"login_username")));
	password=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(mw,"login_password")));

	netkind_id=gtk_combo_box_get_active(GTK_COMBO_BOX(linphone_gtk_get_widget(mw,"login_internet_kind")));

	if (netkind_id==NetworkKindAdsl){
		linphone_core_set_upload_bandwidth(lc,256);
		linphone_core_set_download_bandwidth(lc,512);
	}else if (netkind_id==NetworkKindOpticalFiber){
		/*infinite*/
		linphone_core_set_upload_bandwidth(lc,0);
		linphone_core_set_download_bandwidth(lc,0);
	}
	osip_from_init(&from);
	osip_from_parse(from,linphone_proxy_config_get_identity(cfg));
	osip_free(from->url->username);
	from->url->username=osip_strdup(username);
	osip_from_to_str(from,&identity);
	osip_from_free(from);
	if (sip_setup_context_login_account(ssctx,identity,password)==0){
		guint t=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(mw),"login_tout"));
		if (t!=0) g_source_remove(t);
		t=g_timeout_add(50000,(GSourceFunc)check_login_ok,cfg);
		g_object_set_data(G_OBJECT(mw),"login_tout",GINT_TO_POINTER(t));
	}
}
