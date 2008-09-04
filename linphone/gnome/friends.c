/***************************************************************************
                          friends.c  -  display of friend's list

                             -------------------
    begin                : Mon Dec 17 2001
    copyright            : (C) 2001 by Simon Morlat
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


#define get_friend_list() (&uiobj->main_window.friendlist)
#define get_core()	(uiobj->core)
#define get_main_window() (&uiobj->main_window)

enum{
	FRIEND_PRESENCE_IMG,
	FRIEND_SIP_ADDRESS,
	FRIEND_PRESENCE_STATUS,
	FRIEND_ID,
	FRIEND_LIST_NCOL
};

void friend_list_set_friend_status(FriendList *fl, LinphoneFriend * fid, const gchar *url, const gchar *status, const gchar *img){
	GtkTreeIter iter;
	LinphoneFriend *tmp=0;
	gboolean found=FALSE;
	GtkTreeModel *model=gtk_tree_view_get_model(GTK_TREE_VIEW(fl->friendlist));
	if (gtk_tree_model_get_iter_first(model,&iter)) {
		do{
			gtk_tree_model_get(model,&iter,FRIEND_ID,&tmp,-1);
			//printf("tmp=%i, fid=%i",tmp,fid);
			if (fid==tmp) {
				GdkPixbuf *pixbuf;
				gtk_list_store_set(GTK_LIST_STORE(model),&iter,FRIEND_PRESENCE_STATUS,status,-1);
				pixbuf = create_pixbuf(img);
				if (pixbuf)
				  {
				    gtk_list_store_set(GTK_LIST_STORE(model),&iter,FRIEND_PRESENCE_IMG, pixbuf,-1);
				  }
				  found=TRUE;
			}
		}while(gtk_tree_model_iter_next(model,&iter));
	}
	if (found==FALSE){
		//printf("Adding new notifier\n");
		GdkPixbuf *pixbuf;
		gtk_list_store_append(GTK_LIST_STORE(model),&iter);
		gtk_list_store_set(GTK_LIST_STORE(model),&iter,FRIEND_SIP_ADDRESS, url, FRIEND_PRESENCE_STATUS,status,FRIEND_ID,fid,-1);
		pixbuf = create_pixbuf(img);
		if (pixbuf) gtk_list_store_set(GTK_LIST_STORE(model),&iter,FRIEND_PRESENCE_IMG, pixbuf,-1);
	}
}


void
on_friendlist_row_activated            (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{
	GtkTreeSelection *select;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar* friend;
	select = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected (select, &model, &iter))
	{
		gtk_tree_model_get (model, &iter,FRIEND_SIP_ADDRESS , &friend, -1);
		gtk_entry_set_text(GTK_ENTRY(get_main_window()->addressentry),friend);
		g_free(friend);
	}
}

void friend_list_init(FriendList *fl,LinphoneCore *lc,GtkWidget *mainwidget)
{
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	
	
	store = gtk_list_store_new (FRIEND_LIST_NCOL, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,  G_TYPE_POINTER);
	fl->lc=lc;
	fl->friendlist=lookup_widget(mainwidget,"friendlist");
	/* need to add friends to the store here ...*/
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(fl->friendlist),GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes (NULL,
                                                   renderer,
                                                   "pixbuf", FRIEND_PRESENCE_IMG,
                                                   NULL);
	gtk_tree_view_column_set_min_width (column, 29);
	gtk_tree_view_append_column (GTK_TREE_VIEW (fl->friendlist), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                   renderer,
                                                   "text", FRIEND_SIP_ADDRESS,
                                                   NULL);
	g_object_set (G_OBJECT(column), "resizable", TRUE, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (fl->friendlist), column);

	column = gtk_tree_view_column_new_with_attributes (_("Presence status"),
                                                   renderer,
                                                   "text", FRIEND_PRESENCE_STATUS,
                                                   NULL);
	g_object_set (G_OBJECT(column), "resizable", TRUE, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (fl->friendlist), column);
	
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (fl->friendlist));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	
}
