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

#include <gtk/gtk.h>
#include "linphone.h"

static GtkWidget *linphone_gtk_create_contact_menu(GtkWidget *contact_list);

enum{
	FRIEND_PRESENCE_IMG,
	FRIEND_NAME,
	FRIEND_PRESENCE_STATUS,
	FRIEND_ID,
	FRIEND_CHATROOM,
	FRIEND_SIP_ADDRESS,
	FRIEND_ICON,
	FRIEND_CALL,
	FRIEND_CHAT,
	FRIEND_LIST_NCOL
};

typedef struct _status_picture_tab_t{
	LinphoneOnlineStatus status;
	const char *img;
} status_picture_tab_t;

status_picture_tab_t status_picture_tab[]={
	{	LinphoneStatusOnline,		"status-green.png"	},
	{	LinphoneStatusBusy,		"status-orange.png"		},
	{	LinphoneStatusBeRightBack,	"status-orange.png"		},
	{	LinphoneStatusAway,		"status-orange.png"		},
	{	LinphoneStatusOnThePhone,	"status-orange.png"		},
	{	LinphoneStatusOutToLunch,	"status-orange.png"		},
	{	LinphoneStatusDoNotDisturb,	"status-red.png"	},
	{	LinphoneStatusMoved,		"status-orange.png"	},
	{	LinphoneStatusAltService,	"status-orange.png"	},
	{	LinphoneStatusOffline,	"status-offline.png"		},
	{	LinphoneStatusPending,	"status-offline.png"		},
	{	LinphoneStatusEnd,		NULL			},
};

static GdkPixbuf *create_status_picture(LinphoneOnlineStatus ss){
	status_picture_tab_t *t=status_picture_tab;
	while(t->img!=NULL){
		if (ss==t->status){
			GdkPixbuf *pixbuf;
			pixbuf = create_pixbuf(t->img);
			return pixbuf;
		}
		++t;
	}
	g_error("No pixmap defined for status %i",ss);
	return NULL;
}

static GdkPixbuf *create_call_picture(){
	GdkPixbuf *pixbuf;
	pixbuf = create_pixbuf("call.png");
	return pixbuf;
}

static GdkPixbuf *create_unread_msg(){
	GdkPixbuf *pixbuf;
	pixbuf = create_pixbuf("active_chat.png");
	return pixbuf;
}

static GdkPixbuf *create_chat_picture(){
	GdkPixbuf *pixbuf;
	pixbuf = create_pixbuf("chat.png");
	return pixbuf;
}

/*
void linphone_gtk_set_friend_status(GtkWidget *friendlist , LinphoneFriend * fid, const gchar *url, const gchar *status, const gchar *img){
	GtkTreeIter iter;
	LinphoneFriend *tmp=0;

	GtkTreeModel *model=gtk_tree_view_get_model(GTK_TREE_VIEW(friendlist));
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
			}
		}while(gtk_tree_model_iter_next(model,&iter));
	}
}
*/

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
	if (gtk_tree_selection_get_selected (select, &model, &iter))
	{
		gtk_tree_model_get (model, &iter,FRIEND_ID , &lf, -1);
		friend=linphone_address_as_string(linphone_friend_get_address(lf));
		gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"uribar")),friend);
		ms_free(friend);
	}
}

void linphone_gtk_add_contact(){
	GtkWidget *w=linphone_gtk_create_window("contact");
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
		linphone_gtk_show_contact(lf);
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
			char *from=g_object_get_data(G_OBJECT(friendlist),"from");
			char *addr=linphone_address_as_string(linphone_friend_get_address(lf));
			if(g_strcmp0(from,addr)==0){
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
		linphone_gtk_show_friends();
	}
}

static void linphone_gtk_call_selected(GtkTreeView *treeview){
	linphone_gtk_set_selection_to_uri_bar(treeview);
	linphone_gtk_start_call(linphone_gtk_get_widget(gtk_widget_get_toplevel(GTK_WIDGET(treeview)),
					"start_call"));
}

