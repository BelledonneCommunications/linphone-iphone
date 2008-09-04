/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@free.fr)

callbacks.c -- gtk callbacks, and osipua callbacks.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "linphone.h"
#include <gdk/gdkkeysyms.h>

#define get_core()	(uiobj->core)
#define get_main_window() (&uiobj->main_window)
#define get_uiobj()	(uiobj)

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *about2;
  about2 = create_about2 ();
  gtk_widget_show (about2);
}


gint
on_prop1_close                         (GnomeDialog     *gnomedialog,
                                        gpointer         user_data)
{	
	LinphoneMainWindow *obj=get_main_window();
	gnome_appbar_clear_stack( GNOME_APPBAR(obj->status_bar));
	
	return(FALSE);
}

void
on_parametres1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	LinphoneGnomeUI *ui=get_uiobj();
	linphone_property_box_init(&ui->propbox);
}



void
on_user_manual1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gnome_help_display("index.xml",NULL,NULL);
}


gboolean
on_play_vol_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	int vol;
	vol=(gtk_range_get_adjustment(GTK_RANGE(widget)))->value;
	linphone_core_set_play_level(get_core(),vol);
	return FALSE;
}


gboolean
on_rec_vol_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	int vol;
	vol=(gtk_range_get_adjustment(GTK_RANGE(widget)))->value;
	linphone_core_set_rec_level(get_core(),vol);
	return FALSE;
}


gboolean
on_ring_vol_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	int vol;
	vol=(gtk_range_get_adjustment(GTK_RANGE(widget)))->value;
	linphone_core_set_ring_level(get_core(),vol);
	return FALSE;
}


void
on_prop1_help                          (GnomePropertyBox *gnomepropertybox,
                                        gint             arg1,
                                        gpointer         user_data)
{
	gnome_help_display("index.html",NULL,NULL);
}




void
on_fermer1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	linphone_gnome_ui_hide(get_uiobj());
}



#if 0
/*this is when the panel size changes*/
void applet_change_pixel_size(GtkWidget *w, int size, gpointer data)
{
	GtkWidget *pixmap,*button;
	
	pixmap=gtk_object_get_data(GTK_OBJECT(applet),"applet_pixmap");	
	button=(GtkWidget*)gtk_object_get_data(GTK_OBJECT(applet),"applet_button");	
	if (button==NULL)
	{
		printf("Cannot find applet button\n");
		return;
	}
	if (pixmap!=NULL) gtk_widget_destroy(pixmap);
	pixmap = gnome_pixmap_new_from_xpm_d_at_size(linphone2_xpm,
							 size-4, size-4);
	gtk_object_set_data(GTK_OBJECT(applet),"applet_pixmap",pixmap);
  	gtk_widget_show(pixmap);
  	gtk_container_add(GTK_CONTAINER(button), pixmap);	
}
#endif

void
on_adresse_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	show_address_book();
}

void on_address_book_show(GtkWidget *widget,gpointer user_data)
{
	
}



void
on_showmore_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	gint state;
	GtkWidget *optioncontrols=get_main_window()->optioncontrols;
	state=gtk_toggle_button_get_active(togglebutton);
	if (state) gtk_widget_show(optioncontrols);
	else {
		gtk_widget_hide(optioncontrols);
	}
}


void
on_useRPC_toggled                      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
#ifdef VINCENT_MAURY_RSVP
	LinphoneCore *lc=get_core();
	gboolean state;
	state=gtk_toggle_button_get_active(togglebutton);
	/* change RPC settings according to state */
	if (state)
	{
		if (linphone_core_set_rpc_mode(lc,1)!=0) /* set rpc on */
		{
			printf("RPC error. unable to set rpc on !\n");
			printf("Check to see if RPC server is running\n");
			gtk_toggle_button_set_active(togglebutton,FALSE);
			/*linphone_core_set_rpc_mode(lc,0);*/
		}
	}
	else
	{
		if (linphone_core_set_rpc_mode(lc,0)!=0) /* set rpc off */
			printf("RPC error. That's impossible !!\n");
	}
#endif
}

