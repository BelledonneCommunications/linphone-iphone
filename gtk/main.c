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


#define VIDEOSELFVIEW_DEFAULT 0

#include "linphone.h"
#include "linphone/lpconfig.h"
#include "liblinphone_gitversion.h"
#include <bctoolbox/vfs.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef HAVE_GTK_OSX
#include <gtkosxapplication.h>
#endif

#ifdef _WIN32
#include "direct.h"
#define chdir _chdir
#ifndef F_OK
#define F_OK 00 /*visual studio does not define F_OK*/
#endif
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

#include "status_icon.h"


const char *this_program_ident_string="linphone_ident_string=" LINPHONE_VERSION;

static LinphoneCore *the_core=NULL;
static GtkWidget *the_ui=NULL;
static LinphoneLDAPContactProvider* ldap_provider = NULL;

static void linphone_gtk_global_state_changed(LinphoneCore *lc, LinphoneGlobalState state, const char*str);
static void linphone_gtk_registration_state_changed(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState rs, const char *msg);
static void linphone_gtk_notify_recv(LinphoneCore *lc, LinphoneFriend * fid);
static void linphone_gtk_new_unknown_subscriber(LinphoneCore *lc, LinphoneFriend *lf, const char *url);
static void linphone_gtk_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username, const char *domain);
static void linphone_gtk_configuring_status(LinphoneCore *lc, LinphoneConfiguringState status, const char *message);
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
static gchar * addr_to_call = NULL;
static int start_option = START_LINPHONE;
static gboolean no_video=FALSE;
static gboolean iconified=FALSE;
static gboolean run_audio_assistant=FALSE;
static gboolean version=FALSE;
static gboolean selftest=FALSE;
static gchar *workingdir=NULL;
static char *progpath=NULL;
gchar *linphone_logfile=NULL;
static gboolean workaround_gtk_entry_chinese_bug=FALSE;
static gchar *custom_config_file=NULL;
static gboolean restart=FALSE;
static GtkWidget *config_fetching_dialog=NULL;

#if _MSC_VER

#define LINPHONE_OPTION(optlname, optsname, optarg, optargdata, optdesc) \
{ \
	optlname, \
	optsname, \
	0, \
	optarg, \
	optargdata, \
	optdesc, \
	NULL \
}

#else

#define LINPHONE_OPTION(optlname, optsname, optarg, optargdata, optdesc) \
{ \
	.long_name = optlname, \
	.short_name = optsname, \
	.arg = optarg, \
	.arg_data = optargdata, \
	.description = optdesc, \
}

#endif

static GOptionEntry linphone_options[]={
	LINPHONE_OPTION("verbose",             '\0', G_OPTION_ARG_NONE,     (gpointer)&verbose,              N_("log to stdout some debug information while running.")),
	LINPHONE_OPTION("version",             '\0', G_OPTION_ARG_NONE,     (gpointer)&version,              N_("display version and exit.")),
	LINPHONE_OPTION("logfile",             'l',  G_OPTION_ARG_STRING,   &linphone_logfile,               N_("path to a file to write logs into.")),
	LINPHONE_OPTION("no-video",            '\0', G_OPTION_ARG_NONE,     (gpointer)&no_video,             N_("Start linphone with video disabled.")),
	LINPHONE_OPTION("iconified",           '\0', G_OPTION_ARG_NONE,     (gpointer)&iconified,            N_("Start only in the system tray, do not show the main interface.")),
	LINPHONE_OPTION("call",                'c',  G_OPTION_ARG_STRING,   &addr_to_call,                   N_("address to call right now")),
	LINPHONE_OPTION("workdir",             '\0', G_OPTION_ARG_STRING,   (gpointer) & workingdir,         N_("Specifiy a working directory (should be the base of the installation, eg: c:\\Program Files\\Linphone)")),
	LINPHONE_OPTION("config",              '\0', G_OPTION_ARG_FILENAME, (gpointer) &custom_config_file,  N_("Configuration file")),
	LINPHONE_OPTION("run-audio-assistant", '\0', G_OPTION_ARG_NONE,     (gpointer) &run_audio_assistant, N_("Run the audio assistant")),
	LINPHONE_OPTION("selftest",            '\0', G_OPTION_ARG_NONE,     (gpointer) &selftest,            N_("Run self test and exit 0 if succeed")),
	{0}
};

#define INSTALLED_XML_DIR PACKAGE_DATA_DIR "/linphone"
#define RELATIVE_XML_DIR
#define BUILD_TREE_XML_DIR "gtk"

#ifndef _WIN32
#define CONFIG_FILE ".linphonerc"
#define SECRETS_FILE ".linphone-zidcache"
#define CERTIFICATES_PATH ".linphone-usr-crt"
#else
#define CONFIG_FILE "linphonerc"
#define SECRETS_FILE "linphone-zidcache"
#define CERTIFICATES_PATH "linphone-usr-crt"
#endif

