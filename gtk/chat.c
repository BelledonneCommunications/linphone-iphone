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
	linphone_gtk_create_chat_picture(FALSE);
	g_object_set_data(G_OBJECT(friendlist),"chatview",NULL);
	g_object_set_data(G_OBJECT(w),"from_message",NULL);
	g_object_set_data(G_OBJECT(w),"cr",NULL);
	gtk_widget_destroy(w);
}

GtkWidget *create_tab_chat_header(LinphoneChatRoom *cr,const LinphoneAddress *uri){
	GtkWidget *w=gtk_hbox_new (FALSE,0);
	GtkWidget *i=create_pixmap ("chat.png");
	GtkWidget *l;
	GtkWidget *image=gtk_image_new_from_stock(GTK_STOCK_CLOSE,GTK_ICON_SIZE_MENU);
	GtkWidget *b=gtk_button_new();
	
	gtk_button_set_image(GTK_BUTTON(b),image);
	gtk_button_set_relief(GTK_BUTTON(b),GTK_RELIEF_NONE);
	gtk_widget_set_size_request(b,25,20);
	g_signal_connect_swapped(G_OBJECT(b),"clicked",G_CALLBACK(linphone_gtk_quit_chatroom),cr);

	const char *display=linphone_address_get_display_name(uri);
	if (display==NULL || display[0]=='\0') {
		display=linphone_address_get_username(uri);
	}
	l=gtk_label_new (display);
	gtk_box_pack_start (GTK_BOX(w),i,FALSE,FALSE,0);
	gtk_box_pack_start (GTK_BOX(w),l,FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(w),b,TRUE,TRUE,0);
	gtk_widget_show_all(w);
	return w;
}

void udpate_tab_chat_header(GtkWidget *chat_view,const LinphoneAddress *uri,LinphoneChatRoom *cr){
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkNotebook *notebook=GTK_NOTEBOOK(linphone_gtk_get_widget(main_window,"viewswitch"));
	GtkWidget *w=gtk_hbox_new (FALSE,0);
	GtkWidget *i=create_pixmap ("chat.png");
	GtkWidget *l;
	GtkWidget *image=gtk_image_new_from_stock(GTK_STOCK_CLOSE,GTK_ICON_SIZE_MENU);
	GtkWidget *b=gtk_button_new();

	gtk_button_set_image(GTK_BUTTON(b),image);
	gtk_button_set_relief(GTK_BUTTON(b),GTK_RELIEF_NONE);
	gtk_widget_set_size_request(b,25,20);
	g_signal_connect_swapped(G_OBJECT(b),"clicked",G_CALLBACK(linphone_gtk_quit_chatroom),cr);

	const char *display=linphone_address_get_display_name(uri);
	if (display==NULL || display[0]=='\0') {
		display=linphone_address_get_username(uri);
	}
	l=gtk_label_new (display);
	gtk_box_pack_start (GTK_BOX(w),i,FALSE,FALSE,0);
	gtk_box_pack_start (GTK_BOX(w),l,FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(w),b,TRUE,TRUE,0);

	gtk_notebook_set_tab_label(notebook,chat_view,w);
	gtk_widget_show_all(w);

}

