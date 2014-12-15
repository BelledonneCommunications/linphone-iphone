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

#include <stdio.h>
#include "CUnit/Basic.h"
#include "linphonecore.h"
#include "liblinphone_tester.h"
#include "lpconfig.h"
#include "private.h"

static void linphone_version_test(void){
	const char *version=linphone_core_get_version();
	/*make sure the git version is always included in the version number*/
	CU_ASSERT_TRUE(strstr(version,"unknown")==NULL);
}

static void core_init_test(void) {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	memset (&v_table,0,sizeof(v_table));
	lc = linphone_core_new(&v_table,NULL,NULL,NULL);
	/* until we have good certificates on our test server... */
	linphone_core_verify_server_certificates(lc,FALSE);
	CU_ASSERT_PTR_NOT_NULL_FATAL(lc);
	linphone_core_destroy(lc);
}

static void linphone_address_test(void) {
	linphone_address_destroy(create_linphone_address(NULL));
}

static void core_sip_transport_test(void) {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	LCSipTransports tr;
	memset (&v_table,0,sizeof(v_table));
	lc = linphone_core_new(&v_table,NULL,NULL,NULL);
	CU_ASSERT_PTR_NOT_NULL_FATAL(lc);
	linphone_core_get_sip_transports(lc,&tr);
	CU_ASSERT_EQUAL(tr.udp_port,5060); /*default config*/
	CU_ASSERT_EQUAL(tr.tcp_port,5060); /*default config*/

	tr.udp_port=LC_SIP_TRANSPORT_RANDOM;
	tr.tcp_port=LC_SIP_TRANSPORT_RANDOM;
	tr.tls_port=LC_SIP_TRANSPORT_RANDOM;

	linphone_core_set_sip_transports(lc,&tr);
	linphone_core_get_sip_transports(lc,&tr);

	CU_ASSERT_NOT_EQUAL(tr.udp_port,5060); /*default config*/
	CU_ASSERT_NOT_EQUAL(tr.tcp_port,5060); /*default config*/

	CU_ASSERT_EQUAL(lp_config_get_int(linphone_core_get_config(lc),"sip","sip_port",-2),LC_SIP_TRANSPORT_RANDOM);
	CU_ASSERT_EQUAL(lp_config_get_int(linphone_core_get_config(lc),"sip","sip_tcp_port",-2),LC_SIP_TRANSPORT_RANDOM);
	CU_ASSERT_EQUAL(lp_config_get_int(linphone_core_get_config(lc),"sip","sip_tls_port",-2),LC_SIP_TRANSPORT_RANDOM);

	linphone_core_destroy(lc);
}

static void linphone_interpret_url_test()
{
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	const char* sips_address = "sips:margaux@sip.linphone.org";
	LinphoneAddress* address;

	memset ( &v_table,0,sizeof ( v_table ) );
	lc = linphone_core_new ( &v_table,NULL,NULL,NULL );
	CU_ASSERT_PTR_NOT_NULL_FATAL ( lc );

	address = linphone_core_interpret_url(lc, sips_address);

	CU_ASSERT_PTR_NOT_NULL_FATAL(address);
	CU_ASSERT_STRING_EQUAL_FATAL(linphone_address_get_scheme(address), "sips");
	CU_ASSERT_STRING_EQUAL_FATAL(linphone_address_get_username(address), "margaux");
	CU_ASSERT_STRING_EQUAL_FATAL(linphone_address_get_domain(address), "sip.linphone.org");

	linphone_address_destroy(address);

	linphone_core_destroy ( lc );
}

static void linphone_lpconfig_from_buffer(){

	static const char* buffer = "[buffer]\ntest=ok";
	static const char* buffer_linebreaks = "[buffer_linebreaks]\n\n\n\r\n\n\r\ntest=ok";
	LpConfig* conf;

	conf = lp_config_new_from_buffer(buffer);
	CU_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"buffer","test",""),"ok");
	lp_config_destroy(conf);

	conf = lp_config_new_from_buffer(buffer_linebreaks);
	CU_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"buffer_linebreaks","test",""),"ok");
	lp_config_destroy(conf);
}

static void linphone_lpconfig_from_buffer_zerolen_value(){
	/* parameters that have no value should return NULL, not "". */
	static const char* zerolen = "[test]\nzero_len=\nnon_zero_len=test";
	LpConfig* conf;

	conf = lp_config_new_from_buffer(zerolen);

	CU_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","zero_len","LOL"),"LOL");
	CU_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","non_zero_len",""),"test");

	lp_config_set_string(conf, "test", "non_zero_len", ""); /* should remove "non_zero_len" */
	CU_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","non_zero_len","LOL"), "LOL");

	lp_config_destroy(conf);
}

static void linphone_lpconfig_from_file_zerolen_value(){
	/* parameters that have no value should return NULL, not "". */
	static const char* zero_rc_file = "zero_length_params_rc";
	char* rc_path = ms_strdup_printf("%s/rcfiles/%s", liblinphone_tester_file_prefix, zero_rc_file);
	LpConfig* conf;

	/* not using lp_config_new() because it expects a readable file, and iOS (for instance)
	   stores the app bundle in read-only */
	conf = lp_config_new_with_factory(NULL, rc_path);

	CU_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","zero_len","LOL"),"LOL");

	// non_zero_len=test -> should return test
	CU_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","non_zero_len",""),"test");

	lp_config_set_string(conf, "test", "non_zero_len", ""); /* should remove "non_zero_len" */
	CU_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","non_zero_len","LOL"), "LOL");

	lp_config_destroy(conf);
}

