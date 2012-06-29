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


#define VIDEOSELFVIEW_DEFAULT 0

#include "linphone.h"
#include "lpconfig.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_GTK_OSX
#include <gtkosxapplication.h>
#endif

#ifdef WIN32
#define chdir _chdir
#endif

#if defined(HAVE_NOTIFY1) || defined(HAVE_NOTIFY4)
#define HAVE_NOTIFY
#endif

#ifdef HAVE_NOTIFY
#include <libnotify/notify.h>
#endif

#define LINPHONE_ICON "linphone.png"

const char *this_program_ident_string="linphone_ident_string=" LINPHONE_VERSION;

static LinphoneCore *the_core=NULL;
static GtkWidget *the_ui=NULL;
GtkWidget *the_wizard=NULL;

static void linphone_gtk_registration_state_changed(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState rs, const char *msg);
static void linphone_gtk_notify_recv(LinphoneCore *lc, LinphoneFriend * fid);
static void linphone_gtk_new_unknown_subscriber(LinphoneCore *lc, LinphoneFriend *lf, const char *url);
static void linphone_gtk_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username);
static void linphone_gtk_display_status(LinphoneCore *lc, const char *status);
static void linphone_gtk_display_message(LinphoneCore *lc, const char *msg);
static void linphone_gtk_display_warning(LinphoneCore *lc, const char *warning);
static void linphone_gtk_display_url(LinphoneCore *lc, const char *msg, const char *url);
static void linphone_gtk_call_log_updated(LinphoneCore *lc, LinphoneCallLog *cl);
static void linphone_gtk_call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cs, const char *msg);
static void linphone_gtk_call_encryption_changed(LinphoneCore *lc, LinphoneCall *call, bool_t enabled, const char *token);
static void linphone_gtk_transfer_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate);
static gboolean linphone_gtk_auto_answer(LinphoneCall *call);
static void linphone_gtk_status_icon_set_blinking(gboolean val);
void _linphone_gtk_enable_video(gboolean val);


static gboolean verbose=0;
static gboolean auto_answer = 0;
static gchar * addr_to_call = NULL;
static gboolean no_video=FALSE;
static gboolean iconified=FALSE;
static gchar *workingdir=NULL;
static char *progpath=NULL;
gchar *linphone_logfile=NULL;
static gboolean workaround_gtk_entry_chinese_bug=FALSE;

