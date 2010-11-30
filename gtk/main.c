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

//#define USE_LIBGLADE 1

#define VIDEOSELFVIEW_DEFAULT 1

#include "linphone.h"
#include "lpconfig.h"



#ifdef USE_LIBGLADE
#include <glade/glade.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define LINPHONE_ICON "linphone.png"

const char *this_program_ident_string="linphone_ident_string=" LINPHONE_VERSION;

static LinphoneCore *the_core=NULL;
static GtkWidget *the_ui=NULL;

static void linphone_gtk_registration_state_changed(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState rs, const char *msg);
static void linphone_gtk_show(LinphoneCore *lc);
static void linphone_gtk_notify_recv(LinphoneCore *lc, LinphoneFriend * fid);
static void linphone_gtk_new_unknown_subscriber(LinphoneCore *lc, LinphoneFriend *lf, const char *url);
static void linphone_gtk_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username);
static void linphone_gtk_display_status(LinphoneCore *lc, const char *status);
static void linphone_gtk_display_message(LinphoneCore *lc, const char *msg);
static void linphone_gtk_display_warning(LinphoneCore *lc, const char *warning);
static void linphone_gtk_display_url(LinphoneCore *lc, const char *msg, const char *url);
static void linphone_gtk_call_log_updated(LinphoneCore *lc, LinphoneCallLog *cl);
static void linphone_gtk_refer_received(LinphoneCore *lc, const char  *refer_to);
static void linphone_gtk_call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cs, const char *msg);
static gboolean linphone_gtk_auto_answer(LinphoneCall *call);


