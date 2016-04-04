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
#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"
#include <bctoolbox/tester.h>

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wstrict-prototypes"

#ifdef HAVE_GTK
#include <gtk/gtk.h>
#endif

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif

#if _WIN32
#define unlink _unlink
#endif

static bool_t liblinphone_tester_ipv6_enabled=FALSE;
static int liblinphone_tester_keep_accounts_flag = 0;
static int liblinphone_tester_keep_record_files = FALSE;
static int liblinphone_tester_leak_detector_disabled = FALSE;
int manager_count = 0;
int leaked_objects_count = 0;
const MSAudioDiffParams audio_cmp_params = {10,2000};

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
	if (counters->last_received_info_message) linphone_info_message_destroy(counters->last_received_info_message);
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
		if (ortp_file_exist(filepath) != 0) {
			ms_fatal("Could not find file %s in path %s, did you configured resources directory correctly?", file, path);
		}
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

bool_t wait_for_stun_resolution(LinphoneCoreManager *m) {
	MSTimeSpec start;
	int timeout_ms = 10000;

	liblinphone_tester_clock_start(&start);
	while (m->lc->net_conf.stun_addrinfo == NULL && !liblinphone_tester_clock_elapsed(&start,timeout_ms)) {
		linphone_core_iterate(m->lc);
		ms_usleep(20000);
	}
	return m->lc->net_conf.stun_addrinfo != NULL;
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
	Sal *sal = sal_init(NULL);
	bool_t supported = sal_transport_available(sal,(SalTransport)transport);
	if (!supported) ms_message("TLS transport not supported, falling back to TCP if possible otherwise skipping test.");
	sal_uninit(sal);
	return supported;
}

