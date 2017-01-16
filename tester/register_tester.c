/*
	belle-sip - SIP (RFC3261) library.
    Copyright (C) 2010  Belledonne Communications SARL

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


static void auth_info_requested(LinphoneCore *lc, const char *realm, const char *username, const char *domain) {
	LinphoneAuthInfo *info;
	info=linphone_auth_info_new(test_username,NULL,test_password,NULL,realm,domain);
	linphone_core_add_auth_info(lc,info);
	linphone_auth_info_destroy(info);
}

static void authentication_requested(LinphoneCore *lc, LinphoneAuthInfo *auth_info, LinphoneAuthMethod method) {
	linphone_auth_info_set_passwd(auth_info, test_password);
	linphone_core_add_auth_info(lc, auth_info); /*add authentication info to LinphoneCore*/
}

static LinphoneCoreManager* create_lcm_with_auth(unsigned int with_auth) {
	LinphoneCoreManager* lcm = linphone_core_manager_new(NULL);

	if (with_auth) {
		LinphoneCoreVTable* vtable = linphone_core_v_table_new();
		vtable->authentication_requested = authentication_requested;
		linphone_core_add_listener(lcm->lc, vtable);
	}

	/*to allow testing with 127.0.0.1*/
	linphone_core_set_network_reachable(lcm->lc, TRUE);
	return lcm;
}

static LinphoneCoreManager* create_lcm(void) {
	return create_lcm_with_auth(0);
}

void registration_state_changed(struct _LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message){
		stats* counters;
		ms_message("New registration state %s for user id [%s] at proxy [%s]\n"
				,linphone_registration_state_to_string(cstate)
				,linphone_proxy_config_get_identity(cfg)
				,linphone_proxy_config_get_addr(cfg));
		counters = get_stats(lc);
		switch (cstate) {
		case LinphoneRegistrationNone:counters->number_of_LinphoneRegistrationNone++;break;
		case LinphoneRegistrationProgress:counters->number_of_LinphoneRegistrationProgress++;break;
		case LinphoneRegistrationOk:counters->number_of_LinphoneRegistrationOk++;break;
		case LinphoneRegistrationCleared:counters->number_of_LinphoneRegistrationCleared++;break;
		case LinphoneRegistrationFailed:counters->number_of_LinphoneRegistrationFailed++;break;
		default:
			BC_FAIL("unexpected event");break;
		}

}

static void register_with_refresh_base_3(LinphoneCore* lc
											, bool_t refresh
											,const char* domain
											,const char* route
											,bool_t late_auth_info
											,LCSipTransports transport
											,LinphoneRegistrationState expected_final_state) {
	int retry=0;
	char* addr;
	LinphoneProxyConfig* proxy_cfg;
	stats* counters;
	LinphoneAddress *from;
	const char* server_addr;
	LinphoneAuthInfo *info;

	BC_ASSERT_PTR_NOT_NULL(lc);
	if (!lc) return;

	counters = get_stats(lc);
	reset_counters(counters);
	linphone_core_set_sip_transports(lc,&transport);

	proxy_cfg = linphone_proxy_config_new();

	from = create_linphone_address(domain);

	linphone_proxy_config_set_identity(proxy_cfg,addr=linphone_address_as_string(from));
	ms_free(addr);
	server_addr = linphone_address_get_domain(from);

	linphone_proxy_config_enable_register(proxy_cfg,TRUE);
	linphone_proxy_config_set_expires(proxy_cfg,1);
	if (route) {
		linphone_proxy_config_set_route(proxy_cfg,route);
		linphone_proxy_config_set_server_addr(proxy_cfg,route);
	} else {
		linphone_proxy_config_set_server_addr(proxy_cfg,server_addr);
	}
	linphone_address_unref(from);

	linphone_core_add_proxy_config(lc,proxy_cfg);
	linphone_core_set_default_proxy(lc,proxy_cfg);

	while (counters->number_of_LinphoneRegistrationOk<1+(refresh!=0)
			&& retry++ <(1100 /*only wait 11 s if final state is progress*/+(expected_final_state==LinphoneRegistrationProgress?0:2000))) {
		linphone_core_iterate(lc);
		if (counters->number_of_auth_info_requested>0 && linphone_proxy_config_get_state(proxy_cfg) == LinphoneRegistrationFailed && late_auth_info) {
			if (!linphone_core_get_auth_info_list(lc)) {
				BC_ASSERT_EQUAL(linphone_proxy_config_get_error(proxy_cfg),LinphoneReasonUnauthorized, int, "%d");
				info=linphone_auth_info_new(test_username,NULL,test_password,NULL,auth_domain,NULL); /*create authentication structure from identity*/
				linphone_core_add_auth_info(lc,info); /*add authentication info to LinphoneCore*/
				linphone_auth_info_destroy(info);
			}
		}
		if (linphone_proxy_config_get_error(proxy_cfg) == LinphoneReasonBadCredentials
				|| (counters->number_of_auth_info_requested>2 &&linphone_proxy_config_get_error(proxy_cfg) == LinphoneReasonUnauthorized)) /*no need to continue if auth cannot be found*/
			break; /*no need to continue*/
		ms_usleep(10000);
	}

	BC_ASSERT_EQUAL(linphone_proxy_config_is_registered(proxy_cfg), expected_final_state == LinphoneRegistrationOk, int, "%d");
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationNone,0, int, "%d");
	BC_ASSERT_TRUE(counters->number_of_LinphoneRegistrationProgress>=1);
	if (expected_final_state == LinphoneRegistrationOk) {
		BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationOk,1+(refresh!=0), int, "%d");
		BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationFailed,late_auth_info?1:0, int, "%d");
	} else
		/*checking to be done outside this functions*/
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationCleared,0, int, "%d");
	linphone_proxy_config_unref(proxy_cfg);
}

static void register_with_refresh_base_2(LinphoneCore* lc
											, bool_t refresh
											,const char* domain
											,const char* route
											,bool_t late_auth_info
											,LCSipTransports transport) {
	register_with_refresh_base_3(lc, refresh, domain, route, late_auth_info, transport,LinphoneRegistrationOk );
}
static void register_with_refresh_base(LinphoneCore* lc, bool_t refresh,const char* domain,const char* route) {
	LCSipTransports transport = {5070,5070,0,5071};
	register_with_refresh_base_2(lc,refresh,domain,route,FALSE,transport);
}

static void register_with_refresh(LinphoneCoreManager* lcm, bool_t refresh,const char* domain,const char* route) {
	stats* counters = &lcm->stat;
	register_with_refresh_base(lcm->lc,refresh,domain,route);
	linphone_core_manager_stop(lcm);
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationCleared,1, int, "%d");
}