static gboolean verbose=0;
static gboolean auto_answer = 0;
static gchar * addr_to_call = NULL;
static gboolean iconified=FALSE;
#ifdef WIN32
static gchar *workingdir=NULL;
#endif
static char *progpath=NULL;

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
	{
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
#define BUILD_TREE_XML_DIR "gtk"

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


#define FACTORY_CONFIG_FILE "linphonerc.factory"
static char _factory_config_file[1024];
static const char *linphone_gtk_get_factory_config_file(){
	/*try accessing a local file first if exists*/
	if (access(FACTORY_CONFIG_FILE,F_OK)==0){
		snprintf(_factory_config_file,sizeof(_factory_config_file),
						 "%s",FACTORY_CONFIG_FILE);
	} else {
		char *progdir;
		
		if (progpath != NULL) {
			char *basename;
			progdir = strdup(progpath);
#ifdef WIN32
			basename = strrchr(progdir, '\\');
			if (basename != NULL) {
				basename ++;
				*basename = '\0';
				snprintf(_factory_config_file, sizeof(_factory_config_file),
								 "%s\\..\\%s", progdir, FACTORY_CONFIG_FILE);
			} else {
				if (workingdir!=NULL) {
					snprintf(_factory_config_file, sizeof(_factory_config_file),
									 "%s\\%s", workingdir, FACTORY_CONFIG_FILE);
				} else {
					free(progdir);
					return NULL;
				}
			}
#else
			basename = strrchr(progdir, '/');
			if (basename != NULL) {
				basename ++;
				*basename = '\0';
				snprintf(_factory_config_file, sizeof(_factory_config_file),
								 "%s/../share/Linphone/%s", progdir, FACTORY_CONFIG_FILE);
			} else {
				free(progdir);
				return NULL;
			}
#endif
			free(progdir);
		}
	}
	return _factory_config_file;
}

static void linphone_gtk_init_liblinphone(const char *config_file,
		const char *factory_config_file) {
	LinphoneCoreVTable vtable={0};

	vtable.call_state_changed=linphone_gtk_call_state_changed;
	vtable.registration_state_changed=linphone_gtk_registration_state_changed;
	vtable.show=linphone_gtk_show;
	vtable.notify_presence_recv=linphone_gtk_notify_recv;
	vtable.new_subscription_request=linphone_gtk_new_unknown_subscriber;
	vtable.auth_info_requested=linphone_gtk_auth_info_requested;
	vtable.display_status=linphone_gtk_display_status;
	vtable.display_message=linphone_gtk_display_message;
	vtable.display_warning=linphone_gtk_display_warning;
	vtable.display_url=linphone_gtk_display_url;
	vtable.call_log_updated=linphone_gtk_call_log_updated;
	vtable.text_received=linphone_gtk_text_received;
	vtable.refer_received=linphone_gtk_refer_received;
	vtable.buddy_info_updated=linphone_gtk_buddy_info_updated;

	linphone_core_set_user_agent("Linphone", LINPHONE_VERSION);
	the_core=linphone_core_new(&vtable,config_file,factory_config_file,NULL);
	linphone_core_set_waiting_callback(the_core,linphone_gtk_wait,NULL);
}



LinphoneCore *linphone_gtk_get_core(void){
	return the_core;
}

GtkWidget *linphone_gtk_get_main_window(){
	return the_ui;
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
		icon_path=linphone_gtk_get_ui_config("icon",LINPHONE_ICON);
		config_loaded=TRUE;
	}
	if (hiddens)
		linphone_gtk_visibility_set(hiddens,window_name,w,FALSE);
	if (shown)
		linphone_gtk_visibility_set(shown,window_name,w,TRUE);
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

static int get_ui_file(const char *name, char *path, int pathsize){
	snprintf(path,pathsize,"%s/%s.ui",BUILD_TREE_XML_DIR,name);
	if (access(path,F_OK)!=0){
		snprintf(path,pathsize,"%s/%s.ui",INSTALLED_XML_DIR,name);
		if (access(path,F_OK)!=0){
			g_error("Could not locate neither %s/%s.ui and %s/%s.ui .",BUILD_TREE_XML_DIR,name,
				INSTALLED_XML_DIR,name);
			return -1;
		}
	}
	return 0;
}

GtkWidget *linphone_gtk_create_window(const char *window_name){
	GError* error = NULL;
	GtkBuilder* builder = gtk_builder_new ();
	char path[512];
	GtkWidget *w;

	if (get_ui_file(window_name,path,sizeof(path))==-1) return NULL;
	
	if (!gtk_builder_add_from_file (builder, path, &error)){
		g_error("Couldn't load builder file: %s", error->message);
		g_error_free (error);
		return NULL;
	}
	w=GTK_WIDGET(gtk_builder_get_object (builder,window_name));
	if (w==NULL){
		g_error("Could not retrieve '%s' window from xml file",window_name);
		return NULL;
	}
	g_object_set_data(G_OBJECT(w),"builder",builder);
	gtk_builder_connect_signals(builder,w);
	linphone_gtk_configure_window(w,window_name);
	return w;
}

GtkWidget *linphone_gtk_create_widget(const char *filename, const char *widget_name){
	char path[2048];
	GtkWidget *w;
	GtkBuilder* builder = gtk_builder_new ();
	GError *error=NULL;
	gchar *object_ids[2];
	object_ids[0]=g_strdup(widget_name);
	object_ids[1]=NULL;
	
	if (get_ui_file(filename,path,sizeof(path))==-1) return NULL;
	if (!gtk_builder_add_objects_from_file(builder,path,object_ids,&error)){
		g_error("Couldn't load %s from builder file %s: %s", widget_name,path,error->message);
		g_error_free (error);
		g_free(object_ids[0]);
		return NULL;
	}
	g_free(object_ids[0]);
	w=GTK_WIDGET(gtk_builder_get_object (builder,widget_name));
	if (w==NULL){
		g_error("Could not retrieve '%s' window from xml file",widget_name);
		return NULL;
	}
	g_object_set_data(G_OBJECT(w),"builder",builder);
	gtk_builder_connect_signals(builder,w);
	return w;
}

GtkWidget *linphone_gtk_get_widget(GtkWidget *window, const char *name){
	GtkBuilder *builder=(GtkBuilder*)g_object_get_data(G_OBJECT(window),"builder");
	GObject *w;
	if (builder==NULL){
		g_error("Fail to retrieve builder from window !");
		return NULL;
	}
	w=gtk_builder_get_object(builder,name);
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
	const char *tmp;
	GdkPixbuf *logo=create_pixbuf(
	    linphone_gtk_get_ui_config("logo","linphone-banner.png"));
	static const char *defcfg="defcfg";
	
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
	if (logo)	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about),logo);
	tmp=linphone_gtk_get_ui_config("artists",defcfg);
	if (tmp!=defcfg){
		const char *tmp2[2];
		tmp2[0]=tmp;
		tmp2[1]=NULL;
		gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(about),tmp2);
	}
	tmp=linphone_gtk_get_ui_config("translators",defcfg);
	if (tmp!=defcfg)
		gtk_about_dialog_set_translator_credits (GTK_ABOUT_DIALOG(about),tmp);
	tmp=linphone_gtk_get_ui_config("comments",defcfg);
	if (tmp!=defcfg)
		gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about),tmp);
	gtk_widget_show(about);
}

