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
#include "direct.h"
#endif

#if defined(HAVE_NOTIFY1) || defined(HAVE_NOTIFY4)
#define HAVE_NOTIFY
#endif

#ifdef HAVE_NOTIFY
#include <libnotify/notify.h>
#endif

#ifdef ENABLE_NLS
#include <locale.h>
#endif


const char *this_program_ident_string="linphone_ident_string=" LINPHONE_VERSION;

static LinphoneCore *the_core=NULL;
static GtkWidget *the_ui=NULL;
static LinphoneLDAPContactProvider* ldap_provider = NULL;

static void linphone_gtk_global_state_changed(LinphoneCore *lc, LinphoneGlobalState state, const char*str);
static void linphone_gtk_registration_state_changed(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState rs, const char *msg);
static void linphone_gtk_notify_recv(LinphoneCore *lc, LinphoneFriend * fid);
static void linphone_gtk_new_unknown_subscriber(LinphoneCore *lc, LinphoneFriend *lf, const char *url);
static void linphone_gtk_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username, const char *domain);
static void linphone_gtk_display_status(LinphoneCore *lc, const char *status);
static void linphone_gtk_configuring_status(LinphoneCore *lc, LinphoneConfiguringState status, const char *message);
static void linphone_gtk_display_message(LinphoneCore *lc, const char *msg);
static void linphone_gtk_display_warning(LinphoneCore *lc, const char *warning);
static void linphone_gtk_display_url(LinphoneCore *lc, const char *msg, const char *url);
static void linphone_gtk_call_log_updated(LinphoneCore *lc, LinphoneCallLog *cl);
static void linphone_gtk_call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cs, const char *msg);
static void linphone_gtk_call_encryption_changed(LinphoneCore *lc, LinphoneCall *call, bool_t enabled, const char *token);
static void linphone_gtk_transfer_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate);
static void linphone_gtk_dtmf_received(LinphoneCore *lc, LinphoneCall *call, int dtmf);
void linphone_gtk_save_main_window_position(GtkWindow* mw, GdkEvent *event, gpointer data);
static gboolean linphone_gtk_auto_answer(LinphoneCall *call);
void linphone_gtk_status_icon_set_blinking(gboolean val);
void _linphone_gtk_enable_video(gboolean val);
void linphone_gtk_on_uribar_changed(GtkEditable *uribar, gpointer user_data);
static void linphone_gtk_init_ui(void);
static void linphone_gtk_quit(void);