static GOptionEntry linphone_options[]={
	{
		.long_name="verbose",
		.short_name= '\0',
		.arg=G_OPTION_ARG_NONE,
		.arg_data= (gpointer)&verbose,
		.description=N_("log to stdout some debug information while running.")
	},
	{
	    .long_name = "logfile",
	    .short_name = 'l',
	    .arg = G_OPTION_ARG_STRING,
	    .arg_data = &linphone_logfile,
	    .description = N_("path to a file to write logs into.")
	},
	{
	    .long_name = "no-video",
	    .short_name = '\0',
	    .arg = G_OPTION_ARG_NONE,
	    .arg_data = (gpointer)&no_video,
	    .description = N_("Start linphone with video disabled.")
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
	{
	    .long_name = "workdir",
	    .short_name = '\0',
	    .arg = G_OPTION_ARG_STRING,
	    .arg_data = (gpointer) & workingdir,
	    .description = N_("Specifiy a working directory (should be the base of the installation, eg: c:\\Program Files\\Linphone)")
	},
	{0}
};

#define INSTALLED_XML_DIR PACKAGE_DATA_DIR "/linphone"
#define RELATIVE_XML_DIR 
#define BUILD_TREE_XML_DIR "gtk"

#ifndef WIN32
#define CONFIG_FILE ".linphonerc"
#define SECRETS_FILE ".linphone-zidcache"
#else
#define CONFIG_FILE "linphonerc"
#define SECRETS_FILE "linphone-zidcache"
#endif


char *linphone_gtk_get_config_file(const char *filename){
	const int path_max=1024;
	char *config_file=g_malloc0(path_max);
	if (filename==NULL) filename=CONFIG_FILE;
	/*try accessing a local file first if exists*/
	if (access(CONFIG_FILE,F_OK)==0){
		snprintf(config_file,path_max,"%s",filename);
	}else{
#ifdef WIN32
		const char *appdata=getenv("APPDATA");
		if (appdata){
			snprintf(config_file,path_max,"%s\\%s",appdata,LINPHONE_CONFIG_DIR);
			CreateDirectory(config_file,NULL);
			snprintf(config_file,path_max,"%s\\%s\\%s",appdata,LINPHONE_CONFIG_DIR,filename);
		}
#else
		const char *home=getenv("HOME");
		if (home==NULL) home=".";
		snprintf(config_file,path_max,"%s/%s",home,filename);
#endif
	}
	return config_file;
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
	gchar *secrets_file=linphone_gtk_get_config_file(SECRETS_FILE);

	vtable.call_state_changed=linphone_gtk_call_state_changed;
	vtable.registration_state_changed=linphone_gtk_registration_state_changed;
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
	vtable.call_encryption_changed=linphone_gtk_call_encryption_changed;
	vtable.transfer_state_changed=linphone_gtk_transfer_state_changed;

	linphone_core_set_user_agent("Linphone", LINPHONE_VERSION);
	the_core=linphone_core_new(&vtable,config_file,factory_config_file,NULL);
	linphone_core_set_waiting_callback(the_core,linphone_gtk_wait,NULL);
	linphone_core_set_zrtp_secrets_file(the_core,secrets_file);
	g_free(secrets_file);
	linphone_core_enable_video(the_core,TRUE,TRUE);
	if (no_video) {
		_linphone_gtk_enable_video(FALSE);
		linphone_gtk_set_ui_config_int("videoselfview",0);
	}
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

static int get_ui_file(const char *name, char *path, int pathsize){
	snprintf(path,pathsize,"%s/%s.ui",BUILD_TREE_XML_DIR,name);
	if (access(path,F_OK)!=0){
		snprintf(path,pathsize,"%s/%s.ui",INSTALLED_XML_DIR,name);
		if (access(path,F_OK)!=0){
			g_error("Could not locate neither %s/%s.ui nor %s/%s.ui",BUILD_TREE_XML_DIR,name,
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
	g_signal_connect_swapped(G_OBJECT(w),"destroy",(GCallback)g_object_unref,builder);
	gtk_builder_connect_signals(builder,w);
	return w;
}

static void entry_unmapped(GtkWidget *entry){
	g_message("Entry is unmapped, calling unrealize to workaround chinese bug.");
	gtk_widget_unrealize(entry);
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
	if (workaround_gtk_entry_chinese_bug){
		if (strcmp(G_OBJECT_TYPE_NAME(w),"GtkEntry")==0){
			if (g_object_get_data(G_OBJECT(w),"entry_bug_workaround")==NULL){
				g_object_set_data(G_OBJECT(w),"entry_bug_workaround",GINT_TO_POINTER(1));
				g_message("%s is a GtkEntry",name);
				g_signal_connect(G_OBJECT(w),"unmap",(GCallback)entry_unmapped,NULL);
			}
		}
	}
	return GTK_WIDGET(w);
}


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


bool_t linphone_gtk_video_enabled(void){
	const LinphoneVideoPolicy *vpol=linphone_core_get_video_policy(linphone_gtk_get_core());
	return vpol->automatically_accept && vpol->automatically_initiate;
}

void linphone_gtk_show_main_window(){
	GtkWidget *w=linphone_gtk_get_main_window();
	LinphoneCore *lc=linphone_gtk_get_core();
	linphone_core_enable_video_preview(lc,linphone_gtk_get_ui_config_int("videoselfview",
	    	VIDEOSELFVIEW_DEFAULT));
	gtk_widget_show(w);
	gtk_window_present(GTK_WINDOW(w));
}

void linphone_gtk_call_terminated(LinphoneCall *call, const char *error){
	GtkWidget *mw=linphone_gtk_get_main_window();
	if (linphone_core_get_calls(linphone_gtk_get_core())==NULL){
	    gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"terminate_call"),FALSE);
	    gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"start_call"),TRUE);
	}
	if (linphone_gtk_use_in_call_view() && call)
		linphone_gtk_in_call_view_terminate(call,error);
	update_video_title();
}

static void linphone_gtk_update_call_buttons(LinphoneCall *call){
	LinphoneCore *lc=linphone_gtk_get_core();
	GtkWidget *mw=linphone_gtk_get_main_window();
	const MSList *calls=linphone_core_get_calls(lc);
	GtkWidget *button;
	bool_t start_active=TRUE;
	bool_t stop_active=FALSE;
	bool_t add_call=FALSE;
	int call_list_size=ms_list_size(calls);
	
	if (calls==NULL){
		start_active=TRUE;
		stop_active=FALSE;
	}else{
		stop_active=TRUE;	
		start_active=TRUE;
		add_call=TRUE;
	}
	button=linphone_gtk_get_widget(mw,"start_call");
	gtk_widget_set_sensitive(button,start_active);
	gtk_widget_set_visible(button,!add_call);
	
	button=linphone_gtk_get_widget(mw,"add_call");
	if (linphone_core_sound_resources_locked(lc) || (call && linphone_call_get_state(call)==LinphoneCallIncomingReceived)) {
		gtk_widget_set_sensitive(button,FALSE);
	} else {
		gtk_widget_set_sensitive(button,start_active);
	}
	gtk_widget_set_visible(button,add_call);
	
	gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"terminate_call"),stop_active);

	linphone_gtk_enable_transfer_button(lc,call_list_size>1);
	linphone_gtk_enable_conference_button(lc,call_list_size>1);
	update_video_title();
	if (call) linphone_gtk_update_video_button(call);
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

	call=linphone_gtk_get_currently_displayed_call(NULL);
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
	gboolean is_conf;
	LinphoneCall *call=linphone_gtk_get_currently_displayed_call(&is_conf);
	if (call){
		linphone_core_terminate_call(linphone_gtk_get_core(),call);
	}else if (is_conf){
		linphone_core_terminate_conference(linphone_gtk_get_core());
	}
}