void linphone_gtk_friend_list_update_chat_picture(){
	GtkTreeIter iter;
	GtkWidget *w = linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(w,"contact_list");
	GtkTreeModel *model=gtk_tree_view_get_model(GTK_TREE_VIEW(friendlist));
	LinphoneChatRoom *cr=NULL;
	int nbmsg=0;
	if (gtk_tree_model_get_iter_first(model,&iter)) {
		do{
			gtk_tree_model_get (model, &iter,FRIEND_CHATROOM , &cr, -1);
			nbmsg=linphone_chat_room_get_unread_messages_count(cr);
			if(nbmsg != 0){
				gtk_list_store_set(GTK_LIST_STORE(model),&iter,FRIEND_CHAT,create_unread_msg(),-1);
			} else {
				gtk_list_store_set(GTK_LIST_STORE(model),&iter,FRIEND_CHAT,create_chat_picture(),-1);
			}
		}while(gtk_tree_model_iter_next(model,&iter));
	}
}

static gboolean grab_focus(GtkWidget *w){
	gtk_widget_grab_focus(w);
	return FALSE;
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
	char *la_str=linphone_address_as_string(la);
	
	lf=linphone_core_get_friend_by_address(linphone_gtk_get_core(),la_str);
	if(lf==NULL){
		cr=linphone_gtk_create_chatroom(la);
		g_object_set_data(G_OBJECT(friendlist),"from",la_str);
		if(chat_view==NULL){
			chat_view=linphone_gtk_init_chatroom(cr,la);
			g_object_set_data(G_OBJECT(friendlist),"chatview",(gpointer)chat_view);
		} else {
			linphone_gtk_load_chatroom(cr,la,chat_view);
		}
		gtk_notebook_set_current_page(notebook,gtk_notebook_page_num(notebook,chat_view));
		linphone_gtk_friend_list_update_chat_picture();
		g_idle_add((GSourceFunc)grab_focus,linphone_gtk_get_widget(chat_view,"text_entry"));
	} else {
		store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(friendlist)));
		if (gtk_tree_model_get_iter_first(model,&iter)) {
			do{
				const LinphoneAddress *uri;
				char *lf_str;
				gtk_tree_model_get(model, &iter,FRIEND_ID , &lf, -1);
				uri=linphone_friend_get_address(lf);
				lf_str=linphone_address_as_string(uri);
				if( g_strcmp0(lf_str,la_str)==0){
					gtk_tree_model_get (model, &iter,FRIEND_CHATROOM , &cr, -1);
					if(cr==NULL){
						cr=linphone_gtk_create_chatroom(uri);
						gtk_list_store_set(store,&iter,FRIEND_CHATROOM,cr,-1);
					}
					g_object_set_data(G_OBJECT(friendlist),"from",linphone_address_as_string(uri));
					if(chat_view==NULL){
						chat_view=linphone_gtk_init_chatroom(cr,uri);
						g_object_set_data(G_OBJECT(friendlist),"chatview",(gpointer)chat_view);
					} else {
						linphone_gtk_load_chatroom(cr,uri,chat_view);
					}
					gtk_notebook_set_current_page(notebook,gtk_notebook_page_num(notebook,chat_view));
					linphone_gtk_friend_list_update_chat_picture();
					g_idle_add((GSourceFunc)grab_focus,linphone_gtk_get_widget(chat_view,"text_entry"));
					break;
				}
			}while(gtk_tree_model_iter_next(model,&iter));
		}
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
				linphone_chat_room_mark_as_read(cr);
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
		const LinphoneAddress *uri;
		gtk_tree_model_get (model, &iter,FRIEND_ID , &lf, -1);
		gtk_tree_model_get (model, &iter,FRIEND_CHATROOM , &cr, -1);
		uri=linphone_friend_get_address(lf);
		if(cr==NULL){
			cr=linphone_gtk_create_chatroom(uri);
			gtk_list_store_set(store,&iter,FRIEND_CHATROOM,cr,-1);
		}
		page=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
		g_object_set_data(G_OBJECT(friendlist),"from",linphone_address_as_string(uri));
		if(page==NULL){
			page=linphone_gtk_init_chatroom(cr,uri);
			g_object_set_data(G_OBJECT(friendlist),"chatview",(gpointer)page);
		} else {
			linphone_gtk_load_chatroom(cr,uri,page);
		}
		linphone_chat_room_mark_as_read(cr);
		gtk_notebook_set_current_page(notebook,gtk_notebook_page_num(notebook,page));
		linphone_gtk_friend_list_update_chat_picture();
		g_idle_add((GSourceFunc)grab_focus,linphone_gtk_get_widget(page,"text_entry"));
	}
}

