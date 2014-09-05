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
#include <sys/types.h>
#include <sys/stat.h>
#include "CUnit/Basic.h"
#include "linphonecore.h"
#include "lpconfig.h"
#include "private.h"
#include "liblinphone_tester.h"

static void call_with_transport_base(bool_t use_tunnel, LinphoneMediaEncryption encryption) {
	if (linphone_core_tunnel_available()){
		/*enabling the tunnel cause another REGISTER to be made*/
		int pauline_register_count_expected = use_tunnel ? 2 : 1;
		char *tmp_char;
		LinphoneCoreManager *pauline = linphone_core_manager_new( "pauline_rc");
		LinphoneCoreManager *marie = linphone_core_manager_new( "marie_rc");
		LinphoneCall *pauline_call;

		/*tunnel works only in UDP mode*/
		LinphoneProxyConfig *proxy = linphone_core_get_default_proxy_config(pauline->lc);
		LinphoneAddress *server_addr = linphone_address_new(linphone_proxy_config_get_server_addr(proxy));
		LinphoneAddress *route = linphone_address_new(linphone_proxy_config_get_route(proxy));
		linphone_proxy_config_edit(proxy);
		linphone_address_set_transport(server_addr, LinphoneTransportUdp);
		linphone_address_set_transport(route, LinphoneTransportUdp);
		tmp_char = linphone_address_as_string(server_addr);
		linphone_proxy_config_set_server_addr(proxy, tmp_char);
		ms_free(tmp_char);
		tmp_char = linphone_address_as_string(route);
		linphone_proxy_config_set_route(proxy, tmp_char);
		ms_free(tmp_char);

		linphone_core_set_media_encryption(pauline->lc, encryption);

		if (use_tunnel){
			LinphoneTunnel *tunnel = linphone_core_get_tunnel(pauline->lc);
			LinphoneTunnelConfig *config = linphone_tunnel_config_new();

			linphone_tunnel_enable(tunnel, TRUE);
			linphone_tunnel_config_set_host(config, "tunnel.linphone.org");
			linphone_tunnel_config_set_port(config, 443);
			linphone_tunnel_add_server(tunnel, config);
		}
		linphone_proxy_config_done(proxy);

		CU_ASSERT_TRUE(wait_for(pauline->lc,NULL,&pauline->stat.number_of_LinphoneRegistrationOk,pauline_register_count_expected));
		CU_ASSERT_TRUE(wait_for(marie->lc, NULL, &marie->stat.number_of_LinphoneRegistrationOk, 1));

		CU_ASSERT_TRUE(call(pauline,marie));
		pauline_call=linphone_core_get_current_call(pauline->lc);
		CU_ASSERT_PTR_NOT_NULL(pauline_call);
		if (pauline_call!=NULL){
			CU_ASSERT_EQUAL(linphone_call_params_get_media_encryption(linphone_call_get_current_params(pauline_call)),
				encryption);
		}
		end_call(pauline,marie);

		linphone_address_destroy(server_addr);
		linphone_address_destroy(route);
		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}else{
		ms_warning("Could not test %s because tunnel functionality is not available",__FUNCTION__);
	}
}

static void call_with_tunnel(void) {
	call_with_transport_base(TRUE,LinphoneMediaEncryptionNone);
}

static void call_with_tunnel_srtp(void) {
	call_with_transport_base(TRUE,LinphoneMediaEncryptionSRTP);
}

test_t transport_tests[] = {
	{ "Tunnel only", call_with_tunnel },
	{ "Tunnel with SRTP", call_with_tunnel_srtp },
};

test_suite_t transport_test_suite = {
	"Transport",
	NULL,
	NULL,
	sizeof(transport_tests) / sizeof(transport_tests[0]),
	transport_tests
};
