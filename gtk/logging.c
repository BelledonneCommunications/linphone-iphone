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

#ifndef WIN32
#include <sys/stat.h>
#include <sys/types.h>
#endif
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

extern gchar *linphone_logfile;

static GtkWidget *log_window=NULL;
static GStaticMutex log_mutex=G_STATIC_MUTEX_INIT;
static GList *log_queue=NULL;
static const char *dateformat="%Y%m%d-%H:%M:%S";

#define LOG_MAX_CHARS 1000000  /*1 mega bytes of traces*/

typedef struct _LinphoneGtkLog{
	OrtpLogLevel lev;
	gchar *msg;
}LinphoneGtkLog;


/******
 * Module to log to a file
 ******/

/* Marker to insert as a line at the start of every log file */
#define LOGFILE_MARKER_START  "----<start>----"
/* Marker to insert as a line at the end of every log file */
#define LOGFILE_MARKER_STOP  "----<end>----"
/* Number of files to keep in history, log file rotation will be
   performed. */
#define LOGFILE_ROTATION 4
/* Pointer to opened log file */
static FILE *_logfile = NULL;


/* Called on exit, print out the marker, close the file and avoid to
   continue logging. */
void linphone_gtk_log_uninit()
{
	if (_logfile != NULL) {
		fprintf(_logfile, "%s\n", LOGFILE_MARKER_STOP);
		fclose(_logfile);
		_logfile = NULL;
	}
}

/* Called when we start logging, find a good place for the log files,
   perform rotation, insert the start marker and return the pointer to
   the file that should be used for logging, or NULL on errors or if
   disabled. */
static FILE *linphone_gtk_log_init()
{
	static char _logdir[1024];
	static char _logfname[1024];
	static gboolean _log_init = FALSE;
	const char *dst_fname=NULL;

	if (!_log_init) {
		if (linphone_gtk_get_core()!=NULL){
			dst_fname = linphone_gtk_get_ui_config("logfile",NULL);
			dateformat=linphone_gtk_get_ui_config("logfile_date_format",dateformat);
		}
		/* For anything to happen, we need a logfile configuration variable,
		 this is our trigger */
		if (dst_fname) {
			/* arrange for _logdir to contain a
			 directory that has been created and _logfname to contain the
			 path to a file to which we will log */
#ifdef WIN32
			const char *appdata=getenv("LOCALAPPDATA");
			if (appdata) {
				snprintf(_logdir, sizeof(_logdir),"%s\\Linphone", appdata);
				mkdir(_logdir);
			} else {
				_logdir[0] = '\0';
			}
#define PATH_SEPARATOR '\\'
#else
			const char *home=getenv("HOME");
			if (home) {
				snprintf(_logdir, sizeof(_logdir),"%s/.linphone", home);
				mkdir(_logdir,S_IRUSR | S_IWUSR | S_IRGRP);
			} else {
				_logdir[0] = '\0';
			}
#define PATH_SEPARATOR '/'
#endif
			if (_logdir[0] != '\0') {
				/* We have a directory, fix the path to the log file in it and
				 open the file so that we will be appending to it. */
				snprintf(_logfname, sizeof(_logfname), "%s%c%s",_logdir, PATH_SEPARATOR, dst_fname);
			}
		}else if (linphone_logfile!=NULL){
			snprintf(_logfname,sizeof(_logfname),"%s",linphone_logfile);
		}
		
		if (_logfname[0]!='\0'){
			/* If the constant LOGFILE_ROTATION is greater than zero, then
			 we kick away a simple rotation that will ensure that there
			 are never more than LOGFILE_ROTATION+1 old copies of the
			 log file on the disk.  The oldest file is always rotated
			 "away" as expected.  Rotated files have the same name as
			 the main log file, though with a number 0..LOGFILE_ROTATION
			 at the end, where the greater the number is, the older the
			 file is. */
			if (ortp_file_exist(_logfname)==0 && LOGFILE_ROTATION > 0) {
				int i;
				char old_fname[1024];
				char new_fname[1024];

				/* Rotate away existing files.  We make sure to remove the
				 old files otherwise rename() would not work properly.  We
				 have to loop in reverse here. */
				for (i=LOGFILE_ROTATION-1;i>=0;i--) {
					snprintf(old_fname, sizeof(old_fname), "%s%c%s.%d",
						_logdir, PATH_SEPARATOR, dst_fname, i);
					snprintf(new_fname, sizeof(new_fname), "%s%c%s.%d",
						_logdir, PATH_SEPARATOR, dst_fname, i+1);
					if (ortp_file_exist(old_fname)==0) {
						if (ortp_file_exist(new_fname)==0)
							unlink(new_fname);
						rename(old_fname, new_fname);
					}
				}
				/* Move current log file as the first of the rotation.  Make
				 sure to remove the old .0 also, since otherwise rename()
				 would not work as expected. */
				snprintf(new_fname, sizeof(new_fname), "%s%c%s.%d",
					_logdir, PATH_SEPARATOR, dst_fname, 0);
				if (ortp_file_exist(new_fname)==0)
					unlink(new_fname);
				rename(_logfname, new_fname);
			}
			/* Start a new log file and mark that we have now initialised */
			_logfile = fopen(_logfname, "w");
			fprintf(_logfile, "%s\n", LOGFILE_MARKER_START);
		}
		_log_init = TRUE;
	}
	return _logfile;
}

