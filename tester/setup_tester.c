/*
	liblinphone_tester - liblinphone test suite
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


#include "linphone/core.h"
#include "liblinphone_tester.h"
#include "linphone/lpconfig.h"
#include "private.h"

static void linphone_version_test(void){
	const char *version=linphone_core_get_version();
	/*make sure the git version is always included in the version number*/
	BC_ASSERT_PTR_NOT_NULL(version);
	BC_ASSERT_PTR_NULL(strstr(version,"unknown"));
}

static void core_init_test(void) {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	memset (&v_table,0,sizeof(v_table));
	lc = linphone_core_new(&v_table,NULL,NULL,NULL);
	/* until we have good certificates on our test server... */
	linphone_core_verify_server_certificates(lc,FALSE);
	if (BC_ASSERT_PTR_NOT_NULL(lc)) {
		linphone_core_destroy(lc);
	}
}

static void linphone_address_test(void) {
	linphone_address_unref(create_linphone_address(NULL));
	BC_ASSERT_PTR_NULL(linphone_address_new("sip:@sip.linphone.org"));

}

static void core_sip_transport_test(void) {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	LCSipTransports tr;
	memset (&v_table,0,sizeof(v_table));
	lc = linphone_core_new(&v_table,NULL,NULL,NULL);
	if (!BC_ASSERT_PTR_NOT_NULL(lc)) return;
	linphone_core_get_sip_transports(lc,&tr);
	BC_ASSERT_EQUAL(tr.udp_port,5060, int, "%d"); /*default config*/
	BC_ASSERT_EQUAL(tr.tcp_port,5060, int, "%d"); /*default config*/

	tr.udp_port=LC_SIP_TRANSPORT_RANDOM;
	tr.tcp_port=LC_SIP_TRANSPORT_RANDOM;
	tr.tls_port=LC_SIP_TRANSPORT_RANDOM;

	linphone_core_set_sip_transports(lc,&tr);
	linphone_core_get_sip_transports(lc,&tr);

	BC_ASSERT_NOT_EQUAL(tr.udp_port,5060,int,"%d"); /*default config*/
	BC_ASSERT_NOT_EQUAL(tr.tcp_port,5060,int,"%d"); /*default config*/

	BC_ASSERT_EQUAL(lp_config_get_int(linphone_core_get_config(lc),"sip","sip_port",-2),LC_SIP_TRANSPORT_RANDOM, int, "%d");
	BC_ASSERT_EQUAL(lp_config_get_int(linphone_core_get_config(lc),"sip","sip_tcp_port",-2),LC_SIP_TRANSPORT_RANDOM, int, "%d");
	BC_ASSERT_EQUAL(lp_config_get_int(linphone_core_get_config(lc),"sip","sip_tls_port",-2),LC_SIP_TRANSPORT_RANDOM, int, "%d");

	linphone_core_destroy(lc);
}

static void linphone_interpret_url_test(void)
{
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	const char* sips_address = "sips:margaux@sip.linphone.org";
	LinphoneAddress* address;
	LinphoneProxyConfig *proxy_config;
	char *tmp;
	memset ( &v_table,0,sizeof ( v_table ) );
	lc = linphone_core_new ( &v_table,NULL,NULL,NULL );
	if (!BC_ASSERT_PTR_NOT_NULL( lc )) return;

	proxy_config =linphone_core_create_proxy_config(lc);
	linphone_proxy_config_set_identity(proxy_config, "sip:moi@sip.linphone.org");
	linphone_proxy_config_enable_register(proxy_config, FALSE);
	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org");
	linphone_core_add_proxy_config(lc, proxy_config);
	linphone_core_set_default_proxy_config(lc,proxy_config);
	linphone_proxy_config_unref(proxy_config);

	address = linphone_core_interpret_url(lc, sips_address);
	BC_ASSERT_PTR_NOT_NULL(address);
	BC_ASSERT_STRING_EQUAL(linphone_address_get_scheme(address), "sips");
	BC_ASSERT_STRING_EQUAL(linphone_address_get_username(address), "margaux");
	BC_ASSERT_STRING_EQUAL(linphone_address_get_domain(address), "sip.linphone.org");
	linphone_address_unref(address);

	address = linphone_core_interpret_url(lc,"23");
	BC_ASSERT_PTR_NOT_NULL(address);
	BC_ASSERT_STRING_EQUAL(linphone_address_get_scheme(address), "sip");
	BC_ASSERT_STRING_EQUAL(linphone_address_get_username(address), "23");
	BC_ASSERT_STRING_EQUAL(linphone_address_get_domain(address), "sip.linphone.org");
	linphone_address_unref(address);

	address = linphone_core_interpret_url(lc,"#24");
	BC_ASSERT_PTR_NOT_NULL(address);
	BC_ASSERT_STRING_EQUAL(linphone_address_get_scheme(address), "sip");
	BC_ASSERT_STRING_EQUAL(linphone_address_get_username(address), "#24");
	BC_ASSERT_STRING_EQUAL(linphone_address_get_domain(address), "sip.linphone.org");
	tmp = linphone_address_as_string(address);
	BC_ASSERT_TRUE(strcmp (tmp,"sip:%2324@sip.linphone.org") == 0);
	linphone_address_unref(address);

	address = linphone_core_interpret_url(lc,tmp);
	BC_ASSERT_STRING_EQUAL(linphone_address_get_scheme(address), "sip");
	BC_ASSERT_STRING_EQUAL(linphone_address_get_username(address), "#24");
	BC_ASSERT_STRING_EQUAL(linphone_address_get_domain(address), "sip.linphone.org");
	linphone_address_unref(address);
	ms_free(tmp);

	linphone_core_destroy (lc);
}

