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

#ifdef HAVE_GTK_OSX
#include <gtkosxapplication.h>
#endif

void linphone_gtk_quit_chatroom(LinphoneChatRoom *cr) {
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	GtkWidget *nb=linphone_gtk_get_widget(main_window,"viewswitch");
	GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
	GtkWidget *w=g_object_get_data(G_OBJECT(friendlist),"chatview");
	int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"idx"));
	g_return_if_fail(w!=NULL);
	gtk_notebook_remove_page (GTK_NOTEBOOK(nb),idx);
	linphone_gtk_update_chat_picture(FALSE);
	g_object_set_data(G_OBJECT(friendlist),"chatview",NULL);
	gtk_widget_destroy(w);
}

GtkWidget *create_tab_chat_header(LinphoneChatRoom *cr,const LinphoneAddress *uri){
	GtkWidget *w=gtk_hbox_new (FALSE,0);
	GtkWidget *i=create_pixmap ("chat.png");
	GtkWidget *l;
	GtkWidget *b=gtk_button_new_with_label("x");
	
	gtk_widget_set_size_request(b,20,20);
	g_signal_connect_swapped(G_OBJECT(b),"clicked",G_CALLBACK(linphone_gtk_quit_chatroom),cr);
	gchar *text=g_strdup_printf("Chat ");
	l=gtk_label_new (text);
	gtk_box_pack_start (GTK_BOX(w),i,FALSE,FALSE,0);
	gtk_box_pack_start (GTK_BOX(w),l,FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(w),b,TRUE,TRUE,0);
	gtk_widget_show_all(w);
	return w;
}

void linphone_gtk_push_text(GtkWidget *w, const LinphoneAddress *from, const char *message, gboolean me,LinphoneChatRoom *cr){
    GtkTextView *text=GTK_TEXT_VIEW(linphone_gtk_get_widget(w,"textview"));
    GtkTextBuffer *buffer=gtk_text_view_get_buffer(text);
    GtkTextIter iter,begin;
    GList *l = g_object_get_data(G_OBJECT(w),"list");
    gtk_text_buffer_get_start_iter(buffer,&begin);
	int off;
	gtk_text_buffer_get_end_iter(buffer,&iter);
	off=gtk_text_iter_get_offset(&iter);

	if(g_strcmp0((char *)g_object_get_data(G_OBJECT(w),"from_message"),linphone_address_as_string(from))!=0){
		gtk_text_buffer_get_iter_at_offset(buffer,&iter,off);
		const char *display=linphone_address_get_display_name(from);
		if (display==NULL || display[0]=='\0') {
			display=linphone_address_get_username(from);
		}	
		gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,display,-1,"bold",NULL);
		gtk_text_buffer_get_end_iter(buffer,&iter);
		gtk_text_buffer_insert(buffer,&iter,":",-1);
		gtk_text_buffer_get_end_iter(buffer,&iter);
		gtk_text_buffer_insert(buffer,&iter,"\n",-1);
		g_object_set_data(G_OBJECT(w),"from_message",linphone_address_as_string(from));
	}
	gtk_text_buffer_get_end_iter(buffer,&iter);
	gtk_text_buffer_get_iter_at_offset(buffer,&begin,off);
	if(me){
		l=g_list_append(l,GINT_TO_POINTER(gtk_text_iter_get_offset(&iter)));
		g_object_set_data(G_OBJECT(w),"list",l);
	}
	gtk_text_buffer_insert(buffer,&iter,"\t",-1);
	gtk_text_buffer_insert(buffer,&iter,message,-1);
	gtk_text_buffer_get_end_iter(buffer,&iter);
	gtk_text_buffer_insert(buffer,&iter,"\n",-1);

	GtkTextMark *mark=gtk_text_buffer_create_mark(buffer,NULL,&iter,FALSE);
	gtk_text_view_scroll_mark_onscreen(text,mark);
	//gtk_text_buffer_get_end_iter(b,&iter);
	//gtk_text_iter_forward_to_line_end(&iter);
	//gtk_text_view_scroll_to_iter(v,&iter,0,TRUE,1.0,1.0);
    gtk_text_buffer_get_bounds (buffer, &begin, &iter);

    GHashTable *hash=(GHashTable *)g_object_get_data(G_OBJECT(linphone_gtk_get_main_window()),"history");
    if(me){
        g_hash_table_insert(hash,linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(cr)),
                            (gpointer)gtk_text_buffer_get_text(buffer,&begin,&iter,FALSE));
    } else {

        g_hash_table_insert(hash,linphone_address_as_string_uri_only(from),
                            (gpointer)gtk_text_buffer_get_text(buffer,&begin,&iter,FALSE));
    }
    g_object_set_data(G_OBJECT(linphone_gtk_get_main_window()),"history",hash);
}