static void linphone_gtk_log_file(OrtpLogLevel lev, const char *msg)
{
	time_t now;
	FILE *outlog;

	outlog = linphone_gtk_log_init();
	if (outlog != NULL) {
		/* We have an opened file and we have initialised properly, it's
		 time to write all these log messages. We convert the log level
		 from oRTP into something readable and timestamp each log
		 message.  The format of the time	stamp can be controlled by
		 logfile_date_format in the GtkUi section of the config file,
		 but it defaults to something compact, but yet readable. */
		const char *lname="undef";
		char date[256];

		/* Convert level constant to text */
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
				lname="undef";
				break;
		}
		/* Get current time and format it properly */
		now = time(NULL);
		strftime(date, sizeof(date), dateformat, localtime(&now));
		/* Now print out the message to the logfile.  We don't flush,
		 maybe we should do to ensure that we have all the messages in
		 case of a crash (which is one of the main reasons we have a
		     log facility in the first place). */
		fprintf(outlog, "[%s] [%s] %s\n", date, lname, msg);
		fflush(outlog);
	}
}

void linphone_gtk_log_hide(){
	if (log_window)
		gtk_widget_hide(log_window);
}

void linphone_gtk_create_log_window(void){
	GtkTextBuffer *b;
	log_window=linphone_gtk_create_window("log");
	b=gtk_text_view_get_buffer(GTK_TEXT_VIEW(linphone_gtk_get_widget(log_window,"textview")));
	gtk_text_buffer_create_tag(b,"red","foreground","red",NULL);
	gtk_text_buffer_create_tag(b,"orange","foreground","orange",NULL);
	/*prevent the log window from being destroyed*/
	g_signal_connect (G_OBJECT (log_window), "delete-event",
		G_CALLBACK (gtk_widget_hide_on_delete), log_window);

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

static void linphone_gtk_display_log(GtkTextView *v, OrtpLogLevel lev, const char *msg){
	GtkTextIter iter,begin;
	int off;
	GtkTextBuffer *b;
	const char *lname="undef";

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
	
	while(gtk_text_buffer_get_char_count(b)>LOG_MAX_CHARS){
		GtkTextIter iter_line_after;
		gtk_text_buffer_get_start_iter(b,&iter);
		iter_line_after=iter;
		if (gtk_text_iter_forward_line(&iter_line_after)){
			gtk_text_buffer_delete(b,&iter,&iter_line_after);
		}
	}
	
}

static void stick_to_end(GtkTextView *v){
	GtkTextBuffer *b;
	GtkTextIter iter;
	b=gtk_text_view_get_buffer(v);
	gtk_text_buffer_get_end_iter(b,&iter);
	gtk_text_view_scroll_to_iter(v,&iter,0,FALSE,1.0,0);
}

void linphone_gtk_log_scroll_to_end(GtkToggleButton *button){
	if (gtk_toggle_button_get_active(button)){
		GtkTextView *v=GTK_TEXT_VIEW(linphone_gtk_get_widget(log_window,"textview"));
		stick_to_end(v);
	}
}

/*
 * called from Gtk main loop.
**/
gboolean linphone_gtk_check_logs(){
	GList *elem;
	GtkTextView *v=NULL;
	if (log_window) v=GTK_TEXT_VIEW(linphone_gtk_get_widget(log_window,"textview"));
	g_static_mutex_lock(&log_mutex);
	for(elem=log_queue;elem!=NULL;elem=elem->next){
		LinphoneGtkLog *lgl=(LinphoneGtkLog*)elem->data;
		if (v) linphone_gtk_display_log(v,lgl->lev,lgl->msg);
		g_free(lgl->msg);
		g_free(lgl);
	}
	if (log_queue) g_list_free(log_queue);
	log_queue=NULL;
	g_static_mutex_unlock(&log_mutex);
	if (v)
		linphone_gtk_log_scroll_to_end(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(log_window,"scroll_to_end")));
	return TRUE;
}


/*
 * Called from any linphone thread.
 */
void linphone_gtk_log_push(OrtpLogLevel lev, const char *fmt, va_list args){
	gchar *msg=g_strdup_vprintf(fmt,args);
	LinphoneGtkLog *lgl=g_new(LinphoneGtkLog,1);
	lgl->lev=lev;
	lgl->msg=msg;
	g_static_mutex_lock(&log_mutex);
	log_queue=g_list_append(log_queue,lgl);
	linphone_gtk_log_file(lev, msg);
	g_static_mutex_unlock(&log_mutex);
}

void linphone_gtk_log_clear(void){
	if (log_window){
		GtkTextIter end,begin;
		GtkTextView *v;
		GtkTextBuffer *b;
		v=GTK_TEXT_VIEW(linphone_gtk_get_widget(log_window,"textview"));
		b=gtk_text_view_get_buffer(v);
		gtk_text_buffer_get_start_iter(b,&begin);
		gtk_text_buffer_get_end_iter(b,&end);
		gtk_text_buffer_delete(b,&begin,&end);
	}
}


