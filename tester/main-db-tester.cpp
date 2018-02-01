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

#include "address/address.h"
#include "core/core-p.h"
#include "db/main-db.h"
#include "event-log/events.h"

// TODO: Remove me. <3
#include "private.h"

#include "liblinphone_tester.h"
#include "tools/tester.h"

// =============================================================================

using namespace std;

using namespace LinphonePrivate;

// -----------------------------------------------------------------------------

class MainDbProvider {
public:
	MainDbProvider () {
		mCoreManager = linphone_core_manager_create("marie_rc");
		char *dbPath = bc_tester_res("db/linphone.db");
		linphone_config_set_string(linphone_core_get_config(mCoreManager->lc), "storage", "uri", dbPath);
		bctbx_free(dbPath);
		linphone_core_manager_start(mCoreManager, false);
	}

	~MainDbProvider () {
		linphone_core_manager_destroy(mCoreManager);
	}

	const MainDb &getMainDb () {
		return *L_GET_PRIVATE(mCoreManager->lc->cppPtr)->mainDb;
	}

private:
	LinphoneCoreManager *mCoreManager;
};

// -----------------------------------------------------------------------------

static void get_events_count () {
	MainDbProvider provider;
	const MainDb &mainDb = provider.getMainDb();
	BC_ASSERT_EQUAL(mainDb.getEventCount(), 5175, int, "%d");
	BC_ASSERT_EQUAL(mainDb.getEventCount(MainDb::ConferenceCallFilter), 0, int, "%d");
	BC_ASSERT_EQUAL(mainDb.getEventCount(MainDb::ConferenceInfoFilter), 18, int, "%d");
	BC_ASSERT_EQUAL(mainDb.getEventCount(MainDb::ConferenceChatMessageFilter), 5157, int, "%d");
	BC_ASSERT_EQUAL(mainDb.getEventCount(MainDb::NoFilter), 5175, int, "%d");
}

static void get_messages_count () {
	MainDbProvider provider;
	const MainDb &mainDb = provider.getMainDb();
	BC_ASSERT_EQUAL(mainDb.getChatMessageCount(), 5157, int, "%d");
	BC_ASSERT_EQUAL(
		mainDb.getChatMessageCount(
			ChatRoomId(IdentityAddress("sip:test-3@sip.linphone.org"), IdentityAddress("sip:test-1@sip.linphone.org"))
		),
		861, int, "%d"
	);
}

static void get_unread_messages_count () {
	MainDbProvider provider;
	const MainDb &mainDb = provider.getMainDb();
	BC_ASSERT_EQUAL(mainDb.getUnreadChatMessageCount(), 2, int, "%d");
	BC_ASSERT_EQUAL(
		mainDb.getUnreadChatMessageCount(
			ChatRoomId(IdentityAddress("sip:test-3@sip.linphone.org"), IdentityAddress("sip:test-1@sip.linphone.org"))
		),
		0, int, "%d"
	);
}

static void get_history () {
	MainDbProvider provider;
	const MainDb &mainDb = provider.getMainDb();
	BC_ASSERT_EQUAL(
		mainDb.getHistoryRange(
			ChatRoomId(IdentityAddress("sip:test-4@sip.linphone.org"), IdentityAddress("sip:test-1@sip.linphone.org")),
			0, -1, MainDb::Filter::ConferenceChatMessageFilter
		).size(),
		54,
		int,
		"%d"
	);
	BC_ASSERT_EQUAL(
		mainDb.getHistoryRange(
			ChatRoomId(IdentityAddress("sip:test-7@sip.linphone.org"), IdentityAddress("sip:test-7@sip.linphone.org")),
			0, -1, MainDb::Filter::ConferenceCallFilter
		).size(),
		0,
		int,
		"%d"
	);
	BC_ASSERT_EQUAL(
		mainDb.getHistoryRange(
			ChatRoomId(IdentityAddress("sip:test-1@sip.linphone.org"), IdentityAddress("sip:test-1@sip.linphone.org")),
			0, -1, MainDb::Filter::ConferenceChatMessageFilter
		).size(),
		804,
		int,
		"%d"
	);
	BC_ASSERT_EQUAL(
		mainDb.getHistory(
			ChatRoomId(IdentityAddress("sip:test-1@sip.linphone.org"), IdentityAddress("sip:test-1@sip.linphone.org")),
			100, MainDb::Filter::ConferenceChatMessageFilter
		).size(),
		100,
		int,
		"%d"
	);
}