void linphone_gtk_contact_activated(GtkTreeView     *treeview,
                                    GtkTreePath     *path,
                                    GtkTreeViewColumn *column,
                                        gpointer         user_data)
{
	//linphone_gtk_call_selected(treeview);
}

void linphone_gtk_contact_clicked(GtkTreeView     *treeview){
	linphone_gtk_set_selection_to_uri_bar(treeview);
	if(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(treeview),"numcol"))==1){
		linphone_gtk_call_selected(treeview);
	} else {
		if(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(treeview),"numcol"))==2){
			linphone_gtk_chat_selected(GTK_WIDGET(treeview));
		}
	}
	g_object_set_data(G_OBJECT(treeview),"numcol",GINT_TO_POINTER(0));
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

static GtkWidget * create_presence_menu(){
	GtkWidget *menu=gtk_menu_new();
	GtkWidget *menu_item;
	GdkPixbuf *pbuf;
	status_picture_tab_t *t;
	for(t=status_picture_tab;t->img!=NULL;++t){
		if (t->status==LinphoneStatusPending){
			continue;
		}
		menu_item=gtk_image_menu_item_new_with_label(linphone_online_status_to_string(t->status));
		pbuf=create_status_picture(t->status);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),
						gtk_image_new_from_pixbuf(pbuf));
		g_object_unref(G_OBJECT(pbuf));
		gtk_widget_show(menu_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_set_my_presence,GINT_TO_POINTER(t->status));
	}
	return menu;
}

void linphone_gtk_set_my_presence(LinphoneOnlineStatus ss){
	GtkWidget *button=linphone_gtk_get_widget(linphone_gtk_get_main_window(),"presence_button");
	GdkPixbuf *pbuf=create_status_picture(ss);
	GtkWidget *image=gtk_image_new_from_pixbuf(pbuf);
	GtkWidget *menu;
	g_object_unref(G_OBJECT(pbuf));
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
	const char *text=gtk_entry_get_text(entry);
	if (text && strlen(text)>0){
		char *uri;
		LinphoneFriend *lf;
		linphone_core_interpret_friend_uri(linphone_gtk_get_core(),text,&uri);
		if (uri==NULL){
			return ;
		}
		lf=linphone_core_get_friend_by_address(linphone_gtk_get_core(),uri);
		if (lf==NULL)
			lf=linphone_friend_new_with_addr(uri);
		if (lf!=NULL){
			linphone_gtk_show_contact(lf);
		}
		ms_free(uri);
	}
}

static void update_star(GtkEntry *entry, gboolean is_known){
	GdkPixbuf *active,*starred,*unstarred;
	active=gtk_entry_get_icon_pixbuf(entry,GTK_ENTRY_ICON_SECONDARY);
	starred=g_object_get_data(G_OBJECT(entry),"starred_icon");
	unstarred=g_object_get_data(G_OBJECT(entry),"unstarred_icon");
	if (is_known && (active==unstarred)){
		gtk_entry_set_icon_from_pixbuf(entry,GTK_ENTRY_ICON_SECONDARY,starred);
		gtk_entry_set_icon_tooltip_text(GTK_ENTRY(entry),GTK_ENTRY_ICON_SECONDARY,NULL);
	}else if ((!is_known) && (active==starred)){
		gtk_entry_set_icon_tooltip_text(GTK_ENTRY(entry),GTK_ENTRY_ICON_SECONDARY,_("Add to addressbook"));
		gtk_entry_set_icon_from_pixbuf(entry,GTK_ENTRY_ICON_SECONDARY,unstarred);
	}
}

static void check_contact(GtkEditable *editable, LinphoneCore *lc){
	char *tmp=gtk_editable_get_chars(editable,0,-1);
	if (tmp!=NULL){
		if (strlen(tmp)>0){
			char *uri=NULL;
			linphone_core_interpret_friend_uri(lc,tmp,&uri);
			if (uri){
				LinphoneFriend *lf=linphone_core_get_friend_by_address(lc,uri);
				ms_free(uri);
				if (lf) {
					update_star(GTK_ENTRY(editable),TRUE);
					g_free(tmp);
					return;
				}
			}
		}
		g_free(tmp);
	}
	update_star(GTK_ENTRY(editable),FALSE);
}

