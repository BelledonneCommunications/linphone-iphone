/*
	belle-sip - SIP (RFC3261) library.
    Copyright (C) 2010  Belledonne Communications SARL

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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


static test_suite_t **test_suite = NULL;
static int nb_test_suites = 0;


#if HAVE_CU_CURSES
static unsigned char curses = 0;
#endif


static  stats global_stat;
const char* test_domain="sipopen.example.org";
const char* auth_domain="sip.example.org";
const char* test_username="liblinphone_tester";
const char* test_password="secret";
const char* test_route="sip2.linphone.org";

#if WINAPI_FAMILY_PHONE_APP
const char *liblinphone_tester_file_prefix="Assets";
#else
const char *liblinphone_tester_file_prefix="./tester";
#endif

LinphoneAddress * create_linphone_address(const char * domain) {
	LinphoneAddress *addr = linphone_address_new(NULL);
	CU_ASSERT_PTR_NOT_NULL_FATAL(addr);
	linphone_address_set_username(addr,test_username);
	CU_ASSERT_STRING_EQUAL(test_username,linphone_address_get_username(addr));
	if (!domain) domain= test_route;
	linphone_address_set_domain(addr,domain);
	CU_ASSERT_STRING_EQUAL(domain,linphone_address_get_domain(addr));
	linphone_address_set_display_name(addr, NULL);
	linphone_address_set_display_name(addr, "Mr Tester");
	CU_ASSERT_STRING_EQUAL("Mr Tester",linphone_address_get_display_name(addr));
	return addr;
}

void auth_info_requested(LinphoneCore *lc, const char *realm, const char *username) {
	stats* counters;
	LinphoneAuthInfo *info;
	ms_message("Auth info requested  for user id [%s] at realm [%s]\n"
					,username
					,realm);
	counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_auth_info_requested++;
	info=linphone_auth_info_new(test_username,NULL,test_password,NULL,auth_domain); /*create authentication structure from identity*/
	linphone_core_add_auth_info(lc,info); /*add authentication info to LinphoneCore*/

}

LinphoneCore* create_lc_with_auth(unsigned int with_auth) {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	memset (&v_table,0,sizeof(v_table));
	v_table.registration_state_changed=registration_state_changed;
	if (with_auth) {
		v_table.auth_info_requested=auth_info_requested;
	}
	lc = linphone_core_new(&v_table,NULL,NULL,NULL);
	linphone_core_set_user_data(lc,&global_stat);
	return lc;
}

void reset_counters( stats* counters) {
	memset(counters,0,sizeof(stats));
}

LinphoneCore* configure_lc_from(LinphoneCoreVTable* v_table, const char* path, const char* file, int proxy_count) {
	LinphoneCore* lc;
	int retry=0;
	stats* counters;
	char filepath[256];
	char ringpath[256];
	char ringbackpath[256];
	sprintf(filepath, "%s/%s", path, file);
	lc =  linphone_core_new(v_table,NULL,filepath,NULL);
	linphone_core_set_user_data(lc,&global_stat);
	counters = (stats*)linphone_core_get_user_data(lc);
	
	sprintf(ringpath, "%s/%s", path, "oldphone.wav");
	sprintf(ringbackpath, "%s/%s", path, "ringback.wav");
	linphone_core_set_ring(lc, ringpath);
	linphone_core_set_ringback(lc, ringbackpath);

	reset_counters(counters);
	CU_ASSERT_EQUAL(ms_list_size(linphone_core_get_proxy_config_list(lc)),proxy_count);

	while (counters->number_of_LinphoneRegistrationOk<proxy_count && retry++ <20) {
			linphone_core_iterate(lc);
			ms_usleep(100000);
	}
	CU_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationOk,proxy_count);
	return lc;
}

bool_t wait_for(LinphoneCore* lc_1, LinphoneCore* lc_2,int* counter,int value) {
	MSList* lcs=NULL;
	bool_t result;
	if (lc_1)
		lcs=ms_list_append(lcs,lc_1);
	if (lc_2)
		lcs=ms_list_append(lcs,lc_2);
	result=wait_for_list(lcs,counter,value,2000);
	ms_list_free(lcs);
	return result;
}

bool_t wait_for_list(MSList* lcs,int* counter,int value,int timeout_ms) {
	int retry=0;
	MSList* iterator;
	while (*counter<value && retry++ <timeout_ms/100) {
		 for (iterator=lcs;iterator!=NULL;iterator=iterator->next) {
			 linphone_core_iterate((LinphoneCore*)(iterator->data));
		 }
		ms_usleep(100000);
	}
	if(*counter<value) return FALSE;
	else return TRUE;
}

