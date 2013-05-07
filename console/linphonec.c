/****************************************************************************
 *
 *  $Id: linphonec.c,v 1.57 2007/11/14 13:40:27 smorlat Exp $
 *
 *  Copyright (C) 2006  Sandro Santilli <strk@keybit.net>
 *  Copyright (C) 2002  Florian Winterstein <flox@gmx.net>
 *  Copyright (C) 2000  Simon MORLAT <simon.morlat@free.fr>
 *
****************************************************************************
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 ****************************************************************************/
#include <string.h>
#ifndef _WIN32_WCE
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "private.h" /*coreapi/private.h, needed for LINPHONE_VERSION */
#endif /*_WIN32_WCE*/
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>

#include <linphonecore.h>

#include "linphonec.h"

#ifdef WIN32
#include <ws2tcpip.h>
#include <ctype.h>
#ifndef _WIN32_WCE
#include <conio.h>
#endif /*_WIN32_WCE*/
#else
#include <sys/socket.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/stat.h>
#endif

#if defined(_WIN32_WCE)

#if !defined(PATH_MAX)
#define PATH_MAX 256
#endif /*PATH_MAX*/

#if !defined(strdup)
#define strdup _strdup
#endif /*strdup*/

#endif /*_WIN32_WCE*/

#ifdef HAVE_GETTEXT
#include <libintl.h>
#ifndef _
#define _(String) gettext(String)
#endif
#else
#define _(something)	(something)
#endif

#ifndef PACKAGE_DIR
#define PACKAGE_DIR ""
#endif

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif

/***************************************************************************
 *
 *  Types
 *
 ***************************************************************************/

typedef struct {
	LinphoneAuthInfo *elem[MAX_PENDING_AUTH];
	int nitems;
} LPC_AUTH_STACK;

/***************************************************************************
 *
 *  Forward declarations
 *
 ***************************************************************************/

char *lpc_strip_blanks(char *input);

static int handle_configfile_migration(void);
#if !defined(_WIN32_WCE)
static int copy_file(const char *from, const char *to);
#endif /*_WIN32_WCE*/
static int linphonec_parse_cmdline(int argc, char **argv);
static int linphonec_init(int argc, char **argv);
static int linphonec_main_loop (LinphoneCore * opm);
static int linphonec_idle_call (void);
#ifdef HAVE_READLINE
static int linphonec_initialize_readline(void);
static int linphonec_finish_readline();
static char **linephonec_readline_completion(const char *text,
	int start, int end);
#endif

/* These are callback for linphone core */
static void linphonec_prompt_for_auth(LinphoneCore *lc, const char *realm,
	const char *username);
static void linphonec_display_refer (LinphoneCore * lc, const char *refer_to);
static void linphonec_display_something (LinphoneCore * lc, const char *something);
static void linphonec_display_url (LinphoneCore * lc, const char *something, const char *url);
static void linphonec_display_warning (LinphoneCore * lc, const char *something);
static void linphonec_notify_received(LinphoneCore *lc, LinphoneCall *call, const char *from,const char *event);

static void linphonec_notify_presence_received(LinphoneCore *lc,LinphoneFriend *fid);
static void linphonec_new_unknown_subscriber(LinphoneCore *lc,
		LinphoneFriend *lf, const char *url);

static void linphonec_text_received(LinphoneCore *lc, LinphoneChatRoom *cr,
		const LinphoneAddress *from, const char *msg);
static void linphonec_display_status (LinphoneCore * lc, const char *something);
static void linphonec_dtmf_received(LinphoneCore *lc, LinphoneCall *call, int dtmf);
static void print_prompt(LinphoneCore *opm);
void linphonec_out(const char *fmt,...);
/***************************************************************************
 *
 * Global variables
 *
 ***************************************************************************/

LinphoneCore *linphonec;
FILE *mylogfile;
#ifdef HAVE_READLINE
static char *histfile_name=NULL;
static char last_in_history[256];
#endif
//auto answer (-a) option
static bool_t auto_answer=FALSE;
static bool_t answer_call=FALSE;
static bool_t vcap_enabled=FALSE;
static bool_t display_enabled=FALSE;
static bool_t preview_enabled=FALSE;
static bool_t show_general_state=FALSE;
static bool_t unix_socket=FALSE;
static bool_t linphonec_running=TRUE;
LPC_AUTH_STACK auth_stack;
static int trace_level = 0;
static char *logfile_name = NULL;
static char configfile_name[PATH_MAX];
static char zrtpsecrets[PATH_MAX];
static const char *factory_configfile_name=NULL;
static char *sip_addr_to_call = NULL; /* for autocall */
static int window_id = 0; /* 0=standalone window, or window id for embedding video */
#if !defined(_WIN32_WCE)
static ortp_pipe_t client_sock=ORTP_PIPE_INVALID;
#endif /*_WIN32_WCE*/
char prompt[PROMPT_MAX_LEN];
#if !defined(_WIN32_WCE)
static ortp_thread_t pipe_reader_th;
static bool_t pipe_reader_run=FALSE;
#endif /*_WIN32_WCE*/
#if !defined(_WIN32_WCE)
static ortp_pipe_t server_sock;
#endif /*_WIN32_WCE*/

bool_t linphonec_camera_enabled=TRUE;



void linphonec_call_identify(LinphoneCall* call){
	static long callid=1;
	linphone_call_set_user_pointer (call,(void*)callid);
	callid++;
}

LinphoneCall *linphonec_get_call(long id){
	const MSList *elem=linphone_core_get_calls(linphonec);
	for (;elem!=NULL;elem=elem->next){
		LinphoneCall *call=(LinphoneCall*)elem->data;
		if (linphone_call_get_user_pointer (call)==(void*)id){
			return call;
		}
	}
	linphonec_out("Sorry, no call with id %i exists at this time.\n",id);
	return NULL;
}

/***************************************************************************
 *
 * Linphone core callbacks
 *
 ***************************************************************************/