static void register_with_refresh_with_send_error(void) {
	int retry=0;
	LinphoneCoreManager* lcm = create_lcm_with_auth(1);
	stats* counters = &lcm->stat;
	LinphoneAuthInfo *info=linphone_auth_info_new(test_username,NULL,test_password,NULL,auth_domain,NULL); /*create authentication structure from identity*/
	char route[256];
	sprintf(route,"sip:%s",test_route);
	linphone_core_add_auth_info(lcm->lc,info); /*add authentication info to LinphoneCore*/
	linphone_auth_info_destroy(info);
	register_with_refresh_base(lcm->lc,TRUE,auth_domain,route);
	/*simultate a network error*/
	sal_set_send_error(lcm->lc->sal, -1);
	while (counters->number_of_LinphoneRegistrationProgress<2 && retry++ <200) {
			linphone_core_iterate(lcm->lc);
			ms_usleep(10000);
	}
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationFailed,0, int, "%d");
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationProgress,2, int, "%d");

	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationCleared,0, int, "%d");

	linphone_core_manager_destroy(lcm);
}

static void simple_register(void){
	LinphoneCoreManager* lcm = create_lcm();
	stats* counters = &lcm->stat;
	register_with_refresh(lcm,FALSE,NULL,NULL);
	BC_ASSERT_EQUAL(counters->number_of_auth_info_requested,0, int, "%d");
	linphone_core_manager_destroy(lcm);
}

static void register_with_custom_headers(void){
	LinphoneCoreManager *marie=linphone_core_manager_new("marie_rc");
	LinphoneProxyConfig *cfg=linphone_core_get_default_proxy_config(marie->lc);
	int initial_register_ok=marie->stat.number_of_LinphoneRegistrationOk;
	const char *value;

	linphone_core_set_network_reachable(marie->lc, FALSE);
	linphone_proxy_config_set_custom_header(cfg, "ah-bah-ouais", "...mais bon.");
	/*unfortunately it is difficult to programmatically check that sent custom headers are actually sent.
	 * A server development would be required here.*/

	linphone_core_set_network_reachable(marie->lc, TRUE);
	wait_for(marie->lc, NULL, &marie->stat.number_of_LinphoneRegistrationOk,initial_register_ok+1);
	value=linphone_proxy_config_get_custom_header(cfg, "Server");
	BC_ASSERT_PTR_NOT_NULL(value);
	if (value) BC_ASSERT_PTR_NOT_NULL(strstr(value, "Flexisip"));
	linphone_core_manager_destroy(marie);
}

static void simple_unregister(void){
	LinphoneCoreManager* lcm = create_lcm();
	stats* counters = &lcm->stat;
	LinphoneProxyConfig* proxy_config;
	register_with_refresh_base(lcm->lc,FALSE,NULL,NULL);

	proxy_config = linphone_core_get_default_proxy_config(lcm->lc);

	linphone_proxy_config_edit(proxy_config);
	reset_counters(counters); /*clear stats*/

	/*nothing is supposed to arrive until done*/
	BC_ASSERT_FALSE(wait_for_until(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationCleared,1,3000));
	linphone_proxy_config_enable_register(proxy_config,FALSE);
	linphone_proxy_config_done(proxy_config);
	BC_ASSERT_TRUE(wait_for(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationCleared,1));
	linphone_core_manager_destroy(lcm);
}

static void change_expires(void){
	LinphoneCoreManager* lcm = create_lcm();
	stats* counters = &lcm->stat;
	LinphoneProxyConfig* proxy_config;
	register_with_refresh_base(lcm->lc,FALSE,NULL,NULL);

	proxy_config = linphone_core_get_default_proxy_config(lcm->lc);

	linphone_proxy_config_edit(proxy_config);

	/*nothing is supposed to arrive until done*/
	BC_ASSERT_FALSE(wait_for_until(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationCleared,1,3000));

	linphone_proxy_config_set_expires(proxy_config,3);
	reset_counters(counters); /*clear stats*/
	linphone_proxy_config_done(proxy_config);
	BC_ASSERT_TRUE(wait_for(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationOk,1));
	/*wait 2s without receive refresh*/
	BC_ASSERT_FALSE(wait_for_until(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationOk,2,2000));
	/* now, it should be ok*/
	BC_ASSERT_TRUE(wait_for(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationOk,2));
	linphone_core_manager_destroy(lcm);
}

/*take care of min expires configuration from server*/
static void simple_register_with_refresh(void) {
	LinphoneCoreManager* lcm = create_lcm();
	stats* counters = &lcm->stat;
	register_with_refresh(lcm,TRUE,NULL,NULL);
	BC_ASSERT_EQUAL(counters->number_of_auth_info_requested,0, int, "%d");
	linphone_core_manager_destroy(lcm);
}

static void simple_auth_register_with_refresh(void) {
	LinphoneCoreManager* lcm = create_lcm_with_auth(1);
	stats* counters = &lcm->stat;
	char route[256];
	sprintf(route,"sip:%s",test_route);
	register_with_refresh(lcm,TRUE,auth_domain,route);
	BC_ASSERT_EQUAL(counters->number_of_auth_info_requested,1, int, "%d");
	linphone_core_manager_destroy(lcm);
}

static void simple_tcp_register(void){
	char route[256];
	LinphoneCoreManager* lcm;
	sprintf(route,"sip:%s;transport=tcp",test_route);
	lcm = create_lcm();
	register_with_refresh(lcm,FALSE,test_domain,route);
	linphone_core_manager_destroy(lcm);
}

static void simple_tcp_register_compatibility_mode(void){
	char route[256];
	LinphoneCoreManager* lcm;
	LCSipTransports transport = {0,5070,0,0};
	sprintf(route,"sip:%s",test_route);
	lcm = create_lcm();
	register_with_refresh_base_2(lcm->lc,FALSE,test_domain,route,FALSE,transport);
	linphone_core_manager_destroy(lcm);
}

static void simple_tls_register(void){
	if (transport_supported(LinphoneTransportTls)) {
		char route[256];
		LinphoneCoreManager* lcm = create_lcm();
		sprintf(route,"sip:%s;transport=tls",test_route);
		register_with_refresh(lcm,FALSE,test_domain,route);
		linphone_core_manager_destroy(lcm);
	}
}


static void simple_authenticated_register(void){
	stats* counters;
	LinphoneCoreManager* lcm = create_lcm();
	LinphoneAuthInfo *info=linphone_auth_info_new(test_username,NULL,test_password,NULL,auth_domain,NULL); /*create authentication structure from identity*/
	char route[256];
	sprintf(route,"sip:%s",test_route);
	linphone_core_add_auth_info(lcm->lc,info); /*add authentication info to LinphoneCore*/
	linphone_auth_info_destroy(info);
	counters = &lcm->stat;
	register_with_refresh(lcm,FALSE,auth_domain,route);
	BC_ASSERT_EQUAL(counters->number_of_auth_info_requested,0, int, "%d");
	linphone_core_manager_destroy(lcm);
}

