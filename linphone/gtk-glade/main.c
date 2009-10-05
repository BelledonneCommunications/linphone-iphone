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

#define USE_LIBGLADE 1

#include "linphone.h"
#include "lpconfig.h"



#ifdef USE_LIBGLADE
#include <glade/glade.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define LINPHONE_ICON "linphone2.png"

const char *this_program_ident_string="linphone_ident_string=" LINPHONE_VERSION;

static LinphoneCore *the_core=NULL;
static GtkWidget *the_ui=NULL;

static void linphone_gtk_show(LinphoneCore *lc);
static void linphone_gtk_inv_recv(LinphoneCore *lc, const char *from);
static void linphone_gtk_bye_recv(LinphoneCore *lc, const char *from);
static void linphone_gtk_notify_recv(LinphoneCore *lc, LinphoneFriend * fid, const char *url, const char *status, const char *img);
static void linphone_gtk_new_unknown_subscriber(LinphoneCore *lc, LinphoneFriend *lf, const char *url);
static void linphone_gtk_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username);
static void linphone_gtk_display_status(LinphoneCore *lc, const char *status);
static void linphone_gtk_display_message(LinphoneCore *lc, const char *msg);
static void linphone_gtk_display_warning(LinphoneCore *lc, const char *warning);
static void linphone_gtk_display_url(LinphoneCore *lc, const char *msg, const char *url);
static void linphone_gtk_display_question(LinphoneCore *lc, const char *question);
static void linphone_gtk_call_log_updated(LinphoneCore *lc, LinphoneCallLog *cl);
static void linphone_gtk_general_state(LinphoneCore *lc, LinphoneGeneralState *gstate);
static void linphone_gtk_refer_received(LinphoneCore *lc, const char *refer_to);
static gboolean linphone_gtk_auto_answer(GtkWidget *incall_window);

static LinphoneCoreVTable vtable={
	.show=linphone_gtk_show,
	.inv_recv=linphone_gtk_inv_recv,
	.bye_recv=linphone_gtk_bye_recv,
	.notify_recv=linphone_gtk_notify_recv,
	.new_unknown_subscriber=linphone_gtk_new_unknown_subscriber,
	.auth_info_requested=linphone_gtk_auth_info_requested,
	.display_status=linphone_gtk_display_status,
	.display_message=linphone_gtk_display_message,
	.display_warning=linphone_gtk_display_warning,
	.display_url=linphone_gtk_display_url,
	.display_question=linphone_gtk_display_question,
	.call_log_updated=linphone_gtk_call_log_updated,
	.text_received=linphone_gtk_text_received,
	.general_state=linphone_gtk_general_state,
	.refer_received=linphone_gtk_refer_received
};

static gboolean verbose=0;
static gboolean auto_answer = 0;
static gchar * addr_to_call = NULL;
static gboolean iconified=FALSE;
#ifdef WIN32
static gchar *workingdir=NULL;
#endif

static GOptionEntry linphone_options[]={
	{
		.long_name="verbose",
		.short_name= '\0',
		.arg=G_OPTION_ARG_NONE,
		.arg_data= (gpointer)&verbose,
		.description=N_("log to stdout some debug information while running.")
	},
	{
		.long_name="iconified",
		.short_name= '\0',
		.arg=G_OPTION_ARG_NONE,
		.arg_data= (gpointer)&iconified,
		.description=N_("Start only in the system tray, do not show the main interface.")
	},
	{
	    .long_name = "call",
	    .short_name = 'c',
	    .arg = G_OPTION_ARG_STRING,
	    .arg_data = &addr_to_call,
	    .description = N_("address to call right now")
	},
	{
	    .long_name = "auto-answer",
	    .short_name = 'a',
	    .arg = G_OPTION_ARG_NONE,
	    .arg_data = (gpointer) & auto_answer,
	    .description = N_("if set automatically answer incoming calls")
	},
#ifdef WIN32
	{				/* zsd addition */
	    .long_name = "workdir",
	    .short_name = '\0',
	    .arg = G_OPTION_ARG_STRING,
	    .arg_data = (gpointer) & workingdir,
	    .description = N_("Specifiy a working directory (should be the base of the installation, eg: c:\\Program Files\\Linphone)")
	},
#endif
	{0}
};

#define INSTALLED_XML_DIR PACKAGE_DATA_DIR "/linphone"
#define BUILD_TREE_XML_DIR "gtk-glade"

#ifndef WIN32
#define CONFIG_FILE ".linphonerc"
#else
#define CONFIG_FILE "linphonerc"
#endif



static char _config_file[1024];


const char *linphone_gtk_get_config_file(){
	/*try accessing a local file first if exists*/
	if (access(CONFIG_FILE,F_OK)==0){
		snprintf(_config_file,sizeof(_config_file),"%s",CONFIG_FILE);
	}else{
#ifdef WIN32
		const char *appdata=getenv("APPDATA");
		if (appdata){
			snprintf(_config_file,sizeof(_config_file),"%s\\%s",appdata,LINPHONE_CONFIG_DIR);
			CreateDirectory(_config_file,NULL);
			snprintf(_config_file,sizeof(_config_file),"%s\\%s",appdata,LINPHONE_CONFIG_DIR "\\" CONFIG_FILE);
		}
#else
		const char *home=getenv("HOME");
		if (home==NULL) home=".";
		snprintf(_config_file,sizeof(_config_file),"%s/%s",home,CONFIG_FILE);
#endif
	}
	return _config_file;
}

static void linphone_gtk_init_liblinphone(const char *file){
	linphone_core_set_user_agent("Linphone", LINPHONE_VERSION);
	the_core=linphone_core_new(&vtable,file,NULL);
	linphone_core_set_waiting_callback(the_core,linphone_gtk_wait,NULL);
}



