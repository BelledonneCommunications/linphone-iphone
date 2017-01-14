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
#include <bctoolbox/vfs.h>
#include <gdk/gdkkeysyms.h>

static GtkWidget *linphone_gtk_create_contact_menu(GtkWidget *contact_list);

enum{
	FRIEND_PRESENCE_IMG,
	FRIEND_NAME,
	FRIEND_ID,
	FRIEND_CHATROOM,
	FRIEND_SIP_ADDRESS,
	FRIEND_CHAT,
	FRIEND_CALL_BUTTON_VISIBLE,
	FRIEND_CHAT_BUTTON_VISIBLE,
	FRIEND_LIST_NCOL
};

typedef struct _status_picture_tab_t{
	LinphoneOnlineStatus status;
	const char *img;
} status_picture_tab_t;

status_picture_tab_t status_picture_tab[]={
	{ LinphoneStatusOnline       , "linphone-status-online"       },
	{ LinphoneStatusBusy         , "linphone-status-away"         },
	{ LinphoneStatusBeRightBack  , "linphone-status-away"         },
	{ LinphoneStatusAway         , "linphone-status-away"         },
	{ LinphoneStatusOnThePhone   , "linphone-status-away"         },
	{ LinphoneStatusOutToLunch   , "linphone-status-away"         },
	{ LinphoneStatusDoNotDisturb , "linphone-status-donotdisturb" },
	{ LinphoneStatusMoved        , "linphone-status-away"         },
	{ LinphoneStatusAltService   , "linphone-status-away"         },
	{ LinphoneStatusOffline      , "linphone-status-offline"      },
	{ LinphoneStatusPending      , "linphone-status-offline"      },
	{ LinphoneStatusEnd          , NULL                           }
};

static const char *status_to_icon_name(LinphoneOnlineStatus ss) {
	status_picture_tab_t *t=status_picture_tab;
	while(t->img!=NULL){
		if (ss==t->status) {
			return t->img;
		}
		++t;
	}
	g_error("No icon name defined for status %i",ss);
	return NULL;
}

static GtkWidget *create_status_picture(LinphoneOnlineStatus ss, GtkIconSize icon_size){
	const char *icon_name = status_to_icon_name(ss);
	if(icon_name) return gtk_image_new_from_icon_name(icon_name, icon_size);
	else return NULL;
}

gboolean linphone_gtk_friend_list_is_contact(const LinphoneAddress *addr){
	LinphoneFriend *lf;
	char *addr_str=linphone_address_as_string(addr);
	lf=linphone_core_get_friend_by_address(linphone_gtk_get_core(),addr_str);
	if(lf == NULL){
		return FALSE;
	} return TRUE;
}

static void linphone_gtk_set_selection_to_uri_bar(GtkTreeView *treeview){
	GtkTreeSelection *select;
	GtkTreeIter iter;
	GtkTreeModel *model;
	LinphoneFriend *lf=NULL;
	gchar* friend;
	select = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected (select, &model, &iter)) {
		const LinphoneAddress *addr;
		gtk_tree_model_get (model, &iter,FRIEND_ID , &lf, -1);
		addr = linphone_friend_get_address(lf);
		if (addr) {
			friend=linphone_address_as_string(addr);
			gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"uribar")),friend);
			ms_free(friend);
		}
	}
}

void linphone_gtk_add_contact(void){
	GtkWidget *w=linphone_gtk_create_window("contact", linphone_gtk_get_main_window());
	int presence_enabled=linphone_gtk_get_ui_config_int("use_subscribe_notify",1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"show_presence")),presence_enabled);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"allow_presence")),
					presence_enabled);
	gtk_widget_show(w);
}

void linphone_gtk_edit_contact(GtkWidget *button){
	GtkWidget *w=gtk_widget_get_toplevel(button);
	GtkTreeSelection *select;
	GtkTreeIter iter;
	GtkTreeModel *model;
	LinphoneFriend *lf=NULL;
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(linphone_gtk_get_widget(w,"contact_list")));
	if (gtk_tree_selection_get_selected (select, &model, &iter))
	{
		gtk_tree_model_get (model, &iter,FRIEND_ID , &lf, -1);
		linphone_gtk_show_contact(lf, w);
	}
}

void linphone_gtk_remove_contact(GtkWidget *button){
	GtkWidget *w=gtk_widget_get_toplevel(button);
	GtkTreeSelection *select;
	GtkTreeIter iter;
	GtkTreeModel *model;
	LinphoneFriend *lf=NULL;
	LinphoneChatRoom *cr=NULL;
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(linphone_gtk_get_widget(w,"contact_list")));
	if (gtk_tree_selection_get_selected (select, &model, &iter))
	{
		gtk_tree_model_get (model, &iter,FRIEND_ID , &lf, -1);
		linphone_core_remove_friend(linphone_gtk_get_core(),lf);
		gtk_tree_model_get (model, &iter,FRIEND_CHATROOM , &cr, -1);
		linphone_chat_room_delete_history(cr);
		linphone_gtk_show_friends();
	}
}