static void linphone_gtk_init_bookmark_icon(void){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *entry=linphone_gtk_get_widget(mw,"uribar");
	GdkPixbuf *pbuf=create_pixbuf("contact_unstarred.png");
	gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(entry),GTK_ENTRY_ICON_SECONDARY,pbuf);
	g_object_set_data_full(G_OBJECT(entry),"unstarred_icon",pbuf,g_object_unref);
	pbuf=create_pixbuf("contact_starred.png");
	g_object_set_data_full(G_OBJECT(entry),"starred_icon",pbuf,g_object_unref);
	gtk_entry_set_icon_activatable(GTK_ENTRY(entry),GTK_ENTRY_ICON_SECONDARY,TRUE);
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

static void on_name_column_clicked(GtkTreeModel *model){
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
	LinphoneChatRoom *cr=linphone_core_get_chat_room(lc,linphone_friend_get_address(lf));
	
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
	int w1,w2;
	w1=get_friend_weight(lf1);
	w2=get_friend_weight(lf2);
	if (w1==w2){
		const char *u1,*u2;
		const LinphoneAddress *addr1,*addr2;
		addr1=linphone_friend_get_address(lf1);
		addr2=linphone_friend_get_address(lf2);
		u1=linphone_address_get_username(addr1);
		u2=linphone_address_get_username(addr2);
		if (u1 && u2) return strcasecmp(u1,u2);
		if (u1) return 1;
		else return -1;
	}
	return w2-w1;
}

static MSList *sort_friend_list(const MSList *friends){
	MSList *ret=NULL;
	const MSList *elem;
	LinphoneFriend *lf;

	for(elem=friends;elem!=NULL;elem=elem->next){
		lf=(LinphoneFriend*)elem->data;
		ret=ms_list_insert_sorted(ret,lf,(MSCompareFunc)friend_compare_func);
	}
	return ret;
}

static void on_presence_column_clicked(GtkTreeModel *model){
	GtkSortType st;
	gint column;

	gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model),&column,&st);
	if (column==FRIEND_ID){
		if (st==GTK_SORT_ASCENDING) st=GTK_SORT_DESCENDING;
		else st=GTK_SORT_ASCENDING;
	}else st=GTK_SORT_ASCENDING;
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),FRIEND_ID,st);
}

void create_button(){
	GtkWidget *main_window = linphone_gtk_get_main_window ();
	GtkWidget *button_add = linphone_gtk_get_widget(main_window,"add_button");
	GtkWidget *image;

	image=gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
	gtk_container_add (GTK_CONTAINER (button_add), image);
}

static void linphone_gtk_friend_list_init(GtkWidget *friendlist){
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;

	linphone_gtk_init_bookmark_icon();

	store = gtk_list_store_new(FRIEND_LIST_NCOL,GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,  G_TYPE_POINTER,
								G_TYPE_POINTER, G_TYPE_STRING, GDK_TYPE_PIXBUF, GDK_TYPE_PIXBUF, GDK_TYPE_PIXBUF);

	gtk_tree_view_set_model(GTK_TREE_VIEW(friendlist),GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));

	/* Tree specification*/
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(friendlist),FALSE);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(friendlist),friend_search_func,NULL,NULL);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(friendlist),FRIEND_NAME);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store),FRIEND_NAME,friend_sort,NULL,NULL);
	
	/*Name and presence column*/
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Presence status"),
                                                   renderer,
                                                   "text", FRIEND_PRESENCE_STATUS,
                                                   NULL);
	g_object_set (G_OBJECT(column), "resizable", TRUE, NULL);
	g_signal_connect_swapped(G_OBJECT(column),"clicked",(GCallback)on_presence_column_clicked,GTK_TREE_MODEL(store));
	gtk_tree_view_column_set_clickable(column,TRUE);
	gtk_tree_view_column_set_visible(column,linphone_gtk_get_ui_config_int("friendlist_status",1));
	gtk_tree_view_column_set_min_width(column,50);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column,renderer,TRUE);
	gtk_tree_view_column_add_attribute  (column,renderer,
                                                         "pixbuf",
                                                         FRIEND_PRESENCE_IMG);
	gtk_tree_view_append_column (GTK_TREE_VIEW (friendlist), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                   renderer,
                                                   "text", FRIEND_NAME,NULL);
	g_object_set (G_OBJECT(column), "resizable", TRUE, NULL);
	g_signal_connect_swapped(G_OBJECT(column),"clicked",(GCallback)on_name_column_clicked,GTK_TREE_MODEL(store));
	gtk_tree_view_column_set_clickable(column,TRUE);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_max_width(column,60);
	gtk_tree_view_append_column (GTK_TREE_VIEW (friendlist), column);

	/* Call column*/
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes (_("Call"),renderer,"pixbuf",FRIEND_CALL,NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (friendlist), column);

	/* Chat column*/
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes (_("Chat"),renderer,"pixbuf",FRIEND_CHAT,NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (friendlist), column);

	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (friendlist));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

	gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(friendlist),GTK_TREE_VIEW_GRID_LINES_NONE);