static void ha1_authenticated_register(void){
	stats* counters;
	LinphoneCoreManager* lcm = create_lcm();
	char ha1[33];
	LinphoneAuthInfo *info;
	char route[256];
	sal_auth_compute_ha1(test_username,auth_domain,test_password,ha1);
	info=linphone_auth_info_new(test_username,NULL,NULL,ha1,auth_domain,NULL); /*create authentication structure from identity*/
	sprintf(route,"sip:%s",test_route);
	linphone_core_add_auth_info(lcm->lc,info); /*add authentication info to LinphoneCore*/
	linphone_auth_info_destroy(info);
	counters = &lcm->stat;
	register_with_refresh(lcm,FALSE,auth_domain,route);
	BC_ASSERT_EQUAL(counters->number_of_auth_info_requested,0, int, "%d");
	linphone_core_manager_destroy(lcm);
}

static void authenticated_register_with_no_initial_credentials(void){
	LinphoneCoreManager *lcm;
	LinphoneCoreVTable* vtable = linphone_core_v_table_new();
	stats* counters;
	char route[256];

	sprintf(route,"sip:%s",test_route);

	lcm = linphone_core_manager_new(NULL);

	vtable->auth_info_requested=auth_info_requested;
	linphone_core_add_listener(lcm->lc,vtable);

	counters= get_stats(lcm->lc);
	counters->number_of_auth_info_requested=0;
	register_with_refresh(lcm,FALSE,auth_domain,route);
	BC_ASSERT_EQUAL(counters->number_of_auth_info_requested,1, int, "%d");
	linphone_core_manager_destroy(lcm);
}


static void authenticated_register_with_late_credentials(void){
	LinphoneCoreManager *lcm;
	stats* counters;
	char route[256];
	LCSipTransports transport = {5070,5070,0,5071};

	sprintf(route,"sip:%s",test_route);

	lcm =  linphone_core_manager_new(NULL);

	counters = get_stats(lcm->lc);
	register_with_refresh_base_2(lcm->lc,FALSE,auth_domain,route,TRUE,transport);
	BC_ASSERT_EQUAL(counters->number_of_auth_info_requested,1, int, "%d");
	linphone_core_manager_destroy(lcm);
}

static void authenticated_register_with_provided_credentials(void){
	LinphoneCoreManager *lcm;
	stats* counters;
	LinphoneProxyConfig *cfg;
	char route[256];
	LinphoneAddress *from;
	char *addr;
	LinphoneAuthInfo *ai;

	sprintf(route,"sip:%s",test_route);

	lcm =  linphone_core_manager_new(NULL);

	counters = get_stats(lcm->lc);
	cfg = linphone_core_create_proxy_config(lcm->lc);
	from = create_linphone_address(auth_domain);

	linphone_proxy_config_set_identity(cfg, addr=linphone_address_as_string(from));
	ms_free(addr);

	linphone_proxy_config_enable_register(cfg,TRUE);
	linphone_proxy_config_set_expires(cfg,1);
	linphone_proxy_config_set_route(cfg, test_route);
	linphone_proxy_config_set_server_addr(cfg,test_route);
	linphone_address_unref(from);

	ai = linphone_auth_info_new(test_username, NULL, test_password, NULL, NULL, NULL);
	linphone_core_add_auth_info(lcm->lc, ai);
	linphone_auth_info_destroy(ai);
	linphone_core_add_proxy_config(lcm->lc, cfg);

	BC_ASSERT_TRUE(wait_for(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationOk,1));
	BC_ASSERT_EQUAL(counters->number_of_auth_info_requested,0, int, "%d");

	BC_ASSERT_PTR_NULL(lp_config_get_string(lcm->lc->config, "auth_info_0", "passwd", NULL));
	BC_ASSERT_PTR_NOT_NULL(lp_config_get_string(lcm->lc->config, "auth_info_0", "ha1", NULL));

	linphone_proxy_config_destroy(cfg);
	linphone_core_manager_destroy(lcm);
}

static void authenticated_register_with_wrong_late_credentials(void){
	LinphoneCoreManager *lcm;
	stats* counters;
	LCSipTransports transport = {5070,5070,0,5071};
	char route[256];
	const char* saved_test_passwd=test_password;
	char* wrong_passwd="mot de pass tout pourri";

	test_password=wrong_passwd;

	sprintf(route,"sip:%s",test_route);

	lcm =  linphone_core_manager_new(NULL);

	counters = get_stats(lcm->lc);
	register_with_refresh_base_3(lcm->lc,FALSE,auth_domain,route,TRUE,transport,LinphoneRegistrationFailed);
	BC_ASSERT_EQUAL(counters->number_of_auth_info_requested,2, int, "%d");
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationFailed,2, int, "%d");
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationProgress,2, int, "%d");
	test_password=saved_test_passwd;

	linphone_core_manager_destroy(lcm);
}

static void authenticated_register_with_wrong_credentials_with_params_base(const char* user_agent,LinphoneCoreManager *lcm) {
	stats* counters;
	LCSipTransports transport = {5070,5070,0,5071};
	LinphoneAuthInfo *info=linphone_auth_info_new(test_username,NULL,"wrong passwd",NULL,auth_domain,NULL); /*create authentication structure from identity*/
	char route[256];

	sprintf(route,"sip:%s",test_route);

	sal_set_refresher_retry_after(lcm->lc->sal,500);
	if (user_agent) {
		linphone_core_set_user_agent(lcm->lc,user_agent,NULL);
	}
	linphone_core_add_auth_info(lcm->lc,info); /*add wrong authentication info to LinphoneCore*/
	linphone_auth_info_destroy(info);
	counters = get_stats(lcm->lc);
	register_with_refresh_base_3(lcm->lc,TRUE,auth_domain,route,FALSE,transport,LinphoneRegistrationFailed);
	//BC_ASSERT_EQUAL(counters->number_of_auth_info_requested,3, int, "%d"); register_with_refresh_base_3 does not alow to precisely check number of number_of_auth_info_requested
	/*wait for retry*/
	BC_ASSERT_TRUE(wait_for(lcm->lc,lcm->lc,&counters->number_of_auth_info_requested,4));
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationFailed,1, int, "%d");

	/*check the detailed error info */
	if (!user_agent || strcmp(user_agent,"tester-no-403")!=0){
		LinphoneProxyConfig *cfg=NULL;
		cfg = linphone_core_get_default_proxy_config(lcm->lc);
		BC_ASSERT_PTR_NOT_NULL(cfg);
		if (cfg){
			const LinphoneErrorInfo *ei=linphone_proxy_config_get_error_info(cfg);
			const char *phrase=linphone_error_info_get_phrase(ei);
			BC_ASSERT_PTR_NOT_NULL(phrase);
			if (phrase) BC_ASSERT_STRING_EQUAL(phrase,"Forbidden");
			BC_ASSERT_EQUAL(linphone_error_info_get_protocol_code(ei),403, int, "%d");
			BC_ASSERT_PTR_NULL(linphone_error_info_get_details(ei));
		}

	}
	}
