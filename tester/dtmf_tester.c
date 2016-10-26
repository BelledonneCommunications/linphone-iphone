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

void dtmf_received(LinphoneCore *lc, LinphoneCall *call, int dtmf) {
	stats* counters = get_stats(lc);
	char** dst = &counters->dtmf_list_received;
	*dst = *dst ?
				ms_strcat_printf(*dst, "%c", dtmf)
				: ms_strdup_printf("%c", dtmf);
	counters->dtmf_count++;
}

void send_dtmf_base(LinphoneCoreManager **pmarie, LinphoneCoreManager **ppauline, bool_t use_rfc2833, bool_t use_sipinfo, char dtmf, char* dtmf_seq, bool_t use_opus) {
	char* expected = NULL;
	int dtmf_count_prev;
	LinphoneCoreManager *marie = *pmarie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager *pauline = *ppauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCall *marie_call = NULL;

	if (use_opus) {
		//if (!ms_filter_codec_supported("opus")) {
		if(!ms_factory_codec_supported(marie->lc->factory, "opus") && !ms_factory_codec_supported(pauline->lc->factory, "opus")){

			ms_warning("Opus not supported, skipping test.");
			return;
		}
		disable_all_audio_codecs_except_one(marie->lc, "opus", 48000);
		disable_all_audio_codecs_except_one(pauline->lc, "opus", 48000);
	}

	linphone_core_set_use_rfc2833_for_dtmf(marie->lc, use_rfc2833);
	linphone_core_set_use_info_for_dtmf(marie->lc, use_sipinfo);
	linphone_core_set_use_rfc2833_for_dtmf(pauline->lc, use_rfc2833);
	linphone_core_set_use_info_for_dtmf(pauline->lc, use_sipinfo);

	BC_ASSERT_TRUE(call(pauline,marie));

	marie_call = linphone_core_get_current_call(marie->lc);

	BC_ASSERT_PTR_NOT_NULL(marie_call);

	if (!marie_call) return;

	if (dtmf != '\0') {
		dtmf_count_prev = pauline->stat.dtmf_count;
		linphone_call_send_dtmf(marie_call, dtmf);

		/*wait for the DTMF to be received from pauline*/
		BC_ASSERT_TRUE(wait_for_until(marie->lc, pauline->lc, &pauline->stat.dtmf_count, dtmf_count_prev+1, 10000));
		expected = ms_strdup_printf("%c", dtmf);
	}

	if (dtmf_seq != NULL) {
		int dtmf_delay_ms = lp_config_get_int(marie_call->core->config,"net","dtmf_delay_ms",200);
		dtmf_count_prev = pauline->stat.dtmf_count;
		linphone_call_send_dtmfs(marie_call, dtmf_seq);

		/*wait for the DTMF sequence to be received from pauline*/
		BC_ASSERT_TRUE(wait_for_until(marie->lc, pauline->lc, &pauline->stat.dtmf_count, (int)(dtmf_count_prev + strlen(dtmf_seq)), (int)(10000 + dtmf_delay_ms * strlen(dtmf_seq))));
		expected = (dtmf!='\0')?ms_strdup_printf("%c%s",dtmf,dtmf_seq):ms_strdup(dtmf_seq);
	}

	if (expected != NULL) {
		BC_ASSERT_PTR_NOT_NULL(pauline->stat.dtmf_list_received);
		if (pauline->stat.dtmf_list_received) {
			BC_ASSERT_STRING_EQUAL(pauline->stat.dtmf_list_received, expected);
		}
		ms_free(expected);
	} else {
		BC_ASSERT_PTR_NULL(pauline->stat.dtmf_list_received);
	}
}

