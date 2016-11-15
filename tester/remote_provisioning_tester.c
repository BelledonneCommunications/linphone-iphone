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
#include "private.h"
#include "liblinphone_tester.h"

void linphone_configuration_status(LinphoneCore *lc, LinphoneConfiguringState status, const char *message) {
	stats* counters;
	ms_message("Configuring state = %i with message %s", status, message?message:"");

	counters = get_stats(lc);
	if (status == LinphoneConfiguringSkipped) {
		counters->number_of_LinphoneConfiguringSkipped++;
	} else if (status == LinphoneConfiguringFailed) {
		counters->number_of_LinphoneConfiguringFailed++;
	} else if (status == LinphoneConfiguringSuccessful) {
		counters->number_of_LinphoneConfiguringSuccessful++;
	}
}

static void remote_provisioning_skipped(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_rc", FALSE);
	BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringSkipped,1));
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_http(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_remote_rc", FALSE);
	BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringSuccessful,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneRegistrationOk,1));
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_transient(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_transient_remote_rc", FALSE);
	BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringSuccessful,1));
	BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneRegistrationOk,1));
	BC_ASSERT_TRUE(linphone_core_is_provisioning_transient(marie->lc));
	BC_ASSERT_PTR_NULL(linphone_core_get_provisioning_uri(marie->lc));
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_https(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new2("marie_remote_https_rc", FALSE);
		BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringSuccessful,1));
		BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneRegistrationOk,1));
		linphone_core_manager_destroy(marie);
	}
}

static void remote_provisioning_not_found(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_remote_404_rc", FALSE);
	BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringFailed,1));
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_invalid(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_remote_invalid_rc", FALSE);
	BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringFailed,1));
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_invalid_uri(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_remote_invalid_uri_rc", FALSE);
	BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringFailed,1));
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_default_values(void) {
	LinphoneProxyConfig *lpc;
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_remote_default_values_rc", FALSE);
	BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringSuccessful,1));
	lpc = linphone_core_create_proxy_config(marie->lc);
	BC_ASSERT_TRUE(lpc->reg_sendregister);
	BC_ASSERT_EQUAL(lpc->expires, 604800, int, "%d");
	BC_ASSERT_STRING_EQUAL(lpc->reg_proxy, "<sip:sip.linphone.org:5223;transport=tls>");
	BC_ASSERT_STRING_EQUAL(lpc->reg_route, "<sip:sip.linphone.org:5223;transport=tls>");
	BC_ASSERT_STRING_EQUAL(lpc->reg_identity, "sip:?@sip.linphone.org");
	{
		LpConfig* lp = linphone_core_get_config(marie->lc);
		BC_ASSERT_STRING_EQUAL(lp_config_get_string(lp,"app","toto","empty"),"titi");
	}
	linphone_proxy_config_destroy(lpc);
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_file(void) {
	LinphoneCoreManager* marie;
	const LpConfig* conf;
#if TARGET_OS_IPHONE
	ms_message("Skipping remote provisioning from file on iOS");
	return;
#elif defined(ANDROID)
	marie = linphone_core_manager_new2("marie_remote_localfile_android_rc", FALSE);
#elif defined(LINPHONE_WINDOWS_UNIVERSAL)
	marie = linphone_core_manager_new2("marie_remote_localfile_win10_rc", FALSE);
#else
	marie = ms_new0(LinphoneCoreManager, 1);
	linphone_core_manager_init(marie, "marie_remote_localfile_rc",NULL);
	// fix relative path to absolute path
	{
		char* path = bc_tester_res("rcfiles/marie_remote_localfile2_rc");
		char* abspath = ms_strdup_printf("file://%s", path);
		lp_config_set_string(marie->lc->config, "misc", "config-uri", abspath);
		linphone_core_manager_start(marie, 1);
		ms_free(path);
		ms_free(abspath);
	}
#endif
	BC_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringSuccessful,1));

	conf = linphone_core_get_config( marie->lc );
	BC_ASSERT_EQUAL( lp_config_get_int(conf,"misc","tester_file_ok", 0), 1 , int, "%d");

	linphone_core_manager_destroy(marie);
}


test_t remote_provisioning_tests[] = {
	TEST_NO_TAG("Remote provisioning skipped", remote_provisioning_skipped),
	TEST_NO_TAG("Remote provisioning successful behind http", remote_provisioning_http),
	TEST_NO_TAG("Remote provisioning successful behind https", remote_provisioning_https),
	TEST_NO_TAG("Remote provisioning 404 not found", remote_provisioning_not_found),
	TEST_NO_TAG("Remote provisioning invalid", remote_provisioning_invalid),
	TEST_NO_TAG("Remote provisioning transient successful", remote_provisioning_transient),
	TEST_NO_TAG("Remote provisioning default values", remote_provisioning_default_values),
	TEST_NO_TAG("Remote provisioning from file", remote_provisioning_file),
	TEST_NO_TAG("Remote provisioning invalid URI", remote_provisioning_invalid_uri)
};

test_suite_t remote_provisioning_test_suite = {"RemoteProvisioning", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
											   sizeof(remote_provisioning_tests) / sizeof(remote_provisioning_tests[0]),
											   remote_provisioning_tests};
