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

typedef struct _stats {
	int number_of_LinphoneRegistrationNone;
	int number_of_LinphoneRegistrationProgress ;
	int number_of_LinphoneRegistrationOk ;
	int number_of_LinphoneRegistrationCleared ;
	int number_of_LinphoneRegistrationFailed ;
	int number_of_auth_info_requested ;


	int number_of_LinphoneCallIncomingReceived;
	int number_of_LinphoneCallOutgoingInit;
	int number_of_LinphoneCallOutgoingProgress;
	int number_of_LinphoneCallOutgoingRinging;
	int number_of_LinphoneCallOutgoingEarlyMedia;
	int number_of_LinphoneCallConnected;
	int number_of_LinphoneCallStreamsRunning;
	int number_of_LinphoneCallPausing;
	int number_of_LinphoneCallPaused;
	int number_of_LinphoneCallResuming;
	int number_of_LinphoneCallRefered;
	int number_of_LinphoneCallError;
	int number_of_LinphoneCallEnd;
	int number_of_LinphoneCallPausedByRemote;
	int number_of_LinphoneCallUpdatedByRemote;
	int number_of_LinphoneCallIncomingEarlyMedia;
	int number_of_LinphoneCallUpdated;
	int number_of_LinphoneCallReleased;
}stats;
static  stats global_stat;

static void reset_counters( stats* counters) {
	memset(counters,0,sizeof(stats));
}

static void registration_state_changed(struct _LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message){
		ms_message("New registration state %s for user id [%s] at proxy [%s]\n"
				,linphone_registration_state_to_string(cstate)
				,linphone_proxy_config_get_identity(cfg)
				,linphone_proxy_config_get_addr(cfg));
		stats* counters = (stats*)linphone_core_get_user_data(lc);
		switch (cstate) {
		case LinphoneRegistrationNone:counters->number_of_LinphoneRegistrationNone++;break;
		case LinphoneRegistrationProgress:counters->number_of_LinphoneRegistrationProgress++;break;
		case LinphoneRegistrationOk:counters->number_of_LinphoneRegistrationOk++;break;
		case LinphoneRegistrationCleared:counters->number_of_LinphoneRegistrationCleared++;break;
		case LinphoneRegistrationFailed:counters->number_of_LinphoneRegistrationFailed++;break;
		default:
			CU_FAIL("unexpected event");break;
		}

}


static LinphoneCore* create_lc() {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	memset (&v_table,0,sizeof(v_table));
	v_table.registration_state_changed=registration_state_changed;
	lc = linphone_core_new(&v_table,NULL,NULL,NULL);
	linphone_core_set_user_data(lc,&global_stat);
	return lc;
}

static void register_with_refresh(LinphoneCore* lc, bool_t refresh,const char* domain,const char* route) {
	int retry=0;
	LCSipTransports transport = {5070,5070,0,5071};

	CU_ASSERT_PTR_NOT_NULL_FATAL(lc);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	reset_counters(counters);
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

	while (counters->number_of_LinphoneRegistrationOk<1 && retry++ <20) {
		linphone_core_iterate(lc);
		ms_usleep(100000);
	}
	CU_ASSERT_TRUE(linphone_proxy_config_is_registered(proxy_cfg));
	if (refresh) {
		/*wait until refresh*/
		while (counters->number_of_LinphoneRegistrationOk<2 && retry++ <310) {
			linphone_core_iterate(lc);
			ms_usleep(100000);
		}
		linphone_core_destroy(lc);
		CU_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationNone,0);
		CU_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationProgress,2);
		CU_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationOk,2);
		CU_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationCleared,1);
		CU_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationFailed,0);
	} else {
		linphone_core_destroy(lc);
		CU_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationNone,0);
		CU_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationProgress,1);
		CU_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationOk,1);
		CU_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationCleared,1);
		CU_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationFailed,0);
	}


}

