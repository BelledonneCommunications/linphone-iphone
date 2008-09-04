/***************************************************************************
                          addressbook.c  -  
                             -------------------
    begin                : Wed Jan 30 2002
    copyright            : (C) 2002 by Simon Morlat
    email                : simon.morlat@linphone.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "linphone.h"

#define get_address_book()	(&uiobj->addressbook)
#define get_main_window() (&uiobj->main_window)
#define get_core() (uiobj->core)
#define get_uiobj() (uiobj)

void fill_address_book(GtkWidget *address_list);

void ab_destroyed(){
	get_uiobj()->ab=NULL;
}

void show_address_book(){
	if (get_uiobj()->ab!=NULL){
		gtk_window_present(GTK_WINDOW(get_uiobj()->ab));
	}else{
		get_uiobj()->ab=create_and_fill_address_book();
		g_signal_connect(G_OBJECT(get_uiobj()->ab),"destroy",G_CALLBACK(ab_destroyed),NULL);
		gtk_widget_show(get_uiobj()->ab);
	}
}

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

void contact_draw(GtkWidget *w, LinphoneProxyConfig *cfg){
	GtkWidget *table=lookup_widget(w,"table10");
	GtkWidget *combo;
	combo=proxy_combo_box_new(cfg);
	gtk_widget_show(combo);
	gtk_table_attach(GTK_TABLE(table),combo,1,2,2,3, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	GLADE_HOOKUP_OBJECT(w,combo,"proxy");
	combo=gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo),_("Wait"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo),_("Deny"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo),_("Accept"));
	gtk_widget_show(combo);
	gtk_table_attach(GTK_TABLE(table),combo,1,2,3,4, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	GLADE_HOOKUP_OBJECT(w,combo,"pol");
}

GtkWidget * contact_new(LinphoneFriend *lf, GtkWidget *ab){
	GtkWidget *w=create_contact_box();
	contact_draw(w,NULL);
	gtk_widget_show(w);
	g_object_set_data(G_OBJECT(w),"friend_ref",(gpointer)lf);
	g_object_set_data(G_OBJECT(w),"address_book",(gpointer)ab);
	g_object_set_data(G_OBJECT(w),"add",GINT_TO_POINTER(TRUE));
	gtk_combo_box_set_active(GTK_COMBO_BOX(lookup_widget(w,"pol")),lf->pol);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(w,"send_subscribe")),lf->subscribe);
	return w;
}

GtkWidget * contact_edit(LinphoneFriend *lf, GtkWidget *ab){
	GtkWidget *w=create_contact_box();
	gchar *tmpstr;
	contact_draw(w,lf->proxy);
	
	g_object_set_data(G_OBJECT(w),"friend_ref",(gpointer)lf);
	linphone_friend_edit(lf);
	tmpstr=linphone_friend_get_name(lf);
	if (tmpstr!=NULL) {
		gtk_entry_set_text(GTK_ENTRY(lookup_widget(w,"name")),tmpstr);
		g_free(tmpstr);
	}
	tmpstr=linphone_friend_get_addr(lf);
	gtk_entry_set_text(GTK_ENTRY(lookup_widget(w,"sipaddr")),tmpstr);
	g_free(tmpstr);
	
	gtk_combo_box_set_active(GTK_COMBO_BOX(lookup_widget(w,"pol")),lf->pol);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(w,"send_subscribe")),lf->subscribe);
	
	gtk_widget_show(w);
	if (ab!=NULL) g_object_set_data(G_OBJECT(w),"address_book",(gpointer)ab);
	return w;
}

GtkWidget * subscriber_edit(LinphoneFriend *lf){
	GtkWidget *w=contact_edit(lf,NULL);
	g_object_set_data(G_OBJECT(w),"add",GINT_TO_POINTER(TRUE));
	return w;
}

gint contact_ok(GtkWidget *dialog){
	gchar *name,*sipaddr;
	gchar *url;
	gboolean add=FALSE;
	GtkWidget *ab;
	LinphoneFriend *lf;
	int err;
	lf=(LinphoneFriend*)g_object_get_data(G_OBJECT(dialog),"friend_ref");
	add=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog),"add"));
	name=gtk_editable_get_chars(GTK_EDITABLE(lookup_widget(dialog,"name")),0,-1);
	sipaddr=gtk_editable_get_chars(GTK_EDITABLE(lookup_widget(dialog,"sipaddr")),0,-1);
	url=g_strdup_printf("%s <%s>",name,sipaddr);
	/* workaround a bug in osip ? */
	/* something doesn't like addresses like "machin <<sip:truc@bidule>>" */
	if (strchr(sipaddr,'<')==NULL){
		err=linphone_friend_set_sip_addr(lf,url);
	}else err=-1;
	if (err<0){
		linphone_gnome_ui_display_something(get_uiobj(),GTK_MESSAGE_WARNING,_("Bad sip address: a sip address looks like sip:user@domain"));
		linphone_friend_destroy(lf);
		g_free(name);
		g_free(sipaddr);
		g_free(url);
		return -1;		
	}
	g_free(name);
	g_free(sipaddr);
	g_free(url);
	linphone_friend_set_proxy(lf,proxy_combo_box_get_selected(lookup_widget(dialog,"proxy")));
	linphone_friend_set_inc_subscribe_policy(lf,gtk_combo_box_get_active(GTK_COMBO_BOX(lookup_widget(dialog,"pol"))));
	linphone_friend_send_subscribe(lf,gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(dialog,"send_subscribe"))));
	if (add){
		linphone_core_add_friend(get_core(),lf);
	}
	else linphone_friend_done(lf);
	/* ask the address book to redraw itself */
	ab=g_object_get_data(G_OBJECT(dialog),"address_book");
	if (ab!=NULL) fill_address_book(lookup_widget(ab,"address_list"));
	return 0;
}