gboolean linphone_gtk_on_key_press(GtkWidget *widget, GdkEvent *event, gpointer user_data) {

	if (event->type == GDK_KEY_PRESS && ((GdkEventKey*)event)->state & GDK_CONTROL_MASK) {
		int cpt;
		int key = -1;
		static int key_map[9] = {0};
		if (key_map[0] == 0) {
			GdkKeymapKey *keys = NULL;
			gint n_keys = 0;
			for (cpt = 0; cpt < 9; cpt++) {
				if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(),
					GDK_KEY_1+cpt, &keys, &n_keys))
					key_map[cpt] = keys->keycode;
			}
		}

		for(cpt = 0; cpt < 9 ; cpt++) {
			if (key_map[cpt] == (((GdkEventKey*)event)->hardware_keycode)) {
				key = cpt;
				break;
			}
		}

		if (key != -1) {
			GtkWidget *main_window = linphone_gtk_get_main_window();
			GtkWidget *friendlist = linphone_gtk_get_widget(main_window,"contact_list");
			GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(friendlist));
			GtkTreeIter iter;

			if (gtk_tree_model_get_iter_first(model, &iter)) {
				int index = 0;
				LinphoneFriend *lf = NULL;
				LinphoneChatRoom *cr;
				do{
					if (index == key) {
						const LinphoneAddress *addr;
						gtk_tree_model_get (model, &iter,FRIEND_CHATROOM , &cr, -1);
						gtk_tree_model_get (model, &iter,FRIEND_ID , &lf, -1);
						if (lf != NULL) {
							addr = linphone_friend_get_address(lf);
							if (addr != NULL) {
								linphone_gtk_friend_list_set_chat_conversation(addr);
							}
						}
						if (cr != NULL){
							linphone_gtk_mark_chat_read(cr);
							linphone_gtk_friend_list_update_button_display(GTK_TREE_VIEW(friendlist));
						}
						return TRUE;
					}
					index++;
				}while(gtk_tree_model_iter_next(model,&iter) && index <= 9);
			}
		}
	}
	return FALSE;
}

void linphone_gtk_delete_history(GtkWidget *button){
	GtkWidget *w=linphone_gtk_get_main_window();
	GtkTreeSelection *select;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkWidget *chat_view;
	LinphoneFriend *lf=NULL;
	GtkWidget *friendlist;

	friendlist=linphone_gtk_get_widget(w,"contact_list");
	chat_view=(GtkWidget *)g_object_get_data(G_OBJECT(friendlist),"chatview");
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(friendlist));
	if (gtk_tree_selection_get_selected (select, &model, &iter))
	{
		LinphoneChatRoom *cr;
		gtk_tree_model_get (model, &iter,FRIEND_ID , &lf, -1);
		gtk_tree_model_get (model, &iter,FRIEND_CHATROOM , &cr, -1);
		linphone_chat_room_delete_history(cr);
		if(chat_view!=NULL){
			const LinphoneAddress *from=linphone_gtk_friend_list_get_active_address();
			const LinphoneAddress *addr=linphone_friend_get_address(lf);
			if (addr != NULL) {
				if(linphone_address_weak_equal(from,addr)){
					GtkTextView *text_view=GTK_TEXT_VIEW(linphone_gtk_get_widget(chat_view,"textview"));
					GtkTextIter start;
					GtkTextIter end;
					GtkTextBuffer *text_buffer;

					text_buffer=gtk_text_view_get_buffer(text_view);
					gtk_text_buffer_get_bounds(text_buffer, &start, &end);
					gtk_text_buffer_delete (text_buffer, &start, &end);
					g_object_set_data(G_OBJECT(chat_view),"from_message",NULL);
				}
			}
		}
		linphone_gtk_show_friends();
	}
}

static void linphone_gtk_call_selected(GtkTreeView *treeview){
	linphone_gtk_set_selection_to_uri_bar(treeview);
	linphone_gtk_start_call(linphone_gtk_get_widget(gtk_widget_get_toplevel(GTK_WIDGET(treeview)),
					"start_call"));
}

void linphone_gtk_friend_list_update_button_display(GtkTreeView *friendlist){
	GtkTreeIter iter, selected_iter;
	GtkTreeModel *model=gtk_tree_view_get_model(friendlist);
	GtkTreeSelection *select=gtk_tree_view_get_selection(friendlist);
	LinphoneChatRoom *cr=NULL;
	gboolean is_composing;
	int nbmsg=0;
	GtkTreePath *selected_path = NULL;
	GtkTreePath *hovered_row = (GtkTreePath *)g_object_get_data(G_OBJECT(friendlist), "hovered_row");

	if (gtk_tree_selection_get_selected(select, &model, &selected_iter)){
		selected_path = gtk_tree_model_get_path(model, &selected_iter);
	}

	if (gtk_tree_model_get_iter_first(model,&iter)) {
		do{
			const char *icon_name = NULL;
			gboolean show_chat_button = FALSE;
			gboolean show_call_button = FALSE;
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);

			gtk_tree_model_get (model, &iter,FRIEND_CHATROOM , &cr, -1);
			nbmsg=linphone_chat_room_get_unread_messages_count(cr);
			is_composing=linphone_chat_room_is_remote_composing(cr);
			if(nbmsg != 0){
				if (is_composing) icon_name = "linphone-chat-new-message-and-writing";
				else icon_name = "linphone-chat-new-message";
				show_chat_button = TRUE;
			} else {
				if (is_composing) {
					icon_name = "linphone-chat-writing";
					show_chat_button = TRUE;
				} else {
					icon_name = "linphone-chat-nothing";
				}
			}

			if ((selected_path && gtk_tree_path_compare(path, selected_path) == 0)
					|| (hovered_row && gtk_tree_path_compare(path, hovered_row) == 0)){
				show_chat_button = TRUE;
				show_call_button = TRUE;
			}

			gtk_list_store_set(GTK_LIST_STORE(model),&iter,FRIEND_CHAT,icon_name,
					FRIEND_CHAT_BUTTON_VISIBLE, show_chat_button, -1);
			gtk_list_store_set(GTK_LIST_STORE(model), &iter, FRIEND_CALL_BUTTON_VISIBLE, show_call_button, -1);

			gtk_tree_path_free(path);
		}while(gtk_tree_model_iter_next(model,&iter));
	}
	if (selected_path) gtk_tree_path_free(selected_path);
}

