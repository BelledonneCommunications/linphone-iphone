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
#include "lpconfig.h"
#include "private.h"
#include "liblinphone_tester.h"

static void upnp_start_n_stop(void) {
	int tmp = 0;
	LinphoneCoreManager* lc_upnp = linphone_core_manager_new2( "upnp_rc", FALSE);
	wait_for(lc_upnp->lc,lc_upnp->lc,&tmp,1);
#ifdef BUILD_UPNP
	CU_ASSERT_TRUE(lc_upnp->lc->upnp != NULL);
#endif
	linphone_core_manager_destroy(lc_upnp);
}

static void upnp_check_state(void) {
	int tmp = 0;
	LinphoneCoreManager* lc_upnp = linphone_core_manager_new2( "upnp_rc", FALSE);
	wait_for(lc_upnp->lc,lc_upnp->lc,&tmp,1);
	CU_ASSERT_TRUE(linphone_core_get_upnp_state(lc_upnp->lc) == LinphoneUpnpStateOk);
	linphone_core_manager_destroy(lc_upnp);
}

static void upnp_check_ipaddress(void) {
	int tmp = 0;
	const char *addr;
	LinphoneCoreManager* lc_upnp = linphone_core_manager_new2( "upnp_rc", FALSE);
	wait_for(lc_upnp->lc,lc_upnp->lc,&tmp,1);
	addr = linphone_core_get_upnp_external_ipaddress(lc_upnp->lc);
	CU_ASSERT_TRUE(addr != NULL && strlen(addr)>=7);
	linphone_core_manager_destroy(lc_upnp);
}

test_t upnp_tests[] = {
	{ "Start and stop", upnp_start_n_stop },
	{ "Check state", upnp_check_state },
	{ "Check ip address", upnp_check_ipaddress },
};

test_suite_t upnp_test_suite = {
	"Upnp",
	NULL,
	NULL,
	sizeof(upnp_tests) / sizeof(upnp_tests[0]),
	upnp_tests
};
