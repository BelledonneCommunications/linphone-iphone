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

#ifdef HAVE_GTK_OSX
#include <gtkosxapplication.h>
#endif

#if defined(_WIN32) && !defined(F_OK)
#define F_OK 00 /*visual studio does not define F_OK*/
#endif

#define NB_MSG_HIST 250000

#define CONFIG_FILE ".linphone-history.db"

#include "regex.h"

GRegex *uri_regex = NULL;

static void free_uri_regex(void) {
	if(uri_regex) g_regex_unref(uri_regex);
}

static const GRegex *get_uri_regex(void) {
	const gchar *pattern = BC_REGEX_URI;
	GError *error = NULL;
	if(uri_regex == NULL) {
		uri_regex = g_regex_new(pattern, G_REGEX_OPTIMIZE, 0, &error);
		if(error) {
			g_warning("Could not parse regex pattern for URIs: %s", error->message);
			g_error_free(error);
			uri_regex = NULL;
			return NULL;
		}
		atexit(free_uri_regex);
	}
	return uri_regex;
}

char *linphone_gtk_message_storage_get_db_file(const char *filename){
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

void linphone_gtk_mark_chat_read(LinphoneChatRoom *cr) {
	linphone_chat_room_mark_as_read(cr);
#ifdef __APPLE__
	linphone_gtk_update_badge_count();
#endif
}

void linphone_gtk_quit_chatroom(LinphoneChatRoom *cr) {
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	GtkWidget *nb=linphone_gtk_get_widget(main_window,"viewswitch");
	GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
	GtkWidget *w=g_object_get_data(G_OBJECT(friendlist),"chatview");
	gchar *from;

	g_return_if_fail(w!=NULL);
	gtk_notebook_remove_page(GTK_NOTEBOOK(nb),gtk_notebook_page_num(GTK_NOTEBOOK(nb),w));
	linphone_gtk_mark_chat_read(cr);
	g_object_set_data(G_OBJECT(friendlist),"chatview",NULL);
	from=g_object_get_data(G_OBJECT(w),"from_message");
	if (from){
		g_object_set_data(G_OBJECT(w),"from_message",NULL);
		g_free(from);
	}
	g_object_set_data(G_OBJECT(w),"cr",NULL);
	linphone_gtk_friend_list_set_active_address(NULL);
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

GtkWidget *create_tab_chat_header(LinphoneChatRoom *cr, const LinphoneAddress *uri){
	GtkWidget *tab_header = linphone_gtk_make_tab_header(get_display_name(uri), "linphone-start-chat", TRUE, G_CALLBACK(linphone_gtk_quit_chatroom), cr);
	gtk_widget_show_all(tab_header);
	return tab_header;
}

void update_chat_header(GtkNotebook *notebook, GtkWidget *chat_view, LinphoneChatRoom *cr, const LinphoneAddress *uri) {
	GtkWidget *header = linphone_gtk_make_tab_header(get_display_name(uri), "linphone-start-chat", TRUE, G_CALLBACK(linphone_gtk_quit_chatroom), cr);
	gtk_widget_show_all(header);
	gtk_notebook_set_tab_label(notebook, chat_view, header);
}

static gboolean scroll_to_end(GtkTextView *w){
	GtkTextBuffer *buffer=gtk_text_view_get_buffer(w);
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(buffer,&iter);
	gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (w), &iter, 0.0, FALSE, 0, 0);
	return FALSE;
}

static void write_body(GtkTextBuffer *buffer, GtkTextIter *iter, const gchar *text, gint len, gboolean is_me, gboolean is_link) {
	const char *me_tag_name = is_me ? "me" : NULL;
	const char *link_tag_name = is_link ? "link" : NULL;
	if(me_tag_name) {
		gtk_text_buffer_insert_with_tags_by_name(buffer, iter, text, len, "body", me_tag_name, link_tag_name, NULL);
	} else {
		gtk_text_buffer_insert_with_tags_by_name(buffer, iter, text, len, "body", link_tag_name, NULL);
	}
}

void linphone_gtk_push_text(GtkWidget *w, const LinphoneAddress *from,
                 gboolean me,LinphoneChatRoom *cr,LinphoneChatMessage *msg, gboolean hist){
	GtkTextView *text=GTK_TEXT_VIEW(linphone_gtk_get_widget(w,"textview"));
	GtkTextBuffer *buffer=gtk_text_view_get_buffer(text);
	GtkTextIter iter;
	char *from_str=linphone_address_as_string_uri_only(from);
	gchar *from_message=(gchar *)g_object_get_data(G_OBJECT(w),"from_message");
	GHashTable *table=(GHashTable*)g_object_get_data(G_OBJECT(w),"table");
	const GRegex *uri_regex = get_uri_regex();
	GMatchInfo *match_info = NULL;
	const char *message = linphone_chat_message_get_text(msg);
	time_t t;
	char buf[80];
	time_t tnow;
	struct tm *tm;
	int tnow_day;
	int tnow_year;
	int pos = 0, start, end;

	gtk_text_buffer_get_end_iter(buffer, &iter);
	if (g_strcmp0(from_message,from_str)!=0){
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, get_display_name(from), -1,
		                                         "from", me ? "me" : NULL, NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer,&iter, " : ", -1,
		                                         "from", me ? "me" : NULL, NULL);
		gtk_text_buffer_insert(buffer,&iter,"\n",-1);
		g_free(from_message);
		g_object_set_data(G_OBJECT(w),"from_message",g_strdup(from_str));
	}
	ms_free(from_str);

	if (!message) {
		const char *external_body_url = linphone_chat_message_get_external_body_url(msg);
		if (external_body_url) message = external_body_url;
	}

	// Inserts message body and tags URIs as hypertext links
	if(message) {
		g_regex_match(uri_regex, message, 0, &match_info);
		while(g_match_info_matches(match_info)) {
			g_match_info_fetch_pos(match_info, 0, &start, &end);
			if(pos < start) write_body(buffer, &iter, &message[pos], start-pos, me, FALSE);
			write_body(buffer, &iter, &message[start], end-start, me, TRUE);
			pos = end;
			g_match_info_next(match_info, NULL);
		}
		if((size_t)pos < strlen(message)) write_body(buffer, &iter, &message[pos], -1, me, FALSE);
		gtk_text_buffer_insert(buffer,&iter,"\n",-1);
		g_match_info_free(match_info);
	}

	g_hash_table_insert(table,GUINT_TO_POINTER(linphone_chat_message_get_storage_id(msg)),GINT_TO_POINTER(gtk_text_iter_get_line(&iter)));
	switch (linphone_chat_message_get_state (msg)){
		case LinphoneChatMessageStateInProgress:
			gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,_("Sending..."),-1,"status", me ? "me" : NULL, NULL);
			break;
		case LinphoneChatMessageStateDelivered:
			t=linphone_chat_message_get_time(msg);
			tnow=time(NULL);
			tm=localtime(&tnow);
			tnow_day=tm->tm_yday;
			tnow_year=tm->tm_year;
			tm=localtime(&t);
			if(tnow_day != tm->tm_yday || (tnow_day == tm->tm_yday && tnow_year != tm->tm_year)) {
				strftime(buf,80,"%a %x, %H:%M",tm);
			} else {
				strftime(buf,80,"%H:%M:%S",tm);
			}
			gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,buf,-1,"status", me ? "me" : NULL, NULL);
			break;
		case LinphoneChatMessageStateNotDelivered:
			gtk_text_buffer_insert_with_tags_by_name(buffer,&iter,_("Message not sent"),-1,"status", me ? "me" : NULL, NULL);
			break;
		default:
			break;
	}
	gtk_text_buffer_insert(buffer,&iter,"\n",-1);
	g_idle_add((GSourceFunc)scroll_to_end,text);
}

