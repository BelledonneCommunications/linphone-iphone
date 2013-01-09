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

#include "linphone.h"
#include <glib.h>
#include <glib/gprintf.h>
static LinphoneAccountCreator *linphone_gtk_assistant_get_creator(GtkWidget*w);

static const int PASSWORD_MIN_SIZE = 6;
static const int LOGIN_MIN_SIZE = 4;

static GtkWidget *the_assistant=NULL;
static GdkPixbuf *ok;
static GdkPixbuf *notok;

static GtkWidget *create_intro(){
	GtkWidget *vbox=gtk_vbox_new(FALSE,2);
	GtkWidget *label=gtk_label_new(_("Welcome !\nThis assistant will help you to use a SIP account for your calls."));
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 2);
	g_object_set_data(G_OBJECT(vbox),"label",label);
	gtk_widget_show_all(vbox);
	return vbox;
}

static GtkWidget *create_setup_signin_choice(){
	GtkWidget *vbox=gtk_vbox_new(FALSE,2);
	GtkWidget *t1=gtk_radio_button_new_with_label(NULL,_("Create an account on linphone.org"));
	GtkWidget *t2=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(t1),_("I have already a linphone.org account and I just want to use it"));
	GtkWidget *t3=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(t1),_("I have already a sip account and I just want to use it"));
	gtk_box_pack_start (GTK_BOX (vbox), t1, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), t2, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), t3, TRUE, TRUE, 2);
	gtk_widget_show_all(vbox);
	g_object_set_data(G_OBJECT(vbox),"create_account",t1);
	g_object_set_data(G_OBJECT(vbox),"setup_linphone_account",t2);
	g_object_set_data(G_OBJECT(vbox),"setup_account",t3);
	return vbox;
}

static int all_account_information_entered(GtkWidget *w) {
	GtkEntry* username = GTK_ENTRY(g_object_get_data(G_OBJECT(w),"username"));
	GtkEntry* domain = GTK_ENTRY(g_object_get_data(G_OBJECT(w),"domain"));

	if (gtk_entry_get_text_length(username) > 0 &&
	gtk_entry_get_text_length(domain) > 0 &&
	g_regex_match_simple("^[a-zA-Z]+[a-zA-Z0-9.\\-_]{2,}$", gtk_entry_get_text(username), 0, 0) &&
	g_regex_match_simple("^(sip:)?([a-z0-9]+([\\.-][a-z0-9]+)*)+\\.[a-z]{2,}$", gtk_entry_get_text(domain), 0, 0)) {
		return 1;
	}
	return 0;
}

static void account_informations_changed(GtkEntry *entry, GtkWidget *w) {
	GtkWidget *assistant=gtk_widget_get_toplevel(w);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant),w,
		all_account_information_entered(w)>0);
}

static void linphone_account_informations_changed(GtkEntry *entry, GtkWidget *w) {
	GtkWidget *assistant=gtk_widget_get_toplevel(w);
	GtkEntry* username = GTK_ENTRY(g_object_get_data(G_OBJECT(w),"username"));

	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant),w,
		gtk_entry_get_text_length(username) >= LOGIN_MIN_SIZE);
}

static GtkWidget *create_linphone_account_informations_page() {
	GtkWidget *vbox=gtk_table_new(3, 2, TRUE);
	GtkWidget *label=gtk_label_new(_("Enter your linphone.org username"));

	GdkColor color;
	gdk_color_parse ("red", &color);
	GtkWidget *labelEmpty=gtk_label_new(NULL);
	gtk_widget_modify_fg(labelEmpty, GTK_STATE_NORMAL, &color);

	GtkWidget *labelUsername=gtk_label_new(_("Username:"));
	GtkWidget *entryUsername=gtk_entry_new();
	GtkWidget *labelPassword=gtk_label_new(_("Password:"));
	GtkWidget *entryPassword=gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(entryPassword), FALSE);

	gtk_table_attach_defaults(GTK_TABLE(vbox), label, 0, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelUsername, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(vbox), entryUsername, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelPassword, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(vbox), entryPassword, 1, 2, 2, 3);

	gtk_widget_show_all(vbox);
	g_object_set_data(G_OBJECT(vbox),"username",entryUsername);
	g_object_set_data(G_OBJECT(vbox),"password",entryPassword);
	g_object_set_data(G_OBJECT(vbox),"errorstring",labelEmpty);
	g_signal_connect(G_OBJECT(entryUsername),"changed",(GCallback)linphone_account_informations_changed,vbox);
	return vbox;
}

