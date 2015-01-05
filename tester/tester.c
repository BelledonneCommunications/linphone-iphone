 /*
 tester - liblinphone test suite
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
#include "CUnit/TestRun.h"
#include "CUnit/Automated.h"
#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"
#if HAVE_CU_CURSES
#include "CUnit/CUCurses.h"
#endif

static test_suite_t **test_suite = NULL;
static int nb_test_suites = 0;


#if HAVE_CU_CURSES
static unsigned char curses = 0;
#endif

const char* test_domain="sipopen.example.org";
const char* auth_domain="sip.example.org";
const char* test_username="liblinphone_tester";
const char* test_password="secret";
const char* test_route="sip2.linphone.org";
int liblinphone_tester_use_log_file=0;
static int liblinphone_tester_keep_accounts_flag = 0;
static int manager_count = 0;

static const char* liblinphone_tester_xml_file = NULL;
static int      liblinphone_tester_xml_enabled = FALSE;

#if WINAPI_FAMILY_PHONE_APP
const char *liblinphone_tester_file_prefix="Assets";
#elif defined(__QNX__)
const char *liblinphone_tester_file_prefix="./app/native/assets/";
#else
const char *liblinphone_tester_file_prefix=".";
#endif

/* TODO: have the same "static" for QNX and windows as above? */
#ifdef ANDROID
const char *liblinphone_tester_writable_dir_prefix = "/data/data/org.linphone.tester/cache";
#else
const char *liblinphone_tester_writable_dir_prefix = ".";
#endif

const char *userhostsfile = "tester_hosts";
static void network_reachable(LinphoneCore *lc, bool_t reachable) {
	stats* counters;
	ms_message("Network reachable [%s]",reachable?"TRUE":"FALSE");
	counters = get_stats(lc);
	if (reachable)
		counters->number_of_NetworkReachableTrue++;
	else
		counters->number_of_NetworkReachableFalse++;
}
void liblinphone_tester_clock_start(MSTimeSpec *start){
	ms_get_cur_time(start);
}

bool_t liblinphone_tester_clock_elapsed(const MSTimeSpec *start, int value_ms){
	MSTimeSpec current;
	ms_get_cur_time(&current);
	if ((((current.tv_sec-start->tv_sec)*1000LL) + ((current.tv_nsec-start->tv_nsec)/1000000LL))>=value_ms)
		return TRUE;
	return FALSE;
}

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

static void auth_info_requested(LinphoneCore *lc, const char *realm, const char *username, const char *domain) {
	stats* counters;
	ms_message("Auth info requested  for user id [%s] at realm [%s]\n"
			   ,username
			   ,realm);
	counters = get_stats(lc);
	counters->number_of_auth_info_requested++;
}



void reset_counters( stats* counters) {
	if (counters->last_received_chat_message) linphone_chat_message_unref(counters->last_received_chat_message);
	memset(counters,0,sizeof(stats));
}

LinphoneCore* configure_lc_from(LinphoneCoreVTable* v_table, const char* path, const char* file, void* user_data) {
	LinphoneCore* lc;
	LpConfig* config = NULL;
	char *filepath         = NULL;
	char *ringpath         = NULL;
	char *ringbackpath     = NULL;
	char *rootcapath       = NULL;
	char *dnsuserhostspath = NULL;
	char *nowebcampath     = NULL;

	if (path==NULL) path=".";

	if (file){
		filepath = ms_strdup_printf("%s/%s", path, file);
		CU_ASSERT_TRUE_FATAL(ortp_file_exist(filepath)==0);
		config = lp_config_new_with_factory(NULL,filepath);
	}


	// setup dynamic-path assets
	ringpath         = ms_strdup_printf("%s/sounds/oldphone.wav",path);
	ringbackpath     = ms_strdup_printf("%s/sounds/ringback.wav", path);
	nowebcampath     = ms_strdup_printf("%s/images/nowebcamCIF.jpg", path);
	rootcapath       = ms_strdup_printf("%s/certificates/cn/cafile.pem", path);
	dnsuserhostspath = ms_strdup_printf( "%s/%s", path, userhostsfile);


	if( config != NULL ) {
		lp_config_set_string(config, "sound", "remote_ring", ringbackpath);
		lp_config_set_string(config, "sound", "local_ring" , ringpath);
		lp_config_set_string(config, "sip",   "root_ca"    , rootcapath);
		lc = linphone_core_new_with_config(v_table, config, user_data);
	} else {
		lc = linphone_core_new(v_table,NULL,(filepath!=NULL&&filepath[0]!='\0') ? filepath : NULL, user_data);

		linphone_core_set_ring(lc, ringpath);
		linphone_core_set_ringback(lc, ringbackpath);
		linphone_core_set_root_ca(lc,rootcapath);
	}

	sal_enable_test_features(lc->sal,TRUE);
	sal_set_dns_user_hosts_file(lc->sal, dnsuserhostspath);
	linphone_core_set_static_picture(lc,nowebcampath);


	ms_free(ringpath);
	ms_free(ringbackpath);
	ms_free(nowebcampath);
	ms_free(rootcapath);
	ms_free(dnsuserhostspath);

	if( filepath ) ms_free(filepath);

	if( config ) lp_config_unref(config);

	return lc;
}