static void linphone_lpconfig_from_buffer(void){
	const char* buffer = "[buffer]\ntest=ok";
	const char* buffer_linebreaks = "[buffer_linebreaks]\n\n\n\r\n\n\r\ntest=ok";
	LpConfig* conf;

	conf = lp_config_new_from_buffer(buffer);
	BC_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"buffer","test",""),"ok");
	lp_config_destroy(conf);

	conf = lp_config_new_from_buffer(buffer_linebreaks);
	BC_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"buffer_linebreaks","test",""),"ok");
	lp_config_destroy(conf);
}

static void linphone_lpconfig_from_buffer_zerolen_value(void){
	/* parameters that have no value should return NULL, not "". */
	const char* zerolen = "[test]\nzero_len=\nnon_zero_len=test";
	LpConfig* conf;

	conf = lp_config_new_from_buffer(zerolen);

	BC_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","zero_len","LOL"),"LOL");
	BC_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","non_zero_len",""),"test");

	lp_config_set_string(conf, "test", "non_zero_len", ""); /* should remove "non_zero_len" */
	BC_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","non_zero_len","LOL"), "LOL");

	lp_config_destroy(conf);
}

static void linphone_lpconfig_from_file_zerolen_value(void){
	/* parameters that have no value should return NULL, not "". */
	const char* zero_rc_file = "zero_length_params_rc";
	char* rc_path = ms_strdup_printf("%s/rcfiles/%s", bc_tester_get_resource_dir_prefix(), zero_rc_file);
	LpConfig* conf;

	/* not using lp_config_new() because it expects a readable file, and iOS (for instance)
	   stores the app bundle in read-only */
	conf = lp_config_new_with_factory(NULL, rc_path);

	BC_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","zero_len","LOL"),"LOL");

	// non_zero_len=test -> should return test
	BC_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","non_zero_len",""),"test");

	lp_config_set_string(conf, "test", "non_zero_len", ""); /* should remove "non_zero_len" */
	BC_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","non_zero_len","LOL"), "LOL");

	ms_free(rc_path);
	lp_config_destroy(conf);
}

static void linphone_lpconfig_from_xml_zerolen_value(void){
	const char* zero_xml_file = "remote_zero_length_params_rc";
	char* xml_path = ms_strdup_printf("%s/rcfiles/%s", bc_tester_get_resource_dir_prefix(), zero_xml_file);
	LpConfig* conf;

	LinphoneCoreManager* mgr = linphone_core_manager_new2("empty_rc",FALSE);

	/* BUG
	 * This test makes a provisionning by xml outside of the Configuring state of the LinphoneCore.
	 * It is leaking memory because the config is litterally erased and rewritten by the invocation
	 * of the private function linphone_remote_provisioning_load_file .
	 */ 
	
	BC_ASSERT_EQUAL(linphone_remote_provisioning_load_file(mgr->lc, xml_path), 0, int, "%d");

	conf = mgr->lc->config;

	BC_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","zero_len","LOL"),"LOL");
	BC_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","non_zero_len",""),"test");

	lp_config_set_string(conf, "test", "non_zero_len", ""); /* should remove "non_zero_len" */
	BC_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","non_zero_len","LOL"), "LOL");

	linphone_core_manager_destroy(mgr);
	ms_free(xml_path);
}

