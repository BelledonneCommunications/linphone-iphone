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
#ifdef HAVE_GTK
#include <gtk/gtk.h>
#endif
#if _WIN32
#define unlink _unlink
#endif

static bool_t liblinphone_tester_ipv6_enabled=FALSE;
static int liblinphone_tester_keep_accounts_flag = 0;
static int liblinphone_tester_keep_record_files = FALSE;
int manager_count = 0;

const char* test_domain="sipopen.example.org";
const char* auth_domain="sip.example.org";
const char* test_username="liblinphone_tester";
const char* test_password="secret";
const char* test_route="sip2.linphone.org";
const char *userhostsfile = "tester_hosts";

const char *liblinphone_tester_mire_id="Mire: Mire (synthetic moving picture)";

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

void liblinphone_tester_enable_ipv6(bool_t enabled){
	liblinphone_tester_ipv6_enabled=enabled;
}

LinphoneAddress * create_linphone_address(const char * domain) {
	LinphoneAddress *addr = linphone_address_new(NULL);
	BC_ASSERT_PTR_NOT_NULL_FATAL(addr);
	linphone_address_set_username(addr,test_username);
	BC_ASSERT_STRING_EQUAL(test_username,linphone_address_get_username(addr));
	if (!domain) domain= test_route;
	linphone_address_set_domain(addr,domain);
	BC_ASSERT_STRING_EQUAL(domain,linphone_address_get_domain(addr));
	linphone_address_set_display_name(addr, NULL);
	linphone_address_set_display_name(addr, "Mr Tester");
	BC_ASSERT_STRING_EQUAL("Mr Tester",linphone_address_get_display_name(addr));
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
		BC_ASSERT_EQUAL_FATAL(ortp_file_exist(filepath),0,int, "%d");
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

	linphone_core_enable_ipv6(lc, liblinphone_tester_ipv6_enabled);

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
#ifdef HAVE_GTK
			gdk_threads_enter();
			gtk_main_iteration_do(FALSE);
			gdk_threads_leave();
#endif
			linphone_core_iterate((LinphoneCore*)(iterator->data));
		}