LinphoneCore *linphone_gtk_get_core(void){
	return the_core;
}

GtkWidget *linphone_gtk_get_main_window(){
	return the_ui;
}

static void parse_item(const char *item, const char *window_name, GtkWidget *w,  gboolean show){
	char tmp[64];
	char *dot;
	strcpy(tmp,item);
	dot=strchr(tmp,'.');
	if (dot){
		*dot='\0';
		dot++;
		if (strcmp(window_name,tmp)==0){
			GtkWidget *wd=linphone_gtk_get_widget(w,dot);
			if (wd) {
				if (!show) gtk_widget_hide(wd);
				else gtk_widget_show(wd);
			}
		}
	}
}

static void parse_widgets(const char *hiddens, const char *window_name, GtkWidget *w, gboolean show){
	char item[64];
	const char *i;
	const char *b;
	int len;
	for(b=i=hiddens;*i!='\0';++i){
		if (*i==' '){
			len=MIN(i-b,sizeof(item)-1);
			strncpy(item,b,len);
			item[len]='\0';
			b=i+1;
			parse_item(item,window_name,w,show);
		}
	}
	len=MIN(i-b,sizeof(item)-1);
	if (len>0){
		strncpy(item,b,len);
		item[len]='\0';
		parse_item(item,window_name,w,show);
	}
}

static void linphone_gtk_configure_window(GtkWidget *w, const char *window_name){
	static const char *icon_path=NULL;
	static const char *hiddens=NULL;
	static const char *shown=NULL;
	static bool_t config_loaded=FALSE;
	if (linphone_gtk_get_core()==NULL) return;
	if (config_loaded==FALSE){
		hiddens=linphone_gtk_get_ui_config("hidden_widgets",NULL);
		shown=linphone_gtk_get_ui_config("shown_widgets",NULL);
		icon_path=linphone_gtk_get_ui_config("icon",NULL);
		config_loaded=TRUE;
	}
	if (hiddens)
		parse_widgets(hiddens,window_name,w,FALSE);
	if (shown)
		parse_widgets(shown,window_name,w,TRUE);
	if (icon_path) {
		GdkPixbuf *pbuf=create_pixbuf(icon_path);
		gtk_window_set_icon(GTK_WINDOW(w),pbuf);
		g_object_unref(G_OBJECT(pbuf));
	}
}

#ifdef USE_LIBGLADE

GtkWidget *linphone_gtk_create_window(const char *window_name){
	GtkWidget *w;
	GladeXML *gxml;
	char path[2048];
	snprintf(path,sizeof(path),"%s/%s.glade",BUILD_TREE_XML_DIR,window_name);
	if (access(path,F_OK)!=0){
		snprintf(path,sizeof(path),"%s/%s.glade",INSTALLED_XML_DIR,window_name);
		if (access(path,F_OK)!=0){
			g_error("Could not locate neither %s/%s.glade and %s/%s.glade .",BUILD_TREE_XML_DIR,window_name,
				INSTALLED_XML_DIR,window_name);
			return NULL;
		}
	}
	gxml=glade_xml_new(path,NULL,NULL);
	glade_xml_signal_autoconnect(gxml);
	w=glade_xml_get_widget(gxml,window_name);
	if (w==NULL) g_error("Could not retrieve '%s' window from xml file",window_name);
	linphone_gtk_configure_window(w,window_name);
	return w;
}

GtkWidget *linphone_gtk_get_widget(GtkWidget *window, const char *name){
	GtkWidget *w;
	GladeXML *gxml=glade_get_widget_tree(window);
	if (gxml==NULL) g_error("Could not retrieve XML tree of window %s",name);
	w=glade_xml_get_widget(gxml,name);
	if (w==NULL) g_error("Could not retrieve widget %s",name);
	return GTK_WIDGET(w);
}

#else

GtkWidget *linphone_gtk_create_window(const char *window_name){
	
}

GtkWidget *linphone_gtk_get_widget(GtkWidget *window, const char *name){
	GObject *w=gtk_builder_get_object(the_ui,name);
	if (w==NULL){
		g_error("No widget named %s found in xml interface.",name);
	}
	return GTK_WIDGET(w);
}

#endif

void linphone_gtk_display_something(GtkMessageType type,const gchar *message){
	GtkWidget *dialog;
	GtkWidget *main_window=linphone_gtk_get_main_window();
	
	gtk_widget_show(main_window);
	if (type==GTK_MESSAGE_QUESTION)
	{
		/* draw a question box. link to dialog_click callback */
		dialog = gtk_message_dialog_new (
				GTK_WINDOW(main_window),
                                GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_QUESTION,
                                GTK_BUTTONS_YES_NO,
                                "%s",
				(const gchar*)message);
		/* connect to some callback : REVISIT */
		/*
		g_signal_connect_swapped (G_OBJECT (dialog), "response",
                           G_CALLBACK (dialog_click),
                           G_OBJECT (dialog));
		*/
		/* actually show the box */
		gtk_widget_show(dialog);
	}
	else
	{
		dialog = gtk_message_dialog_new (GTK_WINDOW(main_window),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  type,
                                  GTK_BUTTONS_CLOSE,
                                  "%s",
                                  (const gchar*)message);
		/* Destroy the dialog when the user responds to it (e.g. clicks a button) */
		g_signal_connect_swapped (G_OBJECT (dialog), "response",
                           G_CALLBACK (gtk_widget_destroy),
                           G_OBJECT (dialog));
		gtk_widget_show(dialog);
	}
}

void linphone_gtk_about_response(GtkDialog *dialog, gint id){
	if (id==GTK_RESPONSE_CANCEL){
		gtk_widget_destroy(GTK_WIDGET(dialog));
	}
}

static void about_url_clicked(GtkAboutDialog *dialog, const char *url, gpointer data){
	g_message("About url clicked");
	linphone_gtk_open_browser(url);
}