static GtkWidget *create_account_informations_page() {
	GtkWidget *vbox=gtk_table_new(6, 2, FALSE);
	GtkWidget *label=gtk_label_new(_("Enter your account informations"));

	GdkColor color;
	gdk_color_parse ("red", &color);
	GtkWidget *labelEmpty=gtk_label_new(NULL);
	gtk_widget_modify_fg(labelEmpty, GTK_STATE_NORMAL, &color);

	GtkWidget *labelUsername=gtk_label_new(_("Username*"));
	GtkWidget *labelPassword=gtk_label_new(_("Password*"));
	GtkWidget *entryPassword=gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(entryPassword), FALSE);
	GtkWidget *labelDomain=gtk_label_new(_("Domain*"));
	GtkWidget *labelProxy=gtk_label_new(_("Proxy"));
	GtkWidget *entryUsername=gtk_entry_new();
	GtkWidget *entryDomain=gtk_entry_new();
	GtkWidget *entryRoute=gtk_entry_new();

	gtk_table_attach_defaults(GTK_TABLE(vbox), label, 0, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelUsername, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(vbox), entryUsername, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelPassword, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(vbox), entryPassword, 1, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelDomain, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(vbox), entryDomain, 1, 2, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelProxy, 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(vbox), entryRoute, 1, 2, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelEmpty, 0, 2, 5, 6);
	gtk_widget_show_all(vbox);

	g_object_set_data(G_OBJECT(vbox),"username",entryUsername);
	g_object_set_data(G_OBJECT(vbox),"password",entryPassword);
	g_object_set_data(G_OBJECT(vbox),"domain",entryDomain);
	g_object_set_data(G_OBJECT(vbox),"proxy",entryRoute);
	g_object_set_data(G_OBJECT(vbox),"errorstring",labelEmpty);
	g_signal_connect(G_OBJECT(entryUsername),"changed",(GCallback)account_informations_changed,vbox);
	g_signal_connect(G_OBJECT(entryDomain),"changed",(GCallback)account_informations_changed,vbox);
	g_signal_connect(G_OBJECT(entryRoute),"changed",(GCallback)account_informations_changed,vbox);

	return vbox;
}

static int create_account(GtkWidget *page) {
	LinphoneAccountCreator *creator=linphone_gtk_assistant_get_creator(gtk_widget_get_toplevel(page));
	LinphoneProxyConfig *res=linphone_account_creator_validate(creator);
	if (res) {
		if (!g_regex_match_simple("^sip:[a-zA-Z]+[a-zA-Z0-9.\\-_]{2,}@sip.linphone.org$",creator->username, 0, 0)) {
			gchar identity[128];
			g_snprintf(identity, sizeof(identity), "sip:%s@sip.linphone.org", creator->username);
			linphone_account_creator_set_username(creator, identity);
			linphone_account_creator_set_domain(creator, "sip:sip.linphone.org");
		}
		return 1;
	}
	return 0;
}

static int is_account_information_correct(GtkWidget *w) {
	int is_username_available = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"is_username_available"));
	int is_email_correct = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"is_email_correct"));
	int is_password_correct = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"is_password_correct"));

	if (is_username_available == 1 && is_email_correct == 1 && is_password_correct == 1) {
		return 1;
	}
	return 0;
}

