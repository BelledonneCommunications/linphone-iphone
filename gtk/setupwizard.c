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

static const int PASSWORD_MIN_SIZE = 6;
static const int LOGIN_MIN_SIZE = 4;
static GtkWidget *the_assistant = NULL;


static LinphoneAccountCreator * linphone_gtk_assistant_get_creator(GtkWidget *w) {
	return (LinphoneAccountCreator *)g_object_get_data(G_OBJECT(w), "creator");
}

static void linphone_gtk_account_validate_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status) {
	GtkWidget *assistant = (GtkWidget *)linphone_account_creator_get_user_data(creator);
	if (status == LinphoneAccountCreatorOk) {
		// Go to page_6_linphone_account_validation_wait
		gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), 6);
	} else { // Error when attempting to create the account
		// Go to page_8_error
		gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), 8);
	}
	gtk_assistant_commit(GTK_ASSISTANT(assistant));
}

static void create_account(GtkWidget *assistant) {
	LinphoneAccountCreator *creator = linphone_gtk_assistant_get_creator(assistant);
	linphone_account_creator_validate(creator);
}

static void linphone_gtk_test_account_validation_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status) {
	GtkWidget *assistant = (GtkWidget *)linphone_account_creator_get_user_data(creator);
	if (status == LinphoneAccountCreatorOk) {
		// Go to page_9_finish
		gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), 9);
	} else {
		// Go to page_8_error
		gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), 8);
	}
}

static void check_account_validation(GtkWidget *assistant) {
	LinphoneAccountCreator *creator = linphone_gtk_assistant_get_creator(assistant);
	linphone_account_creator_test_validation(creator);
}

static void linphone_gtk_assistant_closed(GtkWidget *w) {
	linphone_gtk_close_assistant();
}

static void linphone_gtk_assistant_prepare(GtkWidget *assistant, GtkWidget *page) {
	int pagenum = gtk_assistant_get_current_page(GTK_ASSISTANT(assistant));

	switch (pagenum) {
		case 5:
			create_account(assistant);
			break;
		case 7:
			check_account_validation(assistant);
			break;
		case 9:
			if (linphone_account_creator_configure(linphone_gtk_assistant_get_creator(assistant)) != NULL) {
				linphone_gtk_load_identities();
			}
			gtk_assistant_commit(GTK_ASSISTANT(assistant));
			break;
		default:
			break;
	}
}

static gint destroy_assistant(GtkWidget* w){
	gtk_widget_destroy(w);
	the_assistant = NULL;
	return FALSE;
}

static int linphone_gtk_assistant_forward(int curpage, gpointer data) {
	GtkWidget *w = GTK_WIDGET(data);
	GtkWidget *box = gtk_assistant_get_nth_page(GTK_ASSISTANT(w), curpage);
	LinphoneAccountCreator *creator = linphone_gtk_assistant_get_creator(w);

	switch (curpage) {
		case 0:
			curpage = 1; // Go to page_1_choice
			break;
		case 1:
			{
				GtkWidget *create_button = (GtkWidget*)g_object_get_data(G_OBJECT(box), "create_account");
				GtkWidget *setup_linphone_account = (GtkWidget*)g_object_get_data(G_OBJECT(box), "setup_linphone_account");
				GtkWidget *setup_account = (GtkWidget*)g_object_get_data(G_OBJECT(box), "setup_account");
				GtkWidget *config_uri = (GtkWidget*)g_object_get_data(G_OBJECT(box), "config-uri");

				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(create_button))) {
					curpage = 4; // Go to page_4_linphone_account_creation_configuration
				} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(setup_linphone_account))) {
					curpage = 3; // Go to page_3_linphone_account_configuration
				} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(setup_account))) {
					curpage = 2; // Go to page_2_external_account_configuration
				} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(config_uri))) {
					/* Destroy the assistant and popup config-uri dialog */
					gtk_widget_hide(w);
					linphone_gtk_set_configuration_uri();
					curpage = 0;
					g_idle_add((GSourceFunc)destroy_assistant, w);
				}
			}
			break;
		case 2:
			linphone_account_creator_set_username(creator, gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"username"))));
			linphone_account_creator_set_domain(creator, gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"domain"))));
			linphone_account_creator_set_route(creator, gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box), "proxy"))));
			linphone_account_creator_set_password(creator, gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box), "password"))));
			curpage = 9; // Go to page_9_finish
			break;
		case 3:
			linphone_account_creator_set_username(creator, gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"username"))));
			linphone_account_creator_set_domain(creator, "sip.linphone.org");
			linphone_account_creator_set_route(creator, "sip.linphone.org");
			linphone_account_creator_set_password(creator, gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box), "password"))));
			curpage = 9; // Go to page_9_finish
			break;
		case 4:
			linphone_account_creator_set_username(creator, gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box), "username"))));
			linphone_account_creator_set_password(creator, gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box), "password"))));
			linphone_account_creator_set_email(creator, gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box), "email"))));
			linphone_account_creator_enable_newsletter_subscription(creator, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(box), "newsletter"))));
			curpage = 5; // Go to page_5_linphone_account_creation_in_progress
			break;
		case 6:
			curpage = 7; // Go to page_7_linphone_account_validation_check_in_progress
			break;
		default:
			break;
	}
	return curpage;
}