void linphone_gtk_show_about(){
	struct stat filestat;
	const char *license_file=PACKAGE_DATA_DIR "/linphone/COPYING";
	GtkWidget *about;
	
	about=linphone_gtk_create_window("about");
	gtk_about_dialog_set_url_hook(about_url_clicked,NULL,NULL);
	memset(&filestat,0,sizeof(filestat));
	if (stat(license_file,&filestat)!=0){
		license_file="COPYING";
		stat(license_file,&filestat);
	}
	if (filestat.st_size>0){
		char *license=g_malloc(filestat.st_size+1);
		FILE *f=fopen(license_file,"r");
		if (f && fread(license,filestat.st_size,1,f)==1){
			license[filestat.st_size]='\0';
			gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about),license);
		}
		g_free(license);
	}
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about),LINPHONE_VERSION);
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about),linphone_gtk_get_ui_config("title","Linphone"));
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about),linphone_gtk_get_ui_config("home","http://www.linphone.org"));
	gtk_widget_show(about);
}

static void set_video_window_decorations(GdkWindow *w){
	const char *title=linphone_gtk_get_ui_config("title","Linphone");
	const char *icon_path=linphone_gtk_get_ui_config("icon","linphone2.png");
	char video_title[256];
	GdkPixbuf *pbuf=create_pixbuf(icon_path);
	if (!linphone_core_in_call(linphone_gtk_get_core())){
		snprintf(video_title,sizeof(video_title),"%s video",title);	
	}else{
		const char *uri=linphone_core_get_remote_uri(linphone_gtk_get_core());
		gchar *display_name=linphone_gtk_get_display_name(uri);
		snprintf(video_title,sizeof(video_title),"Call with %s",display_name);
		g_free(display_name);
	}
	gdk_window_set_title(w,video_title);
	/*gdk_window_set_urgency_hint(w,TRUE);*/
	gdk_window_raise(w);
	if (pbuf){
		GList *l=NULL;
		l=g_list_append(l,pbuf);
		gdk_window_set_icon_list(w,l);
		g_list_free(l);
		g_object_unref(G_OBJECT(pbuf));
	}
}

static gboolean video_needs_update=FALSE;

static void update_video_title(){
	video_needs_update=TRUE;
}

static gboolean linphone_gtk_iterate(LinphoneCore *lc){
	static gboolean first_time=TRUE;
	unsigned long id;
	static unsigned long previd=0;
	static gboolean in_iterate=FALSE;
	
	/*avoid reentrancy*/
	if (in_iterate) return TRUE;
	in_iterate=TRUE;
	linphone_core_iterate(lc);
	if (first_time){
		/*after the first call to iterate, SipSetupContexts should be ready, so take actions:*/
		linphone_gtk_show_directory_search();
		first_time=FALSE;
	}

	id=linphone_core_get_native_video_window_id(lc);
	if (id!=previd || video_needs_update){
		GdkWindow *w;
		previd=id;
		if (id!=0){
			ms_message("Updating window decorations");
#ifndef WIN32
			w=gdk_window_foreign_new(id);
#else
			w=gdk_window_foreign_new((HANDLE)id);
#endif
			if (w) {
				set_video_window_decorations(w);
				g_object_unref(G_OBJECT(w));
			}
			else ms_error("gdk_window_foreign_new() failed");
			if (video_needs_update) video_needs_update=FALSE;
		}
	}
	if (addr_to_call!=NULL){
		/*make sure we are not showing the login screen*/
		GtkWidget *mw=linphone_gtk_get_main_window();
		GtkWidget *login_frame=linphone_gtk_get_widget(mw,"login_frame");
		if (!GTK_WIDGET_VISIBLE(login_frame)){
			GtkWidget *uri_bar=linphone_gtk_get_widget(mw,"uribar");
			gtk_entry_set_text(GTK_ENTRY(uri_bar),addr_to_call);
			addr_to_call=NULL;
			linphone_gtk_start_call(uri_bar);
		}
	}
	in_iterate=FALSE;
	return TRUE;
}

static void load_uri_history(){
	GtkEntry *uribar=GTK_ENTRY(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"uribar"));
	char key[20];
	int i;
	GtkEntryCompletion *gep=gtk_entry_completion_new();
	GtkListStore *model=gtk_list_store_new(1,G_TYPE_STRING);
	for (i=0;;i++){
		const char *uri;
		snprintf(key,sizeof(key),"uri%i",i);
		uri=linphone_gtk_get_ui_config(key,NULL);
		if (uri!=NULL) {
			GtkTreeIter iter;
			gtk_list_store_append(model,&iter);
			gtk_list_store_set(model,&iter,0,uri,-1);
			if (i==0) gtk_entry_set_text(uribar,uri);
		}
		else break;
	}
	gtk_entry_completion_set_model(gep,GTK_TREE_MODEL(model));
	gtk_entry_completion_set_text_column(gep,0);
	gtk_entry_set_completion(uribar,gep);
}

static void save_uri_history(){
	LinphoneCore *lc=linphone_gtk_get_core();
	LpConfig *cfg=linphone_core_get_config(lc);
	GtkEntry *uribar=GTK_ENTRY(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"uribar"));
	char key[20];
	int i=0;
	char *uri=NULL;
	GtkTreeIter iter;
	GtkTreeModel *model=gtk_entry_completion_get_model(gtk_entry_get_completion(uribar));

	if (!gtk_tree_model_get_iter_first(model,&iter)) return;
	do {
		gtk_tree_model_get(model,&iter,0,&uri,-1);
		if (uri) {
			snprintf(key,sizeof(key),"uri%i",i);
			lp_config_set_string(cfg,"GtkUi",key,uri);
			g_free(uri);
		}else break;
		i++;
		if (i>5) break;
	}while(gtk_tree_model_iter_next(model,&iter));
	lp_config_sync(cfg);
}