static void get_conference_notified_events () {
	MainDbProvider provider;
	const MainDb &mainDb = provider.getMainDb();
	list<shared_ptr<EventLog>> events = mainDb.getConferenceNotifiedEvents(
		ChatRoomId(IdentityAddress("sip:test-44@sip.linphone.org"), IdentityAddress("sip:test-1@sip.linphone.org")),
		1
	);
	BC_ASSERT_EQUAL(events.size(), 3, int, "%d");
	if (events.size() != 3)
		return;

	shared_ptr<EventLog> event;
	auto it = events.cbegin();

	event = *it;
	if (!BC_ASSERT_TRUE(event->getType() == EventLog::Type::ConferenceParticipantRemoved)) return;
	{
		shared_ptr<ConferenceParticipantEvent> participantEvent = static_pointer_cast<ConferenceParticipantEvent>(event);
		BC_ASSERT_TRUE(participantEvent->getChatRoomId().getPeerAddress().asString() == "sip:test-44@sip.linphone.org");
		BC_ASSERT_TRUE(participantEvent->getParticipantAddress().asString() == "sip:test-11@sip.linphone.org");
		BC_ASSERT_TRUE(participantEvent->getNotifyId() == 2);
	}

	event = *++it;
	if (!BC_ASSERT_TRUE(event->getType() == EventLog::Type::ConferenceParticipantDeviceAdded)) return;
	{
		shared_ptr<ConferenceParticipantDeviceEvent> deviceEvent = static_pointer_cast<
			ConferenceParticipantDeviceEvent
		>(event);
		BC_ASSERT_TRUE(deviceEvent->getChatRoomId().getPeerAddress().asString() == "sip:test-44@sip.linphone.org");
		BC_ASSERT_TRUE(deviceEvent->getParticipantAddress().asString() == "sip:test-11@sip.linphone.org");
		BC_ASSERT_TRUE(deviceEvent->getNotifyId() == 3);
		BC_ASSERT_TRUE(deviceEvent->getDeviceAddress().asString() == "sip:test-47@sip.linphone.org");
	}

	event = *++it;
	if (!BC_ASSERT_TRUE(event->getType() == EventLog::Type::ConferenceParticipantDeviceRemoved)) return;
	{
		shared_ptr<ConferenceParticipantDeviceEvent> deviceEvent = static_pointer_cast<
			ConferenceParticipantDeviceEvent
		>(event);
		BC_ASSERT_TRUE(deviceEvent->getChatRoomId().getPeerAddress().asString() == "sip:test-44@sip.linphone.org");
		BC_ASSERT_TRUE(deviceEvent->getParticipantAddress().asString() == "sip:test-11@sip.linphone.org");
		BC_ASSERT_TRUE(deviceEvent->getNotifyId() == 4);
		BC_ASSERT_TRUE(deviceEvent->getDeviceAddress().asString() == "sip:test-47@sip.linphone.org");
	}
}

test_t main_db_tests[] = {
	TEST_NO_TAG("Get events count", get_events_count),
	TEST_NO_TAG("Get messages count", get_messages_count),
	TEST_NO_TAG("Get unread messages count", get_unread_messages_count),
	TEST_NO_TAG("Get history", get_history),
	TEST_NO_TAG("Get conference events", get_conference_notified_events)
};

test_suite_t main_db_test_suite = {
	"MainDb", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(main_db_tests) / sizeof(main_db_tests[0]), main_db_tests
};
