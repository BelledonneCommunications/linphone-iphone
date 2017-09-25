/*
 * conference-event-tester.cpp
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

#include "linphone/core.h"
#include "private.h"
#include "liblinphone_tester.h"
#include "conference/conference-listener.h"
#include "conference/local-conference.h"
#include "conference/remote-conference.h"

using namespace LinphonePrivate;
using namespace std;

static string first_invite = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<resource-lists xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"urn:ietf:params:xml:ns:resource-lists\">"
"<list>"
" <entry uri=\"sip:alice@sip.linphone.org\"/>"
" <entry uri=\"sip:bob@sip.linphone.org\" display-name=\"Le Bricoleur\"/>"
" <entry uri=\"sip:john-doe@sip.linphone.org\"/>"
" <entry uri=\"sip:anne-onyme@sip.linphone.org\"/>"
" <entry uri=\"sip:sarah-bache@sip.linphone.org\" display-name=\"Sarah\"/>"
"</list>"
"</resource-lists>";

static string alice_addr = "sip:alice@sip.linphone.org";
static string bob_addr = "sip:bob@sip.linphone.org";
static string john_addr = "sip:john-doe@sip.linphone.org";
static string anne_addr = "sip:anne-onyme@sip.linphone.org";
static string sarah_addr = "sip:sarah-bache@sip.linphone.org";

static string bob_name = "Le Bricoleur";
static string sarah_name = "Sarah";

void first_invite_parsing() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	Address marie_identity(linphone_address_as_string_uri_only(marie->identity));
	LocalConference localConf(marie->lc, marie_identity);
	list<shared_ptr<Address>> addresses = localConf.parseResourceLists(first_invite);
	BC_ASSERT_EQUAL(addresses.size(), 5, int, "%d");
	if(addresses.size() != 5) {
		goto end;
	}
	BC_ASSERT_TRUE(addresses.front()->asStringUriOnly() == alice_addr);
	addresses.pop_front();

	BC_ASSERT_TRUE(addresses.front()->asStringUriOnly() == bob_addr);
	BC_ASSERT_TRUE(addresses.front()->getDisplayName() == bob_name);
	addresses.pop_front();

	BC_ASSERT_TRUE(addresses.front()->asStringUriOnly() == john_addr);
	addresses.pop_front();

	BC_ASSERT_TRUE(addresses.front()->asStringUriOnly() == anne_addr);
	addresses.pop_front();

	BC_ASSERT_TRUE(addresses.front()->asStringUriOnly() == sarah_addr);
	BC_ASSERT_TRUE(addresses.front()->getDisplayName() == sarah_name);
	addresses.pop_front();
	
end:
	linphone_core_manager_destroy(marie);
}

test_t conference_tests[] = {
	TEST_NO_TAG("First invite parsing", first_invite_parsing)
};

test_suite_t conference_test_suite = {
	"Conference",
	nullptr,
	nullptr,
	liblinphone_tester_before_each,
	liblinphone_tester_after_each,
	sizeof(conference_tests) / sizeof(conference_tests[0]), conference_tests
};
