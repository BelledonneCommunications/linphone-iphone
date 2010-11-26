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
LinphoneAccountCreator *linphone_gtk_assistant_get_creator(GtkWidget*w);

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
	GtkWidget *t1=gtk_radio_button_new_with_label(NULL,_("Create an account by choosing a username"));
	GtkWidget *t2=gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(t1),_("I have already an account and just want to use it"));
	gtk_box_pack_start (GTK_BOX (vbox), t1, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), t2, TRUE, TRUE, 2);
	gtk_widget_show_all(vbox);
	g_object_set_data(G_OBJECT(vbox),"create_account",t1);
	g_object_set_data(G_OBJECT(vbox),"setup_account",t2);
	return vbox;
}

static void create_username_changed(GtkEntry *entry, GtkWidget *w){
	GtkWidget *assistant=gtk_widget_get_toplevel(w);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant),w,
		gtk_entry_get_text_length(entry)>=3);
}

static GtkWidget *create_username_chooser(){
	GtkWidget *vbox=gtk_vbox_new(FALSE,2);
	GtkWidget *hbox=gtk_hbox_new(FALSE,2);
	GtkWidget *label=gtk_label_new(_("Please choose a username:"));
	GtkWidget *label2=gtk_label_new(_("Username:"));
	GtkWidget *label3=gtk_label_new(NULL);
	GtkWidget *entry=gtk_entry_new();
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), label2, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), label3, TRUE, TRUE, 2);
	gtk_widget_show_all(vbox);
	g_object_set_data(G_OBJECT(vbox),"username",entry);
	g_object_set_data(G_OBJECT(vbox),"errorstring",label3);
	g_signal_connect(G_OBJECT(entry),"changed",(GCallback)create_username_changed,vbox);
	return vbox;
}

static GtkWidget *create_username_checking_page(){
	GtkWidget *vbox=gtk_vbox_new(FALSE,2);
	GtkWidget *label=gtk_label_new(NULL);
	GtkWidget *progress=gtk_progress_bar_new();
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), progress, TRUE, TRUE, 2);
	g_object_set_data(G_OBJECT(vbox),"label",label);
	g_object_set_data(G_OBJECT(vbox),"progress",progress);
	gtk_widget_show_all(vbox);
	return vbox;
}

static void *progress_bar_update(LinphoneCore *lc, void *ctx, LinphoneWaitingState ws, const char *purpose, float progress){
	GtkWidget *pb=(GtkWidget*)ctx;
	if (ws==LinphoneWaitingProgress) gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pb));
	else if (ws==LinphoneWaitingFinished) gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pb),1);
	return ctx;
}

static void check_username(GtkWidget *page){
	GtkWidget *progress=(GtkWidget*)g_object_get_data(G_OBJECT(page),"progress");
	GtkWidget *label=(GtkWidget*)g_object_get_data(G_OBJECT(page),"label");
	LinphoneAccountCreator *creator=linphone_gtk_assistant_get_creator(gtk_widget_get_toplevel(page));
	gchar *text=g_strdup_printf(_("Checking if '%s' is available..."),linphone_account_creator_get_username(creator));
	LinphoneAccountCreator *c=linphone_gtk_assistant_get_creator(gtk_widget_get_toplevel(page));
	int res;
	gtk_label_set_text(GTK_LABEL(label),text);
	g_free(text);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress),_("Please wait..."));
	linphone_core_set_waiting_callback(linphone_gtk_get_core(),progress_bar_update,progress);
	res=linphone_account_creator_test_existence(c);
	if (res==1){
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress),_("Sorry this username already exists. Please try a new one."));
	}else if (res==0){
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress),_("Ok !"));
		gtk_assistant_set_page_complete(GTK_ASSISTANT(gtk_widget_get_toplevel(page)),page,TRUE);
	}else if (res==-1){
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress),_("Communication problem, please try again later."));
	}
	linphone_core_set_waiting_callback(linphone_gtk_get_core(),linphone_gtk_wait,NULL);
}

static GtkWidget *create_confirmation_page(){
	GtkWidget *vbox=gtk_vbox_new(FALSE,2);
	GtkWidget *label=gtk_label_new(NULL);
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 2);
	g_object_set_data(G_OBJECT(vbox),"label",label);
	gtk_widget_show_all(vbox);
	return vbox;
}

static GtkWidget *create_creation_page(){
	GtkWidget *vbox=gtk_vbox_new(FALSE,2);
	GtkWidget *label=gtk_label_new(NULL);
	GtkWidget *progress=gtk_progress_bar_new();
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), progress, TRUE, TRUE, 2);
	g_object_set_data(G_OBJECT(vbox),"label",label);
	g_object_set_data(G_OBJECT(vbox),"progress",progress);
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

static void linphone_gtk_assistant_closed(GtkWidget *w){
	gtk_widget_destroy(w);
}