static void account_email_changed(GtkEntry *entry, GtkWidget *w) {
	// Verifying if email entered is correct, and if form is correctly filled, let the user go next page

	GtkEntry* email = GTK_ENTRY(g_object_get_data(G_OBJECT(w),"email"));
	GtkImage* isEmailOk = GTK_IMAGE(g_object_get_data(G_OBJECT(w),"emailOk"));
	GtkWidget *assistant=gtk_widget_get_toplevel(w);

	if (g_regex_match_simple("^[a-z0-9]+([_\\.-][a-z0-9]+)*@([a-z0-9]+([\\.-][a-z0-9]+)*)+\\.[a-z]{2,}$", gtk_entry_get_text(email), 0, 0)) {
		g_object_set_data(G_OBJECT(w),"is_email_correct",GINT_TO_POINTER(1));
		gtk_image_set_from_pixbuf(isEmailOk, ok);
	}
	else {
		g_object_set_data(G_OBJECT(w),"is_email_correct",GINT_TO_POINTER(0));
		gtk_image_set_from_pixbuf(isEmailOk, notok);
	}
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant),w,
			is_account_information_correct(w)>0);
}

static void account_password_changed(GtkEntry *entry, GtkWidget *w) {
	// Verifying if passwords entered match, and if form is correctly filled, let the user go next page

	GtkEntry* password = GTK_ENTRY(g_object_get_data(G_OBJECT(w),"password"));
	GtkImage* isPasswordOk = GTK_IMAGE(g_object_get_data(G_OBJECT(w),"passwordOk"));
	GtkEntry* password_confirm = GTK_ENTRY(g_object_get_data(G_OBJECT(w),"password_confirm"));
	GtkWidget *assistant=gtk_widget_get_toplevel(w);
	GtkLabel* passwordError = GTK_LABEL(g_object_get_data(G_OBJECT(w),"error"));

	if (gtk_entry_get_text_length(password) >= PASSWORD_MIN_SIZE &&
	g_ascii_strcasecmp(gtk_entry_get_text(password), gtk_entry_get_text(password_confirm)) == 0) {
		g_object_set_data(G_OBJECT(w),"is_password_correct",GINT_TO_POINTER(1));
		gtk_image_set_from_pixbuf(isPasswordOk, ok);
		gtk_label_set_text(passwordError, "");
	}
	else {
		if (gtk_entry_get_text_length(password) < PASSWORD_MIN_SIZE) {
			gtk_label_set_text(passwordError, "Password is too short !");
		}
		else if (!g_ascii_strcasecmp(gtk_entry_get_text(password), gtk_entry_get_text(password_confirm)) == 0) {
			gtk_label_set_text(passwordError, "Passwords don't match !");
		}
		g_object_set_data(G_OBJECT(w),"is_password_correct",GINT_TO_POINTER(0));
		gtk_image_set_from_pixbuf(isPasswordOk, notok);
	}
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant),w,
			is_account_information_correct(w)>0);
}

gboolean update_interface_with_username_availability(gpointer *w) {
	GtkWidget *assistant = gtk_widget_get_toplevel(GTK_WIDGET(w));
	GtkImage* isUsernameOk = GTK_IMAGE(g_object_get_data(G_OBJECT(w),"usernameOk"));
	GtkLabel* usernameError = GTK_LABEL(g_object_get_data(G_OBJECT(w),"error"));
	int account_existing = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"is_username_used"));

	if (account_existing == 0) {
		g_object_set_data(G_OBJECT(w),"is_username_available",GINT_TO_POINTER(1));
		gtk_image_set_from_pixbuf(isUsernameOk, ok);
		gtk_label_set_text(usernameError, "");
	}
	else {
		gtk_label_set_text(usernameError, "Username is already in use !");
		g_object_set_data(G_OBJECT(w),"is_username_available",GINT_TO_POINTER(0));
		gtk_image_set_from_pixbuf(isUsernameOk, notok);
	}

	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant),GTK_WIDGET(w),
				is_account_information_correct(GTK_WIDGET(w))>0);

	return FALSE;
}

void* check_username_availability(void* w) {
	LinphoneAccountCreator *creator=linphone_gtk_assistant_get_creator(gtk_widget_get_toplevel(GTK_WIDGET(w)));

	int account_existing = linphone_account_creator_test_existence(creator);

	g_object_set_data(G_OBJECT(w),"is_username_used",GINT_TO_POINTER(account_existing));
	gdk_threads_add_idle((GSourceFunc)update_interface_with_username_availability, (void*)w);

	return NULL;
}