#if GTK_CHECK_VERSION(2,12,0)
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(friendlist),FRIEND_SIP_ADDRESS);
#endif

	gtk_widget_set_size_request(friendlist,200,120);
	/*gtk_combo_box_set_active(GTK_COMBO_BOX(linphone_gtk_get_widget(
					gtk_widget_get_toplevel(friendlist),"show_category")),0);*/
}

void linphone_gtk_show_directory_search(void){
	LinphoneProxyConfig *cfg=NULL;
	SipSetupContext * ssc=NULL;
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *search_box=linphone_gtk_get_widget(mw,"directory_search_box");

	linphone_core_get_default_proxy(linphone_gtk_get_core(),&cfg);
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
	linphone_core_get_default_proxy(linphone_gtk_get_core(),&cfg);
	GtkWidget *w=linphone_gtk_show_buddy_lookup_window(linphone_proxy_config_get_sip_setup_context(cfg));
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
	const MSList *itf;
	//GtkWidget *filter=linphone_gtk_get_widget(mw,"search_bar");
	LinphoneCore *core=linphone_gtk_get_core();
	//const gchar *search=NULL;
	//gboolean lookup=FALSE;
	MSList *sorted;
	LinphoneChatRoom *cr=NULL;

	linphone_gtk_show_directory_search();
	if (gtk_tree_view_get_model(GTK_TREE_VIEW(friendlist))==NULL){
		linphone_gtk_friend_list_init(friendlist);
	}
	
	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(friendlist)));
	gtk_list_store_clear(store);

	//search=gtk_entry_get_text(GTK_ENTRY(filter));
	//if (search==NULL || search[0]=='\0')
	//	lookup=FALSE;
	//else lookup=TRUE;

	sorted=sort_friend_list(linphone_core_get_friend_list(core));

	for(itf=sorted;itf!=NULL;itf=ms_list_next(itf)){
		LinphoneFriend *lf=(LinphoneFriend*)itf->data;
		const LinphoneAddress *f_uri=linphone_friend_get_address(lf);
		char *uri=linphone_address_as_string(f_uri);
		const char *name=linphone_address_get_display_name(f_uri);
		const char *display=name;
		char *escaped=NULL;
		//char buf[26]={0};
		int nbmsg=0;

		/*if (lookup){
			if (strstr(uri,search)==NULL){
				ms_free(uri);
				continue;
			}
		}*/
		//BuddyInfo *bi;
		gboolean send_subscribe=linphone_friend_get_send_subscribe(lf);
		if (name==NULL || name[0]=='\0') {
			display=linphone_address_get_username(f_uri);
		}
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,FRIEND_NAME, display,FRIEND_ID,lf,
			    FRIEND_PRESENCE_IMG, send_subscribe ? create_status_picture(linphone_friend_get_status(lf)) : NULL,
				FRIEND_CHAT,create_chat_picture(),FRIEND_CALL,create_call_picture(),-1);
		cr=linphone_gtk_create_chatroom(f_uri);
		gtk_list_store_set(store,&iter,FRIEND_CHATROOM,cr,-1);
		nbmsg=linphone_chat_room_get_unread_messages_count(cr);
		if(nbmsg != 0){
			gtk_list_store_set(store,&iter,FRIEND_CHAT,create_unread_msg(),-1);
		}
		escaped=g_markup_escape_text(uri,-1);
		gtk_list_store_set(store,&iter,FRIEND_SIP_ADDRESS,escaped,-1);
		g_free(escaped);
		//bi=linphone_friend_get_info(lf);
		/*if (bi!=NULL && bi->image_data!=NULL){
			GdkPixbuf *pbuf=
				_gdk_pixbuf_new_from_memory_at_scale(bi->image_data,bi->image_length,-1,40,TRUE);
			if (pbuf) {
				//gtk_list_store_set(store,&iter,FRIEND_ICON,pbuf,-1);
				g_object_unref(G_OBJECT(pbuf));
			}
		}*/
		ms_free(uri);
	}
	ms_list_free(sorted);
}

