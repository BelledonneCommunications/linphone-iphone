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


test_t setup_tests[] = {
	{ "Linphone Address", linphone_address_test },
	{ "Linphone core init/uninit", core_init_test },
	{ "Linphone random transport port",core_sip_transport_test},
	{ "Linphone interpret url", linphone_interpret_url_test }
};

test_suite_t setup_test_suite = {
	"Setup",
	NULL,
	NULL,
	sizeof(setup_tests) / sizeof(setup_tests[0]),
	setup_tests
};

