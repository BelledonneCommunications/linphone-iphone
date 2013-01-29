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


const char* test_domain="sipopen.example.org";
const char* auth_domain="sip.example.org";
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

LinphoneAddress * create_linphone_address(const char * domain) {
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
	linphone_address_destroy(create_linphone_address(NULL));
}


static  stats global_stat;


void auth_info_requested(LinphoneCore *lc, const char *realm, const char *username) {
	ms_message("Auth info requested  for user id [%s] at realm [%s]\n"
					,username
					,realm);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_auth_info_requested++;
	LinphoneAuthInfo *info=linphone_auth_info_new(test_username,NULL,test_password,NULL,auth_domain); /*create authentication structure from identity*/
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

LinphoneCore* configure_lc_from(LinphoneCoreVTable* v_table, const char* file,int proxy_count) {
	LinphoneCore* lc;
	int retry=0;
	lc =  linphone_core_new(v_table,NULL,file,NULL);
	linphone_core_set_user_data(lc,&global_stat);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	linphone_core_set_ring(lc,"./share/rings/oldphone.wav");
	linphone_core_set_ringback(lc,"./share/ringback.wav");

	reset_counters(counters);
	CU_ASSERT_EQUAL(ms_list_size(linphone_core_get_proxy_config_list(lc)),proxy_count);

	while (counters->number_of_LinphoneRegistrationOk<proxy_count && retry++ <20) {
			linphone_core_iterate(lc);
			ms_usleep(100000);
	}
	CU_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationOk,proxy_count);
	return lc;
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
	case LinphoneCallUpdating :counters->number_of_LinphoneCallUpdating++;break;
	case LinphoneCallReleased :counters->number_of_LinphoneCallReleased++;break;
	default:
		CU_FAIL("unexpected event");break;
	}
}

static void text_message_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from_address, const char *message) {
	char* from=linphone_address_as_string(from_address);
	ms_message("Message from [%s]  is [%s]",from,message);
	ms_free(from);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_LinphoneMessageReceived++;
}

void new_subscribtion_request(LinphoneCore *lc, LinphoneFriend *lf, const char *url){
	char* from=linphone_address_as_string(linphone_friend_get_address(lf));
	ms_message("New subscription request  from [%s]  url [%s]",from,url);
	ms_free(from);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_NewSubscriptionRequest++;
	linphone_core_add_friend(lc,lf); /*accept subscription*/

}
static void notify_presence_received(LinphoneCore *lc, LinphoneFriend * lf) {
	char* from=linphone_address_as_string(linphone_friend_get_address(lf));
	ms_message("New Notify request  from [%s] ",from);
	ms_free(from);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_NotifyReceived++;
}
bool_t wait_for(LinphoneCore* lc_1, LinphoneCore* lc_2,int* counter,int value) {
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
	if((pt = linphone_core_find_payload_type(lc,type,rate,1))) {
		linphone_core_enable_payload_type(lc,pt, 1);
	}
}


LinphoneCoreManager* linphone_core_manager_new(const char* rc_file) {
	LinphoneCoreManager* mgr= malloc(sizeof(LinphoneCoreManager));
	LinphoneProxyConfig* proxy;
	memset (mgr,0,sizeof(LinphoneCoreManager));
	mgr->v_table.registration_state_changed=registration_state_changed;
	mgr->v_table.call_state_changed=call_state_changed;
	mgr->v_table.text_received=text_message_received;
	mgr->v_table.new_subscription_request=new_subscribtion_request;
	mgr->v_table.notify_presence_recv=notify_presence_received;
	mgr->lc=configure_lc_from(&mgr->v_table,rc_file,1);
	enable_codec(mgr->lc,"PCMU",8000);
	linphone_core_set_user_data(mgr->lc,&mgr->stat);
	linphone_core_get_default_proxy(mgr->lc,&proxy);
	mgr->identity = linphone_address_new(linphone_proxy_config_get_identity(proxy));
	linphone_address_clean(mgr->identity);
	return mgr;
}
void linphone_core_manager_destroy(LinphoneCoreManager* mgr) {
	linphone_core_destroy(mgr->lc);
	linphone_address_destroy(mgr->identity);
	free(mgr);
}



int init_test_suite () {

CU_pSuite pSuite = CU_add_suite("Setup", init, uninit);


	if (NULL == CU_add_test(pSuite, "linphone address tester", linphone_address_test)) {
		return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "linphone core init/uninit tester", core_init_test)) {
		return CU_get_error();
	}

	register_test_suite();

	call_test_suite();

	message_test_suite();

	presence_test_suite();

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
		}	else if (strcmp(argv[i],"--auth-domain")==0){
			i++;
			auth_domain=argv[i];
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
