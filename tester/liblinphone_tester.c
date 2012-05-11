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


static int number_of_LinphoneCallIncomingReceived=0;
static int number_of_LinphoneCallOutgoingInit=0;
static int number_of_LinphoneCallOutgoingProgress=0;
static int number_of_LinphoneCallOutgoingRinging=0;
static int number_of_LinphoneCallOutgoingEarlyMedia=0;
static int number_of_LinphoneCallConnected=0;
static int number_of_LinphoneCallStreamsRunning=0;
static int number_of_LinphoneCallPausing=0;
static int number_of_LinphoneCallPaused=0;
static int number_of_LinphoneCallResuming=0;
static int number_of_LinphoneCallRefered=0;
static int number_of_LinphoneCallError=0;
static int number_of_LinphoneCallEnd=0;
static int number_of_LinphoneCallPausedByRemote=0;
static int number_of_LinphoneCallUpdatedByRemote=0;
static int number_of_LinphoneCallIncomingEarlyMedia=0;
static int number_of_LinphoneCallUpdated=0;
static int number_of_LinphoneCallReleased=0;

static void reset_counters() {
	number_of_LinphoneRegistrationNone=0;
	number_of_LinphoneRegistrationProgress =0;
	number_of_LinphoneRegistrationOk =0;
	number_of_LinphoneRegistrationCleared =0;
	number_of_LinphoneRegistrationFailed =0;
	number_of_auth_info_requested =0;
	number_of_LinphoneCallIncomingReceived=0;
	number_of_LinphoneCallOutgoingInit=0;
	number_of_LinphoneCallOutgoingProgress=0;
	number_of_LinphoneCallOutgoingRinging=0;
	number_of_LinphoneCallOutgoingEarlyMedia=0;
	number_of_LinphoneCallConnected=0;
	number_of_LinphoneCallStreamsRunning=0;
	number_of_LinphoneCallPausing=0;
	number_of_LinphoneCallPaused=0;
	number_of_LinphoneCallResuming=0;
	number_of_LinphoneCallRefered=0;
	number_of_LinphoneCallError=0;
	number_of_LinphoneCallEnd=0;
	number_of_LinphoneCallPausedByRemote=0;
	number_of_LinphoneCallUpdatedByRemote=0;
	number_of_LinphoneCallIncomingEarlyMedia=0;
	number_of_LinphoneCallUpdated=0;
	number_of_LinphoneCallReleased=0;
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


static LinphoneCore* create_lc() {
	LinphoneCoreVTable v_table;

	memset (&v_table,0,sizeof(v_table));
	v_table.registration_state_changed=registration_state_changed;
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

	linphone_proxy_config_enable_register(proxy_cfg,TRUE);
	linphone_proxy_config_expires(proxy_cfg,30);
	if (route) {
		linphone_proxy_config_set_route(proxy_cfg,route);
		linphone_proxy_config_set_server_addr(proxy_cfg,route);
	} else {
		linphone_proxy_config_set_server_addr(proxy_cfg,server_addr);
	}
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
static void simple_tcp_register(){
	char route[256];
	sprintf(route,"sip:%s;transport=tcp",test_domain);
	register_with_refresh(create_lc(),FALSE,NULL,route);
}
static void simple_tls_register(){
	char route[256];
	sprintf(route,"sip:%s;transport=tls",test_domain);
	register_with_refresh(create_lc(),FALSE,NULL,route);
}

static void simple_authenticated_register(){
	LinphoneCore* lc = create_lc();
	LinphoneAuthInfo *info=linphone_auth_info_new(test_username,NULL,test_password,NULL,auth_domain); /*create authentication structure from identity*/
	linphone_core_add_auth_info(lc,info); /*add authentication info to LinphoneCore*/

	register_with_refresh(lc,FALSE,auth_domain,NULL);
	CU_ASSERT_EQUAL(number_of_auth_info_requested,0);
}

static void auth_info_requested(LinphoneCore *lc, const char *realm, const char *username) {
	ms_message("Auth info requested  for user id [%s] at realm [%s]\n"
					,username
					,realm);
	number_of_auth_info_requested++;
	LinphoneAuthInfo *info=linphone_auth_info_new(test_username,NULL,test_password,NULL,auth_domain); /*create authentication structure from identity*/
	linphone_core_add_auth_info(lc,info); /*add authentication info to LinphoneCore*/

}

static void authenticated_register_with_no_initial_credentials(){
	number_of_auth_info_requested=0;
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	memset (&v_table,0,sizeof(v_table));
	v_table.registration_state_changed=registration_state_changed;
	v_table.auth_info_requested=auth_info_requested;
	lc =  linphone_core_new(&v_table,NULL,NULL,NULL);

	register_with_refresh(lc,FALSE,auth_domain,NULL);
	CU_ASSERT_EQUAL(number_of_auth_info_requested,1);
}


static LinphoneCore* configure_lc(LinphoneCoreVTable* v_table) {

	LinphoneCore* lc;
	int retry=0;
	memset (v_table,0,sizeof(LinphoneCoreVTable));
	reset_counters();
	lc =  linphone_core_new(v_table,NULL,"./multi_account_lrc",NULL);

	CU_ASSERT_EQUAL(ms_list_size(linphone_core_get_proxy_config_list(lc)),3);

	while (number_of_LinphoneRegistrationOk<3 && retry++ <20) {
			linphone_core_iterate(lc);
			ms_usleep(100000);
	}
	CU_ASSERT_EQUAL(number_of_LinphoneRegistrationOk,3);
	return lc;
}
static void multiple_proxy(){
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	v_table.registration_state_changed=registration_state_changed;
	lc=configure_lc(&v_table);
	linphone_core_destroy(lc);
}
static void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg){
	char* to=linphone_call_get_remote_address_as_string(call);
	ms_message("call from [??] to [%s], new state is [%s]",to,linphone_call_state_to_string(cstate));
	ms_free(to);
	switch (cstate) {
	case LinphoneCallIncomingReceived:number_of_LinphoneCallIncomingReceived++;break;
	case LinphoneCallOutgoingInit :number_of_LinphoneCallOutgoingInit++;break;
	case LinphoneCallOutgoingProgress :number_of_LinphoneCallOutgoingProgress++;break;
	case LinphoneCallOutgoingRinging :number_of_LinphoneCallOutgoingRinging++;break;
	case LinphoneCallOutgoingEarlyMedia :number_of_LinphoneCallOutgoingEarlyMedia++;break;
	case LinphoneCallConnected :number_of_LinphoneCallConnected++;break;
	case LinphoneCallStreamsRunning :number_of_LinphoneCallStreamsRunning++;break;
	case LinphoneCallPausing :number_of_LinphoneCallPausing++;break;
	case LinphoneCallPaused :number_of_LinphoneCallPaused++;break;
	case LinphoneCallResuming :number_of_LinphoneCallResuming++;break;
	case LinphoneCallRefered :number_of_LinphoneCallRefered++;break;
	case LinphoneCallError :number_of_LinphoneCallError++;break;
	case LinphoneCallEnd :number_of_LinphoneCallEnd++;break;
	case LinphoneCallPausedByRemote :number_of_LinphoneCallPausedByRemote++;break;
	case LinphoneCallUpdatedByRemote :number_of_LinphoneCallUpdatedByRemote++;break;
	case LinphoneCallIncomingEarlyMedia :number_of_LinphoneCallIncomingEarlyMedia++;break;
	case LinphoneCallUpdated :number_of_LinphoneCallUpdated++;break;
	case LinphoneCallReleased :number_of_LinphoneCallReleased++;break;
	default:
		CU_FAIL("unexpected event");break;
	}
}
static void simple_call_declined() {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	int retry=0;

	v_table.registration_state_changed=registration_state_changed;
	v_table.call_state_changed=call_state_changed;
	lc=configure_lc(&v_table);
	linphone_core_invite(lc,"marie");

	while (number_of_LinphoneCallIncomingReceived<1 && retry++ <20) {
				linphone_core_iterate(lc);
				ms_usleep(100000);
	}
	CU_ASSERT_EQUAL(number_of_LinphoneCallIncomingReceived,1);
	CU_ASSERT_TRUE(linphone_core_inc_invite_pending(lc));
	linphone_core_terminate_call(lc,linphone_core_get_current_call(lc));
	CU_ASSERT_EQUAL(number_of_LinphoneCallReleased,1);
	linphone_core_destroy(lc);
}
int init_test_suite () {

CU_pSuite pSuite = CU_add_suite("liblinphone init test suite", init, uninit);
	if (NULL == CU_add_test(pSuite, "simple call declined", simple_call_declined)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "linphone address tester", linphone_address_test)) {
		return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "linphone core init/uninit tester", core_init_test)) {
		return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "simple register tester", simple_register)) {
		return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "tcp register tester", simple_tcp_register)) {
		return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "tls register tester", simple_tls_register)) {
		return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "simple register with digest auth tester", simple_authenticated_register)) {
		return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "register with digest auth tester without initial credentials", authenticated_register_with_no_initial_credentials)) {
		return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "multi account", multiple_proxy)) {
		return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "simple call declined", simple_call_declined)) {
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