/*
 * Linphone core callback
 */
static void
linphonec_display_refer (LinphoneCore * lc, const char *refer_to)
{
	linphonec_out("Receiving out of call refer to %s\n", refer_to);
}

/*
 * Linphone core callback
 */
static void
linphonec_display_something (LinphoneCore * lc, const char *something)
{
	fprintf (stdout, "%s\n%s", something,prompt);
	fflush(stdout);
}

/*
 * Linphone core callback
 */
static void
linphonec_display_status (LinphoneCore * lc, const char *something)
{
	fprintf (stdout, "%s\n%s", something,prompt);
	fflush(stdout);
}

/*
 * Linphone core callback
 */
static void
linphonec_display_warning (LinphoneCore * lc, const char *something)
{
	fprintf (stdout, "Warning: %s\n%s", something,prompt);
	fflush(stdout);
}

/*
 * Linphone core callback
 */
static void
linphonec_display_url (LinphoneCore * lc, const char *something, const char *url)
{
	fprintf (stdout, "%s : %s\n", something, url);
}

/*
 * Linphone core callback
 */
static void
linphonec_prompt_for_auth(LinphoneCore *lc, const char *realm, const char *username)
{
	/* no prompt possible when using pipes or tcp mode*/
	if (unix_socket){
		linphone_core_abort_authentication(lc,NULL);
	}else{
		LinphoneAuthInfo *pending_auth;

		if ( auth_stack.nitems+1 > MAX_PENDING_AUTH )
		{
			fprintf(stderr,
				"Can't accept another authentication request.\n"
				"Consider incrementing MAX_PENDING_AUTH macro.\n");
			return;
		}

		pending_auth=linphone_auth_info_new(username,NULL,NULL,NULL,realm);
		auth_stack.elem[auth_stack.nitems++]=pending_auth;
	}
}

/*
 * Linphone core callback
 */
static void
linphonec_notify_received(LinphoneCore *lc, LinphoneCall *call, const char *from,const char *event)
{
	if(!strcmp(event,"refer"))
	{
		linphonec_out("The distand endpoint %s of call %li has been transfered, you can safely close the call.\n",
		              from,(long)linphone_call_get_user_pointer (call));
	}
}


/*
 * Linphone core callback
 */
static void
linphonec_notify_presence_received(LinphoneCore *lc,LinphoneFriend *fid)
{
	char *tmp=linphone_address_as_string(linphone_friend_get_address(fid));
	printf("Friend %s is %s\n", tmp, linphone_online_status_to_string(linphone_friend_get_status(fid)));
	ms_free(tmp);
	// todo: update Friend list state (unimplemented)
}

/*
 * Linphone core callback
 */
static void
linphonec_new_unknown_subscriber(LinphoneCore *lc, LinphoneFriend *lf,
		const char *url)
{
	printf("Friend %s requested subscription "
		"(accept/deny is not implemented yet)\n", url);
	// This means that this person wishes to be notified
	// of your presence information (online, busy, away...).

}

static void linphonec_call_updated(LinphoneCall *call){
	const LinphoneCallParams *cp=linphone_call_get_current_params(call);
	if (!linphone_call_camera_enabled (call) && linphone_call_params_video_enabled (cp)){
		linphonec_out("Far end requests to share video.\nType 'camera on' if you agree.\n");
	}
}

static void linphonec_call_encryption_changed(LinphoneCore *lc, LinphoneCall *call, bool_t encrypted, const char *auth_token) {
	long id=(long)linphone_call_get_user_pointer (call);
	if (!encrypted) {
		linphonec_out("Call %i is not fully encrypted and auth token is %s.\n", id,
				(auth_token != NULL) ? auth_token : "absent");
	} else {
		linphonec_out("Call %i is fully encrypted and auth token is %s.\n", id,
				(auth_token != NULL) ? auth_token : "absent");
	}
}

static void linphonec_call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState st, const char *msg){
	char *from=linphone_call_get_remote_address_as_string(call);
	long id=(long)linphone_call_get_user_pointer (call);
	switch(st){
		case LinphoneCallEnd:
			linphonec_out("Call %i with %s ended (%s).\n", id, from, linphone_reason_to_string(linphone_call_get_reason(call)));
		break;
		case LinphoneCallResuming:
			linphonec_out("Resuming call %i with %s.\n", id, from);
		break;
		case LinphoneCallStreamsRunning:
			linphonec_out("Media streams established with %s for call %i (%s).\n", from,id,( linphone_call_params_video_enabled( linphone_call_get_current_params(call)) ? "video":"audio"));
		break;
		case LinphoneCallPausing:
			linphonec_out("Pausing call %i with %s.\n", id, from);
		break;
		case LinphoneCallPaused:
			linphonec_out("Call %i with %s is now paused.\n", id, from);
		break;
		case LinphoneCallPausedByRemote:
			linphonec_out("Call %i has been paused by %s.\n",id,from);
		break;
		case LinphoneCallIncomingReceived:
			linphonec_call_identify(call);
			linphone_call_enable_camera (call,linphonec_camera_enabled);
			id=(long)linphone_call_get_user_pointer (call);
			linphonec_set_caller(from);
			if ( auto_answer)  {
				answer_call=TRUE;
			}
			linphonec_out("Receiving new incoming call from %s, assigned id %i\n", from,id);
		break;
		case LinphoneCallOutgoingInit:
			linphonec_call_identify(call);
			id=(long)linphone_call_get_user_pointer (call);
			linphonec_out("Establishing call id to %s, assigned id %i\n", from,id);
		break;
		case LinphoneCallUpdatedByRemote:
			linphonec_call_updated(call);
		break;
		case LinphoneCallOutgoingProgress:
			linphonec_out("Call %i to %s in progress.\n", id, from);
			break;
		case LinphoneCallOutgoingRinging:
			linphonec_out("Call %i to %s ringing.\n", id, from);
			break;
		case LinphoneCallConnected:
			linphonec_out("Call %i with %s connected.\n", id, from);
			break;
		case LinphoneCallOutgoingEarlyMedia:
			linphonec_out("Call %i with %s early media.\n", id, from);
			break;
		case LinphoneCallError:
			linphonec_out("Call %i with %s error.\n", id, from);
			break;
		default:
		break;
	}
	ms_free(from);
}

