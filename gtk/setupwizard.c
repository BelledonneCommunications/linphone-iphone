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
#include "regex.h"
#include <glib.h>
#include <glib/gprintf.h>

static const int PASSWORD_MIN_SIZE = 6;
static const int LOGIN_MIN_SIZE = 4;
static GtkWidget *the_assistant = NULL;


static LinphoneAccountCreator * linphone_gtk_assistant_get_creator(GtkWidget *w) {
	return (LinphoneAccountCreator *)g_object_get_data(G_OBJECT(w), "creator");
}

static void linphone_gtk_create_account_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char* resp) {
	GtkWidget *assistant = (GtkWidget *)linphone_account_creator_get_user_data(creator);
	if (status == LinphoneAccountCreatorAccountCreated) {
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
	linphone_account_creator_create_account(creator);
}

static void linphone_gtk_test_account_validation_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char* resp) {
	GtkWidget *assistant = (GtkWidget *)linphone_account_creator_get_user_data(creator);
	if (status == LinphoneAccountCreatorAccountActivated) {
		// Go to page_9_finish
		gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), 9);
	} else {
		// Go to page_8_error
		gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), 8);
	}
}

static void check_account_validation(GtkWidget *assistant) {
	LinphoneAccountCreator *creator = linphone_gtk_assistant_get_creator(assistant);
	linphone_account_creator_is_account_activated(creator);
}

void linphone_gtk_assistant_closed(GtkWidget *w) {
	linphone_gtk_close_assistant();
}

void linphone_gtk_assistant_prepare(GtkWidget *assistant) {
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
	LinphoneAccountCreator *creator = linphone_gtk_assistant_get_creator(w);

	switch (curpage) {
		case 0:
			curpage = 1; // Go to page_1_choice
			break;
		case 1:
			{
				GtkWidget *create_button = linphone_gtk_get_widget(w, "radio_create_account");
				GtkWidget *setup_linphone_account = linphone_gtk_get_widget(w, "radio_setup_lp_account");
				GtkWidget *setup_account = linphone_gtk_get_widget(w, "radio_setup_account");
				GtkWidget *config_uri = linphone_gtk_get_widget(w, "radio_config_uri");

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
		{
			GtkEntry *username_entry = GTK_ENTRY(linphone_gtk_get_widget(w, "p2_entry_username"));
			GtkEntry *domain_entry = GTK_ENTRY(linphone_gtk_get_widget(w, "p2_entry_domain"));
			GtkEntry *proxy_entry = GTK_ENTRY(linphone_gtk_get_widget(w, "p2_entry_proxy"));
			GtkEntry *password_entry = GTK_ENTRY(linphone_gtk_get_widget(w, "p2_entry_password"));
			linphone_account_creator_set_username(creator, gtk_entry_get_text(username_entry));
			linphone_account_creator_set_domain(creator, gtk_entry_get_text(domain_entry));
			linphone_account_creator_set_route(creator, gtk_entry_get_text(proxy_entry));
			linphone_account_creator_set_password(creator, gtk_entry_get_text(password_entry));
			curpage = 9; // Go to page_9_finish
			break;
		}
		case 3:
		{
			GtkEntry *username_entry = GTK_ENTRY(linphone_gtk_get_widget(w, "p3_entry_username"));
			GtkEntry *password_entry = GTK_ENTRY(linphone_gtk_get_widget(w, "p3_entry_password"));
			linphone_account_creator_set_username(creator, gtk_entry_get_text(username_entry));
			linphone_account_creator_set_domain(creator, "sip.linphone.org");
			linphone_account_creator_set_route(creator, "sip.linphone.org");
			linphone_account_creator_set_password(creator, gtk_entry_get_text(password_entry));
			curpage = 9; // Go to page_9_finish
			break;
		}
		case 4:
		{
			GtkEntry *password_entry = GTK_ENTRY(linphone_gtk_get_widget(w, "p4_entry_password1"));
			GtkEntry *username_entry = GTK_ENTRY(linphone_gtk_get_widget(w, "p4_entry_username"));
			GtkEntry *email_entry = GTK_ENTRY(linphone_gtk_get_widget(w, "p4_entry_email"));
			//GtkToggleButton *newsletter = GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w, "p4_check_newsletter"));
			linphone_account_creator_set_username(creator, gtk_entry_get_text(username_entry));
			linphone_account_creator_set_password(creator, gtk_entry_get_text(password_entry));
			linphone_account_creator_set_email(creator, gtk_entry_get_text(email_entry));
			//linphone_account_creator_enable_newsletter_subscription(creator, gtk_toggle_button_get_active(newsletter));
			curpage = 5; // Go to page_5_linphone_account_creation_in_progress
			break;
		}
		case 6:
			curpage = 7; // Go to page_7_linphone_account_validation_check_in_progress
			break;
		default:
			break;
	}
	return curpage;
}

static int external_account_configuration_complete(GtkWidget *page) {
	GtkWidget *assistant = gtk_widget_get_toplevel(page);
	GtkEntry* username = GTK_ENTRY(linphone_gtk_get_widget(assistant, "p2_entry_username"));
	GtkEntry* domain = GTK_ENTRY(linphone_gtk_get_widget(assistant, "p2_entry_domain"));

	if ((gtk_entry_get_text_length(username) > 0)
		&& (gtk_entry_get_text_length(domain) > 0)
		&& (g_regex_match_simple("^[a-zA-Z0-9+]+[a-zA-Z0-9.\\+\\-_]{2,}$", gtk_entry_get_text(username), 0, 0))
		&& (g_regex_match_simple("^(sip:)?([a-zA-Z0-9\\+]+([\\.-][a-zA-Z0-9+]+)*)$", gtk_entry_get_text(domain), 0, 0))) {
		return 1;
	}
	return 0;
}

void linphone_gtk_external_account_configuration_changed(GtkEntry *entry) {
	GtkWidget *assistant = gtk_widget_get_toplevel(GTK_WIDGET(entry));
	gint current_page_num = gtk_assistant_get_current_page(GTK_ASSISTANT(assistant));
	GtkWidget *page = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), current_page_num);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page, external_account_configuration_complete(page) > 0);
}