const LinphoneAddress* linphone_gtk_get_used_identity(){
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneProxyConfig *cfg;
	linphone_core_get_default_proxy(lc,&cfg);
	if (cfg) return linphone_address_new(linphone_proxy_config_get_identity(cfg));
	else  return linphone_core_get_primary_contact_parsed(lc);
}


/* function in dev for displaying ack*/
void update_chat_state_message(LinphoneChatMessageState state){
    /*GdkPixbuf *pixbuf;

	switch (state) {
		case LinphoneChatMessageStateInProgress:
			pixbuf=create_pixbuf("chat_message_in_progress.png");
			break;
		case LinphoneChatMessageStateDelivered:
			pixbuf=create_pixbuf("chat_message_delivered.png");
			break;
		case  LinphoneChatMessageStateNotDelivered:
			pixbuf=create_pixbuf("chat_message_not_delivered.png");
			break;
		default : pixbuf=NULL;
	}

    GtkWidget *main_window=linphone_gtk_get_main_window();
    GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
	GtkWidget *page=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
	if(page!=NULL){
		GtkTextView *text=GTK_TEXT_VIEW(linphone_gtk_get_widget(page,"textview"));
		GtkTextBuffer *b=gtk_text_view_get_buffer(text);
		GtkTextIter iter;
		GList *l = g_object_get_data(G_OBJECT(page),"list");
		gtk_text_buffer_get_end_iter(b,&iter);
		gtk_text_buffer_get_iter_at_offset(b,&iter,GPOINTER_TO_INT(g_list_nth_data(l,0)));
		fprintf(stdout,"offset check %i \n",GPOINTER_TO_INT(g_list_nth_data(l,0)));
		l=g_list_remove(l,g_list_nth_data(l,0));
		gtk_text_buffer_insert_pixbuf(b,&iter,pixbuf);

		//gtk_text_buffer_get_end_iter(b,&iter);
		//gtk_text_buffer_insert_pixbuf(b,&iter,pixbuf);
		//gtk_text_buffer_get_end_iter(b,&iter);
		//gtk_text_buffer_insert(b,&iter,"\n",-1);
		g_object_set_data(G_OBJECT(page),"list",l);
	} else {
		fprintf(stdout,"NULLLL\n");
	}*/
}

static void on_chat_state_changed(LinphoneChatMessage *msg, LinphoneChatMessageState state, void *user_pointer){
	g_message("chat message state is %s",linphone_chat_message_state_to_string(state));
    update_chat_state_message(state);
}

void linphone_gtk_send_text(LinphoneChatRoom *cr){
    GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
    GtkWidget *w=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
	GtkWidget *entry=linphone_gtk_get_widget(w,"text_entry");
	const gchar *entered;
	
	entered=gtk_entry_get_text(GTK_ENTRY(entry));
	if (strlen(entered)>0) {
		LinphoneChatMessage *msg;
		linphone_gtk_push_text(w,
				linphone_gtk_get_used_identity(),
				entered,TRUE,g_object_get_data(G_OBJECT(w),"cr"));
        msg=linphone_chat_room_create_message(cr,entered);
		linphone_chat_room_send_message2(cr,msg,on_chat_state_changed,NULL);
		gtk_entry_set_text(GTK_ENTRY(entry),"");

	}
}

GtkWidget* linphone_gtk_init_chatroom(LinphoneChatRoom *cr, const LinphoneAddress *with){
	GtkWidget *chat_view=linphone_gtk_create_widget("main","chatroom_frame");
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	GHashTable *hash=g_object_get_data(G_OBJECT(main_window),"history");
	GtkNotebook *notebook=(GtkNotebook *)linphone_gtk_get_widget(main_window,"viewswitch");
	GtkWidget *text=linphone_gtk_get_widget(chat_view,"textview");
	int idx;

	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW(text),GTK_WRAP_WORD);
	gtk_text_view_set_editable (GTK_TEXT_VIEW(text),FALSE);
	gtk_notebook_append_page (notebook,chat_view,create_tab_chat_header(cr,with));
	idx = gtk_notebook_page_num(notebook, chat_view);
	gtk_notebook_set_current_page(notebook, idx);
	gtk_widget_show(chat_view);

	GList *l = NULL;
	g_object_set_data(G_OBJECT(chat_view),"cr",cr);
	g_object_set_data(G_OBJECT(chat_view),"idx",GINT_TO_POINTER(idx));
	g_object_set_data(G_OBJECT(chat_view),"from_message",NULL);
	g_object_set_data(G_OBJECT(chat_view),"from_chatroom",linphone_address_as_string_uri_only(with));
	g_object_set_data(G_OBJECT(chat_view),"list",l);

	gchar *buf=g_hash_table_lookup(hash,linphone_address_as_string_uri_only(with));
	if(buf != NULL){
        GtkTextIter start;
        GtkTextIter end;
        
        GtkTextBuffer *text_buffer;
        text_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
        gtk_text_buffer_get_bounds(text_buffer, &start, &end);
        g_object_set_data(G_OBJECT(chat_view),"cr",cr);
        gtk_text_buffer_delete (text_buffer, &start, &end);
        gtk_text_buffer_insert(text_buffer,&start,buf,-1);
	}

	gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
	                                  "right","justification", GTK_JUSTIFY_RIGHT,NULL);

	gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
	                                  "bold","weight", PANGO_WEIGHT_BOLD,NULL);
	
	GtkWidget *button = linphone_gtk_get_widget(chat_view,"send");
	g_signal_connect_swapped(G_OBJECT(button),"clicked",(GCallback)linphone_gtk_send_text,cr);

	GtkWidget *entry = linphone_gtk_get_widget(chat_view,"text_entry");
	g_signal_connect_swapped(G_OBJECT(entry),"activate",(GCallback)linphone_gtk_send_text,cr);

	return chat_view;
}