static void authenticated_register_with_wrong_credentials_with_params(const char* user_agent) {
	LinphoneCoreManager *lcm = linphone_core_manager_new(NULL);
	authenticated_register_with_wrong_credentials_with_params_base(user_agent,lcm);
	linphone_core_manager_destroy(lcm);
}
static void authenticated_register_with_wrong_credentials(void) {
	authenticated_register_with_wrong_credentials_with_params(NULL);
}
static void authenticated_register_with_wrong_credentials_2(void) {
	LinphoneCoreManager *lcm = linphone_core_manager_new(NULL);
	stats* counters = get_stats(lcm->lc);
	int current_in_progress;
	LinphoneProxyConfig* proxy;

	authenticated_register_with_wrong_credentials_with_params_base(NULL,lcm);

	proxy = linphone_core_get_default_proxy_config(lcm->lc);
	/*Make sure registration attempts are stopped*/
	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_enable_register(proxy,FALSE);
	linphone_proxy_config_done(proxy);
	current_in_progress=counters->number_of_LinphoneRegistrationProgress;
	BC_ASSERT_FALSE(wait_for(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationProgress,current_in_progress+1));

	linphone_core_manager_destroy(lcm);
}
static void authenticated_register_with_wrong_credentials_without_403(void) {
	authenticated_register_with_wrong_credentials_with_params("tester-no-403");
}
static LinphoneCoreManager* configure_lcm(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager *lcm=linphone_core_manager_new2( "multi_account_rc", FALSE);
		stats *counters=&lcm->stat;
		BC_ASSERT_TRUE(wait_for(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationOk,(int)bctbx_list_size(linphone_core_get_proxy_config_list(lcm->lc))));
		BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationFailed,0, int, "%d");
		return lcm;
	}
	return NULL;
}

static void multiple_proxy(void){
	LinphoneCoreManager *lcm=configure_lcm();
	if (lcm) {
		linphone_core_manager_destroy(lcm);
	}
}

static void network_state_change(void){
	int register_ok;
	stats *counters;
	LinphoneCoreManager *lcm=configure_lcm();
	if (lcm) {
		LinphoneCore *lc=lcm->lc;

		counters = get_stats(lc);
		register_ok=counters->number_of_LinphoneRegistrationOk;
		linphone_core_set_network_reachable(lc,FALSE);
		BC_ASSERT_TRUE(wait_for(lc,lc,&counters->number_of_NetworkReachableFalse,1));
		BC_ASSERT_TRUE(wait_for(lc,lc,&counters->number_of_LinphoneRegistrationNone,register_ok));
		linphone_core_set_network_reachable(lc,TRUE);
		BC_ASSERT_TRUE(wait_for(lc,lc,&counters->number_of_NetworkReachableTrue,1));
		wait_for(lc,lc,&counters->number_of_LinphoneRegistrationOk,2*register_ok);

		linphone_core_manager_destroy(lcm);
	}
}
static int get_number_of_udp_proxy(const LinphoneCore* lc) {
	int number_of_udp_proxy=0;
	LinphoneProxyConfig* proxy_cfg;
	const bctbx_list_t* proxys;
	for (proxys=linphone_core_get_proxy_config_list(lc);proxys!=NULL;proxys=proxys->next) {
			proxy_cfg=(LinphoneProxyConfig*)proxys->data;
			if (strcmp("udp",linphone_proxy_config_get_transport(proxy_cfg))==0)
				number_of_udp_proxy++;
	}
	return number_of_udp_proxy;
}
static void transport_change(void){
	LinphoneCoreManager *lcm;
	LinphoneCore* lc;
	int register_ok;
	stats* counters ;
	LCSipTransports sip_tr;
	LCSipTransports sip_tr_orig;
	int number_of_udp_proxy=0;
	int total_number_of_proxies;

	lcm=configure_lcm();
	if (lcm) {
		memset(&sip_tr,0,sizeof(sip_tr));
		lc=lcm->lc;
		counters = get_stats(lc);
		register_ok=counters->number_of_LinphoneRegistrationOk;

		number_of_udp_proxy=get_number_of_udp_proxy(lc);
		total_number_of_proxies=(int)bctbx_list_size(linphone_core_get_proxy_config_list(lc));
		linphone_core_get_sip_transports(lc,&sip_tr_orig);

		sip_tr.udp_port=sip_tr_orig.udp_port;

		/*keep only udp*/
		linphone_core_set_sip_transports(lc,&sip_tr);
		BC_ASSERT_TRUE(wait_for(lc,lc,&counters->number_of_LinphoneRegistrationOk,register_ok+number_of_udp_proxy));

		BC_ASSERT_TRUE(wait_for(lc,lc,&counters->number_of_LinphoneRegistrationFailed,total_number_of_proxies-number_of_udp_proxy));

		linphone_core_manager_destroy(lcm);
	}
}

static void transport_dont_bind(void){
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_tcp_rc");
	stats* counters = &pauline->stat;
	LCSipTransports tr;
	
	memset(&tr, 0, sizeof(tr));
	tr.udp_port = 0;
	tr.tcp_port = LC_SIP_TRANSPORT_DONTBIND;
	tr.tls_port = LC_SIP_TRANSPORT_DONTBIND;
	
	linphone_core_set_sip_transports(pauline->lc, &tr);
	BC_ASSERT_TRUE(wait_for_until(pauline->lc,pauline->lc,&counters->number_of_LinphoneRegistrationOk,2,15000));
	memset(&tr, 0, sizeof(tr));
	linphone_core_get_sip_transports_used(pauline->lc, &tr);
	BC_ASSERT_EQUAL(tr.udp_port, 0, int, "%i");
	BC_ASSERT_EQUAL(tr.tcp_port, LC_SIP_TRANSPORT_DONTBIND, int, "%i");
	BC_ASSERT_EQUAL(tr.tls_port, LC_SIP_TRANSPORT_DONTBIND, int, "%i");
	linphone_core_manager_destroy(pauline);
}

static void transport_busy(void){
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_tcp_rc");
	LCSipTransports tr;
	
	memset(&tr, 0, sizeof(tr));
	tr.udp_port = 5070;
	tr.tcp_port = 5070;
	tr.tls_port = 5071;
	
	linphone_core_set_sip_transports(pauline->lc, &tr);
	
	{
		LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
		linphone_core_set_sip_transports(marie->lc, &tr);
		memset(&tr, 0, sizeof(tr));
		linphone_core_get_sip_transports_used(pauline->lc, &tr);
		/*BC_ASSERT_EQUAL(tr.udp_port, 0, int, "%i");
		BC_ASSERT_EQUAL(tr.tcp_port, 0, int, "%i");
		BC_ASSERT_EQUAL(tr.tls_port, 0, int, "%i");*/
		linphone_core_manager_destroy(marie);
	}
	
	linphone_core_manager_destroy(pauline);
}

