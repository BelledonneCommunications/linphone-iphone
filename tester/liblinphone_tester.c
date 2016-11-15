/*
 liblinphone_tester - liblinphone test suite
 Copyright (C) 2013  Belledonne Communications SARL

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "linphone/core.h"
#include "private.h"
#include "liblinphone_tester.h"

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic push
#endif
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#else
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#endif

#ifdef HAVE_GTK
#include <gtk/gtk.h>
#endif

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif


static FILE * log_file = NULL;

#ifdef ANDROID

#include <android/log.h>
#include <jni.h>
#define CALLBACK_BUFFER_SIZE  1024

static JNIEnv *current_env = NULL;
static jobject current_obj = 0;
static const char* LogDomain = "liblinphone_tester";

int main(int argc, char** argv);

void liblinphone_android_log_handler(int prio, const char *fmt, va_list args) {
	char str[4096];
	char *current;
	char *next;

	vsnprintf(str, sizeof(str) - 1, fmt, args);
	str[sizeof(str) - 1] = '\0';
	if (strlen(str) < 512) {
		__android_log_write(prio, LogDomain, str);
	} else {
		current = str;
		while ((next = strchr(current, '\n')) != NULL) {
			*next = '\0';
			__android_log_write(prio, LogDomain, current);
			current = next + 1;
		}
		__android_log_write(prio, LogDomain, current);
	}
}

static void liblinphone_android_ortp_log_handler(const char *domain, OrtpLogLevel lev, const char *fmt, va_list args) {
	int prio;
	switch(lev){
		case ORTP_DEBUG:	prio = ANDROID_LOG_DEBUG;	break;
		case ORTP_MESSAGE:	prio = ANDROID_LOG_INFO;	break;
		case ORTP_WARNING:	prio = ANDROID_LOG_WARN;	break;
		case ORTP_ERROR:	prio = ANDROID_LOG_ERROR;	break;
		case ORTP_FATAL:	prio = ANDROID_LOG_FATAL;	break;
		default:			prio = ANDROID_LOG_DEFAULT;	break;
	}
	liblinphone_android_log_handler(prio, fmt, args);
}

static void liblinphone_android_bctbx_log_handler(const char *domain, BctbxLogLevel lev, const char *fmt, va_list args) {
	int prio;
	switch(lev){
		case BCTBX_LOG_DEBUG:	prio = ANDROID_LOG_DEBUG;	break;
		case BCTBX_LOG_MESSAGE:	prio = ANDROID_LOG_INFO;	break;
		case BCTBX_LOG_WARNING:	prio = ANDROID_LOG_WARN;	break;
		case BCTBX_LOG_ERROR:	prio = ANDROID_LOG_ERROR;	break;
		case BCTBX_LOG_FATAL:	prio = ANDROID_LOG_FATAL;	break;
		default:			prio = ANDROID_LOG_DEFAULT;	break;
	}
	liblinphone_android_log_handler(prio, fmt, args);
}

void bcunit_android_trace_handler(int level, const char *fmt, va_list args) {
	char buffer[CALLBACK_BUFFER_SIZE];
	jstring javaString;
	jclass cls;
	jmethodID method;
	jint javaLevel = level;
	JNIEnv *env = current_env;
	if(env == NULL) return;
	vsnprintf(buffer, CALLBACK_BUFFER_SIZE, fmt, args);
	javaString = (*env)->NewStringUTF(env, buffer);
	cls = (*env)->GetObjectClass(env, current_obj);
	method = (*env)->GetMethodID(env, cls, "printLog", "(ILjava/lang/String;)V");
	(*env)->CallVoidMethod(env, current_obj, method, javaLevel, javaString);
	(*env)->DeleteLocalRef(env,javaString);
	(*env)->DeleteLocalRef(env,cls);
}

JNIEXPORT jint JNICALL Java_org_linphone_tester_Tester_run(JNIEnv *env, jobject obj, jobjectArray stringArray) {
	int i, ret;
	int argc = (*env)->GetArrayLength(env, stringArray);
	char **argv = (char**) malloc(sizeof(char*) * argc);

	for (i=0; i<argc; i++) {
		jstring string = (jstring) (*env)->GetObjectArrayElement(env, stringArray, i);
		const char *rawString = (const char *) (*env)->GetStringUTFChars(env, string, 0);
		argv[i] = strdup(rawString);
		(*env)->ReleaseStringUTFChars(env, string, rawString);
	}
	current_env = env;
	current_obj = obj;
	bc_set_trace_handler(bcunit_android_trace_handler);
	ret = main(argc, argv);
	current_env = NULL;
	bc_set_trace_handler(NULL);
	for (i=0; i<argc; i++) {
		free(argv[i]);
	}
	free(argv);
	return ret;
}

JNIEXPORT void JNICALL Java_org_linphone_tester_Tester_keepAccounts(JNIEnv *env, jclass c, jboolean keep) {
	liblinphone_tester_keep_accounts((int)keep);
}

JNIEXPORT void JNICALL Java_org_linphone_tester_Tester_clearAccounts(JNIEnv *env, jclass c) {
	liblinphone_tester_clear_accounts();
}
#endif /* ANDROID */