void send_dtmf_cleanup(LinphoneCoreManager *marie, LinphoneCoreManager *pauline) {
	LinphoneCall *marie_call = linphone_core_get_current_call(marie->lc);
	if (marie_call) {
		BC_ASSERT_PTR_NULL(marie_call->dtmfs_timer);
		BC_ASSERT_PTR_NULL(marie_call->dtmf_sequence);

		/*just to sleep*/
		linphone_core_terminate_all_calls(pauline->lc);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void send_dtmf_rfc2833(void) {
	LinphoneCoreManager *marie, *pauline;
	send_dtmf_base(&marie, &pauline, TRUE,FALSE,'1',NULL,FALSE);
	send_dtmf_cleanup(marie, pauline);
}

static void send_dtmf_sip_info(void) {
	LinphoneCoreManager *marie, *pauline;
	send_dtmf_base(&marie, &pauline, FALSE,TRUE,'#',NULL,FALSE);
	send_dtmf_cleanup(marie, pauline);
}

static void send_dtmfs_sequence_rfc2833(void) {
	LinphoneCoreManager *marie, *pauline;
	send_dtmf_base(&marie, &pauline, TRUE,FALSE,'\0',"1230#",FALSE);
	send_dtmf_cleanup(marie, pauline);
}

static void send_dtmfs_sequence_sip_info(void) {
	LinphoneCoreManager *marie, *pauline;
	send_dtmf_base(&marie, &pauline, FALSE,TRUE,'\0',"1230#",FALSE);
	send_dtmf_cleanup(marie, pauline);
}

static void send_dtmfs_sequence_not_ready(void) {
	LinphoneCoreManager *marie;
	marie = linphone_core_manager_new( "marie_rc");
	BC_ASSERT_EQUAL(linphone_call_send_dtmfs(linphone_core_get_current_call(marie->lc), "123"), -1, int, "%d");
	linphone_core_manager_destroy(marie);
}

static void send_dtmfs_sequence_call_state_changed(void) {
	LinphoneCoreManager *marie, *pauline;
	LinphoneCall *marie_call = NULL;
	send_dtmf_base(&marie, &pauline, FALSE,TRUE,'\0',NULL,FALSE);

	marie_call = linphone_core_get_current_call(marie->lc);
	if (marie_call) {
		/*very long DTMF(around 4 sec to be sent)*/
		linphone_call_send_dtmfs(marie_call, "123456789123456789");
		/*just after, change call state, and expect DTMF to be canceled*/
		linphone_core_pause_call(marie_call->core,marie_call);
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallPausing,1));
		BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphoneCallPaused,1));

		/*wait a few time to ensure that no DTMF are received*/
		wait_for_until(marie->lc, pauline->lc, NULL, 0, 1000);

		BC_ASSERT_PTR_NULL(pauline->stat.dtmf_list_received);
	}
	end_call(marie, pauline);
	send_dtmf_cleanup(marie, pauline);
}

static void send_dtmf_rfc2833_opus(void) {
	LinphoneCoreManager *marie, *pauline;
	send_dtmf_base(&marie, &pauline, TRUE,FALSE,'1',NULL,TRUE);
	send_dtmf_cleanup(marie, pauline);
}

test_t dtmf_tests[] = {
	TEST_NO_TAG("Send DTMF using RFC2833",send_dtmf_rfc2833),
	TEST_NO_TAG("Send DTMF using SIP INFO",send_dtmf_sip_info),
	TEST_NO_TAG("Send DTMF sequence using RFC2833",send_dtmfs_sequence_rfc2833),
	TEST_NO_TAG("Send DTMF sequence using SIP INFO",send_dtmfs_sequence_sip_info),
	TEST_NO_TAG("DTMF sequence not sent if invalid call",send_dtmfs_sequence_not_ready),
	TEST_NO_TAG("DTMF sequence canceled if call state changed",send_dtmfs_sequence_call_state_changed),
	TEST_NO_TAG("Send DTMF using RFC2833 using Opus",send_dtmf_rfc2833_opus)
};

test_suite_t dtmf_test_suite = {"DTMF", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
								sizeof(dtmf_tests) / sizeof(dtmf_tests[0]), dtmf_tests};