static int linphone_gtk_assistant_forward(int curpage, gpointer data){
	GtkWidget *w=(GtkWidget*)data;
	GtkWidget *box=gtk_assistant_get_nth_page(GTK_ASSISTANT(w),curpage);
	if (curpage==1){
		GtkWidget *create_button=(GtkWidget*)g_object_get_data(G_OBJECT(box),"create_account");
		if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(create_button))){
			g_error("Not implemented yet...");
		}
	}else if (curpage==2){
		LinphoneAccountCreator *c=linphone_gtk_assistant_get_creator(w);
		linphone_account_creator_set_username(c,gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(box),"username"))));
	}
	return curpage+1;
}

static void linphone_gtk_assistant_apply(GtkWidget *w){
	LinphoneAccountCreator *creator=linphone_gtk_assistant_get_creator(w);
	GtkWidget *page=gtk_assistant_get_nth_page(GTK_ASSISTANT(w),gtk_assistant_get_current_page(GTK_ASSISTANT(w)));
	GtkWidget *progress=(GtkWidget*)g_object_get_data(G_OBJECT(page),"progress");
	LinphoneProxyConfig *res;
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress),_("Please wait..."));
	linphone_core_set_waiting_callback(linphone_gtk_get_core(),progress_bar_update,progress);
	res=linphone_account_creator_validate(creator);
	if (res){
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress),_("Ok !"));
		gtk_assistant_set_page_complete(GTK_ASSISTANT(w),page,TRUE);
	}else{
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress),_("Communication problem, please try again later."));
	}
	linphone_core_set_waiting_callback(linphone_gtk_get_core(),linphone_gtk_wait,NULL);
	if (res) linphone_core_add_proxy_config(linphone_gtk_get_core(),res);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w),page,TRUE);
}

static void linphone_gtk_assistant_prepare(GtkWidget *assistant, GtkWidget *page){
	int pagenum=gtk_assistant_get_current_page(GTK_ASSISTANT(assistant));
	if (pagenum==3){
		check_username(page);
	}else if (pagenum==4){
		GtkWidget *label=(GtkWidget*)g_object_get_data(G_OBJECT(page),"label");
		LinphoneAccountCreator *creator=linphone_gtk_assistant_get_creator(assistant);
		gchar *text=g_strdup_printf("You have choosen '%s' as username.\nDo you confirm the creation of the account ?",linphone_account_creator_get_username(creator));
		gtk_label_set_text(GTK_LABEL(label),text);
		g_free(text);
	}else if (pagenum==5){
		GtkWidget *label=(GtkWidget*)g_object_get_data(G_OBJECT(page),"label");
		LinphoneAccountCreator *creator=linphone_gtk_assistant_get_creator(assistant);
		gchar *text=g_strdup_printf("Account creation in progress for '%s'",linphone_account_creator_get_username(creator));
		gtk_label_set_text(GTK_LABEL(label),text);
		g_free(text);
	}
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

LinphoneAccountCreator *linphone_gtk_assistant_get_creator(GtkWidget*w){
	return (LinphoneAccountCreator*)g_object_get_data(G_OBJECT(w),"creator");
}

GtkWidget * linphone_gtk_create_assistant(void){
	GtkWidget *w=gtk_assistant_new();
	GtkWidget *p1=create_intro();
	GtkWidget *p2=create_setup_signin_choice();
	GtkWidget *p3=create_username_chooser();
	GtkWidget *checking=create_username_checking_page();
	GtkWidget *confirm=create_confirmation_page();
	GtkWidget *creation=create_creation_page();
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
	gtk_assistant_append_page(GTK_ASSISTANT(w),p3);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),p3,GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),p3,_("Choosing a username"));
	
	gtk_assistant_append_page(GTK_ASSISTANT(w),checking);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),checking,GTK_ASSISTANT_PAGE_PROGRESS);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),checking,_("Verifying"));
	
	gtk_assistant_append_page(GTK_ASSISTANT(w),confirm);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),confirm,GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),confirm,_("Confirmation"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w),confirm,TRUE);

	gtk_assistant_append_page(GTK_ASSISTANT(w),creation);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),creation,GTK_ASSISTANT_PAGE_PROGRESS);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),creation,_("Creating your account"));

	gtk_assistant_append_page(GTK_ASSISTANT(w),end);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),end,GTK_ASSISTANT_PAGE_SUMMARY);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),end,_("Now ready !"));
	
	gtk_assistant_set_forward_page_func(GTK_ASSISTANT(w),linphone_gtk_assistant_forward,w,NULL);
	g_signal_connect(G_OBJECT(w),"close",(GCallback)linphone_gtk_assistant_closed,NULL);
	g_signal_connect(G_OBJECT(w),"cancel",(GCallback)linphone_gtk_assistant_closed,NULL);
	g_signal_connect(G_OBJECT(w),"apply",(GCallback)linphone_gtk_assistant_apply,NULL);
	g_signal_connect(G_OBJECT(w),"prepare",(GCallback)linphone_gtk_assistant_prepare,NULL);
	gtk_widget_show(w);
	
	return w;
}