void update_chat_state_message(LinphoneChatMessageState state,LinphoneChatMessage *msg){
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
	GtkWidget *page=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
	GHashTable *table=(GHashTable*)g_object_get_data(G_OBJECT(page),"table");

	if (page!=NULL) {
		char buf[80];
		time_t t;
		struct tm *tm;
		GtkTextView *text=GTK_TEXT_VIEW(linphone_gtk_get_widget(page,"textview"));
		GtkTextBuffer *b=gtk_text_view_get_buffer(text);
		GtkTextIter iter;
		GtkTextIter end;
		GtkTextIter start;
		gint line;
		gpointer hash_table_ptr = g_hash_table_lookup(table,GUINT_TO_POINTER(linphone_chat_message_get_storage_id(msg)));
		if (hash_table_ptr != NULL) {
			line = GPOINTER_TO_INT(hash_table_ptr);
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
					gtk_text_buffer_insert_with_tags_by_name(b,&iter,_("Sending..."),-1,"status", "me", NULL);
					break;
				case LinphoneChatMessageStateDelivered:
					t=time(NULL);
					tm=localtime(&t);
					strftime(buf,80,"%H:%M:%S",tm);
					gtk_text_buffer_insert_with_tags_by_name(b,&iter,(gchar*)buf,-1,"status", "me", NULL);
					break;
				case LinphoneChatMessageStateNotDelivered:
					gtk_text_buffer_insert_with_tags_by_name(b,&iter,_("Message not sent"),-1,"status", "me", NULL);
					break;
				default:
					break;
			}
		}
	}
}