static void linphone_lpconfig_from_xml_zerolen_value(){
	static const char* zero_xml_file = "remote_zero_length_params_rc";
	char* xml_path = ms_strdup_printf("%s/rcfiles/%s", liblinphone_tester_file_prefix, zero_xml_file);
	LpConfig* conf;

	LinphoneCoreManager* mgr = linphone_core_manager_new2("empty_rc",FALSE);

	CU_ASSERT_EQUAL(linphone_remote_provisioning_load_file(mgr->lc, xml_path), 0);

	conf = mgr->lc->config;

	CU_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","zero_len","LOL"),"LOL");
	CU_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","non_zero_len",""),"test");

	lp_config_set_string(conf, "test", "non_zero_len", ""); /* should remove "non_zero_len" */
	CU_ASSERT_STRING_EQUAL(lp_config_get_string(conf,"test","non_zero_len","LOL"), "LOL");

	linphone_core_manager_destroy(mgr);

}

void linphone_proxy_config_address_equal_test() {
	LinphoneAddress *a = linphone_address_new("sip:toto@titi");
	LinphoneAddress *b = linphone_address_new("sips:toto@titi");
	LinphoneAddress *c = linphone_address_new("sip:toto@titi;transport=tcp");
	LinphoneAddress *d = linphone_address_new("sip:toto@titu");
	LinphoneAddress *e = linphone_address_new("sip:toto@titi;transport=udp");
	LinphoneAddress *f = linphone_address_new("sip:toto@titi?X-Create-Account=yes");

	CU_ASSERT_EQUAL(linphone_proxy_config_address_equal(a,NULL), LinphoneProxyConfigAddressDifferent);
	CU_ASSERT_EQUAL(linphone_proxy_config_address_equal(a,b), LinphoneProxyConfigAddressDifferent);
	CU_ASSERT_EQUAL(linphone_proxy_config_address_equal(a,c), LinphoneProxyConfigAddressDifferent);
	CU_ASSERT_EQUAL(linphone_proxy_config_address_equal(a,d), LinphoneProxyConfigAddressDifferent);
	CU_ASSERT_EQUAL(linphone_proxy_config_address_equal(a,e), LinphoneProxyConfigAddressWeakEqual);
	CU_ASSERT_EQUAL(linphone_proxy_config_address_equal(NULL,NULL), LinphoneProxyConfigAddressEqual);
	CU_ASSERT_EQUAL(linphone_proxy_config_address_equal(a,f), LinphoneProxyConfigAddressWeakEqual);
	CU_ASSERT_EQUAL(linphone_proxy_config_address_equal(c,f), LinphoneProxyConfigAddressDifferent);
	CU_ASSERT_EQUAL(linphone_proxy_config_address_equal(e,f), LinphoneProxyConfigAddressWeakEqual);

	linphone_address_destroy(a);
	linphone_address_destroy(b);
	linphone_address_destroy(c);
	linphone_address_destroy(d);
}

void linphone_proxy_config_is_server_config_changed_test() {
	LinphoneProxyConfig* proxy_config = linphone_proxy_config_new();

	linphone_proxy_config_done(proxy_config); /*test done without edit*/

	linphone_proxy_config_set_identity(proxy_config,"sip:toto@titi");
	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_set_identity(proxy_config,"sips:toto@titi");
	CU_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressDifferent);

	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org");
	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_set_server_addr(proxy_config,"sip:toto.com");
	CU_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressDifferent);

	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org");
	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org:4444");
	CU_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressDifferent);

	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org");
	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org;transport=tcp");
	CU_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressDifferent);

	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org");
	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_set_server_addr(proxy_config,"sip:sip.linphone.org;param=blue");
	CU_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressWeakEqual);


	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_set_contact_parameters(proxy_config,"blabla=blue");
	CU_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressEqual);

	linphone_proxy_config_edit(proxy_config);
	linphone_proxy_config_enable_register(proxy_config,TRUE);
	CU_ASSERT_EQUAL(linphone_proxy_config_is_server_config_changed(proxy_config), LinphoneProxyConfigAddressEqual);

	linphone_proxy_config_destroy(proxy_config);
}

static void chat_root_test(void) {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	memset (&v_table,0,sizeof(v_table));
	lc = linphone_core_new(&v_table,NULL,NULL,NULL);
	CU_ASSERT_PTR_NOT_NULL_FATAL(lc);
	linphone_core_create_chat_room(lc,"sip:toto@titi.com");
	linphone_core_destroy(lc);
}

test_t setup_tests[] = {
	{ "Version check", linphone_version_test },
	{ "Linphone Address", linphone_address_test },
	{ "Linphone proxy config address equal (internal api)", linphone_proxy_config_address_equal_test},
	{ "Linphone proxy config server address change (internal api)", linphone_proxy_config_is_server_config_changed_test},
	{ "Linphone core init/uninit", core_init_test },
	{ "Linphone random transport port",core_sip_transport_test},
	{ "Linphone interpret url", linphone_interpret_url_test },
	{ "LPConfig from buffer", linphone_lpconfig_from_buffer },
	{ "LPConfig zero_len value from buffer", linphone_lpconfig_from_buffer_zerolen_value },
	{ "LPConfig zero_len value from file", linphone_lpconfig_from_file_zerolen_value },
	{ "LPConfig zero_len value from XML", linphone_lpconfig_from_xml_zerolen_value },
	{ "Chat room", chat_root_test }
};

test_suite_t setup_test_suite = {
	"Setup",
	NULL,
	NULL,
	sizeof(setup_tests) / sizeof(setup_tests[0]),
	setup_tests
};

