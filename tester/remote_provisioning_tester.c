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
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringSkipped,1));
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_http(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_remote_rc", FALSE);
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringSuccessful,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneRegistrationOk,1));
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_transient(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_transient_remote_rc", FALSE);
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringSuccessful,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneRegistrationOk,1));
	CU_ASSERT_TRUE(linphone_core_is_provisioning_transient(marie->lc) == TRUE);
	CU_ASSERT_TRUE(linphone_core_get_provisioning_uri(marie->lc) == NULL);
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_https(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_remote_https_rc", FALSE);
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringSuccessful,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneRegistrationOk,1));
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_not_found(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_remote_404_rc", FALSE);
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringFailed,1));
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_invalid(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_remote_invalid_rc", FALSE);
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringFailed,1));
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_invalid_uri(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_remote_invalid_uri_rc", FALSE);
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringFailed,1));
	linphone_core_manager_destroy(marie);
}

static void remote_provisioning_default_values(void) {
	LinphoneProxyConfig *lpc;
	LinphoneCoreManager* marie = linphone_core_manager_new2("marie_remote_default_values_rc", FALSE);
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringSuccessful,1));
	lpc = linphone_core_create_proxy_config(marie->lc);
	CU_ASSERT_TRUE(lpc->reg_sendregister == TRUE);
	CU_ASSERT_TRUE(lpc->expires == 604800);
	CU_ASSERT_STRING_EQUAL(lpc->reg_proxy, "<sip:sip.linphone.org:5223;transport=tls>");
	CU_ASSERT_STRING_EQUAL(lpc->reg_route, "<sip:sip.linphone.org:5223;transport=tls>");
	CU_ASSERT_STRING_EQUAL(lpc->reg_identity, "sip:?@sip.linphone.org");
	{
		LpConfig* lp = linphone_core_get_config(marie->lc);
		CU_ASSERT_STRING_EQUAL(lp_config_get_string(lp,"app","toto","empty"),"titi");
	}

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
#else
	marie = linphone_core_manager_new2("marie_remote_localfile_rc", FALSE);
#endif
	CU_ASSERT_TRUE(wait_for(marie->lc,NULL,&marie->stat.number_of_LinphoneConfiguringSuccessful,1));

	conf = linphone_core_get_config( marie->lc );
	CU_ASSERT_EQUAL( lp_config_get_int(conf,"misc","tester_file_ok", 0), 1 );

	linphone_core_manager_destroy(marie);
}


test_t remote_provisioning_tests[] = {
	{ "Remote provisioning skipped", remote_provisioning_skipped },
	{ "Remote provisioning successful behind http", remote_provisioning_http },
	{ "Remote provisioning successful behind https", remote_provisioning_https },
	{ "Remote provisioning 404 not found", remote_provisioning_not_found },
	{ "Remote provisioning invalid", remote_provisioning_invalid },
	{ "Remote provisioning transient successful", remote_provisioning_transient },
	{ "Remote provisioning default values", remote_provisioning_default_values },
	{ "Remote provisioning from file", remote_provisioning_file },
	{ "Remote provisioning invalid URI", remote_provisioning_invalid_uri }
};

test_suite_t remote_provisioning_test_suite = {
	"RemoteProvisioning",
	NULL,
	NULL,
	sizeof(remote_provisioning_tests) / sizeof(remote_provisioning_tests[0]),
	remote_provisioning_tests
};
