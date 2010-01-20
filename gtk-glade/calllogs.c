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
	GtkTextView *v=GTK_TEXT_VIEW(linphone_gtk_get_widget(w,"logtextview"));
	GtkTextBuffer *b=gtk_text_view_get_buffer(v);
	GtkTextIter iter,begin;
	int off;
	char *logmsg;
	const MSList *logs;
	for (logs=linphone_core_get_call_logs(linphone_gtk_get_core());logs!=NULL;logs=logs->next){
		LinphoneCallLog *cl=(LinphoneCallLog*)logs->data;
		logmsg=linphone_call_log_to_str(cl);
		gtk_text_buffer_get_end_iter(b,&iter);
		off=gtk_text_iter_get_offset(&iter);
		gtk_text_buffer_insert(b,&iter,logmsg,-1);
		gtk_text_buffer_get_end_iter(b,&iter);
		gtk_text_buffer_insert(b,&iter,"\n",-1);
		gtk_text_buffer_get_end_iter(b,&iter);
		gtk_text_buffer_get_iter_at_offset(b,&begin,off);
		gtk_text_buffer_apply_tag_by_name(b,cl->dir==LinphoneCallOutgoing ? "green" : "blue" ,&begin,&iter);
		ms_free(logmsg);	
	}
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_view_scroll_to_iter(v,&iter,0,FALSE,0,0);
}

void linphone_gtk_call_log_response(GtkWidget *w){
	GtkWidget *mw=linphone_gtk_get_main_window();
	g_object_set_data(G_OBJECT(mw),"call_logs",NULL);
	gtk_widget_destroy(w);
}

GtkWidget * linphone_gtk_show_call_logs(void){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkTextBuffer *b;
	GtkWidget *w=(GtkWidget*)g_object_get_data(G_OBJECT(linphone_gtk_get_main_window()),"call_logs");
	if (w==NULL){
		w=linphone_gtk_create_window("call_logs");
		g_object_set_data(G_OBJECT(mw),"call_logs",w);
		g_signal_connect(G_OBJECT(w),"response",(GCallback)linphone_gtk_call_log_response,NULL);
		gtk_widget_show(w);
		b=gtk_text_view_get_buffer(GTK_TEXT_VIEW(linphone_gtk_get_widget(w,"logtextview")));
		gtk_text_buffer_create_tag(b,"blue","foreground","blue",NULL);
		gtk_text_buffer_create_tag(b,"green","foreground","green",NULL);
		linphone_gtk_call_log_update(w);
	}else gtk_window_present(GTK_WINDOW(w));
	return w;
}