static void set_video_window_decorations(GdkWindow *w){
	const char *title=linphone_gtk_get_ui_config("title","Linphone");
	const char *icon_path=linphone_gtk_get_ui_config("icon",LINPHONE_ICON);
	char video_title[256];
	GdkPixbuf *pbuf=create_pixbuf(icon_path);
	if (!linphone_core_in_call(linphone_gtk_get_core())){
		snprintf(video_title,sizeof(video_title),"%s video",title);
		/* When not in call, treat the video as a normal window */
		gdk_window_set_keep_above(w, FALSE);
	}else{
		LinphoneAddress *uri =
			linphone_address_clone(linphone_core_get_current_call_remote_address(linphone_gtk_get_core()));
		char *display_name;

		linphone_address_clean(uri);
		if (linphone_address_get_display_name(uri)!=NULL){
			display_name=ms_strdup(linphone_address_get_display_name(uri));
		}else{
			display_name=linphone_address_as_string(uri);
		}
		snprintf(video_title,sizeof(video_title),_("Call with %s"),display_name);
		linphone_address_destroy(uri);
		ms_free(display_name);

		/* During calls, bring up the video window, arrange so that
		it is above all the other windows */
		gdk_window_deiconify(w);
		gdk_window_set_keep_above(w,TRUE);
		/* Maybe we should have the following, but then we want to
		have a timer that turns it off after a little while. */
		/* gdk_window_set_urgency_hint(w,TRUE); */
	}
	gdk_window_set_title(w,video_title);
	/* Refrain the video window to be closed at all times. */
	gdk_window_set_functions(w,
				 GDK_FUNC_RESIZE|GDK_FUNC_MOVE|
				 GDK_FUNC_MINIMIZE|GDK_FUNC_MAXIMIZE);
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
	static unsigned long preview_previd=0;
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
	id=linphone_core_get_native_preview_window_id (lc);
	if (id!=preview_previd ){
		GdkWindow *w;
		preview_previd=id;
		if (id!=0){
			ms_message("Updating window decorations for preview");
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

void linphone_gtk_call_terminated(LinphoneCall *call, const char *error){
	GtkWidget *mw=linphone_gtk_get_main_window();
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"terminate_call"),FALSE);
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"start_call"),TRUE);
	
	if (linphone_gtk_use_in_call_view() && call)
		linphone_gtk_in_call_view_terminate(call,error);
	update_video_title();
}

static gboolean in_call_timer(){
	LinphoneCall *call=linphone_core_get_current_call(linphone_gtk_get_core());
	if (call){
		linphone_gtk_in_call_view_update_duration(call);
		return TRUE;
	}
	return FALSE;
}

static bool_t all_other_calls_paused(LinphoneCall *refcall, const MSList *calls){
	for(;calls!=NULL;calls=calls->next){
		LinphoneCall *call=(LinphoneCall*)calls->data;
		LinphoneCallState cs=linphone_call_get_state(call);
		if (refcall!=call){
			if (cs!=LinphoneCallPaused  && cs!=LinphoneCallPausing)
				return FALSE;
		}
	}
	return TRUE;
}

