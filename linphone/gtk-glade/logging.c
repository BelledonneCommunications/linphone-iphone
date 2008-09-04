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

static GtkWidget *log_window=NULL;
static GStaticMutex log_mutex=G_STATIC_MUTEX_INIT;
static GList *log_queue=NULL;

typedef struct _LinphoneGtkLog{
	OrtpLogLevel lev;
	gchar *msg;
}LinphoneGtkLog;

void linphone_gtk_create_log_window(void){
	GtkTextBuffer *b;
	log_window=linphone_gtk_create_window("log");
	b=gtk_text_view_get_buffer(GTK_TEXT_VIEW(linphone_gtk_get_widget(log_window,"textview")));
	gtk_text_buffer_create_tag(b,"red","foreground","red",NULL);
	gtk_text_buffer_create_tag(b,"orange","foreground","orange",NULL);
}

void linphone_gtk_destroy_log_window(void){
	GtkWidget *w=log_window;
	g_static_mutex_lock(&log_mutex);
	log_window=NULL;
	gtk_widget_destroy(w);
	g_static_mutex_unlock(&log_mutex);
}

void linphone_gtk_log_show(void){
	gtk_widget_show(log_window);
	gtk_window_present(GTK_WINDOW(log_window));
}

static void linphone_gtk_display_log(OrtpLogLevel lev, const char *msg){
	GtkTextIter iter,begin;
	int off;
	static GtkTextView *v=NULL;
	GtkTextBuffer *b;
	const char *lname="undef";

	if (log_window==NULL) {
		return;
	}

	if (v==NULL) v=GTK_TEXT_VIEW(linphone_gtk_get_widget(log_window,"textview"));
	b=gtk_text_view_get_buffer(v);
	switch(lev){
		case ORTP_DEBUG:
			lname="debug";
			break;
		case ORTP_MESSAGE:
			lname="message";
			break;
		case ORTP_WARNING:
			lname="warning";
			break;
		case ORTP_ERROR:
			lname="error";
			break;
		case ORTP_FATAL:
			lname="fatal";
			break;
		default:
			g_error("Bad level !");
	}
	gtk_text_buffer_get_end_iter(b,&iter);
	off=gtk_text_iter_get_offset(&iter);
	gtk_text_buffer_insert(b,&iter,lname,-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_buffer_insert(b,&iter,": ",-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_buffer_insert(b,&iter,msg,-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_buffer_insert(b,&iter,"\n",-1);
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_buffer_get_iter_at_offset(b,&begin,off);
	if (lev==ORTP_ERROR || lev==ORTP_FATAL) gtk_text_buffer_apply_tag_by_name(b,"red",&begin,&iter);
	else if (lev==ORTP_WARNING) gtk_text_buffer_apply_tag_by_name(b,"orange",&begin,&iter);
	gtk_text_buffer_get_end_iter(b,&iter);
	//gtk_text_view_scroll_to_iter(v,&iter,0,FALSE,0,0);
}

gboolean linphone_gtk_check_logs(){
	GList *elem;
	g_static_mutex_lock(&log_mutex);
	for(elem=log_queue;elem!=NULL;elem=elem->next){
		LinphoneGtkLog *lgl=(LinphoneGtkLog*)elem->data;
		linphone_gtk_display_log(lgl->lev,lgl->msg);
		g_free(lgl->msg);
		g_free(lgl);
	}
	if (log_queue) g_list_free(log_queue);
	log_queue=NULL;
	g_static_mutex_unlock(&log_mutex);
	return TRUE;
}

void linphone_gtk_log_push(OrtpLogLevel lev, const char *fmt, va_list args){
	gchar *msg=g_strdup_vprintf(fmt,args);
	LinphoneGtkLog *lgl=g_new(LinphoneGtkLog,1);
	lgl->lev=lev;
	lgl->msg=msg;
	g_static_mutex_lock(&log_mutex);
	log_queue=g_list_append(log_queue,lgl);
	g_static_mutex_unlock(&log_mutex);
}

