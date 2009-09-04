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
#include "sipsetup.h"

static void linphone_gtk_display_lookup_results(GtkWidget *w, const MSList *results);

enum {
	LOOKUP_RESULT_NAME,
	LOOKUP_RESULT_SIP_URI,
	LOOKUP_RESULT_ADDRESS,
	LOOKUP_RESULT_NCOL
};

void linphone_gtk_buddy_lookup_window_destroyed(GtkWidget *w){
	guint tid=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"typing_timeout"));
	if (tid!=0){
		g_source_remove(tid);
	}
	tid=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"buddylookup_processing"));
	if (tid!=0){
		g_source_remove(tid);
	}
}

static void enable_add_buddy_button(GtkWidget *w){
	gtk_widget_set_sensitive(linphone_gtk_get_widget(w,"add_buddy"),TRUE);
}

static void disable_add_buddy_button(GtkWidget *w){
	gtk_widget_set_sensitive(linphone_gtk_get_widget(w,"add_buddy"),FALSE);
}

GtkWidget * linphone_gtk_show_buddy_lookup_window(SipSetupContext *ctx){
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	GtkWidget *w=linphone_gtk_create_window("buddylookup");
	GtkWidget *results=linphone_gtk_get_widget(w,"search_results");
	GtkProgressBar *pb=GTK_PROGRESS_BAR(linphone_gtk_get_widget(w,"progressbar"));
	
	store = gtk_list_store_new(LOOKUP_RESULT_NCOL, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(results),GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Firstname, Lastname"),
                                                   renderer,
                                                   "text", LOOKUP_RESULT_NAME,
                                                   NULL);
	g_object_set (G_OBJECT(column), "resizable", TRUE, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (results), column);

	column = gtk_tree_view_column_new_with_attributes (_("SIP address"),
                                                   renderer,
                                                   "text", LOOKUP_RESULT_SIP_URI,
                                                   NULL);
	g_object_set (G_OBJECT(column), "resizable", TRUE, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (results), column);
	
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (results));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect_swapped(G_OBJECT(select),"changed",(GCallback)enable_add_buddy_button,w);
#if GTK_CHECK_VERSION(2,12,0)
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(results),LOOKUP_RESULT_ADDRESS);
#endif
	g_object_set_data(G_OBJECT(w),"SipSetupContext",ctx);
	g_object_weak_ref(G_OBJECT(w),(GWeakNotify)linphone_gtk_buddy_lookup_window_destroyed,w);
	//g_signal_connect_swapped(G_OBJECT(w),"destroy",(GCallback)linphone_gtk_buddy_lookup_window_destroyed,w);
	gtk_progress_bar_set_fraction(pb,0);
	gtk_progress_bar_set_text(pb,NULL);
	gtk_dialog_add_button(GTK_DIALOG(w),GTK_STOCK_CLOSE,GTK_RESPONSE_CLOSE);
	g_object_set_data(G_OBJECT(w),"last_state",GINT_TO_POINTER(-1));

	gtk_widget_show(w);
	return w;
}


void linphone_gtk_buddy_lookup_set_keyword(GtkWidget *w, const char *kw){
	gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"keyword")),kw);
}

static gboolean linphone_gtk_process_buddy_lookup(GtkWidget *w){
	BuddyLookupStatus bls;
	SipSetupContext *ctx;
	int last_state;
	gchar *tmp;
	MSList *results=NULL;
	GtkProgressBar *pb=GTK_PROGRESS_BAR(linphone_gtk_get_widget(w,"progressbar"));
	ctx=(SipSetupContext*)g_object_get_data(G_OBJECT(w),"SipSetupContext");
	last_state=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"last_state"));
	bls=sip_setup_context_get_buddy_lookup_status(ctx);
	if (last_state==bls) return TRUE;
	switch(bls){
		case BuddyLookupNone:
			gtk_progress_bar_set_fraction(pb,0);
			gtk_progress_bar_set_text(pb,NULL);
			break;
		case BuddyLookupFailure:
			gtk_progress_bar_set_fraction(pb,0);
			gtk_progress_bar_set_text(pb,_("Error communicating with server."));
			break;
		case BuddyLookupConnecting:
			gtk_progress_bar_set_fraction(pb,0.2);
			gtk_progress_bar_set_text(pb,_("Connecting..."));
			break;
		case BuddyLookupConnected:
			gtk_progress_bar_set_fraction(pb,0.4);
			gtk_progress_bar_set_text(pb,_("Connected"));
			break;
		case BuddyLookupReceivingResponse:
			gtk_progress_bar_set_fraction(pb,0.8);
			gtk_progress_bar_set_text(pb,_("Receiving data..."));
			break;
		case BuddyLookupDone:
			sip_setup_context_get_buddy_lookup_results(ctx,&results);
			linphone_gtk_display_lookup_results(
					linphone_gtk_get_widget(w,"search_results"),
					results);
			gtk_progress_bar_set_fraction(pb,1);
			tmp=g_strdup_printf(ngettext("Found %i contact",
                        "Found %i contacts", ms_list_size(results)),
                    ms_list_size(results));
			gtk_progress_bar_set_text(pb,tmp);
			g_free(tmp);
			if (results) sip_setup_context_free_results(results);
			break;
	}
	g_object_set_data(G_OBJECT(w),"last_state",GINT_TO_POINTER(bls));
	return TRUE;
}

