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

const char *test_domain="sipopen.example.org";
const char *auth_domain="sip.example.org";
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

	int number_of_LinphoneMessageReceived;
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

	while (counters->number_of_LinphoneRegistrationOk<proxy_count && retry++ <20) {
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

static void text_message_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from_address, const char *message) {

	char* from=linphone_address_as_string(from_address);
	ms_message("Message from [%s]  is [%s]",from,message);
	ms_free(from);
	stats* counters = (stats*)linphone_core_get_user_data(lc);
	counters->number_of_LinphoneMessageReceived++;
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
typedef struct _LinphoneCoreManager {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	stats stat;
	LinphoneAddress* identity;
} LinphoneCoreManager;

static LinphoneCoreManager* linphone_core_manager_new(const char* rc_file) {
	LinphoneCoreManager* mgr= malloc(sizeof(LinphoneCoreManager));
	LinphoneProxyConfig* proxy;
	memset (mgr,0,sizeof(LinphoneCoreManager));
	mgr->v_table.registration_state_changed=registration_state_changed;
	mgr->v_table.call_state_changed=call_state_changed;
	mgr->v_table.text_received=text_message_received;
	mgr->lc=configure_lc_from(&mgr->v_table,rc_file,1);
	enable_codec(mgr->lc,"PCMU",8000);
	linphone_core_set_user_data(mgr->lc,&mgr->stat);
	linphone_core_get_default_proxy(mgr->lc,&proxy);
	mgr->identity = linphone_address_new(linphone_proxy_config_get_identity(proxy));
	linphone_address_clean(mgr->identity);
	return mgr;
}
static void linphone_core_manager_destroy(LinphoneCoreManager* mgr) {
	linphone_core_destroy(mgr->lc);
	linphone_address_destroy(mgr->identity);
	free(mgr);
}

static bool_t call(LinphoneCoreManager* caller_mgr,LinphoneCoreManager* callee_mgr) {
	LinphoneProxyConfig* proxy;
	linphone_core_get_default_proxy(callee_mgr->lc,&proxy);
	CU_ASSERT_PTR_NOT_NULL_FATAL(proxy);


	CU_ASSERT_PTR_NOT_NULL_FATAL(linphone_core_invite_address(caller_mgr->lc,callee_mgr->identity));

	/*linphone_core_invite(caller_mgr->lc,"pauline");*/

	CU_ASSERT_TRUE_FATAL(wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallIncomingReceived,1));
	CU_ASSERT_TRUE(linphone_core_inc_invite_pending(callee_mgr->lc));
	CU_ASSERT_EQUAL(caller_mgr->stat.number_of_LinphoneCallOutgoingProgress,1);
	CU_ASSERT_TRUE_FATAL(wait_for(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallOutgoingRinging,1));

	linphone_core_get_default_proxy(caller_mgr->lc,&proxy);
	CU_ASSERT_PTR_NOT_NULL_FATAL(proxy);
	LinphoneAddress* identity = linphone_address_new(linphone_proxy_config_get_identity(proxy));
	CU_ASSERT_TRUE(linphone_address_weak_equal(identity,linphone_core_get_current_call_remote_address(callee_mgr->lc)));
	linphone_address_destroy(identity);

	linphone_core_accept_call(callee_mgr->lc,linphone_core_get_current_call(callee_mgr->lc));

	CU_ASSERT_TRUE_FATAL(wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallConnected,1));
	CU_ASSERT_TRUE_FATAL(wait_for(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallConnected,1));
	/*just to sleep*/
	return wait_for(callee_mgr->lc,caller_mgr->lc,&caller_mgr->stat.number_of_LinphoneCallStreamsRunning,1)
			&&
			wait_for(callee_mgr->lc,caller_mgr->lc,&callee_mgr->stat.number_of_LinphoneCallStreamsRunning,1);

}
static void simple_call() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");

	LinphoneCore* lc_marie=marie->lc;
	LinphoneCore* lc_pauline=pauline->lc;
	stats* stat_marie=&marie->stat;
	stats* stat_pauline=&pauline->stat;



	linphone_core_invite(lc_marie,"pauline");

	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallIncomingReceived,1));
	CU_ASSERT_TRUE(linphone_core_inc_invite_pending(lc_pauline));
	CU_ASSERT_EQUAL(stat_marie->number_of_LinphoneCallOutgoingProgress,1);
	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallOutgoingRinging,1));

	LinphoneProxyConfig* proxy;
	linphone_core_get_default_proxy(lc_marie,&proxy);
	CU_ASSERT_PTR_NOT_NULL_FATAL(proxy);
	LinphoneAddress* identity = linphone_address_new(linphone_proxy_config_get_identity(proxy));
	CU_ASSERT_TRUE(linphone_address_weak_equal(identity,linphone_core_get_current_call_remote_address(lc_pauline)));
	linphone_address_destroy(identity);

	linphone_core_accept_call(lc_pauline,linphone_core_get_current_call(lc_pauline));

	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallConnected,1));
	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallConnected,1));
	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallStreamsRunning,1));
	CU_ASSERT_TRUE_FATAL(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallStreamsRunning,1));
	/*just to sleep*/
	wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallStreamsRunning,3);
	linphone_core_terminate_all_calls(lc_pauline);
	CU_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_pauline->number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(lc_pauline,lc_marie,&stat_marie->number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void call_canceled() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");

	LinphoneCall* out_call = linphone_core_invite(pauline->lc,"marie");
	linphone_call_ref(out_call);
	CU_ASSERT_TRUE_FATAL(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallOutgoingInit,1));

	linphone_core_terminate_call(pauline->lc,out_call);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	//CU_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonCanceled);
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneCallEnd,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneCallIncomingReceived,0);
	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void call_ringing_canceled() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");

	LinphoneCall* out_call = linphone_core_invite(pauline->lc,"marie");
	linphone_call_ref(out_call);
	CU_ASSERT_TRUE_FATAL(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallIncomingReceived,1));

	linphone_core_terminate_call(pauline->lc,out_call);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	//CU_ASSERT_EQUAL(linphone_call_get_reason(in_call),LinphoneReasonDeclined);
	//CU_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonDeclined);
	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_early_declined() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");

	LinphoneCall* out_call = linphone_core_invite(pauline->lc,"marie");
	linphone_call_ref(out_call);
	CU_ASSERT_TRUE_FATAL(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallIncomingReceived,1));
	LinphoneCall* in_call=linphone_core_get_current_call(marie->lc);
	linphone_call_ref(in_call);

	linphone_core_terminate_call(marie->lc,in_call);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_EQUAL(linphone_call_get_reason(in_call),LinphoneReasonDeclined);
	CU_ASSERT_EQUAL(linphone_call_get_reason(out_call),LinphoneReasonDeclined);
	linphone_call_unref(in_call);
	linphone_call_unref(out_call);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_terminated_by_caller() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");

	CU_ASSERT_TRUE(call(pauline,marie));
	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_paused_resumed() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	LinphoneCall* call_obj;

	CU_ASSERT_TRUE(call(pauline,marie));
	call_obj = linphone_core_get_current_call(pauline->lc);

	linphone_core_pause_call(pauline->lc,call_obj);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallPaused,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallPausedByRemote,1));

	linphone_core_resume_call(pauline->lc,call_obj);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallStreamsRunning,2));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallStreamsRunning,2));

	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void call_srtp() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");

	linphone_core_set_media_encryption(marie->lc,LinphoneMediaEncryptionSRTP);
	linphone_core_set_media_encryption(pauline->lc,LinphoneMediaEncryptionSRTP);

	CU_ASSERT_TRUE(call(pauline,marie));

	CU_ASSERT_EQUAL(linphone_core_get_media_encryption(marie->lc),LinphoneMediaEncryptionSRTP);
	CU_ASSERT_EQUAL(linphone_core_get_media_encryption(pauline->lc),LinphoneMediaEncryptionSRTP);

	/*just to sleep*/
	linphone_core_terminate_all_calls(marie->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
static void text_message() {
	LinphoneCoreManager* marie = linphone_core_manager_new("./tester/marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new("./tester/pauline_rc");
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
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
	if (NULL == CU_add_test(pSuite, "call_early_declined", call_early_declined)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "call_canceled", call_canceled)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "call_ringing_canceled", call_ringing_canceled)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "simple_call", simple_call)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "call_terminated_by_caller", call_terminated_by_caller)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "call_paused_resumed", call_paused_resumed)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "call_srtp", call_srtp)) {
			return CU_get_error();
	}
	if (NULL == CU_add_test(pSuite, "text_message", text_message)) {
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