void linphone_gtk_account_configuration_changed(GtkEntry *entry, GtkAssistant *assistant) {
	gboolean complete = (gtk_entry_get_text_length(entry) > 0);
	GtkWidget *page = gtk_assistant_get_nth_page(assistant, gtk_assistant_get_current_page(assistant));
	gtk_assistant_set_page_complete(assistant, page, complete);
}

static gboolean linphone_account_creation_configuration_correct(GtkWidget *w) {
	gint is_username_available = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "is_username_available"));
	gint is_email_correct = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "is_email_correct"));
	gint is_password_correct = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "is_password_correct"));
	return (is_username_available && is_email_correct && is_password_correct);
}

static gboolean update_interface_with_username_availability(GtkWidget *page) {
	GtkWidget *assistant = gtk_widget_get_toplevel(page);
	GtkImage* isUsernameOk = GTK_IMAGE(linphone_gtk_get_widget(assistant, "p4_image_username_ok"));
	GtkLabel* usernameError = GTK_LABEL(linphone_gtk_get_widget(assistant, "p4_label_error"));
	int account_status = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "is_username_used"));

	if (account_status == LinphoneAccountCreatorAccountNotExist) {
		g_object_set_data(G_OBJECT(page), "is_username_available", GINT_TO_POINTER(1));
		gtk_image_set_from_stock(isUsernameOk, GTK_STOCK_OK, GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_label_set_text(usernameError, "");
	} else if (account_status == LinphoneAccountCreatorAccountExist) {
		gtk_label_set_text(usernameError, _("Username is already in use!"));
		g_object_set_data(G_OBJECT(page), "is_username_available", GINT_TO_POINTER(0));
		gtk_image_set_from_stock(isUsernameOk, GTK_STOCK_NO, GTK_ICON_SIZE_LARGE_TOOLBAR);
	} else {
		gtk_label_set_text(usernameError, _("Failed to check username availability. Please try again later."));
		g_object_set_data(G_OBJECT(page), "is_username_available", GINT_TO_POINTER(0));
		gtk_image_set_from_stock(isUsernameOk, GTK_STOCK_NO, GTK_ICON_SIZE_LARGE_TOOLBAR);
	}
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page, linphone_account_creation_configuration_correct(page) > 0);
	return FALSE;
}