bool_t wait_for_until(LinphoneCore* lc_1, LinphoneCore* lc_2,int* counter,int value,int timout) {
	MSList* lcs=NULL;
	bool_t result;
	if (lc_1)
		lcs=ms_list_append(lcs,lc_1);
	if (lc_2)
		lcs=ms_list_append(lcs,lc_2);
	result=wait_for_list(lcs,counter,value,timout);
	ms_list_free(lcs);
	return result;
}

bool_t wait_for(LinphoneCore* lc_1, LinphoneCore* lc_2,int* counter,int value) {
	return wait_for_until(lc_1, lc_2,counter,value,10000);
}

bool_t wait_for_list(MSList* lcs,int* counter,int value,int timeout_ms) {
	MSList* iterator;
	MSTimeSpec start;

	liblinphone_tester_clock_start(&start);
	while ((counter==NULL || *counter<value) && !liblinphone_tester_clock_elapsed(&start,timeout_ms)) {
		for (iterator=lcs;iterator!=NULL;iterator=iterator->next) {
			linphone_core_iterate((LinphoneCore*)(iterator->data));
		}
		ms_usleep(20000);
	}
	if(counter && *counter<value) return FALSE;
	else return TRUE;
}

static void set_codec_enable(LinphoneCore* lc,const char* type,int rate,bool_t enable) {
	MSList* codecs=ms_list_copy(linphone_core_get_audio_codecs(lc));
	MSList* codecs_it;
	PayloadType* pt;
	for (codecs_it=codecs;codecs_it!=NULL;codecs_it=codecs_it->next) {
		linphone_core_enable_payload_type(lc,(PayloadType*)codecs_it->data,0);
	}
	if((pt = linphone_core_find_payload_type(lc,type,rate,1))) {
		linphone_core_enable_payload_type(lc,pt, enable);
	}
	ms_list_free(codecs);
}

static void enable_codec(LinphoneCore* lc,const char* type,int rate) {
	set_codec_enable(lc,type,rate,TRUE);
}
stats * get_stats(LinphoneCore *lc){
	LinphoneCoreManager *manager=(LinphoneCoreManager *)linphone_core_get_user_data(lc);
	return &manager->stat;
}

LinphoneCoreManager *get_manager(LinphoneCore *lc){
	LinphoneCoreManager *manager=(LinphoneCoreManager *)linphone_core_get_user_data(lc);
	return manager;
}