static GtkWidget * create_intro_page(void) {
	GtkWidget *vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget *label = gtk_label_new(_("Welcome!\nThis assistant will help you to use a SIP account for your calls."));
	gtk_box_pack_start(GTK_BOX (vbox), label, TRUE, TRUE, 2);
	g_object_set_data(G_OBJECT(vbox), "label", label);
	gtk_widget_show_all(vbox);
	return vbox;
}

static GtkWidget * create_choice_page(void) {
	GtkWidget *vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget *t1 = gtk_radio_button_new_with_label(NULL, _("Create an account on linphone.org"));
	GtkWidget *t2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(t1), _("I have already a linphone.org account and I just want to use it"));
	GtkWidget *t3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(t1), _("I have already a sip account and I just want to use it"));
	GtkWidget *t4 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(t1), _("I want to specify a remote configuration URI"));

	gtk_box_pack_start(GTK_BOX (vbox), t1, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX (vbox), t2, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX (vbox), t3, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX (vbox), t4, TRUE, TRUE, 2);
	gtk_widget_show_all(vbox);
	g_object_set_data(G_OBJECT(vbox), "create_account", t1);
	g_object_set_data(G_OBJECT(vbox), "setup_linphone_account", t2);
	g_object_set_data(G_OBJECT(vbox), "setup_account", t3);
	g_object_set_data(G_OBJECT(vbox), "config-uri", t4);
	return vbox;
}

static int external_account_configuration_complete(GtkWidget *w) {
	GtkEntry* username = GTK_ENTRY(g_object_get_data(G_OBJECT(w), "username"));
	GtkEntry* domain = GTK_ENTRY(g_object_get_data(G_OBJECT(w), "domain"));

	if ((gtk_entry_get_text_length(username) > 0)
		&& (gtk_entry_get_text_length(domain) > 0)
		&& (g_regex_match_simple("^[a-zA-Z0-9+]+[a-zA-Z0-9.\\+\\-_]{2,}$", gtk_entry_get_text(username), 0, 0))
		&& (g_regex_match_simple("^(sip:)?([a-zA-Z0-9\\+]+([\\.-][a-zA-Z0-9+]+)*)$", gtk_entry_get_text(domain), 0, 0))) {
		return 1;
	}
	return 0;
}

static void external_account_configuration_changed(GtkEntry *entry, GtkWidget *w) {
	GtkWidget *assistant = gtk_widget_get_toplevel(w);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), w, external_account_configuration_complete(w) > 0);
}

