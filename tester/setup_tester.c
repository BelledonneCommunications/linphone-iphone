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


#include "liblinphone_tester.h"
#include "linphone/core.h"
#include "linphone/friend.h"
#include "linphone/friendlist.h"
#include "linphone/lpconfig.h"
#include "linphone/api/c-magic-search.h"
#include "tester_utils.h"

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#define S_SIZE_FRIEND 12
static const unsigned int sSizeFriend = S_SIZE_FRIEND;
static const char *sFriends[S_SIZE_FRIEND] = {
	"sip:charu@sip.test.org",//0
	"sip:charette@sip.example.org",//1
	"sip:allo@sip.example.org",//2
	"sip:hello@sip.example.org",//3
	"sip:hello@sip.test.org",//4
	"sip:marie@sip.example.org",//5
	"sip:laura@sip.example.org",//6
	"sip:loic@sip.example.org",//7
	"sip:laure@sip.test.org",//8
	"sip:loic@sip.test.org",//9
	"sip:+111223344@sip.example.org",//10
	"sip:+33655667788@sip.example.org"//11
};

static void _create_friends_from_tab(LinphoneCore *lc, LinphoneFriendList *list, const char *friends[], const unsigned int size) {
	unsigned int i;
	for (i = 0 ; i < size ; i++) {
		LinphoneFriend *fr = linphone_core_create_friend_with_address(lc, friends[i]);
		linphone_friend_list_add_friend(list, fr);
	}
}

static void _remove_friends_from_list(LinphoneFriendList *list, const char *friends[], const unsigned int size) {
	unsigned int i;
	for (i = 0 ; i < size ; i++) {
		LinphoneFriend *fr = linphone_friend_list_find_friend_by_uri(list, friends[i]);
		if (fr) {
			linphone_friend_list_remove_friend(list, fr);
			linphone_friend_unref(fr);
		}
	}
}

static void _check_friend_result_list(LinphoneCore *lc, const bctbx_list_t *resultList, const unsigned int index, const char* uri, const char* phone) {
	if (index >= bctbx_list_size(resultList)) {
		ms_error("Attempt to access result to an outbound index");
		return;
	}
	const LinphoneSearchResult *sr = bctbx_list_nth_data(resultList, index);
	const LinphoneFriend *lf = linphone_search_result_get_friend(sr);
	if (lf || linphone_search_result_get_address(sr)) {
		const LinphoneAddress *la = (linphone_search_result_get_address(sr)) ?
			linphone_search_result_get_address(sr) : linphone_friend_get_address(lf);
		if (la) {
			char* fa = linphone_address_as_string(la);
			BC_ASSERT_STRING_EQUAL(fa , uri);
			free(fa);
			return;
		} else if (phone) {
			const LinphonePresenceModel *presence = linphone_friend_get_presence_model_for_uri_or_tel(lf, phone);
			if (BC_ASSERT_PTR_NOT_NULL(presence)) {
				char *contact = linphone_presence_model_get_contact(presence);
				BC_ASSERT_STRING_EQUAL(contact, uri);
				free(contact);
				return;
			}
		}
	} else {
		const bctbx_list_t *callLog = linphone_core_get_call_logs(lc);
		const bctbx_list_t *f;
		for (f = callLog ; f != NULL ; f = bctbx_list_next(f)) {
			LinphoneCallLog *log = (LinphoneCallLog*)(f->data);
			const LinphoneAddress *addr = (linphone_call_log_get_dir(log) == LinphoneCallIncoming) ?
			linphone_call_log_get_from_address(log) : linphone_call_log_get_to_address(log);
			if (addr) {
				char *addrUri = linphone_address_as_string_uri_only(addr);
				if (addrUri && strcmp(addrUri, uri) == 0) {
					bctbx_free(addrUri);
					return;
				}
				if (addrUri) bctbx_free(addrUri);
			}
		}
	}
	BC_ASSERT(FALSE);
	ms_error("Address NULL and Presence NULL");
}

static void _create_call_log(LinphoneCore *lc, LinphoneAddress *addrFrom, LinphoneAddress *addrTo) {
	linphone_call_log_unref(
		linphone_core_create_call_log(lc, addrFrom, addrTo, LinphoneCallOutgoing, 100, time(NULL), time(NULL), LinphoneCallSuccess, FALSE, 1.0)
	);
}

static void linphone_version_test(void){
	const char *version=linphone_core_get_version();
	/*make sure the git version is always included in the version number*/
	BC_ASSERT_PTR_NOT_NULL(version);
	BC_ASSERT_PTR_NULL(strstr(version,"unknown"));
}

static void core_init_test(void) {
	LinphoneCore* lc;
	lc = linphone_factory_create_core_2(linphone_factory_get(),NULL,NULL,NULL, NULL, system_context);

	/* until we have good certificates on our test server... */
	linphone_core_verify_server_certificates(lc,FALSE);
	if (BC_ASSERT_PTR_NOT_NULL(lc)) {
		linphone_core_unref(lc);
	}
}

