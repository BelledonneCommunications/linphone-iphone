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


void linphone_gtk_call_log_update(GtkWidget *w){
	GtkTreeView *v=GTK_TREE_VIEW(linphone_gtk_get_widget(w,"logs_view"));
	GtkListStore *store;
	const MSList *logs;

	store=(GtkListStore*)gtk_tree_view_get_model(v);
	if (store==NULL){
		store=gtk_list_store_new(3,G_TYPE_STRING,G_TYPE_STRING, G_TYPE_POINTER);
		gtk_tree_view_set_model(v,GTK_TREE_MODEL(store));
		g_object_unref(G_OBJECT(store));
	}
	gtk_list_store_clear (store);

	for (logs=linphone_core_get_call_logs(linphone_gtk_get_core());logs!=NULL;logs=logs->next){
		LinphoneCallLog *cl=(LinphoneCallLog*)logs->data;
		GtkTreeIter iter;
		LinphoneAddress *la=cl->dir==LinphoneCallIncoming ? cl->from : cl->to;
		char *addr= linphone_address_as_string_uri_only (la);
		const char *display;
		gchar *logtxt;
		display=linphone_address_get_display_name (la);
		if (display==NULL){
			display=linphone_address_get_username (la);
			if (display==NULL)
				display=linphone_address_get_domain (la);
		}
		logtxt=g_markup_printf_escaped("<big><b>%s</b></big>\t<small><i>%s</i></small>\n"
		                               "%s\t%i minutes %i seconds",display, addr, cl->start_date,
		                               cl->duration/60,cl->duration%60);
		gtk_list_store_append (store,&iter);
		gtk_list_store_set (store,&iter,
		               0, cl->dir==LinphoneCallOutgoing ? GTK_STOCK_GO_UP : GTK_STOCK_GO_DOWN,
		               1, logtxt,2,la,-1);
		ms_free(addr);
		g_free(logtxt);
	}
	
}

static bool_t put_selection_to_uribar(GtkWidget *treeview){
	GtkTreeSelection *sel;

	sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	if (sel!=NULL){
		GtkTreeModel *model=NULL;
		GtkTreeIter iter;
		if (gtk_tree_selection_get_selected (sel,&model,&iter)){
			gpointer pla;
			LinphoneAddress *la;
			char *tmp;
			gtk_tree_model_get(model,&iter,2,&pla,-1);
			la=(LinphoneAddress*)pla;
			tmp=linphone_address_as_string (la);
			gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"uribar")),tmp);
			ms_free(tmp);
			return TRUE;
		}
	}
	return FALSE;
}

void linphone_gtk_history_row_activated(GtkWidget *treeview){
	put_selection_to_uribar(treeview);
}

void linphone_gtk_call_log_response(GtkWidget *w, guint response_id){
	GtkWidget *mw=linphone_gtk_get_main_window();
	if (response_id==1){
		if (put_selection_to_uribar(linphone_gtk_get_widget(w,"logs_view")))
			linphone_gtk_start_call(linphone_gtk_get_widget(mw,"start_call"));
	}else if (response_id==2){
		linphone_core_clear_call_logs (linphone_gtk_get_core());
		linphone_gtk_call_log_update(w);
		return;
	}
	g_object_set_data(G_OBJECT(mw),"call_logs",NULL);
	gtk_widget_destroy(w);
}


static void fill_renderers(GtkTreeView *v){
	GtkTreeViewColumn *c;
	GtkCellRenderer *r=gtk_cell_renderer_pixbuf_new ();

	g_object_set(r,"stock-size",GTK_ICON_SIZE_BUTTON,NULL);
	c=gtk_tree_view_column_new_with_attributes("icon",r,"stock-id",0,NULL);
	gtk_tree_view_append_column (v,c);

	r=gtk_cell_renderer_text_new ();
	c=gtk_tree_view_column_new_with_attributes("sipaddress",r,"markup",1,NULL);
	gtk_tree_view_append_column (v,c);
}

GtkWidget * linphone_gtk_show_call_logs(void){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *w=(GtkWidget*)g_object_get_data(G_OBJECT(linphone_gtk_get_main_window()),"call_logs");
	if (w==NULL){
		w=linphone_gtk_create_window("call_logs");
		gtk_button_set_image(GTK_BUTTON(linphone_gtk_get_widget(w,"call_back_button")),
		                     create_pixmap (linphone_gtk_get_ui_config("callback_button","status-green.png")));
		fill_renderers(GTK_TREE_VIEW(linphone_gtk_get_widget(w,"logs_view")));
		g_object_set_data(G_OBJECT(mw),"call_logs",w);
		g_signal_connect(G_OBJECT(w),"response",(GCallback)linphone_gtk_call_log_response,NULL);
		gtk_widget_show(w);
		linphone_gtk_call_log_update(w);
	}else gtk_window_present(GTK_WINDOW(w));
	return w;
}

