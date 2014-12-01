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

#include <stdio.h>
#include "CUnit/Basic.h"
#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"
#if HAVE_CU_CURSES
#include "CUnit/CUCurses.h"
#endif

extern int liblinphone_tester_use_log_file;

#ifdef ANDROID

#include <android/log.h>
#include <jni.h>
#include <CUnit/Util.h>
#define CALLBACK_BUFFER_SIZE  1024

static JNIEnv *current_env = NULL;
static jobject current_obj = 0;
static const char* LogDomain = "liblinphone_tester";

void linphone_android_log_handler(int prio, const char *fmt, va_list args) {
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

static void linphone_android_ortp_log_handler(OrtpLogLevel lev, const char *fmt, va_list args) {
	int prio;
	switch(lev){
        case ORTP_DEBUG:	prio = ANDROID_LOG_DEBUG;	break;
        case ORTP_MESSAGE:	prio = ANDROID_LOG_INFO;	break;
        case ORTP_WARNING:	prio = ANDROID_LOG_WARN;	break;
        case ORTP_ERROR:	prio = ANDROID_LOG_ERROR;	break;
        case ORTP_FATAL:	prio = ANDROID_LOG_FATAL;	break;
        default:			prio = ANDROID_LOG_DEFAULT;	break;
	}
	linphone_android_log_handler(prio, fmt, args);
}

void cunit_android_trace_handler(int level, const char *fmt, va_list args) {
	char buffer[CALLBACK_BUFFER_SIZE];
	JNIEnv *env = current_env;
	if(env == NULL) return;
	vsnprintf(buffer, CALLBACK_BUFFER_SIZE, fmt, args);
	jstring javaString = (*env)->NewStringUTF(env, buffer);
	jint javaLevel = level;
	jclass cls = (*env)->GetObjectClass(env, current_obj);
	jmethodID method = (*env)->GetMethodID(env, cls, "printLog", "(ILjava/lang/String;)V");
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
	CU_set_trace_handler(cunit_android_trace_handler);
	ret = main(argc, argv);
	current_env = NULL;
	CU_set_trace_handler(NULL);
	for (i=0; i<argc; i++) {
		free(argv[i]);
	}
	free(argv);
	return ret;
}

#endif /* ANDROID */

#ifdef __QNX__
static void liblinphone_tester_qnx_log_handler(OrtpLogLevel lev, const char *fmt, va_list args) {
	ortp_qnx_log_handler("liblinphone_tester", lev, fmt, args);
}
#endif /* __QNX__ */



void helper(const char *name) {
	liblinphone_tester_fprintf(stderr,"%s --help\n"
			"\t\t\t--verbose\n"
			"\t\t\t--silent\n"
			"\t\t\t--list-suites\n"
			"\t\t\t--list-tests <suite>\n"
			"\t\t\t--config <config path>\n"
			"\t\t\t--domain <test sip domain>\n"
			"\t\t\t--auth-domain <test auth domain>\n"
			"\t\t\t--suite <suite name>\n"
			"\t\t\t--test <test name>\n"
			"\t\t\t--dns-hosts </etc/hosts -like file to used to override DNS names (default: tester_hosts)>\n"
			"\t\t\t--log-file <output log file path>\n"
#if HAVE_CU_CURSES
			"\t\t\t--curses\n"
#endif
			, name);
}

#define CHECK_ARG(argument, index, argc)                      \
if(index >= argc) {                                           \
fprintf(stderr, "Missing argument for \"%s\"\n", argument);   \
return -1;                                                    \
}                                                             \

#ifndef WINAPI_FAMILY_PHONE_APP


int main (int argc, char *argv[])
{
	int i;
	int ret;
	const char *suite_name=NULL;
	const char *test_name=NULL;
	FILE* log_file=NULL;
#if defined(ANDROID)
	linphone_core_set_log_handler(linphone_android_ortp_log_handler);
#elif defined(__QNX__)
	linphone_core_set_log_handler(liblinphone_tester_qnx_log_handler);
#else
	linphone_core_set_log_file(NULL);
#endif

	liblinphone_tester_init();

	for(i=1;i<argc;++i){
		if (strcmp(argv[i],"--help")==0){
			helper(argv[0]);
			return 0;
		} else if (strcmp(argv[i],"--verbose")==0){
			linphone_core_set_log_level(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
		} else if (strcmp(argv[i],"--silent")==0){
			linphone_core_set_log_level(ORTP_FATAL);
		} else if (strcmp(argv[i],"--domain")==0){
			CHECK_ARG("--domain", ++i, argc);
			test_domain=argv[i];
		} else if (strcmp(argv[i],"--auth-domain")==0){
			CHECK_ARG("--auth-domain", ++i, argc);
			auth_domain=argv[i];
		} else if (strcmp(argv[i],"--test")==0){
			CHECK_ARG("--test", ++i, argc);
			test_name=argv[i];
		} else if (strcmp(argv[i],"--config")==0){
			CHECK_ARG("--config", ++i, argc);
			liblinphone_tester_file_prefix=argv[i];
		}else if (strcmp(argv[i],"--dns-hosts")==0){
			CHECK_ARG("--dns-hosts", ++i, argc);
			userhostsfile=argv[i];
		}else if (strcmp(argv[i],"--suite")==0){
			CHECK_ARG("--suite", ++i, argc);
			suite_name=argv[i];
		} else if (strcmp(argv[i],"--list-suites")==0){
			liblinphone_tester_list_suites();
			return 0;
		} else if (strcmp(argv[i],"--list-tests")==0){
			CHECK_ARG("--list-tests", ++i, argc);
			suite_name = argv[i];
			liblinphone_tester_list_suite_tests(suite_name);
			return 0;
		} else if (strcmp(argv[i],"--log-file")==0){
			CHECK_ARG("--log-file", ++i, argc);
			log_file=fopen(argv[i],"w");
			if (!log_file) {
				ms_fatal("Cannot open file [%s] for writting logs because [%s]",argv[i],strerror(errno));
			} else {
				liblinphone_tester_use_log_file=1;
				liblinphone_tester_fprintf(stdout,"Redirecting traces to file [%s]",argv[i]);
				linphone_core_set_log_file(log_file);
			}

		}else {
			liblinphone_tester_fprintf(stderr, "Unknown option \"%s\"\n", argv[i]); \
			helper(argv[0]);
			return -1;
		}
	}

	ret = liblinphone_tester_run_tests(suite_name, test_name);
	liblinphone_tester_uninit();
	return ret;
}
#endif /* WINAPI_FAMILY_PHONE_APP */