static gboolean grab_focus(GtkWidget *w){
	gtk_widget_grab_focus(w);
	return FALSE;
}

void linphone_gtk_friend_list_set_active_address(const LinphoneAddress *addr){
	GtkWidget *w=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(w,"contact_list");
	g_object_set_data_full(G_OBJECT(friendlist),"from", addr ? linphone_address_clone(addr) : NULL, (GDestroyNotify)linphone_address_unref);
}

const LinphoneAddress *linphone_gtk_friend_list_get_active_address(void){
	GtkWidget *w=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(w,"contact_list");
	return (const LinphoneAddress*)g_object_get_data(G_OBJECT(friendlist),"from");
}

void linphone_gtk_friend_list_set_chat_conversation(const LinphoneAddress *la){
	GtkTreeIter iter;
	GtkListStore *store=NULL;
	GtkWidget *w = linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(w,"contact_list");
	GtkTreeModel *model=gtk_tree_view_get_model(GTK_TREE_VIEW(friendlist));
	GtkWidget *chat_view=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
	LinphoneFriend *lf=NULL;
	LinphoneChatRoom *cr=NULL;
	GtkNotebook *notebook=(GtkNotebook *)linphone_gtk_get_widget(w,"viewswitch");
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(friendlist));

	lf=linphone_core_find_friend(linphone_gtk_get_core(),la);
	if(lf==NULL){
		cr=linphone_gtk_create_chatroom(la);
		linphone_gtk_friend_list_set_active_address(la);
	} else {
		store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(friendlist)));
		if (gtk_tree_model_get_iter_first(model,&iter)) {
			do{
				const LinphoneAddress *addr;
				gtk_tree_model_get(model, &iter,FRIEND_ID , &lf, -1);
				addr=linphone_friend_get_address(lf);
				if (addr != NULL) {
					if (linphone_address_weak_equal(addr,la)){
						gtk_tree_model_get (model, &iter,FRIEND_CHATROOM , &cr, -1);
						if(cr==NULL){
							cr=linphone_gtk_create_chatroom(addr);
							gtk_list_store_set(store,&iter,FRIEND_CHATROOM,cr,-1);
						}
						linphone_gtk_friend_list_set_active_address(addr);
						gtk_tree_selection_select_iter(selection, &iter);
						break;
					}
				}
			}while(gtk_tree_model_iter_next(model,&iter));
		}
	}
	if(cr) {
		if(chat_view == NULL){
			chat_view=linphone_gtk_init_chatroom(cr,la);
			g_object_set_data(G_OBJECT(friendlist),"chatview",(gpointer)chat_view);
		} else {
			linphone_gtk_load_chatroom(cr,la,chat_view);
		}
		gtk_notebook_set_current_page(notebook,gtk_notebook_page_num(notebook,chat_view));
		g_idle_add((GSourceFunc)grab_focus,linphone_gtk_get_widget(chat_view,"text_entry"));
	}
}

void linphone_gtk_notebook_tab_select(GtkNotebook *notebook,GtkWidget *page,guint page_num, gpointer data){
	GtkWidget *w=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(w,"contact_list");
	GtkWidget *chat_view;
	LinphoneChatRoom *cr=NULL;
 	chat_view=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
 	if(page != NULL){
 		notebook=(GtkNotebook *)linphone_gtk_get_widget(w,"viewswitch");
 		if(gtk_notebook_page_num(notebook,page)==gtk_notebook_page_num(notebook,chat_view)){
 			cr=g_object_get_data(G_OBJECT(chat_view),"cr");
 			if(cr!=NULL){
				linphone_gtk_mark_chat_read(cr);
 				linphone_gtk_show_friends();
			}
 		}
	}
}

void linphone_gtk_chat_selected(GtkWidget *item){
	GtkWidget *w=gtk_widget_get_toplevel(item);
	GtkTreeSelection *select;
	GtkListStore *store=NULL;
	GtkTreeIter iter;
	GtkTreeModel *model;
	LinphoneFriend *lf=NULL;
	LinphoneChatRoom *cr=NULL;
	GtkWidget *friendlist=linphone_gtk_get_widget(w,"contact_list");
	GtkWidget *page;

	select=gtk_tree_view_get_selection(GTK_TREE_VIEW(friendlist));
	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(item)));
	if (gtk_tree_selection_get_selected (select, &model, &iter)){
		GtkNotebook *notebook=(GtkNotebook *)linphone_gtk_get_widget(w,"viewswitch");
		const LinphoneAddress *addr;
		gtk_tree_model_get (model, &iter,FRIEND_ID , &lf, -1);
		gtk_tree_model_get (model, &iter,FRIEND_CHATROOM , &cr, -1);
		addr=linphone_friend_get_address(lf);
		if (addr != NULL) {
			if(cr==NULL){
				cr=linphone_gtk_create_chatroom(addr);
				gtk_list_store_set(store,&iter,FRIEND_CHATROOM,cr,-1);
			}
			page=GTK_WIDGET(g_object_get_data(G_OBJECT(friendlist),"chatview"));
			linphone_gtk_friend_list_set_active_address(addr);
			if(page==NULL){
				page=linphone_gtk_init_chatroom(cr,addr);
				g_object_set_data(G_OBJECT(friendlist),"chatview",(gpointer)page);
			} else {
				linphone_gtk_load_chatroom(cr,addr,page);
			}
			linphone_gtk_mark_chat_read(cr);
			gtk_notebook_set_current_page(notebook,gtk_notebook_page_num(notebook,page));
			g_idle_add((GSourceFunc)grab_focus,linphone_gtk_get_widget(page,"text_entry"));
		}
	}
}