static void linphone_gtk_update_call_buttons(LinphoneCall *call){
	LinphoneCore *lc=linphone_gtk_get_core();
	GtkWidget *mw=linphone_gtk_get_main_window();
	const MSList *calls=linphone_core_get_calls(lc);
	GtkWidget *button;
	bool_t start_active=TRUE;
	bool_t stop_active=FALSE;
	bool_t add_call=FALSE;
	
	if (calls==NULL){
		start_active=TRUE;
		stop_active=FALSE;
	}else{
		stop_active=TRUE;
		if (all_other_calls_paused(NULL,calls)){
			start_active=TRUE;
			add_call=TRUE;
		}else if (call!=NULL && linphone_call_get_state(call)==LinphoneCallIncomingReceived && all_other_calls_paused(call,calls)){
			if (ms_list_size(calls)>1){
				start_active=TRUE;
				add_call=TRUE;
			}else{
				start_active=TRUE;
				add_call=FALSE;
			}
		}else{
			start_active=FALSE;
		}
	}
	button=linphone_gtk_get_widget(mw,"start_call");
	gtk_widget_set_sensitive(button,start_active);
	gtk_widget_set_visible(button,!add_call);
	
	button=linphone_gtk_get_widget(mw,"add_call");
	gtk_widget_set_sensitive(button,start_active);
	gtk_widget_set_visible(button,add_call);
	
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"terminate_call"),stop_active);
	if (linphone_core_get_calls(lc)==NULL){
		linphone_gtk_enable_mute_button(
				GTK_BUTTON(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"main_mute")),
			FALSE);
	}
	update_video_title();
}

static gboolean linphone_gtk_start_call_do(GtkWidget *uri_bar){
	const char *entered=gtk_entry_get_text(GTK_ENTRY(uri_bar));
	if (linphone_core_invite(linphone_gtk_get_core(),entered)!=NULL) {
		completion_add_text(GTK_ENTRY(uri_bar),entered);
	}else{
		linphone_gtk_call_terminated(NULL,NULL);
	}
	return FALSE;
}

static gboolean linphone_gtk_auto_answer(LinphoneCall *call){
	if (linphone_call_get_state(call)==LinphoneCallIncomingReceived){
		linphone_core_accept_call (linphone_gtk_get_core(),call);
		linphone_call_unref(call);
	}
	return FALSE;
}


void linphone_gtk_start_call(GtkWidget *w){
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneCall *call;
	/*change into in-call mode, then do the work later as it might block a bit */
	GtkWidget *mw=gtk_widget_get_toplevel(w);
	GtkWidget *uri_bar=linphone_gtk_get_widget(mw,"uribar");

	call=linphone_gtk_get_currently_displayed_call ();
	if (call!=NULL && linphone_call_get_state(call)==LinphoneCallIncomingReceived){
		linphone_core_accept_call(lc,call);
	}else{
		/*immediately disable the button and delay a bit the execution the linphone_core_invite()
		so that we don't freeze the button. linphone_core_invite() might block for some hundreds of milliseconds*/
		gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"start_call"),FALSE);
		g_timeout_add(100,(GSourceFunc)linphone_gtk_start_call_do,uri_bar);
	}
	
}

void linphone_gtk_uri_bar_activate(GtkWidget *w){
	linphone_gtk_start_call(w);
}


void linphone_gtk_terminate_call(GtkWidget *button){
	LinphoneCall *call=linphone_gtk_get_currently_displayed_call ();
	if (call)
		linphone_core_terminate_call(linphone_gtk_get_core(),call);
}

void linphone_gtk_decline_clicked(GtkWidget *button){
	LinphoneCall *call=linphone_gtk_get_currently_displayed_call ();
	if (call)
		linphone_core_terminate_call(linphone_gtk_get_core(),call);
}

void linphone_gtk_answer_clicked(GtkWidget *button){
	LinphoneCall *call=linphone_gtk_get_currently_displayed_call ();
	if (call){
		linphone_core_pause_all_calls(linphone_gtk_get_core());
		linphone_core_accept_call(linphone_gtk_get_core(),call);
	}
}

void linphone_gtk_set_audio_video(){
	linphone_core_enable_video(linphone_gtk_get_core(),TRUE,TRUE);
	linphone_core_enable_video_preview(linphone_gtk_get_core(),
	    linphone_gtk_get_ui_config_int("videoselfview",VIDEOSELFVIEW_DEFAULT));
}