static void simple_register(){
	LinphoneCore* lc = create_lc();
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	register_with_refresh(lc,FALSE,NULL,NULL);
	CU_ASSERT_EQUAL(counters->number_of_auth_info_requested,0);
}
static void simple_tcp_register(){
	char route[256];
	sprintf(route,"sip:%s;transport=tcp",test_domain);
	LinphoneCore* lc = create_lc();
	register_with_refresh(lc,FALSE,NULL,route);
}
static void simple_tls_register(){
	char route[256];
	sprintf(route,"sip:%s;transport=tls",test_domain);
	LinphoneCore* lc = create_lc();
	register_with_refresh(lc,FALSE,NULL,route);
}

static void simple_authenticated_register(){
	LinphoneCore* lc = create_lc();
	LinphoneAuthInfo *info=linphone_auth_info_new(test_username,NULL,test_password,NULL,auth_domain); /*create authentication structure from identity*/
	linphone_core_add_auth_info(lc,info); /*add authentication info to LinphoneCore*/
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	register_with_refresh(lc,FALSE,auth_domain,NULL);
	CU_ASSERT_EQUAL(counters->number_of_auth_info_requested,0);
}

static void auth_info_requested(LinphoneCore *lc, const char *realm, const char *username) {
	ms_message("Auth info requested  for user id [%s] at realm [%s]\n"
					,username
					,realm);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_auth_info_requested++;
	LinphoneAuthInfo *info=linphone_auth_info_new(test_username,NULL,test_password,NULL,auth_domain); /*create authentication structure from identity*/
	linphone_core_add_auth_info(lc,info); /*add authentication info to LinphoneCore*/

}

static void authenticated_register_with_no_initial_credentials(){
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	memset (&v_table,0,sizeof(v_table));
	v_table.registration_state_changed=registration_state_changed;
	v_table.auth_info_requested=auth_info_requested;
	lc =  linphone_core_new(&v_table,NULL,NULL,NULL);
	linphone_core_set_user_data(lc,&global_stat);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_auth_info_requested=0;
	register_with_refresh(lc,FALSE,auth_domain,NULL);
	CU_ASSERT_EQUAL(counters->number_of_auth_info_requested,1);
}


