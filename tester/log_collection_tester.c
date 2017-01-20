/*
	Linphone
	Copyright (C) 2014  Belledonne Communications SARL

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

#ifndef _XOPEN_SOURCE
	#define _XOPEN_SOURCE 700 // To have definition of strptime, snprintf and getline
#endif
#include <time.h>
#include "linphone/core.h"
#include "private.h"
#include "liblinphone_tester.h"

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif


/*getline is POSIX 2008, not available on many systems.*/
#if (defined(ANDROID) && !defined(__LP64__)) || defined(_WIN32) || defined(__QNX__)
/* This code is public domain -- Will Hartung 4/9/09 */
static ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
	char *bufptr = NULL;
	char *p = bufptr;
	size_t size;
	int c;

	if (lineptr == NULL) {
		return -1;
	}
	if (stream == NULL) {
		return -1;
	}
	if (n == NULL) {
		return -1;
	}
	bufptr = *lineptr;
	size = *n;

	c = fgetc(stream);
	if (c == EOF) {
		return -1;
	}
	if (bufptr == NULL) {
		bufptr = malloc(128);
		if (bufptr == NULL) {
			return -1;
		}
		size = 128;
	}
	p = bufptr;
	while(c != EOF) {
		size_t curpos = p-bufptr;

		if (curpos > (size - 1)) {
			size = size + 128;
			bufptr = realloc(bufptr, size);
			p = bufptr + curpos;
			if (bufptr == NULL) {
				return -1;
			}
		}
		*p++ = c;
		if (c == '\n') {
			break;
		}
		c = fgetc(stream);
	}

	*p++ = '\0';
	*lineptr = bufptr;
	*n = size;

	return (ssize_t)(p - bufptr) - 1;
}
#endif

static LinphoneLogCollectionState old_collection_state;
static void collect_init(void)  {
	old_collection_state = linphone_core_log_collection_enabled();
	linphone_core_set_log_collection_path(bc_tester_get_writable_dir_prefix());
}

static void collect_cleanup(LinphoneCoreManager *marie)  {
	linphone_core_manager_destroy(marie);

	linphone_core_enable_log_collection(old_collection_state);
	linphone_core_reset_log_collection();
}

static LinphoneCoreManager* setup(LinphoneLogCollectionState log_collection_state)  {
	LinphoneCoreManager *marie;
	int timeout = 300;

	collect_init();
	linphone_core_enable_log_collection(log_collection_state);

	marie = linphone_core_manager_new2("marie_rc", 0);
	// wait a few seconds to generate some traffic
	while (--timeout){
		// Generate some logs - error logs because we must ensure that
		// even if user did not enable logs, we will see them
		ms_error("(test error)Timeout in %d...", timeout);
	}
	return marie;
}

#if HAVE_ZLIB

/*returns uncompressed log file*/
static FILE* gzuncompress(const char* filepath) {
		gzFile file = gzopen(filepath, "rb");
		FILE *output = NULL;
		FILE *ret;
		char *newname = ms_strdup_printf("%s.txt", filepath);
		char buffer[512]={0};
		output = fopen(newname, "wb");
		while (gzread(file, buffer, 511) > 0) {
			fputs(buffer, output);
			memset(buffer, 0, strlen(buffer));
		}
		fclose(output);
		BC_ASSERT_EQUAL(gzclose(file), Z_OK, int, "%d");
		ret=fopen(newname, "rb");
		ms_free(newname);
		return ret;
}
#endif

static time_t get_current_time(void) {
	struct timeval tp;
	struct tm *lt;
#ifndef _WIN32
	struct tm tmbuf;
#endif
	time_t tt;
	ortp_gettimeofday(&tp,NULL);
	tt = (time_t)tp.tv_sec;

#ifdef _WIN32
	lt = localtime(&tt);
#else
	lt = localtime_r(&tt,&tmbuf);
#endif
	return mktime(lt);
}

static time_t check_file(LinphoneCoreManager* mgr)  {

	time_t cur_time = get_current_time();
	char*    filepath = linphone_core_compress_log_collection();
	time_t  log_time = -1;
	uint32_t timediff = 0;
	FILE *file = NULL;

	BC_ASSERT_PTR_NOT_NULL(filepath);

	if (filepath != NULL) {
		int line_count = 0;
		char *line = NULL;
		size_t line_size = 256;
#ifndef _WIN32
		struct tm tm_curr = {0};
		time_t time_prev = 0;
#endif

#if HAVE_ZLIB
		// 0) if zlib is enabled, we must decompress the file first
		file = gzuncompress(filepath);
#else
		file = fopen(filepath, "rb");
#endif
		BC_ASSERT_PTR_NOT_NULL(file);
		if (!file) return 0;
		// 1) expect to find folder name in filename path
		BC_ASSERT_PTR_NOT_NULL(strstr(filepath, bc_tester_get_writable_dir_prefix()));

		// 2) check file contents
		while (getline(&line, &line_size, file) != -1) {
			// a) there should be at least 25 lines
			++line_count;
#ifndef _WIN32
			// b) logs should be ordered by date (format: 2014-11-04 15:22:12:606)
			if (strlen(line) > 24) {
				char date[24] = {'\0'};
				memcpy(date, line, 23);
				/*reset tm_curr to reset milliseconds and below fields*/
				memset(&tm_curr, 0, sizeof(struct tm));
				if (strptime(date, "%Y-%m-%d %H:%M:%S", &tm_curr) != NULL) {
					tm_curr.tm_isdst = -1; // LOL
					log_time = mktime(&tm_curr);
					BC_ASSERT_GREATER(log_time , time_prev, long int, "%ld");
					time_prev = log_time;
				}
			}
#endif
		}
		BC_ASSERT_GREATER(line_count , 25, int, "%d");
		free(line);
		fclose(file);
		ms_free(filepath);


		timediff = labs((long int)log_time - (long int)cur_time);
#ifndef _WIN32
		BC_ASSERT_LOWER(timediff, 1, unsigned, "%u");
		if( !(timediff <= 1) ){
			char buffers[2][128] = {{0}};
			strftime(buffers[0], sizeof(buffers[0]), "%Y-%m-%d %H:%M:%S", localtime(&log_time));
			strftime(buffers[1], sizeof(buffers[1]), "%Y-%m-%d %H:%M:%S", localtime(&cur_time));

			ms_error("log_time: %ld (%s), cur_time: %ld (%s) timediff: %u"
				, (long int)log_time, buffers[0]
				, (long int)cur_time, buffers[1]
				, timediff
			);
		}
#else
		(void)timediff;
		ms_warning("strptime() not available for this platform, test is incomplete.");
#endif
	}
	// return latest time in file
	return log_time;
}