static void account_username_changed(GtkEntry *entry, GtkWidget *w) {
	// Verifying if username choosed is available, and if form is correctly filled, let the user go next page
	GtkWidget *assistant = gtk_widget_get_toplevel(GTK_WIDGET(w));
	GtkEntry* username = GTK_ENTRY(g_object_get_data(G_OBJECT(w),"username"));
	GtkImage* isUsernameOk = GTK_IMAGE(g_object_get_data(G_OBJECT(w),"usernameOk"));
	GtkLabel* usernameError = GTK_LABEL(g_object_get_data(G_OBJECT(w),"error"));

	LinphoneAccountCreator *creator=linphone_gtk_assistant_get_creator(assistant);
	linphone_account_creator_set_username(creator, gtk_entry_get_text(username));

	if (g_regex_match_simple("^[a-zA-Z]+[a-zA-Z0-9.\\-_]{3,}$", gtk_entry_get_text(username), 0, 0)) {
#if !GLIB_CHECK_VERSION(2, 31, 0)
		g_thread_create(check_username_availability, (void*)w, FALSE, NULL);
#else
		g_thread_new(NULL, check_username_availability, w);
#endif
	}
	else {
		if (gtk_entry_get_text_length(username) < LOGIN_MIN_SIZE) {
			gtk_label_set_text(usernameError, "Username is too short");
		}
		else if (!g_regex_match_simple("^[a-zA-Z]+[a-zA-Z0-9.\\-_]{3,}$", gtk_entry_get_text(username), 0, 0)) {
			gtk_label_set_text(usernameError, "Unauthorized username");
		}
		g_object_set_data(G_OBJECT(w),"is_username_available",GINT_TO_POINTER(0));
		gtk_image_set_from_pixbuf(isUsernameOk, notok);

		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant),w,
				is_account_information_correct(w)>0);
	}
}

static GtkWidget *create_account_information_page() {
	GtkWidget *vbox=gtk_table_new(7, 3, FALSE);

	GtkWidget *label=gtk_label_new(_("(*) Required fields"));
	GtkWidget *labelUsername=gtk_label_new(_("Username: (*)"));
	GtkWidget *isUsernameOk=gtk_image_new_from_pixbuf(notok);
	GtkWidget *labelPassword=gtk_label_new(_("Password: (*)"));
	GtkWidget *isPasswordOk=gtk_image_new_from_pixbuf(notok);
	GtkWidget *labelEmail=gtk_label_new(_("Email: (*)"));
	GtkWidget *isEmailOk=gtk_image_new_from_pixbuf(notok);
	GtkWidget *labelPassword2=gtk_label_new(_("Confirm your password: (*)"));
	GtkWidget *entryUsername=gtk_entry_new();
	GtkWidget *entryPassword=gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(entryPassword), FALSE);
	GtkWidget *entryEmail=gtk_entry_new();
	GtkWidget *entryPassword2=gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(entryPassword2), FALSE);
	GtkWidget *checkNewsletter=gtk_check_button_new_with_label("Keep me informed with linphone updates");

	GdkColor color;
	gdk_color_parse ("red", &color);
	GtkWidget *labelError=gtk_label_new(NULL);
	gtk_widget_modify_fg(labelError, GTK_STATE_NORMAL, &color);

	GtkWidget *passwordVbox1=gtk_vbox_new(FALSE,2);
	GtkWidget *passwordVbox2=gtk_vbox_new(FALSE,2);
	gtk_box_pack_start (GTK_BOX (passwordVbox1), labelPassword, TRUE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (passwordVbox1), labelPassword2, TRUE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (passwordVbox2), entryPassword, TRUE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (passwordVbox2), entryPassword2, TRUE, FALSE, 2);

	gtk_table_attach_defaults(GTK_TABLE(vbox), label, 0, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelEmail, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(vbox), entryEmail, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(vbox), isEmailOk, 2, 3, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelUsername, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(vbox), entryUsername, 1, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(vbox), isUsernameOk, 2, 3, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(vbox), passwordVbox1, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(vbox), passwordVbox2, 1, 2, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(vbox), isPasswordOk, 2, 3, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelError, 1, 4, 5, 6);
	gtk_table_attach_defaults(GTK_TABLE(vbox), checkNewsletter, 0, 3, 6, 7);

	gtk_widget_show_all(vbox);
	g_object_set_data(G_OBJECT(vbox),"username",entryUsername);
	g_object_set_data(G_OBJECT(vbox),"password",entryPassword);
	g_object_set_data(G_OBJECT(vbox),"email",entryEmail);
	g_object_set_data(G_OBJECT(vbox),"usernameOk",isUsernameOk);
	g_object_set_data(G_OBJECT(vbox),"passwordOk",isPasswordOk);
	g_object_set_data(G_OBJECT(vbox),"emailOk",isEmailOk);
	g_object_set_data(G_OBJECT(vbox),"password_confirm",entryPassword2);
	g_object_set_data(G_OBJECT(vbox),"newsletter",checkNewsletter);
	g_object_set_data(G_OBJECT(vbox),"error",labelError);
	g_signal_connect(G_OBJECT(entryUsername),"changed",(GCallback)account_username_changed,vbox);
	g_signal_connect(G_OBJECT(entryPassword),"changed",(GCallback)account_password_changed,vbox);
	g_signal_connect(G_OBJECT(entryEmail),"changed",(GCallback)account_email_changed,vbox);
	g_signal_connect(G_OBJECT(entryPassword2),"changed",(GCallback)account_password_changed,vbox);
	return vbox;
}