static void proxy_transport_change(void){
	LinphoneCoreManager* lcm = create_lcm();
	stats* counters = &lcm->stat;
	LinphoneProxyConfig* proxy_config;
	LinphoneAddress* addr;
	char* addr_as_string;
	LinphoneAuthInfo *info=linphone_auth_info_new(test_username,NULL,test_password,NULL,auth_domain,NULL); /*create authentication structure from identity*/
	linphone_core_add_auth_info(lcm->lc,info); /*add authentication info to LinphoneCore*/
	linphone_auth_info_destroy(info);
	register_with_refresh_base(lcm->lc,FALSE,auth_domain,NULL);

	proxy_config = linphone_core_get_default_proxy_config(lcm->lc);
	reset_counters(counters); /*clear stats*/
	linphone_proxy_config_edit(proxy_config);

	BC_ASSERT_FALSE(wait_for_until(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationCleared,1,3000));
	addr = linphone_address_new(linphone_proxy_config_get_addr(proxy_config));

	if (LinphoneTransportTcp == linphone_address_get_transport(addr)) {
		linphone_address_set_transport(addr,LinphoneTransportUdp);
	} else {
		linphone_address_set_transport(addr,LinphoneTransportTcp);
	}
	linphone_proxy_config_set_server_addr(proxy_config,addr_as_string=linphone_address_as_string(addr));

	linphone_proxy_config_done(proxy_config);

	BC_ASSERT(wait_for(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationOk,1));
	/*as we change p[roxy server destination, we should'nt be notified about the clear*/
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationCleared,0, int, "%d");
	ms_free(addr_as_string);
	linphone_address_unref(addr);
	linphone_core_manager_destroy(lcm);

}
static void proxy_transport_change_with_wrong_port(void) {
	LinphoneCoreManager* lcm = create_lcm();
	stats* counters = &lcm->stat;
	LinphoneProxyConfig* proxy_config;
	LinphoneAuthInfo *info=linphone_auth_info_new(test_username,NULL,test_password,NULL,auth_domain,NULL); /*create authentication structure from identity*/
	char route[256];
	LCSipTransports transport= {LC_SIP_TRANSPORT_RANDOM,LC_SIP_TRANSPORT_RANDOM,LC_SIP_TRANSPORT_RANDOM,LC_SIP_TRANSPORT_RANDOM};
	sprintf(route,"sip:%s",test_route);

	linphone_core_add_auth_info(lcm->lc,info); /*add authentication info to LinphoneCore*/
	linphone_auth_info_destroy(info);
	register_with_refresh_base_3(lcm->lc, FALSE, auth_domain, "sip2.linphone.org:5987", 0,transport,LinphoneRegistrationProgress);

	proxy_config = linphone_core_get_default_proxy_config(lcm->lc);
	linphone_proxy_config_edit(proxy_config);

	BC_ASSERT_FALSE(wait_for_until(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationCleared,1,3000));
	linphone_proxy_config_set_server_addr(proxy_config,route);
	linphone_proxy_config_done(proxy_config);

	BC_ASSERT(wait_for(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationOk,1));
	/*as we change proxy server destination, we should'nt be notified about the clear*/
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationCleared,0, int, "%d");
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationOk,1, int, "%d");
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationProgress,1, int, "%d");
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationFailed,0, int, "%d");

	linphone_core_manager_destroy(lcm);

}

static void proxy_transport_change_with_wrong_port_givin_up(void) {
	LinphoneCoreManager* lcm = create_lcm();
	stats* counters = &lcm->stat;
	LinphoneProxyConfig* proxy_config;
	LinphoneAuthInfo *info=linphone_auth_info_new(test_username,NULL,test_password,NULL,auth_domain,NULL); /*create authentication structure from identity*/
	char route[256];
	LCSipTransports transport= {LC_SIP_TRANSPORT_RANDOM,LC_SIP_TRANSPORT_RANDOM,LC_SIP_TRANSPORT_RANDOM,LC_SIP_TRANSPORT_RANDOM};
	sprintf(route,"sip:%s",test_route);

	linphone_core_add_auth_info(lcm->lc,info); /*add authentication info to LinphoneCore*/
	linphone_auth_info_destroy(info);
	register_with_refresh_base_3(lcm->lc, FALSE, auth_domain, "sip2.linphone.org:5987", 0,transport,LinphoneRegistrationProgress);

	proxy_config = linphone_core_get_default_proxy_config(lcm->lc);
	linphone_proxy_config_edit(proxy_config);

	BC_ASSERT_FALSE(wait_for_until(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationCleared,1,3000));
	linphone_proxy_config_enableregister(proxy_config,FALSE);
	linphone_proxy_config_done(proxy_config);

	BC_ASSERT(wait_for(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationCleared,1));
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationOk,0, int, "%d");
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationProgress,1, int, "%d");
	BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationFailed,0, int, "%d");

	linphone_core_manager_destroy(lcm);

}

static void io_recv_error(void){
	LinphoneCoreManager *lcm;
	LinphoneCore* lc;
	int register_ok;
	stats* counters ;
	int number_of_udp_proxy=0;


	lcm=configure_lcm();
	if (lcm) {
		lc=lcm->lc;
		counters = get_stats(lc);
		register_ok=counters->number_of_LinphoneRegistrationOk;
		number_of_udp_proxy=get_number_of_udp_proxy(lc);
		sal_set_recv_error(lc->sal, 0);

		BC_ASSERT_TRUE(wait_for(lc,lc,&counters->number_of_LinphoneRegistrationProgress,2*(register_ok-number_of_udp_proxy) /*because 1 udp*/));
		BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationFailed,0,int,"%d");

		sal_set_recv_error(lc->sal, 1); /*reset*/

		linphone_core_manager_destroy(lcm);
	}
}

static void io_recv_error_retry_immediatly(void){
	LinphoneCoreManager *lcm;
	LinphoneCore* lc;
	int register_ok;
	stats* counters ;
	int number_of_udp_proxy=0;

	lcm=configure_lcm();
	if (lcm) {
		lc=lcm->lc;
		counters = get_stats(lc);
		register_ok=counters->number_of_LinphoneRegistrationOk;
		number_of_udp_proxy=get_number_of_udp_proxy(lc);
		sal_set_recv_error(lc->sal, 0);

		BC_ASSERT_TRUE(wait_for(lc,NULL,&counters->number_of_LinphoneRegistrationProgress,(register_ok-number_of_udp_proxy)+register_ok /*because 1 udp*/));
		BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationFailed,0,int,"%d");
		sal_set_recv_error(lc->sal, 1); /*reset*/

		BC_ASSERT_TRUE(wait_for_until(lc,lc,&counters->number_of_LinphoneRegistrationOk,register_ok-number_of_udp_proxy+register_ok,30000));

		linphone_core_manager_destroy(lcm);
	}
}