LinphoneChatRoom * linphone_gtk_create_chatroom(const LinphoneAddress *with){
	LinphoneChatRoom *cr=linphone_core_create_chat_room(linphone_gtk_get_core(),linphone_address_as_string(with));
	if (!cr) return NULL;
	return cr;
}



void linphone_gtk_load_chatroom(LinphoneChatRoom *cr,const LinphoneAddress *uri,GtkWidget *chat_view){
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	GHashTable *hash=g_object_get_data(G_OBJECT(main_window),"history");
    if(g_strcmp0((char *)g_object_get_data(G_OBJECT(chat_view),"from_chatroom"),
								linphone_address_as_string_uri_only(uri))!=0)
    {
        GtkTextView *text_view=GTK_TEXT_VIEW(linphone_gtk_get_widget(chat_view,"textview"));
        GtkTextIter start;
        GtkTextIter end;
        gchar *buf=g_hash_table_lookup(hash,linphone_address_as_string_uri_only(uri));
        GtkTextBuffer *text_buffer;
        text_buffer=gtk_text_view_get_buffer(text_view);
        gtk_text_buffer_get_bounds(text_buffer, &start, &end);
        g_object_set_data(G_OBJECT(chat_view),"cr",cr);
        gtk_text_buffer_delete (text_buffer, &start, &end);
        if(buf!=NULL){
            gtk_text_buffer_insert(text_buffer,&start,buf,-1);
        } else {
                g_object_set_data(G_OBJECT(chat_view),"from_message",NULL);
                GtkWidget *entry = linphone_gtk_get_widget(chat_view,"text_entry");
                g_signal_connect_swapped(G_OBJECT(entry),"activate",(GCallback)linphone_gtk_send_text,cr);

                GtkWidget *button = linphone_gtk_get_widget(chat_view,"send");
                g_signal_connect_swapped(G_OBJECT(button),"clicked",(GCallback)linphone_gtk_send_text,cr);
            }

        g_object_set_data(G_OBJECT(chat_view),"from_chatroom",linphone_address_as_string_uri_only(uri));
	}
}

void linphone_gtk_chat_destroyed(GtkWidget *w){
	LinphoneChatRoom *cr=(LinphoneChatRoom*)g_object_get_data(G_OBJECT(w),"cr");
	linphone_chat_room_destroy(cr);
}

void linphone_gtk_chat_close(GtkWidget *button){
	GtkWidget *w=gtk_widget_get_toplevel(button);
	gtk_widget_destroy(w);
}


void linphone_gtk_text_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from, const char *message){
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
    GtkWidget *w;

    w=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
    if(w!=NULL){
        linphone_gtk_load_chatroom(room,from,w);
    } else {
        w=linphone_gtk_init_chatroom(room,from);
        g_object_set_data(G_OBJECT(friendlist),"chatview",(gpointer)w);
    }
	

	#ifdef HAVE_GTK_OSX
	/* Notified when a new message is sent */
	linphone_gtk_status_icon_set_blinking(TRUE);
	#else
	if(!gtk_window_is_active(GTK_WINDOW(main_window))){
		if(!GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"is_notified"))){
			linphone_gtk_notify(NULL,message);
			g_object_set_data(G_OBJECT(w),"is_notified",GINT_TO_POINTER(TRUE));
		} else {
			g_object_set_data(G_OBJECT(w),"is_notified",GINT_TO_POINTER(FALSE));
		}
	}
	#endif
	linphone_gtk_push_text(w,from,message,FALSE,room);
	//linphone_gtk_update_chat_picture(TRUE);
	//gtk_window_present(GTK_WINDOW(w));
	/*gtk_window_set_urgency_hint(GTK_WINDOW(w),TRUE);*/
}

