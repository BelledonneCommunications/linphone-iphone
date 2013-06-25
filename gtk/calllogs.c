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

static void fill_renderers(GtkTreeView *v){
	GtkTreeViewColumn *c;
	GtkCellRenderer *r;
	r=gtk_cell_renderer_pixbuf_new();

	c=gtk_tree_view_column_new_with_attributes("icon",r,"pixbuf",0,NULL);
	gtk_tree_view_append_column (v,c);

	r=gtk_cell_renderer_text_new ();
	c=gtk_tree_view_column_new_with_attributes("sipaddress",r,"markup",1,NULL);
	gtk_tree_view_append_column (v,c);
}

void call_log_selection_changed(GtkTreeView *v){
	GtkTreeSelection *select;
	GtkTreeIter iter;
	GtkTreeModel *model=NULL;
	
	select = gtk_tree_view_get_selection(v);
	if (select!=NULL){
		if (gtk_tree_selection_get_selected (select, &model, &iter)){
			GtkTreePath *path=gtk_tree_model_get_path(model,&iter);
			gtk_tree_view_collapse_all(v);
			gtk_tree_view_expand_row(v,path,TRUE);
			gtk_tree_path_free(path);
		}
	}
}

void linphone_gtk_call_log_chat_selected(GtkWidget *w){
	GtkTreeSelection *select;
	GtkTreeIter iter;
	
	select=gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
	if (select!=NULL){
		GtkTreeModel *model=NULL;
		if (gtk_tree_selection_get_selected (select,&model,&iter)){
			gpointer pla;
			LinphoneAddress *la;
			gtk_tree_model_get(model,&iter,2,&pla,-1);
			la=(LinphoneAddress*)pla;
			if (la!=NULL){
				linphone_gtk_friend_list_set_chat_conversation(la);
			}
		}
	}
}

void linphone_gtk_call_log_add_contact(GtkWidget *w){
	GtkTreeSelection *select;
	GtkTreeIter iter;
	
	select=gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
	if (select!=NULL){
		GtkTreeModel *model=NULL;
		if (gtk_tree_selection_get_selected (select,&model,&iter)){
			gpointer pla;
			LinphoneAddress *la;
			LinphoneFriend *lf;
			gtk_tree_model_get(model,&iter,2,&pla,-1);
			la=(LinphoneAddress*)pla;
			if (la!=NULL){
				char *uri=linphone_address_as_string(la);
				lf=linphone_friend_new_with_addr(uri);
				linphone_gtk_show_contact(lf);
				ms_free(uri);
			}
		}
	}
}

static bool_t put_selection_to_uribar(GtkWidget *treeview){
	GtkTreeSelection *sel;
	
	sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	if (sel!=NULL){
		GtkTreeModel *model=NULL;
		GtkTreeIter iter;
		if (gtk_tree_selection_get_selected (sel,&model,&iter)){
			char *tmp;
			LinphoneAddress *la;
			gtk_tree_model_get(model,&iter,2,&la,-1);
			tmp=linphone_address_as_string(la);
			if(tmp!=NULL) 
				gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"uribar")),tmp);
			ms_free(tmp);
			return TRUE;
		}
	}
	return FALSE;
}

static void linphone_gtk_call_selected(GtkTreeView *treeview){
	put_selection_to_uribar(GTK_WIDGET(treeview));
	linphone_gtk_start_call(linphone_gtk_get_widget(gtk_widget_get_toplevel(GTK_WIDGET(treeview)),
					"start_call"));
}