static LinphoneCore* configure_lc_from(LinphoneCoreVTable* v_table, const char* file,int proxy_count) {
	LinphoneCore* lc;
	int retry=0;
	lc =  linphone_core_new(v_table,NULL,file,NULL);
	linphone_core_set_user_data(lc,&global_stat);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	linphone_core_set_ring(lc,"./share/rings/oldphone.wav");
	linphone_core_set_ringback(lc,"./share/ringback.wav");

	reset_counters(counters);
	CU_ASSERT_EQUAL(ms_list_size(linphone_core_get_proxy_config_list(lc)),proxy_count);

	while (counters->number_of_LinphoneRegistrationOk<3 && retry++ <20) {
			linphone_core_iterate(lc);
			ms_usleep(100000);
	}
	CU_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationOk,proxy_count);
	return lc;
}
static LinphoneCore* configure_lc(LinphoneCoreVTable* v_table) {
	return configure_lc_from(v_table,"./tester/multi_account_lrc",3);
}
static void multiple_proxy(){
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	memset (&v_table,0,sizeof(LinphoneCoreVTable));
	v_table.registration_state_changed=registration_state_changed;
	lc=configure_lc(&v_table);
	linphone_core_destroy(lc);
}
static void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg){
	char* to=linphone_address_as_string(linphone_call_get_call_log(call)->to);
	char* from=linphone_address_as_string(linphone_call_get_call_log(call)->from);

	ms_message("call from [%s] to [%s], new state is [%s]",from,to,linphone_call_state_to_string(cstate));
	ms_free(to);
	ms_free(from);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	switch (cstate) {
	case LinphoneCallIncomingReceived:counters->number_of_LinphoneCallIncomingReceived++;break;
	case LinphoneCallOutgoingInit :counters->number_of_LinphoneCallOutgoingInit++;break;
	case LinphoneCallOutgoingProgress :counters->number_of_LinphoneCallOutgoingProgress++;break;
	case LinphoneCallOutgoingRinging :counters->number_of_LinphoneCallOutgoingRinging++;break;
	case LinphoneCallOutgoingEarlyMedia :counters->number_of_LinphoneCallOutgoingEarlyMedia++;break;
	case LinphoneCallConnected :counters->number_of_LinphoneCallConnected++;break;
	case LinphoneCallStreamsRunning :counters->number_of_LinphoneCallStreamsRunning++;break;
	case LinphoneCallPausing :counters->number_of_LinphoneCallPausing++;break;
	case LinphoneCallPaused :counters->number_of_LinphoneCallPaused++;break;
	case LinphoneCallResuming :counters->number_of_LinphoneCallResuming++;break;
	case LinphoneCallRefered :counters->number_of_LinphoneCallRefered++;break;
	case LinphoneCallError :counters->number_of_LinphoneCallError++;break;
	case LinphoneCallEnd :counters->number_of_LinphoneCallEnd++;break;
	case LinphoneCallPausedByRemote :counters->number_of_LinphoneCallPausedByRemote++;break;
	case LinphoneCallUpdatedByRemote :counters->number_of_LinphoneCallUpdatedByRemote++;break;
	case LinphoneCallIncomingEarlyMedia :counters->number_of_LinphoneCallIncomingEarlyMedia++;break;
	case LinphoneCallUpdated :counters->number_of_LinphoneCallUpdated++;break;
	case LinphoneCallReleased :counters->number_of_LinphoneCallReleased++;break;
	default:
		CU_FAIL("unexpected event");break;
	}
}
static void simple_call_declined() {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	int retry=0;

	memset (&v_table,0,sizeof(LinphoneCoreVTable));
	v_table.registration_state_changed=registration_state_changed;
	v_table.call_state_changed=call_state_changed;
	lc=configure_lc(&v_table);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	linphone_core_invite(lc,"marie");

	while (counters->number_of_LinphoneCallIncomingReceived<1 && retry++ <20) {
				linphone_core_iterate(lc);
				ms_usleep(100000);
	}
	CU_ASSERT_EQUAL(counters->number_of_LinphoneCallIncomingReceived,1);
	CU_ASSERT_TRUE(linphone_core_inc_invite_pending(lc));
	/*linphone_core_terminate_call(lc,linphone_core_get_current_call(lc));*/
	CU_ASSERT_EQUAL(counters->number_of_LinphoneCallReleased,1);
	linphone_core_destroy(lc);
}
static bool_t wait_for(LinphoneCore* lc_1, LinphoneCore* lc_2,int* counter,int value) {
	int retry=0;
	while (*counter<value && retry++ <20) {
		if (lc_1) linphone_core_iterate(lc_1);
		if (lc_2) linphone_core_iterate(lc_2);
		ms_usleep(100000);
	}
	if(*counter<value) return FALSE;
	else return TRUE;
}
static void enable_codec(LinphoneCore* lc,const char* type,int rate) {
	MSList* codecs=ms_list_copy(linphone_core_get_audio_codecs(lc));
	MSList* codecs_it;
	for (codecs_it=codecs;codecs_it!=NULL;codecs_it=codecs_it->next) {
			linphone_core_enable_payload_type(lc,(PayloadType*)codecs_it->data,0);
	}
	PayloadType* pt;
	if((pt = linphone_core_find_payload_type(lc,type,rate))) {
		linphone_core_enable_payload_type(lc,pt, 1);
	}
}
static void simple_call() {
	LinphoneCoreVTable v_table_marie;
	LinphoneCore* lc_marie;
	LinphoneCoreVTable v_table_pauline;
	LinphoneCore* lc_pauline;
	stats stat_marie;
	stats stat_pauline;
	reset_counters(&stat_marie);
	reset_counters(&stat_pauline);



	memset (&v_table_marie,0,sizeof(LinphoneCoreVTable));
	v_table_marie.registration_state_changed=registration_state_changed;
	v_table_marie.call_state_changed=call_state_changed;

	lc_marie=configure_lc_from(&v_table_marie,"./tester/marie_rc",1);
	enable_codec(lc_marie,"PCMU",8000);
	linphone_core_set_user_data(lc_marie,&stat_marie);

	memset (&v_table_pauline,0,sizeof(LinphoneCoreVTable));
	v_table_pauline.registration_state_changed=registration_state_changed;
	v_table_pauline.call_state_changed=call_state_changed;

	lc_pauline=configure_lc_from(&v_table_pauline,"./tester/pauline_rc",1);
	linphone_core_set_user_data(lc_pauline,&stat_pauline);

	linphone_core_invite(lc_marie,"pauline");

	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_pauline.number_of_LinphoneCallIncomingReceived,1));
	CU_ASSERT_TRUE(linphone_core_inc_invite_pending(lc_pauline));
	CU_ASSERT_EQUAL(stat_marie.number_of_LinphoneCallOutgoingProgress,1);
	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_marie.number_of_LinphoneCallOutgoingRinging,1));

	LinphoneProxyConfig* proxy;
	linphone_core_get_default_proxy(lc_marie,&proxy);
	CU_ASSERT_PTR_NOT_NULL_FATAL(proxy);

	CU_ASSERT_STRING_EQUAL(linphone_proxy_config_get_identity(proxy),linphone_core_get_current_call_remote_address(lc_pauline))

	linphone_core_accept_call(lc_pauline,linphone_core_get_current_call(lc_pauline));

	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_pauline.number_of_LinphoneCallConnected,1));
	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_marie.number_of_LinphoneCallConnected,1));
	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_pauline.number_of_LinphoneCallStreamsRunning,1));
	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_marie.number_of_LinphoneCallStreamsRunning,1));
	/*just to sleep*/
	wait_for(lc_pauline,lc_marie,&stat_marie.number_of_LinphoneCallStreamsRunning,3);
	linphone_core_terminate_all_calls(lc_pauline);
	CU_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_pauline.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_marie.number_of_LinphoneCallEnd,1));

	linphone_core_destroy(lc_marie);
	linphone_core_destroy(lc_pauline);
}
int init_test_suite () {

CU_pSuite pSuite = CU_add_suite("liblinphone", init, uninit);


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
	if (NULL == CU_add_test(pSuite, "simple_call_declined", simple_call_declined)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "simple_call", simple_call)) {
			return CU_get_error();
	}

	return 0;
}
int main (int argc, char *argv[]) {
	int i;
	char *test_name=NULL;
	char *suite_name=NULL;
	for(i=1;i<argc;++i){
		if (strcmp(argv[i],"--help")==0){
				fprintf(stderr,"%s \t--help\n\t\t\t--verbose",argv[0]);
				return 0;
		}else if (strcmp(argv[i],"--verbose")==0){
			 linphone_core_enable_logs(NULL);
		}else if (strcmp(argv[i],"--domain")==0){
			i++;
			test_domain=argv[i];
		}else if (strcmp(argv[i],"--test")==0){
			i++;
			test_name=argv[i];
		}else if (strcmp(argv[i],"--suite")==0){
			i++;
			suite_name=argv[i];
		}
	}
	
	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	init_test_suite();
	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
if (suite_name){
#if 1 /*HAVE_CU_GET_SUITE*/
		CU_pSuite suite;
		suite=CU_get_suite(suite_name);
		if (test_name) {
			CU_pTest test=CU_get_test_by_name(test_name, suite);
			CU_basic_run_test(suite, test);
		} else
			CU_basic_run_suite(suite);
#else
	fprintf(stderr,"Your CUnit version does not support suite selection.\n");
#endif
	} else
		CU_basic_run_tests();

	CU_cleanup_registry();
	return CU_get_error();

}