static void enable_codec(LinphoneCore* lc,const char* type,int rate) {
	MSList* codecs=ms_list_copy(linphone_core_get_audio_codecs(lc));
	MSList* codecs_it;
	PayloadType* pt;
	for (codecs_it=codecs;codecs_it!=NULL;codecs_it=codecs_it->next) {
			linphone_core_enable_payload_type(lc,(PayloadType*)codecs_it->data,0);
	}
	if((pt = linphone_core_find_payload_type(lc,type,rate,1))) {
		linphone_core_enable_payload_type(lc,pt, 1);
	}
}

LinphoneCoreManager* linphone_core_manager_new(const char* path, const char* rc_file) {
	LinphoneCoreManager* mgr= malloc(sizeof(LinphoneCoreManager));
	LinphoneProxyConfig* proxy;
	memset (mgr,0,sizeof(LinphoneCoreManager));
	mgr->v_table.registration_state_changed=registration_state_changed;
	mgr->v_table.call_state_changed=call_state_changed;
	mgr->v_table.text_received=text_message_received;
	mgr->v_table.message_received=message_received;
	mgr->v_table.new_subscription_request=new_subscribtion_request;
	mgr->v_table.notify_presence_recv=notify_presence_received;
	mgr->v_table.transfer_state_changed=linphone_transfer_state_changed;
	mgr->lc=configure_lc_from(&mgr->v_table, path, rc_file, rc_file?1:0);
	enable_codec(mgr->lc,"PCMU",8000);
	linphone_core_set_user_data(mgr->lc,&mgr->stat);
	linphone_core_get_default_proxy(mgr->lc,&proxy);
	if (proxy) {
		mgr->identity = linphone_address_new(linphone_proxy_config_get_identity(proxy));
		linphone_address_clean(mgr->identity);
	}
	return mgr;
}

void linphone_core_manager_destroy(LinphoneCoreManager* mgr) {
	linphone_core_destroy(mgr->lc);
	if (mgr->identity) linphone_address_destroy(mgr->identity);
	free(mgr);
}


static void add_test_suite(test_suite_t *suite) {
	if (test_suite == NULL) {
		test_suite = (test_suite_t **)malloc(10 * sizeof(test_suite_t *));
	}
	test_suite[nb_test_suites] = suite;
	nb_test_suites++;
	if ((nb_test_suites % 10) == 0) {
		test_suite = (test_suite_t **)realloc(test_suite, (nb_test_suites + 10) * sizeof(test_suite_t *));
	}
}

static int run_test_suite(test_suite_t *suite) {
	int i;

	CU_pSuite pSuite = CU_add_suite(suite->name, suite->init_func, suite->cleanup_func);

	for (i = 0; i < suite->nb_tests; i++) {
		if (NULL == CU_add_test(pSuite, suite->tests[i].name, suite->tests[i].func)) {
			return CU_get_error();
		}
	}

	return 0;
}

static int test_suite_index(const char *suite_name) {
	int i;

	for (i = 0; i < liblinphone_tester_nb_test_suites(); i++) {
		if ((strcmp(suite_name, test_suite[i]->name) == 0) && (strlen(suite_name) == strlen(test_suite[i]->name))) {
			return i;
		}
	}

	return -1;
}

int liblinphone_tester_nb_test_suites(void) {
	return nb_test_suites;
}

int liblinphone_tester_nb_tests(const char *suite_name) {
	int i = test_suite_index(suite_name);
	if (i < 0) return 0;
	return test_suite[i]->nb_tests;
}

const char * liblinphone_tester_test_suite_name(int suite_index) {
	if (suite_index >= liblinphone_tester_nb_test_suites()) return NULL;
	return test_suite[suite_index]->name;
}

const char * liblinphone_tester_test_name(const char *suite_name, int test_index) {
	int suite_index = test_suite_index(suite_name);
	if ((suite_index < 0) || (suite_index >= liblinphone_tester_nb_test_suites())) return NULL;
	if (test_index >= test_suite[suite_index]->nb_tests) return NULL;
	return test_suite[suite_index]->tests[test_index].name;
}

void liblinphone_tester_init(void) {
	add_test_suite(&setup_test_suite);
	add_test_suite(&register_test_suite);
	add_test_suite(&call_test_suite);
	add_test_suite(&message_test_suite);
	add_test_suite(&presence_test_suite);
}

void liblinphone_tester_uninit(void) {
	if (test_suite != NULL) {
		free(test_suite);
		test_suite = NULL;
		nb_test_suites = 0;
	}
}

