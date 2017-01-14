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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "linphone.h"

void test_button_clicked_cb(GtkWidget *button);
void linphone_gtk_exit_login_frame(void);


static void do_login(SipSetupContext *ssctx, const char *identity, const char * passwd, const char *userid){
	if (sip_setup_context_login_account(ssctx,identity,passwd,userid)==0){
	}
}

static gboolean do_login_noprompt(LinphoneProxyConfig *cfg){
	SipSetupContext *ssctx=linphone_proxy_config_get_sip_setup_context(cfg);
	LinphoneAddress *addr;
	const char *username;
	char *tmp;
	if (ssctx==NULL) return TRUE;/*not ready ?*/
	username=linphone_gtk_get_ui_config ("login_username",NULL);
	if (username==NULL) {
		linphone_gtk_show_login_frame(cfg,TRUE);
		return FALSE;
	}
	addr=linphone_address_clone(linphone_proxy_config_get_identity_address(cfg));
	linphone_address_set_username(addr,username);
	tmp=linphone_address_as_string (addr);
	do_login(ssctx,tmp,NULL,NULL);
	linphone_address_unref(addr);
	linphone_gtk_load_identities();
	return FALSE;
}

static void linphone_gtk_init_login_frame(GtkWidget *login_frame, LinphoneProxyConfig *cfg) {
	gboolean auto_login=linphone_gtk_get_ui_config_int("automatic_login",0);
	const char *login_image=linphone_gtk_get_ui_config("login_image","linphone-banner.png");
	GtkWidget *label=linphone_gtk_get_widget(login_frame,"login_label");
	LinphoneCore *lc=linphone_gtk_get_core();
	gchar *str;
	LinphoneAddress *from;
	const LinphoneAuthInfo *ai;
	const char *passwd=NULL;
	const char *userid=NULL;
	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(login_frame, "automatic_login")),auto_login);
	
	if (login_image){
		GdkPixbuf *pbuf=create_pixbuf (login_image);
		gtk_image_set_from_pixbuf (GTK_IMAGE(linphone_gtk_get_widget(login_frame, "login_image")), pbuf);
		g_object_unref(G_OBJECT(pbuf));
	}
	
	if (linphone_gtk_get_ui_config_int("login_needs_userid",FALSE)){
		gtk_widget_show(linphone_gtk_get_widget(login_frame,"userid"));
		gtk_widget_show(linphone_gtk_get_widget(login_frame,"login_userid"));
	}
	
	str=g_strdup_printf(_("Please enter login information for %s"),linphone_proxy_config_get_domain(cfg));
	gtk_label_set_text(GTK_LABEL(label),str);
	g_object_set_data(G_OBJECT(login_frame),"login_proxy_config",cfg);
	g_free(str);
	
	from=linphone_address_new(linphone_proxy_config_get_identity(cfg));
	if (linphone_address_get_username(from)[0]=='?'){
		const char *username=linphone_gtk_get_ui_config ("login_username",NULL);
		if (username)
			linphone_address_set_username(from,username);
	}

	ai=linphone_core_find_auth_info(lc,linphone_proxy_config_get_domain(cfg),linphone_address_get_username(from),NULL);
	/*display the last entered username, if not '?????'*/
	if (linphone_address_get_username(from)[0]!='?')
		gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(login_frame,"login_username")),
			linphone_address_get_username(from));
	if (ai) {
		passwd=linphone_auth_info_get_passwd(ai);
		userid=linphone_auth_info_get_userid(ai);
	}
	gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(login_frame,"login_password")),
		passwd!=NULL ? passwd : "");
	gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(login_frame,"login_userid")),
		userid ? userid : "");

	linphone_address_unref(from);
}

void linphone_gtk_show_login_frame(LinphoneProxyConfig *cfg, gboolean disable_auto_login){
	GtkWidget *mw=linphone_gtk_get_main_window();
	gboolean auto_login=linphone_gtk_get_ui_config_int("automatic_login",0);
	GtkWidget *main_frame = linphone_gtk_get_widget(mw, "main_frame");
	GtkWidget *main_layout = linphone_gtk_get_widget(mw, "main_layout");
	GtkWidget *login_frame;

	if (auto_login && !disable_auto_login){
		g_timeout_add(250,(GSourceFunc)do_login_noprompt,cfg);
		return;
	}
	
	login_frame = linphone_gtk_create_widget("login_frame");
	linphone_gtk_init_login_frame(login_frame, cfg);
	g_object_set_data_full(G_OBJECT(mw), "main_frame", g_object_ref(main_frame), g_object_unref);
	g_object_set_data(G_OBJECT(mw), "login_frame", login_frame);
	gtk_container_remove(GTK_CONTAINER(main_layout), main_frame);
	gtk_box_pack_start(GTK_BOX(main_layout), login_frame, TRUE, TRUE, 0);
	gtk_widget_show(login_frame);
	
	gtk_widget_hide(linphone_gtk_get_widget(mw,"disconnect_item"));
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"options_menu"),FALSE);
}

void linphone_gtk_exit_login_frame(void){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *main_layout = linphone_gtk_get_widget(mw, "main_layout");
	GtkWidget *main_frame = GTK_WIDGET(g_object_get_data(G_OBJECT(mw), "main_frame"));
	GtkWidget *login_frame = GTK_WIDGET(g_object_get_data(G_OBJECT(mw), "login_frame"));
	
	gtk_container_remove(GTK_CONTAINER(main_layout), login_frame);
	gtk_box_pack_start(GTK_BOX(main_layout), main_frame, TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(mw), "login_frame", NULL);
	g_object_set_data(G_OBJECT(mw), "main_frame", NULL);
	
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"options_menu"),TRUE);
	gtk_widget_show(linphone_gtk_get_widget(mw,"disconnect_item"));
}

void linphone_gtk_logout_clicked(void){
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneProxyConfig *cfg=NULL;
	cfg = linphone_core_get_default_proxy_config(lc);
	if (cfg){
		SipSetupContext *ss=linphone_proxy_config_get_sip_setup_context(cfg);
		if (ss){
			sip_setup_context_logout(ss);
			linphone_gtk_show_login_frame(cfg,TRUE);
		}
	}
}



void linphone_gtk_login_frame_connect_clicked(GtkWidget *button, GtkWidget *login_frame){
	const char *username;
	const char *password;
	const char *userid;
	char *identity;
	gboolean autologin;
	LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)g_object_get_data(G_OBJECT(login_frame),"login_proxy_config");
	LinphoneAddress *from;
	SipSetupContext *ssctx=linphone_proxy_config_get_sip_setup_context(cfg);

	username=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(login_frame,"login_username")));
	password=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(login_frame,"login_password")));
	userid=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(login_frame,"login_userid")));

	if (username==NULL || username[0]=='\0')
		return;

	autologin=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(login_frame,"automatic_login")));
	linphone_gtk_set_ui_config_int("automatic_login",autologin);
	linphone_gtk_set_ui_config("login_username",username);

	from=linphone_address_new(linphone_proxy_config_get_identity(cfg));
	linphone_address_set_username(from,username);
	identity=linphone_address_as_string(from);
	do_login(ssctx,identity,password,userid);
	/*we need to refresh the identities since the proxy config may have changed.*/
	linphone_gtk_load_identities();
}
