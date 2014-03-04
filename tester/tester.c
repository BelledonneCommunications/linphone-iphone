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
#include "CUnit/Basic.h"
#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"
#if HAVE_CU_CURSES
#include "CUnit/CUCurses.h"
#endif

static LinphoneCore* configure_lc_from(LinphoneCoreVTable* v_table, const char* path, const char* file, void* user_data);

static test_suite_t **test_suite = NULL;
static int nb_test_suites = 0;


#if HAVE_CU_CURSES
static unsigned char curses = 0;
#endif

#if TARGET_OS_IPHONE
#include "liblinphonetester_ios.h"
#endif


const char* test_domain="sipopen.example.org";
const char* auth_domain="sip.example.org";
const char* test_username="liblinphone_tester";
const char* test_password="secret";
const char* test_route="sip2.linphone.org";

#if WINAPI_FAMILY_PHONE_APP
const char *liblinphone_tester_file_prefix="Assets";
#elif defined(__QNX__)
const char *liblinphone_tester_file_prefix="./app/native/assets/";
#else
const char *liblinphone_tester_file_prefix=".";
#endif

const char *userhostsfile = "tester_hosts";

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
	memset(counters,0,sizeof(stats));
}

static LinphoneCore* configure_lc_from(LinphoneCoreVTable* v_table, const char* path, const char* file, void* user_data) {
	LinphoneCore* lc;
	char filepath[256]={0};
	char ringpath[256]={0};
	char ringbackpath[256]={0};
	char rootcapath[256]={0};
	char dnsuserhostspath[256]={0};
	char nowebcampath[256]={0};

	if (path==NULL) path=".";

	if (file){
		sprintf(filepath, "%s/%s", path, file);
		CU_ASSERT_TRUE_FATAL(ortp_file_exist(filepath)==0);
	}

	lc =  linphone_core_new(v_table,NULL,*filepath!='\0' ? filepath : NULL, user_data);

	sal_enable_test_features(lc->sal,TRUE);
	snprintf(rootcapath, sizeof(rootcapath), "%s/certificates/cn/cafile.pem", path);
	linphone_core_set_root_ca(lc,rootcapath);

	sprintf(dnsuserhostspath, "%s/%s", path, userhostsfile);
	sal_set_dns_user_hosts_file(lc->sal, dnsuserhostspath);

	snprintf(ringpath,sizeof(ringpath), "%s/sounds/oldphone.wav",path);
	snprintf(ringbackpath,sizeof(ringbackpath), "%s/sounds/ringback.wav", path);
	linphone_core_set_ring(lc, ringpath);
	linphone_core_set_ringback(lc, ringbackpath);

	snprintf(nowebcampath, sizeof(nowebcampath), "%s/images/nowebcamCIF.jpg", path);
	linphone_core_set_static_picture(lc,nowebcampath);
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
	return wait_for_until(lc_1, lc_2,counter,value,3000);
}

bool_t wait_for_list(MSList* lcs,int* counter,int value,int timeout_ms) {
	int retry=0;
	MSList* iterator;
	while ((counter==NULL || *counter<value) && retry++ <timeout_ms/100) {
        for (iterator=lcs;iterator!=NULL;iterator=iterator->next) {
            linphone_core_iterate((LinphoneCore*)(iterator->data));
        }
		ms_usleep(100000);
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
	int retry=0;

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

	reset_counters(&mgr->stat);
	if (rc_file) rc_path = ms_strdup_printf("rcfiles/%s", rc_file);
	mgr->lc=configure_lc_from(&mgr->v_table, liblinphone_tester_file_prefix, rc_path, mgr);
	/*CU_ASSERT_EQUAL(ms_list_size(linphone_core_get_proxy_config_list(lc)),proxy_count);*/
	if (check_for_proxies && rc_file) /**/
		proxy_count=ms_list_size(linphone_core_get_proxy_config_list(mgr->lc));
	else
		proxy_count=0;

	while (mgr->stat.number_of_LinphoneRegistrationOk<proxy_count && retry++ <(30+ (proxy_count>2?(proxy_count-2)*10:0))) {
		linphone_core_iterate(mgr->lc);
		ms_usleep(100000);
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
	int ret;
	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	for (i = 0; i < liblinphone_tester_nb_test_suites(); i++) {
		run_test_suite(test_suite[i]);
	}

	if (suite_name){
		CU_pSuite suite;
		CU_basic_set_mode(CU_BRM_VERBOSE);
		suite=CU_get_suite_by_name(suite_name, CU_get_registry());
		if (test_name) {
			CU_pTest test=CU_get_test_by_name(test_name, suite);
			CU_ErrorCode err= CU_basic_run_test(suite, test);
			if (err != CUE_SUCCESS) ms_error("CU_basic_run_test error %d", err);
		} else
			CU_basic_run_suite(suite);
	} else
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

	ret=CU_get_number_of_tests_failed()!=0;
	CU_cleanup_registry();
	return ret;
}