static GtkWidget * create_external_account_configuration_page(void) {
	GtkWidget *vbox = gtk_table_new(6, 2, FALSE);
	GtkWidget *label = gtk_label_new(_("Enter your account information"));
	GdkColor color;
	GtkWidget *labelEmpty;
	GtkWidget *labelUsername;
	GtkWidget *labelPassword;
	GtkWidget *entryPassword;
	GtkWidget *labelDomain;
	GtkWidget *labelProxy;
	GtkWidget *entryUsername;
	GtkWidget *entryDomain;
	GtkWidget *entryRoute;

	gdk_color_parse("red", &color);
	labelEmpty = gtk_label_new(NULL);
	gtk_widget_modify_fg(labelEmpty, GTK_STATE_NORMAL, &color);
	labelUsername = gtk_label_new(_("Username*"));
	labelPassword = gtk_label_new(_("Password*"));
	entryPassword = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(entryPassword), FALSE);
	labelDomain = gtk_label_new(_("Domain*"));
	labelProxy = gtk_label_new(_("Proxy"));
	entryUsername = gtk_entry_new();
	entryDomain = gtk_entry_new();
	entryRoute = gtk_entry_new();

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

	g_object_set_data(G_OBJECT(vbox), "username", entryUsername);
	g_object_set_data(G_OBJECT(vbox), "password", entryPassword);
	g_object_set_data(G_OBJECT(vbox), "domain", entryDomain);
	g_object_set_data(G_OBJECT(vbox), "proxy", entryRoute);
	g_object_set_data(G_OBJECT(vbox), "errorstring", labelEmpty);
	g_signal_connect(G_OBJECT(entryUsername), "changed", (GCallback)external_account_configuration_changed, vbox);
	g_signal_connect(G_OBJECT(entryDomain), "changed", (GCallback)external_account_configuration_changed, vbox);
	g_signal_connect(G_OBJECT(entryRoute), "changed", (GCallback)external_account_configuration_changed, vbox);

	return vbox;
}

static void linphone_account_configuration_changed(GtkEntry *entry, GtkWidget *w) {
	GtkWidget *assistant = gtk_widget_get_toplevel(w);
	GtkEntry* username = GTK_ENTRY(g_object_get_data(G_OBJECT(w), "username"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), w, gtk_entry_get_text_length(username) > 0);
}

static GtkWidget * create_linphone_account_configuration_page(void) {
	GtkWidget *vbox = gtk_table_new(3, 2, TRUE);
	GtkWidget *label = gtk_label_new(_("Enter your linphone.org username"));
	GdkColor color;
	GtkWidget *labelEmpty;
	GtkWidget *labelUsername;
	GtkWidget *entryUsername;
	GtkWidget *labelPassword;
	GtkWidget *entryPassword;

	gdk_color_parse("red", &color);
	labelEmpty = gtk_label_new(NULL);
	gtk_widget_modify_fg(labelEmpty, GTK_STATE_NORMAL, &color);

	labelUsername = gtk_label_new(_("Username:"));
	entryUsername = gtk_entry_new();
	labelPassword = gtk_label_new(_("Password:"));
	entryPassword = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(entryPassword), FALSE);

	gtk_table_attach_defaults(GTK_TABLE(vbox), label, 0, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelUsername, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(vbox), entryUsername, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelPassword, 0, 1, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(vbox), entryPassword, 1, 2, 2, 3);

	gtk_widget_show_all(vbox);
	g_object_set_data(G_OBJECT(vbox), "username", entryUsername);
	g_object_set_data(G_OBJECT(vbox), "password", entryPassword);
	g_object_set_data(G_OBJECT(vbox), "errorstring", labelEmpty);
	g_signal_connect(G_OBJECT(entryUsername), "changed", (GCallback)linphone_account_configuration_changed, vbox);
	return vbox;
}

static int linphone_account_creation_configuration_correct(GtkWidget *w) {
	int is_username_available = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "is_username_available"));
	int is_email_correct = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "is_email_correct"));
	int is_password_correct = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "is_password_correct"));
	if ((is_username_available == 1) && (is_email_correct == 1) && (is_password_correct == 1)) {
		return 1;
	}
	return 0;
}