static void on_chat_state_changed(LinphoneChatMessage *msg, LinphoneChatMessageState state){
	update_chat_state_message(state,msg);
}

void linphone_gtk_compose_text(void) {
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
	GtkWidget *w=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
	LinphoneChatRoom *cr=g_object_get_data(G_OBJECT(w),"cr");
	if (cr) {
		linphone_chat_room_compose(cr);
		linphone_gtk_mark_chat_read(cr);
		linphone_gtk_friend_list_update_button_display(GTK_TREE_VIEW(friendlist));
	}
}

void linphone_gtk_send_text(void){
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
	GtkWidget *w=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
	GtkWidget *entry=linphone_gtk_get_widget(w,"text_entry");
	const gchar *entered;
	LinphoneChatRoom *cr=g_object_get_data(G_OBJECT(w),"cr");
	entered=gtk_entry_get_text(GTK_ENTRY(entry));
	if (strlen(entered)>0) {
		LinphoneChatMessage *msg;
		LinphoneChatMessageCbs *cbs;
		msg=linphone_chat_message_ref(linphone_chat_room_create_message(cr,entered));
		cbs=linphone_chat_message_get_callbacks(msg);
		linphone_chat_message_cbs_set_msg_state_changed(cbs,on_chat_state_changed);
		linphone_chat_room_send_chat_message(cr,msg);
		linphone_gtk_push_text(w,linphone_chat_message_get_from(msg),
				TRUE,cr,msg,FALSE);

		// Disconnect and reconnect the "changed" signal to prevent triggering it when clearing the text entry.
		g_signal_handlers_disconnect_by_func(G_OBJECT(entry),(GCallback)linphone_gtk_compose_text,NULL);
		gtk_entry_set_text(GTK_ENTRY(entry),"");
		g_signal_connect_swapped(G_OBJECT(entry),"changed",(GCallback)linphone_gtk_compose_text,NULL);

		linphone_chat_message_unref(msg);
	}
}

static void linphone_gtk_chat_message_destroy(LinphoneChatMessage *msg){
	linphone_chat_message_destroy(msg);
}

void linphone_gtk_free_list(bctbx_list_t *messages){
	bctbx_list_for_each(messages,(void (*)(void*))linphone_gtk_chat_message_destroy);
	bctbx_list_free(messages);
}