static void io_recv_error_late_recovery(void){
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager *lcm;
		LinphoneCore* lc;
		int register_ok;
		stats* counters ;
		int number_of_udp_proxy=0;
		bctbx_list_t* lcs;
		lcm=linphone_core_manager_new2( "multi_account_rc",FALSE); /*to make sure iterates are not call yet*/
		lc=lcm->lc;
		sal_set_refresher_retry_after(lc->sal,1000);
		counters=&lcm->stat;
		BC_ASSERT_TRUE(wait_for(lcm->lc,lcm->lc,&counters->number_of_LinphoneRegistrationOk,(int)bctbx_list_size(linphone_core_get_proxy_config_list(lcm->lc))));


		counters = get_stats(lc);
		register_ok=counters->number_of_LinphoneRegistrationOk;
		number_of_udp_proxy=get_number_of_udp_proxy(lc);
		/*simulate a general socket error*/
		sal_set_recv_error(lc->sal, 0);
		sal_set_send_error(lc->sal, -1);

		BC_ASSERT_TRUE(wait_for(lc,NULL,&counters->number_of_LinphoneRegistrationProgress,(register_ok-number_of_udp_proxy)+register_ok /*because 1 udp*/));
		BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationFailed,0,int,"%d");

		BC_ASSERT_TRUE(wait_for_list(lcs=bctbx_list_append(NULL,lc),&counters->number_of_LinphoneRegistrationFailed,(register_ok-number_of_udp_proxy),sal_get_refresher_retry_after(lc->sal)+3000));

		sal_set_recv_error(lc->sal, 1); /*reset*/
		sal_set_send_error(lc->sal, 0);

		BC_ASSERT_TRUE(wait_for_list(lcs=bctbx_list_append(NULL,lc),&counters->number_of_LinphoneRegistrationOk,register_ok-number_of_udp_proxy +register_ok,sal_get_refresher_retry_after(lc->sal)+3000));
		linphone_core_manager_destroy(lcm);
	}
}

static void io_recv_error_without_active_register(void){
	LinphoneCoreManager *lcm;
	LinphoneCore* lc;
	int register_ok;
	stats* counters ;
	bctbx_list_t* proxys;
	int dummy=0;

	lcm=configure_lcm();
	if (lcm) {
		lc=lcm->lc;
		counters = get_stats(lc);

		register_ok=counters->number_of_LinphoneRegistrationOk;

		for (proxys=bctbx_list_copy(linphone_core_get_proxy_config_list(lc));proxys!=NULL;proxys=proxys->next) {
			LinphoneProxyConfig* proxy_cfg=(LinphoneProxyConfig*)proxys->data;
			linphone_proxy_config_edit(proxy_cfg);
			linphone_proxy_config_enableregister(proxy_cfg,FALSE);
			linphone_proxy_config_done(proxy_cfg);
		}
		bctbx_list_free(proxys);
		/*wait for unregistrations*/
		BC_ASSERT_TRUE(wait_for(lc,lc,&counters->number_of_LinphoneRegistrationCleared,register_ok /*because 1 udp*/));

		sal_set_recv_error(lc->sal, 0);

		/*nothing should happen because no active registration*/
		wait_for_until(lc,lc, &dummy, 1, 3000);
		BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationProgress, (int)bctbx_list_size(linphone_core_get_proxy_config_list(lc)), int, "%d");

		BC_ASSERT_EQUAL(counters->number_of_LinphoneRegistrationFailed,0,int,"%d");

		sal_set_recv_error(lc->sal, 1); /*reset*/

		linphone_core_manager_destroy(lcm);
	}
}


static void tls_certificate_failure(void){
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* lcm;
		LinphoneCore *lc;
		char *rootcapath = bc_tester_res("certificates/cn/agent.pem"); /*bad root ca*/

		lcm=linphone_core_manager_new2("pauline_rc",FALSE);
		lc=lcm->lc;
		linphone_core_set_root_ca(lcm->lc,rootcapath);
		linphone_core_set_network_reachable(lc,TRUE);
		BC_ASSERT_TRUE(wait_for(lcm->lc,lcm->lc,&lcm->stat.number_of_LinphoneRegistrationFailed,1));
		linphone_core_set_root_ca(lcm->lc,NULL); /*no root ca*/
		linphone_core_refresh_registers(lcm->lc);
		BC_ASSERT_TRUE(wait_for(lc,lc,&lcm->stat.number_of_LinphoneRegistrationFailed,2));
		bc_free(rootcapath);
		rootcapath = bc_tester_res("certificates/cn/cafile.pem"); /*good root ca*/
		linphone_core_set_root_ca(lcm->lc,rootcapath);
		linphone_core_refresh_registers(lcm->lc);
		BC_ASSERT_TRUE(wait_for(lc,lc,&lcm->stat.number_of_LinphoneRegistrationOk,1));
		BC_ASSERT_EQUAL(lcm->stat.number_of_LinphoneRegistrationFailed,2, int, "%d");
		linphone_core_manager_destroy(lcm);
		bc_free(rootcapath);
	}
}

char *read_file(const char *path) {
	long  numbytes = 0;
	size_t readbytes;
	char *buffer = NULL;
	FILE *infile = fopen(path, "rb");
	
	BC_ASSERT_PTR_NOT_NULL(infile);
	if (infile) {
		fseek(infile, 0L, SEEK_END);
		numbytes = ftell(infile);
		fseek(infile, 0L, SEEK_SET);
		buffer = (char*)ms_malloc((numbytes + 1) * sizeof(char));
		readbytes = fread(buffer, sizeof(char), numbytes, infile);
		fclose(infile);
		buffer[readbytes] = '\0';
	}
	return buffer;
}

static void tls_certificate_data(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* lcm;
		LinphoneCore *lc;
		char *rootcapath = bc_tester_res("certificates/cn/agent.pem"); /*bad root ca*/
		char *data = read_file(rootcapath);

		lcm = linphone_core_manager_new2("pauline_rc",FALSE);
		lc = lcm->lc;
		linphone_core_set_root_ca_data(lcm->lc, data);
		linphone_core_set_network_reachable(lc, TRUE);
		BC_ASSERT_TRUE(wait_for(lcm->lc, lcm->lc, &lcm->stat.number_of_LinphoneRegistrationFailed, 1));
		linphone_core_set_root_ca_data(lcm->lc, NULL); /*no root ca*/
		linphone_core_refresh_registers(lcm->lc);
		BC_ASSERT_TRUE(wait_for(lc, lc, &lcm->stat.number_of_LinphoneRegistrationFailed, 2));
		bc_free(rootcapath);
		ms_free(data);
		rootcapath = bc_tester_res("certificates/cn/cafile.pem"); /*good root ca*/
		data = read_file(rootcapath);
		linphone_core_set_root_ca_data(lcm->lc, data);
		linphone_core_refresh_registers(lcm->lc);
		BC_ASSERT_TRUE(wait_for(lc, lc, &lcm->stat.number_of_LinphoneRegistrationOk, 1));
		BC_ASSERT_EQUAL(lcm->stat.number_of_LinphoneRegistrationFailed, 2, int, "%d");
		linphone_core_manager_destroy(lcm);
		bc_free(rootcapath);
		ms_free(data);
	}
}