static gboolean update_interface_with_username_availability(void *w) {
	GtkWidget *assistant = gtk_widget_get_toplevel(GTK_WIDGET(w));
	GtkImage* isUsernameOk = GTK_IMAGE(g_object_get_data(G_OBJECT(w), "usernameOk"));
	GtkLabel* usernameError = GTK_LABEL(g_object_get_data(G_OBJECT(w), "error"));
	int account_existing = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "is_username_used"));

	if (account_existing == 0) {
		GdkPixbuf *ok_pixbuf = GDK_PIXBUF(g_object_get_data(G_OBJECT(the_assistant), "ok_pixbuf"));
		g_object_set_data(G_OBJECT(w), "is_username_available", GINT_TO_POINTER(1));
		gtk_image_set_from_pixbuf(isUsernameOk, ok_pixbuf);
		gtk_label_set_text(usernameError, "");
	} else {
		GdkPixbuf *nok_pixbuf = GDK_PIXBUF(g_object_get_data(G_OBJECT(the_assistant), "nok_pixbuf"));
		gtk_label_set_text(usernameError, "Username is already in use!");
		g_object_set_data(G_OBJECT(w), "is_username_available", GINT_TO_POINTER(0));
		gtk_image_set_from_pixbuf(isUsernameOk, nok_pixbuf);
	}
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), GTK_WIDGET(w), linphone_account_creation_configuration_correct(GTK_WIDGET(w)) > 0);
	return FALSE;
}

static void linphone_gtk_test_account_existence_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status) {
	GtkWidget *assistant = (GtkWidget *)linphone_account_creator_get_user_data(creator);
	GtkWidget *page = g_object_get_data(G_OBJECT(assistant), "linphone_account_creation_configuration");
	int account_existing = (status != LinphoneAccountCreatorOk);
	g_object_set_data(G_OBJECT(page), "is_username_used", GINT_TO_POINTER(account_existing));
	gdk_threads_add_idle((GSourceFunc)update_interface_with_username_availability, (void*)page);
}

static gboolean check_username_availability(GtkWidget *assistant) {
	LinphoneAccountCreator *creator = linphone_gtk_assistant_get_creator(assistant);
	GtkWidget *page = GUINT_TO_POINTER(g_object_get_data(G_OBJECT(assistant), "linphone_account_creation_configuration"));
	g_object_set_data(G_OBJECT(page), "usernameAvailabilityTimerID", GUINT_TO_POINTER(0));
	linphone_account_creator_test_existence(creator);
	return FALSE;
}

static void linphone_account_creation_username_changed(GtkEntry *entry, GtkWidget *w) {
	GtkWidget *assistant = gtk_widget_get_toplevel(GTK_WIDGET(w));
	GtkEntry* username = GTK_ENTRY(g_object_get_data(G_OBJECT(w), "username"));
	GtkImage* isUsernameOk = GTK_IMAGE(g_object_get_data(G_OBJECT(w), "usernameOk"));
	GtkLabel* usernameError = GTK_LABEL(g_object_get_data(G_OBJECT(w), "error"));

	LinphoneAccountCreator *creator = linphone_gtk_assistant_get_creator(assistant);
	linphone_account_creator_set_username(creator, gtk_entry_get_text(username));
	linphone_account_creator_set_domain(creator, "sip.linphone.org");
	linphone_account_creator_set_route(creator, "sip.linphone.org");

	if (g_regex_match_simple("^[a-zA-Z]+[a-zA-Z0-9.\\-_]{3,}$", gtk_entry_get_text(username), 0, 0)) {
		guint timerID = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(w), "usernameAvailabilityTimerID"));
		if (timerID > 0) {
			g_source_remove(timerID);
		}
		timerID = g_timeout_add(500, (GSourceFunc)check_username_availability, assistant);
		g_object_set_data(G_OBJECT(w), "usernameAvailabilityTimerID", GUINT_TO_POINTER(timerID));
	} else {
		GdkPixbuf *nok_pixbuf = GDK_PIXBUF(g_object_get_data(G_OBJECT(the_assistant), "nok_pixbuf"));
		if (gtk_entry_get_text_length(username) < LOGIN_MIN_SIZE) {
			gtk_label_set_text(usernameError, "Username is too short");
		} else if (!g_regex_match_simple("^[a-zA-Z]+[a-zA-Z0-9.\\-_]{3,}$", gtk_entry_get_text(username), 0, 0)) {
			gtk_label_set_text(usernameError, "Unauthorized username");
		}
		g_object_set_data(G_OBJECT(w), "is_username_available", GINT_TO_POINTER(0));
		gtk_image_set_from_pixbuf(isUsernameOk, nok_pixbuf);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), w, linphone_account_creation_configuration_correct(w) > 0);
	}
}

