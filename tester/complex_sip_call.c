/*
	liblinphone_tester - liblinphone test suite
	Copyright (C) 2015  Belledonne Communications SARL

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


#include "linphonecore.h"
#include "liblinphone_tester.h"
#include "lpconfig.h"
#include "private.h"


static FILE *sip_start(const char *senario, const char* dest_username, LinphoneAddress* dest_addres) {
	char *dest;
	char *command;
	FILE *file;
	
	if (linphone_address_get_port(dest_addres)>0)
		dest = ms_strdup_printf("%s:%i",linphone_address_get_domain(dest_addres),linphone_address_get_port(dest_addres));
	else
		dest = ms_strdup_printf("%s",linphone_address_get_domain(dest_addres));
	
	command = ms_strdup_printf("sipp -sf %s -s %s %s -trace_err -trace_msg -rtp_echo -m 1 -d 1000",senario,dest_username,dest);
	
	ms_message("Starting sipp commad [%s]",command);
	file = popen(command, "r");
	ms_free(command);
	ms_free(dest);
	return file;
}
/*static void dest_server_server_resolved(void *data, const char *name, struct addrinfo *ai_list) {
	*(struct addrinfo **)data =ai_list;
}*/
static void sip_update_within_icoming_reinvite_with_no_sdp(void) {
	LinphoneCoreManager *mgr;
/*	LinphoneProxyConfig *proxy = linphone_core_get_default_proxy_config(mgr->lc);
	LinphoneAddress *dest = linphone_address_new(linphone_proxy_config_get_route(proxy) ?linphone_proxy_config_get_route(proxy):linphone_proxy_config_get_server_addr(proxy));
	struct addrinfo *addrinfo = NULL;
	char ipstring [INET6_ADDRSTRLEN];
	int err;
	int port = linphone_address_get_port(dest);*/
	char *identity_char;
	char *scen;
	FILE * sipp_out;
	
	/*currently we use direct connection because sipp do not properly set ACK request uri*/
	mgr= linphone_core_manager_new2( "empty_rc", FALSE);
	mgr->identity= linphone_core_get_primary_contact_parsed(mgr->lc);
	linphone_address_set_username(mgr->identity,"marie");
	identity_char=linphone_address_as_string(mgr->identity);
	linphone_core_set_primary_contact(mgr->lc,identity_char);
	linphone_core_iterate(mgr->lc);
	/*
	sal_resolve_a(	mgr->lc->sal
				  ,linphone_address_get_domain(dest)
				  ,linphone_address_get_port(dest)
				  ,AF_INET
				  ,(SalResolverCallback)dest_server_server_resolved
				  ,&addrinfo);
	linphone_address_destroy(dest);
	dest=linphone_address_new(NULL);
	
	wait_for(mgr->lc, mgr->lc, (int*)&addrinfo, 1);
	err=getnameinfo((struct sockaddr *)addrinfo->ai_addr,addrinfo->ai_addrlen,ipstring,INET6_ADDRSTRLEN,NULL,0,NI_NUMERICHOST);
	linphone_address_set_domain(dest, ipstring);
	if (port > 0)
		linphone_address_set_port(dest, port);
	*/
	scen = bc_tester_res("sipp/sip_update_within_icoming_reinvite_with_no_sdp.xml");
	
	sipp_out = sip_start(scen, linphone_address_get_username(mgr->identity), mgr->identity);

	if (sipp_out) {
		BC_ASSERT_TRUE(wait_for(mgr->lc, mgr->lc, &mgr->stat.number_of_LinphoneCallIncomingReceived, 1));
		linphone_core_accept_call(mgr->lc, linphone_core_get_current_call(mgr->lc));
		BC_ASSERT_TRUE(wait_for(mgr->lc, mgr->lc, &mgr->stat.number_of_LinphoneCallStreamsRunning, 2));
		BC_ASSERT_TRUE(wait_for(mgr->lc, mgr->lc, &mgr->stat.number_of_LinphoneCallEnd, 1));
		pclose(sipp_out);
	}
	linphone_core_manager_destroy(mgr);
	
}

static void call_with_video_mline_before_audio_in_sdp() {
	LinphoneCoreManager *mgr;
	char *identity_char;
	char *scen;
	FILE * sipp_out;
	LinphoneCall *call = NULL;
	
	/*currently we use direct connection because sipp do not properly set ACK request uri*/
	mgr= linphone_core_manager_new2( "empty_rc", FALSE);
	mgr->identity= linphone_core_get_primary_contact_parsed(mgr->lc);
	linphone_address_set_username(mgr->identity,"marie");
	identity_char=linphone_address_as_string(mgr->identity);
	linphone_core_set_primary_contact(mgr->lc,identity_char);
	
	linphone_core_iterate(mgr->lc);
	
	scen = bc_tester_res("sipp/call_with_video_mline_before_audio_in_sdp.xml");
	
	sipp_out = sip_start(scen, linphone_address_get_username(mgr->identity), mgr->identity);

	if (sipp_out) {
		BC_ASSERT_TRUE(wait_for(mgr->lc, mgr->lc, &mgr->stat.number_of_LinphoneCallIncomingReceived, 1));
		call = linphone_core_get_current_call(mgr->lc);
		linphone_core_accept_call(mgr->lc, call);
		BC_ASSERT_TRUE(wait_for(mgr->lc, mgr->lc, &mgr->stat.number_of_LinphoneCallStreamsRunning, 1));
		BC_ASSERT_EQUAL(call->main_audio_stream_index, 1, int, "%d");
		BC_ASSERT_EQUAL(call->main_video_stream_index, 0, int, "%d");
		BC_ASSERT_TRUE(wait_for(mgr->lc, mgr->lc, &mgr->stat.number_of_LinphoneCallEnd, 1));
		pclose(sipp_out);
	}
	linphone_core_manager_destroy(mgr);
}

static test_t tests[] = {
	{ "SIP UPDATE within incoming reinvite without sdp", sip_update_within_icoming_reinvite_with_no_sdp },
	{ "Call with video mline before audio in sdp", call_with_video_mline_before_audio_in_sdp },
};

test_suite_t complex_sip_call_test_suite = {
	"Complex SIP Call",
	liblinphone_tester_setup,
	NULL,
	sizeof(tests) / sizeof(tests[0]),
	tests
};