/*
 * Linphone core callback
 */
static void
linphonec_text_received(LinphoneCore *lc, LinphoneChatRoom *cr,
		const LinphoneAddress *from, const char *msg)
{
	linphonec_out("Message received from %s: %s\n", linphone_address_as_string(from), msg);
	// TODO: provide mechanism for answering.. ('say' command?)
}


static void linphonec_dtmf_received(LinphoneCore *lc, LinphoneCall *call, int dtmf){
	char *from=linphone_call_get_remote_address_as_string(call);
	fprintf(stdout,"Receiving tone %c from %s\n",dtmf,from);
	fflush(stdout);
	ms_free(from);
}

static char received_prompt[PROMPT_MAX_LEN];
static ms_mutex_t prompt_mutex;
static bool_t have_prompt=FALSE;

static void *prompt_reader_thread(void *arg){
	char *ret;
	char tmp[PROMPT_MAX_LEN];
	while ((ret=fgets(tmp,sizeof(tmp),stdin))!=NULL){
		ms_mutex_lock(&prompt_mutex);
		strcpy(received_prompt,ret);
		have_prompt=TRUE;
		ms_mutex_unlock(&prompt_mutex);
	}
	return NULL;
}

static void start_prompt_reader(void){
	ortp_thread_t th;
	ms_mutex_init(&prompt_mutex,NULL);
	ortp_thread_create(&th,NULL,prompt_reader_thread,NULL);
}
#if !defined(_WIN32_WCE)
static ortp_pipe_t create_server_socket(void){
	char path[128];
#ifndef WIN32
	snprintf(path,sizeof(path)-1,"linphonec-%i",getuid());
#else
	{
		TCHAR username[128];
		DWORD size=sizeof(username)-1;
		GetUserName(username,&size);
		snprintf(path,sizeof(path)-1,"linphonec-%s",username);
	}
#endif
	return ortp_server_pipe_create(path);
}


static void *pipe_thread(void*p){
	char tmp[250];
	server_sock=create_server_socket();
	if (server_sock==ORTP_PIPE_INVALID) return NULL;
	while(pipe_reader_run){
		while(client_sock!=ORTP_PIPE_INVALID){ /*sleep until the last command is finished*/
#ifndef WIN32
			usleep(20000);
#else
			Sleep(20);
#endif
		}
		client_sock=ortp_server_pipe_accept_client(server_sock);
		if (client_sock!=ORTP_PIPE_INVALID){
			int len;
			/*now read from the client */
			if ((len=ortp_pipe_read(client_sock,(uint8_t*)tmp,sizeof(tmp)-1))>0){
				ortp_mutex_lock(&prompt_mutex);
				tmp[len]='\0';
				strcpy(received_prompt,tmp);
				printf("Receiving command '%s'\n",received_prompt);fflush(stdout);
				have_prompt=TRUE;
				ortp_mutex_unlock(&prompt_mutex);
			}else{
				printf("read nothing\n");fflush(stdout);
				ortp_server_pipe_close_client(client_sock);
				client_sock=ORTP_PIPE_INVALID;
			}

		}else{
			if (pipe_reader_run) fprintf(stderr,"accept() failed: %s\n",strerror(errno));
		}
	}
	ms_message("Exiting pipe_reader_thread.");
	fflush(stdout);
	return NULL;
}

static void start_pipe_reader(void){
	ms_mutex_init(&prompt_mutex,NULL);
	pipe_reader_run=TRUE;
	ortp_thread_create(&pipe_reader_th,NULL,pipe_thread,NULL);
}

static void stop_pipe_reader(void){
	pipe_reader_run=FALSE;
	linphonec_command_finished();
	ortp_server_pipe_close(server_sock);
	ortp_thread_join(pipe_reader_th,NULL);
}
#endif /*_WIN32_WCE*/

#ifdef HAVE_READLINE
#define BOOL_HAVE_READLINE 1
#else
#define BOOL_HAVE_READLINE 0
#endif