static void linphone_account_creation_email_changed(GtkEntry *entry, GtkWidget *w) {
	GtkEntry* email = GTK_ENTRY(g_object_get_data(G_OBJECT(w), "email"));
	GtkImage* isEmailOk = GTK_IMAGE(g_object_get_data(G_OBJECT(w), "emailOk"));
	GtkWidget *assistant = gtk_widget_get_toplevel(w);

	if (g_regex_match_simple("^[a-z0-9]([a-z0-9_\\+\\.-]+)@[a-z0-9]([a-z0-9\\.-]+)\\.[a-z]{2,}$", gtk_entry_get_text(email), 0, 0)) {
		GdkPixbuf *ok_pixbuf = GDK_PIXBUF(g_object_get_data(G_OBJECT(the_assistant), "ok_pixbuf"));
		g_object_set_data(G_OBJECT(w), "is_email_correct", GINT_TO_POINTER(1));
		gtk_image_set_from_pixbuf(isEmailOk, ok_pixbuf);
	} else {
		GdkPixbuf *nok_pixbuf = GDK_PIXBUF(g_object_get_data(G_OBJECT(the_assistant), "nok_pixbuf"));
		g_object_set_data(G_OBJECT(w), "is_email_correct", GINT_TO_POINTER(0));
		gtk_image_set_from_pixbuf(isEmailOk, nok_pixbuf);
	}
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), w, linphone_account_creation_configuration_correct(w) > 0);
}

static void linphone_account_creation_password_changed(GtkEntry *entry, GtkWidget *w) {
	GtkEntry* password = GTK_ENTRY(g_object_get_data(G_OBJECT(w), "password"));
	GtkImage* isPasswordOk = GTK_IMAGE(g_object_get_data(G_OBJECT(w), "passwordOk"));
	GtkEntry* password_confirm = GTK_ENTRY(g_object_get_data(G_OBJECT(w), "password_confirm"));
	GtkWidget *assistant = gtk_widget_get_toplevel(w);
	GtkLabel* passwordError = GTK_LABEL(g_object_get_data(G_OBJECT(w), "error"));

	if ((gtk_entry_get_text_length(password) >= PASSWORD_MIN_SIZE)
		&& (g_ascii_strcasecmp(gtk_entry_get_text(password), gtk_entry_get_text(password_confirm)) == 0)) {
		GdkPixbuf *ok_pixbuf = GDK_PIXBUF(g_object_get_data(G_OBJECT(the_assistant), "ok_pixbuf"));
		g_object_set_data(G_OBJECT(w), "is_password_correct", GINT_TO_POINTER(1));
		gtk_image_set_from_pixbuf(isPasswordOk, ok_pixbuf);
		gtk_label_set_text(passwordError, "");
	} else {
		GdkPixbuf *nok_pixbuf = GDK_PIXBUF(g_object_get_data(G_OBJECT(the_assistant), "nok_pixbuf"));
		if (gtk_entry_get_text_length(password) < PASSWORD_MIN_SIZE) {
			gtk_label_set_text(passwordError, "Password is too short !");
		} else if (!g_ascii_strcasecmp(gtk_entry_get_text(password), gtk_entry_get_text(password_confirm)) == 0) {
			gtk_label_set_text(passwordError, "Passwords don't match !");
		}
		g_object_set_data(G_OBJECT(w), "is_password_correct", GINT_TO_POINTER(0));
		gtk_image_set_from_pixbuf(isPasswordOk, nok_pixbuf);
	}
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), w, linphone_account_creation_configuration_correct(w) > 0);
}