void linphone_gtk_show_contact(LinphoneFriend *lf){
	GtkWidget *w=linphone_gtk_create_window("contact");
	char *uri;
	const char *name;
	const LinphoneAddress *f_uri=linphone_friend_get_address(lf);
	uri=linphone_address_as_string_uri_only(f_uri);
	name=linphone_address_get_display_name(f_uri);
	if (uri) {
		gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"sip_address")),uri);
		ms_free(uri);
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
	char *fixed_uri=NULL;
	gboolean show_presence=FALSE,allow_presence=FALSE;
	const gchar *name,*uri;
	LinphoneAddress* friend_address;
	if (lf==NULL){
		lf=linphone_friend_new();
		if (linphone_gtk_get_ui_config_int("use_subscribe_notify",1)==1){
			show_presence=FALSE;
			allow_presence=FALSE;
		}
		linphone_friend_set_inc_subscribe_policy(lf,allow_presence ? LinphoneSPAccept : LinphoneSPDeny);
		linphone_friend_send_subscribe(lf,show_presence);
	}
	name=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"name")));
	uri=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"sip_address")));
	show_presence=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"show_presence")));
	allow_presence=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"allow_presence")));
	linphone_core_interpret_friend_uri(linphone_gtk_get_core(),uri,&fixed_uri);
	if (fixed_uri==NULL){
		linphone_gtk_display_something(GTK_MESSAGE_WARNING,_("Invalid sip contact !"));
		return ;
	}
	friend_address = linphone_address_new(fixed_uri);
	linphone_address_set_display_name(friend_address,name);
	linphone_friend_set_addr(lf,friend_address);
	linphone_address_destroy(friend_address);

	linphone_friend_send_subscribe(lf,show_presence);
	linphone_friend_set_inc_subscribe_policy(lf,allow_presence==TRUE ? LinphoneSPAccept : LinphoneSPDeny);
	if (linphone_friend_in_list(lf)) {
		linphone_friend_done(lf);
	} else {
		lf2=linphone_core_get_friend_by_address(linphone_gtk_get_core(),fixed_uri);
		if(lf2==NULL){
			linphone_friend_set_name(lf,name);
			linphone_core_add_friend(linphone_gtk_get_core(),lf);
		}
	}
	ms_free(fixed_uri);
	linphone_gtk_show_friends();
	gtk_widget_destroy(w);
}