static void collect_files_disabled(void)  {
	LinphoneCoreManager* marie = setup(LinphoneLogCollectionDisabled);
	BC_ASSERT_PTR_NULL(linphone_core_compress_log_collection());
	collect_cleanup(marie);
}

static void collect_files_filled(void) {
	LinphoneCoreManager* marie = setup(LinphoneLogCollectionEnabled);
	check_file(marie);
	collect_cleanup(marie);
}

static void collect_files_small_size(void)  {
	LinphoneCoreManager* marie = setup(LinphoneLogCollectionEnabled);
	linphone_core_set_log_collection_max_file_size(5000);
	check_file(marie);
	collect_cleanup(marie);
}

static void collect_files_changing_size(void)  {
	LinphoneCoreManager* marie = setup(LinphoneLogCollectionEnabled);
	int waiting = 100;

	check_file(marie);

	linphone_core_set_log_collection_max_file_size(5000);
	// Generate some logs
	while (--waiting) ms_error("(test error)Waiting %d...", waiting);

	check_file(marie);

	collect_cleanup(marie);
}
static void logCollectionUploadStateChangedCb(LinphoneCore *lc, LinphoneCoreLogCollectionUploadState state, const char *info) {

	stats* counters = get_stats(lc);
	ms_message("lc [%p], logCollectionUploadStateChanged to [%s], info [%s]",lc
																			,linphone_core_log_collection_upload_state_to_string(state)
																			,info);
	switch(state) {
		case LinphoneCoreLogCollectionUploadStateInProgress:
			counters->number_of_LinphoneCoreLogCollectionUploadStateInProgress++;
			break;
		case LinphoneCoreLogCollectionUploadStateDelivered:
			counters->number_of_LinphoneCoreLogCollectionUploadStateDelivered++;
			BC_ASSERT_GREATER((int)strlen(info), 0, int, "%d");
			break;
		case LinphoneCoreLogCollectionUploadStateNotDelivered:
			counters->number_of_LinphoneCoreLogCollectionUploadStateNotDelivered++;
			break;
	}
}
static void upload_collected_traces(void)  {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = setup(LinphoneLogCollectionEnabled);
		int waiting = 100;
		LinphoneCoreVTable *v_table = linphone_core_v_table_new();
		v_table->log_collection_upload_state_changed = logCollectionUploadStateChangedCb;
		linphone_core_add_listener(marie->lc, v_table);

		linphone_core_set_log_collection_max_file_size(5000);
		linphone_core_set_log_collection_upload_server_url(marie->lc,"https://www.linphone.org:444/lft.php");
		// Generate some logs
		while (--waiting) ms_error("(test error)Waiting %d...", waiting);
		ms_free(linphone_core_compress_log_collection());
		linphone_core_upload_log_collection(marie->lc);
		BC_ASSERT_TRUE(wait_for_until(marie->lc,marie->lc,&marie->stat.number_of_LinphoneCoreLogCollectionUploadStateDelivered,1, 10000));

		/*try 2 times*/
		waiting=100;
		linphone_core_reset_log_collection();
		while (--waiting) ms_error("(test error)Waiting %d...", waiting);
		ms_free(linphone_core_compress_log_collection());
		linphone_core_upload_log_collection(marie->lc);
		BC_ASSERT_TRUE(wait_for_until(marie->lc,marie->lc,&marie->stat.number_of_LinphoneCoreLogCollectionUploadStateDelivered,2, 10000));
		collect_cleanup(marie);
	}
}

test_t log_collection_tests[] = {
	TEST_NO_TAG("No file when disabled", collect_files_disabled),
	TEST_NO_TAG("Collect files filled when enabled", collect_files_filled),
	TEST_NO_TAG("Logs collected into small file", collect_files_small_size),
	TEST_NO_TAG("Logs collected when decreasing max size", collect_files_changing_size),
	TEST_NO_TAG("Upload collected traces", upload_collected_traces)
};

test_suite_t log_collection_test_suite = {"LogCollection", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
										  sizeof(log_collection_tests) / sizeof(log_collection_tests[0]),
										  log_collection_tests};