/*
static GtkWidget *create_confirmation_page(){
	GtkWidget *vbox=gtk_vbox_new(FALSE,2);
	GtkWidget *label=gtk_label_new(NULL);
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 2);
	g_object_set_data(G_OBJECT(vbox),"label",label);
	gtk_widget_show_all(vbox);
	return vbox;
}
*/

static GtkWidget *create_error_page(){
	GtkWidget *vbox=gtk_table_new(2, 1, FALSE);
	GtkWidget *label=gtk_label_new(_("Error, account not validated, username already used or server unreachable.\nPlease go back and try again."));

	gtk_table_attach(GTK_TABLE(vbox), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 100);

	g_object_set_data(G_OBJECT(vbox),"label",label);
	gtk_widget_show_all(vbox);
	return vbox;
}

static GtkWidget *create_finish_page(){
	GtkWidget *vbox=gtk_vbox_new(FALSE,2);
	GtkWidget *label=gtk_label_new(_("Thank you. Your account is now configured and ready for use."));
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 2);
	gtk_widget_show_all(vbox);
	return vbox;
}

static GtkWidget *wait_for_activation() {
	GtkWidget *vbox=gtk_table_new(2, 1, FALSE);
	GtkWidget *label=gtk_label_new(_("Please validate your account by clicking on the link we just sent you by email.\n"
			"Then come back here and press Next button."));

	gtk_table_attach(GTK_TABLE(vbox), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 100);

	g_object_set_data(G_OBJECT(vbox),"label",label);
	gtk_widget_show_all(vbox);
	return vbox;
}

static int is_account_validated(GtkWidget *page) {
	LinphoneAccountCreator *creator=linphone_gtk_assistant_get_creator(gtk_widget_get_toplevel(page));
	return linphone_account_creator_test_validation(creator);
}

static void linphone_gtk_assistant_closed(GtkWidget *w){
	linphone_gtk_close_assistant();
}