void linphone_gtk_set_audio_only(){
	linphone_core_enable_video(linphone_gtk_get_core(),FALSE,FALSE);
	linphone_core_enable_video_preview(linphone_gtk_get_core(),FALSE);
}

void linphone_gtk_enable_self_view(GtkWidget *w){
	gboolean val=gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	LinphoneCore *lc=linphone_gtk_get_core();
	linphone_core_enable_video_preview(lc,val);
	linphone_core_enable_self_view(lc,val);
	linphone_gtk_set_ui_config_int("videoselfview",val);
}

void linphone_gtk_used_identity_changed(GtkWidget *w){
	int active=gtk_combo_box_get_active(GTK_COMBO_BOX(w));
	char *sel=gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	if (sel && strlen(sel)>0){ //avoid a dummy "changed" at gui startup
		linphone_core_set_default_proxy_index(linphone_gtk_get_core(),(active==0) ? -1 : (active-1));
		linphone_gtk_show_directory_search();
	}
	if (sel) g_free(sel);
}

static void linphone_gtk_show_main_window(){
	GtkWidget *w=linphone_gtk_get_main_window();
	LinphoneCore *lc=linphone_gtk_get_core();
	if (linphone_core_video_enabled(lc)){
		linphone_core_enable_video_preview(lc,linphone_gtk_get_ui_config_int("videoselfview",
	    	VIDEOSELFVIEW_DEFAULT));
	}
	gtk_widget_show(w);
	gtk_window_present(GTK_WINDOW(w));
}

static void linphone_gtk_show(LinphoneCore *lc){
	linphone_gtk_show_main_window();
}

static void linphone_gtk_notify_recv(LinphoneCore *lc, LinphoneFriend * fid){
	linphone_gtk_show_friends();
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

	if (linphone_gtk_get_ui_config_int("subscribe_deny_all",0)){
		linphone_core_reject_subscriber(linphone_gtk_get_core(),lf);
		return;
	}

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
	GtkWidget *mw=linphone_gtk_get_main_window();
	
	if (mw && GTK_WIDGET_VISIBLE(linphone_gtk_get_widget(mw,"login_frame"))){
		/*don't prompt for authentication when login frame is visible*/
		linphone_core_abort_authentication(lc,NULL);
		return;
	}

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

static void linphone_gtk_call_log_updated(LinphoneCore *lc, LinphoneCallLog *cl){
	GtkWidget *w=(GtkWidget*)g_object_get_data(G_OBJECT(linphone_gtk_get_main_window()),"call_logs");
	if (w) linphone_gtk_call_log_update(w);
}

static void linphone_gtk_call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cs, const char *msg){
	switch(cs){
		case LinphoneCallOutgoingInit:
			linphone_gtk_create_in_call_view (call);
		break;
		case LinphoneCallOutgoingProgress:
			linphone_gtk_in_call_view_set_calling (call);
		break;
		case LinphoneCallStreamsRunning:
			linphone_gtk_in_call_view_set_in_call(call);
			linphone_gtk_enable_mute_button(
				GTK_BUTTON(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"main_mute")),
			TRUE);
			g_timeout_add(250,(GSourceFunc)in_call_timer,NULL);
		break;
		case LinphoneCallError:
			linphone_gtk_in_call_view_terminate (call,msg);
		break;
		case LinphoneCallEnd:
			linphone_gtk_in_call_view_terminate(call,NULL);
		break;
		case LinphoneCallIncomingReceived:
			linphone_gtk_create_in_call_view (call);
			linphone_gtk_in_call_view_set_incoming(call,!all_other_calls_paused (call,linphone_core_get_calls(lc)));
			if (auto_answer)  {
				linphone_call_ref(call);
				g_timeout_add(2000,(GSourceFunc)linphone_gtk_auto_answer ,call);
			}		
		break;
		case LinphoneCallResuming:
			linphone_gtk_enable_hold_button(call,TRUE,TRUE);
			linphone_gtk_in_call_view_set_in_call (call);
		break;
		case LinphoneCallPausing:
			linphone_gtk_enable_hold_button(call,TRUE,FALSE);
		case LinphoneCallPausedByRemote:
			linphone_gtk_in_call_view_set_paused(call);
		break;
		case LinphoneCallConnected:
			linphone_gtk_enable_hold_button (call,TRUE,TRUE);
		break;
		default:
		break;
	}
	linphone_gtk_update_call_buttons (call);
}

