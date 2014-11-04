/*
	belle-sip - SIP (RFC3261) library.
    Copyright (C) 2010  Belledonne Communications SARL

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
#include <time.h>
#include "CUnit/Basic.h"
#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"

extern char *strptime(char*, char*, struct tm*);

LinphoneCoreManager* setup(bool_t enable_logs)  {
	LinphoneCoreManager *marie;
	int timeout_ms = 3000;
	
	linphone_core_enable_log_collection(enable_logs);
	

	// linphone_core_set_log_collection_size(10);
	marie = linphone_core_manager_new( "marie_rc");
	// wait a few seconds to generate some traffic
	while (timeout_ms > 0){
		linphone_core_iterate(marie->lc);
		ms_usleep(100000); //100 ms sleep
		timeout_ms -= 100;
		// Generate some logs
		ms_message("Time left: %d", timeout_ms);
	}
	return marie;
}

time_t check_file(char * filepath, bool_t remove_file)  {
	time_t time_curr = -1;
	if (filepath != NULL) {
		int line_count = 0;
		FILE *file = fopen(filepath, "r");
		char *line = NULL;
		size_t line_size = 256;
		struct tm tm_curr;
		time_t time_prev = -1;

		// 1) expect to find folder name in filename path
		CU_ASSERT_PTR_NOT_NULL(strstr(filepath, liblinphone_tester_writable_dir_prefix));

		// 2) check file contents
		while (getline(&line, &line_size, file) != -1) {
			// a) there should be at least 100 lines
			++line_count;

			// b) logs should be ordered by date (format: 2014-11-04 15:22:12:606)
			if (strlen(line) > 24) {
				char date[24] = {'\0'};
				memcpy(date, line, 23);
				if (strptime(date, "%Y-%m-%d %H:%M:%S", &tm_curr) != NULL) {
					time_curr = mktime(&tm_curr);
					CU_ASSERT_TRUE(time_curr >= time_prev);
					time_prev = time_curr;
				}
			}
		}
		CU_ASSERT(line_count > 100);
		free(line);
		fclose(file);
		if (remove_file)  {
			remove(filepath);
		}
		ms_free(filepath);
	}
	// return latest time in file
	return time_curr;
}

static OrtpLogLevel old_log_level;
// static LinphoneLogCollectionState old_collection_state;
static int collect_init()  {
	old_log_level = ortp_get_log_level_mask();
	// old_collection_state = liblinphone_log_collection_enabled;
	// CU_ASSERT_FALSE("Fixme: // old_collection_state = liblinphone_log_collection_enabled;");

	// if we want some logs, we must force them... even if user dont want to!
	linphone_core_set_log_level(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
	linphone_core_set_log_collection_path(liblinphone_tester_writable_dir_prefix);

	return 0;
}

static int collect_cleanup()  {
	linphone_core_set_log_level(old_log_level);
	// liblinphone_log_collection_enabled = old_collection_state;
	// CU_ASSERT_FALSE("Fixme: // liblinphone_log_collection_enabled = old_collection_state;");

	return 0;
}

static void collect_files_disabled()  {
	LinphoneCoreManager* marie = setup(FALSE);
	CU_ASSERT_PTR_NULL(linphone_core_compress_log_collection(marie->lc));
	linphone_core_manager_destroy(marie);
}

static void collect_files_filled() {
	LinphoneCoreManager* marie = setup(TRUE);
	char * filepath = linphone_core_compress_log_collection(marie->lc);
	CU_ASSERT_PTR_NOT_NULL(filepath);
	CU_ASSERT_EQUAL(ms_time(0), check_file(filepath, FALSE));
	linphone_core_manager_destroy(marie);
}

static void collect_files_small_size()  {
	LinphoneCoreManager* marie = setup(TRUE);
	// linphone_core_set_log_collection_size(10);
	char * filepath= linphone_core_compress_log_collection(marie->lc);
	CU_ASSERT_PTR_NOT_NULL(filepath);
	CU_ASSERT_EQUAL(ms_time(0), check_file(filepath, TRUE));
	linphone_core_manager_destroy(marie);
}

test_t log_collection_tests[] = {
	{ "No file when disabled", collect_files_disabled},
	{ "Collect files filled when enabled", collect_files_filled},
	{ "Logs collected into small file", collect_files_small_size},
};

test_suite_t log_collection_test_suite = {
	"LogCollection",
	collect_init,
	collect_cleanup,
	sizeof(log_collection_tests) / sizeof(log_collection_tests[0]),
	log_collection_tests
};

