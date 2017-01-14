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

#include <sys/types.h>
#include <sys/stat.h>
#include "linphone/core.h"
#include "linphone/lpconfig.h"
#include "private.h"
#include "liblinphone_tester.h"

/* Retrieve the public IP from a given hostname */
int get_ip_from_hostname(const char * tunnel_hostname, char *ip, size_t ip_size){
	struct addrinfo hints;
	struct addrinfo *res = NULL;
	int err;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((err = getaddrinfo(tunnel_hostname, NULL, &hints, &res))){
		ms_error("error while retrieving IP from %s: %s", tunnel_hostname, gai_strerror(err));
		return err;
	}

	bctbx_addrinfo_to_ip_address(res, ip, ip_size, NULL);
	freeaddrinfo(res);
	return err;
}
static char* get_public_contact_ip(LinphoneCore* lc)  {
	const LinphoneAddress * contact = linphone_proxy_config_get_contact(linphone_core_get_default_proxy_config(lc));
	BC_ASSERT_PTR_NOT_NULL(contact);
	return ms_strdup(linphone_address_get_domain(contact));
}


static void call_with_tunnel_base(LinphoneTunnelMode tunnel_mode, bool_t with_sip, LinphoneMediaEncryption encryption, bool_t with_video_and_ice) {
	if (linphone_core_tunnel_available()){
		LinphoneCoreManager *pauline = linphone_core_manager_new( "pauline_rc");
		LinphoneCoreManager *marie = linphone_core_manager_new( "marie_rc");
		LinphoneCall *pauline_call, *marie_call;
		LinphoneProxyConfig *proxy = linphone_core_get_default_proxy_config(pauline->lc);
		LinphoneAddress *server_addr = linphone_address_new(linphone_proxy_config_get_server_addr(proxy));
		LinphoneAddress *route = linphone_address_new(linphone_proxy_config_get_route(proxy));
		char  tunnel_ip[64];
		char *public_ip, *public_ip2=NULL;
		BC_ASSERT_FALSE(get_ip_from_hostname("tunnel.linphone.org",tunnel_ip,sizeof(tunnel_ip)));
						
		BC_ASSERT_TRUE(wait_for(pauline->lc,NULL,&pauline->stat.number_of_LinphoneRegistrationOk,1));
		public_ip = get_public_contact_ip(pauline->lc);
		BC_ASSERT_STRING_NOT_EQUAL(public_ip, tunnel_ip);

		linphone_core_set_media_encryption(pauline->lc, encryption);

		if (with_video_and_ice){
			/*we want to test that tunnel is able to work with long SIP message, above mtu.
			 * Enable ICE and many codec to make the SIP message bigger*/
			linphone_core_set_firewall_policy(marie->lc, LinphonePolicyUseIce);
			linphone_core_set_firewall_policy(pauline->lc, LinphonePolicyUseIce);
			linphone_core_enable_payload_type(pauline->lc,
				linphone_core_find_payload_type(pauline->lc, "speex", 32000, 1), TRUE);
			linphone_core_enable_payload_type(pauline->lc,
				linphone_core_find_payload_type(pauline->lc, "speex", 16000, 1), TRUE);
			linphone_core_enable_payload_type(pauline->lc,
				linphone_core_find_payload_type(pauline->lc, "G722", 8000, 1), TRUE);
		}

		if (tunnel_mode != LinphoneTunnelModeDisable){
			LinphoneTunnel *tunnel = linphone_core_get_tunnel(pauline->lc);
			LinphoneTunnelConfig *config = linphone_tunnel_config_new();

			linphone_tunnel_config_set_host(config, "tunnel.linphone.org");
			linphone_tunnel_config_set_port(config, 443);
			linphone_tunnel_config_set_remote_udp_mirror_port(config, 12345);
			linphone_tunnel_add_server(tunnel, config);
			linphone_tunnel_set_mode(tunnel, tunnel_mode);
			linphone_tunnel_enable_sip(tunnel, with_sip);
			linphone_tunnel_config_unref(config);

			/*
			 * Enabling the tunnel with sip cause another REGISTER to be made.
			 * In automatic mode, the udp test should conclude (assuming we have a normal network), that no
			 * tunnel is needed. Thus the number of registrations should stay to 1.
			 * The library is missing a notification of "tunnel connectivity test finished" to enable the
			 * full testing of the automatic mode.
			 */

			if (tunnel_mode == LinphoneTunnelModeEnable && with_sip) {
				BC_ASSERT_TRUE(wait_for(pauline->lc,NULL,&pauline->stat.number_of_LinphoneRegistrationOk,2));
				/* Ensure that we did use the tunnel. If so, we should see contact changed from:
				Contact: <sip:pauline@192.168.0.201>;.[...]
				To:
				Contact: <sip:pauline@91.121.209.194:43867>;[....] (91.121.209.194 must be tunnel.liphone.org)
				*/
				ms_free(public_ip);
				public_ip = get_public_contact_ip(pauline->lc);
				BC_ASSERT_STRING_EQUAL(public_ip, tunnel_ip);
			} else {
				public_ip2 = get_public_contact_ip(pauline->lc);
				BC_ASSERT_STRING_EQUAL(public_ip, public_ip2);
			}
		}

		BC_ASSERT_TRUE(call(pauline,marie));
		pauline_call=linphone_core_get_current_call(pauline->lc);
		BC_ASSERT_PTR_NOT_NULL(pauline_call);
		if (pauline_call!=NULL){
			BC_ASSERT_EQUAL(linphone_call_params_get_media_encryption(linphone_call_get_current_params(pauline_call)),
				encryption, int, "%d");
		}
		if (tunnel_mode == LinphoneTunnelModeEnable && with_sip){
			/* make sure the call from pauline arrived from the tunnel by checking the contact address*/
			marie_call = linphone_core_get_current_call(marie->lc);
			BC_ASSERT_PTR_NOT_NULL(marie_call);
			if (marie_call){
				const char *remote_contact = linphone_call_get_remote_contact(marie_call);
				BC_ASSERT_PTR_NOT_NULL(remote_contact);
				if (remote_contact){
					LinphoneAddress *tmp = linphone_address_new(remote_contact);
					BC_ASSERT_PTR_NOT_NULL(tmp);
					if (tmp){
						BC_ASSERT_STRING_EQUAL(linphone_address_get_domain(tmp), tunnel_ip);
						linphone_address_unref(tmp);
					}
				}
			}
		}
#ifdef VIDEO_ENABLED
		if (with_video_and_ice){
			BC_ASSERT_TRUE(request_video(pauline, marie, TRUE));
		}
#endif
		end_call(pauline,marie);

		ms_free(public_ip);
		if(public_ip2 != NULL) ms_free(public_ip2);
		linphone_address_unref(server_addr);
		linphone_address_unref(route);
		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}else{
		ms_warning("Could not test %s because tunnel functionality is not available",__FUNCTION__);
	}
}