void linphone_proxy_config_address_equal_test(void) {
	LinphoneAddress *a = linphone_address_new("sip:toto@titi");
	LinphoneAddress *b = linphone_address_new("sips:toto@titi");
	LinphoneAddress *c = linphone_address_new("sip:toto@titi;transport=tcp");
	LinphoneAddress *d = linphone_address_new("sip:toto@titu");
	LinphoneAddress *e = linphone_address_new("sip:toto@titi;transport=udp");
	LinphoneAddress *f = linphone_address_new("sip:toto@titi?X-Create-Account=yes");

	BC_ASSERT_EQUAL(linphone_proxy_config_address_equal(a,NULL), LinphoneProxyConfigAddressDifferent, int, "%d");
	BC_ASSERT_EQUAL(linphone_proxy_config_address_equal(a,b), LinphoneProxyConfigAddressDifferent, int, "%d");
	BC_ASSERT_EQUAL(linphone_proxy_config_address_equal(a,c), LinphoneProxyConfigAddressDifferent, int, "%d");
	BC_ASSERT_EQUAL(linphone_proxy_config_address_equal(a,d), LinphoneProxyConfigAddressDifferent, int, "%d");
	BC_ASSERT_EQUAL(linphone_proxy_config_address_equal(a,e), LinphoneProxyConfigAddressWeakEqual, int, "%d");
	BC_ASSERT_EQUAL(linphone_proxy_config_address_equal(NULL,NULL), LinphoneProxyConfigAddressEqual, int, "%d");
	BC_ASSERT_EQUAL(linphone_proxy_config_address_equal(a,f), LinphoneProxyConfigAddressWeakEqual, int, "%d");
	BC_ASSERT_EQUAL(linphone_proxy_config_address_equal(c,f), LinphoneProxyConfigAddressDifferent, int, "%d");
	BC_ASSERT_EQUAL(linphone_proxy_config_address_equal(e,f), LinphoneProxyConfigAddressWeakEqual, int, "%d");

	linphone_address_unref(a);
	linphone_address_unref(b);
	linphone_address_unref(c);
	linphone_address_unref(d);
	linphone_address_unref(e);
	linphone_address_unref(f);
}

void linphone_proxy_config_is_server_config_changed_test(void) {
	LinphoneProxyConfig* proxy_config = linphone_proxy_config_new();

	linphone_proxy_config_done(proxy_config); /*test done without edit*/

	linphone_proxy_config_set_identity(proxy_config,"sip:toto@titi");
	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_set_identity(proxy_config,"sips:toto@titi");
	BC_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressDifferent, int, "%d");

	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org");
	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_set_server_addr(proxy_config,"sip:toto.com");
	BC_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressDifferent, int, "%d");

	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org");
	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org:4444");
	BC_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressDifferent, int, "%d");

	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org");
	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org;transport=tcp");
	BC_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressDifferent, int, "%d");

	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org");
	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org;param=blue");
	BC_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressWeakEqual, int, "%d");


	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_set_contact_parameters(proxy_config,"blabla=blue");
	BC_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressEqual, int, "%d");

	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_enable_register(proxy_config,TRUE);
	BC_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressEqual, int, "%d");

	linphone_proxy_config_destroy(proxy_config);
}

static void chat_room_test(void) {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	memset (&v_table,0,sizeof(v_table));
	lc = linphone_core_new(&v_table,NULL,NULL,NULL);
	if (!BC_ASSERT_PTR_NOT_NULL(lc)) return;
	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room_from_uri(lc,"sip:toto@titi.com"));
	linphone_core_destroy(lc);
}

static void devices_reload_test(void) {
	char *devid1;
	char *devid2;
	LinphoneCoreManager *mgr = linphone_core_manager_new2("empty_rc", FALSE);

	devid1 = ms_strdup(linphone_core_get_capture_device(mgr->lc));
	linphone_core_reload_sound_devices(mgr->lc);
	devid2 = ms_strdup(linphone_core_get_capture_device(mgr->lc));
	BC_ASSERT_STRING_EQUAL(devid1, devid2);
	ms_free(devid1);
	ms_free(devid2);

	devid1 = ms_strdup(linphone_core_get_video_device(mgr->lc));
	linphone_core_reload_video_devices(mgr->lc);
	devid2 = ms_strdup(linphone_core_get_video_device(mgr->lc));

	if (devid1 && devid2) {
		BC_ASSERT_STRING_EQUAL(devid1, devid2);
	} else {
		BC_ASSERT_PTR_NULL(devid1);
		BC_ASSERT_PTR_NULL(devid2);
	}
	ms_free(devid1);
	ms_free(devid2);

	linphone_core_manager_destroy(mgr);
}

static void codec_usability_test(void) {
	LinphoneCoreManager *mgr = linphone_core_manager_new2("empty_rc", FALSE);
	PayloadType *pt = linphone_core_find_payload_type(mgr->lc, "PCMU", 8000, -1);

	BC_ASSERT_PTR_NOT_NULL(pt);
	if (!pt) goto end;
	/*no limit*/
	linphone_core_set_upload_bandwidth(mgr->lc, 0);
	linphone_core_set_download_bandwidth(mgr->lc, 0);
	BC_ASSERT_TRUE(linphone_core_check_payload_type_usability(mgr->lc, pt));
	/*low limit*/
	linphone_core_set_upload_bandwidth(mgr->lc, 50);
	linphone_core_set_download_bandwidth(mgr->lc, 50);
	BC_ASSERT_FALSE(linphone_core_check_payload_type_usability(mgr->lc, pt));

	/*reasonable limit*/
	linphone_core_set_upload_bandwidth(mgr->lc, 200);
	linphone_core_set_download_bandwidth(mgr->lc, 200);
	BC_ASSERT_TRUE(linphone_core_check_payload_type_usability(mgr->lc, pt));

end:
	linphone_core_manager_destroy(mgr);
}

