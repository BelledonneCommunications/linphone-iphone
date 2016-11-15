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

#include "linphone.h"
#include "linphone/lpconfig.h"


void linphone_gtk_set_configuration_uri(void){
	GtkWidget *w=linphone_gtk_create_window("config-uri", linphone_gtk_get_main_window());
	GtkWidget *entry=linphone_gtk_get_widget(w,"uri_entry");
	const char *uri=linphone_core_get_provisioning_uri(linphone_gtk_get_core());
	if (uri) gtk_entry_set_text(GTK_ENTRY(entry),uri);
	gtk_widget_show(w);
}

void linphone_gtk_config_uri_changed(GtkWidget *button){
	GtkWidget *w=gtk_widget_get_toplevel(button);
	GtkWidget *entry=linphone_gtk_get_widget(w,"uri_entry");
	const char *uri=gtk_entry_get_text(GTK_ENTRY(entry));
	
	if (uri && (strlen(uri)==0 || strcmp(uri,"https://")==0)) uri=NULL;
	
	if(linphone_core_set_provisioning_uri(linphone_gtk_get_core(),uri) == 0) {
		gtk_widget_destroy(w);
		if (uri){
#ifndef _WIN32
			linphone_gtk_schedule_restart();
			gtk_main_quit();
#else
			GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(linphone_gtk_get_main_window()),
													   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
													   GTK_MESSAGE_INFO,
													   GTK_BUTTONS_OK,
													   _("Remote provisioning URI successfully set. Please restart Linphone in order to load the new remote settings"));
			g_signal_connect_swapped(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), dialog);
			gtk_widget_show(dialog);
#endif
		}
	} else {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(w),
								   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
								   GTK_MESSAGE_ERROR,
								   GTK_BUTTONS_OK,
								   _("Invalid remote provisioning URI"));
		g_signal_connect_swapped(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), dialog);
		gtk_widget_show(dialog);
	}
}

void linphone_gtk_config_uri_cancel(GtkWidget *button){
	GtkWidget *w=gtk_widget_get_toplevel(button);
	gtk_widget_destroy(w);
}

GtkWidget * linphone_gtk_show_config_fetching(void){
	LinphoneCore *lc=linphone_gtk_get_core();
	GtkWidget *w=linphone_gtk_create_window("provisioning-fetch", linphone_gtk_get_main_window());
	g_message("Fetching started");
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(w),_("fetching from %s"),linphone_core_get_provisioning_uri(lc));
#if GTK_CHECK_VERSION(2,20,0)
	{
		GtkWidget *spinner=gtk_spinner_new();
		gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG(w),spinner);
	}
#endif
	gtk_widget_show(w);
	return w;
}

void linphone_gtk_close_config_fetching(GtkWidget *w, LinphoneConfiguringState state){
	LinphoneCore *lc=linphone_gtk_get_core();
	gtk_widget_destroy(w);
	g_message("Fetching finished");
	if (state==LinphoneConfiguringFailed){
		GtkWidget *msg=gtk_message_dialog_new(NULL,0,GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE,_("Downloading of remote configuration from %s failed."),
			linphone_core_get_provisioning_uri(lc));
		g_signal_connect(G_OBJECT(msg),"response",(GCallback)gtk_widget_destroy,NULL);
		gtk_widget_show(msg);
	}
}