void linphone_gtk_contact_clicked(GtkTreeSelection *selection){
	GtkTreeView *friendlist = gtk_tree_selection_get_tree_view(selection);
	GtkWidget *mw = linphone_gtk_get_main_window();
	GtkWidget *edit_button = linphone_gtk_get_widget(mw, "edit_button");
	GtkWidget *remove_button = linphone_gtk_get_widget(mw, "remove_button");

	linphone_gtk_set_selection_to_uri_bar(friendlist);
	linphone_gtk_friend_list_update_button_display(friendlist);
	if(gtk_tree_selection_get_selected(selection, NULL, NULL)) {
		gtk_widget_set_sensitive(edit_button, TRUE);
		gtk_widget_set_sensitive(remove_button, TRUE);
	} else {
		gtk_widget_set_sensitive(edit_button, FALSE);
		gtk_widget_set_sensitive(remove_button, FALSE);
	}
}


void linphone_gtk_add_button_clicked(void){
	linphone_gtk_add_contact();
}

void linphone_gtk_edit_button_clicked(GtkWidget *button){
	linphone_gtk_edit_contact(button);
}

void linphone_gtk_remove_button_clicked(GtkWidget *button){
	linphone_gtk_remove_contact(button);
}

static GtkWidget * create_presence_menu(void){
	GtkWidget *menu=gtk_menu_new();
	GtkWidget *menu_item;
	status_picture_tab_t *t;
	for(t=status_picture_tab;t->img!=NULL;++t){
		if (t->status==LinphoneStatusPending){
			continue;
		}
		menu_item=gtk_image_menu_item_new_with_label(linphone_online_status_to_string(t->status));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),
				gtk_image_new_from_icon_name(t->img, GTK_ICON_SIZE_LARGE_TOOLBAR));
		gtk_widget_show(menu_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_set_my_presence,GINT_TO_POINTER(t->status));
	}
	return menu;
}

void linphone_gtk_set_my_presence(LinphoneOnlineStatus ss){
	GtkWidget *button=linphone_gtk_get_widget(linphone_gtk_get_main_window(),"presence_button");
	GtkWidget *image=create_status_picture(ss, GTK_ICON_SIZE_LARGE_TOOLBAR);
	GtkWidget *menu;

	gtk_widget_set_tooltip_text(button,linphone_online_status_to_string(ss));
	gtk_button_set_image(GTK_BUTTON(button),image);
	/*prepare menu*/
	menu=(GtkWidget*)g_object_get_data(G_OBJECT(button),"presence_menu");
	if (menu==NULL){
		menu=create_presence_menu();
		/*the menu is destroyed when the button is destroyed*/
		g_object_weak_ref(G_OBJECT(button),(GWeakNotify)gtk_widget_destroy,menu);
		g_object_set_data(G_OBJECT(button),"presence_menu",menu);
	}
	linphone_core_set_presence_info(linphone_gtk_get_core(),0,NULL,ss);
}

void linphone_gtk_my_presence_clicked(GtkWidget *button){
	GtkWidget *menu=(GtkWidget*)g_object_get_data(G_OBJECT(button),"presence_menu");
	gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,0,
			gtk_get_current_event_time());
	gtk_widget_show(menu);
}

static void icon_press_handler(GtkEntry *entry){
	GtkWidget *w = gtk_widget_get_toplevel(GTK_WIDGET(entry));
	const char *text=gtk_entry_get_text(entry);
	if (text && strlen(text)>0){
		LinphoneAddress *addr;
		LinphoneFriend *lf;
		char *uri;
		addr=linphone_core_interpret_url(linphone_gtk_get_core(),text);
		if (addr==NULL){
			return ;
		}
		uri=linphone_address_as_string_uri_only(addr);
		lf=linphone_core_get_friend_by_address(linphone_gtk_get_core(),uri);
		ms_free(uri);
		if (lf==NULL)
			lf=linphone_core_create_friend(linphone_gtk_get_core());
		if (lf!=NULL){
			linphone_friend_set_address(lf,addr);
			linphone_gtk_show_contact(lf, w);
		}
		linphone_address_unref(addr);
	}
}

static void update_star(GtkEntry *entry, gboolean is_known){
	if (is_known){
		gtk_entry_set_icon_from_icon_name(entry,GTK_ENTRY_ICON_SECONDARY,NULL);
		gtk_entry_set_icon_sensitive(GTK_ENTRY(entry),GTK_ENTRY_ICON_SECONDARY,FALSE);
		gtk_entry_set_icon_tooltip_text(GTK_ENTRY(entry),GTK_ENTRY_ICON_SECONDARY,NULL);
	}else{
		gtk_entry_set_icon_from_icon_name(entry,GTK_ENTRY_ICON_SECONDARY,"linphone-contact-add");
		gtk_entry_set_icon_sensitive(GTK_ENTRY(entry),GTK_ENTRY_ICON_SECONDARY,TRUE);
		gtk_entry_set_icon_tooltip_text(GTK_ENTRY(entry),GTK_ENTRY_ICON_SECONDARY,_("Add to addressbook"));
	}
}

static void check_contact(GtkEditable *editable, LinphoneCore *lc){
	bool_t known = TRUE;
	char *tmp = gtk_editable_get_chars(editable, 0, -1);
	if (tmp != NULL) {
		if (strlen(tmp) > 0) {
			known = linphone_gtk_is_friend(lc, tmp);
		}
		g_free(tmp);
	}
	update_star(GTK_ENTRY(editable), known);
}

static void linphone_gtk_init_bookmark_icon(void){
	GtkWidget *entry = linphone_gtk_get_widget(linphone_gtk_get_main_window(), "uribar");
	g_signal_connect(G_OBJECT(entry),"icon-release",(GCallback)icon_press_handler,NULL);
	g_signal_connect(G_OBJECT(GTK_EDITABLE(entry)),"changed",(GCallback)check_contact,linphone_gtk_get_core());
}