static void completion_add_text(GtkEntry *entry, const char *text){
	GtkTreeIter iter;
	GtkTreeModel *model=gtk_entry_completion_get_model(gtk_entry_get_completion(entry));
	
	if (gtk_tree_model_get_iter_first(model,&iter)){ 
		do {
			gchar *uri=NULL;
			gtk_tree_model_get(model,&iter,0,&uri,-1);
			if (uri!=NULL){
				if (strcmp(uri,text)==0) {
					/*remove text */
					gtk_list_store_remove(GTK_LIST_STORE(model),&iter);
					g_free(uri);
					break;
				}
				g_free(uri);
			}
		}while (gtk_tree_model_iter_next(model,&iter));
	}
	/* and prepend it on top of the list */
	gtk_list_store_prepend(GTK_LIST_STORE(model),&iter);
	gtk_list_store_set(GTK_LIST_STORE(model),&iter,0,text,-1);
	save_uri_history();
}

void linphone_gtk_call_terminated(const char *error){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *icw;
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"terminate_call"),FALSE);
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"start_call"),TRUE);
	gtk_widget_hide_all(linphone_gtk_get_widget(mw,"go_to_call_view_box"));
	linphone_gtk_enable_mute_button(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(mw,"main_mute")),FALSE);
	if (linphone_gtk_use_in_call_view())
		linphone_gtk_in_call_view_terminate(error);
	update_video_title();
	icw=GTK_WIDGET(g_object_get_data(G_OBJECT(mw),"incoming_call"));
	if (icw!=NULL){
		g_object_set_data(G_OBJECT(mw),"incoming_call",NULL);
		gtk_widget_destroy(icw);
	}
}

static gboolean in_call_timer(){
	if (linphone_core_in_call(linphone_gtk_get_core())){
		linphone_gtk_in_call_view_update_duration(
			linphone_core_get_current_call_duration(linphone_gtk_get_core()));
		return TRUE;
	}
	return FALSE;
}

static void linphone_gtk_call_started(GtkWidget *mw){
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"start_call"),FALSE);
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"terminate_call"),TRUE);
	gtk_widget_show_all(linphone_gtk_get_widget(mw,"go_to_call_view_box"));
	update_video_title();
	if (linphone_gtk_use_in_call_view())
		g_timeout_add(250,(GSourceFunc)in_call_timer,NULL);
}

static gboolean linphone_gtk_start_call_do(GtkWidget *uri_bar){
	const char *entered=gtk_entry_get_text(GTK_ENTRY(uri_bar));
	if (linphone_core_invite(linphone_gtk_get_core(),entered)==0) {
		completion_add_text(GTK_ENTRY(uri_bar),entered);
	}else{
	}
	return FALSE;
}

static void _linphone_gtk_accept_call(){
	LinphoneCore *lc=linphone_gtk_get_core();
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *icw=GTK_WIDGET(g_object_get_data(G_OBJECT(mw),"incoming_call"));
	if (icw!=NULL){
		g_object_set_data(G_OBJECT(mw),"incoming_call",NULL);
		gtk_widget_destroy(icw);
	}

	linphone_core_accept_call(lc,NULL);
	linphone_gtk_call_started(linphone_gtk_get_main_window());
	if (linphone_gtk_use_in_call_view()){
		linphone_gtk_in_call_view_set_in_call();
		linphone_gtk_show_in_call_view();
	}
	linphone_gtk_enable_mute_button(
		GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"main_mute"))
		,TRUE);
}

void linphone_gtk_start_call(GtkWidget *w){
	LinphoneCore *lc=linphone_gtk_get_core();
	if (linphone_core_inc_invite_pending(lc)){
		/*accept the call*/
		_linphone_gtk_accept_call();
	}else if (linphone_core_in_call(lc)) {
		/*already in call */
	}else{
		/*change into in-call mode, then do the work later as it might block a bit */
		GtkWidget *mw=gtk_widget_get_toplevel(w);
		GtkWidget *uri_bar=linphone_gtk_get_widget(mw,"uribar");
		const char *entered=gtk_entry_get_text(GTK_ENTRY(uri_bar));
		linphone_gtk_call_started(mw);
		if (linphone_gtk_use_in_call_view()){
			linphone_gtk_in_call_view_set_calling(entered);
			linphone_gtk_show_in_call_view();
		}
		g_timeout_add(100,(GSourceFunc)linphone_gtk_start_call_do,uri_bar);
	}
}

void linphone_gtk_uri_bar_activate(GtkWidget *w){
	linphone_gtk_start_call(w);
}


void linphone_gtk_terminate_call(GtkWidget *button){
	linphone_core_terminate_call(linphone_gtk_get_core(),NULL);
}

void linphone_gtk_decline_call(GtkWidget *button){
	linphone_core_terminate_call(linphone_gtk_get_core(),NULL);
	/* zsd note: there was a big here in 3.0.0 which caused an abort if
	 * someone clicked "decline"... the following line of code looks
	 * like a fix for that. */
	gtk_widget_destroy(gtk_widget_get_toplevel(button));
}

void linphone_gtk_accept_call(GtkWidget *button){
	_linphone_gtk_accept_call();
}

static gboolean linphone_gtk_auto_answer(GtkWidget *incall_window){
	linphone_gtk_accept_call(linphone_gtk_get_widget(incall_window,"accept_call"));
	return FALSE;
}

void linphone_gtk_set_audio_video(){
	linphone_core_enable_video(linphone_gtk_get_core(),TRUE,TRUE);
	linphone_core_enable_video_preview(linphone_gtk_get_core(),TRUE);
}