static void linphone_address_test(void) {
	LinphoneAddress *address;

	linphone_address_unref(create_linphone_address(NULL));
	BC_ASSERT_PTR_NULL(linphone_address_new("sip:@sip.linphone.org"));

	address = linphone_address_new("sip:90.110.127.31");
	if (!BC_ASSERT_PTR_NOT_NULL(address)) return;
	linphone_address_unref(address);

	address = linphone_address_new("sip:[::ffff:90.110.127.31]");
	if (!BC_ASSERT_PTR_NOT_NULL(address)) return;
	linphone_address_unref(address);
}

static void core_sip_transport_test(void) {
	LinphoneCore* lc;
	LCSipTransports tr;
	lc = linphone_factory_create_core_2(linphone_factory_get(),NULL,NULL,NULL, NULL, system_context);
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

	linphone_core_unref(lc);
}

static void linphone_interpret_url_test(void) {
	LinphoneCore* lc;
	const char* sips_address = "sips:margaux@sip.linphone.org";
	LinphoneAddress* address;
	LinphoneProxyConfig *proxy_config;
	char *tmp;
	lc = linphone_factory_create_core_2(linphone_factory_get(),NULL,NULL,NULL, NULL, system_context);
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

	linphone_core_unref(lc);
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

	conf = linphone_core_get_config(mgr->lc);

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
	LinphoneCore* lc;
	lc = linphone_factory_create_core_2(linphone_factory_get(),NULL,NULL,NULL, NULL, system_context);
	if (!BC_ASSERT_PTR_NOT_NULL(lc)) return;
	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room_from_uri(lc,"sip:toto@titi.com"));
	linphone_core_unref(lc);
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

static void custom_tones_setup(void){
	LinphoneCoreManager *mgr = linphone_core_manager_new2("empty_rc", FALSE);
	const char *tone;

	linphone_core_set_tone(mgr->lc, LinphoneToneCallOnHold, "callonhold.wav");
	tone = linphone_core_get_tone_file(mgr->lc, LinphoneToneCallOnHold);
	BC_ASSERT_PTR_NOT_NULL(tone);
	if (tone){
		BC_ASSERT_STRING_EQUAL(tone, "callonhold.wav");
	}
	linphone_core_set_tone(mgr->lc, LinphoneToneCallOnHold, "callonhold2.wav");
	tone = linphone_core_get_tone_file(mgr->lc, LinphoneToneCallOnHold);
	BC_ASSERT_PTR_NOT_NULL(tone);
	if (tone){
		BC_ASSERT_STRING_EQUAL(tone, "callonhold2.wav");
	}
	linphone_core_manager_destroy(mgr);
}