LinphoneCoreManager* linphone_core_manager_new2(const char* rc_file, int check_for_proxies) {
	LinphoneCoreManager* mgr= ms_new0(LinphoneCoreManager,1);
	LinphoneProxyConfig* proxy;
	char *rc_path = NULL;
	int proxy_count;

	mgr->v_table.registration_state_changed=registration_state_changed;
	mgr->v_table.auth_info_requested=auth_info_requested;
	mgr->v_table.call_state_changed=call_state_changed;
	mgr->v_table.text_received=text_message_received;
	mgr->v_table.message_received=message_received;
	mgr->v_table.is_composing_received=is_composing_received;
	mgr->v_table.new_subscription_requested=new_subscription_requested;
	mgr->v_table.notify_presence_received=notify_presence_received;
	mgr->v_table.transfer_state_changed=linphone_transfer_state_changed;
	mgr->v_table.info_received=info_message_received;
	mgr->v_table.subscription_state_changed=linphone_subscription_state_change;
	mgr->v_table.notify_received=linphone_notify_received;
	mgr->v_table.publish_state_changed=linphone_publish_state_changed;
	mgr->v_table.configuring_status=linphone_configuration_status;
	mgr->v_table.call_encryption_changed=linphone_call_encryption_changed;
	mgr->v_table.network_reachable=network_reachable;
	mgr->v_table.dtmf_received=dtmf_received;
	mgr->v_table.call_stats_updated=call_stats_updated;

	reset_counters(&mgr->stat);
	if (rc_file) rc_path = ms_strdup_printf("rcfiles/%s", rc_file);
	mgr->lc=configure_lc_from(&mgr->v_table, liblinphone_tester_file_prefix, rc_path, mgr);
	linphone_core_manager_check_accounts(mgr);
	/*CU_ASSERT_EQUAL(ms_list_size(linphone_core_get_proxy_config_list(lc)),proxy_count);*/
	if (check_for_proxies && rc_file) /**/
		proxy_count=ms_list_size(linphone_core_get_proxy_config_list(mgr->lc));
	else
		proxy_count=0;

	manager_count++;

#if TARGET_OS_IPHONE
	linphone_core_set_ringer_device( mgr->lc, "AQ: Audio Queue Device");
	linphone_core_set_ringback(mgr->lc, NULL);
#endif

	if( manager_count >= 2){
		char hellopath[512];
		ms_message("Manager for '%s' using files", rc_file ? rc_file : "--");
		linphone_core_use_files(mgr->lc, TRUE);
		snprintf(hellopath,sizeof(hellopath), "%s/sounds/hello8000.wav", liblinphone_tester_file_prefix);
		linphone_core_set_play_file(mgr->lc,hellopath);
	}

	if (proxy_count){
#define REGISTER_TIMEOUT 20 /* seconds */
		int success = wait_for_until(mgr->lc,NULL,&mgr->stat.number_of_LinphoneRegistrationOk,
									proxy_count,(REGISTER_TIMEOUT * 1000 * proxy_count));
		if( !success ){
			ms_error("Did not register after %d seconds for %d proxies", REGISTER_TIMEOUT, proxy_count);
		}
	}
	CU_ASSERT_EQUAL(mgr->stat.number_of_LinphoneRegistrationOk,proxy_count);
	enable_codec(mgr->lc,"PCMU",8000);

	linphone_core_get_default_proxy(mgr->lc,&proxy);
	if (proxy) {
		mgr->identity = linphone_address_new(linphone_proxy_config_get_identity(proxy));
		linphone_address_clean(mgr->identity);
	}
	if (rc_path) ms_free(rc_path);
	return mgr;
}

LinphoneCoreManager* linphone_core_manager_new( const char* rc_file) {
	return linphone_core_manager_new2(rc_file, TRUE);
}

void linphone_core_manager_stop(LinphoneCoreManager *mgr){
	if (mgr->lc) {
		linphone_core_destroy(mgr->lc);
		mgr->lc=NULL;
	}
}

