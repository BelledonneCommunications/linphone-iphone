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

#include "liblinphone_tester.h"
#include "private.h"

LinphoneCoreManager* marie;
LinphoneCoreManager* pauline;
LinphoneCall *marie_call;

void dtmf_received(LinphoneCore *lc, LinphoneCall *call, int dtmf) {
	stats* counters = get_stats(lc);
	char** dst = &counters->dtmf_list_received;
	*dst = *dst ?
				ms_strcat_printf(*dst, "%c", dtmf)
				: ms_strdup_printf("%c", dtmf);
	counters->dtmf_count++;
}

void send_dtmf_base(bool_t use_rfc2833, bool_t use_sipinfo, char dtmf, char* dtmf_seq) {
	char* expected = NULL;
	int dtmf_count_prev;
	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new( "pauline_rc");

	linphone_core_set_use_rfc2833_for_dtmf(marie->lc, use_rfc2833);
	linphone_core_set_use_info_for_dtmf(marie->lc, use_sipinfo);
	linphone_core_set_use_rfc2833_for_dtmf(pauline->lc, use_rfc2833);
	linphone_core_set_use_info_for_dtmf(pauline->lc, use_sipinfo);

	CU_ASSERT_TRUE(call(pauline,marie));

	marie_call = linphone_core_get_current_call(marie->lc);

	if (dtmf != '\0') {
		dtmf_count_prev = pauline->stat.dtmf_count;
		linphone_call_send_dtmf(marie_call, dtmf);

		/*wait for the DTMF to be received from pauline*/
		CU_ASSERT_TRUE(wait_for_until(marie->lc, pauline->lc, &pauline->stat.dtmf_count, dtmf_count_prev+1, 10000));
		expected = ms_strdup_printf("%c", dtmf);
	}

	if (dtmf_seq != NULL) {
		int dtmf_delay_ms = lp_config_get_int(marie_call->core->config,"net","dtmf_delay_ms",200);
		dtmf_count_prev = pauline->stat.dtmf_count;
		linphone_call_send_dtmfs(marie_call, dtmf_seq);

		/*wait for the DTMF sequence to be received from pauline*/
		CU_ASSERT_TRUE(wait_for_until(marie->lc, pauline->lc, &pauline->stat.dtmf_count, dtmf_count_prev + strlen(dtmf_seq), 10000 + dtmf_delay_ms * strlen(dtmf_seq)));
		expected = (dtmf!='\0')?ms_strdup_printf("%c%s",dtmf,dtmf_seq):ms_strdup(dtmf_seq);
	}

	if (expected != NULL) {
		CU_ASSERT_PTR_NOT_NULL(pauline->stat.dtmf_list_received);
		if (pauline->stat.dtmf_list_received) {
			CU_ASSERT_STRING_EQUAL(pauline->stat.dtmf_list_received, expected);
		}
		ms_free(expected);
	} else {
		CU_ASSERT_PTR_NULL(pauline->stat.dtmf_list_received);
	}
}

void send_dtmf_cleanup() {
	CU_ASSERT_PTR_NULL(marie_call->dtmfs_timer);
	CU_ASSERT_PTR_NULL(marie_call->dtmf_sequence);

	/*just to sleep*/
	linphone_core_terminate_all_calls(pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void send_dtmf_rfc2833() {
	send_dtmf_base(TRUE,FALSE,'1',NULL);
	send_dtmf_cleanup();
}

static void send_dtmf_sip_info() {
	send_dtmf_base(FALSE,TRUE,'#',NULL);
	send_dtmf_cleanup();
}

static void send_dtmfs_sequence_rfc2833() {
	send_dtmf_base(TRUE,FALSE,'\0',"1230#");
	send_dtmf_cleanup();
}

static void send_dtmfs_sequence_sip_info() {
	send_dtmf_base(FALSE,TRUE,'\0',"1230#");
	send_dtmf_cleanup();
}

static void send_dtmfs_sequence_not_ready() {
	marie = linphone_core_manager_new( "marie_rc");
	CU_ASSERT_EQUAL(linphone_call_send_dtmfs(linphone_core_get_current_call(marie->lc), "123"), -1);
	linphone_core_manager_destroy(marie);
}

static void send_dtmfs_sequence_call_state_changed() {
	send_dtmf_base(FALSE,TRUE,'\0',NULL);

	/*very long DTMF(around 4 sec to be sent)*/
	linphone_call_send_dtmfs(marie_call, "123456789123456789");
	/*just after, change call state, and expect DTMF to be canceled*/
	linphone_core_pause_call(marie_call->core,marie_call);
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallPausing,1));
	CU_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallPaused,1));

	/*wait a few time to ensure that no DTMF are received*/
	wait_for_until(marie->lc, pauline->lc, NULL, 0, 1000);

	CU_ASSERT_PTR_NULL(pauline->stat.dtmf_list_received);

	send_dtmf_cleanup();
}

test_t dtmf_tests[] = {
	{ "Send DTMF using RFC2833",send_dtmf_rfc2833},
	{ "Send DTMF using SIP INFO",send_dtmf_sip_info},
	{ "Send DTMF sequence using RFC2833",send_dtmfs_sequence_rfc2833},
	{ "Send DTMF sequence using SIP INFO",send_dtmfs_sequence_sip_info},
	{ "DTMF sequence not sent if invalid call",send_dtmfs_sequence_not_ready},
	{ "DTMF sequence canceled if call state changed",send_dtmfs_sequence_call_state_changed},
};

test_suite_t dtmf_test_suite = {
	"DTMF",
	NULL,
	NULL,
	sizeof(dtmf_tests) / sizeof(dtmf_tests[0]),
	dtmf_tests
};