static void linphone_gtk_assistant_prepare(GtkWidget *assistant, GtkWidget *page){
	int pagenum=gtk_assistant_get_current_page(GTK_ASSISTANT(assistant));

	if (pagenum == 5) {
		gtk_assistant_commit(GTK_ASSISTANT(assistant));
	} else if (pagenum == gtk_assistant_get_n_pages(GTK_ASSISTANT(assistant)) - 1) {
		// Saving the account and making it default
		LinphoneAccountCreator *creator=linphone_gtk_assistant_get_creator(assistant);
		LinphoneProxyConfig *cfg=linphone_proxy_config_new();
		linphone_proxy_config_set_identity(cfg, creator->username);
		linphone_proxy_config_set_server_addr(cfg, creator->domain);
		linphone_proxy_config_set_route(cfg, creator->route);
		linphone_proxy_config_expires(cfg, 3600);
		linphone_proxy_config_enable_publish(cfg, FALSE);
		linphone_proxy_config_enable_register(cfg, TRUE);

		gchar *username = creator->username + 4;
		const gchar *needle = "@";
		username = g_strndup(username, (g_strrstr(username, needle) - username));
		gchar domain[128];
		g_snprintf(domain, sizeof(domain), "\"%s\"", creator->domain + 4);
		LinphoneAuthInfo *info=linphone_auth_info_new(username, username, creator->password, NULL, domain);
		linphone_core_add_auth_info(linphone_gtk_get_core(),info);

		if (linphone_core_add_proxy_config(linphone_gtk_get_core(),cfg)==-1)
			return;

		linphone_core_set_default_proxy(linphone_gtk_get_core(),cfg);
		linphone_gtk_load_identities();

		// If account created on sip.linphone.org, we configure linphone to use TLS by default
		g_warning("Domain : %s", creator->domain);
		if (strcmp(creator->domain, "sip:sip.linphone.org") == 0) {
			LCSipTransports tr;
			LinphoneCore* lc = linphone_gtk_get_core();
			linphone_core_get_sip_transports(lc,&tr);
			tr.tls_port = tr.udp_port + tr.tcp_port + tr.tls_port;
			tr.udp_port = 0;
			tr.tcp_port = 0;
			linphone_core_set_sip_transports(lc,&tr);
		}
	}
}

static int linphone_gtk_assistant_forward(int curpage, gpointer data){
	GtkWidget *w=(GtkWidget*)data;
	GtkWidget *box=gtk_assistant_get_nth_page(GTK_ASSISTANT(w),curpage);
	if (curpage==1){
		GtkWidget *create_button=(GtkWidget*)g_object_get_data(G_OBJECT(box),"create_account");
		GtkWidget *setup_linphone_account=(GtkWidget*)g_object_get_data(G_OBJECT(box),"setup_linphone_account");
		GtkWidget *setup_account=(GtkWidget*)g_object_get_data(G_OBJECT(box),"setup_account");

		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(create_button))) {
			curpage += 3; // Going to P33
		}
		else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(setup_linphone_account))) {
			curpage += 2; // Going to P32
		}
		else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(setup_account))) {
			curpage += 1; // Going to P31
		}
	}
	else if (curpage == 2) { // Account's informations entered
		LinphoneAccountCreator *c=linphone_gtk_assistant_get_creator(w);
		gchar identity[128];
		g_snprintf(identity, sizeof(identity), "sip:%s@%s", gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"username"))), gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"domain"))));

		gchar proxy[128];
		g_snprintf(proxy, sizeof(proxy), "sip:%s", gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"domain"))));

		linphone_account_creator_set_username(c, identity);
		linphone_account_creator_set_domain(c, proxy);
		linphone_account_creator_set_route(c, gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"proxy"))));
		linphone_account_creator_set_password(c,gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"password"))));
		curpage = gtk_assistant_get_n_pages(GTK_ASSISTANT(w)) - 1; // Going to the last page
	}
	else if (curpage == 3) { // Linphone Account's informations entered
		LinphoneAccountCreator *c=linphone_gtk_assistant_get_creator(w);
		gchar identity[128];
		g_snprintf(identity, sizeof(identity), "sip:%s@sip.linphone.org", gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"username"))));
		linphone_account_creator_set_username(c, identity);
		linphone_account_creator_set_domain(c, "sip:sip.linphone.org");
		linphone_account_creator_set_password(c,gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"password"))));
		curpage = gtk_assistant_get_n_pages(GTK_ASSISTANT(w)) - 1; // Going to the last page
	}
	else if (curpage == 4) { // Password & Email entered
		LinphoneAccountCreator *c=linphone_gtk_assistant_get_creator(w);
		linphone_account_creator_set_username(c, gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"username"))));
		linphone_account_creator_set_password(c,gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"password"))));
		linphone_account_creator_set_email(c,gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"email"))));
		linphone_account_creator_set_suscribe(c,gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(box),"newsletter"))));
		if (create_account(w) == 1) {
			curpage += 1;
		} else { // Error when attempting to create the account
			curpage += 2;
		}
	}
	else if (curpage == 5) { // Waiting for account validation
		if (is_account_validated(w) == 1) {
			curpage += 2; // Going to the last page
		} else {
			curpage += 1;
		}
	}
	else {
		curpage += 1;
	}
	return curpage;
}