static GtkWidget *linphone_gtk_create_contact_menu(GtkWidget *contact_list){
	GtkWidget *menu=gtk_menu_new();
	GtkWidget *menu_item;
	gchar *call_label=NULL;
	gchar *text_label=NULL;
	gchar *edit_label=NULL;
	gchar *delete_label=NULL;
	gchar *delete_hist_label=NULL;
	gchar *name=NULL;
	GtkTreeSelection *select;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkWidget *image;
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneProxyConfig *cfg=NULL;
	SipSetupContext * ssc=NULL;

	linphone_core_get_default_proxy(lc,&cfg);
	if (cfg){
		ssc=linphone_proxy_config_get_sip_setup_context(cfg);
	}

	g_signal_connect(G_OBJECT(menu), "selection-done", G_CALLBACK (gtk_widget_destroy), NULL);
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(contact_list));
	if (gtk_tree_selection_get_selected (select, &model, &iter)){
		gtk_tree_model_get(model, &iter,FRIEND_NAME , &name, -1);
		call_label=g_strdup_printf(_("Call %s"),name);
		text_label=g_strdup_printf(_("Send text to %s"),name);
		edit_label=g_strdup_printf(_("Edit contact '%s'"),name);
		delete_label=g_strdup_printf(_("Delete contact '%s'"),name);
		delete_hist_label=g_strdup_printf(_("Delete chat history of '%s'"),name);
		g_free(name);
	}
	if (call_label){
		menu_item=gtk_image_menu_item_new_with_label(call_label);
		image=gtk_image_new_from_stock(GTK_STOCK_NETWORK,GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
		gtk_widget_show(image);
		gtk_widget_show(menu_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_call_selected,contact_list);
	}
	if (text_label){
		menu_item=gtk_image_menu_item_new_with_label(text_label);
		image=gtk_image_new_from_stock(GTK_STOCK_NETWORK,GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
		gtk_widget_show(image);
		gtk_widget_show(menu_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_chat_selected,contact_list);
	}
	if (edit_label){
		menu_item=gtk_image_menu_item_new_with_label(edit_label);
		image=gtk_image_new_from_stock(GTK_STOCK_EDIT,GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
		gtk_widget_show(image);
		gtk_widget_show(menu_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_edit_contact,contact_list);
	}
	if (delete_label){
		menu_item=gtk_image_menu_item_new_with_label(delete_label);
		image=gtk_image_new_from_stock(GTK_STOCK_DELETE,GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
		gtk_widget_show(image);
		gtk_widget_show(menu_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_remove_contact,contact_list);
	}

	if (delete_hist_label){
		menu_item=gtk_image_menu_item_new_with_label(delete_hist_label);
		image=gtk_image_new_from_stock(GTK_STOCK_CLEAR,GTK_ICON_SIZE_MENU);
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
		image=gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
		gtk_widget_show(image);
		gtk_widget_show(menu_item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_show_buddy_lookup_window,ssc);
		gtk_widget_show(menu);
	}

	menu_item=gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
	g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_add_contact,contact_list);
	gtk_widget_show(menu);
	gtk_menu_attach_to_widget (GTK_MENU (menu), contact_list, NULL);

	if (call_label) g_free(call_label);
	if (text_label) g_free(text_label);
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

gint get_col_number_from_tree_view_column (GtkTreeViewColumn *col){
	GList *cols;
	gint   num;
	g_return_val_if_fail ( col != NULL, -1 );
	g_return_val_if_fail ( col->tree_view != NULL, -1 );
	cols = gtk_tree_view_get_columns(GTK_TREE_VIEW(col->tree_view));
	num = g_list_index(cols, (gpointer) col);
	g_list_free(cols);

	return num;
}

static gint tree_view_get_cell_from_pos(GtkTreeView *view, guint x, guint y){
	GtkTreeViewColumn *col = NULL;
	GList *node, *columns;
	gint colx = 0;
	GtkTreePath *path;
	GtkTreeViewDropPosition pos;
	
	g_return_val_if_fail ( view != NULL, 0 );
	columns = gtk_tree_view_get_columns(view);

	gtk_tree_view_get_dest_row_at_pos(view,x,y,&path,&pos);
	if(path != NULL){
		for (node = columns;  node != NULL && col == NULL;  node = node->next){
			GtkTreeViewColumn *checkcol = (GtkTreeViewColumn*) node->data;
			if (x >= colx  &&  x < (colx + checkcol->width)){
				col = checkcol;
				gint num = get_col_number_from_tree_view_column(col);
				return num;
			} else {
				colx += checkcol->width;
			}
		}
	}
	g_list_free(columns);
	return 0;
}

gboolean linphone_gtk_contact_list_button_pressed(GtkWidget *widget, GdkEventButton *event){
	/* Ignore double-clicks and triple-clicks */
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
	{
		return linphone_gtk_popup_contact_menu(widget, event);
	} else if(event->button == 1 && event->type == GDK_BUTTON_PRESS){
		gint numcol = tree_view_get_cell_from_pos(GTK_TREE_VIEW(widget),event->x,event->y);
		if(numcol==2){
			g_object_set_data(G_OBJECT(widget),"numcol",GINT_TO_POINTER(1));
		} else if(numcol==3){
			g_object_set_data(G_OBJECT(widget),"numcol",GINT_TO_POINTER(2));
		}
	}
	return FALSE;
}

void linphone_gtk_buddy_info_updated(LinphoneCore *lc, LinphoneFriend *lf){
	/*refresh the entire list*/
	linphone_gtk_show_friends();
}