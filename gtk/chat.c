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

#define NB_MSG_HIST 250

#define CONFIG_FILE ".linphone-history.db"

const char *linphone_gtk_message_storage_get_db_file(const char *filename){
	const int path_max=1024;
	static char *db_file=NULL;
	
	if (db_file) return db_file;
	
	db_file=(char *)malloc(path_max*sizeof(char));
	if (filename==NULL) filename=CONFIG_FILE;
	/*try accessing a local file first if exists*/
	if (access(CONFIG_FILE,F_OK)==0){
		snprintf(db_file,path_max,"%s",filename);
	}else{
#ifdef WIN32
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


void linphone_gtk_quit_chatroom(LinphoneChatRoom *cr) {
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	GtkWidget *nb=linphone_gtk_get_widget(main_window,"viewswitch");
	GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
	GtkWidget *w=g_object_get_data(G_OBJECT(friendlist),"chatview");
	gchar *from;
	GHashTable *table=g_object_get_data(G_OBJECT(w),"table");
	
	g_return_if_fail(w!=NULL);
	gtk_notebook_remove_page(GTK_NOTEBOOK(nb),gtk_notebook_page_num(GTK_NOTEBOOK(nb),w));
	linphone_chat_room_mark_as_read(cr);
	linphone_gtk_friend_list_update_chat_picture();
	g_object_set_data(G_OBJECT(friendlist),"chatview",NULL);
	from=g_object_get_data(G_OBJECT(w),"from_message");
	if (from){
		g_object_set_data(G_OBJECT(w),"from_message",NULL);
		g_free(from);
	}
	g_hash_table_destroy(table);
	g_object_set_data(G_OBJECT(w),"cr",NULL);
	g_object_set_data(G_OBJECT(friendlist),"from",NULL);
	gtk_widget_destroy(w);
}

const char* get_display_name(const LinphoneAddress *from){
	const char *display;
	display=linphone_address_get_display_name(from);
	if (display==NULL || display[0]=='\0') {
		display=linphone_address_get_username(from);
	}
	return display;
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
	l=gtk_label_new(get_display_name(uri));
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
	l=gtk_label_new (get_display_name(uri));
	gtk_box_pack_start (GTK_BOX(w),i,FALSE,FALSE,0);
	gtk_box_pack_start (GTK_BOX(w),l,FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(w),b,TRUE,TRUE,0);
	gtk_notebook_set_tab_label(notebook,chat_view,w);
	gtk_widget_show_all(w);
}

static gboolean scroll_to_end(GtkTextView *w){
	GtkTextBuffer *buffer=gtk_text_view_get_buffer(w);
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(buffer,&iter);
	GtkTextMark *mark=gtk_text_buffer_create_mark(buffer,NULL,&iter,FALSE);
	gtk_text_view_scroll_mark_onscreen(w,mark); 
	return FALSE;
}

void linphone_gtk_push_text(GtkWidget *w, const LinphoneAddress *from, 
                 gboolean me,LinphoneChatRoom *cr,LinphoneChatMessage *msg, gboolean hist){
	GtkTextView *text=GTK_TEXT_VIEW(linphone_gtk_get_widget(w,"textview"));
	GtkTextBuffer *buffer=gtk_text_view_get_buffer(text);
	GtkTextIter iter,begin;
	int off;
	char *from_str=linphone_address_as_string_uri_only(from);
	gchar *from_message=(gchar *)g_object_get_data(G_OBJECT(w),"from_message");
	GHashTable *table=(GHashTable*)g_object_get_data(G_OBJECT(w),"table");
	time_t t;
	char buf[80];
	time_t tnow;
	struct tm *tm;
	int tnow_day;
	int tnow_year;
	
	gtk_text_buffer_get_start_iter(buffer,&begin);
	gtk_text_buffer_get_end_iter(buffer,&iter);
	off=gtk_text_iter_get_offset(&iter);
	if(g_strcmp0(from_message,from_str)!=0){
		gtk_text_buffer_get_iter_at_offset(buffer,&iter,off);
		gtk_text_buffer_get_end_iter(buffer,&iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,get_display_name(from),-1,"bold",me ? "bg":NULL,NULL);
		gtk_text_buffer_get_end_iter(buffer,&iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer,&iter," : ",-1,"bold",me ? "bg":NULL,NULL);
		gtk_text_buffer_get_end_iter(buffer,&iter);
		gtk_text_buffer_insert(buffer,&iter,"\n",-1);
		g_free(from_message);
		g_object_set_data(G_OBJECT(w),"from_message",g_strdup(from_str));
	}
	gtk_text_buffer_get_end_iter(buffer,&iter);
	gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,linphone_chat_message_get_text(msg),-1,"margin",me ? "bg":NULL,NULL);
	gtk_text_buffer_get_end_iter(buffer,&iter);
	gtk_text_buffer_insert(buffer,&iter,"\n",-1);
	gtk_text_buffer_get_end_iter(buffer,&iter);
	t=linphone_chat_message_get_time(msg);
	switch (linphone_chat_message_get_state (msg)){
		case LinphoneChatMessageStateInProgress:
		{
			g_hash_table_insert(table,(gpointer)msg,GINT_TO_POINTER(gtk_text_iter_get_line(&iter)));
			gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,"Sending .. ",-1,									
		                                "right","small","italic","font_grey","bg",NULL);
			g_object_set_data(G_OBJECT(w),"table",table);
			break;
		}
		case LinphoneChatMessageStateDelivered:
		{
			tnow=time(NULL);
			tm=localtime(&tnow);
			tnow_day=tm->tm_yday;
			tnow_year=tm->tm_year;
			tm=localtime(&t);
			if(tnow_day != tm->tm_yday || (tnow_day == tm->tm_yday && tnow_year != tm->tm_year)) {
				strftime(buf,80,"%a %x, %H:%M",tm);
			} else {
				strftime(buf,80,"%H:%M",tm);
			}
			gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,buf,-1,									
	                      "right","small","italic","font_grey",me ? "bg":NULL,NULL);
			break;
		}
		case  LinphoneChatMessageStateNotDelivered:
				gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,"Message not sent",-1,									
	                       "right","small","italic","font_grey",me ? "bg":NULL,NULL);
				break;
		default : gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,"Sending ..",-1,									
	                       "right","small","italic","font_grey",me ? "bg":NULL,NULL);
	}
	gtk_text_buffer_get_end_iter(buffer,&iter);
	gtk_text_buffer_insert(buffer,&iter,"\n",-1);
	g_idle_add((GSourceFunc)scroll_to_end,text);
	ms_free(from_str);
}