#ifndef HAVE_GTK_OSX
static gint main_window_x=0;
static gint main_window_y=0;
#endif
static gboolean verbose=0;
static gboolean quit_done=FALSE;
static gboolean auto_answer = 0;
static gchar * addr_to_call = NULL;
static int start_option = START_LINPHONE;
static gboolean no_video=FALSE;
static gboolean iconified=FALSE;
static gboolean run_audio_assistant=FALSE;
static gboolean selftest=FALSE;
static gchar *workingdir=NULL;
static char *progpath=NULL;
gchar *linphone_logfile=NULL;
static gboolean workaround_gtk_entry_chinese_bug=FALSE;
static gchar *custom_config_file=NULL;
static gboolean restart=FALSE;
static GtkWidget *config_fetching_dialog=NULL;

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
	{
		.long_name = "config",
		.short_name = '\0',
		.arg = G_OPTION_ARG_FILENAME,
		.arg_data = (gpointer) &custom_config_file,
		.description = N_("Configuration file")
	},
	{
		.long_name = "run-audio-assistant",
		.short_name = '\0',
		.arg = G_OPTION_ARG_NONE,
		.arg_data = (gpointer) &run_audio_assistant,
		.description = N_("Run the audio assistant")
	},
	{
		.long_name = "selftest",
		.short_name = '\0',
		.arg = G_OPTION_ARG_NONE,
		.arg_data = (gpointer) &selftest,
		.description = N_("Run self test and exit 0 if succeed")
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
	} else if (g_path_is_absolute(filename)) {
		snprintf(config_file,path_max,"%s",filename);
	} else{
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

LinphoneLDAPContactProvider* linphone_gtk_get_ldap(void){
	return ldap_provider;
}

int linphone_gtk_is_ldap_supported(void){
	return linphone_ldap_contact_provider_available();
}

void linphone_gtk_set_ldap(LinphoneLDAPContactProvider* ldap)
{
	if( ldap_provider )
		linphone_contact_provider_unref(ldap_provider);

	ldap_provider = ldap ? linphone_ldap_contact_provider_ref( ldap )
						 : NULL;
}

void linphone_gtk_schedule_restart(void){
	restart=TRUE;
}

gboolean linphone_gtk_get_audio_assistant_option(void){
	return run_audio_assistant;
}

static void linphone_gtk_init_liblinphone(const char *config_file,
		const char *factory_config_file, const char *db_file) {
	LinphoneCoreVTable vtable={0};
	gchar *secrets_file=linphone_gtk_get_config_file(SECRETS_FILE);

	vtable.global_state_changed=linphone_gtk_global_state_changed;
	vtable.call_state_changed=linphone_gtk_call_state_changed;
	vtable.registration_state_changed=linphone_gtk_registration_state_changed;
	vtable.notify_presence_received=linphone_gtk_notify_recv;
	vtable.new_subscription_requested=linphone_gtk_new_unknown_subscriber;
	vtable.auth_info_requested=linphone_gtk_auth_info_requested;
	vtable.display_status=linphone_gtk_display_status;
	vtable.display_message=linphone_gtk_display_message;
	vtable.display_warning=linphone_gtk_display_warning;
	vtable.display_url=linphone_gtk_display_url;
	vtable.call_log_updated=linphone_gtk_call_log_updated;
	//vtable.text_received=linphone_gtk_text_received;
	vtable.message_received=linphone_gtk_text_received;
	vtable.is_composing_received=linphone_gtk_is_composing_received;
	vtable.refer_received=linphone_gtk_refer_received;
	vtable.buddy_info_updated=linphone_gtk_buddy_info_updated;
	vtable.call_encryption_changed=linphone_gtk_call_encryption_changed;
	vtable.transfer_state_changed=linphone_gtk_transfer_state_changed;
	vtable.dtmf_received=linphone_gtk_dtmf_received;
	vtable.configuring_status=linphone_gtk_configuring_status;

	the_core=linphone_core_new(&vtable,config_file,factory_config_file,NULL);
	linphone_core_migrate_to_multi_transport(the_core);
	//lp_config_set_int(linphone_core_get_config(the_core), "sip", "store_auth_info", 0);


	if( lp_config_has_section(linphone_core_get_config(the_core),"ldap") ){
		LpConfig* cfg = linphone_core_get_config(the_core);
		LinphoneDictionary* ldap_cfg = lp_config_section_to_dict(cfg, "ldap");
		linphone_gtk_set_ldap( linphone_ldap_contact_provider_create(the_core, ldap_cfg) );
	}

	linphone_core_set_user_agent(the_core,"Linphone", LINPHONE_VERSION);
	linphone_core_set_waiting_callback(the_core,linphone_gtk_wait,NULL);
	linphone_core_set_zrtp_secrets_file(the_core,secrets_file);
	g_free(secrets_file);
	linphone_core_enable_video_capture(the_core, TRUE);
	linphone_core_enable_video_display(the_core, TRUE);
	linphone_core_set_native_video_window_id(the_core,-1);/*don't create the window*/
	if (no_video) {
		_linphone_gtk_enable_video(FALSE);
		linphone_gtk_set_ui_config_int("videoselfview",0);
	}
	if (db_file) linphone_core_set_chat_database_path(the_core,db_file);
}

LinphoneCore *linphone_gtk_get_core(void){
	return the_core;
}

GtkWidget *linphone_gtk_get_main_window(){
	return the_ui;
}

void linphone_gtk_destroy_main_window() {
	linphone_gtk_destroy_window(the_ui);
	the_ui = NULL;
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
		if(pbuf != NULL) {
			gtk_window_set_icon(GTK_WINDOW(w),pbuf);
			g_object_unref(G_OBJECT(pbuf));
		}
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

void linphone_gtk_destroy_window(GtkWidget *widget) {
	GtkBuilder* builder = g_object_get_data(G_OBJECT(widget), "builder");
	gtk_widget_destroy(widget);
	g_object_unref (G_OBJECT (builder));
}

GtkWidget *linphone_gtk_create_window(const char *window_name){
	GError* error = NULL;
	GtkBuilder* builder = gtk_builder_new ();
	char path[512];
	GtkWidget *w;

	if (get_ui_file(window_name,path,sizeof(path))==-1) return NULL;

	gtk_builder_set_translation_domain(builder,GETTEXT_PACKAGE);

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
	g_object_set_data(G_OBJECT(w), "builder",builder);
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

	gtk_builder_set_translation_domain(builder,GETTEXT_PACKAGE);

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

static void entry_unmapped(GtkWidget *widget){
	ms_message("%s is unmapped, calling unrealize to workaround chinese bug.",G_OBJECT_TYPE_NAME(widget));
	gtk_widget_unrealize(widget);
}

GtkWidget *linphone_gtk_get_widget(GtkWidget *window, const char *name){
	GtkBuilder *builder;
	GObject *w;
	if (window==NULL) return NULL;
	builder=(GtkBuilder*)g_object_get_data(G_OBJECT(window),"builder");
	if (builder==NULL){
		g_error("Fail to retrieve builder from window !");
		return NULL;
	}
	w=gtk_builder_get_object(builder,name);
	if (w==NULL){
		g_error("No widget named %s found in xml interface.",name);
	}
	if (workaround_gtk_entry_chinese_bug){
		if (strcmp(G_OBJECT_TYPE_NAME(w),"GtkEntry")==0 || strcmp(G_OBJECT_TYPE_NAME(w),"GtkTextView")==0){
			if (g_object_get_data(G_OBJECT(w),"entry_bug_workaround")==NULL){
				g_object_set_data(G_OBJECT(w),"entry_bug_workaround",GINT_TO_POINTER(1));
				ms_message("%s is a %s",name,G_OBJECT_TYPE_NAME(w));
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


static gboolean linphone_gtk_iterate(LinphoneCore *lc){
	static gboolean first_time=TRUE;
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

static gboolean uribar_completion_matchfunc(GtkEntryCompletion *completion, const gchar *key, GtkTreeIter *iter, gpointer user_data){
	char* address = NULL;
	gboolean ret  = FALSE;
	gchar *tmp= NULL;
	gtk_tree_model_get(gtk_entry_completion_get_model(completion),iter,0,&address,-1);

	tmp = g_utf8_casefold(address,-1);
	if (tmp){
		if (strstr(tmp,key))
			ret=TRUE;
		g_free(tmp);
	}

	if( address)
		g_free(address);

	return ret;
}

static void load_uri_history(){
	GtkEntry *uribar=GTK_ENTRY(linphone_gtk_get_widget(linphone_gtk_get_main_window(),"uribar"));
	char key[20];
	int i;
	GtkEntryCompletion *gep=gtk_entry_completion_new();
	GtkListStore *model=gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_INT);
	for (i=0;;i++){
		const char *uri;
		snprintf(key,sizeof(key),"uri%i",i);
		uri=linphone_gtk_get_ui_config(key,NULL);
		if (uri!=NULL) {
			GtkTreeIter iter;
			gtk_list_store_append(model,&iter);
			gtk_list_store_set(model,&iter,0,uri,1,COMPLETION_HISTORY,-1);
			if (i==0) gtk_entry_set_text(uribar,uri);
		}
		else break;
	}
	gtk_entry_completion_set_model(gep,GTK_TREE_MODEL(model));
	gtk_entry_completion_set_text_column(gep,0);
	gtk_entry_completion_set_popup_completion(gep, TRUE);
	gtk_entry_completion_set_match_func(gep,uribar_completion_matchfunc, NULL, NULL);
	gtk_entry_completion_set_minimum_key_length(gep,3);
	gtk_entry_set_completion(uribar,gep);
	g_signal_connect (G_OBJECT (uribar), "changed", G_CALLBACK(linphone_gtk_on_uribar_changed), NULL);
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
	gtk_list_store_set(GTK_LIST_STORE(model),&iter,0,text,1,COMPLETION_HISTORY,-1);
	save_uri_history();
}

void on_contact_provider_search_results( LinphoneContactSearch* req, MSList* friends, void* data )
{
	GtkTreeIter    iter;
	GtkEntry*    uribar = GTK_ENTRY(data);
	GtkEntryCompletion* compl = gtk_entry_get_completion(uribar);
	GtkTreeModel* model = gtk_entry_completion_get_model(compl);
	GtkListStore*  list = GTK_LIST_STORE(model);
	LinphoneLDAPContactSearch* search = linphone_ldap_contact_search_cast(req);
	gboolean valid;

	// clear completion list from previous non-history entries
	valid = gtk_tree_model_get_iter_first(model,&iter);
	while(valid)
	{
		char* url;
		int type;
		gtk_tree_model_get(model,&iter, 0,&url, 1,&type, -1);

		if (type != COMPLETION_HISTORY) {
			valid = gtk_list_store_remove(list, &iter);
		} else {
			valid = gtk_tree_model_iter_next(model,&iter);
		}

		if( url ) g_free(url);
		if( !valid ) break;
	}

	// add new non-history related matches
	while( friends ){
		LinphoneFriend* lf = friends->data;
		if( lf ) {
			const LinphoneAddress* la = linphone_friend_get_address(lf);
			if( la ){
				char *addr = linphone_address_as_string(la);

				if( addr ){
					ms_message("[LDAP]Insert match: %s",  addr);
					gtk_list_store_insert_with_values(list, &iter, -1,
													  0, addr,
													  1, COMPLETION_LDAP, -1);
					ms_free(addr);
				}
			}
		}
		friends = friends->next;
	}
	gtk_entry_completion_complete(compl);
	// save the number of LDAP results to better decide if new results should be fetched when search predicate gets bigger
	gtk_object_set_data(GTK_OBJECT(uribar), "ldap_res_cout",
						GINT_TO_POINTER(
							linphone_ldap_contact_search_result_count(search)
							)
						);

	// Gtk bug? we need to emit a "changed" signal so that the completion appears if
	// the list of results was previously empty
	g_signal_handlers_block_by_func(uribar, linphone_gtk_on_uribar_changed, NULL);
	g_signal_emit_by_name(uribar, "changed");
	g_signal_handlers_unblock_by_func(uribar, linphone_gtk_on_uribar_changed, NULL);
}

struct CompletionTimeout {
	guint timeout_id;
};

static gboolean launch_contact_provider_search(void *userdata)
{
	LinphoneLDAPContactProvider* ldap = linphone_gtk_get_ldap();
	GtkWidget*      uribar = GTK_WIDGET(userdata);
	const gchar* predicate = gtk_entry_get_text(GTK_ENTRY(uribar));
	gchar* previous_search = gtk_object_get_data(GTK_OBJECT(uribar), "previous_search");
	unsigned int prev_res_count = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(uribar), "ldap_res_cout"));

	if( ldap && strlen(predicate) >= 3 ){ // don't search too small predicates
		unsigned int max_res_count = linphone_ldap_contact_provider_get_max_result(ldap);
		LinphoneContactSearch* search;
		if( previous_search  &&
			(strstr(predicate, previous_search) == predicate) && // last search contained results from this one
			(prev_res_count != max_res_count) ){ // and we didn't reach the max result limit

			ms_message("Don't launch search on already searched data (current: %s, old search: %s), (%d/%d results)",
					   predicate, previous_search, prev_res_count, max_res_count);
			return FALSE;
		}

		// save current search
		if( previous_search ) ms_free(previous_search);
		gtk_object_set_data(GTK_OBJECT(uribar), "previous_search", ms_strdup(predicate));

		ms_message("launch_contact_provider_search");
		search =linphone_contact_provider_begin_search(
					linphone_contact_provider_cast(ldap_provider),
					predicate, on_contact_provider_search_results, uribar
					);

		if(search)
			linphone_contact_search_ref(search);
	}
	return FALSE;
}

void linphone_gtk_on_uribar_changed(GtkEditable *uribar, gpointer user_data)
{
	if( linphone_gtk_get_ldap() ) {
		gchar* text = gtk_editable_get_chars(uribar, 0,-1);
		gint timeout = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(uribar), "complete_timeout"));
		if( text ) g_free(text);

		if( timeout != 0 ) {
			g_source_remove(timeout);
		}

		timeout = g_timeout_add_seconds(1,(GSourceFunc)launch_contact_provider_search, uribar);

		gtk_object_set_data(GTK_OBJECT(uribar),"complete_timeout", GINT_TO_POINTER(timeout) );
	}
}

bool_t linphone_gtk_video_enabled(void){
	const LinphoneVideoPolicy *vpol=linphone_core_get_video_policy(linphone_gtk_get_core());
	return vpol->automatically_accept && vpol->automatically_initiate;
}

void linphone_gtk_show_main_window(){
	GtkWidget *w=linphone_gtk_get_main_window();
	gtk_widget_show(w);
	gtk_window_present(GTK_WINDOW(w));
}

void linphone_gtk_call_terminated(LinphoneCall *call, const char *error){
	GtkWidget *mw=linphone_gtk_get_main_window();
	if (linphone_core_get_calls(linphone_gtk_get_core())==NULL){
	    gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"start_call"),TRUE);
	}
	if (linphone_gtk_use_in_call_view() && call)
		linphone_gtk_in_call_view_terminate(call,error);
}

static void linphone_gtk_update_call_buttons(LinphoneCall *call){
	LinphoneCore *lc=linphone_gtk_get_core();
	GtkWidget *mw=linphone_gtk_get_main_window();
	const MSList *calls=linphone_core_get_calls(lc);
	GtkWidget *button;
	bool_t start_active=TRUE;
	//bool_t stop_active=FALSE;
	bool_t add_call=FALSE;
	int call_list_size=ms_list_size(calls);
	GtkWidget *conf_frame;

	if (calls==NULL){
		start_active=TRUE;
		//stop_active=FALSE;
	}else{
		//stop_active=TRUE;
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

	//gtk_widget_set_sensitive(linphone_gtk_get_widget(mw,"terminate_call"),stop_active);
	conf_frame=(GtkWidget *)g_object_get_data(G_OBJECT(mw),"conf_frame");
	if(conf_frame==NULL){
		linphone_gtk_enable_transfer_button(lc,call_list_size>1);
		linphone_gtk_enable_conference_button(lc,call_list_size>1);
	} else {
		linphone_gtk_enable_transfer_button(lc,FALSE);
		linphone_gtk_enable_conference_button(lc,FALSE);
	}
	if (call) {
		linphone_gtk_update_video_button(call);
	}
}

gchar *linphone_gtk_get_record_path(const LinphoneAddress *address, gboolean is_conference){
	const char *dir=g_get_user_special_dir(G_USER_DIRECTORY_MUSIC);
	const char *id="unknown";
	char filename[256]={0};
	char date[64]={0};
	time_t curtime=time(NULL);
	struct tm loctime;
	const char **fmts=linphone_core_get_supported_file_formats(linphone_gtk_get_core());
	int i;
	const char *ext="wav";

#ifdef WIN32
	loctime=*localtime(&curtime);
#else
	localtime_r(&curtime,&loctime);
#endif
	snprintf(date,sizeof(date)-1,"%i%02i%02i-%02i%02i",loctime.tm_year+1900,loctime.tm_mon+1,loctime.tm_mday, loctime.tm_hour, loctime.tm_min);

	for (i=0;fmts[i]!=NULL;++i){
		if (strcmp(fmts[i],"mkv")==0){
			ext="mkv";
			break;
		}
	}

	if (address){
		id=linphone_address_get_username(address);
		if (id==NULL) id=linphone_address_get_domain(address);
	}
	if (is_conference){
		snprintf(filename,sizeof(filename)-1,"%s-conference-%s.%s",
			linphone_gtk_get_ui_config("title","Linphone"),
			date,ext);
	}else{
		snprintf(filename,sizeof(filename)-1,"%s-call-%s-%s.%s",
			linphone_gtk_get_ui_config("title","Linphone"),
			date,
			id,ext);
	}
	if (!dir) {
		ms_message ("No directory for music, using [%s] instead",dir=getenv("HOME"));
	}
	return g_build_filename(dir,filename,NULL);
}

static gboolean linphone_gtk_start_call_do(GtkWidget *uri_bar){
	const char *entered=gtk_entry_get_text(GTK_ENTRY(uri_bar));
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneAddress *addr=linphone_core_interpret_url(lc,entered);

	if (addr!=NULL){
		LinphoneCallParams *params=linphone_core_create_default_call_parameters(lc);
		gchar *record_file=linphone_gtk_get_record_path(addr,FALSE);
		linphone_call_params_set_record_file(params,record_file);
		linphone_core_invite_address_with_params(lc,addr,params);
		completion_add_text(GTK_ENTRY(uri_bar),entered);
		linphone_address_destroy(addr);
		linphone_call_params_destroy(params);
		g_free(record_file);
	}else{
		linphone_gtk_call_terminated(NULL,NULL);
	}
	return FALSE;
}


static void accept_incoming_call(LinphoneCall *call){
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneCallParams *params=linphone_core_create_default_call_parameters(lc);
	gchar *record_file=linphone_gtk_get_record_path(linphone_call_get_remote_address(call),FALSE);
	linphone_call_params_set_record_file(params,record_file);
	linphone_core_accept_call_with_params(lc,call,params);
	linphone_call_params_destroy(params);
}

static gboolean linphone_gtk_auto_answer(LinphoneCall *call){
	LinphoneCallState state=linphone_call_get_state(call);
	if (state==LinphoneCallIncomingReceived || state==LinphoneCallIncomingEarlyMedia){
		accept_incoming_call(call);
	}
	return FALSE;
}

void linphone_gtk_start_call(GtkWidget *w){
	LinphoneCall *call=linphone_gtk_get_currently_displayed_call(NULL);
	/*change into in-call mode, then do the work later as it might block a bit */
	GtkWidget *mw=gtk_widget_get_toplevel(w);
	GtkWidget *uri_bar=linphone_gtk_get_widget(mw,"uribar");
	LinphoneCallState state= call ? linphone_call_get_state(call) : LinphoneCallIdle;

	if (state == LinphoneCallIncomingReceived || state == LinphoneCallIncomingEarlyMedia){
		accept_incoming_call(call);
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
		accept_incoming_call(call);
		linphone_gtk_show_main_window(); /* useful when the button is clicked on a notification */
	}
}

void _linphone_gtk_enable_video(gboolean val){
	LinphoneVideoPolicy policy={0};
	policy.automatically_initiate=policy.automatically_accept=val;
	linphone_core_enable_video_capture(linphone_gtk_get_core(), TRUE);
	linphone_core_enable_video_display(linphone_gtk_get_core(), TRUE);
	linphone_core_set_video_policy(linphone_gtk_get_core(),&policy);
}

void linphone_gtk_enable_video(GtkWidget *w){
	gboolean val=gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	//GtkWidget *selfview_item=linphone_gtk_get_widget(linphone_gtk_get_main_window(),"selfview_item");
	_linphone_gtk_enable_video(val);
}

void linphone_gtk_enable_self_view(GtkWidget *w){
	gboolean val=gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
	LinphoneCore *lc=linphone_gtk_get_core();
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
	gchar *message;

	if (linphone_gtk_get_ui_config_int("subscribe_deny_all",0)){
		linphone_core_reject_subscriber(linphone_gtk_get_core(),lf);
		return;
	}

	message=g_strdup_printf(_("%s would like to add you to his contact list.\nWould you allow him to see your presence status or add him to your contact list ?\nIf you answer no, this person will be temporarily blacklisted."),url);
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

static void linphone_gtk_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username, const char *domain){
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

	msg=g_strdup_printf(_("Please enter your password for username <i>%s</i>\n at realm <i>%s</i>:"),
		username,realm);
	gtk_label_set_markup(GTK_LABEL(label),msg);
	g_free(msg);
	gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"userid_entry")),username);
	info=linphone_auth_info_new(username, NULL, NULL, NULL,realm,domain);
	g_object_set_data(G_OBJECT(w),"auth_info",info);
	g_object_weak_ref(G_OBJECT(w),(GWeakNotify)linphone_auth_info_destroy,info);
	gtk_widget_show(w);
	auth_timeout_new(w);
}