char *linphonec_readline(char *prompt){
	if (unix_socket || !BOOL_HAVE_READLINE ){
		static bool_t prompt_reader_started=FALSE;
		static bool_t pipe_reader_started=FALSE;
		if (!prompt_reader_started){
			start_prompt_reader();
			prompt_reader_started=TRUE;
		}
		if (unix_socket && !pipe_reader_started){
#if !defined(_WIN32_WCE)
			start_pipe_reader();
			pipe_reader_started=TRUE;
#endif /*_WIN32_WCE*/
		}
		fprintf(stdout,"%s",prompt);
		fflush(stdout);
		while(1){
			ms_mutex_lock(&prompt_mutex);
			if (have_prompt){
				char *ret=strdup(received_prompt);
				have_prompt=FALSE;
				ms_mutex_unlock(&prompt_mutex);
				return ret;
			}
			ms_mutex_unlock(&prompt_mutex);
			linphonec_idle_call();
#ifdef WIN32
			Sleep(20);
			/* Following is to get the video window going as it
				 should. Maybe should we only have this on when the option -V
				 or -D is on? */
			MSG msg;
	
			if (PeekMessage(&msg, NULL, 0, 0,1)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
#else
			usleep(20000);
#endif
		}
	}else{
#ifdef HAVE_READLINE
		char* ret=readline(prompt);
		return ret;
#endif
	}
}

void linphonec_out(const char *fmt,...){
	char *res;
	va_list args;
	va_start (args, fmt);
	res=ortp_strdup_vprintf(fmt,args);
	va_end (args);
	printf("%s",res);
	fflush(stdout);
#if !defined(_WIN32_WCE)
	if (client_sock!=ORTP_PIPE_INVALID){
		if (ortp_pipe_write(client_sock,(uint8_t*)res,strlen(res))==-1){
			fprintf(stderr,"Fail to send output via pipe: %s",strerror(errno));
		}
	}
#endif /*_WIN32_WCE*/
	ortp_free(res);
}

void linphonec_command_finished(void){
#if !defined(_WIN32_WCE)
	if (client_sock!=ORTP_PIPE_INVALID){
		ortp_server_pipe_close_client(client_sock);
		client_sock=ORTP_PIPE_INVALID;
	}
#endif /*_WIN32_WCE*/
}

void linphonec_set_autoanswer(bool_t enabled){
	auto_answer=enabled;
}

bool_t linphonec_get_autoanswer(){
	return auto_answer;
}

LinphoneCoreVTable linphonec_vtable={0};

/***************************************************************************/
/*
 * Main
 *
 * Use globals:
 *
 *	- char *histfile_name
 *	- FILE *mylogfile
 */
#if defined (_WIN32_WCE)

char **convert_args_to_ascii(int argc, _TCHAR **wargv){
	int i;
	char **result=malloc(argc*sizeof(char*));
	char argtmp[128];
	for(i=0;i<argc;++i){
		wcstombs(argtmp,wargv[i],sizeof(argtmp));
		result[i]=strdup(argtmp);
	}
	return result;
}

int _tmain(int argc, _TCHAR* wargv[]) {
	char **argv=convert_args_to_ascii(argc,wargv);
	trace_level=6;

#else
int
main (int argc, char *argv[]) {
#endif
	linphonec_vtable.call_state_changed=linphonec_call_state_changed;
	linphonec_vtable.notify_presence_recv = linphonec_notify_presence_received;
	linphonec_vtable.new_subscription_request = linphonec_new_unknown_subscriber;
	linphonec_vtable.auth_info_requested = linphonec_prompt_for_auth;
	linphonec_vtable.display_status = linphonec_display_status;
	linphonec_vtable.display_message=linphonec_display_something;
	linphonec_vtable.display_warning=linphonec_display_warning;
	linphonec_vtable.display_url=linphonec_display_url;
	linphonec_vtable.text_received=linphonec_text_received;
	linphonec_vtable.dtmf_received=linphonec_dtmf_received;
	linphonec_vtable.refer_received=linphonec_display_refer;
	linphonec_vtable.notify_recv=linphonec_notify_received;
	linphonec_vtable.call_encryption_changed=linphonec_call_encryption_changed;
	
	if (! linphonec_init(argc, argv) ) exit(EXIT_FAILURE);

	linphonec_main_loop (linphonec);

	linphonec_finish(EXIT_SUCCESS);

	exit(EXIT_SUCCESS); /* should never reach here */
}

/*
 * Initialize linphonec
 */
static int
linphonec_init(int argc, char **argv)
{

	//g_mem_set_vtable(&dbgtable);

	/*
	 * Set initial values for global variables
	 */
	mylogfile = NULL;
	
	
#ifndef _WIN32
	snprintf(configfile_name, PATH_MAX, "%s/.linphonerc",
			getenv("HOME"));
	snprintf(zrtpsecrets, PATH_MAX, "%s/.linphone-zidcache",
			getenv("HOME"));
#elif defined(_WIN32_WCE)
	strncpy(configfile_name,PACKAGE_DIR "\\linphonerc",PATH_MAX);
	mylogfile=fopen(PACKAGE_DIR "\\" "linphonec.log","w");
	printf("Logs are redirected in" PACKAGE_DIR "\\linphonec.log");
#else
	snprintf(configfile_name, PATH_MAX, "%s/Linphone/linphonerc",
			getenv("APPDATA"));
	snprintf(zrtpsecrets, PATH_MAX, "%s/Linphone/linphone-zidcache",
			getenv("APPDATA"));
#endif
	/* Handle configuration filename changes */
	switch (handle_configfile_migration())
	{
		case -1: /* error during file copies */
			fprintf(stderr,
				"Error in configuration file migration\n");
			break;

		case 0: /* nothing done */
		case 1: /* migrated */
		default:
			break;
	}

#ifdef ENABLE_NLS
	if (NULL == bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR))
		perror ("bindtextdomain failed");
#ifndef __ARM__
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
	textdomain (GETTEXT_PACKAGE);
#else
	printf ("NLS disabled.\n");
#endif

	linphonec_parse_cmdline(argc, argv);

	if (trace_level > 0)
	{
		if (logfile_name != NULL)
			mylogfile = fopen (logfile_name, "w+");

		if (mylogfile == NULL)
		{
			mylogfile = stdout;
			fprintf (stderr,
				 "INFO: no logfile, logging to stdout\n");
		}
		linphone_core_enable_logs(mylogfile);
	}
	else
	{
		linphone_core_disable_logs();
	}
	/*
	 * Initialize auth stack
	 */
	auth_stack.nitems=0;

	/*
	 * Initialize linphone core
	 */
	linphonec=linphone_core_new (&linphonec_vtable, configfile_name, factory_configfile_name, NULL);
	linphone_core_set_zrtp_secrets_file(linphonec,zrtpsecrets);
	linphone_core_enable_video(linphonec,vcap_enabled,display_enabled);
	if (display_enabled && window_id != 0) 
	{
		printf ("Setting window_id: 0x%x\n", window_id);
		linphone_core_set_native_video_window_id(linphonec,window_id);
	}

	linphone_core_enable_video_preview(linphonec,preview_enabled);
	if (!(vcap_enabled || display_enabled)) printf("Warning: video is disabled in linphonec, use -V or -C or -D to enable.\n");
#ifdef HAVE_READLINE
	/*
	 * Initialize readline
	 */
	linphonec_initialize_readline();
#endif
#if !defined(_WIN32_WCE)
	/*
	 * Initialize signal handlers
	 */
	signal(SIGTERM, linphonec_finish);
	signal(SIGINT, linphonec_finish);
#endif /*_WIN32_WCE*/
	return 1;
}


void linphonec_main_loop_exit(void){
	linphonec_running=FALSE;
}

/*
 * Close linphonec, cleanly terminating
 * any pending call
 */
void
linphonec_finish(int exit_status)
{
	// Do not allow concurrent destroying to prevent glibc errors
	static bool_t terminating=FALSE;
	if (terminating) return; 
	terminating=TRUE;
	linphonec_out("Terminating...\n");

	/* Terminate any pending call */
	linphone_core_terminate_all_calls(linphonec);
#ifdef HAVE_READLINE
	linphonec_finish_readline();
#endif
#if !defined(_WIN32_WCE)
	if (pipe_reader_run)
		stop_pipe_reader();
#endif /*_WIN32_WCE*/

	linphone_core_destroy (linphonec);

	if (mylogfile != NULL && mylogfile != stdout)
	{
		fclose (mylogfile);
	}
	printf("\n");
	exit(exit_status);

}

/*
 * This is called from idle_call() whenever
 * pending_auth != NULL.
 *
 * It prompts user for a password.
 * Hitting ^D (EOF) would make this function
 * return 0 (Cancel).
 * Any other input would try to set linphone core
 * auth_password for the pending_auth, add the auth_info
 * and return 1.
 */
int
linphonec_prompt_for_auth_final(LinphoneCore *lc)
{
	static int reentrancy=0;
	char *input, *iptr;
	char auth_prompt[256];
#ifdef HAVE_READLINE
	rl_hook_func_t *old_event_hook;
#endif

	if (reentrancy!=0) return 0;
	
	reentrancy++;
	
	LinphoneAuthInfo *pending_auth=auth_stack.elem[auth_stack.nitems-1];

	snprintf(auth_prompt, 256, "Password for %s on %s: ",
		pending_auth->username, pending_auth->realm);

	printf("\n");
#ifdef HAVE_READLINE
	/*
	 * Disable event hook to avoid entering an
	 * infinite loop. This would prevent idle_call
	 * from being called during authentication reads.
	 * Note that it might be undesiderable...
	 */
	old_event_hook=rl_event_hook;
	rl_event_hook=NULL;
#endif

	while (1)
	{
		input=linphonec_readline(auth_prompt);

		/*
		 * If EOF (^D) is sent you probably don't want
		 * to provide an auth password... should give up
		 * the operation, but there's no mechanism to
		 * send this info back to caller currently...
		 */
		if ( ! input )
		{
			printf("Cancel requested, but not implemented.\n");
			continue;
		}

		/* Strip blanks */
		iptr=lpc_strip_blanks(input);

		/*
		 * Only blanks, continue asking
		 */
		if ( ! *iptr )
		{
			free(input);
			continue;
		}

		/* Something typed, let's try */
		break;
	}

	/*
	 * No check is done here to ensure password is correct.
	 * I guess password will be asked again later.
	 */
	linphone_auth_info_set_passwd(pending_auth, input);
	linphone_core_add_auth_info(lc, pending_auth);
	linphone_auth_info_destroy(pending_auth);
	auth_stack.elem[auth_stack.nitems-1]=0;
	--(auth_stack.nitems);
#ifdef HAVE_READLINE
	/*
	 * Reset line_buffer, to avoid the password
	 * to be used again from outer readline
	 */
	rl_line_buffer[0]='\0';
	rl_event_hook=old_event_hook;
#endif
	return 1;
}

void
print_usage (int exit_status)
{
	fprintf (stdout, "\n"
"usage: linphonec [-c file] [-s sipaddr] [-a] [-V] [-d level ] [-l logfile]\n"
       "linphonec -v\n"
"\n"
"  -b  file             specify path of readonly factory configuration file.\n"
"  -c  file             specify path of configuration file.\n"
"  -d  level            be verbose. 0 is no output. 6 is all output\n"
"  -l  logfile          specify the log file for your SIP phone\n"
"  -s  sipaddress       specify the sip call to do at startup\n"
"  -a                   enable auto answering for incoming calls\n"
"  -V                   enable video features globally (disabled by default)\n"
"  -C                   enable video capture only (disabled by default)\n"
"  -D                   enable video display only (disabled by default)\n"
"  -S                   show general state messages (disabled by default)\n"
"  --wid  windowid      force embedding of video window into provided windowid (disabled by default)\n"
"  -v or --version      display version and exits.\n");

  	exit(exit_status);
}

#ifdef VIDEO_ENABLED

#ifdef HAVE_X11_XLIB_H
static void x11_apply_video_params(VideoParams *params, Window window){
	XWindowChanges wc;
	unsigned int flags=0;
	static Display *display = NULL;
	const char *dname=getenv("DISPLAY");

	if (display==NULL && dname!=NULL){
		XInitThreads();
		display=XOpenDisplay(dname);
	}

	if (display==NULL){
		ms_error("Could not open display %s",dname);
		return;
	}
	memset(&wc,0,sizeof(wc));
	wc.x=params->x;
	wc.y=params->y;
	wc.width=params->w;
	wc.height=params->h;
	if (params->x!=-1 ){
		flags|=CWX|CWY;
	}
	if (params->w!=-1){
		flags|=CWWidth|CWHeight;
	}
	/*printf("XConfigureWindow x=%i,y=%i,w=%i,h=%i\n",
	       wc.x, wc.y ,wc.width, wc.height);*/
	XConfigureWindow(display,window,flags,&wc);
	if (params->show)
		XMapWindow(display,window);
	else
		XUnmapWindow(display,window);
	XSync(display,FALSE);
}
#endif


static void lpc_apply_video_params(){
	static unsigned long old_wid=0,old_pwid=0;
	unsigned long wid=linphone_core_get_native_video_window_id (linphonec);
	unsigned long pwid=linphone_core_get_native_preview_window_id (linphonec);

	if (wid!=0 && (lpc_video_params.refresh || old_wid!=wid)){
		lpc_video_params.refresh=FALSE;
#ifdef HAVE_X11_XLIB_H
		if (lpc_video_params.wid==0){  // do not manage window if embedded
			x11_apply_video_params(&lpc_video_params,wid);
		} else {
		        linphone_core_show_video(linphonec, lpc_video_params.show);
		}
#endif
	}
	old_wid=wid;
	if (pwid!=0 && (lpc_preview_params.refresh || old_pwid!=pwid)){
		lpc_preview_params.refresh=FALSE;
#ifdef HAVE_X11_XLIB_H
		/*printf("wid=%lu pwid=%lu\n",wid,pwid);*/
		if (lpc_preview_params.wid==0){  // do not manage window if embedded
			printf("Refreshing\n");
			x11_apply_video_params(&lpc_preview_params,pwid);
		}
#endif
	}
	old_pwid=pwid;
}

#endif


/*
 *
 * Called every second from main read loop.
 *
 * Will use the following globals:
 *
 *  - LinphoneCore linphonec
 *  - LPC_AUTH_STACK auth_stack;
 *
 */
static int
linphonec_idle_call ()
{
	LinphoneCore *opm=linphonec;

	/* Uncomment the following to verify being called */
	/* printf(".\n"); */

	linphone_core_iterate(opm);
	if (answer_call){
		fprintf (stdout, "-------auto answering to call-------\n" );
		linphone_core_accept_call(opm,NULL);
		answer_call=FALSE;
	}
	/* auto call handling */
	if (sip_addr_to_call != NULL )
	{
		char buf[512];
		snprintf (buf, sizeof(buf),"call %s", sip_addr_to_call);
		sip_addr_to_call=NULL;
		linphonec_parse_command_line(linphonec, buf);
	}

	if ( auth_stack.nitems )
	{
		/*
		 * Inhibit command completion
		 * during password prompts
		 */
#ifdef HAVE_READLINE
		rl_inhibit_completion=1;
#endif
		linphonec_prompt_for_auth_final(opm);
#ifdef HAVE_READLINE
		rl_inhibit_completion=0;
#endif
	}

#ifdef VIDEO_ENABLED
	lpc_apply_video_params();
#endif

	return 0;
}

#ifdef HAVE_READLINE
/*
 * Use globals:
 *
 *	- char *histfile_name (also sets this)
 *      - char *last_in_history (allocates it)
 */
static int
linphonec_initialize_readline()
{
	/*rl_bind_key('\t', rl_insert);*/

	/* Allow conditional parsing of ~/.inputrc */
	rl_readline_name = "linphonec";

	/* Call idle_call() every second */
	rl_set_keyboard_input_timeout(LPC_READLINE_TIMEOUT);
	rl_event_hook=linphonec_idle_call;

	/* Set history file and read it */
	histfile_name = ms_strdup_printf ("%s/.linphonec_history",
		getenv("HOME"));
	read_history(histfile_name);

	/* Initialized last_in_history cache*/
	last_in_history[0] = '\0';

	/* Register a completion function */
	rl_attempted_completion_function = linephonec_readline_completion;

	/* printf("Readline initialized.\n"); */
        setlinebuf(stdout);
	return 0;
}

/*
 * Uses globals:
 *
 *	- char *histfile_name (writes history to file and frees it)
 *	- char *last_in_history (frees it)
 *
 */
static int
linphonec_finish_readline()
{

	stifle_history(HISTSIZE);
	write_history(histfile_name);
	free(histfile_name);
	histfile_name=NULL;
	return 0;
}

#endif

static void print_prompt(LinphoneCore *opm){
#ifdef IDENTITY_AS_PROMPT
	snprintf(prompt, PROMPT_MAX_LEN, "%s> ",
		linphone_core_get_primary_contact(opm));
#else
	snprintf(prompt, PROMPT_MAX_LEN, "linphonec> ");
#endif
}

static int
linphonec_main_loop (LinphoneCore * opm)
{
	char *input;

	print_prompt(opm);


	while (linphonec_running && (input=linphonec_readline(prompt)))
	{
		char *iptr; /* input and input pointer */
		size_t input_len;

		/* Strip blanks */
		iptr=lpc_strip_blanks(input);

		input_len = strlen(iptr);

		/*
		 * Do nothing but release memory
		 * if only blanks are read
		 */
		if ( ! input_len )
		{
			free(input);
			continue;
		}

#ifdef HAVE_READLINE
		/*
		 * Only add to history if not already
		 * last item in it, and only if the command
		 * doesn't start with a space (to allow for
		 * hiding passwords)
		 */
		if ( iptr == input && strcmp(last_in_history, iptr) )
		{
			strncpy(last_in_history,iptr,sizeof(last_in_history));
			last_in_history[sizeof(last_in_history)-1]='\0';
			add_history(iptr);
		}
#endif

		linphonec_parse_command_line(linphonec, iptr);
		linphonec_command_finished();
		free(input);
	}

	return 0;
}

/*
 *  Parse command line switches
 *
 *  Use globals:
 *
 *	- int trace_level
 *	- char *logfile_name
 *	- char *configfile_name
 *	- char *sipAddr
 */
static int
linphonec_parse_cmdline(int argc, char **argv)
{
	int arg_num=1;

	while (arg_num < argc)
	{
		int old_arg_num = arg_num;
		if (strncmp ("-d", argv[arg_num], 2) == 0)
		{
			arg_num++;
			if (arg_num < argc)
				trace_level = atoi (argv[arg_num]);
			else
				trace_level = 1;
		}
		else if (strncmp ("-l", argv[arg_num], 2) == 0)
		{
			arg_num++;
			if (arg_num < argc)
				logfile_name = argv[arg_num];
		}
		else if (strncmp ("-c", argv[arg_num], 2) == 0)
		{
			if ( ++arg_num >= argc ) print_usage(EXIT_FAILURE);
#if !defined(_WIN32_WCE)
			if (access(argv[arg_num],F_OK)!=0 )
			{
				fprintf (stderr,
					"Cannot open config file %s.\n",
					 argv[arg_num]);
				exit(EXIT_FAILURE);
			}
#endif /*_WIN32_WCE*/
			snprintf(configfile_name, PATH_MAX, "%s", argv[arg_num]);
		}
		else if (strncmp ("-b", argv[arg_num], 2) == 0)
		{
			if ( ++arg_num >= argc ) print_usage(EXIT_FAILURE);
#if !defined(_WIN32_WCE)
			if (access(argv[arg_num],F_OK)!=0 )
			{
				fprintf (stderr,
					"Cannot open config file %s.\n",
					 argv[arg_num]);
				exit(EXIT_FAILURE);
			}
#endif /*_WIN32_WCE*/
			factory_configfile_name = argv[arg_num];
		}
		else if (strncmp ("-s", argv[arg_num], 2) == 0)
		{
			arg_num++;
			if (arg_num < argc)
				sip_addr_to_call = argv[arg_num];
		}
                else if (strncmp ("-a", argv[arg_num], 2) == 0)
                {
                        auto_answer = TRUE;
                }
		else if (strncmp ("-C", argv[arg_num], 2) == 0)
                {
                        vcap_enabled = TRUE;
                }
		else if (strncmp ("-D", argv[arg_num], 2) == 0)
                {
                        display_enabled = TRUE;
                }
		else if (strncmp ("-V", argv[arg_num], 2) == 0)
                {
                        display_enabled = TRUE;
			vcap_enabled = TRUE;
			preview_enabled=TRUE;
                }
		else if ((strncmp ("-v", argv[arg_num], 2) == 0)
			 ||
			 (strncmp
			  ("--version", argv[arg_num],
			   strlen ("--version")) == 0))
		{
#if !defined(_WIN32_WCE)
			printf ("version: " LINPHONE_VERSION "\n");
#endif
			exit (EXIT_SUCCESS);
		}
		else if (strncmp ("-S", argv[arg_num], 2) == 0)
		{
			show_general_state = TRUE;
		}
		else if (strncmp ("--pipe", argv[arg_num], 6) == 0)
		{
			unix_socket=1;
		}
		else if (strncmp ("--wid", argv[arg_num], 5) == 0)
		{
			arg_num++;
			if (arg_num < argc) {
				char *tmp;
				window_id = strtol( argv[arg_num], &tmp, 0 );
				lpc_video_params.wid = window_id;
			}
		}
		else if (old_arg_num == arg_num)
		{
			fprintf (stderr, "ERROR: bad arguments\n");
			print_usage (EXIT_FAILURE);
		}
		arg_num++;
	}

	return 1;
}

/*
 * Up to version 1.2.1 linphone used ~/.linphonec for
 * CLI and ~/.gnome2/linphone for GUI as configuration file.
 * In newer version both interfaces will use ~/.linphonerc.
 *
 * This function helps transparently migrating from one
 * to the other layout using the following heuristic:
 *
 *	IF new_config EXISTS => do nothing
 *	ELSE IF old_cli_config EXISTS => copy to new_config
 *	ELSE IF old_gui_config EXISTS => copy to new_config
 *
 * Returns:
 *	 0 if it did nothing
 *	 1 if it migrated successfully
 *	-1 on error
 */
static int
handle_configfile_migration()
{
#if !defined(_WIN32_WCE)
	char *old_cfg_gui;
	char *old_cfg_cli;
	char *new_cfg;
#if !defined(_WIN32_WCE)
	const char *home = getenv("HOME");
#else
	const char *home = ".";
#endif /*_WIN32_WCE*/
	new_cfg = ms_strdup_printf("%s/.linphonerc", home);

	/*
	 * If the *NEW* configuration already exists
	 * do nothing.
	 */
	if (access(new_cfg,F_OK)==0)
	{
		free(new_cfg);
		return 0;
	}

	old_cfg_cli = ms_strdup_printf("%s/.linphonec", home);

	/*
	 * If the *OLD* CLI configurations exist copy it to
	 * the new file and make it a symlink.
	 */
	if (access(old_cfg_cli, F_OK)==0)
	{
		if ( ! copy_file(old_cfg_cli, new_cfg) )
		{
			free(old_cfg_cli);
			free(new_cfg);
			return -1;
		}
		printf("%s copied to %s\n", old_cfg_cli, new_cfg);
		free(old_cfg_cli);
		free(new_cfg);
		return 1;
	}

	free(old_cfg_cli);
	old_cfg_gui = ms_strdup_printf("%s/.gnome2/linphone", home);

	/*
	 * If the *OLD* GUI configurations exist copy it to
	 * the new file and make it a symlink.
	 */
	if (access(old_cfg_gui, F_OK)==0)
	{
		if ( ! copy_file(old_cfg_gui, new_cfg) )
		{
			exit(EXIT_FAILURE);
			free(old_cfg_gui);
			free(new_cfg);
			return -1;
		}
		printf("%s copied to %s\n", old_cfg_gui, new_cfg);
		free(old_cfg_gui);
		free(new_cfg);
		return 1;
	}

	free(old_cfg_gui);
	free(new_cfg);
#endif /*_WIN32_WCE*/
	return 0;
}
#if !defined(_WIN32_WCE)
/*
 * Copy file "from" to file "to".
 * Destination file is truncated if existing.
 * Return 1 on success, 0 on error (printing an error).
 */
static int
copy_file(const char *from, const char *to)
{
	char message[256];
	FILE *in, *out;
	char buf[256];
	size_t n;

	/* Open "from" file for reading */
	in=fopen(from, "r");
	if ( in == NULL )
	{
		snprintf(message, 255, "Can't open %s for reading: %s\n",
			from, strerror(errno));
		fprintf(stderr, "%s", message);
		return 0;
	}

	/* Open "to" file for writing (will truncate existing files) */
	out=fopen(to, "w");
	if ( out == NULL )
	{
		snprintf(message, 255, "Can't open %s for writing: %s\n",
			to, strerror(errno));
		fprintf(stderr, "%s", message);
		fclose(in);
		return 0;
	}

	/* Copy data from "in" to "out" */
	while ( (n=fread(buf, 1, sizeof buf, in)) > 0 )
	{
		if ( ! fwrite(buf, 1, n, out) )
		{
			fclose(in);
			fclose(out);
			return 0;
		}
	}

	fclose(in);
	fclose(out);

	return 1;
}
#endif /*_WIN32_WCE*/

#ifdef HAVE_READLINE
static char **
linephonec_readline_completion(const char *text, int start, int end)
{
	char **matches = NULL;

	/*
	 * Prevent readline from falling
	 * back to filename-completion
	 */
	rl_attempted_completion_over=1;

	/*
	 * If this is the start of line we complete with commands
	 */
	if ( ! start )
	{
		return rl_completion_matches(text, linphonec_command_generator);
	}

	/*
	 * Otherwise, we should peek at command name
	 * or context to implement a smart completion.
	 * For example: "call .." could return
	 * friends' sip-uri as matches
	 */

	return matches;
}

#endif

/*
 * Strip blanks from a string.
 * Return a pointer into the provided string.
 * Modifies input adding a NULL at first
 * of trailing blanks.
 */
char *
lpc_strip_blanks(char *input)
{
	char *iptr;

	/* Find first non-blank */
	while(*input && isspace(*input)) ++input;

	/* Find last non-blank */
	iptr=input+strlen(input);
	if (iptr > input) {
		while(isspace(*--iptr));
		*(iptr+1)='\0';
	}

	return input;
}

/****************************************************************************
 *
 * $Log: linphonec.c,v $
 * Revision 1.57  2007/11/14 13:40:27  smorlat
 * fix --disable-video build.
 *
 * Revision 1.56  2007/09/26 14:07:27  fixkowalski
 * - ANSI/C++ compilation issues with non-GCC compilers
 * - Faster epm-based packaging
 * - Ability to build & run on FC6's eXosip/osip
 *
 * Revision 1.55  2007/09/24 16:01:58  smorlat
 * fix bugs.
 *
 * Revision 1.54  2007/08/22 14:06:11  smorlat
 * authentication bugs fixed.
 *
 * Revision 1.53  2007/02/13 21:31:01  smorlat
 * added patch for general state.
 * new doxygen for oRTP
 * gtk-doc removed.
 *
 * Revision 1.52  2007/01/10 14:11:24  smorlat
 * add --video to linphonec.
 *
 * Revision 1.51  2006/08/21 12:49:59  smorlat
 * merged several little patches.
 *
 * Revision 1.50  2006/07/26 08:17:28  smorlat
 * fix bugs.
 *
 * Revision 1.49  2006/07/17 18:45:00  smorlat
 * support for several event queues in ortp.
 * glib dependency removed from coreapi/ and console/
 *
 * Revision 1.48  2006/04/09 12:45:32  smorlat
 * linphonec improvements.
 *
 * Revision 1.47  2006/04/04 08:04:34  smorlat
 * switched to mediastreamer2, most bugs fixed.
 *
 * Revision 1.46  2006/03/16 17:17:40  smorlat
 * fix various bugs.
 *
 * Revision 1.45  2006/03/12 21:48:31  smorlat
 * gcc-2.95 compile error fixed.
 * mediastreamer2 in progress
 *
 * Revision 1.44  2006/03/04 11:17:10  smorlat
 * mediastreamer2 in progress.
 *
 * Revision 1.43  2006/02/13 09:50:50  strk
 * fixed unused variable warning.
 *
 * Revision 1.42  2006/02/02 15:39:18  strk
 * - Added 'friend list' and 'friend call' commands
 * - Allowed for multiple DTFM send in a single line
 * - Added status-specific callback (bare version)
 *
 * Revision 1.41  2006/02/02 13:30:05  strk
 * - Padded vtable with missing callbacks
 *   (fixing a segfault on friends subscription)
 * - Handled friends notify (bare version)
 * - Handled text messages receive (bare version)
 * - Printed message on subscription request (bare version)
 *
 * Revision 1.40  2006/01/26 09:48:05  strk
 * Added limits.h include
 *
 * Revision 1.39  2006/01/26 02:11:01  strk
 * Removed unused variables, fixed copyright date
 *
 * Revision 1.38  2006/01/25 18:33:02  strk
 * Removed the -t swich, terminate_on_close made the default behaviour
 *
 * Revision 1.37  2006/01/20 14:12:34  strk
 * Added linphonec_init() and linphonec_finish() functions.
 * Handled SIGINT and SIGTERM to invoke linphonec_finish().
 * Handling of auto-termination (-t) moved to linphonec_finish().
 * Reworked main (input read) loop to not rely on 'terminate'
 * and 'run' variable (dropped). configfile_name allocated on stack
 * using PATH_MAX limit. Changed print_usage signature to allow
 * for an exit_status specification.
 *
 * Revision 1.36  2006/01/18 09:25:32  strk
 * Command completion inhibited in proxy addition and auth request prompts.
 * Avoided use of readline's internal filename completion.
 *
 * Revision 1.35  2006/01/14 13:29:32  strk
 * Reworked commands interface to use a table structure,
 * used by command line parser and help function.
 * Implemented first level of completion (commands).
 * Added notification of invalid "answer" and "terminate"
 * commands (no incoming call, no active call).
 * Forbidden "call" intialization when a call is already active.
 * Cleaned up all commands, adding more feedback and error checks.
 *
 * Revision 1.34  2006/01/13 13:00:29  strk
 * Added linphonec.h. Code layout change (added comments, forward decl,
 * globals on top, copyright notices and Logs). Handled out-of-memory
 * condition on history management. Removed assumption on sizeof(char).
 * Fixed bug in authentication prompt (introduced by readline).
 * Added support for multiple authentication requests (up to MAX_PENDING_AUTH).
 *
 *
 ****************************************************************************/