static void linphone_gtk_registration_state_changed(LinphoneCore *lc, LinphoneProxyConfig *cfg, 
                                                    LinphoneRegistrationState rs, const char *msg){
	switch (rs){
		case LinphoneRegistrationOk:
			if (cfg){
				SipSetup *ss=linphone_proxy_config_get_sip_setup(cfg);
				if (ss && (sip_setup_get_capabilities(ss) & SIP_SETUP_CAP_LOGIN)){
					linphone_gtk_exit_login_frame();
				}
			}
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
#ifndef WIN32
#if GTK_CHECK_VERSION(2,18,3)
	gtk_show_uri(NULL,url,GDK_CURRENT_TIME,NULL);
#else
	char cl[255];
	snprintf(cl,sizeof(cl),"/usr/bin/x-www-browser %s",url);
	g_spawn_command_line_async(cl,NULL);
#endif
#else /*WIN32*/
	ShellExecute(0,"open",url,NULL,NULL,1);
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

static void linphone_gtk_dtmf_pressed(GtkButton *button){
	const char *label=gtk_button_get_label(button);
	GtkWidget *uri_bar=linphone_gtk_get_widget(gtk_widget_get_toplevel(GTK_WIDGET(button)),"uribar");
	int pos=-1;
	gtk_editable_insert_text(GTK_EDITABLE(uri_bar),label,1,&pos);
	linphone_core_play_dtmf (linphone_gtk_get_core(),label[0],-1);
	if (linphone_core_in_call(linphone_gtk_get_core())){
		linphone_core_send_dtmf(linphone_gtk_get_core(),label[0]);
	}
}

static void linphone_gtk_dtmf_released(GtkButton *button){
	linphone_core_stop_dtmf (linphone_gtk_get_core());
}

static void linphone_gtk_connect_digits(void){
	GtkContainer *cont=GTK_CONTAINER(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"dtmf_table"));
	GList *children=gtk_container_get_children(cont);
	GList *elem;
	for(elem=children;elem!=NULL;elem=elem->next){
		GtkButton *button=GTK_BUTTON(elem->data);
		g_signal_connect(G_OBJECT(button),"pressed",(GCallback)linphone_gtk_dtmf_pressed,NULL);
		g_signal_connect(G_OBJECT(button),"released",(GCallback)linphone_gtk_dtmf_released,NULL);
	}
}

static void linphone_gtk_check_menu_items(void){
	bool_t audio_only=!linphone_core_video_enabled(linphone_gtk_get_core());
	bool_t selfview=linphone_gtk_get_ui_config_int("videoselfview",VIDEOSELFVIEW_DEFAULT);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(linphone_gtk_get_widget(
					linphone_gtk_get_main_window(),
					audio_only ? "audio_only_item" : "video_item")), TRUE);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(linphone_gtk_get_widget(
					linphone_gtk_get_main_window(),"selfview_item")),selfview);
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
	static const char *add_call_icon;
	static const char *stop_call_icon;
	static const char *search_icon;
	static gboolean update_check_menu;
	static gboolean buttons_have_borders;
	static gboolean show_abcd;
	GtkWidget *w=linphone_gtk_get_main_window();
	if (!config_loaded){
		title=linphone_gtk_get_ui_config("title","Linphone");
		home=linphone_gtk_get_ui_config("home","http://www.linphone.org");
		start_call_icon=linphone_gtk_get_ui_config("start_call_icon","startcall-green.png");
		add_call_icon=linphone_gtk_get_ui_config("add_call_icon","addcall-green.png");
		stop_call_icon=linphone_gtk_get_ui_config("stop_call_icon","stopcall-red.png");
		search_icon=linphone_gtk_get_ui_config("directory_search_icon",NULL);
		update_check_menu=linphone_gtk_get_ui_config_int("update_check_menu",0);
		buttons_have_borders=linphone_gtk_get_ui_config_int("buttons_border",1);
		show_abcd=linphone_gtk_get_ui_config_int("show_abcd",1);
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
		gtk_button_set_image(GTK_BUTTON(linphone_gtk_get_widget(w,"start_call")),
		                    create_pixmap (start_call_icon));
		if (!buttons_have_borders)
			gtk_button_set_relief(GTK_BUTTON(linphone_gtk_get_widget(w,"start_call")),GTK_RELIEF_NONE);
	}
	if (add_call_icon){
		gtk_button_set_image(GTK_BUTTON(linphone_gtk_get_widget(w,"add_call")),
		                    create_pixmap (add_call_icon));
		if (!buttons_have_borders)
			gtk_button_set_relief(GTK_BUTTON(linphone_gtk_get_widget(w,"add_call")),GTK_RELIEF_NONE);
	}
	if (stop_call_icon){
		gtk_button_set_image(GTK_BUTTON(linphone_gtk_get_widget(w,"terminate_call")),
		                    create_pixmap (stop_call_icon));
		if (!buttons_have_borders)
			gtk_button_set_relief(GTK_BUTTON(linphone_gtk_get_widget(w,"terminate_call")),GTK_RELIEF_NONE);
	}
	if (search_icon){
		GdkPixbuf *pbuf=create_pixbuf(search_icon);
		gtk_image_set_from_pixbuf(GTK_IMAGE(linphone_gtk_get_widget(w,"directory_search_button_icon")),pbuf);
		g_object_unref(G_OBJECT(pbuf));
	}
	if (home){
		gchar *tmp;
		GtkWidget *menu_item=linphone_gtk_get_widget(w,"home_item");
		tmp=g_strdup(home);
		g_object_set_data(G_OBJECT(menu_item),"home",tmp);
	}
	{
		GdkPixbuf *pbuf=create_pixbuf("contact-orange.png");
		if (pbuf) {
			gtk_image_set_from_pixbuf(GTK_IMAGE(linphone_gtk_get_widget(w,"contact_tab_icon")),pbuf);
			g_object_unref(G_OBJECT(pbuf));
		}
	}
	{
		GdkPixbuf *pbuf=create_pixbuf("dialer-orange.png");
		if (pbuf) {
			gtk_image_set_from_pixbuf(GTK_IMAGE(linphone_gtk_get_widget(w,"keypad_tab_icon")),pbuf);
			g_object_unref(G_OBJECT(pbuf));
		}
	}
	if (linphone_gtk_can_manage_accounts())
		gtk_widget_show(linphone_gtk_get_widget(w,"assistant_item"));
	if (update_check_menu){
		gtk_widget_show(linphone_gtk_get_widget(w,"versioncheck_item"));
	}
	if (!show_abcd){
		gtk_widget_hide(linphone_gtk_get_widget(w,"dtmf_A"));
		gtk_widget_hide(linphone_gtk_get_widget(w,"dtmf_B"));
		gtk_widget_hide(linphone_gtk_get_widget(w,"dtmf_C"));
		gtk_widget_hide(linphone_gtk_get_widget(w,"dtmf_D"));
		gtk_table_resize(GTK_TABLE(linphone_gtk_get_widget(w,"dtmf_table")),4,3);
	}
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