static void linphone_gtk_dtmf_received(LinphoneCore *lc, LinphoneCall *call, int dtmf){
	ms_message("Dtmf %c received.",dtmf);
}

static void linphone_gtk_display_status(LinphoneCore *lc, const char *status){
	GtkWidget *w=linphone_gtk_get_main_window();
	GtkWidget *status_bar=linphone_gtk_get_widget(w,"status_bar");

	gtk_statusbar_push(GTK_STATUSBAR(status_bar),
			gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar),""),
			status);
}

static void linphone_gtk_configuring_status(LinphoneCore *lc, LinphoneConfiguringState status, const char *message) {
	if (config_fetching_dialog) linphone_gtk_close_config_fetching(config_fetching_dialog, status);
	config_fetching_dialog=NULL;
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

static NotifyNotification* build_notification(const char *title, const char *body) {
	NotifyNotification *n = notify_notification_new(title, body, NULL
#ifdef HAVE_NOTIFY1
		,NULL
#endif
	);
#ifndef HAVE_NOTIFY1
	{
		const char *icon_path = linphone_gtk_get_ui_config("icon", LINPHONE_ICON);
		GdkPixbuf *pbuf = create_pixbuf(icon_path);
		/*with notify1, this function makes the notification crash the app with obscure dbus glib critical errors*/
		notify_notification_set_icon_from_pixbuf(n, pbuf);
	}
#endif
	return n;
}

static void show_notification(NotifyNotification* n){
	if (n && !notify_notification_show(n,NULL))
		ms_error("Failed to send notification.");
}

static void make_notification(const char *title, const char *body){
	show_notification(build_notification(title,body));
}

#endif

void linphone_gtk_notify(LinphoneCall *call, const char *msg){
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
				make_notification(_("Call error"),body=g_markup_printf_escaped("<b>%s</b>\n%s",msg,remote));
			break;
			case LinphoneCallEnd:
				make_notification(_("Call ended"),body=g_markup_printf_escaped("<b>%s</b>",remote));
			break;
			case LinphoneCallIncomingReceived:
				n=build_notification(_("Incoming call"),body=g_markup_printf_escaped("<b>%s</b>",remote));
				if (notify_actions_supported()) {
					notify_notification_add_action (n,"answer", _("Answer"),
						NOTIFY_ACTION_CALLBACK(linphone_gtk_answer_clicked),NULL,NULL);
					notify_notification_add_action (n,"decline",_("Decline"),
						NOTIFY_ACTION_CALLBACK(linphone_gtk_decline_clicked),NULL,NULL);
				}
				show_notification(n);
			break;
			case LinphoneCallPausedByRemote:
				make_notification(_("Call paused"),body=g_markup_printf_escaped(_("<b>by %s</b>"),remote));
			break;
			default:
			break;
		}
		if (body) g_free(body);
		if (remote) g_free(remote);
#endif
	}
}