void linphone_gtk_set_audio_only(){
	linphone_core_enable_video(linphone_gtk_get_core(),FALSE,FALSE);
	linphone_core_enable_video_preview(linphone_gtk_get_core(),FALSE);
}

void linphone_gtk_enable_self_view(GtkWidget *w){
	linphone_core_enable_self_view(linphone_gtk_get_core(),
		gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}

void linphone_gtk_used_identity_changed(GtkWidget *w){
	int active=gtk_combo_box_get_active(GTK_COMBO_BOX(w));
	char *sel=gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	if (sel && strlen(sel)>0){ //avoid a dummy "changed" at gui startup
		linphone_core_set_default_proxy_index(linphone_gtk_get_core(),(active==0) ? -1 : (active-1));
		linphone_gtk_show_directory_search();
	}
}

static void linphone_gtk_show_main_window(){
	GtkWidget *w=linphone_gtk_get_main_window();
	LinphoneCore *lc=linphone_gtk_get_core();
	linphone_core_enable_video_preview(lc,linphone_core_video_enabled(lc));
	gtk_widget_show(w);
	gtk_window_present(GTK_WINDOW(w));
}

static void linphone_gtk_show(LinphoneCore *lc){
	linphone_gtk_show_main_window();
}

static void linphone_gtk_inv_recv(LinphoneCore *lc, const char *from){
	GtkWidget *w=linphone_gtk_create_window("incoming_call");
	GtkWidget *label;
	gchar *msg;

	if (auto_answer){
		g_timeout_add(2000,(GSourceFunc)linphone_gtk_auto_answer,w);
	}

	gtk_window_set_transient_for(GTK_WINDOW(w),GTK_WINDOW(linphone_gtk_get_main_window()));
	gtk_window_set_position(GTK_WINDOW(w),GTK_WIN_POS_CENTER_ON_PARENT);

	label=linphone_gtk_get_widget(w,"message");
	msg=g_strdup_printf(_("Incoming call from %s"),from);
	gtk_label_set_text(GTK_LABEL(label),msg);
	gtk_window_set_title(GTK_WINDOW(w),msg);
	gtk_widget_show(w);
	gtk_window_present(GTK_WINDOW(w));
	/*gtk_window_set_urgency_hint(GTK_WINDOW(w),TRUE);*/
	g_free(msg);
	g_object_set_data(G_OBJECT(linphone_gtk_get_main_window()),"incoming_call",w);
	gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"uribar")),
			from);
}

static void linphone_gtk_bye_recv(LinphoneCore *lc, const char *from){
	
}

static void linphone_gtk_notify_recv(LinphoneCore *lc, LinphoneFriend * fid, const char *url, const char *status, const char *img){
}

static void linphone_gtk_new_subscriber_response(GtkWidget *dialog, guint response_id, LinphoneFriend *lf){
	switch(response_id){
		case GTK_RESPONSE_YES:
			linphone_gtk_show_contact(lf);
		break;
		default:
			linphone_core_reject_subscriber(linphone_gtk_get_core(),lf);
	}
	gtk_widget_destroy(dialog);
}

static void linphone_gtk_new_unknown_subscriber(LinphoneCore *lc, LinphoneFriend *lf, const char *url){
	GtkWidget *dialog;
	gchar *message=g_strdup_printf(_("%s would like to add you to his contact list.\nWould you allow him to see your presence status or add him to your contact list ?\nIf you answer no, this person will be temporarily blacklisted."),url);
	dialog = gtk_message_dialog_new (
				GTK_WINDOW(linphone_gtk_get_main_window()),
                                GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_QUESTION,
                                GTK_BUTTONS_YES_NO,
                                "%s",
				message);
	g_free(message);
	g_signal_connect(G_OBJECT (dialog), "response",
		G_CALLBACK (linphone_gtk_new_subscriber_response),lf);
	/* actually show the box */
	gtk_widget_show(dialog);
}

typedef struct _AuthTimeout{
	GtkWidget *w;
} AuthTimeout;


static void auth_timeout_clean(AuthTimeout *tout){
	tout->w=NULL;
}

static gboolean auth_timeout_destroy(AuthTimeout *tout){
	if (tout->w)  {
		g_object_weak_unref(G_OBJECT(tout->w),(GWeakNotify)auth_timeout_clean,tout);
		gtk_widget_destroy(tout->w);
	}
	g_free(tout);
	return FALSE;
}

static AuthTimeout * auth_timeout_new(GtkWidget *w){
	AuthTimeout *tout=g_new(AuthTimeout,1);
	tout->w=w;
	/*so that the timeout no more references the widget when it is destroyed:*/
	g_object_weak_ref(G_OBJECT(w),(GWeakNotify)auth_timeout_clean,tout);
	/*so that the widget is automatically destroyed after some time */
	g_timeout_add(30000,(GtkFunction)auth_timeout_destroy,tout);
	return tout;
}

void linphone_gtk_password_cancel(GtkWidget *w){
	LinphoneAuthInfo *info;
	GtkWidget *window=gtk_widget_get_toplevel(w);
	info=(LinphoneAuthInfo*)g_object_get_data(G_OBJECT(window),"auth_info");
	linphone_core_abort_authentication(linphone_gtk_get_core(),info);
	gtk_widget_destroy(window);
}

void linphone_gtk_password_ok(GtkWidget *w){
	GtkWidget *entry;
	GtkWidget *window=gtk_widget_get_toplevel(w);
	LinphoneAuthInfo *info;
	info=(LinphoneAuthInfo*)g_object_get_data(G_OBJECT(window),"auth_info");
	g_object_weak_unref(G_OBJECT(window),(GWeakNotify)linphone_auth_info_destroy,info);
	entry=linphone_gtk_get_widget(window,"password_entry");
	linphone_auth_info_set_passwd(info,gtk_entry_get_text(GTK_ENTRY(entry)));
	linphone_auth_info_set_userid(info,
		gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(window,"userid_entry"))));
	linphone_core_add_auth_info(linphone_gtk_get_core(),info);
	gtk_widget_destroy(window);
}