const LinphoneAddress* linphone_gtk_get_used_identity(){
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneProxyConfig *cfg;
	linphone_core_get_default_proxy(lc,&cfg);
	if (cfg) return linphone_address_new(linphone_proxy_config_get_identity(cfg));
	else  return linphone_core_get_primary_contact_parsed(lc);
}

void update_chat_state_message(LinphoneChatMessageState state,LinphoneChatMessage *msg){
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
	GtkWidget *page=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
	GHashTable *table=(GHashTable*)g_object_get_data(G_OBJECT(page),"table");
	
	if(page!=NULL){
		GtkTextView *text=GTK_TEXT_VIEW(linphone_gtk_get_widget(page,"textview"));
		GtkTextBuffer *b=gtk_text_view_get_buffer(text);
		GtkTextIter iter;
		GtkTextIter end;
		GtkTextIter start;
		gchar *result;
		gint line;
		line=GPOINTER_TO_INT(g_hash_table_lookup(table,msg));

		gtk_text_buffer_get_iter_at_line(b,&iter,line);
		if(gtk_text_iter_get_chars_in_line(&iter) >0) {
			gtk_text_buffer_get_iter_at_line_offset(b,&start,line,
					gtk_text_iter_get_chars_in_line(&iter)-1);
		}else{
			gtk_text_buffer_get_iter_at_line_offset(b,&start,line,0);
		}
		gtk_text_buffer_get_iter_at_line_offset(b,&end,line,0);
		gtk_text_buffer_delete(b,&start,&end);
		gtk_text_buffer_get_iter_at_line(b,&iter,line);

		switch (state) {
			case LinphoneChatMessageStateInProgress:
				result="Sending ";
				break;
			case LinphoneChatMessageStateDelivered:
			{
				time_t t=time(NULL);
				struct tm *tm=localtime(&t);
				char buf[80];
				strftime(buf,80,"%H:%M",tm);
				result=buf;
				break;
			}
			case  LinphoneChatMessageStateNotDelivered:
				result="Message not sent";
				break;
			default : result="Sending ..";
		}
		gtk_text_buffer_insert_with_tags_by_name(b,&iter,result,-1,
												"right","small","italic","font_grey","bg",NULL);
		g_hash_table_remove(table,msg);
		g_object_set_data(G_OBJECT(page),"table",table);
	} 
}

static void on_chat_state_changed(LinphoneChatMessage *msg, LinphoneChatMessageState state, void *user_pointer){
	update_chat_state_message(state,msg);
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
		linphone_chat_room_send_message2(cr,msg,on_chat_state_changed,NULL);
		linphone_gtk_push_text(w,linphone_chat_message_get_from(msg),
				TRUE,cr,msg,FALSE);
		gtk_entry_set_text(GTK_ENTRY(entry),"");
	}
}

static void linphone_gtk_chat_message_destroy(LinphoneChatMessage *msg){
	linphone_chat_message_destroy(msg);
}