static gboolean friend_search_func(GtkTreeModel *model, gint column,
                                                         const gchar *key,
                                                         GtkTreeIter *iter,
                                                         gpointer search_data){
	char *name=NULL;
	gboolean ret=TRUE;
	gtk_tree_model_get(model,iter,FRIEND_NAME,&name,-1);
	if (name!=NULL){
		gchar *uname=g_utf8_casefold(name,-1); /* need that to perform case-insensitive search in utf8 */
		gchar *ukey=g_utf8_casefold(key,-1);
		ret=strstr(uname,ukey)==NULL;
		g_free(uname);
		g_free(ukey);
		g_free(name);
	}
	return ret;
}

static gint friend_sort(GtkTreeModel *model, GtkTreeIter *a,GtkTreeIter *b,gpointer user_data){
	char *n1=NULL,*n2=NULL;
	int ret;
	gtk_tree_model_get(model,a,FRIEND_NAME,&n1,-1);
	gtk_tree_model_get(model,b,FRIEND_NAME,&n2,-1);
	if (n1 && n2) {
		ret=strcmp(n1,n2);
		g_free(n1);
		g_free(n2);
	}else if (n1){
		g_free(n1);
		ret=-1;
	}else if (n2){
		g_free(n2);
		ret=1;
	}else ret=0;
	return ret;
}

void linphone_gtk_friend_list_on_name_column_clicked(GtkTreeModel *model){
	GtkSortType st;
	gint column;

	gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model),&column,&st);
	if (column==FRIEND_NAME){
		if (st==GTK_SORT_ASCENDING) st=GTK_SORT_DESCENDING;
		else st=GTK_SORT_ASCENDING;
	}else st=GTK_SORT_ASCENDING;
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),FRIEND_NAME,st);
}

static int get_friend_weight(const LinphoneFriend *lf){
	int w=0;
	LinphoneCore *lc=linphone_gtk_get_core();
	const LinphoneAddress *addr = linphone_friend_get_address(lf);
	LinphoneChatRoom *cr = NULL;

	if (addr != NULL) {
		cr = linphone_core_get_chat_room(lc, addr);
	}
	if (cr && linphone_chat_room_get_unread_messages_count(cr)>0){
		w+=2000;
	}

	switch(linphone_friend_get_status(lf)){
		case LinphoneStatusOnline:
			w+=1000;
		break;
		case LinphoneStatusOffline:
			if (linphone_friend_get_send_subscribe(lf))
				w+=100;
		break;
		default:
			w+=500;
		break;
	}
	return w;
}

static int friend_compare_func(const LinphoneFriend *lf1, const LinphoneFriend *lf2){
	int w1,w2,ret;
	w1=get_friend_weight(lf1);
	w2=get_friend_weight(lf2);
	if (w1==w2){
		const char *u1,*u2;
		const LinphoneAddress *addr1,*addr2;
		addr1=linphone_friend_get_address(lf1);
		addr2=linphone_friend_get_address(lf2);
		if ((addr1 == NULL) && (addr2 == NULL)) return 0;
		if ((addr1 == NULL) && (addr2 != NULL)) return -1;
		if ((addr1 != NULL) && (addr2 == NULL)) return 1;
		u1=linphone_friend_get_name(lf1) ? linphone_friend_get_name(lf1) : linphone_address_get_display_name(addr1) ? linphone_address_get_display_name(addr1) : linphone_address_get_username(addr1);
		u2=linphone_friend_get_name(lf2) ? linphone_friend_get_name(lf2) :linphone_address_get_display_name(addr2) ? linphone_address_get_display_name(addr2) : linphone_address_get_username(addr2);
		if (u1 && u2) {
			ret = strcasecmp(u1,u2);
		} else if (u1) {
			ret = 1;
		} else {
			ret = -1;
		}
	} else {
		ret = w2-w1;
	}
	return ret;
}

static bctbx_list_t *sort_friend_list(const bctbx_list_t *friends){
	bctbx_list_t *ret=NULL;
	const bctbx_list_t *elem;
	LinphoneFriend *lf;

	for(elem=friends;elem!=NULL;elem=elem->next){
		lf=(LinphoneFriend*)elem->data;
		ret=bctbx_list_insert_sorted(ret,lf,(bctbx_compare_func)friend_compare_func);
	}
	return ret;
}

#if 0
void linphone_gtk_friend_list_on_presence_column_clicked(GtkTreeModel *model){
	GtkSortType st;
	gint column;

	gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model),&column,&st);
	if (column==FRIEND_ID){
		if (st==GTK_SORT_ASCENDING) st=GTK_SORT_DESCENDING;
		else st=GTK_SORT_ASCENDING;
	}else st=GTK_SORT_ASCENDING;
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),FRIEND_ID,st);
}
#endif

static void linphone_gtk_friend_list_init(GtkWidget *friendlist){
	GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(friendlist));
	GtkTreeSelection *select = gtk_tree_view_get_selection (GTK_TREE_VIEW (friendlist));

	linphone_gtk_init_bookmark_icon();
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(friendlist),friend_search_func,NULL,NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store),FRIEND_NAME,friend_sort,NULL,NULL);
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(select), "changed", G_CALLBACK(linphone_gtk_contact_clicked), NULL);

	g_object_set_data(G_OBJECT(friendlist), "friendlist_initialized", (gpointer)TRUE);
}

