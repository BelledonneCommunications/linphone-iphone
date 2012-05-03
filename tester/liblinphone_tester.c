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

const char *test_domain="sip.example.org";
const char *auth_domain="auth.example.org";
const char* test_username="liblinphone_tester";
const char* test_password="secret";

static int init(void) {
	return 0;
}
static int uninit(void) {
	return 0;
}
static void core_init_test(void) {
	LinphoneCoreVTable v_table;
	memset (&v_table,0,sizeof(v_table));
	LinphoneCore* lc = linphone_core_new(&v_table,NULL,NULL,NULL);
	CU_ASSERT_PTR_NOT_NULL_FATAL(lc);
	linphone_core_destroy(lc);
}

static LinphoneAddress * create_linphone_address(const char * domain) {
	LinphoneAddress *addr = linphone_address_new(NULL);
	CU_ASSERT_PTR_NOT_NULL_FATAL(addr);
	linphone_address_set_username(addr,test_username);
	CU_ASSERT_STRING_EQUAL(test_username,linphone_address_get_username(addr));
	if (!domain) domain= test_domain;
	linphone_address_set_domain(addr,domain);
	CU_ASSERT_STRING_EQUAL(domain,linphone_address_get_domain(addr));
	linphone_address_set_display_name(addr, NULL);
	linphone_address_set_display_name(addr, "Mr Tester");
	CU_ASSERT_STRING_EQUAL("Mr Tester",linphone_address_get_display_name(addr));
	return addr;
}
static void linphone_address_test(void) {
	ms_free(create_linphone_address(NULL));
}

static int number_of_LinphoneRegistrationNone=0;
static int number_of_LinphoneRegistrationProgress =0;
static int number_of_LinphoneRegistrationOk =0;
static int number_of_LinphoneRegistrationCleared =0;
static int number_of_LinphoneRegistrationFailed =0;
static int number_of_auth_info_requested =0;

static void reset_counters() {
	number_of_LinphoneRegistrationNone=0;
	number_of_LinphoneRegistrationProgress =0;
	number_of_LinphoneRegistrationOk =0;
	number_of_LinphoneRegistrationCleared =0;
	number_of_LinphoneRegistrationFailed =0;
	number_of_auth_info_requested =0;
}

static void registration_state_changed(struct _LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message){
		ms_message("New registration state %s for user id [%s] at proxy [%s]\n"
				,linphone_registration_state_to_string(cstate)
				,linphone_proxy_config_get_identity(cfg)
				,linphone_proxy_config_get_addr(cfg));
		switch (cstate) {
		case LinphoneRegistrationNone:number_of_LinphoneRegistrationNone++;break;
		case LinphoneRegistrationProgress:number_of_LinphoneRegistrationProgress++;break;
		case LinphoneRegistrationOk:number_of_LinphoneRegistrationOk++;break;
		case LinphoneRegistrationCleared:number_of_LinphoneRegistrationCleared++;break;
		case LinphoneRegistrationFailed:number_of_LinphoneRegistrationFailed++;break;
		default:
			CU_FAIL("unexpected event");break;
		}

}