void linphone_gtk_decline_clicked(GtkWidget *button){
	LinphoneCall *call=linphone_gtk_get_currently_displayed_call(NULL);
	if (call)
		linphone_core_terminate_call(linphone_gtk_get_core(),call);
}

void linphone_gtk_answer_clicked(GtkWidget *button){
	LinphoneCall *call=linphone_gtk_get_currently_displayed_call(NULL);
	if (call){
		linphone_core_accept_call(linphone_gtk_get_core(),call);
		linphone_gtk_show_main_window(); /* useful when the button is clicked on a notification */
	}
}

void _linphone_gtk_enable_video(gboolean val){
	LinphoneVideoPolicy policy={0};
	policy.automatically_initiate=policy.automatically_accept=val;
	linphone_core_enable_video(linphone_gtk_get_core(),TRUE,TRUE);
	linphone_core_set_video_policy(linphone_gtk_get_core(),&policy);
	
	if (val){
		linphone_core_enable_video_preview(linphone_gtk_get_core(),
		linphone_gtk_get_ui_config_int("videoselfview",VIDEOSELFVIEW_DEFAULT));
	}else{
		linphone_core_enable_video_preview(linphone_gtk_get_core(),FALSE);
	}
}

void linphone_gtk_enable_video(GtkWidget *w){
	gboolean val=gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	//GtkWidget *selfview_item=linphone_gtk_get_widget(linphone_gtk_get_main_window(),"selfview_item");
	_linphone_gtk_enable_video(val);
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


void on_proxy_refresh_button_clicked(GtkWidget *w){
	LinphoneCore *lc=linphone_gtk_get_core();
	MSList const *item=linphone_core_get_proxy_config_list(lc);
	while (item != NULL) {
		LinphoneProxyConfig *lpc=(LinphoneProxyConfig*)item->data;
		linphone_proxy_config_edit(lpc);
		linphone_proxy_config_done(lpc);
		item = item->next;
	}
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
	linphone_gtk_call_log_update(linphone_gtk_get_main_window());
}

#ifdef HAVE_NOTIFY
static bool_t notify_actions_supported() {
	bool_t accepts_actions = FALSE;
	GList *capabilities = notify_get_server_caps();
	GList *c;
	if(capabilities != NULL) {
		for(c = capabilities; c != NULL; c = c->next) {
			if(strcmp((char*)c->data, "actions") == 0 ) {
				accepts_actions = TRUE;
				break;
			}
		}
		g_list_foreach(capabilities, (GFunc)g_free, NULL);
		g_list_free(capabilities);
	}
	return accepts_actions;
}

static NotifyNotification* build_notification(const char *title, const char *body){
	 return notify_notification_new(title,body,linphone_gtk_get_ui_config("icon",LINPHONE_ICON)
#ifdef HAVE_NOTIFY1
        ,NULL
#endif
	);
}

static void show_notification(NotifyNotification* n){
	if (n && !notify_notification_show(n,NULL))
		ms_error("Failed to send notification.");
}

static void make_notification(const char *title, const char *body){
	show_notification(build_notification(title,body));
}

#endif

static void linphone_gtk_notify(LinphoneCall *call, const char *msg){
#ifdef HAVE_NOTIFY
	if (!notify_is_initted())
		if (!notify_init ("Linphone")) ms_error("Libnotify failed to init.");
#endif
	if (!call) {
#ifdef HAVE_NOTIFY
		if (!notify_notification_show(notify_notification_new("Linphone",msg,NULL
#ifdef HAVE_NOTIFY1
	,NULL
#endif
),NULL))
				ms_error("Failed to send notification.");
#else
		linphone_gtk_show_main_window();
#endif
	} else if (!gtk_window_is_active((GtkWindow*)linphone_gtk_get_main_window())) {
#ifdef HAVE_NOTIFY
		char *body=NULL;
		char *remote=call!=NULL ? linphone_call_get_remote_address_as_string(call) : NULL;
		NotifyNotification *n;
		switch(linphone_call_get_state(call)){
			case LinphoneCallError:
				make_notification(_("Call error"),body=g_markup_printf_escaped("<span size=\"large\">%s</span>\n%s",msg,remote));
			break;
			case LinphoneCallEnd:
				make_notification(_("Call ended"),body=g_markup_printf_escaped("<span size=\"large\">%s</span>",remote));
			break;
			case LinphoneCallIncomingReceived:
				n=build_notification(_("Incoming call"),body=g_markup_printf_escaped("<span size=\"large\">%s</span>",remote));
				if (notify_actions_supported()) {
					notify_notification_add_action (n,"answer", _("Answer"),
						NOTIFY_ACTION_CALLBACK(linphone_gtk_answer_clicked),NULL,NULL);
					notify_notification_add_action (n,"decline",_("Decline"),
						NOTIFY_ACTION_CALLBACK(linphone_gtk_decline_clicked),NULL,NULL);
				}
				show_notification(n);
			break;
			case LinphoneCallPausedByRemote:
				make_notification(_("Call paused"),body=g_markup_printf_escaped(_("<span size=\"large\">by %s</span>"),remote));
			break;
			default:
			break;
		}
		if (body) g_free(body);
		if (remote) g_free(remote);
#endif
	}
}

static void on_call_updated_response(GtkWidget *dialog, gint responseid, LinphoneCall *call){
	if (linphone_call_get_state(call)==LinphoneCallUpdatedByRemote){
		LinphoneCore *lc=linphone_call_get_core(call);
		LinphoneCallParams *params=linphone_call_params_copy(linphone_call_get_current_params(call));
		linphone_call_params_enable_video(params,responseid==GTK_RESPONSE_YES);
		linphone_core_accept_call_update(lc,call,params);
		linphone_call_params_destroy(params);
	}
	linphone_call_unref(call);
	g_source_remove_by_user_data(dialog);
	gtk_widget_destroy(dialog);
}

static void on_call_updated_timeout(GtkWidget *dialog){
	gtk_widget_destroy(dialog);
}

static void linphone_gtk_call_updated_by_remote(LinphoneCall *call){
	LinphoneCore *lc=linphone_call_get_core(call);
	const LinphoneVideoPolicy *pol=linphone_core_get_video_policy(lc);
	const LinphoneCallParams *rparams=linphone_call_get_remote_params(call);
	const LinphoneCallParams *current_params=linphone_call_get_current_params(call);
	gboolean video_requested=linphone_call_params_video_enabled(rparams);
	gboolean video_used=linphone_call_params_video_enabled(current_params);
	g_message("Video used=%i, video requested=%i, automatically_accept=%i",
	          video_used,video_requested,pol->automatically_accept);
	if (video_used==FALSE && video_requested && !pol->automatically_accept){
		linphone_core_defer_call_update(lc,call);
		{
			const LinphoneAddress *addr=linphone_call_get_remote_address(call);
			GtkWidget *dialog;
			const char *dname=linphone_address_get_display_name(addr);
			if (dname==NULL) dname=linphone_address_get_username(addr);
			if (dname==NULL) dname=linphone_address_get_domain(addr);
			dialog=gtk_message_dialog_new(GTK_WINDOW(linphone_gtk_get_main_window()),
			                                         GTK_DIALOG_DESTROY_WITH_PARENT,
			                                         GTK_MESSAGE_WARNING,
			                                         GTK_BUTTONS_YES_NO,
			                                         _("%s proposed to start video. Do you accept ?"),dname);
			g_signal_connect(G_OBJECT(dialog),"response",(GCallback)on_call_updated_response,linphone_call_ref(call));
			g_timeout_add(20000,(GSourceFunc)on_call_updated_timeout,dialog);
			gtk_widget_show(dialog);
		}
	}
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
		break;
		case LinphoneCallUpdatedByRemote:
			linphone_gtk_call_updated_by_remote(call);
		break;
		case LinphoneCallError:
			linphone_gtk_in_call_view_terminate (call,msg);
		break;
		case LinphoneCallEnd:
			linphone_gtk_in_call_view_terminate(call,NULL);
			linphone_gtk_status_icon_set_blinking(FALSE);
		break;
		case LinphoneCallIncomingReceived:
			linphone_gtk_create_in_call_view(call);
			linphone_gtk_in_call_view_set_incoming(call);
			linphone_gtk_status_icon_set_blinking(TRUE);
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
			linphone_gtk_status_icon_set_blinking(FALSE);
		break;
		default:
		break;
	}
	linphone_gtk_notify(call, msg);
	linphone_gtk_update_call_buttons (call);
}