#ifdef LINPHONE_WINDOWS_DESKTOP
		{
			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0,1)){
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
#endif
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

bool_t transport_supported(LinphoneTransportType transport) {
	Sal *sal = sal_init();
	bool_t supported = sal_transport_available(sal,(SalTransport)transport);
	if (!supported) ms_message("TLS transport not supported, falling back to TCP if possible otherwise skipping test.");
	sal_uninit(sal);
	return supported;
}


static void display_status(LinphoneCore *lc, const char *status){
	ms_message("display_status(): %s",status);
}

LinphoneCoreManager* linphone_core_manager_init(const char* rc_file) {
	LinphoneCoreManager* mgr= ms_new0(LinphoneCoreManager,1);
	char *rc_path = NULL;
	char *hellopath = bc_tester_res("sounds/hello8000.wav");
	mgr->number_of_cunit_error_at_creation = CU_get_number_of_failures();
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
	mgr->v_table.display_status=display_status;

	reset_counters(&mgr->stat);
	if (rc_file) rc_path = ms_strdup_printf("rcfiles/%s", rc_file);
	mgr->lc=configure_lc_from(&mgr->v_table, bc_tester_get_resource_dir_prefix(), rc_path, mgr);
	linphone_core_manager_check_accounts(mgr);

	manager_count++;

#if TARGET_OS_IPHONE
	linphone_core_set_ringer_device( mgr->lc, "AQ: Audio Queue Device");
	linphone_core_set_ringback(mgr->lc, NULL);
#endif

#ifdef VIDEO_ENABLED
	{
		MSWebCam *cam;

		cam = ms_web_cam_manager_get_cam(ms_web_cam_manager_get(), "Mire: Mire (synthetic moving picture)");

		if (cam == NULL) {
			MSWebCamDesc *desc = ms_mire_webcam_desc_get();
			if (desc){
				cam=ms_web_cam_new(desc);
				ms_web_cam_manager_add_cam(ms_web_cam_manager_get(), cam);
			}
		}
	}
#endif


	linphone_core_set_play_file(mgr->lc,hellopath); /*is also used when in pause*/
	ms_free(hellopath);

	if( manager_count >= 2){
		char *recordpath = ms_strdup_printf("%s/record_for_lc_%p.wav",bc_tester_get_writable_dir_prefix(),mgr->lc);
		ms_message("Manager for '%s' using files", rc_file ? rc_file : "--");
		linphone_core_set_use_files(mgr->lc, TRUE);
		linphone_core_set_record_file(mgr->lc,recordpath);
		ms_free(recordpath);
	}

	linphone_core_set_user_certificates_path(mgr->lc,bc_tester_get_writable_dir_prefix());

	if (rc_path) ms_free(rc_path);

	return mgr;
}

void linphone_core_manager_start(LinphoneCoreManager *mgr, const char* rc_file, int check_for_proxies) {
	LinphoneProxyConfig* proxy;
	int proxy_count;

	/*BC_ASSERT_EQUAL(ms_list_size(linphone_core_get_proxy_config_list(lc)),proxy_count, int, "%d");*/
	if (check_for_proxies && rc_file) /**/
		proxy_count=ms_list_size(linphone_core_get_proxy_config_list(mgr->lc));
	else
		proxy_count=0;

	if (proxy_count){
#define REGISTER_TIMEOUT 20 /* seconds */
		int success = wait_for_until(mgr->lc,NULL,&mgr->stat.number_of_LinphoneRegistrationOk,
									proxy_count,(REGISTER_TIMEOUT * 1000 * proxy_count));
		if( !success ){
			ms_error("Did not register after %d seconds for %d proxies", REGISTER_TIMEOUT, proxy_count);
		}
	}
	BC_ASSERT_EQUAL(mgr->stat.number_of_LinphoneRegistrationOk,proxy_count, int, "%d");
	enable_codec(mgr->lc,"PCMU",8000);

	linphone_core_get_default_proxy(mgr->lc,&proxy);
	if (proxy) {
		mgr->identity = linphone_address_clone(linphone_proxy_config_get_identity_address(proxy));
		linphone_address_clean(mgr->identity);
	}
}

LinphoneCoreManager* linphone_core_manager_new( const char* rc_file) {
	LinphoneCoreManager *manager = linphone_core_manager_init(rc_file);
	linphone_core_manager_start(manager, rc_file, TRUE);
	return manager;
}

LinphoneCoreManager* linphone_core_manager_new2( const char* rc_file, int check_for_proxies) {
	LinphoneCoreManager *manager = linphone_core_manager_init(rc_file);
	linphone_core_manager_start(manager, rc_file, check_for_proxies);
	return manager;
}

void linphone_core_manager_stop(LinphoneCoreManager *mgr){
	if (mgr->lc) {
		linphone_core_destroy(mgr->lc);
		mgr->lc=NULL;
	}
}

void linphone_core_manager_destroy(LinphoneCoreManager* mgr) {
	if (mgr->lc){
		const char *record_file=linphone_core_get_record_file(mgr->lc);
		if (!liblinphone_tester_keep_record_files && record_file){
			if ((CU_get_number_of_failures()-mgr->number_of_cunit_error_at_creation)>0) {
				ms_message ("Test has failed, keeping recorded file [%s]",record_file);
			} else {
				unlink(record_file);
			}
		}
		linphone_core_destroy(mgr->lc);
	}
	if (mgr->identity) linphone_address_destroy(mgr->identity);
	if (mgr->stat.last_received_chat_message) linphone_chat_message_unref(mgr->stat.last_received_chat_message);
	manager_count--;

	ms_free(mgr);
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

void liblinphone_tester_keep_recorded_files(int keep){
	liblinphone_tester_keep_record_files = keep;
}

void liblinphone_tester_clear_accounts(void){
	account_manager_destroy();
}

void liblinphone_tester_add_suites() {
	bc_tester_add_suite(&setup_test_suite);
	bc_tester_add_suite(&register_test_suite);
	bc_tester_add_suite(&offeranswer_test_suite);
	bc_tester_add_suite(&call_test_suite);
	bc_tester_add_suite(&multi_call_test_suite);
	bc_tester_add_suite(&message_test_suite);
	bc_tester_add_suite(&presence_test_suite);
#ifdef UPNP
	bc_tester_add_suite(&upnp_test_suite);
#endif
	bc_tester_add_suite(&stun_test_suite);
	bc_tester_add_suite(&event_test_suite);
	bc_tester_add_suite(&flexisip_test_suite);
	bc_tester_add_suite(&remote_provisioning_test_suite);
	bc_tester_add_suite(&quality_reporting_test_suite);
	bc_tester_add_suite(&log_collection_test_suite);
	bc_tester_add_suite(&tunnel_test_suite);
	bc_tester_add_suite(&player_test_suite);
	bc_tester_add_suite(&dtmf_test_suite);
#if defined(VIDEO_ENABLED) && defined(HAVE_GTK)
	bc_tester_add_suite(&video_test_suite);
#endif
	bc_tester_add_suite(&multicast_call_test_suite);
	bc_tester_add_suite(&proxy_config_test_suite);
}

static int linphone_core_manager_get_max_audio_bw_base(const int array[],int array_size) {
	int i,result=0;
	for (i=0; i<array_size; i++) {
		result = MAX(result,array[i]);
	}
	return result;
}

static int linphone_core_manager_get_mean_audio_bw_base(const int array[],int array_size) {
	int i,result=0;
	for (i=0; i<array_size; i++) {
		result += array[i];
	}
	return result/array_size;
}

int linphone_core_manager_get_max_audio_down_bw(const LinphoneCoreManager *mgr) {
	return linphone_core_manager_get_max_audio_bw_base(mgr->stat.audio_download_bandwidth
			, sizeof(mgr->stat.audio_download_bandwidth)/sizeof(int));
}
int linphone_core_manager_get_max_audio_up_bw(const LinphoneCoreManager *mgr) {
	return linphone_core_manager_get_max_audio_bw_base(mgr->stat.audio_upload_bandwidth
			, sizeof(mgr->stat.audio_upload_bandwidth)/sizeof(int));
}

int linphone_core_manager_get_mean_audio_down_bw(const LinphoneCoreManager *mgr) {
	return linphone_core_manager_get_mean_audio_bw_base(mgr->stat.audio_download_bandwidth
			, sizeof(mgr->stat.audio_download_bandwidth)/sizeof(int));
}
int linphone_core_manager_get_mean_audio_up_bw(const LinphoneCoreManager *mgr) {
	return linphone_core_manager_get_mean_audio_bw_base(mgr->stat.audio_upload_bandwidth
			, sizeof(mgr->stat.audio_upload_bandwidth)/sizeof(int));
}

int liblinphone_tester_setup() {
	if (manager_count != 0) {
		// crash in some linphone core have not been destroyed because if we continue
		// it will crash in CUnit AND we should NEVER keep a manager alive
		ms_fatal("%d linphone core manager still alive!", manager_count);
		return 1;
	}
	return 0;
}