static void linphone_gtk_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username){
	GtkWidget *w=linphone_gtk_create_window("password");
	GtkWidget *label=linphone_gtk_get_widget(w,"message");
	LinphoneAuthInfo *info;
	gchar *msg;
	msg=g_strdup_printf(_("Please enter your password for username <i>%s</i>\n at domain <i>%s</i>:"),
		username,realm);
	gtk_label_set_markup(GTK_LABEL(label),msg);
	g_free(msg);
	gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"userid_entry")),username);
	info=linphone_auth_info_new(username, NULL, NULL, NULL,realm);
	g_object_set_data(G_OBJECT(w),"auth_info",info);
	g_object_weak_ref(G_OBJECT(w),(GWeakNotify)linphone_auth_info_destroy,info);
	gtk_widget_show(w);
	auth_timeout_new(w);
}

static void linphone_gtk_display_status(LinphoneCore *lc, const char *status){
	GtkWidget *w=linphone_gtk_get_main_window();
	GtkWidget *status_bar=linphone_gtk_get_widget(w,"status_bar");
	gtk_statusbar_push(GTK_STATUSBAR(status_bar),
			gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar),""),
			status);
}

static void linphone_gtk_display_message(LinphoneCore *lc, const char *msg){
	linphone_gtk_display_something(GTK_MESSAGE_INFO,msg);
}

static void linphone_gtk_display_warning(LinphoneCore *lc, const char *warning){
	linphone_gtk_display_something(GTK_MESSAGE_WARNING,warning);
}

static void linphone_gtk_display_url(LinphoneCore *lc, const char *msg, const char *url){
	char richtext[4096];
	snprintf(richtext,sizeof(richtext),"%s %s",msg,url);
	linphone_gtk_display_something(GTK_MESSAGE_INFO,richtext);
}

static void linphone_gtk_display_question(LinphoneCore *lc, const char *question){
	linphone_gtk_display_something(GTK_MESSAGE_QUESTION,question);
}

static void linphone_gtk_call_log_updated(LinphoneCore *lc, LinphoneCallLog *cl){
	GtkWidget *w=(GtkWidget*)g_object_get_data(G_OBJECT(linphone_gtk_get_main_window()),"call_logs");
	if (w) linphone_gtk_call_log_update(w);
}

static void linphone_gtk_general_state(LinphoneCore *lc, LinphoneGeneralState *gstate){
	switch(gstate->new_state){
		case GSTATE_CALL_OUT_CONNECTED:
		case GSTATE_CALL_IN_CONNECTED:
			if (linphone_gtk_use_in_call_view())
				linphone_gtk_in_call_view_set_in_call();
			linphone_gtk_enable_mute_button(
				GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"main_mute")),
			TRUE);
		break;
		case GSTATE_CALL_ERROR:
			linphone_gtk_call_terminated(gstate->message);
		break;
		case GSTATE_CALL_END:
			linphone_gtk_call_terminated(NULL);
		break;
		default:
		break;
	}
}

static void icon_popup_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data){
	GtkWidget *menu=(GtkWidget*)g_object_get_data(G_OBJECT(status_icon),"menu");
	gtk_menu_popup(GTK_MENU(menu),NULL,NULL,gtk_status_icon_position_menu,status_icon,button,activate_time);
}

void linphone_gtk_open_browser(const char *url){
	/*in gtk 2.16, gtk_show_uri does not work...*/
	/*gtk_show_uri(NULL,url,GDK_CURRENT_TIME,NULL);*/
#ifdef WIN32
	ShellExecute(0,"open",url,NULL,NULL,1);
#else
	char cl[255];
	snprintf(cl,sizeof(cl),"/usr/bin/x-www-browser %s",url);
	g_spawn_command_line_async(cl,NULL);
#endif
}

void linphone_gtk_link_to_website(GtkWidget *item){
	const gchar *home=(const gchar*)g_object_get_data(G_OBJECT(item),"home");
	linphone_gtk_open_browser(home);
}

static GtkWidget *create_icon_menu(){
	GtkWidget *menu=gtk_menu_new();
	GtkWidget *menu_item;
	GtkWidget *image;
	gchar *tmp;
	const gchar *homesite;
	
	homesite=linphone_gtk_get_ui_config("home","http://www.linphone.org");
	menu_item=gtk_image_menu_item_new_with_label(_("Website link"));
	tmp=g_strdup(homesite);
	g_object_set_data(G_OBJECT(menu_item),"home",tmp);
	g_object_weak_ref(G_OBJECT(menu_item),(GWeakNotify)g_free,tmp);
	
	image=gtk_image_new_from_stock(GTK_STOCK_HELP,GTK_ICON_SIZE_MENU);
	gtk_widget_show(image);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
	//g_object_unref(G_OBJECT(image));
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
	g_signal_connect(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_link_to_website,NULL);
	
	menu_item=gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT,NULL);
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
	g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_show_about,NULL);
	menu_item=gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT,NULL);
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
	g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)gtk_main_quit,NULL);
	gtk_widget_show(menu);
	return menu;
}

static GtkStatusIcon *icon=NULL;