static gboolean keyword_typing_finished(GtkWidget *w){
	guint tid=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"typing_timeout"));
	const char *keyword;
	SipSetupContext *ctx;
	if (tid!=0){
		g_source_remove(tid);
	}
	keyword=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"keyword")));
	if (strlen(keyword)>=1){
		guint tid2;
		ctx=(SipSetupContext*)g_object_get_data(G_OBJECT(w),"SipSetupContext");
		sip_setup_context_lookup_buddy(ctx,keyword);
		if (g_object_get_data(G_OBJECT(w),"buddylookup_processing")==NULL){
			tid2=g_timeout_add(20,(GSourceFunc)linphone_gtk_process_buddy_lookup,w);
			g_object_set_data(G_OBJECT(w),"buddylookup_processing",GINT_TO_POINTER(tid2));
		}
	}
	return FALSE;
}

void linphone_gtk_keyword_changed(GtkEditable *e){
	GtkWidget *w=gtk_widget_get_toplevel(GTK_WIDGET(e));
	guint tid=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"typing_timeout"));
	if (tid!=0){
		g_source_remove(tid);
	}
	tid=g_timeout_add(2000,(GSourceFunc)keyword_typing_finished,w);
	g_object_set_data(G_OBJECT(w),"typing_timeout",GINT_TO_POINTER(tid));
}

static void linphone_gtk_display_lookup_results(GtkWidget *w, const MSList *results){
	GtkListStore *store;
	GtkTreeIter iter;
	gchar *tmp;
	const MSList *elem;
	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(w)));
	gtk_list_store_clear(store);
	disable_add_buddy_button(gtk_widget_get_toplevel(w));
	for(elem=results;elem!=NULL;elem=elem->next){
		BuddyInfo *bi=(BuddyInfo*)elem->data;
		gtk_list_store_append(store,&iter);
		tmp=g_strdup_printf("%s, %s (%s)",bi->firstname,bi->lastname,bi->displayname);
		gtk_list_store_set(store,&iter,LOOKUP_RESULT_NAME, tmp,-1);
		g_free(tmp);
		gtk_list_store_set(store,&iter,LOOKUP_RESULT_SIP_URI, bi->sip_uri,-1);
		tmp=g_strdup_printf("%s, %s %s\n%s",bi->address.street, bi->address.zip, bi->address.town, bi->address.country);
		gtk_list_store_set(store,&iter,LOOKUP_RESULT_ADDRESS, tmp,-1);
		g_free(tmp);
	}
}

void linphone_gtk_add_buddy_from_database(GtkWidget *button){
	GtkWidget *w=gtk_widget_get_toplevel(button);
	GtkTreeSelection *select;
	GtkTreeIter iter;
	GtkTreeModel *model;
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(linphone_gtk_get_widget(w,"search_results")));
	if (gtk_tree_selection_get_selected (select, &model, &iter))
	{
		char *uri;
		char *name;
		char *addr;
		LinphoneFriend *lf;
		gtk_tree_model_get (model, &iter,LOOKUP_RESULT_SIP_URI , &uri,LOOKUP_RESULT_NAME, &name, -1);
		addr=g_strdup_printf("%s <%s>",name,uri);
		lf=linphone_friend_new_with_addr(addr);
		linphone_core_add_friend(linphone_gtk_get_core(),lf);
		linphone_gtk_show_friends();
		g_free(addr);
		g_free(uri);
		g_free(name);
	}
}

/*called when double clicking on a contact */
void linphone_gtk_buddy_lookup_contact_activated(GtkWidget *treeview){
	linphone_gtk_add_buddy_from_database(treeview);
	gtk_widget_destroy(gtk_widget_get_toplevel(treeview));
}