void linphone_gtk_show_directory_search(void){
	LinphoneProxyConfig *cfg=NULL;
	SipSetupContext * ssc=NULL;
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *search_box=linphone_gtk_get_widget(mw,"directory_search_box");

	cfg = linphone_core_get_default_proxy_config(linphone_gtk_get_core());
	if (cfg){
		ssc=linphone_proxy_config_get_sip_setup_context(cfg);
		if (ssc!=NULL && sip_setup_context_get_capabilities(ssc) & SIP_SETUP_CAP_BUDDY_LOOKUP){
			GtkWidget *entry=linphone_gtk_get_widget(mw,"directory_search_entry");
			gchar  *tooltip;
			GdkColor grey={0,40000,40000,40000};
			gtk_widget_show(search_box);
			tooltip=g_strdup_printf(_("Search in %s directory"),linphone_proxy_config_get_domain(cfg));
			gtk_widget_modify_text(entry,GTK_STATE_NORMAL,&grey);
			gtk_entry_set_text(GTK_ENTRY(entry),tooltip);
			g_object_set_data(G_OBJECT(entry),"active",GINT_TO_POINTER(0));
			g_free(tooltip);
			return;
		}
	}
	gtk_widget_hide(search_box);
}

gboolean linphone_gtk_directory_search_focus_out(GtkWidget *entry){
	if (gtk_entry_get_text_length(GTK_ENTRY(entry))==0)
		linphone_gtk_show_directory_search();
	return FALSE;
}

gboolean linphone_gtk_directory_search_focus_in(GtkWidget *entry){
	if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry),"active"))==0){
		gtk_entry_set_text(GTK_ENTRY(entry),"");
		gtk_widget_modify_text(entry,GTK_STATE_NORMAL,NULL);
		g_object_set_data(G_OBJECT(entry),"active",GINT_TO_POINTER(1));
	}
	return FALSE;
}

void linphone_gtk_directory_search_activate(GtkWidget *entry){
	LinphoneProxyConfig *cfg;
	GtkWidget *w;
	cfg = linphone_core_get_default_proxy_config(linphone_gtk_get_core());
	w=linphone_gtk_show_buddy_lookup_window(linphone_proxy_config_get_sip_setup_context(cfg));
	if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry),"active"))==1)
		linphone_gtk_buddy_lookup_set_keyword(w,gtk_entry_get_text(GTK_ENTRY(entry)));
}

void linphone_gtk_directory_search_button_clicked(GtkWidget *button){
	linphone_gtk_directory_search_activate(
		linphone_gtk_get_widget(gtk_widget_get_toplevel(button),"directory_search_entry"));
}

void linphone_gtk_show_friends(void){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(mw,"contact_list");
	GtkListStore *store=NULL;
	GtkTreeIter iter;
	const bctbx_list_t *itf;
	LinphoneCore *core=linphone_gtk_get_core();
	bctbx_list_t *sorted;
	LinphoneChatRoom *cr=NULL;

	linphone_gtk_show_directory_search();
	if (!g_object_get_data(G_OBJECT(friendlist), "friendlist_initialized")) {
		linphone_gtk_friend_list_init(friendlist);
	}

	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(friendlist)));
	gtk_list_store_clear(store);

	sorted=sort_friend_list(linphone_core_get_friend_list(core));

	for(itf=sorted;itf!=NULL;itf=bctbx_list_next(itf)){
		LinphoneFriend *lf=(LinphoneFriend*)itf->data;
		const LinphoneAddress *f_addr=linphone_friend_get_address(lf);
		const char *name=linphone_friend_get_name(lf);
		char *uri = NULL;
		const char *display=name;
		char *escaped=NULL;
		int nbmsg=0;

		//BuddyInfo *bi;
		gboolean send_subscribe=linphone_friend_get_send_subscribe(lf);
		if (f_addr != NULL) uri = linphone_address_as_string(f_addr);
		if ((display==NULL || display[0]=='\0') && (f_addr != NULL)) {
			display=linphone_address_get_username(f_addr);
		}
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,FRIEND_NAME, display,FRIEND_ID,lf,
				FRIEND_PRESENCE_IMG, send_subscribe ? status_to_icon_name(linphone_friend_get_status(lf)) : NULL,
				FRIEND_CHAT,"linphone-chat-nothing", -1);

		if (f_addr != NULL) {
			cr=linphone_gtk_create_chatroom(f_addr);
			gtk_list_store_set(store,&iter,FRIEND_CHATROOM,cr,-1);
			nbmsg=linphone_chat_room_get_unread_messages_count(cr);
			if(nbmsg != 0){
				gtk_list_store_set(store,&iter,FRIEND_CHAT,"linphone-chat-new-message",
						FRIEND_CHAT_BUTTON_VISIBLE, TRUE, -1);
			}
			escaped=g_markup_escape_text(uri,-1);
			gtk_list_store_set(store,&iter,FRIEND_SIP_ADDRESS,escaped,-1);
			g_free(escaped);
			ms_free(uri);
		}
	}
	bctbx_list_free(sorted);
}

void linphone_gtk_show_contact(LinphoneFriend *lf, GtkWidget *parent){
	GtkWidget *w = linphone_gtk_create_window("contact", parent);
	char *uri;
	const char *name = linphone_friend_get_name(lf);
	const LinphoneAddress *f_addr = linphone_friend_get_address(lf);

	if (f_addr != NULL) {
		uri=linphone_address_as_string_uri_only(f_addr);
		if (uri) {
			gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"sip_address")),uri);
			ms_free(uri);
		}
	}

	if (name){
		gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"name")),name);
	}

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"show_presence")),
					linphone_friend_get_send_subscribe(lf));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"allow_presence")),
					linphone_friend_get_inc_subscribe_policy(lf)==LinphoneSPAccept);
	g_object_set_data(G_OBJECT(w),"friend_ref",(gpointer)lf);

	gtk_widget_show(w);
}

