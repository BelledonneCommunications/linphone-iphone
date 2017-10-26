/*
 * main-db-tester.cpp
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "db/main-db.h"

#include "liblinphone_tester.h"

// =============================================================================

using namespace std;

using namespace LinphonePrivate;

// -----------------------------------------------------------------------------

static const string getDatabasePath () {
	static const string path = string(bc_tester_get_resource_dir_prefix()) + "/db/linphone.db";
	return path;
}

// -----------------------------------------------------------------------------

static void open_database () {
	MainDb mainDb(nullptr);
	BC_ASSERT_TRUE(mainDb.connect(MainDb::Sqlite3, getDatabasePath()));
}

static void get_events_count () {
	MainDb mainDb(nullptr);
	BC_ASSERT_TRUE(mainDb.connect(MainDb::Sqlite3, getDatabasePath()));
	BC_ASSERT_EQUAL(mainDb.getEventsCount(), 4976, int, "%d");
	BC_ASSERT_EQUAL(mainDb.getEventsCount(MainDb::ConferenceCallFilter), 0, int, "%d");
	BC_ASSERT_EQUAL(mainDb.getEventsCount(MainDb::ConferenceInfoFilter), 0, int, "%d");
	BC_ASSERT_EQUAL(mainDb.getEventsCount(MainDb::ConferenceChatMessageFilter), 4976, int, "%d");
	BC_ASSERT_EQUAL(mainDb.getEventsCount(MainDb::NoFilter), 4976, int, "%d");
}

static void get_messages_count () {
	MainDb mainDb(nullptr);
	BC_ASSERT_TRUE(mainDb.connect(MainDb::Sqlite3, getDatabasePath()));
	BC_ASSERT_EQUAL(mainDb.getMessagesCount(), 4976, int, "%d");
	BC_ASSERT_EQUAL(mainDb.getMessagesCount("sip:test-39@sip.linphone.org"), 3, int, "%d");
}

static void get_unread_messages_count () {
	MainDb mainDb(nullptr);
	BC_ASSERT_TRUE(mainDb.connect(MainDb::Sqlite3, getDatabasePath()));
	BC_ASSERT_EQUAL(mainDb.getUnreadMessagesCount(), 2, int, "%d");
	BC_ASSERT_EQUAL(mainDb.getUnreadMessagesCount("sip:test-39@sip.linphone.org"), 0, int, "%d");
}

static void get_history () {
	MainDb mainDb(nullptr);
	BC_ASSERT_TRUE(mainDb.connect(MainDb::Sqlite3, getDatabasePath()));
	BC_ASSERT_EQUAL(
		mainDb.getHistoryRange("sip:test-39@sip.linphone.org", 0, -1, MainDb::Filter::ConferenceChatMessageFilter).size(),
		3,
		int,
		"%d"
	);
	BC_ASSERT_EQUAL(
		mainDb.getHistoryRange("sip:test-7@sip.linphone.org", 0, -1, MainDb::Filter::ConferenceCallFilter).size(),
		0,
		int,
		"%d"
	);
	BC_ASSERT_EQUAL(
		mainDb.getHistoryRange("sip:test-1@sip.linphone.org", 0, -1, MainDb::Filter::ConferenceChatMessageFilter).size(),
		862,
		int,
		"%d"
	);
	BC_ASSERT_EQUAL(
		mainDb.getHistory("sip:test-1@sip.linphone.org", 100, MainDb::Filter::ConferenceChatMessageFilter).size(),
		100,
		int,
		"%d"
	);
}

test_t main_db_tests[] = {
	TEST_NO_TAG("Open database", open_database),
	TEST_NO_TAG("Get events count", get_events_count),
	TEST_NO_TAG("Get messages count", get_messages_count),
	TEST_NO_TAG("Get unread messages count", get_unread_messages_count),
	TEST_NO_TAG("Get history", get_history)
};

test_suite_t main_db_test_suite = {
	"MainDb", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(main_db_tests) / sizeof(main_db_tests[0]), main_db_tests
};