static void auth_info_requested(LinphoneCore *lc, const char *realm, const char *username) {
	ms_message("Auth info requested  for user id [%s] at realm [%s]\n"
					,username
					,realm);
	number_of_auth_info_requested++;

}
static LinphoneCore* create_lc() {
	LinphoneCoreVTable v_table;

	memset (&v_table,0,sizeof(v_table));
	v_table.registration_state_changed=registration_state_changed;
	v_table.auth_info_requested=auth_info_requested;
	return linphone_core_new(&v_table,NULL,NULL,NULL);
}
static void register_with_refresh(LinphoneCore* lc, bool_t refresh,const char* domain,const char* route) {
	int retry=0;
	LCSipTransports transport = {5070,5070,0,5071};
	reset_counters();
	CU_ASSERT_PTR_NOT_NULL_FATAL(lc);

	linphone_core_set_sip_transports(lc,&transport);
	LinphoneProxyConfig* proxy_cfg;

	proxy_cfg = linphone_proxy_config_new();

	LinphoneAddress *from = create_linphone_address(domain);

	linphone_proxy_config_set_identity(proxy_cfg,linphone_address_as_string(from));
	const char* server_addr = linphone_address_get_domain(from);
	linphone_proxy_config_set_server_addr(proxy_cfg,server_addr);
	linphone_proxy_config_enable_register(proxy_cfg,TRUE);
	linphone_proxy_config_expires(proxy_cfg,30);
	if (route) linphone_proxy_config_set_route(proxy_cfg,route);
	linphone_address_destroy(from);

	linphone_core_add_proxy_config(lc,proxy_cfg);
	linphone_core_set_default_proxy(lc,proxy_cfg);

	while (number_of_LinphoneRegistrationOk<1 && retry++ <20) {
		linphone_core_iterate(lc);
		ms_usleep(100000);
	}
	CU_ASSERT_TRUE(linphone_proxy_config_is_registered(proxy_cfg));
	if (refresh) {
		/*wait until refresh*/
		while (number_of_LinphoneRegistrationOk<2 && retry++ <310) {
			linphone_core_iterate(lc);
			ms_usleep(100000);
		}
		linphone_core_destroy(lc);
		CU_ASSERT_EQUAL(number_of_LinphoneRegistrationNone,0);
		CU_ASSERT_EQUAL(number_of_LinphoneRegistrationProgress,2);
		CU_ASSERT_EQUAL(number_of_LinphoneRegistrationOk,2);
		CU_ASSERT_EQUAL(number_of_LinphoneRegistrationCleared,1);
		CU_ASSERT_EQUAL(number_of_LinphoneRegistrationFailed,0);
	} else {
		linphone_core_destroy(lc);
		CU_ASSERT_EQUAL(number_of_LinphoneRegistrationNone,0);
		CU_ASSERT_EQUAL(number_of_LinphoneRegistrationProgress,1);
		CU_ASSERT_EQUAL(number_of_LinphoneRegistrationOk,1);
		CU_ASSERT_EQUAL(number_of_LinphoneRegistrationCleared,1);
		CU_ASSERT_EQUAL(number_of_LinphoneRegistrationFailed,0);
	}


}

static void simple_register(){
	register_with_refresh(create_lc(),FALSE,NULL,NULL);
	CU_ASSERT_EQUAL(number_of_auth_info_requested,0);
}
static void simple_authenticated_register(){
	number_of_auth_info_requested=0;
	LinphoneCore* lc = create_lc();
	LinphoneAuthInfo *info=linphone_auth_info_new(test_username,NULL,test_password,NULL,auth_domain); /*create authentication structure from identity*/
	linphone_core_add_auth_info(lc,info); /*add authentication info to LinphoneCore*/

	register_with_refresh(lc,FALSE,auth_domain,NULL);
	CU_ASSERT_EQUAL(number_of_auth_info_requested,1);
}
int init_test_suite () {

CU_pSuite pSuite = CU_add_suite("liblinphone init test suite", init, uninit);

	if (NULL == CU_add_test(pSuite, "linphone address tester", linphone_address_test)) {
		return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "linphone core init/uninit tester", core_init_test)) {
		return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "simple register tester", simple_register)) {
		return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "simple register with digest auth tester", simple_authenticated_register)) {
		return CU_get_error();
	}
	return 0;
}
int main (int argc, char *argv[]) {
	int i;
	for(i=1;i<argc;++i){
		if (strcmp(argv[i],"--help")==0){
				fprintf(stderr,"%s \t--help\n\t\t\t--verbose",argv[0]);
				return 0;
		}else if (strcmp(argv[i],"--verbose")==0){
			 linphone_core_enable_logs(NULL);
		}else if (strcmp(argv[i],"--domain")==0){
			i++;
			test_domain=argv[i];
		}
	}
	
	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	init_test_suite();
	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return CU_get_error();

}