static void call_with_tunnel(void) {
	call_with_tunnel_base(LinphoneTunnelModeEnable, TRUE, LinphoneMediaEncryptionNone, FALSE);
}

static void call_with_tunnel_srtp(void) {
	call_with_tunnel_base(LinphoneTunnelModeEnable, TRUE, LinphoneMediaEncryptionSRTP, FALSE);
}

static void call_with_tunnel_without_sip(void) {
	call_with_tunnel_base(LinphoneTunnelModeEnable, FALSE, LinphoneMediaEncryptionNone, FALSE);
}

static void call_with_tunnel_auto(void) {
	call_with_tunnel_base(LinphoneTunnelModeAuto, TRUE, LinphoneMediaEncryptionNone, FALSE);
}

static void call_with_tunnel_auto_without_sip_with_srtp(void) {
	call_with_tunnel_base(LinphoneTunnelModeAuto, FALSE, LinphoneMediaEncryptionSRTP, FALSE);
}

#ifdef VIDEO_ENABLED

static void full_tunnel_video_ice_call(void){
	if (linphone_core_tunnel_available()){
		call_with_tunnel_base(LinphoneTunnelModeEnable, TRUE, LinphoneMediaEncryptionNone, TRUE);
	}else
		ms_warning("Could not test %s because tunnel functionality is not available",__FUNCTION__);
}

static void tunnel_srtp_video_ice_call(void) {
	if (linphone_core_tunnel_available())
		call_base(LinphoneMediaEncryptionSRTP,TRUE,FALSE,LinphonePolicyUseIce,TRUE);
	else
		ms_warning("Could not test %s because tunnel functionality is not available",__FUNCTION__);
}
static void tunnel_zrtp_video_ice_call(void) {
	if (linphone_core_tunnel_available())
		call_base(LinphoneMediaEncryptionZRTP,TRUE,FALSE,LinphonePolicyUseIce,TRUE);
	else
		ms_warning("Could not test %s because tunnel functionality is not available",__FUNCTION__);
}

static void tunnel_dtls_video_ice_call(void) {
	if (linphone_core_tunnel_available())
		call_base(LinphoneMediaEncryptionDTLS,TRUE,FALSE,LinphonePolicyUseIce,TRUE);
	else
		ms_warning("Could not test %s because tunnel functionality is not available",__FUNCTION__);
}

static void tunnel_video_ice_call(void) {
	if (linphone_core_tunnel_available())
		call_base(LinphoneMediaEncryptionNone,TRUE,FALSE,LinphonePolicyUseIce,TRUE);
	else
		ms_warning("Could not test %s because tunnel functionality is not available",__FUNCTION__);
}
#endif