static void linphone_gtk_call_encryption_changed(LinphoneCore *lc, LinphoneCall *call, bool_t enabled, const char *token){
	linphone_gtk_in_call_view_show_encryption(call);
}

static void linphone_gtk_transfer_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate){
	linphone_gtk_in_call_view_set_transfer_status(call,cstate);
}

static void update_registration_status(LinphoneProxyConfig *cfg, LinphoneRegistrationState rs){
	GtkComboBox *box=GTK_COMBO_BOX(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"identities"));
	GtkTreeModel *model=gtk_combo_box_get_model(box);
	GtkTreeIter iter;
	gboolean found=FALSE;
	const char *stock_id=NULL;
	
	if (gtk_tree_model_get_iter_first(model,&iter)){
		gpointer p;
		do{
			gtk_tree_model_get(model,&iter,2,&p,-1);
			if (p==cfg) {
				found=TRUE;
				break;
			}
		}while(gtk_tree_model_iter_next(model,&iter));
	}
	if (!found) {
		g_warning("Could not find proxy config in combo box of identities.");
		return;
	}
	switch (rs){
		case LinphoneRegistrationOk:
			stock_id=GTK_STOCK_YES;
		break;
		case LinphoneRegistrationProgress:
			stock_id=GTK_STOCK_REFRESH;
		break;
		case LinphoneRegistrationCleared:
			stock_id=NULL;
		break;
		case LinphoneRegistrationFailed:
			stock_id=GTK_STOCK_DIALOG_WARNING;
		break;
		default:
		break;
	}
	gtk_list_store_set(GTK_LIST_STORE(model),&iter,1,stock_id,-1);
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
	update_registration_status(cfg,rs);
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