static GtkWidget * create_linphone_account_creation_configuration_page(void) {
	GtkWidget *vbox = gtk_table_new(7, 3, FALSE);
	GdkPixbuf *nok_pixbuf = GDK_PIXBUF(g_object_get_data(G_OBJECT(the_assistant), "nok_pixbuf"));
	GtkWidget *label = gtk_label_new(_("(*) Required fields"));
	GtkWidget *labelUsername = gtk_label_new(_("Username: (*)"));
	GtkWidget *isUsernameOk = gtk_image_new_from_pixbuf(nok_pixbuf);
	GtkWidget *labelPassword = gtk_label_new(_("Password: (*)"));
	GtkWidget *isPasswordOk = gtk_image_new_from_pixbuf(nok_pixbuf);
	GtkWidget *labelEmail = gtk_label_new(_("Email: (*)"));
	GtkWidget *isEmailOk = gtk_image_new_from_pixbuf(nok_pixbuf);
	GtkWidget *labelPassword2 = gtk_label_new(_("Confirm your password: (*)"));
	GtkWidget *entryUsername = gtk_entry_new();
	GtkWidget *entryPassword = gtk_entry_new();
	GtkWidget *entryEmail;
	GtkWidget *entryPassword2;
	GtkWidget *checkNewsletter;
	GtkWidget *labelError;
	GtkWidget *passwordVbox1;
	GtkWidget *passwordVbox2;
	GdkColor color;

	gtk_entry_set_visibility(GTK_ENTRY(entryPassword), FALSE);
	entryEmail = gtk_entry_new();
	entryPassword2 = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(entryPassword2), FALSE);
	checkNewsletter = gtk_check_button_new_with_label(_("Keep me informed with linphone updates"));

	gdk_color_parse("red", &color);
	labelError = gtk_label_new(NULL);
	gtk_widget_modify_fg(labelError, GTK_STATE_NORMAL, &color);

	passwordVbox1 = gtk_vbox_new(FALSE, 2);
	passwordVbox2 = gtk_vbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX (passwordVbox1), labelPassword, TRUE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX (passwordVbox1), labelPassword2, TRUE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX (passwordVbox2), entryPassword, TRUE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX (passwordVbox2), entryPassword2, TRUE, FALSE, 2);

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
	g_object_set_data(G_OBJECT(vbox), "username", entryUsername);
	g_object_set_data(G_OBJECT(vbox), "password", entryPassword);
	g_object_set_data(G_OBJECT(vbox), "email", entryEmail);
	g_object_set_data(G_OBJECT(vbox), "usernameOk", isUsernameOk);
	g_object_set_data(G_OBJECT(vbox), "passwordOk", isPasswordOk);
	g_object_set_data(G_OBJECT(vbox), "emailOk", isEmailOk);
	g_object_set_data(G_OBJECT(vbox), "password_confirm", entryPassword2);
	g_object_set_data(G_OBJECT(vbox), "newsletter", checkNewsletter);
	g_object_set_data(G_OBJECT(vbox), "error", labelError);
	g_signal_connect(G_OBJECT(entryUsername), "changed", (GCallback)linphone_account_creation_username_changed, vbox);
	g_signal_connect(G_OBJECT(entryPassword), "changed", (GCallback)linphone_account_creation_password_changed, vbox);
	g_signal_connect(G_OBJECT(entryEmail), "changed", (GCallback)linphone_account_creation_email_changed, vbox);
	g_signal_connect(G_OBJECT(entryPassword2), "changed", (GCallback)linphone_account_creation_password_changed, vbox);
	return vbox;
}

static GtkWidget * create_linphone_account_creation_in_progress_page(void) {
	GtkWidget *vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget *label = gtk_label_new(_("Your account is being created, please wait."));
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 2);
	gtk_widget_show_all(vbox);
	return vbox;
}