static GtkWidget *linphone_gtk_create_call_log_menu(GtkWidget *call_log){
	GtkWidget *menu=gtk_menu_new();
	GtkWidget *menu_item;
	gchar *call_label=NULL;
	gchar *text_label=NULL;
	gchar *name=NULL;
	GtkWidget *image;
	GtkTreeSelection *select;
	GtkTreeIter iter;
	
	select=gtk_tree_view_get_selection(GTK_TREE_VIEW(call_log));
	if (select!=NULL){
		GtkTreeModel *model=NULL;
		if (gtk_tree_selection_get_selected (select,&model,&iter)){
			gpointer pla;
			LinphoneAddress *la;
			gtk_tree_model_get(model,&iter,2,&pla,-1);
			la=(LinphoneAddress*)pla;
			name=linphone_address_as_string(la);
			call_label=g_strdup_printf(_("Call %s"),name);
			text_label=g_strdup_printf(_("Send text to %s"),name);
			g_free(name);
		}
	}
	if (call_label){
		menu_item=gtk_image_menu_item_new_with_label(call_label);
		image=gtk_image_new_from_stock(GTK_STOCK_NETWORK,GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
		gtk_widget_show(image);
		gtk_widget_show(menu_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_call_selected,call_log);
	}
	if (text_label){
		menu_item=gtk_image_menu_item_new_with_label(text_label);
		image=gtk_image_new_from_stock(GTK_STOCK_NETWORK,GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
		gtk_widget_show(image);
		gtk_widget_show(menu_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_call_log_chat_selected,call_log);
	}
	menu_item=gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
	g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_call_log_add_contact,call_log);
	gtk_widget_show(menu);
	gtk_menu_attach_to_widget(GTK_MENU(menu),call_log, NULL);

	if (call_label) g_free(call_label);
	if (text_label) g_free(text_label);
	return menu;
}

gboolean linphone_gtk_call_log_popup_contact(GtkWidget *list, GdkEventButton *event){
	GtkWidget *m=linphone_gtk_create_call_log_menu(list);
	gtk_menu_popup (GTK_MENU (m), NULL, NULL, NULL, NULL,
                  event ? event->button : 0, event ? event->time : gtk_get_current_event_time());
	return TRUE;
}

gboolean linphone_gtk_call_log_button_pressed(GtkWidget *widget, GdkEventButton *event){
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS){
		return linphone_gtk_call_log_popup_contact(widget, event);
	}
	return FALSE;
}

void linphone_gtk_call_log_clear_missed_call(){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkNotebook *notebook=GTK_NOTEBOOK(linphone_gtk_get_widget(mw,"viewswitch"));
	GtkWidget *page=gtk_notebook_get_nth_page(notebook,0);
	GtkWidget *box=gtk_hbox_new(FALSE,0);
	GtkWidget *image=gtk_image_new_from_stock(GTK_STOCK_REFRESH,GTK_ICON_SIZE_MENU);
	GtkWidget *l;
	const gchar*text=gtk_label_get_text(GTK_LABEL(linphone_gtk_get_widget(mw,"label3")));
	
	l=gtk_label_new(text);
	gtk_box_pack_start(GTK_BOX(box),image,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(box),l,FALSE,FALSE,0);
	gtk_notebook_set_tab_label(notebook,page,box);
	gtk_widget_show_all(box);
}

gboolean linphone_gtk_call_log_reset_missed_call(GtkWidget *w, GdkEvent *event,gpointer user_data){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkNotebook *notebook=GTK_NOTEBOOK(linphone_gtk_get_widget(mw,"viewswitch"));
	gtk_notebook_set_current_page(notebook,0);
	linphone_core_reset_missed_calls_count(linphone_gtk_get_core());
	linphone_gtk_call_log_clear_missed_call();
	return TRUE;
}

void linphone_gtk_call_log_display_missed_call(int nb){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkNotebook *notebook=GTK_NOTEBOOK(linphone_gtk_get_widget(mw,"viewswitch"));
	GtkWidget *page=gtk_notebook_get_nth_page(notebook,0);
	GtkWidget *ebox=gtk_event_box_new();
	GtkWidget *box=gtk_hbox_new(FALSE,0);
	GtkWidget *image=gtk_image_new_from_stock(GTK_STOCK_REFRESH,GTK_ICON_SIZE_MENU);
	GtkWidget *l;
	gchar *buf;
	
	buf=g_markup_printf_escaped(_("<b>Recent calls (%i)</b>"),nb);
	l=gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(l),buf);
	gtk_box_pack_start(GTK_BOX(box),image,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(box),l,FALSE,FALSE,0);
	gtk_container_add(GTK_CONTAINER(ebox),box);
	gtk_notebook_set_tab_label(notebook,page,ebox);
	gtk_widget_add_events(ebox,GDK_BUTTON_PRESS_MASK);
	g_signal_connect(G_OBJECT(ebox),"button_press_event",(GCallback)linphone_gtk_call_log_reset_missed_call,NULL);
	gtk_widget_show_all(ebox);
}