int liblinphone_tester_run_tests(const char *suite_name, const char *test_name) {
	int i;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	for (i = 0; i < liblinphone_tester_nb_test_suites(); i++) {
		run_test_suite(test_suite[i]);
	}

#if HAVE_CU_GET_SUITE
	if (suite_name){
		CU_pSuite suite;
		CU_basic_set_mode(CU_BRM_VERBOSE);
		suite=CU_get_suite(suite_name);
		if (test_name) {
			CU_pTest test=CU_get_test_by_name(test_name, suite);
			CU_basic_run_test(suite, test);
		} else
			CU_basic_run_suite(suite);
	} else
#endif
	{
#if HAVE_CU_CURSES
		if (curses) {
			/* Run tests using the CUnit curses interface */
			CU_curses_run_tests();
		}
		else
#endif
		{
			/* Run all tests using the CUnit Basic interface */
			CU_basic_set_mode(CU_BRM_VERBOSE);
			CU_basic_run_tests();
		}
	}

	CU_cleanup_registry();
	return CU_get_error();
}

#ifdef ANDROID
#include <android/log.h>

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
	default:		prio = ANDROID_LOG_DEFAULT;	break;
	}
	linphone_android_log_handler(prio, fmt, args);
}
#endif

#ifndef WINAPI_FAMILY_PHONE_APP
int main (int argc, char *argv[]) {
	int i,j;
	int ret;
	const char *suite_name=NULL;
	const char *test_name=NULL;

	for(i=1;i<argc;++i){
		if (strcmp(argv[i],"--help")==0){
			fprintf(stderr,"%s \t--help\n"
					"\t\t\t--verbose\n"
					"\t\t\t--list-suites\n"
					"\t\t\t--list-tests <suite>\n"
					"\t\t\t--config <config path>\n"
					"\t\t\t--domain <test sip domain>\n"
					"\t\t\t---auth-domain <test auth domain>\n"
#if HAVE_CU_GET_SUITE
					"\t\t\t--suite <suite name>\n"
					"\t\t\t--test <test name>\n"
#endif
#if HAVE_CU_CURSES
					"\t\t\t--curses\n"
#endif
					, argv[0]);
			return 0;
		}else if (strcmp(argv[i],"--verbose")==0){
#ifndef ANDROID
			linphone_core_enable_logs(NULL);
#else
			linphone_core_enable_logs_with_cb(linphone_android_ortp_log_handler);
#endif
		}else if (strcmp(argv[i],"--domain")==0){
			i++;
			test_domain=argv[i];
		}else if (strcmp(argv[i],"--auth-domain")==0){
			i++;
			auth_domain=argv[i];
		}else if (strcmp(argv[i],"--test")==0){
			i++;
			test_name=argv[i];
		}else if (strcmp(argv[i],"--config")==0){
			i++;
			liblinphone_tester_file_prefix=argv[i];
		}else if (strcmp(argv[i],"--suite")==0){
			i++;
			suite_name=argv[i];
		}else if (strcmp(argv[i],"--list-suites")==0){
			for(j=0;j<liblinphone_tester_nb_test_suites();j++) {
				suite_name = liblinphone_tester_test_suite_name(j);
				fprintf(stdout, "%s\n", suite_name);
			}	
		}else if (strcmp(argv[i],"--list-tests")==0){
			suite_name = argv[++i];
			for(j=0;j<liblinphone_tester_nb_tests(suite_name);j++) {
				test_name = liblinphone_tester_test_name(suite_name, j);
				fprintf(stdout, "%s\n", test_name);
			}	
		}
	}
	
	liblinphone_tester_init();
	ret = liblinphone_tester_run_tests(suite_name, test_name);
	liblinphone_tester_uninit();
	return ret;
}
#endif

#ifdef ANDROID
#include <jni.h>
#include <CUnit/Util.h>
#define CALLBACK_BUFFER_SIZE  1024
static JNIEnv *current_env = NULL;
static jobject current_obj = 0;

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
}

JNIEXPORT jint JNICALL Java_org_linphone_tester_Tester_run(JNIEnv *env, jobject obj, jobjectArray stringArray) {
	int i, ret;
	int argc = (*env)->GetArrayLength(env, stringArray);
	char **argv = (char**) malloc(sizeof(char*) * argc);

	for (i=0; i<argc; i++) {
		jstring string = (jstring) (*env)->GetObjectArrayElement(env, stringArray, i);
		const char *rawString = (const char *) (*env)->GetStringUTFChars(env, string, 0);
		argv[i] = strdup(rawString);
		(*env)->ReleaseStringUTFChars(env, argv[i], rawString);
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
#endif