static GtkWidget * create_linphone_account_validation_wait_page(void) {
	GtkWidget *vbox = gtk_table_new(2, 1, FALSE);
	GtkWidget *label = gtk_label_new(_("Please validate your account by clicking on the link we just sent you by email.\n"
		"Then come back here and press Next button."));
	gtk_table_attach(GTK_TABLE(vbox), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 100);
	g_object_set_data(G_OBJECT(vbox), "label", label);
	gtk_widget_show_all(vbox);
	return vbox;
}

static GtkWidget * create_linphone_account_validation_check_in_progress_page(void) {
	GtkWidget *vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget *label = gtk_label_new(_("Checking if your account is been validated, please wait."));
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 2);
	gtk_widget_show_all(vbox);
	return vbox;
}

static GtkWidget * create_error_page(void) {
	GtkWidget *vbox = gtk_table_new(2, 1, FALSE);
	GtkWidget *label = gtk_label_new(_("Error, account not validated, username already used or server unreachable.\nPlease go back and try again."));
	gtk_table_attach(GTK_TABLE(vbox), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 100);
	g_object_set_data(G_OBJECT(vbox), "label", label);
	gtk_widget_show_all(vbox);
	return vbox;
}

static GtkWidget * create_finish_page(void) {
	GtkWidget *vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget *label = gtk_label_new(_("Thank you. Your account is now configured and ready for use."));
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 2);
	gtk_widget_show_all(vbox);
	return vbox;
}

static LinphoneAccountCreator * linphone_gtk_assistant_init(GtkWidget *w) {
	LinphoneAccountCreator *creator = linphone_account_creator_new(linphone_gtk_get_core(), "https://www.linphone.org/wizard.php");
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	linphone_account_creator_set_user_data(creator, w);
	linphone_account_creator_cbs_set_existence_tested(cbs, linphone_gtk_test_account_existence_cb);
	linphone_account_creator_cbs_set_validation_tested(cbs, linphone_gtk_test_account_validation_cb);
	linphone_account_creator_cbs_set_validated(cbs, linphone_gtk_account_validate_cb);
	g_object_set_data(G_OBJECT(w), "creator", creator);
	return creator;
}