static void search_friend_in_alphabetical_order(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);

	const char *name1SipUri = {"sip:toto@sip.example.org"};
	const char *name2SipUri = {"sip:stephanie@sip.example.org"};
	const char *name3SipUri = {"sip:alber@sip.example.org"};
	const char *name4SipUri = {"sip:gauthier@sip.example.org"};
	const char *name5SipUri = {"sip:gal@sip.example.org"};

	LinphoneFriend *friend1 = linphone_core_create_friend(manager->lc);
	LinphoneFriend *friend2 = linphone_core_create_friend(manager->lc);
	LinphoneFriend *friend3 = linphone_core_create_friend(manager->lc);
	LinphoneFriend *friend4 = linphone_core_create_friend(manager->lc);
	LinphoneFriend *friend5 = linphone_core_create_friend(manager->lc);

	LinphoneVcard *vcard1 = linphone_factory_create_vcard(linphone_factory_get());
	LinphoneVcard *vcard2 = linphone_factory_create_vcard(linphone_factory_get());
	LinphoneVcard *vcard3 = linphone_factory_create_vcard(linphone_factory_get());
	LinphoneVcard *vcard4 = linphone_factory_create_vcard(linphone_factory_get());
	LinphoneVcard *vcard5 = linphone_factory_create_vcard(linphone_factory_get());

	const char *name1 = {"STEPHANIE delarue"};
	const char *name2 = {"alias delarue"};
	const char *name3 = {"Alber josh"};
	const char *name4 = {"gauthier wei"};
	const char *name5 = {"gal tcho"};

	linphone_vcard_set_full_name(vcard1, name1); // STEPHANIE delarue
	linphone_vcard_set_url(vcard1, name1SipUri); //sip:toto@sip.example.org
	linphone_vcard_add_sip_address(vcard1, name1SipUri);
	linphone_friend_set_vcard(friend1, vcard1);
	linphone_core_add_friend(manager->lc, friend1);

	linphone_vcard_set_full_name(vcard2, name2); // alias delarue
	linphone_vcard_set_url(vcard2, name2SipUri); //sip:stephanie@sip.example.org
	linphone_vcard_add_sip_address(vcard2, name2SipUri);
	linphone_friend_set_vcard(friend2, vcard2);
	linphone_core_add_friend(manager->lc, friend2);

	linphone_vcard_set_full_name(vcard3, name3); // Alber josh
	linphone_vcard_set_url(vcard3, name3SipUri); //sip:alber@sip.example.org
	linphone_vcard_add_sip_address(vcard3, name3SipUri);
	linphone_friend_set_vcard(friend3, vcard3);
	linphone_core_add_friend(manager->lc, friend3);

	linphone_vcard_set_full_name(vcard4, name4); // gauthier wei
	linphone_vcard_set_url(vcard4, name4SipUri); //sip:gauthier@sip.example.org
	linphone_vcard_add_sip_address(vcard4, name4SipUri);
	linphone_friend_set_vcard(friend4, vcard4);
	linphone_core_add_friend(manager->lc, friend4);

	linphone_vcard_set_full_name(vcard5, name5); // gal tcho
	linphone_vcard_set_url(vcard5, name5SipUri); //sip:gal@sip.example.org
	linphone_vcard_add_sip_address(vcard5, name5SipUri);
	linphone_friend_set_vcard(friend5, vcard5);
	linphone_core_add_friend(manager->lc, friend5);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 5, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, name3SipUri, NULL);//"sip:stephanie@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, name2SipUri, NULL);//"sip:alber@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 2, name5SipUri, NULL);//"sip:gal@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 3, name4SipUri, NULL);//"sip:gauthier@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 4, name1SipUri, NULL);//"sip:toto@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	linphone_magic_search_reset_search_cache(magicSearch);

	linphone_friend_list_remove_friend(lfl, friend1);
	linphone_friend_list_remove_friend(lfl, friend2);
	linphone_friend_list_remove_friend(lfl, friend3);
	linphone_friend_list_remove_friend(lfl, friend4);
	linphone_friend_list_remove_friend(lfl, friend5);

	if (friend1) linphone_friend_unref(friend1);
	if (friend2) linphone_friend_unref(friend2);
	if (friend3) linphone_friend_unref(friend3);
	if (friend4) linphone_friend_unref(friend4);
	if (friend5) linphone_friend_unref(friend5);

	if (vcard1) linphone_vcard_unref(vcard1);
	if (vcard2) linphone_vcard_unref(vcard2);
	if (vcard3) linphone_vcard_unref(vcard3);
	if (vcard4) linphone_vcard_unref(vcard4);
	if (vcard5) linphone_vcard_unref(vcard5);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);

}