static void linphone_gtk_test_account_existence_cb(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char* resp) {
	GtkWidget *assistant = (GtkWidget *)linphone_account_creator_get_user_data(creator);
	GtkWidget *page = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), gtk_assistant_get_current_page(GTK_ASSISTANT(assistant)));
	g_object_set_data(G_OBJECT(page), "is_username_used", GINT_TO_POINTER(status));
	gdk_threads_add_idle((GSourceFunc)update_interface_with_username_availability, page);
}

static gboolean check_username_availability(GtkWidget *assistant) {
	LinphoneAccountCreator *creator = linphone_gtk_assistant_get_creator(assistant);
	GtkWidget *page = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), gtk_assistant_get_current_page(GTK_ASSISTANT(assistant)));
	g_object_set_data(G_OBJECT(page), "usernameAvailabilityTimerID", GUINT_TO_POINTER(0));
	linphone_account_creator_is_account_used(creator);
	return FALSE;
}

void linphone_gtk_account_creation_username_changed(GtkEntry *entry) {
	GtkWidget *assistant = gtk_widget_get_toplevel(GTK_WIDGET(entry));
	GtkEntry* username = GTK_ENTRY(linphone_gtk_get_widget(assistant, "p4_entry_username"));
	GtkImage* isUsernameOk = GTK_IMAGE(linphone_gtk_get_widget(assistant, "p4_image_username_ok"));
	GtkLabel* usernameError = GTK_LABEL(linphone_gtk_get_widget(assistant, "p4_label_error"));
	gint current_page_num = gtk_assistant_get_current_page(GTK_ASSISTANT(assistant));
	GtkWidget *page = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), current_page_num);

	LinphoneAccountCreator *creator = linphone_gtk_assistant_get_creator(assistant);
	linphone_account_creator_set_username(creator, gtk_entry_get_text(username));
	linphone_account_creator_set_domain(creator, "sip.linphone.org");
	linphone_account_creator_set_route(creator, "sip.linphone.org");

	if (g_regex_match_simple("^[a-zA-Z]+[a-zA-Z0-9.\\-_]{3,}$", gtk_entry_get_text(username), 0, 0)) {
		guint timerID = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(page), "usernameAvailabilityTimerID"));
		if (timerID > 0) {
			g_source_remove(timerID);
		}
		timerID = g_timeout_add(500, (GSourceFunc)check_username_availability, assistant);
		g_object_set_data(G_OBJECT(page), "usernameAvailabilityTimerID", GUINT_TO_POINTER(timerID));
	} else {
		if (gtk_entry_get_text_length(username) < LOGIN_MIN_SIZE) {
			gtk_label_set_text(usernameError, "Username is too short");
		} else if (!g_regex_match_simple("^[a-zA-Z]+[a-zA-Z0-9.\\-_]{3,}$", gtk_entry_get_text(username), 0, 0)) {
			gtk_label_set_text(usernameError, "Unauthorized username");
		}
		g_object_set_data(G_OBJECT(page), "is_username_available", GINT_TO_POINTER(0));
		gtk_image_set_from_stock(isUsernameOk, GTK_STOCK_NO, GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page, linphone_account_creation_configuration_correct(page) > 0);
	}
}