void
on_useRSVP_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
#ifdef VINCENT_MAURY_RSVP
	LinphoneCore *lc=get_core();
	LinphoneGnomeUI *ui=get_uiobj();
	gboolean state;
	state=gtk_toggle_button_get_active(togglebutton);
	/* change the QoS settings function of the state */
	if (state)
	{
		linphone_core_set_rsvp_mode(lc,1); /* set RSVP on */
		gtk_widget_show(lookup_widget(ui->propbox.prop,"useRPC")); /* show RPC checkbox */
	}
	else
	{
		linphone_core_set_rsvp_mode(lc,0); /* set RSVP off */
		/* uncheck RPC if necessary and hide RPC checkbox */
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
					lookup_widget(ui->propbox.prop,"useRPC")),FALSE);
		gtk_widget_hide(lookup_widget(ui->propbox.prop,"useRPC"));
	}
#endif
}

#ifdef VINCENT_MAURY_RSVP
/* callback called when you click the yes/no dialog box
 * send yes or no to the core_change_qos which knows the question
 * and will be able to ajust qos */
void dialog_click (GtkDialog *dialog,gint arg1,gpointer user_data)
{
	LinphoneCore *lc=get_core();
	if (lc->call==NULL)
		return;
	
	if (arg1==GTK_RESPONSE_YES)
	{
		printf("YES\n");
		linphone_core_change_qos(lc, 1); /* 1 = yes */
	}
	else 
	{
		printf("NO\n");
		linphone_core_change_qos(lc, 0); /* 0 = no */
	}
	gtk_widget_destroy((GtkWidget*)dialog);
}
#endif


void
on_alt_href_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	gchar *url;
	GtkWidget *label;
	osip_from_t * from;
	LinphoneGnomeUI *ui=get_uiobj();
	label=GTK_BIN(button)->child;
	gtk_label_get(GTK_LABEL(label),&url);
	osip_from_init(&from);
	if ( osip_from_parse(from,url) <0){
		/* do something here */
	}else
	{ /* it was a sip url, so display it in the entry*/
		gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(ui->main_window.addressentry))),url);
	}
	osip_from_free(from);
}

void
on_alt_href_realize                    (GtkWidget       *widget,
                                        gpointer         user_data)
{
  GdkCursor *cursor = gdk_cursor_new(GDK_HAND2);
  gdk_window_set_cursor(widget->window, cursor);
  gdk_cursor_destroy(cursor);
}


void
on_dtmf_3_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dtmf_entry=get_main_window()->dtmfentry;
	gtk_entry_append_text(GTK_ENTRY(dtmf_entry),"3");
}


void
on_dmtf_2_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dtmf_entry=get_main_window()->dtmfentry;
	gtk_entry_append_text(GTK_ENTRY(dtmf_entry),"2");
}


void
on_dtmf_1_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dtmf_entry=get_main_window()->dtmfentry;
	gtk_entry_append_text(GTK_ENTRY(dtmf_entry),"1");
}


void
on_dtmf_4_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dtmf_entry=get_main_window()->dtmfentry;
	gtk_entry_append_text(GTK_ENTRY(dtmf_entry),"4");
}


void
on_dtmf_5_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dtmf_entry=get_main_window()->dtmfentry;
	gtk_entry_append_text(GTK_ENTRY(dtmf_entry),"5");
}


void
on_dtmf_6_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dtmf_entry=get_main_window()->dtmfentry;
	gtk_entry_append_text(GTK_ENTRY(dtmf_entry),"6");
}


void
on_dtmf_7_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dtmf_entry=get_main_window()->dtmfentry;
	gtk_entry_append_text(GTK_ENTRY(dtmf_entry),"7");
}


void
on_dtmf_8_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dtmf_entry=get_main_window()->dtmfentry;
	gtk_entry_append_text(GTK_ENTRY(dtmf_entry),"8");
	
}


void
on_dtmf_9_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dtmf_entry=get_main_window()->dtmfentry;
	gtk_entry_append_text(GTK_ENTRY(dtmf_entry),"9");
	
}


void
on_dtmf_star_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dtmf_entry=get_main_window()->dtmfentry;
	gtk_entry_append_text(GTK_ENTRY(dtmf_entry),"*");
	
}


void
on_dtmf_0_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dtmf_entry=get_main_window()->dtmfentry;
	gtk_entry_append_text(GTK_ENTRY(dtmf_entry),"0");
	
}


