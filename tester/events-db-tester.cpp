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

#include "db/events-db.h"

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
	EventsDb eventsDb;
	eventsDb.connect(EventsDb::Sqlite3, getDatabasePath());
}

test_t events_db_tests[] = {
	TEST_NO_TAG("Open database", open_database)
};

test_suite_t events_db_test_suite = {
	"EventsDb", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(events_db_tests) / sizeof(events_db_tests[0]), events_db_tests
};