void linphone_gtk_show_assistant(GtkWidget *parent) {
	GtkWidget *w;
	GtkWidget *page_0_intro;
	GtkWidget *page_1_choice;
	GtkWidget *page_2_external_account_configuration;
	GtkWidget *page_3_linphone_account_configuration;
	GtkWidget *page_4_linphone_account_creation_configuration;
	GtkWidget *page_5_linphone_account_creation_in_progress;
	GtkWidget *page_6_linphone_account_validation_wait;
	GtkWidget *page_7_linphone_account_validation_check_in_progress;
	GtkWidget *page_8_error;
	GtkWidget *page_9_finish;
	GdkPixbuf *ok_pixbuf;
	GdkPixbuf *nok_pixbuf;

	if (the_assistant != NULL) return;

	w = the_assistant = gtk_assistant_new();
	gtk_window_set_resizable (GTK_WINDOW(w), FALSE);
	gtk_window_set_title(GTK_WINDOW(w), _("SIP account configuration assistant"));

	ok_pixbuf = create_pixbuf(linphone_gtk_get_ui_config("ok", "ok.png"));
	g_object_set_data_full(G_OBJECT(the_assistant), "ok_pixbuf", ok_pixbuf, g_object_unref);
	nok_pixbuf = create_pixbuf(linphone_gtk_get_ui_config("notok", "notok.png"));
	g_object_set_data_full(G_OBJECT(the_assistant), "nok_pixbuf", nok_pixbuf, g_object_unref);

	page_0_intro = create_intro_page();
	page_1_choice = create_choice_page();
	page_2_external_account_configuration = create_external_account_configuration_page();
	page_3_linphone_account_configuration = create_linphone_account_configuration_page();
	page_4_linphone_account_creation_configuration = create_linphone_account_creation_configuration_page();
	page_5_linphone_account_creation_in_progress = create_linphone_account_creation_in_progress_page();
	page_6_linphone_account_validation_wait = create_linphone_account_validation_wait_page();
	page_7_linphone_account_validation_check_in_progress = create_linphone_account_validation_check_in_progress_page();
	page_8_error = create_error_page();
	page_9_finish = create_finish_page();

	linphone_gtk_assistant_init(w);
	gtk_assistant_append_page(GTK_ASSISTANT(w), page_0_intro);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w), page_0_intro, GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w), page_0_intro, _("Welcome to the account setup assistant"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w), page_0_intro, TRUE);

	gtk_assistant_append_page(GTK_ASSISTANT(w), page_1_choice);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w), page_1_choice, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w), page_1_choice, _("Account setup assistant"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w), page_1_choice, TRUE);

	gtk_assistant_append_page(GTK_ASSISTANT(w), page_2_external_account_configuration);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w), page_2_external_account_configuration, GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w), page_2_external_account_configuration, _("Configure your account (step 1/1)"));

	gtk_assistant_append_page(GTK_ASSISTANT(w), page_3_linphone_account_configuration);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w), page_3_linphone_account_configuration, GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w), page_3_linphone_account_configuration, _("Enter your sip username (step 1/1)"));

	gtk_assistant_append_page(GTK_ASSISTANT(w), page_4_linphone_account_creation_configuration);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w), page_4_linphone_account_creation_configuration, GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w), page_4_linphone_account_creation_configuration, _("Enter account information (step 1/2)"));
	g_object_set_data(G_OBJECT(w), "linphone_account_creation_configuration", page_4_linphone_account_creation_configuration);

	gtk_assistant_append_page(GTK_ASSISTANT(w), page_5_linphone_account_creation_in_progress);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w), page_5_linphone_account_creation_in_progress, GTK_ASSISTANT_PAGE_PROGRESS);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w), page_5_linphone_account_creation_in_progress, _("Account creation in progress"));

	gtk_assistant_append_page(GTK_ASSISTANT(w), page_6_linphone_account_validation_wait);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w), page_6_linphone_account_validation_wait, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w), page_6_linphone_account_validation_wait, _("Validation (step 2/2)"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w), page_6_linphone_account_validation_wait, TRUE);

	gtk_assistant_append_page(GTK_ASSISTANT(w), page_7_linphone_account_validation_check_in_progress);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w), page_7_linphone_account_validation_check_in_progress, GTK_ASSISTANT_PAGE_PROGRESS);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w), page_7_linphone_account_validation_check_in_progress, _("Account validation check in progress"));

	gtk_assistant_append_page(GTK_ASSISTANT(w), page_8_error);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w), page_8_error, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w), page_8_error, _("Error"));

	gtk_assistant_append_page(GTK_ASSISTANT(w), page_9_finish);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w), page_9_finish, GTK_ASSISTANT_PAGE_SUMMARY);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w), page_9_finish, _("Terminating"));

	gtk_assistant_set_forward_page_func(GTK_ASSISTANT(w), linphone_gtk_assistant_forward, w, NULL);
	g_signal_connect(G_OBJECT(w), "close", (GCallback)linphone_gtk_assistant_closed, NULL);
	g_signal_connect(G_OBJECT(w), "cancel", (GCallback)linphone_gtk_assistant_closed, NULL);
	g_signal_connect(G_OBJECT(w), "prepare", (GCallback)linphone_gtk_assistant_prepare, NULL);

	gtk_window_set_transient_for(GTK_WINDOW(the_assistant), GTK_WINDOW(linphone_gtk_get_main_window()));

	gtk_widget_show(w);
}

void linphone_gtk_close_assistant(void) {
	GtkWidget *mw;
	if (the_assistant == NULL) {
		return;
	}
	gtk_widget_destroy(the_assistant);
	the_assistant = NULL;

	//reload list of proxy configs because a new one was probably created...
	mw=linphone_gtk_get_main_window();
	if (mw) {
		GtkWidget* pb = (GtkWidget*)g_object_get_data(G_OBJECT(mw),"parameters");
		if (pb) {
			linphone_gtk_show_sip_accounts(pb);
		}
	}
}
