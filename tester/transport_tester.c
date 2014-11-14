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

/* Retrieve the public IP from a given hostname */
static const char* get_ip_from_hostname(const char * tunnel_hostname){
	struct addrinfo hints;
	struct addrinfo *res = NULL, *it = NULL;
	struct sockaddr_in *add;
	char * output = NULL;
	int err;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((err = getaddrinfo(tunnel_hostname, NULL, &hints, &res))){
		ms_error("error while retrieving IP from %s: %s", tunnel_hostname, gai_strerror(err));
		return NULL;
	}

	for (it=res; it!=NULL; it=it->ai_next){
		add = (struct sockaddr_in *) it->ai_addr;
		output = inet_ntoa( add->sin_addr );
	}
	freeaddrinfo(res);
	return output;
}
static char* get_public_contact_ip(LinphoneCore* lc)  {
	long contact_host_ip_len;
	char contact_host_ip[255];
	char * contact = linphone_proxy_config_get_contact(linphone_core_get_default_proxy_config(lc));
	CU_ASSERT_PTR_NOT_NULL(contact);
	contact_host_ip_len = strchr(contact, ':')-contact;
	strncpy(contact_host_ip, contact, contact_host_ip_len);
	contact_host_ip[contact_host_ip_len]='\0';
	ms_free(contact);
	return ms_strdup(contact_host_ip);
}
static void call_with_transport_base(LinphoneTunnelMode tunnel_mode, bool_t with_sip, LinphoneMediaEncryption encryption) {
	if (linphone_core_tunnel_available()){
		LinphoneCoreManager *pauline = linphone_core_manager_new( "pauline_rc");
		LinphoneCoreManager *marie = linphone_core_manager_new( "marie_rc");
		LinphoneCall *pauline_call;
		LinphoneProxyConfig *proxy = linphone_core_get_default_proxy_config(pauline->lc);
		LinphoneAddress *server_addr = linphone_address_new(linphone_proxy_config_get_server_addr(proxy));
		LinphoneAddress *route = linphone_address_new(linphone_proxy_config_get_route(proxy));
		const char * tunnel_ip = get_ip_from_hostname("tunnel.linphone.org");
		char *public_ip, *public_ip2=NULL;

		CU_ASSERT_TRUE(wait_for(pauline->lc,NULL,&pauline->stat.number_of_LinphoneRegistrationOk,1));
		public_ip = get_public_contact_ip(pauline->lc);
		CU_ASSERT_STRING_NOT_EQUAL(public_ip, tunnel_ip);

		linphone_core_set_media_encryption(pauline->lc, encryption);

		if (tunnel_mode != LinphoneTunnelModeDisable){
			LinphoneTunnel *tunnel = linphone_core_get_tunnel(pauline->lc);
			LinphoneTunnelConfig *config = linphone_tunnel_config_new();

			linphone_tunnel_config_set_host(config, "tunnel.linphone.org");
			linphone_tunnel_config_set_port(config, 443);
			linphone_tunnel_config_set_remote_udp_mirror_port(config, 12345);
			linphone_tunnel_add_server(tunnel, config);
			linphone_tunnel_set_mode(tunnel, tunnel_mode);
			linphone_tunnel_enable_sip(tunnel, with_sip);

			/*
			 * Enabling the tunnel with sip cause another REGISTER to be made.
			 * In automatic mode, the udp test should conclude (assuming we have a normal network), that no 
			 * tunnel is needed. Thus the number of registrations should stay to 1.
			 * The library is missing a notification of "tunnel connectivity test finished" to enable the 
			 * full testing of the automatic mode.
			 */

			if(tunnel_mode == LinphoneTunnelModeEnable && with_sip) {
				CU_ASSERT_TRUE(wait_for(pauline->lc,NULL,&pauline->stat.number_of_LinphoneRegistrationOk,2));
				/* Ensure that we did use the tunnel. If so, we should see contact changed from:
				Contact: <sip:pauline@192.168.0.201>;.[...]
				To:
				Contact: <sip:pauline@91.121.209.194:43867>;[....] (91.121.209.194 must be tunnel.liphone.org)
				*/
				ms_free(public_ip);
				public_ip = get_public_contact_ip(pauline->lc);
				CU_ASSERT_STRING_EQUAL(public_ip, tunnel_ip);
			} else {
				public_ip2 = get_public_contact_ip(pauline->lc);
				CU_ASSERT_STRING_EQUAL(public_ip, public_ip2);
			}
		}

		CU_ASSERT_TRUE(call(pauline,marie));
		pauline_call=linphone_core_get_current_call(pauline->lc);
		CU_ASSERT_PTR_NOT_NULL(pauline_call);
		if (pauline_call!=NULL){
			CU_ASSERT_EQUAL(linphone_call_params_get_media_encryption(linphone_call_get_current_params(pauline_call)),
				encryption);
		}
		end_call(pauline,marie);

		ms_free(public_ip);
		if(public_ip2 != NULL) ms_free(public_ip2);
		linphone_address_destroy(server_addr);
		linphone_address_destroy(route);
		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}else{
		ms_warning("Could not test %s because tunnel functionality is not available",__FUNCTION__);
	}
}

static void call_with_tunnel(void) {
	call_with_transport_base(LinphoneTunnelModeEnable, TRUE, LinphoneMediaEncryptionNone);
}

static void call_with_tunnel_srtp(void) {
	call_with_transport_base(LinphoneTunnelModeEnable, TRUE, LinphoneMediaEncryptionSRTP);
}

static void call_with_tunnel_without_sip(void) {
	call_with_transport_base(LinphoneTunnelModeEnable, FALSE, LinphoneMediaEncryptionNone);
}

static void call_with_tunnel_auto(void) {
	call_with_transport_base(LinphoneTunnelModeAuto, TRUE, LinphoneMediaEncryptionNone);
}

static void call_with_tunnel_auto_without_sip_with_srtp(void) {
	call_with_transport_base(LinphoneTunnelModeAuto, FALSE, LinphoneMediaEncryptionSRTP);
}

test_t transport_tests[] = {
	{ "Tunnel only", call_with_tunnel },
	{ "Tunnel with SRTP", call_with_tunnel_srtp },
	{ "Tunnel without SIP", call_with_tunnel_without_sip },
	{ "Tunnel in automatic mode", call_with_tunnel_auto },
	{ "Tunnel in automatic mode with SRTP without SIP", call_with_tunnel_auto_without_sip_with_srtp },
};

test_suite_t transport_test_suite = {
	"Transport",
	NULL,
	NULL,
	sizeof(transport_tests) / sizeof(transport_tests[0]),
	transport_tests
};