void linphone_gtk_call_log_update(GtkWidget *w){
	GtkTreeView *v=GTK_TREE_VIEW(linphone_gtk_get_widget(w,"logs_view"));
	GtkTreeStore *store;
	const MSList *logs;
	GtkTreeSelection *select;
	GtkWidget *notebook=linphone_gtk_get_widget(w,"viewswitch");
	gint nb;

	store=(GtkTreeStore*)gtk_tree_view_get_model(v);
	if (store==NULL){
		store=gtk_tree_store_new(3,GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_POINTER,G_TYPE_STRING);
		gtk_tree_view_set_model(v,GTK_TREE_MODEL(store));
		g_object_unref(G_OBJECT(store));
		fill_renderers(GTK_TREE_VIEW(linphone_gtk_get_widget(w,"logs_view")));
		select=gtk_tree_view_get_selection(v);
		gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
		g_signal_connect_swapped(G_OBJECT(select),"changed",(GCallback)call_log_selection_changed,v);
		g_signal_connect(G_OBJECT(notebook),"focus-tab",(GCallback)linphone_gtk_call_log_reset_missed_call,NULL);
		g_signal_connect(G_OBJECT(v),"button-press-event",(GCallback)linphone_gtk_call_log_button_pressed,NULL);
//		gtk_button_set_image(GTK_BUTTON(linphone_gtk_get_widget(w,"call_back_button")),
//		                     create_pixmap (linphone_gtk_get_ui_config("callback_button","status-green.png")));
	}
	nb=linphone_core_get_missed_calls_count(linphone_gtk_get_core());
	if(nb > 0)
		linphone_gtk_call_log_display_missed_call(nb);
	gtk_tree_store_clear (store);

	for (logs=linphone_core_get_call_logs(linphone_gtk_get_core());logs!=NULL;logs=logs->next){
		LinphoneCallLog *cl=(LinphoneCallLog*)logs->data;
		GtkTreeIter iter, iter2;
		const LinphoneAddress *la=linphone_call_log_get_dir(cl)==LinphoneCallIncoming ? linphone_call_log_get_from(cl) : linphone_call_log_get_to(cl);
		char *addr= linphone_address_as_string(la);
		const char *display;
		gchar *logtxt, *headtxt, *minutes, *seconds;
		gchar quality[20];
		const char *status=NULL;
		gchar *start_date=NULL;
		LinphoneFriend *lf=NULL;
		int duration=linphone_call_log_get_duration(cl);
		time_t start_date_time=linphone_call_log_get_start_date(cl);
		
#if GLIB_CHECK_VERSION(2,26,0)
		if (start_date_time){
			GDateTime *dt=g_date_time_new_from_unix_local(start_date_time);
			start_date=g_date_time_format(dt,"%c");
			g_date_time_unref(dt);
		}
#else
		start_date=g_strdup(ctime(&start_date_time));
#endif
		lf=linphone_core_get_friend_by_address(linphone_gtk_get_core(),addr);
		if(lf != NULL){
			la=linphone_friend_get_address(lf);
			display=linphone_address_get_display_name(la);
		} else {
			display=linphone_address_get_display_name(la);
		}
		if (display==NULL){
			display=linphone_address_get_username (la);
			if (display==NULL){
				display=linphone_address_get_domain (la);
			}
		}
		if (linphone_call_log_get_quality(cl)!=-1){
			snprintf(quality,sizeof(quality),"%.1f",linphone_call_log_get_quality(cl));
		}else snprintf(quality,sizeof(quality)-1,"%s",_("n/a"));
		switch(linphone_call_log_get_status(cl)){
			case LinphoneCallAborted:
				status=_("Aborted");
			break;
			case LinphoneCallMissed:
				status=_("Missed");
			break;
			case LinphoneCallDeclined:
				status=_("Declined");
			break;
			default:
			break;
		}
		minutes=g_markup_printf_escaped(
			ngettext("%i minute", "%i minutes", duration/60),
			duration/60);
		seconds=g_markup_printf_escaped(
			ngettext("%i second", "%i seconds", duration%60),
			duration%60);
		if (status==NULL) {
				headtxt=g_markup_printf_escaped(_("<big><b>%s</b></big>\t%s"),display,start_date ? start_date : "");
				logtxt=g_markup_printf_escaped(
				_("<small><i>%s</i>\t" 
				  "<i>Quality: %s</i></small>\n%s\t%s\t"),
				addr, quality, minutes, seconds);
		} else {
			headtxt=g_markup_printf_escaped(_("<big><b>%s</b></big>\t%s"),display,start_date ? start_date : "");
			logtxt=g_markup_printf_escaped(
			_("<small><i>%s</i></small>\t"
				"\n%s"),addr, status);
		}
		g_free(minutes);
		g_free(seconds);
		if (start_date) g_free(start_date);
		gtk_tree_store_append (store,&iter,NULL);

		GdkPixbuf *incoming = create_pixbuf("call_status_incoming.png");
		GdkPixbuf *outgoing = create_pixbuf("call_status_outgoing.png");
		gtk_tree_store_set (store,&iter,
		               0, linphone_call_log_get_dir(cl)==LinphoneCallOutgoing ? outgoing : incoming,
		               1, headtxt,2,la,-1);	
		gtk_tree_store_append (store,&iter2,&iter);
		gtk_tree_store_set (store,&iter2,1,logtxt,2,la,-1);
		ms_free(addr);
		g_free(logtxt);
		g_free(headtxt);
	}
}