#ifndef HAVE_GTK_OSX

static GtkStatusIcon *icon=NULL;

static void icon_popup_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data){
	GtkWidget *menu=(GtkWidget*)g_object_get_data(G_OBJECT(status_icon),"menu");
	gtk_menu_popup(GTK_MENU(menu),NULL,NULL,gtk_status_icon_position_menu,status_icon,button,activate_time);
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

static void handle_icon_click() {
	GtkWidget *mw=linphone_gtk_get_main_window();
	if (!gtk_window_is_active((GtkWindow*)mw)) {
		linphone_gtk_show_main_window();
	} else {
		gtk_widget_hide(mw);
	}
}

static void linphone_gtk_init_status_icon(){
	const char *icon_path=linphone_gtk_get_ui_config("icon",LINPHONE_ICON);
	const char *call_icon_path=linphone_gtk_get_ui_config("start_call_icon","startcall-green.png");
	GdkPixbuf *pbuf=create_pixbuf(icon_path);
	GtkWidget *menu=create_icon_menu();
	const char *title;
	title=linphone_gtk_get_ui_config("title",_("Linphone - a video internet phone"));
	icon=gtk_status_icon_new_from_pixbuf(pbuf);
#if GTK_CHECK_VERSION(2,20,0)
	gtk_status_icon_set_name(icon,title);
#endif
	g_signal_connect_swapped(G_OBJECT(icon),"activate",(GCallback)handle_icon_click,NULL);
	g_signal_connect(G_OBJECT(icon),"popup-menu",(GCallback)icon_popup_menu,NULL);
	gtk_status_icon_set_tooltip(icon,title);
	gtk_status_icon_set_visible(icon,TRUE);
	g_object_set_data(G_OBJECT(icon),"menu",menu);
	g_object_weak_ref(G_OBJECT(icon),(GWeakNotify)gtk_widget_destroy,menu);
	g_object_set_data(G_OBJECT(icon),"icon",pbuf);
	g_object_weak_ref(G_OBJECT(icon),(GWeakNotify)g_object_unref,pbuf);
	pbuf=create_pixbuf(call_icon_path);
	g_object_set_data(G_OBJECT(icon),"call_icon",pbuf);
}

static gboolean do_icon_blink(GtkStatusIcon *gi){
	GdkPixbuf *call_icon=g_object_get_data(G_OBJECT(gi),"call_icon");
	GdkPixbuf *normal_icon=g_object_get_data(G_OBJECT(gi),"icon");
	GdkPixbuf *cur_icon=gtk_status_icon_get_pixbuf(gi);
	if (cur_icon==call_icon){
		gtk_status_icon_set_from_pixbuf(gi,normal_icon);
	}else{
		gtk_status_icon_set_from_pixbuf(gi,call_icon);
	}
	return TRUE;
}

#endif

static void linphone_gtk_status_icon_set_blinking(gboolean val){
#ifdef HAVE_GTK_OSX
	static gint attention_id;
	GtkOSXApplication *theMacApp=(GtkOSXApplication*)g_object_new(GTK_TYPE_OSX_APPLICATION, NULL);
	if (val)
		attention_id=gtk_osxapplication_attention_request(theMacApp,CRITICAL_REQUEST);
	else gtk_osxapplication_cancel_attention_request(theMacApp,attention_id);
#else
	if (icon!=NULL){
		guint tout;
		tout=(unsigned)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(icon),"timeout"));
		if (val && tout==0){
			tout=g_timeout_add(500,(GSourceFunc)do_icon_blink,icon);
			g_object_set_data(G_OBJECT(icon),"timeout",GINT_TO_POINTER(tout));
		}else if (!val && tout!=0){
			GdkPixbuf *normal_icon=g_object_get_data(G_OBJECT(icon),"icon");
			g_source_remove(tout);
			g_object_set_data(G_OBJECT(icon),"timeout",NULL);
			gtk_status_icon_set_from_pixbuf(icon,normal_icon);
		}
	}
