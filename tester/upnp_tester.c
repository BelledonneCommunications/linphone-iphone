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
#include "linphone/lpconfig.h"
#include "private.h"
#include "liblinphone_tester.h"

static void upnp_start_n_stop(void) {
	int tmp = 0;
	LinphoneCoreManager* lc_upnp = linphone_core_manager_new2( "upnp_rc", FALSE);
	wait_for(lc_upnp->lc,lc_upnp->lc,&tmp,1);
#ifdef BUILD_UPNP
	BC_ASSERT_PTR_NOT_NULL(lc_upnp->lc->upnp);
#endif
	linphone_core_manager_destroy(lc_upnp);
}

static void upnp_check_state(void) {
	int tmp = 0;
	LinphoneCoreManager* lc_upnp = linphone_core_manager_new2( "upnp_rc", FALSE);
	wait_for(lc_upnp->lc,lc_upnp->lc,&tmp,1);
	BC_ASSERT_EQUAL(linphone_core_get_upnp_state(lc_upnp->lc), LinphoneUpnpStateOk, int, "%d");
	linphone_core_manager_destroy(lc_upnp);
}

static void upnp_check_ipaddress(void) {
	int tmp = 0;
	const char *addr;
	LinphoneCoreManager* lc_upnp = linphone_core_manager_new2( "upnp_rc", FALSE);
	wait_for(lc_upnp->lc,lc_upnp->lc,&tmp,1);
	addr = linphone_core_get_upnp_external_ipaddress(lc_upnp->lc);
	BC_ASSERT_PTR_NOT_NULL(addr);
	if (addr!=NULL) {
		BC_ASSERT_GREATER((int)strlen(addr),7,int,"%d");
	}
	linphone_core_manager_destroy(lc_upnp);
}

test_t upnp_tests[] = {
	TEST_NO_TAG("Start and stop", upnp_start_n_stop),
	TEST_NO_TAG("Check state", upnp_check_state),
	TEST_NO_TAG("Check ip address", upnp_check_ipaddress)
};

test_suite_t upnp_test_suite = {"Upnp", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
								sizeof(upnp_tests) / sizeof(upnp_tests[0]), upnp_tests};