void linphone_core_manager_init(LinphoneCoreManager *mgr, const char* rc_file) {
	char *rc_path = NULL;
	char *hellopath = bc_tester_res("sounds/hello8000.wav");
	mgr->number_of_cunit_error_at_creation =  bc_get_number_of_failures();
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
	mgr->lc=configure_lc_from(&mgr->v_table, bc_tester_get_resource_dir_prefix(), rc_path, mgr);
	linphone_core_manager_check_accounts(mgr);

	manager_count++;

#if TARGET_OS_IPHONE
	linphone_core_set_ringer_device( mgr->lc, "AQ: Audio Queue Device");
	linphone_core_set_ringback(mgr->lc, NULL);
#elif __QNX__
	linphone_core_set_playback_device(mgr->lc, "QSA: voice");
#endif

#ifdef VIDEO_ENABLED
	{
		MSWebCam *cam;

		cam = ms_web_cam_manager_get_cam(ms_factory_get_web_cam_manager(mgr->lc->factory), "Mire: Mire (synthetic moving picture)");

		if (cam == NULL) {
			MSWebCamDesc *desc = ms_mire_webcam_desc_get();
			if (desc){
				cam=ms_web_cam_new(desc);
				ms_web_cam_manager_add_cam(ms_factory_get_web_cam_manager(mgr->lc->factory), cam);
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
	/*for now, we need the periodical updates facility to compute bandwidth measurements correctly during tests*/
	lp_config_set_int(linphone_core_get_config(mgr->lc), "misc", "send_call_stats_periodical_updates", 1);

	if (rc_path) ms_free(rc_path);
}

void linphone_core_manager_start(LinphoneCoreManager *mgr, int check_for_proxies) {
	LinphoneProxyConfig* proxy;
	int proxy_count;

	/*BC_ASSERT_EQUAL(ms_list_size(linphone_core_get_proxy_config_list(lc)),proxy_count, int, "%d");*/
	if (check_for_proxies){ /**/
		proxy_count=ms_list_size(linphone_core_get_proxy_config_list(mgr->lc));
	}else{
		proxy_count=0;
		/*this is to prevent registration to go on*/
		linphone_core_set_network_reachable(mgr->lc, FALSE);
	}

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

	proxy = linphone_core_get_default_proxy_config(mgr->lc);
	if (proxy) {
		if (mgr->identity){
			linphone_address_destroy(mgr->identity);
		}
		mgr->identity = linphone_address_clone(linphone_proxy_config_get_identity_address(proxy));
		linphone_address_clean(mgr->identity);
	}

	if (linphone_core_get_stun_server(mgr->lc) != NULL){
		/*before we go, ensure that the stun server is resolved, otherwise all ice related test will fail*/
		BC_ASSERT_TRUE(wait_for_stun_resolution(mgr));
	}
	if (!check_for_proxies){
		/*now that stun server resolution is done, we can start registering*/
		linphone_core_set_network_reachable(mgr->lc, TRUE);
	}
}

LinphoneCoreManager* linphone_core_manager_new( const char* rc_file) {
	LinphoneCoreManager *manager = ms_new0(LinphoneCoreManager, 1);
	linphone_core_manager_init(manager, rc_file);
	linphone_core_manager_start(manager, TRUE);
	return manager;
}

LinphoneCoreManager* linphone_core_manager_new2( const char* rc_file, int check_for_proxies) {
	LinphoneCoreManager *manager = ms_new0(LinphoneCoreManager, 1);
	linphone_core_manager_init(manager, rc_file);
	linphone_core_manager_start(manager, check_for_proxies);
	return manager;
}

void linphone_core_manager_stop(LinphoneCoreManager *mgr){
	if (mgr->lc) {
		linphone_core_destroy(mgr->lc);
		mgr->lc=NULL;
	}
}

void linphone_core_manager_uninit(LinphoneCoreManager *mgr) {
	if (mgr->stat.last_received_chat_message) {
		linphone_chat_message_unref(mgr->stat.last_received_chat_message);
	}
	if (mgr->stat.last_received_info_message) linphone_info_message_destroy(mgr->stat.last_received_info_message);
	if (mgr->lc){
		const char *record_file=linphone_core_get_record_file(mgr->lc);
		int unterminated_calls;

		if (!liblinphone_tester_keep_record_files && record_file){
			if ((bc_get_number_of_failures()-mgr->number_of_cunit_error_at_creation)>0) {
				ms_message ("Test has failed, keeping recorded file [%s]",record_file);
			} else {
				unlink(record_file);
			}
		}
		BC_ASSERT_EQUAL((unterminated_calls=ms_list_size(mgr->lc->calls)), 0, int, "%i");
		if (unterminated_calls != 0) {
			ms_error("There are still %d calls pending, please terminates them before invoking linphone_core_manager_destroy().", unterminated_calls);
		}
		linphone_core_destroy(mgr->lc);
	}
	if (mgr->identity) {
		linphone_address_destroy(mgr->identity);
	}

	manager_count--;
}

void linphone_core_manager_destroy(LinphoneCoreManager* mgr) {
	linphone_core_manager_uninit(mgr);
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
		belle_sip_get_src_addr_for(ai->ai_addr,(socklen_t)ai->ai_addrlen,(struct sockaddr*) &ss,&slen,4444);
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

void liblinphone_tester_disable_leak_detector(int disabled){
	liblinphone_tester_leak_detector_disabled = disabled;
}

void liblinphone_tester_clear_accounts(void){
	account_manager_destroy();
}

void liblinphone_tester_add_suites() {
	bc_tester_add_suite(&setup_test_suite);
	bc_tester_add_suite(&register_test_suite);
	bc_tester_add_suite(&tunnel_test_suite);
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
	bc_tester_add_suite(&player_test_suite);
	bc_tester_add_suite(&dtmf_test_suite);
#if defined(VIDEO_ENABLED) && defined(HAVE_GTK)
	bc_tester_add_suite(&video_test_suite);
#endif
	bc_tester_add_suite(&multicast_call_test_suite);
	bc_tester_add_suite(&proxy_config_test_suite);
#if HAVE_SIPP
	bc_tester_add_suite(&complex_sip_call_test_suite);
#endif
	bc_tester_add_suite(&vcard_test_suite);
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

void liblinphone_tester_before_each(void) {
	if (!liblinphone_tester_leak_detector_disabled){
		belle_sip_object_enable_leak_detector(TRUE);
		leaked_objects_count = belle_sip_object_get_object_count();
	}
}

static char* all_leaks_buffer = NULL;

int liblinphone_tester_after_each(void) {
	int err = 0;
	if (!liblinphone_tester_leak_detector_disabled){
		int leaked_objects = belle_sip_object_get_object_count() - leaked_objects_count;
		if (leaked_objects > 0) {
			char* format = ms_strdup_printf("%d object%s leaked in suite [%s] test [%s], please fix that!",
											leaked_objects, leaked_objects>1?"s were":" was",
											bc_tester_current_suite_name(), bc_tester_current_test_name());
			belle_sip_object_dump_active_objects();
			belle_sip_object_flush_active_objects();
			bc_tester_printf(ORTP_MESSAGE, format);
			ms_error("%s", format);

			all_leaks_buffer = ms_strcat_printf(all_leaks_buffer, "\n%s", format);
		}

		// prevent any future leaks
		{
			const char **tags = bc_tester_current_test_tags();
			int leaks_expected =
				(tags && ((tags[0] && !strcmp(tags[0], "LeaksMemory")) || (tags[1] && !strcmp(tags[1], "LeaksMemory"))));
			// if the test is NOT marked as leaking memory and it actually is, we should make it fail
			if (!leaks_expected && leaked_objects > 0) {
				BC_FAIL("This test is leaking memory!");
				err = 1;
				// and reciprocally
			} else if (leaks_expected && leaked_objects == 0) {
				BC_FAIL("This test is not leaking anymore, please remove LeaksMemory tag!");
				// err = 1; // do not force fail actually, because it can be some false positive warning
			}
		}
	}

	if (manager_count != 0) {
		ms_fatal("%d Linphone core managers are still alive!", manager_count);
	}
	return err;
}

void liblinphone_tester_uninit(void) {
	// show all leaks that happened during the test
	if (all_leaks_buffer) {
		bc_tester_printf(ORTP_MESSAGE, all_leaks_buffer);
		ms_free(all_leaks_buffer);
		all_leaks_buffer = NULL;
	}
	bc_tester_uninit();
}

static void check_ice_from_rtp(LinphoneCall *c1, LinphoneCall *c2, LinphoneStreamType stream_type) {
	MediaStream *ms;
	switch (stream_type) {
	case LinphoneStreamTypeAudio:
		ms=&c1->audiostream->ms;
		break;
	case LinphoneStreamTypeVideo:
		ms=&c1->videostream->ms;
		break;
	case LinphoneStreamTypeText:
		ms=&c1->textstream->ms;
		break;
	default:
		ms_error("Unknown stream type [%s]",  linphone_stream_type_to_string(stream_type));
		BC_ASSERT_FALSE(stream_type >= LinphoneStreamTypeUnknown);
		return;
	}


	if (linphone_call_get_audio_stats(c1)->ice_state == LinphoneIceStateHostConnection && media_stream_started(ms)) {
		char ip[16];
		char port[8];
		getnameinfo((const struct sockaddr *)&c1->audiostream->ms.sessions.rtp_session->rtp.gs.rem_addr
					, c1->audiostream->ms.sessions.rtp_session->rtp.gs.rem_addrlen
					, ip
					, sizeof(ip)
					, port
					, sizeof(port)
					, NI_NUMERICHOST|NI_NUMERICSERV);
		BC_ASSERT_STRING_EQUAL(ip, c2->media_localip);
	}
}
bool_t check_ice(LinphoneCoreManager* caller, LinphoneCoreManager* callee, LinphoneIceState state) {
	LinphoneCall *c1,*c2;
	bool_t audio_success=FALSE;
	bool_t video_success=FALSE;
	bool_t text_success=FALSE;
	bool_t video_enabled, realtime_text_enabled;
	MSTimeSpec ts;

	c1=linphone_core_get_current_call(caller->lc);
	c2=linphone_core_get_current_call(callee->lc);

	BC_ASSERT_PTR_NOT_NULL(c1);
	BC_ASSERT_PTR_NOT_NULL(c2);
	if (!c1 || !c2) return FALSE;
	linphone_call_ref(c1);
	linphone_call_ref(c2);

	BC_ASSERT_EQUAL(linphone_call_params_video_enabled(linphone_call_get_current_params(c1)),linphone_call_params_video_enabled(linphone_call_get_current_params(c2)), int, "%d");
	BC_ASSERT_EQUAL(linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(c1)),linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(c2)), int, "%d");
	video_enabled=linphone_call_params_video_enabled(linphone_call_get_current_params(c1));
	realtime_text_enabled=linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(c1));
	liblinphone_tester_clock_start(&ts);
	do{
		if ((c1 != NULL) && (c2 != NULL)) {
			if (linphone_call_get_audio_stats(c1)->ice_state==state &&
				linphone_call_get_audio_stats(c2)->ice_state==state ){
				audio_success=TRUE;
				check_ice_from_rtp(c1,c2,LinphoneStreamTypeAudio);
				check_ice_from_rtp(c2,c1,LinphoneStreamTypeAudio);
				break;
			}
			linphone_core_iterate(caller->lc);
			linphone_core_iterate(callee->lc);
		}
		ms_usleep(20000);
	}while(!liblinphone_tester_clock_elapsed(&ts,10000));

	if (video_enabled){
		liblinphone_tester_clock_start(&ts);
		do{
			if ((c1 != NULL) && (c2 != NULL)) {
				if (linphone_call_get_video_stats(c1)->ice_state==state &&
					linphone_call_get_video_stats(c2)->ice_state==state ){
					video_success=TRUE;
					check_ice_from_rtp(c1,c2,LinphoneStreamTypeVideo);
					check_ice_from_rtp(c2,c1,LinphoneStreamTypeVideo);
					break;
				}
				linphone_core_iterate(caller->lc);
				linphone_core_iterate(callee->lc);
			}
			ms_usleep(20000);
		}while(!liblinphone_tester_clock_elapsed(&ts,10000));
	}

	if (realtime_text_enabled){
		liblinphone_tester_clock_start(&ts);
		do{
			if ((c1 != NULL) && (c2 != NULL)) {
				if (linphone_call_get_text_stats(c1)->ice_state==state &&
					linphone_call_get_text_stats(c2)->ice_state==state ){
					text_success=TRUE;
					check_ice_from_rtp(c1,c2,LinphoneStreamTypeText);
					check_ice_from_rtp(c2,c1,LinphoneStreamTypeText);
					break;
				}
				linphone_core_iterate(caller->lc);
				linphone_core_iterate(callee->lc);
			}
			ms_usleep(20000);
		}while(!liblinphone_tester_clock_elapsed(&ts,10000));
	}

	/*make sure encryption mode are preserved*/
	if (c1) {
		const LinphoneCallParams* call_param = linphone_call_get_current_params(c1);
		BC_ASSERT_EQUAL(linphone_call_params_get_media_encryption(call_param),linphone_core_get_media_encryption(caller->lc), int, "%d");
	}
	if (c2) {
		const LinphoneCallParams* call_param = linphone_call_get_current_params(c2);
		BC_ASSERT_EQUAL(linphone_call_params_get_media_encryption(call_param),linphone_core_get_media_encryption(callee->lc), int, "%d");
	}
	linphone_call_unref(c1);
	linphone_call_unref(c2);
	return video_enabled ? (realtime_text_enabled ? text_success && audio_success && video_success : audio_success && video_success) : realtime_text_enabled ? text_success && audio_success : audio_success;
}

static void linphone_conference_server_call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg) {
	LinphoneCoreVTable *vtable = linphone_core_get_current_vtable(lc);
	LinphoneConferenceServer *conf_srv = (LinphoneConferenceServer *)vtable->user_data;

	switch(cstate) {
		case LinphoneCallIncomingReceived:
			linphone_core_accept_call(lc, call);
			break;

		case LinphoneCallStreamsRunning:
			if(linphone_call_get_conference(call) == NULL) {
				linphone_core_add_to_conference(lc, call);
				linphone_core_leave_conference(lc);
				if(conf_srv->first_call == NULL) conf_srv->first_call = call;
			}
			break;

		case LinphoneCallEnd:
			if(call == conf_srv->first_call) {
				linphone_core_terminate_conference(lc);
				conf_srv->first_call = NULL;
			}
			break;

		default: break;
	}
}

static void linphone_conference_server_refer_received(LinphoneCore *core, const char *refer_to) {
	char method[20];
	LinphoneAddress *refer_to_addr = linphone_address_new(refer_to);
	char *uri;
	LinphoneCall *call;

	if(refer_to_addr == NULL) return;
	strncpy(method, linphone_address_get_method_param(refer_to_addr), sizeof(method));
	if(strcmp(method, "BYE") == 0) {
		linphone_address_clean(refer_to_addr);
		uri = linphone_address_as_string_uri_only(refer_to_addr);
		call = linphone_core_find_call_from_uri(core, uri);
		if(call) linphone_core_terminate_call(core, call);
		ms_free(uri);
	}
	linphone_address_destroy(refer_to_addr);
}

static void linphone_conference_server_registration_state_changed(LinphoneCore *core,
																  LinphoneProxyConfig *cfg,
																  LinphoneRegistrationState cstate,
																  const char *message) {
	LinphoneCoreVTable *vtable = linphone_core_get_current_vtable(core);
	LinphoneConferenceServer *m = (LinphoneConferenceServer *)linphone_core_v_table_get_user_data(vtable);
	if(cfg == linphone_core_get_default_proxy_config(core)) {
		m->reg_state = cstate;
	}
}

LinphoneConferenceServer* linphone_conference_server_new(const char *rc_file, bool_t do_registration) {
	LinphoneConferenceServer *conf_srv = (LinphoneConferenceServer *)ms_new0(LinphoneConferenceServer, 1);
	LinphoneCoreManager *lm = (LinphoneCoreManager *)conf_srv;

	conf_srv->vtable = linphone_core_v_table_new();
	conf_srv->vtable->call_state_changed = linphone_conference_server_call_state_changed;
	conf_srv->vtable->refer_received = linphone_conference_server_refer_received;
	conf_srv->vtable->registration_state_changed = linphone_conference_server_registration_state_changed;
	conf_srv->vtable->user_data = conf_srv;
	conf_srv->reg_state = LinphoneRegistrationNone;
	linphone_core_manager_init(lm, rc_file);
	linphone_core_add_listener(lm->lc, conf_srv->vtable);
	linphone_core_manager_start(lm, do_registration);
	return conf_srv;
}

void linphone_conference_server_destroy(LinphoneConferenceServer *conf_srv) {
	linphone_core_manager_uninit((LinphoneCoreManager *)conf_srv);
	linphone_core_v_table_destroy(conf_srv->vtable);
	ms_free(conf_srv);
}