/*the purpose of this test is to check that will not block the proxy config during SSL handshake for entire life in case of mistaken configuration*/
static void tls_with_non_tls_server(void){
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager *lcm;
		LinphoneProxyConfig* proxy_cfg;
		LinphoneAddress* addr;
		char tmp[256];
		LinphoneCore *lc;

		lcm=linphone_core_manager_new2( "marie_rc", 0);
		lc=lcm->lc;
		sal_set_transport_timeout(lc->sal,3000);
		proxy_cfg = linphone_core_get_default_proxy_config(lc);
		linphone_proxy_config_edit(proxy_cfg);
		addr=linphone_address_new(linphone_proxy_config_get_addr(proxy_cfg));
		snprintf(tmp,sizeof(tmp),"sip:%s:%i;transport=tls"	,linphone_address_get_domain(addr)
				,(linphone_address_get_port(addr)>0?linphone_address_get_port(addr):5060));
		linphone_proxy_config_set_server_addr(proxy_cfg,tmp);
		linphone_proxy_config_done(proxy_cfg);
		linphone_address_unref(addr);
		BC_ASSERT_TRUE(wait_for_until(lc,lc,&lcm->stat.number_of_LinphoneRegistrationFailed,1,10000));
		linphone_core_manager_destroy(lcm);
	}
}

static void tls_alt_name_register(void){
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* lcm;
		LinphoneCore *lc;
		char *rootcapath = bc_tester_res("certificates/cn/cafile.pem");

		lcm=linphone_core_manager_new2("pauline_alt_rc",FALSE);
		lc=lcm->lc;
		linphone_core_set_root_ca(lc,rootcapath);
		linphone_core_refresh_registers(lc);
		BC_ASSERT_TRUE(wait_for(lc,lc,&lcm->stat.number_of_LinphoneRegistrationOk,1));
		BC_ASSERT_EQUAL(lcm->stat.number_of_LinphoneRegistrationFailed,0, int, "%d");
		linphone_core_manager_destroy(lcm);
		bc_free(rootcapath);
	}
}

static void tls_wildcard_register(void){
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* lcm;
		LinphoneCore *lc;
		char *rootcapath = bc_tester_res("certificates/cn/cafile.pem");

		lcm=linphone_core_manager_new2("pauline_wild_rc",FALSE);
		lc=lcm->lc;
		linphone_core_set_root_ca(lc,rootcapath);
		linphone_core_refresh_registers(lc);
		BC_ASSERT_TRUE(wait_for(lc,lc,&lcm->stat.number_of_LinphoneRegistrationOk,2));
		BC_ASSERT_EQUAL(lcm->stat.number_of_LinphoneRegistrationFailed,0, int, "%d");
		linphone_core_manager_destroy(lcm);
		bc_free(rootcapath);
	}
}

static void redirect(void){
	char route[256];
	LinphoneCoreManager* lcm;
	LCSipTransports transport = {-1,0,0,0};
	sprintf(route,"sip:%s:5064",test_route);
	lcm = create_lcm();
	if (lcm) {
		linphone_core_set_user_agent(lcm->lc,"redirect",NULL);
		register_with_refresh_base_2(lcm->lc,FALSE,test_domain,route,FALSE,transport);
		linphone_core_manager_destroy(lcm);
	}
}

static void tls_auth_global_client_cert(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager *manager = ms_new0(LinphoneCoreManager, 1);
		LpConfig *lpc = NULL;
		char *cert_path = bc_tester_res("certificates/client/cert.pem");
		char *key_path = bc_tester_res("certificates/client/key.pem");
		linphone_core_manager_init(manager, "pauline_tls_client_rc", NULL);
		lpc = manager->lc->config;
		lp_config_set_string(lpc, "sip", "client_cert_chain", cert_path);
		lp_config_set_string(lpc, "sip", "client_cert_key", key_path);
		linphone_core_manager_start(manager, TRUE);
		linphone_core_manager_destroy(manager);
		bc_free(cert_path);
		bc_free(key_path);
	}
}

static void tls_auth_global_client_cert_api(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager *pauline = linphone_core_manager_new2("pauline_tls_client_rc", FALSE);
		char *cert_path = bc_tester_res("certificates/client/cert.pem");
		char *key_path = bc_tester_res("certificates/client/key.pem");
		char *cert = read_file(cert_path);
		char *key = read_file(key_path);
		LinphoneCore *lc = pauline->lc;
		linphone_core_set_tls_cert(lc, cert);
		linphone_core_set_tls_key(lc, key);
		BC_ASSERT_TRUE(wait_for(lc, lc, &pauline->stat.number_of_LinphoneRegistrationOk, 1));
		linphone_core_manager_destroy(pauline);
		ms_free(cert);
		ms_free(key);
		bc_free(cert_path);
		bc_free(key_path);
	}
}

static void tls_auth_global_client_cert_api_path(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager *pauline = linphone_core_manager_new2("pauline_tls_client_rc", FALSE);
		char *cert = bc_tester_res("certificates/client/cert.pem");
		char *key = bc_tester_res("certificates/client/key.pem");
		LinphoneCore *lc = pauline->lc;
		linphone_core_set_tls_cert_path(lc, cert);
		linphone_core_set_tls_key_path(lc, key);
		BC_ASSERT_TRUE(wait_for(lc, lc, &pauline->stat.number_of_LinphoneRegistrationOk, 1));
		linphone_core_manager_destroy(pauline);
		bc_free(cert);
		bc_free(key);
	}
}

static void tls_auth_info_client_cert_api(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager *pauline = linphone_core_manager_new2("pauline_tls_client_rc", FALSE);
		char *cert_path = bc_tester_res("certificates/client/cert.pem");
		char *key_path = bc_tester_res("certificates/client/key.pem");
		char *cert = read_file(cert_path);
		char *key = read_file(key_path);
		LinphoneCore *lc = pauline->lc;
		LinphoneAuthInfo *authInfo = (LinphoneAuthInfo *)lc->auth_info->data;
		linphone_auth_info_set_tls_cert(authInfo, cert);
		linphone_auth_info_set_tls_key(authInfo, key);
		BC_ASSERT_TRUE(wait_for(lc, lc, &pauline->stat.number_of_LinphoneRegistrationOk, 1));
		linphone_core_manager_destroy(pauline);
		ms_free(cert);
		ms_free(key);
		bc_free(cert_path);
		bc_free(key_path);
	}
}

static void tls_auth_info_client_cert_api_path(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager *pauline = linphone_core_manager_new2("pauline_tls_client_rc", FALSE);
		char *cert = bc_tester_res("certificates/client/cert.pem");
		char *key = bc_tester_res("certificates/client/key.pem");
		LinphoneCore *lc = pauline->lc;
		LinphoneAuthInfo *authInfo = (LinphoneAuthInfo *)lc->auth_info->data;
		linphone_auth_info_set_tls_cert_path(authInfo, cert);
		linphone_auth_info_set_tls_key_path(authInfo, key);
		BC_ASSERT_TRUE(wait_for(lc, lc, &pauline->stat.number_of_LinphoneRegistrationOk, 1));
		linphone_core_manager_destroy(pauline);
		bc_free(cert);
		bc_free(key);
	}
}