static void tunnel_srtp_ice_call(void) {
	if (linphone_core_tunnel_available())
		call_base(LinphoneMediaEncryptionSRTP,FALSE,FALSE,LinphonePolicyUseIce,TRUE);
	else
		ms_warning("Could not test %s because tunnel functionality is not available",__FUNCTION__);
}

static void tunnel_zrtp_ice_call(void) {
	if (linphone_core_tunnel_available())
		call_base(LinphoneMediaEncryptionZRTP,FALSE,FALSE,LinphonePolicyUseIce,TRUE);
	else
		ms_warning("Could not test %s because tunnel functionality is not available",__FUNCTION__);
}

static void tunnel_ice_call(void) {
	if (linphone_core_tunnel_available())
		call_base(LinphoneMediaEncryptionNone,FALSE,FALSE,LinphonePolicyUseIce,TRUE);
	else
		ms_warning("Could not test %s because tunnel functionality is not available",__FUNCTION__);
}

static void register_on_second_tunnel(void) {
	if (linphone_core_tunnel_available()) {
		LinphoneCoreManager *pauline = linphone_core_manager_new( "pauline_rc");
		LinphoneTunnel *tunnel = linphone_core_get_tunnel(pauline->lc);
		LinphoneTunnelConfig *config1 = linphone_tunnel_config_new();
		LinphoneTunnelConfig *config2 = linphone_tunnel_config_new();
		char tunnel_ip[64];
		char* public_ip;
		
		BC_ASSERT_FALSE(get_ip_from_hostname("tunnel.linphone.org",tunnel_ip,sizeof(tunnel_ip)));
		linphone_tunnel_simulate_udp_loss(tunnel, TRUE);

		// add a first tunnel config with an invalid port
		linphone_tunnel_config_set_host(config1, "sip3.linphone.org");
		linphone_tunnel_config_set_port(config1, 4141);
		linphone_tunnel_config_set_remote_udp_mirror_port(config1, 54321);
		linphone_tunnel_add_server(tunnel, config1);

		// then a proper server
		linphone_tunnel_config_set_host(config2, "tunnel.linphone.org");
		linphone_tunnel_config_set_port(config2, 443);
		linphone_tunnel_config_set_remote_udp_mirror_port(config2, 12345);
		linphone_tunnel_add_server(tunnel, config2);

		linphone_tunnel_set_mode(tunnel, LinphoneTunnelModeAuto);
		linphone_tunnel_enable_sip(tunnel, TRUE);

		reset_counters(&pauline->stat);

		linphone_core_refresh_registers(pauline->lc);
		// we should expect 2 registers: since tunnel autodetection takes several seconds, a first
		// register will be made, then tunnel will be configured and another register will be fired up
		BC_ASSERT_TRUE(wait_for(pauline->lc, NULL, &pauline->stat.number_of_LinphoneRegistrationOk,2));

		public_ip = get_public_contact_ip(pauline->lc);
		BC_ASSERT_STRING_EQUAL(public_ip, tunnel_ip);
		ms_free(public_ip);

		linphone_tunnel_config_unref(config1);
		linphone_tunnel_config_unref(config2);
		linphone_core_manager_destroy(pauline);
	} else {
		ms_warning("Could not test %s because tunnel functionality is not available",__FUNCTION__);
	}
}

test_t tunnel_tests[] = {
	TEST_NO_TAG("Simple", call_with_tunnel),
	TEST_NO_TAG("With SRTP", call_with_tunnel_srtp),
	TEST_NO_TAG("Without SIP", call_with_tunnel_without_sip),
	TEST_NO_TAG("In automatic mode", call_with_tunnel_auto),
	TEST_NO_TAG("In automatic mode with SRTP without SIP", call_with_tunnel_auto_without_sip_with_srtp),
	TEST_NO_TAG("Ice call", tunnel_ice_call),
	TEST_NO_TAG("SRTP ice call", tunnel_srtp_ice_call),
	TEST_NO_TAG("ZRTP ice call", tunnel_zrtp_ice_call),
#ifdef VIDEO_ENABLED
	TEST_NO_TAG("Ice video call", tunnel_video_ice_call),
	TEST_NO_TAG("With SIP - ice video call", full_tunnel_video_ice_call),
	TEST_NO_TAG("SRTP ice video call", tunnel_srtp_video_ice_call),
	TEST_NO_TAG("DTLS ice video call", tunnel_dtls_video_ice_call),
	TEST_NO_TAG("ZRTP ice video call", tunnel_zrtp_video_ice_call),
#endif
	TEST_NO_TAG("Register on second tunnel", register_on_second_tunnel),
};

test_suite_t tunnel_test_suite = {"Tunnel", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
								  sizeof(tunnel_tests) / sizeof(tunnel_tests[0]), tunnel_tests};