void linphone_gtk_free_list(MSList *messages){
	ms_list_for_each(messages,(void (*)(void*))linphone_gtk_chat_message_destroy);
	ms_list_free(messages);
}

void display_history_message(GtkWidget *chat_view,MSList *messages,const LinphoneAddress *with){
	if(messages != NULL){
		MSList *it;
		char *from_str;
		char *with_str;
		gchar *tmp;
		for(it=messages;it!=NULL;it=it->next){
			LinphoneChatMessage *msg=(LinphoneChatMessage *)it->data;
			from_str=linphone_address_as_string_uri_only(linphone_chat_message_get_from(msg));
			with_str=linphone_address_as_string_uri_only(with);
			linphone_gtk_push_text(chat_view,strcmp(from_str,with_str)==0? with : 
				                       linphone_chat_message_get_from(msg), 
            						strcmp(from_str,with_str)==0? FALSE : TRUE,
			                        linphone_chat_message_get_chat_room(msg),msg,TRUE);
		}
		tmp=g_object_get_data(G_OBJECT(chat_view),"from_message");
		if (tmp){
			g_object_set_data(G_OBJECT(chat_view),"from_message",NULL);
			g_free(tmp);
		}
		ms_free(from_str);
		ms_free(with_str);
		linphone_gtk_free_list(messages);
	} 
}

void linphone_gtk_chat_add_contact(const LinphoneAddress *addr){
	LinphoneFriend *lf=NULL;
	char *uri=linphone_address_as_string(addr);
	lf=linphone_friend_new_with_addr(uri);
	ms_free(uri);
	char *fixed_uri=NULL;
	gboolean show_presence=FALSE;

	linphone_friend_set_inc_subscribe_policy(lf,LinphoneSPDeny);
	linphone_friend_send_subscribe(lf,show_presence);

	linphone_core_interpret_friend_uri(linphone_gtk_get_core(),uri,&fixed_uri);
	if (fixed_uri==NULL){
		linphone_gtk_display_something(GTK_MESSAGE_WARNING,_("Invalid sip contact !"));
		return ;
	}
	linphone_friend_set_addr(lf,addr);
	linphone_core_add_friend(linphone_gtk_get_core(),lf);
	ms_free(fixed_uri);
	linphone_gtk_show_friends();
}

GtkWidget* linphone_gtk_init_chatroom(LinphoneChatRoom *cr, const LinphoneAddress *with){
	GtkWidget *chat_view=linphone_gtk_create_widget("main","chatroom_frame");
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	GtkNotebook *notebook=(GtkNotebook *)linphone_gtk_get_widget(main_window,"viewswitch");
	GtkWidget *text=linphone_gtk_get_widget(chat_view,"textview");
	GdkColor color;
	GdkColor colorb;
	int idx;
	GtkWidget *button;
	GtkWidget *entry;
	MSList *messages;
	GHashTable *table;
	char *with_str;

	color.red = 32512;
  	color.green = 32512;
  	color.blue = 32512;
	colorb.red = 56832;
  	colorb.green = 60928;
  	colorb.blue = 61952;
	
	with_str=linphone_address_as_string_uri_only(with);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text),GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text),FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text),FALSE);
	gtk_notebook_append_page(notebook,chat_view,create_tab_chat_header(cr,with));
	idx = gtk_notebook_page_num(notebook, chat_view);
	gtk_notebook_set_current_page(notebook, idx);
	gtk_widget_show(chat_view);
	table=g_hash_table_new_full(g_direct_hash,g_direct_equal,NULL,NULL);
	g_object_set_data(G_OBJECT(chat_view),"cr",cr);
	g_object_set_data(G_OBJECT(chat_view),"from_message",NULL);
	g_object_set_data(G_OBJECT(chat_view),"table",table);
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
	gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
	                           	"margin","indent",10,NULL);
	gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
	                           	"bg","paragraph-background-gdk",&colorb,NULL);
	messages = linphone_chat_room_get_history(cr,NB_MSG_HIST);
	display_history_message(chat_view,messages,with);
	button = linphone_gtk_get_widget(chat_view,"send");
	g_signal_connect_swapped(G_OBJECT(button),"clicked",(GCallback)linphone_gtk_send_text,NULL);
	entry = linphone_gtk_get_widget(chat_view,"text_entry");
	g_signal_connect_swapped(G_OBJECT(entry),"activate",(GCallback)linphone_gtk_send_text,NULL);
	g_signal_connect(G_OBJECT(notebook),"switch_page",(GCallback)linphone_gtk_notebook_tab_select,NULL);
	ms_free(with_str);
	return chat_view;
}

LinphoneChatRoom * linphone_gtk_create_chatroom(const LinphoneAddress *with){
	char *tmp=linphone_address_as_string(with);
	LinphoneChatRoom *cr=linphone_core_create_chat_room(linphone_gtk_get_core(),tmp);
	ms_free(tmp);
	return cr;
}