static void search_friend_without_filter(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), S_SIZE_FRIEND, int, "%d");
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_with_domain_without_filter(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("marie_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);
	const char *chloeName = "chloe zaya";
	const char *chloeSipUri = "sip:ch@sip.test.org";
	const char *chloePhoneNumber = "0633556644";
	LinphoneFriend *chloeFriend = linphone_core_create_friend(manager->lc);
	LinphonePresenceModel *chloePresence = linphone_core_create_presence_model(manager->lc);
	LinphoneProxyConfig *proxy = linphone_core_get_default_proxy_config(manager->lc);

	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_set_dial_prefix(proxy, "33");
	linphone_proxy_config_done(proxy);
	linphone_core_set_default_proxy(manager->lc, proxy);

	linphone_presence_model_set_contact(chloePresence, chloeSipUri);
	linphone_friend_set_name(chloeFriend, chloeName);
	linphone_friend_add_phone_number(chloeFriend, chloePhoneNumber);
	linphone_friend_set_presence_model_for_uri_or_tel(chloeFriend, chloePhoneNumber, chloePresence);
	linphone_friend_list_add_friend(lfl, chloeFriend);

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "", "sip.test.org");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 5, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[0], NULL);//"sip:charu@sip.test.org"
		_check_friend_result_list(manager->lc, resultList, 1, chloeSipUri, chloePhoneNumber);//"sip:ch@sip.test.org"
		_check_friend_result_list(manager->lc, resultList, 2, sFriends[4], NULL);//"sip:hello@sip.test.org"
		_check_friend_result_list(manager->lc, resultList, 3, sFriends[8], NULL);//"sip:laure@sip.test.org"
		_check_friend_result_list(manager->lc, resultList, 4, sFriends[9], NULL);//"sip:loic@sip.test.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);

	LinphoneFriend *fr = linphone_friend_list_find_friend_by_uri(lfl, chloeSipUri);
	linphone_friend_list_remove_friend(lfl, fr);

	if (chloeFriend) linphone_friend_unref(chloeFriend);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_all_domains(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "llo", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 3, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[2], NULL);//"sip:allo@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[3], NULL);//"sip:hello@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 2, sFriends[4], NULL);//"sip:hello@sip.test.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_one_domain(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "llo", "sip.example.org");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[2], NULL);//"sip:allo@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[3], NULL);//"sip:hello@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_research_estate(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "l", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 7, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[6], NULL);//"sip:laura@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[7], NULL);//"sip:loic@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 2, sFriends[8], NULL);//"sip:laure@sip.test.org"
		_check_friend_result_list(manager->lc, resultList, 3, sFriends[9], NULL);//"sip:loic@sip.test.org"
		_check_friend_result_list(manager->lc, resultList, 4, sFriends[2], NULL);//"sip:allo@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 5, sFriends[3], NULL);//"sip:hello@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 6, sFriends[4], NULL);//"sip:hello@sip.test.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "la", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[8], NULL);//"sip:laure@sip.test.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[6], NULL);//"sip:laura@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_research_estate_reset(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "la", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[6], NULL);//"sip:laura@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[8], NULL);//"sip:laure@sip.test.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	linphone_magic_search_reset_search_cache(magicSearch);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "l", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 7, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[6], NULL);//"sip:laura@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[7], NULL);//"sip:loic@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 2, sFriends[8], NULL);//"sip:laure@sip.test.org"
		_check_friend_result_list(manager->lc, resultList, 3, sFriends[9], NULL);//"sip:loic@sip.test.org"
		_check_friend_result_list(manager->lc, resultList, 4, sFriends[2], NULL);//"sip:allo@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 5, sFriends[3], NULL);//"sip:hello@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 6, sFriends[4], NULL);//"sip:hello@sip.test.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_with_phone_number(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);
	LinphoneFriend *stephanieFriend = linphone_core_create_friend(manager->lc);
	LinphoneVcard *stephanieVcard = linphone_factory_create_vcard(linphone_factory_get());
	const char* stephanieName = {"stephanie de monaco"};
	const char* mariePhoneNumber = {"0633556644"};
	const char* stephaniePhoneNumber = {"0633889977"};

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	linphone_vcard_set_full_name(stephanieVcard, stephanieName); // stephanie de monaco
	linphone_vcard_add_phone_number(stephanieVcard, stephaniePhoneNumber);
	linphone_friend_set_vcard(stephanieFriend, stephanieVcard);
	linphone_core_add_friend(manager->lc, stephanieFriend);

	linphone_friend_add_phone_number(linphone_friend_list_find_friend_by_uri(lfl, sFriends[5]), mariePhoneNumber);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "33", "*");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 3, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[11], NULL);//"sip:+33655667788@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[10], NULL);//"sip:+111223344@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 2, sFriends[5], NULL);//"sip:marie@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	linphone_magic_search_reset_search_cache(magicSearch);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "5", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[11], NULL);//"sip:+33655667788@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[5], NULL);//"sip:marie@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "55", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[11], NULL);//"sip:+33655667788@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[5], NULL);//"sip:marie@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "556", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[11], NULL);//"sip:+33655667788@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[5], NULL);//"sip:marie@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "5566", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[11], NULL);//"sip:+33655667788@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[5], NULL);//"sip:marie@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "55667", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 1, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[11], NULL);//"sip:+33655667788@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "55667", "sip.test.org");

	BC_ASSERT_PTR_NULL(resultList);

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);

	linphone_friend_list_remove_friend(lfl, stephanieFriend);
	if (stephanieFriend) linphone_friend_unref(stephanieFriend);
	if (stephanieVcard) linphone_vcard_unref(stephanieVcard);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_with_presence(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_create("marie_rc");
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);
	const char *chloeName = "chloe zaya";
	const char *chloeSipUri = "sip:ch@sip.example.org";
	const char *chloePhoneNumber = "0633556644";
	LinphoneFriend *chloeFriend = linphone_core_create_friend(manager->lc);
	LinphonePresenceModel *chloePresence = linphone_core_create_presence_model(manager->lc);
	LinphoneProxyConfig *proxy = linphone_core_get_default_proxy_config(manager->lc);

	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_set_dial_prefix(proxy, "33");
	linphone_proxy_config_done(proxy);
	linphone_core_set_default_proxy(manager->lc, proxy);

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);
	linphone_presence_model_set_contact(chloePresence, chloeSipUri);
	linphone_friend_set_name(chloeFriend, chloeName);
	linphone_friend_add_phone_number(chloeFriend, chloePhoneNumber);
	linphone_friend_set_presence_model_for_uri_or_tel(chloeFriend, chloePhoneNumber, chloePresence);
	linphone_friend_list_add_friend(lfl, chloeFriend);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "33", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 4, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[11], NULL);//"sip:+33655667788@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[10], NULL);//"sip:+111223344@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 2, chloeSipUri, chloePhoneNumber);//"sip:ch@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 3, "sip:33@sip.example.org", NULL);//"sip:33@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	linphone_magic_search_reset_search_cache(magicSearch);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "chloe", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, chloeSipUri, chloePhoneNumber);//"sip:ch@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, "sip:chloe@sip.example.org", NULL);//"sip:chloe@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);

	LinphoneFriend *fr = linphone_friend_list_find_friend_by_uri(lfl, chloeSipUri);
	linphone_friend_list_remove_friend(lfl, fr);

	if (chloeFriend) linphone_friend_unref(chloeFriend);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_in_call_log(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);
	const char *chloeSipUri = {"sip:chloe@sip.example.org"};
	const char *benjaminSipUri = {"sip:benjamin@sip.example.org"};
	const char *charlesSipUri = {"sip:charles@sip.test.org"};
	const char *ronanSipUri = {"sip:ronan@sip.example.org"};
	LinphoneAddress *chloeAddress = linphone_address_new(chloeSipUri);
	LinphoneAddress *benjaminAddress = linphone_address_new(benjaminSipUri);
	LinphoneAddress *charlesAddress = linphone_address_new(charlesSipUri);
	LinphoneAddress *ronanAddress = linphone_address_new(ronanSipUri);

	_create_call_log(manager->lc, ronanAddress, chloeAddress);
	_create_call_log(manager->lc, ronanAddress, charlesAddress);
	_create_call_log(manager->lc, ronanAddress, benjaminAddress);

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "ch", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 4, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[0], NULL);//"sip:charu@sip.test.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[1], NULL);//"sip:charette@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 2, chloeSipUri, NULL);//"sip:chloe@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 3, charlesSipUri, NULL);//"sip:charles@sip.test.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	linphone_magic_search_reset_search_cache(magicSearch);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "ch", "sip.test.org");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, sFriends[0], NULL);//"sip:charu@sip.test.org"
		_check_friend_result_list(manager->lc, resultList, 1, charlesSipUri, NULL);//"sip:charles@sip.test.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);

	if (chloeAddress) linphone_address_unref(chloeAddress);
	if (benjaminAddress) linphone_address_unref(benjaminAddress);
	if (charlesAddress) linphone_address_unref(charlesAddress);
	if (ronanAddress) linphone_address_unref(ronanAddress);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_in_call_log_already_exist(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("marie_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);
	const char *ronanSipUri = {"sip:ronan@sip.example.org"};
	const char *chloeName = "chloe zaya";
	const char *chloeSipUri = "sip:chloe@sip.example.org";
	const char *chloePhoneNumber = "0633556644";
	LinphoneFriend *chloeFriend = linphone_core_create_friend(manager->lc);
	LinphonePresenceModel *chloePresence = linphone_core_create_presence_model(manager->lc);
	LinphoneProxyConfig *proxy = linphone_core_get_default_proxy_config(manager->lc);
	LinphoneAddress *ronanAddress = linphone_address_new(ronanSipUri);
	LinphoneAddress *chloeAddress = linphone_address_new(chloeSipUri);

	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_set_dial_prefix(proxy, "33");
	linphone_proxy_config_done(proxy);
	linphone_core_set_default_proxy(manager->lc, proxy);

	linphone_presence_model_set_contact(chloePresence, chloeSipUri);
	linphone_friend_set_name(chloeFriend, chloeName);
	linphone_friend_set_address(chloeFriend, chloeAddress);
	linphone_friend_add_phone_number(chloeFriend, chloePhoneNumber);
	linphone_friend_set_presence_model_for_uri_or_tel(chloeFriend, chloePhoneNumber, chloePresence);
	linphone_friend_list_add_friend(lfl, chloeFriend);

	_create_call_log(manager->lc, ronanAddress, chloeAddress);

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "ch", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 5, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, chloeSipUri, NULL);//"sip:chloe@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, sFriends[0], NULL);//"sip:charu@sip.test.org"
		_check_friend_result_list(manager->lc, resultList, 2, sFriends[1], NULL);//"sip:charette@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 3, "sip:pauline@sip.example.org", NULL);//In the linphonerc "sip:pauline@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 4,"sip:ch@sip.example.org", NULL);//"sip:ch@sip.example.org"
		const LinphoneSearchResult *sr = bctbx_list_nth_data(resultList, 0);
		if (BC_ASSERT_PTR_NOT_NULL(sr)) {
			const LinphoneFriend *lf = linphone_search_result_get_friend(sr);
			BC_ASSERT_PTR_NOT_NULL(lf);
		}
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);

	LinphoneFriend *fr = linphone_friend_list_find_friend_by_uri(lfl, chloeSipUri);
	linphone_friend_list_remove_friend(lfl, fr);

	if (chloeFriend) linphone_friend_unref(chloeFriend);

	if (chloeAddress) linphone_address_unref(chloeAddress);
	if (ronanAddress) linphone_address_unref(ronanAddress);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_last_item_is_filter(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_create("marie_rc");
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "newaddress", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 1, int, "%d");
		const LinphoneSearchResult *sr = bctbx_list_nth_data(resultList, 0);
		if (BC_ASSERT_PTR_NOT_NULL(sr)) {
			const LinphoneAddress *srAddress = linphone_search_result_get_address(sr);
			if (BC_ASSERT_PTR_NOT_NULL(srAddress)) {
				BC_ASSERT_STRING_EQUAL(linphone_address_get_username(srAddress), "newaddress");
			}
		}
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_with_name(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);
	const char *stephanie1SipUri = {"sip:toto@sip.example.org"};
	const char *stephanie2SipUri = {"sip:stephanie@sip.example.org"};
	LinphoneFriend *stephanie1Friend = linphone_core_create_friend(manager->lc);
	LinphoneFriend *stephanie2Friend = linphone_core_create_friend(manager->lc);
	LinphoneVcard *stephanie1Vcard = linphone_factory_create_vcard(linphone_factory_get());
	LinphoneVcard *stephanie2Vcard = linphone_factory_create_vcard(linphone_factory_get());
	const char *stephanie1Name = {"stephanie delarue"};
	const char *stephanie2Name = {"alias delarue"};

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	linphone_vcard_set_full_name(stephanie1Vcard, stephanie1Name); // stephanie delarue
	linphone_vcard_set_url(stephanie1Vcard, stephanie1SipUri); //sip:toto@sip.example.org
	linphone_vcard_add_sip_address(stephanie1Vcard, stephanie1SipUri);
	linphone_friend_set_vcard(stephanie1Friend, stephanie1Vcard);
	linphone_core_add_friend(manager->lc, stephanie1Friend);

	linphone_vcard_set_full_name(stephanie2Vcard, stephanie2Name); // alias delarue
	linphone_vcard_set_url(stephanie2Vcard, stephanie2SipUri); //sip:stephanie@sip.example.org
	linphone_vcard_add_sip_address(stephanie2Vcard, stephanie2SipUri);
	linphone_friend_set_vcard(stephanie2Friend, stephanie2Vcard);
	linphone_core_add_friend(manager->lc, stephanie2Friend);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "stephanie", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, stephanie1SipUri, NULL);//"sip:toto@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, stephanie2SipUri, NULL);//"sip:stephanie@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	linphone_magic_search_reset_search_cache(magicSearch);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "delarue", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, stephanie2SipUri, NULL);//"sip:stephanie@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, stephanie1SipUri, NULL);//"sip:toto@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);
	linphone_friend_list_remove_friend(lfl, stephanie1Friend);
	linphone_friend_list_remove_friend(lfl, stephanie2Friend);
	if (stephanie1Friend) linphone_friend_unref(stephanie1Friend);
	if (stephanie2Friend) linphone_friend_unref(stephanie2Friend);
	if (stephanie1Vcard) linphone_vcard_unref(stephanie1Vcard);
	if (stephanie2Vcard) linphone_vcard_unref(stephanie2Vcard);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_with_name_with_uppercase(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);
	const char *stephanie1SipUri = {"sip:toto@sip.example.org"};
	const char *stephanie2SipUri = {"sip:stephanie@sip.example.org"};
	LinphoneFriend *stephanie1Friend = linphone_core_create_friend(manager->lc);
	LinphoneFriend *stephanie2Friend = linphone_core_create_friend(manager->lc);
	LinphoneVcard *stephanie1Vcard = linphone_factory_create_vcard(linphone_factory_get());
	LinphoneVcard *stephanie2Vcard = linphone_factory_create_vcard(linphone_factory_get());
	const char *stephanie1Name = {"STEPHANIE delarue"};
	const char *stephanie2Name = {"alias delarue"};

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	linphone_vcard_set_full_name(stephanie1Vcard, stephanie1Name); // STEPHANIE delarue
	linphone_vcard_set_url(stephanie1Vcard, stephanie1SipUri); //sip:toto@sip.example.org
	linphone_vcard_add_sip_address(stephanie1Vcard, stephanie1SipUri);
	linphone_friend_set_vcard(stephanie1Friend, stephanie1Vcard);
	linphone_core_add_friend(manager->lc, stephanie1Friend);

	linphone_vcard_set_full_name(stephanie2Vcard, stephanie2Name); // alias delarue
	linphone_vcard_set_url(stephanie2Vcard, stephanie2SipUri); //sip:stephanie@sip.example.org
	linphone_vcard_add_sip_address(stephanie2Vcard, stephanie2SipUri);
	linphone_friend_set_vcard(stephanie2Friend, stephanie2Vcard);
	linphone_core_add_friend(manager->lc, stephanie2Friend);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "stephanie", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, stephanie1SipUri, NULL);//"sip:toto@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, stephanie2SipUri, NULL);//"sip:stephanie@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	linphone_magic_search_reset_search_cache(magicSearch);

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);
	linphone_friend_list_remove_friend(lfl, stephanie1Friend);
	linphone_friend_list_remove_friend(lfl, stephanie2Friend);
	if (stephanie1Friend) linphone_friend_unref(stephanie1Friend);
	if (stephanie2Friend) linphone_friend_unref(stephanie2Friend);
	if (stephanie1Vcard) linphone_vcard_unref(stephanie1Vcard);
	if (stephanie2Vcard) linphone_vcard_unref(stephanie2Vcard);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_with_multiple_sip_address(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);
	const char *stephanieSipUri1 = {"sip:toto@sip.example.org"};
	const char *stephanieSipUri2 = {"sip:stephanie@sip.example.org"};
	LinphoneFriend *stephanieFriend = linphone_core_create_friend(manager->lc);
	LinphoneVcard *stephanieVcard = linphone_factory_create_vcard(linphone_factory_get());
	const char *stephanieName = {"stephanie delarue"};

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	linphone_vcard_set_full_name(stephanieVcard, stephanieName); // stephanie delarue
	linphone_vcard_set_url(stephanieVcard, stephanieSipUri1); //sip:toto@sip.example.org
	linphone_vcard_add_sip_address(stephanieVcard, stephanieSipUri1);
	linphone_vcard_add_sip_address(stephanieVcard, stephanieSipUri2);
	linphone_friend_set_vcard(stephanieFriend, stephanieVcard);
	linphone_core_add_friend(manager->lc, stephanieFriend);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "stephanie", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, stephanieSipUri2, NULL);//"sip:stephanie@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, stephanieSipUri1, NULL);//"sip:toto@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	linphone_magic_search_reset_search_cache(magicSearch);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "delarue", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 2, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, stephanieSipUri2, NULL);//"sip:stephanie@sip.example.org"
		_check_friend_result_list(manager->lc, resultList, 1, stephanieSipUri1, NULL);//"sip:toto@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);
	linphone_friend_list_remove_friend(lfl, stephanieFriend);
	if (stephanieFriend) linphone_friend_unref(stephanieFriend);
	if (stephanieVcard) linphone_vcard_unref(stephanieVcard);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_with_same_address(void) {
	LinphoneMagicSearch *magicSearch = NULL;
	bctbx_list_t *resultList = NULL;
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	LinphoneFriendList *lfl = linphone_core_get_default_friend_list(manager->lc);
	const char *stephanieSipUri = {"sip:stephanie@sip.example.org"};
	LinphoneFriend *stephanieFriend1 = linphone_core_create_friend(manager->lc);
	LinphoneFriend *stephanieFriend2 = linphone_core_create_friend(manager->lc);
	LinphoneVcard *stephanieVcard1 = linphone_factory_create_vcard(linphone_factory_get());
	LinphoneVcard *stephanieVcard2 = linphone_factory_create_vcard(linphone_factory_get());
	const char *stephanieName = {"stephanie delarue"};

	_create_friends_from_tab(manager->lc, lfl, sFriends, sSizeFriend);

	linphone_vcard_set_full_name(stephanieVcard1, stephanieName); // stephanie delarue
	linphone_vcard_set_url(stephanieVcard1, stephanieSipUri); //sip:stephanie@sip.example.org
	linphone_vcard_add_sip_address(stephanieVcard1, stephanieSipUri);
	linphone_friend_set_vcard(stephanieFriend1, stephanieVcard1);
	linphone_core_add_friend(manager->lc, stephanieFriend1);

	linphone_vcard_set_full_name(stephanieVcard2, stephanieName); // stephanie delarue
	linphone_vcard_set_url(stephanieVcard2, stephanieSipUri); //sip:stephanie@sip.example.org
	linphone_vcard_add_sip_address(stephanieVcard2, stephanieSipUri);
	linphone_friend_set_vcard(stephanieFriend2, stephanieVcard2);
	linphone_core_add_friend(manager->lc, stephanieFriend2);

	magicSearch = linphone_magic_search_new(manager->lc);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "stephanie", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 1, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, stephanieSipUri, NULL);//"sip:stephanie@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	linphone_magic_search_reset_search_cache(magicSearch);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "delarue", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), 1, int, "%d");
		_check_friend_result_list(manager->lc, resultList, 0, stephanieSipUri, NULL);//"sip:stephanie@sip.example.org"
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	linphone_magic_search_reset_search_cache(magicSearch);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "", "");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), S_SIZE_FRIEND+1, int, "%d");
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	linphone_magic_search_reset_search_cache(magicSearch);

	resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, "", "*");

	if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
		BC_ASSERT_EQUAL(bctbx_list_size(resultList), S_SIZE_FRIEND+1, int, "%d");
		bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
	}

	_remove_friends_from_list(lfl, sFriends, sSizeFriend);
	linphone_friend_list_remove_friend(lfl, stephanieFriend1);
	linphone_friend_list_remove_friend(lfl, stephanieFriend2);
	if (stephanieFriend1) linphone_friend_unref(stephanieFriend1);
	if (stephanieFriend2) linphone_friend_unref(stephanieFriend2);
	if (stephanieVcard1) linphone_vcard_unref(stephanieVcard1);
	if (stephanieVcard2) linphone_vcard_unref(stephanieVcard2);

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
}