char *linphone_gtk_get_config_file(const char *filename){
	const int path_max=1024;
	char *config_file=g_malloc0(path_max);
	if (filename==NULL) filename=CONFIG_FILE;
	if (g_path_is_absolute(filename)) {
		snprintf(config_file,path_max,"%s",filename);
	} else{
#ifdef _WIN32
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
static const char *linphone_gtk_get_factory_config_file(void){
	char* path = NULL;
	/*try accessing a local file first if exists*/
	if (bctbx_file_exist(FACTORY_CONFIG_FILE)==0){
		path = ms_strdup(FACTORY_CONFIG_FILE);
	} else {
		char *progdir;

		if (progpath != NULL) {
			char *basename;
			progdir = strdup(progpath);
#ifdef _WIN32
			basename = strrchr(progdir, '\\');
			if (basename != NULL) {
				basename ++;
				*basename = '\0';
				path = ms_strdup_printf("%s\\..\\%s", progdir, FACTORY_CONFIG_FILE);
			} else if (workingdir!=NULL) {
				path = ms_strdup_printf("%s\\%s", workingdir, FACTORY_CONFIG_FILE);
			}
#else
			basename = strrchr(progdir, '/');
			if (basename != NULL) {
				basename ++;
				*basename = '\0';
				path = ms_strdup_printf("%s/../share/linphone/%s", progdir, FACTORY_CONFIG_FILE);
			}
#endif
			free(progdir);
		}
	}
	if (path) {
		ms_message("Factory config file expected at %s", path);
		//use factory file only if it exists
		if (bctbx_file_exist(path)==0){
			snprintf(_factory_config_file, sizeof(_factory_config_file), "%s", path);
			ms_free(path);
			return _factory_config_file;
		}
		ms_free(path);
	}
	return NULL;
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
		const char *factory_config_file, const char *chat_messages_db_file,
		const char *call_logs_db_file, const char *friends_db_file) {
	LinphoneCoreVTable vtable={0};
	gchar *secrets_file=linphone_gtk_get_config_file(SECRETS_FILE);
	gchar *user_certificates_dir=linphone_gtk_get_config_file(CERTIFICATES_PATH);

	vtable.global_state_changed=linphone_gtk_global_state_changed;
	vtable.call_state_changed=linphone_gtk_call_state_changed;
	vtable.registration_state_changed=linphone_gtk_registration_state_changed;
	vtable.notify_presence_received=linphone_gtk_notify_recv;
	vtable.new_subscription_requested=linphone_gtk_new_unknown_subscriber;
	vtable.auth_info_requested=linphone_gtk_auth_info_requested;
	vtable.call_log_updated=linphone_gtk_call_log_updated;
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
	linphone_core_set_user_certificates_path(the_core,user_certificates_dir);
	g_free(user_certificates_dir);
	linphone_core_enable_video_capture(the_core, TRUE);
	linphone_core_enable_video_display(the_core, TRUE);
	linphone_core_set_native_video_window_id(the_core,LINPHONE_VIDEO_DISPLAY_NONE);/*don't create the window*/
	if (no_video) {
		_linphone_gtk_enable_video(FALSE);
		linphone_gtk_set_ui_config_int("videoselfview",0);
	}
	if (chat_messages_db_file) linphone_core_set_chat_database_path(the_core,chat_messages_db_file);
	if (call_logs_db_file) linphone_core_set_call_logs_database_path(the_core, call_logs_db_file);
	if (friends_db_file) linphone_core_set_friends_database_path(the_core, friends_db_file);
}

LinphoneCore *linphone_gtk_get_core(void){
	return the_core;
}

GtkWidget *linphone_gtk_get_main_window(void){
	return the_ui;
}

void linphone_gtk_destroy_main_window(void) {
	linphone_gtk_destroy_window(the_ui);
	the_ui = NULL;
}

static void linphone_gtk_configure_window(GtkWidget *w, const char *window_name){
	static const char *hiddens=NULL;
	static const char *shown=NULL;
	static bool_t config_loaded=FALSE;
	if (linphone_gtk_get_core()==NULL) return;
	if (config_loaded==FALSE){
		hiddens=linphone_gtk_get_ui_config("hidden_widgets",NULL);
		shown=linphone_gtk_get_ui_config("shown_widgets",NULL);
		config_loaded=TRUE;
	}
	if (hiddens) linphone_gtk_visibility_set(hiddens,window_name,w,FALSE);
	if (shown) linphone_gtk_visibility_set(shown,window_name,w,TRUE);
}

static int get_ui_file(const char *name, char *path, int pathsize){
	snprintf(path,pathsize,"%s/%s.ui",BUILD_TREE_XML_DIR,name);
	if (bctbx_file_exist(path)!=0){
		snprintf(path,pathsize,"%s/%s.ui",INSTALLED_XML_DIR,name);
		if (bctbx_file_exist(path)!=0){
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

GtkWidget *linphone_gtk_create_widget(const char *widget_name) {
	char path[2048];
	GtkBuilder *builder = gtk_builder_new();
	GError *error = NULL;
	GObject *obj;

	if(get_ui_file(widget_name, path, sizeof(path)) == -1) goto fail;

	gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);

	if(gtk_builder_add_from_file(builder, path, &error) == 0) {
		g_error("Couldn't load builder file: %s", error->message);
		g_error_free(error);
		goto fail;
	}

	obj = gtk_builder_get_object(builder, widget_name);
	if(obj == NULL) {
		g_error("'%s' widget not found", widget_name);
		goto fail;
	}
	g_object_set_data(G_OBJECT(obj), "builder", builder);
	g_signal_connect_data(G_OBJECT(obj),"destroy",(GCallback)g_object_unref,builder, NULL, G_CONNECT_AFTER|G_CONNECT_SWAPPED);
	gtk_builder_connect_signals(builder, obj);

	return GTK_WIDGET(obj);

fail:
	g_object_unref(builder);
	return NULL;
}

GtkWidget *linphone_gtk_create_window(const char *window_name, GtkWidget *parent){
	GtkWidget *w = linphone_gtk_create_widget(window_name);
	if(w) {
		linphone_gtk_configure_window(w,window_name);
		if(parent) {
			gtk_window_set_transient_for(GTK_WINDOW(w), GTK_WINDOW(parent));
			gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER_ON_PARENT);
		}
	}
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

void linphone_gtk_show_about(void){
	struct stat filestat;
	const char *license_file=PACKAGE_DATA_DIR "/linphone/COPYING";
	GtkWidget *about;
	const char *tmp;
	GdkPixbuf *logo=create_pixbuf(
		linphone_gtk_get_ui_config("logo","linphone-banner.png"));
	static const char *defcfg="defcfg";

	about=linphone_gtk_create_window("about", the_ui);

	gtk_about_dialog_set_url_hook(about_url_clicked,NULL,NULL);

	memset(&filestat,0,sizeof(filestat));
	if (stat(license_file,&filestat)!=0){
		license_file="COPYING";
		stat(license_file,&filestat);
	}
	if (filestat.st_size>0){
		char *license=g_malloc(filestat.st_size+1);
		FILE *f=fopen(license_file,"r");
		if (f && fread(license,1,filestat.st_size,f)>0){
			license[filestat.st_size]='\0';
			gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about),license);
		}
		g_free(license);
	}
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about),linphone_core_get_version());
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about),linphone_gtk_get_ui_config("title","Linphone"));
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about),linphone_gtk_get_ui_config("home","http://www.linphone.org"));
	if (logo) {
		gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about), logo);
		g_object_unref(logo);
	}
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
		if (g_object_get_data(G_OBJECT(mw), "login_frame") == NULL){
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
	gtk_tree_model_get(gtk_entry_completion_get_model(completion),iter,0,&address,-1);

	if(address) {
		gchar *tmp = g_utf8_casefold(address,-1);
		if (strstr(tmp,key)) ret=TRUE;
		g_free(tmp);
		g_free(address);
	}

	return ret;
}