static LinphoneAccountCreator * linphone_gtk_assistant_init(GtkWidget *w){
	const MSList *elem;
	LinphoneCore *lc=linphone_gtk_get_core();
	for(elem=linphone_core_get_sip_setups(lc);elem!=NULL;elem=elem->next){
		SipSetup *ss=(SipSetup*)elem->data;
		if (sip_setup_get_capabilities(ss) & SIP_SETUP_CAP_ACCOUNT_MANAGER){
			LinphoneAccountCreator *creator=linphone_account_creator_new(lc,ss->name);
			g_object_set_data(G_OBJECT(w),"creator",creator);
			return creator;
		}
	}
	return NULL;
}

static LinphoneAccountCreator *linphone_gtk_assistant_get_creator(GtkWidget*w){
	return (LinphoneAccountCreator*)g_object_get_data(G_OBJECT(w),"creator");
}

void linphone_gtk_close_assistant(void){
	if(the_assistant==NULL)
		return;
	gtk_widget_destroy(the_assistant);	
	the_assistant = NULL;
}

void linphone_gtk_show_assistant(void){
	if(the_assistant!=NULL)
		return;
	GtkWidget *w=the_assistant=gtk_assistant_new();
	gtk_window_set_resizable (GTK_WINDOW(w), FALSE);

	ok = create_pixbuf(linphone_gtk_get_ui_config("ok","ok.png"));
	notok = create_pixbuf(linphone_gtk_get_ui_config("notok","notok.png"));

	GtkWidget *p1=create_intro();
	GtkWidget *p2=create_setup_signin_choice();
	GtkWidget *p31=create_account_informations_page();
	GtkWidget *p32=create_linphone_account_informations_page();
	GtkWidget *p33=create_account_information_page();
	//GtkWidget *confirm=create_confirmation_page();
	GtkWidget *validate=wait_for_activation();
	GtkWidget *error=create_error_page();
	GtkWidget *end=create_finish_page();
	
	linphone_gtk_assistant_init(w);
	gtk_assistant_append_page(GTK_ASSISTANT(w),p1);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),p1,GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),p1,_("Welcome to the account setup assistant"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w),p1,TRUE);

	gtk_assistant_append_page(GTK_ASSISTANT(w),p2);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),p2,GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),p2,_("Account setup assistant"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w),p2,TRUE);

	gtk_assistant_append_page(GTK_ASSISTANT(w),p31);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),p31,GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w),p31,FALSE);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),p31,_("Configure your account (step 1/1)"));

	gtk_assistant_append_page(GTK_ASSISTANT(w),p32);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),p32,GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w),p32,FALSE);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),p32,_("Enter your sip username (step 1/1)"));

	gtk_assistant_append_page(GTK_ASSISTANT(w),p33);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),p33,GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),p33,_("Enter account information (step 1/2)"));

	/*gtk_assistant_append_page(GTK_ASSISTANT(w),confirm);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),confirm,GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),confirm,_("Confirmation (step 2/2)"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w),confirm,TRUE);*/

	gtk_assistant_append_page(GTK_ASSISTANT(w),validate);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),validate,GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),validate,_("Validation (step 2/2)"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w),validate,TRUE);

	gtk_assistant_append_page(GTK_ASSISTANT(w),error);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),error,GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),error,_("Error"));

	gtk_assistant_append_page(GTK_ASSISTANT(w),end);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),end,GTK_ASSISTANT_PAGE_SUMMARY);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),end,_("Terminating"));

	gtk_assistant_set_forward_page_func(GTK_ASSISTANT(w),linphone_gtk_assistant_forward,w,NULL);
	g_signal_connect(G_OBJECT(w),"close",(GCallback)linphone_gtk_assistant_closed,NULL);
	g_signal_connect(G_OBJECT(w),"cancel",(GCallback)linphone_gtk_assistant_closed,NULL);
	g_signal_connect(G_OBJECT(w),"prepare",(GCallback)linphone_gtk_assistant_prepare,NULL);

	gtk_widget_show(w);
}

