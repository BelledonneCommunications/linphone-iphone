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

GtkWidget * linphone_gtk_init_chatroom(LinphoneChatRoom *cr, const char *with){
	GtkWidget *w;
	GtkTextBuffer *b;
	gchar *tmp;
	w=linphone_gtk_create_window("chatroom");
	tmp=g_strdup_printf(_("Chat with %s"),with);
	gtk_window_set_title(GTK_WINDOW(w),tmp);
	g_free(tmp);
	g_object_set_data(G_OBJECT(w),"cr",cr);
	gtk_widget_show(w);
	linphone_chat_room_set_user_data(cr,w);
	b=gtk_text_view_get_buffer(GTK_TEXT_VIEW(linphone_gtk_get_widget(w,"textlog")));
	gtk_text_buffer_create_tag(b,"blue","foreground","blue",NULL);
	gtk_text_buffer_create_tag(b,"green","foreground","green",NULL);
	return w;
}

void linphone_gtk_create_chatroom(const char *with){
	LinphoneChatRoom *cr=linphone_core_create_chat_room(linphone_gtk_get_core(),with);
	if (!cr) return;
	linphone_gtk_init_chatroom(cr,with);
}

void linphone_gtk_chat_destroyed(GtkWidget *w){
	LinphoneChatRoom *cr=(LinphoneChatRoom*)g_object_get_data(G_OBJECT(w),"cr");
	linphone_chat_room_destroy(cr);
}

void linphone_gtk_chat_close(GtkWidget *button){
	GtkWidget *w=gtk_widget_get_toplevel(button);
	gtk_widget_destroy(w);
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
	gtk_text_buffer_apply_tag_by_name(b,me ? "green" : "blue" ,&begin,&iter);
	gtk_text_buffer_insert(b,&iter,message,-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_buffer_insert(b,&iter,"\n",-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_view_scroll_to_iter(v,&iter,0,FALSE,0,0);
}

const char* linphone_gtk_get_used_identity(){
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneProxyConfig *cfg;
	linphone_core_get_default_proxy(lc,&cfg);
	if (cfg) return linphone_proxy_config_get_identity(cfg);
	else return linphone_core_get_primary_contact(lc);
}

void linphone_gtk_send_text(GtkWidget *button){
	GtkWidget *w=gtk_widget_get_toplevel(button);
	GtkWidget *entry=linphone_gtk_get_widget(w,"text_entry");
	LinphoneChatRoom *cr=(LinphoneChatRoom*)g_object_get_data(G_OBJECT(w),"cr");
	const gchar *entered;
	entered=gtk_entry_get_text(GTK_ENTRY(entry));
	if (strlen(entered)>0) {
		linphone_gtk_push_text(GTK_TEXT_VIEW(linphone_gtk_get_widget(w,"textlog")),
				linphone_gtk_get_used_identity(),
				entered,TRUE);
		linphone_chat_room_send_message(cr,entered);
		gtk_entry_set_text(GTK_ENTRY(entry),"");
	}
}

void linphone_gtk_text_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from, const char *message){
	GtkWidget *w=(GtkWidget*)linphone_chat_room_get_user_data(room);
	if (w==NULL){
		w=linphone_gtk_init_chatroom(room,linphone_address_as_string_uri_only(from));
	}
	linphone_gtk_push_text(GTK_TEXT_VIEW(linphone_gtk_get_widget(w,"textlog")),
				linphone_address_as_string_uri_only(from),
				message,FALSE);
	gtk_window_present(GTK_WINDOW(w));
	/*gtk_window_set_urgency_hint(GTK_WINDOW(w),TRUE);*/
}