static void linphone_gtk_init_status_icon(){
	const char *icon_path=linphone_gtk_get_ui_config("icon",LINPHONE_ICON);
	GdkPixbuf *pbuf=create_pixbuf(icon_path);
	GtkWidget *menu=create_icon_menu();
	const char *title;
	icon=gtk_status_icon_new_from_pixbuf(pbuf);
	g_object_unref(G_OBJECT(pbuf));
	g_signal_connect_swapped(G_OBJECT(icon),"activate",(GCallback)linphone_gtk_show_main_window,linphone_gtk_get_main_window());
	g_signal_connect(G_OBJECT(icon),"popup-menu",(GCallback)icon_popup_menu,NULL);
	title=linphone_gtk_get_ui_config("title",_("Linphone - a video internet phone"));
	gtk_status_icon_set_tooltip(icon,title);
	gtk_status_icon_set_visible(icon,TRUE);
	g_object_set_data(G_OBJECT(icon),"menu",menu);
	g_object_weak_ref(G_OBJECT(icon),(GWeakNotify)gtk_widget_destroy,menu);
}

void linphone_gtk_load_identities(void){
	const MSList *elem;
	GtkComboBox *box=GTK_COMBO_BOX(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"identities"));
	char *def_identity;
	LinphoneProxyConfig *def=NULL;
	int def_index=0,i;
	GtkListStore *store;

	store=GTK_LIST_STORE(gtk_combo_box_get_model(box));
	gtk_list_store_clear(store);

	linphone_core_get_default_proxy(linphone_gtk_get_core(),&def);
	def_identity=g_strdup_printf(_("%s (Default)"),linphone_core_get_primary_contact(linphone_gtk_get_core()));
	gtk_combo_box_append_text(box,def_identity);
	g_free(def_identity);
	for(i=1,elem=linphone_core_get_proxy_config_list(linphone_gtk_get_core());
			elem!=NULL;
			elem=ms_list_next(elem),i++){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		gtk_combo_box_append_text(box,linphone_proxy_config_get_identity(cfg));
		if (cfg==def) {
			def_index=i;
		}
	}
	gtk_combo_box_set_active(box,def_index);
}

static void linphone_gtk_dtmf_clicked(GtkButton *button){
	const char *label=gtk_button_get_label(button);
	if (linphone_core_in_call(linphone_gtk_get_core())){
		linphone_core_send_dtmf(linphone_gtk_get_core(),label[0]);
	}else{
		GtkWidget *uri_bar=linphone_gtk_get_widget(gtk_widget_get_toplevel(GTK_WIDGET(button)),"uribar");
		int pos=-1;
		gtk_editable_insert_text(GTK_EDITABLE(uri_bar),label,1,&pos);
	}
}

static void linphone_gtk_connect_digits(void){
	GtkContainer *cont=GTK_CONTAINER(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"dtmf_table"));
	GList *children=gtk_container_get_children(cont);
	GList *elem;
	for(elem=children;elem!=NULL;elem=elem->next){
		GtkButton *button=GTK_BUTTON(elem->data);
		g_signal_connect(G_OBJECT(button),"clicked",(GCallback)linphone_gtk_dtmf_clicked,NULL);
	}
}

static void linphone_gtk_check_menu_items(void){
	bool_t audio_only=!linphone_core_video_enabled(linphone_gtk_get_core());
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(linphone_gtk_get_widget(
					linphone_gtk_get_main_window(),
					audio_only ? "audio_only_item" : "video_item")), TRUE);
}

static gboolean linphone_gtk_can_manage_accounts(){
	LinphoneCore *lc=linphone_gtk_get_core();
	const MSList *elem;
	for(elem=linphone_core_get_sip_setups(lc);elem!=NULL;elem=elem->next){
		SipSetup *ss=(SipSetup*)elem->data;
		if (sip_setup_get_capabilities(ss) & SIP_SETUP_CAP_ACCOUNT_MANAGER){
			return TRUE;
		}
	}
	return FALSE;
}

static void linphone_gtk_configure_main_window(){
	static gboolean config_loaded=FALSE;
	static const char *title;
	static const char *home;
	static const char *start_call_icon;
	static const char *stop_call_icon;
	GtkWidget *w=linphone_gtk_get_main_window();
	if (!config_loaded){
		title=linphone_gtk_get_ui_config("title","Linphone");
		home=linphone_gtk_get_ui_config("home","http://www.linphone.org");
		start_call_icon=linphone_gtk_get_ui_config("start_call_icon","green.png");
		stop_call_icon=linphone_gtk_get_ui_config("stop_call_icon","red.png");
		config_loaded=TRUE;
	}
	linphone_gtk_configure_window(w,"main_window");
	if (title) {
		gtk_window_set_title(GTK_WINDOW(w),title);
#if GTK_CHECK_VERSION(2,16,0)
		gtk_menu_item_set_label(GTK_MENU_ITEM(linphone_gtk_get_widget(w,"main_menu")),title);
#endif
	}
	if (start_call_icon){
		GdkPixbuf *pbuf=create_pixbuf(start_call_icon);
		gtk_image_set_from_pixbuf(GTK_IMAGE(linphone_gtk_get_widget(w,"start_call_icon")),pbuf);
		g_object_unref(G_OBJECT(pbuf));
	}
	if (stop_call_icon){
		GdkPixbuf *pbuf=create_pixbuf(stop_call_icon);
		gtk_image_set_from_pixbuf(GTK_IMAGE(linphone_gtk_get_widget(w,"terminate_call_icon")),pbuf);
		gtk_image_set_from_pixbuf(GTK_IMAGE(linphone_gtk_get_widget(w,"in_call_terminate_icon")),pbuf);
		g_object_unref(G_OBJECT(pbuf));
	}
	if (home){
		gchar *tmp;
		GtkWidget *menu_item=linphone_gtk_get_widget(w,"home_item");
		tmp=g_strdup(home);
		g_object_set_data(G_OBJECT(menu_item),"home",tmp);
	}
	if (!linphone_gtk_can_manage_accounts())
		gtk_widget_hide(linphone_gtk_get_widget(w,"run_assistant"));
}