void linphone_gtk_account_creation_email_changed(GtkEntry *entry) {
	GtkWidget *assistant = gtk_widget_get_toplevel(GTK_WIDGET(entry));
	GtkEntry* email = GTK_ENTRY(linphone_gtk_get_widget(assistant, "p4_entry_email"));
	GtkImage* isEmailOk = GTK_IMAGE(linphone_gtk_get_widget(assistant, "p4_image_email_ok"));
	gint current_page_num = gtk_assistant_get_current_page(GTK_ASSISTANT(assistant));
	GtkWidget *page = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), current_page_num);

	if (g_regex_match_simple("^" BC_REGEX_RESTRICTIVE_EMAIL_ADDR "$", gtk_entry_get_text(email), 0, 0)) {
		g_object_set_data(G_OBJECT(page), "is_email_correct", GINT_TO_POINTER(1));
		gtk_image_set_from_stock(isEmailOk, GTK_STOCK_OK, GTK_ICON_SIZE_LARGE_TOOLBAR);
	} else {
		g_object_set_data(G_OBJECT(page), "is_email_correct", GINT_TO_POINTER(0));
		gtk_image_set_from_stock(isEmailOk, GTK_STOCK_NO, GTK_ICON_SIZE_LARGE_TOOLBAR);
	}
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page, linphone_account_creation_configuration_correct(page) > 0);
}

void linphone_gtk_account_creation_password_changed(GtkEntry *entry) {
	GtkWidget *assistant = gtk_widget_get_toplevel(GTK_WIDGET(entry));
	GtkEntry* password = GTK_ENTRY(linphone_gtk_get_widget(assistant, "p4_entry_password1"));
	GtkEntry* password_confirm = GTK_ENTRY(linphone_gtk_get_widget(assistant, "p4_entry_password2"));
	GtkImage* isPasswordOk = GTK_IMAGE(linphone_gtk_get_widget(assistant, "p4_image_password_ok"));
	GtkLabel* passwordError = GTK_LABEL(linphone_gtk_get_widget(assistant, "p4_label_error"));
	gint current_page_num = gtk_assistant_get_current_page(GTK_ASSISTANT(assistant));
	GtkWidget *page = gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant), current_page_num);

	if ((gtk_entry_get_text_length(password) >= PASSWORD_MIN_SIZE)
		&& (g_ascii_strcasecmp(gtk_entry_get_text(password), gtk_entry_get_text(password_confirm)) == 0)) {
		g_object_set_data(G_OBJECT(page), "is_password_correct", GINT_TO_POINTER(1));
		gtk_image_set_from_stock(isPasswordOk, GTK_STOCK_OK, GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_label_set_text(passwordError, "");
	} else {
		if (gtk_entry_get_text_length(password) < PASSWORD_MIN_SIZE) {
			gtk_label_set_text(passwordError, "Password is too short !");
		} else if (g_ascii_strcasecmp(gtk_entry_get_text(password), gtk_entry_get_text(password_confirm)) != 0) {
			gtk_label_set_text(passwordError, "Passwords don't match !");
		}
		g_object_set_data(G_OBJECT(page), "is_password_correct", GINT_TO_POINTER(0));
		gtk_image_set_from_stock(isPasswordOk, GTK_STOCK_NO, GTK_ICON_SIZE_LARGE_TOOLBAR);
	}
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page, linphone_account_creation_configuration_correct(page) > 0);
}

static void linphone_gtk_assistant_init(GtkWidget *w) {
	LinphoneAccountCreator *creator = linphone_account_creator_new(linphone_gtk_get_core(), "https://subscribe.linphone.org:444/wizard.php");
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	linphone_account_creator_set_user_data(creator, w);
	linphone_account_creator_cbs_set_is_account_used(cbs, linphone_gtk_test_account_existence_cb);
	linphone_account_creator_cbs_set_is_account_activated(cbs, linphone_gtk_test_account_validation_cb);
	linphone_account_creator_cbs_set_create_account(cbs, linphone_gtk_create_account_cb);
	g_object_set_data(G_OBJECT(w), "creator", creator);

	gtk_assistant_set_forward_page_func(GTK_ASSISTANT(w), linphone_gtk_assistant_forward, w, NULL);
}

void linphone_gtk_show_assistant(void) {
	if (the_assistant != NULL) return;
	the_assistant = linphone_gtk_create_window("setup_wizard", linphone_gtk_get_main_window());
	linphone_gtk_assistant_init(the_assistant);
	gtk_widget_show(the_assistant);
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
