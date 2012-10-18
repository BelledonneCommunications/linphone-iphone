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
	GtkWidget *w=linphone_gtk_get_widget(main_window,"chatroom_frame");
	GtkWidget *nb=linphone_gtk_get_widget(main_window,"viewswitch");
	int idx;
	g_return_if_fail(w!=NULL);
	idx=gtk_notebook_page_num(GTK_NOTEBOOK(nb),w);
	gtk_notebook_remove_page (GTK_NOTEBOOK(nb),idx);
	linphone_chat_room_set_user_data(cr,NULL);
	gtk_widget_destroy(w);
}

GtkWidget *create_tab_chat_header(LinphoneChatRoom *cr){
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

void linphone_gtk_push_text(GtkTextView *v, const char *from, const char *message, gboolean me){
	GtkTextBuffer *b=gtk_text_view_get_buffer(v);
	GtkTextIter iter,begin;
	int off;
	gtk_text_buffer_get_end_iter(b,&iter);
	off=gtk_text_iter_get_offset(&iter);
	gtk_text_buffer_insert(b,&iter,from,-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_buffer_insert(b,&iter,":\t",-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_buffer_get_iter_at_offset(b,&begin,off);
	//gtk_text_buffer_apply_tag_by_name(b,me ? "green" : "blue" ,&begin,&iter);
	gtk_text_buffer_insert(b,&iter,message,-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_buffer_insert(b,&iter,"\n",-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	
	GtkTextMark *mark=gtk_text_buffer_create_mark(b,NULL,&iter,FALSE);
	gtk_text_view_scroll_mark_onscreen(v,mark);
	//gtk_text_buffer_get_end_iter(b,&iter);
	//gtk_text_iter_forward_to_line_end(&iter);
	//gtk_text_view_scroll_to_iter(v,&iter,0,TRUE,1.0,1.0);
}

/*void linphone_gtk_push_text_start(GtkTextView *v, const char *from, const char *message){
	GtkTextBuffer *b=gtk_text_view_get_buffer(v);
	GtkTextIter iter,begin;
	int off;
	gtk_text_buffer_get_end_iter(b,&iter);
	off=gtk_text_iter_get_offset(&iter);
	gtk_text_buffer_insert(b,&iter,from,-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_buffer_insert(b,&iter,":\t",-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_buffer_get_iter_at_offset(b,&begin,off);
	//gtk_text_buffer_apply_tag_by_name(b,me ? "green" : "blue" ,&begin,&iter);
	gtk_text_buffer_insert(b,&iter,message,-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_buffer_insert(b,&iter,"\n",-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	
	//GtkTextMark *mark=gtk_text_buffer_create_mark(b,NULL,&iter,FALSE);
	//gtk_text_view_scroll_mark_onscreen(v,mark);
	//gtk_text_buffer_get_end_iter(b,&iter);
	//gtk_text_iter_forward_to_line_end(&iter);
	//gtk_text_view_scroll_to_iter(v,&iter,0,TRUE,1.0,1.0);
}*/

const char* linphone_gtk_get_used_identity(){
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneProxyConfig *cfg;
	linphone_core_get_default_proxy(lc,&cfg);
	const char* display;
	const LinphoneAddress* u;
	if (cfg) {
		u = linphone_address_new(linphone_proxy_config_get_identity(cfg));
	} else {
		u = linphone_core_get_primary_contact_parsed(lc);
	}
	display=linphone_address_get_display_name(u);
	if (display==NULL || display[0]=='\0') {
			display=linphone_address_as_string(u);
	}
	return display;
}

static void on_chat_state_changed(LinphoneChatMessage *msg, LinphoneChatMessageState state, void *user_pointer){
	g_message("chat message state is %s",linphone_chat_message_state_to_string(state));
}

void linphone_gtk_send_text(LinphoneChatRoom *cr){
	GtkWidget *chat_view=(GtkWidget*)linphone_chat_room_get_user_data(cr);
	GtkWidget *entry= linphone_gtk_get_widget(chat_view,"text_entry");
	const gchar *entered;
	entered=gtk_entry_get_text(GTK_ENTRY(entry));
	
	if (strlen(entered)>0) {
		LinphoneChatMessage *msg;
		linphone_gtk_push_text(GTK_TEXT_VIEW(linphone_gtk_get_widget(chat_view,"textview")),
				linphone_gtk_get_used_identity(),
				entered,TRUE);
		msg=linphone_chat_room_create_message(cr,entered);
		linphone_chat_room_send_message2(cr,msg,on_chat_state_changed,NULL);
		gtk_entry_set_text(GTK_ENTRY(entry),"");
	}
}


void linphone_gtk_init_chatroom(LinphoneChatRoom *cr, const char *with){
	GtkWidget *chat_view=linphone_gtk_create_widget("main","chatroom_frame");
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	GtkNotebook *notebook=(GtkNotebook *)linphone_gtk_get_widget(main_window,"viewswitch");
	GtkTextBuffer *b;
	int idx;

	b=gtk_text_view_get_buffer(GTK_TEXT_VIEW(linphone_gtk_get_widget(chat_view,"textview")));
	gtk_text_buffer_create_tag(b,"blue","foreground","blue",NULL);
	gtk_text_buffer_create_tag(b,"green","foreground","green",NULL);
	gtk_notebook_append_page (notebook,chat_view,create_tab_chat_header(cr));
	idx = gtk_notebook_page_num(notebook, chat_view);
	gtk_notebook_set_current_page(notebook, idx);
	gtk_widget_show(chat_view);
	g_object_set_data(G_OBJECT(chat_view),"cr",cr);
	linphone_chat_room_set_user_data(cr,chat_view);
	
	//linphone_gtk_get_used_identity()
	
	GtkWidget *text=linphone_gtk_get_widget(chat_view,"textview");
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW(text),GTK_WRAP_WORD);
	//linphone_gtk_push_text(GTK_TEXT_VIEW(linphone_gtk_get_widget(chat_view,"textview")),NULL,
	//						"Conversation avec moi",FALSE);
	
	//linphone_gtk_push_text(GTK_TEXT_VIEW(linphone_gtk_get_widget(chat_view,"textview")),with,
	//						"Conversation avec ",FALSE);
	
	//Initialisation des signaux
	GtkWidget *button = linphone_gtk_get_widget(chat_view,"send");
	g_signal_connect_swapped(G_OBJECT(button),"clicked",(GCallback)linphone_gtk_send_text,cr);
	
	GtkWidget *entry = linphone_gtk_get_widget(chat_view,"text_entry");
	g_signal_connect_swapped(G_OBJECT(entry),"activate",(GCallback)linphone_gtk_send_text,cr);
}


LinphoneChatRoom * linphone_gtk_create_chatroom(const char *with){
	LinphoneChatRoom *cr=linphone_core_create_chat_room(linphone_gtk_get_core(),with);
	if (!cr) return NULL;
	linphone_gtk_init_chatroom(cr,with);
	return cr;
}

void linphone_gtk_load_chatroom(LinphoneChatRoom *cr,const char* uri){
	GtkWidget *w=(GtkWidget*)linphone_chat_room_get_user_data(cr);
	if(w==NULL){
		linphone_gtk_init_chatroom(cr, uri);
	} else {
		//TODO
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
	GtkWidget *w=(GtkWidget*)linphone_chat_room_get_user_data(room);	
	if (w==NULL){		
		linphone_gtk_init_chatroom(room,linphone_address_as_string_uri_only(from));
		w=(GtkWidget*)linphone_chat_room_get_user_data(room);
	}

	#ifdef HAVE_GTK_OSX
	/* Notified when a new message is sent */
	linphone_gtk_status_icon_set_blinking(TRUE);
	#else 
	if(!GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"is_notified"))){
		linphone_gtk_notify(NULL,message);
		g_object_set_data(G_OBJECT(w),"is_notified",GINT_TO_POINTER(TRUE));
	} else {
		g_object_set_data(G_OBJECT(w),"is_notified",GINT_TO_POINTER(FALSE));
	}
	#endif
	
	
	const char* display = linphone_address_get_display_name(from);
	if(display==NULL){
		display = linphone_address_as_string(from);
	}
	linphone_gtk_push_text(GTK_TEXT_VIEW(linphone_gtk_get_widget(w,"textview")),
				display,
				message,FALSE);
	//gtk_window_present(GTK_WINDOW(w));
	/*gtk_window_set_urgency_hint(GTK_WINDOW(w),TRUE);*/
}