void display_history_message(GtkWidget *chat_view,bctbx_list_t *messages,const LinphoneAddress *with){
	if (messages != NULL){
		bctbx_list_t *it;
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
			ms_free(from_str);
			ms_free(with_str);
		}
		tmp=g_object_get_data(G_OBJECT(chat_view),"from_message");
		if (tmp){
			g_object_set_data(G_OBJECT(chat_view),"from_message",NULL);
			g_free(tmp);
		}

		linphone_gtk_free_list(messages);
	}
}

static void linphone_gtk_chat_add_contact(const LinphoneAddress *addr){
	LinphoneFriend *lf=NULL;
	LinphoneCore *lc = linphone_gtk_get_core();
	gboolean show_presence=FALSE;
	char *uri=linphone_address_as_string(addr);

	lf=linphone_core_create_friend_with_address(lc, uri);
	ms_free(uri);

	linphone_friend_set_inc_subscribe_policy(lf,LinphoneSPWait);
	linphone_friend_send_subscribe(lf,show_presence);

	linphone_friend_set_address(lf,addr);
	linphone_core_add_friend(lc, lf);
	linphone_gtk_show_friends();
}

static GdkColor *_linphone_gtk_chatroom_get_link_color(GtkWidget *chatview) {
	GValue color_value = {0};
	g_value_init(&color_value, GDK_TYPE_COLOR);
	gtk_style_get_style_property(
		gtk_widget_get_style(chatview),
		G_OBJECT_TYPE(chatview),
		"link-color", &color_value);

	return (GdkColor *)g_value_get_boxed(&color_value);
}

static gboolean link_event_handler(GtkTextTag *tag, GObject *text_view,GdkEvent *event, GtkTextIter *iter, GtkWidget *chat_view) {
	if(event->type == GDK_BUTTON_PRESS) {
		GtkTextIter uri_begin = *iter;
		GtkTextIter uri_end = *iter;
		gchar *uri = NULL;
		LinphoneChatRoom *chat_room = (LinphoneChatRoom *)g_object_get_data(G_OBJECT(chat_view), "cr");
		GtkWidget *main_window = linphone_gtk_get_main_window();
		GtkWidget *friendlist = linphone_gtk_get_widget(main_window, "contact_list");

		gtk_text_iter_backward_to_tag_toggle(&uri_begin, tag);
		gtk_text_iter_forward_to_tag_toggle(&uri_end, tag);
		uri = gtk_text_iter_get_slice(&uri_begin, &uri_end);
		if(((GdkEventButton *)event)->button == 1) {
			linphone_gtk_open_browser(uri);
		} else if(((GdkEventButton *)event)->button == 3) {
			GtkMenu *menu = GTK_MENU(g_object_get_data(text_view, "link_ctx_menu"));
			g_object_set_data_full(G_OBJECT(menu), "uri", g_strdup(uri), g_free);
			gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 3, gdk_event_get_time(event));
		}
		g_free(uri);

		linphone_gtk_mark_chat_read(chat_room);
		linphone_gtk_friend_list_update_button_display(GTK_TREE_VIEW(friendlist));

		return TRUE;
	}
	return FALSE;
}

static void chatroom_enable_hand_cursor(GdkWindow *window, gboolean hand_cursor_enabled) {
#if GTK_CHECK_VERSION(2,22,0)
	GdkCursor *cursor = gdk_window_get_cursor(window);
	GdkCursor *new_cursor = NULL;
	if(!hand_cursor_enabled && gdk_cursor_get_cursor_type(cursor) != GDK_XTERM) {
		new_cursor = gdk_cursor_new(GDK_XTERM);
	} else if(hand_cursor_enabled && gdk_cursor_get_cursor_type(cursor) != GDK_HAND1) {
		new_cursor = gdk_cursor_new(GDK_HAND1);
	}
	if(new_cursor) {
		gdk_window_set_cursor(window, new_cursor);
		gdk_cursor_unref(new_cursor);
	}
#endif
}