static void log_handler(int lev, const char *fmt, va_list args) {
#ifdef _WIN32
	vfprintf(lev == ORTP_ERROR ? stderr : stdout, fmt, args);
	fprintf(lev == ORTP_ERROR ? stderr : stdout, "\n");
#else
	va_list cap;
	va_copy(cap,args);
#ifdef ANDROID
	/* IMPORTANT: needed by liblinphone tester to retrieve suite list...*/
	bcunit_android_trace_handler(lev == ORTP_ERROR, fmt, cap);
#else
	/* Otherwise, we must use stdio to avoid log formatting (for autocompletion etc.) */
	vfprintf(lev == ORTP_ERROR ? stderr : stdout, fmt, cap);
	fprintf(lev == ORTP_ERROR ? stderr : stdout, "\n");
#endif
	va_end(cap);
#endif
	if (log_file){
		ortp_logv_out(ORTP_LOG_DOMAIN, lev, fmt, args);
	}
}

void liblinphone_tester_init(void(*ftester_printf)(int level, const char *fmt, va_list args)) {
	if (! log_file) {
#if defined(ANDROID)
		linphone_core_set_log_handler(liblinphone_android_ortp_log_handler);
		bctbx_set_log_handler(liblinphone_android_bctbx_log_handler);
#endif
	}

	if (ftester_printf == NULL) ftester_printf = log_handler;
	bc_tester_init(ftester_printf, ORTP_MESSAGE, ORTP_ERROR, "rcfiles");
	liblinphone_tester_add_suites();
}

int liblinphone_tester_set_log_file(const char *filename) {
	if (log_file) {
		fclose(log_file);
	}
	log_file = fopen(filename, "w");
	if (!log_file) {
		ms_error("Cannot open file [%s] for writing logs because [%s]", filename, strerror(errno));
		return -1;
	}
	ms_message("Redirecting traces to file [%s]", filename);
	bctbx_set_log_file(log_file);
	ortp_set_log_file(log_file);
	return 0;
}


#if !TARGET_OS_IPHONE && !(defined(LINPHONE_WINDOWS_PHONE) || defined(LINPHONE_WINDOWS_UNIVERSAL))

static const char* liblinphone_helper =
		"\t\t\t--verbose\n"
		"\t\t\t--silent\n"
		"\t\t\t--log-file <output log file path>\n"
		"\t\t\t--domain <test sip domain>\n"
		"\t\t\t--auth-domain <test auth domain>\n"
		"\t\t\t--dns-hosts </etc/hosts -like file to used to override DNS names (default: tester_hosts)>\n"
		"\t\t\t--keep-recorded-files\n"
		"\t\t\t--disable-leak-detector\n"
		"\t\t\t--disable-tls-support\n"
		"\t\t\t--no-ipv6 (turn off IPv6 in LinphoneCore, tests requiring IPv6 will be skipped)\n"
		"\t\t\t--show-account-manager-logs (show temporary test account creation logs)\n"
		;

int main (int argc, char *argv[])
{
	int i;
	int ret;

#ifdef HAVE_GTK
	gtk_init(&argc, &argv);
#if !GLIB_CHECK_VERSION(2,32,0) // backward compatibility with Debian 6 and CentOS 6
	g_thread_init(NULL);
#endif
	gdk_threads_init();
#endif

	liblinphone_tester_init(NULL);
	linphone_core_set_log_level(ORTP_ERROR);

	for(i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--verbose") == 0) {
			linphone_core_set_log_level(ORTP_MESSAGE);
		} else if (strcmp(argv[i], "--silent") == 0) {
			linphone_core_set_log_level(ORTP_FATAL);
		} else if (strcmp(argv[i],"--log-file")==0){
			CHECK_ARG("--log-file", ++i, argc);
			if (liblinphone_tester_set_log_file(argv[i]) < 0) return -2;
		} else if (strcmp(argv[i],"--domain")==0){
			CHECK_ARG("--domain", ++i, argc);
			test_domain=argv[i];
		} else if (strcmp(argv[i],"--auth-domain")==0){
			CHECK_ARG("--auth-domain", ++i, argc);
			auth_domain=argv[i];
		}else if (strcmp(argv[i],"--dns-hosts")==0){
			CHECK_ARG("--dns-hosts", ++i, argc);
			userhostsfile=argv[i];
		} else if (strcmp(argv[i],"--keep-recorded-files")==0){
			liblinphone_tester_keep_recorded_files(TRUE);
		} else if (strcmp(argv[i],"--disable-leak-detector")==0){
			liblinphone_tester_disable_leak_detector(TRUE);
		} else if (strcmp(argv[i],"--disable-tls-support")==0){
			liblinphone_tester_tls_support_disabled = TRUE;
		} else if (strcmp(argv[i],"--no-ipv6")==0){
			liblinphonetester_ipv6 = FALSE;
		} else if (strcmp(argv[i],"--show-account-manager-logs")==0){
			liblinphonetester_show_account_manager_logs=TRUE;
		} else {
			int bret = bc_tester_parse_args(argc, argv, i);
			if (bret>0) {
				i += bret - 1;
				continue;
			} else if (bret<0) {
				bc_tester_helper(argv[0], liblinphone_helper);
			}
			return bret;
		}
	}

	ret = bc_tester_start(argv[0]);
	liblinphone_tester_uninit();
	return ret;
}


#endif