static void authentication_requested_2(LinphoneCore *lc, LinphoneAuthInfo *auth_info, LinphoneAuthMethod method) {
	char *cert = bc_tester_res("certificates/client/cert.pem");
	char *key = bc_tester_res("certificates/client/key.pem");
	BC_ASSERT_EQUAL(method, LinphoneAuthTls, int, "%i");
	linphone_auth_info_set_tls_cert_path(auth_info, cert);
	linphone_auth_info_set_tls_key_path(auth_info, key);
	linphone_core_add_auth_info(lc, auth_info);
	bc_free(cert);
	bc_free(key);
}

static void tls_auth_info_client_cert_cb(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager *lcm;
		LinphoneCoreVTable* vtable = linphone_core_v_table_new();
		stats* counters;

		lcm = linphone_core_manager_new(NULL);

		vtable->authentication_requested=authentication_requested_2;
		linphone_core_add_listener(lcm->lc,vtable);

		counters= get_stats(lcm->lc);
		counters->number_of_auth_info_requested=0;
		register_with_refresh(lcm,FALSE,auth_domain,"sip2.linphone.org:5063;transport=tls");
		BC_ASSERT_EQUAL(counters->number_of_auth_info_requested,1, int, "%d");
		linphone_core_manager_destroy(lcm);
	}
}

static void authentication_requested_3(LinphoneCore *lc, LinphoneAuthInfo *auth_info, LinphoneAuthMethod method) {
	char *cert_path = bc_tester_res("certificates/client/cert.pem");
	char *key_path = bc_tester_res("certificates/client/key.pem");
	char *cert = read_file(cert_path);
	char *key = read_file(key_path);
	BC_ASSERT_EQUAL(method, LinphoneAuthTls, int, "%i");
	linphone_auth_info_set_tls_cert(auth_info, cert);
	linphone_auth_info_set_tls_key(auth_info, key);
	linphone_core_add_auth_info(lc, auth_info);
	ms_free(cert);
	ms_free(key);
	bc_free(cert_path);
	bc_free(key_path);
}

static void tls_auth_info_client_cert_cb_2(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager *lcm;
		LinphoneCoreVTable* vtable = linphone_core_v_table_new();
		stats* counters;

		lcm = linphone_core_manager_new(NULL);

		vtable->authentication_requested=authentication_requested_3;
		linphone_core_add_listener(lcm->lc,vtable);

		counters= get_stats(lcm->lc);
		counters->number_of_auth_info_requested=0;
		register_with_refresh(lcm,FALSE,auth_domain,"sip2.linphone.org:5063;transport=tls");
		BC_ASSERT_EQUAL(counters->number_of_auth_info_requested,1, int, "%d");
		linphone_core_manager_destroy(lcm);
	}
}


test_t register_tests[] = {
	TEST_NO_TAG("Simple register", simple_register),
	TEST_NO_TAG("Simple register unregister", simple_unregister),
	TEST_NO_TAG("TCP register", simple_tcp_register),
	TEST_NO_TAG("Register with custom headers", register_with_custom_headers),
	TEST_NO_TAG("TCP register compatibility mode", simple_tcp_register_compatibility_mode),
	TEST_NO_TAG("TLS register", simple_tls_register),
	TEST_NO_TAG("TLS register with alt. name certificate", tls_alt_name_register),
	TEST_NO_TAG("TLS register with wildcard certificate", tls_wildcard_register),
	TEST_NO_TAG("TLS certificate not verified",tls_certificate_failure),
	TEST_NO_TAG("TLS certificate given by string instead of file",tls_certificate_data),
	TEST_NO_TAG("TLS with non tls server",tls_with_non_tls_server),
	TEST_NO_TAG("Simple authenticated register", simple_authenticated_register),
	TEST_NO_TAG("Ha1 authenticated register", ha1_authenticated_register),
	TEST_NO_TAG("Digest auth without initial credentials", authenticated_register_with_no_initial_credentials),
	TEST_NO_TAG("Digest auth with wrong credentials", authenticated_register_with_wrong_credentials),
	TEST_NO_TAG("Digest auth with wrong credentials, check if registration attempts are stopped", authenticated_register_with_wrong_credentials_2),
	TEST_NO_TAG("Digest auth with wrong credentials without 403", authenticated_register_with_wrong_credentials_without_403),
	TEST_NO_TAG("Authenticated register with wrong late credentials", authenticated_register_with_wrong_late_credentials),
	TEST_NO_TAG("Authenticated register with late credentials", authenticated_register_with_late_credentials),
	TEST_NO_TAG("Authenticated register with provided credentials", authenticated_register_with_provided_credentials),
	TEST_NO_TAG("Register with refresh", simple_register_with_refresh),
	TEST_NO_TAG("Authenticated register with refresh", simple_auth_register_with_refresh),
	TEST_NO_TAG("Register with refresh and send error", register_with_refresh_with_send_error),
	TEST_NO_TAG("Multi account", multiple_proxy),
	TEST_NO_TAG("Transport changes", transport_change),
	TEST_NO_TAG("Transport configured with dontbind option", transport_dont_bind),
	TEST_NO_TAG("Transport busy", transport_busy),
	TEST_NO_TAG("Proxy transport changes", proxy_transport_change),
	TEST_NO_TAG("Proxy transport changes with wrong address at first", proxy_transport_change_with_wrong_port),
	TEST_NO_TAG("Proxy transport changes with wrong address, giving up",proxy_transport_change_with_wrong_port_givin_up),
	TEST_NO_TAG("Change expires", change_expires),
	TEST_NO_TAG("Network state change", network_state_change),
	TEST_NO_TAG("Io recv error", io_recv_error),
	TEST_NO_TAG("Io recv error with recovery", io_recv_error_retry_immediatly),
	TEST_NO_TAG("Io recv error with late recovery", io_recv_error_late_recovery),
	TEST_NO_TAG("Io recv error without active registration", io_recv_error_without_active_register),
	TEST_NO_TAG("Simple redirect", redirect),
	TEST_NO_TAG("Global TLS client certificate authentication", tls_auth_global_client_cert),
	TEST_NO_TAG("Global TLS client certificate authentication using API", tls_auth_global_client_cert_api),
	TEST_NO_TAG("Global TLS client certificate authentication using API 2", tls_auth_global_client_cert_api_path),
	TEST_NO_TAG("AuthInfo TLS client certificate authentication using API", tls_auth_info_client_cert_api),
	TEST_NO_TAG("AuthInfo TLS client certificate authentication using API 2", tls_auth_info_client_cert_api_path),
	TEST_NO_TAG("AuthInfo TLS client certificate authentication in callback", tls_auth_info_client_cert_cb),
	TEST_NO_TAG("AuthInfo TLS client certificate authentication in callback 2", tls_auth_info_client_cert_cb_2),
};

test_suite_t register_test_suite = {"Register", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
									sizeof(register_tests) / sizeof(register_tests[0]), register_tests};