/*this test checks default codec list, assuming VP8 and H264 are both supported.
 * - with an empty config, the order must be as expected: VP8 first, H264 second.
 * - with a config that references only H264, VP8 must be added automatically as first codec.
 * - with a config that references only VP8, H264 must be added in second position.
**/
static void codec_setup(void){
	LinphoneCoreManager *mgr = linphone_core_manager_new2("empty_rc", FALSE);
	PayloadType *vp8, *h264;
	const bctbx_list_t *codecs;
	if ((vp8 = linphone_core_find_payload_type(mgr->lc, "VP8", 90000, -1)) == NULL ||
		(h264 = linphone_core_find_payload_type(mgr->lc, "H264", 90000, -1)) == NULL){
		linphone_core_manager_destroy(mgr);
		ms_error("H264 or VP8 not available, test skipped.");
		BC_PASS("H264 or VP8 not available, test skipped.");
		return;
	}
	codecs = linphone_core_get_video_codecs(mgr->lc);
	BC_ASSERT_TRUE(bctbx_list_size(codecs)>=2);
	BC_ASSERT_TRUE(codecs->data == vp8);
	BC_ASSERT_TRUE(codecs->next->data == h264);
	linphone_core_manager_destroy(mgr);
	
	mgr = linphone_core_manager_new2("marie_h264_rc", FALSE);
	vp8 = linphone_core_find_payload_type(mgr->lc, "VP8", 90000, -1);
	h264 = linphone_core_find_payload_type(mgr->lc, "H264", 90000, -1);
	codecs = linphone_core_get_video_codecs(mgr->lc);
	BC_ASSERT_TRUE(bctbx_list_size(codecs)>=2);
	BC_ASSERT_PTR_NOT_NULL(vp8);
	BC_ASSERT_PTR_NOT_NULL(h264);
	BC_ASSERT_TRUE(codecs->data == vp8);
	BC_ASSERT_TRUE(codecs->next->data == h264);
	linphone_core_manager_destroy(mgr);
	
	mgr = linphone_core_manager_new2("marie_rc", FALSE);
	vp8 = linphone_core_find_payload_type(mgr->lc, "VP8", 90000, -1);
	h264 = linphone_core_find_payload_type(mgr->lc, "H264", 90000, -1);
	codecs = linphone_core_get_video_codecs(mgr->lc);
	BC_ASSERT_TRUE(bctbx_list_size(codecs)>=2);
	BC_ASSERT_PTR_NOT_NULL(vp8);
	BC_ASSERT_PTR_NOT_NULL(h264);
	BC_ASSERT_TRUE(codecs->data == vp8);
	BC_ASSERT_TRUE(codecs->next->data == h264);
	linphone_core_manager_destroy(mgr);
	
}

test_t setup_tests[] = {
	TEST_NO_TAG("Version check", linphone_version_test),
	TEST_NO_TAG("Linphone Address", linphone_address_test),
	TEST_NO_TAG("Linphone proxy config address equal (internal api)", linphone_proxy_config_address_equal_test),
	TEST_NO_TAG("Linphone proxy config server address change (internal api)", linphone_proxy_config_is_server_config_changed_test),
	TEST_NO_TAG("Linphone core init/uninit", core_init_test),
	TEST_NO_TAG("Linphone random transport port",core_sip_transport_test),
	TEST_NO_TAG("Linphone interpret url", linphone_interpret_url_test),
	TEST_NO_TAG("LPConfig from buffer", linphone_lpconfig_from_buffer),
	TEST_NO_TAG("LPConfig zero_len value from buffer", linphone_lpconfig_from_buffer_zerolen_value),
	TEST_NO_TAG("LPConfig zero_len value from file", linphone_lpconfig_from_file_zerolen_value),
	TEST_ONE_TAG("LPConfig zero_len value from XML", linphone_lpconfig_from_xml_zerolen_value, "LeaksMemory"),
	TEST_NO_TAG("Chat room", chat_room_test),
	TEST_NO_TAG("Devices reload", devices_reload_test),
	TEST_NO_TAG("Codec usability", codec_usability_test),
	TEST_NO_TAG("Codec setup", codec_setup)
};

test_suite_t setup_test_suite = {"Setup", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
								 sizeof(setup_tests) / sizeof(setup_tests[0]), setup_tests};