#endif
}

void linphone_gtk_options_activate(GtkWidget *item){
#ifndef HAVE_GTK_OSX
	gtk_widget_set_visible(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"quit_item"),
		TRUE);
#endif
}

static void init_identity_combo(GtkComboBox *box){
	GtkListStore *store;
	GtkCellRenderer *r1,*r2;
	store=gtk_list_store_new(3,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_POINTER);
	gtk_cell_layout_clear(GTK_CELL_LAYOUT(box));
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(box),(r1=gtk_cell_renderer_text_new()),TRUE);
	gtk_cell_layout_pack_end(GTK_CELL_LAYOUT(box),(r2=gtk_cell_renderer_pixbuf_new()),FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(box),r1,"text",0);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(box),r2,"stock-id",1);
	g_object_set(G_OBJECT(r1),"ellipsize",PANGO_ELLIPSIZE_END,NULL);
	gtk_combo_box_set_model(box,GTK_TREE_MODEL(store));
}

void linphone_gtk_load_identities(void){
	const MSList *elem;
	GtkComboBox *box=GTK_COMBO_BOX(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"identities"));
	char *def_identity;
	LinphoneProxyConfig *def=NULL;
	int def_index=0,i;
	GtkListStore *store;
	GtkTreeIter iter;

	store=GTK_LIST_STORE(gtk_combo_box_get_model(box));
	if (gtk_tree_model_get_n_columns(GTK_TREE_MODEL(store))==1){
		/* model is empty, this is the first time we go here */
		init_identity_combo(box);
		store=GTK_LIST_STORE(gtk_combo_box_get_model(box));
	}
	gtk_list_store_clear(store);
	linphone_core_get_default_proxy(linphone_gtk_get_core(),&def);
	def_identity=g_strdup_printf(_("%s (Default)"),linphone_core_get_primary_contact(linphone_gtk_get_core()));
	gtk_list_store_append(store,&iter);
	gtk_list_store_set(store,&iter,0,def_identity,1,NULL,2,NULL,-1);
	g_free(def_identity);
	for(i=1,elem=linphone_core_get_proxy_config_list(linphone_gtk_get_core());
			elem!=NULL;
			elem=ms_list_next(elem),i++){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,0,linphone_proxy_config_get_identity(cfg),1,
		                   linphone_proxy_config_is_registered(cfg) ? GTK_STOCK_YES : NULL,
		                   2,cfg,-1);
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
	bool_t video_enabled=linphone_gtk_video_enabled();
	bool_t selfview=linphone_gtk_get_ui_config_int("videoselfview",VIDEOSELFVIEW_DEFAULT);
	GtkWidget *selfview_item=linphone_gtk_get_widget(
					linphone_gtk_get_main_window(),"selfview_item");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(linphone_gtk_get_widget(
					linphone_gtk_get_main_window(),"enable_video_item")), video_enabled);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(selfview_item),selfview);
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
		/*
		GdkPixbuf *pbuf=create_pixbuf("contact-orange.png");
		if (pbuf) {
			gtk_image_set_from_pixbuf(GTK_IMAGE(linphone_gtk_get_widget(w,"contact_tab_icon")),pbuf);
			g_object_unref(G_OBJECT(pbuf));
		}
		*/
	}
	{
		GdkPixbuf *pbuf=create_pixbuf("dialer-orange.png");
		if (pbuf) {
			GtkImage *img=GTK_IMAGE(linphone_gtk_get_widget(w,"keypad_tab_icon"));
			int w,h;
			GdkPixbuf *scaled;
			gtk_icon_size_lookup(GTK_ICON_SIZE_MENU,&w,&h);
			scaled=gdk_pixbuf_scale_simple(pbuf,w,h,GDK_INTERP_BILINEAR);
			gtk_image_set_from_pixbuf(img,scaled);
			g_object_unref(G_OBJECT(scaled));
			g_object_unref(G_OBJECT(pbuf));
		}
	}
	if (linphone_gtk_can_manage_accounts()) {
		gtk_widget_show(linphone_gtk_get_widget(w,"assistant_item"));
	}
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
#ifdef __APPLE__ /*until with have a better option*/
	gtk_window_iconify(GTK_WINDOW(mw));