static gboolean chatroom_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	gint wx, wy, bx, by;
	GtkTextView *chatroom = GTK_TEXT_VIEW(widget);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(chatroom);
	GtkTextTag *link_tag = gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(buffer), "link");
	GdkWindow *window = gtk_text_view_get_window(chatroom, GTK_TEXT_WINDOW_TEXT);
	GtkTextIter iter;
	if(event->type == GDK_MOTION_NOTIFY) {
		GdkEventMotion *motion_ev = (GdkEventMotion *)event;
		wx = (gint)motion_ev->x;
		wy = (gint)motion_ev->y;
		gtk_text_view_window_to_buffer_coords(chatroom, GTK_TEXT_WINDOW_TEXT, wx, wy, &bx, &by);
		gtk_text_view_get_iter_at_location(chatroom, &iter, bx, by);
		if(gtk_text_iter_has_tag(&iter, link_tag)) {
			chatroom_enable_hand_cursor(window, TRUE);
		} else {
			chatroom_enable_hand_cursor(window, FALSE);
		}
	}
	return FALSE;
}

static gboolean copy_uri_into_clipboard_handler(GtkMenuItem *menuitem, gpointer user_data) {
	GtkWidget *menu = gtk_widget_get_parent(GTK_WIDGET(menuitem));
	const gchar *uri = (const gchar *)g_object_get_data(G_OBJECT(menu), "uri");
	GtkClipboard *clipboard = NULL;
	GdkAtom clipboard_atom = gdk_atom_intern("CLIPBOARD", TRUE);
	if(clipboard_atom == GDK_NONE) {
		g_warning("Could not find CLIPBOARD atom");
		return FALSE;
	}
	clipboard = gtk_clipboard_get(clipboard_atom);
	if(uri) gtk_clipboard_set_text(clipboard, uri, -1);
	return FALSE;
}

static void refresh_lime_icon(GtkWidget* chat_view, LinphoneChatRoom*cr) {
	GtkWidget *lime_icon = linphone_gtk_get_widget(chat_view, "lime_icon");
	if (linphone_chat_room_lime_available(cr)) {
		gtk_widget_show(lime_icon);
	} else {
		gtk_widget_hide(lime_icon);
	}
}