enum{
	SIP_ADDRESS_COLUMN,
	FRIEND_REFERENCE,
	AB_NCOLUMNS
};

void choose_address_and_close(GtkWidget *ab){
	GtkTreeSelection *select;
	GtkWidget *addressentry=get_main_window()->addressentry;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *address=NULL;
	GtkWidget *address_list=lookup_widget(ab,"address_list");
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (address_list));
	if (select==NULL) return;
	if (gtk_tree_selection_get_selected (select, &model, &iter))
	{
		gtk_tree_model_get (model, &iter,SIP_ADDRESS_COLUMN , &address, -1);
	}
	if (address!=NULL){
		gtk_entry_set_text (GTK_ENTRY(addressentry),address);
		g_free(address);
	}
	gtk_widget_destroy(ab);
}
void
address_book_close                  (GtkWidget       *object,
                                        gpointer         user_data)
{
	gtk_widget_destroy(gtk_widget_get_toplevel(object));	
}

void address_selection_changed_cb(GtkTreeSelection *selection, gpointer data)
{
	
}
gboolean address_button_press(GtkWidget *widget,GdkEventButton *event,gpointer user_data)
{
	GtkWidget *ab=(GtkWidget*)user_data;
	if (event->type==GDK_2BUTTON_PRESS){
		choose_address_and_close(ab);
		return TRUE;
	}
	return FALSE;
}

void fill_address_book(GtkWidget *address_list){
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreeModel *model;
	MSList *elem;
	gchar *tmpstr;
	/* fill the store */
	elem=linphone_core_get_friend_list(get_core());
	model=gtk_tree_view_get_model(GTK_TREE_VIEW(address_list));
	store=GTK_LIST_STORE(model);
	gtk_list_store_clear(store);
	for(;elem!=NULL;elem=ms_list_next(elem)){
		LinphoneFriend *lf=(LinphoneFriend*)elem->data;
		tmpstr=linphone_friend_get_url(lf);
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,SIP_ADDRESS_COLUMN,tmpstr,FRIEND_REFERENCE,(gpointer)lf,-1);
		ms_free(tmpstr);
	}
}

GtkWidget *create_and_fill_address_book(){
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	GtkWidget *address_list;
	GtkWidget *ret=create_address_book();
	
	address_list=lookup_widget(ret,"address_list");
	store = gtk_list_store_new (AB_NCOLUMNS, G_TYPE_STRING,G_TYPE_POINTER);
	gtk_tree_view_set_model(GTK_TREE_VIEW(address_list),GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Contact list"),
                                                   renderer,
                                                   "text", SIP_ADDRESS_COLUMN,
                                                   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (address_list), column);
	
	/* Setup the selection handler */
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (address_list));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed",
                  G_CALLBACK (address_selection_changed_cb),
                  NULL);
	
	/* setup handler for double click */
	g_signal_connect(G_OBJECT(address_list),"button-press-event",G_CALLBACK(address_button_press),(gpointer)ret);
	
	fill_address_book(address_list);
	return ret;
}

void
on_modify_address_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkTreeSelection *select;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkWidget *address_list=lookup_widget(gtk_widget_get_toplevel(GTK_WIDGET(button)),"address_list");
	
	/* change the address in the view */
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (address_list));
	if (gtk_tree_selection_get_selected (select, &model, &iter))
	{
		LinphoneFriend *lf=NULL;
		gtk_tree_model_get(model,&iter,FRIEND_REFERENCE,&lf,-1);
		contact_edit(lf,gtk_widget_get_toplevel(GTK_WIDGET(button)));
	}
}

void on_add_address_clicked(GtkButton *button,gpointer user_data)
{
	LinphoneFriend *lf=linphone_friend_new();
	contact_new(lf,gtk_widget_get_toplevel(GTK_WIDGET(button)));
}

void on_remove_address_clicked(GtkButton *button,gpointer user_data)
{
	GtkTreeSelection *select;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkWidget *address_list=lookup_widget(gtk_widget_get_toplevel(GTK_WIDGET(button)),"address_list");
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (address_list));
	if (gtk_tree_selection_get_selected (select, &model, &iter))
	{
		LinphoneFriend *lf=NULL;
		gtk_tree_model_get(model,&iter,FRIEND_REFERENCE,&lf,-1);
		linphone_core_remove_friend(get_core(),lf);
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	}
}

void on_select_address_clicked(GtkButton *button,gpointer user_data)
{
	choose_address_and_close(gtk_widget_get_toplevel(GTK_WIDGET(button)));
}


void
on_contact_box_response                (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
	switch (response_id){
		case GTK_RESPONSE_OK:
			contact_ok(GTK_WIDGET(dialog));
			break;
		default:
			break;
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
}