void
on_dtmf_pound_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *dtmf_entry=get_main_window()->dtmfentry;
	gtk_entry_append_text(GTK_ENTRY(dtmf_entry),"#");
	
}


void
on_dtmf_entry_changed                  (GtkEditable     *editable,
                                        gpointer         user_data)
{
	gchar *dtmfs;
	gint len;
	/* get the last entry in the text box and plays it */
	dtmfs=gtk_editable_get_chars(editable,0,-1);
	g_return_if_fail(dtmfs!=NULL);
	len=strlen(dtmfs);
	if (len>0){
		g_message("Sending dtmf %c",dtmfs[len-1]);
		linphone_core_send_dtmf(get_uiobj()->core,dtmfs[len-1]);
	}
	g_free(dtmfs);
}


void
on_exit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
#ifdef LINPHONE_APPLET
#else	
	gtk_widget_destroy(get_uiobj()->main_window.window);
#endif
}


void on_app1_destroy(GtkWidget *app1, gpointer user_data)
{
#ifdef LINPHONE_APPLET
#else	
	gtk_main_quit();
#endif
}

void
on_display_ab_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	show_address_book();
}


void
on_inc_subscr_dialog_response          (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
	LinphoneFriend *lf=(LinphoneFriend*)g_object_get_data(G_OBJECT(dialog),"friend_ref");
	switch(response_id){
		case GTK_RESPONSE_ACCEPT:
			subscriber_edit(lf);
			break;
		case GTK_RESPONSE_REJECT:
			linphone_core_reject_subscriber(get_core(),lf);
			break;
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void authentication_dialog_ok(GtkWidget *w)
{
	gchar *realm,*username,*userid,*passwd;
	LinphoneAuthInfo *info;
	realm=gtk_editable_get_chars(GTK_EDITABLE(lookup_widget(w,"realm")),0,-1);
	username=gtk_editable_get_chars(GTK_EDITABLE(lookup_widget(w,"username")),0,-1);	
	userid=gtk_editable_get_chars(GTK_EDITABLE(lookup_widget(w,"userid")),0,-1);
	passwd=gtk_editable_get_chars(GTK_EDITABLE(lookup_widget(w,"passwd")),0,-1);
	info=linphone_auth_info_new(username,userid,passwd,NULL,realm);
	linphone_core_add_auth_info(get_core(),info);
	g_free(username);
	g_free(userid);
	g_free(passwd);
	g_free(realm);
}

void
on_authentication_dialog_response      (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
	switch(response_id){
		case GTK_RESPONSE_OK:
			authentication_dialog_ok(GTK_WIDGET(dialog));
			gtk_widget_destroy(GTK_WIDGET(dialog));
			break;
	}
}

void
on_clear_auth_info_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
	linphone_core_clear_all_auth_info(get_core());
}


void
on_call_history_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	linphone_gnome_show_call_logs_window(get_uiobj());
}


void
on_call_logs_response                  (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
	gtk_widget_destroy(GTK_WIDGET(dialog));
}


void
on_call_logs_destroy                   (GtkObject       *object,
                                        gpointer         user_data)
{
	get_uiobj()->logs=NULL;
}


static void completion_add_text(GtkEntry *entry, const char *text){
	GtkTreeIter iter;
	GtkTreeModel *model=gtk_entry_completion_get_model(gtk_entry_get_completion(entry));
	
	if (gtk_tree_model_get_iter_first(model,&iter)){ 
		do {
			gchar *uri=NULL;
			gtk_tree_model_get(model,&iter,0,&uri,-1);
			if (uri!=NULL){
				if (strcmp(uri,text)==0) {
					/*remove text */
					gtk_list_store_remove(GTK_LIST_STORE(model),&iter);
					g_free(uri);
					break;
				}
				g_free(uri);
			}
		}while (gtk_tree_model_iter_next(model,&iter));
	}
	/* and prepend it on top of the list */
	gtk_list_store_prepend(GTK_LIST_STORE(model),&iter);
	gtk_list_store_set(GTK_LIST_STORE(model),&iter,0,text,-1);
}