void linphone_gtk_history_row_activated(GtkWidget *treeview){
	if (put_selection_to_uribar(treeview)){
		GtkWidget *mw=linphone_gtk_get_main_window();
		linphone_gtk_start_call(linphone_gtk_get_widget(mw,"start_call"));
	}
}

void linphone_gtk_history_row_selected(GtkWidget *treeview){
	put_selection_to_uribar(treeview);
}

void linphone_gtk_clear_call_logs(GtkWidget *button){
	linphone_core_clear_call_logs (linphone_gtk_get_core());
	linphone_gtk_call_log_clear_missed_call();
	linphone_gtk_call_log_update(gtk_widget_get_toplevel(button));
}

void linphone_gtk_call_log_callback(GtkWidget *button){
	GtkWidget *mw=linphone_gtk_get_main_window();
	if (put_selection_to_uribar(linphone_gtk_get_widget(mw,"logs_view")))
			linphone_gtk_start_call(linphone_gtk_get_widget(mw,"start_call"));
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

GtkWidget * linphone_gtk_show_call_logs(void){
	GtkWidget *mw=linphone_gtk_get_main_window();

	GtkWidget *w=(GtkWidget*)g_object_get_data(G_OBJECT(linphone_gtk_get_main_window()),"call_logs");
	if (w==NULL){
		w=linphone_gtk_create_window("call_logs");
//		gtk_button_set_image(GTK_BUTTON(linphone_gtk_get_widget(w,"call_back_button")),
//		                     create_pixmap (linphone_gtk_get_ui_config("callback_button","status-green.png")));
		g_object_set_data(G_OBJECT(mw),"call_logs",w);
		g_signal_connect(G_OBJECT(w),"response",(GCallback)linphone_gtk_call_log_response,NULL);
		gtk_widget_show(w);
		linphone_gtk_call_log_update(w);
	}else gtk_window_present(GTK_WINDOW(w));
	return w;
}