void linphone_core_manager_destroy(LinphoneCoreManager* mgr) {
	if (mgr->lc) linphone_core_destroy(mgr->lc);
	if (mgr->identity) linphone_address_destroy(mgr->identity);
	if (mgr->stat.last_received_chat_message) linphone_chat_message_unref(mgr->stat.last_received_chat_message);
	manager_count--;
	ms_free(mgr);
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

int liblinphone_tester_test_suite_index(const char *suite_name) {
	int i;

	for (i = 0; i < liblinphone_tester_nb_test_suites(); i++) {
		if ((strcmp(suite_name, test_suite[i]->name) == 0) && (strlen(suite_name) == strlen(test_suite[i]->name))) {
			return i;
		}
	}

	return -1;
}

void liblinphone_tester_list_suites() {
	int j;
	for(j=0;j<liblinphone_tester_nb_test_suites();j++) {
		liblinphone_tester_fprintf(stdout, "%s\n", liblinphone_tester_test_suite_name(j));
	}
}

void liblinphone_tester_list_suite_tests(const char *suite_name) {
	int j;
	for( j = 0; j < liblinphone_tester_nb_tests(suite_name); j++) {
		const char *test_name = liblinphone_tester_test_name(suite_name, j);
		liblinphone_tester_fprintf(stdout, "%s\n", test_name);
	}
}

int liblinphone_tester_test_index(const char *suite_name, const char *test_name) {
	int j,i;

	j = liblinphone_tester_test_suite_index(suite_name);
	if(j != -1) {
		for (i = 0; i < test_suite[j]->nb_tests; i++) {
			if ((strcmp(test_name, test_suite[j]->tests[i].name) == 0) && (strlen(test_name) == strlen(test_suite[j]->tests[i].name))) {
				return i;
			}
		}
	}

	return -1;
}

int liblinphone_tester_nb_test_suites(void) {
	return nb_test_suites;
}

int liblinphone_tester_nb_tests(const char *suite_name) {
	int i = liblinphone_tester_test_suite_index(suite_name);
	if (i < 0) return 0;
	return test_suite[i]->nb_tests;
}

const char * liblinphone_tester_test_suite_name(int suite_index) {
	if (suite_index >= liblinphone_tester_nb_test_suites()) return NULL;
	return test_suite[suite_index]->name;
}

const char * liblinphone_tester_test_name(const char *suite_name, int test_index) {
	int suite_index = liblinphone_tester_test_suite_index(suite_name);
	if ((suite_index < 0) || (suite_index >= liblinphone_tester_nb_test_suites())) return NULL;
	if (test_index >= test_suite[suite_index]->nb_tests) return NULL;
	return test_suite[suite_index]->tests[test_index].name;
}

void liblinphone_tester_set_fileprefix(const char* file_prefix){
	liblinphone_tester_file_prefix = file_prefix;
}

void liblinphone_tester_set_writable_dir_prefix(const char* writable_dir_prefix){
	liblinphone_tester_writable_dir_prefix = writable_dir_prefix;
}


void liblinphone_tester_init(void) {
	add_test_suite(&setup_test_suite);
	add_test_suite(&register_test_suite);
	add_test_suite(&call_test_suite);
	add_test_suite(&message_test_suite);
	add_test_suite(&presence_test_suite);
#ifdef UPNP
	add_test_suite(&upnp_test_suite);
#endif
	add_test_suite(&stun_test_suite);
	add_test_suite(&event_test_suite);
	add_test_suite(&flexisip_test_suite);
	add_test_suite(&remote_provisioning_test_suite);
	add_test_suite(&quality_reporting_test_suite);
	add_test_suite(&log_collection_test_suite);
	add_test_suite(&transport_test_suite);
	add_test_suite(&player_test_suite);
	add_test_suite(&dtmf_test_suite);
}

void liblinphone_tester_uninit(void) {
	if (test_suite != NULL) {
		free(test_suite);
		test_suite = NULL;
		nb_test_suites = 0;
	}
}

/*derivated from cunit*/
static void test_complete_message_handler(const CU_pTest pTest,
                                                const CU_pSuite pSuite,
                                                const CU_pFailureRecord pFailureList) {
    int i;
    CU_pFailureRecord pFailure = pFailureList;
	if (pFailure) {
		if (liblinphone_tester_use_log_file) ms_warning("Suite [%s], Test [%s] had failures:", pSuite->pName, pTest->pName);
		liblinphone_tester_fprintf(stdout,"\nSuite [%s], Test [%s] had failures:", pSuite->pName, pTest->pName);
	} else {
		if (liblinphone_tester_use_log_file) ms_warning(" passed");
		liblinphone_tester_fprintf(stdout," passed");
	}
      for (i = 1 ; (NULL != pFailure) ; pFailure = pFailure->pNext, i++) {
    	  if (liblinphone_tester_use_log_file) ms_warning("\n    %d. %s:%u  - %s", i,
            (NULL != pFailure->strFileName) ? pFailure->strFileName : "",
            pFailure->uiLineNumber,
            (NULL != pFailure->strCondition) ? pFailure->strCondition : "");
    	  liblinphone_tester_fprintf(stdout,"\n    %d. %s:%u  - %s", i,
            (NULL != pFailure->strFileName) ? pFailure->strFileName : "",
            pFailure->uiLineNumber,
            (NULL != pFailure->strCondition) ? pFailure->strCondition : "");
      }
 }


static void test_all_tests_complete_message_handler(const CU_pFailureRecord pFailure) {
  if (liblinphone_tester_use_log_file) ms_warning("\n\n %s",CU_get_run_results_string());
  liblinphone_tester_fprintf(stdout,"\n\n %s",CU_get_run_results_string());
}

static void test_suite_init_failure_message_handler(const CU_pSuite pSuite) {
	if (liblinphone_tester_use_log_file) ms_warning("Suite initialization failed for [%s].", pSuite->pName);
    liblinphone_tester_fprintf(stdout,"Suite initialization failed for [%s].", pSuite->pName);
}

static void test_suite_cleanup_failure_message_handler(const CU_pSuite pSuite) {
	if (liblinphone_tester_use_log_file) ms_warning("Suite cleanup failed for '%s'.", pSuite->pName);
	liblinphone_tester_fprintf(stdout,"Suite cleanup failed for [%s].", pSuite->pName);
}

static void test_start_message_handler(const CU_pTest pTest, const CU_pSuite pSuite) {
	if (liblinphone_tester_use_log_file) ms_warning("Suite [%s] Test [%s]", pSuite->pName,pTest->pName);
	liblinphone_tester_fprintf(stdout,"\nSuite [%s] Test [%s]\n", pSuite->pName,pTest->pName);
}
static void test_suite_start_message_handler(const CU_pSuite pSuite) {
	if (liblinphone_tester_use_log_file) ms_warning("Suite [%s]", pSuite->pName);
	liblinphone_tester_fprintf(stdout,"\nSuite [%s]", pSuite->pName);
}

int liblinphone_tester_run_tests(const char *suite_name, const char *test_name) {
	int i;
	int ret;
	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	for (i = 0; i < liblinphone_tester_nb_test_suites(); i++) {
		run_test_suite(test_suite[i]);
	}

	CU_set_test_start_handler(test_start_message_handler);
	CU_set_test_complete_handler(test_complete_message_handler);
	CU_set_all_test_complete_handler(test_all_tests_complete_message_handler);
	CU_set_suite_init_failure_handler(test_suite_init_failure_message_handler);
	CU_set_suite_cleanup_failure_handler(test_suite_cleanup_failure_message_handler);
	CU_set_suite_start_handler(test_suite_start_message_handler);


	if( liblinphone_tester_xml_file != NULL ){
		CU_set_output_filename(liblinphone_tester_xml_file);
	}
	if( liblinphone_tester_xml_enabled != 0 ){
		CU_automated_run_tests();
	} else {

#if !HAVE_CU_GET_SUITE
		if( suite_name ){
			ms_warning("Tester compiled without CU_get_suite() function, running all tests instead of suite '%s'\n", suite_name);
		}
#else
		if (suite_name){
			CU_pSuite suite;
			suite=CU_get_suite(suite_name);
			if (!suite) {
				ms_error("Could not find suite '%s'. Available suites are:", suite_name);
				liblinphone_tester_list_suites();
				return -1;
			} else if (test_name) {
				CU_pTest test=CU_get_test_by_name(test_name, suite);
				if (!test) {
					ms_error("Could not find test '%s' in suite '%s'. Available tests are:", test_name, suite_name);
					// do not use suite_name here, since this method is case sentisitive
					liblinphone_tester_list_suite_tests(suite->pName);
					return -2;
				} else {
					CU_ErrorCode err= CU_run_test(suite, test);
					if (err != CUE_SUCCESS) ms_error("CU_basic_run_test error %d", err);
				}
			} else {
				CU_run_suite(suite);
			}
		}
		else
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
				CU_run_all_tests();
			}
		}

	}
	ret=CU_get_number_of_tests_failed()!=0;

	/* Redisplay list of failed tests on end */
	if (CU_get_number_of_failure_records()){
		CU_basic_show_failures(CU_get_failure_list());
		liblinphone_tester_fprintf(stdout,"\n");
	}

	CU_cleanup_registry();

	if( liblinphone_tester_keep_accounts_flag == 0){
		liblinphone_tester_clear_accounts();
	}
	return ret;
}