void
on_callbutton_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	LinphoneGnomeUI *ui=get_uiobj();
	LinphoneCore *lc=get_core();
	GtkEntry *entry=GTK_ENTRY(ui->main_window.addressentry);
	
	if (lc->call==NULL){
		const gchar *sipurl=NULL;
		int err;
		/* we have no dialog in progress */
		/* get the url to call */
		sipurl=gtk_entry_get_text(entry);
		err=linphone_core_invite(lc,sipurl);
		if (err==0) completion_add_text(entry,sipurl);
	}else {
		linphone_core_accept_call(lc,NULL);
	}
}

void
on_hangup_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	LinphoneGnomeUI *ui=get_uiobj();
	LinphoneCore *lc=get_core();
	if (lc->call!=NULL){
		/* same trick here as for linphone_core_accept_dialog: defer it to gtk's idle loop */
		gtk_window_set_title(GTK_WINDOW(ui->main_window.window),"linphone");
		linphone_core_terminate_call(lc,NULL);
	}
}

GtkWidget *chatroom_new(const gchar *url, LinphoneChatRoom *cr){
	GtkWidget *gcr=NULL;
	if (cr==NULL)
		cr=linphone_core_create_chat_room(get_core(),url);
	if (cr!=NULL){
		gchar *tmp;
		gcr=create_chatroom();
		g_object_set_data(G_OBJECT(gcr),"chatroom",(gpointer)cr);
		linphone_chat_room_set_user_data(cr,(gpointer)gcr);
		tmp=g_strdup_printf(_("Chat with %s"),url);
		gtk_window_set_title(GTK_WINDOW(gcr),tmp);
		g_free(tmp);
	}
	return gcr;
}

void chatroom_append(GtkWidget *gcr, const gchar *from, const gchar *message){
	GtkTextBuffer *tb;
	gchar *str;
	GtkTextIter enditer;
	GtkTextView *tv=GTK_TEXT_VIEW(lookup_widget(gcr,"chattext"));
	tb=gtk_text_view_get_buffer(tv);
	g_return_if_fail(tb!=NULL);
	gtk_text_buffer_get_end_iter(tb,&enditer);
	str=g_strdup_printf("[%s]\t:%s\n",from,message);
	gtk_text_buffer_insert(tb,&enditer,str,strlen(str));
	g_free(str);
}

void chatroom_close(GtkWidget *gcr){
	LinphoneChatRoom *cr;
	cr=(LinphoneChatRoom*)g_object_get_data(G_OBJECT(gcr),"chatroom");
	linphone_chat_room_destroy(cr);
}


void
on_chat_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
	gchar *sipurl;
	
	sipurl=gtk_editable_get_chars(GTK_EDITABLE(get_main_window()->addressentry),0,-1);
	GtkWidget *gcr=chatroom_new(sipurl,NULL);
	if (gcr!=NULL) gtk_widget_show(gcr);
	g_free(sipurl);
}


void
on_chatbox_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(button)));
}



void
on_chatentry_activate                  (GtkEntry        *entry,
                                        gpointer         user_data)
{
	LinphoneChatRoom *cr;
	gchar *text;
	text=gtk_editable_get_chars(GTK_EDITABLE(entry),0,-1);
	if (strlen(text)>0){
		GtkWidget *gcr=gtk_widget_get_toplevel(GTK_WIDGET(entry));
		cr=(LinphoneChatRoom*)g_object_get_data(G_OBJECT(gcr),"chatroom");
		linphone_chat_room_send_message(cr,text);
		chatroom_append(gcr,linphone_core_get_primary_contact(get_core()),text);
		gtk_editable_delete_text(GTK_EDITABLE(entry),0,-1);
	}
}

void
on_chatroom_destroy                    (GtkObject       *object,
                                        gpointer         user_data)
{
	chatroom_close(GTK_WIDGET(object));
}



void
on_addressentry_activate               (GtkEntry        *entry,
                                        gpointer         user_data)
{
	on_callbutton_clicked(NULL,NULL);
}

void
on_addressentry_destroy                (GtkObject       *object,
                                        gpointer         user_data)
{
	linphone_gnome_save_uri_history(get_uiobj());
}

void
on_video_enabled_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	linphone_core_enable_video(get_core(),gtk_toggle_button_get_active(togglebutton));
}


void
on_echocancelation_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	linphone_core_enable_echo_cancelation(get_core(),
		gtk_toggle_button_get_active(togglebutton));
}