void linphone_gtk_contact_cancel(GtkWidget *button){
	gtk_widget_destroy(gtk_widget_get_toplevel(button));
}

void linphone_gtk_contact_ok(GtkWidget *button){
	GtkWidget *w=gtk_widget_get_toplevel(button);
	LinphoneFriend *lf=(LinphoneFriend*)g_object_get_data(G_OBJECT(w),"friend_ref");
	LinphoneFriend *lf2;
	gboolean show_presence=FALSE,allow_presence=FALSE;
	const gchar *name,*uri;
	LinphoneAddress* friend_address;
	if (lf==NULL){
		lf=linphone_core_create_friend(linphone_gtk_get_core());
		if (linphone_gtk_get_ui_config_int("use_subscribe_notify",1)==1){
			show_presence=FALSE;
			allow_presence=FALSE;
		}
		linphone_friend_set_inc_subscribe_policy(lf,allow_presence ? LinphoneSPAccept : LinphoneSPDeny);
		linphone_friend_send_subscribe(lf,show_presence);
	}

	name = NULL;
	if(gtk_entry_get_text_length(GTK_ENTRY(linphone_gtk_get_widget(w,"name"))) != 0){
		name=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"name")));
	}
	uri=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"sip_address")));
	show_presence=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"show_presence")));
	allow_presence=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"allow_presence")));
	friend_address=linphone_core_interpret_url(linphone_gtk_get_core(),uri);
	if (friend_address==NULL){
		linphone_gtk_display_something(GTK_MESSAGE_WARNING,_("Invalid sip contact !"));
		return ;
	}

	linphone_friend_set_address(lf,friend_address);
	linphone_friend_set_name(lf,name);
	linphone_friend_send_subscribe(lf,show_presence);
	linphone_friend_set_inc_subscribe_policy(lf,allow_presence==TRUE ? LinphoneSPAccept : LinphoneSPDeny);
	if (linphone_friend_in_list(lf)) {
		linphone_friend_done(lf);
	} else {
		char *uri=linphone_address_as_string_uri_only(friend_address);
		lf2=linphone_core_get_friend_by_address(linphone_gtk_get_core(),uri);
		ms_free(uri);
		if(lf2==NULL){
			linphone_core_add_friend(linphone_gtk_get_core(),lf);
		}
	}
	linphone_address_unref(friend_address);
	linphone_gtk_show_friends();
	gtk_widget_destroy(w);
}

static GtkWidget *linphone_gtk_create_contact_menu(GtkWidget *contact_list){
	GtkWidget *menu=gtk_menu_new();
	GtkWidget *menu_item;
	gchar *edit_label=NULL;
	gchar *delete_label=NULL;
	gchar *delete_hist_label=NULL;
	gchar *add_contact_label=NULL;
	gchar *name=NULL;
	GtkTreeSelection *select;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkWidget *image;
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneProxyConfig *cfg=NULL;
	SipSetupContext * ssc=NULL;
	bool_t show_menu_separator=FALSE;

	cfg = linphone_core_get_default_proxy_config(lc);
	if (cfg){
		ssc=linphone_proxy_config_get_sip_setup_context(cfg);
	}

	g_signal_connect(G_OBJECT(menu), "selection-done", G_CALLBACK (gtk_widget_destroy), NULL);
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(contact_list));
	add_contact_label=g_strdup_printf(_("Add a new contact"));
	if (gtk_tree_selection_get_selected (select, &model, &iter)){
		gtk_tree_model_get(model, &iter,FRIEND_NAME , &name, -1);
		edit_label=g_strdup_printf(_("Edit contact '%s'"),name);
		delete_label=g_strdup_printf(_("Delete contact '%s'"),name);
		delete_hist_label=g_strdup_printf(_("Delete chat history of '%s'"),name);
		g_free(name);
		show_menu_separator=TRUE;
	}
	if (edit_label){
		menu_item=gtk_image_menu_item_new_with_label(edit_label);
		image=gtk_image_new_from_icon_name("linphone-edit",GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
		gtk_widget_show(image);
		gtk_widget_show(menu_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_edit_contact,contact_list);
	}
	if (delete_label){
		menu_item=gtk_image_menu_item_new_with_label(delete_label);
		image=gtk_image_new_from_icon_name("linphone-delete",GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
		gtk_widget_show(image);
		gtk_widget_show(menu_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_remove_contact,contact_list);
	}

	if (delete_hist_label){
		GtkWidget *menu_item_separator=gtk_separator_menu_item_new();
		gtk_widget_show(menu_item_separator);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item_separator);
		menu_item=gtk_image_menu_item_new_with_label(delete_hist_label);
		image=gtk_image_new_from_icon_name("linphone-delete",GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
		gtk_widget_show(image);
		gtk_widget_show(menu_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_delete_history,contact_list);
	}

	if (ssc && (sip_setup_context_get_capabilities(ssc) & SIP_SETUP_CAP_BUDDY_LOOKUP)) {
		gchar *tmp=g_strdup_printf(_("Add new contact from %s directory"),linphone_proxy_config_get_domain(cfg));
		menu_item=gtk_image_menu_item_new_with_label(tmp);
		g_free(tmp);
		image=gtk_image_new_from_icon_name("linphone-contact-add",GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
		gtk_widget_show(image);
		gtk_widget_show(menu_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_show_buddy_lookup_window,ssc);
	}

	if (show_menu_separator) {
		GtkWidget *menu_item_separator=gtk_separator_menu_item_new();
		gtk_widget_show(menu_item_separator);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item_separator);
	}

	menu_item=gtk_image_menu_item_new_with_label(add_contact_label);
	image=gtk_image_new_from_icon_name("linphone-contact-add",GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
	gtk_widget_show(image);
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
	g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_add_contact,contact_list);
	gtk_widget_show(menu);
	gtk_menu_attach_to_widget (GTK_MENU (menu), contact_list, NULL);

	g_free(add_contact_label);
	if (edit_label) g_free(edit_label);
	if (delete_label) g_free(delete_label);
	return menu;
}

gboolean linphone_gtk_popup_contact_menu(GtkWidget *list, GdkEventButton *event){
	GtkWidget *m=linphone_gtk_create_contact_menu(list);
	gtk_menu_popup (GTK_MENU (m), NULL, NULL, NULL, NULL,
                  event ? event->button : 0, event ? event->time : gtk_get_current_event_time());
	return TRUE;
}

static int get_column_index(GtkTreeView *friendlist, const GtkTreeViewColumn *column) {
	GList *columns = gtk_tree_view_get_columns(friendlist);
	int i = g_list_index(columns, column);
	g_list_free(columns);
	return i;
}

static void select_row(GtkTreeView *treeview, GtkTreePath *path) {
	GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
	gtk_tree_selection_select_path(selection, path);
}

gboolean linphone_gtk_contact_list_button_pressed(GtkTreeView *friendlist, GdkEventButton *event){
	/* Ignore double-clicks and triple-clicks */
	gboolean ret = FALSE;
	int x_bin, y_bin;
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(friendlist);

	gtk_tree_view_convert_widget_to_bin_window_coords(friendlist, (gint)event->x, (gint)event->y, &x_bin, &y_bin);
	gtk_tree_view_get_path_at_pos(friendlist, x_bin, y_bin, &path, &column, NULL, NULL);

	if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
		if(path) gtk_tree_selection_select_path(selection, path);
		ret = linphone_gtk_popup_contact_menu(GTK_WIDGET(friendlist), event);
	} else if(event->button == 1 && event->type == GDK_BUTTON_PRESS){
		if(path && column) {
			int numcol = get_column_index(friendlist, column);
			if(numcol == 2) {
				select_row(friendlist, path);
				linphone_gtk_call_selected(friendlist);
				ret = TRUE;
			} else if(numcol == 3) {
				select_row(friendlist, path);
				linphone_gtk_chat_selected(GTK_WIDGET(friendlist));
				ret = TRUE;
			}
		}
	}
	if(path) gtk_tree_path_free(path);
	return ret;
}