gboolean linphone_gtk_close(GtkWidget *mw){
	/*shutdown calls if any*/
	LinphoneCore *lc=linphone_gtk_get_core();
	if (linphone_core_in_call(lc)){
		linphone_core_terminate_all_calls(lc);
	}
	linphone_core_enable_video_preview(lc,FALSE);
	gtk_widget_hide(mw);
	return TRUE;
}

static void linphone_gtk_init_main_window(){
	GtkWidget *main_window;

	linphone_gtk_configure_main_window();
	linphone_gtk_manage_login();
	load_uri_history();
	linphone_gtk_load_identities();
	linphone_gtk_set_my_presence(linphone_core_get_presence_info(linphone_gtk_get_core()));
	linphone_gtk_show_friends();
	linphone_gtk_connect_digits();
	linphone_gtk_check_menu_items();
	main_window=linphone_gtk_get_main_window();
	linphone_gtk_enable_mute_button(GTK_BUTTON(linphone_gtk_get_widget(main_window,
					"main_mute")),FALSE);
	if (!linphone_gtk_use_in_call_view()) {
		gtk_widget_show(linphone_gtk_get_widget(main_window, "main_mute"));
	}
	linphone_gtk_update_call_buttons (NULL);
	/*prevent the main window from being destroyed by a user click on WM controls, instead we hide it*/
	g_signal_connect (G_OBJECT (main_window), "delete-event",
		G_CALLBACK (linphone_gtk_close), main_window);
}