void linphone_gtk_manage_login(void){
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneProxyConfig *cfg=NULL;
	linphone_core_get_default_proxy(lc,&cfg);
	if (cfg){
		SipSetup *ss=linphone_proxy_config_get_sip_setup(cfg);
		if (ss && (sip_setup_get_capabilities(ss) & SIP_SETUP_CAP_LOGIN)){
			linphone_gtk_show_login_frame(cfg);
		}
	}
}

static void linphone_gtk_init_main_window(){
	linphone_gtk_configure_main_window();
	linphone_gtk_manage_login();
	load_uri_history();
	linphone_gtk_load_identities();
	linphone_gtk_set_my_presence(linphone_core_get_presence_info(linphone_gtk_get_core()));
	linphone_gtk_show_friends();
	linphone_gtk_connect_digits();
	linphone_gtk_check_menu_items();
	linphone_gtk_enable_mute_button(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(linphone_gtk_get_main_window(),
					"main_mute")),FALSE);
	linphone_gtk_enable_mute_button(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(linphone_gtk_get_main_window(),
					"incall_mute")),FALSE);
	if (linphone_core_in_call(linphone_gtk_get_core())) linphone_gtk_call_started(
		linphone_gtk_get_main_window());/*hide the call button, show terminate button*/
}

void linphone_gtk_close(){
	/* couldn't find a way to prevent closing to destroy the main window*/
	LinphoneCore *lc=linphone_gtk_get_core();
	the_ui=NULL;
	the_ui=linphone_gtk_create_window("main");
	linphone_gtk_init_main_window();
	/*shutdown call if any*/
	if (linphone_core_in_call(lc)){
		linphone_core_terminate_call(lc,NULL);
		linphone_gtk_call_terminated(NULL);
	}
	linphone_core_enable_video_preview(lc,FALSE);
}

void linphone_gtk_log_handler(OrtpLogLevel lev, const char *fmt, va_list args){
	if (verbose){
		const char *lname="undef";
		char *msg;
		#ifdef __linux
		va_list cap;/*copy of our argument list: a va_list cannot be re-used (SIGSEGV on linux 64 bits)*/
		#endif
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
#ifdef __linux
		va_copy(cap,args);
		msg=g_strdup_vprintf(fmt,cap);
		va_end(cap);
#else
		msg=g_strdup_vprintf(fmt,args);
#endif
		fprintf(stdout,"linphone-%s : %s\n",lname,msg);
		ortp_free(msg);
	}
	linphone_gtk_log_push(lev,fmt,args);
}


static void linphone_gtk_refer_received(LinphoneCore *lc, const char *refer_to){
    GtkEntry * uri_bar =GTK_ENTRY(linphone_gtk_get_widget(
		linphone_gtk_get_main_window(), "uribar"));
	linphone_gtk_show_main_window();
	gtk_entry_set_text(uri_bar, refer_to);
	linphone_gtk_start_call(linphone_gtk_get_main_window());
}

int main(int argc, char *argv[]){
#ifdef ENABLE_NLS
	void *p;
#endif
	const char *config_file;
	const char *lang;

	g_thread_init(NULL);
	gdk_threads_init();
	
	config_file=linphone_gtk_get_config_file();

#ifdef WIN32
	if (workingdir!=NULL)
		_chdir(workingdir);

	/*workaround for windows: sometimes LANG is defined to an integer value, not understood by gtk */
	if ((lang=getenv("LANG"))!=NULL){
		if (atoi(lang)!=0){
			char tmp[128];
			snprintf(tmp,sizeof(tmp),"LANG=",lang);
			_putenv(tmp);
		}
	}
#endif
	
	if ((lang=linphone_gtk_get_lang(config_file))!=NULL && lang[0]!='\0'){
#ifdef WIN32
		char tmp[128];
		snprintf(tmp,sizeof(tmp),"LANG=%s",lang);
		_putenv(tmp);
#else
		setenv("LANG",lang,1);
#endif
	}

#ifdef ENABLE_NLS
	p=bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	if (p==NULL) perror("bindtextdomain failed");
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#else
	g_message("NLS disabled.\n");
#endif
#ifdef WIN32
	gtk_rc_add_default_file("./gtkrc");
#endif
	gdk_threads_enter();
	
	if (!gtk_init_with_args(&argc,&argv,_("A free SIP video-phone"),
				linphone_options,NULL,NULL)){
		gdk_threads_leave();
		return -1;
	}
	if (linphone_core_wake_up_possible_already_running_instance(
		config_file, addr_to_call) == 0){
		g_message("addr_to_call=%s",addr_to_call);
		g_warning("Another running instance of linphone has been detected. It has been woken-up.");
		g_warning("This instance is going to exit now.");
		gdk_threads_leave();
		return 0;
	}

	add_pixmap_directory("pixmaps");
	add_pixmap_directory(PACKAGE_DATA_DIR "/pixmaps/linphone");

	the_ui=linphone_gtk_create_window("main");
	
	linphone_gtk_create_log_window();
	linphone_core_enable_logs_with_cb(linphone_gtk_log_handler);

	linphone_gtk_init_liblinphone(config_file);
	/* do not lower timeouts under 30 ms because it exhibits a bug on gtk+/win32, with cpu running 20% all the time...*/
	gtk_timeout_add(30,(GtkFunction)linphone_gtk_iterate,(gpointer)linphone_gtk_get_core());
	gtk_timeout_add(30,(GtkFunction)linphone_gtk_check_logs,(gpointer)NULL);
	linphone_gtk_init_main_window();
	linphone_gtk_init_status_icon();
	if (!iconified)
		linphone_gtk_show_main_window();
	linphone_gtk_check_for_new_version();

	gtk_main();
	gdk_threads_leave();
	linphone_gtk_destroy_log_window();
	linphone_core_destroy(the_core);
	/*workaround a bug on win32 that makes status icon still present in the systray even after program exit.*/
	gtk_status_icon_set_visible(icon,FALSE);
	return 0;
}