#else
	gtk_widget_hide(mw);
#endif
	return TRUE;
}

#ifdef HAVE_GTK_OSX
static gboolean on_window_state_event(GtkWidget *w, GdkEventWindowState *event){
	bool_t video_enabled=linphone_gtk_video_enabled();
	if ((event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) ||(event->new_window_state & GDK_WINDOW_STATE_WITHDRAWN) ){
		linphone_core_enable_video_preview(linphone_gtk_get_core(),FALSE);
	}else{
		linphone_core_enable_video_preview(linphone_gtk_get_core(),
		linphone_gtk_get_ui_config_int("videoselfview",VIDEOSELFVIEW_DEFAULT) && video_enabled);
	}
	return FALSE;
}
#endif


static void linphone_gtk_init_main_window(){
	GtkWidget *main_window;

	linphone_gtk_configure_main_window();
	linphone_gtk_manage_login();
	load_uri_history();
	linphone_gtk_load_identities();
	linphone_gtk_set_my_presence(linphone_core_get_presence_info(linphone_gtk_get_core()));
	linphone_gtk_show_friends();
	linphone_gtk_connect_digits();
	main_window=linphone_gtk_get_main_window();
	linphone_gtk_call_log_update(main_window);
	
	linphone_gtk_update_call_buttons (NULL);
	/*prevent the main window from being destroyed by a user click on WM controls, instead we hide it*/
	g_signal_connect (G_OBJECT (main_window), "delete-event",
		G_CALLBACK (linphone_gtk_close), main_window);
#ifdef HAVE_GTK_OSX
	{
		GtkWidget *menubar=linphone_gtk_get_widget(main_window,"menubar1");
		GtkOSXApplication *theMacApp = (GtkOSXApplication*)g_object_new(GTK_TYPE_OSX_APPLICATION, NULL);
		gtk_osxapplication_set_menu_bar(theMacApp,GTK_MENU_SHELL(menubar));
		gtk_widget_hide(menubar);
		gtk_osxapplication_ready(theMacApp);
	}
	g_signal_connect(G_OBJECT(main_window), "window-state-event",G_CALLBACK(on_window_state_event), NULL);
#endif
	linphone_gtk_check_menu_items();
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


void linphone_gtk_refer_received(LinphoneCore *lc, const char *refer_to){
	GtkEntry * uri_bar =GTK_ENTRY(linphone_gtk_get_widget(
		linphone_gtk_get_main_window(), "uribar"));
	char *text;
	linphone_gtk_notify(NULL,(text=ms_strdup_printf(_("We are transferred to %s"),refer_to)));
	g_free(text);
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

#ifdef BUILD_WIZARD
// Display the account wizard
void linphone_gtk_display_wizard() {
	if (the_wizard == NULL || !gtk_widget_get_visible(the_wizard)) { // Only one instance of the wizard at the same time
		the_wizard = linphone_gtk_create_assistant();
	}
}
#endif

static void linphone_gtk_quit(void){
	static gboolean quit_done=FALSE;
	if (!quit_done){
		quit_done=TRUE;
		linphone_gtk_unmonitor_usb();
		g_source_remove_by_user_data(linphone_gtk_get_core());
		linphone_gtk_uninit_instance();
		linphone_gtk_destroy_log_window();
		linphone_core_destroy(the_core);
		linphone_gtk_log_uninit();
#ifdef HAVE_NOTIFY
		notify_uninit();
#endif
		gdk_threads_leave();
	}
}

#ifdef HAVE_GTK_OSX
/*
This is not the correct way to implement block termination.
The good way would be to call gtk_main_quit(), and return TRUE.
Unfortunately this does not work, because if we return TRUE the NSApplication sometimes calls the CFRunLoop recursively, which prevents gtk_main() to exit.
As a result the program cannot exit at all.
As a workaround we do all the cleanup (unregistration and config save) within the handler.
*/
static gboolean on_block_termination(void){
	gtk_main_quit();
	linphone_gtk_quit();
	return FALSE;
}
#endif

int main(int argc, char *argv[]){
#ifdef ENABLE_NLS
	void *p;
#endif
	char *config_file;
	const char *factory_config_file;
	const char *lang;
	GtkSettings *settings;
	GdkPixbuf *pbuf;
	const char *app_name="Linphone";

#if !GLIB_CHECK_VERSION(2, 31, 0)
	g_thread_init(NULL);
#endif
	gdk_threads_init();
	
	progpath = strdup(argv[0]);
	
	config_file=linphone_gtk_get_config_file(NULL);
	

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
		if (strncmp(lang,"zh",2)==0){
			workaround_gtk_entry_chinese_bug=TRUE;
		}
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
	g_type_class_unref (g_type_class_ref (GTK_TYPE_BUTTON));
	g_object_set(settings, "gtk-menu-images", TRUE, NULL);
	g_object_set(settings, "gtk-button-images", TRUE, NULL);

	if (workingdir!=NULL){
		if (chdir(workingdir)==-1){
			g_error("Could not change directory to %s : %s",workingdir,strerror(errno));
		}
	}

	/* Now, look for the factory configuration file, we do it this late
		 since we want to have had time to change directory and to parse
		 the options, in case we needed to access the working directory */
	factory_config_file = linphone_gtk_get_factory_config_file();

	if (linphone_gtk_init_instance(app_name, addr_to_call) == FALSE){
		g_warning("Another running instance of linphone has been detected. It has been woken-up.");
		g_warning("This instance is going to exit now.");
		gdk_threads_leave();
		return 0;
	}

	add_pixmap_directory("pixmaps");
	add_pixmap_directory(PACKAGE_DATA_DIR "/pixmaps/linphone");

#ifdef HAVE_GTK_OSX
	GtkOSXApplication *theMacApp = (GtkOSXApplication*)g_object_new(GTK_TYPE_OSX_APPLICATION, NULL);
	g_signal_connect(G_OBJECT(theMacApp),"NSApplicationDidBecomeActive",(GCallback)linphone_gtk_show_main_window,NULL);
	g_signal_connect(G_OBJECT(theMacApp),"NSApplicationWillTerminate",(GCallback)gtk_main_quit,NULL);
	/*never block termination:*/
	g_signal_connect(G_OBJECT(theMacApp),"NSApplicationBlockTermination",(GCallback)on_block_termination,NULL);
#endif
	
	the_ui=linphone_gtk_create_window("main");
	
	linphone_gtk_create_log_window();
	linphone_core_enable_logs_with_cb(linphone_gtk_log_handler);

	linphone_gtk_init_liblinphone(config_file, factory_config_file);
	
	g_set_application_name(app_name);
	pbuf=create_pixbuf(linphone_gtk_get_ui_config("icon",LINPHONE_ICON));
	if (pbuf!=NULL) gtk_window_set_default_icon(pbuf);
	
	/* do not lower timeouts under 30 ms because it exhibits a bug on gtk+/win32, with cpu running 20% all the time...*/
	gtk_timeout_add(30,(GtkFunction)linphone_gtk_iterate,(gpointer)linphone_gtk_get_core());
	gtk_timeout_add(30,(GtkFunction)linphone_gtk_check_logs,(gpointer)NULL);
	linphone_gtk_init_main_window();

#ifdef BUILD_WIZARD
	// Veryfing if at least one sip account is configured. If not, show wizard
	if (linphone_core_get_proxy_config_list(linphone_gtk_get_core()) == NULL) {
		linphone_gtk_display_wizard();
	}
#endif

#ifndef HAVE_GTK_OSX
	linphone_gtk_init_status_icon();
#endif
	if (!iconified){
		linphone_gtk_show_main_window();
		linphone_gtk_check_soundcards();
	}
	if (linphone_gtk_get_ui_config_int("update_check_menu",0)==0)
		linphone_gtk_check_for_new_version();
	linphone_gtk_monitor_usb();

	gtk_main();
	linphone_gtk_quit();
#ifndef HAVE_GTK_OSX
	/*workaround a bug on win32 that makes status icon still present in the systray even after program exit.*/
	gtk_status_icon_set_visible(icon,FALSE);
#endif
	free(progpath);
	return 0;
}

