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

#include <stdio.h>
#include "CUnit/Basic.h"
#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"
#include "ortp/stun.h"
#include "ortp/port.h"


static const char* stun_address = "stun.linphone.org";


static int test_stun_encode( char*buffer, size_t len, bool_t expect_fail )
{
	StunAtrString username;
	StunAtrString password;
	StunMessage req;
	memset(&req, 0, sizeof(StunMessage));
	memset(&username,0,sizeof(username));
	memset(&password,0,sizeof(password));
	stunBuildReqSimple( &req, &username, TRUE , TRUE , 11);
	len = stunEncodeMessage( &req, buffer, len, &password);
	if (len<=0){
		if( expect_fail )
			ms_message("Fail to encode stun message (EXPECTED).\n");
		else
			ms_error("Fail to encode stun message.\n");
		return -1;
	}
	return len;
}


static void linphone_stun_test_encode()
{
	char smallBuff[12];
	size_t smallLen = 12;
	char bigBuff[STUN_MAX_MESSAGE_SIZE];
	size_t bigLen = STUN_MAX_MESSAGE_SIZE;

	size_t len = test_stun_encode(smallBuff, smallLen, TRUE);
	CU_ASSERT(len == -1);

	len = test_stun_encode(bigBuff, bigLen, TRUE);
	CU_ASSERT(len > 0);
	ms_message("STUN message encoded in %i bytes", (int)len);
}


static void linphone_stun_test_grab_ip()
{
	LinphoneCoreManager* lc_stun = linphone_core_manager_new2( "stun_rc", FALSE);
	LinphoneCall dummy_call;
	int ping_time;
	int tmp=0;

	memset(&dummy_call, 0, sizeof(LinphoneCall));
	dummy_call.media_ports[0].rtp_port = 7078;
	dummy_call.media_ports[1].rtp_port = 9078;

	linphone_core_set_stun_server(lc_stun->lc, stun_address);
	CU_ASSERT_STRING_EQUAL(stun_address, linphone_core_get_stun_server(lc_stun->lc));

	wait_for(lc_stun->lc,lc_stun->lc,&tmp,1);

	ping_time = linphone_core_run_stun_tests(lc_stun->lc, &dummy_call);
	CU_ASSERT(ping_time != -1);

	ms_message("Round trip to STUN: %d ms", ping_time);

	CU_ASSERT( dummy_call.ac.addr[0] != '\0');
	CU_ASSERT( dummy_call.ac.port != 0);
#ifdef VIDEO_ENABLED
	CU_ASSERT( dummy_call.vc.addr[0] != '\0');
	CU_ASSERT( dummy_call.vc.port != 0);
#endif

	ms_message("STUN test result: local audio port maps to %s:%i",
			dummy_call.ac.addr,
			dummy_call.ac.port);
#ifdef VIDEO_ENABLED
	ms_message("STUN test result: local video port maps to %s:%i",
			dummy_call.vc.addr,
			dummy_call.vc.port);
#endif

	linphone_core_manager_destroy(lc_stun);
}


test_t stun_tests[] = {
	{ "Basic Stun test (Ping/public IP)", linphone_stun_test_grab_ip },
	{ "STUN encode buffer protection", linphone_stun_test_encode },
};

test_suite_t stun_test_suite = {
	"Stun",
	NULL,
	NULL,
	sizeof(stun_tests) / sizeof(stun_tests[0]),
	stun_tests
};