static void linphone_gtk_global_state_changed(LinphoneCore *lc, LinphoneGlobalState state, const char*str){
	switch(state){
		case LinphoneGlobalStartup:
			the_core=lc;
		break;
		case LinphoneGlobalConfiguring:
			if (linphone_core_get_provisioning_uri(lc)){
				config_fetching_dialog=linphone_gtk_show_config_fetching();
			}
		break;
		case LinphoneGlobalOn:
			linphone_gtk_init_ui();
			if (selftest) {
				gtk_timeout_add(300,(GtkFunction)gtk_main_quit,NULL);
			}
		break;
		default:
		break;
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
			linphone_gtk_call_update_tab_header(call,FALSE);
		case LinphoneCallPausedByRemote:
			linphone_gtk_in_call_view_set_paused(call);
			linphone_gtk_call_update_tab_header(call,TRUE);
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
		/*ignored, this is a notification for a removed proxy config.*/
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

void linphone_gtk_save_main_window_position(GtkWindow* mw, GdkEvent *event, gpointer data){
       gtk_window_get_position(GTK_WINDOW(mw), &main_window_x, &main_window_y);
}

static void handle_icon_click() {
	GtkWidget *mw=linphone_gtk_get_main_window();
	if (!gtk_window_is_active((GtkWindow*)mw)) {
		if(!gtk_widget_is_drawable(mw)){
			//we only move if window was hidden. If it was simply behind the window stack, ie, drawable, we keep it as it was
			gtk_window_move (GTK_WINDOW(mw), main_window_x, main_window_y);
		}
		linphone_gtk_show_main_window();
	} else {
		linphone_gtk_save_main_window_position((GtkWindow*)mw, NULL, NULL);
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
#if GTK_CHECK_VERSION(2,20,2)
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

void linphone_gtk_status_icon_set_blinking(gboolean val){
#ifdef HAVE_GTK_OSX
	static gint attention_id;
	GtkosxApplication *theMacApp=gtkosx_application_get();
	if (val)
		attention_id=gtkosx_application_attention_request(theMacApp,CRITICAL_REQUEST);
	else gtkosx_application_cancel_attention_request(theMacApp,attention_id);
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
	const char *label=(char *)g_object_get_data(G_OBJECT(button),"label");
	GtkWidget *uri_bar=linphone_gtk_get_widget(linphone_gtk_get_main_window(),"uribar");
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


static void linphone_gtk_connect_digits(GtkWidget *w){
	GtkContainer *cont=GTK_CONTAINER(linphone_gtk_get_widget(w,"dtmf_table"));
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
	if (search_icon){
		GdkPixbuf *pbuf=create_pixbuf(search_icon);
		if(pbuf != NULL) {
			gtk_image_set_from_pixbuf(GTK_IMAGE(linphone_gtk_get_widget(w,"directory_search_button_icon")),pbuf);
			g_object_unref(G_OBJECT(pbuf));
		}
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
		GdkPixbuf *pbuf=create_pixbuf("dialer.png");
		if (pbuf) {
			GtkButton *button=GTK_BUTTON(linphone_gtk_get_widget(w,"keypad"));
			gtk_button_set_image(button,gtk_image_new_from_pixbuf (pbuf));
		}
	}
	if (linphone_gtk_can_manage_accounts()) {
		gtk_widget_show(linphone_gtk_get_widget(w,"assistant_item"));
	}
	if (update_check_menu){
		gtk_widget_show(linphone_gtk_get_widget(w,"versioncheck_item"));
	}
	g_object_set_data(G_OBJECT(w),"show_abcd",GINT_TO_POINTER(show_abcd));
}

void linphone_gtk_manage_login(void){
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneProxyConfig *cfg=NULL;
	linphone_core_get_default_proxy(lc,&cfg);
	if (cfg){
		SipSetup *ss=linphone_proxy_config_get_sip_setup(cfg);
		if (ss && (sip_setup_get_capabilities(ss) & SIP_SETUP_CAP_LOGIN)){
			linphone_gtk_show_login_frame(cfg,FALSE);
		}
	}
}

gboolean linphone_gtk_close(GtkWidget *mw){
	/*shutdown calls if any*/
	LinphoneCore *lc=linphone_gtk_get_core();
	GtkWidget *camera_preview=linphone_gtk_get_camera_preview_window();
	if (linphone_core_in_call(lc)){
		linphone_core_terminate_all_calls(lc);
	}
	if (camera_preview) gtk_widget_destroy(camera_preview);
#ifdef __APPLE__ /*until with have a better option*/
	gtk_window_iconify(GTK_WINDOW(mw));
#else
	gtk_widget_hide(mw);
#endif
	return TRUE;
}

#ifdef HAVE_GTK_OSX
static gboolean on_window_state_event(GtkWidget *w, GdkEventWindowState *event){
	return FALSE;
}
#endif

void linphone_gtk_init_dtmf_table(GtkWidget *mw){
	GtkWidget *dtmf_table=linphone_gtk_get_widget(mw,"dtmf_table");
	gtk_widget_set_direction(dtmf_table, GTK_TEXT_DIR_LTR);

	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_A")),"label","A");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_B")),"label","B");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_C")),"label","C");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_D")),"label","D");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_1")),"label","1");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_2")),"label","2");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_3")),"label","3");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_4")),"label","4");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_5")),"label","5");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_6")),"label","6");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_7")),"label","7");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_8")),"label","8");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_9")),"label","9");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_0")),"label","0");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_#")),"label","#");
	g_object_set_data(G_OBJECT(linphone_gtk_get_widget(mw,"dtmf_*")),"label","*");
}

