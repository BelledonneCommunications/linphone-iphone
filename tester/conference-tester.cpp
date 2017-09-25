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
#include "conference/local-conference.h"
#include "conference/remote-conference.h"

using namespace LinphonePrivate;
using namespace std;

static const string firstInvite = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<resource-lists xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"urn:ietf:params:xml:ns:resource-lists\">"
"<list>"
" <entry uri=\"sip:alice@sip.linphone.org\"/>"
" <entry uri=\"sip:bob@sip.linphone.org\"><display-name>Le Bricoleur</display-name></entry>"
" <entry uri=\"sip:john-doe@sip.linphone.org\"/>"
" <entry uri=\"sip:anne-onyme@sip.linphone.org\"/>"
" <entry uri=\"sip:sarah-bache@sip.linphone.org\"><display-name>Sarah</display-name></entry>"
"</list>"
"</resource-lists>";

static const string aliceAddr = "sip:alice@sip.linphone.org";
static const string bobAddr = "sip:bob@sip.linphone.org";
static const string johnAddr = "sip:john-doe@sip.linphone.org";
static const string anneAddr = "sip:anne-onyme@sip.linphone.org";
static const string sarahAddr = "sip:sarah-bache@sip.linphone.org";

static const string bobName = "Le Bricoleur";
static const string sarahName = "Sarah";

static void check_first_invite(LinphoneCore *core, const Address &address, string invite) {
	LocalConference localConf(core, address);
	list<Address> addresses = localConf.parseResourceLists(invite);
	BC_ASSERT_EQUAL(addresses.size(), 5, int, "%d");
	if (addresses.size() != 5)
		return;
	BC_ASSERT_TRUE(addresses.front().asStringUriOnly() == aliceAddr);
	BC_ASSERT_TRUE(addresses.front().getDisplayName().empty());
	addresses.pop_front();

	BC_ASSERT_TRUE(addresses.front().asStringUriOnly() == bobAddr);
	BC_ASSERT_TRUE(addresses.front().getDisplayName() == bobName);
	addresses.pop_front();

	BC_ASSERT_TRUE(addresses.front().asStringUriOnly() == johnAddr);
	BC_ASSERT_TRUE(addresses.front().getDisplayName().empty());
	addresses.pop_front();

	BC_ASSERT_TRUE(addresses.front().asStringUriOnly() == anneAddr);
	BC_ASSERT_TRUE(addresses.front().getDisplayName().empty());
	addresses.pop_front();

	BC_ASSERT_TRUE(addresses.front().asStringUriOnly() == sarahAddr);
	BC_ASSERT_TRUE(addresses.front().getDisplayName() == sarahName);
	addresses.pop_front();
}

void first_invite_parsing () {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	const Address marieIdentity(linphone_address_as_string_uri_only(marie->identity));
	check_first_invite(marie->lc, marieIdentity, firstInvite);
	linphone_core_manager_destroy(marie);
}

void first_invite_serializing() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	const Address marieIdentity(linphone_address_as_string_uri_only(marie->identity));
	RemoteConference remoteConf(marie->lc, marieIdentity);
	list<Address> addresses = list<Address>();
	string xmlBody;

	Address aliceIdentity(aliceAddr);
	Address bobIdentity(bobAddr);
	Address johnIdentity(johnAddr);
	Address anneIdentity(anneAddr);
	Address sarahIdentity(sarahAddr);

	bobIdentity.setDisplayName(bobName);
	sarahIdentity.setDisplayName(sarahName);

	addresses.push_back(aliceIdentity);
	addresses.push_back(bobIdentity);
	addresses.push_back(johnIdentity);
	addresses.push_back(anneIdentity);
	addresses.push_back(sarahIdentity);

	xmlBody = remoteConf.getResourceLists(addresses);
	check_first_invite(marie->lc, marieIdentity, xmlBody);
	linphone_core_manager_destroy(marie);
}

test_t conference_tests[] = {
	TEST_NO_TAG("First invite parsing", first_invite_parsing),
	TEST_NO_TAG("First invite serializing", first_invite_serializing)
};

test_suite_t conference_test_suite = {
	"Conference",
	nullptr,
	nullptr,
	liblinphone_tester_before_each,
	liblinphone_tester_after_each,
	sizeof(conference_tests) / sizeof(conference_tests[0]), conference_tests
};
