/*
	belle-sip - SIP (RFC3261) library.
	Copyright (C) 2014  Belledonne Communications SARL

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"
#include "mediastreamer2/stun.h"
#include "ortp/port.h"


static const char *stun_address = "stun.linphone.org";


static size_t test_stun_encode(char **buffer)
{
	MSStunMessage *req = ms_stun_binding_request_create();
	UInt96 tr_id = ms_stun_message_get_tr_id(req);
	tr_id.octet[0] = 11;
	ms_stun_message_set_tr_id(req, tr_id);
	return ms_stun_message_encode(req, buffer);
}


static void linphone_stun_test_encode(void)
{
	char *buffer = NULL;
	size_t len = test_stun_encode(&buffer);
	BC_ASSERT(len > 0);
	BC_ASSERT_PTR_NOT_NULL(buffer);
	if (buffer != NULL) ms_free(buffer);
	ms_message("STUN message encoded in %i bytes", (int)len);
}


static void linphone_stun_test_grab_ip(void)
{
	LinphoneCoreManager* lc_stun = linphone_core_manager_new2("stun_rc", FALSE);
	LinphoneCall dummy_call;
	int ping_time;
	int tmp = 0;

	memset(&dummy_call, 0, sizeof(LinphoneCall));
	dummy_call.main_audio_stream_index = 0;
	dummy_call.main_video_stream_index = 1;
	dummy_call.main_text_stream_index = 2;
	dummy_call.media_ports[dummy_call.main_audio_stream_index].rtp_port = 7078;
	dummy_call.media_ports[dummy_call.main_video_stream_index].rtp_port = 9078;
	dummy_call.media_ports[dummy_call.main_text_stream_index].rtp_port = 11078;

	linphone_core_set_stun_server(lc_stun->lc, stun_address);
	BC_ASSERT_STRING_EQUAL(stun_address, linphone_core_get_stun_server(lc_stun->lc));

	wait_for(lc_stun->lc, lc_stun->lc, &tmp, 1);

	ping_time = linphone_core_run_stun_tests(lc_stun->lc, &dummy_call);
	BC_ASSERT(ping_time != -1);

	ms_message("Round trip to STUN: %d ms", ping_time);

	BC_ASSERT(dummy_call.ac.addr[0] != '\0');
	BC_ASSERT(dummy_call.ac.port != 0);
#ifdef VIDEO_ENABLED
	BC_ASSERT(dummy_call.vc.addr[0] != '\0');
	BC_ASSERT(dummy_call.vc.port != 0);
#endif
	BC_ASSERT(dummy_call.tc.addr[0] != '\0');
	BC_ASSERT(dummy_call.tc.port != 0);

	ms_message("STUN test result: local audio port maps to %s:%i", dummy_call.ac.addr, dummy_call.ac.port);
#ifdef VIDEO_ENABLED
	ms_message("STUN test result: local video port maps to %s:%i", dummy_call.vc.addr, dummy_call.vc.port);
#endif
	ms_message("STUN test result: local text port maps to %s:%i", dummy_call.tc.addr, dummy_call.tc.port);

	linphone_core_manager_destroy(lc_stun);
}


test_t stun_tests[] = {
	TEST_NO_TAG("Basic Stun test (Ping/public IP)", linphone_stun_test_grab_ip),
	TEST_NO_TAG("STUN encode", linphone_stun_test_encode)
};

test_suite_t stun_test_suite = {"Stun", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
								sizeof(stun_tests) / sizeof(stun_tests[0]), stun_tests};