static void load_uri_history(void){
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

static void save_uri_history(void){
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

void on_contact_provider_search_results( LinphoneContactSearch* req, bctbx_list_t* friends, void* data )
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
#ifdef HAVE_GTK_OSX
	GtkWidget *icon = linphone_gtk_get_widget(w, "history_tab_icon");
	GtkWidget *label = linphone_gtk_get_widget(w, "history_tab_label");
	gtk_misc_set_alignment(GTK_MISC(icon), 0.5f, 0.25f);
	gtk_misc_set_alignment(GTK_MISC(label), 0.5f, 0.f);
#endif
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
	const bctbx_list_t *calls=linphone_core_get_calls(lc);
	GtkWidget *button;
	bool_t add_call=(calls!=NULL);
	int call_list_size=bctbx_list_size(calls);
	GtkWidget *conf_frame;

	button=linphone_gtk_get_widget(mw,"start_call");
	gtk_widget_set_sensitive(button,TRUE);
	gtk_widget_set_visible(button,!add_call);

	button=linphone_gtk_get_widget(mw,"add_call");

	if (linphone_core_sound_resources_locked(lc) || (call && linphone_call_get_state(call)==LinphoneCallIncomingReceived)) {
		gtk_widget_set_sensitive(button,FALSE);
	} else {
		gtk_widget_set_sensitive(button,TRUE);
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

#ifdef _WIN32
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

gchar *linphone_gtk_get_snapshot_path(void) {
	const char *dir=g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
	char filename[256]={0};
	char date[64]={0};
	time_t curtime=time(NULL);
	struct tm loctime;
	const char *ext="jpg";

#ifdef _WIN32
	loctime=*localtime(&curtime);
#else
	localtime_r(&curtime,&loctime);
#endif
	snprintf(date,sizeof(date)-1,"%i%02i%02i-%02i%02i%02i",loctime.tm_year+1900,loctime.tm_mon+1,loctime.tm_mday, loctime.tm_hour, loctime.tm_min, loctime.tm_sec);
	snprintf(filename,sizeof(filename)-1,"%s-snapshot-%s.%s",
			linphone_gtk_get_ui_config("title","Linphone"),
			date, ext);
	if (!dir) {
		ms_message ("No directory for pictures, using [%s] instead",dir=getenv("HOME"));
	}
	return g_build_filename(dir,filename,NULL);
}

static gboolean linphone_gtk_start_call_do(GtkWidget *uri_bar){
	const char *entered=gtk_entry_get_text(GTK_ENTRY(uri_bar));
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneAddress *addr=linphone_core_interpret_url(lc,entered);

	if (addr!=NULL){
		LinphoneCallParams *params=linphone_core_create_call_params(lc, NULL);
		gchar *record_file=linphone_gtk_get_record_path(addr,FALSE);
		linphone_call_params_set_record_file(params,record_file);
		linphone_core_invite_address_with_params(lc,addr,params);
		completion_add_text(GTK_ENTRY(uri_bar),entered);
		linphone_address_unref(addr);
		linphone_call_params_unref(params);
		g_free(record_file);
	}else{
		linphone_gtk_call_terminated(NULL,NULL);
	}
	return FALSE;
}

static void accept_incoming_call(LinphoneCall *call){
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneCallParams *params = linphone_core_create_call_params(lc, call);
	gchar *record_file=linphone_gtk_get_record_path(linphone_call_get_remote_address(call),FALSE);
	linphone_call_params_set_record_file(params,record_file);
	linphone_core_accept_call_with_params(lc,call,params);
	linphone_call_params_unref(params);
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

void linphone_gtk_start_chat(GtkWidget *w){
	GtkWidget *mw=gtk_widget_get_toplevel(w);
	GtkWidget *uri_bar=linphone_gtk_get_widget(mw,"uribar");
	const char *entered=gtk_entry_get_text(GTK_ENTRY(uri_bar));
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneAddress *addr=linphone_core_interpret_url(lc,entered);
	if (addr) {
		linphone_gtk_friend_list_set_chat_conversation(addr);
		linphone_address_unref(addr);
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
	linphone_core_refresh_registers(lc);
}

static gboolean grab_focus(GtkWidget *w){
	gtk_widget_grab_focus(w);
	return FALSE;
}

void linphone_gtk_viewswitch_changed(GtkNotebook *notebook, GtkWidget *page, gint page_num, gpointer user_data){
	GtkWidget *main_window = linphone_gtk_get_main_window();
	GtkWidget *friendlist = linphone_gtk_get_widget(main_window,"contact_list");
	GtkWidget *w = (GtkWidget*)g_object_get_data(G_OBJECT(friendlist),"chatview");

	if (page_num == gtk_notebook_page_num(GTK_NOTEBOOK(notebook),w)) {
		g_idle_add((GSourceFunc)grab_focus,linphone_gtk_get_widget(page,"text_entry"));
	}
}

static void linphone_gtk_notify_recv(LinphoneCore *lc, LinphoneFriend * fid){
	linphone_gtk_show_friends();
}

static void linphone_gtk_new_subscriber_response(GtkWidget *dialog, guint response_id, LinphoneFriend *lf){
	switch(response_id){
		case GTK_RESPONSE_YES:
			linphone_gtk_show_contact(lf, the_ui);
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

	message=g_strdup_printf(_("%s would like to add you to his/her contact list.\nWould you add him/her to your contact list and allow him/her to see your presence status?\nIf you answer no, this person will be temporarily blacklisted."),url);
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
	GtkWidget *w=linphone_gtk_create_window("password", the_ui);
	GtkWidget *label=linphone_gtk_get_widget(w,"message");
	LinphoneAuthInfo *info;
	gchar *msg;
	GtkWidget *mw=linphone_gtk_get_main_window();

	if (mw && g_object_get_data(G_OBJECT(mw), "login_frame") != NULL){
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


static void linphone_gtk_configuring_status(LinphoneCore *lc, LinphoneConfiguringState status, const char *message) {
	if (config_fetching_dialog) linphone_gtk_close_config_fetching(config_fetching_dialog, status);
	config_fetching_dialog=NULL;
}

static void linphone_gtk_call_log_updated(LinphoneCore *lc, LinphoneCallLog *cl){
	GtkWidget *w=(GtkWidget*)g_object_get_data(G_OBJECT(linphone_gtk_get_main_window()),"call_logs");
	if (w) linphone_gtk_call_log_update(w);
	linphone_gtk_call_log_update(linphone_gtk_get_main_window());
}

#ifdef HAVE_NOTIFY
static bool_t notify_actions_supported(void) {
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
		GError *error = NULL;
		const char *icon_name = linphone_gtk_get_ui_config("icon_name", LINPHONE_ICON_NAME);
		GdkPixbuf *pbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), icon_name, 48, 0, &error);
		if(error) {
			g_warning("Could not load '%s' icon: %s", icon_name, error->message);
			g_error_free(error);
		}
		notify_notification_set_image_from_pixbuf(n, pbuf);
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

void linphone_gtk_notify(LinphoneCall *call, LinphoneChatMessage *chat_message, const char *msg){
#ifdef HAVE_NOTIFY
	if (!notify_is_initted())
		if (!notify_init ("Linphone")) ms_error("Libnotify failed to init.");
#endif
	if (!call) {
#ifdef HAVE_NOTIFY
		if (chat_message) {
			const LinphoneAddress *address = linphone_chat_message_get_peer_address(chat_message);
			char *remote = linphone_address_as_string(address);
			make_notification(remote, linphone_chat_message_get_text(chat_message));
		} else {
			if (!notify_notification_show(notify_notification_new("Linphone",msg,NULL
#ifdef HAVE_NOTIFY1
				,NULL
#endif
				),NULL)) {
				ms_error("Failed to send notification.");
			}
		}
#else
		linphone_gtk_show_main_window();
#endif
	} else if (!gtk_window_is_active((GtkWindow*)linphone_gtk_get_main_window())) {
		gboolean show_main_window = FALSE;
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
				if (n){
					if (notify_actions_supported()) {
						notify_notification_add_action (n,"answer", _("Answer"),
							NOTIFY_ACTION_CALLBACK(linphone_gtk_answer_clicked),NULL,NULL);
						notify_notification_add_action (n,"decline",_("Decline"),
							NOTIFY_ACTION_CALLBACK(linphone_gtk_decline_clicked),NULL,NULL);
					}
					show_notification(n);
				}else show_main_window = TRUE;
			break;
			case LinphoneCallPausedByRemote:
				make_notification(_("Call paused"),body=g_markup_printf_escaped(_("<b>by %s</b>"),remote));
			break;
			default:
			break;
		}
		if (body) g_free(body);
		if (remote) g_free(remote);
#else
		if (linphone_call_get_state(call) == LinphoneCallIncomingReceived)
			show_main_window = TRUE;
#endif
		if (show_main_window) linphone_gtk_show_main_window();
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

static void on_call_updated_response(GtkWidget *dialog, gint responseid, gpointer user_data){
	LinphoneCall *call = (LinphoneCall *)g_object_get_data(G_OBJECT(dialog), "call");
	if (linphone_call_get_state(call)==LinphoneCallUpdatedByRemote){
		LinphoneCore *lc=linphone_call_get_core(call);
		LinphoneCallParams *params = linphone_core_create_call_params(lc, call);
		linphone_call_params_enable_video(params,responseid==GTK_RESPONSE_YES);
		linphone_core_accept_call_update(lc,call,params);
		linphone_call_params_unref(params);
	}
	g_source_remove_by_user_data(dialog);
	gtk_widget_destroy(dialog);
}

static void on_call_updated_timeout(GtkWidget *dialog){
	on_call_updated_response(dialog, GTK_RESPONSE_NO, NULL);
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
	if (!video_used && video_requested && !pol->automatically_accept){
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
			g_object_set_data_full(G_OBJECT(dialog), "call", linphone_call_ref(call), (GDestroyNotify)linphone_call_unref);
			g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(on_call_updated_response), NULL);
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
			if (linphone_gtk_auto_answer_enabled())  {
				int delay = linphone_gtk_get_ui_config_int("auto_answer_delay", 2000);
				linphone_call_ref(call);
				g_timeout_add(delay, (GSourceFunc)linphone_gtk_auto_answer, call);
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
	linphone_gtk_notify(call, NULL, msg);
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
	const char *icon_name=NULL;

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
			icon_name="linphone-ok";
		break;
		case LinphoneRegistrationProgress:
			icon_name="linphone-inprogress";
		break;
		case LinphoneRegistrationCleared:
			icon_name=NULL;
		break;
		case LinphoneRegistrationFailed:
			icon_name="linphone-failed";
		break;
		default:
		break;
	}
	gtk_list_store_set(GTK_LIST_STORE(model),&iter,1,icon_name,-1);
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

void linphone_gtk_open_browser(const char *uri) {
#ifdef __APPLE__
	GError *error = NULL;
	char cmd_line[256];

	g_snprintf(cmd_line, sizeof(cmd_line), "%s %s", "/usr/bin/open", uri);
	g_spawn_command_line_async(cmd_line, &error);
	if (error) {
		g_warning("Could not open %s: %s", uri, error->message);
		g_error_free(error);
	}
#elif defined(_WIN32)
	HINSTANCE instance = ShellExecute(NULL, "open", uri, NULL, NULL, SW_SHOWNORMAL);
	if ((int)instance <= 32) {
		g_warning("Could not open %s (error #%i)", uri, (int)instance);
	}
#else
	GError *error = NULL;
	gtk_show_uri(NULL, uri, GDK_CURRENT_TIME, &error);
	if (error) {
		g_warning("Could not open %s: %s", uri, error->message);
		g_error_free(error);
	}
#endif
}

void linphone_gtk_link_to_website(GtkWidget *item){
	const gchar *home=(const gchar*)g_object_get_data(G_OBJECT(item),"home");
	linphone_gtk_open_browser(home);
}

static GtkWidget *create_icon_menu(void){
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

#ifndef HAVE_GTK_OSX
void linphone_gtk_save_main_window_position(GtkWindow* mw, GdkEvent *event, gpointer data){
	   gtk_window_get_position(GTK_WINDOW(mw), &main_window_x, &main_window_y);
}
#endif

static void handle_icon_click(LinphoneStatusIcon *si, void *user_data) {
#ifndef HAVE_GTK_OSX
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
#endif
}

static void linphone_gtk_status_icon_initialised_cb(LinphoneStatusIconParams *params) {
	LinphoneStatusIcon *icon = linphone_status_icon_get();
	if(icon) {
		linphone_status_icon_start(icon, params);
	}
	linphone_status_icon_params_unref(params);
}

static void linphone_gtk_init_status_icon(void) {
	GtkWidget *menu = create_icon_menu();
	LinphoneStatusIconParams *params = linphone_status_icon_params_new();
	linphone_status_icon_params_set_menu(params, menu);
	linphone_status_icon_params_set_title(params, _("Linphone"));
	linphone_status_icon_params_set_description(params, _("A video internet phone"));
	linphone_status_icon_params_set_on_click_cb(params, handle_icon_click, NULL);

	if(linphone_status_icon_init(
		(LinphoneStatusIconReadyCb)linphone_gtk_status_icon_initialised_cb,
		params)) {

		LinphoneStatusIcon *icon = linphone_status_icon_get();
		if(icon) {
			linphone_status_icon_start(icon, params);
		}
		linphone_status_icon_params_unref(params);
	}
}

void linphone_gtk_status_icon_set_blinking(gboolean val) {
	LinphoneStatusIcon *icon = linphone_status_icon_get();
	if(icon) {
		linphone_status_icon_enable_blinking(icon, val);
	}
#ifdef __APPLE__
	linphone_gtk_update_badge_count();
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
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(box),r2,"icon-name",1);
	g_object_set(G_OBJECT(r1),"ellipsize",PANGO_ELLIPSIZE_END,NULL);
	gtk_combo_box_set_model(box,GTK_TREE_MODEL(store));
}

void linphone_gtk_load_identities(void){
	const bctbx_list_t *elem;
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
	def = linphone_core_get_default_proxy_config(linphone_gtk_get_core());
	def_identity=g_strdup_printf(_("%s (Default)"),linphone_core_get_primary_contact(linphone_gtk_get_core()));
	gtk_list_store_append(store,&iter);
	gtk_list_store_set(store,&iter,0,def_identity,1,NULL,2,NULL,-1);
	g_free(def_identity);
	for(i=1,elem=linphone_core_get_proxy_config_list(linphone_gtk_get_core());
			elem!=NULL;
			elem=bctbx_list_next(elem),i++){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,0,linphone_proxy_config_get_identity(cfg),1,
						   linphone_proxy_config_is_registered(cfg) ? "linphone-ok" : NULL,
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
		linphone_call_send_dtmf(linphone_core_get_current_call(linphone_gtk_get_core()),label[0]);
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

static gboolean linphone_gtk_can_manage_accounts(void){
	LinphoneCore *lc=linphone_gtk_get_core();
	const bctbx_list_t *elem;
	for(elem=linphone_core_get_sip_setups(lc);elem!=NULL;elem=elem->next){
		SipSetup *ss=(SipSetup*)elem->data;
		if (sip_setup_get_capabilities(ss) & SIP_SETUP_CAP_ACCOUNT_MANAGER){
			return TRUE;
		}
	}
	return FALSE;
}

static void linphone_gtk_configure_main_window(void){
	static gboolean config_loaded=FALSE;
	static const char *title;
	static const char *home;
	static const char *search_icon;
	static gboolean update_check_menu;
	static gboolean show_abcd;
	GtkWidget *w=linphone_gtk_get_main_window();

	if (!config_loaded){
		title=linphone_gtk_get_ui_config("title","Linphone");
		home=linphone_gtk_get_ui_config("home","http://www.linphone.org");
		search_icon=linphone_gtk_get_ui_config("directory_search_icon",NULL);
		update_check_menu=linphone_gtk_get_ui_config_int("update_check_menu",0);
		show_abcd=linphone_gtk_get_ui_config_int("show_abcd",1);
		config_loaded=TRUE;
	}
	linphone_gtk_configure_window(w,"main_window");
	if (title) {
		gtk_window_set_title(GTK_WINDOW(w),title);
	}
	if (search_icon){
		GdkPixbuf *pbuf=create_pixbuf(search_icon);
		if(pbuf) {
			gtk_image_set_from_pixbuf(GTK_IMAGE(linphone_gtk_get_widget(w,"directory_search_button_icon")),pbuf);
			g_object_unref(G_OBJECT(pbuf));
		}
	}
	if (home){
		gchar *tmp;
		GtkWidget *menu_item=linphone_gtk_get_widget(w,"home_item");
		tmp=g_strdup(home);
		g_object_set_data_full(G_OBJECT(menu_item),"home",tmp, (GDestroyNotify)g_free);
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
	LinphoneProxyConfig *cfg=linphone_core_get_default_proxy_config(lc);
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

static void linphone_gtk_show_keypad(void){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *k=(GtkWidget *)g_object_get_data(G_OBJECT(mw),"keypad");
	GtkWidget *keypad;
	if(k!=NULL){
		gtk_widget_destroy(k);
	}
	keypad=linphone_gtk_create_window("keypad", NULL);
	linphone_gtk_connect_digits(keypad);
	linphone_gtk_init_dtmf_table(keypad);
	g_object_set_data(G_OBJECT(mw),"keypad", keypad);
	if(!GPOINTER_TO_INT(g_object_get_data(G_OBJECT(mw),"show_abcd"))){
		gtk_widget_hide(linphone_gtk_get_widget(keypad,"dtmf_A"));
		gtk_widget_hide(linphone_gtk_get_widget(keypad,"dtmf_B"));
		gtk_widget_hide(linphone_gtk_get_widget(keypad,"dtmf_C"));
		gtk_widget_hide(linphone_gtk_get_widget(keypad,"dtmf_D"));
		gtk_table_resize(GTK_TABLE(linphone_gtk_get_widget(keypad,"dtmf_table")),4,3);
	}
	gtk_widget_show(keypad);
}

static void linphone_gtk_destroy_keypad(void) {
	GtkWidget *mw = linphone_gtk_get_main_window();
	GtkWidget *keypad = GTK_WIDGET(g_object_get_data(G_OBJECT(mw), "keypad"));
	if(keypad) {
		gtk_widget_destroy(keypad);
		g_object_set_data(G_OBJECT(mw), "keypad", NULL);
	}
}

void linphone_gtk_show_keypad_checked(GtkCheckMenuItem *check_menu_item) {
	if(gtk_check_menu_item_get_active(check_menu_item)) {
		linphone_gtk_show_keypad();
	} else {
		linphone_gtk_destroy_keypad();
	}
}

void linphone_gtk_import_contacts(void) {
	GtkWidget *mw = linphone_gtk_get_main_window();
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Open vCard file", (GtkWindow *)mw, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		LinphoneCore *lc = linphone_gtk_get_core();
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		LinphoneFriendList *list = linphone_core_get_default_friend_list(lc);
		linphone_friend_list_import_friends_from_vcard4_file(list, filename);
		g_free(filename);
		linphone_gtk_show_friends();
	}
	gtk_widget_destroy(dialog);
}

void linphone_gtk_export_contacts(void) {
	GtkWidget *mw = linphone_gtk_get_main_window();
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Save vCards as", (GtkWindow *)mw, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

	if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		LinphoneCore *lc = linphone_gtk_get_core();
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		LinphoneFriendList *list = linphone_core_get_default_friend_list(lc);
		linphone_friend_list_export_friends_as_vcard4_file(list, filename);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

gboolean linphone_gtk_keypad_destroyed_handler(void) {
	GtkWidget *mw = linphone_gtk_get_main_window();
	GtkWidget *show_keypad_item = linphone_gtk_get_widget(mw, "show_keypad_menu_item");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(show_keypad_item), FALSE);
	return FALSE;
}

void linphone_gtk_update_status_bar_icons(void) {
	GtkWidget *mw = linphone_gtk_get_main_window();
	GtkWidget *icon = linphone_gtk_get_widget(mw, "autoanswer_icon");
	gtk_widget_set_visible(icon, linphone_gtk_auto_answer_enabled());
}

static void linphone_gtk_init_main_window(void){
	GtkWidget *main_window;
	linphone_gtk_configure_main_window();
	linphone_gtk_manage_login();
	linphone_gtk_load_identities();
	linphone_gtk_set_my_presence(linphone_core_get_presence_info(linphone_gtk_get_core()));
	linphone_gtk_show_friends();
	linphone_gtk_update_status_bar_icons();
	load_uri_history();
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
		gtk_widget_show(main_window);
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
#ifdef BUILD_WIZARD
	gtk_widget_set_visible(linphone_gtk_get_widget(main_window, "assistant_item"), TRUE);
#else
	gtk_widget_set_visible(linphone_gtk_get_widget(main_window, "assistant_item"), FALSE);
#endif
}

void linphone_gtk_log_handler(const char*domain, OrtpLogLevel lev, const char *fmt, va_list args){
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
	char method[20] = "";
	LinphoneAddress *addr = linphone_address_new(refer_to);
	if(addr) {
		const char *tmp = linphone_address_get_method_param(addr);
		strncpy(method, tmp, sizeof(20));
		linphone_address_unref(addr);
	}
	if(strlen(method) == 0 || strcmp(method, "INVITE") == 0) {
		GtkEntry * uri_bar =GTK_ENTRY(linphone_gtk_get_widget(
			linphone_gtk_get_main_window(), "uribar"));
		char *text;
		linphone_gtk_notify(NULL,NULL,(text=ms_strdup_printf(_("We are transferred to %s"),refer_to)));
		g_free(text);
		gtk_entry_set_text(uri_bar, refer_to);
		linphone_gtk_start_call(linphone_gtk_get_main_window());
	}
}

static void linphone_gtk_check_soundcards(void){
	const char **devices=linphone_core_get_sound_devices(linphone_gtk_get_core());
	if (devices==NULL || devices[0]==NULL){
		linphone_gtk_display_something(GTK_MESSAGE_WARNING,
			_("No sound cards have been detected on this computer.\n"
				"You won't be able to send or receive audio calls."));
	}
}

static void linphone_gtk_quit_core(void){
#ifdef HAVE_GTK_OSX
	{
		GtkosxApplication *theMacApp = gtkosx_application_get();
		gtkosx_application_set_menu_bar(theMacApp,NULL);
	}
#endif
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
#ifdef HAVE_NOTIFY
		notify_uninit();
#endif
		linphone_status_icon_uninit();
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

	linphone_gtk_init_status_icon();

	if (!iconified){
		linphone_gtk_show_main_window();
		linphone_gtk_check_soundcards();
	}
	if (linphone_gtk_get_ui_config_int("update_check_menu",0)==0)
		linphone_gtk_check_for_new_version();
	linphone_gtk_monitor_usb();
}

static void sigint_handler(int signum){
	gtk_main_quit();
}

static void populate_xdg_data_dirs_envvar(void) {
#ifndef _WIN32
	int i;
	gchar *value;
	gchar **paths;

	if(g_getenv("XDG_DATA_DIRS") == NULL) {
		value = g_strdup("/usr/share:/usr/local/share:/opt/local/share");
	} else {
		value = g_strdup(g_getenv("XDG_DATA_DIRS"));
	}
	paths = g_strsplit(value, ":", -1);
	for(i=0; paths[i] && strcmp(paths[i], PACKAGE_DATA_DIR) != 0; i++);
	if(paths[i] == NULL) {
		gchar *new_value = g_strdup_printf("%s:%s", PACKAGE_DATA_DIR, value);
		g_setenv("XDG_DATA_DIRS", new_value, TRUE);
		g_free(new_value);
	}
	g_strfreev(paths);
#endif
}

int main(int argc, char *argv[]){
	char *config_file;
	const char *factory_config_file;
	const char *lang;
	GtkSettings *settings;
	const char *icon_name=LINPHONE_ICON_NAME;
	const char *app_name="Linphone";
	LpConfig *factory;
	char *chat_messages_db_file, *call_logs_db_file, *friends_db_file;
	GError *error=NULL;
	const char *tmp;

#if !GLIB_CHECK_VERSION(2, 31, 0)
	g_thread_init(NULL);
#endif
	gdk_threads_init();

	progpath = strdup(argv[0]);

	config_file=linphone_gtk_get_config_file(NULL);

	workingdir= (tmp=g_getenv("LINPHONE_WORKDIR")) ? g_strdup(tmp) : NULL;

#ifdef __linux
	/*for pulseaudio:*/
	g_setenv("PULSE_PROP_media.role", "phone", TRUE);
#endif

	populate_xdg_data_dirs_envvar();

	lang=linphone_gtk_get_lang(config_file);
	if (lang == NULL || lang[0]=='\0'){
		lang = g_getenv("LANGUAGE");
		if (!lang) lang = g_getenv("LANG");
	}
	if (lang && lang[0]!='\0'){
#ifdef _WIN32
		if (strncmp(lang,"zh",2)==0){
			workaround_gtk_entry_chinese_bug=TRUE;
		}
#endif
		g_setenv("LANGUAGE",lang,1);
	}

#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	/*do not use textdomain(): this sets a global default domain. On Mac OS bundle, it breaks gtk translations (obscure bug somewhere)*/
	/*textdomain (GETTEXT_PACKAGE);*/
#else
	g_message("NLS disabled.\n");
#endif
#ifdef _WIN32
	gtk_rc_add_default_file("./gtkrc");
#endif
	gdk_threads_enter();

	if (!gtk_init_with_args(&argc,&argv,_("A free SIP video-phone"),
				linphone_options,NULL,&error)){
		gdk_threads_leave();
		g_critical("%s", error->message);
		return -1;
	}
	if(version) {
		g_message("Linphone version %s.", linphone_core_get_version());
		return 0;
	}

	if (config_file) g_free(config_file);
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
		icon_name=lp_config_get_string(factory,"GtkUi","icon_name",LINPHONE_ICON_NAME);
	}
	g_set_application_name(app_name);
	gtk_window_set_default_icon_name(icon_name);

#ifdef HAVE_GTK_OSX
	GtkosxApplication *theMacApp = gtkosx_application_get();
	g_signal_connect(G_OBJECT(theMacApp),"NSApplicationDidBecomeActive",(GCallback)linphone_gtk_show_main_window,NULL);
	g_signal_connect(G_OBJECT(theMacApp),"NSApplicationWillTerminate",(GCallback)gtk_main_quit,NULL);
	/*never block termination:*/
	g_signal_connect(G_OBJECT(theMacApp),"NSApplicationBlockTermination",(GCallback)on_block_termination,NULL);
#endif

core_start:
	if (linphone_gtk_init_instance(app_name, start_option, addr_to_call) == FALSE){
		g_warning("Another running instance of Linphone has been detected. It has been woken-up.");
		g_warning("This instance is going to exit now.");
		gdk_threads_leave();
		return 0;
	}
	the_ui=linphone_gtk_create_window("main", NULL);

	g_object_set_data(G_OBJECT(the_ui),"is_created",GINT_TO_POINTER(FALSE));
	g_signal_connect(G_OBJECT (the_ui), "key_press_event", G_CALLBACK (linphone_gtk_on_key_press), NULL);

	linphone_gtk_create_log_window();
	linphone_core_enable_logs_with_cb(linphone_gtk_log_handler);
	/*it is possible to filter in or out some logs by configuring per log domain:*/
	/*ortp_set_log_level_mask("belle-sip", ORTP_ERROR);*/

	chat_messages_db_file=linphone_gtk_message_storage_get_db_file(NULL);
	call_logs_db_file = linphone_gtk_call_logs_storage_get_db_file(NULL);
	friends_db_file = linphone_gtk_friends_storage_get_db_file(NULL);
	linphone_gtk_init_liblinphone(config_file, factory_config_file, chat_messages_db_file, call_logs_db_file, friends_db_file);
	g_free(chat_messages_db_file);
	g_free(call_logs_db_file);
	g_free(friends_db_file);

	linphone_gtk_call_log_update(the_ui);
	linphone_gtk_show_friends();

	/* do not lower timeouts under 30 ms because it exhibits a bug on gtk+/win32, with cpu running 20% all the time...*/
	gtk_timeout_add(30,(GtkFunction)linphone_gtk_iterate,(gpointer)linphone_gtk_get_core());
	gtk_timeout_add(30,(GtkFunction)linphone_gtk_check_logs,(gpointer)linphone_gtk_get_core());

	signal(SIGINT, sigint_handler);

	gtk_main();
	linphone_gtk_quit();

	if (restart){
		quit_done=FALSE;
		restart=FALSE;
		goto core_start;
	}
	if (config_file) g_free(config_file);
	free(progpath);
	/*output a translated "hello" string to the terminal, which allows the builder to check that translations are working.*/
	if (selftest){
		printf(_("Hello\n"));
	}
	return 0;
}

#ifdef _MSC_VER
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	return main(__argc, __argv);
}
#endif

GtkWidget *linphone_gtk_make_tab_header(const gchar *label, const gchar *icon_name, gboolean show_quit_button, GCallback cb, gpointer user_data) {
	GtkWidget *tab_header=gtk_hbox_new (FALSE,0);
	GtkWidget *label_widget = gtk_label_new (label);

	if(icon_name) {
		GtkWidget *icon=gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
#ifdef HAVE_GTK_OSX
		gtk_misc_set_alignment(GTK_MISC(icon), 0.5f, 0.25f);
#endif
		gtk_box_pack_start (GTK_BOX(tab_header),icon,FALSE,FALSE,4);
	}
#ifdef HAVE_GTK_OSX
	gtk_misc_set_alignment(GTK_MISC(label_widget), 0.5f, 0.f);
#endif
	gtk_box_pack_start (GTK_BOX(tab_header),label_widget,FALSE,FALSE,0);
	if(show_quit_button) {
		GtkWidget *button = gtk_button_new();
		GtkWidget *button_image=gtk_image_new_from_stock(GTK_STOCK_CLOSE,GTK_ICON_SIZE_MENU);
		gtk_button_set_image(GTK_BUTTON(button),button_image);
		gtk_button_set_relief(GTK_BUTTON(button),GTK_RELIEF_NONE);
#ifdef HAVE_GTK_OSX
		gtk_misc_set_alignment(GTK_MISC(button_image), 0.5f, 0.f);
#endif
		g_signal_connect_swapped(G_OBJECT(button),"clicked",cb,user_data);
		gtk_box_pack_end(GTK_BOX(tab_header),button,FALSE,FALSE,4);
		g_object_set_data(G_OBJECT(tab_header), "button", button);
	}
	g_object_set_data(G_OBJECT(tab_header), "label", label_widget);
	return tab_header;
}