void linphone_gtk_buddy_info_updated(LinphoneCore *lc, LinphoneFriend *lf){
	/*refresh the entire list*/
	linphone_gtk_show_friends();
}

static gboolean update_hovered_row_path(GtkTreeView *friendlist, int x_window, int y_window) {
	int x_bin, y_bin;
	GtkTreePath *new_path;
	GtkTreePath *old_path = (GtkTreePath *)g_object_get_data(G_OBJECT(friendlist), "hovered_row");
	gtk_tree_view_convert_widget_to_bin_window_coords(friendlist, x_window, y_window, &x_bin, &y_bin);
	gtk_tree_view_get_path_at_pos(friendlist, x_bin, y_bin, &new_path, NULL, NULL, NULL);
	if((new_path == NULL && old_path == NULL) || (new_path && old_path && gtk_tree_path_compare(new_path, old_path) == 0)) {
		if(new_path) gtk_tree_path_free(new_path);
		return FALSE;
	} else {
		g_object_set_data_full(G_OBJECT(friendlist), "hovered_row", new_path, (GDestroyNotify)gtk_tree_path_free);
		return TRUE;
	}
}

gboolean linphone_gtk_friend_list_enter_event_handler(GtkTreeView *friendlist, GdkEventCrossing *event) {
	gboolean path_has_changed = update_hovered_row_path(friendlist, (int)event->x, (int)event->y);
	if(path_has_changed) linphone_gtk_friend_list_update_button_display(friendlist);
	return FALSE;
}

gboolean linphone_gtk_friend_list_leave_event_handler(GtkTreeView *friendlist, GdkEventCrossing *event) {
	GtkTreePath *hovered_row = (GtkTreePath *)g_object_get_data(G_OBJECT(friendlist), "hovered_row");
	if(hovered_row) {
		g_object_set_data(G_OBJECT(friendlist), "hovered_row", NULL);
		linphone_gtk_friend_list_update_button_display(friendlist);
	}
	return FALSE;
}

gboolean linphone_gtk_friend_list_motion_event_handler(GtkTreeView *friendlist, GdkEventMotion *event) {
	gboolean path_has_changed = update_hovered_row_path(friendlist, (int)event->x, (int)event->y);
	if(path_has_changed) linphone_gtk_friend_list_update_button_display(friendlist);
	return FALSE;
}

#define CONFIG_FILE ".linphone-friends.db"

char *linphone_gtk_friends_storage_get_db_file(const char *filename){
	const int path_max=1024;
	char *db_file=NULL;

	db_file=(char *)g_malloc(path_max*sizeof(char));
	if (filename==NULL) filename=CONFIG_FILE;
	/*try accessing a local file first if exists*/
	if (bctbx_file_exist(CONFIG_FILE)==0){
		snprintf(db_file,path_max,"%s",filename);
	}else{
#ifdef _WIN32
		const char *appdata=getenv("APPDATA");
		if (appdata){
			snprintf(db_file,path_max,"%s\\%s",appdata,LINPHONE_CONFIG_DIR);
			CreateDirectory(db_file,NULL);
			snprintf(db_file,path_max,"%s\\%s\\%s",appdata,LINPHONE_CONFIG_DIR,filename);
		}
#else
		const char *home=getenv("HOME");
		if (home==NULL) home=".";
		snprintf(db_file,path_max,"%s/%s",home,filename);
#endif
	}
	return db_file;
}