static gint linphone_gtk_window_focused(GtkWidget* widget, GdkEvent *event, gpointer user_data) {
	// if we are in a chat, mark it as read
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget(main_window,"contact_list");
	GtkWidget *w=(GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");
	LinphoneChatRoom *cr=w?g_object_get_data(G_OBJECT(w),"cr"):NULL;
	if (cr) {
		linphone_gtk_mark_chat_read(cr);
	}
	refresh_lime_icon(w, cr);
	return FALSE;
}

GtkWidget* linphone_gtk_init_chatroom(LinphoneChatRoom *cr, const LinphoneAddress *with){
	GtkWidget *chat_view=linphone_gtk_create_widget("chatroom_frame");
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkNotebook *notebook=(GtkNotebook *)linphone_gtk_get_widget(main_window,"viewswitch");
	GtkWidget *text=linphone_gtk_get_widget(chat_view,"textview");
	GdkColor color_grey = {0, 32512, 32512, 32512};
	GdkColor color_light_grey = {0, 56832, 60928, 61952};
	GdkColor color_black = {0};
	int idx;
	GtkWidget *button;
	GtkWidget *entry = linphone_gtk_get_widget(chat_view,"text_entry");
	bctbx_list_t *messages;
	GHashTable *table;
	GtkTextTag *tmp_tag;
	GtkWidget *link_ctx_menu = gtk_menu_new();
	GtkWidget *link_ctx_menu_copy_item = gtk_menu_item_new_with_label(_("Copy"));

	gtk_notebook_append_page(notebook,chat_view,create_tab_chat_header(cr,with));
	idx = gtk_notebook_page_num(notebook, chat_view);
	gtk_notebook_set_current_page(notebook, idx);
	gtk_widget_show(chat_view);
	table=g_hash_table_new_full(g_direct_hash,g_direct_equal,NULL,NULL);
	g_object_set_data(G_OBJECT(chat_view),"cr",cr);
	g_object_set_data(G_OBJECT(chat_view),"from_message",NULL);
	g_object_set_data_full(G_OBJECT(chat_view),"table",table,(GDestroyNotify)g_hash_table_destroy);

	gtk_text_buffer_create_tag(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
		"me",
		"foreground_gdk", &color_black,
		"paragraph-background-gdk", &color_light_grey,
		NULL);

	gtk_text_buffer_create_tag(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
		"from",
		"weight", PANGO_WEIGHT_BOLD,
		NULL);

	gtk_text_buffer_create_tag(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
		"body",
		"indent", 10,
		NULL);

	gtk_text_buffer_create_tag(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
		"status",
		"size-points", 9.0,
		"foreground_gdk", &color_grey,
		"style", PANGO_STYLE_ITALIC,
		"justification", GTK_JUSTIFY_RIGHT,
		NULL);

	tmp_tag = gtk_text_buffer_create_tag(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
		"link",
		"underline", PANGO_UNDERLINE_SINGLE,
		"foreground_gdk", _linphone_gtk_chatroom_get_link_color(chat_view),
		NULL);
	g_signal_connect(G_OBJECT(tmp_tag), "event", G_CALLBACK(link_event_handler), chat_view);
	g_signal_connect(G_OBJECT(text), "event", G_CALLBACK(chatroom_event), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(link_ctx_menu), link_ctx_menu_copy_item);
	g_signal_connect(G_OBJECT(link_ctx_menu_copy_item), "activate", G_CALLBACK(copy_uri_into_clipboard_handler), NULL);
	gtk_widget_show_all(link_ctx_menu);
	g_object_set_data_full(G_OBJECT(text), "link_ctx_menu", link_ctx_menu, g_object_unref);
	g_object_ref_sink(G_OBJECT(link_ctx_menu));

	messages = linphone_chat_room_get_history(cr,NB_MSG_HIST);
	display_history_message(chat_view,messages,with);
	button = linphone_gtk_get_widget(chat_view,"send");
	g_signal_connect_swapped(G_OBJECT(button),"clicked",(GCallback)linphone_gtk_send_text,NULL);

	g_signal_connect_swapped(G_OBJECT(entry),"activate",(GCallback)linphone_gtk_send_text,NULL);
	g_signal_connect_swapped(G_OBJECT(entry),"changed",(GCallback)linphone_gtk_compose_text,NULL);
	g_signal_connect(G_OBJECT(notebook),"switch_page",(GCallback)linphone_gtk_notebook_tab_select,NULL);

	gtk_signal_connect(GTK_OBJECT(main_window), "focus-in-event", GTK_SIGNAL_FUNC(linphone_gtk_window_focused), NULL);

	refresh_lime_icon(chat_view, cr);

	return chat_view;
}

LinphoneChatRoom * linphone_gtk_create_chatroom(const LinphoneAddress *with){
	LinphoneChatRoom *cr=linphone_core_get_chat_room(linphone_gtk_get_core(), with);
	return cr;
}

void linphone_gtk_load_chatroom(LinphoneChatRoom *cr,const LinphoneAddress *uri,GtkWidget *chat_view){
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	GtkWidget *notebook = linphone_gtk_get_widget(main_window, "viewswitch");
	LinphoneChatRoom *cr2=(LinphoneChatRoom *)g_object_get_data(G_OBJECT(chat_view),"cr");
	const LinphoneAddress *from=linphone_chat_room_get_peer_address(cr2);
	char *from_str=linphone_address_as_string_uri_only(from);
	char *uri_str=linphone_address_as_string(uri);
	char *uri_only=linphone_address_as_string_uri_only(uri);
	bctbx_list_t *messages=NULL;

	if(g_strcmp0(from_str,uri_only)!=0){
		GtkTextView *text_view=GTK_TEXT_VIEW(linphone_gtk_get_widget(chat_view,"textview"));
		GtkTextIter start;
		GtkTextIter end;
		GtkTextBuffer *text_buffer;

		text_buffer=gtk_text_view_get_buffer(text_view);
		gtk_text_buffer_get_bounds(text_buffer, &start, &end);
		gtk_text_buffer_delete (text_buffer, &start, &end);
		update_chat_header(GTK_NOTEBOOK(notebook), chat_view, cr, uri);
		g_object_set_data(G_OBJECT(chat_view),"cr",cr);
		g_object_set_data(G_OBJECT(linphone_gtk_get_widget(main_window,"contact_list")),"chatview",(gpointer)chat_view);
		messages=linphone_chat_room_get_history(cr,NB_MSG_HIST);
		g_object_set_data(G_OBJECT(chat_view),"from_message",g_strdup(uri_str));
		display_history_message(chat_view,messages,uri);
		gtk_text_buffer_get_end_iter(text_buffer,&end);
		gtk_text_view_scroll_to_iter(text_view,&end,0,FALSE,1.0,0);
	}
	refresh_lime_icon(chat_view, cr);

	ms_free(from_str);
	ms_free(uri_str);
	ms_free(uri_only);
}

void linphone_gtk_chat_destroyed(GtkWidget *w){
	/*
	LinphoneChatRoom *cr=(LinphoneChatRoom*)g_object_get_data(G_OBJECT(w),"cr");
	*/
}


void linphone_gtk_text_received ( LinphoneCore *lc, LinphoneChatRoom *room,
								  LinphoneChatMessage *msg ) {
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *friendlist=linphone_gtk_get_widget ( main_window,"contact_list" );
	GtkWidget *w;
	gboolean send=TRUE;
	/*GtkNotebook *notebook= ( GtkNotebook * ) linphone_gtk_get_widget ( main_window,"viewswitch" );*/
	const LinphoneAddress *from= linphone_chat_message_get_from ( msg );

	w= ( GtkWidget* ) g_object_get_data ( G_OBJECT ( friendlist ),"chatview" );
	if ( w!=NULL ) {
	/* Chat window opened */
		const LinphoneAddress *from_chatview=linphone_gtk_friend_list_get_active_address();
		if (linphone_address_weak_equal(from,from_chatview)) {
			send=TRUE;
		} else {
			if ( !linphone_gtk_friend_list_is_contact ( linphone_chat_message_get_from ( msg ) ) ) {
				linphone_gtk_chat_add_contact ( linphone_chat_message_get_from ( msg ) );
			}
			send=FALSE;
		}
	} else {
	/* Chat window closed */
#ifdef SQLITE_STORAGE_ENABLED
		send=FALSE;
#else
		send=TRUE;
#endif
		if ( !linphone_gtk_friend_list_is_contact ( linphone_chat_message_get_from ( msg ) ) ) {
			linphone_gtk_chat_add_contact ( linphone_chat_message_get_from ( msg ) );
		}
		w=linphone_gtk_init_chatroom ( room,linphone_chat_message_get_from ( msg ) );
		g_object_set_data ( G_OBJECT ( friendlist ),"chatview", ( gpointer ) w );
		linphone_gtk_friend_list_set_active_address(from);
	}

#ifdef HAVE_GTK_OSX
	/* Notified when a new message is sent */
	linphone_gtk_status_icon_set_blinking ( TRUE );
#else
	if ( !gtk_window_is_active ( GTK_WINDOW ( main_window ) ) ) {
		if ( !GPOINTER_TO_INT ( g_object_get_data ( G_OBJECT ( w ),"is_notified" ) ) ) {
			linphone_gtk_notify ( NULL, msg, NULL );
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
	linphone_core_play_local(lc,linphone_gtk_get_sound_path("incoming_chat.wav"));
	linphone_gtk_show_friends();

}

void linphone_gtk_is_composing_received(LinphoneCore *lc, LinphoneChatRoom *room) {
	GtkWidget *main_window = linphone_gtk_get_main_window();
	GtkWidget *friendlist = linphone_gtk_get_widget(main_window, "contact_list");
	linphone_gtk_friend_list_update_button_display(GTK_TREE_VIEW(friendlist));
}
