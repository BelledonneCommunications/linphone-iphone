/*
 * events-db-tester.cpp
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
	MainDb eventsDb;
	BC_ASSERT_TRUE(eventsDb.connect(MainDb::Sqlite3, getDatabasePath()));

	eventsDb.import(AbstractDb::Backend::Sqlite3, "/home/rabhamon/.local/share/linphone/message-history.db");
}

static void get_events_count () {
	MainDb eventsDb;
	BC_ASSERT_TRUE(eventsDb.connect(MainDb::Sqlite3, getDatabasePath()));
	BC_ASSERT_EQUAL(eventsDb.getEventsCount(), 4976, int, "%d");
	BC_ASSERT_EQUAL(eventsDb.getEventsCount(MainDb::CallFilter), 0, int, "%d");
	BC_ASSERT_EQUAL(eventsDb.getEventsCount(MainDb::ConferenceFilter), 0, int, "%d");
	BC_ASSERT_EQUAL(eventsDb.getEventsCount(MainDb::MessageFilter), 4976, int, "%d");
	BC_ASSERT_EQUAL(eventsDb.getEventsCount(MainDb::NoFilter), 4976, int, "%d");
}

static void get_messages_count () {
	MainDb eventsDb;
	BC_ASSERT_TRUE(eventsDb.connect(MainDb::Sqlite3, getDatabasePath()));
	BC_ASSERT_EQUAL(eventsDb.getMessagesCount(), 4976, int, "%d");
	BC_ASSERT_EQUAL(eventsDb.getMessagesCount("sip:test-7@sip.linphone.org"), 3, int, "%d");
}

static void get_unread_messages_count () {
	MainDb eventsDb;
	BC_ASSERT_TRUE(eventsDb.connect(MainDb::Sqlite3, getDatabasePath()));
	BC_ASSERT_EQUAL(eventsDb.getUnreadMessagesCount(), 2, int, "%d");
	BC_ASSERT_EQUAL(eventsDb.getUnreadMessagesCount("sip:test-7@sip.linphone.org"), 0, int, "%d");
}

test_t events_db_tests[] = {
	TEST_NO_TAG("Open database", open_database),
	TEST_NO_TAG("Get events count", get_events_count),
	TEST_NO_TAG("Get messages count", get_messages_count),
	TEST_NO_TAG("Get unread messages count", get_unread_messages_count)
};

test_suite_t events_db_test_suite = {
	"EventsDb", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(events_db_tests) / sizeof(events_db_tests[0]), events_db_tests
};