static gboolean key_allowed(guint32 code){
	static const char *allowed="1234567890#*ABCD";
	return code!=0 && strchr(allowed,(char)code)!=NULL;
}

static GtkButton *get_button_from_key(GtkWidget *w, GdkEvent *event){
	guint keyval=event->key.keyval;
	guint32 code=gdk_keyval_to_unicode(keyval);
	code=g_unichar_toupper(code);
	if (key_allowed(code)){
		char widgetname[16];
		w=gtk_widget_get_toplevel(w);
		snprintf(widgetname,sizeof(widgetname),"dtmf_%c",code);
		return GTK_BUTTON(linphone_gtk_get_widget(w,widgetname));
	}
	return NULL;
}

void linphone_gtk_keypad_key_pressed(GtkWidget *w, GdkEvent *event, gpointer userdata){
	GtkButton *button=get_button_from_key(w,event);
	if (button) {
		linphone_gtk_dtmf_pressed(button);
		/*g_signal_emit_by_name(button, "button-press-event");*/
	}
}

void linphone_gtk_keypad_key_released(GtkWidget *w, GdkEvent *event, gpointer userdata){
	GtkButton *button=get_button_from_key(w,event);
	if (button) {
		linphone_gtk_dtmf_released(button);
		/*g_signal_emit_by_name(button, "button-release-event");*/
	}
}