static void search_friend_large_database(void) {
	char *dbPath = bc_tester_res("db/friends.db");
	char *searchedFriend = "6295103032641994169";
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	linphone_core_set_friends_database_path(manager->lc, dbPath);
	LinphoneMagicSearch *magicSearch = linphone_magic_search_new(manager->lc);

	for (size_t i = 1; i < strlen(searchedFriend) ; i++) {
		MSTimeSpec start, current;
		char subBuff[20];
		memcpy(subBuff, searchedFriend, i);
		subBuff[i] = '\0';
		liblinphone_tester_clock_start(&start);
		bctbx_list_t *resultList = linphone_magic_search_get_contact_list_from_filter(magicSearch, subBuff, "");
		if (BC_ASSERT_PTR_NOT_NULL(resultList)) {
			long long time;
			ms_get_cur_time(&current);
			time = ((current.tv_sec - start.tv_sec) * 1000LL) + ((current.tv_nsec - start.tv_nsec) / 1000000LL);
			ms_message("Searching time: %lld ms", time);
			BC_ASSERT_LOWER(time, 10000, long long, "%lld");
			ms_message("List size: %zu", bctbx_list_size(resultList));
			bctbx_list_free_with_data(resultList, (bctbx_list_free_func)linphone_magic_search_unref);
		}
	}

	linphone_magic_search_unref(magicSearch);
	linphone_core_manager_destroy(manager);
	free(dbPath);
}