int  liblinphone_tester_fprintf(FILE * stream, const char * format, ...) {
	int result;
	va_list args;
	va_start(args, format);
#ifndef ANDROID
	result = vfprintf(stream,format,args);
	fflush(stream);
#else
	/*used by liblinphone tester to retrieve suite list*/
	result = 0;
	cunit_android_trace_handler(stream, format, args);
#endif
	va_end(args);
	return result;
}

int liblinphone_tester_ipv6_available(void){
	struct addrinfo *ai=belle_sip_ip_address_to_addrinfo(AF_INET6,"2a01:e00::2",53);
	if (ai){
		struct sockaddr_storage ss;
		struct addrinfo src;
		socklen_t slen=sizeof(ss);
		char localip[128];
		int port=0;
		belle_sip_get_src_addr_for(ai->ai_addr,ai->ai_addrlen,(struct sockaddr*) &ss,&slen,4444);
		src.ai_addr=(struct sockaddr*) &ss;
		src.ai_addrlen=slen;
		belle_sip_addrinfo_to_ip(&src,localip, sizeof(localip),&port);
		freeaddrinfo(ai);
		return strcmp(localip,"::1")!=0;
	}
	return FALSE;
}

void liblinphone_tester_keep_accounts( int keep ){
	liblinphone_tester_keep_accounts_flag = keep;
}

void liblinphone_tester_clear_accounts(void){
	account_manager_destroy();
}

void liblinphone_tester_enable_xml( bool_t enable ){
	liblinphone_tester_xml_enabled = enable;
}

void liblinphone_tester_set_xml_output(const char *xml_path ) {
	liblinphone_tester_xml_file = xml_path;
}

const char* liblinphone_tester_get_xml_output( void ) {
	return liblinphone_tester_xml_file;
}