void linphone_gtk_push_text(GtkWidget *w, const LinphoneAddress *from, 
                    const char *message, gboolean me,LinphoneChatRoom *cr, time_t t){
    GtkTextView *text=GTK_TEXT_VIEW(linphone_gtk_get_widget(w,"textview"));
    GtkTextBuffer *buffer=gtk_text_view_get_buffer(text);
    GtkTextIter iter,begin,end;
    gtk_text_buffer_get_start_iter(buffer,&begin);
	int off;
	gtk_text_buffer_get_end_iter(buffer,&iter);
	off=gtk_text_iter_get_offset(&iter);
	GList *list=g_object_get_data(G_OBJECT(w),"list");

	if(g_strcmp0((char *)g_object_get_data(G_OBJECT(w),"from_message"),linphone_address_as_string(from))!=0){
		gtk_text_buffer_get_iter_at_offset(buffer,&iter,off);
		const char *display=linphone_address_get_display_name(from);
		if (display==NULL || display[0]=='\0') {
			display=linphone_address_get_username(from);
		}
		gtk_text_buffer_get_end_iter(buffer,&iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,display,-1,"bold",me ? "left" : "left",NULL);
		gtk_text_buffer_get_end_iter(buffer,&iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer,&iter," : ",-1,"bold",me ? "left" : "left",NULL);
		gtk_text_buffer_get_end_iter(buffer,&iter);
		gtk_text_buffer_insert(buffer,&iter,"\n",-1);
		g_object_set_data(G_OBJECT(w),"from_message",linphone_address_as_string(from));
	}
	gtk_text_buffer_get_end_iter(buffer,&iter);
	gtk_text_buffer_get_iter_at_offset(buffer,&begin,off);
	gtk_text_buffer_get_end_iter(buffer,&iter);				
	gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,message,-1,me ? "left" : "left",NULL);
	gtk_text_buffer_get_end_iter(buffer,&iter);	
	gtk_text_buffer_insert(buffer,&iter,"\n",-1);
	gtk_text_buffer_get_bounds (buffer, &begin, &end);	
    GHashTable *hash=(GHashTable *)g_object_get_data(G_OBJECT(linphone_gtk_get_main_window()),"history");
    if(me){
        g_hash_table_insert(hash,linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(cr)),
                            (gpointer)gtk_text_buffer_get_text(buffer,&begin,&end,FALSE));
    } else {
        g_hash_table_insert(hash,linphone_address_as_string_uri_only(from),
                            (gpointer)gtk_text_buffer_get_text(buffer,&begin,&end,FALSE));
    }
	g_object_set_data(G_OBJECT(linphone_gtk_get_main_window()),"history",hash);
	

	gtk_text_buffer_get_end_iter(buffer,&iter);
	if(me){
		list=g_list_append(list,GINT_TO_POINTER(gtk_text_iter_get_line(&iter)));
		gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,"Message in progress.. ",-1,									
		                                         "italic","right","small","font_grey",NULL);
		g_object_set_data(G_OBJECT(w),"list",list);
	} else {
		struct tm *tm=localtime(&t);
		char buf[80];
		strftime(buf,80,"Received at %H:%M",tm);
		gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,buf,-1,									
	                       "italic","right","small","font_grey",NULL);
	}
	gtk_text_buffer_get_end_iter(buffer,&iter);
	gtk_text_buffer_insert(buffer,&iter,"\n",-1);
	GtkTextMark *mark=gtk_text_buffer_create_mark(buffer,NULL,&iter,FALSE);
	gtk_text_view_scroll_mark_onscreen(text,mark);
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
    GtkWidget *main_window=linphone_gtk_get_main_window();
    GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
	GtkWidget *page=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
	GList *list=g_object_get_data(G_OBJECT(page),"list");
	
	if(page!=NULL){
		GtkTextView *text=GTK_TEXT_VIEW(linphone_gtk_get_widget(page,"textview"));
		GtkTextBuffer *b=gtk_text_view_get_buffer(text);
		GtkTextIter iter;
		GtkTextIter end;
		GtkTextIter start;

		gtk_text_buffer_get_iter_at_line(b,&iter,
		                           GPOINTER_TO_INT(g_list_nth_data(list,0)));
		if(gtk_text_iter_get_chars_in_line(&iter) >0) {
			gtk_text_buffer_get_iter_at_line_offset(b,&start,
			    				GPOINTER_TO_INT(g_list_nth_data(list,0)),
			                    gtk_text_iter_get_chars_in_line(&iter)-1);
		}else {
			gtk_text_buffer_get_iter_at_line_offset(b,&start,
			                        GPOINTER_TO_INT(g_list_nth_data(list,0)),0);
		}
		gtk_text_buffer_get_iter_at_line_offset(b,&end,
		                            GPOINTER_TO_INT(g_list_nth_data(list,0)),0);
		gtk_text_buffer_delete(b,&start,&end);
		gtk_text_buffer_get_iter_at_line(b,&iter,GPOINTER_TO_INT(g_list_nth_data(list,0)));
		gchar *result;
		switch (state) {
			case LinphoneChatMessageStateInProgress:
				result="Message in progress.. ";
				break;
			case LinphoneChatMessageStateDelivered:
				result="Message delivered ";
				break;
			case  LinphoneChatMessageStateNotDelivered:
				result="Message not delivered ";
				break;
			default : result="Message in progress.. ";
		}
		gtk_text_buffer_insert_with_tags_by_name(b,&iter,result,-1,
												"italic","right","small","font_grey",NULL);
		list=g_list_remove(list,g_list_nth_data(list,0));
		g_object_set_data(G_OBJECT(page),"list",list);
	} 
}

static void on_chat_state_changed(LinphoneChatMessage *msg, LinphoneChatMessageState state, void *user_pointer){
	g_message("chat message state is %s",linphone_chat_message_state_to_string(state));
	update_chat_state_message(state);
}