/*the webrtc AEC implementation is brought to mediastreamer2 by a plugin.
 * We finally check here that if the plugin is correctly loaded and the right choice of echo canceller implementation is made*/
static void echo_canceller_check(void){
	LinphoneCoreManager* manager = linphone_core_manager_new2("empty_rc", FALSE);
	MSFactory *factory = linphone_core_get_ms_factory(manager->lc);
	const char *expected_filter = "MSSpeexEC";
	AudioStream *as = audio_stream_new2(factory, NULL, 43000, 43001);
	const char *ec_filter = NULL;
	
	BC_ASSERT_PTR_NOT_NULL(as);
	if (as){
		MSFilter *ecf = as->ec;
		BC_ASSERT_PTR_NOT_NULL(ecf);
		if (ecf){
			ec_filter = ecf->desc->name;
		}
	}
	BC_ASSERT_PTR_NOT_NULL(ec_filter);

#if defined(__linux) || (defined(__APPLE__) && !TARGET_OS_IPHONE) || defined(_WIN32)
	expected_filter = "MSWebRTCAEC";
#elif defined(__ANDROID__)
	expected_filter = "MSWebRTCAECM";
#endif
	if (ec_filter){
		BC_ASSERT_STRING_EQUAL(ec_filter, expected_filter);
	}
	audio_stream_stop(as);
	linphone_core_manager_destroy(manager);
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
	TEST_NO_TAG("LPConfig zero_len value from XML", linphone_lpconfig_from_xml_zerolen_value),
	TEST_NO_TAG("Chat room", chat_room_test),
	TEST_NO_TAG("Devices reload", devices_reload_test),
	TEST_NO_TAG("Codec usability", codec_usability_test),
	TEST_NO_TAG("Codec setup", codec_setup),
	TEST_NO_TAG("Custom tones setup", custom_tones_setup),
	TEST_NO_TAG("Appropriate software echo canceller check", echo_canceller_check),
	TEST_ONE_TAG("Return friend list in alphabetical order", search_friend_in_alphabetical_order, "MagicSearch"),
	TEST_ONE_TAG("Search friend without filter and domain", search_friend_without_filter, "MagicSearch"),
	TEST_ONE_TAG("Search friend with domain and without filter", search_friend_with_domain_without_filter, "MagicSearch"),
	TEST_ONE_TAG("Search friend from all domains", search_friend_all_domains, "MagicSearch"),
	TEST_ONE_TAG("Search friend from one domain", search_friend_one_domain, "MagicSearch"),
	TEST_ONE_TAG("Multiple looking for friends with the same cache", search_friend_research_estate, "MagicSearch"),
	TEST_ONE_TAG("Multiple looking for friends with cache resetting", search_friend_research_estate_reset, "MagicSearch"),
	TEST_ONE_TAG("Search friend with phone number", search_friend_with_phone_number, "MagicSearch"),
	TEST_ONE_TAG("Search friend and find it with its presence", search_friend_with_presence, "MagicSearch"),
	TEST_ONE_TAG("Search friend in call log", search_friend_in_call_log, "MagicSearch"),
	TEST_ONE_TAG("Search friend in call log but don't add address which already exist", search_friend_in_call_log_already_exist, "MagicSearch"),
	TEST_ONE_TAG("Search friend last item is the filter", search_friend_last_item_is_filter, "MagicSearch"),
	TEST_ONE_TAG("Search friend with name", search_friend_with_name, "MagicSearch"),
	TEST_ONE_TAG("Search friend with uppercase name", search_friend_with_name_with_uppercase, "MagicSearch"),
	TEST_ONE_TAG("Search friend with multiple sip address", search_friend_with_multiple_sip_address, "MagicSearch"),
	TEST_ONE_TAG("Search friend with same address", search_friend_with_same_address, "MagicSearch"),
	TEST_ONE_TAG("Search friend in large friends database", search_friend_large_database, "MagicSearch")
};

test_suite_t setup_test_suite = {"Setup", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
								 sizeof(setup_tests) / sizeof(setup_tests[0]), setup_tests};