void linphone_gtk_create_keypad(GtkWidget *button){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *k=(GtkWidget *)g_object_get_data(G_OBJECT(mw),"keypad");
	GtkWidget *keypad;
	if(k!=NULL){
		gtk_widget_destroy(k);
	}
	keypad=linphone_gtk_create_window("keypad");
	linphone_gtk_connect_digits(keypad);
	linphone_gtk_init_dtmf_table(keypad);
	g_object_set_data(G_OBJECT(mw),"keypad",(gpointer)keypad);
	if(!GPOINTER_TO_INT(g_object_get_data(G_OBJECT(mw),"show_abcd"))){
		gtk_widget_hide(linphone_gtk_get_widget(keypad,"dtmf_A"));
		gtk_widget_hide(linphone_gtk_get_widget(keypad,"dtmf_B"));
		gtk_widget_hide(linphone_gtk_get_widget(keypad,"dtmf_C"));
		gtk_widget_hide(linphone_gtk_get_widget(keypad,"dtmf_D"));
		gtk_table_resize(GTK_TABLE(linphone_gtk_get_widget(keypad,"dtmf_table")),4,3);
	}
	gtk_widget_show(keypad);
}

static void linphone_gtk_init_main_window(){
	GtkWidget *main_window;
	linphone_gtk_configure_main_window();
	linphone_gtk_manage_login();
	load_uri_history();
	linphone_gtk_load_identities();
	linphone_gtk_set_my_presence(linphone_core_get_presence_info(linphone_gtk_get_core()));
	linphone_gtk_show_friends();
	linphone_core_reset_missed_calls_count(linphone_gtk_get_core());
	main_window=linphone_gtk_get_main_window();
	linphone_gtk_call_log_update(main_window);

	linphone_gtk_update_call_buttons (NULL);
	g_object_set_data(G_OBJECT(main_window),"keypad",NULL);
	g_object_set_data(G_OBJECT(main_window),"is_conf",GINT_TO_POINTER(FALSE));
	/*prevent the main window from being destroyed by a user click on WM controls, instead we hide it*/
	g_signal_connect (G_OBJECT (main_window), "delete-event",
		G_CALLBACK (linphone_gtk_close), main_window);
#ifdef HAVE_GTK_OSX
	{
		GtkWidget *menubar=linphone_gtk_get_widget(main_window,"menubar1");
		GtkosxApplication *theMacApp = gtkosx_application_get();
		gtkosx_application_set_menu_bar(theMacApp,GTK_MENU_SHELL(menubar));
		gtk_widget_hide(menubar);
		gtkosx_application_ready(theMacApp);
	}
	g_signal_connect(G_OBJECT(main_window), "window-state-event",G_CALLBACK(on_window_state_event), NULL);
#endif
	linphone_gtk_check_menu_items();
	linphone_core_enable_video_preview(linphone_gtk_get_core(),FALSE);
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

static void linphone_gtk_quit_core(void){
	linphone_gtk_unmonitor_usb();
	g_source_remove_by_user_data(linphone_gtk_get_core());
#ifdef BUILD_WIZARD
	linphone_gtk_close_assistant();
#endif
	linphone_gtk_set_ldap(NULL);
	linphone_gtk_destroy_log_window();
	linphone_core_destroy(the_core);
	linphone_gtk_log_uninit();
}

static void linphone_gtk_quit(void){
	if (!quit_done){
		quit_done=TRUE;
		linphone_gtk_quit_core();
		linphone_gtk_uninit_instance();
#ifndef HAVE_GTK_OSX
		g_object_unref(icon);
		icon=NULL;
#endif
#ifdef HAVE_NOTIFY
		notify_uninit();
#endif
		gtk_widget_destroy(the_ui);
		the_ui=NULL;
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

static void linphone_gtk_init_ui(void){
	linphone_gtk_init_main_window();

#ifdef BUILD_WIZARD
	// Veryfing if at least one sip account is configured. If not, show wizard
	if (linphone_core_get_proxy_config_list(linphone_gtk_get_core()) == NULL) {
		linphone_gtk_show_assistant();
	}
#endif

	if(run_audio_assistant){
		linphone_gtk_show_audio_assistant();
		start_option=START_AUDIO_ASSISTANT;
		iconified = TRUE;
	}
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
}

int main(int argc, char *argv[]){
	char *config_file;
	const char *factory_config_file;
	const char *lang;
	GtkSettings *settings;
	const char *icon_path=LINPHONE_ICON;
	GdkPixbuf *pbuf;
	const char *app_name="Linphone";
	LpConfig *factory;
	const char *db_file;
	GError *error=NULL;
	const char *tmp;

#if !GLIB_CHECK_VERSION(2, 31, 0)
	g_thread_init(NULL);
#endif
	gdk_threads_init();

	progpath = strdup(argv[0]);

	config_file=linphone_gtk_get_config_file(NULL);
	
	workingdir= (tmp=g_getenv("LINPHONE_WORKDIR")) ? g_strdup(tmp) : NULL;

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
#elif __APPLE__
		setenv("LANG",lang,1);
#else
		setenv("LANGUAGE",lang,1);
#endif
	}

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	setlocale(LC_ALL,"");
	/*do not use textdomain(): this sets a global default domain. On Mac OS bundle, it breaks gtk translations (obscure bug somewhere)*/
	/*textdomain (GETTEXT_PACKAGE);*/
#else
	g_message("NLS disabled.\n");
#endif
#ifdef WIN32
	gtk_rc_add_default_file("./gtkrc");
#endif
	gdk_threads_enter();

	if (!gtk_init_with_args(&argc,&argv,_("A free SIP video-phone"),
				linphone_options,NULL,&error)){
		gdk_threads_leave();
		g_critical("%s", error->message);
		return -1;
	}
	if (config_file) free(config_file);
	if (custom_config_file && !g_path_is_absolute(custom_config_file)) {
		gchar *res = g_get_current_dir();
		res = g_strjoin(G_DIR_SEPARATOR_S, res, custom_config_file, NULL);
		free(custom_config_file);
		custom_config_file = res;
	}
	config_file=linphone_gtk_get_config_file(custom_config_file);

	if(run_audio_assistant) start_option=START_AUDIO_ASSISTANT;
	if(addr_to_call != NULL) start_option=START_LINPHONE_WITH_CALL;

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

#if defined(__APPLE__) && defined(ENABLE_NLS)
	/*workaround for bundles. GTK is unable to find translations in the bundle (obscure bug again).
	So we help it:*/
	{
		if (g_file_test(PACKAGE_LOCALE_DIR, G_FILE_TEST_IS_DIR)){
			bindtextdomain("gtk20",PACKAGE_LOCALE_DIR);
			bindtextdomain("gdk-pixbuf",PACKAGE_LOCALE_DIR);
			bindtextdomain("glib20",PACKAGE_LOCALE_DIR);
		}
	}
#endif
	add_pixmap_directory("pixmaps");
	add_pixmap_directory(PACKAGE_DATA_DIR "/pixmaps/linphone");

	/* Now, look for the factory configuration file, we do it this late
		 since we want to have had time to change directory and to parse
		 the options, in case we needed to access the working directory */
	factory_config_file = linphone_gtk_get_factory_config_file();
	if (factory_config_file){
		factory=lp_config_new(NULL);
		lp_config_read_file(factory,factory_config_file);
		app_name=lp_config_get_string(factory,"GtkUi","title","Linphone");
		icon_path=lp_config_get_string(factory,"GtkUi","icon",LINPHONE_ICON);
	}
	g_set_application_name(app_name);
	pbuf=create_pixbuf(icon_path);
	if (pbuf!=NULL) gtk_window_set_default_icon(pbuf);

#ifdef HAVE_GTK_OSX
	GtkosxApplication *theMacApp = gtkosx_application_get();
	g_signal_connect(G_OBJECT(theMacApp),"NSApplicationDidBecomeActive",(GCallback)linphone_gtk_show_main_window,NULL);
	g_signal_connect(G_OBJECT(theMacApp),"NSApplicationWillTerminate",(GCallback)gtk_main_quit,NULL);
	/*never block termination:*/
	g_signal_connect(G_OBJECT(theMacApp),"NSApplicationBlockTermination",(GCallback)on_block_termination,NULL);
#endif

core_start:
	if (linphone_gtk_init_instance(app_name, start_option, addr_to_call) == FALSE){
		g_warning("Another running instance of linphone has been detected. It has been woken-up.");
		g_warning("This instance is going to exit now.");
		gdk_threads_leave();
		return 0;
	}

	the_ui=linphone_gtk_create_window("main");

	g_object_set_data(G_OBJECT(the_ui),"is_created",GINT_TO_POINTER(FALSE));

	linphone_gtk_create_log_window();
	linphone_core_enable_logs_with_cb(linphone_gtk_log_handler);

	db_file=linphone_gtk_message_storage_get_db_file(NULL);

	linphone_gtk_init_liblinphone(config_file, factory_config_file, db_file);

	/* do not lower timeouts under 30 ms because it exhibits a bug on gtk+/win32, with cpu running 20% all the time...*/
	gtk_timeout_add(30,(GtkFunction)linphone_gtk_iterate,(gpointer)linphone_gtk_get_core());
	gtk_timeout_add(30,(GtkFunction)linphone_gtk_check_logs,(gpointer)linphone_gtk_get_core());

	gtk_main();
	linphone_gtk_quit();

	if (restart){
		quit_done=FALSE;
		restart=FALSE;
		goto core_start;
	}
	if (config_file) free(config_file);
#ifndef HAVE_GTK_OSX
	/*workaround a bug on win32 that makes status icon still present in the systray even after program exit.*/
	if (icon) gtk_status_icon_set_visible(icon,FALSE);
#endif
	free(progpath);
	return 0;
}