void linphone_gtk_send_text(){
    GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
    GtkWidget *w=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
	GtkWidget *entry=linphone_gtk_get_widget(w,"text_entry");
	const gchar *entered;
	LinphoneChatRoom *cr=g_object_get_data(G_OBJECT(w),"cr");
	entered=gtk_entry_get_text(GTK_ENTRY(entry));
	if (strlen(entered)>0) {
		LinphoneChatMessage *msg;
		msg=linphone_chat_room_create_message(cr,entered);
		linphone_gtk_push_text(w,
				linphone_gtk_get_used_identity(),
				entered,TRUE,cr,linphone_chat_message_get_time(msg));
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
	GdkColor color;
	int idx;
	
	color.red = 32512;
  	color.green = 32512;
  	color.blue = 32512;
	
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW(text),GTK_WRAP_WORD);
	gtk_text_view_set_editable (GTK_TEXT_VIEW(text),FALSE);
	gtk_notebook_append_page (notebook,chat_view,create_tab_chat_header(cr,with));
	idx = gtk_notebook_page_num(notebook, chat_view);
	gtk_notebook_set_current_page(notebook, idx);
	gtk_widget_show(chat_view);
	
	g_object_set_data(G_OBJECT(chat_view),"cr",cr);
	g_object_set_data(G_OBJECT(chat_view),"idx",GINT_TO_POINTER(idx));
	g_object_set_data(G_OBJECT(chat_view),"from_message",NULL);
	g_object_set_data(G_OBJECT(chat_view),"from_chatroom",(gpointer) with);

	GList *list=NULL;
	g_object_set_data(G_OBJECT(chat_view),"list",list);

	gchar *buf=g_hash_table_lookup(hash,linphone_address_as_string_uri_only(with));
	if(buf != NULL){
        GtkTextIter start;
        GtkTextIter end;
        
        GtkTextBuffer *text_buffer;
        text_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
        gtk_text_buffer_get_bounds(text_buffer, &start, &end);
        gtk_text_buffer_delete (text_buffer, &start, &end);
        gtk_text_buffer_insert(text_buffer,&start,buf,-1);
	}

	gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
	                        "right","justification", GTK_JUSTIFY_RIGHT,NULL);
	gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
	                        "left","justification", GTK_JUSTIFY_LEFT,NULL);
	gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
	                       	    "bold","weight", PANGO_WEIGHT_BOLD,NULL);
	gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
								"italic","style", PANGO_STYLE_ITALIC,NULL);
	gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
	                           	"small","size",9*PANGO_SCALE,NULL);
	gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
	                           	"font_grey","foreground-gdk",&color,NULL);
	
	GtkWidget *button = linphone_gtk_get_widget(chat_view,"send");
	g_signal_connect_swapped(G_OBJECT(button),"clicked",(GCallback)linphone_gtk_send_text,NULL);

	GtkWidget *entry = linphone_gtk_get_widget(chat_view,"text_entry");
	g_signal_connect_swapped(G_OBJECT(entry),"activate",(GCallback)linphone_gtk_send_text,NULL);

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
	LinphoneAddress *from=(LinphoneAddress *)g_object_get_data(G_OBJECT(chat_view),"from_chatroom");
    if(g_strcmp0(linphone_address_as_string(from),linphone_address_as_string(uri))!=0)
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
            gtk_text_buffer_insert_with_tags_by_name(text_buffer,&start,buf,-1,"font_grey",NULL);
			GtkTextMark *mark=gtk_text_buffer_create_mark(text_buffer, NULL, &start, FALSE);
			gtk_text_view_scroll_to_mark(text_view,mark, 0, FALSE, 0, 0);
        }

		udpate_tab_chat_header(chat_view,uri,cr);
		g_object_set_data(G_OBJECT(chat_view),"cr",cr);
        g_object_set_data(G_OBJECT(chat_view),"from_chatroom",(gpointer) uri);
		g_object_set_data(G_OBJECT(chat_view),"from_message",linphone_address_as_string_uri_only(uri));
		g_object_set_data(G_OBJECT(linphone_gtk_get_widget(main_window,"contact_list")),"chatview",(gpointer)chat_view);
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


void linphone_gtk_text_received(LinphoneCore *lc, LinphoneChatRoom *room, 
                   		LinphoneChatMessage *msg){
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
    GtkWidget *w;
							   

    w=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
    if(w!=NULL){
        linphone_gtk_load_chatroom(room,linphone_chat_message_get_from(msg),w);
    } else {
        w=linphone_gtk_init_chatroom(room,linphone_chat_message_get_from(msg));
        g_object_set_data(G_OBJECT(friendlist),"chatview",(gpointer)w);
		g_object_set_data(G_OBJECT(friendlist),"from",(gpointer)linphone_chat_message_get_from(msg));
    }

	const char *display=linphone_address_get_display_name(linphone_chat_message_get_from(msg));
		if (display==NULL || display[0]=='\0') {
			display=linphone_address_get_username(linphone_chat_message_get_from(msg));
	}
	
	#ifdef HAVE_GTK_OSXs
	/* Notified when a new message is sent */
	linphone_gtk_status_icon_set_blinking(TRUE);
	#else
	if(!gtk_window_is_active(GTK_WINDOW(main_window))){
		if(!GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"is_notified"))){
			linphone_gtk_notify(NULL,linphone_chat_message_get_text(msg));
			g_object_set_data(G_OBJECT(w),"is_notified",GINT_TO_POINTER(TRUE));
		} else {
			g_object_set_data(G_OBJECT(w),"is_notified",GINT_TO_POINTER(FALSE));
		}
	}
	#endif
	linphone_gtk_push_text(w,linphone_chat_message_get_from(msg),
	                       linphone_chat_message_get_text(msg),FALSE,room,linphone_chat_message_get_time(msg));
	linphone_gtk_update_chat_picture();
	//gtk_window_present(GTK_WINDOW(w));
	/*gtk_window_set_urgency_hint(GTK_WINDOW(w),TRUE);*/
}