void linphone_gtk_log_handler(OrtpLogLevel lev, const char *fmt, va_list args){
	if (verbose){
		const char *lname="undef";
		char *msg;
	#if defined(__linux) || defined(__APPLE__)
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
#if defined(__linux) || defined(__APPLE__)
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

static void linphone_gtk_check_soundcards(){
	const char **devices=linphone_core_get_sound_devices(linphone_gtk_get_core());
	if (devices==NULL || devices[0]==NULL){
		linphone_gtk_display_something(GTK_MESSAGE_WARNING,
		    _("No sound cards have been detected on this computer.\n"
			    "You won't be able to send or receive audio calls."));
	}
}

int main(int argc, char *argv[]){
#ifdef ENABLE_NLS
	void *p;
#endif
	const char *config_file;
	const char *factory_config_file;
	const char *lang;
	GtkSettings *settings;
	GdkPixbuf *pbuf;

	g_thread_init(NULL);
	gdk_threads_init();
	
	progpath = strdup(argv[0]);
	
	config_file=linphone_gtk_get_config_file();

#ifdef WIN32
	/*workaround for windows: sometimes LANG is defined to an integer value, not understood by gtk */
	if ((lang=getenv("LANG"))!=NULL){
		if (atoi(lang)!=0){
			char tmp[128];
			snprintf(tmp,sizeof(tmp),"LANG=",lang);
			_putenv(tmp);
		}
	}
#else
	/*for pulseaudio:*/
	g_setenv("PULSE_PROP_media.role", "phone", TRUE);
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
	
	settings=gtk_settings_get_default();
	g_type_class_unref (g_type_class_ref (GTK_TYPE_IMAGE_MENU_ITEM));
	g_object_set(settings, "gtk-menu-images", TRUE, NULL);
	g_object_set(settings, "gtk-button-images", TRUE, NULL);
#ifdef WIN32
	if (workingdir!=NULL)
		_chdir(workingdir);
#endif
	/* Now, look for the factory configuration file, we do it this late
		 since we want to have had time to change directory and to parse
		 the options, in case we needed to access the working directory */
	factory_config_file = linphone_gtk_get_factory_config_file();

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

	linphone_gtk_init_liblinphone(config_file, factory_config_file);
	
	g_set_application_name(linphone_gtk_get_ui_config("title","Linphone"));
	pbuf=create_pixbuf(linphone_gtk_get_ui_config("icon",LINPHONE_ICON));
	if (pbuf!=NULL) gtk_window_set_default_icon(pbuf);
	
	/* do not lower timeouts under 30 ms because it exhibits a bug on gtk+/win32, with cpu running 20% all the time...*/
	gtk_timeout_add(30,(GtkFunction)linphone_gtk_iterate,(gpointer)linphone_gtk_get_core());
	gtk_timeout_add(30,(GtkFunction)linphone_gtk_check_logs,(gpointer)NULL);
	linphone_gtk_init_main_window();
	linphone_gtk_init_status_icon();
	if (!iconified){
		linphone_gtk_show_main_window();
		linphone_gtk_check_soundcards();
	}
	if (linphone_gtk_get_ui_config_int("update_check_menu",0)==0)
		linphone_gtk_check_for_new_version();

	gtk_main();
	gdk_threads_leave();
	linphone_gtk_destroy_log_window();
	linphone_core_destroy(the_core);
	/*workaround a bug on win32 that makes status icon still present in the systray even after program exit.*/
	gtk_status_icon_set_visible(icon,FALSE);
	free(progpath);
	return 0;
}