void linphone_gtk_load_chatroom(LinphoneChatRoom *cr,const LinphoneAddress *uri,GtkWidget *chat_view){
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	LinphoneChatRoom *cr2=(LinphoneChatRoom *)g_object_get_data(G_OBJECT(chat_view),"cr");
	const LinphoneAddress *from=linphone_chat_room_get_peer_address(cr2);
	char *from_str=linphone_address_as_string_uri_only(from);
	char *uri_str=linphone_address_as_string(uri);
	char *uri_only=linphone_address_as_string_uri_only(uri);
	MSList *messages=NULL;
	
	if(g_strcmp0(from_str,uri_only)!=0){
		GtkTextView *text_view=GTK_TEXT_VIEW(linphone_gtk_get_widget(chat_view,"textview"));
		GtkTextIter start;
		GtkTextIter end;
		GtkTextBuffer *text_buffer;

		text_buffer=gtk_text_view_get_buffer(text_view);
		gtk_text_buffer_get_bounds(text_buffer, &start, &end);
		gtk_text_buffer_delete (text_buffer, &start, &end);
		udpate_tab_chat_header(chat_view,uri,cr);
		g_object_set_data(G_OBJECT(chat_view),"cr",cr);
		g_object_set_data(G_OBJECT(linphone_gtk_get_widget(main_window,"contact_list")),"chatview",(gpointer)chat_view);
		messages=linphone_chat_room_get_history(cr,NB_MSG_HIST);
		g_object_set_data(G_OBJECT(chat_view),"from_message",g_strdup(uri_str));
		display_history_message(chat_view,messages,uri);
		gtk_text_buffer_get_end_iter(text_buffer,&end);
		gtk_text_view_scroll_to_iter(text_view,&end,0,FALSE,1.0,0);
	}
	ms_free(from_str);
	ms_free(uri_str);
	ms_free(uri_only);
}

void linphone_gtk_chat_destroyed(GtkWidget *w){
	LinphoneChatRoom *cr=(LinphoneChatRoom*)g_object_get_data(G_OBJECT(w),"cr");
	linphone_chat_room_destroy(cr);
}


void linphone_gtk_text_received ( LinphoneCore *lc, LinphoneChatRoom *room,
								  LinphoneChatMessage *msg ) {
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget ( main_window,"contact_list" );
	GtkWidget *w;
	gboolean send=TRUE;
	/*GtkNotebook *notebook= ( GtkNotebook * ) linphone_gtk_get_widget ( main_window,"viewswitch" );*/
	char *from=linphone_address_as_string ( linphone_chat_message_get_from ( msg ) );

	w= ( GtkWidget* ) g_object_get_data ( G_OBJECT ( friendlist ),"chatview" );
	if ( w!=NULL ) {
		char *from_chatview= ( char * ) g_object_get_data ( G_OBJECT ( friendlist ),"from" );
		if ( g_strcmp0 ( from,from_chatview ) ==0 ) {
			send=TRUE;
		} else {
			if ( !linphone_gtk_friend_list_is_contact ( linphone_chat_message_get_from ( msg ) ) ) {
				linphone_gtk_chat_add_contact ( linphone_chat_message_get_from ( msg ) );
			}
			send=FALSE;
		}
	} else {
		send=FALSE;
		if ( !linphone_gtk_friend_list_is_contact ( linphone_chat_message_get_from ( msg ) ) ) {
			linphone_gtk_chat_add_contact ( linphone_chat_message_get_from ( msg ) );
		}
		w=linphone_gtk_init_chatroom ( room,linphone_chat_message_get_from ( msg ) );
		g_object_set_data ( G_OBJECT ( friendlist ),"chatview", ( gpointer ) w );
		g_object_set_data ( G_OBJECT ( friendlist ),"from",from );
	}

#ifdef HAVE_GTK_OSX
	/* Notified when a new message is sent */
	linphone_gtk_status_icon_set_blinking ( TRUE );
#else
	if ( !gtk_window_is_active ( GTK_WINDOW ( main_window ) ) ) {
		if ( !GPOINTER_TO_INT ( g_object_get_data ( G_OBJECT ( w ),"is_notified" ) ) ) {
			linphone_gtk_notify ( NULL,linphone_chat_message_get_text ( msg ) );
			g_object_set_data ( G_OBJECT ( w ),"is_notified",GINT_TO_POINTER ( TRUE ) );
		} else {
			g_object_set_data ( G_OBJECT ( w ),"is_notified",GINT_TO_POINTER ( FALSE ) );
		}
	}
#endif
	if ( send ) {
		linphone_gtk_push_text ( w,linphone_chat_message_get_from ( msg ),
								 FALSE,room,msg,FALSE );
	}
	linphone_gtk_show_friends();
	
}
